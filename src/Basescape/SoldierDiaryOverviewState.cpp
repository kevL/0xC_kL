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

#include "SoldierDiaryOverviewState.h"

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

#include "../Ruleset/RuleInterface.h"
#include "../Ruleset/Ruleset.h"

#include "../Savegame/Base.h"
//#include "../Savegame/SavedGame.h"
//#include "../Savegame/Soldier.h"
#include "../Savegame/SoldierDead.h"
#include "../Savegame/SoldierDiary.h"
#include "../Savegame/TacticalStatistics.h"


namespace OpenXcom
{

/**
 * The primary diary-screen that shows all the missions that a Soldier has had.
 * @param base			- pointer to the Base to get info from
 * @param solId			- soldier-ID to show info for
 * @param solInfo		- pointer to the SoldierInfo screen
 * @param solInfoDead	- pointer to the SoldierInfoDead screen
 */
SoldierDiaryOverviewState::SoldierDiaryOverviewState(
		Base* const base,
		size_t solId,
		SoldierInfoState* const solInfo,
		SoldierInfoDeadState* const solInfoDead)
	:
		_base(base),
		_solId(solId),
		_solInfo(solInfo),
		_solInfoDead(solInfoDead),
		_recall(0u)
{
	if (_base != nullptr)
	{
		_listBase = _base->getSoldiers();
		_listDead = nullptr;
		_rows = _listBase->size();
	}
	else
	{
		_listDead = _game->getSavedGame()->getDeadSoldiers();
		_listBase = nullptr;
		_rows = _listDead->size();
	}

	_window			= new Window(this);

	_txtTitle		= new Text(310, 16, 5, 8);
	_txtBaseLabel	= new Text(310,  9, 5, 25);

	_btnPrev		= new TextButton(28, 14,   8, 8);
	_btnNext		= new TextButton(28, 14, 284, 8);

	_txtMissionId	= new Text(30, 9,  16, 36);
	_txtLocation	= new Text(74, 9,  46, 36);
	_txtStatus		= new Text(98, 9, 120, 36);
	_txtDate		= new Text(83, 9, 218, 36);

	_lstDiary		= new TextList(285, 129, 16, 44);

	_btnMissions	= new TextButton(70, 16,   8, 177);
	_btnKills		= new TextButton(70, 16,  86, 177);
	_btnAwards		= new TextButton(70, 16, 164, 177);
	_btnOk			= new TextButton(70, 16, 242, 177);

	setInterface("awards");

	add(_window,		"window",	"awards");
	add(_txtTitle,		"title",	"awards");
	add(_txtBaseLabel,	"title",	"awards");

	add(_btnPrev,		"button",	"awards");
	add(_btnNext,		"button",	"awards");

	add(_txtMissionId,	"window",	"awards");
	add(_txtLocation,	"window",	"awards");
	add(_txtStatus,		"window",	"awards");
	add(_txtDate,		"window",	"awards");

	add(_lstDiary,		"list",		"awards");

	add(_btnMissions,	"button2",	"awards");
	add(_btnKills,		"button2",	"awards");
	add(_btnAwards,		"button2",	"awards");
	add(_btnOk,			"button2",	"awards");

	centerSurfaces();


	_window->setBackground(_game->getResourcePack()->getSurface("BACK02.SCR"));

	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setBig();

	if (_listBase != nullptr)
	{
		_txtBaseLabel->setAlign(ALIGN_CENTER);
		_txtBaseLabel->setText(_base->getLabel());

		if (_rows > 1u)
		{
			_btnPrev->setText(L"<");
			_btnPrev->onMouseClick(		static_cast<ActionHandler>(&SoldierDiaryOverviewState::btnPrevClick));
			_btnPrev->onKeyboardPress(	static_cast<ActionHandler>(&SoldierDiaryOverviewState::btnPrevClick),
										Options::keyBattlePrevUnit);

			_btnNext->setText(L">");
			_btnNext->onMouseClick(		static_cast<ActionHandler>(&SoldierDiaryOverviewState::btnNextClick));
			_btnNext->onKeyboardPress(	static_cast<ActionHandler>(&SoldierDiaryOverviewState::btnNextClick),
										Options::keyBattleNextUnit);
		}
		else
		{
			_btnPrev->setVisible(false);
			_btnNext->setVisible(false);
		}
	}
	else // list is reversed in the Memorial.
	{
		_txtBaseLabel->setVisible(false);

		if (_rows > 1u)
		{
			_btnPrev->setText(L"<");
			_btnPrev->onMouseClick(		static_cast<ActionHandler>(&SoldierDiaryOverviewState::btnNextClick));
			_btnPrev->onKeyboardPress(	static_cast<ActionHandler>(&SoldierDiaryOverviewState::btnNextClick),
										Options::keyBattlePrevUnit);

			_btnNext->setText(L">");
			_btnNext->onMouseClick(		static_cast<ActionHandler>(&SoldierDiaryOverviewState::btnPrevClick));
			_btnNext->onKeyboardPress(	static_cast<ActionHandler>(&SoldierDiaryOverviewState::btnPrevClick),
										Options::keyBattleNextUnit);
		}
		else
		{
			_btnPrev->setVisible(false);
			_btnNext->setVisible(false);
		}
	}

	_txtMissionId->setText(tr("STR_ID"));
	_txtLocation->setText(tr("STR_LOCATION"));
	_txtStatus->setText(tr("STR_STATUS"));
	_txtDate->setText(tr("STR_DATE_MISSION"));

	Uint8 color (static_cast<Uint8>(_game->getRuleset()->getInterface("awards")->getElement("list")->color2));

	_lstDiary->setColumns(6, 30,74,98,25,22,30);
	_lstDiary->setArrowColor(color);
	_lstDiary->setBackground(_window);
	_lstDiary->setSelectable();
	_lstDiary->setMargin();
	_lstDiary->onMouseClick(static_cast<ActionHandler>(&SoldierDiaryOverviewState::lstMissionInfoClick));


	_btnMissions->setText(tr("STR_MISSIONS_UC"));
	_btnMissions->onMouseClick(		static_cast<ActionHandler>(&SoldierDiaryOverviewState::btnMissionsClick));
	_btnMissions->onKeyboardPress(	static_cast<ActionHandler>(&SoldierDiaryOverviewState::btnMissionsClick),
									SDLK_m);

	_btnKills->setText(tr("STR_KILLS_UC"));
	_btnKills->onMouseClick(	static_cast<ActionHandler>(&SoldierDiaryOverviewState::btnKillsClick));
	_btnKills->onKeyboardPress(	static_cast<ActionHandler>(&SoldierDiaryOverviewState::btnKillsClick),
								SDLK_k);

	_btnAwards->setText(tr("STR_AWARDS_UC"));
	_btnAwards->onMouseClick(	static_cast<ActionHandler>(&SoldierDiaryOverviewState::btnMedalsClick));
	_btnAwards->onKeyboardPress(static_cast<ActionHandler>(&SoldierDiaryOverviewState::btnMedalsClick),
								SDLK_a);
//	_btnAwards->setVisible(_game->getRuleset()->getAwardsList().empty() == false);

	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick(	static_cast<ActionHandler>(&SoldierDiaryOverviewState::btnOkClick));
	_btnOk->onKeyboardPress(static_cast<ActionHandler>(&SoldierDiaryOverviewState::btnOkClick),
							Options::keyOk);
	_btnOk->onKeyboardPress(static_cast<ActionHandler>(&SoldierDiaryOverviewState::btnOkClick),
							Options::keyOkKeypad);
	_btnOk->onKeyboardPress(static_cast<ActionHandler>(&SoldierDiaryOverviewState::btnOkClick),
							Options::keyCancel);
}

/**
 * dTor.
 */
SoldierDiaryOverviewState::~SoldierDiaryOverviewState()
{}

/**
 * Clears and reinitializes the list of missions for each Soldier.
 */
void SoldierDiaryOverviewState::init()
{
	State::init();

	SoldierDiary* diary (nullptr);

//	if (_solId >= _rows) // safety.
//		_solId = 0;

	if (_listBase != nullptr)
	{
		const Soldier* const sol (_listBase->at(_solId));
		diary = sol->getDiary();
		_txtTitle->setText(sol->getLabel());
	}
	else
	{
		const SoldierDead* const solDead (_listDead->at(_solId));
		diary = solDead->getDiary();
		_txtTitle->setText(solDead->getLabel());
	}


	_lstDiary->clearList();

//	if (diary == nullptr) return; // safety.


	const std::vector<TacticalStatistics*>& tacticals (_game->getSavedGame()->getTacticalStatistics());
	for (std::vector<TacticalStatistics*>::const_reverse_iterator
			rit = tacticals.rbegin();
			rit != tacticals.rend();
			++rit)
	{
		const int missionId ((*rit)->id);

		for (std::vector<int>::const_iterator
				j = diary->getTacticalIdList().begin();
				j != diary->getTacticalIdList().end();
				++j)
		{
			if (*j == missionId) // This mission is in the Soldier's vector of missions.
			{
				std::wstring
					wst1,
					wst2;

				if ((*rit)->country == "STR_UNKNOWN")
					wst1 = tr((*rit)->region);
				else
					wst1 = tr((*rit)->country);

				if ((*rit)->success == true)
					wst2 = tr("STR_VICTORY");
				else
					wst2 = tr("STR_DEFEAT");

				wst2 += L" - ";
				wst2 += tr((*rit)->rating); // NOTE: This could take a performance hit vs. using std::wostringstream.

				_lstDiary->addRow(
								6,
								Text::intWide(missionId).c_str(),
								wst1.c_str(),
								wst2.c_str(),
								(*rit)->timeStat.getDayString(_game->getLanguage()).c_str(),
								tr((*rit)->timeStat.getMonthString()).c_str(),
								Text::intWide((*rit)->timeStat.getYear()).c_str());
				break;
			}
		}
	}

	_lstDiary->scrollTo(_recall);
}

/**
 * Sets the current Soldier by his/her ID.
 * @param solId - the soldier-id
 */
void SoldierDiaryOverviewState::setSoldierId(size_t solId)
{
	_solId = solId;
}

/**
 * Exits to the previous screen.
 * @param action - pointer to an Action
 */
void SoldierDiaryOverviewState::btnOkClick(Action*)
{
	if (_listBase != nullptr)
		_solInfo->setSoldierId(_solId);
	else
		_solInfoDead->setSoldierId(_solId);

	_game->popState();
}

/**
 * Opens the Soldier's kills-screen.
 * @param action - pointer to an Action
 */
void SoldierDiaryOverviewState::btnKillsClick(Action*)
{
	_recall = _lstDiary->getScroll();
	_game->pushState(new SoldierDiaryPerformanceState(
												_base,
												_solId,
												this,
												DIARY_KILLS));
}

/**
 * Opens the Soldier's missions-screen.
 * @param action - pointer to an Action
 */
void SoldierDiaryOverviewState::btnMissionsClick(Action*)
{
	_recall = _lstDiary->getScroll();
	_game->pushState(new SoldierDiaryPerformanceState(
												_base,
												_solId,
												this,
												DIARY_MISSIONS));
}

/**
 * Opens the Soldier's medals-screen.
 * @param action - pointer to an Action
 */
void SoldierDiaryOverviewState::btnMedalsClick(Action*)
{
	_recall = _lstDiary->getScroll();
	_game->pushState(new SoldierDiaryPerformanceState(
												_base,
												_solId,
												this,
												DIARY_MEDALS));
}

/**
 * Goes to the previous Soldier.
 * @param action - pointer to an Action
 */
void SoldierDiaryOverviewState::btnPrevClick(Action*)
{
	_recall = 0u;

	if (_solId == 0u)
		_solId = _rows - 1u;
	else
		--_solId;

	init();
}

/**
 * Goes to the next Soldier.
 * @param action - pointer to an Action
 */
void SoldierDiaryOverviewState::btnNextClick(Action*)
{
	_recall = 0u;

	if (++_solId == _rows)
		_solId = 0u;

	init();
}

/**
 * Shows the selected Soldier's mission-info.
 * @param action - pointer to an Action
 */
void SoldierDiaryOverviewState::lstMissionInfoClick(Action*)
{
	_recall = _lstDiary->getScroll();

	const size_t r (_lstDiary->getRows() - _lstDiary->getSelectedRow() - 1u);
	_game->pushState(new SoldierDiaryMissionState(
												_base,
												_solId,
												r));
}

}
