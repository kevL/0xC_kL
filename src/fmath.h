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

#include <cmath>
#include <limits>


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
 * Returns true if x or y (lon,lat) is NaN or Inf.
 */
template<class _Tx>
inline bool isNaNorInf(
		const _Tx& x,
		const _Tx& y)
{
	if (   std::isnan(x) || std::isnan(y)
		|| std::isinf(x) || std::isinf(y))
	{
		return true;
	}
	return false;
}

/**
 *
 *
template<class _Tx>
inline _Tx Sign(const _Tx& x)
{
	return (_Tx(0) < x) - (x < _Tx(0));
} */

//#ifndef M_PI
//static const long double M_PI = 3.1415926535897932384626433832795029L;
//#endif

// http://www.nongnu.org/avr-libc/user-manual/group__avr__math.html
// NOTE: I should probably leave off the #ifdefs so that *if* these *are* defined
// the whole thing borks.
#ifndef M_E
static const double M_E			= 2.7182818284590452354;
#endif
#ifndef M_LOG2E
static const double M_LOG2E		= 1.4426950408889634074;  /* log_2 e */
#endif
#ifndef M_LOG10E
static const double M_LOG10E	= 0.43429448190325182765; /* log_10 e */
#endif
#ifndef M_LN2
static const double M_LN2		= 0.69314718055994530942; /* log_e 2 */
#endif
#ifndef M_LN10
static const double M_LN10		= 2.30258509299404568402; /* log_e 10 */
#endif
#ifndef M_PI
static const double M_PI		= 3.14159265358979323846; /* pi */
#endif
#ifndef M_PI_2
static const double M_PI_2		= 1.57079632679489661923; /* pi/2 */
#endif
#ifndef M_PI_4
static const double M_PI_4		= 0.78539816339744830962; /* pi/4 */
#endif
#ifndef M_1_PI
static const double M_1_PI		= 0.31830988618379067154; /* 1/pi */
#endif
#ifndef M_2_PI
static const double M_2_PI		= 0.63661977236758134308; /* 2/pi */
#endif
#ifndef M_2_SQRTPI
static const double M_2_SQRTPI	= 1.12837916709551257390; /* 2/sqrt(pi) */
#endif
#ifndef M_SQRT2
static const double M_SQRT2		= 1.41421356237309504880; /* sqrt(2) */
#endif
#ifndef M_SQRT1_2
static const double M_SQRT1_2	= 0.70710678118654752440; /* 1/sqrt(2) */
#endif

#endif
