/*
 * Copyright 2010-2016 OpenXcom Developers.
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

#ifndef OPENXCOM_FMATH_H
#define OPENXCOM_FMATH_H

//#define _USE_MATH_DEFINES
#include <cmath>
#include <limits>

//#ifndef M_PI
//#	define M_PI		3.14159265358979323846
//#	define M_PI_2	1.57079632679489661923
//#	define M_PI_4	0.785398163397448309616
//#endif


/**
 * Returns true if two floating-point values are within epsilon.
 */
template<class _Tx>
inline bool AreSame(
		const _Tx& l,
		const _Tx& r)
{
	return std::fabs(l - r) <= std::numeric_limits<_Tx>::epsilon();
}


/**
 * Rounds a floating-point value up or down to its nearest whole value.
 * @note The standard library functions ceil() and floor() expect doubles.
 */
template<class _Tx>
inline _Tx Round(const _Tx& x)
{
	return (x < static_cast<_Tx>(0.)) ? std::ceil(x - static_cast<_Tx>(0.5)) : std::floor(x + static_cast<_Tx>(0.5));
}


/**
 * Returns the square of a value.
 */
template<class _Tx>
inline _Tx Sqr(const _Tx& x)
{
	return x * x;
}

/**
 *
 *
template<class _Tx>
inline _Tx Sign(const _Tx& x)
{
	return (_Tx(0) < x) - (x < _Tx(0));
} */

/**
 * Returns true if x or y (lon,lat) is NaN or Inf.
 */
template<class _Tx>
inline bool isNaNorInf(
		const _Tx& x,
		const _Tx& y)
{
	if (std::isnan(x) || std::isnan(y)
		|| std::isinf(x) || std::isinf(y))
	{
		return true;
	}
	return false;
}

#endif
