/*
 * Copyright 2010-2017 OpenXcom Developers.
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

#include "DerankSoldierState.h"

#include "../Engine/Game.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Options.h"

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/Window.h"

#include "../Resource/ResourcePack.h"

#include "../Savegame/Base.h"
#include "../Savegame/Soldier.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the DerankSoldier window.
 * @param base			- pointer to the Base to get info from
 * @param solId			- the soldier-ID to derank
 * @param isPlayerError	- true if the Soldier's rank is too low for a demotion
 */
DerankSoldierState::DerankSoldierState(
		Base* const base,
		size_t solId,
		bool isPlayerError)
	:
		_base(base),
		_solId(solId)
{
	_fullScreen = false;

	_window		= new Window(this, 152, 80, 84, 60);
	_txtTitle	= new Text(142, 16, 89, 75);
	_txtSoldier	= new Text(142,  9, 89, 95);
	_btnCancel	= new TextButton(48, 16,  98, 115);
	_btnOk		= new TextButton(48, 16, 174, 115);

	setInterface("sackSoldier");

	add(_window,		"window",	"sackSoldier");
	add(_txtTitle,		"text",		"sackSoldier");
	add(_txtSoldier,	"text",		"sackSoldier");
	add(_btnCancel,		"button",	"sackSoldier");
	add(_btnOk,			"button",	"sackSoldier");

	centerSurfaces();


	_window->setBackground(_game->getResourcePack()->getSurface("BACK13.SCR"));

	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick(	static_cast<ActionHandler>(&DerankSoldierState::btnOkClick));
	_btnOk->onKeyboardPress(static_cast<ActionHandler>(&DerankSoldierState::btnOkClick),
							Options::keyOk);
	_btnOk->onKeyboardPress(static_cast<ActionHandler>(&DerankSoldierState::btnOkClick),
							Options::keyOkKeypad);

	_btnCancel->setText(tr("STR_CANCEL_UC"));
	_btnCancel->onMouseClick(	static_cast<ActionHandler>(&DerankSoldierState::btnCancelClick));
	_btnCancel->onKeyboardPress(static_cast<ActionHandler>(&DerankSoldierState::btnCancelClick),
								Options::keyCancel);

	_txtTitle->setText(tr("STR_DERANK"));
	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setBig();

	if (isPlayerError == false)
		_txtSoldier->setText(_base->getSoldiers()->at(_solId)->getLabel());
	else
	{
		_txtSoldier->setText(tr("STR_CANNOT_DERANK"));
		_btnOk->setVisible(false);
	}
	_txtSoldier->setAlign(ALIGN_CENTER);
}

/**
 * dTor.
 */
DerankSoldierState::~DerankSoldierState()
{}

/**
 * Sacks the soldier and returns to the previous screen.
 * @param action - pointer to an Action
 */
void DerankSoldierState::btnOkClick(Action*)
{
	_base->getSoldiers()->at(_solId)->demoteRank();
	_game->popState();
}

/**
 * Exits to the previous screen.
 * @param action - pointer to an Action
 */
void DerankSoldierState::btnCancelClick(Action*)
{
	_game->popState();
}

}
