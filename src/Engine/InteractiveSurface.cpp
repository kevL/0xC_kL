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

#include "InteractiveSurface.h"

#include "Action.h"
#include "Game.h"


namespace OpenXcom
{

const SDLKey InteractiveSurface::SDLK_ANY = static_cast<SDLKey>(-1); // using an unused keycode to represent "any key" = g++ WARNING.


/**
 * Sets up the InteractiveSurface with the specified size and position.
 * @param width		- width in pixels (default 320)
 * @param height	- height in pixels (default 200)
 * @param x			- x-position in pixels (default 0)
 * @param y			- y-position in pixels (default 0)
 */
InteractiveSurface::InteractiveSurface(
		int width,
		int height,
		int x,
		int y)
	:
		Surface(
				width,
				height,
				x,y),
		_rodentState(0u),
		_in(nullptr),
		_over(nullptr),
		_out(nullptr),
		_isHovered(false),
		_isFocused(true),
		_isListButton(false)
{}

/**
 * dTor.
 */
InteractiveSurface::~InteractiveSurface() // virtual.
{}

/**
 * Checks if a button has been pressed.
 * @param btnId - an SDL-button identifier (default 0)
 */
bool InteractiveSurface::isButtonPressed(Uint8 btnId) const
{
	switch (btnId)
	{
		case 0u:
			return (_rodentState != 0u);

		default:
			return (_rodentState & SDL_BUTTON(btnId)) != 0u;
	}
}

/**
 * Checks if a button has been handled.
 * @param btnId - an SDL-button identifier (default 0)
 */
bool InteractiveSurface::isButtonHandled(Uint8 btnId) // virtual.
{
	bool handled = (_click.find(0u) != _click.end()
				|| _press.find(0u) != _press.end()
				|| _release.find(0u) != _release.end());

	if (handled == false && btnId != 0u)
		handled = (_click.find(btnId) != _click.end()
				|| _press.find(btnId) != _press.end()
				|| _release.find(btnId) != _release.end());

	return handled;
}

/**
 * Sets a button as pressed.
 * @param btnId		- an SDL-button identifier
 * @param pressed	- true if pressed
 */
void InteractiveSurface::setButtonPressed(
		Uint8 btnId,
		bool pressed)
{
	if (pressed == true)
		_rodentState = _rodentState |   SDL_BUTTON(btnId);
	else
		_rodentState = _rodentState & (!SDL_BUTTON(btnId));
}

/**
 * Changes the visibility of this Surface.
 * @note A non-visible surface isn't blitted nor does it receive events.
 * @param visible - true if visible (default true)
 */
void InteractiveSurface::setVisible(bool visible)
{
	if ((_visible = visible) == false)
		unpress(nullptr);
}

/**
 * Called whenever an action occurs and processes it to check if it's relevant
 * to this Surface and converts it into a meaningful interaction like a "click"
 * calling the respective handlers.
 * @param action	- pointer to an Action
 * @param state		- State that the ActionHandlers belong to
 */
void InteractiveSurface::handle( // virtual.
		Action* action,
		State* state)
{
	if (_visible == true && _hidden == false)
	{
		action->setSender(this);

		switch (action->getDetails()->type)
		{
			case SDL_MOUSEBUTTONUP:
			case SDL_MOUSEBUTTONDOWN:
				action->setMouseAction(
									action->getDetails()->button.x,
									action->getDetails()->button.y,
									getX(),
									getY());
				break;

			case SDL_MOUSEMOTION:
				action->setMouseAction(
									action->getDetails()->motion.x,
									action->getDetails()->motion.y,
									getX(),
									getY());
		}

		if (action->isMouseAction() == true)
		{
			if (   action->getAbsoluteMouseX() >= getX()
				&& action->getAbsoluteMouseX() <  getX() + getWidth()
				&& action->getAbsoluteMouseY() >= getY()
				&& action->getAbsoluteMouseY() <  getY() + getHeight())
			{
				if (_isHovered == false)
				{
					_isHovered = true;
					mouseIn(action, state);
				}

				if (_isListButton == true
					&& action->getDetails()->type == SDL_MOUSEMOTION)
				{
					_rodentState = action->getMouseState();
					for (Uint8
							i = 1u;
							i <= MOUSEBUTTONS;
							++i)
					{
						if (isButtonPressed(i) == true)
						{
							action->getDetails()->button.button = i;
							mousePress(action, state);
						}
					}
				}

				mouseOver(action, state);
			}
			else
			{
				if (_isHovered == true)
				{
					_isHovered = false;
					mouseOut(action, state);

					if (_isListButton == true
						&& action->getDetails()->type == SDL_MOUSEMOTION)
					{
						for (Uint8
								i = 1u;
								i <= MOUSEBUTTONS;
								++i)
						{
							if (isButtonPressed(i) == true)
								setButtonPressed(i, false);

							action->getDetails()->button.button = i;
							mouseRelease(action, state);
						}
					}
				}
			}
		}

		switch (action->getDetails()->type)
		{
			case SDL_MOUSEBUTTONDOWN:
				if (_isHovered == true
					&& isButtonPressed(action->getDetails()->button.button) == false)
				{
					setButtonPressed(action->getDetails()->button.button, true);
					mousePress(action, state);
				}
				break;

			case SDL_MOUSEBUTTONUP:
				if (isButtonPressed(action->getDetails()->button.button) == true)
				{
					setButtonPressed(action->getDetails()->button.button, false);
					mouseRelease(action, state);
					if (_isHovered == true)
						mouseClick(action, state);
				}
		}

		if (_isFocused == true)
		{
			switch (action->getDetails()->type)
			{
				case SDL_KEYDOWN:
					keyboardPress(action, state);
					break;

				case SDL_KEYUP:
					keyboardRelease(action, state);
			}
		}
	}
}

/**
 * Changes this Surface's focus.
 * @note Surfaces will receive keyboard-events only if focused.
 * @param focus - true if focused (default true)
 */
void InteractiveSurface::setFocus(bool focus)
{
	_isFocused = focus;
}

/**
 * Returns this Surface's focus.
 * @note Surfaces will receive keyboard-events only if focused.
 * @return, true if focused
 */
bool InteractiveSurface::isFocused() const
{
	return _isFocused;
}

/**
 * Simulates a mouse-button-release.
 * @note Used in circumstances where this Surface is unpressed without user input.
 * @param state - pointer to running State
 */
void InteractiveSurface::unpress(State* const state) // virtual.
{
	if (isButtonPressed() == true)
	{
		_rodentState = 0u;

		Action* a (State::getGamePtr()->getSynthMouseUp());
		mouseRelease(a, state);
		delete a;
	}
}

/**
 * Called every time there's a mouse-press over this Surface.
 * @note Allows this Surface to have custom functionality for this action and
 * can be called externally to simulate the action.
 * @param action	- pointer to an Action
 * @param state		- State that the ActionHandlers belong to
 */
void InteractiveSurface::mousePress(Action* action, State* state) // virtual.
{
	std::map<Uint8, ActionHandler>::const_iterator allHandler (_press.find(0u));
	if (allHandler != _press.end())
	{
		ActionHandler handler (allHandler->second);
		(state->*handler)(action);
	}

	std::map<Uint8, ActionHandler>::const_iterator oneHandler (_press.find(action->getDetails()->button.button));
	if (oneHandler != _press.end())
	{
		ActionHandler handler (oneHandler->second);
		(state->*handler)(action);
	}
}

/**
 * Called every time there's a mouse-release over this Surface.
 * @note Allows this Surface to have custom functionality for this action and
 * can be called externally to simulate the action.
 * @param action	- pointer to an Action
 * @param state		- State that the ActionHandlers belong to
 */
void InteractiveSurface::mouseRelease(Action* action, State* state) // virtual.
{
	std::map<Uint8, ActionHandler>::const_iterator allHandler (_release.find(0u));
	if (allHandler != _release.end())
	{
		ActionHandler handler (allHandler->second);
		(state->*handler)(action);
	}

	std::map<Uint8, ActionHandler>::const_iterator oneHandler (_release.find(action->getDetails()->button.button));
	if (oneHandler != _release.end())
	{
		ActionHandler handler (oneHandler->second);
		(state->*handler)(action);
	}
}

/**
 * Called every time there's a mouse-click on this Surface.
 * @note Allows this Surface to have custom functionality for this action and
 * can be called externally to simulate the action.
 * @param action	- pointer to an Action
 * @param state		- State that the ActionHandlers belong to
 */
void InteractiveSurface::mouseClick(Action* action, State* state) // virtual.
{
	std::map<Uint8, ActionHandler>::const_iterator allHandler (_click.find(0u));
	if (allHandler != _click.end())
	{
		ActionHandler handler (allHandler->second);
		(state->*handler)(action);
	}

	std::map<Uint8, ActionHandler>::const_iterator oneHandler (_click.find(action->getDetails()->button.button));
	if (oneHandler != _click.end())
	{
		ActionHandler handler (oneHandler->second);
		(state->*handler)(action);
	}
}

/**
 * Called every time the mouse moves into this Surface.
 * @note Allows this Surface to have custom functionality for this action and
 * can be called externally to simulate the action.
 * @param action	- pointer to an Action
 * @param state		- State that the ActionHandlers belong to
 */
void InteractiveSurface::mouseIn(Action* action, State* state) // virtual.
{
	if (_in != nullptr)
		(state->*_in)(action);
}

/**
 * Called every time the mouse moves over this Surface.
 * @note Allows this Surface to have custom functionality for this action and
 * can be called externally to simulate the action.
 * @param action	- pointer to an Action
 * @param state		- State that the ActionHandlers belong to
 */
void InteractiveSurface::mouseOver(Action* action, State* state) // virtual.
{
	if (_over != nullptr)
		(state->*_over)(action);
}

/**
 * Called every time the mouse moves out of this Surface.
 * @note Allows this Surface to have custom functionality for this action and
 * can be called externally to simulate the action.
 * @param action	- pointer to an Action
 * @param state		- State that the ActionHandlers belong to
 */
void InteractiveSurface::mouseOut(Action* action, State* state) // virtual.
{
	if (_out != nullptr)
		(state->*_out)(action);
}

/**
 * Called every time there's a keyboard-press when this Surface is focused.
 * @note Allows this Surface to have custom functionality for this action and
 * can be called externally to simulate the action.
 * @param action	- pointer to an Action
 * @param state		- State that the ActionHandlers belong to
 */
void InteractiveSurface::keyboardPress(Action* action, State* state) // virtual.
{
	std::map<SDLKey, ActionHandler>::const_iterator allHandler (_keyPress.find(SDLK_ANY));
	if (allHandler != _keyPress.end())
	{
		ActionHandler handler (allHandler->second);
		(state->*handler)(action);
	}

	// Check if Ctrl, Alt or Shift are pressed and if TRUE then disallow.
	// This creates problems but ensures that CTRL+key etc. are reserved for
	// usage elsehow. I guess, haven't looked into it.
	std::map<SDLKey, ActionHandler>::const_iterator oneHandler (_keyPress.find(action->getDetails()->key.keysym.sym));
	if (oneHandler != _keyPress.end()
		&& (action->getDetails()->key.keysym.mod & (KMOD_CTRL | KMOD_ALT | KMOD_SHIFT)) == 0)
	{
		ActionHandler handler (oneHandler->second);
		(state->*handler)(action);
	}
}

/**
 * Called every time there's a keyboard-release over this Surface.
 * @note Allows this Surface to have custom functionality for this action and
 * can be called externally to simulate the action.
 * @param action	- pointer to an Action
 * @param state		- State that the ActionHandlers belong to
 */
void InteractiveSurface::keyboardRelease(Action* action, State* state) // virtual.
{
	std::map<SDLKey, ActionHandler>::const_iterator allHandler (_keyRelease.find(SDLK_ANY));
	if (allHandler != _keyRelease.end())
	{
		ActionHandler handler (allHandler->second);
		(state->*handler)(action);
	}

	// Check if Ctrl, Alt or Shift are pressed and if TRUE then disallow.
	// This creates problems but ensures that CTRL+key etc. are reserved for
	// usage elsehow. I guess, haven't looked into it.
	std::map<SDLKey, ActionHandler>::const_iterator oneHandler (_keyRelease.find(action->getDetails()->key.keysym.sym));
	if (oneHandler != _keyRelease.end()
		&& (action->getDetails()->key.keysym.mod & (KMOD_CTRL | KMOD_ALT | KMOD_SHIFT)) == 0)
	{
		ActionHandler handler (oneHandler->second);
		(state->*handler)(action);
	}
}

/**
 * Sets a function to be called every time this Surface is mouse-pressed.
 * @param handler	- ActionHandler
 * @param btnId		- mouse-button to check for, set to 0 for any button (default 0)
 */
void InteractiveSurface::onMousePress(
		ActionHandler handler,
		Uint8 btnId)
{
	if (handler != nullptr)
		_press[btnId] = handler;
	else
		_press.erase(btnId);
}

/**
 * Sets a function to be called every time this Surface is mouse-released.
 * @param handler	- ActionHandler
 * @param btnId		- mouse-button to check for, set to 0 for any button (default 0)
 */
void InteractiveSurface::onMouseRelease(
		ActionHandler handler,
		Uint8 btnId)
{
	if (handler != nullptr)
		_release[btnId] = handler;
	else
		_release.erase(btnId);
}

/**
 * Sets a function to be called every time this Surface is mouse-clicked.
 * @param handler	- ActionHandler
 * @param btnId		- mouse-button to check for, set to 0 for any button (default SDL_BUTTON_LEFT=1)
 */
void InteractiveSurface::onMouseClick(
		ActionHandler handler,
		Uint8 btnId)
{
	if (handler != nullptr)
		_click[btnId] = handler;
	else
		_click.erase(btnId);
}

/**
 * Sets a function to be called every time the mouse moves into this Surface.
 * @param handler - ActionHandler
 */
void InteractiveSurface::onMouseIn(ActionHandler handler)
{
	_in = handler;
}

/**
 * Sets a function to be called every time the mouse moves over this Surface.
 * @param handler - ActionHandler
 */
void InteractiveSurface::onMouseOver(ActionHandler handler)
{
	_over = handler;
}

/**
 * Sets a function to be called every time the mouse moves out of this Surface.
 * @param handler - ActionHandler
 */
void InteractiveSurface::onMouseOut(ActionHandler handler)
{
	_out = handler;
}

/**
 * Sets a function to be called every time a key is pressed.
 * @note The Surface must be focused and key-modifiers are NOT allowed unless
 * @a keyRelease is left at default SDLK_ANY. Any repercussions of that need to
 * be handled in the final functions.
 * @param handler	- ActionHandler
 * @param keyPress	- keyboard-key to check for, set to 0 for any key (default SDLK_ANY) // kL_note: Fix the SDLK_ANY hocus-pocus.
 */
void InteractiveSurface::onKeyboardPress(
		ActionHandler handler,
		SDLKey keyPress)
{
	if (handler != nullptr)
		_keyPress[keyPress] = handler;
	else
		_keyPress.erase(keyPress);
}

/**
 * Sets a function to be called every time a key is released.
 * @note The Surface must be focused and key-modifiers are NOT allowed unless
 * @a keyRelease is left at default SDLK_ANY. Any repercussions of that need to
 * be handled in the final functions.
 * @param handler		- ActionHandler
 * @param keyRelease	- keyboard-key to check for, set to 0 for any key (default SDLK_ANY) // kL_note: Fix the SDLK_ANY hocus-pocus.
 */
void InteractiveSurface::onKeyboardRelease(
		ActionHandler handler,
		SDLKey keyRelease)
{
	if (handler != nullptr)
		_keyRelease[keyRelease] = handler;
	else
		_keyRelease.erase(keyRelease);
}

/**
 * Flags this InteractiveSurface as a button of a TextList.
 */
void InteractiveSurface::setListButton()
{
	_isListButton = true;
}

}
