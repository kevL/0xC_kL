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

#include "SoldierDiaryMissionState.h"

//#include <cstddef> // nullptr (for NB code-assistant only)
//#include <sstream>
//#include <vector>

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
#include "../Savegame/BattleUnitStatistics.h"
#include "../Savegame/MissionStatistics.h"
//#include "../Savegame/SavedGame.h"
//#include "../Savegame/Soldier.h"
#include "../Savegame/SoldierDead.h"
#include "../Savegame/SoldierDiary.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the Soldier Diary missions-description window.
 * @param base		- pointer to the Base to get info from
 * @param soldierId	- ID of the selected soldier
 * @param rowEntry	- list-row to get mission info from
 */
SoldierDiaryMissionState::SoldierDiaryMissionState(
		Base* const base,
		size_t soldierId,
		size_t rowEntry)
	:
		_base(base),
		_soldierId(soldierId),
		_rowEntry(rowEntry),
		_diary(nullptr)
{
	_fullScreen = false;

	_window			= new Window(this, 292, 141, 14, 34, POPUP_VERTICAL);

	_txtTitle		= new Text(120, 16, 100, 42);
//	_txtMissionId	= new Text(20, 9, 288, 42); // TODO: Adjust that.

	_btnPrev		= new TextButton(24, 14,  76, 42);
	_btnNext		= new TextButton(24, 14, 220, 42);

	_txtMissionType	= new Text(144, 9, 46, 60);
	_txtUFO			= new Text(144, 9, 46, 69);
	_txtScore		= new Text(144, 9, 46, 78);
	_txtDaysWounded	= new Text(144, 9, 46, 87);

	_txtRace		= new Text(80, 9, 188, 60);
	_txtDaylight	= new Text(80, 9, 188, 69);
	_txtKills		= new Text(80, 9, 188, 78);
	_txtPoints		= new Text(80, 9, 188, 87);

	_srfLine		= new Surface(120, 1, 100, 98);
	_srfLineShade	= new Surface(120, 1, 101, 99);

	_lstKills		= new TextList(247, 49, 46, 101);

	_btnOk			= new TextButton(180, 16, 70, 152);

	setInterface("awardsMissionInfo");

	add(_window,			"window",	"awardsMissionInfo");
	add(_txtTitle,			"title",	"awardsMissionInfo");
//	add(_txtMissionId,		"text",		"awardsMissionInfo");
	add(_btnPrev,			"button",	"awardsMissionInfo");
	add(_btnNext,			"button",	"awardsMissionInfo");
	add(_txtMissionType,	"text",		"awardsMissionInfo");
	add(_txtUFO,			"text",		"awardsMissionInfo");
	add(_txtScore,			"text",		"awardsMissionInfo");
	add(_txtPoints,			"text",		"awardsMissionInfo");
	add(_txtRace,			"text",		"awardsMissionInfo");
	add(_txtDaylight,		"text",		"awardsMissionInfo");
	add(_txtKills,			"text",		"awardsMissionInfo");
	add(_txtDaysWounded,	"text",		"awardsMissionInfo");
	add(_srfLine);
	add(_srfLineShade);
	add(_lstKills,			"list",		"awardsMissionInfo");
	add(_btnOk,				"button",	"awardsMissionInfo");

	centerAllSurfaces();


	_window->setBackground(_game->getResourcePack()->getSurface("BACK16.SCR"));

	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setBig();

	_btnPrev->setText(L"<");
	_btnPrev->onMouseClick((ActionHandler)& SoldierDiaryMissionState::btnNextClick); // list is reversed.
	_btnPrev->onKeyboardPress(
			(ActionHandler)& SoldierDiaryMissionState::btnNextClick,
			Options::keyBattlePrevUnit);

	_btnNext->setText(L">");
	_btnNext->onMouseClick((ActionHandler)& SoldierDiaryMissionState::btnPrevClick); // list is reversed.
	_btnNext->onKeyboardPress(
			(ActionHandler)& SoldierDiaryMissionState::btnPrevClick,
			Options::keyBattleNextUnit);

	_color = _game->getRuleset()->getInterface("awardsMissionInfo")->getElement("list")->color2;

	_srfLine	 ->drawLine(0,0, 120,0, _color + 2u);
	_srfLineShade->drawLine(0,0, 120,0, BLACK);

	_lstKills->setArrowColor(_color);
	_lstKills->setColumns(3, 30,100,100);
	_lstKills->setMargin();

	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)& SoldierDiaryMissionState::btnOkClick);
	_btnOk->onKeyboardPress(
			(ActionHandler)& SoldierDiaryMissionState::btnOkClick,
			Options::keyOk);
	_btnOk->onKeyboardPress(
			(ActionHandler)& SoldierDiaryMissionState::btnOkClick,
			Options::keyOkKeypad);
	_btnOk->onKeyboardPress(
			(ActionHandler)& SoldierDiaryMissionState::btnOkClick,
			Options::keyCancel);
}

/**
 * dTor.
 */
SoldierDiaryMissionState::~SoldierDiaryMissionState()
{}

/**
 * Clears the variables and re-initializes the stats for each mission.
 */
void SoldierDiaryMissionState::init()
{
	State::init();

	const std::vector<MissionStatistics*>* const stats (_game->getSavedGame()->getMissionStatistics());
	size_t missionId;
	int daysWounded;

	if (_base != nullptr)
	{
		const std::vector<Soldier*>* const listLive (_base->getSoldiers());

//		if (_soldierId >= listLive->size()) // safety.
//			_soldierId = 0;

		const Soldier* const sol (listLive->at(_soldierId));
		_diary = sol->getDiary();

		missionId = _diary->getMissionIdList().at(_rowEntry);
//		if (missionId > stats->size()) // safety.
//			missionId = 0u;

		daysWounded = stats->at(missionId)->injuryList[sol->getId()];
	}
	else
	{
		const std::vector<SoldierDead*>* const listDead (_game->getSavedGame()->getDeadSoldiers());

//		if (_soldierId >= listDead->size()) // safety.
//			_soldierId = 0;

		const SoldierDead* const solDead (listDead->at(_soldierId));
		_diary = solDead->getDiary();

		missionId = _diary->getMissionIdList().at(_rowEntry);
//		if (missionId > stats->size()) // safety.
//			missionId = 0u;

		daysWounded = stats->at(missionId)->injuryList[solDead->getId()];
	}

	const bool vis (_diary->getMissionTotal() > 1u);
	_btnPrev->setVisible(vis);
	_btnNext->setVisible(vis);

	_txtTitle->setText(tr("STR_MISSION_UC_").arg(missionId));
//	_txtMissionId->setText(Text::intWide(static_cast<int>(missionId)));

	_txtScore->setText(tr("STR_SCORE_VALUE_").arg(stats->at(missionId)->score));
	_txtMissionType->setText(tr("STR_MISSION_TYPE_").arg(tr(stats->at(missionId)->type))); // 'type' was, getMissionTypeLowerCase()

	if (stats->at(missionId)->ufo != "NO_UFO")
	{
		_txtUFO->setVisible();
		_txtUFO->setText(tr("STR_UFO_TYPE_").arg(tr(stats->at(missionId)->ufo)));
	}
	else
		_txtUFO->setVisible(false);

	if (stats->at(missionId)->alienRace != "STR_UNKNOWN")
	{
		_txtRace->setVisible();
		_txtRace->setText(tr("STR_RACE_TYPE_").arg(tr(stats->at(missionId)->alienRace)));
	}
	else
		_txtRace->setVisible(false);

	if (stats->at(missionId)->type != "STR_BASE_DEFENSE"
		&& stats->at(missionId)->type != "STR_ALIEN_BASE_ASSAULT")
	{
		_txtDaylight->setVisible();
		if (stats->at(missionId)->shade < MissionStatistics::NIGHT_SHADE)
			_txtDaylight->setText(tr("STR_DAYLIGHT_TYPE_").arg(tr("STR_DAY")));
		else
			_txtDaylight->setText(tr("STR_DAYLIGHT_TYPE_").arg(tr("STR_NIGHT")));
	}
	else
		_txtDaylight->setVisible(false);

	if (daysWounded != 0)
	{
		_txtDaysWounded->setVisible();
		if (daysWounded == -1)
			_txtDaysWounded->setText(tr("STR_DAYS_WOUNDED_").arg(tr("STR_KIA")).arg(L""));
		else if (daysWounded == -2)
			_txtDaysWounded->setText(tr("STR_DAYS_WOUNDED_").arg(tr("STR_MIA")).arg(L""));
		else
			_txtDaysWounded->setText(tr("STR_DAYS_WOUNDED_").arg(daysWounded).arg(L" dy"));
	}
	else
		_txtDaysWounded->setVisible(false);


	_lstKills->clearList();

	int
		kills  (0),
		points (0);
	size_t row (0);

	for (std::vector<BattleUnitKill*>::const_iterator
			i = _diary->getKills().begin();
			i != _diary->getKills().end();
			++i)
	{
		if ((*i)->_mission == static_cast<int>(missionId))
		{
			++kills;
			points += (*i)->_points;

			std::wostringstream
				woststr1,
				woststr2;

			woststr1 << tr((*i)->_race) << L" " << tr((*i)->_rank);

			if ((*i)->_status == STATUS_DEAD)
				woststr2 << tr("STR_KILLED");
			else
				woststr2 << tr("STR_STUNNED");

			_lstKills->addRow(
							3,
							woststr2.str().c_str(),
							woststr1.str().c_str(),
							tr((*i)->_weapon).c_str());
			_lstKills->setCellColor(row++, 0, _color);
		}
	}

	if (kills != 0)
	{
		_txtKills->setVisible();
		_txtKills->setText(tr("STR_TAKEDOWNS_").arg(kills));
	}
	else
		_txtKills->setVisible(false);

	if (points != 0)
	{
		_txtPoints->setVisible();
		_txtPoints->setText(tr("STR_POINTS_VALUE_").arg(points));
	}
	else
		_txtPoints->setVisible(false);
}

/**
 * Goes to the previous mission.
 * @param action - pointer to an Action
 */
void SoldierDiaryMissionState::btnPrevClick(Action*)
{
	if (_rowEntry == 0)
		_rowEntry = _diary->getMissionTotal() - 1;
	else
		--_rowEntry;

	init();
}

/**
 * Goes to the next mission.
 * @param action - pointer to an Action
 */
void SoldierDiaryMissionState::btnNextClick(Action*)
{
	if (++_rowEntry >= _diary->getMissionTotal())
		_rowEntry = 0;

	init();
}

/**
 * Returns to the previous screen.
 * @param action - pointer to an Action
 */
void SoldierDiaryMissionState::btnOkClick(Action*)
{
	_game->popState();
}

}
