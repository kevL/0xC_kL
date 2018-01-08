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

#include "ConfirmLoadState.h"

#include "ListLoadState.h"
#include "LoadGameState.h"

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
 * Initializes all the elements in the ConfirmLoad screen.
 * @param origin	- game section that originated this state
 * @param file		- reference to the name of the save file without extension
 * @param parent	- pointer to parent ListLoadState
 */
ConfirmLoadState::ConfirmLoadState(
		OptionsOrigin origin,
		const std::string& file,
		ListLoadState* const parent)
	:
		_origin(origin),
		_file(file),
		_parent(parent)
{
	_fullScreen = false;

	_window		= new Window(this, 216, 100, 52, 50, POPUP_BOTH);
	_txtText	= new Text(180, 60, 70, 60);

	_btnNo		= new TextButton(60, 20, 65, 122);
	_btnYes		= new TextButton(60, 20, 195, 122);

	setInterface(
			"saveMenus",
			false,
			_origin == OPT_BATTLESCAPE);

	add(_window,	"confirmLoad", "saveMenus");
	add(_btnYes,	"confirmLoad", "saveMenus");
	add(_btnNo,		"confirmLoad", "saveMenus");
	add(_txtText,	"confirmLoad", "saveMenus");

	centerSurfaces();


	_window->setBackground(_game->getResourcePack()->getSurface("BACK01.SCR"));

	_btnYes->setText(tr("STR_YES"));
	_btnYes->onMouseClick(		static_cast<ActionHandler>(&ConfirmLoadState::btnYesClick));
	_btnYes->onKeyboardPress(	static_cast<ActionHandler>(&ConfirmLoadState::btnYesClick),
								Options::keyOk);
	_btnYes->onKeyboardPress(	static_cast<ActionHandler>(&ConfirmLoadState::btnYesClick),
								Options::keyOkKeypad);

	_btnNo->setText(tr("STR_NO"));
	_btnNo->onMouseClick(	static_cast<ActionHandler>(&ConfirmLoadState::btnNoClick));
	_btnNo->onKeyboardPress(static_cast<ActionHandler>(&ConfirmLoadState::btnNoClick),
							Options::keyCancel);

	_txtText->setText(tr("STR_MISSING_CONTENT_PROMPT"));
	_txtText->setAlign(ALIGN_CENTER);
	_txtText->setBig();
	_txtText->setWordWrap();

	if (_origin == OPT_BATTLESCAPE)
		applyBattlescapeColors();
}

/**
 * Cleans up this ConfirmLoad state.
 */
ConfirmLoadState::~ConfirmLoadState()
{}

/**
 * Proceed to load the save.
 * @param action - pointer to an Action
 */
void ConfirmLoadState::btnYesClick(Action*)
{
	_parent->hideElements();

	_game->popState();
	_game->pushState(new LoadGameState(
									_origin,
									_file,
									_palette,
									_parent));
}

/**
 * Abort loading and return to save-list.
 * @param action - pointer to an Action
 */
void ConfirmLoadState::btnNoClick(Action*)
{
	_game->popState();
}

}
