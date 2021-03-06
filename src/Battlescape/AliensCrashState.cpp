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

#include "AliensCrashState.h"

#include "DebriefingState.h"

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
 * Initializes all the elements in the AliensCrash screen.
 */
AliensCrashState::AliensCrashState()
{
	_window		= new Window(this, 256, 160, 32, 20);
	_txtTitle	= new Text(246, 80, 37, 50);
	_btnOk		= new TextButton(120, 18, 100, 154);

	setPalette(PAL_BATTLESCAPE);

	add(_window,	"messageWindowBorder",	"battlescape");
	add(_txtTitle,	"messageWindows",		"battlescape");
	add(_btnOk,		"messageWindowButtons",	"battlescape");

	centerSurfaces();


	_window->setBackground(_game->getResourcePack()->getSurface("Diehard"));
	_window->setHighContrast();

	_btnOk->setText(tr("STR_OK"));
	_btnOk->setHighContrast();
	_btnOk->onMouseClick(	static_cast<ActionHandler>(&AliensCrashState::btnOkClick));
	_btnOk->onKeyboardPress(static_cast<ActionHandler>(&AliensCrashState::btnOkClick),
							Options::keyOk);
	_btnOk->onKeyboardPress(static_cast<ActionHandler>(&AliensCrashState::btnOkClick),
							Options::keyOkKeypad);
	_btnOk->onKeyboardPress(static_cast<ActionHandler>(&AliensCrashState::btnOkClick),
							Options::keyCancel);

	_txtTitle->setText(tr("STR_ALL_ALIENS_KILLED_IN_CRASH"));
	_txtTitle->setHighContrast();
	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setVerticalAlign(ALIGN_MIDDLE);
	_txtTitle->setBig();
	_txtTitle->setWordWrap();
}

/**
 * dTor.
 */
AliensCrashState::~AliensCrashState()
{}

/**
 * Exits to the previous screen.
 * @param action - pointer to an Action
 */
void AliensCrashState::btnOkClick(Action*)
{
	_game->popState();
	_game->pushState(new DebriefingState());
}

}
