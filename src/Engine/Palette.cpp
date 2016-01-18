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

#include "Palette.h"

#include <cstring>
#include <fstream>

#include "Exception.h"


namespace OpenXcom
{

/**
 * Initializes a brand new Palette.
 */
Palette::Palette()
	:
		_colors(0),
		_count(0)
{}

/**
 * Deletes any colors contained within.
 */
Palette::~Palette()
{
	delete[] _colors;
}

/**
 * Loads an X-Com palette from a file. X-Com palettes are just a set
 * of RGB colors in a row, on a 0-63 scale, which have to be adjusted
 * for modern computers (0-255 scale).
 * @param file		- reference the filename of the palette
 * @param qColors	- number of colors in the palette
 * @param offset	- position of the palette in the file in bytes (default 0)
 * @sa http://www.ufopaedia.org/index.php?title=PALETTES.DAT
 */
void Palette::loadDat(
		const std::string& file,
		int qColors,
		int offset)
{
	if (_colors != 0)
	{
		throw Exception("loadDat can be run only once");
	}

	_count = static_cast<size_t>(qColors);
	_colors = new SDL_Color[_count];

	std::memset(
			_colors,
			0,
			sizeof(SDL_Color) * _count);

	// Load file and put colors in palette
	std::ifstream palFile (file.c_str(), std::ios::in | std::ios::binary);
	if (palFile.fail() == true)
	{
		throw Exception(file + " not found");
	}

	// Move pointer to proper palette
	palFile.seekg(offset, std::ios::beg);

	Uint8 value[3];

	for (size_t // Correct X-Com colors to RGB colors
			i = 0;
			i < _count && palFile.read((char*)value, 3);
			++i)
	{
		_colors[i].r = static_cast<Uint8>(value[0] * 4);
		_colors[i].g = static_cast<Uint8>(value[1] * 4);
		_colors[i].b = static_cast<Uint8>(value[2] * 4);
		_colors[i].unused = 255;
	}
	_colors[0].unused = 0;

	palFile.close();
}

/**
 * Provides access to colors contained in the palette.
 * @param offset - offset to a specific color (default 0)
 * @return, pointer to the requested SDL_Color
 */
SDL_Color* Palette::getColors(int offset) const
{
	return _colors + offset;
}

/**
 * Converts an SDL_Color struct into an hexadecimal RGBA color value.
 * Mostly used for operations with SDL_gfx that require colors in this format.
 * @param pal	- pointer to requested palette
 * @param color	- requested color in the palette
 * @return, hexadecimal RGBA value
 */
Uint32 Palette::getRGBA(
		SDL_Color* pal,
		Uint8 color)
{
	return ((Uint32)pal[color].r << 24) | ((Uint32)pal[color].g << 16) | ((Uint32)pal[color].b << 8) | (Uint32)0xFF;
}

/**
 * About TFTD i'd guess.
 * @param pal		- pointer to SDL_Color
 * @param qColors	- quantity of colors
 *
void Palette::setColors(
		SDL_Color* pal,
		int qColors)
{
	if (_colors != 0)
	{
		throw Exception("Palette::setColors can be run only once");
	}

	_count = static_cast<size_t>(qColors);
	_colors = new SDL_Color[_count];
	std::memset(_colors, 0, sizeof(SDL_Color) * _count);

	for (size_t i = 0; i < _count; ++i) // Correct X-Com colors to RGB colors
	{
		// TFTD's LBM colors are good the way they are - no need for adjustment here, except...
		_colors[i].r = pal[i].r;
		_colors[i].g = pal[i].g;
		_colors[i].b = pal[i].b;
		_colors[i].unused = 255;

		if (i > 15
			&& _colors[i].r == _colors[0].r
			&& _colors[i].g == _colors[0].g
			&& _colors[i].b == _colors[0].b)
		{
			// SDL "optimizes" surfaces by using RGB colour matching to reassign pixels to an "earlier" matching colour in the palette,
			// meaning any pixels in a surface that are meant to be black will be reassigned as colour 0, rendering them transparent.
			// avoid this eventuality by altering the "later" colours just enough to disambiguate them without causing them to look significantly different.
			// SDL 2.0 has some functionality that should render this hack unnecessary.
			++_colors[i].r;
			++_colors[i].g;
			++_colors[i].b;
		}
	}
	_colors[0].unused = 0;
} */

/**
 * Writes a palette file from TestState.
 * @param file - reference the file
 *
void Palette::savePal(const std::string& file) const
{
	std::ofstream palFile (file.c_str(), std::ios::out | std::ios::binary); // init.
	short qColors = static_cast<short>(_count);

	palFile << "RIFF"; // RIFF header
	const int bytes = 4 + 4 + 4 + 4 + 2 + 2 + (static_cast<int>(qColors) * 4);
	palFile.write((char*)&bytes, sizeof(bytes));
	palFile << "PAL ";

	palFile << "data"; // Data chunk
	const int data = (qColors * 4) + 4;
	palFile.write((char*)&data, sizeof(data));
	const short version = 0x0300;
	palFile.write((char*)&version, sizeof(version));
	palFile.write((char*)&qColors, sizeof(qColors));

	SDL_Color* color = getColors(); // Colors
	for (short i = 0; i != qColors; ++i)
	{
		char ch = 0;
		palFile.write((char*)&color->r, 1);
		palFile.write((char*)&color->g, 1);
		palFile.write((char*)&color->b, 1);
		palFile.write(&ch, 1);
		++color;
	}
	palFile.close();
} */

}
