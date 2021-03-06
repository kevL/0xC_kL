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

#include "Font.h"

#include "CrossPlatform.h"
#include "DosFont.h"
//#include "Language.h"
#include "Surface.h"


namespace OpenXcom
{

std::wstring Font::_index;

SDL_Color Font::_palette[6u]
{
	{  0,   0,   0,   0},
	{255, 255, 255, 255},
	{207, 207, 207, 255},
	{159, 159, 159, 255},
	{111, 111, 111, 255},
	{ 63,  63,  63, 255}
};

SDL_Color Font::_terminal[2u]
{
	{  0,   0,   0,   0},
	{192, 192, 192, 255}
};

/**
 * Initializes this Font with a blank Surface.
 */
Font::Font()
	:
		_surface(nullptr),
		_width(0),
		_height(0),
		_spacing(0),
		_monospace(false)
{}

/**
 * Deletes this Font's Surface.
 */
Font::~Font()
{
	delete _surface;
}

/**
 * Loads the characters contained in each Font from a UTF-8 string to use as the
 * index.
 * @param index - reference to a string of characters
 */
void Font::setIndex(const std::wstring& index) // static.
{
	_index = index;
}

/**
 * Loads this Font from a YAML file.
 * @param node - refrence a YAML node
 */
void Font::load(const YAML::Node& node)
{
	_width		= node["width"]		.as<int>(_width);
	_height		= node["height"]	.as<int>(_height);
	_spacing	= node["spacing"]	.as<int>(_spacing);
	_monospace	= node["monospace"]	.as<bool>(_monospace);

	const std::string glyphs ("Language/" + node["glyphs"].as<std::string>());

	Surface* const font (new Surface(_width, _height));
	font->loadImage(CrossPlatform::getDataFile(glyphs));

	_surface = new Surface(
						font->getWidth(),
						font->getHeight());
	_surface->setPalette(_palette, 0,6);
	font->blit(_surface);

	delete font;

	init();
}

/**
 * Generates a pre-defined Codepage 437 (MS-DOS terminal) font.
 * @note Used for the DOS-Art load-screen during StartState.
 */
void Font::loadTerminal()
{
	_width = 9;
	_height = 16;
	_spacing = 0;
	_monospace = true;

	SDL_RWops* const rw (SDL_RWFromConstMem(dosFont, DOSFONT_SIZE));
	SDL_Surface* const srf (SDL_LoadBMP_RW(rw, 0));
	SDL_FreeRW(rw);

	_surface = new Surface(srf->w, srf->h);
	_surface->setPalette(_terminal, 0,2);

	SDL_BlitSurface(
				srf,
				nullptr,
				_surface->getSurface(),
				nullptr);
	SDL_FreeSurface(srf);

	const std::wstring id (_index);
	_index = L" !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~";

	init();

	_index = id;
}

/**
 * Calculates the real size and position of each character in the Surface
 * and stores them in SDL_Rect's for future use by other classes.
 */
void Font::init()
{
	_surface->lock();
	const int len (_surface->getWidth() / _width);

	if (_monospace == true)
	{
		for (size_t
				i = 0u;
				i != _index.length();
				++i)
		{
			SDL_Rect rect;
			const int
				startX (static_cast<int>(i) % len * _width),
				startY (static_cast<int>(i) / len * _height);

			rect.x = static_cast<Sint16>(startX);
			rect.y = static_cast<Sint16>(startY);
			rect.w = static_cast<Uint16>(_width);
			rect.h = static_cast<Uint16>(_height);

			_chars[_index[i]] = rect;
		}
	}
	else
	{
		for (size_t
				i = 0u;
				i != _index.length();
				++i)
		{
			SDL_Rect rect;
			const int
				startX (static_cast<int>(i) % len * _width),
				startY (static_cast<int>(i) / len * _height);
			int
				left  (-1),
				right (-1);

			for (int
					x = startX;
					x != startX + _width;
					++x)
			{
				for (int
						y = startY;
						y != startY + _height && left == -1;
						++y)
				{
					if (_surface->getPixelColor(x,y) != 0)
						left = x;
				}
			}

			for (int
					x = startX + _width - 1;
					x >= startX;
					--x)
			{
				for (int
						y = startY + _height;
						y-- != startY && right == -1;
						)
				{
					if (_surface->getPixelColor(x,y) != 0)
						right = x;
				}
			}

			rect.x = static_cast<Sint16>(left);
			rect.y = static_cast<Sint16>(startY);
			rect.w = static_cast<Uint16>(right - left + 1);
			rect.h = static_cast<Uint16>(_height);

			_chars[_index[i]] = rect;
		}
	}
	_surface->unlock();
}

/**
 * Returns a particular character from the set stored in this Font.
 * @param fontChar - character to use for size/position
 * @return, pointer to the Surface with the respective cropping rectangle
 */
Surface* Font::getChar(wchar_t fontChar)
{
	if (_chars.find(fontChar) != _chars.end())
	{
		_surface->getCrop()->x = _chars[fontChar].x;
		_surface->getCrop()->y = _chars[fontChar].y;
		_surface->getCrop()->w = _chars[fontChar].w;
		_surface->getCrop()->h = _chars[fontChar].h;

		return _surface;
	}
	return nullptr;
}

/**
 * Gets the maximum width for any character in this Font.
 * @return, width in pixels
 */
int Font::getWidth() const
{
	return _width;
}

/**
 * Gets the maximum height for any character in this Font.
 * @return, height in pixels
 */
int Font::getHeight() const
{
	return _height;
}

/**
 * Gets the spacing for any character in this Font.
 * @return, spacing in pixels
 * @note This does not refer to character spacing within the surface
 * but to the spacing used between multiple characters on a line.
 */
int Font::getSpacing() const
{
	return _spacing;
}

/**
 * Gets the dimensions of a particular character in this Font.
 * @param fontChar - font character
 * @return, width and height dimensions (x/y are set as safeties)
 */
SDL_Rect Font::getCharSize(wchar_t fontChar)
{
	SDL_Rect charSize {0,0,0u,0u};

	if (fontChar != TOKEN_FLIP_COLORS
		&& isLinebreak(fontChar) == false
		&& isSpace(fontChar) == false)
	{
		if (_chars.find(fontChar) == _chars.end()) 
			fontChar = L'_';

		charSize.w = static_cast<Uint16>(_chars[fontChar].w + _spacing);
		charSize.h = static_cast<Uint16>(_chars[fontChar].h + _spacing);
	}
	else
	{
		if (_monospace == true)
			charSize.w = static_cast<Uint16>(_width + _spacing);
		else if (isNonBreakableSpace(fontChar) == true)
			charSize.w = static_cast<Uint16>(_width >> 2u);
		else
			charSize.w = static_cast<Uint16>(_width >> 1u);

		charSize.h = static_cast<Uint16>(_height + _spacing);
	}

	charSize.x = static_cast<Sint16>(charSize.w); // in case anyone mixes them up
	charSize.y = static_cast<Sint16>(charSize.h);

	return charSize;
}

/**
 * Gets the Surface stored within this Font.
 * @note Used for loading the actual graphic into this Font.
 * @return, pointer to the internal Surface
 */
Surface* Font::getSurface() const
{
	return _surface;
}

}
