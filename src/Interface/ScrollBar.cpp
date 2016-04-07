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

#include "ScrollBar.h"

//#include <algorithm>

#include "../fmath.h"

#include "../Engine/Action.h"
#include "../Engine/Palette.h"
#include "../Engine/Timer.h"

#include "../Interface/TextList.h"


namespace OpenXcom
{

/**
 * Sets up the ScrollBar with a specified size and position.
 * @param width		- width in pixels
 * @param height	- height in pixels
 * @param x			- x-position in pixels (default 0)
 * @param y			- y-position in pixels (default 0)
 */
ScrollBar::ScrollBar(
		int width,
		int height,
		int x,
		int y)
	:
		InteractiveSurface(
			width,
			height,
			x,
			y),
		_list(0),
		_color(0),
		_pressed(false),
		_contrast(false),
		_offset(0),
		_bg(0)
{
	_track = new Surface(width - 2, height, x,y);
	_thumb = new Surface(width, height, x,y);

	_thumbRect.x =
	_thumbRect.y = 0;
	_thumbRect.w =
	_thumbRect.h = 0;

	_timer = new Timer(122u);
	_timer->onTimer((SurfaceHandler)& ScrollBar::scroll);
}

/**
 * Deletes contents.
 */
ScrollBar::~ScrollBar()
{
	delete _track;
	delete _thumb;
	delete _timer;
}

/**
 * Changes the position of this Surface in the x-axis.
 * @param x - x-position in pixels
 */
void ScrollBar::setX(int x)
{
	Surface::setX(x);

	_track->setX(x + 1);
	_thumb->setX(x);
}

/**
 * Changes the position of this Surface in the y-axis.
 * @param y - y-position in pixels
 */
void ScrollBar::setY(int y)
{
	Surface::setY(y);

	_track->setY(y);
	_thumb->setY(y);
}

/**
 * Changes the height of this ScrollBar.
 * @param height - height in pixels
 */
void ScrollBar::setHeight(int height)
{
	Surface::setHeight(height);

	_track->setHeight(height);
	_thumb->setHeight(height);

	_redraw = true;
}

/**
 * Changes the color used to render this ScrollBar.
 * @param color - color-value
 */
void ScrollBar::setColor(Uint8 color)
{
	_color = color;
}

/**
 * Returns the color used to render this ScrollBar.
 * @return, color-value
 */
Uint8 ScrollBar::getColor() const
{
	return _color;
}

/**
 * Enables/disables high-contrast-color.
 * @note Mostly used for Battlescape text.
 * @param contrast - true for high-contrast
 */
void ScrollBar::setHighContrast(bool contrast)
{
	_contrast = contrast;
}

/**
 * Changes the TextList associated with this ScrollBar.
 * @note This makes the button scroll that list.
 * @param textList - pointer to TextList
 */
void ScrollBar::setTextList(TextList* textList)
{
	_list = textList;
}

/**
 * Changes the Surface used to draw the background of the track.
 * @param bg - pointer to a Surface
 */
void ScrollBar::setBackground(Surface* const bg)
{
	_bg = bg;
}

/**
 * Replaces a certain amount of colors in this ScrollBar's Palette.
 * @param colors		- pointer to the set of colors
 * @param firstcolor	- offset of the first color to replace
 * @param ncolors		- amount of colors to replace
 */
void ScrollBar::setPalette(
		SDL_Color* const colors,
		int firstcolor,
		int ncolors)
{
	Surface::setPalette(colors, firstcolor, ncolors);

	_track->setPalette(colors, firstcolor, ncolors);
	_thumb->setPalette(colors, firstcolor, ncolors);
}

/**
 * Automatically updates this ScrollBar when the mouse moves.
 * @param action	- pointer to an Action
 * @param state		- State that the ActionHandlers belong to
 */
void ScrollBar::handle(Action* action, State* state)
{
	InteractiveSurface::handle(action, state); // kL_note: screw it. Okay, try it again ... nah Screw it.

	if (_pressed == true)
	{
		switch (action->getDetails()->type)
		{
			case SDL_MOUSEMOTION:
			case SDL_MOUSEBUTTONDOWN:
			{
				const int y (std::min(getHeight() - static_cast<int>(_thumbRect.h) + 1,
									  std::max(0,
											   static_cast<int>(action->getAbsoluteYMouse()) - getY() + _offset)));

				const double scale (static_cast<double>(_list->getRows()) / static_cast<double>(getHeight()));
				const size_t scroll (static_cast<size_t>(Round(static_cast<double>(y) * scale)));

				_list->scrollTo(scroll);
			}
		}
	}
}

/**
 * Blits this ScrollBar.
 * @param surface - pointer to a Surface to blit onto
 */
void ScrollBar::blit(Surface* surface)
{
	Surface::blit(surface);

	if (_visible == true && _hidden == false)
	{
		_track->blit(surface);
		_thumb->blit(surface);

		invalidate();
	}
}

/**
 * This ScrollBar only moves while the button is pressed.
 * @param action	- pointer to an Action
 * @param state		- State that the ActionHandlers belong to
 */
void ScrollBar::mousePress(Action* action, State* state)
{
	InteractiveSurface::mousePress(action, state);

	switch (action->getDetails()->button.button)
	{
		case SDL_BUTTON_LEFT:
		{
			_pressed = true;

			const int cursorY (static_cast<int>(action->getAbsoluteYMouse()) - getY());
			if (cursorY >= static_cast<int>(_thumbRect.y)
				&& cursorY < static_cast<int>(_thumbRect.y) + static_cast<int>(_thumbRect.h))
			{
				_offset = static_cast<int>(_thumbRect.y) - cursorY;
			}
			else
				_offset = -(static_cast<int>(_thumbRect.h) / 2);
			break;
		}

		case SDL_BUTTON_WHEELUP:
			_list->scrollUp(false, true);
			break;

		case SDL_BUTTON_WHEELDOWN:
			_list->scrollDown(false, true);
	}
}

/**
 * This ScrollBar stops moving when the button is released.
 * @param action	- pointer to an Action
 * @param state		- State that the ActionHandlers belong to
 */
void ScrollBar::mouseRelease(Action* action, State* state)
{
	InteractiveSurface::mouseRelease(action, state);

	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
	{
		_pressed = false;
		_offset = 0;
	}
}

/**
 * Passes ticks to keyboard-actions.
 */
void ScrollBar::think()
{
	InteractiveSurface::think();
	_timer->think(nullptr, this);
}

/**
 * Scrolls the list.
 */
void ScrollBar::scroll()
{
	const Uint8* const keystate (SDL_GetKeyState(nullptr));
	if (keystate[SDLK_UP] == 1 || keystate[SDLK_KP8] == 1)
		_list->scrollUp();												// up 1 line
	else if (keystate[SDLK_DOWN] == 1 || keystate[SDLK_KP2] == 1)
		_list->scrollDown();											// down 1 line
	else if (keystate[SDLK_PAGEUP] == 1 || keystate[SDLK_KP9] == 1)
		_list->scrollTo(_list->getScroll() - _list->getVisibleRows());	// up 1 page
	else if (keystate[SDLK_PAGEDOWN] == 1 || keystate[SDLK_KP3] == 1)
		_list->scrollTo(_list->getScroll() + _list->getVisibleRows());	// down 1 page
}

/**
 * Handles keyboard-presses.
 * @param action	- pointer to an Action
 * @param state		- State that the ActionHandlers belong to
 */
void ScrollBar::keyboardPress(Action* action, State* state)
{
	if (action->getDetails()->type == SDL_KEYDOWN)
	{
		InteractiveSurface::keyboardPress(action, state);

		switch (action->getDetails()->key.keysym.sym)
		{
			case SDLK_HOME:		// to top
			case SDLK_KP7:
				_list->scrollUp(true);
				break;

			case SDLK_END:		// to bottom
			case SDLK_KP1:
				_list->scrollDown(true);
				break;

			case SDLK_UP:		// up 1 line
			case SDLK_KP8:
				_timer->start();
				_list->scrollUp();
				break;

			case SDLK_DOWN:		// down 1 line
			case SDLK_KP2:
				_timer->start();
				_list->scrollDown();
				break;

			case SDLK_PAGEUP:	// up 1 page
			case SDLK_KP9:
				_timer->start();
				_list->scrollTo(_list->getScroll() - _list->getVisibleRows());
				break;

			case SDLK_PAGEDOWN:	// down 1 page
			case SDLK_KP3:
				_timer->start();
				_list->scrollTo(_list->getScroll() + _list->getVisibleRows());
		}
	}
}

/**
 * Handles keyboard-releases.
 * @param action	- pointer to an Action
 * @param state		- State that the ActionHandlers belong to
 */
void ScrollBar::keyboardRelease(Action* action, State* state)
{
	InteractiveSurface::keyboardRelease(action, state);
	_timer->stop();
}

/**
 * Updates the thumb according to the current list-position.
 */
void ScrollBar::draw()
{
	Surface::draw();

	drawTrack();
	drawThumb();
}

/**
 * Draws the track (background-bar) semi-transparent.
 */
void ScrollBar::drawTrack()
{
	if (_bg != nullptr)
	{
		_track->copy(_bg);

		if (_list->getComboBox() != nullptr)
			_track->offset(1, Palette::PAL_bgID);
		else
			_track->offsetBlock(-5);
	}
}

/**
 * Draws the thumb (button) as a hollow square.
 */
void ScrollBar::drawThumb()
{
	const double scale (static_cast<double>(getHeight()) / static_cast<double>(_list->getRows()));

	_thumbRect.x = 0;
	_thumbRect.y = static_cast<Sint16>(std::floor(static_cast<double>(_list->getScroll()) * scale));
	_thumbRect.w = static_cast<Uint16>(_thumb->getWidth());
	_thumbRect.h = static_cast<Uint16>(std::ceil(static_cast<double>(_list->getVisibleRows()) * scale));

	_thumb->clear();

	_thumb->lock();
	SDL_Rect rect (_thumbRect); // draw filled button
	Uint8 color (_color + 2);

	--rect.w;
	--rect.h;
	_thumb->drawRect(&rect, color);

	++rect.x;
	++rect.y;
	color = _color + 5;
	_thumb->drawRect(&rect, color);

	--rect.w;
	--rect.h;
	color = _color + 4;
	_thumb->drawRect(&rect, color);


	_thumb->setPixelColor(
					static_cast<int>(_thumbRect.x),
					static_cast<int>(_thumbRect.y),
					_color + 1);
	_thumb->setPixelColor(
					static_cast<int>(_thumbRect.x),
					static_cast<int>(_thumbRect.y) + static_cast<int>(_thumbRect.h) - 1,
					_color + 4);
	_thumb->setPixelColor(
					static_cast<int>(_thumbRect.x) + static_cast<int>(_thumbRect.w) - 1,
					static_cast<int>(_thumbRect.y),
					_color + 4);

	if (static_cast<int>(rect.h) - 4 > 0)
	{
		color = _color + 5; // hollow it out

		++rect.x;
		++rect.y;
		rect.w -= 3;
		rect.h -= 3;
		_thumb->drawRect(&rect, color);

		++rect.x;
		++rect.y;
		color = _color + 2;
		_thumb->drawRect(&rect, color);

		--rect.w;
		--rect.h;
		color = 0;
		_thumb->drawRect(&rect, color);


		_thumb->setPixelColor(
						static_cast<int>(_thumbRect.x) + 2 + static_cast<int>(_thumbRect.w) - 1 - 4,
						static_cast<int>(_thumbRect.y) + 2 + static_cast<int>(_thumbRect.h) - 1 - 4,
						_color + 1);
		_thumb->setPixelColor(
						static_cast<int>(_thumbRect.x) + 2,
						static_cast<int>(_thumbRect.y) + 2 + static_cast<int>(_thumbRect.h) - 1 - 4,
						_color + 4);
		_thumb->setPixelColor(
						static_cast<int>(_thumbRect.x) + 2 + static_cast<int>(_thumbRect.w) - 1 - 4,
						static_cast<int>(_thumbRect.y) + 2,
						_color + 4);
	}
	_thumb->unlock();
}

}
