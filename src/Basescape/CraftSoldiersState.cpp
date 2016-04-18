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

#include "CraftSoldiersState.h"

//#include <climits>

#include "SoldierInfoState.h"

#include "../Battlescape/BattlescapeGenerator.h"
#include "../Battlescape/InventoryState.h"

#include "../Engine/Action.h"
#include "../Engine/Game.h"
#include "../Engine/Language.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Options.h"
#include "../Engine/Screen.h"
#include "../Engine/Sound.h"

#include "../Resource/XcomResourcePack.h"

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextList.h"
#include "../Interface/Window.h"

//#include "../Ruleset/RuleCraft.h"
#include "../Ruleset/RuleInterface.h"
#include "../Ruleset/Ruleset.h"

#include "../Savegame/Base.h"
#include "../Savegame/Craft.h"
#include "../Savegame/ItemContainer.h"
#include "../Savegame/SavedBattleGame.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/Soldier.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the Craft Soldiers screen.
 * @param base		- pointer to the Base to get info from
 * @param craftId	- ID of the selected craft
 */
CraftSoldiersState::CraftSoldiersState(
		Base* base,
		size_t craftId)
	:
		_base(base),
		_craft(base->getCrafts()->at(craftId))
{
	_window			= new Window(this, 320, 200);

	_txtCost		= new Text(150, 9, 24, -10);

	_txtTitle		= new Text(300, 17, 16, 8);
	_txtBaseLabel	= new Text( 80, 9, 224, 8);

	_txtSpace		= new Text(110, 9,  16, 24);
	_txtLoad		= new Text(110, 9, 171, 24);

	_txtName		= new Text(116, 9,  16, 33);
	_txtRank		= new Text( 93, 9, 140, 33);
	_txtCraft		= new Text( 71, 9, 225, 33);

	_lstSoldiers	= new TextList(285, 129, 16, 42);

	_btnUnload		= new TextButton(94, 16,  16, 177);
	_btnInventory	= new TextButton(94, 16, 113, 177);
	_btnOk			= new TextButton(94, 16, 210, 177);

	setInterface("craftSoldiers");

	add(_window,		"window",	"craftSoldiers");
	add(_txtCost,		"text",		"craftSoldiers");
	add(_txtTitle,		"text",		"craftSoldiers");
	add(_txtBaseLabel,	"text",		"craftSoldiers");
	add(_txtSpace,		"text",		"craftSoldiers");
	add(_txtLoad,		"text",		"craftSoldiers");
	add(_txtName,		"text",		"craftSoldiers");
	add(_txtRank,		"text",		"craftSoldiers");
	add(_txtCraft,		"text",		"craftSoldiers");
	add(_lstSoldiers,	"list",		"craftSoldiers");
	add(_btnUnload,		"button",	"craftSoldiers");
	add(_btnInventory,	"button",	"craftSoldiers");
	add(_btnOk,			"button",	"craftSoldiers");

	centerAllSurfaces();


	_window->setBackground(_game->getResourcePack()->getSurface("BACK02.SCR"));

	_txtTitle->setText(tr("STR_SELECT_SQUAD_FOR_CRAFT")
						.arg(_craft->getName(_game->getLanguage())));
	_txtTitle->setBig();

	_txtBaseLabel->setText(_base->getName());
	_txtBaseLabel->setAlign(ALIGN_RIGHT);

	_txtName->setText(tr("STR_NAME_UC"));
	_txtRank->setText(tr("STR_RANK"));
	_txtCraft->setText(tr("STR_CRAFT"));

	_lstSoldiers->setColumns(3, 116,85,71);
	_lstSoldiers->setArrowColumn(180, ARROW_VERTICAL);
	_lstSoldiers->setBackground(_window);
	_lstSoldiers->setSelectable();
	_lstSoldiers->onMousePress((ActionHandler)& CraftSoldiersState::lstSoldiersPress);
	_lstSoldiers->onLeftArrowClick((ActionHandler)& CraftSoldiersState::lstLeftArrowClick);
	_lstSoldiers->onRightArrowClick((ActionHandler)& CraftSoldiersState::lstRightArrowClick);

	_btnUnload->setText(_game->getLanguage()->getString("STR_UNLOAD_CRAFT"));
	_btnUnload->onMouseClick((ActionHandler)& CraftSoldiersState::btnUnloadClick);
	_btnUnload->onKeyboardPress(
					(ActionHandler)& CraftSoldiersState::btnUnloadClick,
					SDLK_u);

	_btnInventory->setText(tr("STR_LOADOUT"));
	_btnInventory->onMouseClick((ActionHandler)& CraftSoldiersState::btnInventoryClick);
	_btnInventory->onKeyboardPress(
					(ActionHandler)& CraftSoldiersState::btnInventoryClick,
					SDLK_i);

	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)& CraftSoldiersState::btnOkClick);
	_btnOk->onKeyboardPress(
					(ActionHandler)& CraftSoldiersState::btnOkClick,
					Options::keyOk);
	_btnOk->onKeyboardPress(
					(ActionHandler)& CraftSoldiersState::btnOkClick,
					Options::keyOkKeypad);
	_btnOk->onKeyboardPress(
					(ActionHandler)& CraftSoldiersState::btnOkClick,
					Options::keyCancel);
}

/**
 * dTor.
 */
CraftSoldiersState::~CraftSoldiersState()
{}

/**
 * Shows the soldiers in a list.
 */
void CraftSoldiersState::init()
{
	State::init();

	// Reset stuff when coming back from pre-battle Inventory.
	const SavedBattleGame* const battleSave (_game->getSavedGame()->getBattleSave());
	if (battleSave != nullptr)
	{
//		unfortunately this would rely on the list of battleSoldiers *not* changing when entering/cancelling InventoryState
//		_selUnitId = battleSave->getSelectedUnit()->getBattleOrder();
		_game->getSavedGame()->setBattleSave();
		_craft->setTactical(false);
	}

	_lstSoldiers->clearList();

	size_t row = 0;
	Uint8 color;

	for (std::vector<Soldier*>::const_iterator
			i = _base->getSoldiers()->begin();
			i != _base->getSoldiers()->end();
			++i, ++row)
	{
		_lstSoldiers->addRow(
						3,
						(*i)->getName().c_str(),
						tr((*i)->getRankString()).c_str(),
						(*i)->getCraftString(_game->getLanguage()).c_str());

		if ((*i)->getCraft() == nullptr)
			color = _lstSoldiers->getColor();
		else
		{
			if ((*i)->getCraft() == _craft)
				color = _lstSoldiers->getSecondaryColor();
			else
				color = static_cast<Uint8>(_game->getRuleset()->getInterface("craftSoldiers")->getElement("otherCraft")->color);
		}

		_lstSoldiers->setRowColor(row, color);

		if ((*i)->getSickbay() != 0)
		{
			const int pct = (*i)->getRecoveryPct();
			if (pct > 50)
				color = ORANGE;
			else if (pct > 10)
				color = YELLOW;
			else
				color = GREEN;

			_lstSoldiers->setCellColor(row, 2, color, true);
		}
	}

	_txtSpace->setText(tr("STR_SPACE_CREW_HWP_FREE_")
					.arg(_craft->getQtySoldiers())
					.arg(_craft->getQtyVehicles())
					.arg(_craft->getSpaceAvailable()));
	_txtLoad->setText(tr("STR_LOAD_CAPACITY_FREE_")
					.arg(_craft->getLoadCapacity())
					.arg(_craft->getLoadCapacity() - _craft->calcLoadCurrent()));

	displayExtraButtons();
	calculateTacticalCost();

	_lstSoldiers->scrollTo(_base->getRecallRow(REC_SOLDIER));
	_lstSoldiers->draw();
}

/**
 * Returns to the previous screen.
 * @param action - pointer to an Action
 */
void CraftSoldiersState::btnOkClick(Action*)
{
	_base->setRecallRow(
					REC_SOLDIER,
					_lstSoldiers->getScroll());
	_game->popState();
}

/**
 * Unloads all soldiers from current transport craft.
 * @param action - pointer to an Action
 */
void CraftSoldiersState::btnUnloadClick(Action*)
{
	for (std::vector<Soldier*>::const_iterator
			i = _base->getSoldiers()->begin();
			i != _base->getSoldiers()->end();
			++i)
	{
		if ((*i)->getCraft() == _craft)
			(*i)->setCraft();
	}

	_base->setRecallRow(
					REC_SOLDIER,
					_lstSoldiers->getScroll());
	init();
}

/**
 * LMB assigns and de-assigns soldiers from a Craft. RMB shows soldier info.
 * @param action - pointer to an Action
 */
void CraftSoldiersState::lstSoldiersPress(Action* action)
{
	const double mx = action->getAbsoluteXMouse();
	if (mx >= static_cast<double>(_lstSoldiers->getArrowsLeftEdge())
		&& mx < static_cast<double>(_lstSoldiers->getArrowsRightEdge()))
	{
		return;
	}

	const size_t row = _lstSoldiers->getSelectedRow();

	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
	{
		Soldier* const sol = _base->getSoldiers()->at(_lstSoldiers->getSelectedRow());

		if (sol->getSickbay() != 0
			|| (sol->getCraft() != nullptr
				&& sol->getCraft()->getCraftStatus() == CS_OUT))
		{
			return;
		}

		Uint8 color;
		if (sol->getCraft() == nullptr
			&& _craft->getSpaceAvailable() != 0
			&& _craft->getLoadCapacity() - _craft->calcLoadCurrent() > 9)
		{
			sol->setCraft(_craft);
			color = _lstSoldiers->getSecondaryColor();
			_lstSoldiers->setCellText(
									row, 2,
									_craft->getName(_game->getLanguage()));
		}
		else
		{
			color = _lstSoldiers->getColor();

			if (sol->getCraft() != nullptr
				&& sol->getCraft()->getCraftStatus() != CS_OUT)
			{
				sol->setCraft();
				_lstSoldiers->setCellText(
										row, 2,
										tr("STR_NONE_UC"));
			}
		}
		_lstSoldiers->setRowColor(row, color);

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
	else if (action->getDetails()->button.button == SDL_BUTTON_RIGHT)
	{
		_base->setRecallRow(
						REC_SOLDIER,
						_lstSoldiers->getScroll());
		_game->pushState(new SoldierInfoState(_base, row));
		kL_soundPop->play(Mix_GroupAvailable(0));
	}
}

/**
 * Reorders a soldier up.
 * @param action - pointer to an Action
 */
void CraftSoldiersState::lstLeftArrowClick(Action* action)
{
	_base->setRecallRow(
					REC_SOLDIER,
					_lstSoldiers->getScroll());

	const size_t row = _lstSoldiers->getSelectedRow();
	if (row > 0)
	{
		Soldier* const sol = _base->getSoldiers()->at(row);

		if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
		{
			_base->getSoldiers()->at(row) = _base->getSoldiers()->at(row - 1);
			_base->getSoldiers()->at(row - 1) = sol;

			if (row != _lstSoldiers->getScroll())
			{
				SDL_WarpMouse(
						static_cast<Uint16>(action->getLeftBlackBand() + action->getMouseX()),
						static_cast<Uint16>(action->getTopBlackBand() + action->getMouseY()
												- static_cast<int>(8. * action->getScaleY())));
			}
			else
			{
				_base->setRecallRow(
								REC_SOLDIER,
								_lstSoldiers->getScroll() - 1);
				_lstSoldiers->scrollUp();
			}
		}
		else if (action->getDetails()->button.button == SDL_BUTTON_RIGHT)
		{
			_base->setRecallRow(
							REC_SOLDIER,
							_lstSoldiers->getScroll() + 1);

			_base->getSoldiers()->erase(_base->getSoldiers()->begin() + row);
			_base->getSoldiers()->insert(
									_base->getSoldiers()->begin(),
									sol);
		}

		init();
	}
}

/**
 * Reorders a soldier down.
 * @param action - pointer to an Action
 */
void CraftSoldiersState::lstRightArrowClick(Action* action)
{
	_base->setRecallRow(
					REC_SOLDIER,
					_lstSoldiers->getScroll());

	const size_t
		qtySoldiers = _base->getSoldiers()->size(),
		row = _lstSoldiers->getSelectedRow();

	if (qtySoldiers != 0
		&& row < qtySoldiers - 1)
	{
		Soldier* const sol = _base->getSoldiers()->at(row);

		if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
		{
			_base->getSoldiers()->at(row) = _base->getSoldiers()->at(row + 1);
			_base->getSoldiers()->at(row + 1) = sol;

			if (row != _lstSoldiers->getVisibleRows() - 1 + _lstSoldiers->getScroll())
			{
				SDL_WarpMouse(
						static_cast<Uint16>(action->getLeftBlackBand() + action->getMouseX()),
						static_cast<Uint16>(action->getTopBlackBand() + action->getMouseY()
												+ static_cast<int>(8. * action->getScaleY())));
			}
			else
			{
				_base->setRecallRow(
								REC_SOLDIER,
								_lstSoldiers->getScroll() + 1);
				_lstSoldiers->scrollDown();
			}
		}
		else if (action->getDetails()->button.button == SDL_BUTTON_RIGHT)
		{
			_base->getSoldiers()->erase(_base->getSoldiers()->begin() + row);
			_base->getSoldiers()->insert(
									_base->getSoldiers()->end(),
									sol);
		}

		init();
	}
}

/**
* Displays the inventory screen for the soldiers inside the craft.
* @param action - pointer to an Action
*/
void CraftSoldiersState::btnInventoryClick(Action*)
{
	_base->setRecallRow(
					REC_SOLDIER,
					_lstSoldiers->getScroll());

	SavedBattleGame* const battleSave (new SavedBattleGame());
	_game->getSavedGame()->setBattleSave(battleSave);

	BattlescapeGenerator bGen = BattlescapeGenerator(_game);
	bGen.runInventory(_craft);
//	unfortunately this would rely on the list of battleSoldiers *not* changing when entering/cancelling InventoryState
//	bGen.runInventory(_craft, nullptr, _selUnitId);

	_game->getScreen()->clear();
	_game->pushState(new InventoryState());
}

/**
 * Sets current cost to send the Craft on a mission.
 */
void CraftSoldiersState::calculateTacticalCost() // private.
{
	const int cost (_base->calcSoldierBonuses(_craft)
				  + _craft->getRules()->getSoldiers() * 1000);
	_txtCost->setText(tr("STR_COST_").arg(Text::formatCurrency(cost)));
}

/**
 * Decides whether to show extra buttons - Unload and Inventory.
 */
void CraftSoldiersState::displayExtraButtons() const // private.
{
	const bool hasSoldier (_craft->getQtySoldiers() != 0);
	_btnUnload->setVisible(hasSoldier);
	_btnInventory->setVisible(hasSoldier
						   && _craft->getCraftItems()->getTotalQuantity() != 0
						   && _game->getSavedGame()->getMonthsPassed() != -1);
}

}
