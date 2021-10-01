/* Define array which is centered along 0 index; It allows access to negative indecies and returns default value if index is out of range
or bool(T) returns null

T type must allow the following operators: operator *(), operator +(), operator bool(), operator int(), operator T()
*/
#ifndef _CENTER_ARRAY_H__
#define _CENTER_ARRAY_H__

#include <vector>

namespace hm
{
	template<typename T, T _def_val> class CenterArray
	{
	private:

		std::vector<T> _arr;
		int _center_index = 0;

	public:

		CenterArray(const T& first_val)
		{
			_arr.push_back(first_val);
		}

		CenterArray()
		{
			_arr.emplace_back();
		}

		// Return value according to the index
		const T& getVal(int index) const
		{
#if defined(__LP64__) || defined(_WIN64)
			typedef long long INTL;
			const long long up_0 = 0x7fffffffffffffff;
#else
			typedef int INTL;
			const int up_0 = 0x7fffffff;
#endif

			int effective_index = index + _center_index;

			int neg_coeff = (effective_index >> (sizeof(int) * 8 - 2)) & 1;
			int pos_coeff = ((_arr.size() - 1 - effective_index) >> (sizeof(int) * 8 - 2)) & 1;
			int null_coeff = ((1 - neg_coeff) * (1 - pos_coeff)) * (1 - ((((INTL(_arr[(1 - neg_coeff) * (1 - pos_coeff) * effective_index]) & up_0) - 1) >> (sizeof(INTL) * 8 - 2)) & 1)) + neg_coeff + pos_coeff;

			return 
				reinterpret_cast<T>((null_coeff * (1 - neg_coeff) * (1 - pos_coeff)) * INTL(_arr[null_coeff * (1 - neg_coeff) * (1 - pos_coeff) * effective_index]) +
				neg_coeff * INTL(_def_val) + pos_coeff * INTL(_def_val) +
				(1 - null_coeff) * INTL(_def_val));
		}

		// Return value according to the index
		const T& getVal_(int index) const
		{
#if defined(__LP64__) || defined(_WIN64)
			typedef long long INTL;
			const long long up_0 = 0x7fffffffffffffff;
#else
			typedef int INTL;
			const int up_0 = 0x7fffffff;
#endif

			int effective_index = index + _center_index;

			int neg_coeff = (effective_index >> (sizeof(int) * 8 - 2)) & 1;
			int pos_coeff = ((_arr.size() - 1 - effective_index) >> (sizeof(int) * 8 - 2)) & 1;
			int null_coeff = ((1 - neg_coeff) * (1 - pos_coeff)) * (1 - ((((INTL(_arr[(1 - neg_coeff) * (1 - pos_coeff) * effective_index]) & up_0) - 1) >> (sizeof(INTL) * 8 - 2)) & 1)) + neg_coeff + pos_coeff;

			if (neg_coeff + pos_coeff + (1 - null_coeff))
				return _def_val;

			return _arr[effective_index];
		}

		// Return value according to the index
		const T& getVal__(int index) const
		{
			int effective_index = index + _center_index;

			if (effective_index < 0 || effective_index >= _arr.size() || _arr[effective_index] == nullptr)
				return _def_val;

			return _arr[effective_index];
		}

		// Add value according to the index
		void addVal(const T& val, int index)
		{
			int effective_index = index + _center_index;

			if (effective_index < 0)
			{
				// Recalculate center index
				int inc = 0 - effective_index;
				_center_index += inc;

				// Insert stub elements
				for (int i = 0; i < inc; ++i)
				{
					_arr.insert(_arr.begin(), T());
				}

				// Insert val at the begin
				_arr[0] = val;
			}
			else if (effective_index < _arr.size())
			{
				_arr[effective_index] = val;
			}
			else
			{
				_arr.resize(effective_index + 1);
				_arr[effective_index] = val;
			}
		}

		void remVal(int index)
		{
			int effective_index = index + _center_index;
			assert(effective_index >= 0);

			if (effective_index == 0)
			{
				_arr.erase(_arr.begin());
				_center_index--;
			}
			else if (effective_index == _arr.size() - 1)
			{
				_arr.erase(_arr.begin() + effective_index);
			}
			else if (effective_index > 0 && effective_index < _arr.size() - 1)
			{
				_arr[effective_index] = nullptr;
			}
		}

		const T& operator[](int i) const
		{
			return _arr[i];
		}

		int size() const
		{
			return _arr.size();
		}

		int center_index() const
		{
			return _center_index;
		}
	};
}

#endif
