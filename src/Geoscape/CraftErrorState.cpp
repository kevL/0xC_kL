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

#include "CraftErrorState.h"

#include "GeoscapeState.h"

#include "../Engine/Game.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Options.h"

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/Window.h"

#include "../Resource/ResourcePack.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in a Cannot Refuel/Rearm window.
 * @param state	- pointer to the GeoscapeState
 * @param wst	- reference to the error message
 */
CraftErrorState::CraftErrorState(
		GeoscapeState* const geoState,
		const std::wstring& wst)
	:
		_geoState(geoState)
{
	_fullScreen = false;

	_window     = new Window(this, 256, 160, 32, 20, POPUP_BOTH);
	_txtMessage = new Text(226, 118, 47, 30);
	_btnOk5Secs = new TextButton(100, 18,  48, 150);
	_btnOk      = new TextButton(100, 18, 172, 150);

	setInterface("geoCraftScreens");

	add(_window,     "window", "geoCraftScreens");
	add(_txtMessage, "text1",  "geoCraftScreens");
	add(_btnOk5Secs, "button", "geoCraftScreens");
	add(_btnOk,      "button", "geoCraftScreens");

	centerSurfaces();


	_window->setBackground(_game->getResourcePack()->getSurface("BACK12.SCR"));

	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick(   static_cast<ActionHandler>(&CraftErrorState::btnOkClick));
	_btnOk->onKeyboardPress(static_cast<ActionHandler>(&CraftErrorState::btnOkClick),
							Options::keyCancel);

	_txtMessage->setAlign(ALIGN_CENTER);
	_txtMessage->setVerticalAlign(ALIGN_MIDDLE);
	_txtMessage->setBig();
	_txtMessage->setWordWrap();
	_txtMessage->setText(wst);
}

/**
 * dTor.
 */
CraftErrorState::~CraftErrorState()
{}

/**
 * Initializes the state.
 */
void CraftErrorState::init() // override
{
	State::init();

	if (_geoState->is5Sec() == false)
	{
		_btnOk5Secs->setText(tr("STR_OK_5_SECONDS"));
		_btnOk5Secs->onMouseClick(static_cast<ActionHandler>(&CraftErrorState::btnOk5SecsClick));
		_btnOk5Secs->onKeyboardPress(static_cast<ActionHandler>(&CraftErrorState::btnOk5SecsClick),
									 Options::keyOk);
		_btnOk5Secs->onKeyboardPress(static_cast<ActionHandler>(&CraftErrorState::btnOk5SecsClick),
									 Options::keyOkKeypad);
	}
	else
	{
		_btnOk5Secs->setVisible(false);

		_btnOk->onKeyboardPress(static_cast<ActionHandler>(&CraftErrorState::btnOkClick),
								Options::keyOk);
		_btnOk->onKeyboardPress(static_cast<ActionHandler>(&CraftErrorState::btnOkClick),
								Options::keyOkKeypad);
	}
}

/**
 * Closes the window.
 * @param action - pointer to an Action
 */
void CraftErrorState::btnOkClick(Action*)
{
	_game->popState();
}

/**
 * Closes the window.
 * @param action - pointer to an Action
 */
void CraftErrorState::btnOk5SecsClick(Action*)
{
	_geoState->resetTimer();
	_game->popState();
}

}
