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

#include "SoldierDiaryOverviewState.h"

//#include <cstddef> // nullptr (for NB code-assistant only)
//#include <string>
//#include <vector>

#include "SoldierDiaryMissionState.h"
#include "SoldierDiaryPerformanceState.h"
#include "SoldierInfoState.h"
#include "SoldierInfoDeadState.h"

#include "../Engine/Game.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Options.h"

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextList.h"
#include "../Interface/Window.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/Ruleset.h"

#include "../Savegame/Base.h"
#include "../Savegame/MissionStatistics.h"
//#include "../Savegame/SavedGame.h"
//#include "../Savegame/Soldier.h"
#include "../Savegame/SoldierDead.h"
#include "../Savegame/SoldierDiary.h"


namespace OpenXcom
{

/**
 * Diary screen that shows all the missions a soldier has.
 * @param base					- pointer to the Base to get info from
 * @param soldierId				- ID of the selected soldier
 * @param soldierInfoState		- pointer to the Soldier Info screen
 * @param soldierInfoDeadState	- pointer to the Dead Soldier Info screen
 */
SoldierDiaryOverviewState::SoldierDiaryOverviewState(
		Base* const base,
		size_t soldierId,
		SoldierInfoState* soldierInfoState,
		SoldierInfoDeadState* soldierInfoDeadState)
	:
		_base(base),
		_soldierId(soldierId),
		_soldierInfoState(soldierInfoState),
		_soldierInfoDeadState(soldierInfoDeadState),
		_curRow(0)
{
	if (_base == nullptr)
	{
		_listDead = _game->getSavedGame()->getDeadSoldiers();
		_list = nullptr;
	}
	else
	{
		_list = _base->getSoldiers();
		_listDead = nullptr;
	}

	_window			= new Window(this, 320, 200);

	_txtTitle		= new Text(310, 16, 5, 8);
	_txtBaseLabel	= new Text(310,  9, 5, 25);

	_btnPrev		= new TextButton(28, 14,   8, 8);
	_btnNext		= new TextButton(28, 14, 284, 8);

	_txtLocation	= new Text( 94, 9,  16, 36);
	_txtStatus		= new Text(108, 9, 110, 36);
	_txtDate		= new Text( 90, 9, 218, 36);

	_lstDiary		= new TextList(285, 129, 16, 44);

	_btnMissions	= new TextButton(70, 16,   8, 177);
	_btnKills		= new TextButton(70, 16,  86, 177);
	_btnAwards		= new TextButton(70, 16, 164, 177);
	_btnOk			= new TextButton(70, 16, 242, 177);

	setPalette("PAL_BASESCAPE");

	add(_window);
	add(_txtTitle);
	add(_txtBaseLabel);

	add(_btnPrev);
	add(_btnNext);

	add(_txtLocation);
	add(_txtStatus);
	add(_txtDate);

	add(_lstDiary);

	add(_btnMissions);
	add(_btnKills);
	add(_btnAwards);
	add(_btnOk);

	centerAllSurfaces();


	_window->setColor(PINK);
	_window->setBackground(_game->getResourcePack()->getSurface("BACK02.SCR"));

	_txtTitle->setColor(BLUE);
	_txtTitle->setBig();
	_txtTitle->setAlign(ALIGN_CENTER);

	if (_base != nullptr)
	{
		_txtBaseLabel->setColor(BLUE);
		_txtBaseLabel->setAlign(ALIGN_CENTER);
		_txtBaseLabel->setText(_base->getName(_game->getLanguage()));
	}
	else
		_txtBaseLabel->setVisible(false);

	_btnPrev->setColor(PURPLE);
	_btnPrev->setText(L"<");

	_btnNext->setColor(PURPLE);
	_btnNext->setText(L">");

	if (_base == nullptr)
	{
		_btnPrev->onMouseClick((ActionHandler)& SoldierDiaryOverviewState::btnNextClick);
		_btnPrev->onKeyboardPress(
							(ActionHandler)& SoldierDiaryOverviewState::btnNextClick,
							Options::keyBattlePrevUnit);
		_btnNext->onMouseClick((ActionHandler)& SoldierDiaryOverviewState::btnPrevClick);
		_btnNext->onKeyboardPress(
						(ActionHandler)& SoldierDiaryOverviewState::btnPrevClick,
						Options::keyBattleNextUnit);
	}
	else
	{
		_btnPrev->onMouseClick((ActionHandler)& SoldierDiaryOverviewState::btnPrevClick);
		_btnPrev->onKeyboardPress(
						(ActionHandler)& SoldierDiaryOverviewState::btnPrevClick,
						Options::keyBattlePrevUnit);
		_btnNext->onMouseClick((ActionHandler)& SoldierDiaryOverviewState::btnNextClick);
		_btnNext->onKeyboardPress(
						(ActionHandler)& SoldierDiaryOverviewState::btnNextClick,
						Options::keyBattleNextUnit);
	}

	_txtLocation->setText(tr("STR_LOCATION"));
	_txtLocation->setColor(PINK);

	_txtStatus->setText(tr("STR_STATUS"));
	_txtStatus->setColor(PINK);

	_txtDate->setText(tr("STR_DATE_MISSION"));
	_txtDate->setColor(PINK);

	_lstDiary->setColumns(5, 94,108,25,22,30);
	_lstDiary->setColor(WHITE);
	_lstDiary->setArrowColor(PINK);
	_lstDiary->setBackground(_window);
	_lstDiary->setSelectable();
	_lstDiary->setMargin();
	_lstDiary->onMouseClick((ActionHandler)& SoldierDiaryOverviewState::lstDiaryInfoClick);


	_btnMissions->setText(tr("STR_MISSIONS_UC"));
	_btnMissions->setColor(BLUE);
	_btnMissions->onMouseClick((ActionHandler)& SoldierDiaryOverviewState::btnMissionsClick);

	_btnKills->setText(tr("STR_KILLS_UC"));
	_btnKills->setColor(BLUE);
	_btnKills->onMouseClick((ActionHandler)& SoldierDiaryOverviewState::btnKillsClick);

	_btnAwards->setText(tr("STR_AWARDS_UC"));
	_btnAwards->setColor(BLUE);
	_btnAwards->onMouseClick((ActionHandler)& SoldierDiaryOverviewState::btnMedalsClick);
//	_btnAwards->setVisible(_game->getRuleset()->getAwardsList().empty() == false); // safety.

	_btnOk->setText(tr("STR_OK"));
	_btnOk->setColor(BLUE);
	_btnOk->onMouseClick((ActionHandler)& SoldierDiaryOverviewState::btnOkClick);
	_btnOk->onKeyboardPress(
					(ActionHandler)& SoldierDiaryOverviewState::btnOkClick,
					Options::keyOk);
	_btnOk->onKeyboardPress(
					(ActionHandler)& SoldierDiaryOverviewState::btnOkClick,
					Options::keyOkKeypad);
	_btnOk->onKeyboardPress(
					(ActionHandler)& SoldierDiaryOverviewState::btnOkClick,
					Options::keyCancel);
}

/**
 * dTor.
 */
SoldierDiaryOverviewState::~SoldierDiaryOverviewState()
{}

/**
 * Clears and reinitializes the list of missions for the soldier.
 */
void SoldierDiaryOverviewState::init()
{
	State::init();

	SoldierDiary* diary = nullptr;

	if (_base == nullptr)
	{
		if (_soldierId >= _listDead->size())
			_soldierId = 0;

		const SoldierDead* const solDead = _listDead->at(_soldierId);
		diary = solDead->getDiary();

		_txtTitle->setText(solDead->getName());
	}
	else
	{
		if (_soldierId >= _list->size())
			_soldierId = 0;

		const Soldier* const sol = _list->at(_soldierId);
		diary = sol->getDiary();

		_txtTitle->setText(sol->getName());
	}


	_lstDiary->clearList();

	if (diary == nullptr) // safety.
		return;


	const std::vector<MissionStatistics*>* const missionStatistics = _game->getSavedGame()->getMissionStatistics();
	for (std::vector<MissionStatistics*>::const_reverse_iterator
			i = missionStatistics->rbegin();
			i != missionStatistics->rend();
			++i)
	{
		const int missionId = (*i)->id;

		for (std::vector<int>::const_iterator
				j = diary->getMissionIdList().begin();
				j != diary->getMissionIdList().end();
				++j)
		{
			if (*j == missionId) // This mission is in the soldier's vector of missions.
			{
				std::wostringstream
					location,
					status,
					year;

				if ((*i)->country == "STR_UNKNOWN")
					location << tr((*i)->region);
				else
					location << tr((*i)->country);

				if ((*i)->success == true)
					status << tr("STR_MISSION_WIN");
				else
					status << tr("STR_MISSION_LOSS");

				status << " - " << tr((*i)->rating);

				year << (*i)->timeStat.getYear();
				_lstDiary->addRow(
								5,
								location.str().c_str(),
								status.str().c_str(),
								(*i)->timeStat.getDayString(_game->getLanguage()).c_str(),
								tr((*i)->timeStat.getMonthString()).c_str(),
								year.str().c_str());
				break;
			}
		}
	}

	_lstDiary->scrollTo(_curRow);
}

/**
 * Sets the soldier's ID.
 */
void SoldierDiaryOverviewState::setSoldierId(size_t soldierId)
{
	_soldierId = soldierId;
}

/**
 * Returns to the previous screen.
 * @param action - pointer to an Action
 */
void SoldierDiaryOverviewState::btnOkClick(Action*)
{
	if (_base == nullptr)
		_soldierInfoDeadState->setSoldierId(_soldierId);
	else
		_soldierInfoState->setSoldierId(_soldierId);

	_game->popState();
}

/**
 * Returns to the previous screen.
 * @param action - pointer to an Action
 */
void SoldierDiaryOverviewState::btnKillsClick(Action*)
{
	_curRow = _lstDiary->getScroll();
	_game->pushState(new SoldierDiaryPerformanceState(
												_base,
												_soldierId,
												this,
												DIARY_KILLS));
}

/**
 * Returns to the previous screen.
 * @param action - pointer to an Action
 */
void SoldierDiaryOverviewState::btnMissionsClick(Action*)
{
	_curRow = _lstDiary->getScroll();
	_game->pushState(new SoldierDiaryPerformanceState(
												_base,
												_soldierId,
												this,
												DIARY_MISSIONS));
}

/**
 * Returns to the previous screen.
 * @param action - pointer to an Action
 */
void SoldierDiaryOverviewState::btnMedalsClick(Action*)
{
	_curRow = _lstDiary->getScroll();
	_game->pushState(new SoldierDiaryPerformanceState(
												_base,
												_soldierId,
												this,
												DIARY_MEDALS));
}

/**
 * Goes to the previous soldier.
 * @param action - pointer to an Action
 */
void SoldierDiaryOverviewState::btnPrevClick(Action*)
{
	_curRow = 0;

	size_t rows;
	if (_base == nullptr)
		rows = _listDead->size();
	else
		rows = _list->size();

	if (_soldierId == 0)
		_soldierId = rows - 1;
	else
		--_soldierId;

	init();
}

/**
 * Goes to the next soldier.
 * @param action - pointer to an Action
 */
void SoldierDiaryOverviewState::btnNextClick(Action*)
{
	_curRow = 0;

	size_t rows;
	if (_base == nullptr)
		rows = _listDead->size();
	else
		rows = _list->size();

	if (++_soldierId >= rows)
		_soldierId = 0;

	init();
}

/**
 * Shows the selected soldier's info.
 * @param action - pointer to an Action
 */
void SoldierDiaryOverviewState::lstDiaryInfoClick(Action*)
{
	_curRow = _lstDiary->getScroll();

	const size_t row = _lstDiary->getRows() - _lstDiary->getSelectedRow() - 1;
	_game->pushState(new SoldierDiaryMissionState(
												_base,
												_soldierId,
												row));
}

}
