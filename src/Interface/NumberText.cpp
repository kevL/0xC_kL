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

#include "NumberText.h"

#include <sstream>


namespace OpenXcom
{

bool NumberText::init = true; // static.

Surface
	* NumberText::_chars[DIGITS + 1u]  {},	// static.
	* NumberText::_charsBorder[DIGITS] {};	// static.


/**
 * Sets up a NumberText.
 * @param width		- width in pixels
 * @param height	- height in pixels
 * @param x			- x-position in pixels (default 0)
 * @param y			- y-position in pixels (default 0)
 * @param append	- true to append an 'h' (default false)
 */
NumberText::NumberText(
		int width,
		int height,
		int x,
		int y,
		bool append)
	:
		Surface(
			width,
			height,
			x,y),
		_append(append),
		_value(0u),
		_color(0u),
		_colorBorder(0u),
		_bordered(false)
{
	if (init == true)
		createStaticSurfaces();
}

/**
 * dTor.
 */
NumberText::~NumberText()
{}

/**
 * Deletes all static Surfaces.
 */
void NumberText::deleteStaticSurfaces() // static.
{
	if (init == false)
	{
		for (size_t
				i = 0u;
				i != DIGITS;
				++i)
		{
			delete _chars[i];
			delete _charsBorder[i];
		}
		delete _chars[10u]; // 'h'
	}
}

/**
 * Creates digits as Surfaces.
 */
void NumberText::createStaticSurfaces() // private/static.
{
	init = false;

	_chars[0u] = new Surface(WIDTH, HEIGHT);
	_chars[0u]->lock();
	_chars[0u]->setPixelColor(0,0, FG);
	_chars[0u]->setPixelColor(1,0, FG);
	_chars[0u]->setPixelColor(2,0, FG);
	_chars[0u]->setPixelColor(0,1, FG);
	_chars[0u]->setPixelColor(0,2, FG);
	_chars[0u]->setPixelColor(0,3, FG);
	_chars[0u]->setPixelColor(2,1, FG);
	_chars[0u]->setPixelColor(2,2, FG);
	_chars[0u]->setPixelColor(2,3, FG);
	_chars[0u]->setPixelColor(0,4, FG);
	_chars[0u]->setPixelColor(1,4, FG);
	_chars[0u]->setPixelColor(2,4, FG);
	_chars[0u]->unlock();

	_chars[1u] = new Surface(WIDTH, HEIGHT);
	_chars[1u]->lock();
	_chars[1u]->setPixelColor(1,0, FG);
	_chars[1u]->setPixelColor(1,1, FG);
	_chars[1u]->setPixelColor(1,2, FG);
	_chars[1u]->setPixelColor(1,3, FG);
	_chars[1u]->setPixelColor(0,4, FG);
	_chars[1u]->setPixelColor(1,4, FG);
	_chars[1u]->setPixelColor(2,4, FG);
	_chars[1u]->setPixelColor(0,1, FG);
	_chars[1u]->unlock();

	_chars[2u] = new Surface(WIDTH, HEIGHT);
	_chars[2u]->lock();
	_chars[2u]->setPixelColor(0,0, FG);
	_chars[2u]->setPixelColor(1,0, FG);
	_chars[2u]->setPixelColor(2,0, FG);
	_chars[2u]->setPixelColor(2,1, FG);
	_chars[2u]->setPixelColor(0,2, FG);
	_chars[2u]->setPixelColor(1,2, FG);
	_chars[2u]->setPixelColor(2,2, FG);
	_chars[2u]->setPixelColor(0,3, FG);
	_chars[2u]->setPixelColor(0,4, FG);
	_chars[2u]->setPixelColor(1,4, FG);
	_chars[2u]->setPixelColor(2,4, FG);
	_chars[2u]->unlock();

	_chars[3u] = new Surface(WIDTH, HEIGHT);
	_chars[3u]->lock();
	_chars[3u]->setPixelColor(0,0, FG);
	_chars[3u]->setPixelColor(1,0, FG);
	_chars[3u]->setPixelColor(2,0, FG);
	_chars[3u]->setPixelColor(2,1, FG);
	_chars[3u]->setPixelColor(2,2, FG);
	_chars[3u]->setPixelColor(2,3, FG);
	_chars[3u]->setPixelColor(0,2, FG);
	_chars[3u]->setPixelColor(1,2, FG);
	_chars[3u]->setPixelColor(0,4, FG);
	_chars[3u]->setPixelColor(1,4, FG);
	_chars[3u]->setPixelColor(2,4, FG);
	_chars[3u]->unlock();

	_chars[4u] = new Surface(WIDTH, HEIGHT);
	_chars[4u]->lock();
	_chars[4u]->setPixelColor(0,0, FG);
	_chars[4u]->setPixelColor(0,1, FG);
	_chars[4u]->setPixelColor(0,2, FG);
	_chars[4u]->setPixelColor(1,2, FG);
	_chars[4u]->setPixelColor(2,0, FG);
	_chars[4u]->setPixelColor(2,1, FG);
	_chars[4u]->setPixelColor(2,2, FG);
	_chars[4u]->setPixelColor(2,3, FG);
	_chars[4u]->setPixelColor(2,4, FG);
	_chars[4u]->unlock();

	_chars[5u] = new Surface(WIDTH, HEIGHT);
	_chars[5u]->lock();
	_chars[5u]->setPixelColor(0,0, FG);
	_chars[5u]->setPixelColor(1,0, FG);
	_chars[5u]->setPixelColor(2,0, FG);
	_chars[5u]->setPixelColor(0,1, FG);
	_chars[5u]->setPixelColor(0,2, FG);
	_chars[5u]->setPixelColor(1,2, FG);
	_chars[5u]->setPixelColor(2,2, FG);
	_chars[5u]->setPixelColor(2,3, FG);
	_chars[5u]->setPixelColor(0,4, FG);
	_chars[5u]->setPixelColor(1,4, FG);
	_chars[5u]->setPixelColor(2,4, FG);
	_chars[5u]->unlock();

	_chars[6u] = new Surface(WIDTH, HEIGHT);
	_chars[6u]->lock();
	_chars[6u]->setPixelColor(0,0, FG);
	_chars[6u]->setPixelColor(1,0, FG);
	_chars[6u]->setPixelColor(2,0, FG);
	_chars[6u]->setPixelColor(0,1, FG);
	_chars[6u]->setPixelColor(0,2, FG);
	_chars[6u]->setPixelColor(1,2, FG);
	_chars[6u]->setPixelColor(2,2, FG);
	_chars[6u]->setPixelColor(0,3, FG);
	_chars[6u]->setPixelColor(2,3, FG);
	_chars[6u]->setPixelColor(0,4, FG);
	_chars[6u]->setPixelColor(1,4, FG);
	_chars[6u]->setPixelColor(2,4, FG);
	_chars[6u]->unlock();

	_chars[7u] = new Surface(WIDTH, HEIGHT);
	_chars[7u]->lock();
	_chars[7u]->setPixelColor(0,0, FG);
	_chars[7u]->setPixelColor(1,0, FG);
	_chars[7u]->setPixelColor(2,0, FG);
	_chars[7u]->setPixelColor(2,1, FG);
	_chars[7u]->setPixelColor(2,2, FG);
	_chars[7u]->setPixelColor(2,3, FG);
	_chars[7u]->setPixelColor(2,4, FG);
	_chars[7u]->unlock();

	_chars[8u] = new Surface(WIDTH, HEIGHT);
	_chars[8u]->lock();
	_chars[8u]->setPixelColor(0,0, FG);
	_chars[8u]->setPixelColor(1,0, FG);
	_chars[8u]->setPixelColor(2,0, FG);
	_chars[8u]->setPixelColor(0,1, FG);
	_chars[8u]->setPixelColor(0,2, FG);
	_chars[8u]->setPixelColor(0,3, FG);
	_chars[8u]->setPixelColor(2,1, FG);
	_chars[8u]->setPixelColor(2,2, FG);
	_chars[8u]->setPixelColor(2,3, FG);
	_chars[8u]->setPixelColor(1,2, FG);
	_chars[8u]->setPixelColor(0,4, FG);
	_chars[8u]->setPixelColor(1,4, FG);
	_chars[8u]->setPixelColor(2,4, FG);
	_chars[8u]->unlock();

	_chars[9u] = new Surface(WIDTH, HEIGHT);
	_chars[9u]->lock();
	_chars[9u]->setPixelColor(0,0, FG);
	_chars[9u]->setPixelColor(1,0, FG);
	_chars[9u]->setPixelColor(2,0, FG);
	_chars[9u]->setPixelColor(0,1, FG);
	_chars[9u]->setPixelColor(0,2, FG);
	_chars[9u]->setPixelColor(2,1, FG);
	_chars[9u]->setPixelColor(2,2, FG);
	_chars[9u]->setPixelColor(2,3, FG);
	_chars[9u]->setPixelColor(1,2, FG);
	_chars[9u]->setPixelColor(0,4, FG);
	_chars[9u]->setPixelColor(1,4, FG);
	_chars[9u]->setPixelColor(2,4, FG);
	_chars[9u]->unlock();

	for (size_t
			i = 0u;
			i != DIGITS;
			++i)
	{
		_charsBorder[i] = new Surface(WIDTH_B, HEIGHT_B);
		_charsBorder[i]->lock();
		for (int
				y = 0;
				y != HEIGHT_B;
				++y)
		{
			for (int
					x = 0;
					x != WIDTH_B;
					++x)
			{
				_charsBorder[i]->setPixelColor(x,y, BG);
			}
		}
		_chars[i]->blitNShade(
						_charsBorder[i],
						0,0,0);
		_charsBorder[i]->unlock();

	}

	_chars[10u] = new Surface(WIDTH, HEIGHT); // letter 'h'
	_chars[10u]->lock();
	_chars[10u]->setPixelColor(0,0, FG);
	_chars[10u]->setPixelColor(0,1, FG);
	_chars[10u]->setPixelColor(0,2, FG);
	_chars[10u]->setPixelColor(0,3, FG);
	_chars[10u]->setPixelColor(0,4, FG);
	_chars[10u]->setPixelColor(1,2, FG);
	_chars[10u]->setPixelColor(2,2, FG);
	_chars[10u]->setPixelColor(2,3, FG);
	_chars[10u]->setPixelColor(2,4, FG);
	_chars[10u]->unlock();
}

/**
 * Changes the value used to render the digits.
 * @param value - digits value (default 0)
 */
void NumberText::setValue(unsigned value)
{
	_value = value;
	_redraw = true;
}

/**
 * Gets the value used to render the digits.
 * @return, digits value
 */
unsigned NumberText::getValue() const
{
	return _value;
}

/**
 * Sets whether or not to draw a border around the digits.
 * @param bordered - true to border (default true)
 */
void NumberText::setBordered(bool bordered)
{
	_bordered = bordered;
}

/**
 * Sets the color used to render digits.
 * @note Set color to 0 to draw w/ the init color (FG).
 * @param color - color value
 */
void NumberText::setColor(Uint8 color)
{
	_color = color;
	_redraw = true;
}

/**
 * Gets the color used to render digits.
 * @note Returns 0 (not the init color) if no color was specified.
 * @return, color value
 */
Uint8 NumberText::getColor() const
{
	return _color;
}

/**
 * Sets the color used to render the border.
 * @note Set color to 0 to draw w/ the init color (BG).
 * @param color - color value
 */
void NumberText::setColorBorder(Uint8 color)
{
	_colorBorder = color;
	_redraw = true;
}

/**
 * Gets the color used to render the border.
 * @note Returns 0 (not the init color) if no border-color was specified.
 * @return, color value
 */
Uint8 NumberText::getColorBorder() const
{
	return _colorBorder;
}

/**
 * Replaces a specified quantity of colors in this NumberText palette.
 * @param colors		- pointer to the set of colors
 * @param firstcolor	- offset of the first color to replace (default 0)
 * @param ncolors		- amount of colors to replace (default 256)
 */
void NumberText::setPalette(
		SDL_Color* const colors,
		int firstcolor,
		int ncolors)
{
	Surface::setPalette(colors, firstcolor, ncolors);

	for (size_t
			i = 0u;
			i != DIGITS;
			++i)
	{
		_chars[i]->setPalette(colors, firstcolor, ncolors);
		_charsBorder[i]->setPalette(colors, firstcolor, ncolors);
	}
	_chars[10u]->setPalette(colors, firstcolor, ncolors);
}

/**
 * Draws all the digits in the digits.
 */
void NumberText::draw()
{
	Surface::draw();

	std::ostringstream oststr;
	oststr << _value;
	const std::string st (oststr.str());

	int dx (0);
	if (_bordered == false)
	{
		for (std::string::const_iterator
				i = st.begin();
				i != st.end();
				++i)
		{
			_chars[*i - '0']->setX(dx);
			_chars[*i - '0']->setY(0);
			_chars[*i - '0']->blit(this);
			dx += _chars[*i - '0']->getWidth() + 1;
		}

		if (_append == true)
		{
			_chars[10u]->setX(dx);
			_chars[10u]->setY(0);
			_chars[10u]->blit(this);
		}
	}
	else
	{
		for (std::string::const_iterator
				i = st.begin();
				i != st.end();
				++i)
		{
			_charsBorder[*i - '0']->setX(dx);
			_charsBorder[*i - '0']->setY(0);
			_charsBorder[*i - '0']->blit(this);
			dx += _chars[*i - '0']->getWidth() + 1;
		}

		if (_colorBorder != 0u)
		{
			const int pixels (_surface->w * _surface->h);
			int
				x (0),
				y (0);
			Uint8 color;
			for (int
					i = 0;
					i != pixels;
					++i)
			{
				switch (getPixelColor(x,y))
				{
					case 0u:
						color = 0u;
						break;
					case BG:
						color = _colorBorder;
						break;

					default:
						color = FG;
				}
				setPixelIterative(&x,&y, color);
			}
		}
	}

	if (_color != 0u)
	{
		const int pixels (_surface->w * _surface->h);
		int
			x (0),
			y (0);
		Uint8 color;
		for (int
				i = 0;
				i != pixels;
				++i)
		{
			switch (getPixelColor(x,y))
			{
				case 0u:
					color = 0u;
					break;
				case FG:
					color = _color;
					break;

				default:
					if (_colorBorder == 0u)
						color = BG;
					else
						color = _colorBorder;
			}
			setPixelIterative(&x,&y, color);
		}
	}
}

}
