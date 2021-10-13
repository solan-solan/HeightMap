
#include "cocos2d.h"
#include "RenderTexture3D.h"
#include "renderer/backend/CommandBuffer.h"
#include "renderer/backend/Device.h"

using namespace cocos2d;
using namespace BigTrip;

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
//		w = (int)(w * CC_CONTENT_SCALE_FACTOR());
//		h = (int)(h * CC_CONTENT_SCALE_FACTOR());
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

		if (bitmask::any(_renderTargetFlags, RenderTargetFlag::COLOR))
		{
			_texture2D = new (std::nothrow) Texture2D();
			if (_texture2D)
			{
				_texture2D->updateTextureDescriptor(descriptor, CC_ENABLE_PREMULTIPLIED_ALPHA != 0);
			}
			else
				break;

			_texture2D->setAntiAliasTexParameters();
			if (_texture2DCopy)
			{
				_texture2DCopy->setAntiAliasTexParameters();
			}
		}

		if (bitmask::any(_renderTargetFlags, (RenderTargetFlag::DEPTH | RenderTargetFlag::STENCIL)) && PixelFormat::D24S8 == depthStencilFormat)
		{
			descriptor.textureFormat = depthStencilFormat;

			_depthStencilTexture = new (std::nothrow) Texture2D;
			if (!_depthStencilTexture)
			{
				assert(0);
				break;
			}

			_depthStencilTexture->updateTextureDescriptor(descriptor);
		}

		_renderTarget = backend::Device::getInstance()->newRenderTarget(_renderTargetFlags,
			_texture2D ? _texture2D->getBackendTexture() : nullptr,
			_depthStencilTexture ? _depthStencilTexture->getBackendTexture() : nullptr,
			_depthStencilTexture ? _depthStencilTexture->getBackendTexture() : nullptr
		);

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
}

RenderTexture3D::~RenderTexture3D()
{
	assert(!_texture2D || (_texture2D->getReferenceCount() == 2 && _texture2D->getBackendTexture()->getReferenceCount() == 2));
	CC_SAFE_RELEASE(_texture2D);
}