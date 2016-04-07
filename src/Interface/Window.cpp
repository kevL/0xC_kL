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

#include "Window.h"

//#include <SDL.h>
//#include <SDL_mixer.h>
//#include "../fmath.h"

#include "../Engine/RNG.h"
#include "../Engine/Sound.h"
#include "../Engine/Timer.h"


namespace OpenXcom
{

// Speed: for high-quality filters & shaders, like 4xHQX, use a faster value
// like 0.135f - for quicker filters & shaders slow the popup down with a value
// like 0.076f.
const float Window::POPUP_SPEED = 0.135f; // larger is faster step.

Sound* Window::soundPopup[3] = {0,0,0}; // static.


/**
 * Sets up a blank window with the specified size and position.
 * @param state		- pointer to State the window belongs to
 * @param width		- width in pixels
 * @param height	- height in pixels
 * @param x			- x-position in pixels (default 0)
 * @param y			- y-position in pixels (default 0)
 * @param popType	- popup animation type (default POPUP_NONE)
 * @param toggle	- true to toggle screen before & after popup (default true)
 */
Window::Window(
		State* state,
		int width,
		int height,
		int x,
		int y,
		PopupType popType,
		bool toggle)
	:
		Surface(
			width,
			height,
			x,y),
		_state(state),
		_popType(popType),
		_toggle(toggle),
		_bg(nullptr),
		_dX(-x),
		_dY(-y),
		_color(0),
		_popStep(0.f),
		_contrast(false),
		_fullScreen(false),
		_thinBorder(false),
		_bgX(0),
		_bgY(0),
		_colorFill(0),
		_popProgress(POP_START)
{
	_timer = new Timer(15u);
	_timer->onTimer((SurfaceHandler)& Window::popup);

	switch (_popType)
	{
		case POPUP_NONE:
			_popProgress = POP_HALT;
			break;

		default:
			_hidden = true;
			_timer->start();

			if (_state != nullptr)
			{
				_fullScreen = _state->isFullScreen();
				if (_fullScreen == true && _toggle == true) // <- for opening UfoPaedia in battlescape w/ black BG.
					_state->toggleScreen();
			}
	}
}

/**
 * Deletes timer.
 */
Window::~Window()
{
	delete _timer;
}

/**
 * Keeps the animation timers running.
 */
void Window::think()
{
	if (_hidden == true && _popProgress != POP_HALT)
	{
		_state->hideAll();
		_hidden = false;
	}
	_timer->think(nullptr, this);
}

/**
 * Plays the window popup animation.
 */
void Window::popup() // private.
{
	if (_popProgress == POP_START)
	{
		_popProgress = POP_GO;
		soundPopup[static_cast<size_t>(RNG::seedless(1,2))]->play(Mix_GroupAvailable(0));
	}

	if (_popProgress == POP_GO
		&& (_popStep += POPUP_SPEED) > 1.f)
	{
		_popProgress = POP_HALT;
	}

	if (_popProgress == POP_HALT)
	{
		if (_fullScreen == true && _toggle == true)
			_state->toggleScreen();

		_state->showAll();
		_timer->stop();
	}

	_redraw = true;
}

/**
 * Gets if this window has finished popping up.
 * @return, true if popup is finished
 */
bool Window::isPopupDone() const
{
	return (_popProgress == POP_HALT);
}

/**
 * Draws the bordered window with a graphic background.
 * @note The background never moves with the window; it's always aligned to the
 * top-left corner of the screen and cropped to fit inside the area.
 */
void Window::draw()
{
	Surface::draw();

	SDL_Rect rect;
	switch (_popType)
	{
		default:
		case POPUP_NONE:
			rect.x = 0;
			rect.w = static_cast<Uint16>(getWidth());

			rect.y = 0;
			rect.h = static_cast<Uint16>(getHeight());
			break;

		case POPUP_BOTH:
			rect.x = static_cast<Sint16>(
					(static_cast<float>(getWidth()) - (static_cast<float>(getWidth()) * _popStep))) / 2;
			rect.w = static_cast<Uint16>(
					 static_cast<float>(getWidth()) * _popStep);

			rect.y = static_cast<Sint16>(
					(static_cast<float>(getHeight()) - (static_cast<float>(getHeight()) * _popStep))) / 2;
			rect.h = static_cast<Uint16>(
					 static_cast<float>(getHeight()) * _popStep);
			break;

		case POPUP_HORIZONTAL:
			rect.x = static_cast<Sint16>(
					(static_cast<float>(getWidth()) - (static_cast<float>(getWidth()) * _popStep))) / 2;
			rect.w = static_cast<Uint16>(
					 static_cast<float>(getWidth()) * _popStep);

			rect.y = 0;
			rect.h = static_cast<Uint16>(getHeight());
			break;

		case POPUP_VERTICAL:
			rect.y = static_cast<Sint16>(
					(static_cast<float>(getHeight()) - (static_cast<float>(getHeight()) * _popStep))) / 2;
			rect.h = static_cast<Uint16>(
					 static_cast<float>(getHeight()) * _popStep);

			rect.x = 0;
			rect.w = static_cast<Uint16>(getWidth());
	}

	Uint8
		color,
		gradient;

	if (_contrast == true)	gradient = 2;
	else					gradient = 1;

	color = _color + (gradient * 3);

	if (_thinBorder == true)
	{
		color = _color + gradient;
		for (int
				i = 0;
				i != 5;
				++i)
		{
			if (rect.w > 0 && rect.h > 0)
				drawRect(&rect, color);

			if (i % 2 == 0)
			{
				++rect.x;
				++rect.y;
			}
			--rect.w;
			--rect.h;

			switch (i)
			{
				case 0:
					color = _color + (gradient * 5);
					setPixelColor(rect.w, 0, color);
					break;
				case 1:
					color = _color + (gradient * 2);
					break;
				case 2:
					color = _color + (gradient * 4);
					setPixelColor(rect.w + 1, 1, color);
					break;
				case 3:
					color = _color + (gradient * 3);
			}
		}
	}
	else
	{
		for (int
				i = 0;
				i != 5;
				++i)
		{
			if (rect.w > 0 && rect.h > 0)
				drawRect(&rect, color);

			if (i < 2)	color -= gradient;
			else		color += gradient;

			++rect.x;
			++rect.y;

			if (rect.w > 1)	rect.w -= 2;
			else			rect.w = 0;

			if (rect.h > 1)	rect.h -= 2;
			else			rect.h = 0;
		}
	}

	if (rect.w != 0 && rect.h != 0)
	{
		if (_bg != nullptr)
		{
			_bg->getCrop()->x = static_cast<Sint16>(static_cast<int>(rect.x) - _dX - _bgX);
			_bg->getCrop()->y = static_cast<Sint16>(static_cast<int>(rect.y) - _dY - _bgY);
			_bg->getCrop()->w = rect.w;
			_bg->getCrop()->h = rect.h;

			_bg->setX(static_cast<int>(rect.x));
			_bg->setY(static_cast<int>(rect.y));

			_bg->blit(this);
		}
		else
			drawRect(&rect, _colorFill);
	}
}

/**
 * Changes the color used to draw the shaded border.
 * @param color - color value
 */
void Window::setColor(Uint8 color)
{
	_color = color;
	_redraw = true;
}

/**
 * Returns the color used to draw the shaded border.
 * @return, color value
 */
Uint8 Window::getColor() const
{
	return _color;
}

/**
 * Enables/disables high contrast color.
 * @note Mostly used for Battlescape UI.
 * @param contrast - high contrast setting (default true)
 */
void Window::setHighContrast(bool contrast)
{
	_contrast = contrast;
	_redraw = true;
}

/**
 * Sets the surface used to draw the background of the window.
 * @param bg - background
 * @param dX - x offset (default 0)
 * @param dY - y offset (default 0)
 */
void Window::setBackground(
		Surface* const bg,
		int dX,
		int dY)
{
	_bg = bg;
	_bgX = dX;
	_bgY = dY;
	_redraw = true;
}

/**
 * Sets the background to a solid color instead of transparent.
 * @note A background picture will override a background fill.
 * @param color - fill color (0 is transparent)
 */
void Window::setBackgroundFill(Uint8 color)
{
	_colorFill = color;
}

/**
 * Changes the horizontal offset of the surface in the x-axis.
 * @param dX - x-position in pixels
 */
void Window::setDX(int dX)
{
	_dX = dX;
}

/**
 * Changes the vertical offset of the surface in the y-axis.
 * @param dY - y-position in pixels
 */
void Window::setDY(int dY)
{
	_dY = dY;
}

/**
 * Changes the window to have a thin border.
 */
void Window::setThinBorder()
{
	_thinBorder = true;
}

}
