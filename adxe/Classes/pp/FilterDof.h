/* Dof filter
*/
#ifndef _FILTER_DOG_H__
#define _FILTER_DOG_H__

#include "FilterBase.h"

namespace pp
{
	class RenderTexture3D;

	class FilterDof : public FilterBase
	{
	
	private:

		bool init(const cocos2d::Vec4& clip_plane, int radius);

	public:

		FilterDof(const cocos2d::Size& sz, const cocos2d::Vec4& clip_plane, int radius, bool depth_enable);

		void setDepthTexture(const cocos2d::Texture2D* depth_text);

		// Set depth coeff
		void setDepthCoeff(float depth_coeff);
	};
}

#endif
