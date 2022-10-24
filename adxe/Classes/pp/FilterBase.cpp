#include "FilterBase.h"
#include "RenderTexture3D.h"
#include "RTSprite.h"
#include "renderer/backend/Device.h"

using namespace cocos2d;
using namespace pp;

FilterBase::FilterBase(const Size& sz, bool depth_enable) : _depth_enable(depth_enable)
{
	RenderTargetFlag flag = RenderTargetFlag::COLOR;
	if (_depth_enable)
		flag |= RenderTargetFlag::DEPTH;
	if (sz.width)
	{
		_render_texture = RenderTexture3D::create(sz.width, sz.height, backend::PixelFormat::RGBA8, backend::PixelFormat::D24S8, flag);
		_render_texture->retain();
	}
}

FilterBase::~FilterBase()
{
	if (_programState)
	{
		assert(_programState->getReferenceCount() == 1);
		_programState->release();
	}

	if (_render_texture)
	{
		assert(_render_texture->getReferenceCount() == 1);
		_render_texture->release();
	}
}

void FilterBase::initLayout()
{
	///a_position
	_programState->setVertexAttrib(backend::ATTRIBUTE_NAME_POSITION,
		_programState->getAttributeLocation(backend::Attribute::POSITION),
		backend::VertexFormat::FLOAT3,
		offsetof(RTSprite::VERTEX, pos),
		false);

	///a_texCoord
	_programState->setVertexAttrib(backend::ATTRIBUTE_NAME_TEXCOORD,
		_programState->getAttributeLocation(backend::Attribute::TEXCOORD),
		backend::VertexFormat::FLOAT2,
		offsetof(RTSprite::VERTEX, coord),
		false);

	_programState->setVertexStride(sizeof(RTSprite::VERTEX));

	_vpLoc = _programState->getUniformLocation("u_MVPMatrix");
}

void FilterBase::init()
{
	std::string vert_sh = FileUtils::getInstance()->getStringFromFile("res/shaders/sprite_render.vert");
	std::string fr_sh = FileUtils::getInstance()->getStringFromFile("res/shaders/render_scene.frag");

	auto program = backend::Device::getInstance()->newProgram(vert_sh, fr_sh);
	_programState = new backend::ProgramState(program);

	initLayout();

	assert(_render_texture);

	_programState->setTexture(_programState->getUniformLocation("u_texture"), 0, _render_texture->getTexture()->getBackendTexture());
}

void FilterBase::draw(CustomCommand& cmd, Renderer* renderer, const Mat4& transform, uint32_t flags)
{
	auto& pipelineDescriptor = cmd.getPipelineDescriptor();
	pipelineDescriptor.programState = _programState;

	cmd.init(0);
	renderer->addCommand(&cmd);

	auto camera = Camera::getVisitingCamera();
	
	// Set ViewProjection matrix
	const Mat4& view_proj_mat = camera->getViewProjectionMatrix() * transform;
	_programState->setUniform(_vpLoc, view_proj_mat.m, sizeof(view_proj_mat.m));
}

void FilterBase::begin()
{
	if (_depth_enable)
	{
//		_render_texture->beginWithClear(1.f);
		_render_texture->beginWithClear(0.f, 0.f, 0.f, 0.f, 1.f, 0, ClearFlag::ALL);
		return;
	}
	_render_texture->begin();
}

void FilterBase::end()
{
	_render_texture->end();
}
