#include "LodHM.h"
#include "HeightMap.h"
#include "renderer/backend/Device.h"

using namespace hm;
using namespace cocos2d;

const Vec3 LodHM::_lod_color[7] = {
	Vec3(0.f, 0.f, 1.f),  // BLUE
	Vec3(0.f, 1.f, 0.f),  // GREEN
	Vec3(1.f, 0.f, 0.f),  // RED
	Vec3(1.f, 0.f, 1.f),  // MAGENTA
	Vec3(1.f, 1.f, 0.f),  // YELLOW
	Vec3(1.f, 0.5f, 0.f), // ORANGE
	Vec3(1.f, 1.f, 1.f)   // WHITE
};

#define LOD_ALPHA_FROM_RATE 4
#define LOD_ALPHA_TO_RATE 8

std::atomic_char LodHM::_force_level_update = {0};

LodHM::LodHM(HeightMap* hM, int lod_count)
{
	assert(lod_count > 0);

	// Store variables
	_hM = hM;

	if (_hM->_prop._lod_data.size() > lod_count - 1)
	{// Store lod data if it is presented in the level file
		_w = _hM->_prop._lod_data.at(lod_count - 1).width;
		_h = _hM->_prop._lod_data.at(lod_count - 1).height;
	}
	else
	{
		_w = std::pow(2.0, lod_count + _hM->_prop._lod_factor_w) + 1;

		if (_w < MIN_LOD_W)
			_w = MIN_LOD_W;

		_h = std::pow(2.0, lod_count + _hM->_prop._lod_factor_h) + 1;

		if (_h < MIN_LOD_H)
			_h = MIN_LOD_H;
	}

	_helper._half_w = _w / 2;
	_helper._half_h = _h / 2;

	// Store lod number
	_lod_num = lod_count;

	int mult_step = (_lod_num + _hM->_prop._lod_init) * _hM->_prop._lod_rank;
	int mult_center_step = (_lod_num + _hM->_prop._lod_init + 1) * _hM->_prop._lod_rank;
	_helper._level_mult = std::pow(2, mult_step);
	_helper._level_mult_center = std::pow(2, mult_center_step);

	// Create array for verticies and opengl buffers
	createVertexArray();

	// Create buffers for the drawing landscape as wire only for debug
#if defined(COCOS2D_DEBUG) && (COCOS2D_DEBUG > 0)
	createLineBuffers();
	createNormBuffers();
#endif

	// Create shader for this Level of LodHM
	createShader();

#if defined(COCOS2D_DEBUG) && (COCOS2D_DEBUG > 0)
	// Create grid shader for this
	createShaderGrid();
	createShaderNorm();
#endif
	
	// If not first lod
	if (_lod_num > 1)
	{
		_next = new LodHM(hM, _lod_num - 1);
	}
}

LodHM::~LodHM()
{
	for (auto& ld : _layer_draw)
	    CC_SAFE_RELEASE_NULL(ld._programState);
	CC_SAFE_RELEASE_NULL(_programStateGrid);
	CC_SAFE_RELEASE_NULL(_programStateNorm);

	// Delete next lod level
	delete _next;
}

unsigned char LodHM::updateGLbuffer()
{
	//	Lod::LOD_STATE ls;
	unsigned char ret = 0;

	if (_next)
		ret |= _next->updateGLbuffer();

	if ((ret&~1) & (1 << (_lod_num - 1)))
	{
		// Set Not draw quad
		float prev_mult = _hM->_prop._scale.x * std::pow(2, _lod_num - 2);
		float mult = (LOD_ALPHA_TO_RATE + 2) * prev_mult;
		Vec4 n_draw = Vec4(_nextLayer.xs + mult, _nextLayer.xe - mult, _nextLayer.zs + mult, _nextLayer.ze - mult);
		for (int q = 0; q < _layer_count; ++q)
		    _layer_draw.at(q)._programState->setUniform(_allLodLoc._ndraw, &n_draw, sizeof(n_draw));
	}

	if (_lod_state == NONE)
		return ret;

	_layer_draw.at(0)._customCommand.updateVertexBuffer(lVert.data(), 0, lVert.size() * sizeof(ONEVERTEX));

#if defined(COCOS2D_DEBUG) && (COCOS2D_DEBUG > 0)
	if (_show_grid)
	{
		if (_lod_num > 1)
		{
			// Set Not draw quad
			float prev_mult = _hM->_prop._scale.x * std::pow(2, _lod_num - 2);
			float mult = (LOD_ALPHA_TO_RATE + 2) * prev_mult;
			Vec4 n_draw = Vec4(_nextLayer.xs + mult, _nextLayer.xe - mult, _nextLayer.zs + mult, _nextLayer.ze - mult);
			_programStateGrid->setUniform(_allLodLocGrid._ndraw, &n_draw, sizeof(n_draw));
		}
		_customCommandGrid.updateVertexBuffer(lVert.data(), 0, lVert.size() * sizeof(ONEVERTEX));
	}

	if (_norm_size)
	{
		if (_lod_num > 1)
		{
			// Set Not draw quad
			float prev_mult = _hM->_prop._scale.x * std::pow(2, _lod_num - 2);
			float mult = (LOD_ALPHA_TO_RATE + 2) * prev_mult;
			Vec4 n_draw = Vec4(_nextLayer.xs + mult, _nextLayer.xe - mult, _nextLayer.zs + mult, _nextLayer.ze - mult);
			_programStateNorm->setUniform(_allLodLocNorm._ndraw, &n_draw, sizeof(n_draw));
		}
		updateNormBuffers();
	}
#endif

	// Set the lod was updated
	ret |= (1 << _lod_num);

	_lod_state = NONE;

	return ret;
}

void LodHM::createVertexArray()
{
	// Reserve memory space
	lVert.reserve(_w * _h);

	// Fill vertex array
	for (int i = 0; i < _w * _h; ++i)
		lVert.push_back(ONEVERTEX());

	// Fill indicies array
	for (unsigned int j = 0; j < _h - 1; ++j) // Lines
		for (unsigned int i = 0; i < _w - 1; ++i) // Columns
		{
			// First tris
			iVert.push_back(i + j * _w);
			iVert.push_back(i + j * _w + _w);
			iVert.push_back(i + j * _w + _w + 1);

			// Second tris
			iVert.push_back(i + j * _w);
			iVert.push_back(i + j * _w + _w + 1);
			iVert.push_back(i + j * _w + 1);
		}

	_iVertCount = iVert.size();

	_layer_draw.at(0)._customCommand.createVertexBuffer(sizeof(ONEVERTEX), lVert.size(), CustomCommand::BufferUsage::DYNAMIC);
	_layer_draw.at(0)._customCommand.createIndexBuffer((CustomCommand::IndexFormat)cocos_index_format, iVert.size(), CustomCommand::BufferUsage::STATIC);

	_layer_draw.at(0)._customCommand.updateVertexBuffer(&lVert[0], lVert.size() * sizeof(ONEVERTEX));
	_layer_draw.at(0)._customCommand.updateIndexBuffer(&iVert[0], iVert.size() * sizeof(INDEX_TYPE));

	_layer_draw.at(0)._customCommand.setTransparent(false);
	_layer_draw.at(0)._customCommand.set3D(true);
	_layer_draw.at(0)._customCommand.setBeforeCallback(CC_CALLBACK_0(LodHM::onBeforeDraw, this));
	_layer_draw.at(0)._customCommand.setAfterCallback(CC_CALLBACK_0(LodHM::onAfterDraw, this));

	// Set blending
	auto& blend = _layer_draw.at(0)._customCommand.getPipelineDescriptor().blendDescriptor;
	blend.blendEnabled = true;
	blend.sourceRGBBlendFactor = cocos2d::backend::BlendFactor::SRC_ALPHA;
	blend.destinationRGBBlendFactor = cocos2d::backend::BlendFactor::ONE_MINUS_SRC_ALPHA;
	blend.sourceAlphaBlendFactor = cocos2d::backend::BlendFactor::SRC_ALPHA;
	blend.destinationAlphaBlendFactor = cocos2d::backend::BlendFactor::ONE_MINUS_SRC_ALPHA;
	for (int i = 1; i < _hM->_prop._layers.size(); ++i)
	{
		_layer_draw.at(i)._customCommand.setVertexBuffer(_layer_draw.at(0)._customCommand.getVertexBuffer());
		_layer_draw.at(i)._customCommand.setIndexBuffer(_layer_draw.at(0)._customCommand.getIndexBuffer(), _layer_draw.at(0)._customCommand.getIndexFormat());
		_layer_draw.at(i)._customCommand.setIndexDrawInfo(0, _layer_draw.at(0)._customCommand.getIndexDrawCount());
		_layer_draw.at(i)._customCommand.setTransparent(false);
		_layer_draw.at(i)._customCommand.set3D(true);
		_layer_draw.at(i)._customCommand.setBeforeCallback(CC_CALLBACK_0(LodHM::onBeforeDraw, this));
		_layer_draw.at(i)._customCommand.setAfterCallback(CC_CALLBACK_0(LodHM::onAfterDraw, this));

		// Set blending
		auto& blend = _layer_draw.at(i)._customCommand.getPipelineDescriptor().blendDescriptor;
		blend.blendEnabled = true;
		blend.sourceRGBBlendFactor = cocos2d::backend::BlendFactor::SRC_ALPHA;
		blend.destinationRGBBlendFactor = cocos2d::backend::BlendFactor::ONE_MINUS_SRC_ALPHA;
		blend.sourceAlphaBlendFactor = cocos2d::backend::BlendFactor::SRC_ALPHA;
		blend.destinationAlphaBlendFactor = cocos2d::backend::BlendFactor::ONE_MINUS_SRC_ALPHA;
	}
}

LodHM::NOT_DRAW LodHM::updateVertexArray(const cocos2d::Vec3 & p, int prev_lev_mult)
{
	// Update underlying level
	NOT_DRAW nextLayer;
	if (_next)
		nextLayer = _next->updateVertexArray(p, levelMult());

	prev_lev_mult /= levelMult();

	// If previouse buffer was not updated yet
	if (_lod_state != NONE || nextLayer == NOT_DRAW(0.f, 0.f, 0.f, 0.f, true) && nextLayer.needRedr == true)
		return NOT_DRAW(0.f, 0.f, 0.f, 0.f, true);

	_nextLayer = nextLayer;

	// Width and Height according to the Level number
	auto hLev = (_h - 1) * levelMult();
	auto wLev = (_w - 1) * levelMult();

	// Center x
	float center_x = _hM->_pos.x - std::fmod(_hM->_pos.x, float(levelMultCenter() * _hM->_prop._scale.x));
	float start_x = center_x - int(wLev / 2.f) * _hM->_prop._scale.x;

	// Center z
	float center_z = _hM->_pos.z - std::fmod(_hM->_pos.z, float(levelMultCenter() * _hM->_prop._scale.z));
	float start_z = center_z - int(hLev / 2.f) * _hM->_prop._scale.z;

	// If must be forcibly updated - not needed to check new position
	if (!_force_level_update)
	{
		// If position was not changed - the updated is not needed
		auto _newInd = cocos2d::Vec2(start_x, start_z);
		if (_old_pos_ind == _newInd && _nextLayer.needRedr == false)
			return NOT_DRAW(start_z, start_x, start_z + hLev * _hM->_prop._scale.z, start_x + wLev * _hM->_prop._scale.x, false);

		// Save new indicies
		_old_pos_ind = _newInd;
	}
	else
	{
		--_force_level_update;
	}

	float center_x_grass = 0.f;
	float center_z_grass = 0.f;
	if (!_next)
	{
		int j_cur_chunk, i_cur_chunk;
		_hM->getZtoNorm(center_z, &j_cur_chunk);
		_hM->getXtoNorm(center_x, &i_cur_chunk);
		_hM->setCurrentChunk(j_cur_chunk, i_cur_chunk);

		// Adjust grass center (It is needed for smooth grass appear)
		if (_hM->_programState)
		{
			center_x_grass = _hM->_pos.x - std::fmod(_hM->_pos.x, _hM->_prop._scale.x);
			if (center_x_grass == center_x)
				center_x_grass += _hM->_prop._scale.x * 2.f;
			else
				center_x_grass = center_x;
			center_z_grass = _hM->_pos.z - std::fmod(_hM->_pos.z, _hM->_prop._scale.z);
			if (center_z_grass == center_z)
				center_z_grass += _hM->_prop._scale.z * 2.f;
			else
				center_z_grass = center_z;
		}
	}

	// Update vertex datas
	int grass_mdl_cnt = 0;
	unsigned int iArr = 0;

	int jjj = 0;
	NOT_DRAW n_draw(start_z, start_x, start_z + hLev * _hM->_prop._scale.z, start_x + wLev * _hM->_prop._scale.x, false);
	for (float zp = start_z; jjj < _h; zp += levelMult() * _hM->_prop._scale.z, ++jjj) // For each string
	{
		int iii = 0;
		for (float xp = start_x; iii < _w; xp += levelMult() * _hM->_prop._scale.x, ++iii) // For each column
		{
			iArr = iii + jjj * _w;

			int j_world_num;
			float norm_z = _hM->getZtoNorm(zp, &j_world_num);
			int j = _hM->zToJ(norm_z);

			int i_world_num;
			float norm_x = _hM->getXtoNorm(xp, &i_world_num);
			int i = _hM->xToI(norm_x);

			auto& one = _hM->HEIGHT(i_world_num, j_world_num, i, j);

			lVert[iArr].y = one.h;

			int ost_i = iii & (prev_lev_mult - 1);
			if ((!(jjj * (jjj ^ (_h - 1)))) * ost_i)
			{
				float dst = float(ost_i) / float(prev_lev_mult);

				// Get h1
				float x = xp - levelMult() * _hM->_prop._scale.x * ost_i;
				int i_world_num;
				float norm_x = _hM->getXtoNorm(x, &i_world_num);
				int i = _hM->xToI(norm_x);

				float h1 = _hM->HEIGHT(i_world_num, j_world_num, i, j).h;

				// Get h2
				x = xp + levelMult() * _hM->_prop._scale.x * (prev_lev_mult - ost_i);
				norm_x = _hM->getXtoNorm(x, &i_world_num);
				i = _hM->xToI(norm_x);

				float h2 = _hM->HEIGHT(i_world_num, j_world_num, i, j).h;

				float mh = h1 + (h2 - h1) * dst;

				lVert[iArr].y = mh;
			}

			int ost_j = jjj & (prev_lev_mult - 1);
			if ((!(iii * (iii ^ (_w - 1)))) * ost_j)
			{
				float dst = float(ost_j) / float(prev_lev_mult);

				// Get h1
				float z = zp - levelMult() * _hM->_prop._scale.z * ost_j;
				int j_world_num;
				float norm_z = _hM->getZtoNorm(z, &j_world_num);
				int j = _hM->zToJ(norm_z);

				float h1 = _hM->HEIGHT(i_world_num, j_world_num, i, j).h;

				// Get h2
				z = zp + levelMult() * _hM->_prop._scale.z * (prev_lev_mult - ost_j);
				norm_z = _hM->getZtoNorm(z, &j_world_num);
				j = _hM->zToJ(norm_z);

				float h2 = _hM->HEIGHT(i_world_num, j_world_num, i, j).h;

				float mh = h1 + (h2 - h1) * dst;

				lVert[iArr].y = mh;
			}

			if (!((iii - 1) * ((iii + 1) ^ (_w - 1)) + (jjj - 1) * ((jjj + 1) ^ (_h - 1))))
			{
				assert((jjj == 1) || (jjj == _h - 2));
				assert((iii == 1) || (iii == _w - 2));

				// Get h1
				float z = zp - levelMult() * _hM->_prop._scale.z;
				int j_world_num;
				float norm_z = _hM->getZtoNorm(z, &j_world_num);
				int j = _hM->zToJ(norm_z);

				float x = xp - levelMult() * _hM->_prop._scale.x;
				int i_world_num;
				float norm_x = _hM->getXtoNorm(x, &i_world_num);
				int i = _hM->xToI(norm_x);

				float h1 = _hM->HEIGHT(i_world_num, j_world_num, i, j).h;

				// Get h2
				z = zp + levelMult() * _hM->_prop._scale.z;
				norm_z = _hM->getZtoNorm(z, &j_world_num);
				j = _hM->zToJ(norm_z);

				x = xp + levelMult() * _hM->_prop._scale.x;
				norm_x = _hM->getXtoNorm(x, &i_world_num);
				i = _hM->xToI(norm_x);

				float h2 = _hM->HEIGHT(i_world_num, j_world_num, i, j).h;

				float mh = (h1 + h2) / 2;

				lVert[iArr].y = mh;
			}

			// Set vertex index
			lVert[iArr].x = xp;
			lVert[iArr].z = zp;

			OneChunk::ONE_HEIGHT::NORMAL ret_n;
			_hM->getNormalForVertex(xp, zp, 1.f, &ret_n);
			
			int pack_norm = ((ret_n.x << 16) & 0xff0000) | ((ret_n.y << 8) & 0xff00) | (ret_n.z & 0xff);
			lVert[iArr].npack = float(pack_norm);

			for (int q = 0; q < _layer_count; ++q)
				lVert[iArr].alpha[q] = float(one.lay[q].alpha);

			// Update grass data for the first Lod
			// Adjust grass center
			if (_hM->_programState && !_next)
			{
				float x_grass_coeff = std::abs(center_x_grass - xp) / _hM->_prop._scale.x;
				float z_grass_coeff = std::abs(center_z_grass - zp) / _hM->_prop._scale.z;
				if (x_grass_coeff <= _hM->_grass_prop._tile_count_coef && z_grass_coeff <= _hM->_grass_prop._tile_count_coef)
				{
					std::srand((i + 1) * (i + 1) * (j + 1) * (j + 1));
					for (int l = 0; l < _hM->_grass_prop._rate; ++l)
					{
						unsigned int idx = grass_mdl_cnt * _hM->_grass_prop._rate + l;

						float rnd = rand_minus1_1();
						float xr = _hM->_grass_prop._shift * rnd;
						if (i == 0)
							xr = rnd * 0.5;
						if (i == _hM->_prop._width - 1.0)
							xr = rnd * 0.5 * -1.0;

						rnd = rand_minus1_1();
						float yr = _hM->_grass_prop._shift * rnd;
						if (i == 0)
							yr = rnd * 0.5;
						if (i == _hM->_prop._height - 1.0)
							yr = rnd * 0.5 * -1.0;

						float x_sh = xp + xr;
						float z_sh = zp + yr;
						Vec3 n;
						float y = _hM->getHeight(x_sh, z_sh, &n) / _hM->_prop._scale.y;
						_hM->_gVert[idx].vert[0].i = xp; _hM->_gVert[idx].vert[0].j = zp; _hM->_gVert[idx].vert[0].y = y; _hM->_gVert[idx].vert[0].ratex = xr; _hM->_gVert[idx].vert[0].ratey = yr;
						_hM->_gVert[idx].vert[1].i = xp; _hM->_gVert[idx].vert[1].j = zp; _hM->_gVert[idx].vert[1].y = y; _hM->_gVert[idx].vert[1].ratex = xr; _hM->_gVert[idx].vert[1].ratey = yr;
						_hM->_gVert[idx].vert[2].i = xp; _hM->_gVert[idx].vert[2].j = zp; _hM->_gVert[idx].vert[2].y = y; _hM->_gVert[idx].vert[2].ratex = xr; _hM->_gVert[idx].vert[2].ratey = yr;
						_hM->_gVert[idx].vert[3].i = xp; _hM->_gVert[idx].vert[3].j = zp; _hM->_gVert[idx].vert[3].y = y; _hM->_gVert[idx].vert[3].ratex = xr; _hM->_gVert[idx].vert[3].ratey = yr;
					}
					grass_mdl_cnt++;
					_hM->_grass_gl._mdl_cnt = grass_mdl_cnt * _hM->_grass_prop._rate;
				}
			}
		}
	}

	_lod_state = UPDATE_GL_BUFFER;

	// Return the current drawn square
	return n_draw;
}

void LodHM::drawLandScape(cocos2d::Renderer * renderer, const cocos2d::Mat4 & transform, uint32_t flags)
{
	if (_next)
		_next->drawLandScape(renderer, transform, flags);

	// Draw
	auto camera = Camera::getVisitingCamera();
	const Mat4& view_proj_mat = camera->getViewProjectionMatrix();
	Vec3 c_pos = camera->getPosition3D();

	for (int i = 0; i < _layer_count; ++i)
	{
		auto& pipelineDescriptor = _layer_draw.at(i)._customCommand.getPipelineDescriptor();
		pipelineDescriptor.programState = _layer_draw.at(i)._programState;
		_layer_draw.at(i)._customCommand.init(0);
		renderer->addCommand(&_layer_draw.at(i)._customCommand);

		// Set VP
		_layer_draw.at(i)._programState->setUniform(_allLodLoc._vp, view_proj_mat.m, sizeof(view_proj_mat.m));

		// Set Camera pos
		_layer_draw.at(i)._programState->setUniform(_allLodLoc._cam_pos_loc, &c_pos, sizeof(c_pos));
	}

#if defined(COCOS2D_DEBUG) && (COCOS2D_DEBUG > 0)
    if (_show_grid)
	{
		_customCommandGrid.init(0.f);
		renderer->addCommand(&_customCommandGrid);

		_programStateGrid->setUniform(_allLodLocGrid._vp, view_proj_mat.m, sizeof(view_proj_mat.m));
		_programStateGrid->setUniform(_allLodLocGrid._cam_pos_loc, &c_pos, sizeof(c_pos));
	}

	if (_norm_size)
	{
		_customCommandNorm.init(0.f);
		renderer->addCommand(&_customCommandNorm);

		_programStateNorm->setUniform(_allLodLocNorm._vp, view_proj_mat.m, sizeof(view_proj_mat.m));
		_programStateNorm->setUniform(_allLodLocNorm._cam_pos_loc, &c_pos, sizeof(c_pos));
	}
#endif
}

unsigned int LodHM::getvertexCount() const
{
	return lVert.size() + ((_next) ? _next->getvertexCount() : 0);
}

void LodHM::setGridModeZ(bool is_z_mode)
{
	if (_next)
		_next->setGridModeZ(is_z_mode);

	_customCommandGrid.set3D(!is_z_mode);
}

void LodHM::createLineBuffers()
{
	// Fill line indicies array
	for (unsigned int j = 0; j < _h; ++j) // Lines
		for (unsigned int i = 0; i < _w; ++i) // Columns
		{
			if (i < _w - 1)
			{
				// Third Line
				iVertLine.push_back(i + j * _w);
				iVertLine.push_back(i + j * _w + 1);
			}

			if (j == _h - 1)
				continue;

			// First line
			iVertLine.push_back(i + j * _w);
			iVertLine.push_back(i + j * _w + _w);

			if (i < _w - 1)
			{
				// Sec Line
				iVertLine.push_back(i + j * _w);
				iVertLine.push_back(i + j * _w + _w + 1);
			}
		}

	_customCommandGrid.createVertexBuffer(sizeof(ONEVERTEX), lVert.size(), CustomCommand::BufferUsage::DYNAMIC);
	_customCommandGrid.createIndexBuffer((CustomCommand::IndexFormat)cocos_index_format, iVertLine.size(), CustomCommand::BufferUsage::STATIC);

	_customCommandGrid.updateVertexBuffer(&lVert[0], lVert.size() * sizeof(ONEVERTEX));
	_customCommandGrid.updateIndexBuffer(&iVertLine[0], iVertLine.size() * sizeof(INDEX_TYPE));

	_customCommandGrid.setTransparent(false);
	_customCommandGrid.set3D(true);

	_customCommandGrid.setPrimitiveType(CustomCommand::PrimitiveType::LINE);
	_customCommandGrid.setLineWidth(1.f);

}

void LodHM::createNormBuffers()
{
	// Fill vertex array
	for (int i = 0; i < _w * _h * 2; ++i)
		lVertNormUp.push_back(ONEVERTEX());

	// Fill norm indicies array
	for (unsigned int i = 0; i < _w * _h; ++i)
	{
		iVertNorm.push_back(i);
		iVertNorm.push_back(i + _w * _h);
	}

	_customCommandNorm.createVertexBuffer(sizeof(ONEVERTEX), lVert.size() + lVertNormUp.size(), CustomCommand::BufferUsage::DYNAMIC);
	_customCommandNorm.createIndexBuffer((CustomCommand::IndexFormat)cocos_index_format, iVertNorm.size(), CustomCommand::BufferUsage::STATIC);

	_customCommandNorm.updateVertexBuffer(&lVertNormUp[0], lVertNormUp.size() * sizeof(ONEVERTEX));
	_customCommandNorm.updateIndexBuffer(&iVertNorm[0], iVertNorm.size() * sizeof(INDEX_TYPE));

	_customCommandNorm.setTransparent(false);
	_customCommandNorm.set3D(true);

	_customCommandNorm.setPrimitiveType(CustomCommand::PrimitiveType::LINE);
	_customCommandNorm.setLineWidth(1.f);
}

bool LodHM::rayCast(const cocos2d::Vec3 & start_p, const cocos2d::Vec3 & end_p, cocos2d::Vec3 * ret)
{
	// Algorithm works only for the first layer
	if (this->_next)
		return this->_next->rayCast(start_p, end_p, ret);

	cocos2d::Vec3 p_res;
	std::vector<cocos2d::Vec3> res;
	res.reserve(10);

	// Searching along all triangles
	for (unsigned int j = 0; j < _h - 1; ++j) // Lines
		for (unsigned int i = 0; i < _w - 1; ++i) // Columns
		{
			float p1_x = lVert[j * _w + i].x;
			float p1_z = lVert[j * _w + i].z;
			float p2_x = lVert[j * _w + i].x;
			float p2_z = lVert[(j + 1) * _w + i].z;
			float p3_x = lVert[j * _w + i + 1].x;
			float p3_z = lVert[(j + 1) * _w + i].z;

			// First tris
			if (rayCastOneTriangle(start_p, end_p,
				cocos2d::Vec3(p1_x, lVert[i + j * _h].y * _hM->_prop._scale.y, p1_z),
				cocos2d::Vec3(p2_x, lVert[i + j * _h + _h].y * _hM->_prop._scale.y, p2_z),
				cocos2d::Vec3(p3_x, lVert[i + j * _h + _h + 1].y * _hM->_prop._scale.y, p3_z),
				&p_res) == true)
				res.push_back(p_res);

			p1_x = lVert[j * _w + i].x;
			p1_z = lVert[j * _w + i].z;
			p2_x = lVert[j * _w + i + 1].x;
			p2_z = lVert[(j + 1) * _w + i].z;
			p3_x = lVert[j * _w + i + 1].x;
			p3_z = lVert[j * _w + i].z;

			// Second tris
			if (rayCastOneTriangle(start_p, end_p,
				cocos2d::Vec3(p1_x, lVert[i + j * _h].y * _hM->_prop._scale.y, p1_z),
				cocos2d::Vec3(p2_x, lVert[i + j * _h + _h + 1].y * _hM->_prop._scale.y, p2_z),
				cocos2d::Vec3(p3_x, lVert[i + j * _h + 1].y * _hM->_prop._scale.y, p3_z),
				&p_res) == true)
				res.push_back(p_res);
		}

	// No intersection
	if (res.size() == 0)
		return false;

	// Seaching the point which is closer to the start_p
	*ret = res[0];
	float len_r = (*ret - start_p).length();
	for (auto v = res.begin() + 1; v != res.end(); ++v)
	{
		float len = (*v - start_p).length();

		if (len < len_r)
		{
			len_r = len;
			*ret = *v;
		}
	}

	return true;
}

void LodHM::updateNormBuffers()
{
	_customCommandNorm.updateVertexBuffer(lVert.data(), 0, lVert.size() * sizeof(ONEVERTEX));

	// Update up vertex
	for (int i = 0; i < lVert.size(); ++i)
	{
		float xn = lVert[i].npack / 65536.0 - 128.0;
		float yn = lVert[i].npack / 256.0;
		yn = (int(yn) % 256) - 128.0;
		float zn = (int(lVert[i].npack) % 256) - 128.0;
		Vec3 v_normal = Vec3(xn / 127.0, yn / 127.0, zn / 127.0);
		v_normal.normalize();

		lVertNormUp[i] = lVert[i];
		lVertNormUp[i].x += v_normal.x * _norm_size;
		lVertNormUp[i].y += v_normal.y * _norm_size;
		lVertNormUp[i].z += v_normal.z * _norm_size;
	}

	_customCommandNorm.updateVertexBuffer(lVertNormUp.data(), lVert.size() * sizeof(ONEVERTEX), lVert.size() * sizeof(ONEVERTEX));
}

bool LodHM::rayCastOneTriangle(const cocos2d::Vec3 & start_p, const cocos2d::Vec3 & end_p,
	const cocos2d::Vec3 & p1, const cocos2d::Vec3 & p2, const cocos2d::Vec3 & p3,
	cocos2d::Vec3 * p_res)
{
	const double _EPS = 0.000000001;
	const double _M_PI = 3.14159265358979323846;
	const double _RTOD = 180. / _M_PI;

	// Normal counting
	cocos2d::Vec3 n = cocos2d::Vec3(p3.x - p1.x, p3.y - p1.y, p3.z - p1.z);
	cocos2d::Vec3 v2 = cocos2d::Vec3(p2.x - p1.x, p2.y - p1.y, p2.z - p1.z);
	n.cross(v2);
	n.normalize();

	// D ratio in the plane equation Ax+By+Cz+D=0
	float D = -1 * (n.x * p1.x + n.y * p1.y + n.z * p1.z);

	// The denominator
	float denom = n.x * (end_p.x - start_p.x) + n.y * (end_p.y - start_p.y) + n.z * (end_p.z - start_p.z);

	// Line and plane don't intersect
	if (std::abs(denom) < _EPS)
		return false;

	float mu = -1 * (D + n.x * start_p.x + n.y * start_p.y + n.z * start_p.z) / denom;

	// Intersection not along line segment
	if (mu < 0.0 || mu > 1.0)
		return false;

	// Calculating the intersection point
	p_res->x = start_p.x + mu * (end_p.x - start_p.x);
	p_res->y = start_p.y + mu * (end_p.y - start_p.y);
	p_res->z = start_p.z + mu * (end_p.z - start_p.z);

	// Determine whether or not the intersection point is bounded by p1 ,p2 ,p3
	cocos2d::Vec3 n1O;
	cocos2d::Vec3::cross(p3 - p1, *p_res - p1, &n1O);

	cocos2d::Vec3 n2O;
	cocos2d::Vec3::cross(*p_res - p1, p2 - p1, &n2O);

	cocos2d::Vec3 n3O;
	cocos2d::Vec3::cross(p3 - *p_res, p2 - *p_res, &n3O);

	float sc1 = cocos2d::Vec3::dot(n, n1O);
	float sc2 = cocos2d::Vec3::dot(n, n2O);
	float sc3 = cocos2d::Vec3::dot(n, n3O);

	return (sc1 > 0 && sc2 > 0 && sc3 > 0);
}

void LodHM::createShader()
{
	std::string vertexSource = FileUtils::getInstance()->getStringFromFile(FileUtils::getInstance()->fullPathForFilename("res/shaders/model_terrain.vert"));
	std::string fragmentSource = FileUtils::getInstance()->getStringFromFile(FileUtils::getInstance()->fullPathForFilename("res/shaders/model_terrain.frag"));

	std::string def = StringUtils::format("#define MAX_LAYER_COUNT %i\n", MAX_LAYER_COUNT);
	def += StringUtils::format("#define LAYER_TEXTURE_SIZE %i\n", LAYER_TEXTURE_SIZE);

	if (_lod_num == 1)
		def += "#define FIRST_LOD\n";

	if (_hM->_prop._lod_data.at(_lod_num - 1).text_lod_dist_to)
		def += "#define TEXT_LOD\n";

	def += "#define FOG\n";

	if (_hM->_prop._is_normal_map)
	{
		def += "#define NORMAL_MAP\n";
	}

	if (_hM->_prop._layers.size() > 1)
		def += "#define MULTI_LAYER\n";

	auto program = backend::Device::getInstance()->newProgram(def + vertexSource, def + fragmentSource);
	_layer_draw.at(0)._programState = new backend::ProgramState(program);

	auto layout = _layer_draw.at(0)._programState->getVertexLayout();

	const auto & attributeInfo = _layer_draw.at(0)._programState->getProgram()->getActiveAttributes();

	const auto & iter1 = attributeInfo.find("a_height");
	if (iter1 != attributeInfo.end())
		layout->setAttribute("a_height", iter1->second.location, backend::VertexFormat::FLOAT, offsetof(ONEVERTEX, y), false);

	const auto & iter2 = attributeInfo.find("a_npack");
	if (iter2 != attributeInfo.end())
		layout->setAttribute("a_npack", iter2->second.location, backend::VertexFormat::FLOAT, offsetof(ONEVERTEX, npack), false);

	const auto & iter3 = attributeInfo.find("a_alpha");
	if (iter3 != attributeInfo.end())
	{
		#if MAX_LAYER_COUNT == 1
		    layout->setAttribute("a_alpha", iter3->second.location, backend::VertexFormat::FLOAT, offsetof(ONEVERTEX, alpha), false);
        #elif MAX_LAYER_COUNT == 2
		    layout->setAttribute("a_alpha", iter3->second.location, backend::VertexFormat::FLOAT2, offsetof(ONEVERTEX, alpha), false);
        #elif MAX_LAYER_COUNT == 3
		    layout->setAttribute("a_alpha", iter3->second.location, backend::VertexFormat::FLOAT3, offsetof(ONEVERTEX, alpha), false);
        #elif MAX_LAYER_COUNT == 4
		    layout->setAttribute("a_alpha", iter3->second.location, backend::VertexFormat::FLOAT3, offsetof(ONEVERTEX, alpha), false);
        #endif
	}

	const auto & iter5 = attributeInfo.find("a_vertex_x");
	if (iter5 != attributeInfo.end())
		layout->setAttribute("a_vertex_x", iter5->second.location, backend::VertexFormat::FLOAT, offsetof(ONEVERTEX, x), false);

	const auto & iter6 = attributeInfo.find("a_vertex_z");
	if (iter6 != attributeInfo.end())
		layout->setAttribute("a_vertex_z", iter6->second.location, backend::VertexFormat::FLOAT, offsetof(ONEVERTEX, z), false);

	layout->setLayout(sizeof(ONEVERTEX));

	_allLodLoc._sun_light_dir = _layer_draw.at(0)._programState->getUniformLocation("u_DirLightSun");
	_allLodLoc._sun_light_col = _layer_draw.at(0)._programState->getUniformLocation("u_ColLightSun");
	_allLodLoc._ambient_light_dir = _layer_draw.at(0)._programState->getUniformLocation("u_AmbientLightSourceColor");
	_allLodLoc._fog_col = _layer_draw.at(0)._programState->getUniformLocation("u_ColFog");
	_allLodLoc._vp = _layer_draw.at(0)._programState->getUniformLocation("u_VPMatrix");

	auto loc_darker_dist = _layer_draw.at(0)._programState->getUniformLocation("u_darker_dist");
	_layer_draw.at(0)._programState->setUniform(loc_darker_dist, &_hM->_prop._darker_dist, sizeof(float));

	_allLodLoc._scale = _layer_draw.at(0)._programState->getUniformLocation("u_scale");
	_layer_draw.at(0)._programState->setUniform(_allLodLoc._scale, &_hM->_prop._scale, sizeof(Vec3));
	
	if (_lod_num > 1)
	{
		_allLodLoc._ndraw = _layer_draw.at(0)._programState->getUniformLocation("u_Ndraw");
		Vec4 n_draw = Vec4::ZERO;
		_layer_draw.at(0)._programState->setUniform(_allLodLoc._ndraw, &n_draw, sizeof(n_draw));

		auto radiusLodLoc = _layer_draw.at(0)._programState->getUniformLocation("u_lod_radius");
		Vec3 radius_lod;
		// Works only for _lod_init == -1 and _lod_rank == 1, 
		// when _hM->_prop._scale.x == _hM->_prop._scale.z and
		// _hM->_prop._width == _hM->_prop._height
		float prev_mult = _hM->_prop._scale.x * std::pow(2, _lod_num - 2);
		radius_lod.x = (_hM->_prop._lod_data.at(_lod_num - 2).width / 2) * prev_mult - prev_mult * LOD_ALPHA_FROM_RATE;
		radius_lod.z = (_hM->_prop._lod_data.at(_lod_num - 2).width / 2) * prev_mult - prev_mult * LOD_ALPHA_TO_RATE;
		radius_lod.y = (_lod_num - 1) * 0.01f;
		_layer_draw.at(0)._programState->setUniform(radiusLodLoc, &radius_lod, sizeof(radius_lod));
	}

	_allLodLoc._nearFogPlane = _layer_draw.at(0)._programState->getUniformLocation("u_near_fog_plane");
	_allLodLoc._farFogPlane = _layer_draw.at(0)._programState->getUniformLocation("u_far_fog_plane");

	// Set texture size for the 0 layer
	auto loc_texture_size = _layer_draw.at(0)._programState->getUniformLocation("u_text_tile_size");
	auto loc_specular_fct = _layer_draw.at(0)._programState->getUniformLocation("u_specular_factor");
	std::array<Vec2, LAYER_TEXTURE_SIZE> text_size;
	std::array<float, LAYER_TEXTURE_SIZE> spec_fct;
	for (int i = 0; i < LAYER_TEXTURE_SIZE; ++i)
	{
		float sc_sz_coef = _hM->_prop._layers.at(0)._text.at(i).scale_size_coef;

		if (_lod_num == 1)
		    text_size.at(i).x = _hM->_prop._layers.at(0)._text.at(i).first_size;
		else
			text_size.at(i).x = (sc_sz_coef)?_hM->_prop._layers.at(0)._text.at(i).first_size *
			    _hM->_prop._layers.at(0)._text.at(i).scale_size_coef * _lod_num:
			    _hM->_prop._layers.at(0)._text.at(i).first_size;

		text_size.at(i).y = (sc_sz_coef)?_hM->_prop._layers.at(0)._text.at(i).first_size *
			_hM->_prop._layers.at(0)._text.at(i).scale_size_coef * (_lod_num + 1):
			_hM->_prop._layers.at(0)._text.at(i).first_size;

		spec_fct.at(i) = _hM->_prop._layers.at(0)._text.at(i).specular_factor;
	}
	_layer_draw.at(0)._programState->setUniform(loc_texture_size, text_size.data(), sizeof(Vec2) * text_size.size());
	_layer_draw.at(0)._programState->setUniform(loc_specular_fct, spec_fct.data(), sizeof(float) * spec_fct.size());

	// Set texture lod distance if needed
	if (_hM->_prop._lod_data.at(_lod_num - 1).text_lod_dist_to)
	{
		auto loc = _layer_draw.at(0)._programState->getUniformLocation("u_text_lod_distance");
		Vec2 dist = Vec2::ZERO;
		dist.x = _hM->_prop._lod_data.at(_lod_num - 1).text_lod_dist_to;
		if (_lod_num > 1)
			dist.y = _hM->_prop._lod_data.at(_lod_num - 1).text_lod_dist_from;
		_layer_draw.at(0)._programState->setUniform(loc, &dist, sizeof(dist));
	}

	_allLodLoc._diff_loc = _layer_draw.at(0)._programState->getUniformLocation("u_text");

	if (_hM->_prop._is_normal_map)
	{
		_allLodLoc._norm_loc = _layer_draw.at(0)._programState->getUniformLocation("u_text_n");
		
		auto loc_normal_scale = _layer_draw.at(0)._programState->getUniformLocation("u_normal_scale");
		std::array<float, LAYER_TEXTURE_SIZE> normal_scale;
		for (int i = 0; i < LAYER_TEXTURE_SIZE; ++i)
			normal_scale.at(i) = _hM->_prop._layers.at(0)._text.at(i).normal_map_scale;
		_layer_draw.at(0)._programState->setUniform(loc_normal_scale, &(normal_scale), sizeof(float) * normal_scale.size());
	}

	_allLodLoc._cam_pos_loc = _layer_draw.at(0)._programState->getUniformLocation("u_camPos");

	// Set uniform to define layer number
	float lay_num = 0;
	auto loc_layer_num = _layer_draw.at(0)._programState->getUniformLocation("u_layer_num");
	_layer_draw.at(0)._programState->setUniform(loc_layer_num, &lay_num, sizeof(lay_num));

	// Set uniform for layer draw distance
	if (_hM->_prop._layers.size() > 1)
	{
		auto loc_layer_dist = _layer_draw.at(0)._programState->getUniformLocation("u_dist_alpha_layer");
		Vec2 dist_layer = Vec2(1.f, 0.f);
		_layer_draw.at(0)._programState->setUniform(loc_layer_dist, &dist_layer, sizeof(Vec2));

		// Copy null programState for all layers
		float dist_prev_lod = 0.f;
		if (_lod_num > 1)
			dist_prev_lod = (_hM->_prop._lod_data.at(_lod_num - 2).width / 2) * _hM->_prop._scale.x * std::pow(2, _lod_num - 2); // Only for _lod_init == -1 and _lod_rank == 1
		for (int i = 1; i < _hM->_prop._layers.size(); ++i)
		{
			dist_layer = Vec2(_hM->_prop._layers.at(i)._dist, _hM->_prop._layers.at(i)._dist / 2.f);
			if (dist_layer.x > dist_prev_lod)
			{
				_layer_count++;

				_layer_draw.at(i)._programState = _layer_draw.at(0)._programState->clone();
				lay_num = i;
				_layer_draw.at(i)._programState->setUniform(loc_layer_num, &lay_num, sizeof(lay_num));
				_layer_draw.at(i)._programState->setUniform(loc_layer_dist, &dist_layer, sizeof(Vec2));

				// Set texture size for other layers
				std::array<Vec2, LAYER_TEXTURE_SIZE> text_size;
				for (int j = 0; j < LAYER_TEXTURE_SIZE; ++j)
				{
					float sc_sz_coef = _hM->_prop._layers.at(i)._text.at(j).scale_size_coef;

					if (_lod_num == 1)
						text_size.at(j).x = _hM->_prop._layers.at(i)._text.at(j).first_size;
					else
						text_size.at(j).x = (sc_sz_coef) ? _hM->_prop._layers.at(i)._text.at(j).first_size *
						_hM->_prop._layers.at(i)._text.at(j).scale_size_coef * _lod_num :
						_hM->_prop._layers.at(i)._text.at(j).first_size;

					text_size.at(j).y = (sc_sz_coef) ? _hM->_prop._layers.at(i)._text.at(j).first_size *
						_hM->_prop._layers.at(i)._text.at(j).scale_size_coef * (_lod_num + 1) :
						_hM->_prop._layers.at(i)._text.at(j).first_size;
				}
				_layer_draw.at(i)._programState->setUniform(loc_texture_size, text_size.data(), sizeof(Vec2) * text_size.size());
			}
		}
	}
}

void LodHM::createShaderGrid()
{
	std::string vertexSource = FileUtils::getInstance()->getStringFromFile(FileUtils::getInstance()->fullPathForFilename("res/shaders/model_terrain_grid.vert"));
	std::string fragmentSource = FileUtils::getInstance()->getStringFromFile(FileUtils::getInstance()->fullPathForFilename("res/shaders/model_terrain_grid.frag"));

	std::string def;
	if (_lod_num == 1)
		def += "#define FIRST_LOD\n";

	auto program = backend::Device::getInstance()->newProgram(def + vertexSource, def + fragmentSource);
	_programStateGrid = new backend::ProgramState(program);

	auto & pipelineDescriptor = _customCommandGrid.getPipelineDescriptor();
	pipelineDescriptor.programState = _programStateGrid;
	auto layout = _programStateGrid->getVertexLayout();

	const auto & attributeInfo = _programStateGrid->getProgram()->getActiveAttributes();

	const auto & iter1 = attributeInfo.find("a_height");
	if (iter1 != attributeInfo.end())
		layout->setAttribute("a_height", iter1->second.location, backend::VertexFormat::FLOAT, offsetof(ONEVERTEX, y), false);

	const auto & iter5 = attributeInfo.find("a_vertex_x");
	if (iter5 != attributeInfo.end())
		layout->setAttribute("a_vertex_x", iter5->second.location, backend::VertexFormat::FLOAT, offsetof(ONEVERTEX, x), false);

	const auto & iter6 = attributeInfo.find("a_vertex_z");
	if (iter6 != attributeInfo.end())
		layout->setAttribute("a_vertex_z", iter6->second.location, backend::VertexFormat::FLOAT, offsetof(ONEVERTEX, z), false);

	layout->setLayout(sizeof(ONEVERTEX));

	_allLodLocGrid._vp = _programStateGrid->getUniformLocation("u_VPMatrix");

	_allLodLocGrid._scale = _programStateGrid->getUniformLocation("u_scale");
	_programStateGrid->setUniform(_allLodLocGrid._scale, &_hM->_prop._scale, sizeof(Vec3));

	if (_lod_num > 1)
	{
		_allLodLocGrid._ndraw = _programStateGrid->getUniformLocation("u_Ndraw");
		Vec4 n_draw = Vec4::ZERO;
		_programStateGrid->setUniform(_allLodLocGrid._ndraw, &n_draw, sizeof(n_draw));

		auto radiusLodLoc = _programStateGrid->getUniformLocation("u_lod_radius");
		Vec3 radius_lod;
		// Works only for _lod_init == -1 and _lod_rank == 1, 
		// when _hM->_prop._scale.x == _hM->_prop._scale.z and
		// _hM->_prop._width == _hM->_prop._height
		float prev_mult = _hM->_prop._scale.x * std::pow(2, _lod_num - 2);
		radius_lod.x = (_hM->_prop._lod_data.at(_lod_num - 2).width / 2) * prev_mult - prev_mult * LOD_ALPHA_FROM_RATE;
		radius_lod.z = (_hM->_prop._lod_data.at(_lod_num - 2).width / 2) * prev_mult - prev_mult * LOD_ALPHA_TO_RATE;

		radius_lod.y = (_lod_num - 1) * 0.01f;
		_programStateGrid->setUniform(radiusLodLoc, &radius_lod, sizeof(radius_lod));
	}

	_allLodLocGrid._cam_pos_loc = _programStateGrid->getUniformLocation("u_camPos");

	auto loc = _programStateGrid->getUniformLocation("u_color");
	int idx = (_lod_num - 1) & ~8;
	Vec4 color = Vec4(_lod_color[idx].x, _lod_color[idx].y, _lod_color[idx].z, 1.f);
	_programStateGrid->setUniform(loc, &color, sizeof(color));
}

void LodHM::createShaderNorm()
{
	std::string vertexSource = FileUtils::getInstance()->getStringFromFile(FileUtils::getInstance()->fullPathForFilename("res/shaders/model_terrain_grid.vert"));
	std::string fragmentSource = FileUtils::getInstance()->getStringFromFile(FileUtils::getInstance()->fullPathForFilename("res/shaders/model_terrain_grid.frag"));

	std::string def;
	if (_lod_num == 1)
		def += "#define FIRST_LOD\n";

	auto program = backend::Device::getInstance()->newProgram(def + vertexSource, def + fragmentSource);
	_programStateNorm = new backend::ProgramState(program);

	auto& pipelineDescriptor = _customCommandNorm.getPipelineDescriptor();
	pipelineDescriptor.programState = _programStateNorm;
	auto layout = _programStateNorm->getVertexLayout();

	const auto& attributeInfo = _programStateNorm->getProgram()->getActiveAttributes();

	const auto& iter1 = attributeInfo.find("a_height");
	if (iter1 != attributeInfo.end())
		layout->setAttribute("a_height", iter1->second.location, backend::VertexFormat::FLOAT, offsetof(ONEVERTEX, y), false);

	const auto & iter5 = attributeInfo.find("a_vertex_x");
	if (iter5 != attributeInfo.end())
		layout->setAttribute("a_vertex_x", iter5->second.location, backend::VertexFormat::FLOAT, offsetof(ONEVERTEX, x), false);

	const auto & iter6 = attributeInfo.find("a_vertex_z");
	if (iter6 != attributeInfo.end())
		layout->setAttribute("a_vertex_z", iter6->second.location, backend::VertexFormat::FLOAT, offsetof(ONEVERTEX, z), false);

	layout->setLayout(sizeof(ONEVERTEX));

	_allLodLocNorm._vp = _programStateNorm->getUniformLocation("u_VPMatrix");

	_allLodLocNorm._scale = _programStateNorm->getUniformLocation("u_scale");
	_programStateNorm->setUniform(_allLodLocNorm._scale, &_hM->_prop._scale, sizeof(Vec3));

	if (_lod_num > 1)
	{
		_allLodLocNorm._ndraw = _programStateNorm->getUniformLocation("u_Ndraw");
		Vec4 n_draw = Vec4::ZERO;
		_programStateNorm->setUniform(_allLodLocNorm._ndraw, &n_draw, sizeof(n_draw));

		auto radiusLodLoc = _programStateNorm->getUniformLocation("u_lod_radius");
		Vec3 radius_lod;
		
		// Works only for _lod_init == -1 and _lod_rank == 1, 
		// when _hM->_prop._scale.x == _hM->_prop._scale.z and
		// _hM->_prop._width == _hM->_prop._height
		float prev_mult = _hM->_prop._scale.x * std::pow(2, _lod_num - 2);
		radius_lod.x = (_hM->_prop._lod_data.at(_lod_num - 2).width / 2) * prev_mult - prev_mult * LOD_ALPHA_FROM_RATE;
		radius_lod.z = (_hM->_prop._lod_data.at(_lod_num - 2).width / 2) * prev_mult - prev_mult * LOD_ALPHA_TO_RATE;

		radius_lod.y = (_lod_num - 1) * 0.01f;
		_programStateNorm->setUniform(radiusLodLoc, &radius_lod, sizeof(radius_lod));
	}

	_allLodLocNorm._cam_pos_loc = _programStateNorm->getUniformLocation("u_camPos");

	auto loc = _programStateNorm->getUniformLocation("u_color");
	int idx = (_lod_num - 1) & ~8;
	Vec4 color = Vec4(_lod_color[idx].x, _lod_color[idx].y, _lod_color[idx].z, 1.f);
	_programStateNorm->setUniform(loc, &color, sizeof(color));
}

void LodHM::updateSkyLightData()
{
	if (_next)
		_next->updateSkyLightData();

	for (int i = 0; i < _layer_count; ++i)
	{
		// Light
		_layer_draw.at(i)._programState->setUniform(_allLodLoc._sun_light_dir, &_hM->_prop._light.dirAtmLight, sizeof(_hM->_prop._light.dirAtmLight));
		_layer_draw.at(i)._programState->setUniform(_allLodLoc._sun_light_col, &_hM->_prop._light.colAtmLight, sizeof(_hM->_prop._light.colAtmLight));
		_layer_draw.at(i)._programState->setUniform(_allLodLoc._ambient_light_dir, &_hM->_prop._light.ambAtmLight, sizeof(_hM->_prop._light.ambAtmLight));

		// Fog
		_layer_draw.at(i)._programState->setUniform(_allLodLoc._fog_col, &_hM->_prop._light.colFog, sizeof(_hM->_prop._light.colFog));
		_layer_draw.at(i)._programState->setUniform(_allLodLoc._nearFogPlane, &_hM->_prop._light.fog_near_plane, sizeof(_hM->_prop._light.fog_near_plane));
		_layer_draw.at(i)._programState->setUniform(_allLodLoc._farFogPlane, &_hM->_prop._light.fog_far_plane, sizeof(_hM->_prop._light.fog_far_plane));
	}
}

void LodHM::setTextures()
{
	if (_next)
		_next->setTextures();

	for (int i = 0; i < _layer_count; ++i)
	{
		int text_slot = 0;
		std::vector<int> slots;
		std::vector<cocos2d::backend::TextureBackend*> txt;
		for (int j = 0; j < LAYER_TEXTURE_SIZE; ++j)
		{
			slots.push_back(j + text_slot);
			txt.push_back(_hM->_layerData[i]._text[j].diff->getBackendTexture());
		}
		_layer_draw.at(i)._programState->setTextureArray(_allLodLoc._diff_loc, slots, txt);

		if (_hM->_prop._is_normal_map)
		{
			text_slot = LAYER_TEXTURE_SIZE;
			slots.clear();
			txt.clear();
			for (int j = 0; j < LAYER_TEXTURE_SIZE; ++j)
			{
				slots.push_back(j + text_slot);
				txt.push_back(_hM->_layerData[i]._text[j].norm->getBackendTexture());
			}
			_layer_draw.at(i)._programState->setTextureArray(_allLodLoc._norm_loc, slots, txt);
		}
	}
}

void LodHM::unsetTextures()
{
	if (_next)
		_next->unsetTextures();

	for (int i = 0 ; i < _layer_count; ++i)
	    _hM->clearProgramStateTextures(_layer_draw.at(i)._programState);
}

void LodHM::onBeforeDraw()
{
	auto renderer = Director::getInstance()->getRenderer();
	_rendererDepthCmpFunc = renderer->getDepthCompareFunction();
	renderer->setDepthCompareFunction(backend::CompareFunction::LESS_EQUAL);
}

void LodHM::onAfterDraw()
{
	auto renderer = Director::getInstance()->getRenderer();
	renderer->setDepthCompareFunction(_rendererDepthCmpFunc);
}
