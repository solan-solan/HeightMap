/* Sprite to represent Filter
*/
#ifndef _RTSprite_H__
#define _RTSprite_H__

#include "2d/CCSprite.h"
#include <vector>

namespace pp
{
	class RenderTexture3D;
	class FilterBase;

	class RTSprite : public cocos2d::Sprite
	{

	public:

		struct VERTEX
		{
			cocos2d::Vec3 pos;
			cocos2d::Vec2 coord;
			VERTEX(const cocos2d::Vec3& p, const cocos2d::Vec2& c) : pos(p), coord(c)
			{
			}
		};

	protected:

		FilterBase* _filter = nullptr;

		std::vector<VERTEX> _model;
		std::vector<unsigned short> _index;

		cocos2d::CustomCommand _customCommand;

		bool _is_top = false;

	public:

		static RTSprite* create(FilterBase* filter, const cocos2d::Size& sz);

		virtual ~RTSprite();

		virtual bool init() override;

		virtual void draw(cocos2d::Renderer* renderer, const cocos2d::Mat4& transform, uint32_t flags) override;

		// Begin render to the filter
		virtual void begin();

		// End render to the filter
		virtual void end();

		// Set top
		void setTop(bool top)
		{
			_is_top = top;
		}

		// Check if the sprite is top
		bool isTop() const 
		{
			return _is_top;
		}

		virtual FilterBase* getFilter() const
		{
			return _filter;
		}

		virtual void reset()
		{
		}

	};
}

#endif
