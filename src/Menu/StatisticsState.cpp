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

#include "StatisticsState.h"

#include <algorithm>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include "MainMenuState.h"

#include "../Engine/Game.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Options.h"

#include "../Ruleset/Ruleset.h"

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextList.h"
#include "../Interface/Window.h"

#include "../Resource/ResourcePack.h"

#include "../Savegame/Base.h"
#include "../Savegame/BattleUnitStatistics.h"
#include "../Savegame/Country.h"
#include "../Savegame/MissionStatistics.h"
#include "../Savegame/ResearchGeneral.h"
#include "../Savegame/SoldierDead.h"
#include "../Savegame/SoldierDeath.h"
#include "../Savegame/SoldierDiary.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the Statistics window.
 */
StatisticsState::StatisticsState()
	:
		_endType(_game->getSavedGame()->getEnding())
{
	_window		= new Window(this, 320, 200, 0, 0, POPUP_BOTH);
	_txtTitle	= new Text(320, 16, 0, 8);
	_lstStats	= new TextList(285, 137, 16, 34);
	_btnOk		= new TextButton(288, 16, 16, 177);

	setInterface("gameStatistics");

	add(_window,	"window",	"gameStatistics");
	add(_btnOk,		"button",	"gameStatistics");
	add(_txtTitle,	"text",		"gameStatistics");
	add(_lstStats,	"list",		"gameStatistics");

	centerAllSurfaces();


	_window->setBackground(_game->getResourcePack()->getSurface("BACK01.SCR"));

	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setBig();

	_lstStats->setColumns(2, 200,80);
	_lstStats->setDot(true);
	_lstStats->setMargin(16);

	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)& StatisticsState::btnOkClick);
	_btnOk->onKeyboardPress(
					(ActionHandler)& StatisticsState::btnOkClick,
					Options::keyOk);
	_btnOk->onKeyboardPress(
					(ActionHandler)& StatisticsState::btnOkClick,
					Options::keyOkKeypad);
	_btnOk->onKeyboardPress(
					(ActionHandler)& StatisticsState::btnOkClick,
					Options::keyCancel);

	listStats();
}

/**
 * dTor.
 */
StatisticsState::~StatisticsState()
{}

/**
 * Totals a vector of integers or floating-points.
 * @param vect - a vector of integers or floating-points
 * @return, the total
 */
template <typename _Tx>
_Tx StatisticsState::total(const std::vector<_Tx>& vect) const
{
	_Tx total (static_cast<_Tx>(0));
	for (typename std::vector<_Tx>::const_iterator
			i = vect.begin();
			i != vect.end();
			++i)
	{
		total += *i;
	}
	return total;
}

/**
 * Tabulates all the statistics.
 */
void StatisticsState::listStats()
{
	SavedGame* const gameSave (_game->getSavedGame());

	std::wostringstream woststr;
	switch (_endType)
	{
		case END_WIN:
			woststr << tr("STR_VICTORY");
			break;
		case END_LOSE:
			woststr << tr("STR_DEFEAT");
			break;

		default:
		case END_NONE:
			woststr << tr("STR_STATISTICS");
	}
	const GameTime* const gt (gameSave->getTime());
	woststr << L" "// L'\x02'
			<< gt->getDayString(_game->getLanguage())
			<< L" "
			<< tr(gt->getMonthString())
			<< L" "
			<< gt->getYear();
	_txtTitle->setText(woststr.str());

	const int monthlyScore (total(gameSave->getResearchScores()) / gameSave->getResearchScores().size()); // huh. Is this total. no.
	const int64_t
		totalIncome (total(gameSave->getIncomeList())),			// Check these also.
		totalExpenses (total(gameSave->getExpenditureList()));	// They are inaccurate after 12+ months (because only the latest 12 months are maintained).

	int
		missionsWin		(0),
		missionsLoss	(0),
		nightMissions	(0),
		bestScore		(-9999),
		worstScore		( 9999);

	for (std::vector<MissionStatistics*>::const_iterator
			i = gameSave->getMissionStatistics()->begin();
			i != gameSave->getMissionStatistics()->end();
			++i)
	{
		if ((*i)->success == true)
			++missionsWin;
		else
			++missionsLoss;

		bestScore  = std::max(bestScore,  (*i)->score);
		worstScore = std::min(worstScore, (*i)->score);

		if ((*i)->shade >= MissionStatistics::NIGHT_SHADE)
			++nightMissions;
	}

	bestScore  = (bestScore == -9999) ? 0 : bestScore; // make sure dorky values aren't left in
	worstScore = (worstScore == 9999) ? 0 : worstScore;

	std::vector<Soldier*> solLive;
	for (std::vector<Base*>::const_iterator
			i = gameSave->getBases()->begin();
			i != gameSave->getBases()->end();
			++i)
	{
		solLive.insert(
					solLive.end(),
					(*i)->getSoldiers()->begin(),
					(*i)->getSoldiers()->end());
	}

	const std::vector<SoldierDead*>* const solDead (gameSave->getDeadSoldiers());

	const int
		soldiersLost (static_cast<int>(solDead->size())),
		soldiersRecruited (static_cast<int>(solLive.size()) + soldiersLost);

	int
		aliensKilled	(0),
		aliensCaptured	(0),
		friendlyKills	(0),
		shotsFired		(0),
		shotsLanded		(0),

		daysWounded		(0),
		longestMonths	(0);

	std::map<std::string, int>
		killsByAlien,
		killsByWeapon,
		weaponTotal;

	for (std::vector<Soldier*>::const_iterator
			i = solLive.begin();
			i != solLive.end();
			++i)
	{
		SoldierDiary* const diary ((*i)->getDiary());

		aliensKilled	+= diary->getKillTotal();
		aliensCaptured	+= diary->getStunTotal();
		daysWounded		+= diary->getDaysWoundedTotal();
		shotsFired		+= diary->getShotsFiredTotal();
		shotsLanded		+= diary->getShotsLandedTotal();

		longestMonths = std::max(longestMonths,
								 diary->getMonthsService());

		weaponTotal = diary->getWeaponTotal();
		for (std::map<std::string, int>::const_iterator
				j = weaponTotal.begin();
				j != weaponTotal.end();
				++j)
		{
			killsByWeapon[j->first] += j->second;
		}

		for (std::vector<BattleUnitKill*>::const_iterator
				j = diary->getKills().begin();
				j != diary->getKills().end();
				++j)
		{
			switch ((*j)->_faction)
			{
				case FACTION_PLAYER:
				case FACTION_NEUTRAL:
					++friendlyKills;
					break;

				case FACTION_HOSTILE:
					++killsByAlien[(*j)->_race]; // TODO: Add up kills-by-rank also. Perhaps separate kills/stuns.
			}
		}
	}

	for (std::vector<SoldierDead*>::const_iterator
			i = solDead->begin();
			i != solDead->end();
			++i)
	{
		SoldierDiary* const diary ((*i)->getDiary());

		aliensKilled	+= diary->getKillTotal();
		aliensCaptured	+= diary->getStunTotal();
		daysWounded		+= diary->getDaysWoundedTotal();
		shotsFired		+= diary->getShotsFiredTotal();
		shotsLanded		+= diary->getShotsLandedTotal();

		longestMonths = std::max(longestMonths,
								 diary->getMonthsService());

		weaponTotal = diary->getWeaponTotal();
		for (std::map<std::string, int>::const_iterator
				j = weaponTotal.begin();
				j != weaponTotal.end();
				++j)
		{
			killsByWeapon[j->first] += j->second;
		}

		for (std::vector<BattleUnitKill*>::const_iterator
				j = diary->getKills().begin();
				j != diary->getKills().end();
				++j)
		{
			switch ((*j)->_faction)
			{
				case FACTION_PLAYER:
				case FACTION_NEUTRAL:
					++friendlyKills;
					break;

				case FACTION_HOSTILE:
					++killsByAlien[(*j)->_race]; // TODO: Add up kills-by-rank also. Perhaps separate kills/stuns.
			}
		}
	}

	// TODO: Add civilian stats:
	// - rescued
	// - slain by aLiens
	// - slain by xCom.

	int accuracy;
	if (shotsFired != 0)
		accuracy = 100 * shotsLanded / shotsFired;
	else
		accuracy = 0; // TODO: -1 for no-shots-fired.

	int typeWeapon (0);
	std::string bestWeapon ("STR_NONE");
	for (std::map<std::string, int>::const_iterator
			i = killsByWeapon.begin();
			i != killsByWeapon.end();
			++i)
	{
		if (i->second > typeWeapon)
		{
			typeWeapon = i->second;
			bestWeapon = i->first;
		}
	}

	int typeKilled (0);
	std::string worstAlien ("STR_NONE");
	for (std::map<std::string, int>::const_iterator
			i = killsByAlien.begin();
			i != killsByAlien.end();
			++i)
	{
		if (i->second > typeKilled)
		{
			typeKilled = i->second;
			worstAlien = i->first;
		}
	}

	std::map<std::string, int> ids (gameSave->getObjectIds());
	const int
		ufosDetected (std::max(0, ids["STR_UFO"] - 1)),
		alienBases   (std::max(0, ids["STR_ALIEN_BASE"] - 1)), // detected only.
		terrorSites  (std::max(0, ids["STR_TERROR_SITE"] - 1));

	int totalCrafts (0);
	for (std::vector<std::string>::const_iterator
			i = _game->getRuleset()->getCraftsList().begin();
			i != _game->getRuleset()->getCraftsList().end();
			++i)
	{
		if (ids.find(*i) != ids.end())
			totalCrafts += ids[*i] - 1; // TODO: Show quantity of Craft lost ... or sold.
	}

	const int currentBases (gameSave->getBases()->size());

	int
		currentScientists (0),
		currentEngineers  (0);
	for (std::vector<Base*>::const_iterator
			i = gameSave->getBases()->begin();
			i != gameSave->getBases()->end();
			++i)
	{
		currentScientists += (*i)->getTotalScientists();
		currentEngineers += (*i)->getTotalEngineers();
	}

	int countriesLost (0);
	for (std::vector<Country*>::const_iterator
			i = gameSave->getCountries()->begin();
			i != gameSave->getCountries()->end();
			++i)
	{
		if ((*i)->getPact() == true) // TODO: Countries total. Also do percent lost.
			++countriesLost;
	}

	int researchDone (0);
	for (std::vector<ResearchGeneral*>::const_iterator
			i = gameSave->getResearchGenerals().begin();
			i != gameSave->getResearchGenerals().end();
			++i)
	{
		if ((*i)->getStatus() == RS_COMPLETED)
			++researchDone;
	}

	const std::string difficulty[]
	{
		"STR_1_BEGINNER",
		"STR_2_EXPERIENCED",
		"STR_3_VETERAN",
		"STR_4_GENIUS",
		"STR_5_SUPERHUMAN"
	};

	_lstStats->addRow(2, tr("STR_DIFFICULTY").c_str(),				tr(difficulty[gameSave->getDifficulty()]).c_str());
	_lstStats->addRow(2, tr("STR_AVERAGE_MONTHLY_RATING").c_str(),	Text::intWide(monthlyScore).c_str());
	_lstStats->addRow(2, tr("STR_TOTAL_INCOME").c_str(),			Text::formatCurrency(totalIncome).c_str());
	_lstStats->addRow(2, tr("STR_TOTAL_EXPENDITURE").c_str(),		Text::formatCurrency(totalExpenses).c_str());
	_lstStats->addRow(2, tr("STR_MISSIONS_WON").c_str(),			Text::intWide(missionsWin).c_str());
	_lstStats->addRow(2, tr("STR_MISSIONS_LOST").c_str(),			Text::intWide(missionsLoss).c_str());
	_lstStats->addRow(2, tr("STR_NIGHT_MISSIONS").c_str(),			Text::intWide(nightMissions).c_str());
	_lstStats->addRow(2, tr("STR_BEST_RATING").c_str(),				Text::intWide(bestScore).c_str());
	_lstStats->addRow(2, tr("STR_WORST_RATING").c_str(),			Text::intWide(worstScore).c_str());
	_lstStats->addRow(2, tr("STR_SOLDIERS_RECRUITED").c_str(),		Text::intWide(soldiersRecruited).c_str());
	_lstStats->addRow(2, tr("STR_SOLDIERS_LOST").c_str(),			Text::intWide(soldiersLost).c_str());
	_lstStats->addRow(2, tr("STR_ALIEN_KILLS").c_str(),				Text::intWide(aliensKilled).c_str());
	_lstStats->addRow(2, tr("STR_ALIEN_CAPTURES").c_str(),			Text::intWide(aliensCaptured).c_str());
	_lstStats->addRow(2, tr("STR_FRIENDLY_KILLS").c_str(),			Text::intWide(friendlyKills).c_str());
	_lstStats->addRow(2, tr("STR_AVERAGE_ACCURACY").c_str(),		Text::formatPercent(accuracy).c_str());
	_lstStats->addRow(2, tr("STR_WEAPON_MOST_KILLS").c_str(),		tr(bestWeapon).c_str());
	_lstStats->addRow(2, tr("STR_ALIEN_MOST_KILLS").c_str(),		tr(worstAlien).c_str());
	_lstStats->addRow(2, tr("STR_LONGEST_SERVICE").c_str(),			Text::intWide(longestMonths).c_str());
	_lstStats->addRow(2, tr("STR_TOTAL_DAYS_WOUNDED").c_str(),		Text::intWide(daysWounded).c_str());
	_lstStats->addRow(2, tr("STR_TOTAL_UFOS").c_str(),				Text::intWide(ufosDetected).c_str());
	_lstStats->addRow(2, tr("STR_TOTAL_ALIEN_BASES").c_str(),		Text::intWide(alienBases).c_str());
	_lstStats->addRow(2, tr("STR_COUNTRIES_LOST").c_str(),			Text::intWide(countriesLost).c_str());
	_lstStats->addRow(2, tr("STR_TOTAL_TERROR_SITES").c_str(),		Text::intWide(terrorSites).c_str());
	_lstStats->addRow(2, tr("STR_TOTAL_BASES").c_str(),				Text::intWide(currentBases).c_str());
	_lstStats->addRow(2, tr("STR_TOTAL_CRAFT").c_str(),				Text::intWide(totalCrafts).c_str());
	_lstStats->addRow(2, tr("STR_TOTAL_SCIENTISTS").c_str(),		Text::intWide(currentScientists).c_str());
	_lstStats->addRow(2, tr("STR_TOTAL_ENGINEERS").c_str(),			Text::intWide(currentEngineers).c_str());
	_lstStats->addRow(2, tr("STR_TOTAL_RESEARCH").c_str(),			Text::intWide(researchDone).c_str());
}

/**
 * Exits the State.
 * @param action - pointer to an Action
 */
void StatisticsState::btnOkClick(Action*)
{
	switch (_endType)
	{
		case END_NONE:
			_game->popState();
			break;

		default:
		case END_WIN:
		case END_LOSE:
			_game->setSavedGame();
			_game->setState(new MainMenuState);
	}
}

}
