#include "RTSprite.h"
#include "RenderTexture3D.h"
#include "FilterBase.h"

using namespace cocos2d;
using namespace pp;

RTSprite::~RTSprite()
{
	if (_filter)
		delete _filter;
}

RTSprite* RTSprite::create(FilterBase* filter, const cocos2d::Size& sz)
{
	auto s = new RTSprite();
	s->_contentSize = sz;
	if (s)
	{
		s->_filter = filter;
		if (s->init())
		{
			s->autorelease();
//			filter->getRenderTexture()->setSprite(s); // RenderTexture3D will retain the sprite
			return s;
		}
	}
	CC_SAFE_RELEASE(s);
	return nullptr;
}

bool RTSprite::init()
{
	auto render_texture = _filter->getRenderTexture();
	if (render_texture)
	{
		_texture = render_texture->getTexture();
		_texture->retain();
	}

	//--------------------------- Create base model

	_model.emplace_back(Vec3{ 0.0, 0.0, 0.0 }, Vec2{ 0.0, 0.0 });
	_model.emplace_back(Vec3{ 0.0, _contentSize.height, 0.0 }, Vec2{ 0.0, 1.0 });
	_model.emplace_back(Vec3{ _contentSize.width, _contentSize.height, 0.0 }, Vec2{ 1.0, 1.0 });
	_model.emplace_back(Vec3{ _contentSize.width, 0.0, 0.0 }, Vec2{ 1.0, 0.0 });

	_index.emplace_back(0);
	_index.emplace_back(2);
	_index.emplace_back(1);
	_index.emplace_back(0);
	_index.emplace_back(3);
	_index.emplace_back(2);

	//---------------------------- Create buffers

	_customCommand.createVertexBuffer(sizeof(VERTEX), _model.size(), CustomCommand::BufferUsage::DYNAMIC);
	_customCommand.createIndexBuffer(CustomCommand::IndexFormat::U_SHORT, _index.size(), CustomCommand::BufferUsage::STATIC);

	_customCommand.setTransparent(true);
	_customCommand.set3D(true);

	_customCommand.updateVertexBuffer(_model.data(), _model.size() * sizeof(VERTEX));
	_customCommand.updateIndexBuffer(_index.data(), _index.size() * sizeof(unsigned short));

	return true;
}

void RTSprite::draw(cocos2d::Renderer* renderer, const cocos2d::Mat4& transform, uint32_t flags)
{
	_filter->draw(_customCommand, renderer, getNodeToWorldTransform(), flags);
}

void RTSprite::begin()
{
	_filter->begin();
}

void RTSprite::end()
{
	_filter->end();
}
