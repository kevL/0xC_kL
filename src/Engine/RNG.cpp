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

#include "RNG.h"

#include <chrono>

#include "../fmath.h"

//#include "Logger.h"


namespace OpenXcom
{

namespace RNG
{

static uint64_t
	x, // internal RNG
	y; // external RNG
// Note: 'internal' means the state is saved to file. 'external' will be used
// for throwaway values such as animation states. The idea is to preserve the
// internal RNG so that it will reproduce predictable results for testing or
// debugging and so it cannot be subject to user-induced temporal anomalies
// (such as the duration between mouse-clicks). Be aware that this is
// problematic on the Geoscape, since timed-events there will access the
// internal RNG.


/*	Written in 2014 by Sebastiano Vigna (vigna@acm.org)

To the extent possible under law, the author has dedicated all copyright
and related and neighboring rights to this software to the public domain
worldwide. This software is distributed without any warranty.

See <http://creativecommons.org/publicdomain/zero/1.0/>. */

/*	This is a good generator if you're short on memory, but otherwise
	we rather suggest to use a xorshift128+ (for maximum speed) or
	xorshift1024* (for speed and very long period) generator. */

/**
 * Advances the internal RNG.
 * @return, next integer
 */
uint64_t next_x()
{
	x ^= x >> 12; // a
	x ^= x << 25; // b
	x ^= x >> 27; // c

	//uLL = 18446744073709551615 Max
	//Log(LOG_INFO) << "RNG x = " << x;
	//if (x == 16029208282934479754uLL)
	//	Log(LOG_INFO) << "stop";

	return x * 2685821657736338717uLL;
}

/**
 * Advances the external RNG.
 * @return, next integer
 */
uint64_t next_y()
{
	y ^= y >> 12; // a
	y ^= y << 25; // b
	y ^= y >> 27; // c

	//Log(LOG_INFO) << "RNG y = " << y;
	return y * 2685821657736338717uLL;
}

/**
 * Gets the current state-value of the internal generator.
 * @return, the SAVE seed
 */
uint64_t getSeed()
{
	//Log(LOG_INFO) << "GET x = " << x;
	return x;
}

/**
 * Seeds both the internal and external generators.
 * @param seed - the LOAD seed (default 0 reset)
 */
void setSeed(uint64_t seed)
{
	y = static_cast<uint64_t>(std::chrono::system_clock::now().time_since_epoch().count());

	switch (seed)
	{
		case 0uLL:
			x = y + 1uLL; // nudge x so it's out of sync w/ y.
			break;
		default:
			x = seed;
	}
	//Log(LOG_INFO) << "SET x = " << x;
}

/**
 * Generates a uniformly distributed random integer within the specified range.
 * @param valMin - minimum number, inclusive
 * @param valMax - maximum number, inclusive
 * @return, generated value
 */
int generate(
		int valMin,
		int valMax)
{
	if (valMin == valMax)
		return valMin;

	if (valMin > valMax)
		std::swap(valMin, valMax);

	return static_cast<int>(next_x() % static_cast<uint64_t>(valMax - valMin + 1)) + valMin;
}

/**
 * Generates a uniformly distributed random floating-point value within the
 * specified range.
 * @param valMin - minimum number, inclusive
 * @param valMax - maximum number, inclusive
 * @return, generated value
 */
double generate(
		double valMin,
		double valMax)
{
	double delta (valMax - valMin);
	if (AreSame(delta, 0.) == true)
		return valMin;

	delta = (static_cast<double>(std::numeric_limits<uint64_t>::max()) / delta);
	if (AreSame(delta, 0.) == true)
		return valMin;

	return (static_cast<double>(next_x()) / delta) + valMin;
}

/**
 * Generates a uniformly distributed random floating-point value within the
 * specified range.
 * @param valMin - minimum number, inclusive
 * @param valMax - maximum number, inclusive
 * @return, generated value
 */
float generate(
		float valMin,
		float valMax)
{
	float delta (valMax - valMin);
	if (AreSame(delta, 0.f) == true)
		return valMin;

	delta = (static_cast<float>(std::numeric_limits<uint64_t>::max()) / delta);
	if (AreSame(delta, 0.f) == true)
		return valMin;

	return (static_cast<float>(next_x()) / delta) + valMin;
}

/**
 * Generates a uniformly distributed random integer within the specified range.
 * @note Distinct from "generate" in that it uses the external generator.
 * @param min - minimum number, inclusive
 * @param max - maximum number, inclusive
 * @return, generated value
 */
int seedless(
		int valMin,
		int valMax)
{
	if (valMin == valMax)
		return valMin;

	if (valMin > valMax)
		std::swap(valMin, valMax);

	return (static_cast<int>(next_y() % static_cast<uint64_t>(valMax - valMin + 1)) + valMin);
}

/*
ftp://ftp.taygeta.com/pub/c/boxmuller.c

Implements the Polar form of the Box-Muller Transformation
(c) Copyright 1994, Everett F. Carter Jr.
	Permission is granted by the author to use this software for
	any application provided this copyright notice is preserved.
*/
/**
 * Gaussian generator.
 * @param deviation	- standard deviation
 * @return, normally distributed value
 */
double boxMuller(double deviation)
{
	// kL_note: Do not store the static vars because when reloading they throw
	// off the predictability of the RNG. Regenerate a fresh return value from
	// scratch every time instead.
	//
	// And there's no use for a mean-value; always use 0.
	double
		x1,x2,
		w;
	do
	{
		x1 = (generate(0.,1.) * 2.) - 1.;
		x2 = (generate(0.,1.) * 2.) - 1.;
		w  = (x1 * x1) + (x2 * x2);
	}
	while (w >= 1.);

	w = std::sqrt(-2. * std::log(w) / w);

	return (x1 * w * deviation);
}
/*	static bool use_last;
	static double y2;
	double y1;

 if (use_last == true) // use value from the previous call
	{
		use_last = false;
		y1 = y2;
	}
	else
	{
		use_last = true;
		double
			x1,x2,
			w;
		do
		{
			x1 = (generate(0.,1.) * 2.) - 1.;
			x2 = (generate(0.,1.) * 2.) - 1.;
			w  = (x1 * x1) + (x2 * x2);
		}
		while (w >= 1.);

		w = std::sqrt((-2. * std::log(w)) / w);
		y1 = x1 * w;
		y2 = x2 * w;
	}
	return (mean + (y1 * deviation)); */

/**
 * Decides whether a percentage chance happens successfully.
 * @note Accepts input-values less than 0 or greater than 100.
 * @param valPct - value as a percentage
 * @return, true if succeeded
 */
bool percent(int valPct)
{
	if (valPct < 1)
		return false;

	if (valPct > 99)
		return true;

	return (generate(0,99) < valPct);
}

/**
 * Picks an entry from a vector.
 * @note Don't try shoving an empty vector into here.
 * @param val - size of the vector
 * @return, picked id
 */
size_t pick(size_t valSize)
{
	return static_cast<size_t>(generate(0, static_cast<int>(valSize) - 1));
}

/**
 * Picks an entry from a vector using the seedless generator.
 * @note Don't try shoving an empty vector into here.
 * @param val		- size of the vector
 * @param external	- true OR false to use the seedless (external) generator
 * @return, picked id
 */
size_t pick(
		size_t valSize,
		bool)
{
	return static_cast<size_t>(seedless(0, static_cast<int>(valSize) - 1));
}

}

}
