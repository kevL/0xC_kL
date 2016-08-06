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

#include "CraftEquipmentState.h"

//#include <algorithm>
//#include <climits>
//#include <sstream>

#include "../Battlescape/BattlescapeGenerator.h"
#include "../Battlescape/InventoryState.h"

#include "../Engine/Action.h"
#include "../Engine/Game.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Options.h"
#include "../Engine/Palette.h"
#include "../Engine/Screen.h"
#include "../Engine/Timer.h"

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextList.h"
#include "../Interface/Window.h"

#include "../Menu/ErrorMessageState.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/RuleArmor.h"
//#include "../Ruleset/RuleCraft.h"
#include "../Ruleset/RuleInterface.h"
#include "../Ruleset/RuleItem.h"
#include "../Ruleset/Ruleset.h"

#include "../Savegame/Base.h"
#include "../Savegame/Craft.h"
#include "../Savegame/ItemContainer.h"
#include "../Savegame/SavedBattleGame.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/Vehicle.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the CraftEquipment screen.
 * @param base		- pointer to the Base to get info from
 * @param craftId	- ID of the selected craft
 */
CraftEquipmentState::CraftEquipmentState(
		Base* const base,
		size_t craftId)
	:
		_base(base),
		_craft(base->getCrafts()->at(craftId)),
		_rules(_game->getRuleset()),
		_sel(0u),
		_selUnitId(0u),
		_recall(0u),
		_isQuickBattle(_game->getSavedGame()->getMonthsPassed() == -1)
{
	_window			= new Window(this, 320, 200);

	_txtTitle		= new Text(300, 17,  16, 8);
	_txtBaseLabel	= new Text( 80,  9, 224, 8);

	_txtSpace		= new Text(110, 9,  16, 26);
	_txtLoad		= new Text(110, 9, 171, 26);

	_txtItem		= new Text(144, 9,  16, 36);
	_txtStores		= new Text( 50, 9, 171, 36);
	_txtCraft		= new Text( 50, 9, 256, 36);

	_lstEquipment	= new TextList(285, 129, 16, 45);

	_btnClear		= new TextButton(94, 16,  16, 177);
	_btnInventory	= new TextButton(94, 16, 113, 177);
	_btnOk			= new TextButton(94, 16, 210, 177);

	setInterface("craftEquipment");

	_ammoColor = static_cast<Uint8>(_rules->getInterface("craftEquipment")->getElement("ammoColor")->color);

	add(_window,		"window",	"craftEquipment");
	add(_txtTitle,		"text",		"craftEquipment");
	add(_txtBaseLabel,	"text",		"craftEquipment");
	add(_txtSpace,		"text",		"craftEquipment");
	add(_txtLoad,		"text",		"craftEquipment");
	add(_txtItem,		"text",		"craftEquipment");
	add(_txtStores,		"text",		"craftEquipment");
	add(_txtCraft,		"text",		"craftEquipment");
	add(_lstEquipment,	"list",		"craftEquipment");
	add(_btnClear,		"button",	"craftEquipment");
	add(_btnInventory,	"button",	"craftEquipment");
	add(_btnOk,			"button",	"craftEquipment");

	if (_isQuickBattle == false)
	{
		_txtCost = new Text(150, 9, 24, -10);
		add(_txtCost, "text", "craftEquipment");
	}

	centerAllSurfaces();


	_window->setBackground(_game->getResourcePack()->getSurface("BACK04.SCR"));

	_btnClear->setText(tr("STR_UNLOAD_CRAFT"));
	_btnClear->onMouseClick(	static_cast<ActionHandler>(&CraftEquipmentState::btnUnloadCraftClick));
	_btnClear->onKeyboardPress(	static_cast<ActionHandler>(&CraftEquipmentState::btnUnloadCraftClick),
								SDLK_u);

	_btnInventory->setText(tr("STR_LOADOUT"));
	_btnInventory->onMouseClick(	static_cast<ActionHandler>(&CraftEquipmentState::btnInventoryClick));
	_btnInventory->onKeyboardPress(	static_cast<ActionHandler>(&CraftEquipmentState::btnInventoryClick),
									SDLK_i);

	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick(	static_cast<ActionHandler>(&CraftEquipmentState::btnOkClick));
	_btnOk->onKeyboardPress(static_cast<ActionHandler>(&CraftEquipmentState::btnOkClick),
							Options::keyCancel);
	_btnOk->onKeyboardPress(static_cast<ActionHandler>(&CraftEquipmentState::btnOkClick),
							Options::keyOk);
	_btnOk->onKeyboardPress(static_cast<ActionHandler>(&CraftEquipmentState::btnOkClick),
							Options::keyOkKeypad);

	_txtTitle->setText(tr("STR_EQUIPMENT_FOR_CRAFT").arg(_craft->getName(_game->getLanguage())));
	_txtTitle->setBig();

	_txtBaseLabel->setAlign(ALIGN_RIGHT);
	_txtBaseLabel->setText(_base->getName());

	_txtItem->setText(tr("STR_ITEM"));
	_txtStores->setText(tr("STR_STORES"));
	_txtCraft->setText(tr("STR_CRAFT"));

	_txtSpace->setText(tr("STR_SPACE_CREW_HWP_FREE_")
						.arg(_craft->getQtySoldiers())
						.arg(_craft->getQtyVehicles())
						.arg(_craft->getSpaceAvailable()));

	_lstEquipment->setColumns(3, 147,85,41);
	_lstEquipment->setBackground(_window);
	_lstEquipment->setSelectable();
	_lstEquipment->setArrow(189, ARROW_HORIZONTAL);

	_lstEquipment->onRightArrowPress(	static_cast<ActionHandler>(&CraftEquipmentState::lstRightArrowPress));
	_lstEquipment->onRightArrowRelease(	static_cast<ActionHandler>(&CraftEquipmentState::lstRightArrowRelease));

	_lstEquipment->onLeftArrowPress(	static_cast<ActionHandler>(&CraftEquipmentState::lstLeftArrowPress));
	_lstEquipment->onLeftArrowRelease(	static_cast<ActionHandler>(&CraftEquipmentState::lstLeftArrowRelease));


	_timerLeft = new Timer(Timer::SCROLL_SLOW);
	_timerLeft->onTimer(static_cast<StateHandler>(&CraftEquipmentState::onLeft));

	_timerRight = new Timer(Timer::SCROLL_SLOW);
	_timerRight->onTimer(static_cast<StateHandler>(&CraftEquipmentState::onRight));
}

/**
 * dTor.
 */
CraftEquipmentState::~CraftEquipmentState()
{
	delete _timerLeft;
	delete _timerRight;
}

/**
 * Resets the stuff when coming back from InventoryState.
 */
void CraftEquipmentState::init()
{
	State::init();

	// Reset stuff when coming back from pre-battle Inventory.
	const SavedBattleGame* const battleSave (_game->getSavedGame()->getBattleSave());
	if (battleSave != nullptr)
	{
		_selUnitId = battleSave->getSelectedUnit()->getBattleOrder();
		_game->getSavedGame()->setBattleSave();
		_craft->setTactical(false);
	}

	updateList();
	showButtons();
	if (_isQuickBattle == false) tacticalCost();
}

/**
 * Updates all values.
 */
void CraftEquipmentState::updateList() // private.
{
	_txtLoad->setText(tr("STR_LOAD_CAPACITY_FREE_")
						.arg(_craft->getLoadCapacity())
						.arg(_craft->getLoadCapacity() - _craft->calcLoadCurrent()));

	_lstEquipment->clearList();

	size_t r (0u);
	std::wostringstream woststr;
	std::wstring wst;
	int
		craftQty,
		clip;
	Uint8 color;

	const RuleItem* itRule;

	const std::vector<std::string>& allItems (_rules->getItemsList());
	for (std::vector<std::string>::const_iterator
			i = allItems.begin();
			i != allItems.end();
			++i)
	{
		itRule = _rules->getItemRule(*i);
		if (itRule->getBigSprite() > -1 // see also BattlescapeGenerator::deployXcom().
			&& itRule->getBattleType() != BT_NONE
			&& itRule->getBattleType() != BT_CORPSE
			&& itRule->getBattleType() != BT_FUEL
			&& _game->getSavedGame()->isResearched(itRule->getRequirements()) == true)
		{
			if (itRule->isFixed() == true)
				craftQty = _craft->getVehicleCount(*i);
			else
				craftQty = _craft->getCraftItems()->getItemQuantity(*i);

			if (_base->getStorageItems()->getItemQuantity(*i) != 0 || craftQty != 0) //|| isQuickBattle == true)
			{
				_items.push_back(*i);

				woststr.str(L"");
				if (_isQuickBattle == false)
					woststr << _base->getStorageItems()->getItemQuantity(*i);
				else
					woststr << "-";

				wst = tr(*i);
				if (itRule->getBattleType() == BT_AMMO) // weapon clips
				{
					wst.insert(0u, L"  ");
					if ((clip = itRule->getFullClip()) > 1)
						wst += (L" (" + Text::intWide(clip) + L")");
				}
				else if (itRule->isFixed() == true // tank w/ Ordnance.
					&& (clip = itRule->getFullClip()) > 0)
				{
					wst += (L" (" + Text::intWide(clip) + L")");
				}

				_lstEquipment->addRow(
									3,
									wst.c_str(),
									woststr.str().c_str(),
									Text::intWide(craftQty).c_str());

				if (craftQty != 0)
					color = _lstEquipment->getSecondaryColor();
				else if (itRule->getBattleType() == BT_AMMO)
					color = _ammoColor;
				else
					color = _lstEquipment->getColor();

				_lstEquipment->setRowColor(r++, color);
			}
		}
	}
	_lstEquipment->scrollTo(_recall);
}

/**
 * Sets current cost to send the Craft out to battle.
 */
void CraftEquipmentState::tacticalCost() const // private.
{
	const int cost (_base->calcSoldierBonuses(_craft)
				  + _craft->getRules()->getSoldierCapacity() * 1000);
	_txtCost->setText(tr("STR_COST_").arg(Text::formatCurrency(cost)));
}

/**
 * Decides whether to show extra buttons - unload-craft and Inventory.
 */
void CraftEquipmentState::showButtons() const // private.
{
	const bool vis (_craft->getCraftItems()->getTotalQuantity() != 0);
	_btnClear->setVisible(vis || _craft->getVehicles()->empty() == false);
	_btnInventory->setVisible(vis && _craft->getQtySoldiers() != 0);
}

/**
 * Starts moving the selected row-items to the Craft.
 * @param action - pointer to an Action
 */
void CraftEquipmentState::lstRightArrowPress(Action* action)
{
	_sel = _lstEquipment->getSelectedRow();

	switch (action->getDetails()->button.button)
	{
		case SDL_BUTTON_LEFT:
			_error.clear();

			rightByValue(1);
			_timerRight->setInterval(Timer::SCROLL_SLOW);
			_timerRight->start();
			break;

		case SDL_BUTTON_RIGHT:
			_error.clear();

			rightByValue(std::numeric_limits<int>::max());
	}
}

/**
 * Stops moving the selected row-items to the Craft.
 * @param action - pointer to an Action
 */
void CraftEquipmentState::lstRightArrowRelease(Action* action)
{
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
		_timerRight->stop();
}

/**
 * Starts moving the selected row-items to the Base.
 * @param action - pointer to an Action
 */
void CraftEquipmentState::lstLeftArrowPress(Action* action)
{
	_sel = _lstEquipment->getSelectedRow();

	switch (action->getDetails()->button.button)
	{
		case SDL_BUTTON_LEFT:
			leftByValue(1);
			_timerLeft->setInterval(Timer::SCROLL_SLOW);
			_timerLeft->start();
			break;

		case SDL_BUTTON_RIGHT:
			leftByValue(std::numeric_limits<int>::max());
	}
}

/**
 * Stops moving the selected row-items to the Base.
 * @param action - pointer to an Action
 */
void CraftEquipmentState::lstLeftArrowRelease(Action* action)
{
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
		_timerLeft->stop();
}

/**
 * Runs the arrow timers.
 */
void CraftEquipmentState::think()
{
	State::think();

	_timerLeft->think(this, nullptr);
	_timerRight->think(this, nullptr);
}

/**
 * Moves the selected row-items to the Craft on Timer ticks.
 */
void CraftEquipmentState::onRight()
{
	_timerRight->setInterval(Timer::SCROLL_FAST);
	rightByValue(1);
}

/**
 * Moves a specified quantity of the selected row-items to the Craft.
 * @param delta - quantity to move right
 */
void CraftEquipmentState::rightByValue(int delta)
{
	if (_error.empty() == false)
		_error.clear();
	else
	{
		int baseQty;
		if (_isQuickBattle == false)
			baseQty = _base->getStorageItems()->getItemQuantity(_items[_sel]);
		else
		{
			if (delta == std::numeric_limits<int>::max())
				delta = 10;

			baseQty = delta;
		}

		if (baseQty != 0)
		{
			bool overLoad (false);
			delta = std::min(delta, baseQty);

			const RuleItem* const itRule (_rules->getItemRule(_items[_sel]));
			if (itRule->isFixed() == true) // load vehicle, convert item to a vehicle
			{
				const int vhclCap (_craft->getRules()->getVehicleCapacity());
				if (vhclCap != 0)
				{
					int quadrants (_rules->getArmor(_rules->getUnitRule(_items[_sel])->getArmorType())->getSize());
					quadrants *= quadrants;

					const int spaceAvailable (std::min(_craft->getSpaceAvailable(),
														vhclCap - _craft->getQtyVehicles(true))
													/ quadrants);

					if (spaceAvailable > 0
						&& _craft->getLoadCapacity() - _craft->calcLoadCurrent() >= quadrants * 10) // note: 10 is the 'load' that a single 'space' uses.
					{
						delta = std::min(delta, spaceAvailable);
						delta = std::min(delta,
										(_craft->getLoadCapacity() - _craft->calcLoadCurrent()) / (quadrants * 10));

						if (itRule->getFullClip() < 1) // no Ammo required.
						{
							for (int
									i = 0;
									i != delta;
									++i)
							{
								_craft->getVehicles()->push_back(new Vehicle(
																		itRule,
																		itRule->getFullClip(),
																		quadrants));
							}

							if (_isQuickBattle == false)
								_base->getStorageItems()->removeItem(_items[_sel], delta);
						}
						else // tank needs Ammo.
						{
							const std::string type (itRule->getCompatibleAmmo()->front());
							const int
								clipsRequired (itRule->getFullClip()),
								baseClips (_base->getStorageItems()->getItemQuantity(type));

							if (_isQuickBattle == false)
								delta = std::min(delta,
												 baseClips / clipsRequired);

							if (delta > 0)
							{
								for (int
										i = 0;
										i != delta;
										++i)
								{
									_craft->getVehicles()->push_back(new Vehicle(
																			itRule,
																			clipsRequired,
																			quadrants));
								}

								if (_isQuickBattle == false)
								{
									_base->getStorageItems()->removeItem(_items[_sel], delta);
									_base->getStorageItems()->removeItem(type, clipsRequired * delta);
								}
							}
							else // not enough Ammo
							{
								_timerRight->stop();

								_error = tr("STR_NOT_ENOUGH_AMMO_TO_ARM_HWP")
																.arg(clipsRequired)
																.arg(tr(type))
																.arg(tr(itRule->getType()));
								_game->pushState(new ErrorMessageState(
																	_error,
																	_palette,
																	COLOR_ERROR,
																	"BACK04.SCR",
																	COLOR_ERROR_BG));
							}
						}
					}
					else
						overLoad = true;
				}
				else
				{
					_error = tr("STR_NO_SUPPORT_UNITS_ALLOWED");
					_game->pushState(new ErrorMessageState(
														_error,
														_palette,
														COLOR_ERROR,
														"BACK04.SCR",
														COLOR_ERROR_BG));
				}
			}
			else if (_craft->getRules()->getItemCapacity() != 0) // load items
			{
				const int loadCur (_craft->calcLoadCurrent());
				if (loadCur + delta > _craft->getLoadCapacity())
				{
					overLoad = true;
					delta = _craft->getLoadCapacity() - loadCur;
				}

				if (delta > 0)
				{
					_craft->getCraftItems()->addItem(_items[_sel], delta);

					if (_isQuickBattle == false)
						_base->getStorageItems()->removeItem(_items[_sel], delta);
				}
			}

			if (overLoad == true)
			{
				_timerRight->stop();

				_error = tr("STR_NO_MORE_EQUIPMENT_ALLOWED", static_cast<unsigned>(_craft->getLoadCapacity()));
				_game->pushState(new ErrorMessageState(
													_error,
													_palette,
													COLOR_ERROR,
													"BACK04.SCR",
													COLOR_ERROR_BG));
			}

			updateListrow();
		}
	}
}

/**
 * Moves the selected row-items to the Base on Timer ticks.
 */
void CraftEquipmentState::onLeft()
{
	_timerLeft->setInterval(Timer::SCROLL_FAST);
	leftByValue(1);
}

/**
 * Moves a specified quantity of selected row-items to the Base.
 * @param delta - quantity to move left
 */
void CraftEquipmentState::leftByValue(int delta)
{
	const RuleItem* const itRule (_rules->getItemRule(_items[_sel]));

	int craftQty;
	if (itRule->isFixed() == true)
		craftQty = _craft->getVehicleCount(_items[_sel]);
	else
		craftQty = _craft->getCraftItems()->getItemQuantity(_items[_sel]);

	if (craftQty != 0)
	{
		delta = std::min(delta, craftQty);

		if (itRule->isFixed() == true) // convert Vehicles to storage-items
		{
			if (_isQuickBattle == false)
			{
				_base->getStorageItems()->addItem(_items[_sel], delta);
				if (itRule->getFullClip() > 0)
					_base->getStorageItems()->addItem(
													itRule->getCompatibleAmmo()->front(),
													itRule->getFullClip() * delta); // Vehicles onboard Craft always have full clips.
			}

			for (std::vector<Vehicle*>::const_iterator
					i = _craft->getVehicles()->begin();
					i != _craft->getVehicles()->end() && delta != 0;
					)
			{
				if ((*i)->getRules() == itRule)
				{
					--delta;
					delete *i;
					i = _craft->getVehicles()->erase(i);
				}
				else
					++i;
			}
		}
		else
		{
			_craft->getCraftItems()->removeItem(_items[_sel], delta);

			if (_isQuickBattle == false)
				_base->getStorageItems()->addItem(_items[_sel], delta);
		}

		updateListrow();
	}
}

/**
 * Updates the displayed quantities of the selected row-item in the list.
 */
void CraftEquipmentState::updateListrow() const // private.
{
	const RuleItem* const itRule (_rules->getItemRule(_items[_sel]));

	int craftQty;
	if (itRule->isFixed() == true)
		craftQty = _craft->getVehicleCount(_items[_sel]);
	else
		craftQty = _craft->getCraftItems()->getItemQuantity(_items[_sel]);

	std::wostringstream woststr;
	if (_isQuickBattle == false)
		woststr << _base->getStorageItems()->getItemQuantity(_items[_sel]);
	else
		woststr << L"-";

	Uint8 color;
	if (craftQty != 0)
		color = _lstEquipment->getSecondaryColor();
	else if (itRule->getBattleType() == BT_AMMO)
		color = _ammoColor;
	else
		color = _lstEquipment->getColor();

	_lstEquipment->setRowColor(_sel, color);
	_lstEquipment->setCellText(_sel, 1u, woststr.str());
	_lstEquipment->setCellText(_sel, 2u, Text::intWide(craftQty));

	_txtSpace->setText(tr("STR_SPACE_CREW_HWP_FREE_")
						.arg(_craft->getQtySoldiers())
						.arg(_craft->getQtyVehicles())
						.arg(_craft->getSpaceAvailable()));
	_txtLoad->setText(tr("STR_LOAD_CAPACITY_FREE_")
						.arg(_craft->getLoadCapacity())
						.arg(_craft->getLoadCapacity() - _craft->calcLoadCurrent()));
	showButtons();
	if (_isQuickBattle == false) tacticalCost();
}

/**
 * Clears the contents of the Craft - moves all items back to stores.
 * @param action - pointer to an Action
 */
void CraftEquipmentState::btnUnloadCraftClick(Action*) // private.
{
	for (
			_sel = 0u;
			_sel != _items.size();
			++_sel)
	{
		leftByValue(std::numeric_limits<int>::max());
	}
}

/**
 * Displays the inventory for any Soldiers in the Craft.
 * @param action - pointer to an Action
 */
void CraftEquipmentState::btnInventoryClick(Action*) // private.
{
	_recall = _lstEquipment->getScroll();

	SavedBattleGame* const battleSave (new SavedBattleGame());
	_game->getSavedGame()->setBattleSave(battleSave);

	BattlescapeGenerator bGen = BattlescapeGenerator(_game);
	bGen.runFakeInventory(_craft, nullptr, _selUnitId);

	_game->getScreen()->clear();
	_game->pushState(new InventoryState());
}

/**
 * Exits to the previous screen.
 * @param action - pointer to an Action
 */
void CraftEquipmentState::btnOkClick(Action*) // private.
{
	_game->popState();
}

}
