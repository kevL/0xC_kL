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

#include "DeleteSaveState.h"

#include "ErrorMessageState.h"

#include "../Engine/CrossPlatform.h"
#include "../Engine/Game.h"
#include "../Engine/Language.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Options.h"

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/Window.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/RuleInterface.h"
#include "../Ruleset/Ruleset.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the DeleteSave screen.
 * @param origin	- game section that originated this state
 * @param file		- name of the save file to delete
 */
DeleteSaveState::DeleteSaveState(
		OptionsOrigin origin,
		const std::string& file)
	:
		_origin(origin),
		_file(Options::getUserFolder() + file)
{
	_fullScreen = false;

	_window     = new Window(this, 310, 100, 5, 50, POPUP_HORIZONTAL);

	_txtMessage = new Text(320, 48, 0, 68);

	_btnNo      = new TextButton(80, 18,  60, 122);
	_btnYes     = new TextButton(80, 18, 180, 122);

	setInterface(
			"saveMenus",
			true,
			_origin == OPT_BATTLESCAPE);


	add(_window,     "confirmDelete", "saveMenus");
	add(_txtMessage, "confirmDelete", "saveMenus");
	add(_btnNo,      "confirmDelete", "saveMenus");
	add(_btnYes,     "confirmDelete", "saveMenus");

	centerSurfaces();


	_window->setBackground(_game->getResourcePack()->getSurface("BACK01.SCR"));

	_btnYes->setText(tr("STR_YES"));
	_btnYes->onMouseClick(   static_cast<ActionHandler>(&DeleteSaveState::btnYesClick));
	_btnYes->onKeyboardPress(static_cast<ActionHandler>(&DeleteSaveState::btnYesClick),
							 Options::keyOk);
	_btnYes->onKeyboardPress(static_cast<ActionHandler>(&DeleteSaveState::btnYesClick),
							 Options::keyOkKeypad);

	_btnNo->setText(tr("STR_NO"));
	_btnNo->onMouseClick(   static_cast<ActionHandler>(&DeleteSaveState::btnNoClick));
	_btnNo->onKeyboardPress(static_cast<ActionHandler>(&DeleteSaveState::btnNoClick),
							Options::keyCancel);

	_txtMessage->setAlign(ALIGN_CENTER);
	_txtMessage->setBig();
	_txtMessage->setText(tr("STR_CONFIRM_DELETE_SAVED_GAME_").arg(Language::fsToWstr(file)));

	if (_origin == OPT_BATTLESCAPE)
		applyBattlescapeColors();
}

/**
 * dTor.
 */
DeleteSaveState::~DeleteSaveState()
{}

/**
 * Cancels this state.
 */
void DeleteSaveState::btnNoClick(Action*)
{
	_game->popState();
}

/**
 * Cancels this state and attempts to delete the focused entry.
 */
void DeleteSaveState::btnYesClick(Action*)
{
	_game->popState();

	if (CrossPlatform::deleteFile(_file) == false)
	{
		const std::wstring error (tr("STR_DELETE_UNSUCCESSFUL"));

		const RuleInterface* const uiRule (_game->getRuleset()->getInterface("errorMessages"));
		switch (_origin)
		{
			case OPT_MAIN_START:
			case OPT_GEOSCAPE:
				_game->pushState(new ErrorMessageState(
													error,
													_palette,
													uiRule->getElement("geoscapeColor")->color,
													"BACK01.SCR",
													uiRule->getElement("geoscapePalette")->color));
				break;

			case OPT_BATTLESCAPE:
				_game->pushState(new ErrorMessageState(
													error,
													_palette,
													uiRule->getElement("battlescapeColor")->color,
													"Diehard",
													uiRule->getElement("battlescapePalette")->color));
		}
	}
}

}
