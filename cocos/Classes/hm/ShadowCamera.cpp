#include "ShadowCamera.h"
#include "RenderTexture3D.h"

using namespace cocos2d;
using namespace hm;

ShadowCamera::~ShadowCamera()
{
	assert(_shdwText->getReferenceCount() == 1 && 
		_shdwText->getDepthText()->getReferenceCount() == 1 &&
		_shdwText->getTexture() == nullptr
	);
	_shdwText->release();
	_shdwText = nullptr;
}

ShadowCamera* ShadowCamera::create(float zoomX, float zoomY, float plane, int depth_text_size)
{
	auto ret = new (std::nothrow) ShadowCamera();
	if (ret)
	{
		ret->init(zoomX, zoomY, plane, depth_text_size);
		ret->autorelease();
		return ret;
	}
	CC_SAFE_DELETE(ret);
	return nullptr;
}

bool ShadowCamera::init(float zoomX, float zoomY, float plane, int depth_text_size)
{
	_zoom[0] = zoomX;
	_zoom[1] = zoomY;
	_nearPlane = 0.1f;// -plane;
	_farPlane = plane;
	cocos2d::Mat4::createOrthographicOffCenter(-_zoom[0], _zoom[0], -_zoom[1], _zoom[1], _nearPlane, _farPlane, &_projection);
	_viewProjectionDirty = true;
	_frustumDirty = true;

	// Create 3D Render Texture for this camera
	//#if CC_TARGET_PLATFORM == CC_PLATFORM_WIN32 || CC_TARGET_PLATFORM == CC_PLATFORM_LINUX		
	//		_shdwText = RenderTexture3D::create(1024.0, 1024.0, backend::PixelFormat::RGBA8888, backend::PixelFormat::D24S8, RenderTargetFlag::DEPTH | RenderTargetFlag::COLOR);
	//#else
	_shdwText = RenderTexture3D::create(depth_text_size, depth_text_size, backend::PixelFormat::RGBA8888, backend::PixelFormat::D24S8, RenderTargetFlag::DEPTH);
	//#endif
	_shdwText->retain();

	return true;
}

void ShadowCamera::reinit(float minX, float maxX, float minY, float maxY, float minZ, float maxZ)
{
	_zoom[0] = maxX;
	_zoom[1] = maxY;
	_nearPlane = minZ;
	_farPlane = maxZ;
	cocos2d::Mat4::createOrthographicOffCenter(minX, maxX, minY, maxY, minZ, maxZ, &_projection);
	_viewProjectionDirty = true;
	_frustumDirty = true;
}

void ShadowCamera::begin_render() const
{
	_shdwText->beginWithClear(1.0);
}

void ShadowCamera::end_render() const
{
	_shdwText->end();
}
