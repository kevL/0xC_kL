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
 * Initializes all the elements in the Craft Equipment screen.
 * @param base		- pointer to the Base to get info from
 * @param craftId	- ID of the selected craft
 */
CraftEquipmentState::CraftEquipmentState(
		Base* base,
		size_t craftId)
	:
		_base(base),
		_craft(base->getCrafts()->at(craftId)),
		_rules(_game->getRuleset()),
		_sel(0),
		_selUnitId(0)
{
	_window			= new Window(this, 320, 200);

	_txtCost		= new Text(150, 9, 24, -10);

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
	add(_txtCost,		"text",		"craftEquipment");
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

	centerAllSurfaces();


	_window->setBackground(_game->getResourcePack()->getSurface("BACK04.SCR"));

	_btnClear->setText(tr("STR_UNLOAD_CRAFT"));
	_btnClear->onMouseClick((ActionHandler)& CraftEquipmentState::btnUnloadCraftClick);
	_btnClear->onKeyboardPress(
					(ActionHandler)& CraftEquipmentState::btnUnloadCraftClick,
					SDLK_u);

	_btnInventory->setText(tr("STR_LOADOUT"));
	_btnInventory->onMouseClick((ActionHandler)& CraftEquipmentState::btnInventoryClick);
	_btnInventory->onKeyboardPress(
					(ActionHandler)& CraftEquipmentState::btnInventoryClick,
					SDLK_i);

	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)& CraftEquipmentState::btnOkClick);
	_btnOk->onKeyboardPress(
					(ActionHandler)& CraftEquipmentState::btnOkClick,
					Options::keyCancel);
	_btnOk->onKeyboardPress(
					(ActionHandler)& CraftEquipmentState::btnOkClick,
					Options::keyOk);
	_btnOk->onKeyboardPress(
					(ActionHandler)& CraftEquipmentState::btnOkClick,
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

	_txtLoad->setText(tr("STR_LOAD_CAPACITY_FREE_")
						.arg(_craft->getLoadCapacity())
						.arg(_craft->getLoadCapacity() - _craft->calcLoadCurrent()));

	_lstEquipment->setArrowColumn(189, ARROW_HORIZONTAL);
	_lstEquipment->setColumns(3, 147,85,41);
	_lstEquipment->setBackground(_window);
	_lstEquipment->setSelectable();
//	_lstEquipment->setAllowScrollOnArrowButtons(!_allowChangeListValuesByMouseWheel);
//	_lstEquipment->onMousePress((ActionHandler)& CraftEquipmentState::lstEquipmentMousePress);
	_lstEquipment->onLeftArrowPress((ActionHandler)& CraftEquipmentState::lstEquipmentLeftArrowPress);
	_lstEquipment->onLeftArrowRelease((ActionHandler)& CraftEquipmentState::lstEquipmentLeftArrowRelease);
	_lstEquipment->onLeftArrowClick((ActionHandler)& CraftEquipmentState::lstEquipmentLeftArrowClick);
	_lstEquipment->onRightArrowPress((ActionHandler)& CraftEquipmentState::lstEquipmentRightArrowPress);
	_lstEquipment->onRightArrowRelease((ActionHandler)& CraftEquipmentState::lstEquipmentRightArrowRelease);
	_lstEquipment->onRightArrowClick((ActionHandler)& CraftEquipmentState::lstEquipmentRightArrowClick);


	size_t row (0);
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

			if (_base->getStorageItems()->getItemQuantity(*i) != 0 || craftQty != 0)
			{
				_items.push_back(*i);

				woststr.str(L"");
				if (_game->getSavedGame()->getMonthsPassed() != -1)
					woststr << _base->getStorageItems()->getItemQuantity(*i);
				else
					woststr << "-";

				wst = tr(*i);
				if (itRule->getBattleType() == BT_AMMO) // weapon clips
				{
					wst.insert(0, L"  ");
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

				_lstEquipment->setRowColor(row++, color);
			}
		}
	}

	_timerLeft = new Timer(Timer::SCROLL_SLOW);
	_timerLeft->onTimer((StateHandler)& CraftEquipmentState::moveLeft);

	_timerRight = new Timer(Timer::SCROLL_SLOW);
	_timerRight->onTimer((StateHandler)& CraftEquipmentState::moveRight);
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
 * Resets the stuff when coming back from soldiers' inventory state.
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

	displayExtraButtons();
	calculateTacticalCost();
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
 * Returns to the previous screen.
 * @param action - pointer to an Action
 */
void CraftEquipmentState::btnOkClick(Action*)
{
	_game->popState();
}

/**
 * Starts moving the item to the base.
 * @param action - pointer to an Action
 */
void CraftEquipmentState::lstEquipmentLeftArrowPress(Action* action)
{
	_sel = _lstEquipment->getSelectedRow();

	if (action->getDetails()->button.button == SDL_BUTTON_LEFT
		&& _timerLeft->isRunning() == false)
	{
		_timerLeft->start();
	}
}

/**
 * Stops moving the item to the base.
 * @param action - pointer to an Action
 */
void CraftEquipmentState::lstEquipmentLeftArrowRelease(Action* action)
{
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
		_timerLeft->stop();
}

/**
 * Moves all the items to the base on right-click.
 * @param action - pointer to an Action
 */
void CraftEquipmentState::lstEquipmentLeftArrowClick(Action* action)
{
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
	{
		moveLeftByValue(1);

		_timerRight->setInterval(Timer::SCROLL_SLOW);
		_timerLeft->setInterval(Timer::SCROLL_SLOW);
	}
	else if (action->getDetails()->button.button == SDL_BUTTON_RIGHT)
		moveLeftByValue(std::numeric_limits<int>::max());
}

/**
 * Starts moving the item to the craft.
 * @param action - pointer to an Action
 */
void CraftEquipmentState::lstEquipmentRightArrowPress(Action* action)
{
	_sel = _lstEquipment->getSelectedRow();

	if (action->getDetails()->button.button == SDL_BUTTON_LEFT
		&& _timerRight->isRunning() == false)
	{
		_timerRight->start();
	}
}

/**
 * Stops moving the item to the craft.
 * @param action - pointer to an Action
 */
void CraftEquipmentState::lstEquipmentRightArrowRelease(Action* action)
{
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
		_timerRight->stop();
}

/**
 * Moves all the items (as much as possible) to the craft on right-click.
 * @param action - pointer to an Action
 */
void CraftEquipmentState::lstEquipmentRightArrowClick(Action* action)
{
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
	{
		moveRightByValue(1);

		_timerRight->setInterval(Timer::SCROLL_SLOW);
		_timerLeft->setInterval(Timer::SCROLL_SLOW);
	}
	else if (action->getDetails()->button.button == SDL_BUTTON_RIGHT)
		moveRightByValue(std::numeric_limits<int>::max());
}

/**
 * Updates the displayed quantities of the selected item on the list.
 */
void CraftEquipmentState::updateQuantity()
{
	const RuleItem* const itRule (_rules->getItemRule(_items[_sel]));

	int craftQty;
	if (itRule->isFixed() == true)
		craftQty = _craft->getVehicleCount(_items[_sel]);
	else
		craftQty = _craft->getCraftItems()->getItemQuantity(_items[_sel]);

	std::wostringstream woststr;
	if (_game->getSavedGame()->getMonthsPassed() != -1)
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
	_lstEquipment->setCellText(_sel, 1, woststr.str());
	_lstEquipment->setCellText(_sel, 2, Text::intWide(craftQty));

	_txtSpace->setText(tr("STR_SPACE_CREW_HWP_FREE_")
						.arg(_craft->getQtySoldiers())
						.arg(_craft->getQtyVehicles())
						.arg(_craft->getSpaceAvailable()));
	_txtLoad->setText(tr("STR_LOAD_CAPACITY_FREE_")
						.arg(_craft->getLoadCapacity())
						.arg(_craft->getLoadCapacity() - _craft->calcLoadCurrent()));

	displayExtraButtons();
	calculateTacticalCost();
}

/**
 * Moves the selected item to the craft.
 */
void CraftEquipmentState::moveRight()
{
	_timerLeft->setInterval(Timer::SCROLL_FAST);
	_timerRight->setInterval(Timer::SCROLL_FAST);

	moveRightByValue(1);
}

/**
 * Moves the given number of items (selected) to the Craft.
 * @param qtyDelta - quantity change
 */
void CraftEquipmentState::moveRightByValue(int qtyDelta)
{
	if (qtyDelta > 0)
	{
		int baseQty;
		if (_game->getSavedGame()->getMonthsPassed() == -1)
		{
			if (qtyDelta == std::numeric_limits<int>::max())
				qtyDelta = 10;

			baseQty = qtyDelta;
		}
		else
			baseQty = _base->getStorageItems()->getItemQuantity(_items[_sel]);

		if (baseQty != 0)
		{
			bool overloaded (false);
			qtyDelta = std::min(qtyDelta, baseQty);

			const RuleItem* const itRule (_rules->getItemRule(_items[_sel]));
			if (itRule->isFixed() == true) // load vehicle, convert item to a vehicle
			{
				int quadrants (_rules->getArmor(_rules->getUnitRule(_items[_sel])->getArmorType())->getSize());
				quadrants *= quadrants;

				const int spaceAvailable (std::min(
												_craft->getRules()->getVehicles() - _craft->getQtyVehicles(true),
												_craft->getSpaceAvailable())
											/ quadrants);

				if (spaceAvailable > 0
					&& _craft->getLoadCapacity() - _craft->calcLoadCurrent() >= quadrants * 10) // note: 10 is the 'load' that a single 'space' uses.
				{
					qtyDelta = std::min(qtyDelta, spaceAvailable);
					qtyDelta = std::min(qtyDelta,
									   (_craft->getLoadCapacity() - _craft->calcLoadCurrent()) / (quadrants * 10));

					if (itRule->getFullClip() < 1) // no Ammo required.
					{
						for (int
								i = 0;
								i != qtyDelta;
								++i)
						{
							_craft->getVehicles()->push_back(new Vehicle(
																	itRule,
																	itRule->getFullClip(),
																	quadrants));
						}

						if (_game->getSavedGame()->getMonthsPassed() != -1)
							_base->getStorageItems()->removeItem(_items[_sel], qtyDelta);
					}
					else // tank needs Ammo.
					{
						const std::string type (itRule->getCompatibleAmmo()->front());
						const int
							clipsRequired (itRule->getFullClip()),
							baseClips (_base->getStorageItems()->getItemQuantity(type));

						if (_game->getSavedGame()->getMonthsPassed() != -1)
							qtyDelta = std::min(qtyDelta,
												baseClips / clipsRequired);

						if (qtyDelta > 0)
						{
							for (int
									i = 0;
									i != qtyDelta;
									++i)
							{
								_craft->getVehicles()->push_back(new Vehicle(
																		itRule,
																		clipsRequired,
																		quadrants));
							}

							if (_game->getSavedGame()->getMonthsPassed() != -1)
							{
								_base->getStorageItems()->removeItem(_items[_sel], qtyDelta);
								_base->getStorageItems()->removeItem(type, clipsRequired * qtyDelta);
							}
						}
						else // not enough Ammo
						{
							_timerRight->stop();
							LocalizedText playerError (tr("STR_NOT_ENOUGH_AMMO_TO_ARM_HWP")
															.arg(tr(type)));
							_game->pushState(new ErrorMessageState(
																playerError,
																_palette,
																Palette::blockOffset(15)+1,
																"BACK04.SCR",
																2));
						}
					}
				}
				else
					overloaded = true;
			}
			else // load item
			{
				if (_craft->getRules()->getItems() != 0
					&& _craft->calcLoadCurrent() + qtyDelta > _craft->getLoadCapacity())
				{
					overloaded = true;
					_timerRight->stop();
					qtyDelta = _craft->getLoadCapacity() - _craft->calcLoadCurrent();
				}

				if (qtyDelta > 0)
				{
					_craft->getCraftItems()->addItem(_items[_sel], qtyDelta);

					if (_game->getSavedGame()->getMonthsPassed() != -1)
						_base->getStorageItems()->removeItem(_items[_sel], qtyDelta);
				}
			}

			if (overloaded == true)
			{
				LocalizedText playerError (tr("STR_NO_MORE_EQUIPMENT_ALLOWED", _craft->getLoadCapacity()));
				_game->pushState(new ErrorMessageState(
													playerError,
													_palette,
													COLOR_ERROR,
													"BACK04.SCR",
													COLOR_ERROR_BG));
			}

			updateQuantity();
		}
	}
}

/**
 * Moves the selected item to the base.
 */
void CraftEquipmentState::moveLeft()
{
	_timerLeft->setInterval(Timer::SCROLL_FAST);
	_timerRight->setInterval(Timer::SCROLL_FAST);

	moveLeftByValue(1);
}

/**
 * Moves the given number of items (selected) to the Base.
 * @param qtyDelta - quantity change
 */
void CraftEquipmentState::moveLeftByValue(int qtyDelta)
{
	if (qtyDelta > 0)
	{
		const RuleItem* const itRule (_rules->getItemRule(_items[_sel]));

		int craftQty;
		if (itRule->isFixed() == true)
			craftQty = _craft->getVehicleCount(_items[_sel]);
		else
			craftQty = _craft->getCraftItems()->getItemQuantity(_items[_sel]);

		if (craftQty != 0)
		{
			qtyDelta = std::min(qtyDelta, craftQty);

			if (itRule->isFixed() == true) // convert vehicle to item
			{
				if (itRule->getFullClip() < 1) // no Ammo.
				{
					if (_game->getSavedGame()->getMonthsPassed() != -1)
						_base->getStorageItems()->addItem(_items[_sel], qtyDelta);

					for (std::vector<Vehicle*>::const_iterator
							i = _craft->getVehicles()->begin();
							i != _craft->getVehicles()->end();
							)
					{
						if ((*i)->getRules() == itRule)
						{
							delete *i;
							i = _craft->getVehicles()->erase(i);

							if (--qtyDelta == 0)
								break;
						}
						else
							++i;
					}
				}
				else // tank has Ammo.
				{
					for (std::vector<Vehicle*>::const_iterator
							i = _craft->getVehicles()->begin();
							i != _craft->getVehicles()->end();
							)
					{
						if ((*i)->getRules() == itRule)
						{
							if (_game->getSavedGame()->getMonthsPassed() != -1)
								_base->getStorageItems()->addItem(
																itRule->getCompatibleAmmo()->front(),
																(*i)->getLoad());
							delete *i;
							i = _craft->getVehicles()->erase(i);
						}
						else
							++i;
					}

					if (_game->getSavedGame()->getMonthsPassed() != -1)
						_base->getStorageItems()->addItem(_items[_sel], craftQty);

					if (craftQty > qtyDelta)
						moveRightByValue(craftQty - qtyDelta);
				}
			}
			else
			{
				_craft->getCraftItems()->removeItem(_items[_sel], qtyDelta);

				if (_game->getSavedGame()->getMonthsPassed() != -1)
					_base->getStorageItems()->addItem(_items[_sel], qtyDelta);
			}

			updateQuantity();
		}
	}
}

/**
 * Empties the contents of the Craft - moves all the items back to the Base.
 * @param action - pointer to an Action
 */
void CraftEquipmentState::btnUnloadCraftClick(Action*)
{
	for (
			_sel = 0;
			_sel != _items.size();
			++_sel)
	{
		moveLeftByValue(std::numeric_limits<int>::max());
	}
}

/**
* Displays the inventory screen for the soldiers inside the craft.
* @param action - pointer to an Action
*/
void CraftEquipmentState::btnInventoryClick(Action*)
{
	SavedBattleGame* const battleSave (new SavedBattleGame());
	_game->getSavedGame()->setBattleSave(battleSave);

	BattlescapeGenerator bGen = BattlescapeGenerator(_game);
	bGen.runInventory(_craft, nullptr, _selUnitId);

	_game->getScreen()->clear();
	_game->pushState(new InventoryState());
}

/**
 * Sets current cost to send the Craft on a mission.
 */
void CraftEquipmentState::calculateTacticalCost() // private.
{
	const int cost (_base->calcSoldierBonuses(_craft)
				  + _craft->getRules()->getSoldiers() * 1000);
	_txtCost->setText(tr("STR_COST_").arg(Text::formatCurrency(cost)));
}

/**
 * Decides whether to show extra buttons - Unload and Inventory.
 */
void CraftEquipmentState::displayExtraButtons() const // private.
{
	const bool hasItem (_craft->getCraftItems()->getTotalQuantity() != 0);
	_btnClear->setVisible(hasItem);
	_btnInventory->setVisible(hasItem
						  && _craft->getQtySoldiers() != 0
						  && _game->getSavedGame()->getMonthsPassed() != -1);
}

/*
 * Handles the mouse-wheels on the arrow-buttons.
 * @param action, Pointer to an action.
 *
void CraftEquipmentState::lstEquipmentMousePress(Action* action)
{
	_sel = _lstEquipment->getSelectedRow();

	if (action->getDetails()->button.button == SDL_BUTTON_WHEELUP)
	{
		_timerRight->stop();
		_timerLeft->stop();

		if (action->getAbsoluteXMouse() >= _lstEquipment->getArrowsLeftEdge()
			&& action->getAbsoluteXMouse() <= _lstEquipment->getArrowsRightEdge())
		{
			moveRightByValue(Options::changeValueByMouseWheel);
		}
	}
	else if (action->getDetails()->button.button == SDL_BUTTON_WHEELDOWN)
	{
		_timerRight->stop();
		_timerLeft->stop();

		if (action->getAbsoluteXMouse() >= _lstEquipment->getArrowsLeftEdge()
			&& action->getAbsoluteXMouse() <= _lstEquipment->getArrowsRightEdge())
		{
			moveLeftByValue(Options::changeValueByMouseWheel);
		}
	}
} */

}
