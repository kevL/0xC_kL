/*
 * Copyright 2010-2019 OpenXcom Developers.
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

#ifndef OPENXCOM_GRAPHSUBSET_H
#define	OPENXCOM_GRAPHSUBSET_H

#include <algorithm>
#include <utility>


namespace OpenXcom
{

struct GraphSubset
{
	int // defines subarea of surface
		_x_beg, _x_end,
		_y_beg, _y_end;

	/// cTor [0]
	GraphSubset(
			int x_max,
			int y_max)
		:
			_x_beg(0), _x_end(x_max),
			_y_beg(0), _y_end(y_max)
	{}

	/// cTor [1]
	GraphSubset(
			std::pair<int,int> x_range,
			std::pair<int,int> y_range)
		:
			_x_beg(x_range.first), _x_end(x_range.second),
			_y_beg(y_range.first), _y_end(y_range.second)
	{}

	/// cTor [2] - copy constructor
//	GraphSubset(const GraphSubset& area)
//		:
//			beg_x(area.beg_x),
//			end_x(area.end_x),
//			beg_y(area.beg_y),
//			end_y(area.end_y)
//	{}


	///
	inline GraphSubset offset(
			int x,
			int y) const
	{
		GraphSubset area (*this);
		area._x_beg += x;
		area._x_end += x;
		area._y_beg += y;
		area._y_end += y;
		return area;
	}

	///
	inline int size_x() const
	{ return _x_end - _x_beg; }

	///
	inline int size_y() const
	{ return _y_end - _y_beg; }


	///
	static inline void intersection_range(
			int& a_beg,
			int& a_end,
			const int& b_beg,
			const int& b_end)
	{
//		if (   static_cast<unsigned>(a_beg) < static_cast<unsigned>(b_end)
//			&& static_cast<unsigned>(a_end) > static_cast<unsigned>(b_beg))
		// hint to the person who came up with all this shyte: learn what a type
		// is and how to use it.
		// UPDATE: Lovely. The code relies on a warning in order to work.
		if (a_beg < b_end && a_end > b_beg) // WARNING [-Wstrict-overflow]
		{
			a_beg = std::max(a_beg, b_beg);
			a_end = std::min(a_end, b_end);
		}
		else
			a_end = a_beg;
	}

	///
	static inline GraphSubset intersection(
			const GraphSubset& a,
			const GraphSubset& b)
	{
		GraphSubset area (a);
		intersection_range(area._x_beg, area._x_end, b._x_beg, b._x_end);
		intersection_range(area._y_beg, area._y_end, b._y_beg, b._y_end);
		return area;
	}

//	static inline GraphSubset intersection(
//			const GraphSubset& a,
//			const GraphSubset& b,
//			const GraphSubset& c)
//	{
//		GraphSubset area (intersection(a, b));
//		intersection_range(area.beg_x, area.end_x, c.beg_x, c.end_x);
//		intersection_range(area.beg_y, area.end_y, c.beg_y, c.end_y);
//		return area;
//	}

//	static inline GraphSubset intersection(
//			const GraphSubset& a,
//			const GraphSubset& b,
//			const GraphSubset& c,
//			const GraphSubset& d)
//	{
//		GraphSubset area (intersection(a, b, c));
//		intersection_range(area.beg_x, area.end_x, d.beg_x, d.end_x);
//		intersection_range(area.beg_y, area.end_y, d.beg_y, d.end_y);
//		return area;
//	}

};

}

#endif
