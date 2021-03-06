/*
 * Copyright 2010-2020 OpenXcom Developers.
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

#ifndef OPENXCOM_RNG_H
#define OPENXCOM_RNG_H

#include <algorithm>
#include <cstdint>


namespace OpenXcom
{

/**
 * Random Number Generator(s) used throughout the game for all your random needs.
 * @note Uses a 64-bit xorshift* pseudo-random number generator.
 */
namespace RNG
{

/// Advances the internal pRNG.
uint64_t next_x();
/// Advances the external pRNG.
uint64_t next_y();

/// Gets the internal seed in use.
uint64_t getSeed();
/// Sets the internal/external seed(s) in use.
void setSeed(uint64_t seed = 0uLL);

/// Generates an integer, inclusive.
int generate(
		int valMin,
		int valMax);
/// Generates a floating-point value, inclusive.
double generate(
		double valMin,
		double valMax);
/// Generates a floating-point value, inclusive.
float generate(
		float valMin,
		float valMax);

/// Generates an integer, inclusive (external version).
int seedless(
		int valMin,
		int valMax);

/// Gets a normally distributed value.
double boxMuller(double deviation);

/// Decides if a percentage chance succeeds.
bool percent(int valPct);

/// Picks an entry from a vector.
size_t pick(size_t valSize);
/// Picks an entry from a vector using the external generator.
size_t pick(
		size_t valSize,
		bool external);


/**
 * Shuffles elements in an STL container.
 * @note This probably does one unnecessary extra loop.
 * @param first	- RAI to first element (random access iterator)
 * @param last	- RAI to last element (random access iterator)
 */
template<class iter>
void shuffle(
		iter first,
		iter last)
{
	std::ptrdiff_t r;
	const std::ptrdiff_t delta (last - first);
	for (std::ptrdiff_t
			i = 0;
			i < delta; // NOTE: Could likely get away with (delta-1)
			++i)
	{
		r = static_cast<std::ptrdiff_t>(next_x() % static_cast<uint64_t>(delta - i) + static_cast<uint64_t>(i));
		std::swap(
				first[i],
				first[r]);
	}
}

}

}

#endif
