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

#include "Bar.h"

#include "../fmath.h"

//#include <SDL.h>


namespace OpenXcom
{

/**
 * Sets up a blank Bar with the specified size and position.
 * @param width		- width in pixels
 * @param height	- height in pixels
 * @param x			- x-position in pixels (default 0)
 * @param y			- y-position in pixels (default 0)
 */
Bar::Bar(
		int width,
		int height,
		int x,
		int y)
	:
		Surface(
			width,
			height,
			x,y),
		_color(0u),
		_color2(0u),
		_borderColor(0u),
		_scale(1.),
		_maxVal(100.),
		_value(0.),
		_value2(0.),
		_invert(false),
		_secondOnTop(true),
		_offSecond_y(0)
{}

/**
 * dTor
 */
Bar::~Bar()
{}

/**
 * Sets the color used to draw the border and contents.
 * @param color - color value
 */
void Bar::setColor(Uint8 color)
{
	_color = color;
	_redraw = true;
}

/**
 * Gets the color used to draw this Bar.
 * @return, color value
 */
Uint8 Bar::getColor() const
{
	return _color;
}

/**
 * Sets the color used to draw the second contents.
 * @param color - color value
 */
void Bar::setSecondaryColor(Uint8 color)
{
	_color2 = color;
	_redraw = true;
}

/**
 * Gets the second color used to draw this Bar.
 * @return, color value
 */
Uint8 Bar::getSecondaryColor() const
{
	return _color2;
}

/**
 * Sets the scale-factor used to draw this Bar's values.
 * @param scale - scale in pixels/unit (default 1.0)
 */
void Bar::setScale(double scale)
{
	_scale = scale;
	_redraw = true;
}

/**
 * Gets the scale-factor used to draw this Bar's values.
 * @return, scale in pixels/unit
 */
double Bar::getScale() const
{
	return _scale;
}

/**
 * Sets the maximum value used to draw the outer border.
 * @param maxVal - maximum value (default 100.)
 */
void Bar::setMaxValue(double maxVal)
{
	_maxVal = maxVal;
	_redraw = true;
}

/**
 * Gets the maximum value used to draw the outer border.
 * @return, maximum value
 *
double Bar::getMax() const
{
	return _maxVal;
} */

/**
 * Sets the value used to draw the inner contents.
 * @param value - current value
 */
void Bar::setValue(double value)
{
	if (value < 0.) value = 0.;

	_value = value;
	_redraw = true;
}

/**
 * Gets the value used to draw the inner contents.
 * @return, current value
 */
double Bar::getValue() const
{
	return _value;
}

/**
 * Sets the value used to draw the second inner contents.
 * @param value - current value
 */
void Bar::setValue2(double value)
{
	if (value < 0.) value = 0.;

	_value2 = value;
	_redraw = true;
}

/**
 * Gets the value used to draw the second inner contents.
 * @return, current value
 */
double Bar::getValue2() const
{
	return _value2;
}

/**
 * Defines whether the second value should be drawn on top.
 * @param onTop - true if second value on top (default true)
 */
void Bar::setSecondValueOnTop(bool onTop)
{
	_secondOnTop = onTop;
}

/**
 * Offsets y-value of second Bar.
 * @note Only works if second is on top.
 * @param y - amount of y to offset by
 */
void Bar::offsetSecond(int y)
{
	_offSecond_y = y;
}

/**
 * Enables/disables color inverting.
 * Some bars have darker borders and others have lighter borders.
 * @param invert - invert setting (default true)
 */
void Bar::setInvert(bool invert)
{
	_invert = invert;
	_redraw = true;
}

/**
 * Draws the bordered Bar filled according to its values.
 */
void Bar::draw()
{
	Surface::draw();

	SDL_Rect rect;
	rect.x =
	rect.y = 0;
	rect.w = static_cast<Uint16>(static_cast<unsigned>(Round(_scale * _maxVal)) + 1u);
	rect.h = static_cast<Uint16>(getHeight());

	if (_invert == true)
		drawRect(&rect, _color);
	else if (_borderColor != 0u)
		drawRect(&rect, _borderColor);
	else
		drawRect(&rect, static_cast<Uint8>(static_cast<unsigned>(_color) + 6u)); // was +4 but red is wonky.

	++rect.y;
	--rect.w;
	rect.h = static_cast<Uint16>(static_cast<unsigned>(rect.h) - 2u);
	drawRect(&rect, 0u);

	double
		width  (_scale * _value),
		width2 (_scale * _value2);

	if (width > 0. && width < 1.) // these ensure that miniscule amounts still show up.
		width = 1.;
	if (width2 > 0. && width2 < 1.)
		width2 = 1.;

	if (_invert == true)
	{
		if (_secondOnTop == true)
		{
			rect.w = static_cast<Uint16>(Round(width));
			drawRect(&rect, static_cast<Uint8>(static_cast<unsigned>(_color) + 4u));

			rect.w = static_cast<Uint16>(Round(width2));
			rect.y = static_cast<Sint16>(static_cast<int>(rect.y) + _offSecond_y);
			drawRect(&rect, static_cast<Uint8>(static_cast<unsigned>(_color2) + 4u));
		}
		else
		{
			rect.w = static_cast<Uint16>(Round(width2));
			drawRect(&rect, static_cast<Uint8>(static_cast<unsigned>(_color2) + 4u));

			rect.w = static_cast<Uint16>(Round(width));
			drawRect(&rect, static_cast<Uint8>(static_cast<unsigned>(_color) + 4u));
		}
	}
	else
	{
		if (_secondOnTop == true)
		{
			rect.w = static_cast<Uint16>(Round(width));
			drawRect(&rect, _color);

			rect.w = static_cast<Uint16>(Round(width2));
			rect.y = static_cast<Sint16>(static_cast<int>(rect.y) + _offSecond_y);
			drawRect(&rect, _color2);
		}
		else
		{
			rect.w = static_cast<Uint16>(Round(width2));
			drawRect(&rect, _color2);

			rect.w = static_cast<Uint16>(Round(width));
			drawRect(&rect, _color);
		}
	}
}

/**
 * Sets the border-color for this Bar.
 * @note Will use base color+4 if none is defined here.
 * @param color - the color for the outline of the bar
 */
void Bar::setBorderColor(Uint8 color)
{
	_borderColor = color;
}

}
