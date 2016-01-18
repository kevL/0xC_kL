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


namespace OpenXcom
{

/**
 * Diary screen that displays/hides the several screens that document a
 * soldier's Performance.
 * @param base		- pointer to the Base to get info from
 * @param soldierId	- ID of the selected soldier
 * @param overview	- pointer to SoldierDiaryOverviewState
 * @param display	- SoldierDiaryDisplay (SoldierDiaryPerformanceState.h)
 */
SoldierDiaryPerformanceState::SoldierDiaryPerformanceState(
		Base* const base,
		const size_t soldierId,
		SoldierDiaryOverviewState* const overview,
		const SoldierDiaryDisplay display)
	:
		_base(base),
		_soldierId(soldierId),
		_overview(overview),
		_display(display),
		_lastScrollPos(0),
		_diary(nullptr)
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

	_window				= new Window(this, 320, 200);

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
	_lstLocation		= new TextList( 92, 113,  16,  52);
	_lstType			= new TextList(114, 113, 108,  52);
	_lstUFO				= new TextList( 92, 113, 222,  52);
	_lstMissionTotals	= new TextList(288,   9,  18, 166);

	// Kill stats
	_txtRace			= new Text(98, 16,  16, 36);
	_txtRank			= new Text(98, 16, 114, 36);
	_txtWeapon			= new Text(98, 16, 212, 36);
	_lstRace			= new TextList( 98, 113,  16,  52);
	_lstRank			= new TextList( 98, 113, 114,  52);
	_lstWeapon			= new TextList( 98, 113, 212,  52);
	_lstKillTotals		= new TextList(210,   9,  18, 166);

	// Award stats
	_txtMedalName		= new Text(90, 9,  16, 36);
	_txtMedalLevel		= new Text(52, 9, 196, 36);
	_txtMedalClass		= new Text(40, 9, 248, 36);
	_lstAwards			= new TextList(240, 97, 48, 49);
	_txtMedalInfo		= new Text(280, 25, 20, 150);


	// Award sprites
	_srtSprite = _game->getResourcePack()->getSurfaceSet("Awards");
	_srtDecor = _game->getResourcePack()->getSurfaceSet("AwardDecorations");
	for (int
			i = 0;
			i != static_cast<int>(LIST_ROWS);
			++i)
	{
		_srfSprite.push_back(new Surface(31, 7, 16, LIST_SPRITES_y + (i * 8)));
		_srfLevel.push_back(new Surface(31, 7, 16, LIST_SPRITES_y + (i * 8)));
	}

//	setPalette(PAL_BASESCAPE);
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
	add(_txtRace,		"text",		"awards");
	add(_txtRank,		"text",		"awards");
	add(_txtWeapon,		"text",		"awards");
	add(_lstRace,		"list",		"awards");
	add(_lstRank,		"list",		"awards");
	add(_lstWeapon,		"list",		"awards");
	add(_lstKillTotals,	"list2",	"awards");

	// Award stats
	add(_txtMedalName,	"text",	"awards");
	add(_txtMedalLevel,	"text",	"awards");
	add(_txtMedalClass,	"text",	"awards");
	add(_lstAwards,		"list",	"awards");
	add(_txtMedalInfo,	"info",	"awards");

	// Award sprites
	for (size_t
			i = 0;
			i != LIST_ROWS;
			++i)
	{
		add(_srfSprite[i]);
		add(_srfLevel[i]);
	}

	centerAllSurfaces();


//	_window->setColor(PINK);
	_window->setBackground(_game->getResourcePack()->getSurface("BACK02.SCR"));

//	_txtTitle->setColor(BLUE);
	_txtTitle->setBig();
	_txtTitle->setAlign(ALIGN_CENTER);


//	_btnNext->setColor(PURPLE);
	_btnNext->setText(L">");

//	_btnPrev->setColor(PURPLE);
	_btnPrev->setText(L"<");

	if (_base == nullptr)
	{
		_txtBaseLabel->setVisible(false);

		_btnNext->onMouseClick((ActionHandler)& SoldierDiaryPerformanceState::btnPrevClick); // list is reversed in Memorial.
		_btnNext->onKeyboardPress(
						(ActionHandler)& SoldierDiaryPerformanceState::btnPrevClick,
						Options::keyBattleNextUnit);

		_btnPrev->onMouseClick((ActionHandler)& SoldierDiaryPerformanceState::btnNextClick);
		_btnPrev->onKeyboardPress(
						(ActionHandler)& SoldierDiaryPerformanceState::btnNextClick,
						Options::keyBattlePrevUnit);
	}
	else
	{
//		_txtBaseLabel->setColor(BLUE);
		_txtBaseLabel->setAlign(ALIGN_CENTER);
		_txtBaseLabel->setText(_base->getName(_game->getLanguage()));

		_btnNext->onMouseClick((ActionHandler)& SoldierDiaryPerformanceState::btnNextClick);
		_btnNext->onKeyboardPress(
						(ActionHandler)& SoldierDiaryPerformanceState::btnNextClick,
						Options::keyBattleNextUnit);

		_btnPrev->onMouseClick((ActionHandler)& SoldierDiaryPerformanceState::btnPrevClick);
		_btnPrev->onKeyboardPress(
						(ActionHandler)& SoldierDiaryPerformanceState::btnPrevClick,
						Options::keyBattlePrevUnit);
	}


//	_btnMissions->setColor(BLUE);
	_btnMissions->setText(tr("STR_MISSIONS_UC"));
	_btnMissions->onMouseClick((ActionHandler)& SoldierDiaryPerformanceState::btnMissionsToggle);
	_btnMissions->onMouseClick(
					(ActionHandler)& SoldierDiaryPerformanceState::btnMissionsToggle,
					SDLK_m);

//	_btnKills->setColor(BLUE);
	_btnKills->setText(tr("STR_KILLS_UC"));
	_btnKills->onMouseClick((ActionHandler)& SoldierDiaryPerformanceState::btnKillsToggle);
	_btnKills->onMouseClick(
					(ActionHandler)& SoldierDiaryPerformanceState::btnKillsToggle,
					SDLK_k);

//	_btnAwards->setColor(BLUE);
	_btnAwards->setText(tr("STR_AWARDS_UC"));
	_btnAwards->onMouseClick((ActionHandler)& SoldierDiaryPerformanceState::btnMedalsToggle);
	_btnAwards->onMouseClick(
					(ActionHandler)& SoldierDiaryPerformanceState::btnMedalsToggle,
					SDLK_a);

//	_btnOk->setColor(BLUE);
	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)& SoldierDiaryPerformanceState::btnOkClick);
	_btnOk->onKeyboardPress(
					(ActionHandler)& SoldierDiaryPerformanceState::btnOkClick,
					Options::keyOk);
	_btnOk->onKeyboardPress(
					(ActionHandler)& SoldierDiaryPerformanceState::btnOkClick,
					Options::keyOkKeypad);
	_btnOk->onKeyboardPress(
					(ActionHandler)& SoldierDiaryPerformanceState::btnOkClick,
					Options::keyCancel);


	// Mission stats ->
//	_txtLocation->setColor(PINK);
	_txtLocation->setText(tr("STR_MISSIONS_BY_LOCATION"));

//	_txtType->setColor(PINK);
	_txtType->setText(tr("STR_MISSIONS_BY_TYPE"));

//	_txtUFO->setColor(PINK);
	_txtUFO->setText(tr("STR_MISSIONS_BY_UFO"));

	Uint8 color		= _game->getRuleset()->getInterface("awards")->getElement("list")->color2;
	_colorBtnUp		= _game->getRuleset()->getInterface("awards")->getElement("button2")->color;
	_colorBtnDown	= _game->getRuleset()->getInterface("awards")->getElement("button2")->color2;
	_color1stCol	= _game->getRuleset()->getInterface("awards")->getElement("list2")->color;

//	_lstLocation->setColor(WHITE);
//	_lstLocation->setArrowColor(PINK);
	_lstLocation->setArrowColor(color);
	_lstLocation->setColumns(2, 80,12);
	_lstLocation->setMargin();

//	_lstType->setColor(WHITE);
//	_lstType->setArrowColor(PINK);
	_lstType->setArrowColor(color);
	_lstType->setColumns(2, 100,14);
	_lstType->setMargin();

//	_lstUFO->setColor(WHITE);
//	_lstUFO->setArrowColor(PINK);
	_lstUFO->setArrowColor(color);
	_lstUFO->setColumns(2, 80,12);
	_lstUFO->setMargin();

//	_lstMissionTotals->setColor(YELLOW);
//	_lstMissionTotals->setSecondaryColor(WHITE);
	_lstMissionTotals->setColumns(4, 70,70,70,78);
	_lstMissionTotals->setMargin();


	// Kill stats ->
//	_txtRace->setColor(PINK);
	_txtRace->setText(tr("STR_KILLS_BY_RACE"));

//	_txtRank->setColor(PINK);
	_txtRank->setText(tr("STR_KILLS_BY_RANK"));

//	_txtWeapon->setColor(PINK);
	_txtWeapon->setText(tr("STR_KILLS_BY_WEAPON"));

//	_lstRace->setColor(WHITE);
//	_lstRace->setArrowColor(PINK);
	_lstRace->setArrowColor(color);
	_lstRace->setColumns(2, 80,18);
	_lstRace->setMargin();

//	_lstRank->setColor(WHITE);
//	_lstRank->setArrowColor(PINK);
	_lstRank->setArrowColor(color);
	_lstRank->setColumns(2, 80,18);
	_lstRank->setMargin();

//	_lstWeapon->setColor(WHITE);
//	_lstWeapon->setArrowColor(PINK);
	_lstWeapon->setArrowColor(color);
	_lstWeapon->setColumns(2, 80,18);
	_lstWeapon->setMargin();

//	_lstKillTotals->setColor(YELLOW);
//	_lstKillTotals->setSecondaryColor(WHITE);
	_lstKillTotals->setColumns(3, 70,70,70);
	_lstKillTotals->setMargin();


	// Award stats ->
//	_txtMedalName->setColor(PINK);
	_txtMedalName->setText(tr("STR_MEDAL_NAME"));

//	_txtMedalLevel->setColor(PINK);
	_txtMedalLevel->setText(tr("STR_MEDAL_DECOR_LEVEL"));

//	_txtMedalClass->setColor(PINK);
	_txtMedalClass->setText(tr("STR_MEDAL_DECOR_CLASS"));

//	_lstAwards->setColor(WHITE);
//	_lstAwards->setArrowColor(PINK);
	_lstAwards->setArrowColor(color);
	_lstAwards->setColumns(3, 148,52,40);
	_lstAwards->setBackground(_window);
	_lstAwards->setSelectable();
	_lstAwards->setMargin();
	_lstAwards->onMouseOver((ActionHandler)& SoldierDiaryPerformanceState::lstMouseOver);
	_lstAwards->onMouseOut((ActionHandler)& SoldierDiaryPerformanceState::lstMouseOut);
	_lstAwards->onMousePress((ActionHandler)& SoldierDiaryPerformanceState::handle);

//	_txtMedalInfo->setColor(BROWN);
	_txtMedalInfo->setHighContrast();
	_txtMedalInfo->setWordWrap();


	switch (_display)
	{
		case DIARY_KILLS:
			_displayGroup = _btnKills;
		break;
		case DIARY_MISSIONS:
			_displayGroup = _btnMissions;
		break;
		case DIARY_MEDALS:
			_displayGroup = _btnAwards;
	}

	_btnKills->setGroup(&_displayGroup);
	_btnMissions->setGroup(&_displayGroup);
	_btnAwards->setGroup(&_displayGroup);
}

/**
 * dTor.
 */
SoldierDiaryPerformanceState::~SoldierDiaryPerformanceState()
{}

/**
 *  Clears and reinitializes the several Performance screens for each soldier.
 */
void SoldierDiaryPerformanceState::init()
{
	State::init();

	for (size_t // clear sprites
			i = 0;
			i != LIST_ROWS;
			++i)
	{
		_srfSprite[i]->clear();
		_srfLevel[i]->clear();
	}

	_lstRank			->scrollTo(0); // reset scroll depth for lists
	_lstRace			->scrollTo(0);
	_lstWeapon			->scrollTo(0);
	_lstKillTotals		->scrollTo(0);
	_lstLocation		->scrollTo(0);
	_lstType			->scrollTo(0);
	_lstUFO				->scrollTo(0);
	_lstMissionTotals	->scrollTo(0);
	_lstAwards			->scrollTo(0);
	_lastScrollPos		= 0;


	bool vis;

	if (_display == DIARY_KILLS) // set visibility for Kill stats
	{
		vis = true;
//		_btnKills->setColor(YELLOW);
		_btnKills->setColor(_colorBtnDown);
	}
	else
	{
		vis = false;
//		_btnKills->setColor(BLUE);
		_btnKills->setColor(_colorBtnUp);
	}
	_txtRace		->setVisible(vis);
	_txtRank		->setVisible(vis);
	_txtWeapon		->setVisible(vis);
	_lstRace		->setVisible(vis);
	_lstRank		->setVisible(vis);
	_lstWeapon		->setVisible(vis);
	_lstKillTotals	->setVisible(vis);


	if (_display == DIARY_MISSIONS) // set visibility for Mission stats
	{
		vis = true;
//		_btnMissions->setColor(YELLOW);
		_btnMissions->setColor(_colorBtnDown);
	}
	else
	{
		vis = false;
//		_btnMissions->setColor(BLUE);
		_btnMissions->setColor(_colorBtnUp);
	}
	_txtLocation		->setVisible(vis);
	_txtType			->setVisible(vis);
	_txtUFO				->setVisible(vis);
	_lstLocation		->setVisible(vis);
	_lstType			->setVisible(vis);
	_lstUFO				->setVisible(vis);
	_lstMissionTotals	->setVisible(vis);


//	_btnAwards->setVisible(_game->getRuleset()->getAwardsList().empty() == false); // safety.

	if (_display == DIARY_MEDALS) // set visibility for awarded Medals
	{
		vis = true;
//		_btnAwards->setColor(YELLOW);
		_btnAwards->setColor(_colorBtnDown);
	}
	else
	{
		vis = false;
//		_btnAwards->setColor(BLUE);
		_btnAwards->setColor(_colorBtnUp);
	}
	_txtMedalName	->setVisible(vis);
	_txtMedalLevel	->setVisible(vis);
	_txtMedalClass	->setVisible(vis);
	_lstAwards		->setVisible(vis);
	_txtMedalInfo	->setVisible(vis);


	_awardsListEntry	.clear();
	_lstKillTotals		->clearList();
	_lstMissionTotals	->clearList();

	_lstRace		->clearList();
	_lstRank		->clearList();
	_lstWeapon		->clearList();
	_lstLocation	->clearList();
	_lstType		->clearList();
	_lstUFO			->clearList();
	_lstAwards		->clearList();

	if (_base == nullptr)
	{
		if (_soldierId >= _listDead->size())
			_soldierId = 0;

		const SoldierDead* const deadSoldier = _listDead->at(_soldierId);
		_diary = deadSoldier->getDiary();

		_txtTitle->setText(deadSoldier->getName());
	}
	else
	{
		if (_soldierId >= _list->size())
			_soldierId = 0;

		const Soldier* const soldier = _list->at(_soldierId);
		_diary = soldier->getDiary();

		_txtTitle->setText(soldier->getName());
	}


	if (_diary == nullptr) // safety.
		return;


	std::wstring
		wst1,
		wst2,
		wst3,
		wst4;

	if (_diary->getMissionTotal() != 0) // Mission stats ->
		wst1 = tr("STR_MISSIONS").arg(_diary->getMissionTotal());
	if (_diary->getWinTotal() != 0)
		wst2 = tr("STR_WINS").arg(_diary->getWinTotal());
	if (_diary->getScoreTotal() != 0)
		wst3 = tr("STR_SCORE_VALUE").arg(_diary->getScoreTotal());
	if (_diary->getDaysWoundedTotal() != 0)
		wst4 = tr("STR_DAYS_WOUNDED").arg(_diary->getDaysWoundedTotal()).arg(L" dy");

	_lstMissionTotals->addRow(
						4,
						wst1.c_str(),
						wst2.c_str(),
						wst3.c_str(),
						wst4.c_str());


	if (_diary->getKillTotal() != 0) // Kill stats ->
		wst1 = tr("STR_KILLS").arg(_diary->getKillTotal());
	else
		wst1 = L"";

	if (_diary->getStunTotal() != 0)
		wst2 = tr("STR_STUNS").arg(_diary->getStunTotal());
	else
		wst2 = L"";

	if (_diary->getScorePoints() != 0)
		wst3 = tr("STR_SCORE_VALUE").arg(_diary->getScorePoints());
	else
		wst3 = L"";

	_lstKillTotals->addRow(
						3,
						wst1.c_str(),
						wst2.c_str(),
						wst3.c_str());


	const size_t lstCols (6);
	TextList* const lstArray[lstCols] = // Mission & Kill stats ->
	{
		_lstRace,
		_lstRank,
		_lstWeapon,
		_lstLocation,
		_lstType,
		_lstUFO
	};

	const std::map<std::string, int> mapArray[lstCols] =
	{
		_diary->getAlienRaceTotal(),
		_diary->getAlienRankTotal(),
		_diary->getWeaponTotal(),
		_diary->getRegionTotal(),
		_diary->getTypeTotal(),
		_diary->getUfoTotal()
	};

	for (size_t
			i = 0;
			i != lstCols;
			++i)
	{
		size_t row (0);
		for (std::map<std::string, int>::const_iterator
				j = mapArray[i].begin();
				j != mapArray[i].end();
				++j)
		{
			if ((*j).first != "NO_UFO")
			{
				std::wostringstream woststr;
				woststr << (*j).second;

				lstArray[i]->addRow(
								2,
								tr((*j).first).c_str(),
								woststr.str().c_str());
//				lstArray[i]->setCellColor(row++, 0, YELLOW);
				lstArray[i]->setCellColor(row++, 0, _color1stCol);
			}
		}
	}


	for (std::vector<SoldierAward*>::const_iterator // Award stats ->
			i = _diary->getSoldierAwards()->begin();
			i != _diary->getSoldierAwards()->end();
			++i)
	{
		if (_game->getRuleset()->getAwardsList().empty() == true)
			break;

		const RuleAward* const awardRule (_game->getRuleset()->getAwardsList()[(*i)->getType()]);
		std::wostringstream
			woststr1,
			woststr2;

		if ((*i)->getQualifier() == "noQual")
		{
			woststr1 << tr((*i)->getType());
			woststr2 << tr(awardRule->getDescription());
		}
		else
		{
			woststr1 << tr((*i)->getType()).arg(tr((*i)->getQualifier()));
			woststr2 << tr(awardRule->getDescription()).arg(tr((*i)->getQualifier()));
		}

		_lstAwards->addRow(
						3,
						woststr1.str().c_str(),
						tr((*i)->getClassDescription()).c_str(),
						tr((*i)->getClassDegree()).c_str());

		_awardsListEntry.push_back(woststr2.str());

		drawMedals();
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
				i = 0;
				i != LIST_ROWS;
				++i)
		{
			_srfSprite[i]->clear();
			_srfLevel[i]->clear();
		}

		const RuleAward* awardRule;
		const size_t scroll = _lstAwards->getScroll();
		int sprite;

		size_t j = 0;
		for (std::vector<SoldierAward*>::const_iterator
				i = _diary->getSoldierAwards()->begin();
				i != _diary->getSoldierAwards()->end();
				++i, ++j)
		{
			if (j >= scroll // show awards that are visible on the list
				&& j - scroll < _srfSprite.size())
			{
				awardRule = _game->getRuleset()->getAwardsList()[(*i)->getType()]; // handle award sprites
				sprite = awardRule->getSprite();
				_srtSprite->getFrame(sprite)->blit(_srfSprite[j - scroll]);

				sprite = static_cast<int>((*i)->getClassLevel()); // handle award decoration sprites
				if (sprite != 0)
					_srtDecor->getFrame(sprite)->blit(_srfLevel[j - scroll]);
			}
		}
	}
}

/**
 * Returns to the previous screen.
 * @param action - pointer to an Action
 */
void SoldierDiaryPerformanceState::btnOkClick(Action*)
{
	_game->popState();
	_game->popState();
}

/**
 * Display Kills totals.
 * @param action - pointer to an Action
 */
void SoldierDiaryPerformanceState::btnKillsToggle(Action*)
{
	if (_display != DIARY_KILLS)
	{
		_display = DIARY_KILLS;
		init();
	}
	else
	{
		_overview->setSoldierId(_soldierId);
		_game->popState();
	}
}

/**
 * Display Missions totals.
 * @param action - pointer to an Action
 */
void SoldierDiaryPerformanceState::btnMissionsToggle(Action*)
{
	if (_display != DIARY_MISSIONS)
	{
		_display = DIARY_MISSIONS;
		init();
	}
	else
	{
		_overview->setSoldierId(_soldierId);
		_game->popState();
	}
}

/**
 * Display Awards medals.
 * @param action - pointer to an Action
 */
void SoldierDiaryPerformanceState::btnMedalsToggle(Action*)
{
	if (_display != DIARY_MEDALS)
	{
		_display = DIARY_MEDALS;
		init();
	}
	else
	{
		_overview->setSoldierId(_soldierId);
		_game->popState();
	}
}

/**
 * Display a Medal's mouse-over info.
 * @param action - pointer to an Action
 */
void SoldierDiaryPerformanceState::lstMouseOver(Action*)
{
	const size_t row = _lstAwards->getSelectedRow();

	if (_awardsListEntry.empty() == true
		|| row > _awardsListEntry.size() - 1)
	{
		_txtMedalInfo->setText(L"");
	}
	else
		_txtMedalInfo->setText(_awardsListEntry[row]);
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
 * Goes to the previous soldier.
 * @param action - pointer to an Action
 */
void SoldierDiaryPerformanceState::btnPrevClick(Action*)
{
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
void SoldierDiaryPerformanceState::btnNextClick(Action*)
{
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
 * Runs state functionality every cycle.
 * @note Used to update award-sprites vector.
 */
void SoldierDiaryPerformanceState::think()
{
	State::think();

	if (_lastScrollPos != _lstAwards->getScroll())
	{
		_lastScrollPos = _lstAwards->getScroll();
		drawMedals();
	}
}

}
