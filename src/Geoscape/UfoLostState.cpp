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

#include "UfoLostState.h"

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
 * Initializes all the elements in the UfoLost window.
 * @param id - reference to the ID of the UFO
 */
UfoLostState::UfoLostState(const std::wstring& id)
	:
		_id(id)
{
	_fullScreen = false;


	_window		= new Window(this, 192, 104, 32, 48, POPUP_BOTH);
	_txtTitle	= new Text(160, 32, 48, 72);
	_btnOk		= new TextButton(80, 16, 88, 114);

	setInterface("UFOInfo");

	add(_window,	"window",	"UFOInfo");
	add(_txtTitle,	"text",		"UFOInfo");
	add(_btnOk,		"button",	"UFOInfo");

	centerAllSurfaces();


	_window->setBackground(_game->getResourcePack()->getSurface("BACK15.SCR"));

	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)& UfoLostState::btnOkClick);
	_btnOk->onKeyboardPress(
					(ActionHandler)& UfoLostState::btnOkClick,
					Options::keyOk);
	_btnOk->onKeyboardPress(
					(ActionHandler)& UfoLostState::btnOkClick,
					Options::keyOkKeypad);
	_btnOk->onKeyboardPress(
					(ActionHandler)& UfoLostState::btnOkClick,
					Options::keyCancel);

	_txtTitle->setBig();
	_txtTitle->setAlign(ALIGN_CENTER);
	std::wstring wst (_id);
	wst += L'\n';
	wst += tr("STR_TRACKING_LOST");
	_txtTitle->setText(wst);
}

/**
 * dTor.
 */
UfoLostState::~UfoLostState()
{}

/**
 * Returns to the previous screen.
 * @param action - pointer to an Action
 */
void UfoLostState::btnOkClick(Action*)
{
	_game->popState();
}

}
