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

#include "CannotReequipState.h"

//#include <sstream>

#include "../Engine/Game.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Options.h"

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextList.h"
#include "../Interface/Window.h"

#include "../Resource/XcomResourcePack.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the Cannot Reequip screen.
 * @param missingItems - vector of ReequipStat items still missing
 */
CannotReequipState::CannotReequipState(std::vector<ReequipStat> missingItems)
{
	_window			= new Window(this, 320, 200);

	_txtTitle		= new Text(300, 69, 10, 9);

	_txtItem		= new Text(162, 9,  16, 77);
	_txtQuantity	= new Text( 46, 9, 178, 77);
	_txtCraft		= new Text( 80, 9, 224, 77);

	_lstItems		= new TextList(285, 89, 16, 87);

	_btnOk			= new TextButton(288, 16, 16, 177);

	setInterface("cannotReequip");

	add(_window,		"window",	"cannotReequip");
	add(_txtTitle,		"heading",	"cannotReequip");
	add(_txtItem,		"text",		"cannotReequip");
	add(_txtQuantity,	"text",		"cannotReequip");
	add(_txtCraft,		"text",		"cannotReequip");
	add(_lstItems,		"list",		"cannotReequip");
	add(_btnOk,			"button",	"cannotReequip");

	centerAllSurfaces();


	_window->setBackground(_game->getResourcePack()->getSurface("BACK01.SCR"));

	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick(	static_cast<ActionHandler>(&CannotReequipState::btnOkClick));
	_btnOk->onKeyboardPress(static_cast<ActionHandler>(&CannotReequipState::btnOkClick),
							Options::keyOk);
	_btnOk->onKeyboardPress(static_cast<ActionHandler>(&CannotReequipState::btnOkClick),
							Options::keyOkKeypad);
	_btnOk->onKeyboardPress(static_cast<ActionHandler>(&CannotReequipState::btnOkClick),
							Options::keyCancel);

	_txtTitle->setText(tr("STR_NOT_ENOUGH_EQUIPMENT_TO_FULLY_RE_EQUIP_SQUAD"));
	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setBig();

	_txtItem->setText(tr("STR_ITEM"));
	_txtQuantity->setText(tr("STR_QUANTITY"));
	_txtCraft->setText(tr("STR_CRAFT"));

	_lstItems->setColumns(3, 154,46,72);
	_lstItems->setBackground(_window);
	_lstItems->setSelectable();
	for (std::vector<ReequipStat>::const_iterator
			i = missingItems.begin();
			i != missingItems.end();
			++i)
	{
		_lstItems->addRow(
						3,
						tr(i->type).c_str(),
						Text::intWide(i->qtyLost).c_str(),
						i->craft.c_str());
	}
}

/**
 * dTor.
 */
CannotReequipState::~CannotReequipState()
{}

/**
 * Exits to the previous screen.
 * @param action - pointer to an Action
 */
void CannotReequipState::btnOkClick(Action*)
{
	if (_game->getQtyStates() == 2u // ie: (1) this, (2) Geoscape
		&& _game->getResourcePack()->isMusicPlaying(OpenXcom::res_MUSIC_TAC_AWARDS))
	{
		_game->getResourcePack()->fadeMusic(_game, 863);
//		_game->getResourcePack()->playMusic(OpenXcom::res_MUSIC_GEO_GLOBE);
	}
	_game->popState();
}

}
