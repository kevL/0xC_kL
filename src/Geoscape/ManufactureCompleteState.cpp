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

#include "ManufactureCompleteState.h"

//#include <assert.h>

#include "GeoscapeState.h"

#include "../Basescape/BasescapeState.h"
#include "../Basescape/ManufactureState.h"

#include "../Engine/Game.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Options.h"
#include "../Engine/Screen.h"

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/Window.h"

#include "../Resource/ResourcePack.h"

#include "../Savegame/Base.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the ManufactureComplete window.
 * @param base		- pointer to Base the project belongs to
 * @param item		- reference to the item that completed
 * @param geoState	- pointer to GeoscapeState
 * @param allocate	- true to show the goToBase button
 * @param doneType	- what ended the production (Manufacture.h)
 */
ManufactureCompleteState::ManufactureCompleteState(
		Base* const base,
		const std::wstring& item,
		GeoscapeState* const geoState,
		bool allocate,
		ManufactureProgress doneType)
	:
		_base(base),
		_geoState(geoState)
{
	_fullScreen = false;

	_window		= new Window(this, 256, 160, 32, 20, POPUP_BOTH);

	_txtMessage	= new Text(200, 115, 60, 35);

	_btnBase	= new TextButton(72, 16,  48, 154);
	_btnOk5Secs	= new TextButton(72, 16, 124, 154);
	_btnOk		= new TextButton(72, 16, 200, 154);

	setInterface("geoManufacture");

	add(_window,		"window",	"geoManufacture");
	add(_txtMessage,	"text1",	"geoManufacture");
	add(_btnBase,		"button",	"geoManufacture");
	add(_btnOk5Secs,	"button",	"geoManufacture");
	add(_btnOk,			"button",	"geoManufacture");

	centerSurfaces();


	_window->setBackground(_game->getResourcePack()->getSurface("BACK17.SCR"));

	_btnOk->setText(tr((allocate == true) ? "STR_OK" : "STR_MORE"));
	_btnOk->onMouseClick(	static_cast<ActionHandler>(&ManufactureCompleteState::btnOkClick));
	_btnOk->onKeyboardPress(static_cast<ActionHandler>(&ManufactureCompleteState::btnOkClick),
							Options::keyCancel);

	_btnOk5Secs->setText(tr("STR_OK_5_SECONDS"));
	_btnOk5Secs->onMouseClick(		static_cast<ActionHandler>(&ManufactureCompleteState::btnOk5SecsClick));
	_btnOk5Secs->onKeyboardPress(	static_cast<ActionHandler>(&ManufactureCompleteState::btnOk5SecsClick),
									Options::keyGeoSpeed1);

	_btnBase->setText(_base->getLabel()); //tr("STR_GO_TO_BASE")
	_btnBase->setVisible(allocate == true);
	_btnBase->onMouseClick(		static_cast<ActionHandler>(&ManufactureCompleteState::btnGotoBaseClick));
	_btnBase->onKeyboardPress(	static_cast<ActionHandler>(&ManufactureCompleteState::btnGotoBaseClick),
								Options::keyOk);
	_btnBase->onKeyboardPress(	static_cast<ActionHandler>(&ManufactureCompleteState::btnGotoBaseClick),
								Options::keyOkKeypad);

	std::wstring wst;
	switch (doneType)
	{
		case PROG_CONSTRUCTION:
			wst = tr("STR_CONSTRUCTION_OF_FACILITY_AT_BASE_IS_COMPLETE")
					.arg(item).arg(base->getLabel());
			break;
		case PROG_COMPLETE:
			wst = tr("STR_PRODUCTION_OF_ITEM_AT_BASE_IS_COMPLETE")
					.arg(item).arg(base->getLabel());
			break;
		case PROG_NOT_ENOUGH_MONEY:
			wst = tr("STR_NOT_ENOUGH_MONEY_TO_PRODUCE_ITEM_AT_BASE")
					.arg(item).arg(base->getLabel());
			break;
		case PROG_NOT_ENOUGH_MATERIALS:
			wst = tr("STR_NOT_ENOUGH_SPECIAL_MATERIALS_TO_PRODUCE_ITEM_AT_BASE")
					.arg(item).arg(base->getLabel());
//			break;
//		default: assert(false);
	}
	_txtMessage->setText(wst);
	_txtMessage->setAlign(ALIGN_CENTER);
	_txtMessage->setVerticalAlign(ALIGN_MIDDLE);
	_txtMessage->setBig();
	_txtMessage->setWordWrap();
}

/**
 * dTor.
 */
ManufactureCompleteState::~ManufactureCompleteState()
{}

/**
 * Initializes the state.
 */
void ManufactureCompleteState::init()
{
	State::init();
	_btnOk5Secs->setVisible(_geoState->is5Sec() == false);
}

/**
 * Closes the window.
 * @param action - pointer to an Action
 */
void ManufactureCompleteState::btnOkClick(Action*)
{
	_game->popState();
}

/**
 * Reduces the speed to 5 Secs and returns to the previous screen.
 * @param action - pointer to an Action
 */
void ManufactureCompleteState::btnOk5SecsClick(Action*)
{
	_geoState->resetTimer();
	_game->popState();
}

/**
 * Goes to the Base where the Manufacture completed.
 * @param action - pointer to an Action
 */
void ManufactureCompleteState::btnGotoBaseClick(Action*)
{
	_game->getScreen()->fadeScreen();

	_geoState->resetTimer();

	_game->popState();
	_game->pushState(new BasescapeState(_base, _geoState->getGlobe()));
}

}
