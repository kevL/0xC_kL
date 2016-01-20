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

#include "SellState.h"

//#include <climits>
//#include <cmath>
#include <iomanip>
//#include <sstream>

#include "../Engine/Action.h"
#include "../Engine/Game.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Options.h"
#include "../Engine/Timer.h"

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextList.h"
#include "../Interface/Window.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/RuleArmor.h"
//#include "../Ruleset/RuleCraft.h"
#include "../Ruleset/RuleCraftWeapon.h"
#include "../Ruleset/RuleInterface.h"
#include "../Ruleset/RuleItem.h"
#include "../Ruleset/Ruleset.h"

#include "../Savegame/BaseFacility.h"
#include "../Savegame/Craft.h"
#include "../Savegame/CraftWeapon.h"
#include "../Savegame/ItemContainer.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/Soldier.h"
//#include "../Savegame/Transfer.h"
#include "../Savegame/Vehicle.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the Sell/Sack screen.
 * @param base		- pointer to the Base to get info from
// * @param origin	- game section that originated this state (default OPT_GEOSCAPE)
 */
SellState::SellState(Base* const base)
//		OptionsOrigin origin)
	:
		_base(base),
//		_origin(origin),
		_sel(0),
		_totalCost(0),
		_hasSci(0),
		_hasEng(0),
		_storeSize(0.)
{
//	bool overfull = Options::storageLimitsEnforced == true
//				 && _base->storesOverfull() == true;

	_window			= new Window(this, 320, 200);

	_txtTitle		= new Text(310, 17,  5, 9);
	_txtBaseLabel	= new Text( 80, 9,  16, 9);
	_txtStorage		= new Text( 85, 9, 219, 9);

	_txtFunds		= new Text(140, 9,  16, 24);
	_txtSales		= new Text(140, 9, 160, 24);

	_txtItem		= new Text(30, 9,  16, 33);
	_txtQuantity	= new Text(54, 9, 166, 33);
	_txtSell		= new Text(20, 9, 226, 33);
	_txtValue		= new Text(40, 9, 248, 33);

	_lstItems		= new TextList(285, 129, 16, 44);

	_btnCancel		= new TextButton(134, 16,  16, 177);
	_btnOk			= new TextButton(134, 16, 170, 177);

	setInterface("sellMenu");

	_colorAmmo = static_cast<Uint8>(_game->getRuleset()->getInterface("sellMenu")->getElement("ammoColor")->color);

	add(_window,		"window",	"sellMenu");
	add(_txtTitle,		"text",		"sellMenu");
	add(_txtBaseLabel,	"text",		"sellMenu");
	add(_txtFunds,		"text",		"sellMenu");
	add(_txtSales,		"text",		"sellMenu");
	add(_txtItem,		"text",		"sellMenu");
	add(_txtStorage,	"text",		"sellMenu");
	add(_txtQuantity,	"text",		"sellMenu");
	add(_txtSell,		"text",		"sellMenu");
	add(_txtValue,		"text",		"sellMenu");
	add(_lstItems,		"list",		"sellMenu");
	add(_btnCancel,		"button",	"sellMenu");
	add(_btnOk,			"button",	"sellMenu");

	centerAllSurfaces();


	_window->setBackground(_game->getResourcePack()->getSurface("BACK13.SCR"));

	_btnOk->setText(tr("STR_SELL_SACK"));
	_btnOk->onMouseClick((ActionHandler)& SellState::btnOkClick);
	_btnOk->onKeyboardPress(
					(ActionHandler)& SellState::btnOkClick,
					Options::keyOk);
	_btnOk->onKeyboardPress(
					(ActionHandler)& SellState::btnOkClick,
					Options::keyOkKeypad);
	_btnOk->setVisible(false);

	_btnCancel->setText(tr("STR_CANCEL"));
	_btnCancel->onMouseClick((ActionHandler)& SellState::btnCancelClick);
	_btnCancel->onKeyboardPress(
					(ActionHandler)& SellState::btnCancelClick,
					Options::keyCancel);
//	if (overfull == true)
//		_btnCancel->setVisible(false);

	_txtTitle->setBig();
	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setText(tr("STR_SELL_ITEMS_SACK_PERSONNEL"));

	_txtBaseLabel->setText(_base->getName(_game->getLanguage()));
	_txtSales->setText(tr("STR_VALUE_OF_SALES")
						.arg(Text::formatFunding(_totalCost)));
	_txtFunds->setText(tr("STR_FUNDS")
						.arg(Text::formatFunding(_game->getSavedGame()->getFunds())));
	_txtItem->setText(tr("STR_ITEM"));

	_txtStorage->setVisible(Options::storageLimitsEnforced);
	_txtStorage->setAlign(ALIGN_RIGHT);
	_txtStorage->setColor(WHITE);
	std::wostringstream woststr;
	woststr << _base->getTotalStores() << L":" << std::fixed << std::setprecision(1) << _base->getUsedStores();
	_txtStorage->setText(woststr.str());

	_txtQuantity->setText(tr("STR_QUANTITY_UC"));
	_txtSell->setText(tr("STR_SELL_SACK"));
	_txtValue->setText(tr("STR_VALUE"));

	_lstItems->setBackground(_window);
	_lstItems->setArrowColumn(182, ARROW_VERTICAL);
	_lstItems->setColumns(4, 142,60,22,53);
	_lstItems->setSelectable();
//	_lstItems->setAllowScrollOnArrowButtons(!_allowChangeListValuesByMouseWheel);
//	_lstItems->onMousePress((ActionHandler)& SellState::lstItemsMousePress);
	_lstItems->onLeftArrowPress((ActionHandler)& SellState::lstItemsLeftArrowPress);
	_lstItems->onLeftArrowRelease((ActionHandler)& SellState::lstItemsLeftArrowRelease);
	_lstItems->onLeftArrowClick((ActionHandler)& SellState::lstItemsLeftArrowClick);
	_lstItems->onRightArrowPress((ActionHandler)& SellState::lstItemsRightArrowPress);
	_lstItems->onRightArrowRelease((ActionHandler)& SellState::lstItemsRightArrowRelease);
	_lstItems->onRightArrowClick((ActionHandler)& SellState::lstItemsRightArrowClick);

	for (std::vector<Soldier*>::const_iterator
			i = _base->getSoldiers()->begin();
			i != _base->getSoldiers()->end();
			++i)
	{
		if ((*i)->getCraft() == nullptr)
		{
			_sellQty.push_back(0);
			_soldiers.push_back(*i);
			_lstItems->addRow(
							4,
							(*i)->getName().c_str(),
							L"1",L"0",L"-");
//							Text::formatFunding(0).c_str());
		}
	}

	int val;

	for (std::vector<Craft*>::const_iterator
			i = _base->getCrafts()->begin();
			i != _base->getCrafts()->end();
			++i)
	{
		if ((*i)->getCraftStatus() != CS_OUT)
		{
			_sellQty.push_back(0);
			_crafts.push_back(*i);
			std::wstring wst;
			val = (*i)->getRules()->getSellCost();
			if (val != 0)
				wst = Text::formatFunding(val);
			else
				wst = L"-";
			_lstItems->addRow(
							4,
							(*i)->getName(_game->getLanguage()).c_str(),
							L"1",L"0",
							wst.c_str());
		}
	}

	val = _base->getScientists();
	if (val != 0)
	{
		_hasSci = 1;
		_sellQty.push_back(0);
		_lstItems->addRow(
						4,
						tr("STR_SCIENTIST").c_str(),
						Text::intWide(val).c_str(),
						L"0",L"-");
//						Text::formatFunding(0).c_str());
	}

	val = _base->getEngineers();
	if (val != 0)
	{
		_hasEng = 1;
		_sellQty.push_back(0);
		_lstItems->addRow(
						4,
						tr("STR_ENGINEER").c_str(),
						Text::intWide(val).c_str(),
						L"0",L"-");
//						Text::formatFunding(0).c_str());
	}


	const SavedGame* const gameSave = _game->getSavedGame();
	const Ruleset* const rules = _game->getRuleset();
	const RuleItem
		* itRule,
		* laRule,
		* clRule;
	const RuleCraftWeapon* cwRule;

	const std::vector<std::string>& itemList = rules->getItemsList();
	for (std::vector<std::string>::const_iterator
			i = itemList.begin();
			i != itemList.end();
			++i)
	{
		const int qty = _base->getStorageItems()->getItemQty(*i);

/*		if (Options::storageLimitsEnforced == true
			&& origin == OPT_BATTLESCAPE)
		{
			for (std::vector<Transfer*>::const_iterator
					j = _base->getTransfers()->begin();
					j != _base->getTransfers()->end();
					++j)
			{
				if ((*j)->getItems() == *i)
					qty += (*j)->getQuantity();
			}

			for (std::vector<Craft*>::const_iterator
					j = _base->getCrafts()->begin();
					j != _base->getCrafts()->end();
					++j)
			{
				qty += (*j)->getItems()->getItem(*i);
			}
		} */

		if (qty != 0
			&& (Options::canSellLiveAliens == true
				|| rules->getItem(*i)->isAlien() == false))
		{
			_sellQty.push_back(0);
			_items.push_back(*i);

			std::wstring item = tr(*i);

			itRule = rules->getItem(*i);
			//Log(LOG_INFO) << (*i) << " sell listOrder " << itRule->getListOrder(); // Prints listOrder to LOG.

			bool craftOrdnance = false;
			const std::vector<std::string>& cwList = rules->getCraftWeaponsList();
			for (std::vector<std::string>::const_iterator
					j = cwList.begin();
					j != cwList.end() && craftOrdnance == false;
					++j)
			{
				// Special handling for treating craft weapons as items
				cwRule = rules->getCraftWeapon(*j);

				laRule = rules->getItem(cwRule->getLauncherItem());
				clRule = rules->getItem(cwRule->getClipItem());

				if (laRule == itRule)
				{
					craftOrdnance = true;

					const int clipSize = cwRule->getAmmoMax(); // Launcher
					if (clipSize != 0)
						item += (L" (" + Text::intWide(clipSize) + L")");
				}
				else if (clRule == itRule)
				{
					craftOrdnance = true;

					const int clipSize = clRule->getClipSize(); // launcher Ammo
					if (clipSize > 1)
						item += (L"s (" + Text::intWide(clipSize) + L")");
				}
			}

			Uint8 color;

			if ((itRule->getBattleType() == BT_AMMO			// #2 - weapon clips & HWP rounds
					|| (itRule->getBattleType() == BT_NONE	// #0 - craft weapon rounds
						&& itRule->getClipSize() != 0))
				&& itRule->getType() != _game->getRuleset()->getAlienFuelType())
			{
				if (itRule->getBattleType() == BT_AMMO
					&& itRule->getType().substr(0,8) != "STR_HWP_") // *cuckoo** weapon clips
				{
					const int clipSize = itRule->getClipSize();
					if (clipSize > 1)
						item += (L" (" + Text::intWide(clipSize) + L")");
				}
				item.insert(0, L"  ");

				color = _colorAmmo;
			}
			else
			{
                if (itRule->isFixed() == true // tank w/ Ordnance.
					&& itRule->getCompatibleAmmo()->empty() == false)
                {
					const RuleItem* const aRule = _game->getRuleset()->getItem(itRule->getCompatibleAmmo()->front());
					const int clipSize = aRule->getClipSize();
					if (clipSize != 0)
						item += (L" (" + Text::intWide(clipSize) + L")");
                }

				color = _lstItems->getColor();
			}

			if (gameSave->isResearched(itRule->getType()) == false				// not researched or research exempt
				&& (gameSave->isResearched(itRule->getRequirements()) == false	// and has requirements to use but not been researched
					|| rules->getItem(*i)->isAlien() == true						// or is an alien
					|| itRule->getBattleType() == BT_CORPSE							// or is a corpse
					|| itRule->getBattleType() == BT_NONE)							// or is not a battlefield item
				&& craftOrdnance == false)										// and is not craft ordnance
			{
				// well, that was !NOT! easy.
				color = YELLOW;
			}

			_lstItems->addRow(
							4,
							item.c_str(),
							Text::intWide(qty).c_str(),
							L"0",
							Text::formatFunding(itRule->getSellCost()).c_str());
			_lstItems->setRowColor(_sellQty.size() - 1, color);
		}
	}

	_lstItems->scrollTo(_base->getRecallRow(REC_SELL));

	_timerInc = new Timer(Timer::SCROLL_SLOW);
	_timerInc->onTimer((StateHandler)& SellState::increase);

	_timerDec = new Timer(Timer::SCROLL_SLOW);
	_timerDec->onTimer((StateHandler)& SellState::decrease);
}

/**
 * dTor.
 */
SellState::~SellState()
{
	delete _timerInc;
	delete _timerDec;
}

/**
 * Runs the arrow timers.
 */
void SellState::think()
{
	State::think();

	_timerInc->think(this, nullptr);
	_timerDec->think(this, nullptr);
}

/**
 * Sells the selected items.
 * @param action - pointer to an Action
 */
void SellState::btnOkClick(Action*)
{
	_base->setRecallRow(REC_SELL, _lstItems->getScroll());

	if (_totalCost != 0)
	{
		_game->getSavedGame()->setFunds(_game->getSavedGame()->getFunds() + _totalCost);
		_base->setCashIncome(_totalCost);
	}

	for (size_t
			sel = 0;
			sel != _sellQty.size();
			++sel)
	{
		if (_sellQty[sel] > 0)
		{
			switch (getSellType(sel))
			{
				case PST_SOLDIER:
					for (std::vector<Soldier*>::const_iterator
							i = _base->getSoldiers()->begin();
							i != _base->getSoldiers()->end();
							++i)
					{
						if (*i == _soldiers[sel])
						{
							if ((*i)->getArmor()->getStoreItem() != "STR_NONE")
								_base->getStorageItems()->addItem((*i)->getArmor()->getStoreItem());

							_base->getSoldiers()->erase(i);
							break;
						}
					}

					delete _soldiers[sel];
				break;

				case PST_CRAFT:
				{
					Craft* const craft = _crafts[getCraftIndex(sel)];

					for (std::vector<CraftWeapon*>::const_iterator // remove weapons from craft
							i = craft->getWeapons()->begin();
							i != craft->getWeapons()->end();
							++i)
					{
						if (*i != nullptr)
						{
							_base->getStorageItems()->addItem((*i)->getRules()->getLauncherItem());
							_base->getStorageItems()->addItem(
														(*i)->getRules()->getClipItem(),
														(*i)->getClipsLoaded(_game->getRuleset()));
						}
					}

					for (std::map<std::string, int>::const_iterator // remove items from craft
							i = craft->getCraftItems()->getContents()->begin();
							i != craft->getCraftItems()->getContents()->end();
							++i)
					{
						_base->getStorageItems()->addItem(i->first, i->second);
					}

					for (std::vector<Vehicle*>::const_iterator // remove vehicles and their ammo from craft
							i = craft->getVehicles()->begin();
							i != craft->getVehicles()->end();
							++i)
					{
						_base->getStorageItems()->addItem((*i)->getRules()->getType());

						if ((*i)->getRules()->getCompatibleAmmo()->empty() == false)
							_base->getStorageItems()->addItem(
														(*i)->getRules()->getCompatibleAmmo()->front(),
														(*i)->getAmmo());
					}

					for (std::vector<Soldier*>::const_iterator // remove soldiers from craft
							i = _base->getSoldiers()->begin();
							i != _base->getSoldiers()->end();
							++i)
					{
						if ((*i)->getCraft() == craft)
							(*i)->setCraft(nullptr);
					}

					for (std::vector<BaseFacility*>::const_iterator // clear craft from hangar
							i = _base->getFacilities()->begin();
							i != _base->getFacilities()->end();
							++i)
					{
						if ((*i)->getCraft() == craft)
						{
							(*i)->setCraft(nullptr);
							break;
						}
					}

					for (std::vector<Craft*>::const_iterator // remove craft from the Base
							i = _base->getCrafts()->begin();
							i != _base->getCrafts()->end();
							++i)
					{
						if (*i == craft)
						{
							_base->getCrafts()->erase(i);
							break;
						}
					}
					delete craft;
				}
				break;

				case PST_SCIENTIST:
					_base->setScientists(_base->getScientists() - _sellQty[sel]);
				break;

				case PST_ENGINEER:
					_base->setEngineers(_base->getEngineers() - _sellQty[sel]);
				break;

				case PST_ITEM:
/*					if (_base->getItems()->getItem(_items[getItemIndex(sel)]) < _sellQty[sel])
					{
						const std::string item = _items[getItemIndex(sel)];
						int toRemove = _sellQty[sel] - _base->getItems()->getItem(item);

						_base->getItems()->removeItem(item, std::numeric_limits<int>::max()); // remove all of said items from base

						// if you still need to remove any, remove them from the crafts first, and keep a running tally // just f'off.
						for (std::vector<Craft*>::const_iterator
								i = _base->getCrafts()->begin();
								i != _base->getCrafts()->end()
									&& toRemove != 0;
								++i)
						{
							if ((*i)->getItems()->getItem(item) < toRemove)
							{
								toRemove -= (*i)->getItems()->getItem(item);
								(*i)->getItems()->removeItem(item, std::numeric_limits<int>::max());
							}
							else
							{
								(*i)->getItems()->removeItem(item, toRemove);
								toRemove = 0;
							}
						}

						// if there are STILL any left to remove, take them from the transfers, and if necessary, delete it.
						for (std::vector<Transfer*>::const_iterator
								i = _base->getTransfers()->begin();
								i != _base->getTransfers()->end()
									&& toRemove != 0;
								)
						{
							if ((*i)->getItems() == item)
							{
								if ((*i)->getQuantity() <= toRemove)
								{
									toRemove -= (*i)->getQuantity();
									delete *i;
									i = _base->getTransfers()->erase(i);
								}
								else
								{
									(*i)->setItems((*i)->getItems(), (*i)->getQuantity() - toRemove);
									toRemove = 0;
								}
							}
							else ++i;
						}
					}
					else */
					_base->getStorageItems()->removeItem(
													_items[getItemIndex(sel)],
													_sellQty[sel]);
			}
		}
	}

	_game->popState();
}

/**
 * Returns to the previous screen.
 * @param action - pointer to an Action
 */
void SellState::btnCancelClick(Action*)
{
	_base->setRecallRow(REC_SELL, _lstItems->getScroll());
	_game->popState();
}

/**
 * Starts increasing the item.
 * @param action - pointer to an Action
 */
void SellState::lstItemsLeftArrowPress(Action* action)
{
	_sel = _lstItems->getSelectedRow();

	if (action->getDetails()->button.button == SDL_BUTTON_LEFT
		&& _timerInc->isRunning() == false)
	{
		_timerInc->start();
	}
}

/**
 * Stops increasing the item.
 * @param action - pointer to an Action
 */
void SellState::lstItemsLeftArrowRelease(Action* action)
{
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
		_timerInc->stop();
}

/**
 * Increases the selected item; by one on left-click, to max on right-click.
 * @param action - pointer to an Action
 */
void SellState::lstItemsLeftArrowClick(Action* action)
{
	if (action->getDetails()->button.button == SDL_BUTTON_RIGHT)
		changeByValue(std::numeric_limits<int>::max(), 1);
	else if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
	{
		if ((SDL_GetModState() & KMOD_CTRL) != 0)
			changeByValue(10,1);
		else
			changeByValue(1,1);

		_timerInc->setInterval(Timer::SCROLL_SLOW);
		_timerDec->setInterval(Timer::SCROLL_SLOW);
	}
}

/**
 * Starts decreasing the item.
 * @param action - pointer to an Action
 */
void SellState::lstItemsRightArrowPress(Action* action)
{
	_sel = _lstItems->getSelectedRow();

	if (action->getDetails()->button.button == SDL_BUTTON_LEFT
		&& _timerDec->isRunning() == false)
	{
		_timerDec->start();
	}
}

/**
 * Stops decreasing the item.
 * @param action - pointer to an Action
 */
void SellState::lstItemsRightArrowRelease(Action* action)
{
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
		_timerDec->stop();
}

/**
 * Decreases the selected item; by one on left-click, to 0 on right-click.
 * @param action - pointer to an Action
 */
void SellState::lstItemsRightArrowClick(Action* action)
{
	if (action->getDetails()->button.button == SDL_BUTTON_RIGHT)
		changeByValue(std::numeric_limits<int>::max(), -1);
	else if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
	{
		if ((SDL_GetModState() & KMOD_CTRL) != 0)
			changeByValue(10,-1);
		else
			changeByValue(1,-1);

		_timerInc->setInterval(Timer::SCROLL_SLOW);
		_timerDec->setInterval(Timer::SCROLL_SLOW);
	}
}

/**
 * Gets the price of the currently selected item.
 * @return, price of the selected item
 */
int SellState::getPrice() // private.
{
	switch (getSellType(_sel))
	{
		case PST_ITEM:
			return _game->getRuleset()->getItem(_items[getItemIndex(_sel)])->getSellCost();

		case PST_CRAFT:
			return _crafts[getCraftIndex(_sel)]->getRules()->getSellCost();
	}

	return 0; // soldier, scientist, engineer
}

/**
 * Gets the quantity of the currently selected item on the base.
 * @return, quantity of selected item on the base
 */
int SellState::getBaseQuantity() // private.
{
//	int qty = 0;
	switch (getSellType(_sel))
	{
		case PST_SOLDIER:
		case PST_CRAFT:
			return 1;

		case PST_SCIENTIST:
			return _base->getScientists();

		case PST_ENGINEER:
			return _base->getEngineers();

		case PST_ITEM:
//			qty = _base->getStorageItems()->getItemQty(_items[getItemIndex(_sel)]);
/*			if (Options::storageLimitsEnforced == true
				&& _origin == OPT_BATTLESCAPE)
			{
				for (std::vector<Transfer*>::const_iterator
						j = _base->getTransfers()->begin();
						j != _base->getTransfers()->end();
						++j)
				{
					if ((*j)->getItems() == _items[getItemIndex(_sel)])
						qty += (*j)->getQuantity();
				}

				for (std::vector<Craft*>::const_iterator
						j = _base->getCrafts()->begin();
						j != _base->getCrafts()->end();
						++j)
				{
					qty += (*j)->getItems()->getItemQty(_items[getItemIndex(_sel)]);
				}
			} */
//			return qty;
			return _base->getStorageItems()->getItemQty(_items[getItemIndex(_sel)]);
	}

	return 0;
}

/**
 * Increases the quantity of the selected item to sell by one.
 */
void SellState::increase()
{
	_timerDec->setInterval(Timer::SCROLL_FAST);
	_timerInc->setInterval(Timer::SCROLL_FAST);

	if ((SDL_GetModState() & KMOD_CTRL) != 0)
		changeByValue(10,1);
	else
		changeByValue(1,1);
}

/**
 * Decreases the quantity of the selected item to sell by one.
 */
void SellState::decrease()
{
	_timerInc->setInterval(Timer::SCROLL_FAST);
	_timerDec->setInterval(Timer::SCROLL_FAST);

	if ((SDL_GetModState() & KMOD_CTRL) != 0)
		changeByValue(10,-1);
	else
		changeByValue(1,-1);
}

/**
 * Increases or decreases the quantity of the selected item to sell.
 * @param qtyDelta	- how much to add or remove
 * @param dir		- direction to change; +1 to increase or -1 to decrease
 */
void SellState::changeByValue(
		int qtyDelta,
		int dir)
{
	if (qtyDelta < 1)
		return;

	if (dir > 0)
	{
		if (_sellQty[_sel] >= getBaseQuantity())
			return;

		qtyDelta = std::min(
						qtyDelta,
						getBaseQuantity() - _sellQty[_sel]);
	}
	else
	{
		if (_sellQty[_sel] < 1)
			return;

		qtyDelta = std::min(
						qtyDelta,
						_sellQty[_sel]);
	}

	_sellQty[_sel] += qtyDelta * dir;
	_totalCost += getPrice() * qtyDelta * dir;

	const RuleItem* itRule;

	switch (getSellType(_sel)) // Calculate the change in storage space.
	{
		case PST_SOLDIER:
			if (_soldiers[_sel]->getArmor()->getStoreItem() != "STR_NONE")
			{
				itRule = _game->getRuleset()->getItem(_soldiers[_sel]->getArmor()->getStoreItem());
				_storeSize += static_cast<double>(dir) * itRule->getSize();
			}
		break;

		case PST_CRAFT:
		{
			double storesReq = 0.;
			Craft* const craft = _crafts[getCraftIndex(_sel)];
			for (std::vector<CraftWeapon*>::const_iterator
					i = craft->getWeapons()->begin();
					i != craft->getWeapons()->end();
					++i)
			{
				if (*i != nullptr)
				{
					itRule = _game->getRuleset()->getItem((*i)->getRules()->getLauncherItem());
					storesReq += itRule->getSize();

					itRule = _game->getRuleset()->getItem((*i)->getRules()->getClipItem());
					if (itRule != nullptr)
						storesReq += static_cast<double>((*i)->getClipsLoaded(_game->getRuleset())) * itRule->getSize();
				}
			}
			_storeSize += static_cast<double>(dir) * storesReq;
		}
		break;

		case PST_ITEM:
			itRule = _game->getRuleset()->getItem(_items[getItemIndex(_sel)]);
			_storeSize -= static_cast<double>(dir * qtyDelta) * itRule->getSize();
	}

	updateItemStrings();
}

/**
 * Updates the quantity-strings of the selected item.
 */
void SellState::updateItemStrings() // private.
{
	_lstItems->setCellText(_sel, 1, Text::intWide(getBaseQuantity() - _sellQty[_sel]));
	_lstItems->setCellText(_sel, 2, Text::intWide(_sellQty[_sel]));

	_txtSales->setText(tr("STR_VALUE_OF_SALES").arg(Text::formatFunding(_totalCost)));

	Uint8 color = _lstItems->getColor();

	if (_sellQty[_sel] > 0)
		color = _lstItems->getSecondaryColor();
	else if (getSellType(_sel) == PST_ITEM)
	{
		const Ruleset* const rules = _game->getRuleset();
		const RuleItem* const itRule = rules->getItem(_items[getItemIndex(_sel)]);

		bool craftOrdnance = false;
		const std::vector<std::string>& cwList = rules->getCraftWeaponsList();
		for (std::vector<std::string>::const_iterator
				i = cwList.begin();
				i != cwList.end();
				++i)
		{
			const RuleCraftWeapon* const cwRule = rules->getCraftWeapon(*i);
			if (itRule == rules->getItem(cwRule->getLauncherItem())
				|| itRule == rules->getItem(cwRule->getClipItem()))
			{
				craftOrdnance = true;
				break;
			}
		}

		const SavedGame* const gameSave = _game->getSavedGame();
		if (gameSave->isResearched(itRule->getType()) == false				// not researched or is research exempt
			&& (gameSave->isResearched(itRule->getRequirements()) == false	// and has requirements to use but not been researched
				|| rules->getItem(itRule->getType())->isAlien() == true			// or is an alien
				|| itRule->getBattleType() == BT_CORPSE							// or is a corpse
				|| itRule->getBattleType() == BT_NONE)							// or is not a battlefield item
			&& craftOrdnance == false)										// and is not craft ordnance
		{
			// well, that was !NOT! easy.
			color = YELLOW;
		}
		else if (itRule->getBattleType() == BT_AMMO
			|| (itRule->getBattleType() == BT_NONE
				&& itRule->getClipSize() != 0))
		{
			color = _colorAmmo;
		}
	}
	_lstItems->setRowColor(_sel, color);


	bool showOk = false;

	if (_totalCost != 0)
		showOk = true;
	else
	{
		for (size_t
				sel = 0;
				sel != _sellQty.size() && showOk == false;
				++sel)
		{
			if (_sellQty[sel] != 0)
			{
				switch (getSellType(sel))
				{
					case PST_CRAFT:
					case PST_SOLDIER:
					case PST_SCIENTIST:
					case PST_ENGINEER:
						showOk = true;
				}
			}
		}
	}

	std::wostringstream woststr;
	woststr << _base->getTotalStores() << L":" << std::fixed << std::setprecision(1) << _base->getUsedStores();
	if (std::abs(_storeSize) > 0.05)
	{
		woststr << L" ";
		if (_storeSize > 0.) woststr << L"+";
		woststr << std::fixed << std::setprecision(1) << _storeSize;
	}
	_txtStorage->setText(woststr.str());

//	if (Options::storageLimitsEnforced == true)
//		showOk = showOk && _base->storesOverfull(_storeSize) == false;
	_btnOk->setVisible(showOk == true);
}

/**
 * Gets the SellType of the selected item.
 * @param sel - index of currently selected item
 * @return, PurchaseSellTransferType (Base.h)
 */
PurchaseSellTransferType SellState::getSellType(size_t sel) const // private.
{
	size_t rowCutoff = _soldiers.size();

	if (sel < rowCutoff)
		return PST_SOLDIER;

	if (sel < (rowCutoff += _crafts.size()))
		return PST_CRAFT;

	if (sel < (rowCutoff += _hasSci))
		return PST_SCIENTIST;

	if (sel < (rowCutoff + _hasEng))
		return PST_ENGINEER;

	return PST_ITEM;
}

/**
 * Gets the index of selected item.
 * @param sel - currently selected item
 * @return, index of the selected item
 */
size_t SellState::getItemIndex(size_t sel) const // private.
{
	return sel
		 - _soldiers.size()
		 - _crafts.size()
		 - _hasSci
		 - _hasEng;
}

/**
 * Gets the index of selected craft.
 * @param sel - selected craft
 * @return, index of the selected craft
 */
size_t SellState::getCraftIndex(size_t sel) const // private.
{
	return sel - _soldiers.size();
}

/*
 * Handles the mouse-wheels on the arrow-buttons.
 * @param action - pointer to an Action
 *
void SellState::lstItemsMousePress(Action* action)
{
	if (Options::changeValueByMouseWheel < 1)
		return;

	_sel = _lstItems->getSelectedRow();

	if (action->getDetails()->button.button == SDL_BUTTON_WHEELUP)
	{
		_timerInc->stop();
		_timerDec->stop();

		if (static_cast<int>(action->getAbsoluteXMouse()) >= _lstItems->getArrowsLeftEdge()
			&& static_cast<int>(action->getAbsoluteXMouse()) <= _lstItems->getArrowsRightEdge())
		{
			changeByValue(Options::changeValueByMouseWheel, 1);
		}
	}
	else if (action->getDetails()->button.button == SDL_BUTTON_WHEELDOWN)
	{
		_timerInc->stop();
		_timerDec->stop();

		if (static_cast<int>(action->getAbsoluteXMouse()) >= _lstItems->getArrowsLeftEdge()
			&& static_cast<int>(action->getAbsoluteXMouse()) <= _lstItems->getArrowsRightEdge())
		{
			changeByValue(Options::changeValueByMouseWheel, -1);
		}
	}
} */

}
