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
			width, height,
			x,y),
		_list(0),
		_color(0u),
		_pressed(false),
		_contrast(false),
		_offset(0),
		_bg(0),
		_scrollDir(MSCROLL_NONE)
{
	_track	= new Surface(width - 2, height, x,y);
	_btn	= new Surface(width,     height, x,y);

	_rect.x =
	_rect.y = 0;
	_rect.w =
	_rect.h = 0u;

	_timerScrollKey = new Timer(122u);
	_timerScrollKey->onTimer((SurfaceHandler)& ScrollBar::keyScroll);

	_timerScrollMouse = new Timer(122u);
	_timerScrollMouse->onTimer((SurfaceHandler)& ScrollBar::mouseScroll);
}

/**
 * Deletes contents.
 */
ScrollBar::~ScrollBar()
{
	delete _track;
	delete _btn;
	delete _timerScrollKey;
}

/**
 * Changes the position of this Surface in the x-axis.
 * @param x - x-position in pixels
 */
void ScrollBar::setX(int x)
{
	Surface::setX(x);

	_track->setX(x + 1);
	_btn->setX(x);
}

/**
 * Changes the position of this Surface in the y-axis.
 * @param y - y-position in pixels
 */
void ScrollBar::setY(int y)
{
	Surface::setY(y);

	_track->setY(y);
	_btn->setY(y);
}

/**
 * Changes the height of this ScrollBar.
 * @param height - height in pixels
 */
void ScrollBar::setHeight(int height)
{
	Surface::setHeight(height);

	_track->setHeight(height);
	_btn->setHeight(height);

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
 * @param contrast - true for high-contrast (default true)
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
void ScrollBar::setTextList(TextList* const textList)
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
	_btn->setPalette(colors, firstcolor, ncolors);
}

/**
 * Blits this ScrollBar.
 * @param surface - pointer to a Surface to blit to
 */
void ScrollBar::blit(Surface* surface)
{
	Surface::blit(surface);

	if (_visible == true && _hidden == false)
	{
		_track->blit(surface);
		_btn->blit(surface);

		invalidate();
	}
}

/**
 * Scrolls the TextList when the mouse drags the scroll-button.
 * @param action	- pointer to an Action
 * @param state		- State that the ActionHandlers belong to
 */
void ScrollBar::handle(Action* action, State* state)
{
	InteractiveSurface::handle(action, state);

	if (_pressed == true)
	{
		switch (action->getDetails()->type)
		{
			case SDL_MOUSEMOTION:
			{
				const double
					scale	(static_cast<double>(_list->getRows()) / static_cast<double>(_surface->h)),
					track_y (static_cast<double>(static_cast<int>(action->getAbsoluteMouseY()) - getY() + _offset));

				const size_t scroll (static_cast<size_t>(Round(track_y * scale)));
				// TODO: Two quirks could/should be addressed:
				// When dragging the scroll-button for a huge-long list with a
				// short vertical track:
				// 1. the cursor creeps ever-so-slightly vertically relative to
				//	  the scroll-button;
				// 2. the scroll-button warps upward when initially grabbed AND
				//	  this mouse-motion event is registered.
				// Both/either effect(s) are slight and the first is negligible.

				_list->scrollTo(scroll);
			}
		}
	}
}

/**
 * This ScrollBar scrolls a bunch of ways.
 * @param action	- pointer to an Action
 * @param state		- State that the ActionHandlers belong to
 */
void ScrollBar::mousePress(Action* action, State* state)
{
	InteractiveSurface::mousePress(action, state);

	switch (action->getDetails()->button.button)
	{
		case SDL_BUTTON_WHEELUP:
			_list->scrollUp(false, true);
			break;

		case SDL_BUTTON_WHEELDOWN:
			_list->scrollDown(false, true);
			break;

		case SDL_BUTTON_LEFT:
		{
			const int cursor_y (static_cast<int>(action->getAbsoluteMouseY()) - getY()); // ie, how far down the cursor is from the top of the track.
			if (cursor_y < static_cast<int>(_rect.y))
			{
				_scrollDir = MSCROLL_UP;
				_list->scrollTo(_list->getScroll() - _list->getVisibleRows());
				_timerScrollMouse->start();
			}
			else if (cursor_y >= static_cast<int>(_rect.y) + static_cast<int>(_rect.h))
			{
				_scrollDir = MSCROLL_DOWN;
				_list->scrollTo(_list->getScroll() + _list->getVisibleRows());
				_timerScrollMouse->start();
			}
			else
			{
				_pressed = true;
				_offset = static_cast<int>(_rect.y) - cursor_y;
			}
			break;
		}

		case SDL_BUTTON_RIGHT:
		{
			const int cursor_y (static_cast<int>(action->getAbsoluteMouseY()) - getY());
			if (cursor_y < static_cast<int>(_rect.y))
				_list->scrollUp(true);
			else if (cursor_y >= static_cast<int>(_rect.y) + static_cast<int>(_rect.h))
				_list->scrollDown(true);
			else
			{
				_pressed = true;
				_offset = static_cast<int>(_rect.y) - cursor_y;
			}
		}
	}
}

/**
 * This ScrollBar stops scrolling when the LMB is released.
 * @param action	- pointer to an Action
 * @param state		- State that the ActionHandlers belong to
 */
void ScrollBar::mouseRelease(Action* action, State* state)
{
	InteractiveSurface::mouseRelease(action, state);

	switch (action->getDetails()->button.button)
	{
		case SDL_BUTTON_LEFT:
		case SDL_BUTTON_RIGHT:
			switch (_scrollDir)
			{
				default:
				case MSCROLL_NONE:
					_pressed = false;
//					_offset = 0;
					break;

				case MSCROLL_UP:
				case MSCROLL_DOWN:
					_timerScrollMouse->stop();
					_scrollDir = MSCROLL_NONE;
			}
	}
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
				_list->scrollUp();
				_timerScrollKey->start();
				break;

			case SDLK_DOWN:		// down 1 line
			case SDLK_KP2:
				_list->scrollDown();
				_timerScrollKey->start();
				break;

			case SDLK_PAGEUP:	// up 1 page
			case SDLK_KP9:
				_list->scrollTo(_list->getScroll() - _list->getVisibleRows());
				_timerScrollKey->start();
				break;

			case SDLK_PAGEDOWN:	// down 1 page
			case SDLK_KP3:
				_list->scrollTo(_list->getScroll() + _list->getVisibleRows());
				_timerScrollKey->start();
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

	_timerScrollKey->stop();
}

/**
 * Passes ticks to mouse- and keyboard-actions.
 */
void ScrollBar::think()
{
	InteractiveSurface::think();

	_timerScrollMouse->think(nullptr, this);
	_timerScrollKey->think(nullptr, this);
}

/**
 * Scrolls the list with the keyboard.
 */
void ScrollBar::keyScroll()
{
	const Uint8* const keystate (SDL_GetKeyState(nullptr));

	if (keystate[SDLK_UP] == 1 || keystate[SDLK_KP8] == 1)
		_list->scrollUp();												// 1 line up

	else if (keystate[SDLK_DOWN] == 1 || keystate[SDLK_KP2] == 1)
		_list->scrollDown();											// 1 line down

	else if (keystate[SDLK_PAGEUP] == 1 || keystate[SDLK_KP9] == 1)
		_list->scrollTo(_list->getScroll() - _list->getVisibleRows());	// 1 page up

	else if (keystate[SDLK_PAGEDOWN] == 1 || keystate[SDLK_KP3] == 1)
		_list->scrollTo(_list->getScroll() + _list->getVisibleRows());	// 1 page down
}

/**
 * Scrolls the list with the mouse.
 */
void ScrollBar::mouseScroll()
{
	switch (_scrollDir)
	{
		case MSCROLL_UP:
			_list->scrollTo(_list->getScroll() - _list->getVisibleRows());
			break;

		case MSCROLL_DOWN:
			_list->scrollTo(_list->getScroll() + _list->getVisibleRows());
	}
}

/**
 * Updates the scroll-button according to the current list-position.
 */
void ScrollBar::draw()
{
	Surface::draw();

	drawTrack();
	drawButton();
}

/**
 * Draws the background-track semi-transparent.
 */
void ScrollBar::drawTrack() // private.
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
 * Draws the scroll-button as a hollowed rectangle.
 */
void ScrollBar::drawButton() // private.
{
//	_rect.x = 0;
	_rect.w = static_cast<Uint16>(_btn->getWidth());

	const double scale (static_cast<double>(_surface->h) / static_cast<double>(_list->getRows()));
	_rect.y = static_cast<Sint16>(std::floor(static_cast<double>(_list->getScroll()) * scale));
	_rect.h = static_cast<Uint16>(std::ceil(static_cast<double>(_list->getVisibleRows()) * scale));

	static const Uint16 BTN_HEIGHT_MIN (4u);
	if (_rect.h < BTN_HEIGHT_MIN)
	{
		_rect.h = BTN_HEIGHT_MIN;
		const double padFactor (static_cast<double>(BTN_HEIGHT_MIN) / static_cast<double>(_surface->h));
		_rect.y -= static_cast<Sint16>(static_cast<double>(_rect.y) * padFactor);
	}


	_btn->clear();

	_btn->lock();
	SDL_Rect rect (_rect); // draw filled button

	--rect.w;
	--rect.h;
	_btn->drawRect(&rect, _color + 2u);

	++rect.x;
	++rect.y;
	_btn->drawRect(&rect, _color + 5u);

	--rect.w;
	--rect.h;
	_btn->drawRect(&rect, _color + 4u);


	_btn->setPixelColor(
					static_cast<int>(_rect.x),
					static_cast<int>(_rect.y),
					_color + 1u);
	_btn->setPixelColor(
					static_cast<int>(_rect.x),
					static_cast<int>(_rect.y) + static_cast<int>(_rect.h) - 1,
					_color + 4u);
	_btn->setPixelColor(
					static_cast<int>(_rect.x) + static_cast<int>(_rect.w) - 1,
					static_cast<int>(_rect.y),
					_color + 4u);

	if (_rect.h > BTN_HEIGHT_MIN) // hollow it out
	{
		++rect.x;
		++rect.y;
		rect.w -= 3u;
		rect.h -= 3u;
		_btn->drawRect(&rect, _color + 5u);

		++rect.x;
		++rect.y;
		_btn->drawRect(&rect, _color + 2u);

		--rect.w;
		--rect.h;
		_btn->drawRect(&rect, 0u);


		_btn->setPixelColor(
						static_cast<int>(_rect.x) + static_cast<int>(_rect.w) - 3,
						static_cast<int>(_rect.y) + static_cast<int>(_rect.h) - 3,
						_color + 1u);
		_btn->setPixelColor(
						static_cast<int>(_rect.x) + 2,
						static_cast<int>(_rect.y) + static_cast<int>(_rect.h) - 3,
						_color + 4u);
		_btn->setPixelColor(
						static_cast<int>(_rect.x) + static_cast<int>(_rect.w) - 3,
						static_cast<int>(_rect.y) + 2,
						_color + 4u);
	}
	_btn->unlock();
}

}
