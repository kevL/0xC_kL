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

#include "ArrowButton.h"

#include "TextList.h"

#include "../Engine/Action.h"
#include "../Engine/Timer.h"


namespace OpenXcom
{

/**
 * Sets up the ArrowButton with the specified size and position.
 * @param shape		- shape of the arrow (ArrowButton.h)
 * @param width		- width in pixels
 * @param height	- height in pixels
 * @param x			- x-position in pixels (default 0)
 * @param y			- y-position in pixels (default 0)
 */
ArrowButton::ArrowButton(
		ArrowShape shape,
		int width,
		int height,
		int x,
		int y)
	:
		ImageButton(
			width,
			height,
			x,y),
		_shape(shape),
		_list(nullptr)
{
	_timer = new Timer(77u);
	_timer->onTimer(static_cast<SurfaceHandler>(&ArrowButton::scroll));
}

/**
 * Deletes the Timer.
 */
ArrowButton::~ArrowButton()
{
	delete _timer;
}

/**
 * Checks if the specified mouse-button has been handled.
 * @param btn - an SDL-button identifier (default 0)
 */
bool ArrowButton::isButtonHandled(Uint8 btn)
{
	if (_list != nullptr)
	{
		switch (btn)
		{
			case SDL_BUTTON_LEFT:
			case SDL_BUTTON_RIGHT:
				return true;

			default:
				return false;
		}
	}
	else
		return ImageButton::isButtonHandled(btn);
}

/**
 * Changes the color for this ImageButton.
 * @param color - color value
 */
void ArrowButton::setColor(Uint8 color)
{
	ImageButton::setColor(color);
	_redraw = true;
}

/**
 * Changes the shape for this ArrowButton.
 * @param shape - shape of the arrow (ArrowButton.h)
 */
void ArrowButton::setShape(ArrowShape shape)
{
	_shape = shape;
	_redraw = true;
}

/**
 * Changes the list associated with this ArrowButton.
 * @note This makes the button scroll that list.
 * @param textList - pointer to TextList
 */
void ArrowButton::setTextList(TextList* const textList)
{
	_list = textList;
}

/**
 * Draws this ArrowButton with the specified arrow-shape.
 */
void ArrowButton::draw()
{
	ImageButton::draw();
	lock();

	SDL_Rect rect; // draw button
	Uint8 color (static_cast<Uint8>(static_cast<unsigned>(_color) + 2u));

	rect.x =
	rect.y = 0;
	rect.w = static_cast<Uint16>(getWidth()  - 1);
	rect.h = static_cast<Uint16>(getHeight() - 1);

	drawRect(&rect, color);

	++rect.x;
	++rect.y;
	color = static_cast<Uint8>(static_cast<unsigned>(_color) + 5u);

	drawRect(&rect, color);

	--rect.w;
	--rect.h;
	color = static_cast<Uint8>(static_cast<unsigned>(_color) + 4u);

	drawRect(&rect, color);

	setPixelColor(
			0,0,
			static_cast<Uint8>(static_cast<unsigned>(_color) + 1u));
	setPixelColor(
			0,
			getHeight() - 1,
			static_cast<Uint8>(static_cast<unsigned>(_color) + 4u));
	setPixelColor(
			getWidth() - 1,
			0,
			static_cast<Uint8>(static_cast<unsigned>(_color) + 4u));

	color = static_cast<Uint8>(static_cast<unsigned>(_color) + 1u);

	switch (_shape)
	{
		case OpenXcom::ARROW_BIG_UP:
			rect.x = 5; // draw arrow square
			rect.y = 8;
			rect.w =
			rect.h = 3u;

			drawRect(&rect, color);

			rect.x = 2; // draw arrow triangle
			rect.y = 7;
			rect.w = 9u;
			rect.h = 1u;

			for (
					;
					rect.w > 1u;
					rect.w = static_cast<Uint16>(static_cast<unsigned>(rect.w) - 2u))
			{
				drawRect(&rect, color);
				++rect.x;
				--rect.y;
			}
			drawRect(&rect, color);
			break;

		case OpenXcom::ARROW_BIG_DOWN:
			rect.x = 5; // draw arrow square
			rect.y = 3;
			rect.w =
			rect.h = 3u;

			drawRect(&rect, color);

			rect.x = 2; // draw arrow triangle
			rect.y = 6;
			rect.w = 9u;
			rect.h = 1u;

			for (
					;
					rect.w > 1u;
					rect.w = static_cast<Uint16>(static_cast<unsigned>(rect.w) - 2u))
			{
				drawRect(&rect, color);
				++rect.x;
				++rect.y;
			}
			drawRect(&rect, color);
			break;

		case OpenXcom::ARROW_SMALL_UP:
			rect.x = 1; // draw arrow triangle 1
			rect.y = 5;
			rect.w = 9u;
			rect.h = 1u;

			for (
					;
					rect.w > 1u;
					rect.w = static_cast<Uint16>(static_cast<unsigned>(rect.w) - 2u))
			{
				drawRect(&rect, static_cast<Uint8>(static_cast<unsigned>(color) + 2u));
				++rect.x;
				--rect.y;
			}
			drawRect(&rect, static_cast<Uint8>(static_cast<unsigned>(color) + 2u));

			rect.x = 2; // draw arrow triangle 2
			rect.y = 5;
			rect.w = 7u;
			rect.h = 1u;

			for (
					;
					rect.w > 1u;
					rect.w = static_cast<Uint16>(static_cast<unsigned>(rect.w) - 2u))
			{
				drawRect(&rect, color);
				++rect.x;
				--rect.y;
			}
			drawRect(&rect, color);
			break;

		case OpenXcom::ARROW_SMALL_DOWN:
			rect.x = 1; // draw arrow triangle 1
			rect.y = 2;
			rect.w = 9u;
			rect.h = 1u;

			for (
					;
					rect.w > 1u;
					rect.w = static_cast<Uint16>(static_cast<unsigned>(rect.w) - 2u))
			{
				drawRect(&rect, static_cast<Uint8>(static_cast<unsigned>(color) + 2u));
				++rect.x;
				++rect.y;
			}
			drawRect(&rect, static_cast<Uint8>(static_cast<unsigned>(color) + 2u));

			rect.x = // draw arrow triangle 2
			rect.y = 2;
			rect.w = 7u;
			rect.h = 1u;

			for (
					;
					rect.w > 1u;
					rect.w = static_cast<Uint16>(static_cast<unsigned>(rect.w) - 2u))
			{
				drawRect(&rect, color);
				++rect.x;
				++rect.y;
			}
			drawRect(&rect, color);
			break;

		case OpenXcom::ARROW_SMALL_LEFT:
			rect.x = 2; // draw arrow triangle 1
			rect.y = 4;
			rect.w = 2u;
			rect.h = 1u;

			for (
					;
					rect.h < 5u;
					rect.h = static_cast<Uint16>(static_cast<unsigned>(rect.h) + 2u))
			{
				drawRect(&rect, static_cast<Uint8>(static_cast<unsigned>(color) + 2u));
				rect.x = static_cast<Sint16>(static_cast<int>(rect.x) + 2);
				--rect.y;
			}
			rect.w = 1u;
			drawRect(&rect, static_cast<Uint8>(static_cast<unsigned>(color) + 2u));

			rect.x = 3; // draw arrow triangle 2
			rect.y = 4;
			rect.w = 2u;
			rect.h = 1u;

			for (
					;
					rect.h < 5u;
					rect.h = static_cast<Uint16>(static_cast<unsigned>(rect.h) + 2u))
			{
				drawRect(&rect, color);
				rect.x = static_cast<Sint16>(static_cast<int>(rect.x) + 2);
				--rect.y;
			}
			rect.w = 1u;
			drawRect(&rect, color);
			break;

		case OpenXcom::ARROW_SMALL_RIGHT:
			rect.x = 7; // draw arrow triangle 1
			rect.y = 4;
			rect.w = 2u;
			rect.h = 1u;

			for (
					;
					rect.h < 5u;
					rect.h = static_cast<Uint16>(static_cast<unsigned>(rect.h) + 2u))
			{
				drawRect(&rect, static_cast<Uint8>(static_cast<unsigned>(color) + 2u));
				rect.x = static_cast<Sint16>(static_cast<int>(rect.x) - 2);
				--rect.y;
			}
			++rect.x;
			rect.w = 1u;
			drawRect(&rect, static_cast<Uint8>(static_cast<unsigned>(color) + 2u));

			rect.x = 6; // draw arrow triangle 2
			rect.y = 4;
			rect.w = 2u;
			rect.h = 1u;

			for (
					;
					rect.h < 5u;
					rect.h = static_cast<Uint16>(static_cast<unsigned>(rect.h) + 2u))
			{
				drawRect(&rect, color);
				rect.x = static_cast<Sint16>(static_cast<int>(rect.x) - 2);
				--rect.y;
			}
			++rect.x;
			rect.w = 1u;
			drawRect(&rect, color);
	}
	unlock();
}

/**
 * Keeps the scrolling Timers running.
 */
void ArrowButton::think()
{
	_timer->think(nullptr, this);
}

/**
 * Scrolls the list.
 */
void ArrowButton::scroll()
{
	switch (_shape)
	{
		case ARROW_BIG_UP:
			_list->scrollUp();
			break;
		case ARROW_BIG_DOWN:
			_list->scrollDown();
	}
}

/**
 * Starts scrolling the associated list.
 * @param action	- pointer to an Action
 * @param state		- State that the ActionHandlers belong to
 */
void ArrowButton::mousePress(Action* action, State* state)
{
	ImageButton::mousePress(action, state);

	if (_list != nullptr)
	{
		switch (action->getDetails()->button.button)
		{
			case SDL_BUTTON_WHEELUP:
				_list->scrollUp(false, true);
				break;
			case SDL_BUTTON_WHEELDOWN:
				_list->scrollDown(false, true);
				break;

			case SDL_BUTTON_LEFT:
				_timer->start();
		}
	}
}

/**
 * Stops scrolling the associated list.
 * @param action	- pointer to an Action
 * @param state		- State that the ActionHandlers belong to
 */
void ArrowButton::mouseRelease(Action* action, State* state)
{
	ImageButton::mouseRelease(action, state);

	if (_list != nullptr
		&& action->getDetails()->button.button == SDL_BUTTON_LEFT)
	{
		_timer->stop();
	}
}

/**
 * Scrolls the associated list to top or bottom.
 * @param action	- pointer to an Action
 * @param state		- State that the ActionHandlers belong to
 */
void ArrowButton::mouseClick(Action* action, State* state)
{
	ImageButton::mouseClick(action, state);

	if (_list != nullptr
		&& action->getDetails()->button.button == SDL_BUTTON_RIGHT)
	{
		switch (_shape)
		{
			case ARROW_BIG_UP:
				_list->scrollUp(true);
				break;
			case ARROW_BIG_DOWN:
				_list->scrollDown(true);
		}
	}
}

}
