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

#include "SoldierDiaryPerformanceState.h"

//#include <string>

#include "SoldierDiaryOverviewState.h"

#include "../Engine/Action.h"
#include "../Engine/Game.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Options.h"
#include "../Engine/SurfaceSet.h"

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextList.h"
#include "../Interface/Window.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/RuleAward.h"
#include "../Ruleset/RuleInterface.h"
#include "../Ruleset/Ruleset.h"

#include "../Savegame/Base.h"
//#include "../Savegame/Soldier.h"
#include "../Savegame/SoldierDead.h"
#include "../Savegame/SoldierDiary.h"

#include "../Ufopaedia/Ufopaedia.h"


namespace OpenXcom
{

/**
 * The diary-screen that displays/hides the several screens that document a
 * Soldier's performance.
 * @param base		- pointer to the Base to get info from
 * @param solId		- soldier-ID to show info for
 * @param overview	- pointer to SoldierDiaryOverviewState
 * @param display	- SoldierDiaryDisplay (SoldierDiaryPerformanceState.h)
 */
SoldierDiaryPerformanceState::SoldierDiaryPerformanceState(
		Base* const base,
		const size_t solId,
		SoldierDiaryOverviewState* const overview,
		const SoldierDiaryDisplay display)
	:
		_solId(solId),
		_overview(overview),
		_display(display),
		_lastScroll(0u),
		_diary(nullptr),
		_init(true)
{
	if (base != nullptr)
	{
		_listBase = base->getSoldiers();
		_listDead = nullptr;
		_rows = _listBase->size();
	}
	else
	{
		_listDead = _game->getSavedGame()->getDeadSoldiers();
		_listBase = nullptr;
		_rows = _listDead->size();
	}

	_window				= new Window(this);

	_txtTitle			= new Text(310, 16, 5,  8);
	_txtBaseLabel		= new Text(310,  9, 5, 25);

	_btnPrev			= new TextButton(28, 14,   8, 8);
	_btnNext			= new TextButton(28, 14, 284, 8);

	_btnMissions		= new TextButton(70, 16,   8, 177);
	_btnKills			= new TextButton(70, 16,  86, 177);
	_btnAwards			= new TextButton(70, 16, 164, 177);
	_btnOk				= new TextButton(70, 16, 242, 177);

	// Mission stats
	_txtLocation		= new Text( 92, 16,  16, 36);
	_txtType			= new Text(114, 16, 108, 36);
	_txtUFO				= new Text( 92, 16, 222, 36);
	_lstLocation		= new TextList( 92, 113,  16,  52); // 14 rows
	_lstType			= new TextList(114, 113, 108,  52);
	_lstUFO				= new TextList( 92, 113, 222,  52);
	_lstMissionTotals	= new TextList(298,   9,  16, 166);

	// Kill stats
	_txtRace			= new Text(98, 16,  16, 36);
	_txtRank			= new Text(98, 16, 114, 36);
	_txtWeapon			= new Text(98, 16, 212, 36);
	_lstRace			= new TextList( 98, 97,  16,  52); // 12 rows
	_lstRank			= new TextList( 98, 97, 114,  52);
	_lstWeapon			= new TextList( 98, 97, 212,  52);
	_lstKillTotals		= new TextList(234,  9,  16, 166);
	_txtProficiency		= new Text(100, 9, 16, 156);

	// Award stats
	_txtMedal			= new Text(90, 9,  16, 36);
	_txtMedalGrade		= new Text(52, 9, 196, 36);
	_txtMedalClass		= new Text(40, 9, 248, 36);
	_lstAwards			= new TextList(240, 97, 48, 49); // 12 rows
	_txtMedalInfo		= new Text(285, 25, 16, 150);


	// Award sprites
	_srtAwards = _game->getResourcePack()->getSurfaceSet("Awards");
	_srtLevels = _game->getResourcePack()->getSurfaceSet("AwardDecorations");
	int offset_y;
	for (size_t
			i = 0u;
			i != SPRITE_ROWS;
			++i)
	{
		offset_y = SPRITES_y + (static_cast<int>(i) * 8);
		_srfAward.push_back(new Surface(31, 7,  16, offset_y));
		_srfDecor.push_back(new Surface(31, 7, 164, offset_y));
	}

	setInterface("awards", true);

	add(_window,		"window",	"awards");
	add(_txtTitle,		"title",	"awards");
	add(_txtBaseLabel,	"title",	"awards");

	add(_btnPrev,		"button",	"awards");
	add(_btnNext,		"button",	"awards");

	add(_btnMissions,	"button2",	"awards");
	add(_btnKills,		"button2",	"awards");
	add(_btnAwards,		"button2",	"awards");
	add(_btnOk,			"button2",	"awards");

	// Mission stats
	add(_txtLocation,		"text",		"awards");
	add(_txtType,			"text",		"awards");
	add(_txtUFO,			"text",		"awards");
	add(_lstLocation,		"list",		"awards");
	add(_lstType,			"list",		"awards");
	add(_lstUFO,			"list",		"awards");
	add(_lstMissionTotals,	"list2",	"awards");

	// Kill stats
	add(_txtRace,			"text",		"awards");
	add(_txtRank,			"text",		"awards");
	add(_txtWeapon,			"text",		"awards");
	add(_lstRace,			"list",		"awards");
	add(_lstRank,			"list",		"awards");
	add(_lstWeapon,			"list",		"awards");
	add(_lstKillTotals,		"list2",	"awards");
	add(_txtProficiency,	"text",		"awards");

	// Award stats
	add(_txtMedal,		"text",	"awards");
	add(_txtMedalGrade,	"text",	"awards");
	add(_txtMedalClass,	"text",	"awards");
	add(_lstAwards,		"list",	"awards");
	add(_txtMedalInfo,	"info",	"awards");

	// Award sprites
	for (size_t
			i = 0u;
			i != SPRITE_ROWS;
			++i)
	{
		add(_srfAward[i]);
		add(_srfDecor[i]);
	}

	centerSurfaces();


	_window->setBackground(_game->getResourcePack()->getSurface("BACK02.SCR"));

	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setBig();

	if (_listBase != nullptr)
	{
		_txtBaseLabel->setAlign(ALIGN_CENTER);
		_txtBaseLabel->setText(base->getLabel());

		if (_rows > 1u)
		{
			_btnPrev->setText(L"<");
			_btnPrev->onMouseClick(		static_cast<ActionHandler>(&SoldierDiaryPerformanceState::btnPrevClick));
			_btnPrev->onKeyboardPress(	static_cast<ActionHandler>(&SoldierDiaryPerformanceState::btnPrevClick),
										Options::keyBattlePrevUnit);

			_btnNext->setText(L">");
			_btnNext->onMouseClick(		static_cast<ActionHandler>(&SoldierDiaryPerformanceState::btnNextClick));
			_btnNext->onKeyboardPress(	static_cast<ActionHandler>(&SoldierDiaryPerformanceState::btnNextClick),
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
			_btnPrev->onMouseClick(		static_cast<ActionHandler>(&SoldierDiaryPerformanceState::btnNextClick));
			_btnPrev->onKeyboardPress(	static_cast<ActionHandler>(&SoldierDiaryPerformanceState::btnNextClick),
										Options::keyBattlePrevUnit);

			_btnNext->setText(L">");
			_btnNext->onMouseClick(		static_cast<ActionHandler>(&SoldierDiaryPerformanceState::btnPrevClick));
			_btnNext->onKeyboardPress(	static_cast<ActionHandler>(&SoldierDiaryPerformanceState::btnPrevClick),
										Options::keyBattleNextUnit);
		}
		else
		{
			_btnPrev->setVisible(false);
			_btnNext->setVisible(false);
		}
	}


	_btnMissions->setText(tr("STR_MISSIONS_UC"));
	_btnMissions->onMouseClick(		static_cast<ActionHandler>(&SoldierDiaryPerformanceState::btnMissionsToggle));
	_btnMissions->onKeyboardPress(	static_cast<ActionHandler>(&SoldierDiaryPerformanceState::btnMissionsToggle),
									SDLK_m);

	_btnKills->setText(tr("STR_KILLS_UC"));
	_btnKills->onMouseClick(	static_cast<ActionHandler>(&SoldierDiaryPerformanceState::btnKillsToggle));
	_btnKills->onKeyboardPress(	static_cast<ActionHandler>(&SoldierDiaryPerformanceState::btnKillsToggle),
								SDLK_k);

	_btnAwards->setText(tr("STR_AWARDS_UC"));
	_btnAwards->onMouseClick(	static_cast<ActionHandler>(&SoldierDiaryPerformanceState::btnMedalsToggle));
	_btnAwards->onKeyboardPress(static_cast<ActionHandler>(&SoldierDiaryPerformanceState::btnMedalsToggle),
								SDLK_a);

	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick(	static_cast<ActionHandler>(&SoldierDiaryPerformanceState::btnOkClick));
	_btnOk->onKeyboardPress(static_cast<ActionHandler>(&SoldierDiaryPerformanceState::btnOkClick),
							Options::keyOk);
	_btnOk->onKeyboardPress(static_cast<ActionHandler>(&SoldierDiaryPerformanceState::btnOkClick),
							Options::keyOkKeypad);
	_btnOk->onKeyboardPress(static_cast<ActionHandler>(&SoldierDiaryPerformanceState::btnOkClick),
							Options::keyCancel);


	// Mission stats ->
	_txtLocation->setText(tr("STR_MISSIONS_BY_LOCATION"));
	_txtType->setText(tr("STR_MISSIONS_BY_TYPE"));
	_txtUFO->setText(tr("STR_MISSIONS_BY_UFO"));

	Uint8 color		 (static_cast<Uint8>(_game->getRuleset()->getInterface("awards")->getElement("list")->color2));
	_colorBtnUp		= static_cast<Uint8>(_game->getRuleset()->getInterface("awards")->getElement("button2")->color);
	_colorBtnDown	= static_cast<Uint8>(_game->getRuleset()->getInterface("awards")->getElement("button2")->color2);
	_color1stCol	= static_cast<Uint8>(_game->getRuleset()->getInterface("awards")->getElement("list2")->color);

	_lstLocation->setArrowColor(color);
	_lstLocation->setColumns(2, 80,12);
	_lstLocation->setMargin();

	_lstType->setArrowColor(color);
	_lstType->setColumns(2, 100,14);
	_lstType->setMargin();

	_lstUFO->setArrowColor(color);
	_lstUFO->setColumns(2, 80,12);
	_lstUFO->setMargin();

	_lstMissionTotals->setColumns(4, 78,78,78,64);
	_lstMissionTotals->setMargin();


	// Kill stats ->
	_txtRace->setText(tr("STR_KILLS_BY_RACE"));
	_txtRank->setText(tr("STR_KILLS_BY_RANK"));
	_txtWeapon->setText(tr("STR_KILLS_BY_WEAPON"));

	_lstRace->setArrowColor(color);
	_lstRace->setColumns(2, 80,18);
	_lstRace->setMargin();

	_lstRank->setArrowColor(color);
	_lstRank->setColumns(2, 80,18);
	_lstRank->setMargin();

	_lstWeapon->setArrowColor(color);
	_lstWeapon->setColumns(2, 80,18);
	_lstWeapon->setMargin();

	_lstKillTotals->setColumns(3, 78,78,78);
	_lstKillTotals->setMargin();


	// Award stats ->
	_txtMedal->setText(tr("STR_MEDAL"));
	_txtMedalGrade->setText(tr("STR_MEDAL_GRADE"));
	_txtMedalClass->setText(tr("STR_MEDAL_CLASS"));

	_lstAwards->setArrowColor(color);
	_lstAwards->setColumns(3, 148,52,40);
	_lstAwards->setBackground(_window);
	_lstAwards->setSelectable();
	_lstAwards->setMargin();
	_lstAwards->onMouseOver(	static_cast<ActionHandler>(&SoldierDiaryPerformanceState::lstMouseOver));
	_lstAwards->onMouseOut(		static_cast<ActionHandler>(&SoldierDiaryPerformanceState::lstMouseOut));
	_lstAwards->onMouseClick(	static_cast<ActionHandler>(&SoldierDiaryPerformanceState::lstMouseClick),
								SDL_BUTTON_RIGHT);
//	_lstAwards->onMousePress(							   &SoldierDiaryPerformanceState::handle); // call to parent-state. huh

	_txtMedalInfo->setHighContrast();
	_txtMedalInfo->setWordWrap();


	switch (_display)
	{
		case DIARY_KILLS:		_displayGroup = _btnKills;		break;
		case DIARY_MISSIONS:	_displayGroup = _btnMissions;	break;
		case DIARY_MEDALS:		_displayGroup = _btnAwards;
	}

	_btnKills	->setGroup(&_displayGroup);
	_btnMissions->setGroup(&_displayGroup);
	_btnAwards	->setGroup(&_displayGroup);
}

/**
 * dTor.
 */
SoldierDiaryPerformanceState::~SoldierDiaryPerformanceState()
{}

/**
 * Clears and reinitializes the several performance-screens for each Soldier.
 */
void SoldierDiaryPerformanceState::init()
{
	if (_init == false)
		_init = true;
	else
	{
		State::init();

		for (size_t // clear sprites
				i = 0u;
				i != SPRITE_ROWS;
				++i)
		{
			_srfAward[i]->clear();
			_srfDecor[i]->clear();
		}

		_lstRank			->scrollTo(); // reset scroll-depth for lists
		_lstRace			->scrollTo();
		_lstWeapon			->scrollTo();
		_lstKillTotals		->scrollTo();
		_lstLocation		->scrollTo();
		_lstType			->scrollTo();
		_lstUFO				->scrollTo();
		_lstMissionTotals	->scrollTo();
		_lstAwards			->scrollTo();
		_lastScroll			= 0u;


		bool vis;

		if (_display == DIARY_KILLS) // set visibility for Kill stats
		{
			vis = true;
			_btnKills->setColor(_colorBtnDown);
		}
		else
		{
			vis = false;
			_btnKills->setColor(_colorBtnUp);
		}
		_txtRace		->setVisible(vis);
		_txtRank		->setVisible(vis);
		_txtWeapon		->setVisible(vis);
		_lstRace		->setVisible(vis);
		_lstRank		->setVisible(vis);
		_lstWeapon		->setVisible(vis);
		_lstKillTotals	->setVisible(vis);
		_txtProficiency	->setVisible(vis);


		if (_display == DIARY_MISSIONS) // set visibility for Mission stats
		{
			vis = true;
			_btnMissions->setColor(_colorBtnDown);
		}
		else
		{
			vis = false;
			_btnMissions->setColor(_colorBtnUp);
		}
		_txtLocation		->setVisible(vis);
		_txtType			->setVisible(vis);
		_txtUFO				->setVisible(vis);
		_lstLocation		->setVisible(vis);
		_lstType			->setVisible(vis);
		_lstUFO				->setVisible(vis);
		_lstMissionTotals	->setVisible(vis);


	//	_btnAwards->setVisible(_game->getRuleset()->getAwardsList().empty() == false);

		if (_display == DIARY_MEDALS) // set visibility for awarded Medals
		{
			vis = true;
			_btnAwards->setColor(_colorBtnDown);
		}
		else
		{
			vis = false;
			_btnAwards->setColor(_colorBtnUp);
		}
		_txtMedal		->setVisible(vis);
		_txtMedalGrade	->setVisible(vis);
		_txtMedalClass	->setVisible(vis);
		_lstAwards		->setVisible(vis);
		_txtMedalInfo	->setVisible(vis);


		_lstKillTotals		->clearList();
		_lstMissionTotals	->clearList();
		_awardsList			.clear();
		_awardTypes			.clear();

		_lstRace		->clearList();
		_lstRank		->clearList();
		_lstWeapon		->clearList();
		_lstLocation	->clearList();
		_lstType		->clearList();
		_lstUFO			->clearList();
		_lstAwards		->clearList();


		if (_listBase != nullptr)
		{
			const Soldier* const soldier (_listBase->at(_solId));
			_diary = soldier->getDiary();
			_txtTitle->setText(soldier->getLabel());
		}
		else
		{
			const SoldierDead* const deceased (_listDead->at(_solId));
			_diary = deceased->getDiary();
			_txtTitle->setText(deceased->getLabel());
		}

		const std::vector<TacticalStatistics*>& tacticals (_game->getSavedGame()->getTacticalStatistics());

		std::wstring
			wst1,
			wst2,
			wst3,
			wst4;

		int t;
		if ((t = static_cast<int>(_diary->getMissionTotal())) != 0) // Mission stats ->
			wst1 = tr("STR_MISSIONS_").arg(t);

		if ((t = _diary->getWinTotal(tacticals)) != 0)
			wst2 = tr("STR_WINS_").arg(t);

		if ((t = _diary->getScoreTotal(tacticals)) != 0)
			wst3 = tr("STR_SCORE_VALUE_").arg(t);

		if ((t = _diary->getDaysWoundedTotal()) != 0)
			wst4 = tr("STR_DAYS_WOUNDED_").arg(t).arg(L" d");

		_lstMissionTotals->addRow(
							4,
							wst1.c_str(),
							wst2.c_str(),
							wst3.c_str(),
							wst4.c_str());


		if ((t = _diary->getKillTotal()) != 0) // Kill stats ->
			wst1 = tr("STR_KILLS_").arg(t);
		else
			wst1 = L"";

		if ((t = _diary->getStunTotal()) != 0)
			wst2 = tr("STR_STUNS_").arg(t);
		else
			wst2 = L"";

		if ((t = _diary->getPointsTotal()) != 0)
			wst3 = tr("STR_SCORE_VALUE_").arg(t);
		else
			wst3 = L"";

		_lstKillTotals->addRow(
							3,
							wst1.c_str(),
							wst2.c_str(),
							wst3.c_str());

		if ((t = _diary->getProficiency()) != -1)
			_txtProficiency->setText(tr("STR_PROFICIENCY_").arg(t).c_str());
		else
			_txtProficiency->setVisible(false);


		static const size_t LST_COLS (6u);

		TextList* const arList[LST_COLS] // Mission & Kill stats ->
		{
			_lstRace,
			_lstRank,
			_lstWeapon,
			_lstLocation,
			_lstType,
			_lstUFO
		};

		const std::map<std::string, int> arStats[LST_COLS]
		{
			_diary->getAlienRaceTotal(),
			_diary->getAlienRankTotal(),
			_diary->getWeaponTotal(),
			_diary->getRegionTotal(tacticals),
			_diary->getTypeTotal(tacticals),
			_diary->getUfoTotal(tacticals)
		};

		size_t r;
		for (size_t
				i = 0u;
				i != LST_COLS;
				++i)
		{
			r = 0u;
			for (std::map<std::string, int>::const_iterator
					j = arStats[i].begin();
					j != arStats[i].end();
					++j)
			{
				if (j->first != "NO_UFO")
				{
					std::wostringstream woststr;
					woststr << j->second;

					arList[i]->addRow(
									2,
									tr(j->first).c_str(),
									woststr.str().c_str());
					arList[i]->setCellColor(r++, 0u, _color1stCol);
				}
			}
		}


	//	if (_game->getRuleset()->getAwardsList().empty() == true) return;

		const RuleAward* awardRule;
		std::string
			type,
			qualifier;

		for (std::vector<SoldierAward*>::const_iterator // Award stats ->
				i = _diary->getSoldierAwards().begin();
				i != _diary->getSoldierAwards().end();
				++i)
		{
			type = (*i)->getType();
			_awardTypes.push_back(type);

			qualifier = (*i)->getQualifier();

			awardRule = _game->getRuleset()->getAwardsList().at(type);
			std::wostringstream
				woststr1,
				woststr2;

			if (qualifier == "noQual")
			{
				woststr1 << tr(type);
				woststr2 << tr(awardRule->getDescription());
			}
			else
			{
				woststr1 << tr(type).arg(tr(qualifier));
				woststr2 << tr(awardRule->getDescription()).arg(tr(qualifier));
			}

			_lstAwards->addRow(
							3,
							woststr1.str().c_str(),
							tr((*i)->getGradeString()).c_str(),
							tr((*i)->getClassString()).c_str());

			_awardsList.push_back(woststr2.str());
			drawMedals();
		}
	}
}

/**
 * Draws sprites.
 */
void SoldierDiaryPerformanceState::drawMedals() // private.
{
	if (_display == DIARY_MEDALS)
	{
		for (size_t
				i = 0u;
				i != SPRITE_ROWS;
				++i)
		{
			_srfAward[i]->clear();
			_srfDecor[i]->clear();
		}

		const RuleAward* awardRule;
		const size_t scroll (_lstAwards->getScroll());
		int sprite;

		size_t j (0u);
		for (std::vector<SoldierAward*>::const_iterator							// show awards that are on the Soldier's list
				i = _diary->getSoldierAwards().begin();
				i != _diary->getSoldierAwards().end();
				++i, ++j)
		{
			if (j >= scroll														// draw each award's sprite
				&& j - scroll < _srfAward.size())
			{
				awardRule = _game->getRuleset()->getAwardsList().at((*i)->getType());
				sprite = awardRule->getSprite();
				_srtAwards->getFrame(sprite)->blit(_srfAward[j - scroll]);

				if ((sprite = static_cast<int>((*i)->getAwardLevel())) != 0)	// draw each award's level-sprite
					_srtLevels->getFrame(sprite)->blit(_srfDecor[j - scroll]);
			}
		}
	}
}

/**
 * Exits this state and also pops SoldierDiaryOverviewState.
 * @param action - pointer to an Action
 */
void SoldierDiaryPerformanceState::btnOkClick(Action*)
{
	_game->popState(); // <- this
	_game->popState(); // <- SoldierDiaryOverviewState
}

/**
 * Display Kills totals.
 * @param action - pointer to an Action
 */
void SoldierDiaryPerformanceState::btnKillsToggle(Action*)
{
	switch (_display)
	{
		case DIARY_KILLS:
			_overview->setSoldierId(_solId);
			_game->popState();
			break;

		case DIARY_MISSIONS:
		case DIARY_MEDALS:
			_display = DIARY_KILLS;
			init();
	}
}

/**
 * Display Missions totals.
 * @param action - pointer to an Action
 */
void SoldierDiaryPerformanceState::btnMissionsToggle(Action*)
{
	switch (_display)
	{
		case DIARY_MISSIONS:
			_overview->setSoldierId(_solId);
			_game->popState();
			break;

		case DIARY_KILLS:
		case DIARY_MEDALS:
			_display = DIARY_MISSIONS;
			init();
	}
}

/**
 * Display Awards medals.
 * @param action - pointer to an Action
 */
void SoldierDiaryPerformanceState::btnMedalsToggle(Action*)
{
	switch (_display)
	{
		case DIARY_MEDALS:
			_overview->setSoldierId(_solId);
			_game->popState();
			break;

		case DIARY_KILLS:
		case DIARY_MISSIONS:
			_display = DIARY_MEDALS;
			init();
	}
}

/**
 * Display a Medal's mouse-over info.
 * @param action - pointer to an Action
 */
void SoldierDiaryPerformanceState::lstMouseOver(Action*)
{
	const size_t r (_lstAwards->getSelectedRow());

	if (r != std::numeric_limits<size_t>::max())
		_txtMedalInfo->setText(_awardsList[r]);
	else
		_txtMedalInfo->setText(L"");
}

/**
 * Clears the Medal information.
 * @param action - pointer to an Action
 */
void SoldierDiaryPerformanceState::lstMouseOut(Action*)
{
	_txtMedalInfo->setText(L"");
}

/**
 * Opens the Ufopaedia article for a clicked Award.
 * @param action - pointer to an Action
 */
void SoldierDiaryPerformanceState::lstMouseClick(Action*)
{
	const size_t r (_lstAwards->getSelectedRow());
	if (r != std::numeric_limits<size_t>::max())
	{
		_init = false; // do not scroll the list to top.
		Ufopaedia::openArticle(_game, _awardTypes[r]);
	}
}

/**
 * Goes to the previous Soldier.
 * @param action - pointer to an Action
 */
void SoldierDiaryPerformanceState::btnPrevClick(Action*)
{
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
void SoldierDiaryPerformanceState::btnNextClick(Action*)
{
	if (++_solId == _rows)
		_solId = 0u;

	init();
}

/**
 * Runs state functionality every cycle.
 * @note Keeps the award-sprites in sync with the list-scroll.
 */
void SoldierDiaryPerformanceState::think()
{
	State::think();

	if (_lastScroll != _lstAwards->getScroll())
	{
		_lastScroll = _lstAwards->getScroll();
		drawMedals();
	}
}

}
