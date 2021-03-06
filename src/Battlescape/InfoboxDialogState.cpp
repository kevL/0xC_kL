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

#include "InfoboxDialogState.h"

#include "Map.h"

#include "../Engine/Game.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Options.h"

#include "../Interface/Cursor.h"
#include "../Interface/Frame.h"
#include "../Interface/Text.h"
#include "../Interface/TextButton.h"

#include "../Savegame/SavedBattleGame.h"
#include "../Savegame/SavedGame.h"


namespace OpenXcom
{

/**
 * Initializes all the elements of an InfoboxDialog.
 * @param wst - reference to a message wide-string
 */
InfoboxDialogState::InfoboxDialogState(const std::wstring& wst)
	:
		_cursorVisible(_game->getCursor()->getVisible()),
		_cursorHidden(_game->getCursor()->getHidden())
{
	_fullScreen = false;

	_frame    = new Frame(260, 90, 30, 86);
	_txtTitle = new Text(250, 58, 35, 93);
	_btnOk    = new TextButton(120, 16, 100, 153);

	setPalette(PAL_BATTLESCAPE);

	add(_frame,    "infoBoxOK",       "battlescape");
	add(_txtTitle, "infoBoxOK",       "battlescape");
	add(_btnOk,    "infoBoxOKButton", "battlescape");

	centerSurfaces();


	_frame->setHighContrast();

	_txtTitle->setText(wst);
	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setVerticalAlign(ALIGN_MIDDLE);
	_txtTitle->setHighContrast();
	_txtTitle->setWordWrap();
	_txtTitle->setBig();

	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick(   static_cast<ActionHandler>(&InfoboxDialogState::btnOkClick));
	_btnOk->onKeyboardPress(static_cast<ActionHandler>(&InfoboxDialogState::btnOkClick),
							Options::keyOk);
	_btnOk->onKeyboardPress(static_cast<ActionHandler>(&InfoboxDialogState::btnOkClick),
							Options::keyOkKeypad);
	_btnOk->onKeyboardPress(static_cast<ActionHandler>(&InfoboxDialogState::btnOkClick),
							Options::keyCancel);
	_btnOk->setHighContrast();

	_game->getCursor()->setVisible();
	_game->getCursor()->setHidden(false);
}

/**
 * dTor.
 */
InfoboxDialogState::~InfoboxDialogState()
{}

/**
 * Exits to the previous screen.
 * @param action - pointer to an Action
 */
void InfoboxDialogState::btnOkClick(Action*)
{
	_game->getCursor()->setVisible(_cursorVisible);
	_game->getCursor()->setHidden(_cursorHidden);

	_game->popState();
}

}
