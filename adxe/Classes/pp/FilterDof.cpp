#include "FilterDof.h"
#include "RenderTexture3D.h"
#include "cocos2d.h"
#include "renderer/backend/DriverBase.h"

using namespace cocos2d;
using namespace pp;

FilterDof::FilterDof(const cocos2d::Size& sz, const cocos2d::Vec4& clip_plane, int radius, bool depth_enable) : FilterBase(sz, depth_enable)
{
	init(clip_plane, radius);
}

bool FilterDof::init(const cocos2d::Vec4& clip_plane, int radius)
{
	std::string vert_sh = FileUtils::getInstance()->getStringFromFile("res/shaders/sprite_render.vert");
	std::string fr_sh = FileUtils::getInstance()->getStringFromFile("res/shaders/dof.frag");

	auto program = backend::DriverBase::getInstance()->newProgram(vert_sh, fr_sh);
	_programState = new backend::ProgramState(program);

	initLayout();

	_programState->setTexture(_programState->getUniformLocation("u_texture"), 0, _render_texture->getTexture()->getBackendTexture());
	if (_depth_enable)
	    _programState->setTexture(_programState->getUniformLocation("u_texture_depth"), 1, _render_texture->getDepthText()->getBackendTexture());
	_programState->setUniform(_programState->getUniformLocation("u_texel_clipplane"), &clip_plane, sizeof(Vec4));
	_programState->setUniform(_programState->getUniformLocation("u_blur_radius"), &radius, sizeof(int));

	return true;
}

void FilterDof::setDepthTexture(const cocos2d::Texture2D* depth_text)
{
	_programState->setTexture(_programState->getUniformLocation("u_texture_depth"), 1, depth_text->getBackendTexture());
}

void FilterDof::setDepthCoeff(float depth_coeff)
{
	_programState->setUniform(_programState->getUniformLocation("u_coef_dist"), &depth_coeff, sizeof(depth_coeff));
}
