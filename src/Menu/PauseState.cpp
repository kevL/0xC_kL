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

#include "PauseState.h"

#include "AbandonGameState.h"
#include "ListLoadState.h"
#include "ListSaveState.h"
#include "OptionsBattlescapeState.h"
#include "OptionsGeoscapeState.h"
#include "OptionsVideoState.h"

#include "../Battlescape/BattlescapeState.h"

#include "../Engine/Game.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Options.h"

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/Window.h"

#include "../Resource/ResourcePack.h"

#include "../Savegame/SavedBattleGame.h"
#include "../Savegame/SavedGame.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the Pause window.
 * @param origin - game section that originated this state
 */
PauseState::PauseState(OptionsOrigin origin)
	:
		_origin(origin)
{
	_fullScreen = false;

	int x;
	switch (_origin)
	{
		case OPT_GEOSCAPE: x = 20; break;

		default:
		case OPT_MENU:
		case OPT_BATTLESCAPE: x = 52;
	}

	_window		= new Window(this, 216, 158, x, 20, POPUP_BOTH);

	_txtTitle	= new Text(206, 15, x + 5, 30);

	_btnLoad	= new TextButton(180, 18, x + 18, 51);
	_btnSave	= new TextButton(180, 18, x + 18, 73);
//	_btnAbandon	= new TextButton(180, 24, x + 18, 97);
	_btnAbandon	= new TextButton(180, 48, x + 18, 97);

//	_btnOptions	= new TextButton(180, 20, x + 18, 125);
	_btnCancel	= new TextButton(180, 18, x + 18, 151);

	setInterface(
			"pauseMenu",
			false,
			_origin == OPT_BATTLESCAPE);

	add(_window,		"window",	"pauseMenu");
	add(_txtTitle,		"text",		"pauseMenu");
	add(_btnLoad,		"button",	"pauseMenu");
	add(_btnSave,		"button",	"pauseMenu");
	add(_btnAbandon,	"button",	"pauseMenu");
//	add(_btnOptions,	"button",	"pauseMenu");
	add(_btnCancel,		"button",	"pauseMenu");

	centerSurfaces();


	_window->setBackground(_game->getResourcePack()->getSurface("BACK01.SCR"));

	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setBig();
	_txtTitle->setText(tr("STR_OPTIONS_UC"));

	_btnLoad->setText(tr("STR_LOAD_GAME"));
	_btnLoad->onMouseClick(		static_cast<ActionHandler>(&PauseState::btnLoadClick));
	_btnLoad->onKeyboardPress(	static_cast<ActionHandler>(&PauseState::btnLoadClick),
								SDLK_l); // load

	_btnSave->setText(tr("STR_SAVE_GAME"));
	_btnSave->onMouseClick(		static_cast<ActionHandler>(&PauseState::btnSaveClick));
	_btnSave->onKeyboardPress(	static_cast<ActionHandler>(&PauseState::btnSaveClick),
								SDLK_s); // save

	_btnAbandon->setText(tr("STR_ABANDON_GAME"));
	_btnAbandon->onMouseClick(		static_cast<ActionHandler>(&PauseState::btnAbandonClick));
	_btnAbandon->onKeyboardPress(	static_cast<ActionHandler>(&PauseState::btnAbandonClick),
									SDLK_a); // abandon
	_btnAbandon->onMouseClick(		static_cast<ActionHandler>(&PauseState::btnAbandonClick));
	_btnAbandon->onKeyboardPress(	static_cast<ActionHandler>(&PauseState::btnAbandonClick),
									SDLK_q); // quit

//	_btnOptions->setText(tr("STR_GAME_OPTIONS"));
//	_btnOptions->onMouseClick(static_cast<ActionHandler>(&PauseState::btnOptionsClick));
//	_btnOptions->setVisible(false);

	_btnCancel->setText(tr("STR_CANCEL_UC"));
	_btnCancel->onMouseClick(	static_cast<ActionHandler>(&PauseState::btnCancelClick));
	_btnCancel->onKeyboardPress(static_cast<ActionHandler>(&PauseState::btnCancelClick),
								Options::keyCancel);
	_btnCancel->onKeyboardPress(static_cast<ActionHandler>(&PauseState::btnCancelClick),
								Options::keyOk);
	_btnCancel->onKeyboardPress(static_cast<ActionHandler>(&PauseState::btnCancelClick),
								Options::keyOkKeypad);
	_btnCancel->onKeyboardPress(static_cast<ActionHandler>(&PauseState::btnCancelClick),
								SDLK_c);

	switch (_origin)
	{
		case OPT_GEOSCAPE:
			_btnCancel->onKeyboardPress(static_cast<ActionHandler>(&PauseState::btnCancelClick),
										Options::keyGeoOptions);
			break;

		case OPT_BATTLESCAPE:
			_btnCancel->onKeyboardPress(static_cast<ActionHandler>(&PauseState::btnCancelClick),
										Options::keyBattleOptions);
			applyBattlescapeColors();
	}

	if (_game->getSavedGame()->isIronman() == true)
	{
		_btnSave->setVisible(false);
		_btnLoad->setVisible(false);

		_btnAbandon->setText(tr("STR_SAVE_AND_ABANDON_GAME"));
	}
}

/**
 * dTor.
 */
PauseState::~PauseState()
{}

/**
 * Opens the Load Game screen.
 * @param action - pointer to an Action
 */
void PauseState::btnLoadClick(Action*)
{
	_game->pushState(new ListLoadState(_origin));
}

/**
 * Opens the Save Game screen.
 * @param action - pointer to an Action
 */
void PauseState::btnSaveClick(Action*)
{
	if (_origin == OPT_BATTLESCAPE)
		_game->getSavedGame()->getBattleSave()->getBattleState()->clearOptionsBtn();

	_game->pushState(new ListSaveState(_origin));
}

/**
 * Opens the Game Options screen.
 * @param action - pointer to an Action
 *
void PauseState::btnOptionsClick(Action*)
{
	Options::backupDisplay();

	switch (_origin)
	{
		case OPT_GEOSCAPE:
			_game->pushState(new OptionsGeoscapeState(_origin));
			break;

		case OPT_BATTLESCAPE:
			_game->pushState(new OptionsBattlescapeState(_origin));
			break;

		default:
			_game->pushState(new OptionsVideoState(_origin));
	}
} */

/**
 * Opens the AbandonGame window.
 * @param action - pointer to an Action
 */
void PauseState::btnAbandonClick(Action*)
{
	_game->pushState(new AbandonGameState(_origin));
}

/**
 * Exits to the previous screen.
 * @param action - pointer to an Action
 */
void PauseState::btnCancelClick(Action*)
{
	if (_origin == OPT_BATTLESCAPE)
		_game->getSavedGame()->getBattleSave()->getBattleState()->clearOptionsBtn();

	_game->popState();
}

}
