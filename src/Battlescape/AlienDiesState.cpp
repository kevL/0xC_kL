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

#include "AlienDiesState.h"

#include "../Engine/Game.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Options.h"

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/Window.h"

#include "../Resource/XcomResourcePack.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the AlienDies screen.
 * @note This can be replaced by invoking ErrorMessageState instead; cf.
 * pop-repo, DebriefingState::init() & ::btnOkClick().
 */
AlienDiesState::AlienDiesState()
{
	_window		= new Window(this);
	_txtTitle	= new Text(300, 160, 10, 8);
	_btnOk		= new TextButton(120, 16, 100, 177);

	setInterface("noContainment");

	add(_window,	"window",	"noContainment");
	add(_txtTitle,	"text",		"noContainment");
	add(_btnOk,		"button",	"noContainment");

	centerSurfaces();


	_window->setBackground(_game->getResourcePack()->getSurface("BACK01.SCR"));

	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick(	static_cast<ActionHandler>(&AlienDiesState::btnOkClick));
	_btnOk->onKeyboardPress(static_cast<ActionHandler>(&AlienDiesState::btnOkClick),
							Options::keyOk);
	_btnOk->onKeyboardPress(static_cast<ActionHandler>(&AlienDiesState::btnOkClick),
							Options::keyOkKeypad);
	_btnOk->onKeyboardPress(static_cast<ActionHandler>(&AlienDiesState::btnOkClick),
							Options::keyCancel);

	_txtTitle->setText(tr("STR_ALIEN_DIES_NO_ALIEN_CONTAINMENT_FACILITY"));
	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setVerticalAlign(ALIGN_MIDDLE);
	_txtTitle->setBig();
	_txtTitle->setWordWrap();
}

/**
 * dTor.
 */
AlienDiesState::~AlienDiesState()
{}

/**
 * Exits to the previous screen.
 * @param action - pointer to an Action
 */
void AlienDiesState::btnOkClick(Action*)
{
	if (_game->getQtyStates() == 2u // ie: (1) this, (2) Geoscape
		&& _game->getResourcePack()->isMusicPlaying(OpenXcom::res_MUSIC_TAC_AWARDS))
	{
		_game->getResourcePack()->fadeMusic(_game, 863);
	}
	_game->popState();
}

}
