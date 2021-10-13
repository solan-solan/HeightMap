
#include "cocos2d.h"
#include "RenderTexture3D.h"
#include "renderer/backend/CommandBuffer.h"
#include "renderer/backend/Device.h"

using namespace cocos2d;
using namespace hm;

RenderTexture3D::RenderTexture3D()
{
}

RenderTexture3D::~RenderTexture3D()
{
	assert(!_texture2D || (_texture2D->getReferenceCount() == 2 && _texture2D->getBackendTexture()->getReferenceCount() == 1));
	assert(!_depthStencilTexture || (_depthStencilTexture->getReferenceCount() == 1 /*&& _depthStencilTexture->getBackendTexture()->getReferenceCount() == 1*/));
	CC_SAFE_RELEASE(_texture2D);
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

void RenderTexture3D::beginWithClear(float r, float g, float b, float a, float depthValue, int stencilValue, ClearFlag flags)
{
    setClearColor(Color4F(r, g, b, a));
    setClearDepth(depthValue);
    setClearStencil(stencilValue);
    setClearFlags(flags);
    begin();
    
	CallbackCommand* command = new CallbackCommand();
	command->init(_globalZOrder, _transformMatrix, Node::FLAGS_RENDER_AS_3D);
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
		}

		class ren : public Renderer
		{
		public:

			void beginRenderPass(const backend::RenderPassDescriptor& descriptor)
			{
				_commandBuffer->beginRenderPass(descriptor);
			}

			void endRenderPass()
			{
				_commandBuffer->endRenderPass();
			}
		} *r = static_cast<ren*>(Director::getInstance()->getRenderer());
		
		r->beginRenderPass(descriptor);
		r->endRenderPass();

		delete command;
	};
	Director::getInstance()->getRenderer()->addCommand(command);
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

bool RenderTexture3D::initWithWidthAndHeight(int w, int h, cocos2d::backend::PixelFormat format, cocos2d::backend::PixelFormat depthStencilFormat, cocos2d::RenderTargetFlag flag)
{
	CCASSERT(format != backend::PixelFormat::A8, "only RGB and RGBA formats are valid for a render texture");

	bool ret = false;
	do
	{
		_renderTargetFlags = flag;

		_fullRect = _rtTextureRect = Rect(0, 0, w, h);
		//w = (int)(w * CC_CONTENT_SCALE_FACTOR());
		//h = (int)(h * CC_CONTENT_SCALE_FACTOR());
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

		if (_renderTargetFlags & RenderTargetFlag::COLOR)
		{
			auto texture = backend::Device::getInstance()->newTexture(descriptor);
			if (!texture)
				break;

			_texture2D = new (std::nothrow) Texture2D();
			if (_texture2D)
			{
				_texture2D->initWithBackendTexture(texture, CC_ENABLE_PREMULTIPLIED_ALPHA != 0);
				_texture2D->setRenderTarget(true);
				texture->release();
			}
			else
				break;

			_texture2D->setAntiAliasTexParameters();
			if (_texture2DCopy)
			{
				_texture2DCopy->setAntiAliasTexParameters();
			}
		}

		if ((_renderTargetFlags & (RenderTargetFlag::DEPTH | RenderTargetFlag::STENCIL)) && PixelFormat::D24S8 == depthStencilFormat)
		{
			descriptor.textureFormat = depthStencilFormat;
			auto texture = backend::Device::getInstance()->newTexture(descriptor);
			if (!texture)
				break;

			_depthStencilTexture = new (std::nothrow) Texture2D;
			if (!_depthStencilTexture)
			{
				texture->release();
				break;
			}

			_depthStencilTexture->initWithBackendTexture(texture);
			texture->release();
		}

		// Disabled by default.
		_autoDraw = false;

		ret = true;
	} while (0);

	return ret;
}
