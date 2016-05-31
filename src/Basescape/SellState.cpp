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

//#include <algorithm>
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
		_sel(0u),
		_costTotal(0),
		_hasSci(0u),
		_hasEng(0u),
		_storeSize(0.)
{
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


//	bool overfull = _base->storesOverfull() == true;

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

	_txtTitle->setText(tr("STR_SELL_ITEMS_SACK_PERSONNEL"));
	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setBig();

	_txtBaseLabel->setText(_base->getName());
	_txtSales->setText(tr("STR_VALUE_OF_SALES")
						.arg(Text::formatCurrency(_costTotal)));
	_txtFunds->setText(tr("STR_FUNDS")
						.arg(Text::formatCurrency(_game->getSavedGame()->getFunds())));
	_txtItem->setText(tr("STR_ITEM"));

	std::wostringstream woststr;
	woststr << _base->getTotalStores() << L":" << std::fixed << std::setprecision(1) << _base->getUsedStores();
	_txtStorage->setText(woststr.str());
	_txtStorage->setAlign(ALIGN_RIGHT);
	_txtStorage->setColor(WHITE);

	_txtQuantity->setText(tr("STR_QUANTITY_UC"));
	_txtSell->setText(tr("STR_SELL_SACK"));
	_txtValue->setText(tr("STR_VALUE"));

	_lstItems->setColumns(4, 142,60,22,53);
	_lstItems->setBackground(_window);
	_lstItems->setSelectable();
	_lstItems->setArrowColumn(182, ARROW_VERTICAL);

	_lstItems->onLeftArrowPress(	(ActionHandler)& SellState::lstLeftArrowPress);
	_lstItems->onLeftArrowRelease(	(ActionHandler)& SellState::lstLeftArrowRelease);

	_lstItems->onRightArrowPress(	(ActionHandler)& SellState::lstRightArrowPress);
	_lstItems->onRightArrowRelease(	(ActionHandler)& SellState::lstRightArrowRelease);

//	_lstItems->setAllowScrollOnArrowButtons(!_allowChangeListValuesByMouseWheel);
//	_lstItems->onMousePress((ActionHandler)& SellState::lstMousePress);


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
//							Text::formatCurrency(0).c_str());
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
				wst = Text::formatCurrency(val);
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
		_hasSci = 1u;
		_sellQty.push_back(0);
		_lstItems->addRow(
						4,
						tr("STR_SCIENTIST").c_str(),
						Text::intWide(val).c_str(),
						L"0",L"-");
//						Text::formatCurrency(0).c_str());
	}

	val = _base->getEngineers();
	if (val != 0)
	{
		_hasEng = 1u;
		_sellQty.push_back(0);
		_lstItems->addRow(
						4,
						tr("STR_ENGINEER").c_str(),
						Text::intWide(val).c_str(),
						L"0",L"-");
//						Text::formatCurrency(0).c_str());
	}


	const SavedGame* const gameSave (_game->getSavedGame());
	const Ruleset* const rules (_game->getRuleset());
	const RuleItem
		* itRule,
		* clRule;
	const RuleCraftWeapon* cwRule;

	std::wstring item;

	int
		baseQty,
		clip;

	const std::vector<std::string>& allItems (rules->getItemsList());
	for (std::vector<std::string>::const_iterator
			i = allItems.begin();
			i != allItems.end();
			++i)
	{
		baseQty = _base->getStorageItems()->getItemQuantity(*i);

		if (baseQty != 0
			&& (Options::canSellLiveAliens == true
				|| rules->getItemRule(*i)->isLiveAlien() == false))
		{
			_sellQty.push_back(0);
			_items.push_back(*i);

			item = tr(*i);

			itRule = rules->getItemRule(*i);
			//Log(LOG_INFO) << (*i) << " sell listOrder " << itRule->getListOrder(); // Prints listOrder to LOG.

			bool craftOrdnance (false);
			const std::vector<std::string>& cwList (rules->getCraftWeaponsList());
			for (std::vector<std::string>::const_iterator
					j = cwList.begin();
					j != cwList.end() && craftOrdnance == false;
					++j)
			{
				cwRule = rules->getCraftWeapon(*j);
				if (rules->getItemRule(cwRule->getLauncherType()) == itRule)
				{
					craftOrdnance = true;
					if ((clip = cwRule->getLoadCapacity()) != 0)
						item += (L" (" + Text::intWide(clip) + L")");
				}
				else if ((clRule = rules->getItemRule(cwRule->getClipType())) == itRule)
				{
					craftOrdnance = true;
					if ((clip = clRule->getFullClip()) > 1)
						item += (L"s (" + Text::intWide(clip) + L")");
				}
			}

			Uint8 color;

			if (itRule->getBattleType() == BT_AMMO
				|| (itRule->getBattleType() == BT_NONE && itRule->getFullClip() != 0))
			{
				color = _colorAmmo;
				item.insert(0, L"  ");
				if (itRule->getBattleType() == BT_AMMO
					&& (clip = itRule->getFullClip()) > 1
					&& itRule->getType().substr(0,8) != "STR_HWP_") // *cuckoo** weapon clips
				{
					item += (L" (" + Text::intWide(clip) + L")");
				}
			}
			else
			{
				color = _lstItems->getColor();
                if (itRule->isFixed() == true // tank w/ Ordnance.
					&& (clip = itRule->getFullClip()) > 0)
                {
					item += (L" (" + Text::intWide(clip) + L")");
                }
			}

			if (gameSave->isResearched(itRule->getType()) == false				// not researched or research exempt
				&& (gameSave->isResearched(itRule->getRequirements()) == false	// and has requirements to use but not been researched
					|| rules->getItemRule(*i)->isLiveAlien() == true				// or is an alien
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
							Text::intWide(baseQty).c_str(),
							L"0",
							Text::formatCurrency(itRule->getSellCost()).c_str());
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

	if (_costTotal != 0)
	{
		_game->getSavedGame()->setFunds(_game->getSavedGame()->getFunds() + _costTotal);
		_base->addCashIncome(_costTotal);
	}

	for (size_t
			sel = 0u;
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
							if ((*i)->getArmor()->isBasic() == false)
								_base->getStorageItems()->addItem((*i)->getArmor()->getStoreItem());

							_base->getSoldiers()->erase(i);
							break;
						}
					}

					delete _soldiers[sel];
					break;

				case PST_CRAFT:
				{
					Craft* const craft (_crafts[getCraftIndex(sel)]);
					craft->unloadCraft(_game->getRuleset(), false);

					for (std::vector<Craft*>::const_iterator
							i = _base->getCrafts()->begin();
							i != _base->getCrafts()->end();
							++i)
					{
						if (*i == craft)
						{
							delete *i;
							_base->getCrafts()->erase(i);
							break;
						}
					}
					break;
				}

				case PST_SCIENTIST:
					_base->setScientists(_base->getScientists() - _sellQty[sel]);
					break;

				case PST_ENGINEER:
					_base->setEngineers(_base->getEngineers() - _sellQty[sel]);
					break;

				case PST_ITEM:
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
void SellState::lstLeftArrowPress(Action* action)
{
	_sel = _lstItems->getSelectedRow();

	switch (action->getDetails()->button.button)
	{
		case SDL_BUTTON_RIGHT:
			changeByValue(std::numeric_limits<int>::max(), 1);
			break;

		case SDL_BUTTON_LEFT:
//			if (_timerInc->isRunning() == false)
			{
				if ((SDL_GetModState() & KMOD_CTRL) != 0)
					changeByValue(10,1);
				else
					changeByValue(1,1);

				_timerInc->setInterval(Timer::SCROLL_SLOW);
				_timerInc->start();
			}
	}
}

/**
 * Stops increasing the item.
 * @param action - pointer to an Action
 */
void SellState::lstLeftArrowRelease(Action* action)
{
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
		_timerInc->stop();
}

/**
 * Starts decreasing the item.
 * @param action - pointer to an Action
 */
void SellState::lstRightArrowPress(Action* action)
{
	_sel = _lstItems->getSelectedRow();

	switch (action->getDetails()->button.button)
	{
		case SDL_BUTTON_RIGHT:
			changeByValue(std::numeric_limits<int>::max(), -1);
			break;

		case SDL_BUTTON_LEFT:
//			if (_timerDec->isRunning() == false)
			{
				if ((SDL_GetModState() & KMOD_CTRL) != 0)
					changeByValue(10,-1);
				else
					changeByValue(1,-1);

				_timerDec->setInterval(Timer::SCROLL_SLOW);
				_timerDec->start();
			}
	}
}

/**
 * Stops decreasing the item.
 * @param action - pointer to an Action
 */
void SellState::lstRightArrowRelease(Action* action)
{
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
		_timerDec->stop();
}

/**
 * Gets the price of the currently selected item.
 * @return, price of the selected item
 */
int SellState::getPrice() const // private.
{
	switch (getSellType(_sel))
	{
		case PST_ITEM:
			return _game->getRuleset()->getItemRule(_items[getItemIndex(_sel)])->getSellCost();

		case PST_CRAFT:
			return _crafts[getCraftIndex(_sel)]->getRules()->getSellCost();
	}
	return 0; // soldier, scientist, engineer
}

/**
 * Gets the quantity of the currently selected item at the Base.
 * @return, quantity of selected item
 */
int SellState::getBaseQuantity() const // private.
{
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
			return _base->getStorageItems()->getItemQuantity(_items[getItemIndex(_sel)]);
	}
	return 0;
}

/**
 * Increases the quantity of the selected item to sell by one.
 */
void SellState::increase()
{
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
	if (qtyDelta > 0)
	{
		switch (dir)
		{
			case 1:
				if (_sellQty[_sel] >= getBaseQuantity())
					return;

				qtyDelta = std::min(qtyDelta,
									getBaseQuantity() - _sellQty[_sel]);
				break;

			case -1:
				if (_sellQty[_sel] < 1)
					return;

				qtyDelta = std::min(qtyDelta,
									_sellQty[_sel]);
		}

		_sellQty[_sel] += qtyDelta * dir;
		_costTotal += getPrice() * qtyDelta * dir;

		const RuleItem* itRule;
		switch (getSellType(_sel)) // Calculate the change in storage space.
		{
			case PST_SOLDIER:
				if (_soldiers[_sel]->getArmor()->isBasic() == false)
				{
					itRule = _game->getRuleset()->getItemRule(_soldiers[_sel]->getArmor()->getStoreItem());
					_storeSize += static_cast<double>(dir) * itRule->getStoreSize();
				}
				break;

			case PST_CRAFT:
			{
				double storesReq (0.);
				Craft* const craft (_crafts[getCraftIndex(_sel)]);
				for (std::vector<CraftWeapon*>::const_iterator
						i = craft->getWeapons()->begin();
						i != craft->getWeapons()->end();
						++i)
				{
					if (*i != nullptr)
					{
						itRule = _game->getRuleset()->getItemRule((*i)->getRules()->getLauncherType());
						storesReq += itRule->getStoreSize();

						itRule = _game->getRuleset()->getItemRule((*i)->getRules()->getClipType());
						if (itRule != nullptr)
							storesReq += static_cast<double>((*i)->getClipsLoaded(_game->getRuleset())) * itRule->getStoreSize();
					}
				}
				_storeSize += static_cast<double>(dir) * storesReq;
				break;
			}

			case PST_ITEM:
				itRule = _game->getRuleset()->getItemRule(_items[getItemIndex(_sel)]);
				_storeSize -= static_cast<double>(dir * qtyDelta) * itRule->getStoreSize();
		}

		update();
	}
}

/**
 * Updates the quantity-strings of the selected item.
 */
void SellState::update() // private.
{
	_lstItems->setCellText(_sel, 1u, Text::intWide(getBaseQuantity() - _sellQty[_sel]));
	_lstItems->setCellText(_sel, 2u, Text::intWide(_sellQty[_sel]));

	_txtSales->setText(tr("STR_VALUE_OF_SALES").arg(Text::formatCurrency(_costTotal)));

	Uint8 color;

	if (_sellQty[_sel] > 0)
		color = _lstItems->getSecondaryColor();
	else if (getSellType(_sel) == PST_ITEM)
	{
		const Ruleset* const rules (_game->getRuleset());
		const RuleItem* const itRule (rules->getItemRule(_items[getItemIndex(_sel)]));

		const RuleCraftWeapon* cwRule;

		bool craftOrdnance (false);
		const std::vector<std::string>& cwList (rules->getCraftWeaponsList());
		for (std::vector<std::string>::const_iterator
				i = cwList.begin();
				i != cwList.end();
				++i)
		{
			cwRule = rules->getCraftWeapon(*i);
			if (itRule == rules->getItemRule(cwRule->getLauncherType())
				|| itRule == rules->getItemRule(cwRule->getClipType()))
			{
				craftOrdnance = true;
				break;
			}
		}

		const SavedGame* const gameSave (_game->getSavedGame());
		if (gameSave->isResearched(itRule->getType()) == false				// not researched or is research exempt
			&& (gameSave->isResearched(itRule->getRequirements()) == false	// and has requirements to use but not been researched
				|| itRule->isLiveAlien() == true								// or is an alien
				|| itRule->getBattleType() == BT_CORPSE							// or is a corpse
				|| itRule->getBattleType() == BT_NONE)							// or is not a battlefield item
			&& craftOrdnance == false)										// and is not craft ordnance
		{
			// well, that was !NOT! easy.
			color = YELLOW;
		}
		else if (itRule->getBattleType() == BT_AMMO
			|| (itRule->getBattleType() == BT_NONE
				&& itRule->getFullClip() != 0))
		{
			color = _colorAmmo;
		}
		else
			color = _lstItems->getColor();
	}
	else
		color = _lstItems->getColor();

	_lstItems->setRowColor(_sel, color);


	bool showOk (false);
	switch (_costTotal)
	{
		default:
			showOk = true;
			break;

		case 0:
			for (size_t
					sel = 0u;
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

//	showOk = showOk && _base->storesOverfull(_storeSize) == false;
	_btnOk->setVisible(showOk == true);
}

/**
 * Gets the SellType of the selected item.
 * @param sel - index of currently selected item
 * @return, PurchaseSellTransferType (Base.h)
 */
PurchaseSellTransferType SellState::getSellType(size_t sel) const // private.
{
	size_t rowCutoff (_soldiers.size());

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

/**
 * Handles the mouse-wheels on the arrow-buttons.
 * @param action - pointer to an Action
 *
void SellState::lstMousePress(Action* action)
{
	if (Options::changeValueByMouseWheel < 1)
		return;

	_sel = _lstItems->getSelectedRow();

	if (action->getDetails()->button.button == SDL_BUTTON_WHEELUP)
	{
		_timerInc->stop();
		_timerDec->stop();

		if (static_cast<int>(action->getAbsoluteMouseX()) >= _lstItems->getArrowsLeftEdge()
			&& static_cast<int>(action->getAbsoluteMouseX()) <= _lstItems->getArrowsRightEdge())
		{
			changeByValue(Options::changeValueByMouseWheel, 1);
		}
	}
	else if (action->getDetails()->button.button == SDL_BUTTON_WHEELDOWN)
	{
		_timerInc->stop();
		_timerDec->stop();

		if (static_cast<int>(action->getAbsoluteMouseX()) >= _lstItems->getArrowsLeftEdge()
			&& static_cast<int>(action->getAbsoluteMouseX()) <= _lstItems->getArrowsRightEdge())
		{
			changeByValue(Options::changeValueByMouseWheel, -1);
		}
	}
} */

}
