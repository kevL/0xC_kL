/*
 * Copyright 2010-2015 OpenXcom Developers.
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

#include "CraftArmorState.h"

//#include <climits>
//#include <string>

#include "SoldierArmorState.h"
#include "SoldierInfoState.h"

#include "../Engine/Action.h"
#include "../Engine/Game.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Options.h"
#include "../Engine/Palette.h"
#include "../Engine/Sound.h"

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextList.h"
#include "../Interface/Window.h"

#include "../Menu/ErrorMessageState.h"

#include "../Resource/XcomResourcePack.h"

#include "../Ruleset/RuleArmor.h"
#include "../Ruleset/RuleCraft.h"
#include "../Ruleset/RuleInterface.h"
#include "../Ruleset/Ruleset.h"

#include "../Savegame/Base.h"
#include "../Savegame/Craft.h"
#include "../Savegame/Soldier.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the Craft Armor screen.
 * @param base		- pointer to the base to get info from
 * @param craftId	- ID of the selected craft (default 0)
 */
CraftArmorState::CraftArmorState(
		Base* const base,
		size_t craftId)
	:
		_base(base),
		_craftId(craftId)
{
	_window			= new Window(this, 320, 200);

	_txtTitle		= new Text(288, 17,  16, 10);
	_txtBaseLabel	= new Text( 80,  9, 224, 10);

	_txtName		= new Text(90, 9,  16, 31);
	_txtArmor		= new Text(50, 9, 106, 31);
	_txtCraft		= new Text(50, 9, 226, 31);

	_lstSoldiers	= new TextList(293, 129, 8, 42);

	_btnOk			= new TextButton(288, 16, 16, 177);

	setInterface("craftArmor");

	add(_window,		"window",	"craftArmor");
	add(_txtTitle,		"text",		"craftArmor");
	add(_txtBaseLabel,	"text",		"craftArmor");
	add(_txtName,		"text",		"craftArmor");
	add(_txtArmor,		"text",		"craftArmor");
	add(_txtCraft,		"text",		"craftArmor");
	add(_lstSoldiers,	"list",		"craftArmor");
	add(_btnOk,			"button",	"craftArmor");

	centerAllSurfaces();


	_window->setBackground(_game->getResourcePack()->getSurface("BACK14.SCR"));

	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)& CraftArmorState::btnOkClick);
	_btnOk->onKeyboardPress(
					(ActionHandler)& CraftArmorState::btnOkClick,
					Options::keyOk);
	_btnOk->onKeyboardPress(
					(ActionHandler)& CraftArmorState::btnOkClick,
					Options::keyOkKeypad);
	_btnOk->onKeyboardPress(
					(ActionHandler)& CraftArmorState::btnOkClick,
					Options::keyCancel);


	_txtTitle->setBig();
	_txtTitle->setText(tr("STR_SELECT_ARMOR"));

	_txtBaseLabel->setAlign(ALIGN_RIGHT);
	_txtBaseLabel->setText(_base->getName(_game->getLanguage()));

	_txtName->setText(tr("STR_NAME_UC"));
	_txtCraft->setText(tr("STR_CRAFT"));
	_txtArmor->setText(tr("STR_ARMOR"));

	_lstSoldiers->setArrowColumn(193, ARROW_VERTICAL);
	_lstSoldiers->setColumns(3, 90,120,73);
	_lstSoldiers->setBackground(_window);
	_lstSoldiers->setSelectable();
	_lstSoldiers->onMousePress((ActionHandler)& CraftArmorState::lstSoldiersPress);
	_lstSoldiers->onLeftArrowClick((ActionHandler)& CraftArmorState::lstLeftArrowClick);
	_lstSoldiers->onRightArrowClick((ActionHandler)& CraftArmorState::lstRightArrowClick);
}

/**
 * dTor.
 */
CraftArmorState::~CraftArmorState()
{}

/**
 * The soldier armors can change after going into other screens.
 */
void CraftArmorState::init()
{
	State::init();

	_lstSoldiers->clearList();

	// in case this is invoked from SoldiersState at a base without any Craft:
	const Craft* craft;
	if (_base->getCrafts()->empty() == false)
		craft = _base->getCrafts()->at(_craftId);
	else
		craft = nullptr;

	size_t row = 0;
	for (std::vector<Soldier*>::const_iterator
			i = _base->getSoldiers()->begin();
			i != _base->getSoldiers()->end();
			++i, ++row)
	{
		_lstSoldiers->addRow(
						3,
						(*i)->getName().c_str(),
						tr((*i)->getArmor()->getType()).c_str(),
						(*i)->getCraftString(_game->getLanguage()).c_str());

		Uint8 color;
		if ((*i)->getCraft() == nullptr)
			color = _lstSoldiers->getColor();
		else
		{
			if ((*i)->getCraft() == craft)
				color = _lstSoldiers->getSecondaryColor();
			else
				color = static_cast<Uint8>(_game->getRuleset()->getInterface("craftArmor")->getElement("otherCraft")->color);
		}

		_lstSoldiers->setRowColor(row, color);

		if ((*i)->getRecovery() > 0)
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

	_lstSoldiers->scrollTo(_base->getRecallRow(REC_SOLDIER));
	_lstSoldiers->draw();
}

/**
 * Returns to the previous screen.
 * @param action - pointer to an Action
 */
void CraftArmorState::btnOkClick(Action*)
{
	_base->setRecallRow(
					REC_SOLDIER,
					_lstSoldiers->getScroll());
	_game->popState();
}

/**
 * LMB shows the Select Armor window.
 * RMB shows soldier info.
 * @param action - pointer to an Action
 */
void CraftArmorState::lstSoldiersPress(Action* action)
{
	const double mx = action->getAbsoluteXMouse();
	if (mx >= static_cast<double>(_lstSoldiers->getArrowsLeftEdge())
		&& mx < static_cast<double>(_lstSoldiers->getArrowsRightEdge()))
	{
		return;
	}

	_base->setRecallRow(
					REC_SOLDIER,
					_lstSoldiers->getScroll());

	size_t soldierId = _lstSoldiers->getSelectedRow();
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
	{
		const Soldier* const soldier = _base->getSoldiers()->at(soldierId);
		if (soldier->getCraft() == nullptr
			|| soldier->getCraft()->getCraftStatus() != "STR_OUT")
		{
			_game->pushState(new SoldierArmorState(
												_base,
												soldierId));
		}
		else
			_game->pushState(new ErrorMessageState(
												tr("STR_SOLDIER_NOT_AT_BASE"),
												_palette,
												Palette::blockOffset(4)+10,
												"BACK12.SCR",
												6));
	}
	else if (action->getDetails()->button.button == SDL_BUTTON_RIGHT)
	{
		_game->pushState(new SoldierInfoState(
											_base,
											soldierId));
		kL_soundPop->play(Mix_GroupAvailable(0));
	}
}
/*		sorry I'll keep SoldierInfoState on RMB; it's easy enough to assign armor. TODO: Could use CTRL+click ....
		SavedGame* _save;
		_save = _game->getSavedGame();
		RuleArmor* a = _game->getRuleset()->getArmor(_save->getLastSelectedArmor());
		if (_game->getSavedGame()->getMonthsPassed() != -1)
		{
			if (_base->getItems()->getItem(a->getStoreItem()) > 0 || a->getStoreItem() == RuleArmor::NONE)
			{
				if (s->getArmor()->getStoreItem() != RuleArmor::NONE)
					_base->getItems()->addItem(s->getArmor()->getStoreItem());
				if (a->getStoreItem() != RuleArmor::NONE)
					_base->getItems()->removeItem(a->getStoreItem());
				s->setArmor(a);
				_lstSoldiers->setCellText(_lstSoldiers->getSelectedRow(), 2, tr(a->getType()));
			}
		}
		else
		{
			s->setArmor(a);
			_lstSoldiers->setCellText(_lstSoldiers->getSelectedRow(), 2, tr(a->getType()));
		} */

/**
 * Reorders a soldier up.
 * @param action - pointer to an Action
 */
void CraftArmorState::lstLeftArrowClick(Action* action)
{
	_base->setRecallRow(
					REC_SOLDIER,
					_lstSoldiers->getScroll());

	const size_t row = _lstSoldiers->getSelectedRow();
	if (row > 0)
	{
		Soldier* const soldier = _base->getSoldiers()->at(row);

		if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
		{
			_base->getSoldiers()->at(row) = _base->getSoldiers()->at(row - 1);
			_base->getSoldiers()->at(row - 1) = soldier;

			if (row != _lstSoldiers->getScroll())
			{
				SDL_WarpMouse(
						static_cast<Uint16>(action->getLeftBlackBand() + action->getXMouse()),
						static_cast<Uint16>(action->getTopBlackBand() + action->getYMouse()
												- static_cast<int>(8. * action->getYScale())));
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
									soldier);
		}
	}

	init();
}

/**
 * Reorders a soldier down.
 * @param action - pointer to an Action
 */
void CraftArmorState::lstRightArrowClick(Action* action)
{
	_base->setRecallRow(
					REC_SOLDIER,
					_lstSoldiers->getScroll());

	const size_t
		qtySoldiers = _base->getSoldiers()->size(),
		row = _lstSoldiers->getSelectedRow();

	if (qtySoldiers > 0
		&& row < qtySoldiers - 1)
	{
		Soldier* const soldier = _base->getSoldiers()->at(row);

		if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
		{
			_base->getSoldiers()->at(row) = _base->getSoldiers()->at(row + 1);
			_base->getSoldiers()->at(row + 1) = soldier;

			if (row != _lstSoldiers->getVisibleRows() - 1 + _lstSoldiers->getScroll())
			{
				SDL_WarpMouse(
						static_cast<Uint16>(action->getLeftBlackBand() + action->getXMouse()),
						static_cast<Uint16>(action->getTopBlackBand() + action->getYMouse()
												+ static_cast<int>(8. * action->getYScale())));
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
									soldier);
		}
	}

	init();
}

}
