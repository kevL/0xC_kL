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

#include "Frame.h"

#include "../Engine/Palette.h"


namespace OpenXcom
{

/**
 * Sets up a blank Frame with the specified size and position.
 * @param width		- width in pixels
 * @param height	- height in pixels
 * @param x			- x-position in pixels (default 0)
 * @param y			- y-position in pixels (default 0)
 */
Frame::Frame(
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
		_colorBg(0u),
		_thickness(5),
		_contrast(1)
{}

/**
 * dTor.
 */
Frame::~Frame()
{}

/**
 * Changes the color used to draw the shaded border.
 * @param color - color value
 */
void Frame::setColor(Uint8 color)
{
	_color = color;
	_redraw = true;
}

/**
 * Changes the color used to draw the shaded border.
 * @note Only really to be used in conjunction with the State::add()
 * function as a convenient wrapper to avoid ugly quacks at that end;
 * better to have them here!
 * @param color - color value
 */
void Frame::setBorderColor(Uint8 color)
{
	_color = color; // NOTE: This simply overwrites setColor().
	_redraw = true; // I'd guess its purpose is to alias "border" in Interfaces.rul to "color".
}

/**
 * Gets the color used to draw the shaded border.
 * @return, color value
 */
Uint8 Frame::getColor() const
{
	return _color;
}

/**
 * Changes the color used to draw the background.
 * @param color - color value
 */
void Frame::setSecondaryColor(Uint8 color)
{
	_colorBg = color;
	_redraw = true;
}

/**
 * Gets the color used to draw the background.
 * @return, color value
 */
Uint8 Frame::getSecondaryColor() const
{
	return _colorBg;
}

/**
 * Enables/disables high-contrast color. Mostly used for Battlescape UI.
 * @param contrast - high-contrast setting (default true)
 */
void Frame::setHighContrast(bool contrast)
{
	_contrast = (contrast == true) ? 2 : 1;
	_redraw = true;
}

/**
 * Changes the thickness of the border to draw.
 * @param thickness - thickness in pixels (default 5)
 */
void Frame::setThickness(int thickness)
{
	_thickness = thickness;
	_redraw = true;
}

/**
 * Draws the bordered frame with a graphic background.
 * The background never moves with the frame, it's  always aligned to the
 * top-left corner of the screen and cropped to fit the inside area.
 */
void Frame::draw()
{
	Surface::draw();

	SDL_Rect rect;
	rect.x =
	rect.y = 0;
	rect.w = static_cast<Uint16>(getWidth());
	rect.h = static_cast<Uint16>(getHeight());

	Uint8
		darkest	(static_cast<Uint8>(Palette::blockOffset(static_cast<Uint8>(_color >> 4u)) + 15u)), // fuck you, c/G++ Thanks.
		// interpretation: darkest = Palette::blockOffset(_color / 16) + 15;
		color	(_color);

	for (int
			i = 0;
			i != _thickness;
			++i)
	{
		if ((_thickness > 1 && i == _thickness - 1)
			|| (color >> 4u) != (_color >> 4u))
		{
			color = darkest;
		}
		else
			color = static_cast<Uint8>(_color + static_cast<Uint8>(std::abs(i - (_thickness >> 1u)) * _contrast)); // ditto.
			// interpretation: color = _color + (abs(i - _thickness / 2) * _contrast);

		drawRect(&rect, color);

		++rect.x;
		++rect.y;

		if (rect.w > 1u)
			rect.w = static_cast<Uint16>(rect.w - 2u);
		else
			rect.w = 1u;

		if (rect.h > 1u)
			rect.h = static_cast<Uint16>(rect.h - 2u);
		else
			rect.h = 1u;
	}
	drawRect(&rect, _colorBg);
}

}
