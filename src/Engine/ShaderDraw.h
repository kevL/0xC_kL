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

#ifndef OPENXCOM_SHADERDRAW_H
#define	OPENXCOM_SHADERDRAW_H

#include "ShaderDrawHelper.h"


namespace OpenXcom
{

/**
 * Universal blit function.
 * @tparam ColorFunc - class that contains static function 'func' that get 5 arguments;
 * 					   function is used to modify these arguments
 * @param dest_frame - destination surface modified by function
 * @param src0_frame - surface or scalar
 * @param src1_frame - surface or scalar
 * @param src2_frame - surface or scalar
 * @param src3_frame - surface or scalar
 */
template<typename ColorFunc,
		 typename DestType,
		 typename Src0Type,
		 typename Src1Type,
		 typename Src2Type,
		 typename Src3Type>

static inline void ShaderDraw(
		const DestType& dest_frame,
		const Src0Type& src0_frame,
		const Src1Type& src1_frame,
		const Src2Type& src2_frame,
		const Src3Type& src3_frame)
{
	// creating helper objects
	helper::controler<DestType> dest (dest_frame);
	helper::controler<Src0Type> src0 (src0_frame);
	helper::controler<Src1Type> src1 (src1_frame);
	helper::controler<Src2Type> src2 (src2_frame);
	helper::controler<Src3Type> src3 (src3_frame);

	// get basic draw range in 2d space
	GraphSubset end_t (dest.get_range());

	// intersections with src ranges
	src0.mod_range(end_t);
	src1.mod_range(end_t);
	src2.mod_range(end_t);
	src3.mod_range(end_t);

	const GraphSubset end (end_t);
	if (end.size_x() == 0 || end.size_y() == 0)
		return;

	// set final draw range in 2d space
	dest.set_range(end);
	src0.set_range(end);
	src1.set_range(end);
	src2.set_range(end);
	src3.set_range(end);


	int
		y_beg (0),
		y_end (end.size_y());

	// determining iteration range in y-axis
	dest.mod_y(y_beg, y_end);
	src0.mod_y(y_beg, y_end);
	src1.mod_y(y_beg, y_end);
	src2.mod_y(y_beg, y_end);
	src3.mod_y(y_beg, y_end);

	if (y_beg >= y_end)
		return;

	// set final iteration range
	dest.set_y(y_beg, y_end);
	src0.set_y(y_beg, y_end);
	src1.set_y(y_beg, y_end);
	src2.set_y(y_beg, y_end);
	src3.set_y(y_beg, y_end);

	// iteration on y-axis
	for (int
			y = y_end - y_beg;
			y > 0;
			--y,
				dest.inc_y(),
				src0.inc_y(),
				src1.inc_y(),
				src2.inc_y(),
				src3.inc_y())
	{
		int
			x_beg (0),
			x_end (end.size_x());

		// determining iteration range in x-axis
		dest.mod_x(x_beg, x_end);
		src0.mod_x(x_beg, x_end);
		src1.mod_x(x_beg, x_end);
		src2.mod_x(x_beg, x_end);
		src3.mod_x(x_beg, x_end);

		if (x_beg >= x_end)
			continue;

		// set final iteration range
		dest.set_x(x_beg, x_end);
		src0.set_x(x_beg, x_end);
		src1.set_x(x_beg, x_end);
		src2.set_x(x_beg, x_end);
		src3.set_x(x_beg, x_end);

		// iteration on x-axis
		for (int
				x = x_end - x_beg;
				x > 0;
				--x,
					dest.inc_x(),
					src0.inc_x(),
					src1.inc_x(),
					src2.inc_x(),
					src3.inc_x())
		{
			ColorFunc::func(
						dest.get_ref(),
						src0.get_ref(),
						src1.get_ref(),
						src2.get_ref(),
						src3.get_ref());
		}
	}
}


template<typename ColorFunc,
		 typename DestType,
		 typename Src0Type,
		 typename Src1Type,
		 typename Src2Type>

static inline void ShaderDraw(
		const DestType& dest_frame,
		const Src0Type& src0_frame,
		const Src1Type& src1_frame,
		const Src2Type& src2_frame)
{
	ShaderDraw<ColorFunc>(
						dest_frame,
						src0_frame,
						src1_frame,
						src2_frame,
						helper::Bogus());
}

template<typename ColorFunc,
		 typename DestType,
		 typename Src0Type,
		 typename Src1Type>

static inline void ShaderDraw(
		const DestType& dest_frame,
		const Src0Type& src0_frame,
		const Src1Type& src1_frame)
{
	ShaderDraw<ColorFunc>(
						dest_frame,
						src0_frame,
						src1_frame,
						helper::Bogus(),
						helper::Bogus());
}

template<typename ColorFunc,
		 typename DestType,
		 typename Src0Type>

static inline void ShaderDraw(
		const DestType& dest_frame,
		const Src0Type& src0_frame)
{
	ShaderDraw<ColorFunc>(
						dest_frame,
						src0_frame,
						helper::Bogus(),
						helper::Bogus(),
						helper::Bogus());
}

/* template<typename ColorFunc,
			typename DestType>
static inline void ShaderDraw(const DestType& dest_frame)
{
	ShaderDraw<ColorFunc>(
						dest_frame,
						helper::Nothing(),
						helper::Nothing(),
						helper::Nothing(),
						helper::Nothing());
} */


template<typename T>
static inline helper::Flat<T> ShaderScalar(T& val)
{
	return helper::Flat<T>(val);
}

template<typename T>
static inline helper::Flat<const T> ShaderScalar(const T& val)
{
	return helper::Flat<const T>(val);
}

}

#endif
