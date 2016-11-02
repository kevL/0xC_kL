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

#include "Cursor.h"

#include <cmath>

//#include <SDL/SDL.h>

#include "../Engine/Action.h"


namespace OpenXcom
{

/**
 * Sets up a cursor with the specified size and position and hides the system cursor.
 * @note The size and position don't really matter since it's a 9x13 shape.
 * They're just for inheritance.
 * @param width		- width in pixels
 * @param height	- height in pixels
 * @param x			- x-position in pixels (default 0)
 * @param y			- y-position in pixels (default 0)
 */
Cursor::Cursor(
		int width,
		int height,
		int x,
		int y)
	:
		Surface(
			width,
			height,
			x,y),
		_color(0),
		_fakeMotion(false)
{}

/**
 * dTor.
 */
Cursor::~Cursor()
{}

/**
 * Automatically updates the cursor position when the mouse moves.
 * @param action - pointer to an Action
 */
void Cursor::handle(Action* action)
{
	if (action->getDetails()->type == SDL_MOUSEMOTION)
	{
		if (_fakeMotion == false)
		{
			setX(static_cast<int>(
				 static_cast<double>(
				 static_cast<int>(action->getDetails()->motion.x) - action->getBorderLeft())
					/ action->getScaleX()));
			setY(static_cast<int>(
				 static_cast<double>(
				 static_cast<int>(action->getDetails()->motion.y) - action->getBorderTop())
					/ action->getScaleY()));
		}
		else
			_fakeMotion = false;
	}
}

/**
 * Informs the cursor not to bother.
 * @note This is needed to prevent rounding errors in handle() when the Map is
 * scrolled or jumped by keyboard. The cursor tends to go off by a pixel on
 * every third call to BattlescapeState::refreshMousePosition() otherwise.
 */
void Cursor::fakeMotion()
{
	_fakeMotion = true;
}

/**
 * Changes the cursor's base color.
 * @param color - color value
 */
void Cursor::setColor(Uint8 color)
{
	_color = color;
	_redraw = true;
}

/**
 * Gets the cursor's base color.
 * @return, color value
 */
Uint8 Cursor::getColor() const
{
	return _color;
}

/**
 * Draws a pointer-shaped cursor graphic.
 */
void Cursor::draw()
{
	Surface::draw();

	Uint8 color (_color);
	Sint16
		x1 (0),
		y1 (0),
		x2 (static_cast<Sint16>(getWidth()  - 1)),
		y2 (static_cast<Sint16>(getHeight() - 1));

	lock();
	for (int
			i = 0;
			i != 4;
			++i)
	{
		drawLine(
				x1,y1,
				x1,y2,
				color);
		drawLine(
				x1,y1,
				x2,
				static_cast<Sint16>(getWidth() - 1),
				color);

		++x1;
		y1 = static_cast<Sint16>(y1 + 2);

		--y2;
		--x2;

		++color;
	}

	this->setPixelColor(
					4,8,
					--color);
	unlock();
}

}
