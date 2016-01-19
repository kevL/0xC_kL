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

#include "DeleteGameState.h"

#include "ErrorMessageState.h"

#include "../Engine/CrossPlatform.h"
#include "../Engine/Game.h"
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
 * Initializes all the elements in the Confirmation screen.
 * @param origin	- game section that originated this state
 * @param file		- name of the save file to delete
 */
DeleteGameState::DeleteGameState(
		OptionsOrigin origin,
		const std::string& file)
	:
		_origin(origin),
		_file(Options::getUserFolder() + file)
{
	_fullScreen = false;

	_window		= new Window(this, 256, 100, 32, 50, POPUP_BOTH);

	_txtMessage	= new Text(246, 32, 37, 70);

	_btnNo		= new TextButton(60, 18,  60, 122);
	_btnYes		= new TextButton(60, 18, 200, 122);

	setInterface(
			"saveMenus",
			false,
			_origin == OPT_BATTLESCAPE);


	add(_window,		"confirmDelete", "saveMenus");
	add(_txtMessage,	"confirmDelete", "saveMenus");
	add(_btnNo,			"confirmDelete", "saveMenus");
	add(_btnYes,		"confirmDelete", "saveMenus");

	centerAllSurfaces();


	_window->setBackground(_game->getResourcePack()->getSurface("BACK01.SCR"));

	_btnYes->setText(tr("STR_YES"));
	_btnYes->onMouseClick((ActionHandler)& DeleteGameState::btnYesClick);
	_btnYes->onKeyboardPress(
					(ActionHandler)& DeleteGameState::btnYesClick,
					Options::keyOk);

	_btnNo->setText(tr("STR_NO"));
	_btnNo->onMouseClick((ActionHandler)& DeleteGameState::btnNoClick);
	_btnNo->onKeyboardPress(
					(ActionHandler)& DeleteGameState::btnNoClick,
					Options::keyCancel);

	_txtMessage->setAlign(ALIGN_CENTER);
	_txtMessage->setBig();
	_txtMessage->setWordWrap();
	_txtMessage->setText(tr("STR_IS_IT_OK_TO_DELETE_THE_SAVED_GAME"));

	if (_origin == OPT_BATTLESCAPE)
		applyBattlescapeTheme();
}

/**
 * dTor.
 */
DeleteGameState::~DeleteGameState()
{}

/**
 *
 */
void DeleteGameState::btnNoClick(Action*)
{
	_game->popState();
}

/**
 *
 */
void DeleteGameState::btnYesClick(Action*)
{
	_game->popState();

	if (CrossPlatform::deleteFile(_file) == false)
	{
		std::wstring error = tr("STR_DELETE_UNSUCCESSFUL");

		if (_origin != OPT_BATTLESCAPE)
			_game->pushState(new ErrorMessageState(
												error,
												_palette,
												_game->getRuleset()->getInterface("errorMessages")->getElement("geoscapeColor")->color,
												"BACK01.SCR",
												_game->getRuleset()->getInterface("errorMessages")->getElement("geoscapePalette")->color));
		else
			_game->pushState(new ErrorMessageState(
												error,
												_palette,
												_game->getRuleset()->getInterface("errorMessages")->getElement("battlescapeColor")->color,
												"Diehard",
												_game->getRuleset()->getInterface("errorMessages")->getElement("battlescapePalette")->color));
	}
}

}
