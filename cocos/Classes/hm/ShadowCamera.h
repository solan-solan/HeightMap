#ifndef _SHADOW_CAMERA_H__
#define _SHADOW_CAMERA_H__

#include "cocos2d.h"

namespace pp
{
	class RenderTexture3D;
}

namespace hm
{
	class ShadowCamera : public cocos2d::Camera
	{

	private:

		pp::RenderTexture3D* _shdwText = nullptr;

	protected:

		bool init(float zoomX, float zoomY, float plane, int depth_text_size);

	public:
		
		virtual ~ShadowCamera();

		static ShadowCamera* create(float zoomX, float zoomY, float plane, int depth_text_size);

		pp::RenderTexture3D* getRenderText() const
		{
			return _shdwText;
		}

		float getZoomX() const
		{
			return _zoom[0];
		}

		float getZoomY() const
		{
			return _zoom[1];
		}

		// Start render to the depth texture
		void begin_render() const;

		// End render to the depth texture
		void end_render() const;

		// Reinit ortographic projection
		void reinit(float minX, float maxX, float minY, float maxY, float minZ, float maxZ);

	};
}

#endif
