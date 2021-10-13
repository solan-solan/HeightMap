#include "HeightMap.h"
#include "rjh.h"
#include "renderer/backend/Device.h"
#include "ShadowCamera.h"
#include "RenderTexture3D.h"

using namespace hm;
using namespace cocos2d;

#define NORMAL_SLOPE 4

namespace hm
{
	HeightChunk def_i_chunk(41, 41, 25);
	CenterArray<HeightChunk*, &def_i_chunk> def_j_chunks;
}

HeightMap* HeightMap::create(const std::string& prop_file, const std::vector<ShadowCamera*>& shdw_cams)
{
	auto* hm = new (std::nothrow) HeightMap();
	hm->_prop_file = prop_file;
	hm->_shdw = shdw_cams;

	if (hm && hm->init())
	{
		hm->_contentSize = hm->getBoundingBox().size;
		hm->autorelease();
		hm->setCameraMask((unsigned short)CameraFlag::USER1);
		
		return hm;
	}

	CC_SAFE_DELETE(hm);
	return nullptr;
}

HeightMap::~HeightMap()
{
	// Stop thread
	stopUpdThread();

	unloadTextures();

	// Delete lods
	if (_lods) delete _lods;

	// Clear CenterArray values
	for (int i = 0; i < _hgt.size(); ++i)
	{
		if (_hgt[i])
		{
			for (int j = 0; j < _hgt[i]->size(); ++j)
			{
				if ((*_hgt[i])[j])
					delete (*_hgt[i])[j];
			}
			delete _hgt[i];
		}
	}

	disableGrass();
}

HeightMap::HeightMap()
{
    _thr_var = false;
}

void HeightMap::loadProps()
{
	assert(FileUtils::getInstance()->isFileExist(_prop_file));

	std::string str;
	{
		Data data = FileUtils::getInstance()->getDataFromFile(_prop_file);
		str = std::string((char*)data.getBytes(), data.getSize());
	}
	rapidjson::Document d;
	d.Parse<0>(str.c_str());

	// Set light data
	_prop._light.dir_cnt = 1; // Supported only 1
	_prop._light.point_cnt = 0; // Not supported now

	// Set height data
	auto height_data_obj = RJH::getObject(d, "height_data");
	if (RJH::isExists(height_data_obj->value, "grass"))
	{
		auto grass_obj = RJH::getObject(height_data_obj->value, "grass");
		_grass_prop._rate = RJH::getInt(grass_obj, "rate", 1);
		_grass_prop._tile_count_coef = RJH::getInt(grass_obj, "tile_count_coef", 0);
		_grass_prop._shift = RJH::getFloat(grass_obj, "shift", 0.f);
		_grass_prop._speed = RJH::getFloat(grass_obj, "speed", 0.f);
		_grass_prop._size_min = RJH::getFloat(grass_obj, "size_min", 1.f);
		_grass_prop._size_max = RJH::getFloat(grass_obj, "size_max", 1.f);
		_grass_prop._is_shadow = RJH::getBool(grass_obj, "is_shadow") && _shdw.size();
	}
	_prop._chunk_count_h = RJH::getInt(height_data_obj, "lod_chunk_count_w_h");
	_prop._chunk_count_w = _prop._chunk_count_h;
	_prop._chunk_size = RJH::getInt(height_data_obj, "lod_chunk_size");
	
	// Set layers data
	auto layers_arr = RJH::getArray(height_data_obj->value, "layers");
	int num_layer = 0;
#if LAYER_TEXTURE_SIZE == 4
	const char* text_name[LAYER_TEXTURE_SIZE] = { "text_a", "text_b", "text_g", "text_r" };
#elif LAYER_TEXTURE_SIZE == 3
		const char* text_name[LAYER_TEXTURE_SIZE] = { "text_a", "text_b", "text_g" };
#elif LAYER_TEXTURE_SIZE == 2
	const char* text_name[LAYER_TEXTURE_SIZE] = { "text_a", "text_b" };
#elif LAYER_TEXTURE_SIZE == 1
	const char* text_name[LAYER_TEXTURE_SIZE] = { "text_a" };
#endif
	for (auto layer_it = RJH::begin(layers_arr); layer_it != RJH::end(layers_arr) && num_layer < MAX_LAYER_COUNT; ++layer_it, ++num_layer)
	{
		HM_PROPERTY::LAYER layer;
		layer._dist = RJH::getFloat(layer_it, "dist_view", 0.f);
		for (int i = 0; i < LAYER_TEXTURE_SIZE; ++i)
		{
			auto text_obj = RJH::getObject(layer_it, rapidjson::Value::StringRefType(text_name[i]));
			layer._text.at(i).diffuse = RJH::getString(text_obj->value, "diffuse", "");
			layer._text.at(i).normal = RJH::getString(text_obj->value, "normal", "");
			layer._text.at(i).first_size = RJH::getFloat(text_obj, "first_lod_size", 0.f);
			layer._text.at(i).scale_size_coef = RJH::getFloat(text_obj, "scale_size_coef", 0.f);
			layer._text.at(i).normal_map_scale = RJH::getFloat(text_obj, "normal_map_scale", 0.f);
			layer._text.at(i).specular_factor = RJH::getFloat(text_obj, "specular_factor", 0.f);
		}
		_prop._layers.push_back(layer);
	}

	_prop._scale = Vec3(RJH::getFloat(height_data_obj, "lod_scale_x_z"), RJH::getFloat(height_data_obj, "lod_scale_y"), RJH::getFloat(height_data_obj, "lod_scale_x_z"));
	_prop._is_normal_map = RJH::getBool(height_data_obj, "normal_map_en");
	_prop._darker_dist = RJH::getFloat(height_data_obj, "darker_dist", 0.f);

	// Set load data
	auto lod_data_arr = RJH::getArray(height_data_obj->value, "lod_data");
	for (auto ld_it = RJH::begin(lod_data_arr); ld_it != RJH::end(lod_data_arr); ++ld_it)
	{
		HM_PROPERTY::LOD_DATA data;
		data.width = RJH::getInt(ld_it, "w_h");
		data.height = data.width;
		data.text_lod_dist_from = RJH::getFloat(ld_it, "text_lod_dist_from");
		data.text_lod_dist_to = RJH::getFloat(ld_it, "text_lod_dist_to");
		data.is_shadow = RJH::getBool(ld_it, "is_shadow") && _shdw.size();
		_prop._lod_data.push_back(data);
	}
	_prop._lod_count = _prop._lod_data.size();
}

bool HeightMap::init()
{
	loadProps();

	_prop._width = _prop._chunk_count_w * _prop._chunk_size;
	_helper._half_width = _prop._width / 2;

	_prop._height = _prop._chunk_count_h * _prop._chunk_size;
	_helper._half_height = _prop._height / 2;

	// Create Level Lods
	CreateLodLevels();

	return true;
}

void HeightMap::CreateLodLevels()
{
	_lods = new LodHM(this, _prop._lod_count);
	_fLod = _lods->getFirstLod();
}

void HeightMap::updatePositionView(const cocos2d::Vec3& p)
{
	if (_mutex.try_lock())
	{
		// Update position
		_pos = Vec3(p.x * _is_need_update.x, p.y * _is_need_update.y, p.z * _is_need_update.z);

		// Update GL buffers
		updateLodsGl();

		_mutex.unlock();
	}
}

unsigned int HeightMap::getVertexCount() const
{
	return _lods->getvertexCount();
}

void HeightMap::updateHeightMap()
{
	_lods->updateVertexArray(_pos, _lods->levelMult());
}

void HeightMap::updateLodsGl()
{
	// Update grass GL buffers if first lod was updated and grass should be rendered
	if (((_lods->updateGLbuffer() >> 1) & 1) * bool(_programState))
		updateGrassGLbuffer();
}

float HeightMap::getXtoNorm(float x, int* i_word_cell) const
{
	int coef = int(x / (-_helper._half_width * _prop._scale.x)) + int(x / ((_prop._width - 1) * _prop._scale.x));
	x += ((_prop._width - 1) * _prop._scale.x) * coef;

	*i_word_cell = -coef;

	return x;
}

float HeightMap::getZtoNorm(float z, int* j_word_cell) const
{
	int coef = int(z / (-_helper._half_height * _prop._scale.z)) + int(z / ((_prop._height - 1) * _prop._scale.z));
	z += ((_prop._height - 1) * _prop._scale.z) * coef;

	*j_word_cell = -coef;

	return z;
}

int HeightMap::xToI(float norm_x) const
{
	return (_helper._half_width * _prop._scale.x + norm_x) / _prop._scale.x;
}

int HeightMap::zToJ(float norm_z) const
{
	return (_helper._half_height * _prop._scale.z + norm_z) / _prop._scale.z;
}

void HeightMap::addHeight(const OneChunk::ONE_HEIGHT& one, const cocos2d::Vec3& pos)
{
	int i_chunk;
	float x_norm = getXtoNorm(pos.x, &i_chunk);
	int i_loc = xToI(x_norm);

	int j_chunk;
	float z_norm = getZtoNorm(pos.z, &j_chunk);
	int j_loc = zToJ(z_norm);

	addHeight(one, i_chunk, j_chunk, i_loc, j_loc);
}

void HeightMap::addHeight(const OneChunk::ONE_HEIGHT& one, int i_chunk, int j_chunk, int i_loc, int j_loc)
{
	auto column = _hgt.getVal(j_chunk);
	if (column == &def_j_chunks)
	{
		column = new CenterArray<HeightChunk*, &def_i_chunk>();
		_hgt.addVal(column, j_chunk);
	}

	auto chunk = column->getVal(i_chunk);
	if (chunk == &def_i_chunk)
	{
		chunk = new HeightChunk(_prop._chunk_count_w, _prop._chunk_count_h, _prop._chunk_size);
		column->addVal(chunk, i_chunk);
	}

	chunk->addHeight(i_loc, j_loc, one);
}

float HeightMap::getHeight(float x_world, float z_world, cocos2d::Vec3* n) const
{
	// Coords of the closer up left vertex
	int i_chunk;
	float x_norm = getXtoNorm(x_world, &i_chunk);
	int i_loc = xToI(x_norm);
	if (i_chunk <= 0 && i_loc == 0)
	{ // FIX of HeightMap keeping issue
		i_loc = _prop._width - 1;
		i_chunk--;
	}

	int j_chunk;
	float z_norm = getZtoNorm(z_world, &j_chunk);
	int j_loc = zToJ(z_norm);
	if (j_chunk <= 0 && j_loc == 0)
	{ // FIX of HeightMap keeping issue
		j_loc = _prop._height - 1;
		j_chunk--;
	}
	
	float x1 = iToX(i_chunk, i_loc);
	float z1 = jToZ(j_chunk, j_loc);

	auto chunk = _hgt.getVal(j_chunk)->getVal(i_chunk);
	float y1 = chunk->getHeight(i_loc, j_loc).h * _prop._scale.y;

	// Coords of the down right vertex
	float x2 = x1 + (_prop._scale.x * _fLod->_helper._level_mult);
	float z2 = z1 + (_prop._scale.z * _fLod->_helper._level_mult);
	x_norm = getXtoNorm(x2, &i_chunk);
	i_loc = xToI(x_norm);
	z_norm = getZtoNorm(z2, &j_chunk);
	j_loc = zToJ(z_norm);

	chunk = _hgt.getVal(j_chunk)->getVal(i_chunk);
	float y2 = chunk->getHeight(i_loc, j_loc).h * _prop._scale.y;

	// Ratios
	float k = (z2 - z1) / (x2 - x1);
	float b = z1 - k * x1;

	// Test if target point is under or over the graphic line
	float z0 = k * x_world + b;

	float x3 = 0.0;
	float y3 = 0.0;
	float z3 = 0.0;
	float polar = 1.0;
	if (z0 > z_world)
	{
		// The target point is under the line
		x3 = x1 + (_prop._scale.x * _fLod->_helper._level_mult);
		z3 = z1;

		x_norm = getXtoNorm(x3, &i_chunk);
		i_loc = xToI(x_norm);
		z_norm = getZtoNorm(z3, &j_chunk);
		j_loc = zToJ(z_norm);

		chunk = _hgt.getVal(j_chunk)->getVal(i_chunk);

		y3 = chunk->getHeight(i_loc, j_loc).h * _prop._scale.y;

		polar = -1.0;
	}
	else
	{
		// The target point is over the line
		x3 = x1;
		z3 = z1 + (_prop._scale.z * _fLod->_helper._level_mult);
		
		x_norm = getXtoNorm(x3, &i_chunk);
		i_loc = xToI(x_norm);
		z_norm = getZtoNorm(z3, &j_chunk);
		j_loc = zToJ(z_norm);

		chunk = _hgt.getVal(j_chunk)->getVal(i_chunk);

		y3 = chunk->getHeight(i_loc, j_loc).h * _prop._scale.y;
	}

	// Normal counting
	*n = cocos2d::Vec3(x3 - x1, y3 - y1, z3 - z1) * polar;
	cocos2d::Vec3 v2 = cocos2d::Vec3(x2 - x1, y2 - y1, z2 - z1);
	n->cross(v2);
	n->normalize();

	// D ratio in the plane equation Ax+By+Cz+D=0
	float D = -1 * (n->x * x1 + n->y * y1 + n->z * z1);

	// Counting the real height
	float y_r = -1 * (D + n->z * z_world + n->x * x_world) / n->y;

	return y_r;
}

bool HeightMap::getHitResult(const Vec2& location_in_view, const cocos2d::Camera* cam, cocos2d::Vec3* res)
{
	cocos2d::Vec3 nearP(location_in_view.x, location_in_view.y, 0), farP(location_in_view.x, location_in_view.y, 1.0f);
	auto size = cocos2d::Director::getInstance()->getWinSize();
	// Start point
	nearP = cam->unproject(nearP);
	// End point
	farP = cam->unproject(farP);

	return this->_lods->rayCast(nearP, farP, res);
}

cocos2d::Vec3 HeightMap::getVector(int i1, int j1, int i2, int j2) const
{
	float x1 = (float(i1) - _helper._half_width) * _prop._scale.x;
	float z1 = (float(j1) - _helper._half_height) * _prop._scale.z;
	float y1 = _hgt.getVal(0)->getVal(0)->getHeight(i1, j1).h * _prop._scale.y;

	float x2 = (float(i2) - _helper._half_width) * _prop._scale.x;
	float z2 = (float(j2) - _helper._half_height) * _prop._scale.z;
	float y2 = _hgt.getVal(0)->getVal(0)->getHeight(i2, j2).h * _prop._scale.y;

	return cocos2d::Vec3(x2 - x1, y2 - y1, z2 - z1);
}

void HeightMap::getNormalForVertex(float x, float z, int lev_mult, OneChunk::ONE_HEIGHT::NORMAL* ret_n)
{
	int i_chunk, j_chunk;

	// Down
	float x1 = x;
	float norm_x = getXtoNorm(x1, &i_chunk);
	int i = xToI(norm_x);
	float z1 = z - _prop._scale.z * lev_mult;
	float norm_z = getZtoNorm(z1, &j_chunk);
	int j = zToJ(norm_z);
	float y_down = _hgt.getVal(j_chunk)->getVal(i_chunk)->getHeight(i, j).h;

	// Up
	z1 = z + _prop._scale.z * lev_mult;
	norm_z = getZtoNorm(z1, &j_chunk);
	j = zToJ(norm_z);
	float y_up = _hgt.getVal(j_chunk)->getVal(i_chunk)->getHeight(i, j).h;

	// Vertical
	Vec3 vert = Vec3(0.f, (y_up - y_down) * _prop._scale.y * NORMAL_SLOPE, _prop._scale.z * lev_mult);

	// Left
	x1 = x - _prop._scale.x * lev_mult;
	norm_x = getXtoNorm(x1, &i_chunk);
	i = xToI(norm_x);
	z1 = z;
	norm_z = getZtoNorm(z1, &j_chunk);
	j = zToJ(norm_z);
	float y_left = _hgt.getVal(j_chunk)->getVal(i_chunk)->getHeight(i, j).h;

	// Right
	x1 = x + _prop._scale.x * lev_mult;
	norm_x = getXtoNorm(x1, &i_chunk);
	i = xToI(norm_x);
	float y_right = _hgt.getVal(j_chunk)->getVal(i_chunk)->getHeight(i, j).h;

	// Gorizontal
	Vec3 gor = Vec3(_prop._scale.x * lev_mult, (y_right - y_left) * _prop._scale.y * NORMAL_SLOPE, 0.f);

	// Normal
	Vec3 n;
	cocos2d::Vec3::cross(vert, gor, &n);
	n.normalize();

	// Pack normal
	ret_n->x = char(n.x * 127) + 128;
	ret_n->y = char(n.y * 127) + 128;
	ret_n->z = char(n.z * 127) + 128;
}

void HeightMap::draw(cocos2d::Renderer* renderer, const cocos2d::Mat4& transform, uint32_t flags)
{
	_lods->updateSkyLightData();
	_lods->drawLandScape(renderer, transform, flags);

	if (_programState)
		drawGrass(renderer, transform, flags);
}

void HeightMap::updateHeights(const cocos2d::Vec3& p)
{
	updatePositionView(p);
}

void HeightMap::unloadTextures()
{
	if (_text_loaded)
	{
		// Unset textures for LodHM
		_lods->unsetTextures();

		// Remove textures
		for (int q = 0; q < _prop._layers.size(); ++q)
		{
			for (int i = 0; i < LAYER_TEXTURE_SIZE; ++i)
			{
				assert(_layerData[q]._text[i].diff->getReferenceCount() == 1 && _layerData[q]._text[i].diff->getBackendTexture()->getReferenceCount() == 1);
				CC_SAFE_RELEASE_NULL(_layerData[q]._text[i].diff);
			}
		}

		// Remove normal textures
		if (_prop._is_normal_map)
		{
			for (int q = 0; q < _prop._layers.size(); ++q)
			{
				for (int i = 0; i < LAYER_TEXTURE_SIZE; ++i)
				{
					assert(_layerData[q]._text[i].norm->getReferenceCount() == 1 && _layerData[q]._text[i].norm->getBackendTexture()->getReferenceCount() == 1);
					CC_SAFE_RELEASE_NULL(_layerData[q]._text[i].norm);
				}
			}
		}

		_text_loaded = false;
	}
}

void HeightMap::loadTextures()
{
	if (!_text_loaded)
	{
		// Texture parameters
		Texture2D::TexParams texPar;
		texPar.sAddressMode = cocos2d::backend::SamplerAddressMode::REPEAT;
		texPar.tAddressMode = cocos2d::backend::SamplerAddressMode::REPEAT;
		texPar.minFilter = cocos2d::backend::SamplerFilter::LINEAR_MIPMAP_LINEAR;
		texPar.magFilter = cocos2d::backend::SamplerFilter::LINEAR;

		for (int i = 0; i < _prop._layers.size(); ++i)
		{
			for (int j = 0; j < LAYER_TEXTURE_SIZE; ++j)
			{
				auto data = FileUtils::getInstance()->getDataFromFile(_prop._layers.at(i)._text.at(j).diffuse);
				assert(!data.isNull());

				Image* img = new Image();
				img->initWithImageData(data.getBytes(), data.getSize());
				_layerData[i]._text[j].diff = new Texture2D();
				_layerData[i]._text[j].diff->initWithImage(img, backend::PixelFormat::RGB888);
				_layerData[i]._text[j].diff->setTexParameters(texPar);
				_layerData[i]._text[j].diff->generateMipmap();

				delete img;

				if (_prop._is_normal_map)
				{
					auto data = FileUtils::getInstance()->getDataFromFile(_prop._layers.at(i)._text.at(j).normal);
					assert(!data.isNull());

					Image* img = new Image();
					img->initWithImageData(data.getBytes(), data.getSize());
					_layerData[i]._text[j].norm = new Texture2D();
					_layerData[i]._text[j].norm->initWithImage(img, backend::PixelFormat::RGB888);
					_layerData[i]._text[j].norm->setTexParameters(texPar);
					_layerData[i]._text[j].norm->generateMipmap();

					delete img;
				}
			}
		}

		// Set textures for LodHM
		_lods->setTextures();

		_text_loaded = true;
	}
}

void HeightMap::createGrassShader()
{
	Texture2D::TexParams texPar;
	texPar.sAddressMode = cocos2d::backend::SamplerAddressMode::CLAMP_TO_EDGE;
	texPar.tAddressMode = cocos2d::backend::SamplerAddressMode::CLAMP_TO_EDGE;
	texPar.minFilter = cocos2d::backend::SamplerFilter::LINEAR_MIPMAP_LINEAR;
	texPar.magFilter = cocos2d::backend::SamplerFilter::LINEAR;

	std::string vert_sh = FileUtils::getInstance()->getStringFromFile("res/shaders/grass_bb.vert");
	std::string fr_sh = FileUtils::getInstance()->getStringFromFile("res/shaders/grass_bb.frag");
	
	std::string def;
	if (_grass_prop._is_shadow)
		def += "#define SHADOW\n";

	auto program = backend::Device::getInstance()->newProgram(def + vert_sh, def + fr_sh);
	_programState = new backend::ProgramState(program);

	auto& pipelineDescriptor = _grass_gl._customCommand.getPipelineDescriptor();
	pipelineDescriptor.programState = _programState;
	auto layout = _programState->getVertexLayout();

	const auto& attributeInfo = _programState->getProgram()->getActiveAttributes();

	// Set attributes
	const auto& iter1 = attributeInfo.find("a_position");
	if (iter1 != attributeInfo.end())
		layout->setAttribute("a_position", iter1->second.location, backend::VertexFormat::FLOAT3, offsetof(GRASS_MODEL::ONEVERTEX_GRASS, i), false);

	const auto& iter2 = attributeInfo.find("a_color");
	if (iter2 != attributeInfo.end())
		layout->setAttribute("a_color", iter2->second.location, backend::VertexFormat::FLOAT2, offsetof(GRASS_MODEL::ONEVERTEX_GRASS, ratex), false);

	const auto& iter3 = attributeInfo.find("a_texCoord");
	if (iter3 != attributeInfo.end())
		layout->setAttribute("a_texCoord", iter3->second.location, backend::VertexFormat::FLOAT2, offsetof(GRASS_MODEL::ONEVERTEX_GRASS, tx), false);

	layout->setLayout(sizeof(GRASS_MODEL::ONEVERTEX_GRASS));

	// Get uniforms
	_grass_gl._upLoc = _programState->getUniformLocation("u_up");
	_grass_gl._rightLoc = _programState->getUniformLocation("u_right");
	_grass_gl._light_dirLoc = _programState->getUniformLocation("u_DirLight");
	_grass_gl._light_dirColLoc = _programState->getUniformLocation("u_DirColLight");
	_grass_gl._light_ambColLoc = _programState->getUniformLocation("u_AmbColLight");
	_grass_gl._timeLoc = _programState->getUniformLocation("u_time");
	_grass_gl._vpLoc = _programState->getUniformLocation("u_VPMatrix");
	_grass_gl._camPosLoc = _programState->getUniformLocation("u_cam_pos");

	if (_grass_prop._is_shadow)
	{
		_grass_gl._lVP_shadow_loc = _programState->getUniformLocation("u_lVP_sh");

		const auto& cameras = getShadowCameras();
		std::vector<Vec2> texel_size;
		std::vector<int> slots;
		std::vector<cocos2d::backend::TextureBackend*> txt;
		auto texels_loc = _programState->getUniformLocation("u_texelSize_sh");
		auto depth_loc = _programState->getUniformLocation("u_text_sh");
		int text_slot = 1; // 0 slot is under grass texture
		for (int j = 0; j < cameras.size(); ++j)
		{
			Size sz = cameras[j]->getRenderText()->getDepthText()->getContentSizeInPixels();
			texel_size.push_back(Vec2(sz.width, sz.height));

			slots.push_back(j + text_slot);
			txt.push_back(cameras[j]->getRenderText()->getDepthText()->getBackendTexture());
		}
		_programState->setUniform(texels_loc, texel_size.data(), sizeof(Vec2) * texel_size.size());
		_programState->setTextureArray(depth_loc, slots, txt);
	}

	// Set grass speed uniform
	auto speedLoc = _programState->getUniformLocation("u_speed");
	float speed = _grass_prop._speed;
	_programState->setUniform(speedLoc, &speed, sizeof(speed));

	// Set grass size
	auto sizeLoc = _programState->getUniformLocation("u_size_min");
	float size = _grass_prop._size_min;
	_programState->setUniform(sizeLoc, &size, sizeof(size));
	sizeLoc = _programState->getUniformLocation("u_size_max");
	size = _grass_prop._size_max;
	_programState->setUniform(sizeLoc, &size, sizeof(size));

	// Set scale uniform
	auto scaleLoc = _programState->getUniformLocation("u_scale");
	_programState->setUniform(scaleLoc, &_prop._scale, sizeof(Vec3));

	// Set alpha for distance uniform
	auto distLoc = _programState->getUniformLocation("u_dist_alpha");
	Vec2 dist_alpha = Vec2(int(_grass_prop._tile_count_coef / 2) - _prop._scale.x, int(_grass_prop._tile_count_coef / 2));
	_programState->setUniform(distLoc, &dist_alpha, sizeof(Vec2));
}

void HeightMap::createGrassBuffers()
{
	// Vertex arrays
	auto gr_count = (_grass_prop._tile_count_coef * 2 + 1) * (_grass_prop._tile_count_coef * 2 + 1);
    if (gr_count > _fLod->_w * _fLod->_h)
	    gr_count = _fLod->_w * _fLod->_h;
	_gVert.resize(gr_count * _grass_prop._rate);
	assert(_gVert.size() * 4 < 0xffff); // Since the _gInd type is CustomCommand::IndexFormat::U_SHORT (Otherwise do it as CustomCommand::IndexFormat::U_INT + the inner type of _gInd as 'int')

	// Index array
	for (int i = 0, j = 0; i < _gVert.size() * 4; i += 4, ++j)
	{
		// First tris
		_gInd.push_back(i);
		_gInd.push_back(i + 3);
		_gInd.push_back(i + 1);
		// Second tris
		_gInd.push_back(i + 3);
		_gInd.push_back(i + 2);
		_gInd.push_back(i + 1);
	}
	int last = _gInd.at(_gInd.size() - 1);

	_grass_gl._customCommand.createVertexBuffer(sizeof(GRASS_MODEL), _gVert.size(), CustomCommand::BufferUsage::DYNAMIC);
	_grass_gl._customCommand.createIndexBuffer(CustomCommand::IndexFormat::U_SHORT, _gInd.size(), CustomCommand::BufferUsage::STATIC);

	_grass_gl._customCommand.updateVertexBuffer(_gVert.data(), _gVert.size() * sizeof(GRASS_MODEL));
	_grass_gl._customCommand.updateIndexBuffer(_gInd.data(), _gInd.size() * sizeof(unsigned short));

	_grass_gl._customCommand.setTransparent(true);
	_grass_gl._customCommand.set3D(true);

	_grass_gl._customCommand.setBeforeCallback(CC_CALLBACK_0(HeightMap::onBeforeDraw, this));
	_grass_gl._customCommand.setAfterCallback(CC_CALLBACK_0(HeightMap::onAfterDraw, this));

	// Set blending
	auto& blend = _grass_gl._customCommand.getPipelineDescriptor().blendDescriptor;
	blend.blendEnabled = true;
	blend.sourceRGBBlendFactor = cocos2d::backend::BlendFactor::SRC_ALPHA;
	blend.destinationRGBBlendFactor = cocos2d::backend::BlendFactor::ONE_MINUS_SRC_ALPHA;
	blend.sourceAlphaBlendFactor = cocos2d::backend::BlendFactor::SRC_ALPHA;
	blend.destinationAlphaBlendFactor = cocos2d::backend::BlendFactor::ONE_MINUS_SRC_ALPHA;
}

void HeightMap::setGrassText(const std::string& grass_text)
{
	if (_programState)
	{
		if (_grassText)
		{
			clearProgramStateTextures(_programState);
			assert(_grassText->getReferenceCount() == 1 && _grassText->getBackendTexture()->getReferenceCount() == 1);
			CC_SAFE_RELEASE_NULL(_grassText);
		}

		Texture2D::TexParams texPar;
		texPar.sAddressMode = cocos2d::backend::SamplerAddressMode::CLAMP_TO_EDGE;
		texPar.tAddressMode = cocos2d::backend::SamplerAddressMode::CLAMP_TO_EDGE;
		texPar.minFilter = cocos2d::backend::SamplerFilter::LINEAR_MIPMAP_LINEAR;
		texPar.magFilter = cocos2d::backend::SamplerFilter::LINEAR;

		Image* img = new Image();
		img->initWithImageFile(grass_text);
		_grassText = new Texture2D();
		_grassText->initWithImage(img, backend::PixelFormat::RGBA8888);
		_grassText->setTexParameters(texPar);
		_grassText->generateMipmap();
		delete img;

		auto textLoc = _programState->getUniformLocation("u_texture");
		_programState->setTexture(textLoc, 0, _grassText->getBackendTexture());
	}
}

void HeightMap::drawGrass(cocos2d::Renderer* renderer, const cocos2d::Mat4& transform, uint32_t flags)
{
	_grass_gl._customCommand.init(0);
	_grass_gl._customCommand.setIndexDrawInfo(0, _grass_gl._draw_mdl_cnt * 6);
	renderer->addCommand(&_grass_gl._customCommand);

	// Set atmosphere light direction
	_programState->setUniform(_grass_gl._light_dirLoc, &_prop._light.dirAtmLight, sizeof(_prop._light.dirAtmLight));
	
	// Set atmosphere light color
	_programState->setUniform(_grass_gl._light_dirColLoc, &_prop._light.colAtmLight, sizeof(_prop._light.colAtmLight));
	_programState->setUniform(_grass_gl._light_ambColLoc, &_prop._light.ambAtmLight, sizeof(_prop._light.ambAtmLight));

	auto camera = Camera::getVisitingCamera();

	// Set right and up direction of the camera
	Vec3 dir;
	camera->getNodeToWorldTransform().getUpVector(&dir);
	_programState->setUniform(_grass_gl._upLoc, &dir, sizeof(dir));
	
	camera->getNodeToWorldTransform().getRightVector(&dir);
	_programState->setUniform(_grass_gl._rightLoc, &dir, sizeof(dir));

	// Set VP
	const Mat4& view_proj_mat = camera->getViewProjectionMatrix();
	_programState->setUniform(_grass_gl._vpLoc, view_proj_mat.m, sizeof(view_proj_mat.m));

	//Set time
	_programState->setUniform(_grass_gl._timeLoc, &_grass_prop._time_passed, sizeof(_grass_prop._time_passed));

    // Set camera position
	Vec3 camPos = Camera::getVisitingCamera()->getPosition3D();
	_programState->setUniform(_grass_gl._camPosLoc, &camPos, sizeof(camPos));

	if (_grass_prop._is_shadow)
	{

		static const cocos2d::Mat4 bias(
			0.5f, 0.0f, 0.0f, 0.5f,
			0.0f, 0.5f, 0.0f, 0.5f,
			0.0f, 0.0f, 0.5f, 0.5f,
			0.0f, 0.0f, 0.0f, 1.0f
		);

		const auto& shadow_cameras = getShadowCameras();
		std::vector<Mat4> lightPos;
		for (int j = 0; j < shadow_cameras.size(); ++j)
		{
			auto lp = bias * shadow_cameras[j]->getViewProjectionMatrix();
			lightPos.push_back(lp);
		}
		_programState->setUniform(_grass_gl._lVP_shadow_loc, lightPos.data(), sizeof(Mat4) * lightPos.size());
	}
}

void HeightMap::enableGrass()
{
	if (_programState)
		return; // The grass is rendering already
	// Create grass gl buffers and shader only for the first layer
	createGrassShader();
	createGrassBuffers();
}

void HeightMap::disableGrass()
{
	if (_programState)
	{
		clearProgramStateTextures(_programState);

		assert(_programState->getReferenceCount() == 1);
		CC_SAFE_RELEASE_NULL(_programState);

		if (_grassText)
		{
			assert(_grassText->getReferenceCount() == 1 && _grassText->getBackendTexture()->getReferenceCount() == 1);
			CC_SAFE_RELEASE_NULL(_grassText);
		}

		_gVert.clear();
		_gInd.clear();
	}
}

void HeightMap::updateGrassGLbuffer()
{
	_grass_gl._draw_mdl_cnt = _grass_gl._mdl_cnt;
	_grass_gl._customCommand.updateVertexBuffer(_gVert.data(), 0, sizeof(GRASS_MODEL) * _grass_gl._draw_mdl_cnt);
}

const OneChunk::ONE_HEIGHT& HeightMap::getTileData(float x_w, float z_w, std::pair<unsigned short, unsigned short>& _loc_idx)
{
	int j_chunk;
	float z_loc = getZtoNorm(z_w, &j_chunk);
	_loc_idx.second = zToJ(z_loc);

	int i_chunk;
	float x_loc = getXtoNorm(x_w, &i_chunk);
	_loc_idx.first = xToI(x_loc);

	return _hgt.getVal(j_chunk)->getVal(i_chunk)->getHeight(_loc_idx.first, _loc_idx.second);
}

void HeightMap::setCurrentChunk(int j_chunk, int i_chunk)
{
	if ((_i_cur_chunk ^ i_chunk) + (_j_cur_chunk ^ j_chunk) != 0)
	{
		_i_cur_chunk = i_chunk;
		_j_cur_chunk = j_chunk;

		CHUNK_PTR(_j_cur_chunk - 1, _i_cur_chunk - 1) = _hgt.getVal(_j_cur_chunk - 1)->getVal(_i_cur_chunk - 1);
		CHUNK_PTR(_j_cur_chunk - 1, _i_cur_chunk) = _hgt.getVal(_j_cur_chunk - 1)->getVal(_i_cur_chunk);
		CHUNK_PTR(_j_cur_chunk - 1, _i_cur_chunk + 1) = _hgt.getVal(_j_cur_chunk - 1)->getVal(_i_cur_chunk + 1);
		CHUNK_PTR(_j_cur_chunk, _i_cur_chunk - 1) = _hgt.getVal(_j_cur_chunk)->getVal(_i_cur_chunk - 1);
		CHUNK_PTR(_j_cur_chunk, _i_cur_chunk) = _hgt.getVal(_j_cur_chunk)->getVal(_i_cur_chunk);
		CHUNK_PTR(_j_cur_chunk, _i_cur_chunk + 1) = _hgt.getVal(_j_cur_chunk)->getVal(_i_cur_chunk + 1);
		CHUNK_PTR(_j_cur_chunk + 1, _i_cur_chunk - 1) = _hgt.getVal(_j_cur_chunk + 1)->getVal(_i_cur_chunk - 1);
		CHUNK_PTR(_j_cur_chunk + 1, _i_cur_chunk) = _hgt.getVal(_j_cur_chunk + 1)->getVal(_i_cur_chunk);
		CHUNK_PTR(_j_cur_chunk + 1, _i_cur_chunk + 1) = _hgt.getVal(_j_cur_chunk + 1)->getVal(_i_cur_chunk + 1);
	}
}

void HeightMap::loadHeightsFromImage(int i_chunk, int j_chunk, const std::string& height_map)
{
	Image img;
	img.initWithImageFile(height_map);
	
	int bit_pp = img.getBitPerPixel() / 8;

	auto data_h = img.getData();
    auto size = img.getDataLen() / bit_pp;
	assert(size == img.getWidth() * img.getHeight());

	for (int i = 0, j = 0; j < size; i += bit_pp, j += 1)
	{
		// Set heights
		OneChunk::ONE_HEIGHT one;
		one.h = data_h[i] + ONE_HEIGHT_DEF_H;
		if (one.h != ONE_HEIGHT_DEF_H)
		{
			int i_loc = j % img.getWidth();
			int j_loc = j / img.getWidth();

			addHeight(one, i_chunk, j_chunk, i_loc, j_loc);
		}
	}
}

void HeightMap::addChunk(int j_chunk, int i_chunk, int chunk_count_w, int chunk_count_h, int chunk_size)
{
	auto line = _hgt.getVal(j_chunk);

	if (line == &def_j_chunks)
	{
		line = new CenterArray<HeightChunk*, & def_i_chunk>();
		_hgt.addVal(line, j_chunk);
	}

	if (line->getVal(i_chunk) == &def_i_chunk)
	{
		// Add Chunk
		HeightChunk* c = new HeightChunk(chunk_count_w, chunk_count_h, chunk_size);
		line->addVal(c, i_chunk);
	}
}

void HeightMap::launchUpdThread()
{
	_thr_var = true;
	_thr_upd = std::thread([this]() {
		while (_thr_var)
		{
			if (_mutex.try_lock())
			{
				updateHeightMap();
				_mutex.unlock();
			}
		}
	});
}

void HeightMap::stopUpdThread()
{
	if (_thr_upd.joinable())
	{
		_thr_var = false;
		_thr_upd.join();
	}
}

void HeightMap::loadHeightsFromFile(const std::string& path, LOAD_HEIGHTS_MODE load_mode, unsigned int layer_color_num)
{
	if (load_mode == LOAD_HEIGHTS_MODE::LOAD_LAYER_COLOR && layer_color_num >= _prop._layers.size())
		return;

	_mutex.lock();

	MeshDatas* meshdatas = new (std::nothrow) MeshDatas();
	MaterialDatas* materialdatas = new (std::nothrow) MaterialDatas();
	NodeDatas* nodeDatas = new (std::nothrow) NodeDatas();

	struct VERTEX
	{
		Vec3 pos;
		Vec3 normal;
		struct vec4
		{
			float x = 0.f; float y = 0.f; float z = 0.f; float w = 0.f;
		} layer_color[];
	};

	if (loadFromFile(path, nodeDatas, meshdatas, materialdatas))
	{		
		for (auto* mesh : meshdatas->meshDatas)
		{
			int perVertexSize = mesh->getPerVertexSize();
			assert(perVertexSize % sizeof(float) == 0);
			perVertexSize /= sizeof(float);
			int vert_num = mesh->vertex.size() / perVertexSize;
			
			int layer_color_count = std::min(size_t((perVertexSize - 6) / 4), _prop._layers.size());

			_prop._load_scale = std::abs(*mesh->vertex.data() - *(mesh->vertex.data() + perVertexSize));
			if (_prop._load_scale == 0.f)
				_prop._load_scale = std::abs(*mesh->vertex.data() - *(mesh->vertex.data() + perVertexSize * 2));
			float sc_to_1 = 1.f / _prop._load_scale;

			const Mat4& tr = getTransform(nodeDatas->nodes, mesh);
			for (int i = 0; i < mesh->vertex.size(); i += perVertexSize)
			{
				VERTEX* vertex = reinterpret_cast<VERTEX*>(mesh->vertex.data() + i);
				Vec3 p_world = vertex->pos;
				tr.transformPoint(&p_world);
				p_world *= sc_to_1;
				p_world.y += ONE_HEIGHT_DEF_H;

				p_world.x = std::round(p_world.x);
				p_world.z = std::round(p_world.z);

				p_world.x *= _prop._scale.x; p_world.z *= _prop._scale.z;

				std::array<unsigned int, MAX_LAYER_COUNT> alpha;
				alpha.fill(ONE_HEIGHT_DEF_ALPHA);

				bool is_color_def = true;
				for (int j = 0; j < layer_color_count; ++j)
				{
					Vec4 color;
					color.x = vertex->layer_color[j].x;
					color.y = vertex->layer_color[j].y;
					color.z = vertex->layer_color[j].z;
					color.w = vertex->layer_color[j].w;
					alpha[j] = color_to_alpha(color);
					if (alpha[j] != ONE_HEIGHT_DEF_ALPHA)
						is_color_def = false;
				}

				if (std::abs(p_world.y - ONE_HEIGHT_DEF_H) > 0.000001 || !is_color_def)
				{
					int i_chunk = 0, j_chunk = 0, i_loc = 0, j_loc = 0;

					TILE_LINK(p_world, &i_chunk, &j_chunk, &i_loc, &j_loc);

					int is_def = 0;
					OneChunk::ONE_HEIGHT one = _hgt.getVal(j_chunk)->getVal(i_chunk)->getHeight(i_loc, j_loc, &is_def);

					if (load_mode == LOAD_HEIGHTS_MODE::LOAD_LAYER_COLOR && !is_color_def)
					{
						one.lay[layer_color_num].alpha = alpha[0];
					}

					if (load_mode == LOAD_HEIGHTS_MODE::LOAD_HEIGHTS)
					{
						one.h = p_world.y;
						for (int j = 0; j < layer_color_count; ++j)
							one.lay[j].alpha = alpha[j];
					}

					addHeight(one, i_chunk, j_chunk, i_loc, j_loc);
				}
			}
		}
	}
	CC_SAFE_DELETE(meshdatas);
	CC_SAFE_DELETE(materialdatas);
	CC_SAFE_DELETE(nodeDatas);

	_mutex.unlock();
}

void HeightMap::loadHeightsFromFile_(const std::string& path, const std::vector<std::string>& layer_text)
{
	_mutex.lock();

	int layer_color_count = std::min(layer_text.size(), _prop._layers.size());

	struct LAYER_INFO
	{
		int bit_pp = 0;
		Image img;
		unsigned char* d = nullptr;
		unsigned int w = 0;
		unsigned int h = 0;
	};
	std::array<LAYER_INFO, MAX_LAYER_COUNT> layers;

	// Init layers
	for (int i = 0; i < layer_color_count; ++i)
	{
		LAYER_INFO& data = layers[i];

		data.img.initWithImageFile(layer_text.at(i));
		data.bit_pp = data.img.getBitPerPixel() / 8;
		data.d = data.img.getData();
		data.w = data.img.getWidth();
		data.h = data.img.getHeight();

		auto size = data.img.getDataLen() / data.bit_pp;
		assert(size == data.w * data.h);
	}

	MeshDatas* meshdatas = new (std::nothrow) MeshDatas();
	MaterialDatas* materialdatas = new (std::nothrow) MaterialDatas();
	NodeDatas* nodeDatas = new (std::nothrow) NodeDatas();

	struct VERTEX
	{
		Vec3 pos;
		Vec3 normal;
		Vec2 uv[];
	};

	if (loadFromFile(path, nodeDatas, meshdatas, materialdatas))
	{
		for (auto* mesh : meshdatas->meshDatas)
		{
			int perVertexSize = mesh->getPerVertexSize();
			assert(perVertexSize % sizeof(float) == 0);
			perVertexSize /= sizeof(float);
			int vert_num = mesh->vertex.size() / perVertexSize;

			int layer_color_count_in_file = (perVertexSize - 6) / 2;
			layer_color_count = std::min(layer_color_count, layer_color_count_in_file);

			_prop._load_scale = std::abs(*mesh->vertex.data() - *(mesh->vertex.data() + perVertexSize));
			if (_prop._load_scale == 0.f)
				_prop._load_scale = std::abs(*mesh->vertex.data() - *(mesh->vertex.data() + perVertexSize * 2));
			float sc_to_1 = 1.f / _prop._load_scale;

			const Mat4& tr = getTransform(nodeDatas->nodes, mesh);
			for (int i = 0; i < mesh->vertex.size(); i += perVertexSize)
			{
				VERTEX* vertex = reinterpret_cast<VERTEX*>(mesh->vertex.data() + i);
				Vec3 p_world = vertex->pos;
				tr.transformPoint(&p_world);
				p_world *= sc_to_1;
				p_world.y += ONE_HEIGHT_DEF_H;

				p_world.x = std::round(p_world.x);
				p_world.z = std::round(p_world.z);

				p_world.x *= _prop._scale.x; p_world.z *= _prop._scale.z;

				std::array<int, MAX_LAYER_COUNT> alpha;
				std::array<int, MAX_LAYER_COUNT> alpha_def;
				alpha_def.fill(0);
				alpha_def[0] = ONE_HEIGHT_DEF_ALPHA;

				bool is_color_def = true;
				for (int j = 0; j < layer_color_count; ++j)
				{
					int x = vertex->uv[j].x * layers[j].w;
					int y = (1.f - vertex->uv[j].y) * layers[j].h;
					unsigned char* pp = layers[j].d + (y * layers[j].w + x) * layers[j].bit_pp;
					Vec4 color = Vec4(pp[0] / 255.f, pp[1] / 255.f, pp[2] / 255.f, 0.f);

					alpha[j] = color_to_alpha(color);
					if (alpha[j] != alpha_def[j])
						is_color_def = false;
				}

				if (std::abs(p_world.y - ONE_HEIGHT_DEF_H) > 0.000001 || !is_color_def)
				{
					int i_chunk = 0, j_chunk = 0, i_loc = 0, j_loc = 0;

					TILE_LINK(p_world, &i_chunk, &j_chunk, &i_loc, &j_loc);

					int is_def = 0;
					OneChunk::ONE_HEIGHT one = _hgt.getVal(j_chunk)->getVal(i_chunk)->getHeight(i_loc, j_loc, &is_def);

					one.h = p_world.y;
					for (int j = 0; j < layer_color_count; ++j)
						one.lay[j].alpha = alpha[j];

					addHeight(one, i_chunk, j_chunk, i_loc, j_loc);
				}
			}
		}
	}
	CC_SAFE_DELETE(meshdatas);
	CC_SAFE_DELETE(materialdatas);
	CC_SAFE_DELETE(nodeDatas);

	_mutex.unlock();
}

void HeightMap::loadHeightsFromFile(const std::string& path, const std::vector<std::string>& layer_text)
{
	_mutex.lock();

	int layer_color_count = std::min(layer_text.size(), _prop._layers.size());

	struct LAYER_INFO
	{
		int bit_pp = 0;
		Image img;
		unsigned char* d = nullptr;
		unsigned int w = 0;
		unsigned int h = 0;
	};
	std::array<LAYER_INFO, MAX_LAYER_COUNT> layers;

	// Init layers
	for (int i = 0; i < layer_color_count; ++i)
	{
		LAYER_INFO& data = layers[i];

		data.img.initWithImageFile(layer_text.at(i));
		data.bit_pp = data.img.getBitPerPixel() / 8;
		assert(data.bit_pp == 4); // Alpha is needed
		data.d = data.img.getData();
		data.w = data.img.getWidth();
		data.h = data.img.getHeight();

		auto size = data.img.getDataLen() / data.bit_pp;
		assert(size == data.w * data.h);
	}

	MeshDatas* meshdatas = new (std::nothrow) MeshDatas();
	MaterialDatas* materialdatas = new (std::nothrow) MaterialDatas();
	NodeDatas* nodeDatas = new (std::nothrow) NodeDatas();

	struct VERTEX
	{
		Vec3 pos;
		Vec3 normal;
	};

	if (loadFromFile(path, nodeDatas, meshdatas, materialdatas))
	{
		Vec3 first_pos = reinterpret_cast<VERTEX*>(meshdatas->meshDatas.at(0)->vertex.data())->pos;
		const Mat4& tr = getTransform(nodeDatas->nodes, meshdatas->meshDatas.at(0));
		tr.transformPoint(&first_pos);

		float x_min = first_pos.x, z_min = first_pos.z;
		float x_max = x_min, z_max = z_min;

		// First pass: Searching of the min, max coords for the uv definishion
		for (auto* mesh : meshdatas->meshDatas)
		{
			int perVertexSize = mesh->getPerVertexSize();
			assert(perVertexSize % sizeof(float) == 0);
			perVertexSize /= sizeof(float);
			const Mat4& tr = getTransform(nodeDatas->nodes, mesh);

			for (int i = 0; i < mesh->vertex.size(); i += perVertexSize)
			{
				VERTEX* vertex = reinterpret_cast<VERTEX*>(mesh->vertex.data() + i);
				Vec3 p_world = vertex->pos;
				tr.transformPoint(&p_world);
				if (p_world.x < x_min)
					x_min = p_world.x;
				if (p_world.x > x_max)
					x_max = p_world.x;
				if (p_world.z < z_min)
					z_min = p_world.z;
				if (p_world.z > z_max)
					z_max = p_world.z;
			}
		}

		// Second pass: building of the grid
		for (auto* mesh : meshdatas->meshDatas)
		{
			int perVertexSize = mesh->getPerVertexSize();
			assert(perVertexSize % sizeof(float) == 0);
			perVertexSize /= sizeof(float);

			_prop._load_scale = std::abs(*mesh->vertex.data() - *(mesh->vertex.data() + perVertexSize));
			if (_prop._load_scale == 0.f)
				_prop._load_scale = std::abs(*mesh->vertex.data() - *(mesh->vertex.data() + perVertexSize * 2));
			float sc_to_1 = 1.f / _prop._load_scale;

			const Mat4& tr = getTransform(nodeDatas->nodes, mesh);
			for (int i = 0; i < mesh->vertex.size(); i += perVertexSize)
			{
				VERTEX* vertex = reinterpret_cast<VERTEX*>(mesh->vertex.data() + i);
				Vec3 p_world = vertex->pos;
				tr.transformPoint(&p_world);

				Vec2 uv;
				uv.x = (p_world.x - x_min) / (x_max - x_min);
				uv.y = (p_world.z - z_min) / (z_max - z_min);
				assert(uv.x >= 0.f && uv.x <= 1.f && uv.y >= 0.f && uv.y <= 1.f);

				p_world *= sc_to_1;
				p_world.y += ONE_HEIGHT_DEF_H;

				p_world.x = std::round(p_world.x);
				p_world.z = std::round(p_world.z);

				p_world.x *= _prop._scale.x; p_world.z *= _prop._scale.z;

				std::array<int, MAX_LAYER_COUNT> alpha;
				std::array<int, MAX_LAYER_COUNT> alpha_def;
				alpha_def.fill(0);
				alpha_def[0] = ONE_HEIGHT_DEF_ALPHA;

				bool is_color_def = true;
				for (int j = 0; j < layer_color_count; ++j)
				{
					int x = uv.x * (layers[j].w - 1);
					int y = uv.y * (layers[j].h - 1);
					unsigned char* pp = layers[j].d + (y * layers[j].w + x) * layers[j].bit_pp;
					Vec4 color = Vec4(pp[0] / 255.f, pp[1] / 255.f, pp[2] / 255.f, pp[3] / 255.f);

					alpha[j] = color_to_alpha(color);
					if (alpha[j] != alpha_def[j])
						is_color_def = false;
				}

				if (std::abs(p_world.y - ONE_HEIGHT_DEF_H) > 0.000001 || !is_color_def)
				{
					int i_chunk = 0, j_chunk = 0, i_loc = 0, j_loc = 0;

					TILE_LINK(p_world, &i_chunk, &j_chunk, &i_loc, &j_loc);

					int is_def = 0;
					OneChunk::ONE_HEIGHT one = _hgt.getVal(j_chunk)->getVal(i_chunk)->getHeight(i_loc, j_loc, &is_def);

					one.h = p_world.y;
					for (int j = 0; j < layer_color_count; ++j)
						one.lay[j].alpha = alpha[j];

					addHeight(one, i_chunk, j_chunk, i_loc, j_loc);
				}
			}
		}
	}
	CC_SAFE_DELETE(meshdatas);
	CC_SAFE_DELETE(materialdatas);
	CC_SAFE_DELETE(nodeDatas);

	_mutex.unlock();
}

unsigned int HeightMap::color_to_alpha(Vec4& color)
{
	color.w = 1.f - color.w;

	typedef unsigned int uuiint;

	return (uuiint(0xff * color.w) << 24) |
		(uuiint(0xff * color.z) << 16) |
		(uuiint(0xff * color.y) << 8) |
			uuiint(0xff * color.x);
}

void HeightMap::clearProgramStateTextures(cocos2d::backend::ProgramState* programState)
{
	class ProgramStateExt : public cocos2d::backend::ProgramState
	{
	public:

		void clearTextures()
		{
			_vertexTextureInfos.clear();
			_fragmentTextureInfos.clear();
		}

	};

	ProgramStateExt* programStateExt = static_cast<ProgramStateExt*>(programState);
	programStateExt->clearTextures();
}

const Mat4& HeightMap::getTransform(std::vector<NodeData*> nodes, MeshData* mesh)
{
	for (auto* node : nodes)
	{
		for (int j = 0; j < node->modelNodeDatas.size(); ++j)
		{
			if (node->modelNodeDatas.at(j)->subMeshId == mesh->subMeshIds.at(0))
				return node->transform;
		}
	}
	assert(0);
}

void HeightMap::onBeforeDraw()
{
	auto renderer = Director::getInstance()->getRenderer();
	_rendererDepthTest = renderer->getDepthTest();
	_rendererDepthWrite = renderer->getDepthWrite();
	renderer->setDepthTest(true);
	renderer->setDepthWrite(true);
}

void HeightMap::onAfterDraw()
{
	auto renderer = Director::getInstance()->getRenderer();
	renderer->setDepthTest(_rendererDepthTest);
	renderer->setDepthWrite(_rendererDepthWrite);
}
