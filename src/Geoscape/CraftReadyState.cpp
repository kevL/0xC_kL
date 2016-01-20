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

#include "CraftReadyState.h"

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
 * Initializes all the elements in a CraftReady window.
 * @param state	- pointer to the GeoscapeState
 * @param wst	- reference to the message
 */
CraftReadyState::CraftReadyState(
		GeoscapeState* state,
		const std::wstring& wst)
	:
		_state(state)
{
	_fullScreen = false;

	_window		= new Window(this, 256, 160, 32, 20, POPUP_BOTH);
	_txtMessage	= new Text(226, 118, 47, 30);
	_btnOk5Secs	= new TextButton(100, 18,  48, 150);
	_btnOk		= new TextButton(100, 18, 172, 150);

	setInterface("geoCraftScreens", true);

	add(_window,		"window",	"geoCraftScreens");
	add(_txtMessage,	"text1",	"geoCraftScreens");
	add(_btnOk5Secs,	"button",	"geoCraftScreens");
	add(_btnOk,			"button",	"geoCraftScreens");

	centerAllSurfaces();


	_window->setBackground(_game->getResourcePack()->getSurface("BACK12.SCR"));

	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)& CraftReadyState::btnOkClick);
	_btnOk->onKeyboardPress(
					(ActionHandler)& CraftReadyState::btnOkClick,
					Options::keyCancel);

	_btnOk5Secs->setText(tr("STR_OK_5_SECONDS"));
	_btnOk5Secs->onMouseClick((ActionHandler)& CraftReadyState::btnOk5SecsClick);

	if (_state->is5Sec() == true)
	{
		_btnOk->onKeyboardPress(
						(ActionHandler)& CraftReadyState::btnOkClick,
						Options::keyOk);
		_btnOk->onKeyboardPress(
						(ActionHandler)& CraftReadyState::btnOkClick,
						Options::keyOkKeypad);
	}
	else
	{
		_btnOk5Secs->onKeyboardPress(
						(ActionHandler)& CraftReadyState::btnOk5SecsClick,
						Options::keyOk);
		_btnOk5Secs->onKeyboardPress(
						(ActionHandler)& CraftReadyState::btnOk5SecsClick,
						Options::keyOkKeypad);
	}

	_txtMessage->setAlign(ALIGN_CENTER);
	_txtMessage->setVerticalAlign(ALIGN_MIDDLE);
	_txtMessage->setBig();
	_txtMessage->setText(wst);
}

/**
 * dTor.
 */
CraftReadyState::~CraftReadyState()
{}

/**
 * Initializes the state.
 */
void CraftReadyState::init()
{
	State::init();
	_btnOk5Secs->setVisible(_state->is5Sec() == false);
}

/**
 * Closes the window.
 * @param action - pointer to an Action
 */
void CraftReadyState::btnOkClick(Action*)
{
	_game->popState();
}

/**
 * Closes the window.
 * @param action - pointer to an Action
 */
void CraftReadyState::btnOk5SecsClick(Action*)
{
	_state->resetTimer();
	_game->popState();
}

}
