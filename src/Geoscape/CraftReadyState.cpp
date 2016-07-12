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

#include "GeoscapeCraftState.h"
#include "GeoscapeState.h"

#include "../Basescape/BasescapeState.h"

#include "../Engine/Game.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Options.h"
#include "../Engine/Screen.h"

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/Window.h"

#include "../Resource/ResourcePack.h"

#include "../SaveGame/Base.h"
#include "../SaveGame/Craft.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in a CraftReady window.
 * @param geoState	- pointer to the GeoscapeState
 * @param craft		- poitner to a Craft
 * @param wst		- reference to a wide-string message
 */
CraftReadyState::CraftReadyState(
		GeoscapeState* const geoState,
		Craft* const craft,
		const std::wstring& wst)
	:
		_geoState(geoState),
		_craft(craft)
{
	_fullScreen = false;

	_window			= new Window(this, 256, 160, 32, 20, POPUP_BOTH);
	_txtMessage		= new Text(226, 100, 47, 30);
	_btnGoToBase	= new TextButton(100, 16,  48, 132);
	_btnCraftInfo	= new TextButton(100, 16, 172, 132);
	_btnOk5Secs		= new TextButton(100, 16,  48, 150);
	_btnOk			= new TextButton(100, 16, 172, 150);

	setInterface("geoCraftScreens", true);

	add(_window,		"window",	"geoCraftScreens");
	add(_txtMessage,	"text1",	"geoCraftScreens");
	add(_btnGoToBase,	"button",	"geoCraftScreens");
	add(_btnCraftInfo,	"button",	"geoCraftScreens");
	add(_btnOk5Secs,	"button",	"geoCraftScreens");
	add(_btnOk,			"button",	"geoCraftScreens");

	centerAllSurfaces();


	_window->setBackground(_game->getResourcePack()->getSurface("BACK12.SCR"));

	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick(	static_cast<ActionHandler>(&CraftReadyState::btnOkClick));
	_btnOk->onKeyboardPress(static_cast<ActionHandler>(&CraftReadyState::btnOkClick),
							Options::keyCancel);

	_btnOk5Secs->setText(tr("STR_OK_5_SECONDS"));
	_btnOk5Secs->onMouseClick(static_cast<ActionHandler>(&CraftReadyState::btnOk5SecsClick));

	if (_geoState->is5Sec() == true)
	{
		_btnOk->onKeyboardPress(static_cast<ActionHandler>(&CraftReadyState::btnOkClick),
								Options::keyOk);
		_btnOk->onKeyboardPress(static_cast<ActionHandler>(&CraftReadyState::btnOkClick),
								Options::keyOkKeypad);
	}
	else
	{
		_btnOk5Secs->onKeyboardPress(	static_cast<ActionHandler>(&CraftReadyState::btnOk5SecsClick),
										Options::keyOk);
		_btnOk5Secs->onKeyboardPress(	static_cast<ActionHandler>(&CraftReadyState::btnOk5SecsClick),
										Options::keyOkKeypad);
	}

	_btnGoToBase->setText(tr("STR_GO_TO_BASE"));
	_btnGoToBase->onMouseClick(static_cast<ActionHandler>(&CraftReadyState::btnGoToBaseClick));

	_btnCraftInfo->setText(tr("STR_CRAFT_INFO"));
	_btnCraftInfo->onMouseClick(static_cast<ActionHandler>(&CraftReadyState::btnCraftInfoClick));

	_txtMessage->setText(wst);
	_txtMessage->setAlign(ALIGN_CENTER);
	_txtMessage->setVerticalAlign(ALIGN_MIDDLE);
	_txtMessage->setBig();
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
	_btnOk5Secs->setVisible(_geoState->is5Sec() == false);
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
 * Closes the window and resets the Geoscape time-compression to 5secs.
 * @param action - pointer to an Action
 */
void CraftReadyState::btnOk5SecsClick(Action*)
{
	_geoState->resetTimer();
	_game->popState();
}

/**
 * Closes the window and goes to the Craft's Base.
 * @param action - pointer to an Action
 */
void CraftReadyState::btnGoToBaseClick(Action*)
{
	_game->getScreen()->fadeScreen();

	_geoState->resetTimer();

	_game->popState();
	_game->pushState(new BasescapeState(
									_craft->getBase(),
									_geoState->getGlobe()));
}

/**
 * Closes the window and opens CraftInfo.
 * @param action - pointer to an Action
 */
void CraftReadyState::btnCraftInfoClick(Action*)
{
	_geoState->resetTimer();

	_game->popState();
	_game->pushState(new GeoscapeCraftState(_craft, _geoState));
}

}
