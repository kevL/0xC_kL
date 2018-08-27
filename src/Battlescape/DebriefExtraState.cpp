/*
 * Copyright 2010-2018 OpenXcom Developers.
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

#include "DebriefExtraState.h"

#include "../fmath.h"

#include "../Engine/Action.h"
#include "../Engine/Game.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Options.h"
#include "../Engine/Timer.h"

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextList.h"
#include "../Interface/Window.h"

#include "../Menu/ErrorMessageState.h"

#include "../Resource/XcomResourcePack.h"

#include "../Ruleset/RuleInterface.h"
#include "../Ruleset/RuleItem.h"
#include "../Ruleset/Ruleset.h"

#include "../SaveGame/Base.h"
#include "../SaveGame/Craft.h"
#include "../Savegame/ItemContainer.h"
#include "../SaveGame/SavedGame.h"
#include "../Savegame/Transfer.h"
#include "../Savegame/Vehicle.h"


namespace OpenXcom
{

/**
 * Creates the DebriefExtra state.
 * @param base			- pointer to the Base that was in tactical
 * @param operation		- the operation title
 * @param surplus		- a map of pointers to RuleItems & ints for gained items
 * @param lostProperty	- a map of pointers to RuleItems & ints for lost items
 * @param solStatDeltas	- a map of wide-strings & vectors of ints
 */
DebriefExtraState::DebriefExtraState(
		Base* const base,
		std::wstring operation,
		std::map<const RuleItem*, int> surplus,
		std::map<const RuleItem*, int> lostProperty,
		std::map<std::wstring, std::vector<int>> solStatDeltas)
	:
		_base(base),
		_surplus(surplus),
		_lostProperty(lostProperty),
		_solStatDeltas(solStatDeltas),
		_sel(0u),
		_costTotal(0),
		_storeSize(0.)
{
	_window       = new Window(this);

	_txtTitle     = new Text(288, 16,  16, 8);
	_txtBaseLabel = new Text( 80,  9,  16, 8);
	_txtScreen    = new Text( 40,  9, 268, 8);

	_txtStorage   = new Text(80, 9,  16, 26);
	_txtQtyItems  = new Text(55, 9, 186, 26);
	_txtBuyOrSell = new Text(30, 9, 241, 26);
	_txtQtyAtBase = new Text(25, 9, 271, 26);

	_lstGained    = new TextList(285, 137, 16, 36);
	_lstLost      = new TextList(285, 137, 16, 36);
	_lstSolStats  = new TextList(285, 145, 16, 30);

	_txtCash      = new Text(73, 9, 239, 179);

	_btnOk        = new TextButton(136, 16, 92, 177);

	setInterface("debriefing");

	add(_window,       "window",  "debriefing");

	add(_txtTitle,     "heading", "debriefing");
	add(_txtBaseLabel, "text",    "debriefing");
	add(_txtScreen,    "text",    "debriefing");

	add(_txtStorage,   "text",    "debriefing");
	add(_txtQtyItems,  "text",    "debriefing");
	add(_txtBuyOrSell, "text",    "debriefing");
	add(_txtQtyAtBase, "text",    "debriefing");

	add(_lstGained,    "list",    "debriefing");
	add(_lstLost,      "list",    "debriefing");
	add(_lstSolStats,  "list",    "debriefing");

	add(_txtCash,      "text",    "debriefing");

	add(_btnOk,        "button",  "debriefing");

	centerSurfaces();


	_window->setBackground(_game->getResourcePack()->getSurface("BACK01.SCR"));

	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick(   static_cast<ActionHandler>(&DebriefExtraState::btnOkClick));
	_btnOk->onKeyboardPress(static_cast<ActionHandler>(&DebriefExtraState::btnOkClick),
							Options::keyOk);
	_btnOk->onKeyboardPress(static_cast<ActionHandler>(&DebriefExtraState::btnOkClick),
							Options::keyOkKeypad);
	_btnOk->onKeyboardPress(static_cast<ActionHandler>(&DebriefExtraState::btnOkClick),
							Options::keyCancel);

	_txtTitle->setText(operation);
	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setBig();

	_txtBaseLabel->setText(_base->getLabel());
	_txtStorage  ->setText(_base->storesDeltaFormat());
	_txtQtyAtBase->setText(L"base");

	_txtScreen->setAlign(ALIGN_RIGHT);

	if (_surplus.empty() == false)
	{
		_txtScreen   ->setText(L"SURPLUS");
		_txtQtyItems ->setText(L"loot");
		_txtBuyOrSell->setText(L"sell");

		_screen = DES_SURPLUS;

		_lstGained->setColumns(4, 162,55,30,20);
		_lstGained->setBackground(_window);
		_lstGained->setSelectable();

		_lstGained->setArrow(192, ARROW_VERTICAL);

		_lstGained->onLeftArrowPress(   static_cast<ActionHandler>(&DebriefExtraState::lstLeftArrowPress));
		_lstGained->onLeftArrowRelease( static_cast<ActionHandler>(&DebriefExtraState::lstLeftArrowRelease));
		_lstGained->onRightArrowPress(  static_cast<ActionHandler>(&DebriefExtraState::lstRightArrowPress));
		_lstGained->onRightArrowRelease(static_cast<ActionHandler>(&DebriefExtraState::lstRightArrowRelease));

		styleList(_surplus, _lstGained);
	}
	else
	{
		_txtScreen   ->setText(L"LOST");
		_txtQtyItems ->setText(L"lost");
		_txtBuyOrSell->setText(L"buy");

		_screen = DES_LOSTPROPERTY;
	}


	if (_lostProperty.empty() == false)
	{
		_lstLost->setColumns(4, 162,55,30,20);
		_lstLost->setBackground(_window);
		_lstLost->setSelectable();

		_lstLost->setArrow(192, ARROW_VERTICAL);

		_lstLost->onLeftArrowPress(   static_cast<ActionHandler>(&DebriefExtraState::lstLeftArrowPress));
		_lstLost->onLeftArrowRelease( static_cast<ActionHandler>(&DebriefExtraState::lstLeftArrowRelease));
		_lstLost->onRightArrowPress(  static_cast<ActionHandler>(&DebriefExtraState::lstRightArrowPress));
		_lstLost->onRightArrowRelease(static_cast<ActionHandler>(&DebriefExtraState::lstRightArrowRelease));

		styleList(_lostProperty, _lstLost);
	}
	else if (_surplus.empty() == true)
	{
		_txtStorage  ->setVisible(false);
		_txtQtyItems ->setVisible(false);
		_txtBuyOrSell->setVisible(false);
		_txtQtyAtBase->setVisible(false);

		_txtScreen->setText(L"STATS");
		_screen = DES_SOLSTATS;
	}


	_lstSolStats->setColumns(12, 98,17,17,17,17,17,17,17,17,17,17,17);
	_lstSolStats->setBackground(_window);
	_lstSolStats->setSelectable();
	_lstSolStats->setMargin();
	_lstSolStats->setVisible(false);

	if (_solStatDeltas.empty() == false)
		buildSoldierStats();


	_lstGained  ->setVisible(_screen == DES_SURPLUS);
	_lstLost    ->setVisible(_screen == DES_LOSTPROPERTY);
	_lstSolStats->setVisible(_screen == DES_SOLSTATS);

	_txtCash->setVisible(false);


	_timerInc = new Timer(Timer::SCROLL_SLOW);
	_timerInc->onTimer(static_cast<StateHandler>(&DebriefExtraState::onIncrease));

	_timerDec = new Timer(Timer::SCROLL_SLOW);
	_timerDec->onTimer(static_cast<StateHandler>(DebriefExtraState::onDecrease));
}

/**
 * dTor.
 */
DebriefExtraState::~DebriefExtraState()
{
	delete _timerInc;
	delete _timerDec;
}

/**
 * Starts increasing the quantity to buy/sell of a selected item.
 * @param action - pointer to an Action
 */
void DebriefExtraState::lstLeftArrowPress(Action* action) // private.
{
	switch (_screen)
	{
		case DES_SURPLUS:
			_sel = _lstGained->getSelectedRow();
			break;
		case DES_LOSTPROPERTY:
			_sel = _lstLost->getSelectedRow();
	}

	switch (action->getDetails()->button.button)
	{
		case SDL_BUTTON_RIGHT:
			_error.clear();
			increase(std::numeric_limits<int>::max());
			break;

		case SDL_BUTTON_LEFT:
			_error.clear();

			increase(stepDelta());
			_timerInc->setInterval(Timer::SCROLL_SLOW);
			_timerInc->start();
	}
}

/**
 * Stops increasing the quantity to buy/sell of a selected item.
 * @param action - pointer to an Action
 */
void DebriefExtraState::lstLeftArrowRelease(Action* action) // private.
{
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
		_timerInc->stop();
}

/**
 * Starts decreasing the quantity to buy/sell of a selected item.
 * @param action - pointer to an Action
 */
void DebriefExtraState::lstRightArrowPress(Action* action) // private.
{
	switch (_screen)
	{
		case DES_SURPLUS:
			_sel = _lstGained->getSelectedRow();
			break;
		case DES_LOSTPROPERTY:
			_sel = _lstLost->getSelectedRow();
	}

	switch (action->getDetails()->button.button)
	{
		case SDL_BUTTON_RIGHT:
			decrease(std::numeric_limits<int>::max());
			break;

		case SDL_BUTTON_LEFT:
			decrease(stepDelta());
			_timerDec->setInterval(Timer::SCROLL_SLOW);
			_timerDec->start();
	}
}

/**
 * Stops decreasing the quantity to buy/sell of a selected item.
 * @param action - pointer to an Action
 */
void DebriefExtraState::lstRightArrowRelease(Action* action) // private.
{
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
		_timerDec->stop();
}

/**
 * Increases the quantity of the selected item to buy/sell.
 */
void DebriefExtraState::onIncrease() // private.
{
	_timerInc->setInterval(Timer::SCROLL_FAST);
	increase(stepDelta());
}

/**
 * Increases the quantity of the selected item to buy/sell by a specified value.
 * @param delta - quantity to add
 */
void DebriefExtraState::increase(int delta) // private.
{
	switch (_screen)
	{
		case DES_SURPLUS:
		{
			const RuleItem* const itRule (getRule(_surplus));
			if (itRule != nullptr && itRule->getSellCost() != 0 && itRule->isLiveAlien() == false
				&& _qtysSell[_sel] < _surplus[itRule])
			{
				delta = std::min(delta,
								_surplus[itRule] - _qtysSell[_sel]);

				_qtysSell[_sel] += delta;
				_costTotal += itRule->getSellCost() * delta;

				_storeSize -= static_cast<double>(delta) * itRule->getStoreSize();
				updateListrow();
			}
			break;
		}

		case DES_LOSTPROPERTY:
			if (_error.empty() == false)
				_error.clear();
			else
			{
				const RuleItem* const itRule (getRule(_lostProperty));
				if (itRule != nullptr && itRule->getBuyCost() != 0)
				{
					if (_costTotal + itRule->getBuyCost() > _game->getSavedGame()->getFunds())
						_error = tr("STR_NOT_ENOUGH_MONEY");
					else if (_storeSize + itRule->getStoreSize()
							> static_cast<double>(_base->getTotalStores()) - _base->getUsedStores() + 0.05)
					{
						_error = tr("STR_NOT_ENOUGH_STORE_SPACE");
					}

					if (_error.empty() == false)
					{
						_timerInc->stop();

						const RuleInterface* const uiRule (_game->getRuleset()->getInterface("debriefing"));
						_game->pushState(new ErrorMessageState(
															_error,
															_palette,
															uiRule->getElement("errorMessage")->color,
															"BACK13.SCR",
															uiRule->getElement("errorPalette")->color));
					}
					else
					{
						int64_t funds (_game->getSavedGame()->getFunds());
						if (funds > INT_MAX_64)
							funds = INT_MAX_64;

						delta = std::min(delta,
										(static_cast<int>(funds) - _costTotal) / itRule->getBuyCost());

						const double storeSizePer (itRule->getStoreSize());
						double allowed;

						if (AreSame(storeSizePer, 0.) == false)
							allowed = (static_cast<double>(_base->getTotalStores()) - _base->getUsedStores() - _storeSize + 0.05)
									/ storeSizePer;
						else
							allowed = std::numeric_limits<double>::max();

						delta = std::min(delta,
										 static_cast<int>(allowed));

						_qtysBuy[_sel] += delta;
						_costTotal += itRule->getBuyCost() * delta;

						_storeSize += static_cast<double>(delta) * storeSizePer;
						updateListrow();
					}
				}
			}
	}
}

/**
 * Decreases the quantity of the selected item to buy/sell.
 */
void DebriefExtraState::onDecrease() // private.
{
	_timerDec->setInterval(Timer::SCROLL_FAST);
	decrease(stepDelta());
}

/**
 * Decreases the quantity of the selected item to buy/sell by a specified value.
 * @param delta - quantity to subtract
 */
void DebriefExtraState::decrease(int delta) // private.
{
	switch (_screen)
	{
		case DES_SURPLUS:
			if (_qtysSell[_sel] > 0)
			{
				const RuleItem* const itRule (getRule(_surplus));
				if (itRule != nullptr) // safety.
				{
					delta = std::min(delta, _qtysSell[_sel]);

					_qtysSell[_sel] -= delta;
					_costTotal -= itRule->getSellCost() * delta;

					_storeSize += itRule->getStoreSize() * static_cast<double>(delta);
					updateListrow();
				}
			}
			break;

		case DES_LOSTPROPERTY:
			if (_qtysBuy[_sel] > 0)
			{
				const RuleItem* const itRule (getRule(_lostProperty));
				if (itRule != nullptr) // safety.
				{
					delta = std::min(delta, _qtysBuy[_sel]);

					_qtysBuy[_sel] -= delta;
					_costTotal -= itRule->getBuyCost() * delta;

					_storeSize -= itRule->getStoreSize() * static_cast<double>(delta);
					updateListrow();
				}
			}
	}
}

/**
 * Updates the buy/sell quantities on-screen.
 */
void DebriefExtraState::updateListrow() // private.
{
	switch (_screen)
	{
		case DES_SURPLUS:
			_lstGained->setCellText(_sel, 2u, Text::intWide(_qtysSell[_sel]));
			_lstGained->setCellText(_sel, 3u, Text::intWide(_qtysSellBase[_sel] - _qtysSell[_sel]));
			break;

		case DES_LOSTPROPERTY:
			_lstLost->setCellText(_sel, 2u, Text::intWide(_qtysBuy[_sel]));
			_lstLost->setCellText(_sel, 3u, Text::intWide(_qtysBuyBase[_sel] + _qtysBuy[_sel]));
	}

	if (_costTotal != 0)
	{
		std::wstring wst;
		switch (_screen)
		{
			case DES_SURPLUS:      wst = L"+"; break;
			case DES_LOSTPROPERTY: wst = L"-";
		}
		_txtCash->setText(wst + Text::formatCurrency(static_cast<int64_t>(_costTotal)));
		_txtCash->setVisible();
	}
	else
		_txtCash->setVisible(false);

	_txtStorage->setText(_base->storesDeltaFormat(_storeSize));
}

/**
 * Gets the rule for the currently selected item.
 * @param list - reference to the list for the current screen
 * @return, pointer to the RuleItem for the selected row
 */
const RuleItem* DebriefExtraState::getRule(const std::map<const RuleItem*, int>& list) const // private.
{
	size_t j (0u);
	for (std::map<const RuleItem*, int>::const_iterator
			i  = list.begin();
			i != list.end();
			++i, ++j)
	{
		if (j == _sel) return i->first;
	}
	return nullptr;
}

/**
 * Gets the rule for a specified item-type in the Lost list.
 * @param type - reference to the type
 * @return, pointer to the RuleItem
 */
const RuleItem* DebriefExtraState::getRule(const std::string& type) const // private.
{
	for (std::map<const RuleItem*, int>::const_iterator
			i  = _lostProperty.begin();
			i != _lostProperty.end();
			++i)
	{
		if (i->first->getType() == type) return i->first;
	}
	return nullptr;
}

/**
 * Runs the list-arrows' Timers.
 */
void DebriefExtraState::think() // private.
{
	State::think();

	_timerInc->think(this, nullptr);
	_timerDec->think(this, nullptr);
}

/**
 * Advances to the next screen if applicable, handles buying/selling orders,
 * and/or dismisses this State.
 * @param action - pointer to an Action
 */
void DebriefExtraState::btnOkClick(Action*)
{
	switch (_screen)
	{
		case DES_SURPLUS:
			_lstGained->setVisible(false);
			_txtCash  ->setVisible(false);

			if (_costTotal != 0)
			{
				_game->getSavedGame()->setFunds(_game->getSavedGame()->getFunds() + _costTotal);
				_base->addCashIncome(_costTotal);
				_costTotal = 0;

				for (size_t // TODO: Allow selling/discarding items that have zero cost.
						i = 0u;
						i != _qtysSell.size();
						++i)
				{
					if (_qtysSell[i] != 0)
						_base->getStorageItems()->removeItem(
														_typesSell[i],
														_qtysSell[i]);
				}
			}

			if (_lostProperty.empty() == false)
			{
				_screen = DES_LOSTPROPERTY;

				_storeSize = 0.;
				_txtStorage->setText(_base->storesDeltaFormat());

				_txtScreen   ->setText(L"LOST");
				_txtQtyItems ->setText(L"lost");
				_txtBuyOrSell->setText(L"buy");

				_lstLost->setVisible();
				break;
			}
			// no break;

		case DES_LOSTPROPERTY:
			_lstLost     ->setVisible(false);
			_txtCash     ->setVisible(false);
			_txtQtyItems ->setVisible(false);
			_txtBuyOrSell->setVisible(false);
			_txtQtyAtBase->setVisible(false);
			_txtStorage  ->setVisible(false);

			if (_costTotal != 0)
			{
				_game->getSavedGame()->setFunds(_game->getSavedGame()->getFunds() - _costTotal);
				_base->addCashSpent(_costTotal);

				for (size_t // TODO: Allow buying items that have zero cost. IE, set not-purchasable to -1 and use cost=0 for no-cost items.
						i = 0u;
						i != _qtysBuy.size();
						++i)
				{
					if (_qtysBuy[i] != 0)
					{
						Transfer* const transfer (new Transfer(getRule(_typesBuy[i])->getTransferTime()));
						transfer->setTransferItems(
											_typesBuy[i],
											_qtysBuy[i]);
						_base->getTransfers()->push_back(transfer);
					}
				}
			}

			if (_solStatDeltas.empty() == false)
			{
				_screen = DES_SOLSTATS;
				_txtScreen->setText(L"STATS");
				_lstSolStats->setVisible();
				break;
			}
			// no break;

		case DES_SOLSTATS:
			if (_game->getQtyStates() == 2u // ie: (1) this, (2) Geoscape
				&& _game->getResourcePack()->isMusicPlaying(OpenXcom::res_MUSIC_TAC_AWARDS))
			{
				_game->getResourcePack()->fadeMusic(_game, 863);
			}
			_game->popState();
	}
}

/**
 * Builds the soldier-stat screen.
 */
void DebriefExtraState::buildSoldierStats() // private.
{
	_lstSolStats->addRow( // vid. BattleUnit::postMissionProcedures().
					12,
					L"",	L"BR",	L"FR",	L"RA",
					L"ML",	L"PA",	L"PD",	L"TR",
					L"TU",	L"HL",	L"ST",	L"EN");

	size_t r (1u);
	for (std::map<std::wstring, std::vector<int>>::const_iterator
			i  = _solStatDeltas.begin();
			i != _solStatDeltas.end();
			++i, ++r)
	{
		_lstSolStats->addRow(
						12,
						i->first.c_str(),
						i->second[ 0u] ? Text::intWide(i->second[ 0u]).c_str() : L"",
						i->second[ 1u] ? Text::intWide(i->second[ 1u]).c_str() : L"",
						i->second[ 2u] ? Text::intWide(i->second[ 2u]).c_str() : L"",
						i->second[ 3u] ? Text::intWide(i->second[ 3u]).c_str() : L"",
						i->second[ 4u] ? Text::intWide(i->second[ 4u]).c_str() : L"",
						i->second[ 5u] ? Text::intWide(i->second[ 5u]).c_str() : L"",
						i->second[ 6u] ? Text::intWide(i->second[ 6u]).c_str() : L"",
						i->second[ 7u] ? Text::intWide(i->second[ 7u]).c_str() : L"",
						i->second[ 8u] ? Text::intWide(i->second[ 8u]).c_str() : L"",
						i->second[ 9u] ? Text::intWide(i->second[ 9u]).c_str() : L"",
						i->second[10u] ? Text::intWide(i->second[10u]).c_str() : L"");
		_lstSolStats->setRowColor(r, YELLOW);
	}
}

/**
 * Formats mapped input to a TextList.
 * @param input	- reference to the mapped-input of pointers-to-RuleItems & quantities
 * @param list	- pointer to a 3-column TextList to format w/ data
 */
void DebriefExtraState::styleList( // private.
		const std::map<const RuleItem*, int>& input,
		TextList* const list)
{
	std::string type;
	std::wstring
		wst1, // type
		wst2; // quantity to buy/sell
	Uint8 color;
	bool contrast;
	size_t r (0u);

	for (std::map<const RuleItem*, int>::const_iterator
			i  = input.begin();
			i != input.end();
			++i, ++r)
	{
		type = i->first->getType();
		wst1 = tr(type);

		if (i->first->isLiveAlien() == true)
		{
			color = GRAY;
			contrast = true;
		}
		else if (_game->getSavedGame()->isResearched(type) == false								// not researched or is research exempt
			&& (_game->getSavedGame()->isResearched(i->first->getRequiredResearch()) == false	// and has requirements to use that have not been researched
				|| i->first->getBattleType() == BT_CORPSE))											// or is a corpse
		{
			color = GREEN;
			contrast = false;

			if (i->first->getBattleType() == BT_AMMO)
				wst1.insert(0, L"  ");
		}
		else if (i->first->getBattleType() == BT_AMMO)
		{
			color = BROWN;
			contrast = true;
			wst1.insert(0, L"  ");
		}
		else
		{
			color = YELLOW;
			contrast = false;
		}

		if ((list == _lstGained
				&& i->first->getSellCost() != 0 && i->first->isLiveAlien() == false)	// NOTE: Selling live aLiens here can conflict with a/the upcoming
			|| (list == _lstLost														// containment-management screen if containment needs management also.
				&& i->first->getBuyCost() != 0))										// So for now simply disallow live-aLien-sales from this State.
		{
			wst2 = Text::intWide(0);
		}
		else
			wst2 = L"-";


		int baseQty (_base->getStorageItems()->getItemQuantity(type));	// items of 'type' in base-stores

		for (std::vector<Transfer*>::const_iterator						// add transfer-items
				j  = _base->getTransfers()->begin();
				j != _base->getTransfers()->end();
				++j)
		{
			if ((*j)->getTransferItems() == type)
				baseQty += (*j)->getQuantity();
		}

		for (std::vector<Craft*>::const_iterator						// add craft-items & vehicles & vehicle-loads
				j  = _base->getCrafts()->begin();
				j != _base->getCrafts()->end();
				++j)
		{
			if ((*j)->getRules()->getSoldierCapacity() != 0)			// is transport-craft
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

			if ((*j)->getRules()->getVehicleCapacity() != 0)			// is transport-craft capable of vehicles
			{
				for (std::vector<Vehicle*>::const_iterator
						k  = (*j)->getVehicles()->begin();
						k != (*j)->getVehicles()->end();
						++k)
				{
					if ((*k)->getRules()->getType() == type)
						++baseQty;
					else if ((*k)->getLoad() > 0
						&& (*k)->getRules()->getClipTypes()->front() == type)
					{
						baseQty += (*k)->getLoad();
					}
				}
			}
		}

		if (list == _lstGained)
		{
			_qtysSell.push_back(0);
			_typesSell.push_back(type);

			_qtysSellBase.push_back(baseQty);
		}
		else
		{
			_qtysBuy.push_back(0);
			_typesBuy.push_back(type);

			_qtysBuyBase.push_back(baseQty);
		}


		list->addRow(
				4,
				wst1.c_str(),
				Text::intWide(i->second).c_str(),
				wst2.c_str(),
				Text::intWide(baseQty).c_str());
		list->setRowColor(r, color, contrast);
	}
}

}
