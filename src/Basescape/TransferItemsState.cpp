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

#include "TransferItemsState.h"

//#include <algorithm>
//#include <limits>
//#include <map>
//#include <sstream>

#include "../fmath.h"

#include "TransferConfirmState.h"

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

#include "../Resource/ResourcePack.h"

//#include "../Ruleset/RuleCraft.h"
#include "../Ruleset/RuleCraftWeapon.h"
#include "../Ruleset/RuleInterface.h"
#include "../Ruleset/RuleItem.h"
#include "../Ruleset/Ruleset.h"

#include "../Savegame/Base.h"
#include "../Savegame/BaseFacility.h"
#include "../Savegame/Craft.h"
#include "../Savegame/ItemContainer.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/Soldier.h"
#include "../Savegame/Vehicle.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the Transfer screen.
 * @param baseSource - pointer to the source Base
 * @param baseTarget - pointer to the target Base
 */
TransferItemsState::TransferItemsState(
		Base* const baseSource,
		Base* const baseTarget)
	:
		_baseSource(baseSource),
		_baseTarget(baseTarget),

		_sel(0u),
		_costTotal(0),

		_qtyPersonnel(0),
		_qtyCraft(0),
		_qtyAlien(0),
		_storeSize(0.),

		_hasSci(0u),
		_hasEng(0u),

		_distance(0.),

		_resetAll(true)
{
	_window			= new Window(this);

	_txtTitle		= new Text(300, 16,  10, 9);
	_txtBaseSource	= new Text( 80,  9,  16, 9);
	_txtBaseTarget	= new Text( 80,  9, 224, 9);

	_txtSpaceSource	= new Text(85, 9,  16, 18);
	_txtSpaceTarget	= new Text(85, 9, 224, 18);

	_txtItem		= new Text(128, 9,  16, 29);
	_txtQuantity	= new Text( 35, 9, 160, 29);
//	_txtTransferQty	= new Text( 46, 9, 200, 29);
	_txtQtyTarget	= new Text( 62, 9, 247, 29);

	_lstItems		= new TextList(285, 137, 16, 39);

	_btnCancel		= new TextButton(134, 16,  16, 177);
	_btnOk			= new TextButton(134, 16, 170, 177);

	setInterface("transferMenu");

	_colorAmmo = static_cast<Uint8>(_game->getRuleset()->getInterface("transferMenu")->getElement("ammoColor")->color);

	add(_window,			"window",	"transferMenu");
	add(_txtTitle,			"text",		"transferMenu");
	add(_txtBaseSource,		"text",		"transferMenu");
	add(_txtBaseTarget,		"text",		"transferMenu");
	add(_txtSpaceSource,	"text",		"transferMenu");
	add(_txtSpaceTarget,	"text",		"transferMenu");
	add(_txtItem,			"text",		"transferMenu");
	add(_txtQuantity,		"text",		"transferMenu");
//	add(_txtTransferQty,	"text",		"transferMenu");
	add(_txtQtyTarget,		"text",		"transferMenu");
	add(_lstItems,			"list",		"transferMenu");
	add(_btnCancel,			"button",	"transferMenu");
	add(_btnOk,				"button",	"transferMenu");

	centerSurfaces();


	_window->setBackground(_game->getResourcePack()->getSurface("BACK13.SCR"));

	_btnOk->setText(tr("STR_TRANSFER"));
	_btnOk->onMouseClick(	static_cast<ActionHandler>(&TransferItemsState::btnOkClick));
	_btnOk->onKeyboardPress(static_cast<ActionHandler>(&TransferItemsState::btnOkClick),
							Options::keyOk);
	_btnOk->onKeyboardPress(static_cast<ActionHandler>(&TransferItemsState::btnOkClick),
							Options::keyOkKeypad);
	_btnOk->setVisible(false);

	_btnCancel->setText(tr("STR_CANCEL"));
	_btnCancel->onMouseClick(	static_cast<ActionHandler>(&TransferItemsState::btnCancelClick));
	_btnCancel->onKeyboardPress(static_cast<ActionHandler>(&TransferItemsState::btnCancelClick),
								Options::keyCancel);

	_txtTitle->setBig();
	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setText(tr("STR_TRANSFER"));

	_txtBaseSource->setText(_baseSource->getLabel());
	_txtBaseTarget->setText(_baseTarget->getLabel());
	_txtBaseTarget->setAlign(ALIGN_RIGHT);

	_txtSpaceSource->setAlign(ALIGN_RIGHT);
	_txtSpaceSource->setColor(WHITE);
	_txtSpaceTarget->setAlign(ALIGN_LEFT);
	_txtSpaceTarget->setColor(WHITE);

	_txtItem->setText(tr("STR_ITEM"));
	_txtQuantity->setText(tr("STR_QUANTITY"));

//	_txtTransferQty->setText(tr("STR_AMOUNT_TO_TRANSFER"));
	_txtQtyTarget->setText(tr("STR_AMOUNT_AT_DESTINATION"));

	_lstItems->setColumns(4, 136,56,31,20);
	_lstItems->setBackground(_window);
	_lstItems->setSelectable();
	_lstItems->setArrow(172, ARROW_VERTICAL);
	_lstItems->onLeftArrowPress(	static_cast<ActionHandler>(&TransferItemsState::lstLeftArrowPress));
	_lstItems->onLeftArrowRelease(	static_cast<ActionHandler>(&TransferItemsState::lstLeftArrowRelease));
	_lstItems->onRightArrowPress(	static_cast<ActionHandler>(&TransferItemsState::lstRightArrowPress));
	_lstItems->onRightArrowRelease(	static_cast<ActionHandler>(&TransferItemsState::lstRightArrowRelease));

	_distance = getDistance();


	_timerInc = new Timer(Timer::SCROLL_SLOW);
	_timerInc->onTimer(static_cast<StateHandler>(&TransferItemsState::onIncrease));

	_timerDec = new Timer(Timer::SCROLL_SLOW);
	_timerDec->onTimer(static_cast<StateHandler>(&TransferItemsState::onDecrease));
}

/**
 * dTor.
 */
TransferItemsState::~TransferItemsState()
{
	delete _timerInc;
	delete _timerDec;
}

/**
 * Initializes the Transfer menu.
 * @note Also called after cancelling TransferConfirmState.
 */
void TransferItemsState::init()
{
	if (_resetAll == false) // update only the storage-space info.
	{
		_resetAll = true;

		_txtSpaceSource->setText(_baseSource->storesDeltaFormat(-_storeSize));
		_txtSpaceTarget->setText(_baseTarget->storesDeltaFormat( _storeSize));
	}
	else
	{
		_lstItems->clearList();

		_baseQty.clear();
		_destQty.clear();
		_transferQty.clear();

		_soldiers.clear();
		_crafts.clear();
		_items.clear();

		_sel =
		_hasSci =
		_hasEng = 0u;
		_costTotal =
		_qtyPersonnel =
		_qtyCraft =
		_qtyAlien = 0;
		_storeSize = 0.;


		_btnOk->setVisible(false);

		for (std::vector<Soldier*>::const_iterator
				i = _baseSource->getSoldiers()->begin();
				i != _baseSource->getSoldiers()->end();
				++i)
		{
			if ((*i)->getCraft() == nullptr)
			{
				_transferQty.push_back(0);
				_baseQty.push_back(1);
				_destQty.push_back(0);
				_soldiers.push_back(*i);

				std::wostringstream woststr;
				woststr << (*i)->getLabel();

				if ((*i)->getSickbay() != 0)
					woststr << L" (" << (*i)->getSickbay() << L" dy)";

				_lstItems->addRow(
								4,
								woststr.str().c_str(),
								L"1",L"0",L"0");
			}
		}

		int val;

		if ((val = _baseSource->getScientists()) != 0)
		{
			_hasSci = 1u;
			_transferQty.push_back(0);
			_baseQty.push_back(val);
			_destQty.push_back(_baseTarget->getTotalScientists());

			_lstItems->addRow(
							4,
							tr("STR_SCIENTIST").c_str(),
							Text::intWide(_baseQty.back()).c_str(),
							L"0",
							Text::intWide(_destQty.back()).c_str());
		}

		if ((val = _baseSource->getEngineers()) != 0)
		{
			_hasEng = 1u;
			_transferQty.push_back(0);
			_baseQty.push_back(val);
			_destQty.push_back(_baseTarget->getTotalEngineers());

			_lstItems->addRow(
							4,
							tr("STR_ENGINEER").c_str(),
							Text::intWide(_baseQty.back()).c_str(),
							L"0",
							Text::intWide(_destQty.back()).c_str());
		}

		for (std::vector<Craft*>::const_iterator
				i = _baseSource->getCrafts()->begin();
				i != _baseSource->getCrafts()->end();
				++i)
		{
			if ((*i)->getCraftStatus() != CS_OUT)
			{
				_transferQty.push_back(0);
				_baseQty.push_back(1);
				_destQty.push_back(0);
				_crafts.push_back(*i);

				_lstItems->addRow(
								4,
								(*i)->getLabel(_game->getLanguage()).c_str(),
								L"1",L"0",L"0");
			}
		}


		const SavedGame* const playSave (_game->getSavedGame());
		const Ruleset* const rules (_game->getRuleset());
		const RuleItem
			* itRule,
			* clRule;
		const RuleCraftWeapon* cwRule;

		const std::vector<std::string>& cwList (rules->getCraftWeaponsList());
		bool craftOrdnance;

		std::string type;
		std::wstring item;

		int
			baseQty,
			destQty,
			clip;

		const std::vector<std::string>& allItems (_game->getRuleset()->getItemsList());
		for (std::vector<std::string>::const_iterator
				i = allItems.begin();
				i != allItems.end();
				++i)
		{
			if ((baseQty = _baseSource->getStorageItems()->getItemQuantity(*i)) != 0)
			{
				_transferQty.push_back(0);
				_baseQty.push_back(baseQty);
				_items.push_back(*i);

				itRule = rules->getItemRule(*i);
				type = itRule->getType();

				destQty = _baseTarget->getStorageItems()->getItemQuantity(*i);

				for (std::vector<Transfer*>::const_iterator // add transfers
						j = _baseTarget->getTransfers()->begin();
						j != _baseTarget->getTransfers()->end();
						++j)
				{
					if ((*j)->getTransferItems() == type)
						destQty += (*j)->getQuantity();
				}

				for (std::vector<Craft*>::const_iterator // add items & vehicles on crafts
						j = _baseTarget->getCrafts()->begin();
						j != _baseTarget->getCrafts()->end();
						++j)
				{
					if ((*j)->getRules()->getSoldierCapacity() != 0) // is transport craft
					{
						for (std::map<std::string, int>::const_iterator // add items on craft
								k = (*j)->getCraftItems()->getContents()->begin();
								k != (*j)->getCraftItems()->getContents()->end();
								++k)
						{
							if (k->first == type)
								destQty += k->second;
						}
					}

					if ((*j)->getRules()->getVehicleCapacity() != 0) // is transport craft capable of vehicles
					{
						for (std::vector<Vehicle*>::const_iterator // add vehicles & vehicle ammo on craft
								k = (*j)->getVehicles()->begin();
								k != (*j)->getVehicles()->end();
								++k)
						{
							if ((*k)->getRules()->getType() == type)
								++destQty;
							else if ((*k)->getLoad() > 0
								&& (*k)->getRules()->getClipTypes()->front() == type)
							{
								destQty += (*k)->getLoad();
							}
						}
					}
				}
				_destQty.push_back(destQty);


				item = tr(*i);

				craftOrdnance = false;
				for (std::vector<std::string>::const_iterator
						j = cwList.begin();
						j != cwList.end();
						++j)
				{
					cwRule = rules->getCraftWeapon(*j);
					if (rules->getItemRule(cwRule->getLauncherType()) == itRule)
					{
						craftOrdnance = true;
						if ((clip = cwRule->getLoadCapacity()) > 0)
							item += (L" (" + Text::intWide(clip) + L")");
						break;
					}

					if ((clRule = rules->getItemRule(cwRule->getClipType())) == itRule)
					{
						craftOrdnance = true;
						if ((clip = clRule->getFullClip()) > 1)
							item += (L"s (" + Text::intWide(clip) + L")");
						break;
					}
				}

				Uint8 color;
				if (    itRule->getBattleType() == BT_AMMO
					|| (itRule->getBattleType() == BT_NONE && itRule->getFullClip() != 0))
				{
					color = _colorAmmo;
					item.insert(0u, L"  ");
					if (itRule->getBattleType() == BT_AMMO
						&& (clip = itRule->getFullClip()) > 1
						&& itRule->getType().substr(0u,8u) != "STR_HWP_") // *cuckoo** weapon clips
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

				if (playSave->isResearched(itRule->getType()) == false					// not researched or is research exempt
					&& (playSave->isResearched(itRule->getRequiredResearch()) == false	// and has requirements to use but not been researched
						|| rules->getItemRule(*i)->isLiveAlien() == true					// or is an alien
						|| itRule->getBattleType() == BT_CORPSE								// or is a corpse
						|| itRule->getBattleType() == BT_NONE)								// or is not a battlefield item
					&& craftOrdnance == false)											// and is not craft ordnance
				{
					// well, that was !NOT! easy.
					color = YELLOW;
				}

				_lstItems->addRow(
								4,
								item.c_str(),
								Text::intWide(baseQty).c_str(),
								L"0",
								Text::intWide(destQty).c_str());
				_lstItems->setRowColor(_baseQty.size() - 1u, color);
			}
		}

		_lstItems->scrollTo(_baseSource->getRecallRow(RCL_TRANSFER));
		_lstItems->draw();

		_txtSpaceSource->setText(_baseSource->storesDeltaFormat());
		_txtSpaceTarget->setText(_baseTarget->storesDeltaFormat());
	}
}

/**
 * Transfers the selected items.
 * @param action - pointer to an Action
 */
void TransferItemsState::btnOkClick(Action*)
{
	_resetAll = false;
	_baseSource->setRecallRow(RCL_TRANSFER, _lstItems->getScroll()); // note that if TransferConfirmState gets cancelled this still takes effect.
	_game->pushState(new TransferConfirmState(_baseTarget, this));
}

/**
 * Exits to the previous screen.
 * @param action - pointer to an Action
 */
void TransferItemsState::btnCancelClick(Action*)
{
	_baseSource->setRecallRow(RCL_TRANSFER, _lstItems->getScroll());
	_game->popState(); // pop main Transfer (this)
//	_game->popState(); // pop choose Destination
}

/**
 * Completes the transfer between Bases.
 * @note Called from TransferConfirmState.
 */
void TransferItemsState::completeTransfer()
{
	_resetAll = true;

	const int eta (static_cast<int>(std::floor(6. + _distance / 10.)));
	_game->getSavedGame()->setFunds(_game->getSavedGame()->getFunds() - _costTotal);
	_baseSource->addCashSpent(_costTotal);

	for (size_t
			sel = 0u;
			sel != _transferQty.size();
			++sel)
	{
		if (_transferQty[sel] != 0)
		{
			Transfer* transfer;

			switch (getTransferType(sel))
			{
				case PST_SOLDIER:
					for (std::vector<Soldier*>::const_iterator
							j = _baseSource->getSoldiers()->begin();
							j != _baseSource->getSoldiers()->end();
							++j)
					{
						if (*j == _soldiers[sel])
						{
							if ((*j)->inPsiTraining() == true)
								(*j)->togglePsiTraining();

							transfer = new Transfer(eta);
							transfer->setSoldier(*j);
							_baseTarget->getTransfers()->push_back(transfer);
							_baseSource->getSoldiers()->erase(j);
							break;
						}
					}
					break;

				case PST_SCIENTIST:
					_baseSource->setScientists(_baseSource->getScientists() - _transferQty[sel]);
					transfer = new Transfer(eta);
					transfer->setScientists(_transferQty[sel]);
					_baseTarget->getTransfers()->push_back(transfer);
					break;

				case PST_ENGINEER:
					_baseSource->setEngineers(_baseSource->getEngineers() - _transferQty[sel]);
					transfer = new Transfer(eta);
					transfer->setEngineers(_transferQty[sel]);
					_baseTarget->getTransfers()->push_back(transfer);
					break;

				case PST_CRAFT:
				{
					Craft* const craft (_crafts[getCraftIndex(sel)]);

					for (std::vector<Craft*>::const_iterator
							j = _baseSource->getCrafts()->begin();
							j != _baseSource->getCrafts()->end();
							++j)
					{
						if (*j == craft)
						{
							craft->unloadCraft(_game->getRuleset());

							transfer = new Transfer(eta);
							transfer->setCraft(*j);
							_baseTarget->getTransfers()->push_back(transfer);

							_baseSource->getCrafts()->erase(j);
							break;
						}
					}
					break;
				}

				case PST_ITEM:
					_baseSource->getStorageItems()->removeItem(
														_items[getItemIndex(sel)],
														_transferQty[sel]);
					transfer = new Transfer(eta);
					transfer->setTransferItems(
											_items[getItemIndex(sel)],
											_transferQty[sel]);
					_baseTarget->getTransfers()->push_back(transfer);
			}
		}
	}
}

/**
 * Starts increasing the item.
 * @param action - pointer to an Action
 */
void TransferItemsState::lstLeftArrowPress(Action* action)
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
void TransferItemsState::lstLeftArrowRelease(Action* action)
{
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
		_timerInc->stop();
}

/**
 * Starts decreasing the item.
 * @param action - pointer to an Action
 */
void TransferItemsState::lstRightArrowPress(Action* action)
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
void TransferItemsState::lstRightArrowRelease(Action* action)
{
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
		_timerDec->stop();
}

/**
 * Runs the arrow timers.
 */
void TransferItemsState::think()
{
	State::think();

	_timerInc->think(this, nullptr);
	_timerDec->think(this, nullptr);
}

/**
 * Increases the quantity of the selected item to transfer by one.
 */
void TransferItemsState::onIncrease()
{
	_timerInc->setInterval(Timer::SCROLL_FAST);
	increaseByValue(stepDelta());
}

/**
 * Increases the quantity of the selected item to transfer.
 * @param delta - quantity to add
 */
void TransferItemsState::increaseByValue(int delta)
{
	if (_error.empty() == false)
		_error.clear();
	else if (_transferQty[_sel] < getSourceQuantity())
	{
		switch (getTransferType(_sel))
		{
			case PST_SOLDIER:
			case PST_SCIENTIST:
			case PST_ENGINEER:
				if (_qtyPersonnel + 1 > _baseTarget->getFreeQuarters())
					_error = tr("STR_NO_FREE_ACCOMMODATION");
				else
				{
					delta = std::min(delta,
									 std::min(_baseTarget->getFreeQuarters() - _qtyPersonnel,
											  getSourceQuantity() - _transferQty[_sel]));
					_qtyPersonnel += delta;
					_baseQty[_sel] -= delta;
					_destQty[_sel] += delta;
					_transferQty[_sel] += delta;
					_costTotal += getCost() * delta;
				}
				break;

			case PST_CRAFT:
				if (_qtyCraft + 1 > _baseTarget->getFreeHangars())
					_error = tr("STR_NO_FREE_HANGARS_FOR_TRANSFER");
				else
				{
					++_qtyCraft;
					--_baseQty[_sel];
					++_destQty[_sel];
					++_transferQty[_sel];
					_costTotal += getCost();
				}
				break;

			case PST_ITEM:
				const RuleItem* const itRule (_game->getRuleset()->getItemRule(_items[getItemIndex(_sel)]));
				if (itRule->isLiveAlien() == false)
				{
					if (_baseTarget->storesOverfull(itRule->getStoreSize() + _storeSize - 0.05))
						_error = tr("STR_NOT_ENOUGH_STORE_SPACE");
					else
					{
						const double storeSizePer (_game->getRuleset()->getItemRule(_items[getItemIndex(_sel)])->getStoreSize());
						double allowed;

						if (AreSame(storeSizePer, 0.) == false)
							allowed = (static_cast<double>(_baseTarget->getTotalStores()) - _baseTarget->getUsedStores() - _storeSize + 0.05)
									/ storeSizePer;
						else
							allowed = std::numeric_limits<double>::max();

						delta = std::min(delta,
										 std::min(static_cast<int>(allowed),
												  getSourceQuantity() - _transferQty[_sel]));
						_storeSize += static_cast<double>(delta) * storeSizePer;
						_baseQty[_sel] -= delta;
						_destQty[_sel] += delta;
						_transferQty[_sel] += delta;
						_costTotal += getCost() * delta;
					}
				}
				else // aLien.
				{
					if (_baseTarget->hasContainment() == false
						|| _qtyAlien + 1 > _baseTarget->getFreeContainment())
					{
						_error = tr("STR_NO_ALIEN_CONTAINMENT_FOR_TRANSFER");
					}
					else
					{
						delta = std::min(delta,
										 std::min(_baseTarget->getFreeContainment() - _qtyAlien,
												  getSourceQuantity() - _transferQty[_sel]));
						_qtyAlien += delta;
						_baseQty[_sel] -= delta;
						_destQty[_sel] += delta;
						_transferQty[_sel] += delta;
						_costTotal += getCost() * delta;
					}
				}
		}

		if (_error.empty() == false)
		{
			_resetAll = false;

			const RuleInterface* const uiRule (_game->getRuleset()->getInterface("transferMenu"));
			_game->pushState(new ErrorMessageState(
												_error,
												_palette,
												uiRule->getElement("errorMessage")->color,
												"BACK13.SCR",
												uiRule->getElement("errorPalette")->color));
		}
		else
			updateListrow();
	}
}

/**
 * Decreases the quantity of the selected item to transfer by one.
 */
void TransferItemsState::onDecrease()
{
	_timerDec->setInterval(Timer::SCROLL_FAST);
	decreaseByValue(stepDelta());
}

/**
 * Decreases the quantity of the selected row to transfer.
 * @param delta - quantity to subtract
 */
void TransferItemsState::decreaseByValue(int delta)
{
	if (_transferQty[_sel] > 0)
	{
		delta = std::min(delta,
						_transferQty[_sel]);

		switch (getTransferType(_sel))
		{
			case PST_SOLDIER:
			case PST_SCIENTIST:
			case PST_ENGINEER:
				_qtyPersonnel -= delta;
				break;

			case PST_CRAFT:
				--_qtyCraft;
				break;

			case PST_ITEM:
			{
				const RuleItem* const itRule (_game->getRuleset()->getItemRule(_items[getItemIndex(_sel)]));
				if (itRule->isLiveAlien() == false)
					_storeSize -= itRule->getStoreSize() * static_cast<double>(delta);
				else
					_qtyAlien -= delta;
			}
		}

		_baseQty[_sel] += delta;
		_destQty[_sel] -= delta;
		_transferQty[_sel] -= delta;
		_costTotal -= getCost() * delta;

		updateListrow();
	}
}

/**
 * Updates the quantity-strings of the selected row.
 */
void TransferItemsState::updateListrow() // private.
{
	_lstItems->setCellText(_sel, 1u, Text::intWide(_baseQty[_sel]));
	_lstItems->setCellText(_sel, 2u, Text::intWide(_transferQty[_sel]));
	_lstItems->setCellText(_sel, 3u, Text::intWide(_destQty[_sel]));

	Uint8 color;

	switch (getTransferType(_sel))
	{
		default:
		case PST_SOLDIER:
		case PST_SCIENTIST:
		case PST_ENGINEER:
		case PST_CRAFT:
			switch (_transferQty[_sel])
			{
				case 0:  color = _lstItems->getColor(); break;
				default: color = _lstItems->getSecondaryColor();
			}
			break;

		case PST_ITEM:
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
				if (   itRule == rules->getItemRule(cwRule->getLauncherType())
					|| itRule == rules->getItemRule(cwRule->getClipType()))
				{
					craftOrdnance = true;
					break;
				}
			}

			const SavedGame* const playSave (_game->getSavedGame());
			if (playSave->isResearched(itRule->getType()) == false					// not researched or is research exempt
				&& (playSave->isResearched(itRule->getRequiredResearch()) == false	// and has requirements to use but not been researched
					|| itRule->isLiveAlien() == true									// or is an alien
					|| itRule->getBattleType() == BT_CORPSE								// or is a corpse
					|| itRule->getBattleType() == BT_NONE)								// or is not a battlefield item
				&& craftOrdnance == false)											// and is not craft ordnance
			{
				// well, that was !NOT! easy.
				color = YELLOW;
			}
			else if (itRule->getBattleType() == BT_AMMO
				||  (itRule->getBattleType() == BT_NONE && itRule->getFullClip() != 0))
			{
				color = _colorAmmo;
			}
			else
				color = _lstItems->getColor();
		}
	}

	_lstItems->setRowColor(_sel, color);

	_txtSpaceSource->setText(_baseSource->storesDeltaFormat(-_storeSize));
	_txtSpaceTarget->setText(_baseTarget->storesDeltaFormat( _storeSize));

	_btnOk->setVisible(_costTotal != 0);
}

/**
 * Gets the total cost of the current Transfer.
 * @return, total cost
 */
int TransferItemsState::getTotalCost() const
{
	return _costTotal;
}

/**
 * Gets the shortest distance between the two Bases.
 * @return, distance
 */
double TransferItemsState::getDistance() const // private.
{
	const double r (51.2); // kL_note: what's this conversion factor is it right
	double
		x[3u],y[3u],z[3u];

	const Base* base (_baseSource);
	for (size_t
			i = 0u;
			i != 2u;
			++i)
	{
		x[i] = r *  std::cos(base->getLatitude()) * std::cos(base->getLongitude());
		y[i] = r *  std::cos(base->getLatitude()) * std::sin(base->getLongitude());
		z[i] = r * -std::sin(base->getLatitude());

		base = _baseTarget;
	}

	x[2u] = x[1u] - x[0u];
	y[2u] = y[1u] - y[0u];
	z[2u] = z[1u] - z[0u];

	return std::sqrt((x[2u] * x[2u]) + (y[2u] * y[2u]) + (z[2u] * z[2u]));
}

/**
 * Gets the transfer cost of the currently selected item.
 * @note All prices increased tenfold.
 * @return, transfer cost
 */
int TransferItemsState::getCost() const // private.
{
	double cost;
	switch (getTransferType(_sel))
	{
		default:
		case PST_SOLDIER:
		case PST_SCIENTIST:
		case PST_ENGINEER: cost = 100.;
			break;

		case PST_CRAFT: cost = 1000.;
			break;

		case PST_ITEM:
			{
				if (_items[getItemIndex(_sel)] == "STR_ALIEN_ALLOYS")
					cost = 0.1;
				else if (_items[getItemIndex(_sel)] == _game->getRuleset()->getAlienFuelType())
					cost = 1.;
				else if (_game->getRuleset()->getItemRule(_items[getItemIndex(_sel)])->isLiveAlien() == true)
					cost = 200.;
				else
					cost = 10.;
			}
	}
	return static_cast<int>(std::ceil(cost * _distance));
}

/**
 * Gets the quantity of the currently selected type on the base.
 * @return, type quantity
 */
int TransferItemsState::getSourceQuantity() const // private.
{
	switch (getTransferType(_sel))
	{
		default:
		case PST_SOLDIER:
		case PST_CRAFT:		return 1;
		case PST_SCIENTIST:	return _baseSource->getScientists();
		case PST_ENGINEER:	return _baseSource->getEngineers();
		case PST_ITEM:
			return _baseSource->getStorageItems()->getItemQuantity(_items[getItemIndex(_sel)]);
	}
}

/**
 * Gets the type of selected Item.
 * @param sel - the selected item
 * @return, PurchaseSellTransferType (Base.h)
 */
PurchaseSellTransferType TransferItemsState::getTransferType(size_t sel) const // private.
{
	size_t rowCutoff (_soldiers.size());

	if (sel <  rowCutoff)						return PST_SOLDIER;
	if (sel < (rowCutoff += _hasSci))			return PST_SCIENTIST;
	if (sel < (rowCutoff += _hasEng))			return PST_ENGINEER;
	if (sel < (rowCutoff +  _crafts.size()))	return PST_CRAFT;

	return PST_ITEM;
}

/**
 * Gets the transfer-index of the currently selected Item.
 * @param sel - selected item
 * @return, transfer-index
 */
size_t TransferItemsState::getItemIndex(size_t sel) const // private.
{
	return sel
		 - _soldiers.size()
		 - _hasSci
		 - _hasEng
		 - _crafts.size();
}

/**
 * Gets the index of selected Craft.
 * @param sel - selected craft
 * @return, index of the selected craft
 */
size_t TransferItemsState::getCraftIndex(size_t sel) const // private.
{
	return sel
		- _soldiers.size()
		- _hasSci
		- _hasEng;
}

}
