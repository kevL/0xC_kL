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

#include "MonthlyReportState.h"

//#include <cmath>
//#include <sstream>

#include "DefeatState.h"

#include "../Battlescape/CeremonyState.h"
#include "../Battlescape/DebriefingState.h" // access TAC_RATING strings.

#include "../Engine/Game.h"
//#include "../Engine/Language.h" // TEST, for soldier label.
#include "../Engine/LocalizedText.h"
#include "../Engine/Options.h"
#include "../Engine/RNG.h"

#include "../Menu/SaveGameState.h"

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextList.h"
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
 * Initializes all the elements in the MonthlyReport screen.
 */
MonthlyReportState::MonthlyReportState()
	:
		_defeated(false),
		_ratingPrior(0),
		_ratingTotal(0),
		_deltaFunds(0),
		_playSave(_game->getSavedGame())
{
	_window		= new Window(this);
	_txtTitle	= new Text(300, 16, 10, 7);

	_txtMonth	= new Text(110, 9,  16, 25);
	_txtRating	= new Text(178, 9, 126, 25);

	_txtChange	= new Text(288, 9, 16, 35);

	_lstCouncil	= new TextList(285, 129, 16, 46);

	_btnOk		= new TextButton(288, 16, 16, 177);

	_txtDefeat	= new Text(288, 160, 16, 10);
	_btnDefeat	= new TextButton(120, 18, 100, 175);

//	_txtIncome		= new Text(300, 9,  16, 32);
//	_txtMaintenance	= new Text(130, 9,  16, 40);
//	_txtBalance		= new Text(160, 9, 146, 40);

	setInterface("monthlyReport");

	add(_window,		"window",	"monthlyReport");
	add(_txtTitle,		"text1",	"monthlyReport");
	add(_txtMonth,		"text1",	"monthlyReport");
	add(_txtRating,		"text1",	"monthlyReport");
	add(_txtChange,		"text1",	"monthlyReport");
	add(_lstCouncil,	"list",		"monthlyReport");
	add(_btnOk,			"button",	"monthlyReport");
	add(_txtDefeat,		"text2",	"monthlyReport");
	add(_btnDefeat,		"button",	"monthlyReport");

//	add(_txtIncome,			"text1", "monthlyReport");
//	add(_txtMaintenance,	"text1", "monthlyReport");
//	add(_txtBalance,		"text1", "monthlyReport");

	centerSurfaces();


	_window->setBackground(_game->getResourcePack()->getSurface("BACK13.SCR"));

	_txtTitle->setText(tr("STR_XCOM_PROJECT_MONTHLY_REPORT"));
	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setBig();


	calculateReport(); // <- sets Rating etc -->

	int
		month (_playSave->getTime()->getMonth() - 1),
		year  (_playSave->getTime()->getYear());

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


	std::wostringstream woststr;
	if (_deltaFunds > 0) woststr << '+';
	woststr << Text::formatCurrency(_deltaFunds * 1000);
	_txtChange->setText(tr("STR_FUNDING_CHANGE_").arg(woststr.str()));

//	woststr.str(L"");
//	woststr << tr("STR_INCOME") << L"> \x01" << Text::formatCurrency(_playSave->getCountryFunding() * 1000);
//	woststr << L" (";
//	if (_deltaFunds > 0) woststr << '+';
//	woststr << Text::formatCurrency(_deltaFunds) << L")";
//	_txtIncome->setText(woststr.str());
//
//	woststr.str(L"");
//	woststr << tr("STR_MAINTENANCE") << L"> \x01" << Text::formatCurrency(_playSave->getBaseMaintenances());
//	_txtMaintenance->setText(woststr.str());
//
//	woststr.str(L"");
//	woststr << tr("STR_BALANCE") << L"> \x01" << Text::formatCurrency(_playSave->getFunds());
//	_txtBalance->setText(woststr.str());

	_lstCouncil->setColumns(1, 285);
	_lstCouncil->setBackground(_window);
	_lstCouncil->setWordWrap();
	_lstCouncil->setMargin();
	_lstCouncil->wrapIndent(false);

	const int defeatThreshold (_game->getRuleset()->getDefeatScore() + (_playSave->getDifficultyInt() * 250));

	std::string track;

	if (_ratingTotal < defeatThreshold)
	{
		st = TAC_RATING[0u]; // terrible
		track = OpenXcom::res_MUSIC_GEO_MONTHLYREPORT_BAD;

		_lstCouncil->addRow(1, tr("STR_COUNCIL_IS_DISSATISFIED_1").c_str());

		if (_ratingPrior >= defeatThreshold) // check defeat by rating
		{
			_lstCouncil->addRow(1, L"");
			_lstCouncil->addRow(1, tr("STR_COUNCIL_IS_DISSATISFIED_2").c_str());
		}
		else
			_defeated = true; // you lose.
	}
	else
	{
		std::string satisfaction;
		if (_ratingTotal > defeatThreshold + 10000)
		{
			st = TAC_RATING[5u]; // terrific
			satisfaction = "STR_COUNCIL_IS_VERY_PLEASED";
		}
		else if (_ratingTotal > defeatThreshold + 5000)
		{
			st = TAC_RATING[4u]; // excellent
			satisfaction = "STR_COUNCIL_IS_VERY_PLEASED";
		}
		else
		{
			satisfaction = "STR_COUNCIL_IS_GENERALLY_SATISFIED";

			if (_ratingTotal > defeatThreshold + 2500)
				st = TAC_RATING[3u]; // good
			else if (_ratingTotal > defeatThreshold + 1000)
				st = TAC_RATING[2u]; // okay
			else
				st = TAC_RATING[1u]; // poor
		}

		_lstCouncil->addRow(1, tr(satisfaction).c_str());

		track = OpenXcom::res_MUSIC_GEO_MONTHLYREPORT;
	}
	_txtRating->setText(tr("STR_MONTHLY_RATING__").arg(_ratingTotal).arg(tr(st)));


	if (_defeated == false)
	{
		if (_playSave->getFunds() < _game->getRuleset()->getDefeatFunds())
		{
			if (_playSave->hasLowFunds() == false) // check defeat by funds
			{
				_playSave->flagLowFunds();

				_lstCouncil->addRow(1, L"");
				_lstCouncil->addRow(1, tr("STR_COUNCIL_REDUCE_DEBTS").c_str());

				track = OpenXcom::res_MUSIC_GEO_MONTHLYREPORT_BAD;
			}
			else
				_defeated = true; // you lose.
		}
		else
			_playSave->flagLowFunds(false);
	}

	if (_defeated == false)
	{
		if (_listHappy.empty() == false)
		{
			_lstCouncil->addRow(1, L"");
			_lstCouncil->addRow(1, countryList(
											_listHappy,
											"STR_COUNTRY_IS_PARTICULARLY_PLEASED",
											"STR_COUNTRIES_ARE_PARTICULARLY_HAPPY").c_str());
		}

		if (_listSad.empty() == false)
		{
			_lstCouncil->addRow(1, L"");
			_lstCouncil->addRow(1, countryList(
											_listSad,
											"STR_COUNTRY_IS_UNHAPPY_WITH_YOUR_ABILITY",
											"STR_COUNTRIES_ARE_UNHAPPY_WITH_YOUR_ABILITY").c_str());
		}

		if (_listPacts.empty() == false)
		{
			_lstCouncil->addRow(1, L"");
			_lstCouncil->addRow(1, countryList(
											_listPacts,
											"STR_COUNTRY_HAS_SIGNED_A_SECRET_PACT",
											"STR_COUNTRIES_HAVE_SIGNED_A_SECRET_PACT").c_str());
		}

		if (_listProject.empty() == false)
		{
			_lstCouncil->addRow(1, L"");
			_lstCouncil->addRow(1, countryList(
											_listProject,
											"STR_COUNTRY_HAS_JOINED_THE_PROJECT",
											"STR_COUNTRIES_HAVE_JOINED_THE_PROJECT").c_str());
		}
	}
	else // defeated ->
	{
		_lstCouncil->addRow(1, L"");
		_lstCouncil->addRow(1, tr("STR_YOU_HAVE_NOT_SUCCEEDED_1").c_str());
		_lstCouncil->addRow(1, L"");
		_lstCouncil->addRow(1, tr("STR_YOU_HAVE_NOT_SUCCEEDED_2").c_str());
		_lstCouncil->addRow(1, L"");
		_lstCouncil->addRow(1, tr("STR_YOU_HAVE_NOT_SUCCEEDED_3").c_str());

		_txtDefeat->setText(tr("STR_YOU_HAVE_FAILED"));
		_txtDefeat->setBig();
		_txtDefeat->setAlign(ALIGN_CENTER);
		_txtDefeat->setVerticalAlign(ALIGN_MIDDLE);
		_txtDefeat->setWordWrap();

		_btnDefeat->setText(tr("STR_OK"));
		_btnDefeat->onMouseClick(	static_cast<ActionHandler>(&MonthlyReportState::btnOkClick));
		_btnDefeat->onKeyboardPress(static_cast<ActionHandler>(&MonthlyReportState::btnOkClick),
									Options::keyOk);
		_btnDefeat->onKeyboardPress(static_cast<ActionHandler>(&MonthlyReportState::btnOkClick),
									Options::keyOkKeypad);
		_btnDefeat->onKeyboardPress(static_cast<ActionHandler>(&MonthlyReportState::btnOkClick),
									Options::keyCancel);
	}

	_txtDefeat->setVisible(false);
	_btnDefeat->setVisible(false);

	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick(	static_cast<ActionHandler>(&MonthlyReportState::btnOkClick));
	_btnOk->onKeyboardPress(static_cast<ActionHandler>(&MonthlyReportState::btnOkClick),
							Options::keyOk);
	_btnOk->onKeyboardPress(static_cast<ActionHandler>(&MonthlyReportState::btnOkClick),
							Options::keyOkKeypad);
	_btnOk->onKeyboardPress(static_cast<ActionHandler>(&MonthlyReportState::btnOkClick),
							Options::keyCancel);

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
void MonthlyReportState::calculateReport() // private.
{
	const int diff (_playSave->getDifficultyInt());
	if (diff != 0)
	{
		PactStatus pactStatus;
		int pactScore;

		for (std::vector<Country*>::const_iterator // add scores for pacted or about-to-pact Countries.
				i  = _playSave->getCountries()->begin();
				i != _playSave->getCountries()->end();
			  ++i)
		{
			if ((pactStatus = (*i)->getPactStatus()) != PACT_NONE
				&& (pactScore = (*i)->getRules()->getPactScore() * diff) != 0)
			{
				Region* region (nullptr);
				for (std::vector<Region*>::const_iterator
						j  = _playSave->getRegions()->begin();
						j != _playSave->getRegions()->end();
					  ++j)
				{
					if ((*j)->getRules()->getType() == (*i)->getRules()->getCountryRegion())
					{
						region = *j;
						break;
					}
				}

				if (pactStatus == PACT_PACTED) // rand 50..100% if already pacted, full pts for about-to-pact Countries.
					pactScore = RNG::generate(pactScore >> 1u,
											  pactScore);

				if (pactScore != 0)
					_playSave->scorePoints(
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

	const size_t assizedId (_playSave->getFundsList().size() - 1u); // <- index of the month assessed

	for (std::vector<Region*>::const_iterator		// NOTE: Only Region scores are evaluated;
			i  = _playSave->getRegions()->begin();	// Country scores are NOT added.
			i != _playSave->getRegions()->end();
		  ++i)
	{
		(*i)->newMonth();

		if (assizedId != 0u)
			_ratingPrior += (*i)->getActivityXCom() .at(assizedId - 1u)
						  - (*i)->getActivityAlien().at(assizedId - 1u);

		scorePlayer += (*i)->getActivityXCom() .at(assizedId);
		scoreAlien  += (*i)->getActivityAlien().at(assizedId);
	}


	for (std::vector<Country*>::const_iterator
			i  = _playSave->getCountries()->begin();
			i != _playSave->getCountries()->end();
		  ++i)
	{
		(*i)->newMonth( // calculates satisfaction & funding & updates pact-vars.
					scorePlayer,
					scoreAlien,
					diff);
		_deltaFunds += (*i)->getFunding().back()
					 - (*i)->getFunding().at(assizedId);

		switch ((*i)->getPactStatus())
		{
			case PACT_NONE:
				switch ((*i)->getSatisfaction()) // NOTE: Pacted Countries have been set 'SAT_NEUTRAL' by Country::newMonth().
				{
					case SAT_SAD:
						_listSad.push_back((*i)->getRules()->getType());
						break;

					case SAT_HAPPY:
						_listHappy.push_back((*i)->getRules()->getType());
						break;

					case SAT_PROJECT:
						_listProject.push_back((*i)->getRules()->getType());
				}
				break;

			case PACT_RECENT:
				_listPacts.push_back((*i)->getRules()->getType());
				(*i)->setPactStatus(PACT_PACTED);
		}
	}

	if (assizedId != 0u)
		_ratingPrior += _playSave->getResearchScores().at(assizedId - 1u); // add research-scores.

	scorePlayer += _playSave->getResearchScores().at(assizedId); // add research-scores.
	_ratingTotal = scorePlayer - scoreAlien;

	_playSave->balanceBudget(); // handle cash-accounts.
}

/**
 * Exits to the previous screen.
 * @param action - pointer to an Action
 */
void MonthlyReportState::btnOkClick(Action*)
{
	if (_defeated == false)
	{
		_game->popState();

		if (_soldiersFeted.empty() == false)
			_game->pushState(new CeremonyState(_soldiersFeted));

		if (_playSave->isIronman() == true)
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
	else if (_txtDefeat->getVisible() == false)
	{
		_window->setColor(static_cast<Uint8>(_game->getRuleset()->getInterface("monthlyReport")->getElement("window")->color2));

		_txtTitle->setVisible(false);
		_txtMonth->setVisible(false);
		_txtRating->setVisible(false);
		_txtChange->setVisible(false);
//		_txtIncome->setVisible(false);
//		_txtMaintenance->setVisible(false);
//		_txtBalance->setVisible(false);
		_lstCouncil->setVisible(false);
		_btnOk->setVisible(false);

		_txtDefeat->setVisible();
		_btnDefeat->setVisible();

		_game->getResourcePack()->fadeMusic(_game, 1157);
		_game->getResourcePack()->playMusic(OpenXcom::res_MUSIC_LOSE);
	}
	else
	{
		_game->popState();
		_game->pushState(new DefeatState());

		_playSave->setEnding(END_LOSE);

		if (_playSave->isIronman() == true)
			_game->pushState(new SaveGameState(
											OPT_GEOSCAPE,
											SAVE_IRONMAN,
											_palette));
	}
}

/**
 * Builds a sentence from a list of countries adding the appropriate
 * separators and pluralization.
 * @param countries	- reference to a vector of strings that is the list of countries
 * @param singular	- reference to a string to append if the returned string is singular
 * @param plural	- reference to a string to append if the returned string is plural
 */
std::wstring MonthlyReportState::countryList( // private.
		const std::vector<std::string>& countries,
		const std::string& singular,
		const std::string& plural) const
{
	std::wostringstream woststr;
	if (countries.empty() == false)
	{
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
						i  = countries.begin() + 1;
						i != countries.end()   - 1;
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
 * Deals with monthly SoldierAwards.
 */
void MonthlyReportState::awards() // private.
{
	for (std::vector<Base*>::const_iterator // Award medals for service time.
			i  = _playSave->getBases()->begin();
			i != _playSave->getBases()->end();
		  ++i)
	{
		for (std::vector<Soldier*>::const_iterator
				j  = (*i)->getSoldiers()->begin();
				j != (*i)->getSoldiers()->end();
			  ++j)
		{
			//Log(LOG_INFO) << "end MONTH report: " << Language::wstrToFs((*j)->getLabel());
			(*j)->getDiary()->addMonthlyService();

			if ((*j)->getDiary()->updateAwards(
											_game->getRuleset(),
											_playSave->getTacticalStatistics()) == true)
			{
				_soldiersFeted.push_back(*j);
			}
		}
	}
}

}
