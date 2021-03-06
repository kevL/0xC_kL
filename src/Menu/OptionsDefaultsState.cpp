/*
 * Copyright 2010-2020 OpenXcom Developers.
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

#include "OptionsDefaultsState.h"

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
 * Initializes all the elements in the Restore Defaults screen.
 * @param origin Game section that originated this state.
 * @param state Pointer to the base Options state.
 */
OptionsDefaultsState::OptionsDefaultsState(
		OptionsOrigin origin,
		OptionsBaseState* state)
	:
		_origin(origin),
		_state(state)
{
	_fullScreen = false;

	_window		= new Window(this, 256, 100, 32, 50, POPUP_BOTH);
	_btnYes		= new TextButton(60, 18, 60, 122);
	_btnNo		= new TextButton(60, 18, 200, 122);
	_txtTitle	= new Text(246, 32, 37, 70);

	setInterface(
			"mainMenu",
			false,
			_origin == OPT_BATTLESCAPE);

	add(_window,	"confirmDefaults", "mainMenu");
	add(_btnYes,	"confirmDefaults", "mainMenu");
	add(_btnNo,		"confirmDefaults", "mainMenu");
	add(_txtTitle,	"confirmDefaults", "mainMenu");

	centerSurfaces();


	_window->setBackground(_game->getResourcePack()->getSurface("BACK01.SCR"));

	_btnYes->setText(tr("STR_YES"));
	_btnYes->onMouseClick(		static_cast<ActionHandler>(&OptionsDefaultsState::btnYesClick));
	_btnYes->onKeyboardPress(	static_cast<ActionHandler>(&OptionsDefaultsState::btnYesClick),
								Options::keyOk);

	_btnNo->setText(tr("STR_NO"));
	_btnNo->onMouseClick(	static_cast<ActionHandler>(&OptionsDefaultsState::btnNoClick));
	_btnNo->onKeyboardPress(static_cast<ActionHandler>(&OptionsDefaultsState::btnNoClick),
							Options::keyCancel);

	_txtTitle->setText(tr("STR_RESTORE_DEFAULTS_PROMPT"));
	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setBig();
	_txtTitle->setWordWrap();

	if (_origin == OPT_BATTLESCAPE)
		applyBattlescapeColors();
}

/**
 * dTor.
 */
OptionsDefaultsState::~OptionsDefaultsState()
{}

/**
 * Restores the default options.
 * @param action - pointer to an Action
 */
void OptionsDefaultsState::btnYesClick(Action* action)
{
	if (_origin == OPT_MAIN_START && Options::rulesets.size() > 1u)
		Options::reload = true;

	Options::resetDefaults();

	_game->defaultLanguage();
	_game->popState();
	_state->btnOkClick(action);
}

/**
 * Closes the window.
 * @param action - pointer to an Action
 */
void OptionsDefaultsState::btnNoClick(Action*)
{
	_game->popState();
}

}
