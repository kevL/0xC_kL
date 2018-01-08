/*
 * Copyright 2010-2018 OpenXcom Developers.
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

#include "Action.h"


namespace OpenXcom
{

/**
 * Creates an Action as in hardware-event.
 * @param event			- pointer to an SDL_Event
 * @param scaleX		- the Screen's x-scaling factor (default 0.0)
 * @param scaleY		- the Screen's y-scaling factor (default 0.0)
 * @param topBlackBand	- the Screen's top black band height (default 0)
 * @param leftBlackBand	- the Screen's left black band width (default 0)
// * @param rodentState	- state of the rodent
 */
Action::Action(
		SDL_Event* const event,
		double scaleX,
		double scaleY,
		int topBlackBand,
		int leftBlackBand)
//		const Uint32 rodentState)
	:
		_event(event),
		_scaleX(scaleX),
		_scaleY(scaleY),
		_borderTop(topBlackBand),
		_borderLeft(leftBlackBand),
		_mouseX(-1),
		_mouseY(-1),
		_surfaceX(-1),
		_surfaceY(-1),
		_sender(nullptr)
//		_rodentState(rodentState)
{}

/**
 * dTor.
 */
Action::~Action()
{}

/**
 * Gets the x-scaling-factor used by the Screen when this Action was fired.
 * @note Used to correct mouse-input.
 * @return, the current Screen's x-scaling-factor
 */
double Action::getScaleX() const
{
	return _scaleX;
}

/**
 * Gets the y-scaling-factor used by the Screen when this Action was fired.
 * @note Used to correct mouse-input.
 * @return, the current Screen's y-scaling-factor
 */
double Action::getScaleY() const
{
	return _scaleY;
}

/**
 * Sets this Action as a mouse-action with the required x/y-properties.
 * @param mX		- mouse's x-position
 * @param mY		- mouse's y-position
 * @param surfaceX	- Surface's x-position
 * @param surfaceY	- Surface's y-position
 */
void Action::setMouseAction(
		int mX,
		int mY,
		int surfaceX,
		int surfaceY)
{
	_mouseX = mX - _borderLeft;
	_mouseY = mY - _borderTop;

	_surfaceX = surfaceX;
	_surfaceY = surfaceY;
}

/**
 * Checks if this Action is a valid mouse-thing.
 * @return, true if valid
 */
bool Action::isMouseAction() const
{
	return (_mouseX != -1);
}

/**
 * Checks if a specified mouse-button invoked this Action.
 * @param btnId - btn-ID
 * @return, true if so
 */
bool Action::isMouseButton(int btnId) const
{
	return (SDL_GetMouseState(nullptr,nullptr) & SDL_BUTTON(btnId));
//	return _rodentState & SDL_BUTTON(btnId);
}

/**
 * Gets the overall mouse-buttons' state.
 * @return, non-SDL-proprietary mouse-button Id
 *
Uint32 Action::getMouseState() const
{
	return _rodentState;
} */

/**
 * Gets the height in pixels of the top black band if any.
 * @return, Screen's top black band
 */
int Action::getBorderTop() const
{
	return _borderTop;
}

/**
 * Gets the width in pixels of the left black band if any.
 * @return, Screen's left black band
 */
int Action::getBorderLeft() const
{
	return _borderLeft;
}

/**
 * Gets the x-position of the mouse-cursor relative to the game window or -1
 * if this isn't a mouse-related action.
 * @return, mouse's x-position
 */
int Action::getMouseX() const
{
	return _mouseX;
}

/**
 * Gets the y-position of the mouse-cursor relative to the game window or -1
 * if this isn't a mouse-related action.
 * @return, mouse's y-position
 */
int Action::getMouseY() const
{
	return _mouseY;
}

/**
 * Gets the absolute x-position of the mouse-cursor relative to the game
 * window corrected for screen-scaling.
 * @return, mouse's absolute x-position
 */
double Action::getAbsoluteMouseX() const
{
	if (_mouseX == -1)
		return -1.;

	return static_cast<double>(_mouseX) / _scaleX;
}

/**
 * Gets the absolute y-position of the mouse-cursor relative to the game
 * window corrected for screen-scaling.
 * @return, mouse's absolute x-position
 */
double Action::getAbsoluteMouseY() const
{
	if (_mouseY == -1)
		return -1.;

	return static_cast<double>(_mouseY) / _scaleY;
}

/**
 * Gets the x-position of the mouse-cursor relative to the Surface that
 * triggered the action corrected for screen-scaling.
 * @return, mouse's relative x-position
 */
double Action::getRelativeMouseX() const
{
	if (_mouseX == -1)
		return -1.;

	return static_cast<double>(_mouseX) - (static_cast<double>(_surfaceX) * _scaleX);
}

/**
 * Gets the y-position of the mouse-cursor relative to the Surface that
 * triggered the action corrected for screen-scaling.
 * @return, mouse's relative y-position
 */
double Action::getRelativeMouseY() const
{
	if (_mouseY == -1)
		return -1.;

	return static_cast<double>(_mouseY) - (static_cast<double>(_surfaceY) * _scaleY);
}

/**
 * Sets the InteractiveSurface that triggered this Action - ie, the sender.
 * @param sender - pointer to InteractiveSurface
 */
void Action::setSender(InteractiveSurface* const sender)
{
	_sender = sender;
}

/**
 * Gets the InteractiveSurface that triggered this Action - ie, the sender.
 * @return, pointer to InteractiveSurface
 */
InteractiveSurface* Action::getSender() const
{
	return _sender;
}

/**
 * Gets details about this Action.
 * @return, pointer to SDL_event
 */
SDL_Event* Action::getDetails() const
{
	return _event;
}

}
