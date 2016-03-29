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

#include "FundingState.h"

//#include <sstream>

#include "../Engine/Game.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Options.h"

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextList.h"
#include "../Interface/Window.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/RuleCountry.h"
#include "../Ruleset/RuleRegion.h" // debug-data
#include "../Ruleset/Ruleset.h"

#include "../Savegame/Base.h"
#include "../Savegame/Country.h"
#include "../Savegame/Region.h" // debug-data
#include "../Savegame/SavedGame.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the Funding screen.
 */
FundingState::FundingState()
{
	_fullScreen = false;

	int dX; // x - 32 to center on Globe
	if (Options::baseXResolution > 320 + 32)
		dX = -32;
	else
		dX = 0;

	_window			= new Window(this, 320, 200, 0 + dX, 0, POPUP_BOTH);

	_txtTitle		= new Text(300, 17, 10 + dX, 9);

	_txtCountry		= new Text(102, 9,  16 + dX, 25);
	_txtFunding		= new Text( 60, 9, 118 + dX, 25);
	_txtChange		= new Text( 60, 9, 178 + dX, 25);
	_txtScore		= new Text( 60, 9, 238 + dX, 25);

	_lstCountries	= new TextList(285, 121, 16 + dX,  34);
	_lstTotal		= new TextList(285,  17, 16 + dX, 157);

	_btnOk			= new TextButton(288, 16, 16 + dX, 177);

	setInterface("fundingWindow");

	add(_window,		"window",	"fundingWindow");
	add(_txtTitle,		"text1",	"fundingWindow");
	add(_txtCountry,	"text2",	"fundingWindow");
	add(_txtFunding,	"text2",	"fundingWindow");
	add(_txtChange,		"text2",	"fundingWindow");
	add(_txtScore,		"text2",	"fundingWindow");
	add(_lstCountries,	"list",		"fundingWindow");
	add(_lstTotal,		"text2",	"fundingWindow");
	add(_btnOk,			"button",	"fundingWindow");

	centerAllSurfaces();


	_window->setBackground(_game->getResourcePack()->getSurface("BACK13.SCR"), dX);

	_btnOk->setText(tr("STR_CANCEL"));
	_btnOk->onMouseClick((ActionHandler)& FundingState::btnOkClick);
	_btnOk->onKeyboardPress(
					(ActionHandler)& FundingState::btnOkClick,
					Options::keyOk);
	_btnOk->onKeyboardPress(
					(ActionHandler)& FundingState::btnOkClick,
					Options::keyOkKeypad);
	_btnOk->onKeyboardPress(
					(ActionHandler)& FundingState::btnOkClick,
					Options::keyCancel);
	_btnOk->onKeyboardPress(
					(ActionHandler)& FundingState::btnOkClick,
					Options::keyGeoFunding);

	_txtTitle->setText(tr("STR_INTERNATIONAL_RELATIONS"));
	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setBig();

	_txtCountry->setText(tr("STR_COUNTRY"));
	_txtFunding->setText(tr("STR_FUNDING"));
	_txtChange->setText(tr("STR_CHANGE"));
	_txtScore->setText(tr("STR_SCORE"));

	// debug-data:
/*	for (std::vector<Country*>::const_iterator
			i = _game->getSavedGame()->getCountries()->begin();
			i != _game->getSavedGame()->getCountries()->end();
			++i)
	{
		const double
			lon ((*i)->getRules()->getLabelLongitude()),
			lat ((*i)->getRules()->getLabelLatitude());

		for (std::vector<Region*>::const_iterator
				j = _game->getSavedGame()->getRegions()->begin();
				j != _game->getSavedGame()->getRegions()->end();
				++j)
		{
			if ((*j)->getRules()->insideRegion(lon,lat) == true)
			{
				Log(LOG_INFO) << (*i)->getRules()->getType() << " in region: " << (*j)->getRules()->getType();
//				break;
			}
		}
	} */ // End debug-data.

	_lstCountries->setColumns(6, 102,60,6,54,6,54);
//	_lstCountries->setMargin();
	_lstCountries->setDot();
	_lstCountries->setBackground(_window);
	_lstCountries->setSelectable();

	static const int REGIONS (15);

	int row (0);
	std::string regionType;
	for (int
			i = 0;
			i != REGIONS;
			++i, ++row)
	{
		switch (i)
		{
			case  0: regionType = "STR_EUROPE";				break;
			case  1: regionType = "STR_SIBERIA";			break;
			case  2: regionType = "STR_NORTH_AMERICA";		break;
			case  3: regionType = "STR_SOUTH_AMERICA";		break;
			case  4: regionType = "STR_CENTRAL_ASIA";		break;
			case  5: regionType = "STR_SOUTH_EAST_ASIA";	break;
			case  6: regionType = "STR_AUSTRALASIA";		break;
			case  7: regionType = "STR_NORTH_AFRICA";		break;
			case  8: regionType = "STR_SOUTHERN_AFRICA";	break;
			case  9: regionType = "STR_ARCTIC";				break;
			case 10: regionType = "STR_ANTARCTICA";			break;
			case 11: regionType = "STR_PACIFIC";			break;
			case 12: regionType = "STR_NORTH_ATLANTIC";		break;
			case 13: regionType = "STR_SOUTH_ATLANTIC";		break;
			case 14: regionType = "STR_INDIAN_OCEAN";
		}
		_lstCountries->addRow(1, tr(regionType).c_str());
		_lstCountries->setRowColor(row, BROWN, true);

		for (std::vector<Country*>::const_iterator
				i = _game->getSavedGame()->getCountries()->begin();
				i != _game->getSavedGame()->getCountries()->end();
				++i)
		{
			if ((*i)->getRules()->getCountryRegion() == regionType)
			{
				++row;
				std::wostringstream
					woststr1,
					woststr2,
					woststr3,
					woststr4,
					woststr5;

				const std::vector<int>
					funds ((*i)->getFunding()),
					actX ((*i)->getActivityXCom()),
					actA ((*i)->getActivityAlien());

				woststr1 << L'\x01' << Text::formatCurrency(funds.at(funds.size() - 1));

				if (funds.size() > 1)
				{
					int change (funds.back() - funds.at(funds.size() - 2));
					if (change > 0)
						woststr2 << L'\x01' << L'+';
					else if (change < 0)
					{
						woststr2 << L'\x01' << L'-';
						change = -change;
					}
					else
						woststr2 << L' ' << L' ';

					woststr3 << L'\x01' << Text::formatCurrency(change);
				}
				else
				{
					woststr2 << L' ' << L' ';
					woststr3 << Text::formatCurrency(0);
				}

				int score (actX.at(actX.size() - 1) - actA.at(actA.size() - 1));
				if (score > -1)
					woststr4 << L' ' << L' ';
				else
				{
					woststr4 << L'\x01' << L'-';
					score = -score;
				}

				woststr5 << L'\x01' << score;

				_lstCountries->addRow(
									6,
									tr((*i)->getRules()->getType()).c_str(),
									woststr1.str().c_str(),
									woststr2.str().c_str(),
									woststr3.str().c_str(),
									woststr4.str().c_str(),
									woststr5.str().c_str());
			}
		}
	}


/*	for (std::vector<Country*>::const_iterator
			i = _game->getSavedGame()->getCountries()->begin();
			i != _game->getSavedGame()->getCountries()->end();
			++i)
	{
		std::wostringstream
			woststr1,
			woststr2,
			woststr3,
			woststr4,
			woststr5;

		const std::vector<int>
			funds ((*i)->getFunding()),
			actX ((*i)->getActivityXCom()),
			actA ((*i)->getActivityAlien());

		woststr1 << L'\x01' << Text::formatCurrency(funds.at(funds.size() - 1));

		if (funds.size() > 1)
		{
			int change (funds.back() - funds.at(funds.size() - 2));
			if (change > 0)
				woststr2 << L'\x01' << L'+';
			else if (change < 0)
			{
				woststr2 << L'\x01' << L'-';
				change = -change;
			}
			else
				woststr2 << L' ' << L' ';

			woststr3 << L'\x01' << Text::formatCurrency(change);
		}
		else
		{
			woststr2 << L' ' << L' ';
			woststr3 << Text::formatCurrency(0);
		}

		int score (actX.at(actX.size() - 1) - actA.at(actA.size() - 1));
		if (score > -1)
			woststr4 << L' ' << L' ';
		else
		{
			woststr4 << L'\x01' << L'-';
			score = -score;
		}

		woststr5 << L'\x01' << score;

		_lstCountries->addRow(
							6,
							tr((*i)->getRules()->getType()).c_str(),
							woststr1.str().c_str(),
							woststr2.str().c_str(),
							woststr3.str().c_str(),
							woststr4.str().c_str(),
							woststr5.str().c_str());
	} */


	const int gross (_game->getSavedGame()->getCountryFunding());
	int net (gross);

	for (std::vector<Base*>::const_iterator
		i = _game->getSavedGame()->getBases()->begin();
		i != _game->getSavedGame()->getBases()->end();
		++i)
	{
		net -= (*i)->getMonthlyMaintenace();
	}

	std::wostringstream
		woststr1,
		woststr2;

	woststr1 << L' ' << L' ';

	if (net > -1)
		woststr2 << L' ' << L' ';
	else
	{
		woststr2 << L'-';
		net = -net;
	}

	_lstTotal->setColumns(4, 122,92,6,54);
	_lstTotal->setDot();
	_lstTotal->addRow(
					4,
					L"",
					tr("STR_TOTAL_GROSS_UC").c_str(),
					woststr1.str().c_str(),
					Text::formatCurrency(gross).c_str());
	_lstTotal->addRow(
					4,
					L"",
					tr("STR_TOTAL_NET_UC").c_str(),
					woststr2.str().c_str(),
					Text::formatCurrency(net).c_str());
}

/**
 * dTor.
 */
FundingState::~FundingState()
{}

/**
 * Returns to the previous screen.
 * @param action - pointer to an Action
 */
void FundingState::btnOkClick(Action*)
{
	_game->popState();
}

}
