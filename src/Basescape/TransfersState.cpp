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

#include "TransfersState.h"

//#include <sstream>

#include "../Engine/Game.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Options.h"

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextList.h"
#include "../Interface/Window.h"

#include "../Resource/ResourcePack.h"

#include "../Savegame/Base.h"
#include "../Savegame/Transfer.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the Transfers window.
 * @param base - pointer to the Base to get info from
 */
TransfersState::TransfersState(Base* const base)
	:
		_base(base)
{
	_fullScreen = false;

	_window			= new Window(this, 320, 184, 0, 8, POPUP_BOTH);

	_txtTitle		= new Text(288, 17, 16, 17);
	_txtBaseLabel	= new Text( 80,  9, 16, 17);

	_txtItem		= new Text(114, 9,  16, 34);
	_txtQuantity	= new Text( 54, 9, 179, 34);
	_txtArrivalTime	= new Text( 28, 9, 254, 34);

	_lstTransfers	= new TextList(285, 121, 16, 45);

	_btnOk			= new TextButton(288, 16, 16, 169);

	setInterface("transferInfo");

	add(_window,			"window",	"transferInfo");
	add(_txtTitle,			"text",		"transferInfo");
	add(_txtBaseLabel,		"text",		"transferInfo");
	add(_txtItem,			"text",		"transferInfo");
	add(_txtQuantity,		"text",		"transferInfo");
	add(_txtArrivalTime,	"text",		"transferInfo");
	add(_lstTransfers,		"list",		"transferInfo");
	add(_btnOk,				"button",	"transferInfo");

	centerSurfaces();


	_window->setBackground(_game->getResourcePack()->getSurface("BACK13.SCR"));

	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick(	static_cast<ActionHandler>(&TransfersState::btnOkClick));
	_btnOk->onKeyboardPress(static_cast<ActionHandler>(&TransfersState::btnOkClick),
							Options::keyOk);
	_btnOk->onKeyboardPress(static_cast<ActionHandler>(&TransfersState::btnOkClick),
							Options::keyOkKeypad);
	_btnOk->onKeyboardPress(static_cast<ActionHandler>(&TransfersState::btnOkClick),
							Options::keyCancel);

	_txtTitle->setBig();
	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setText(tr("STR_TRANSFERS_UC"));

	_txtBaseLabel->setText(_base->getLabel());

	_txtItem->setText(tr("STR_ITEM"));

	_txtQuantity->setText(tr("STR_QUANTITY"));

	_txtArrivalTime->setText(tr("STR_ARRIVAL_TIME_HOURS"));

	_lstTransfers->setColumns(3, 155,75,28);
	_lstTransfers->setBackground(_window);
	_lstTransfers->setSelectable();

	for (std::vector<Transfer*>::const_iterator
			i = _base->getTransfers()->begin();
			i != _base->getTransfers()->end();
			++i)
	{
		_lstTransfers->addRow(
							3,
							(*i)->getLabel(_game->getLanguage()).c_str(),
							Text::intWide((*i)->getQuantity()).c_str(),
							Text::intWide((*i)->getHours()).c_str());
	}
}

/**
 * dTor.
 */
TransfersState::~TransfersState()
{}

/**
 * Exits to the previous screen.
 * @param action - pointer to an Action
 */
void TransfersState::btnOkClick(Action*)
{
	_game->popState();
}

}
