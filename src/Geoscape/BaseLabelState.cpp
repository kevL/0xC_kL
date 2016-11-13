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

#include "BaseLabelState.h"

#include "Globe.h"

#include "../Basescape/PlaceLiftState.h"

#include "../Engine/Action.h"
#include "../Engine/Game.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Options.h"

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextEdit.h"
#include "../Interface/Window.h"

#include "../Resource/ResourcePack.h"

#include "../Savegame/Base.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the BaseLabel window.
 * @param base			- pointer to the Base to label
 * @param globe			- pointer to the Geoscape globe
 * @param isFirstBase	- true if this is the player's first base (default false)
 */
BaseLabelState::BaseLabelState(
		Base* const base,
		Globe* const globe,
		bool isFirstBase)
	:
		_base(base),
		_globe(globe),
		_isFirstBase(isFirstBase)
{
	_globe->onMouseOver(nullptr);

	_fullScreen = false;

	_window		= new Window(this, 192, 88, 32, 60, POPUP_BOTH);
	_txtTitle	= new Text(182, 17, 37, 70);
	_edtLabel	= new TextEdit(this, 127, 16, 59, 94);
	_btnOk		= new TextButton(162, 16, 47, 118);

	setInterface("baseNaming");

	add(_window,	"window",	"baseNaming");
	add(_txtTitle,	"text",		"baseNaming");
	add(_edtLabel,	"text",		"baseNaming");
	add(_btnOk,		"button",	"baseNaming");

	centerSurfaces();


	_window->setBackground(_game->getResourcePack()->getSurface("BACK01.SCR"));

	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick(	static_cast<ActionHandler>(&BaseLabelState::btnOkClick));
	_btnOk->onKeyboardPress(static_cast<ActionHandler>(&BaseLabelState::btnOkClick),
							Options::keyOk);
	_btnOk->onKeyboardPress(static_cast<ActionHandler>(&BaseLabelState::btnOkClick),
							Options::keyOkKeypad);
	_btnOk->setVisible(false);

	_txtTitle->setText(tr("STR_BASE_NAME"));
	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setBig();

	_edtLabel->setBig();
	_edtLabel->setFocusEdit();
	_edtLabel->onTextChange(static_cast<ActionHandler>(&BaseLabelState::edtLabelChange));
}

/**
 * dTor.
 */
BaseLabelState::~BaseLabelState()
{}

/**
 * Updates the base label and disables the OK button if no label is entered.
 * @param action - pointer to an Action
 */
void BaseLabelState::edtLabelChange(Action* action)
{
	_base->setLabel(_edtLabel->getText());

	if (_edtLabel->getText().empty() == true)
		_btnOk->setVisible(false);
	else if (action->getDetails()->key.keysym.sym == Options::keyCancel)
	{
		_btnOk->setVisible(false);
		_edtLabel->setText(L"");
	}
	else
		_btnOk->setVisible();
}

/**
 * Goes to the PlaceLift screen.
 * @param action - pointer to an Action
 */
void BaseLabelState::btnOkClick(Action*)
{
	_game->popState(); // <- this
	_game->popState(); // <- BuildNewBaseState

	if (_isFirstBase == false)
		_game->popState(); // <- ConfirmNewBaseState

	_game->pushState(new PlaceLiftState(
									_base,
									_globe,
									_isFirstBase));
}

}
