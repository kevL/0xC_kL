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

#include "MonthlyReportState.h"

//#include <cmath>
//#include <sstream>

#include "DefeatState.h"

#include "../Battlescape/CeremonyState.h"

#include "../Engine/Game.h"
//#include "../Engine/Language.h" // TEST, for soldier label.
#include "../Engine/LocalizedText.h"
#include "../Engine/Options.h"
#include "../Engine/RNG.h"

#include "../Menu/SaveGameState.h"

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/Window.h"

#include "../Resource/XcomResourcePack.h"

#include "../Ruleset/RuleCountry.h"
#include "../Ruleset/RuleInterface.h"
#include "../Ruleset/RuleRegion.h"
#include "../Ruleset/Ruleset.h"

#include "../Savegame/Base.h"
#include "../Savegame/Country.h"
#include "../Savegame/GameTime.h"
#include "../Savegame/Region.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/SoldierDiary.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the Monthly Report screen.
 */
MonthlyReportState::MonthlyReportState()
	:
		_gameOver(false),
		_ratingPrior(0),
		_ratingTotal(0),
		_deltaFunds(0),
		_gameSave(_game->getSavedGame())
{
	_window		= new Window(this, 320, 200);
	_txtTitle	= new Text(300, 17, 10, 8);

	_txtMonth	= new Text(110, 9,  16, 24);
	_txtRating	= new Text(178, 9, 126, 24);

	_txtChange	= new Text(288,   9, 16, 32);
	_txtDesc	= new Text(288, 140, 16, 40);

	_btnOk		= new TextButton(288, 16, 16, 177);

	_txtFailure	= new Text(288, 160, 16, 10);
	_btnOkLoser	= new TextButton(120, 18, 100, 175);

//	_txtIncome = new Text(300, 9, 16, 32);
//	_txtMaintenance = new Text(130, 9, 16, 40);
//	_txtBalance = new Text(160, 9, 146, 40);

	setInterface("monthlyReport");

	add(_window,		"window",	"monthlyReport");
	add(_txtTitle,		"text1",	"monthlyReport");
	add(_txtMonth,		"text1",	"monthlyReport");
	add(_txtRating,		"text1",	"monthlyReport");
	add(_txtChange,		"text1",	"monthlyReport");
	add(_txtDesc,		"text2",	"monthlyReport");
	add(_btnOk,			"button",	"monthlyReport");
	add(_txtFailure,	"text2",	"monthlyReport");
	add(_btnOkLoser,	"button",	"monthlyReport");

//	add(_txtIncome,			"text1", "monthlyReport");
//	add(_txtMaintenance,	"text1", "monthlyReport");
//	add(_txtBalance,		"text1", "monthlyReport");

	centerSurfaces();


	_window->setBackground(_game->getResourcePack()->getSurface("BACK13.SCR"));

	_txtTitle->setText(tr("STR_XCOM_PROJECT_MONTHLY_REPORT"));
	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setBig();


	calculateChanges(); // <- sets Rating etc.

	int
		month (_gameSave->getTime()->getMonth() - 1),
		year (_gameSave->getTime()->getYear());

	if (month == 0)
	{
		month = 12;
		--year;
	}

	std::string st;
	switch (month)
	{
		case  1: st = "STR_JAN"; break;
		case  2: st = "STR_FEB"; break;
		case  3: st = "STR_MAR"; break;
		case  4: st = "STR_APR"; break;
		case  5: st = "STR_MAY"; break;
		case  6: st = "STR_JUN"; break;
		case  7: st = "STR_JUL"; break;
		case  8: st = "STR_AUG"; break;
		case  9: st = "STR_SEP"; break;
		case 10: st = "STR_OCT"; break;
		case 11: st = "STR_NOV"; break;
		case 12: st = "STR_DEC";
	}
	_txtMonth->setText(tr("STR_MONTH").arg(tr(st)).arg(year));

	const int
		diff (_gameSave->getDifficultyInt()),
		ratingThreshold (_game->getRuleset()->getDefeatScore() + diff * 250);

	std::string track;
	if (_ratingTotal < ratingThreshold)
	{
		st = "STR_RATING_TERRIBLE";
		track = OpenXcom::res_MUSIC_GEO_MONTHLYREPORT_BAD;
	}
	else
	{
		if (_ratingTotal > ratingThreshold + 10000)
			st = "STR_RATING_STUPENDOUS";
		else if (_ratingTotal > ratingThreshold + 5000)
			st = "STR_RATING_EXCELLENT";
		else if (_ratingTotal > ratingThreshold + 2500)
			st = "STR_RATING_GOOD";
		else if (_ratingTotal > ratingThreshold + 1000)
			st = "STR_RATING_OK";
		else
			st = "STR_RATING_POOR";

		track = OpenXcom::res_MUSIC_GEO_MONTHLYREPORT;
	}
	_txtRating->setText(tr("STR_MONTHLY_RATING__").arg(_ratingTotal).arg(tr(st)));

/*	std::wostringstream ss; // ADD:
	ss << tr("STR_INCOME") << L"> \x01" << Text::formatCurrency(_gameSave->getCountryFunding() * 1000);
	ss << L" (";
	if (_deltaFunds > 0)
		ss << '+';
	ss << Text::formatCurrency(_deltaFunds) << L")";
	_txtIncome->setText(ss.str());

	std::wostringstream ss2;
	ss2 << tr("STR_MAINTENANCE") << L"> \x01" << Text::formatCurrency(_gameSave->getBaseMaintenances());
	_txtMaintenance->setText(ss2.str());

	std::wostringstream ss3;
	ss3 << tr("STR_BALANCE") << L"> \x01" << Text::formatCurrency(_gameSave->getFunds());
	_txtBalance->setText(ss3.str());
// end ADD. */

	std::wostringstream woststr;
	if (_deltaFunds > 0) woststr << '+';
	woststr << Text::formatCurrency(_deltaFunds * 1000);
	_txtChange->setText(tr("STR_FUNDING_CHANGE_").arg(woststr.str()));


	if (   _ratingPrior < ratingThreshold // calculate satisfaction
		&& _ratingTotal < ratingThreshold)
	{
		_gameOver = true; // you lose.
		st = "STR_YOU_HAVE_NOT_SUCCEEDED";

		_happyList.clear();
		_sadList.clear();
		_pactList.clear();
	}
	else if (_ratingTotal > 1000 + 2000 * diff)
		st = "STR_COUNCIL_IS_VERY_PLEASED";
	else if (_ratingTotal > -1)
		st = "STR_COUNCIL_IS_GENERALLY_SATISFIED";
	else
		st = "STR_COUNCIL_IS_DISSATISFIED";

	woststr.str(L"");
	woststr << tr(st);

	if (_gameOver == false)
	{
		if (_gameSave->getFunds() < _game->getRuleset()->getDefeatFunds())
		{
			if (_gameSave->getWarned() == true)
			{
				_gameOver = true; // you lose.
				woststr.str(L"");
				woststr << tr("STR_YOU_HAVE_NOT_SUCCEEDED");

				_happyList.clear();
				_sadList.clear();
				_pactList.clear();
			}
			else
			{
				_gameSave->setWarned();
				woststr << "\n\n" << tr("STR_COUNCIL_REDUCE_DEBTS");
				track = OpenXcom::res_MUSIC_GEO_MONTHLYREPORT_BAD;
			}
		}
		else
			_gameSave->setWarned(false);
	}

	woststr << countryList(
					_happyList,
					"STR_COUNTRY_IS_PARTICULARLY_PLEASED",
					"STR_COUNTRIES_ARE_PARTICULARLY_HAPPY");
	woststr << countryList(
					_sadList,
					"STR_COUNTRY_IS_UNHAPPY_WITH_YOUR_ABILITY",
					"STR_COUNTRIES_ARE_UNHAPPY_WITH_YOUR_ABILITY");
	woststr << countryList(
					_pactList,
					"STR_COUNTRY_HAS_SIGNED_A_SECRET_PACT",
					"STR_COUNTRIES_HAVE_SIGNED_A_SECRET_PACT");

	_txtDesc->setText(woststr.str());
	_txtDesc->setWordWrap();


	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick(	static_cast<ActionHandler>(&MonthlyReportState::btnOkClick));
	_btnOk->onKeyboardPress(static_cast<ActionHandler>(&MonthlyReportState::btnOkClick),
							Options::keyOk);
	_btnOk->onKeyboardPress(static_cast<ActionHandler>(&MonthlyReportState::btnOkClick),
							Options::keyOkKeypad);
	_btnOk->onKeyboardPress(static_cast<ActionHandler>(&MonthlyReportState::btnOkClick),
							Options::keyCancel);


	_txtFailure->setText(tr("STR_YOU_HAVE_FAILED"));
	_txtFailure->setBig();
	_txtFailure->setAlign(ALIGN_CENTER);
	_txtFailure->setVerticalAlign(ALIGN_MIDDLE);
	_txtFailure->setWordWrap();
	_txtFailure->setVisible(false);

	_btnOkLoser->setText(tr("STR_OK"));
	_btnOkLoser->onMouseClick(		static_cast<ActionHandler>(&MonthlyReportState::btnOkClick));
	_btnOkLoser->onKeyboardPress(	static_cast<ActionHandler>(&MonthlyReportState::btnOkClick),
									Options::keyOk);
	_btnOkLoser->onKeyboardPress(	static_cast<ActionHandler>(&MonthlyReportState::btnOkClick),
									Options::keyOkKeypad);
	_btnOkLoser->onKeyboardPress(	static_cast<ActionHandler>(&MonthlyReportState::btnOkClick),
									Options::keyCancel);
	_btnOkLoser->setVisible(false);


	_game->getResourcePack()->playMusic(track, "", 1);

	awards();
}

/**
 * dTor.
 */
MonthlyReportState::~MonthlyReportState()
{}

/**
 * Updates palettes.
 */
void MonthlyReportState::init()
{
	State::init();
}

/**
 * Updates all activity counters, gathers all scores, gets countries to sign
 * pacts, adjusts their fundings, assesses their satisfaction, and finally
 * calculates overall total score, with thanks to Volutar for the formulae.
 */
void MonthlyReportState::calculateChanges() // private.
{
	const int diff (_gameSave->getDifficultyInt());
	if (diff != 0)
	{
		int pactScore;
		for (std::vector<Country*>::const_iterator // add scores for pacted or about-to-pact Countries.
				i = _gameSave->getCountries()->begin();
				i != _gameSave->getCountries()->end();
				++i)
		{
			if ((pactScore = (*i)->getRules()->getPactScore() * diff) != 0
				&& (*i)->isPacted() == true)
			{
				Region* region (nullptr);
				for (std::vector<Region*>::const_iterator
						j = _gameSave->getRegions()->begin();
						j != _gameSave->getRegions()->end();
						++j)
				{
					if ((*j)->getRules()->getType() == (*i)->getRules()->getCountryRegion())
					{
						region = *j;
						break;
					}
				}

				if ((*i)->getPact() == true) // rand 50..100% if already pacted, full pts for about-to-pact Countries.
					pactScore = RNG::generate(pactScore >> 1u,
											  pactScore);

				if (pactScore != 0)
					_gameSave->scorePoints(
										region,
										*i,
										pactScore,
										true);
			}
		}
	}

	_ratingPrior = 0;
	int
		scorePlayer (0),
		scoreAlien  (0);

	const size_t assizeId (_gameSave->getFundsList().size() - 1u); // <- index of the month assessed

	for (std::vector<Region*>::const_iterator		// NOTE: Only Region scores are evaluated;
			i = _gameSave->getRegions()->begin();	// Country scores are NOT added.
			i != _gameSave->getRegions()->end();
			++i)
	{
		(*i)->newMonth();

		if (assizeId != 0u)
			_ratingPrior += (*i)->getActivityXCom() .at(assizeId - 1u)
						  - (*i)->getActivityAlien().at(assizeId - 1u);

		scorePlayer += (*i)->getActivityXCom() .at(assizeId);
		scoreAlien  += (*i)->getActivityAlien().at(assizeId);
	}


	for (std::vector<Country*>::const_iterator
			i = _gameSave->getCountries()->begin();
			i != _gameSave->getCountries()->end();
			++i)
	{
		if ((*i)->getRecentPact() == true)
			_pactList.push_back((*i)->getRules()->getType());

		(*i)->newMonth( // calculates satisfaction & funding & updates pact-vars.
					scorePlayer,
					scoreAlien,
					diff);
		_deltaFunds += (*i)->getFunding().back()
					 - (*i)->getFunding().at(assizeId);

		switch ((*i)->getSatisfaction())
		{
			case SAT_SAD:
				_sadList.push_back((*i)->getRules()->getType());
				break;
			case SAT_HAPPY:
				_happyList.push_back((*i)->getRules()->getType());
		}
	}

	if (assizeId != 0u)
		_ratingPrior += _gameSave->getResearchScores().at(assizeId - 1u); // add research-scores.

	scorePlayer += _gameSave->getResearchScores().at(assizeId); // add research-scores.
	_ratingTotal = scorePlayer - scoreAlien;

	_gameSave->balanceBudget(); // handle cash-accounts.
}

/**
 * Exits to the previous screen.
 * @param action - pointer to an Action
 */
void MonthlyReportState::btnOkClick(Action*)
{
	if (_gameOver == false)
	{
		_game->popState();

		if (_soldiersFeted.empty() == false)
			_game->pushState(new CeremonyState(_soldiersFeted));

		if (_gameSave->isIronman() == true)
			_game->pushState(new SaveGameState(
											OPT_GEOSCAPE,
											SAVE_IRONMAN,
											_palette));
		else if (Options::autosave == true) // NOTE: Auto-save points are fucked; they should be done *before* important events, not after.
			_game->pushState(new SaveGameState(
											OPT_GEOSCAPE,
											SAVE_AUTO_GEOSCAPE,
											_palette));
	}
	else if (_txtFailure->getVisible() == false)
	{
		_window->setColor(static_cast<Uint8>(_game->getRuleset()->getInterface("monthlyReport")->getElement("window")->color2));

		_txtTitle->setVisible(false);
		_txtMonth->setVisible(false);
		_txtRating->setVisible(false);
		_txtChange->setVisible(false);
//		_txtIncome->setVisible(false);
//		_txtMaintenance->setVisible(false);
//		_txtBalance->setVisible(false);
		_txtDesc->setVisible(false);
		_btnOk->setVisible(false);

		_txtFailure->setVisible();
		_btnOkLoser->setVisible();

		_game->getResourcePack()->fadeMusic(_game, 1157);
		_game->getResourcePack()->playMusic(OpenXcom::res_MUSIC_LOSE);
	}
	else
	{
		_game->popState();
		_game->pushState(new DefeatState());

		_gameSave->setEnding(END_LOSE);

		if (_gameSave->isIronman() == true)
			_game->pushState(new SaveGameState(
											OPT_GEOSCAPE,
											SAVE_IRONMAN,
											_palette));
	}
}

/**
 * Builds a sentence from a list of countries adding the appropriate
 * separators and pluralization.
 * @param countries	- reference a vector of strings that is the list of countries
 * @param singular	- reference a string to append if the returned string is singular
 * @param plural	- reference a string to append if the returned string is plural
 */
std::wstring MonthlyReportState::countryList( // private.
		const std::vector<std::string>& countries,
		const std::string& singular,
		const std::string& plural) const
{
	std::wostringstream woststr;
	if (countries.empty() == false)
	{
		woststr << "\n\n";
		switch (countries.size())
		{
			case 1u:
				woststr << tr(singular).arg(tr(countries.front()));
				break;

			default:
			{
				LocalizedText countryList (tr(countries.front()));
				std::vector<std::string>::const_iterator i;
				for (
						i = countries.begin() + 1;
						i != countries.end() - 1;
						++i)
				{
					countryList = tr("STR_COUNTRIES_COMMA").arg(countryList).arg(tr(*i));
				}
				countryList = tr("STR_COUNTRIES_AND").arg(countryList).arg(tr(*i));
				woststr << tr(plural).arg(countryList);
			}
		}
	}
	return woststr.str();
}

/**
 * Handles monthly Soldier awards.
 */
void MonthlyReportState::awards() // private.
{
	for (std::vector<Base*>::const_iterator // Award medals for service time.
			i = _gameSave->getBases()->begin();
			i != _gameSave->getBases()->end();
			++i)
	{
		for (std::vector<Soldier*>::const_iterator
				j = (*i)->getSoldiers()->begin();
				j != (*i)->getSoldiers()->end();
				++j)
		{
			(*j)->getDiary()->addMonthlyService();

			//Log(LOG_INFO) << "";
			//Log(LOG_INFO) << "end MONTH report: " << Language::wstrToFs((*j)->getLabel());
			if ((*j)->getDiary()->manageAwards(
											_game->getRuleset(),
											_gameSave->getMissionStatistics()) == true)
			{
				_soldiersFeted.push_back(*j);
			}
		}
	}
}

}
