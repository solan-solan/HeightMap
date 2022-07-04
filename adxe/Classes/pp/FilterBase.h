/* Filter base class
*/
#ifndef _FILTER_BASE_H__
#define _FILTER_BASE_H__

#include "cocos2d.h"

namespace pp
{
	class RenderTexture3D;

	class FilterBase
	{
	
	protected:

		bool _depth_enable = false;

		cocos2d::backend::UniformLocation _vpLoc;

		RenderTexture3D* _render_texture = nullptr;

		cocos2d::backend::ProgramState* _programState = nullptr;

	protected:

		void initLayout();

	public:

		FilterBase(const cocos2d::Size& sz, bool depth_enable);

		virtual void init();

		virtual ~FilterBase();

		virtual void draw(cocos2d::CustomCommand& cmd, cocos2d::Renderer* renderer, const cocos2d::Mat4& transform, uint32_t flags);

		// Begin render to the filter
		virtual void begin();

		// End render to the filter
		virtual void end();

		RenderTexture3D* getRenderTexture() const
		{
			return _render_texture;
		}
	};
}

#endif
