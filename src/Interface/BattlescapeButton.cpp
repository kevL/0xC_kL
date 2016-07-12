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

#include "BattlescapeButton.h"

#include "../Engine/Action.h"


namespace OpenXcom
{

/**
 * Sets up the BattlescapeButton with the specified size and position.
 * @param width		- width in pixels
 * @param height	- height in pixels
 * @param x			- x-position in pixels (default 0)
 * @param y			- y-position in pixels (default 0)
 */
BattlescapeButton::BattlescapeButton(
		int width,
		int height,
		int x,
		int y)
	:
		InteractiveSurface(
			width,
			height,
			x,y),
		_color(0),
		_group(nullptr),
		_inverted(false),
		_toggleMode(INVERT_NONE),
		_altSurface(nullptr)
{}

/**
 * dTor.
 */
BattlescapeButton::~BattlescapeButton() // virtual.
{
	delete _altSurface;
}

/**
 * Changes the color of this BattlescapeButton.
 * @param color - color
 */
void BattlescapeButton::setColor(Uint8 color)
{
	_color = color;
}

/**
 * Gets the color of this BattlescapeButton.
 * @return, color
 */
Uint8 BattlescapeButton::getColor() const
{
	return _color;
}

/**
 * Changes the button-group that this BattlescapeButton belongs to.
 * @param group - pointer to the pressed button pointer in the group,
 *				  nullptr makes it a regular button
 */
void BattlescapeButton::setGroup(BattlescapeButton** group)
{
	_group = group;

	if (_group != nullptr
		&& *_group == this)
	{
		_inverted = true;
	}
}

/**
 * Sets this BattlescapeButton as the pressed button if it's part of a group and
 * inverts the colors when pressed.
 * @param action	- pointer to an Action
 * @param state		- State that the ActionHandlers belong to
 */
void BattlescapeButton::mousePress(Action* action, State* state)
{
	if (_group != nullptr)
	{
		if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
		{
			(*_group)->toggle(false);
			*_group = this;
			_inverted = true;
		}
	}
	else if (_toggleMode == INVERT_CLICK
		&& _inverted == false
		&& isButtonPressed() == true
		&& isButtonHandled(action->getDetails()->button.button) == true)
	{
		_inverted = true;
	}

	InteractiveSurface::mousePress(action, state);
}

/**
 * Sets this BattlescapeButton as the released button if it's part of a group.
 * @param action	- pointer to an Action
 * @param state		- State that the ActionHandlers belong to
 */
void BattlescapeButton::mouseRelease(Action* action, State* state)
{
	if (_inverted == true
		&& isButtonHandled(action->getDetails()->button.button) == true)
	{
		_inverted = false;
	}

	InteractiveSurface::mouseRelease(action, state);
}

/**
 * Inverts this BattlescapeButton explicitly either ON or OFF and keeps track of
 * state using internal variables.
 * @param press - true to set this button as pressed
 */
void BattlescapeButton::toggle(bool press)
{
	if (_toggleMode == INVERT_TOGGLE || _inverted == true)
		_inverted = press;
}

/**
 * Toggle inversion mode: click to press, click to unpress.
 */
void BattlescapeButton::allowToggleInversion()
{
	_toggleMode = INVERT_TOGGLE;
}

/**
 * Click inversion mode: click to press, release to unpress.
 */
void BattlescapeButton::allowClickInversion()
{
	_toggleMode = INVERT_CLICK;
}

/**
 * Initializes the alternate Surface for swapping out as needed.
 * @note Performs a palette-inversion for colored buttons. Uses two separate
 * surfaces because that makes it easy to keep track of whether or not the
 * surface is inverted.
 */
void BattlescapeButton::altSurface()
{
	delete _altSurface;

	_altSurface = new Surface(
							_surface->w,
							_surface->h,
							_x,_y);
	_altSurface->setPalette(getPalette());

	_altSurface->lock();
	for (int
			x = 0, y = 0;
			x < getWidth() && y < getHeight();
			)
	{
		const Uint8 pixel (getPixelColor(x,y));
		if (pixel != 0u)
			_altSurface->setPixelIterative(
										&x,&y,
										static_cast<Uint8>(static_cast<unsigned>(pixel) + 2u * (static_cast<unsigned>(_color) + 3u - static_cast<unsigned>(pixel))));
		else
			_altSurface->setPixelIterative(
										&x,&y,
										0u);
	}
	_altSurface->unlock();
}

/**
 * Blits this surface or the alternate Surface onto another one depending on
 * whether the button is depressed or not.
 * @param srf - pointer to a Surface to blit to
 */
void BattlescapeButton::blit(const Surface* const srf)
{
	if (_inverted == true)
		_altSurface->blit(srf);
	else
		Surface::blit(srf);
}

/**
 * Changes the position of the surface in the x-axis.
 * @param x - x-position in pixels
 */
void BattlescapeButton::setX(int x)
{
	Surface::setX(x);

	if (_altSurface != nullptr)
		_altSurface->setX(x);
}

/**
 * Changes the position of the surface in the y-axis.
 * @param y - y-position in pixels
 */
void BattlescapeButton::setY(int y)
{
	Surface::setY(y);

	if (_altSurface != nullptr)
		_altSurface->setY(y);
}

}
