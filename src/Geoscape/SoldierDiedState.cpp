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

#include "SoldierDiedState.h"

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
 * Initializes all the elements in the SoldierDied window.
 * @param soldier	- reference to the label of the Soldier
 * @param base		- reference to the label of the Soldier's Base
 */
SoldierDiedState::SoldierDiedState(
		const std::wstring& soldier,
		const std::wstring& base)
{
	_fullScreen = false;

	_window		= new Window(this, 192, 104, 32, 48, POPUP_BOTH);
	_txtTitle	= new Text(160, 44, 48,  58);
	_txtBase	= new Text(160,  9, 48, 104);
	_btnOk		= new TextButton(80, 16, 88, 126);

	setPalette(PAL_GEOSCAPE, BACKPAL_RED_D);

	add(_window);
	add(_txtTitle);
	add(_txtBase);
	add(_btnOk);

	centerSurfaces();


	_window->setColor(CYAN);
	_window->setBackground(_game->getResourcePack()->getSurface("BACK15.SCR"));

	_btnOk->setColor(CYAN);
	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick(	static_cast<ActionHandler>(&SoldierDiedState::btnOkClick));
	_btnOk->onKeyboardPress(static_cast<ActionHandler>(&SoldierDiedState::btnOkClick),
							Options::keyOk);
	_btnOk->onKeyboardPress(static_cast<ActionHandler>(&SoldierDiedState::btnOkClick),
							Options::keyOkKeypad);
	_btnOk->onKeyboardPress(static_cast<ActionHandler>(&SoldierDiedState::btnOkClick),
							Options::keyCancel);

	std::wstring notice (soldier);
	notice += L'\n';
	notice += tr("STR_SOLDIER_DIED");
	_txtTitle->setText(notice);
	_txtTitle->setColor(CYAN);
	_txtTitle->setBig();
	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setVerticalAlign(ALIGN_MIDDLE);

	_txtBase->setText(base);
	_txtBase->setColor(CYAN);
	_txtBase->setSmall();
	_txtBase->setAlign(ALIGN_CENTER);
}

/**
 * dTor.
 */
SoldierDiedState::~SoldierDiedState()
{}

/**
 * Exits to the previous screen.
 * @param action - pointer to an Action
 */
void SoldierDiedState::btnOkClick(Action*)
{
	_game->popState();
}

}

