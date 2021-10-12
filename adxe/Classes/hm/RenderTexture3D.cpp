
#include "cocos2d.h"
#include "RenderTexture3D.h"
#include "renderer/backend/CommandBuffer.h"
#include "renderer/backend/Device.h"

using namespace cocos2d;
using namespace hm;

RenderTexture3D::RenderTexture3D()
{
}

RenderTexture3D* RenderTexture3D::create(int w ,int h, backend::PixelFormat eFormat, PixelFormat uDepthStencilFormat, cocos2d::RenderTargetFlag flag)
{
    RenderTexture3D *ret = new (std::nothrow) RenderTexture3D();

    if(ret && ret->initWithWidthAndHeight(w, h, eFormat, uDepthStencilFormat, flag))
    {
		ret->setKeepMatrix(true);
        ret->autorelease();
        return ret;
    }

    CC_SAFE_DELETE(ret);
    return nullptr;
}

void RenderTexture3D::begin()
{
    Director* director = Director::getInstance();
    CCASSERT(nullptr != director, "Director is null when setting matrix stack");
    
    director->pushMatrix(MATRIX_STACK_TYPE::MATRIX_STACK_PROJECTION);
    _projectionMatrix = director->getMatrix(MATRIX_STACK_TYPE::MATRIX_STACK_PROJECTION);
    
    director->pushMatrix(MATRIX_STACK_TYPE::MATRIX_STACK_MODELVIEW);
    _transformMatrix = director->getMatrix(MATRIX_STACK_TYPE::MATRIX_STACK_MODELVIEW);
    
    if(!_keepMatrix)
    {
        director->setProjection(director->getProjection());
        
        const Size& texSize = _texture2D->getContentSizeInPixels();
        
        // Calculate the adjustment ratios based on the old and new projections
        Size size = director->getWinSizeInPixels();
        
        float widthRatio = size.width / texSize.width;
        float heightRatio = size.height / texSize.height;
        
        Mat4 orthoMatrix;
        Mat4::createOrthographicOffCenter((float)-1.0 / widthRatio, (float)1.0 / widthRatio, (float)-1.0 / heightRatio, (float)1.0 / heightRatio, -1, 1, &orthoMatrix);
        director->multiplyMatrix(MATRIX_STACK_TYPE::MATRIX_STACK_PROJECTION, orthoMatrix);
    }

    _groupCommand.init(-1);

    Renderer *renderer =  Director::getInstance()->getRenderer();
    renderer->addCommand(&_groupCommand);
    renderer->pushGroup(_groupCommand.getRenderQueueID());

	_beginCommand.init(_globalZOrder, _transformMatrix, Node::FLAGS_RENDER_AS_3D);
	_beginCommand.setTransparent(_is_transparent_begin);
    _beginCommand.func = CC_CALLBACK_0(RenderTexture3D::onBegin, this);
    renderer->addCommand(&_beginCommand);
}

void RenderTexture3D::end()
{
    _endCommand.init(_globalZOrder);
	_endCommand.set3D(true);

	//----------- It is needed to provide this command to be last in the transparent queue 
	class CallbackCommandExt : public CallbackCommand
	{
		friend RenderTexture3D;
	};
	static_cast<CallbackCommandExt*>(&_endCommand)->_depth = std::numeric_limits<float>::lowest();
	//-----------

	_endCommand.setTransparent(_is_transparent_end);
    _endCommand.func = CC_CALLBACK_0(RenderTexture3D::onEnd, this);

    Director* director = Director::getInstance();
    CCASSERT(nullptr != director, "Director is null when setting matrix stack");
    
    Renderer *renderer = director->getRenderer();
    renderer->addCommand(&_endCommand);
    renderer->popGroup();

    director->popMatrix(MATRIX_STACK_TYPE::MATRIX_STACK_PROJECTION);
    director->popMatrix(MATRIX_STACK_TYPE::MATRIX_STACK_MODELVIEW);
}

void RenderTexture3D::beginWithClear(float depthValue)
{
	beginWithClear(0.f, 0.f, 0.f, 0.f, depthValue, 0, ClearFlag::DEPTH);
}

//============================================

bool RenderTexture3D::initWithWidthAndHeight(int w, int h, cocos2d::backend::PixelFormat format, cocos2d::backend::PixelFormat depthStencilFormat, cocos2d::RenderTargetFlag flag)
{
	CCASSERT(format != backend::PixelFormat::A8, "only RGB and RGBA formats are valid for a render texture");

	bool ret = false;
	do
	{
		_renderTargetFlags = flag;

		_fullRect = _rtTextureRect = Rect(0, 0, w, h);
		w = (int)(w * CC_CONTENT_SCALE_FACTOR());
		h = (int)(h * CC_CONTENT_SCALE_FACTOR());
		_fullviewPort = Rect(0, 0, w, h);

		// textures must be power of two squared
		int powW = 0;
		int powH = 0;

		if (Configuration::getInstance()->supportsNPOT())
		{
			powW = w;
			powH = h;
		}
		else
		{
			powW = ccNextPOT(w);
			powH = ccNextPOT(h);
		}

		backend::TextureDescriptor descriptor;
		descriptor.width = powW;
		descriptor.height = powH;
		descriptor.textureUsage = TextureUsage::RENDER_TARGET;
		descriptor.textureFormat = PixelFormat::RGBA8888;

		if (/*_renderTargetFlags & RenderTargetFlag::COLOR*/bitmask::any(_renderTargetFlags, RenderTargetFlag::COLOR))
		{
//			auto texture = backend::Device::getInstance()->newTexture(descriptor);
//			if (!texture)
//				break;

			_texture2D = new (std::nothrow) Texture2D();
			if (_texture2D)
			{
//				_texture2D->initWithBackendTexture(texture, CC_ENABLE_PREMULTIPLIED_ALPHA != 0);
				_texture2D->updateTextureDescriptor(descriptor, CC_ENABLE_PREMULTIPLIED_ALPHA != 0);
//				_texture2D->setRenderTarget(true);
//				texture->release();
			}
			else
				break;

			//	_renderTargetFlags = RenderTargetFlag::COLOR;

//			clearColorAttachment();

			_texture2D->setAntiAliasTexParameters();
			if (_texture2DCopy)
			{
				_texture2DCopy->setAntiAliasTexParameters();
			}

			// retained
//			setSprite(Sprite::createWithTexture(_texture2D));

//			_texture2D->release();

#if defined(CC_USE_GL) || defined(CC_USE_GLES)
//			_sprite->setFlippedY(true);
#endif

#if CC_ENABLE_PREMULTIPLIED_ALPHA != 0
//			_sprite->setBlendFunc(BlendFunc::ALPHA_PREMULTIPLIED);
//			_sprite->setOpacityModifyRGB(true);
#else
			_sprite->setBlendFunc(BlendFunc::ALPHA_NON_PREMULTIPLIED);
			_sprite->setOpacityModifyRGB(false);
#endif

			// add sprite for backward compatibility
//			addChild(_sprite);

			// TEST
//			_sprite->setBlendFunc(BlendFunc::DISABLE);
//			_sprite->setPositionZ(-50.f);
			// END TEST
		}

		if (bitmask::any(_renderTargetFlags, (RenderTargetFlag::DEPTH | RenderTargetFlag::STENCIL)) && PixelFormat::D24S8 == depthStencilFormat)
		{
			//	_renderTargetFlags = RenderTargetFlag::ALL;
			descriptor.textureFormat = depthStencilFormat;
//			auto texture = backend::Device::getInstance()->newTexture(descriptor);
//			if (!texture)
//				break;

			_depthStencilTexture = new (std::nothrow) Texture2D;
			if (!_depthStencilTexture)
			{
				assert(0);
//				texture->release();
				break;
			}

			_depthStencilTexture->updateTextureDescriptor(descriptor);
//			_texture2D->setRenderTarget(false);
//			texture->release();
		}

		_renderTarget = backend::Device::getInstance()->newRenderTarget(_renderTargetFlags,
			_texture2D ? _texture2D->getBackendTexture() : nullptr,
			_depthStencilTexture ? _depthStencilTexture->getBackendTexture() : nullptr,
			_depthStencilTexture ? _depthStencilTexture->getBackendTexture() : nullptr
		);

//		clearColorAttachment();

//		_texture2D->setAntiAliasTexParameters();
//		if (_texture2DCopy)
//		{
//			_texture2DCopy->setAntiAliasTexParameters();
//		}

		// Disabled by default.
		_autoDraw = false;

		ret = true;
	} while (0);

	return ret;
}

void RenderTexture3D::beginWithClear(float r, float g, float b, float a, float depthValue, int stencilValue, ClearFlag flags)
{
	setClearColor(Color4F(r, g, b, a));
	setClearDepth(depthValue);
	setClearStencil(stencilValue);
	setClearFlags(flags);
	begin();
	/*	CallbackCommand* command = new CallbackCommand();
		command->init(_globalZOrder);
		command->set3D(true);
		command->setTransparent(false);
		command->func = [=]() -> void {
			backend::RenderPassDescriptor descriptor;
			descriptor.needColorAttachment = false;

			if (flags & ClearFlag::COLOR)
			{
				_clearColor = Color4F(r, g, b, a);
				descriptor.clearColorValue = { r, g, b, a };
				descriptor.needClearColor = true;
				descriptor.needColorAttachment = true;
				descriptor.colorAttachmentsTexture[0] = _texture2D->getBackendTexture();
			}
			if (flags & ClearFlag::DEPTH)
			{
				descriptor.clearDepthValue = depthValue;
				descriptor.needClearDepth = true;
				descriptor.depthTestEnabled = true;
				descriptor.depthAttachmentTexture = _depthStencilTexture->getBackendTexture();
			}
			if (flags & ClearFlag::STENCIL)
			{
				descriptor.clearStencilValue = stencilValue;
				descriptor.needClearStencil = true;
				descriptor.stencilTestEnabled = true;
				descriptor.stencilAttachmentTexture = _depthStencilTexture->getBackendTexture();
			}*/

	class ren : public Renderer
	{
	public:

		void clear(ClearFlag flags, const Color4F& color, float depth, unsigned int stencil, float globalOrder)
		{
			_clearFlag = flags;

			CallbackCommand* command = nextClearCommand();
			command->init(globalOrder);
			command->set3D(true);
			command->setTransparent(false);
			command->func = [=]() -> void {
				backend::RenderPassDescriptor descriptor;

				descriptor.flags.clear = flags;
				if (bitmask::any(flags, ClearFlag::COLOR)) {
					_clearColor = color;
					descriptor.clearColorValue = { color.r, color.g, color.b, color.a };
				}

				if (bitmask::any(flags, ClearFlag::DEPTH))
					descriptor.clearDepthValue = depth;

				if (bitmask::any(flags, ClearFlag::STENCIL))
					descriptor.clearStencilValue = stencil;

				_commandBuffer->beginRenderPass(_currentRT, descriptor);
				_commandBuffer->endRenderPass();

				// push to pool for reuse
				_clearCommandsPool.push_back(command);
			};
			addCommand(command);
		}
	} *render = static_cast<ren*>(Director::getInstance()->getRenderer());

	render->clear(_clearFlags, _clearColor, _clearDepth, _clearStencil, _globalZOrder);
	//		r->beginRenderPass(descriptor);
	//		r->endRenderPass();

	/*		delete command;
		};
		Director::getInstance()->getRenderer()->addCommand(command);*/
}

RenderTexture3D::~RenderTexture3D()
{
	//	assert(_texture2D->getReferenceCount() == 2 && _texture2D->getBackendTexture()->getReferenceCount() == 1);
	//	assert(!_depthStencilTexture || (_depthStencilTexture->getReferenceCount() == 1 && _depthStencilTexture->getBackendTexture()->getReferenceCount() == 1));

	assert(!_texture2D || (_texture2D->getReferenceCount() == 2 && _texture2D->getBackendTexture()->getReferenceCount() == 2));
//	assert(!_depthStencilTexture || (_depthStencilTexture->getReferenceCount() == 1 && _depthStencilTexture->getBackendTexture()->getReferenceCount() == 3));

	CC_SAFE_RELEASE(_texture2D);
}

/*void RenderTexture3D::visit(Renderer* renderer, const Mat4& parentTransform, uint32_t parentFlags)
{
	// override visit.
	// Don't call visit on its children
	if (!_visible)
	{
		return;
	}

	uint32_t flags = processParentFlags(parentTransform, parentFlags);

	Director* director = Director::getInstance();
	// IMPORTANT:
	// To ease the migration to v3.0, we still support the Mat4 stack,
	// but it is deprecated and your code should not rely on it
	director->pushMatrix(MATRIX_STACK_TYPE::MATRIX_STACK_MODELVIEW);
	director->loadMatrix(MATRIX_STACK_TYPE::MATRIX_STACK_MODELVIEW, _modelViewTransform);

	_sprite->visit(renderer, _modelViewTransform, flags | FLAGS_RENDER_AS_3D);
	if (isVisitableByVisitingCamera())
	{
		draw(renderer, _modelViewTransform, flags);
	}

	director->popMatrix(MATRIX_STACK_TYPE::MATRIX_STACK_MODELVIEW);

	// FIX ME: Why need to set _orderOfArrival to 0??
	// Please refer to https://github.com/cocos2d/cocos2d-x/pull/6920
	// setOrderOfArrival(0);
}
*/