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

bool NumberText::init = true;

Surface
	* NumberText::_chars[DIGITS + 1u] = {},
	* NumberText::_charsBorder[DIGITS] = {};


/**
 * Sets up a NumberText.
 * @param width		- width in pixels
 * @param height	- height in pixels
 * @param x			- X position in pixels (default 0)
 * @param y			- Y position in pixels (default 0)
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
		delete _chars[10];
	}
}

/**
 * Creates digits as Surfaces.
 */
void NumberText::createStaticSurfaces() // private/static.
{
	init = false;

	_chars[0] = new Surface(WIDTH, HEIGHT);
	_chars[0]->lock();
	_chars[0]->setPixelColor(0,0, FG);
	_chars[0]->setPixelColor(1,0, FG);
	_chars[0]->setPixelColor(2,0, FG);
	_chars[0]->setPixelColor(0,1, FG);
	_chars[0]->setPixelColor(0,2, FG);
	_chars[0]->setPixelColor(0,3, FG);
	_chars[0]->setPixelColor(2,1, FG);
	_chars[0]->setPixelColor(2,2, FG);
	_chars[0]->setPixelColor(2,3, FG);
	_chars[0]->setPixelColor(0,4, FG);
	_chars[0]->setPixelColor(1,4, FG);
	_chars[0]->setPixelColor(2,4, FG);
	_chars[0]->unlock();

	_chars[1] = new Surface(WIDTH, HEIGHT);
	_chars[1]->lock();
	_chars[1]->setPixelColor(1,0, FG);
	_chars[1]->setPixelColor(1,1, FG);
	_chars[1]->setPixelColor(1,2, FG);
	_chars[1]->setPixelColor(1,3, FG);
	_chars[1]->setPixelColor(0,4, FG);
	_chars[1]->setPixelColor(1,4, FG);
	_chars[1]->setPixelColor(2,4, FG);
	_chars[1]->setPixelColor(0,1, FG);
	_chars[1]->unlock();

	_chars[2] = new Surface(WIDTH, HEIGHT);
	_chars[2]->lock();
	_chars[2]->setPixelColor(0,0, FG);
	_chars[2]->setPixelColor(1,0, FG);
	_chars[2]->setPixelColor(2,0, FG);
	_chars[2]->setPixelColor(2,1, FG);
	_chars[2]->setPixelColor(0,2, FG);
	_chars[2]->setPixelColor(1,2, FG);
	_chars[2]->setPixelColor(2,2, FG);
	_chars[2]->setPixelColor(0,3, FG);
	_chars[2]->setPixelColor(0,4, FG);
	_chars[2]->setPixelColor(1,4, FG);
	_chars[2]->setPixelColor(2,4, FG);
	_chars[2]->unlock();

	_chars[3] = new Surface(WIDTH, HEIGHT);
	_chars[3]->lock();
	_chars[3]->setPixelColor(0,0, FG);
	_chars[3]->setPixelColor(1,0, FG);
	_chars[3]->setPixelColor(2,0, FG);
	_chars[3]->setPixelColor(2,1, FG);
	_chars[3]->setPixelColor(2,2, FG);
	_chars[3]->setPixelColor(2,3, FG);
	_chars[3]->setPixelColor(0,2, FG);
	_chars[3]->setPixelColor(1,2, FG);
	_chars[3]->setPixelColor(0,4, FG);
	_chars[3]->setPixelColor(1,4, FG);
	_chars[3]->setPixelColor(2,4, FG);
	_chars[3]->unlock();

	_chars[4] = new Surface(WIDTH, HEIGHT);
	_chars[4]->lock();
	_chars[4]->setPixelColor(0,0, FG);
	_chars[4]->setPixelColor(0,1, FG);
	_chars[4]->setPixelColor(0,2, FG);
	_chars[4]->setPixelColor(1,2, FG);
	_chars[4]->setPixelColor(2,0, FG);
	_chars[4]->setPixelColor(2,1, FG);
	_chars[4]->setPixelColor(2,2, FG);
	_chars[4]->setPixelColor(2,3, FG);
	_chars[4]->setPixelColor(2,4, FG);
	_chars[4]->unlock();

	_chars[5] = new Surface(WIDTH, HEIGHT);
	_chars[5]->lock();
	_chars[5]->setPixelColor(0,0, FG);
	_chars[5]->setPixelColor(1,0, FG);
	_chars[5]->setPixelColor(2,0, FG);
	_chars[5]->setPixelColor(0,1, FG);
	_chars[5]->setPixelColor(0,2, FG);
	_chars[5]->setPixelColor(1,2, FG);
	_chars[5]->setPixelColor(2,2, FG);
	_chars[5]->setPixelColor(2,3, FG);
	_chars[5]->setPixelColor(0,4, FG);
	_chars[5]->setPixelColor(1,4, FG);
	_chars[5]->setPixelColor(2,4, FG);
	_chars[5]->unlock();

	_chars[6] = new Surface(WIDTH, HEIGHT);
	_chars[6]->lock();
	_chars[6]->setPixelColor(0,0, FG);
	_chars[6]->setPixelColor(1,0, FG);
	_chars[6]->setPixelColor(2,0, FG);
	_chars[6]->setPixelColor(0,1, FG);
	_chars[6]->setPixelColor(0,2, FG);
	_chars[6]->setPixelColor(1,2, FG);
	_chars[6]->setPixelColor(2,2, FG);
	_chars[6]->setPixelColor(0,3, FG);
	_chars[6]->setPixelColor(2,3, FG);
	_chars[6]->setPixelColor(0,4, FG);
	_chars[6]->setPixelColor(1,4, FG);
	_chars[6]->setPixelColor(2,4, FG);
	_chars[6]->unlock();

	_chars[7] = new Surface(WIDTH, HEIGHT);
	_chars[7]->lock();
	_chars[7]->setPixelColor(0,0, FG);
	_chars[7]->setPixelColor(1,0, FG);
	_chars[7]->setPixelColor(2,0, FG);
	_chars[7]->setPixelColor(2,1, FG);
	_chars[7]->setPixelColor(2,2, FG);
	_chars[7]->setPixelColor(2,3, FG);
	_chars[7]->setPixelColor(2,4, FG);
	_chars[7]->unlock();

	_chars[8] = new Surface(WIDTH, HEIGHT);
	_chars[8]->lock();
	_chars[8]->setPixelColor(0,0, FG);
	_chars[8]->setPixelColor(1,0, FG);
	_chars[8]->setPixelColor(2,0, FG);
	_chars[8]->setPixelColor(0,1, FG);
	_chars[8]->setPixelColor(0,2, FG);
	_chars[8]->setPixelColor(0,3, FG);
	_chars[8]->setPixelColor(2,1, FG);
	_chars[8]->setPixelColor(2,2, FG);
	_chars[8]->setPixelColor(2,3, FG);
	_chars[8]->setPixelColor(1,2, FG);
	_chars[8]->setPixelColor(0,4, FG);
	_chars[8]->setPixelColor(1,4, FG);
	_chars[8]->setPixelColor(2,4, FG);
	_chars[8]->unlock();

	_chars[9] = new Surface(WIDTH, HEIGHT);
	_chars[9]->lock();
	_chars[9]->setPixelColor(0,0, FG);
	_chars[9]->setPixelColor(1,0, FG);
	_chars[9]->setPixelColor(2,0, FG);
	_chars[9]->setPixelColor(0,1, FG);
	_chars[9]->setPixelColor(0,2, FG);
	_chars[9]->setPixelColor(2,1, FG);
	_chars[9]->setPixelColor(2,2, FG);
	_chars[9]->setPixelColor(2,3, FG);
	_chars[9]->setPixelColor(1,2, FG);
	_chars[9]->setPixelColor(0,4, FG);
	_chars[9]->setPixelColor(1,4, FG);
	_chars[9]->setPixelColor(2,4, FG);
	_chars[9]->unlock();

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

	_chars[10] = new Surface(WIDTH, HEIGHT); // letter 'h'
	_chars[10]->lock();
	_chars[10]->setPixelColor(0,0, FG);
	_chars[10]->setPixelColor(0,1, FG);
	_chars[10]->setPixelColor(0,2, FG);
	_chars[10]->setPixelColor(0,3, FG);
	_chars[10]->setPixelColor(0,4, FG);
	_chars[10]->setPixelColor(1,2, FG);
	_chars[10]->setPixelColor(2,2, FG);
	_chars[10]->setPixelColor(2,3, FG);
	_chars[10]->setPixelColor(2,4, FG);
	_chars[10]->unlock();
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
 * Returns the value used to render the digits.
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
 * Changes the color used to render the digits.
 * @param color - color value
 */
void NumberText::setColor(Uint8 color)
{
	_color = color;
	_redraw = true;
}

/**
 * Returns the color used to render the digits.
 * @return, color value
 */
Uint8 NumberText::getColor() const
{
	return _color;
}

/**
 * Changes the color used to render the border.
 * @param color - color value
 */
void NumberText::setColorBorder(Uint8 color)
{
	_colorBorder = color;
	_redraw = true;
}

/**
 * Returns the color used to render the border.
 * @return, color value
 */
Uint8 NumberText::getColorBorder() const
{
	return _colorBorder;
}

/**
 * Replaces a certain amount of colors in this NumberText palette.
 * @param colors		- pointer to the set of colors
 * @param firstcolor	- offset of the first color to replace (default 0)
 * @param ncolors		- amount of colors to replace (default 256)
 */
void NumberText::setPalette(
		SDL_Color* colors,
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
	_chars[10]->setPalette(colors, firstcolor, ncolors);
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

	int x = 0;
	if (_bordered == false)
	{
		for (std::string::const_iterator
				i = st.begin();
				i != st.end();
				++i)
		{
			_chars[*i - '0']->setX(x);
			_chars[*i - '0']->setY(0);
			_chars[*i - '0']->blit(this);
			x += _chars[*i - '0']->getWidth() + 1;
		}

		if (_append == true)
		{
			_chars[10]->setX(x);
			_chars[10]->setY(0);
			_chars[10]->blit(this);
		}
	}
	else
	{
		for (std::string::const_iterator
				i = st.begin();
				i != st.end();
				++i)
		{
			_charsBorder[*i - '0']->setX(x);
			_charsBorder[*i - '0']->setY(0);
			_charsBorder[*i - '0']->blit(this);
			x += _chars[*i - '0']->getWidth() + 1;
		}

		if (_colorBorder != 0u)
		{
			const int pixels (_surface->w * _surface->h);
			int
				x = 0,
				y = 0;
			Uint8 color;
			for (int
					i = 0;
					i != pixels;
					++i)
			{
				switch (getPixelColor(x,y))
				{
					case 0:
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
			x = 0,
			y = 0;
		Uint8 color;
		for (int
				i = 0;
				i != pixels;
				++i)
		{
			switch (getPixelColor(x,y))
			{
				case  0:
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
