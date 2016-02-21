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

#include "ItemsArrivingState.h"

//#include <algorithm>
//#include <sstream>

#include "GeoscapeState.h"

#include "../Basescape/BasescapeState.h"

#include "../Engine/Action.h"
#include "../Engine/Game.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Options.h"

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextList.h"
#include "../Interface/Window.h"

#include "../Resource/ResourcePack.h"

//#include "../Ruleset/RuleCraft.h"
#include "../Ruleset/RuleCraftWeapon.h"
#include "../Ruleset/RuleItem.h"
#include "../Ruleset/Ruleset.h"

#include "../Savegame/Base.h"
#include "../Savegame/Craft.h"
#include "../Savegame/CraftWeapon.h"
#include "../Savegame/ItemContainer.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/Transfer.h"
#include "../Savegame/Vehicle.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the Items Arriving window.
 * @param state - pointer to the Geoscape state
 */
ItemsArrivingState::ItemsArrivingState(GeoscapeState* const state)
	:
		_state(state)
{
	_fullScreen = false;

	_window			= new Window(this, 320, 184, 0, 8, POPUP_BOTH);
	_txtTitle		= new Text(310, 17, 5, 17);

	_txtItem		= new Text(144, 9,  16, 34);
	_txtQuantity	= new Text( 52, 9, 168, 34);
	_txtDestination	= new Text( 92, 9, 220, 34);

	_lstTransfers	= new TextList(285, 121, 16, 45);

//	_btnGotoBase	= new TextButton(90, 16,  16, 169);
//	_btnOk5Secs		= new TextButton(90, 16, 118, 169);
//	_btnOk			= new TextButton(90, 16, 220, 169);
	_btnOk5Secs		= new TextButton(139, 16,  16, 169);
	_btnOk			= new TextButton(139, 16, 165, 169);

	setInterface("itemsArriving");

	add(_window,			"window",	"itemsArriving");
	add(_txtTitle,			"text1",	"itemsArriving");
	add(_txtItem,			"text1",	"itemsArriving");
	add(_txtQuantity,		"text1",	"itemsArriving");
	add(_txtDestination,	"text1",	"itemsArriving");
	add(_lstTransfers,		"text2",	"itemsArriving");
//	add(_btnGotoBase,		"button",	"itemsArriving");
	add(_btnOk5Secs,		"button",	"itemsArriving");
	add(_btnOk,				"button",	"itemsArriving");

	centerAllSurfaces();


	_window->setBackground(_game->getResourcePack()->getSurface("BACK13.SCR"));

/*	_btnGotoBase->setText(tr("STR_GO_TO_BASE"));
	_btnGotoBase->onMouseClick((ActionHandler)& ItemsArrivingState::btnGotoBaseClick);
	_btnGotoBase->onKeyboardPress(
					(ActionHandler)& ItemsArrivingState::btnGotoBaseClick,
					Options::keyOk); */

	_btnOk5Secs->setText(tr("STR_OK_5_SECONDS"));
	_btnOk5Secs->onMouseClick((ActionHandler)& ItemsArrivingState::btnOk5SecsClick);
	_btnOk5Secs->onKeyboardPress(
					(ActionHandler)& ItemsArrivingState::btnOk5SecsClick,
					Options::keyGeoSpeed1);
	_btnOk5Secs->onKeyboardPress(
					(ActionHandler)& ItemsArrivingState::btnOk5SecsClick,
					Options::keyOk);
	_btnOk5Secs->onKeyboardPress(
					(ActionHandler)& ItemsArrivingState::btnOk5SecsClick,
					Options::keyOkKeypad);

	_btnOk->setText(tr("STR_CANCEL"));
	_btnOk->onMouseClick((ActionHandler)& ItemsArrivingState::btnCancelClick);
	_btnOk->onKeyboardPress(
					(ActionHandler)& ItemsArrivingState::btnCancelClick,
					Options::keyCancel);

	_txtTitle->setText(tr("STR_ITEMS_ARRIVING"));
	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setBig();

	_txtItem->setText(tr("STR_ITEM"));

	_txtQuantity->setText(tr("STR_QUANTITY_UC"));

	_txtDestination->setText(tr("STR_DESTINATION_UC"));

	_lstTransfers->setColumns(3, 144,53,80);
	_lstTransfers->setBackground(_window);
	_lstTransfers->setSelectable();
	_lstTransfers->onMousePress((ActionHandler)& ItemsArrivingState::lstGoToBasePress);

	CraftStatus status;

	for (std::vector<Base*>::const_iterator
			i = _game->getSavedGame()->getBases()->begin();
			i != _game->getSavedGame()->getBases()->end();
			++i)
	{
		for (std::vector<Transfer*>::const_iterator
				j = (*i)->getTransfers()->begin();
				j != (*i)->getTransfers()->end();
				)
		{
			if ((*j)->getHours() == 0)
			{
				_bases.push_back(*i);	// a sequential vector of bases w/ transfers-completed to;
										// it will be matched w/ selectedRow on list-clicks for gotoBase.
				if ((*j)->getTransferType() == PST_ITEM) // check if there's an instant use for item
				{
					const RuleItem* const itRule (_game->getRuleset()->getItem((*j)->getTransferItems()));

					for (std::vector<Craft*>::const_iterator
							k = (*i)->getCrafts()->begin();
							k != (*i)->getCrafts()->end();
							++k)
					{
						status = (*k)->getCraftStatus();
						if (status != CS_OUT)
						{
							if ((*k)->getWarned() == true)
							{
								switch (status)
								{
									case CS_REFUELLING:
										if ((*k)->getRules()->getRefuelItem() == itRule->getType())
											(*k)->setWarned(false);
										break;

									case CS_REARMING:
										for (std::vector<CraftWeapon*>::const_iterator
												l = (*k)->getWeapons()->begin();
												l != (*k)->getWeapons()->end();
												++l)
										{
											if (*l != nullptr
												&& (*l)->getRules()->getClipItem() == itRule->getType())
											{
												(*l)->setCantLoad(false);
												(*k)->setWarned(false);
											}
									}
								}
							}


							for (std::vector<Vehicle*>::const_iterator	// check if it's ammo to reload a vehicle
									l = (*k)->getVehicles()->begin();	// question: what happens to craft returning to base with vehicles that are low on ammo
									l != (*k)->getVehicles()->end();
									++l)
							{
								std::vector<std::string>::const_iterator load (std::find(
																				(*l)->getRules()->getCompatibleAmmo()->begin(),
																				(*l)->getRules()->getCompatibleAmmo()->end(),
																				itRule->getType()));
								if (load != (*l)->getRules()->getCompatibleAmmo()->end()
									&& (*l)->getAmmo() < itRule->getFullClip())
								{
									const int toTank (std::min(
															(*j)->getQuantity(),
															itRule->getFullClip() - (*l)->getAmmo()));
									(*l)->setAmmo((*l)->getAmmo() + toTank);

									// Note that the items have already been delivered in Geoscape->Transfer::advance()
									// so they are removed from the base, not the transfer.
									(*i)->getStorageItems()->removeItem(itRule->getType(), toTank);
								}
							}
						}
					}
				}

				_lstTransfers->addRow(
									3,
									(*j)->getName(_game->getLanguage()).c_str(),
									Text::intWide((*j)->getQuantity()).c_str(),
									(*i)->getName(nullptr).c_str());

				delete *j; // remove transfer
				j = (*i)->getTransfers()->erase(j);
			}
			else
				++j;
		}
	}
}

/**
 * dTor.
 */
ItemsArrivingState::~ItemsArrivingState()
{}

/**
 * Initializes the state.
 */
void ItemsArrivingState::init()
{
	State::init();
	_btnOk5Secs->setVisible(_state->is5Sec() == false);
}

/**
 * Returns to the previous screen.
 * @param action - pointer to an Action
 */
void ItemsArrivingState::btnCancelClick(Action*)
{
	_game->popState();
}

/**
 * Reduces the speed to 5 Secs and returns to the previous screen.
 * @param action - pointer to an Action
 */
void ItemsArrivingState::btnOk5SecsClick(Action*)
{
	_state->resetTimer();
	_game->popState();
}

/**
 * Goes to the base for the respective transfer.
 * @param action - pointer to an Action
 */
/* void ItemsArrivingState::btnGotoBaseClick(Action*)
{
	_state->resetTimer();
	_game->popState();
	_game->pushState(new BasescapeState(_base, _state->getGlobe()));
} */

/**
 * LMB or RMB opens the Basescape for the pressed row.
 * @note Do not pop the state here.
 * @param action - pointer to an Action
 */
void ItemsArrivingState::lstGoToBasePress(Action* action)
{
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
	{
		Base* const base = _bases.at(_lstTransfers->getSelectedRow());
		if (base != nullptr) // make sure player hasn't deconstructed a base while jumping back & forth between bases and the list.
			_game->pushState(new BasescapeState(base, _state->getGlobe()));
	}
}

}
