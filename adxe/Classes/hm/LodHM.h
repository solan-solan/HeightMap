/* Define one Lod of the HeightMap
*/
#ifndef _LOD_HM_H__
#define _LOD_HM_H__

#include "cocos2d.h"
#include "HeightChunk.h"
#include <atomic>

namespace hm
{
	class HeightMap;

#define MIN_LOD_W 3
#define MIN_LOD_H 3

	class LodHM
	{
		enum LOD_STATE
		{
			NONE = 0,
			UPDATE_GL_BUFFER = 1
		};
		std::atomic_int _lod_state = {NONE};

	public:

		// Lod's uniforms
		struct ALL_LOD_LOC
		{
			cocos2d::backend::UniformLocation _diff_loc;
			cocos2d::backend::UniformLocation _norm_loc;
			cocos2d::backend::UniformLocation _cam_pos_loc;
			cocos2d::backend::UniformLocation _sun_light_dir;
			cocos2d::backend::UniformLocation _sun_light_col;
			cocos2d::backend::UniformLocation _ambient_light_dir;
			cocos2d::backend::UniformLocation _fog_col;
			cocos2d::backend::UniformLocation _vp;
			cocos2d::backend::UniformLocation _scale;
			cocos2d::backend::UniformLocation _ndraw;
			cocos2d::backend::UniformLocation _nearFogPlane;
			cocos2d::backend::UniformLocation _farFogPlane;
			cocos2d::backend::UniformLocation _lVP_shadow_loc;
			cocos2d::backend::UniformLocation _depth_loc;
		} _allLodLoc;

		// Loc's for grid shader
		struct ALL_LOD_LOC_GRID
		{
			cocos2d::backend::UniformLocation _vp;
			cocos2d::backend::UniformLocation _scale;
			cocos2d::backend::UniformLocation _ndraw;
			cocos2d::backend::UniformLocation _cam_pos_loc;
		} _allLodLocGrid;

		// Loc's for normals
		struct ALL_LOD_LOC_NORM
		{
			cocos2d::backend::UniformLocation _vp;
			cocos2d::backend::UniformLocation _scale;
			cocos2d::backend::UniformLocation _ndraw;
			cocos2d::backend::UniformLocation _cam_pos_loc;
		} _allLodLocNorm;

	private:
		bool _show_grid = false;

		float _norm_size = 0.f;

		struct ONEVERTEX
		{
			float y = 0.f;
			float npack = 0.f;
			float x = 0.f;
			float z = 0.f;
			float alpha[MAX_LAYER_COUNT];
			ONEVERTEX()
			{
				for (int i = 0; i < MAX_LAYER_COUNT; ++i)
				    alpha[i] = ONE_HEIGHT_DEF_ALPHA;
			}
		};

		struct NOT_DRAW
		{
			float zs = 0;  // z of the up left point
			float xs = 0;  // x of the up left point
			float ze = 0;  // z of the down right point
			float xe = 0;  // x of the down right point
			bool needRedr = false; // To say the up layer that redraw is needed
			NOT_DRAW() {}
			NOT_DRAW(float _zs, float _xs, float _ze, float _xe, bool redr)
			{
				zs = _zs; xs = _xs; ze = _ze; xe = _xe; needRedr = redr;
			}
			bool operator !=(const NOT_DRAW& nd)
			{
				return (nd.zs != zs || nd.xs != xs || nd.ze != ze || nd.xe != xe);
			}
			bool operator ==(const NOT_DRAW& nd)
			{
				return (nd.zs == zs && nd.xs == xs && nd.ze == ze && nd.xe == xe);
			}
		} _nextLayer;

		// Colors of the lod GRID and NORMAL
		static const cocos2d::Vec3 _lod_color[7];

		// Next lod
		LodHM* _next = 0;

		// Height map instance
		HeightMap* _hM = nullptr;

		// Lod number
		unsigned int _lod_num = 0;

		// Is shadow
		bool _is_shadow = false;

		// Landscape vertex massive
		std::vector<ONEVERTEX> lVert;

		// Up part of vertex array for normals
		std::vector<ONEVERTEX> lVertNormUp;

		typedef unsigned short INDEX_TYPE;
		enum
		{
			cocos_index_format = (int)cocos2d::CustomCommand::IndexFormat::U_SHORT
		};

	    // Indicies array
		std::vector<INDEX_TYPE> iVert;

		// Indicies line array
		std::vector<INDEX_TYPE> iVertLine;

		// Indicies of normals array
		std::vector<INDEX_TYPE> iVertNorm;

		// Indicies count for drawing
		unsigned int _iVertCount = 0;

		struct LAYER_DRAW_DATA
		{
			// Command to draw the Lod
			cocos2d::CustomCommand _customCommand;

			// Program state to define shader for Lod drawing
			cocos2d::backend::ProgramState* _programState = nullptr;
		};
		std::array<LAYER_DRAW_DATA, MAX_LAYER_COUNT> _layer_draw;
		int _layer_count = 1; // Layer count to draw

		// Program state to define shader for grid drawing
		cocos2d::backend::ProgramState* _programStateGrid = nullptr;

		// Program state to define shader for normals
		cocos2d::backend::ProgramState* _programStateNorm = nullptr;

		// Command to draw Lod as grid
		cocos2d::CustomCommand _customCommandGrid;

		// Command to draw Lod normals
		cocos2d::CustomCommand _customCommandNorm;

		// Level width in pixels
		int _w = MIN_LOD_W;

		// Level height in pixels
		int _h = MIN_LOD_H;

		// The number of levels which must be updated forcibly - despite to change camera position or not
		static std::atomic_char _force_level_update;

		// Helper varriable for increase performance
		struct HELPER
		{
			int _level_mult_center = 4;
			int _level_mult = 1;
			int _half_w = 0;
			int _half_h = 0;
		} _helper;

		// Previouse position index
		cocos2d::Vec2 _old_pos_ind = { -1.0, -1.0 };

		cocos2d::backend::CompareFunction _rendererDepthCmpFunc;

	private:

		// Get Lod level multiplier
		// It defines the step in texels on the map texture
		inline int levelMult() const
		{
			return _helper._level_mult;
		}

		// Get levelMult to define center
		inline int levelMultCenter() const
		{
			return _helper._level_mult_center;
		}

		// Create Line indicies buffers 
		// This allow to see the landscape as a wire
		void createLineBuffers();

		// Create buffers for normal
		void createNormBuffers();

		// Update normal buffers
		void updateNormBuffers();

		// Set Grid Transparent
		void setGridModeZ(bool is_z_mode);

		// Returns the point of intersection between triangl poligons and line segment [start_p; end_p]
		bool rayCast(const cocos2d::Vec3& start_p, const cocos2d::Vec3& end_p, cocos2d::Vec3* ret);

		//   Tests one triangle on intersection with the line segment [start_p; end_p];
		//   Returns true - if intersection point lies inside triangle (p_res keeps intersection point),
		//           false - if intersection point lies outside triangle or intersection is absent
		bool rayCastOneTriangle(const cocos2d::Vec3& start_p, const cocos2d::Vec3& end_p,
							   const cocos2d::Vec3& p1, const cocos2d::Vec3& p2, const cocos2d::Vec3& p3,
							   cocos2d::Vec3* p_res = 0);

		// Create Shader for the Lod
		void createShader();

		// Create Shader for grid
		void createShaderGrid();

		// Create shader for normals
		void createShaderNorm();

		void onBeforeDraw();
		void onAfterDraw();

	public:

		LodHM() {}
		LodHM(HeightMap* hM, int lod_cound);
		~LodHM();

		// Build vertex/index array
		void createVertexArray();

		// Update vertex array
		NOT_DRAW updateVertexArray(const cocos2d::Vec3 & p, int prev_lev_mult);

		// Draw landscape 
		void drawLandScape(cocos2d::Renderer * renderer, const cocos2d::Mat4 & transform, uint32_t flags);

		// Get vertex count for this lod and underlying
		unsigned int getvertexCount() const;

		// Update GL buffer
		unsigned char updateGLbuffer();

		// Set show grid
		void showGrid(bool show)
		{
			if (_next)
				_next->showGrid(show);
			_show_grid = show;
		}

		// Set show normals
		void showNormals(float size)
		{
			if (_next)
				_next->showNormals(size);
			_norm_size = size;
		}

		// Get first lod
		const LodHM* getFirstLod() const { if (_next) return _next->getFirstLod(); else return this; }

		// Update sky light direction
		void updateSkyLightData();

		// Set HeightMap textures to shader
		void setTextures();

		// Unset HeightMap textures from shader
		void unsetTextures();

		inline void unpack_xz(float xz, float* x, float* z)
		{
			int zz = xz / 65536;
			zz = zz - (zz & 1) * 2 * zz + (zz & 1);
			*z = zz;

			int xx = xz - zz * 65536;
			xx = xx - (xx & 1) * 2 * xx + (xx & 1);
			*x = xx;
		}

		friend HeightMap;
	};

}

#endif
