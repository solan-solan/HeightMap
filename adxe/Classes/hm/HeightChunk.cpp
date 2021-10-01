#include "HeightChunk.h"

using namespace hm;

OneChunk HeightChunk::_def_chunk(25);

HeightChunk::~HeightChunk()
{
	for (int i = 0; i < _arr.size() - 1; ++i)
	{
		if (_arr[i])
			delete _arr[i];
	}
	_arr.clear();
}
