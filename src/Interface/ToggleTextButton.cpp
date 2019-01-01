/*
 * Copyright 2010-2019 OpenXcom Developers.
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

#include "ToggleTextButton.h"

#include <limits>

#include "../Engine/Action.h"


namespace OpenXcom
{

/**
 * Constructs a ToggleTextButton.
 * @note This class does not trigger a TextButton::soundPress as it is currently
 * structured.
 * @param width		- the width in pixels
 * @param height	- the height in pixels
 * @param x			- the x-position in pixels
 * @param y			- the y-position in pixels
 */
ToggleTextButton::ToggleTextButton(
		int width,
		int height,
		int x,
		int y)
	:
		TextButton(
				width,
				height,
				x,y),
		_color(std::numeric_limits<uint8_t>::max()),
		_colorInvert(std::numeric_limits<uint8_t>::max()),
		_isPressed(false),
		_fakeGroup(nullptr)
{
	setGroup(&_fakeGroup);
}

/**
 * dTor.
 */
ToggleTextButton::~ToggleTextButton(void)
{}

/**
 * Handles mouse-presses by toggling the button state.
 * @note Use '_fakeGroup' to trick TextButton into drawing the right thing.
 * @param action	- pointer to an Action
 * @param state		- pointer to a State
 */
void ToggleTextButton::mousePress(Action* action, State* state)
{
	switch (action->getDetails()->button.button)
	{
		case SDL_BUTTON_LEFT:
		case SDL_BUTTON_RIGHT:
			_isPressed = !_isPressed;
			_fakeGroup = (_isPressed == true) ? this : nullptr; // this is the trick that makes TextButton stick

			if (_isPressed == true
				&& _colorInvert != std::numeric_limits<uint8_t>::max())
			{
				TextButton::setColor(_colorInvert);
			}
			else
				TextButton::setColor(_color);
	}
	InteractiveSurface::mousePress(action, state); // skip TextButton's code as it will try to set *_group
	draw();
}

/**
 * Sets the '_isPressed' state of the button and forces it to redraw.
 * @param press - true if pressed
 */
void ToggleTextButton::setPressed(bool press)
{
	_isPressed = press;
	_fakeGroup = (_isPressed == true) ? this : nullptr;

	if (_isPressed == true
		&& _colorInvert != std::numeric_limits<uint8_t>::max())
	{
		TextButton::setColor(_colorInvert);
	}
	else
		TextButton::setColor(_color);

	_redraw = true;
}

/**
 * Sets the color of this button.
 * @param color - color
 */
void ToggleTextButton::setColor(Uint8 color)
{
	TextButton::setColor(_color = color);
}

/**
 * Surface::invert() is called when this value is set and the button is depressed.
 * @param color - the invert color
 */
void ToggleTextButton::setColorInvert(Uint8 color)
{
	_colorInvert = color;
	_fakeGroup = nullptr;

	_redraw = true;
}

/**
 * Handles draw() in case the button needs to be painted with a garish color.
 */
void ToggleTextButton::draw()
{
	if (_colorInvert != std::numeric_limits<uint8_t>::max())
		_fakeGroup = nullptr; // nevermind, TextButton. We'll invert the surface ourselves.

	TextButton::draw();

	if (_isPressed == true
		&& _colorInvert != std::numeric_limits<uint8_t>::max())
	{
		this->invert(static_cast<Uint8>(_colorInvert + 4));
	}
}

}
