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
 * Initializes all the elements in the CraftSoldiers screen.
 * @param base		- pointer to the Base to get info from
 * @param craftId	- ID of the selected craft
 */
CraftSoldiersState::CraftSoldiersState(
		Base* const base,
		size_t craftId)
	:
		_base(base),
		_craft(base->getCrafts()->at(craftId)),
		_isQuickBattle(_game->getSavedGame()->getMonthsElapsed() == -1)
{
	_window			= new Window(this);

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

	if (_isQuickBattle == false)
	{
		_txtCost = new Text(150, 9, 24, -10);
		add(_txtCost, "text", "craftSoldiers");
	}

	centerSurfaces();


	_window->setBackground(_game->getResourcePack()->getSurface("BACK02.SCR"));

	_txtTitle->setText(tr("STR_SELECT_SQUAD_FOR_CRAFT")
						.arg(_craft->getLabel(
										_game->getLanguage(),
										_isQuickBattle == false)));
	_txtTitle->setBig();

	_txtBaseLabel->setText(_base->getLabel());
	_txtBaseLabel->setAlign(ALIGN_RIGHT);

	_txtName->setText(tr("STR_NAME_UC"));
	_txtRank->setText(tr("STR_RANK"));
	_txtCraft->setText(tr("STR_CRAFT"));

	_lstSoldiers->setColumns(3, 116,85,71);
	_lstSoldiers->setArrow(180, ARROW_VERTICAL);
	_lstSoldiers->setBackground(_window);
	_lstSoldiers->setSelectable();
	_lstSoldiers->onMousePress(		static_cast<ActionHandler>(&CraftSoldiersState::lstSoldiersPress));
	_lstSoldiers->onLeftArrowClick(	static_cast<ActionHandler>(&CraftSoldiersState::lstLeftArrowClick));
	_lstSoldiers->onRightArrowClick(static_cast<ActionHandler>(&CraftSoldiersState::lstRightArrowClick));

	_btnUnload->setText(_game->getLanguage()->getString("STR_UNLOAD_CRAFT"));
	_btnUnload->onMouseClick(	static_cast<ActionHandler>(&CraftSoldiersState::btnUnloadClick));
	_btnUnload->onKeyboardPress(static_cast<ActionHandler>(&CraftSoldiersState::btnUnloadClick),
								SDLK_u);

	_btnInventory->setText(tr("STR_LOADOUT"));
	_btnInventory->onMouseClick(	static_cast<ActionHandler>(&CraftSoldiersState::btnInventoryClick));
	_btnInventory->onKeyboardPress(	static_cast<ActionHandler>(&CraftSoldiersState::btnInventoryClick),
									SDLK_i);

	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick(	static_cast<ActionHandler>(&CraftSoldiersState::btnOkClick));
	_btnOk->onKeyboardPress(static_cast<ActionHandler>(&CraftSoldiersState::btnOkClick),
							Options::keyOk);
	_btnOk->onKeyboardPress(static_cast<ActionHandler>(&CraftSoldiersState::btnOkClick),
							Options::keyOkKeypad);
	_btnOk->onKeyboardPress(static_cast<ActionHandler>(&CraftSoldiersState::btnOkClick),
							Options::keyCancel);
}

/**
 * dTor.
 */
CraftSoldiersState::~CraftSoldiersState()
{}

/**
 * Shows the Soldiers in a TextList.
 */
void CraftSoldiersState::init()
{
	State::init();

	// Reset stuff when coming back from pre-battle Inventory.
	const SavedBattleGame* const battleSave (_game->getSavedGame()->getBattleSave());
	if (battleSave != nullptr)
	{
//		_selUnitId = battleSave->getSelectedUnit()->getBattleOrder();	// unfortunately this would depend on the list of battleSoldiers
																		// *not* changing when entering or cancelling InventoryState
		_game->getSavedGame()->setBattleSave();
		_craft->setTactical(false);
	}

	_lstSoldiers->clearList();

	Uint8 color;

	size_t r (0u);
	for (std::vector<Soldier*>::const_iterator
			i = _base->getSoldiers()->begin();
			i != _base->getSoldiers()->end();
			++i, ++r)
	{
		_lstSoldiers->addRow(
						3,
						(*i)->getLabel().c_str(),
						tr((*i)->getRankString()).c_str(),
						(*i)->getCraftLabel(_game->getLanguage()).c_str());

		if ((*i)->getCraft() == nullptr)
			color = _lstSoldiers->getColor();
		else if ((*i)->getCraft() == _craft)
			color = _lstSoldiers->getSecondaryColor();
		else
			color = static_cast<Uint8>(_game->getRuleset()->getInterface("craftSoldiers")->getElement("otherCraft")->color);

		_lstSoldiers->setRowColor(r, color);

		if ((*i)->getSickbay() != 0)
			_lstSoldiers->setCellColor(r, 2u, (*i)->getSickbayColor(), true);
	}

	_txtSpace->setText(tr("STR_SPACE_CREW_HWP_FREE_")
						.arg(_craft->getQtySoldiers())
						.arg(_craft->getQtyVehicles())
						.arg(_craft->getSpaceAvailable()));
	_txtLoad->setText(tr("STR_LOAD_CAPACITY_FREE_")
						.arg(_craft->getLoadCapacity())
						.arg(_craft->getLoadCapacity() - _craft->calcLoadCurrent()));

	extra();

	_lstSoldiers->scrollTo(_base->getRecallRow(RCL_SOLDIER));
	_lstSoldiers->draw();
}

/**
 * Decides whether to show extra buttons -- unload-craft and Inventory -- and
 * whether to show tactical-costs.
 */
void CraftSoldiersState::extra() const // private.
{
	const bool vis (_craft->getQtySoldiers() != 0);
	_btnUnload->setVisible(vis);
	_btnInventory->setVisible(vis && _craft->getCraftItems()->isEmpty() == false);

	if (_isQuickBattle == false)
		_txtCost->setText(tr("STR_COST_").arg(Text::formatCurrency(_craft->getOperationalCost())));
}

/**
 * Exits to the previous screen.
 * @param action - pointer to an Action
 */
void CraftSoldiersState::btnOkClick(Action*)
{
	_base->setRecallRow(
					RCL_SOLDIER,
					_lstSoldiers->getScroll());
	_game->popState();
}

/**
 * Unloads all Soldiers from the current transport Craft.
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
			(*i)->setCraft(
						nullptr,
						_base,
						_isQuickBattle == true);
	}

	_base->setRecallRow(
					RCL_SOLDIER,
					_lstSoldiers->getScroll());
	init();
}

/**
 * LMB assigns and de-assigns Soldiers from a Craft. RMB shows SoldierInfo.
 * @param action - pointer to an Action
 */
void CraftSoldiersState::lstSoldiersPress(Action* action)
{
	const double mX (action->getAbsoluteMouseX());
	if (   mX >= static_cast<double>(_lstSoldiers->getArrowsLeftEdge())
		&& mX <  static_cast<double>(_lstSoldiers->getArrowsRightEdge()))
	{
		return;
	}

	switch (action->getDetails()->button.button)
	{
		case SDL_BUTTON_LEFT:
		{
			Soldier* const sol (_base->getSoldiers()->at(_lstSoldiers->getSelectedRow()));

			if (sol->getSickbay() == 0
				&& (sol->getCraft() == nullptr
					|| sol->getCraft()->getCraftStatus() != CS_OUT))
			{
				const size_t r (_lstSoldiers->getSelectedRow());

				Uint8 color;
				if (sol->getCraft() == nullptr
					&& _craft->getSpaceAvailable() != 0
					&& _craft->getLoadCapacity() - _craft->calcLoadCurrent() > 9)
				{
					color = _lstSoldiers->getSecondaryColor();

					sol->setCraft(
								_craft,
								_base,
								_isQuickBattle == true);
					_lstSoldiers->setCellText(
											r, 2u,
											_craft->getLabel(
														_game->getLanguage(),
														_isQuickBattle == false));
				}
				else
				{
					color = _lstSoldiers->getColor();

					if (sol->getCraft() != nullptr
						&& sol->getCraft()->getCraftStatus() != CS_OUT)
					{
						sol->setCraft(
									nullptr,
									_base,
									_isQuickBattle == true);
						_lstSoldiers->setCellText(
												r, 2u,
												tr("STR_NONE_UC"));
					}
				}
				_lstSoldiers->setRowColor(r, color);

				_txtSpace->setText(tr("STR_SPACE_CREW_HWP_FREE_")
									.arg(_craft->getQtySoldiers())
									.arg(_craft->getQtyVehicles())
									.arg(_craft->getSpaceAvailable()));
				_txtLoad->setText(tr("STR_LOAD_CAPACITY_FREE_")
									.arg(_craft->getLoadCapacity())
									.arg(_craft->getLoadCapacity() - _craft->calcLoadCurrent()));

				extra();
			}
			break;
		}

		case SDL_BUTTON_RIGHT:
			_base->setRecallRow(
							RCL_SOLDIER,
							_lstSoldiers->getScroll());
			_game->pushState(new SoldierInfoState(
											_base,
											_lstSoldiers->getSelectedRow()));
			kL_soundPop->play(Mix_GroupAvailable(0));
	}
}

/**
 * Reorders a Soldier up the list.
 * @param action - pointer to an Action
 */
void CraftSoldiersState::lstLeftArrowClick(Action* action)
{
	_base->setRecallRow(
					RCL_SOLDIER,
					_lstSoldiers->getScroll());

	const size_t r (_lstSoldiers->getSelectedRow());
	if (r > 0u)
	{
		switch (action->getDetails()->button.button)
		{
			case SDL_BUTTON_LEFT:
			{
				Soldier* const sol (_base->getSoldiers()->at(r));

				_base->getSoldiers()->at(r) = _base->getSoldiers()->at(r - 1u);
				_base->getSoldiers()->at(r - 1u) = sol;

				if (r != _lstSoldiers->getScroll())
					SDL_WarpMouse(
							static_cast<Uint16>(action->getBorderLeft() + action->getMouseX()),
							static_cast<Uint16>(action->getBorderTop()  + action->getMouseY()
								- static_cast<int>(8. * action->getScaleY())));
				else
				{
					_base->setRecallRow(
									RCL_SOLDIER,
									_lstSoldiers->getScroll() - 1u);
					_lstSoldiers->scrollUp();
				}

				init();
				break;
			}

			case SDL_BUTTON_RIGHT:
			{
				_base->setRecallRow(
								RCL_SOLDIER,
								_lstSoldiers->getScroll() + 1u);

				Soldier* const sol (_base->getSoldiers()->at(r));

				_base->getSoldiers()->erase(_base->getSoldiers()->begin() + static_cast<std::ptrdiff_t>(r));
				_base->getSoldiers()->insert(
										_base->getSoldiers()->begin(),
										sol);
				init();
			}
		}
	}
}

/**
 * Reorders a Soldier down the list.
 * @param action - pointer to an Action
 */
void CraftSoldiersState::lstRightArrowClick(Action* action)
{
	_base->setRecallRow(
					RCL_SOLDIER,
					_lstSoldiers->getScroll());

	const size_t qtySoldiers (_base->getSoldiers()->size());
	if (qtySoldiers != 0u)
	{
		const size_t r (_lstSoldiers->getSelectedRow());
		if (r < qtySoldiers - 1u)
		{
			switch (action->getDetails()->button.button)
			{
				case SDL_BUTTON_LEFT:
				{
					Soldier* const sol (_base->getSoldiers()->at(r));

					_base->getSoldiers()->at(r) = _base->getSoldiers()->at(r + 1u);
					_base->getSoldiers()->at(r + 1u) = sol;

					if (r != _lstSoldiers->getVisibleRows() + _lstSoldiers->getScroll() - 1u)
						SDL_WarpMouse(
								static_cast<Uint16>(action->getBorderLeft() + action->getMouseX()),
								static_cast<Uint16>(action->getBorderTop()  + action->getMouseY()
									+ static_cast<int>(8. * action->getScaleY())));
					else
					{
						_base->setRecallRow(
										RCL_SOLDIER,
										_lstSoldiers->getScroll() + 1u);
						_lstSoldiers->scrollDown();
					}

					init();
					break;
				}

				case SDL_BUTTON_RIGHT:
				{
					Soldier* const sol (_base->getSoldiers()->at(r));

					_base->getSoldiers()->erase(_base->getSoldiers()->begin() + static_cast<std::ptrdiff_t>(r));
					_base->getSoldiers()->insert(
											_base->getSoldiers()->end(),
											sol);
					init();
				}
			}
		}
	}
}

/**
 * Displays the Inventory for the Soldiers inside the Craft.
 * @param action - pointer to an Action
 */
void CraftSoldiersState::btnInventoryClick(Action*)
{
	_base->setRecallRow(
					RCL_SOLDIER,
					_lstSoldiers->getScroll());

	SavedBattleGame* const battleSave (new SavedBattleGame(_game->getSavedGame()));
	_game->getSavedGame()->setBattleSave(battleSave);

	BattlescapeGenerator bGen = BattlescapeGenerator(_game);
	bGen.runFakeInventory(_craft);

	_game->getScreen()->clear();
	_game->pushState(new InventoryState());
}

}
