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

//#include <algorithm>
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
#include "../Savegame/SavedGame.h"
#include "../Savegame/SoldierDead.h"
#include "../Savegame/SoldierDeath.h"
#include "../Savegame/SoldierDiary.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the Statistics window.
 */
StatisticsState::StatisticsState()
{
	_window		= new Window(this, 320, 200, 0, 0, POPUP_BOTH);
	_txtTitle	= new Text(310, 25, 5, 8);
	_lstStats	= new TextList(280, 136, 12, 36);
	_btnOk		= new TextButton(50, 12, 135, 180);

	setInterface("newGameMenu");

	add(_window,	"window",	"saveMenus");
	add(_btnOk,		"button",	"saveMenus");
	add(_txtTitle,	"text",		"saveMenus");
	add(_lstStats,	"list",		"saveMenus");

	centerAllSurfaces();


	_window->setBackground(_game->getResourcePack()->getSurface("BACK01.SCR"));

	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setBig();

	_lstStats->setColumns(2, 200,80);
	_lstStats->setDot(true);

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
	// TODO: Might want some music here.
}

/**
 * dTor.
 */
StatisticsState::~StatisticsState()
{}

/**
 *
 */
template <typename T>
T StatisticsState::total(const std::vector<T>& vect) const
{
	T total (0);
	for (typename std::vector<T>::const_iterator
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
	switch (gameSave->getEnding())
	{
		case END_WIN:
			woststr << tr("STR_VICTORY");
			break;
		case END_LOSE:
			woststr << tr("STR_DEFEAT");
	}
	const GameTime* const gt (gameSave->getTime());
	woststr << L'\x02'
			<< gt->getDayString(_game->getLanguage())
			<< L" "
			<< tr(gt->getMonthString())
			<< L" "
			<< gt->getYear();
	_txtTitle->setText(woststr.str());

	const int monthlyScore (total(gameSave->getResearchScores()) / gameSave->getResearchScores().size()); // huh. Is this total.
	const int64_t
		totalIncome (total(gameSave->getIncomeList())), // Check these also.
		totalExpenses (total(gameSave->getExpenditureList()));

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
		soldiersRecruited (solLive.size() + solDead->size()),
		soldiersLost (solDead->size());

	int
		aliensKilled	(0),
		aliensCaptured	(0),
		friendlyKills	(0),

		daysWounded		(0),
		longestMonths	(0);

	std::map<std::string, int> weaponKills;
	for (std::vector<Soldier*>::const_iterator
			i = solLive.begin();
			i != solLive.end();
			++i)
	{
		SoldierDiary* const diary ((*i)->getDiary());

		aliensKilled	+= diary->getKillTotal();
		aliensCaptured	+= diary->getStunTotal();
		daysWounded		+= diary->getDaysWoundedTotal();

		longestMonths = std::max(longestMonths,
								 diary->getMonthsService());

		for (std::map<std::string, int>::const_iterator
				j = diary->getWeaponTotal().begin();
				j != diary->getWeaponTotal().end();
				++j)
		{
			if (weaponKills.find(j->first) == weaponKills.end())
				weaponKills[j->first] = j->second;
			else
				weaponKills[j->first] += j->second;
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

		longestMonths = std::max(longestMonths,
								 diary->getMonthsService());

		for (std::map<std::string, int>::const_iterator
				j = diary->getWeaponTotal().begin();
				j != diary->getWeaponTotal().end();
				++j)
		{
			if (weaponKills.find(j->first) == weaponKills.end())
				weaponKills[j->first] = j->second;
			else
				weaponKills[j->first] += j->second;
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
			}
		}
	}

	// TODO: Add civilian stats:
	// - rescued
	// - slain by aLiens
	// - slain by xCom.

	int maxWeapon (0);
	std::string highestWeapon ("STR_NONE");
	for (std::map<std::string, int>::const_iterator
			i = weaponKills.begin();
			i != weaponKills.end();
			++i)
	{
		if (i->second > maxWeapon)
		{
			maxWeapon = i->second;
			highestWeapon = i->first;
		}
	}

	const int
		ufosDetected (gameSave->getCanonicalId("STR_UFO") - 1),
		alienBases   (gameSave->getCanonicalId("STR_ALIEN_BASE") - 1),
		terrorSites  (gameSave->getCanonicalId("STR_TERROR_SITE") - 1);

	int totalCrafts (0);
	for (std::vector<std::string>::const_iterator
			i = _game->getRuleset()->getCraftsList().begin();
			i != _game->getRuleset()->getCraftsList().end();
			++i)
	{
		totalCrafts += gameSave->getCanonicalId(*i) - 1;
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
		currentScientists += (*i)->getScientists();
		currentEngineers += (*i)->getEngineers();
	}

	int countriesLost (0);
	for (std::vector<Country*>::const_iterator
			i = gameSave->getCountries()->begin();
			i != gameSave->getCountries()->end();
			++i)
	{
		if ((*i)->getPact() == true)
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
	_lstStats->addRow(2, tr("STR_WEAPON_MOST_KILLS").c_str(),		tr(highestWeapon).c_str());
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
	_game->setSavedGame();
	_game->setState(new MainMenuState);
}

}
