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

#include "PromotionsState.h"

#include "../Engine/Game.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Options.h"

//#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextList.h"
#include "../Interface/Window.h"

#include "../Resource/XcomResourcePack.h"

#include "../Savegame/Base.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/Soldier.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the Promotions screen.
 */
PromotionsState::PromotionsState()
{
	_window			= new Window(this);

	_txtTitle		= new Text(300, 17, 10, 13);

	_txtName		= new Text(114, 9,  16, 32);
	_txtRank		= new Text( 70, 9, 150, 32);
	_txtBase		= new Text( 80, 9, 220, 32);

	_lstSoldiers	= new TextList(285, 129, 16, 42);

	_btnOk			= new TextButton(288, 16, 16, 177);

	setInterface("promotions");

	add(_window,		"window",	"promotions");
	add(_txtTitle,		"heading",	"promotions");
	add(_txtName,		"text",		"promotions");
	add(_txtRank,		"text",		"promotions");
	add(_txtBase,		"text",		"promotions");
	add(_lstSoldiers,	"list",		"promotions");
	add(_btnOk,			"button",	"promotions");

	centerSurfaces();


	_window->setBackground(_game->getResourcePack()->getSurface("BACK01.SCR"));

	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick(	static_cast<ActionHandler>(&PromotionsState::btnOkClick));
	_btnOk->onKeyboardPress(static_cast<ActionHandler>(&PromotionsState::btnOkClick),
							Options::keyOk);
	_btnOk->onKeyboardPress(static_cast<ActionHandler>(&PromotionsState::btnOkClick),
							Options::keyOkKeypad);
	_btnOk->onKeyboardPress(static_cast<ActionHandler>(&PromotionsState::btnOkClick),
							Options::keyCancel);

	_txtTitle->setText(tr("STR_PROMOTIONS"));
	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setBig();

	_txtName->setText(tr("STR_NAME"));

	_txtRank->setText(tr("STR_NEW_RANK"));

	_txtBase->setText(tr("STR_BASE"));

	_lstSoldiers->setColumns(3, 126,70,80);
	_lstSoldiers->setBackground(_window);
	_lstSoldiers->setSelectable();

	for (std::vector<Base*>::const_iterator
			i = _game->getSavedGame()->getBases()->begin();
			i != _game->getSavedGame()->getBases()->end();
			++i)
	{
		for (std::vector<Soldier*>::const_iterator
				j = (*i)->getSoldiers()->begin();
				j != (*i)->getSoldiers()->end();
				++j)
		{
			if ((*j)->isPromoted() == true)
				_lstSoldiers->addRow(
								3,
								(*j)->getLabel().c_str(),
								tr((*j)->getRankString()).c_str(),
								(*i)->getLabel().c_str());
		}
	}
}

/**
 * dTor.
 */
PromotionsState::~PromotionsState()
{}

/**
 * Exits to the previous screen.
 * @param action - pointer to an Action
 */
void PromotionsState::btnOkClick(Action*)
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
