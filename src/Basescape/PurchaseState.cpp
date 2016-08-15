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

#include "PurchaseState.h"

//#include <algorithm>
#include <iomanip>
//#include <limits>
//#include <map>
//#include <sstream>

#include "../fmath.h"

#include "../Engine/Action.h"
#include "../Engine/Game.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Options.h"
#include "../Engine/Palette.h"
#include "../Engine/Timer.h"

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextList.h"
#include "../Interface/Window.h"

#include "../Menu/ErrorMessageState.h"

#include "../Resource/ResourcePack.h"

//#include "../Ruleset/RuleCraft.h"
#include "../Ruleset/RuleCraftWeapon.h"
#include "../Ruleset/RuleInterface.h"
#include "../Ruleset/RuleItem.h"
#include "../Ruleset/Ruleset.h"
#include "../Ruleset/RuleSoldier.h"

#include "../Savegame/Craft.h"
#include "../Savegame/ItemContainer.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/SoldierDiary.h"
#include "../Savegame/Transfer.h"
#include "../Savegame/Vehicle.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the Purchase/Hire screen.
 * @param base - pointer to the Base to get info from
 */
PurchaseState::PurchaseState(Base* const base)
	:
		_base(base),
		_sel(0u),
		_costTotal(0),
		_qtyPersonnel(0),
		_qtyCraft(0),
		_storeSize(0.)
{
	_window			= new Window(this, 320, 200);

	_txtTitle		= new Text(310, 17,   5, 9);
	_txtBaseLabel	= new Text( 80,  9,  16, 9);
	_txtStorage		= new Text( 85,  9, 219, 9);

	_txtFunds		= new Text(140, 9,  16, 24);
	_txtPurchases	= new Text(140, 9, 160, 24);

	_txtItem		= new Text( 30, 9,  16, 33);
	_txtCost		= new Text(102, 9, 166, 33);
	_txtQuantity	= new Text( 48, 9, 267, 33);

	_lstItems		= new TextList(285, 129, 16, 44);

	_btnCancel		= new TextButton(134, 16,  16, 177);
	_btnOk			= new TextButton(134, 16, 170, 177);

	setInterface("buyMenu");

	const Ruleset* const rules (_game->getRuleset());

	_colorAmmo = static_cast<Uint8>(rules->getInterface("buyMenu")->getElement("ammoColor")->color);

	add(_window,		"window",	"buyMenu");
	add(_txtTitle,		"text",		"buyMenu");
	add(_txtBaseLabel,	"text",		"buyMenu");
	add(_txtFunds,		"text",		"buyMenu");
	add(_txtPurchases,	"text",		"buyMenu");
	add(_txtItem,		"text",		"buyMenu");
	add(_txtStorage,	"text",		"buyMenu");
	add(_txtCost,		"text",		"buyMenu");
	add(_txtQuantity,	"text",		"buyMenu");
	add(_lstItems,		"list",		"buyMenu");
	add(_btnCancel,		"button",	"buyMenu");
	add(_btnOk,			"button",	"buyMenu");

	centerAllSurfaces();


	_window->setBackground(_game->getResourcePack()->getSurface("BACK13.SCR"));

	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick(	static_cast<ActionHandler>(&PurchaseState::btnOkClick));
	_btnOk->onKeyboardPress(static_cast<ActionHandler>(&PurchaseState::btnOkClick),
							Options::keyOk);
	_btnOk->onKeyboardPress(static_cast<ActionHandler>(&PurchaseState::btnOkClick),
							Options::keyOkKeypad);
	_btnOk->setVisible(false);

	_btnCancel->setText(tr("STR_CANCEL"));
	_btnCancel->onMouseClick(	static_cast<ActionHandler>(&PurchaseState::btnCancelClick));
	_btnCancel->onKeyboardPress(static_cast<ActionHandler>(&PurchaseState::btnCancelClick),
								Options::keyCancel);

	_txtTitle->setText(tr("STR_PURCHASE_HIRE_PERSONNEL"));
	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setBig();

	_txtBaseLabel->setText(_base->getLabel());

	_txtFunds->setText(tr("STR_CURRENT_FUNDS_")
						.arg(Text::formatCurrency(_game->getSavedGame()->getFunds())));
	_txtFunds->setSecondaryColor(Palette::blockOffset(13));

	_txtPurchases->setText(tr("STR_COST_OF_PURCHASES_")
						.arg(Text::formatCurrency(_costTotal)));

	_txtItem->setText(tr("STR_ITEM"));

	std::wostringstream woststr;
	woststr << _base->getTotalStores() << L":" << std::fixed << std::setprecision(1) << _base->getUsedStores();
	_txtStorage->setText(woststr.str());
	_txtStorage->setAlign(ALIGN_RIGHT);
	_txtStorage->setColor(WHITE);

	_txtCost->setText(tr("STR_COST_PER_UNIT_UC"));
	_txtQuantity->setText(tr("STR_QUANTITY_UC"));

	_lstItems->setColumns(4, 142,55,46,32);
	_lstItems->setSelectable();
	_lstItems->setBackground(_window);
	_lstItems->setArrow(227, ARROW_VERTICAL);

	_lstItems->onLeftArrowPress(	static_cast<ActionHandler>(&PurchaseState::lstLeftArrowPress));
	_lstItems->onLeftArrowRelease(	static_cast<ActionHandler>(&PurchaseState::lstLeftArrowRelease));

	_lstItems->onRightArrowPress(	static_cast<ActionHandler>(&PurchaseState::lstRightArrowPress));
	_lstItems->onRightArrowRelease(	static_cast<ActionHandler>(&PurchaseState::lstRightArrowRelease));


	// Add soldier-types to purchase-list.
	const RuleSoldier* solRule;
	const std::vector<std::string>& soldierTypes (rules->getSoldiersList());
	for (std::vector<std::string>::const_iterator
			i = soldierTypes.begin();
			i != soldierTypes.end();
			++i)
	{
		solRule = rules->getSoldier(*i);
		if (solRule->getBuyCost() != 0
			&& _game->getSavedGame()->isResearched(solRule->getRequirements()) == true)
		{
			_orderQty.push_back(0);
			_soldiers.push_back(*i);
			_lstItems->addRow(
						4,
						tr(*i).c_str(),
						Text::formatCurrency(rules->getSoldier(*i)->getBuyCost()).c_str(),
						Text::intWide(_base->getSoldierCount(*i)).c_str(),
						L"0");
		}
	}

	_orderQty.push_back(0);
	_lstItems->addRow(
				4,
				tr("STR_SCIENTIST").c_str(),
				Text::formatCurrency(rules->getScientistCost() * 2).c_str(),
				Text::intWide(_base->getTotalScientists()).c_str(),
				L"0");

	_orderQty.push_back(0);
	_lstItems->addRow(
				4,
				tr("STR_ENGINEER").c_str(),
				Text::formatCurrency(rules->getEngineerCost() * 2).c_str(),
				Text::intWide(_base->getTotalEngineers()).c_str(),
				L"0");


	// Add craft-types to purchase-list.
	const RuleCraft* crRule;
	const std::vector<std::string>& craftList (rules->getCraftsList());
	for (std::vector<std::string>::const_iterator
			i = craftList.begin();
			i != craftList.end();
			++i)
	{
		crRule = rules->getCraft(*i);
		if (crRule->getBuyCost() != 0
			&& _game->getSavedGame()->isResearched(crRule->getRequirements()) == true)
		{
			_orderQty.push_back(0);
			_crafts.push_back(*i);
			_lstItems->addRow(
						4,
						tr(*i).c_str(),
						Text::formatCurrency(crRule->getBuyCost()).c_str(),
						Text::intWide(_base->getCraftCount(*i)).c_str(),
						L"0");
		}
	}


//	const std::vector<std::string>& itemList = rules->getItemsList();
	std::vector<std::string> purchaseList (rules->getItemsList()); // Copy, need to remove entries; note that SellState has a more elegant way of handling this ->

	const RuleCraftWeapon* cwRule;
	const RuleItem
		* itRule,
		* laRule,
		* clRule;
	std::string type;
	std::wstring item;
	int
		baseQty,
		clip;

	// Add craft-weapon-types to purchase-list.
	const std::vector<std::string>& cwList (rules->getCraftWeaponsList());
	for (std::vector<std::string>::const_iterator
			i = cwList.begin();
			i != cwList.end();
			++i)
	{
		cwRule = rules->getCraftWeapon(*i);
		laRule = rules->getItemRule(cwRule->getLauncherType());
		type = laRule->getType();

		if (laRule->getBuyCost() != 0) // + isResearched
		{
			_orderQty.push_back(0);
			_items.push_back(type);

			baseQty = _base->getStorageItems()->getItemQuantity(type);
			for (std::vector<Transfer*>::const_iterator
					j = _base->getTransfers()->begin();
					j != _base->getTransfers()->end();
					++j)
			{
				if ((*j)->getTransferType() == PST_ITEM
					&& (*j)->getTransferItems() == type)
				{
					baseQty += (*j)->getQuantity();
				}
			}

			item = tr(type);

			if ((clip = cwRule->getLoadCapacity()) != 0)
				item += (L" (" + Text::intWide(clip) + L")");

			_lstItems->addRow(
							4,
							item.c_str(),
							Text::formatCurrency(laRule->getBuyCost()).c_str(),
							Text::intWide(baseQty).c_str(),
							L"0");

			for (std::vector<std::string>::const_iterator
					j = purchaseList.begin();
					j != purchaseList.end();
					++j)
			{
				if (*j == type)
				{
					purchaseList.erase(j);
					break;
				}
			}
		}

		// Add craft-weapon-load-types to purchase-list.
		clRule = rules->getItemRule(cwRule->getClipType());
		type = clRule->getType();

		if (clRule->getBuyCost() != 0) // clRule != nullptr && // + isResearched
		{
			_orderQty.push_back(0);
			_items.push_back(type);

			baseQty = _base->getStorageItems()->getItemQuantity(type);
			for (std::vector<Transfer*>::const_iterator
					j = _base->getTransfers()->begin();
					j != _base->getTransfers()->end();
					++j)
			{
				if ((*j)->getTransferType() == PST_ITEM
					&& (*j)->getTransferItems() == type)
				{
					baseQty += (*j)->getQuantity();
				}
			}

			item = tr(type);

			if ((clip = clRule->getFullClip()) > 1)
				item += (L"s (" + Text::intWide(clip) + L")");

			item.insert(0u, L"  ");
			_lstItems->addRow(
							4,
							item.c_str(),
							Text::formatCurrency(clRule->getBuyCost()).c_str(),
							Text::intWide(baseQty).c_str(),
							L"0");
			_lstItems->setRowColor(_orderQty.size() - 1u, _colorAmmo);

			for (std::vector<std::string>::const_iterator
					j = purchaseList.begin();
					j != purchaseList.end();
					++j)
			{
				if (*j == type)
				{
					purchaseList.erase(j);
					break;
				}
			}
		}
	}


	// Add items to purchase-list.
	for (std::vector<std::string>::const_iterator
			i = purchaseList.begin();
			i != purchaseList.end();
			++i)
	{
		itRule = rules->getItemRule(*i);
		//Log(LOG_INFO) << (*i) << " list# " << itRule->getListOrder(); // Prints listOrder to LOG.

		if (itRule->getBuyCost() != 0
			&& _game->getSavedGame()->isResearched(itRule->getRequirements()) == true)
		{
			_orderQty.push_back(0);
			_items.push_back(*i);

			baseQty = _base->getStorageItems()->getItemQuantity(*i);
			type = itRule->getType();

			for (std::vector<Transfer*>::const_iterator // add transfer items
					j = _base->getTransfers()->begin();
					j != _base->getTransfers()->end();
					++j)
			{
				if ((*j)->getTransferItems() == type)
					baseQty += (*j)->getQuantity();
			}

			for (std::vector<Craft*>::const_iterator // add craft items & vehicles & vehicle-loads
					j = _base->getCrafts()->begin();
					j != _base->getCrafts()->end();
					++j)
			{
				if ((*j)->getRules()->getSoldierCapacity() != 0) // is transport craft
				{
					for (std::map<std::string, int>::const_iterator
							k = (*j)->getCraftItems()->getContents()->begin();
							k != (*j)->getCraftItems()->getContents()->end();
							++k)
					{
						if (k->first == type)
							baseQty += k->second;
					}
				}

				if ((*j)->getRules()->getVehicleCapacity() != 0) // is transport-craft capable of vehicles
				{
					for (std::vector<Vehicle*>::const_iterator
							k = (*j)->getVehicles()->begin();
							k != (*j)->getVehicles()->end();
							++k)
					{
						if ((*k)->getRules()->getType() == type)
							++baseQty;
						else if ((*k)->getLoad() > 0
							&& (*k)->getRules()->getCompatibleAmmo()->front() == type)
						{
							baseQty += (*k)->getLoad();
						}
					}
				}
			}

			item = tr(*i);

			bool doColor (false);
			if (itRule->getBattleType() == BT_AMMO)		// weapon clips & HWP rounds
//				|| (itRule->getBattleType() == BT_NONE	// craft weapon rounds - ^HANDLED ABOVE^^
//					&& itRule->getClipSize() != 0))
			{
				doColor = true;
				item.insert(0u, L"  ");

				if ((clip = itRule->getFullClip()) > 1
					&& itRule->getType().substr(0u,8u) != "STR_HWP_") // *cuckoo** weapon clips
				{
					item += (L" (" + Text::intWide(clip) + L")");
				}
			}
			else if (itRule->isFixed() == true // tank w/ Ordnance.
				&& (clip = itRule->getFullClip()) > 0)
			{
				item += (L" (" + Text::intWide(clip) + L")");
			}

			_lstItems->addRow(
							4,
							item.c_str(),
							Text::formatCurrency(itRule->getBuyCost()).c_str(),
							Text::intWide(baseQty).c_str(),
							L"0");
			if (doColor == true)
				_lstItems->setRowColor(_orderQty.size() - 1u, _colorAmmo);
		}
	}

	_lstItems->scrollTo(_base->getRecallRow(RCL_PURCHASE));

	_timerInc = new Timer(Timer::SCROLL_SLOW);
	_timerInc->onTimer(static_cast<StateHandler>(&PurchaseState::onIncrease));

	_timerDec = new Timer(Timer::SCROLL_SLOW);
	_timerDec->onTimer(static_cast<StateHandler>(&PurchaseState::onDecrease));
}

/**
 * dTor.
 */
PurchaseState::~PurchaseState()
{
	delete _timerInc;
	delete _timerDec;
}

/**
 * Purchases the selected items.
 * @param action - pointer to an Action
 */
void PurchaseState::btnOkClick(Action*)
{
	_base->setRecallRow(RCL_PURCHASE, _lstItems->getScroll());

	if (_costTotal != 0)
	{
		_game->getSavedGame()->setFunds(_game->getSavedGame()->getFunds() - _costTotal);
		_base->addCashSpent(_costTotal);
	}

	const Ruleset* const rules (_game->getRuleset());

	for (size_t
			sel = 0u;
			sel != _orderQty.size();
			++sel)
	{
		if (_orderQty[sel] != 0)
		{
			Transfer* transfer;

			switch (getPurchaseType(sel))
			{
				case PST_SOLDIER:
					for (int
							i = 0;
							i != _orderQty[sel];
							++i)
					{
						transfer = new Transfer(rules->getPersonnelTime());
						Soldier* const sol (rules->genSoldier(
															_game->getSavedGame(),
															_soldiers[sel]));
						SoldierDiary* const diary (sol->getDiary());
						diary->awardHonoraryMedal();
						transfer->setSoldier(sol);
						_base->getTransfers()->push_back(transfer);
					}
					break;

				case PST_SCIENTIST:
					transfer = new Transfer(rules->getPersonnelTime());
					transfer->setScientists(_orderQty[sel]);
					_base->getTransfers()->push_back(transfer);
					break;

				case PST_ENGINEER:
					transfer = new Transfer(rules->getPersonnelTime());
					transfer->setEngineers(_orderQty[sel]);
					_base->getTransfers()->push_back(transfer);
					break;

				case PST_CRAFT:
					for (int
							i = 0;
							i != _orderQty[sel];
							++i)
					{
						RuleCraft* const crftRule (rules->getCraft(_crafts[getCraftIndex(sel)]));
						transfer = new Transfer(crftRule->getTransferTime());
						Craft* const craft (new Craft(
													crftRule,
													_base,
													_game->getSavedGame(),
													_game->getSavedGame()->getCanonicalId(_crafts[getCraftIndex(sel)])));
						craft->setCraftStatus(CS_REFUELLING);
						transfer->setCraft(craft);
						_base->getTransfers()->push_back(transfer);
					}
					break;

				case PST_ITEM:
				{
					const RuleItem* const itRule (rules->getItemRule(_items[getItemIndex(sel)]));
					transfer = new Transfer(itRule->getTransferTime());
					transfer->setTransferItems(
										_items[getItemIndex(sel)],
										_orderQty[sel]);
					_base->getTransfers()->push_back(transfer);
				}
			}
		}
	}
	_game->popState();
}

/**
 * Exits to the previous screen.
 * @param action - pointer to an Action
 */
void PurchaseState::btnCancelClick(Action*)
{
	_base->setRecallRow(RCL_PURCHASE, _lstItems->getScroll());
	_game->popState();
}

/**
 * Starts increasing the item.
 * @param action - pointer to an Action
 */
void PurchaseState::lstLeftArrowPress(Action* action)
{
	_sel = _lstItems->getSelectedRow();

	switch (action->getDetails()->button.button)
	{
		case SDL_BUTTON_LEFT:
			_error.clear();

			increaseByValue(stepDelta());
			_timerInc->setInterval(Timer::SCROLL_SLOW);
			_timerInc->start();
			break;

		case SDL_BUTTON_RIGHT:
			_error.clear();

			increaseByValue(std::numeric_limits<int>::max());
	}
}

/**
 * Stops increasing the item.
 * @param action - pointer to an Action
 */
void PurchaseState::lstLeftArrowRelease(Action* action)
{
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
		_timerInc->stop();
}

/**
 * Starts decreasing the item.
 * @param action - pointer to an Action
 */
void PurchaseState::lstRightArrowPress(Action* action)
{
	_sel = _lstItems->getSelectedRow();

	switch (action->getDetails()->button.button)
	{
		case SDL_BUTTON_LEFT:
			decreaseByValue(stepDelta());
			_timerDec->setInterval(Timer::SCROLL_SLOW);
			_timerDec->start();
			break;

		case SDL_BUTTON_RIGHT:
			decreaseByValue(std::numeric_limits<int>::max());
	}
}

/**
 * Stops decreasing the item.
 * @param action - pointer to an Action
 */
void PurchaseState::lstRightArrowRelease(Action* action)
{
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
		_timerDec->stop();
}

/**
 * Runs the arrow timers.
 */
void PurchaseState::think()
{
	State::think();

	_timerInc->think(this, nullptr);
	_timerDec->think(this, nullptr);
}

/**
 * Increases the quantity of the selected item to buy by one.
 */
void PurchaseState::onIncrease()
{
	_timerInc->setInterval(Timer::SCROLL_FAST);
	increaseByValue(stepDelta());
}

/**
 * Increases the quantity of the selected item to buy.
 * @param delta - quantity to add
 */
void PurchaseState::increaseByValue(int delta)
{
	if (_error.empty() == false)
		_error.clear();
	else
	{
		if (_costTotal + getPrice() > _game->getSavedGame()->getFunds())
			_error = tr("STR_NOT_ENOUGH_MONEY");
		else
		{
			switch (getPurchaseType(_sel))
			{
				case PST_SOLDIER:
				case PST_SCIENTIST:
				case PST_ENGINEER:
					if (_qtyPersonnel + 1 > _base->getFreeQuarters())
						_error = tr("STR_NOT_ENOUGH_LIVING_SPACE");
					break;

				case PST_CRAFT:
					if (_qtyCraft + 1 > _base->getFreeHangars())
						_error = tr("STR_NO_FREE_HANGARS_FOR_PURCHASE");
					break;

				case PST_ITEM:
					if (_storeSize + _game->getRuleset()->getItemRule(_items[getItemIndex(_sel)])->getStoreSize()
							> static_cast<double>(_base->getTotalStores()) - _base->getUsedStores() + 0.05)
					{
						_error = tr("STR_NOT_ENOUGH_STORE_SPACE");
					}
			}
		}

		if (_error.empty() == false)
		{
			_timerInc->stop();

			const RuleInterface* const uiRule (_game->getRuleset()->getInterface("buyMenu"));
			_game->pushState(new ErrorMessageState(
												_error,
												_palette,
												uiRule->getElement("errorMessage")->color,
												"BACK13.SCR",
												uiRule->getElement("errorPalette")->color));
		}
		else
		{
			delta = std::min(delta,
							(static_cast<int>(_game->getSavedGame()->getFunds()) - _costTotal) / getPrice()); // NOTE: (int)cast renders int64_t useless.

			switch (getPurchaseType(_sel))
			{
				case PST_SOLDIER:
				case PST_SCIENTIST:
				case PST_ENGINEER:
					delta = std::min(delta,
									_base->getFreeQuarters() - _qtyPersonnel);
					_qtyPersonnel += delta;
					break;

				case PST_CRAFT:
					delta = std::min(delta,
									_base->getFreeHangars() - _qtyCraft);
					_qtyCraft += delta;
					break;

				case PST_ITEM:
				{
					const double storeSizePer (_game->getRuleset()->getItemRule(_items[getItemIndex(_sel)])->getStoreSize());
					double allowed;

					if (AreSame(storeSizePer, 0.) == false)
						allowed = (static_cast<double>(_base->getTotalStores()) - _base->getUsedStores() - _storeSize + 0.05) / storeSizePer;
					else
						allowed = std::numeric_limits<double>::max();

					delta = std::min(delta,
									 static_cast<int>(allowed));
					_storeSize += static_cast<double>(delta) * storeSizePer;
				}
			}

			_orderQty[_sel] += delta;
			_costTotal += getPrice() * delta;

			updateListrow();
		}
	}
}

/**
 * Decreases the quantity of the selected item to buy by one.
 */
void PurchaseState::onDecrease()
{
	_timerDec->setInterval(Timer::SCROLL_FAST);
	decreaseByValue(stepDelta());
}

/**
 * Decreases the quantity of the selected item to buy.
 * @param delta - quantity to subtract
 */
void PurchaseState::decreaseByValue(int delta)
{
	if (_orderQty[_sel] > 0)
	{
		delta = std::min(delta, _orderQty[_sel]);

		switch (getPurchaseType(_sel))
		{
			case PST_SOLDIER:
			case PST_SCIENTIST:
			case PST_ENGINEER:
				_qtyPersonnel -= delta;
				break;

			case PST_CRAFT:
				_qtyCraft -= delta;
				break;

			case PST_ITEM:
				_storeSize -= _game->getRuleset()->getItemRule(_items[getItemIndex(_sel)])->getStoreSize() * static_cast<double>(delta);
		}

		_orderQty[_sel] -= delta;
		_costTotal -= getPrice() * delta;

		updateListrow();
	}
}

/**
 * Updates the quantity-strings of the selected item.
 */
void PurchaseState::updateListrow() // private.
{
	_txtPurchases->setText(tr("STR_COST_OF_PURCHASES_")
							.arg(Text::formatCurrency(_costTotal)));

	_lstItems->setCellText(_sel, 3u, Text::intWide(_orderQty[_sel]));

	if (_orderQty[_sel] > 0)
		_lstItems->setRowColor(_sel, _lstItems->getSecondaryColor());
	else
	{
		_lstItems->setRowColor(_sel, _lstItems->getColor());

		if (getPurchaseType(_sel) == PST_ITEM)
		{
			const RuleItem* const itRule (_game->getRuleset()->getItemRule(_items[getItemIndex(_sel)]));
			if (itRule->getBattleType() == BT_AMMO		// ammo for weapon or hwp
				|| (itRule->getBattleType() == BT_NONE	// ammo for craft armament
					&& itRule->getFullClip() != 0))
			{
				_lstItems->setRowColor(_sel, _colorAmmo);
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

	_btnOk->setVisible(_costTotal != 0);
}

/**
 * Gets the price of the currently selected item.
 * @return, the price of the currently selected item
 */
int PurchaseState::getPrice() const // private.
{
	switch (getPurchaseType(_sel))
	{
		case PST_SOLDIER:	return _game->getRuleset()->getSoldier(_soldiers[_sel])->getBuyCost();
		case PST_SCIENTIST:	return _game->getRuleset()->getScientistCost() << 1u;
		case PST_ENGINEER:	return _game->getRuleset()->getEngineerCost() << 1u;
		case PST_CRAFT:		return _game->getRuleset()->getCraft(_crafts[getCraftIndex(_sel)])->getBuyCost();
		case PST_ITEM:		return _game->getRuleset()->getItemRule(_items[getItemIndex(_sel)])->getBuyCost();
	}
	return 0;
}

/**
 * Gets the purchase type.
 * @return, PurchaseSellTransferType (Base.h)
 */
PurchaseSellTransferType PurchaseState::getPurchaseType(size_t sel) const // private.
{
	size_t rowCutoff (_soldiers.size());

	if (sel <  rowCutoff)					return PST_SOLDIER;
	if (sel < (rowCutoff += 1))				return PST_SCIENTIST;
	if (sel < (rowCutoff += 1))				return PST_ENGINEER;
	if (sel < (rowCutoff + _crafts.size()))	return PST_CRAFT;

	return PST_ITEM;
}

/**
 * Gets the index of selected item.
 * @param sel - currently selected item
 * @return, current index
 */
size_t PurchaseState::getItemIndex(size_t sel) const // private.
{
	return sel - _soldiers.size() - _crafts.size() - 2u;
}

/**
 * Gets the index of selected craft.
 * @param sel - currently selected craft
 * @return, current index
 */
size_t PurchaseState::getCraftIndex(size_t sel) const // private.
{
	return sel - _soldiers.size() - 2u;
}

}
