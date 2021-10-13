#ifndef _RENDER_TEXTURE_3D_H__
#define _RENDER_TEXTURE_3D_H__

#include "2d/CCRenderTexture.h"

namespace BigTrip
{

	class RenderTexture3D : public cocos2d::RenderTexture
	{
		bool _is_transparent_begin = false;
		bool _is_transparent_end = true;

	public:
		
		static RenderTexture3D* create(int w, int h, cocos2d::backend::PixelFormat format, cocos2d::backend::PixelFormat depthStencilFormat, cocos2d::RenderTargetFlag flag);

		// Starts grabbing.
		virtual void begin() override;
		virtual void end() override;

		/** FIXME: should be protected.
		 * but due to a bug in PowerVR + Android,
		 * the constructor is public again.
		 * @js ctor
		 */
		RenderTexture3D();

		virtual ~RenderTexture3D();

		void beginWithClear(float depthValue);
		virtual void beginWithClear(float r, float g, float b, float a, float depthValue, int stencilValue, cocos2d::ClearFlag flags) override;

		// Return Depth texture
		cocos2d::Texture2D* getDepthText()
		{
			return _depthStencilTexture;
		}

		cocos2d::Texture2D* getTexture() const
		{
			return _texture2D;
		}

		// Choose commands tranparent to select proper queue for begin/end commands
		void setCommandsTransparent(bool is_transparent_begin, bool is_transparent_end)
		{
			_is_transparent_begin = is_transparent_begin;
			_is_transparent_end = is_transparent_end;
		}

	protected:
		bool initWithWidthAndHeight(int w, int h, cocos2d::backend::PixelFormat format, cocos2d::backend::PixelFormat depthStencilFormat, cocos2d::RenderTargetFlag flag);
	};

}

#endif
