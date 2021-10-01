/* Define one chunk of HeightMap, which allow to save memory
*/
#ifndef _HEIGHT_CHUNK_H__
#define _HEIGHT_CHUNK_H__

#include "cocos2d.h"

namespace hm
{

#define MAX_LAYER_COUNT 2
#define LAYER_TEXTURE_SIZE 4

#define ONE_HEIGHT_DEF_X 128
#define ONE_HEIGHT_DEF_Y 255
#define ONE_HEIGHT_DEF_Z 128
	//#define ONE_HEIGHT_DEF_STATE ST_FREE
#define ONE_HEIGHT_DEF_ALPHA 0xff000000
#define ONE_HEIGHT_DEF_H -255

	class OneChunk
	{
	public:

		struct LAYER_DATA
		{
			unsigned int alpha = ONE_HEIGHT_DEF_ALPHA;
		};

		struct ONE_HEIGHT
		{
			ONE_HEIGHT()
			{
				lay[0].alpha = ONE_HEIGHT_DEF_ALPHA;
				for (int i = 1; i < MAX_LAYER_COUNT; ++i)
				    lay[i].alpha = 0;
			}
			bool isEqualDef()
			{
				int ret = 1;
				for (auto& v : lay)
					ret &= (v.alpha == ONE_HEIGHT_DEF_ALPHA);
				ret &= (h == ONE_HEIGHT_DEF_H);
				return ret;
			}

			struct NORMAL
			{
				char x = ONE_HEIGHT_DEF_X;
				char y = ONE_HEIGHT_DEF_Y;
				char z = ONE_HEIGHT_DEF_Z;
				char gap = 0;
			};
			
//			unsigned int alpha = ONE_HEIGHT_DEF_ALPHA;
			LAYER_DATA lay[MAX_LAYER_COUNT];
			float h = ONE_HEIGHT_DEF_H;
		};

	private:

		std::vector<ONE_HEIGHT> _arr;

	public:

		OneChunk(int size_side)
		{
			_arr.resize(size_side * size_side);
		}

		void addHeight(int i, int j, int side_size, const ONE_HEIGHT& val)
		{
			_arr[j * side_size + i] = val;
		}

		const ONE_HEIGHT& getHeight(int i, int j, int side_size)
		{
			return _arr[j * side_size + i];
		}

		const std::vector<ONE_HEIGHT>& getHeights() const
		{
			return _arr;
		}
	};

	class HeightChunk
	{

	private:

		std::vector<OneChunk*> _arr;
		int _chunk_count_w = 0;
		int _chunk_count_h = 0;
		int _chunk_size = 0;

		static OneChunk _def_chunk;

	public:

		~HeightChunk();

		HeightChunk(int chunk_count_w, int chunk_count_h, int chunk_size) : _chunk_count_w(chunk_count_w), _chunk_count_h(chunk_count_h), _chunk_size(chunk_size)
		{
			_arr.resize(_chunk_count_w * _chunk_count_h + 1);
			_arr[_arr.size() - 1] = &_def_chunk;
		}

		void addHeight(int i, int j, const OneChunk::ONE_HEIGHT& val)
		{
			int i_chunk = i / _chunk_size;
			int j_chunk = j / _chunk_size;

			int i_recalc = i - i_chunk * _chunk_size;
			int j_recalc = j - j_chunk * _chunk_size;

			if (_arr[j_chunk * _chunk_count_w + i_chunk] == nullptr)
				_arr[j_chunk * _chunk_count_w + i_chunk] = new OneChunk(_chunk_size);

			_arr[j_chunk * _chunk_count_w + i_chunk]->addHeight(i_recalc, j_recalc, _chunk_size, val);
		}

		const OneChunk::ONE_HEIGHT& getHeight(int i, int j) const
		{
#if defined(__LP64__) || defined(_WIN64)
			typedef long long INTL;
			const long long up_0 = 0x7fffffffffffffff;
#else
			typedef int INTL;
			const int up_0 = 0x7fffffff;
#endif

			int i_chunk = i / _chunk_size;
			int j_chunk = j / _chunk_size;

			int i_recalc = i - i_chunk * _chunk_size;
			int j_recalc = j - j_chunk * _chunk_size;

//			int coeff_null = bool(_arr[j_chunk * _chunk_count_side + i_chunk]);
			int coeff_null = (1 - ((((INTL(_arr[j_chunk * _chunk_count_w + i_chunk]) & up_0) - 1) >> (sizeof(INTL) * 8 - 2)) & 1));

			return _arr[coeff_null * (j_chunk * _chunk_count_w + i_chunk) + (1 - coeff_null) * (_arr.size() - 1)]->getHeight(i_recalc, j_recalc, _chunk_size);
		}

		const OneChunk::ONE_HEIGHT& getHeight(int i, int j, int* is_def) const
		{
#if defined(__LP64__) || defined(_WIN64)
			typedef long long INTL;
			const long long up_0 = 0x7fffffffffffffff;
#else
			typedef int INTL;
			const int up_0 = 0x7fffffff;
#endif

			int i_chunk = i / _chunk_size;
			int j_chunk = j / _chunk_size;

			int i_recalc = i - i_chunk * _chunk_size;
			int j_recalc = j - j_chunk * _chunk_size;

//			int coeff_null = bool(_arr[j_chunk * _chunk_count_side + i_chunk]);
			int coeff_null = (1 - ((((INTL(_arr[j_chunk * _chunk_count_w + i_chunk]) & up_0) - 1) >> (sizeof(INTL) * 8 - 2)) & 1));

			*is_def = 1 - coeff_null;

			return _arr[coeff_null * (j_chunk * _chunk_count_w + i_chunk) + (1 - coeff_null) * (_arr.size() - 1)]->getHeight(i_recalc, j_recalc, _chunk_size);
		}

		const std::vector<OneChunk*>& getChunks() const
		{
			return _arr;
		}

	};
}

#endif
