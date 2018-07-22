/*
 * Copyright 2010-2018 OpenXcom Developers.
 *
 * This file is part of OpenXcom.
 *
 * OpenXcom is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * OpenXcom is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with OpenXcom. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef OPENXCOM_SHADERREPEAT_H
#define	OPENXCOM_SHADERREPEAT_H

#include "ShaderDraw.h"


namespace OpenXcom
{

template<typename Pixel>
class ShaderRepeat
	:
		public helper::ShaderBase<const Pixel>
{

private:
	int
		_x_off,
		_y_off;

	public:
		typedef helper::ShaderBase<const Pixel> _base;
		friend struct helper::controler<ShaderRepeat<Pixel>>;

		// kL_change: Cut the crap out.
		// ps. I'm sure there's a lot more around here ...

		/// cTor.
		inline ShaderRepeat(
				const std::vector<Pixel>& data,
				int x_max,
				int y_max)
			:
				_base(
					data,
					x_max,
					y_max)
		{
			_x_off =
			_y_off = 0;
		}

/*
		/// struct [0]
		inline ShaderRepeat(const Surface* s)
			:
				_base(s)
		{
			setOffset(0,0);
		}
		/// struct [1]
		inline ShaderRepeat(
				const std::vector<Pixel>& f,
				int max_x,
				int max_y)
			:
				_base(
					f,
					max_x,
					max_y)
		{
			setOffset(0,0);
		}

		/// Sets offset.
		inline void setOffset(
				int x,
				int y)
		{
			_off_x = x;
			_off_y = y;
		}
		/// Adds offset.
		inline void addOffset(
				int x,
				int y)
		{
			_off_x += x;
			_off_y += y;
		} */
};


namespace helper
{

template<typename Pixel>
struct controler<ShaderRepeat<Pixel>>
{
	typedef typename ShaderRepeat<Pixel>::PixelPtr PixelPtr;
	typedef typename ShaderRepeat<Pixel>::PixelRef PixelRef;

	const GraphSubset _range_domain;
	GraphSubset _range_image;

	const int
		_x_off,
		_y_off,
		_x_size,
		_y_size,
		_pitch;
	int
		_x_curr,
		_y_curr;

	const PixelPtr _base;
	PixelPtr
		_x_curr_ptr,
		_y_curr_ptr;

	/// cTor.
	controler(const ShaderRepeat<Pixel>& repeat)
		:
			_base(repeat.ptr()),
			_range_domain(repeat.getDomain()),
			_range_image(0,0),
			_x_off(repeat._x_off),
			_y_off(repeat._y_off),
			_x_size(_range_domain.size_x()),
			_y_size(_range_domain.size_y()),
			_x_curr(0),
			_y_curr(0),
			_pitch(repeat.pitch()),
			_x_curr_ptr(nullptr),
			_y_curr_ptr(nullptr)
	{}

	// not used
//	inline const GraphSubset& get_range()

	///
	inline void mod_range(GraphSubset&)
	{}
	///
	inline void set_range(const GraphSubset& graph)
	{
		_range_image = graph;
	}

	///
	inline void mod_y(
			int&,
			int&)
	{
		_y_curr = (_range_image._y_beg - _y_off) % _y_size;
		if (_y_curr < 0)
			_y_curr += _y_size;
		_y_curr_ptr = _base;
	}
	///
	inline void set_y(
			const int& start,
			const int&)
	{
		_y_curr = (_y_curr + start) % _y_size;
		_y_curr_ptr += (_range_domain._y_beg + _y_curr) * _pitch;
	}
	///
	inline void inc_y()
	{
		++_y_curr;
		_y_curr_ptr += _pitch;
		if (_y_curr == _y_size)
		{
			_y_curr = 0;
			_y_curr_ptr -= _y_size * _pitch;
		}
	}


	///
	inline void mod_x(
			int&,
			int&)
	{
		_x_curr = (_range_image._x_beg - _x_off) % _x_size;
		if (_x_curr < 0)
			_x_curr += _x_size;
		_x_curr_ptr = _y_curr_ptr;
	}
	///
	inline void set_x(
			const int& start,
			const int&)
	{
		_x_curr = (_x_curr + start) % _x_size;
		_x_curr_ptr += _range_domain._x_beg + _x_curr;
	}
	///
	inline void inc_x()
	{
		++_x_curr;
		_x_curr_ptr += 1;
		if (_x_curr == _x_size)
		{
			_x_curr = 0;
			_x_curr_ptr -= _x_size;
		}
	}

	///
	inline PixelRef get_ref()
	{
		return *_x_curr_ptr;
	}
};

}

}

#endif

