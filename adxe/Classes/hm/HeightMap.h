#ifndef _HEIGHT_MAP_H_
#define _HEIGHT_MAP_H_

#include "cocos2d.h"
#include <thread>

#include "LodHM.h"
#include "CenterArray.h"
#include "HeightChunk.h"

#define CHUNK_PTR(j, i) _chunk_arr[(_j_cur_chunk - j) * 3 + (_i_cur_chunk - i) + 4]
#define CHUNK_KEY(j_chunk, i_chunk) int((j_chunk << 16) | (i_chunk & 0xffff))

namespace cocos2d
{
	class Image;
}
namespace hm
{
	class ShadowCamera;

	extern HeightChunk def_i_chunk;
	extern CenterArray<HeightChunk*, &def_i_chunk> def_j_chunks;

	typedef long long llong;
#define KEY_MAP(i_chunk, j_chunk, i_loc, j_loc) llong( (llong(i_chunk)<<48) + ((llong(j_chunk)<<32)&0x0000ffff00000000) + ((llong(i_loc)<<16)&0x00000000ffff0000) + (llong(j_loc)&0x000000000000ffff) )
#define UNPACK(pack) I_CHUNK(pack), J_CHUNK(pack), I_LOC(pack), J_LOC(pack)
#define I_CHUNK(pack) short(int(pack>>48)&0x0000ffff)
#define J_CHUNK(pack) short(int(pack>>32)&0x0000ffff)
#define I_LOC(pack) short(int(pack>>16)&0x0000ffff)
#define J_LOC(pack) short(int(pack)&0x0000ffff)

	class HeightMap : public cocos2d::Sprite3D
	{

	public:
		// The model gl data about Lod
		struct LODGLDATA
		{
			LODGLDATA(GLuint v, GLuint i, unsigned short iCount) : vbo(v), ibo(i), iIndCount(iCount){}
			GLuint vbo = 0;
			GLuint ibo = 0;
			unsigned short iIndCount = 0;
		};

		// Load heights from file mode
		enum LOAD_HEIGHTS_MODE
		{
			LOAD_HEIGHTS,       // There will be loaded heights + first color layer if it is represented in the file
			LOAD_LAYER_COLOR    // There will be loaded layer color with the passed number as function parameter
		};

	private:

		// Textures were loaded
		bool _text_loaded = false;

		// Layer data
		struct LAYER_DATA
		{
			struct TEXTURE_DATA
			{
				cocos2d::Texture2D* diff;
				cocos2d::Texture2D* norm;
			} _text[LAYER_TEXTURE_SIZE];
		} _layerData[MAX_LAYER_COUNT];

		// Grass texture
		cocos2d::Texture2D* _grassText = nullptr;

		// One grass bush model
		struct GRASS_MODEL
		{
			struct ONEVERTEX_GRASS
			{
				float i = 0.0;
				float y = 0.0;
				float j = 0.0;
				float ratex = 0;
				float ratey = 0;
				float tx = 0;
				float ty = 0;
			} vert[4];
			GRASS_MODEL()
			{
				vert[0].tx = 0; vert[0].ty = 1;
				vert[1].tx = 0; vert[1].ty = 0;
				vert[2].tx = 1; vert[2].ty = 0;
				vert[3].tx = 1; vert[3].ty = 1;
			}
		};

		std::string _prop_file;

	public:

		// Height Map property
		struct HM_PROPERTY
		{
			float _load_scale = 0.f;      // Distance between sibling vertex while loading 
			bool _is_normal_map = false;  // Is normal map shuld be represented in the shader
			float _darker_dist = 0.f;     // Distance where ground became darker

			unsigned int _width = 1025;
			unsigned int _height = 1025;

			int _chunk_count_w = 41;
			int _chunk_count_h = 41;
			int _chunk_size = 25;
			int _lod_count = 3;

			cocos2d::Vec3 _scale = { 10.f, 1.f, 10.f };
			int _lod_factor_h = 0;
			int _lod_factor_w = 0;
			int _lod_init = -1;
			int _lod_rank = 1;

			struct LOD_DATA
			{
				int width = 0;
				int height = 0;
				float text_lod_dist_from = 0.f;
				float text_lod_dist_to = 0.f;
				bool is_shadow = false;
			};
			std::vector<LOD_DATA> _lod_data;

			struct LAYER
			{
				enum
				{
					RED,
					GREEN,
					BLUE,
					ALPHA
				};
				struct TEXTURE
				{
					std::string diffuse;
					std::string normal;
					float first_size = 0.f;
					float scale_size_coef = 0.f;
					float normal_map_scale = 0.f;
					float specular_factor = 0.f;
				};
				std::array<TEXTURE, LAYER_TEXTURE_SIZE> _text;
				float _dist = 0.f;
			};
			std::vector<LAYER> _layers;

			struct LIGHT
			{
				unsigned int dir_cnt = 0;
				unsigned int point_cnt = 0;
				cocos2d::Vec3 dirAtmLight;
				cocos2d::Vec3 colAtmLight;
				cocos2d::Vec3 ambAtmLight;
				cocos2d::Vec3 colFog;
				float fog_near_plane = 0.f;
				float fog_far_plane = 0.f;
			} _light;

		} _prop;

    private:

		// Heights data
		CenterArray<CenterArray<HeightChunk*, &def_i_chunk>*, &def_j_chunks> _hgt;

		// Helper data
		struct HELPER
		{
			int _bit_cnt_w = 10;
			int _bit_cnt_h = 10;
			int _half_width = 512;
			int _half_height = 512;
		} _helper;

		struct GRASS_PROPERTY
		{
			// The rate of the grass in one tile (the count of GRASS_MODEL in one tile)
			unsigned short _rate = 1;

			// Coefficient to calculate count of tiles where grass to be grown depend on the first lod center
			unsigned short _tile_count_coef = 0;

			// Buffer calculated grass model count
			unsigned int _mdl_cnt = 0;

			// Draw grass model count
			unsigned int _draw_mdl_cnt = 0;

			// Grass shift
			float _shift = 10.f;

			// Grass speed
			float _speed = 0.f;

			// Min grass size
			float _size_min = 1.f;

			// Max gass size
			float _size_max = 1.f;

			// Time passed
			float _time_passed = 0.f;

			// Is shadow rendering
			bool _is_shadow = false;
		} _grass_prop;

		struct GRASS_GL
		{
			cocos2d::CustomCommand _customCommand;

			cocos2d::backend::UniformLocation _upLoc;
			cocos2d::backend::UniformLocation _rightLoc;
			cocos2d::backend::UniformLocation _light_dirLoc;
			cocos2d::backend::UniformLocation _light_dirColLoc;
			cocos2d::backend::UniformLocation _light_ambColLoc;
			cocos2d::backend::UniformLocation _timeLoc;
			cocos2d::backend::UniformLocation _vpLoc;
			cocos2d::backend::UniformLocation _camPosLoc;
			cocos2d::backend::UniformLocation _lVP_shadow_loc;

			unsigned int _mdl_cnt = 0;
			unsigned int _draw_mdl_cnt = 0;
		} _grass_gl;

		// Viewer position
		cocos2d::Vec3 _pos = { 0.f, 0.f, 0.f };

		// Flag to check if there is no needed in HM updating along some axis
		cocos2d::Vec3 _is_need_update = { 1.f, 1.f, 1.f };

		// The last Lod object
		LodHM* _lods = 0;

		// The first lod object
		const LodHM* _fLod = 0;

		// Mutex for safe variable access
		std::mutex _mutex;

		// Thread object to update heights
		std::thread _thr_upd;

		// Varrible to control when thread should be stoped
		std::atomic<bool> _thr_var;

		// Grass vertex array
		std::vector<GRASS_MODEL> _gVert;

		// Grass index array
		std::vector<unsigned short> _gInd;
		
		// Array of around HeightChunk
		HeightChunk* _chunk_arr[9];

		// Shadow cameras
		std::vector<ShadowCamera*> _shdw;

		// Current chunk number
		int _j_cur_chunk = 0x7fffffff;
		int _i_cur_chunk = 0x7fffffff;

		bool _rendererDepthTest = false;
		bool _rendererDepthWrite = false;

	public:

		virtual ~HeightMap();

		virtual bool init() override;

		inline const OneChunk::ONE_HEIGHT& HEIGHT(int i_chunk, int j_chunk, int i_tile, int j_tile) const
		{
			return _hgt.getVal(j_chunk)->getVal(i_chunk)->getHeight(i_tile, j_tile);
		}

		// Get One tile (which is defined by closest grid vertex) height data according to the passed global point
		inline const OneChunk::ONE_HEIGHT& TILE(const cocos2d::Vec3& pos) const
		{
			int i_chunk;
			float x_norm = getXtoNorm(pos.x, &i_chunk);
			int i_loc = xToI(x_norm);

			int j_chunk;
			float z_norm = getZtoNorm(pos.z, &j_chunk);
			int j_loc = zToJ(z_norm);

			return HEIGHT(i_chunk, j_chunk, i_loc, j_loc);
		}

		// Tile position with respect to the scale base on tile link
		inline cocos2d::Vec3 TILE_POS(int i_chunk, int j_chunk, int i_loc, int j_loc) const
		{
			const auto& one = HEIGHT(i_chunk, j_chunk, i_loc, j_loc);
			return cocos2d::Vec3(iToX(i_chunk, i_loc), one.h, jToZ(j_chunk, j_loc));
		}

		// Tile position with respect to the scale base on some world 'pos'
		inline cocos2d::Vec3 TILE_POS(const cocos2d::Vec3& pos) const
		{
			int i_chunk;
			float x_norm = getXtoNorm(pos.x, &i_chunk);
			int i_loc = xToI(x_norm);
			if (i_chunk <= 0 && i_loc == 0)
			{ // FIX of HeightMap keeping issue
				i_loc = _prop._width - 1;
				i_chunk--;
			}

			int j_chunk;
			float z_norm = getZtoNorm(pos.z, &j_chunk);
			int j_loc = zToJ(z_norm);
			if (j_chunk <= 0 && j_loc == 0)
			{ // FIX of HeightMap keeping issue
				j_loc = _prop._height - 1;
				j_chunk--;
			}

			return TILE_POS(i_chunk, j_chunk, i_loc, j_loc);
		}

		// Adjust chunk and loc links along X axis
		inline void adjust_I_chunk_loc(int& chunk, int& loc)
		{
			if (chunk <= 0 && loc == 0)
			{ // FIX of HeightMap keeping issue
				loc = _prop._width - 1;
				chunk--;
			}
		}

		// Adjust chunk and loc links along Z axis
		inline void adjust_J_chunk_loc(int& chunk, int& loc)
		{
			if (chunk <= 0 && loc == 0)
			{ // FIX of HeightMap keeping issue
				loc = _prop._height - 1;
				chunk--;
			}
		}

		// Returns tile link with its position without heigth
		cocos2d::Vec3 TILE_LINK(const cocos2d::Vec3& pos, int* i_chunk, int* j_chunk, int* i_loc, int* j_loc)
		{
			float x_norm = getXtoNorm(pos.x, i_chunk);
			*i_loc = xToI(x_norm);
			if (*i_chunk <= 0 && *i_loc == 0)
			{ // FIX of HeightMap keeping issue
				*i_loc = _prop._width - 1;
				(*i_chunk)--;
			}

			float z_norm = getZtoNorm(pos.z, j_chunk);
			*j_loc = zToJ(z_norm);
			if (*j_chunk <= 0 && *j_loc == 0)
			{ // FIX of HeightMap keeping issue
				*j_loc = _prop._height - 1;
				(*j_chunk)--;
			}

			return cocos2d::Vec3(iToX(*i_chunk, *i_loc), 0.f, jToZ(*j_chunk, *j_loc));
		}

		// Estimate of tile flatness along axis
		// Returns true if the tile is flat, and certain flatness along the corresponding X, Z axis
		inline bool TILE_FLATNESS(const cocos2d::Vec3& pos, int* x_est, int* z_est) const
		{
			const auto& one = TILE(pos);
			const auto& one_plus_x = TILE(cocos2d::Vec3(pos.x + _prop._scale.x, pos.y, pos.z));
			const auto& one_plus_z = TILE(cocos2d::Vec3(pos.x, pos.y, pos.z + _prop._scale.z));
			const auto& one_plus_xz = TILE(cocos2d::Vec3(pos.x + _prop._scale.x, pos.y, pos.z + _prop._scale.z));

			*x_est = (one_plus_x.h + one_plus_xz.h) - (one.h + one_plus_z.h);
			*z_est = (one_plus_z.h + one_plus_xz.h) - (one.h + one_plus_x.h);

			return !*x_est && !*z_est;
		}

		// Returns vertex count in all lod levels
		unsigned int getVertexCount() const;

		// Get x which is aligned to world center (norma)
		// i_word_cell - number of world cell along X axis
		inline float getXtoNorm(float x, int* i_word_cell) const;

		// Get y which is aligned to world center (norma)
		// j_word_cell - number of world cell along Z axis
		inline float getZtoNorm(float z, int* j_word_cell) const;

		// Norm x to i
		inline int xToI(float norm_x) const;

		// Norm z to j
		inline int zToJ(float norm_z) const;

		// Loc i to glob x
		inline float iToX(int chunk_i, int loc_i) const
		{
			return chunk_i * ((_prop._width - 1) * _prop._scale.x) + ((loc_i - _helper._half_width) * _prop._scale.x);
		}

		// Loc j to glob z
		inline float jToZ(int chunk_j, int loc_j) const
		{
			return chunk_j * ((_prop._height - 1) * _prop._scale.z) + ((loc_j - _helper._half_height) * _prop._scale.z);
		}

		// Returns the height of the ground according to the passed x and z
		float getHeight(float x, float z, cocos2d::Vec3* n) const;

		// Add one height
		// one - tile data
		// pos - some world position
		void addHeight(const OneChunk::ONE_HEIGHT& one, const cocos2d::Vec3& pos);
		// i_chunk, j_chunk, i_loc, j_loc - tile number
		void addHeight(const OneChunk::ONE_HEIGHT& one, int i_chunk, int j_chunk, int i_loc, int j_loc);

		// Show grid
		void showGrid(bool show)
		{
			_lods->showGrid(show);
			updateHeightMap();
		}

		// Show normals
		void showNormals(float size)
		{
			_lods->showNormals(size);
		}

		// Returns width
		unsigned int getWidth() const
		{
			return _prop._width;
		}

		// Return height
		unsigned int getHeight() const
		{
			return _prop._height;
		}

		// Returns 3D position of HeightMap according to the 2D touch and depend on the camera view angel
		bool getHitResult(const cocos2d::Vec2& location_in_view, const cocos2d::Camera* cam, cocos2d::Vec3* res);

		void forceUpdateHeightMap(unsigned char lev)
		{
			LodHM::_force_level_update = lev;
			std::this_thread::sleep_for(std::chrono::milliseconds(5));
		}

		void forceUpdateHeightMap(const cocos2d::Vec3& pos)
		{
			updateHeightMap();
			updateHeights(pos);
		}

		// Thread safe: Update pos value
		void updatePositionView(const cocos2d::Vec3& p);

		// Thread safe: Update verticies data regarding to the _pos value
		void updateHeightMap();

		// Returns scale of the HeightMap
		const cocos2d::Vec3& getScales() const
		{
			return _prop._scale;
		}

		// Update heights relate to the 'p'
		void updateHeights(const cocos2d::Vec3& p);

		// draw object
		virtual void draw(cocos2d::Renderer* renderer, const cocos2d::Mat4& transform, uint32_t flags) override;

		// Create HeightMap from Active LEVEL
		static HeightMap* create(const std::string& prop_file, const std::vector<ShadowCamera*>& shdw_cams);

		// Enable grass rendering
		void enableGrass();
		
		// Disable grass rendering
		void disableGrass();

		// Set Transparent Z mode LodHM grid
		void setGridModeZ(bool is_z_mode)
		{
			_lods->setGridModeZ(is_z_mode);
		}

		// Calculate normal for one vertex which belongs to Vec3(x, height, z)
		void getNormalForVertex(float x, float z, int lev_mult, OneChunk::ONE_HEIGHT::NORMAL* ret_n);

		// Lock HeightMap processing
		void lock()
		{
			_mutex.lock();
		}

		// Unlock HeightMap processing
		void unlock()
		{
			_mutex.unlock();
		}

		// Get tile info according to the (x, z)
		const OneChunk::ONE_HEIGHT& getTileData(float x_w, float z_w, std::pair<unsigned short, unsigned short>& _loc_idx);

		// Load heights from image for the certain HeightChunk
		void loadHeightsFromImage(int i_chunk, int j_chunk, const std::string& height_map);

		// Load heights from file *.c3b, alpha from vertex color (not more than 65536 verticies, otherwise fbx->c3b tool breaks vertex color)
		void loadHeightsFromFile(const std::string& path, LOAD_HEIGHTS_MODE load_mode = LOAD_HEIGHTS_MODE::LOAD_HEIGHTS, unsigned int layer_color_num = 0);

		// Load heights from file *.c3b, alpha from map textures (any count of verticies, since uv is calculated base vertex position)
		void loadHeightsFromFile(const std::string& path, const std::vector<std::string>& layer_text);

		// Load heights from file *.c3b, alpha from map textures with uv as attribute (not more than 65536 verticies, otherwise fbx->c3b tool breaks vertex uv)
		void loadHeightsFromFile_(const std::string& path, const std::vector<std::string>& layer_text);

		// Launch update thread
		void launchUpdThread();

		// Stop update thread
		void stopUpdThread();

		// Load Textures
		void loadTextures();

		// Unload textures
		void unloadTextures();

		// Set Atmosphere light
		void setAtmosphereLight(const cocos2d::Vec3& dir_light, const cocos2d::Vec3& col_light, const cocos2d::Vec3& ambAtmLight, const cocos2d::Vec3& fogColor)
		{
			_prop._light.dirAtmLight = dir_light;
			_prop._light.colAtmLight = col_light;
			_prop._light.ambAtmLight = ambAtmLight;
			_prop._light.colFog = fogColor;
		}

		// Set Fog data
		void setFog(float near_plane, float far_plane)
		{
			_prop._light.fog_near_plane = near_plane;
			_prop._light.fog_far_plane = far_plane;
		}

		// Set time passed (Needed for the grass animation)
		void setTimePassed(float time_passed)
		{
			_grass_prop._time_passed = time_passed;
		}

		// Get properties
		const HM_PROPERTY& getProperty() const
		{
			return _prop;
		}

		// Set grass texture
		void setGrassText(const std::string& grass_text);

		// Get shadow cameras
		const std::vector<ShadowCamera*>& getShadowCameras() const
		{
			return _shdw;
		}

	private:

		HeightMap(); // Use 'create'

		// Load properties from the passed file
		void loadProps();

		// Create grass shader
		void createGrassShader();

		// Create grass buffers
		void createGrassBuffers();

		// Update grass GL buffer
		void updateGrassGLbuffer();

		// Draw grass
		void drawGrass(cocos2d::Renderer* renderer, const cocos2d::Mat4& transform, uint32_t flags);

		// Create Lod levels
		void CreateLodLevels();

		// Update GL buffers for lod
		void updateLodsGl();

		// Calculate vector
		cocos2d::Vec3 getVector(int i1, int j1, int i2, int j2) const;

		// Add HeightMap chunk if it is not exists
		void addChunk(int j_chunk, int i_chunk, int chunk_count_w, int chunk_count_h, int chunk_size);

		// Set current chunk number
		void setCurrentChunk(int j_chunk, int i_chunk);

		// Blender color to alpha
		unsigned int color_to_alpha(cocos2d::Vec4& color);

		void clearProgramStateTextures(cocos2d::backend::ProgramState* programState);

		// Get tarnsform from the node's for the certain mesh in the *.c3b
		const cocos2d::Mat4& getTransform(std::vector<cocos2d::NodeData*> nodes, cocos2d::MeshData* mesh);

		void onBeforeDraw();
		void onAfterDraw();

		friend LodHM;
	};

}

#endif
