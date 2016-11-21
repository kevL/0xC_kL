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

#include "SoldiersState.h"

//#include <sstream>
//#include <string>
//#include <climits>

#include "CraftArmorState.h"
#include "PsiTrainingState.h"
#include "SoldierInfoState.h"

#include "../Battlescape/BattlescapeGenerator.h"
#include "../Battlescape/InventoryState.h"

#include "../Engine/Action.h"
#include "../Engine/Game.h"
#include "../Engine/LocalizedText.h"
//#include "../Engine/Logger.h"
#include "../Engine/Options.h"
#include "../Engine/Screen.h"
#include "../Engine/Sound.h"

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextList.h"
#include "../Interface/Window.h"

#include "../Resource/XcomResourcePack.h"

//#include "../Ruleset/RuleSoldier.h" // minPsi in btnSortClick()

#include "../Savegame/Base.h"
#include "../Savegame/SavedBattleGame.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/Soldier.h"


#include "../Ruleset/RuleAward.h"	// debug SoldierDiary.
#include "../Ruleset/Ruleset.h"		// debug SoldierDiary.


namespace OpenXcom
{

/**
 * Initializes all the elements in the SoldiersState screen.
 * @param base - pointer to the Base to get info from
 */
SoldiersState::SoldiersState(Base* base)
	:
		_base(base)
{
	_window			= new Window(this, 320, 200);

	_txtTitle		= new Text(300, 17,  10, 11);
	_txtBaseLabel	= new Text( 80,  9,  16, 11);
	_txtSoldiers	= new Text( 20,  9, 284, 11);

	_txtName		= new Text(117, 9,  16, 31);
	_txtRank		= new Text( 92, 9, 133, 31);
	_txtCraft		= new Text( 82, 9, 226, 31);

	_lstSoldiers	= new TextList(293, 129, 8, 42);

	_btnSort		= new TextButton(56, 16,  10, 177);
	_btnPsi			= new TextButton(56, 16,  71, 177);
	_btnArmor		= new TextButton(56, 16, 132, 177);
	_btnEquip		= new TextButton(56, 16, 193, 177);
	_btnOk			= new TextButton(56, 16, 254, 177);

	setInterface("soldierList");


	add(_window,		"window",	"soldierList");
	add(_txtTitle,		"text1",	"soldierList");
	add(_txtBaseLabel,	"text2",	"soldierList");
	add(_txtSoldiers,	"text2",	"soldierList");
	add(_txtName,		"text2",	"soldierList");
	add(_txtRank,		"text2",	"soldierList");
	add(_txtCraft,		"text2",	"soldierList");
	add(_lstSoldiers,	"list",		"soldierList");
	add(_btnSort,		"button",	"soldierList");
	add(_btnPsi,		"button",	"soldierList");
	add(_btnArmor,		"button",	"soldierList");
	add(_btnEquip,		"button",	"soldierList");
	add(_btnOk,			"button",	"soldierList");

	centerSurfaces();


	_window->setBackground(_game->getResourcePack()->getSurface("BACK02.SCR"));

	_txtTitle->setText(tr("STR_SOLDIER_LIST"));
	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setBig();

	_txtBaseLabel->setText(_base->getLabel());

	_txtSoldiers->setAlign(ALIGN_RIGHT);

	_btnSort->setText(tr("STR_SORT"));
	_btnSort->onMouseClick(	static_cast<ActionHandler>(&SoldiersState::btnSortClick),
							SDL_BUTTON_LEFT);
	_btnSort->onMouseClick(	static_cast<ActionHandler>(&SoldiersState::btnAutoStatClick),
							SDL_BUTTON_RIGHT);

	_btnPsi->setText(tr("STR_PSIONIC_TRAINING"));
	_btnPsi->onMouseClick(static_cast<ActionHandler>(&SoldiersState::btnPsiTrainingClick));
	_btnPsi->setVisible(_base->hasPsiLabs() == true);

	_btnArmor->setText(tr("STR_ARMOR"));
	_btnArmor->onMouseClick(static_cast<ActionHandler>(&SoldiersState::btnArmorClick));

	_btnEquip->setText(tr("STR_INVENTORY"));
	_btnEquip->onMouseClick(static_cast<ActionHandler>(&SoldiersState::btnEquipClick));
	_btnEquip->setVisible(_base->getAvailableSoldiers() != 0);

	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick(	static_cast<ActionHandler>(&SoldiersState::btnOkClick));
	_btnOk->onKeyboardPress(static_cast<ActionHandler>(&SoldiersState::btnOkClick),
							Options::keyOk);
	_btnOk->onKeyboardPress(static_cast<ActionHandler>(&SoldiersState::btnOkClick),
							Options::keyOkKeypad);
	_btnOk->onKeyboardPress(static_cast<ActionHandler>(&SoldiersState::btnOkClick),
							Options::keyCancel);

	_txtName->setText(tr("STR_NAME_UC"));
	_txtRank->setText(tr("STR_RANK"));
	_txtCraft->setText(tr("STR_CRAFT"));

	_lstSoldiers->setColumns(3, 117,93,71);
	_lstSoldiers->setArrow(193, ARROW_VERTICAL);
	_lstSoldiers->setBackground(_window);
	_lstSoldiers->setSelectable();
	_lstSoldiers->onMousePress(		static_cast<ActionHandler>(&SoldiersState::lstSoldiersPress));
	_lstSoldiers->onLeftArrowClick(	static_cast<ActionHandler>(&SoldiersState::lstLeftArrowClick));
	_lstSoldiers->onRightArrowClick(static_cast<ActionHandler>(&SoldiersState::lstRightArrowClick));


	// DEBUG for Soldier Diary:
	// This prints every Award's criteria to the logfile.
//	int
//		iter1 (0),
//		iter2,
//		iter3;
//	const std::map<std::string, const RuleAward*>& allAwards (_game->getRuleset()->getAwardsList()); // loop over all possible RuleAwards.
//	for (std::map<std::string, const RuleAward*>::const_iterator
//			i = allAwards.begin();
//			i != allAwards.end();
//			++i)
//	{
//		Log(LOG_INFO) << "";
//		Log(LOG_INFO) << "[" << iter1++ << "] " << (*i).first;
//
//		iter2 = 0;
//		const std::vector<std::vector<std::pair<int, std::vector<std::string>>>>* killCriteriaList ((*i).second->getKillCriteria()); // fetch the killCriteria list.
//		for (std::vector<std::vector<std::pair<int, std::vector<std::string>>>>::const_iterator // loop over the OR vectors.
//				orCriteria = killCriteriaList->begin();
//				orCriteria != killCriteriaList->end();
//				++orCriteria)
//		{
//			Log(LOG_INFO) << ". [" << iter2++ << "] orCriteria";
//
//			iter3 = 0;
//			for (std::vector<std::pair<int, std::vector<std::string>>>::const_iterator // loop over the AND vectors.
//					andCriteria = orCriteria->begin();
//					andCriteria != orCriteria->end();
//					++andCriteria)
//			{
//				Log(LOG_INFO) << ". . [" << iter3++ << "] andCriteria";
//
//				for (std::vector<std::string>::const_iterator
//						detail = andCriteria->second.begin();
//						detail != andCriteria->second.end();
//						++detail)
//				{
//					Log(LOG_INFO) << ". . . " << (*andCriteria).first << " - " << *detail;
//				}
//			}
//		}
//	}
}

/**
 * dTor.
 */
SoldiersState::~SoldiersState()
{}

/**
 * Updates the soldiers list after going to other screens.
 */
void SoldiersState::init()
{
	State::init();

	// Reset stuff when coming back from pre-battle Inventory.
	if (_game->getSavedGame()->getBattleSave() != nullptr)
	{
		_game->getSavedGame()->setBattleSave();
		_base->setTactical(false);
	}

	std::wostringstream woststr; // update in case soldier was told to GTFO.
	woststr << _base->getTotalSoldiers();
	_txtSoldiers->setText(woststr.str());

	_lstSoldiers->clearList();
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
		{
			_lstSoldiers->setRowColor(r, _lstSoldiers->getSecondaryColor());
			if ((*i)->getSickbay() != 0)
			{
				Uint8 color;
				const int pct ((*i)->getPctWounds());
				if		(pct > 50)	color = ORANGE;
				else if	(pct > 10)	color = YELLOW;
				else				color = GREEN;

				_lstSoldiers->setCellColor(r, 2u, color, true);
			}
		}
	}
	_lstSoldiers->scrollTo(_base->getRecallRow(RCL_SOLDIER));
	_lstSoldiers->draw();
}

/**
 * Exits to the previous screen.
 * @param action - pointer to an Action
 */
void SoldiersState::btnOkClick(Action*)
{
	_base->setRecallRow(
					RCL_SOLDIER,
					_lstSoldiers->getScroll());
	_game->popState();
}

/**
 * Goes to the Select Armor screen.
 * @param action - pointer to an Action
 */
void SoldiersState::btnArmorClick(Action*)
{
	_base->setRecallRow(
					RCL_SOLDIER,
					_lstSoldiers->getScroll());
	_game->pushState(new CraftArmorState(_base));
}

/**
 * Displays the inventory screen for the soldiers at Base.
 * @param action - pointer to an Action
 */
void SoldiersState::btnEquipClick(Action*)
{
	_base->setRecallRow(
					RCL_SOLDIER,
					_lstSoldiers->getScroll());

	SavedBattleGame* const battleSave (new SavedBattleGame(_game->getSavedGame()));
	_game->getSavedGame()->setBattleSave(battleSave);

	BattlescapeGenerator bGen = BattlescapeGenerator(_game);
	bGen.runFakeInventory(nullptr, _base);

	_game->getScreen()->clear();
	_game->pushState(new InventoryState());
}

/**
 * Opens the Psionic Training screen.
 * @param action - pointer to an Action
 */
void SoldiersState::btnPsiTrainingClick(Action*)
{
	_base->setRecallRow(
					RCL_SOLDIER,
					_lstSoldiers->getScroll());
	_game->pushState(new PsiTrainingState(_base));
}

/* EG, functor; http://stackoverflow.com/questions/26844983/sort-a-pair-vector-in-c
// (see also, http://stackoverflow.com/questions/1380463/sorting-a-vector-of-custom-objects)
struct sort_second
{
	inline const bool operator()(const std::pair<int,int>& left, const std::pair<int,int>& right) const
	{
		return left.second < right.second;
	}
};
// call w/
std::sort(vPair.begin(), vPair.end(), sort_second()); */

/**
 * Sorts the soldiers list.
 * @param action - pointer to an Action
 */
void SoldiersState::btnSortClick(Action*)
{
	_base->setRecallRow(
					RCL_SOLDIER,
					_lstSoldiers->getScroll());
	_base->sortSoldiers();

	init();
}

/**
 * Autostats the soldiers list.
 * @note Right-click on the Sort button.
 * @param action - pointer to an Action
 */
void SoldiersState::btnAutoStatClick(Action*)
{
	_base->setRecallRow(
					RCL_SOLDIER,
					_lstSoldiers->getScroll());

	for (std::vector<Soldier*>::const_iterator
			i = _base->getSoldiers()->begin();
			i != _base->getSoldiers()->end();
			++i)
	{
		(*i)->autoStat();
	}
	init();
}

/**
 * Shows the selected Soldier's info.
 * @param action - pointer to an Action
 */
void SoldiersState::lstSoldiersPress(Action* action)
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
 * Reorders a Soldier up.
 * @param action - pointer to an Action
 */
void SoldiersState::lstLeftArrowClick(Action* action)
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
 * Reorders a Soldier down.
 * @param action - pointer to an Action
 */
void SoldiersState::lstRightArrowClick(Action* action)
{
	_base->setRecallRow(
					RCL_SOLDIER,
					_lstSoldiers->getScroll());

	const size_t qtySoldiers (_base->getSoldiers()->size());
	if (qtySoldiers > 0u)
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

}
