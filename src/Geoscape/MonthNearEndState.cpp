/*
 * Copyright 2010-2019 OpenXcom Developers.
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

#include "MonthNearEndState.h"

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
 * Initializes all the elements in the MonthNearEnd window.
 */
MonthNearEndState::MonthNearEndState()
{
	_fullScreen = false;

	_window		= new Window(this, 224, 120, 16, 40, POPUP_BOTH);

	_txtTitle	= new Text(214, 17, 21, 54);
	_txtMessage	= new Text(214, 50, 21, 70);

	_btnOk		= new TextButton(180, 18, 38, 134);

	setInterface("lowFuel");

	add(_window,		"window",	"lowFuel");
	add(_txtTitle,		"text",		"lowFuel");
	add(_txtMessage,	"text2",	"lowFuel");
	add(_btnOk,			"button",	"lowFuel");

	centerSurfaces();


	_window->setBackground(_game->getResourcePack()->getSurface("BACK12.SCR"));

	_txtTitle->setText(tr("STR_MONTH_NEAR_END"));
	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setBig();

	_txtMessage->setText(tr("STR_MONTH_NEAR_END_NOTICE"));
	_txtMessage->setAlign(ALIGN_CENTER);
	_txtMessage->setVerticalAlign(ALIGN_MIDDLE);
	_txtMessage->setBig();
	_txtMessage->setWordWrap();

	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick(	static_cast<ActionHandler>(&MonthNearEndState::btnOkClick));
	_btnOk->onKeyboardPress(static_cast<ActionHandler>(&MonthNearEndState::btnOkClick),
							Options::keyCancel);
	_btnOk->onKeyboardPress(static_cast<ActionHandler>(&MonthNearEndState::btnOkClick),
							Options::keyOk);
	_btnOk->onKeyboardPress(static_cast<ActionHandler>(&MonthNearEndState::btnOkClick),
							Options::keyOkKeypad);
}

/**
 * dTor.
 */
MonthNearEndState::~MonthNearEndState()
{}

/**
 * Closes the window.
 * @param action - pointer to an Action
 */
void MonthNearEndState::btnOkClick(Action*)
{
	_game->popState();
}

}
