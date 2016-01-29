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

#ifndef _USE_MATH_DEFINES
#	define _USE_MATH_DEFINES
#endif

#include "GraphsState.h"

//#include <sstream>

#include "../Engine/Action.h"
#include "../Engine/Game.h"
#include "../Engine/InteractiveSurface.h"
#include "../Engine/LocalizedText.h"
//#include "../Engine/Logger.h"
#include "../Engine/Options.h"
#include "../Engine/Palette.h"
#include "../Engine/Sound.h"
#include "../Engine/Surface.h"
#include "../Engine/Timer.h"

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextList.h"
#include "../Interface/ToggleTextButton.h"

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


namespace OpenXcom
{

static int recallPage = -1;
static size_t recallCountry = 0;
static GraphsUserFactor recallFactor = GUF_DEFAULT;

static int
	SCREEN_OFFSET_y,
	TOGGLEALL_yCnt,
	TOGGLEALL_yReg;

const float GraphsState::PIXELS_y = 126.f;


/**
 * Helper struct for scrolling past GRAPH_BUTTONS.
 */
struct GraphBtnInfo
{
	LocalizedText _label;
	Uint8
		_colorPushed,
		_colorText;
	int
		_actA,
		_actX;
	bool
		_blinkA,
		_blinkX,
		_pushed;

	/// Builds this struct.
	GraphBtnInfo(
			const LocalizedText& label,
			Uint8 colorPushed,
			int actA,
			int actX,
			Uint8 colorText,
			bool blinkA,
			bool blinkX)
		:
			_label(label),
			_colorPushed(colorPushed),
			_actA(actA),
			_actX(actX),
			_colorText(colorText),
			_blinkA(blinkA),
			_blinkX(blinkX),
			_pushed(false)
	{}
};


/**
 * Initializes all the elements in the Graphs screen.
 */
GraphsState::GraphsState()
	:
//		_btnRegionOffset(0),
		_btnCountryOffset(0),
		_init(true),
		_reset(false),
		_forceVis(true)
{
	const int offsetX	= (Options::baseXResolution - 320) / 2;
	SCREEN_OFFSET_y		= (Options::baseYResolution - 200) / 2;

	_bg = new InteractiveSurface(
							Options::baseXResolution,
							Options::baseYResolution,
							-offsetX,
							-SCREEN_OFFSET_y);
	_bg->onMousePress(
				(ActionHandler)& GraphsState::shiftButtons,
				SDL_BUTTON_WHEELUP);
	_bg->onMousePress(
				(ActionHandler)& GraphsState::shiftButtons,
				SDL_BUTTON_WHEELDOWN);
	_bg->onKeyboardPress(
				(ActionHandler)& GraphsState::shiftButtons,
				SDLK_UP);
	_bg->onKeyboardPress(
				(ActionHandler)& GraphsState::shiftButtons,
				SDLK_KP8);
	_bg->onKeyboardPress(
				(ActionHandler)& GraphsState::shiftButtons,
				SDLK_DOWN);
	_bg->onKeyboardPress(
				(ActionHandler)& GraphsState::shiftButtons,
				SDLK_KP2);
	_bg->onKeyboardPress(
				(ActionHandler)& GraphsState::shiftButtons,
				SDLK_PAGEUP);
	_bg->onKeyboardPress(
				(ActionHandler)& GraphsState::shiftButtons,
				SDLK_KP9);
	_bg->onKeyboardPress(
				(ActionHandler)& GraphsState::shiftButtons,
				SDLK_PAGEDOWN);
	_bg->onKeyboardPress(
				(ActionHandler)& GraphsState::shiftButtons,
				SDLK_KP3);
	_bg->onKeyboardPress(
				(ActionHandler)& GraphsState::shiftButtons,
				SDLK_HOME);
	_bg->onKeyboardPress(
				(ActionHandler)& GraphsState::shiftButtons,
				SDLK_KP7);
	_bg->onKeyboardPress(
				(ActionHandler)& GraphsState::shiftButtons,
				SDLK_END);
	_bg->onKeyboardPress(
				(ActionHandler)& GraphsState::shiftButtons,
				SDLK_KP1);
	_bg->onMousePress(
				(ActionHandler)& GraphsState::btnGeoscapeClick,
				SDL_BUTTON_RIGHT);

	SDL_EnableKeyRepeat(
					180, //SDL_DEFAULT_REPEAT_DELAY,
					60); //SDL_DEFAULT_REPEAT_INTERVAL);

	_isfUfoRegion	= new InteractiveSurface(31, 24,  97);
	_isfXcomRegion	= new InteractiveSurface(31, 24, 129);
	_isfUfoCountry	= new InteractiveSurface(31, 24, 161);
	_isfXcomCountry	= new InteractiveSurface(31, 24, 193);
	_isfIncome		= new InteractiveSurface(31, 24, 225);
	_isfFinance		= new InteractiveSurface(31, 24, 257);
	_isfGeoscape	= new InteractiveSurface(31, 24, 289);

	_btnReset		= new TextButton(40, 16, 96, 26);
	_btnToggleAll	= new ToggleTextButton(24, 10, 66, 0); // y is set according to page.

	_btnFactor1		= new TextButton(16, 16, 272, 26);
	_btnFactor2		= new TextButton(16, 16, 288, 26);
	_btnFactor4		= new TextButton(16, 16, 304, 26);

	switch (recallFactor)
	{
		default:
		case GUF_DEFAULT:
			_userFactor = _btnFactor1;
			break;
		case GUF_HALF:
			_userFactor = _btnFactor2;
			break;
		case GUF_QUARTER:
			_userFactor = _btnFactor4;
	}

	_txtTitle		= new Text(220, 16, 100, 28);
	_txtFactor		= new Text( 35,  9,  96, 28);

	_lstMonths		= new TextList(215, 9, 117, 182); // note These go beyond 320px.
	_lstYears		= new TextList(215, 9, 117, 191);

	_txtScore		= new Text(36, 9, 46, 82);


	setInterface("graphs");

	add(_bg);
	add(_isfUfoRegion);
	add(_isfUfoCountry);
	add(_isfXcomRegion);
	add(_isfXcomCountry);
	add(_isfIncome);
	add(_isfFinance);
	add(_isfGeoscape);
	add(_btnReset,		"button",	"graphs");
	add(_btnToggleAll,	"button",	"graphs");
	add(_btnFactor1,	"button",	"graphs");
	add(_btnFactor2,	"button",	"graphs");
	add(_btnFactor4,	"button",	"graphs");
	add(_lstMonths,		"scale",	"graphs");
	add(_lstYears,		"scale",	"graphs");
	add(_txtTitle,		"text",		"graphs");
	add(_txtFactor,		"text",		"graphs");
	add(_txtScore);

	for (size_t
			i = 0;
			i != TEXTS_y;
			++i)
	{
		_txtScale.push_back(new Text(
									32,10,
									91,
									171 - (static_cast<int>(i) * 14)));
		add(_txtScale.at(i), "scale", "graphs");
	}

	static const Uint8
		COL_OFF_BTN = 13,
		COL_OFF_TXT = 16;

	size_t btnOffset = 0;
	Uint8
		color,
		colorOffset = 0;
	int
		actA,
		actX;
	bool
		blinkA,
		blinkX;

	/* REGIONS */
	for (std::vector<Region*>::const_iterator
			i = _game->getSavedGame()->getRegions()->begin();
			i != _game->getSavedGame()->getRegions()->end();
			++i, ++colorOffset, ++btnOffset)
	{
//		if (colorOffset == 17) colorOffset = 0; // <- only 15 regions IG.
		color = colorOffset * 8;

		actA = (*i)->getActivityAlien().back();
		actX = (*i)->getActivityXCom().back();
		blinkA = (*i)->recentActivityAlien(false, true);
		blinkX = (*i)->recentActivityXCom(false, true);

		// put all the regions into toggles
		_regionToggles.push_back(new GraphBtnInfo(
												tr((*i)->getRules()->getType()), // name of Region
												color + COL_OFF_BTN,
												actA,
												actX,
												color + COL_OFF_TXT,
												blinkA,
												blinkX));

		// first add the GRAPH_BUTTONS (having the respective region's information)
		if (btnOffset < GRAPH_BUTTONS) // leave a slot for the TOTAL btn.
		{
			_btnRegions.push_back(new ToggleTextButton(
													65,10,
													0,
													static_cast<int>(btnOffset) * 10));
			_btnRegions.at(btnOffset)->setText(tr((*i)->getRules()->getType())); // name of Region
			_btnRegions.at(btnOffset)->setInvertColor(color + COL_OFF_BTN);
			_btnRegions.at(btnOffset)->onMousePress(
							(ActionHandler)& GraphsState::btnRegionListClick,
							SDL_BUTTON_LEFT);
			add(_btnRegions.at(btnOffset), "button", "graphs");

			_txtRegionActA.push_back(new Text(
											24,10,
											66,
											(static_cast<int>(btnOffset) * 10) + 1));
			_txtRegionActA.at(btnOffset)->setColor(color + COL_OFF_TXT);
			_txtRegionActA.at(btnOffset)->setText(Text::intWide(actA));
			add(_txtRegionActA.at(btnOffset));

			_txtRegionActX.push_back(new Text(
											24,10,
											66,
											(static_cast<int>(btnOffset) * 10) + 1));
			_txtRegionActX.at(btnOffset)->setColor(color + COL_OFF_TXT);
			_txtRegionActX.at(btnOffset)->setText(Text::intWide(actX));
			add(_txtRegionActX.at(btnOffset));

			_blinkRegionAlien.push_back(blinkA);
			_blinkRegionXCom.push_back(blinkX);
		}


		_alienRegionLines.push_back(new Surface(320,200));
		add(_alienRegionLines.at(btnOffset));

		_xcomRegionLines.push_back(new Surface(320,200));
		add(_xcomRegionLines.at(btnOffset));
	}


	int btnTotal_y;
	if (_regionToggles.size() < GRAPH_BUTTONS)
		btnTotal_y = static_cast<int>(_regionToggles.size());
	else
		btnTotal_y = static_cast<int>(GRAPH_BUTTONS) * 10;

	_btnRegionTotal = new ToggleTextButton( // TOTAL btn.
										65,10,
										0,
										btnTotal_y * 10);

	color = static_cast<Uint8>(
			_game->getRuleset()->getInterface("graphs")->getElement("regionTotal")->color);

	_regionToggles.push_back(new GraphBtnInfo( // TOTAL btn is the last button in the vector.
											tr("STR_TOTAL_UC"),
											color,
											0,0,0,
											false,false));
	_btnRegionTotal->setInvertColor(color);
	_btnRegionTotal->setText(tr("STR_TOTAL_UC"));
	_btnRegionTotal->onMousePress(
					(ActionHandler)& GraphsState::btnRegionListClick,
					SDL_BUTTON_LEFT);

	add(_btnRegionTotal, "button", "graphs");

	_alienRegionLines.push_back(new Surface(320,200));
	add(_alienRegionLines.at(btnOffset));

	_xcomRegionLines.push_back(new Surface(320,200));
	add(_xcomRegionLines.at(btnOffset));


	btnOffset = 0;
	colorOffset = 0;

	/* COUNTRIES */
	for (std::vector<Country*>::const_iterator
			i = _game->getSavedGame()->getCountries()->begin();
			i != _game->getSavedGame()->getCountries()->end();
			++i, ++colorOffset, ++btnOffset)
	{
		if (colorOffset == 17) colorOffset = 0;
		color = colorOffset * 8;

		actA = (*i)->getActivityAlien().back();
		actX = (*i)->getActivityXCom().back();
		blinkA = (*i)->recentActivityAlien(false, true);
		blinkX = (*i)->recentActivityXCom(false, true);

		// put all the countries into toggles
		_countryToggles.push_back(new GraphBtnInfo(
												tr((*i)->getRules()->getType()), // name of Country
												color + COL_OFF_BTN,
												actA,
												actX,
												color + COL_OFF_TXT,
												blinkA,
												blinkX));

		// first add the GRAPH_BUTTONS (having the respective country's information)
		if (btnOffset < GRAPH_BUTTONS) // leave a slot for the TOTAL btn.
		{
			_btnCountries.push_back(new ToggleTextButton(
													65,10,
													0,
													static_cast<int>(btnOffset) * 10));
			_btnCountries.at(btnOffset)->setText(tr((*i)->getRules()->getType())); // name of Country
			_btnCountries.at(btnOffset)->setInvertColor(color + COL_OFF_BTN);
			_btnCountries.at(btnOffset)->onMousePress(
							(ActionHandler)& GraphsState::btnCountryListClick,
							SDL_BUTTON_LEFT);
			add(_btnCountries.at(btnOffset), "button", "graphs");

			_txtCountryActA.push_back(new Text(
											24,10,
											66,
											(static_cast<int>(btnOffset) * 10) + 1));
			_txtCountryActA.at(btnOffset)->setColor(color + COL_OFF_TXT);
			_txtCountryActA.at(btnOffset)->setText(Text::intWide(actA));
			add(_txtCountryActA.at(btnOffset));

			_txtCountryActX.push_back(new Text(
											24,10,
											66,
											(static_cast<int>(btnOffset) * 10) + 1));
			_txtCountryActX.at(btnOffset)->setColor(color + COL_OFF_TXT);
			_txtCountryActX.at(btnOffset)->setText(Text::intWide(actX));
			add(_txtCountryActX.at(btnOffset));

			_blinkCountryAlien.push_back(blinkA);
			_blinkCountryXCom.push_back(blinkX);
		}


		_alienCountryLines.push_back(new Surface(320,200));
		add(_alienCountryLines.at(btnOffset));

		_xcomCountryLines.push_back(new Surface(320,200));
		add(_xcomCountryLines.at(btnOffset));

		_incomeLines.push_back(new Surface(320,200));
		add(_incomeLines.at(btnOffset));
	}


	if (_countryToggles.size() < GRAPH_BUTTONS)
		btnTotal_y = static_cast<int>(_countryToggles.size());
	else
		btnTotal_y = static_cast<int>(GRAPH_BUTTONS);

	_btnCountryTotal = new ToggleTextButton( // TOTAL btn.
										65,10,
										0,
										btnTotal_y * 10);

	color = static_cast<Uint8>(
			_game->getRuleset()->getInterface("graphs")->getElement("countryTotal")->color);

	_countryToggles.push_back(new GraphBtnInfo( // TOTAL btn is the last button in the vector.
											tr("STR_TOTAL_UC"),
											color,
											0,0,0,
											false,false));

	_btnCountryTotal->setInvertColor(color);
	_btnCountryTotal->setText(tr("STR_TOTAL_UC"));
	_btnCountryTotal->onMousePress(
					(ActionHandler)& GraphsState::btnCountryListClick,
					SDL_BUTTON_LEFT);

	add(_btnCountryTotal, "button", "graphs");

	_alienCountryLines.push_back(new Surface(320,200));
	add(_alienCountryLines.at(btnOffset));

	_xcomCountryLines.push_back(new Surface(320,200));
	add(_xcomCountryLines.at(btnOffset));

	_incomeLines.push_back(new Surface(320,200));
	add(_incomeLines.at(btnOffset));


	/* FINANCE */
	for (size_t
			i = 0;
			i != 5u;
			++i)
	{
		_btnFinances.push_back(new ToggleTextButton(
												82,16,
												0,
												static_cast<int>(i) * 16));
		_financeToggles.push_back(false);

		Uint8 multer; // switch colors for Income (was yellow) and Maintenance (was green)
		switch (i)
		{
			case 0: multer = 2; break;
			case 2: multer = 0; break;
			default:
				multer = static_cast<Uint8>(i);
		}

		_btnFinances.at(i)->setInvertColor(multer * 8 + COL_OFF_BTN);
		_btnFinances.at(i)->onMousePress((ActionHandler)& GraphsState::btnFinanceListClick);

		add(_btnFinances.at(i), "button", "graphs");

		_financeLines.push_back(new Surface(320,200));
		add(_financeLines.at(i));
	}

	_btnFinances.at(0)->setText(tr("STR_INCOME"));
	_btnFinances.at(1)->setText(tr("STR_EXPENDITURE"));
	_btnFinances.at(2)->setText(tr("STR_MAINTENANCE"));
	_btnFinances.at(3)->setText(tr("STR_BALANCE"));
	_btnFinances.at(4)->setText(tr("STR_SCORE"));

	_txtScore->setColor(49);
	_txtScore->setAlign(ALIGN_RIGHT);


	// Load back all the buttons' toggled states from SavedGame!

	/* REGION TOGGLES */
	std::string graphRegionToggles = _game->getSavedGame()->getGraphRegionToggles();
	while (graphRegionToggles.size() < _regionToggles.size())
	{
		graphRegionToggles.push_back('0');
	}

	for (size_t
			i = 0;
			i != _regionToggles.size();
			++i)
	{
		_regionToggles[i]->_pushed = (graphRegionToggles[i] == '0') ? false : true;

		if (_regionToggles.size() - 1 == i)
			_btnRegionTotal->setPressed(_regionToggles[i]->_pushed);
		else if (i < GRAPH_BUTTONS)
			_btnRegions.at(i)->setPressed(_regionToggles[i]->_pushed);
	}

	/* COUNTRY TOGGLES */
	std::string graphCountryToggles = _game->getSavedGame()->getGraphCountryToggles();
	while (graphCountryToggles.size() < _countryToggles.size())
	{
		graphCountryToggles.push_back('0');
	}

	for (size_t
			i = 0;
			i != _countryToggles.size();
			++i)
	{
		_countryToggles[i]->_pushed = (graphCountryToggles[i] == '0') ? false : true;

		if (_countryToggles.size() - 1 == i)
			_btnCountryTotal->setPressed(_countryToggles[i]->_pushed);
		else if (i < GRAPH_BUTTONS)
			_btnCountries.at(i)->setPressed(_countryToggles[i]->_pushed);
	}

	/* FINANCE TOGGLES */
	std::string graphFinanceToggles = _game->getSavedGame()->getGraphFinanceToggles();
	while (graphFinanceToggles.size() < _financeToggles.size())
	{
		graphFinanceToggles.push_back('0');
	}

	for (size_t
			i = 0;
			i != _financeToggles.size();
			++i)
	{
		_financeToggles[i] = (graphFinanceToggles[i] == '0') ? false : true;
		_btnFinances.at(i)->setPressed(_financeToggles[i]);
	}

	_btnReset->setText(tr("STR_RESET_UC"));
	_btnReset->onMousePress(
					(ActionHandler)& GraphsState::btnResetPress,
					SDL_BUTTON_LEFT);

//	_btnToggleAll->setText(L"all"); // TODO: use tr("STR_GRAPHS_TOGGLE_ALL")
	_btnToggleAll->onMousePress(
					(ActionHandler)& GraphsState::btnTogglePress,
					SDL_BUTTON_LEFT);

	_btnFactor1->setText(L"1");
	_btnFactor1->setGroup(&_userFactor);
	_btnFactor1->onMousePress(
					(ActionHandler)& GraphsState::btnFactorPress,
					SDL_BUTTON_LEFT);
	_btnFactor1->onKeyboardPress(
					(ActionHandler)& GraphsState::keyFactor,
					SDLK_1);

	_btnFactor2->setText(L"2");
	_btnFactor2->setGroup(&_userFactor);
	_btnFactor2->onMousePress(
					(ActionHandler)& GraphsState::btnFactorPress,
					SDL_BUTTON_LEFT);
	_btnFactor1->onKeyboardPress(
					(ActionHandler)& GraphsState::keyFactor,
					SDLK_2);

	_btnFactor4->setText(L"3");
	_btnFactor4->setGroup(&_userFactor);
	_btnFactor4->onMousePress(
					(ActionHandler)& GraphsState::btnFactorPress,
					SDL_BUTTON_LEFT);
	_btnFactor1->onKeyboardPress(
					(ActionHandler)& GraphsState::keyFactor,
					SDLK_3);

	Surface* const icons = _game->getResourcePack()->getSurface("GRAPHS.SPK");
	icons->setX(offsetX);
	icons->setY(SCREEN_OFFSET_y);
	icons->blit(_bg);

	const Uint8 colorGrid = static_cast<Uint8>(
						   _game->getRuleset()->getInterface("graphs")->getElement("graph")->color);

	_bg->drawRect( // set up the grid
				offsetX + 125,
				SCREEN_OFFSET_y + 49,
				188,127,
				colorGrid);

	for (Sint16
			i = 0;
			i != 5;
			++i)
	{
		for (Sint16
				y = SCREEN_OFFSET_y + 50 + i;
				y < SCREEN_OFFSET_y + 164 + i;
				y += 14)
		{
			for (Sint16
					x = offsetX + 126 + i;
					x < offsetX + 298 + i;
					x += 17)
			{
				if (i == 4)
					color = 0;
				else
					color = colorGrid + 1 + static_cast<Uint8>(i);

				_bg->drawRect(
							x,y,
							16 - (i * 2),
							13 - (i * 2),
							color);
			}
		}
	}

	// i know using textlist for this is ugly and brutal, but YOU try getting
	// this damn text to line up. Also, there's nothing wrong with being ugly or
	// brutal, you should learn tolerance. kL_note: and C++
	_lstMonths->setColumns(MONTHS, 17,17,17,17,17,17,17,17,17,17,17,17); // 204 total
	_lstMonths->addRow(MONTHS, L" ",L" ",L" ",L" ",L" ",L" ",L" ",L" ",L" ",L" ",L" ",L" ");
	_lstMonths->setMargin();

	_lstYears->setColumns(YEARS, 34,34,34,34,34,34); // 204 total
	_lstYears->addRow(YEARS, L".",L".",L".",L".",L".",L".");
	_lstYears->setMargin();


	const GameTime* const gt = _game->getSavedGame()->getTime();
	const int yr = gt->getYear();
	size_t th = static_cast<size_t>(gt->getMonth() - 1);

	for (size_t
			i = 0;
			i != MONTHS_u;
			++i)
	{
		if (i == 1)
		{
			if (th == 11)
				_lstYears->setCellText(0,0, Text::intWide(yr));
			else
				_lstYears->setCellText(0,0, Text::intWide(yr - 1));
		}
		else if (th == 0)
			_lstYears->setCellText(0, i / 2, Text::intWide(yr));

		if (++th == 12) th = 0;

		_lstMonths->setCellText(0, i, tr(GameTime::GAME_MONTHS[th]));
	}

	for (std::vector<Text*>::const_iterator // set up the vertical measurement units
			i = _txtScale.begin();
			i != _txtScale.end();
			++i)
	{
		(*i)->setAlign(ALIGN_RIGHT);
	}


	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setBig();

	_txtFactor->setText(L"$1000");


	_isfUfoRegion->onMousePress(
					(ActionHandler)& GraphsState::btnUfoRegionClick,
					SDL_BUTTON_LEFT);
	_isfXcomRegion->onMousePress(
					(ActionHandler)& GraphsState::btnXcomRegionClick,
					SDL_BUTTON_LEFT);
	_isfUfoCountry->onMousePress(
					(ActionHandler)& GraphsState::btnUfoCountryClick,
					SDL_BUTTON_LEFT);
	_isfXcomCountry->onMousePress(
					(ActionHandler)& GraphsState::btnXcomCountryClick,
					SDL_BUTTON_LEFT);

	_isfIncome->onMousePress(
					(ActionHandler)& GraphsState::btnIncomeClick,
					SDL_BUTTON_LEFT);
	_isfFinance->onMousePress(
					(ActionHandler)& GraphsState::btnFinanceClick,
					SDL_BUTTON_LEFT);

	_isfGeoscape->onMousePress(
					(ActionHandler)& GraphsState::btnGeoscapeClick,
					SDL_BUTTON_LEFT);
	_isfGeoscape->onKeyboardPress(
					(ActionHandler)& GraphsState::btnGeoscapeClick,
					Options::keyCancel);
	_isfGeoscape->onKeyboardPress(
					(ActionHandler)& GraphsState::btnGeoscapeClick,
					Options::keyOk);
	_isfGeoscape->onKeyboardPress(
					(ActionHandler)& GraphsState::btnGeoscapeClick,
					Options::keyOkKeypad);
	_isfGeoscape->onKeyboardPress(
					(ActionHandler)& GraphsState::btnGeoscapeClick,
					Options::keyGeoGraphs);

	centerAllSurfaces();


	_blinkTimer = new Timer(250);
	_blinkTimer->onTimer((StateHandler)& GraphsState::blink);

	initButtons();

	TOGGLEALL_yReg = static_cast<int>(_btnRegions.size()) * 10;
	TOGGLEALL_yCnt = static_cast<int>(_btnCountries.size()) * 10;

	switch (recallPage)
	{
		default:
		case 0: btnUfoRegionClick(nullptr);		break;
		case 1: btnXcomRegionClick(nullptr);	break;
		case 2: btnUfoCountryClick(nullptr);	break;
		case 3: btnXcomCountryClick(nullptr);	break;
		case 4: btnIncomeClick(nullptr);		break;
		case 5: btnFinanceClick(nullptr);
	}
}

/**
 * dTor.
 */
GraphsState::~GraphsState()
{
	delete _blinkTimer;

	std::string toggles;
	for (size_t
			i = 0;
			i != _regionToggles.size();
			++i)
	{
		toggles.push_back(_regionToggles[i]->_pushed ? '1' : '0');
		delete _regionToggles[i];
	}
	_game->getSavedGame()->setGraphRegionToggles(toggles);

	toggles.clear();
	for (size_t
			i = 0;
			i != _countryToggles.size();
			++i)
	{
		toggles.push_back(_countryToggles[i]->_pushed ? '1' : '0');
		delete _countryToggles[i];
	}
	_game->getSavedGame()->setGraphCountryToggles(toggles);

	toggles.clear();
	for (size_t
			i = 0;
			i != _financeToggles.size();
			++i)
	{
		toggles.push_back(_financeToggles[i] ? '1' : '0');
	}
	_game->getSavedGame()->setGraphFinanceToggles(toggles);
}

/**
 * Shifts buttons to their pre-Graph cTor row.
 * @note Countries only since total Regions still fit on the list.
 */
void GraphsState::initButtons() // private.
{
//	if (_countryToggles.size() > GRAPH_BUTTONS)
	scrollButtons(static_cast<int>(recallCountry), true);

	for (std::vector<GraphBtnInfo*>::const_iterator
			i = _regionToggles.begin();
			i != _regionToggles.end();
			++i)
	{
		if ((*i)->_blinkA == true || (*i)->_blinkX == true)
		{
			_blinkTimer->start();
			return;
		}
	}
/*	for (std::vector<GraphBtnInfo*>::const_iterator	// not needed because Country-areas are all subsumed within Regions;
			i = _countryToggles.begin();			// that is, if a country is blinking its region will already be blinking.
			i != _countryToggles.end();				// So just start the gosh-darned timer.
			++i)
	{
		if ((*i)->_blinkA == true || (*i)->_blinkX == true)
		{
			_blinkTimer->start();
			return;
		}
	} */
}

/**
 * Handles state thinking.
 */
void GraphsState::think()
{
	_blinkTimer->think(this, nullptr);
}

/**
 * Makes recent activity Text blink.
 */
void GraphsState::blink() // private.
{
	static bool vis (true);

	if (_reset == true)
		vis = true;
	else if (_forceVis == true)
	{
		_forceVis = false;
		vis = true;
	}
	else
		vis = !vis;

	size_t offset = 0;

	if (_alien == true
		&& _income == false
		&& _country == false
		&& _finance == false)
	{
		for (std::vector<bool>::const_iterator
				i = _blinkRegionAlien.begin();
				i != _blinkRegionAlien.end();
				++i, ++offset)
		{
			if (*i == true)
				_txtRegionActA.at(offset)->setVisible(vis);
			else
				_txtRegionActA.at(offset)->setVisible();
		}
	}
	else if (_alien == true
		&& _income == false
		&& _country == true
		&& _finance == false)
	{
		for (std::vector<bool>::const_iterator
				i = _blinkCountryAlien.begin();
				i != _blinkCountryAlien.end();
				++i, ++offset)
		{
			if (*i == true)
				_txtCountryActA.at(offset)->setVisible(vis);
			else
				_txtCountryActA.at(offset)->setVisible();
		}
	}
	else if (_alien == false
		&& _income == false
		&& _country == false
		&& _finance == false)
	{
		for (std::vector<bool>::const_iterator
				i = _blinkRegionXCom.begin();
				i != _blinkRegionXCom.end();
				++i, ++offset)
		{
			if (*i == true)
				_txtRegionActX.at(offset)->setVisible(vis);
			else
				_txtRegionActX.at(offset)->setVisible();
		}
	}
	else if (_alien == false
		&& _income == false
		&& _country == true
		&& _finance == false)
	{
		for (std::vector<bool>::const_iterator
				i = _blinkCountryXCom.begin();
				i != _blinkCountryXCom.end();
				++i, ++offset)
		{
			if (*i == true)
				_txtCountryActX.at(offset)->setVisible(vis);
			else
				_txtCountryActX.at(offset)->setVisible();
		}
	}

	if (_reset == true)
		_blinkTimer->stop();
}

/**
 * Returns to the previous screen.
 * @param action - pointer to an Action
 */
void GraphsState::btnGeoscapeClick(Action*)
{
	SDL_EnableKeyRepeat(0, SDL_DEFAULT_REPEAT_INTERVAL);

	_game->popState();
	kL_soundPop->play(Mix_GroupAvailable(0));
}

/**
 * Switches to the UFO Regional Activity screen.
 * @param action - pointer to an Action
 */
void GraphsState::btnUfoRegionClick(Action*)
{
	if (recallPage != 0 || _init == true)
	{
		_init = false;
		recallPage = 0;

		_forceVis =

		_alien = true;
		_income =
		_country =
		_finance = false;

		_btnToggleAll->setY(TOGGLEALL_yReg + SCREEN_OFFSET_y);
		_btnToggleAll->setVisible();
		initToggleAll();

		_btnReset->setVisible(_blinkTimer->isRunning() == true);
		drawLines();

		_txtTitle->setText(tr("STR_UFO_ACTIVITY_IN_AREAS"));
		_btnRegionTotal->setVisible();

		for (std::vector<ToggleTextButton*>::const_iterator
				i = _btnRegions.begin();
				i != _btnRegions.end();
				++i)
			(*i)->setVisible();

		for (std::vector<Text*>::const_iterator
				i = _txtRegionActA.begin();
				i != _txtRegionActA.end();
				++i)
			(*i)->setVisible();
	}
}

/**
 * Switches to the xCom Regional Activity screen.
 * @param action - pointer to an Action
 */
void GraphsState::btnXcomRegionClick(Action*)
{
	if (recallPage != 1 || _init == true)
	{
		_init = false;
		recallPage = 1;

		_forceVis = true;

		_alien =
		_income =
		_country =
		_finance = false;

		_btnToggleAll->setY(TOGGLEALL_yReg + SCREEN_OFFSET_y);
		_btnToggleAll->setVisible();
		initToggleAll();

		_btnReset->setVisible(_blinkTimer->isRunning() == true);
		drawLines();

		_txtTitle->setText(tr("STR_XCOM_ACTIVITY_IN_AREAS"));
		_btnRegionTotal->setVisible();

		for (std::vector<ToggleTextButton*>::const_iterator
				i = _btnRegions.begin();
				i != _btnRegions.end();
				++i)
			(*i)->setVisible();

		for (std::vector<Text*>::const_iterator
				i = _txtRegionActX.begin();
				i != _txtRegionActX.end();
				++i)
			(*i)->setVisible();
	}
}

/**
 * Switches to the UFO Country Activity screen.
 * @param action - pointer to an Action
 */
void GraphsState::btnUfoCountryClick(Action*)
{
	if (recallPage != 2 || _init == true)
	{
		_init = false;
		recallPage = 2;

		_forceVis =

		_alien =
		_country = true;
		_income =
		_finance = false;

		_btnToggleAll->setY(TOGGLEALL_yCnt + SCREEN_OFFSET_y);
		_btnToggleAll->setVisible();
		initToggleAll();

		_btnReset->setVisible(_blinkTimer->isRunning() == true);
		drawLines();

		_txtTitle->setText(tr("STR_UFO_ACTIVITY_IN_COUNTRIES"));
		_btnCountryTotal->setVisible();

		for (std::vector<ToggleTextButton*>::const_iterator
				i = _btnCountries.begin();
				i != _btnCountries.end();
				++i)
			(*i)->setVisible();

		for (std::vector<Text*>::const_iterator
				i = _txtCountryActA.begin();
				i != _txtCountryActA.end();
				++i)
			(*i)->setVisible();
	}
}

/**
 * Switches to the xCom Country Activity screen.
 * @param action - pointer to an Action
 */
void GraphsState::btnXcomCountryClick(Action*)
{
	if (recallPage != 3 || _init == true)
	{
		_init = false;
		recallPage = 3;

		_forceVis =

		_country = true;
		_alien =
		_income =
		_finance = false;

		_btnToggleAll->setY(TOGGLEALL_yCnt + SCREEN_OFFSET_y);
		_btnToggleAll->setVisible();
		initToggleAll();

		_btnReset->setVisible(_blinkTimer->isRunning() == true);
		drawLines();

		_txtTitle->setText(tr("STR_XCOM_ACTIVITY_IN_COUNTRIES"));
		_btnCountryTotal->setVisible();

		for (std::vector<ToggleTextButton*>::const_iterator
				i = _btnCountries.begin();
				i != _btnCountries.end();
				++i)
			(*i)->setVisible();

		for (std::vector<Text*>::const_iterator
				i = _txtCountryActX.begin();
				i != _txtCountryActX.end();
				++i)
			(*i)->setVisible();
	}
}

/**
 * Switches to the Income screen.
 * @param action - pointer to an Action
 */
void GraphsState::btnIncomeClick(Action*)
{
	if (recallPage != 4 || _init == true)
	{
		_init = false;
		recallPage = 4;

		_income =
		_country = true;
		_alien =
		_finance = false;

		_btnToggleAll->setVisible(false);

		_btnReset->setVisible(false);
		drawLines();

		_txtFactor->setVisible();

		_txtTitle->setText(tr("STR_INCOME"));
		_btnCountryTotal->setVisible();

		for (std::vector<ToggleTextButton*>::const_iterator
				i = _btnCountries.begin();
				i != _btnCountries.end();
				++i)
			(*i)->setVisible();
	}
}

/**
 * Switches to the Finances screen.
 * @param action - pointer to an Action
 */
void GraphsState::btnFinanceClick(Action*)
{
	if (recallPage != 5 || _init == true)
	{
		_init = false;
		recallPage = 5;

		_finance = true;
		_alien =
		_income =
		_country = false;

		_btnToggleAll->setVisible(false);

		_btnReset->setVisible(false);
		drawLines();

		_txtTitle->setText(tr("STR_FINANCE"));
		_txtScore->setVisible();

		for (std::vector<ToggleTextButton*>::const_iterator
				i = _btnFinances.begin();
				i != _btnFinances.end();
				++i)
			(*i)->setVisible();
	}
}

/**
 * Handles a click on a region button.
 * @param action - pointer to an Action
 */
void GraphsState::btnRegionListClick(Action* action)
{
	ToggleTextButton* btn = dynamic_cast<ToggleTextButton*>(action->getSender());
	size_t btnId;

	if (btn == _btnRegionTotal)
		btnId = _regionToggles.size() - 1;
	else
	{
		for (
				btnId = 0;
				btnId != _btnRegions.size();
				++btnId)
		{
			if (_btnRegions[btnId] == btn)
				break;
		}
	}
	_regionToggles.at(btnId)->_pushed = btn->getPressed();
	initToggleAll();
	drawLines(false);
}

/**
 * Handles a click on a country button.
 * @param action - pointer to an Action
 */
void GraphsState::btnCountryListClick(Action* action)
{
	ToggleTextButton* btn = dynamic_cast<ToggleTextButton*>(action->getSender());
	size_t btnId;

	if (btn == _btnCountryTotal)
		btnId = _countryToggles.size() - 1;
	else
	{
		for (
				btnId = 0;
				btnId != _btnCountries.size();
				++btnId)
		{
			if (_btnCountries[btnId] == btn)
			{
				btnId += _btnCountryOffset;
				break;
			}
		}
	}
	_countryToggles.at(btnId)->_pushed = btn->getPressed();
	initToggleAll();
	drawLines(false);
}

/**
 * Handles a click on a finance button.
 * @param action - pointer to an Action
 */
void GraphsState::btnFinanceListClick(Action* action)
{
	ToggleTextButton* btn = dynamic_cast<ToggleTextButton*>(action->getSender());
	size_t btnId;

	for (
			btnId = 0;
			btnId != _btnFinances.size();
			++btnId)
	{
		if (_btnFinances[btnId] == btn)
			break;
	}
	_financeLines.at(btnId)->setVisible(_financeToggles.at(btnId) == false);
	_financeToggles.at(btnId) = btn->getPressed();
	drawLines(false);
}

/**
 * Resets aLien/xCom activity and the blink indicators.
 * @param action - pointer to an Action
 */
void GraphsState::btnResetPress(Action*) // private.
{
	_reset = true;
	_btnReset->setVisible(false);

	for (std::vector<Region*>::const_iterator
			i = _game->getSavedGame()->getRegions()->begin();
			i != _game->getSavedGame()->getRegions()->end();
			++i)
		(*i)->resetActivity();

	for (std::vector<Country*>::const_iterator
			i = _game->getSavedGame()->getCountries()->begin();
			i != _game->getSavedGame()->getCountries()->end();
			++i)
		(*i)->resetActivity();
}

/**
 * Initializes the toggle-all stuff.
 */
void GraphsState::initToggleAll() // private.
{
	bool allPressed = true;
	if (_country == true)
	{
		for (std::vector<GraphBtnInfo*>::const_iterator
				i = _countryToggles.begin();
				i != _countryToggles.end() - 1; // exclude Total btn.
				++i)
		{
			if ((*i)->_pushed == false)
			{
				allPressed = false;
				break;
			}
		}
	}
	else
	{
		for (std::vector<GraphBtnInfo*>::const_iterator
				i = _regionToggles.begin();
				i != _regionToggles.end() - 1; // exclude Total btn.
				++i)
		{
			if ((*i)->_pushed == false)
			{
				allPressed = false;
				break;
			}
		}
	}

	_btnToggleAll->setPressed(allPressed);
}

/**
 * Toggles all region/country buttons.
 * @param action - pointer to an Action
 */
void GraphsState::btnTogglePress(Action*) // private.
{
	bool vis;
	if (_btnToggleAll->getPressed() == true)
		vis = true;
	else
		vis = false;

	if (_country == true)
	{
		for (size_t
				i = 0;
				i != _game->getSavedGame()->getCountries()->size();
				++i)
			_countryToggles.at(i)->_pushed = vis;

		for (std::vector<ToggleTextButton*>::const_iterator
				i = _btnCountries.begin();
				i != _btnCountries.end();
				++i)
			(*i)->setPressed(vis);
	}
	else
	{
		for (size_t
				i = 0;
				i != _game->getSavedGame()->getRegions()->size();
				++i)
			_regionToggles.at(i)->_pushed = vis;

		for (std::vector<ToggleTextButton*>::const_iterator
				i = _btnRegions.begin();
				i != _btnRegions.end();
				++i)
			(*i)->setPressed(vis);
	}

	drawLines(false);
}

/**
 * Sets the graphs to a user expansion.
 * @param action - pointer to an Action
 */
void GraphsState::btnFactorPress(Action* action) // private.
{
	if (action->getSender() == _btnFactor1)
		recallFactor = GUF_DEFAULT;
	else if (action->getSender() == _btnFactor2)
		recallFactor = GUF_HALF;
	else if (action->getSender() == _btnFactor4)
		recallFactor = GUF_QUARTER;

	drawLines(false);
}

/**
 * Sets the graphs to a user expansion by hot-key.
 * @param action - pointer to an Action
 */
void GraphsState::keyFactor(Action* action)
{
	SDL_Event ev; // need to fake a mouse-click for the group to toggle
	ev.type = SDL_MOUSEBUTTONDOWN;
	ev.button.button = SDL_BUTTON_LEFT;

	Action act = Action(&ev, 0.,0., 0,0);

	if (action->getDetails()->key.keysym.sym == SDLK_1)
	{
		recallFactor = GUF_DEFAULT;
		_btnFactor1->mousePress(&act, this);
	}
	else if (action->getDetails()->key.keysym.sym == SDLK_2)
	{
		recallFactor = GUF_HALF;
		_btnFactor2->mousePress(&act, this);
	}
	else if (action->getDetails()->key.keysym.sym == SDLK_3)
	{
		recallFactor = GUF_QUARTER;
		_btnFactor4->mousePress(&act, this);
	}

	drawLines(false);
}

/**
 * Remove all elements from view.
 */
void GraphsState::resetScreen() // private.
{
	for (std::vector<Surface*>::const_iterator
			i = _alienRegionLines.begin();
			i != _alienRegionLines.end();
			++i)
		(*i)->setVisible(false);

	for (std::vector<Surface*>::const_iterator
			i = _alienCountryLines.begin();
			i != _alienCountryLines.end();
			++i)
		(*i)->setVisible(false);

	for (std::vector<Surface*>::const_iterator
			i = _xcomRegionLines.begin();
			i != _xcomRegionLines.end();
			++i)
		(*i)->setVisible(false);

	for (std::vector<Surface*>::const_iterator
			i = _xcomCountryLines.begin();
			i != _xcomCountryLines.end();
			++i)
		(*i)->setVisible(false);

	for (std::vector<Surface*>::const_iterator
			i = _incomeLines.begin();
			i != _incomeLines.end();
			++i)
		(*i)->setVisible(false);

	for (std::vector<Surface*>::const_iterator
			i = _financeLines.begin();
			i != _financeLines.end();
			++i)
		(*i)->setVisible(false);


	for (std::vector<ToggleTextButton*>::const_iterator
			i = _btnRegions.begin();
			i != _btnRegions.end();
			++i)
		(*i)->setVisible(false);

	for (std::vector<ToggleTextButton*>::const_iterator
			i = _btnCountries.begin();
			i != _btnCountries.end();
			++i)
		(*i)->setVisible(false);

	for (std::vector<ToggleTextButton*>::const_iterator
			i = _btnFinances.begin();
			i != _btnFinances.end();
			++i)
		(*i)->setVisible(false);


	for (std::vector<Text*>::const_iterator
			i = _txtRegionActA.begin();
			i != _txtRegionActA.end();
			++i)
		(*i)->setVisible(false);

	for (std::vector<Text*>::const_iterator
			i = _txtCountryActA.begin();
			i != _txtCountryActA.end();
			++i)
		(*i)->setVisible(false);

	for (std::vector<Text*>::const_iterator
			i = _txtRegionActX.begin();
			i != _txtRegionActX.end();
			++i)
		(*i)->setVisible(false);

	for (std::vector<Text*>::const_iterator
			i = _txtCountryActX.begin();
			i != _txtCountryActX.end();
			++i)
		(*i)->setVisible(false);


	_btnRegionTotal->setVisible(false);
	_btnCountryTotal->setVisible(false);
	_txtFactor->setVisible(false);
	_txtScore->setVisible(false);
}

/**
 * Clears pixels of lines that would otherwise draw overtop the title area.
 * @param srf - pointer to a Surface w/ lines to box in
 */
void GraphsState::boxLines(Surface* const srf) // private.
{
	for (int
			y = 0;		// top of screen
			y != 49;	// top of grid
			++y)
	{
		for (int
				x = 125;	// grid left edge
				x != 313;	// grid right edge
				++x)
		{
			srf->setPixelColor(x,y, 0);
		}
	}
}

/**
 * Updates the text on the vertical scale.
 * @param valLow	- minimum value
 * @param valHigh	- maximum value
 */
void GraphsState::updateScale( // private.
		int valLow,
		int valHigh)
{
	const int delta = std::max(10,
							  (valHigh - valLow) / GRIDCELLS_y);
	for (size_t
			i = 0;
			i != TEXTS_y;
			++i)
	{
		_txtScale.at(i)->setText(Text::intWide(valLow));
		valLow += delta;
	}
}

/**
 * Instead of having all the line drawing in one giant ridiculous routine only
 * call the required routine.
 * @param reset - true if lines needs to be cleared first (default true)
 */
void GraphsState::drawLines(bool reset) // private.
{
	if (reset == true)
		resetScreen();

	if (_country == false && _finance == false)
		drawRegionLines();
	else if (_finance == false)
		drawCountryLines();
	else
		drawFinanceLines();
}

/**
 * Sets up the screens and draws the lines for region buttons to toggle on and off.
 */
void GraphsState::drawRegionLines() // private.
{
	int // calculate totals and set the upward maximum
		scaleHigh = 0,
		scaleLow = 0,
		total,
		act,

		totals[MONTHS_u] = {0,0,0,0,0,0,0,0,0,0,0,0};

	for (size_t
			i = 0;
			i != _game->getSavedGame()->getFundsList().size();
			++i)
	{
		total = 0;

		for (size_t
				j = 0;
				j != _game->getSavedGame()->getRegions()->size();
				++j)
		{
			if (_alien == true)
				act = _game->getSavedGame()->getRegions()->at(j)->getActivityAlien().at(i);
			else
				act = _game->getSavedGame()->getRegions()->at(j)->getActivityXCom().at(i);

			total += act;

			if (act > scaleHigh
				&& _regionToggles.at(j)->_pushed == true)
			{
				scaleHigh = act;
			}

			if (_alien == false // aLiens never get into negative scores.
				&& act < scaleLow
				&& _regionToggles.at(j)->_pushed == true)
			{
				scaleLow = act;
			}
		}

		if (_regionToggles.back()->_pushed == true
			&& total > scaleHigh)
		{
			scaleHigh = total;
		}
	}


	const int low = scaleLow; // adjust the scale to fit the upward maximum
	int delta = scaleHigh - scaleLow;

	switch (recallFactor)
	{
		case GUF_QUARTER:
			delta >>= 1; // no break.
		case GUF_HALF:
			delta >>= 1;
	}

	int test = 10;
	while (delta > GRIDCELLS_y * test)
		test += 10;

	scaleLow = 0;
	scaleHigh = GRIDCELLS_y * test;

	while (low < scaleLow)
	{
		scaleLow -= test;
		scaleHigh -= test;
	}

	// Figure out how many units to the pixel then plot the points for the graph
	// and connect the dots.
	const float pixelUnits = static_cast<float>(scaleHigh - scaleLow) / PIXELS_y;

	int reduction;
	Sint16
		x,y;
	std::vector<Sint16> lineVector;
	Region* region;

	for (size_t // draw region lines
			i = 0;
			i != _game->getSavedGame()->getRegions()->size();
			++i)
	{
		region = _game->getSavedGame()->getRegions()->at(i);

		_alienRegionLines.at(i)->clear();
		_xcomRegionLines.at(i)->clear();

		lineVector.clear();

		for (size_t
				j = 0;
				j != MONTHS_u;
				++j)
		{
			y = 175 + static_cast<Sint16>(Round(static_cast<float>(scaleLow) / pixelUnits));

			if (_alien == true)
			{
				if (j < region->getActivityAlien().size())
				{
					reduction = region->getActivityAlien().at(region->getActivityAlien().size() - (j + 1));
					y -= static_cast<Sint16>(Round(static_cast<float>(reduction) / pixelUnits));
					totals[j] += reduction;
				}
			}
			else
			{
				if (j < region->getActivityXCom().size())
				{
					reduction = region->getActivityXCom().at(region->getActivityXCom().size() - (j + 1));
					y -= static_cast<Sint16>(Round(static_cast<float>(reduction) / pixelUnits));
					totals[j] += reduction;
				}
			}

			if (y > 175) y = 175;

			lineVector.push_back(y);

			if (lineVector.size() > 1)
			{
				x = 312 - static_cast<Sint16>(j) * 17;

				if (_alien == true)
				{
					_alienRegionLines.at(i)->drawLine(
							x,y,
							x + 17,
							lineVector.at(lineVector.size() - 2),
							_regionToggles.at(i)->_colorPushed + 4);

					switch (recallFactor)
					{
						case GUF_HALF:
						case GUF_QUARTER:
							boxLines(_alienRegionLines.at(i));
					}
				}
				else
				{
					_xcomRegionLines.at(i)->drawLine(
							x,y,
							x + 17,
							lineVector.at(lineVector.size() - 2),
							_regionToggles.at(i)->_colorPushed + 4);

					switch (recallFactor)
					{
						case GUF_HALF:
						case GUF_QUARTER:
							boxLines(_xcomRegionLines.at(i));
					}
				}
			}
		}

		if (_alien == true)
			_alienRegionLines.at(i)->setVisible(_regionToggles.at(i)->_pushed);
		else
			_xcomRegionLines.at(i)->setVisible(_regionToggles.at(i)->_pushed);
	}


	if (_alien == true) // set up the TOTAL line
		_alienRegionLines.back()->clear();
	else
		_xcomRegionLines.back()->clear();

	Uint8 color = static_cast<Uint8>(
				_game->getRuleset()->getInterface("graphs")->getElement("regionTotal")->color2);
	lineVector.clear();

	for (size_t
			i = 0;
			i != MONTHS_u;
			++i)
	{
		y = 175 + static_cast<Sint16>(Round(static_cast<float>(scaleLow) / pixelUnits));

		if (totals[i] > 0)
			y -= static_cast<Sint16>(Round(static_cast<float>(totals[i]) / pixelUnits));

		lineVector.push_back(y);

		if (lineVector.size() > 1)
		{
			x = 312 - static_cast<Sint16>(i) * 17;

			if (_alien == true)
			{
				_alienRegionLines.back()->drawLine(
						x,y,
						x + 17,
						lineVector.at(lineVector.size() - 2),
						color);

				switch (recallFactor)
				{
					case GUF_HALF:
					case GUF_QUARTER:
						boxLines(_alienRegionLines.back());
				}
			}
			else
			{
				_xcomRegionLines.back()->drawLine(
						x,y,
						x + 17,
						lineVector.at(lineVector.size() - 2),
						color);

				switch (recallFactor)
				{
					case GUF_HALF:
					case GUF_QUARTER:
						boxLines(_xcomRegionLines.back());
				}
			}
		}
	}

	if (_alien == true)
		_alienRegionLines.back()->setVisible(_regionToggles.back()->_pushed);
	else
		_xcomRegionLines.back()->setVisible(_regionToggles.back()->_pushed);

	updateScale(scaleLow, scaleHigh);
	_txtFactor->setVisible(false);
}

/**
 * Sets up the screens and draws the lines for country buttons to toggle on and off.
 */
void GraphsState::drawCountryLines() // private.
{
	// calculate the totals, and set up the upward maximum
	int
		scaleHigh = 0,
		scaleLow = 0,
		total,
		act,

		totals[MONTHS_u] = {0,0,0,0,0,0,0,0,0,0,0,0};

	for (size_t
			i = 0;
			i != _game->getSavedGame()->getFundsList().size();
			++i)
	{
		total = 0;

		for (size_t
				j = 0;
				j != _game->getSavedGame()->getCountries()->size();
				++j)
		{
			if (_alien == true)
				act = _game->getSavedGame()->getCountries()->at(j)->getActivityAlien().at(i);
			else if (_income == true)
				act = _game->getSavedGame()->getCountries()->at(j)->getFunding().at(i) / 1000;
			else
				act = _game->getSavedGame()->getCountries()->at(j)->getActivityXCom().at(i);

			total += act;

			if (act > scaleHigh
				&& _countryToggles.at(j)->_pushed == true)
			{
				scaleHigh = act;
			}

			if (_alien == false && _income == false // aLien & Income never go into negative values.
				&& act < scaleLow
				&& _countryToggles.at(j)->_pushed == true)
			{
				scaleLow = act;
			}
		}

		if (_countryToggles.back()->_pushed == true
			&& total > scaleHigh)
		{
			scaleHigh = total;
		}
	}


	const int low = scaleLow; // adjust the scale to fit the upward maximum
	int delta = scaleHigh - scaleLow;

	switch (recallFactor)
	{
		case GUF_QUARTER:
			delta >>= 1; // no break.
		case GUF_HALF:
			delta >>= 1;
	}

	int test = 10;
	while (delta > GRIDCELLS_y * test)
		test += 10;

	scaleLow = 0;
	scaleHigh = GRIDCELLS_y * test;

	while (low < scaleLow)
	{
		scaleLow -= test;
		scaleHigh -= test;
	}

	// Figure out how many units to the pixel then plot the points for the graph
	// and connect the dots.
	const float pixelUnits = static_cast<float>(scaleHigh - scaleLow) / PIXELS_y;

	int reduction;
	Sint16
		x,y;
	std::vector<Sint16> lineVector;
	Country* country;

	for (size_t // draw country lines
			i = 0;
			i != _game->getSavedGame()->getCountries()->size();
			++i)
	{
		country = _game->getSavedGame()->getCountries()->at(i);

		_alienCountryLines.at(i)->clear();
		_xcomCountryLines.at(i)->clear();
		_incomeLines.at(i)->clear();

		lineVector.clear();

		for (size_t
				j = 0;
				j != MONTHS_u;
				++j)
		{
			y = 175 + static_cast<Sint16>(Round(static_cast<float>(scaleLow) / pixelUnits));

			if (_alien == true)
			{
				if (j < country->getActivityAlien().size())
				{
					reduction = country->getActivityAlien().at(country->getActivityAlien().size() - (j + 1));
					y -= static_cast<Sint16>(Round(static_cast<float>(reduction) / pixelUnits));
					totals[j] += reduction;
				}
			}
			else if (_income == true)
			{
				if (j < country->getFunding().size())
				{
					reduction = country->getFunding().at(country->getFunding().size() - (j + 1));
					y -= static_cast<Sint16>(Round(static_cast<float>(reduction) / 1000.f / pixelUnits));
					totals[j] += static_cast<int>(Round(static_cast<float>(reduction) / 1000.f));
				}
			}
			else
			{
				if (j < country->getActivityXCom().size())
				{
					reduction = country->getActivityXCom().at(country->getActivityXCom().size() - (j + 1));
					y -= static_cast<Sint16>(Round(static_cast<float>(reduction) / pixelUnits));
					totals[j] += reduction;
				}
			}

			if (y > 175) y = 175;

			lineVector.push_back(y);

			if (lineVector.size() > 1)
			{
				x = 312 - static_cast<Sint16>(j) * 17;

				if (_alien == true)
				{
					_alienCountryLines.at(i)->drawLine(
							x,y,
							x + 17,
							lineVector.at(lineVector.size() - 2),
							_countryToggles.at(i)->_colorPushed + 4);

					switch (recallFactor)
					{
						case GUF_HALF:
						case GUF_QUARTER:
							boxLines(_alienCountryLines.at(i));
					}
				}
				else if (_income == true)
				{
					_incomeLines.at(i)->drawLine(
							x,y,
							x + 17,
							lineVector.at(lineVector.size() - 2),
							_countryToggles.at(i)->_colorPushed + 4);

					switch (recallFactor)
					{
						case GUF_HALF:
						case GUF_QUARTER:
							boxLines(_incomeLines.at(i));
					}
				}
				else
				{
					_xcomCountryLines.at(i)->drawLine(
							x,y,
							x + 17,
							lineVector.at(lineVector.size() - 2),
							_countryToggles.at(i)->_colorPushed + 4);

					switch (recallFactor)
					{
						case GUF_HALF:
						case GUF_QUARTER:
							boxLines(_xcomCountryLines.at(i));
					}
				}
			}
		}

		if (_alien == true)
			_alienCountryLines.at(i)->setVisible(_countryToggles.at(i)->_pushed);
		else if (_income == true)
			_incomeLines.at(i)->setVisible(_countryToggles.at(i)->_pushed);
		else
			_xcomCountryLines.at(i)->setVisible(_countryToggles.at(i)->_pushed);
	}


	if (_alien == true) // set up the TOTAL line
		_alienCountryLines.back()->clear();
	else if (_income == true)
		_incomeLines.back()->clear();
	else
		_xcomCountryLines.back()->clear();

	Uint8 color = static_cast<Uint8>(
				_game->getRuleset()->getInterface("graphs")->getElement("countryTotal")->color2);
	lineVector.clear();

	for (size_t
			i = 0;
			i != MONTHS_u;
			++i)
	{
		y = 175 + static_cast<Sint16>(Round(static_cast<float>(scaleLow) / pixelUnits));

		if (totals[i] > 0)
			y -= static_cast<Sint16>(Round(static_cast<float>(totals[i]) / pixelUnits));

		lineVector.push_back(y);

		if (lineVector.size() > 1)
		{
			x = 312 - static_cast<Sint16>(i) * 17;

			if (_alien == true)
			{
				_alienCountryLines.back()->drawLine(
						x,y,
						x + 17,
						lineVector.at(lineVector.size() - 2),
						color);

				switch (recallFactor)
				{
					case GUF_HALF:
					case GUF_QUARTER:
						boxLines(_alienCountryLines.back());
				}
			}
			else if (_income == true)
			{
				_incomeLines.back()->drawLine(
						x,y,
						x + 17,
						lineVector.at(lineVector.size() - 2),
						color);

				switch (recallFactor)
				{
					case GUF_HALF:
					case GUF_QUARTER:
						boxLines(_incomeLines.back());
				}
			}
			else
			{
				_xcomCountryLines.back()->drawLine(
						x,y,
						x + 17,
						lineVector.at(lineVector.size() - 2),
						color);

				switch (recallFactor)
				{
					case GUF_HALF:
					case GUF_QUARTER:
						boxLines(_xcomCountryLines.back());
				}
			}
		}
	}

	if (_alien == true)
		_alienCountryLines.back()->setVisible(_countryToggles.back()->_pushed);
	else if (_income == true)
		_incomeLines.back()->setVisible(_countryToggles.back()->_pushed);
	else
		_xcomCountryLines.back()->setVisible(_countryToggles.back()->_pushed);

	updateScale(scaleLow, scaleHigh);
	_txtFactor->setVisible(_income);
}

/**
 * Sets up the screens and draws the lines for the finance buttons to toggle on and off.
 */
void GraphsState::drawFinanceLines() // private. // Council Analytics
{
	int
		scaleHigh = 0,
		scaleLow = 0,

		income[MONTHS_u]		= {0,0,0,0,0,0,0,0,0,0,0,0},
		expenditure[MONTHS_u]	= {0,0,0,0,0,0,0,0,0,0,0,0},
		maintenance[MONTHS_u]	= {0,0,0,0,0,0,0,0,0,0,0,0},
		balance[MONTHS_u]		= {0,0,0,0,0,0,0,0,0,0,0,0},
		score[MONTHS_u]			= {0,0,0,0,0,0,0,0,0,0,0,0},

		baseIncomes = 0,
		baseExpenses = 0;

	// start filling those arrays with score values;
	// determine which is the highest one being displayed, so we can adjust the scale
	size_t rit;
	for (size_t
			i = 0;
			i != _game->getSavedGame()->getFundsList().size(); // use Balance as template.
			++i)
	{
		rit = _game->getSavedGame()->getFundsList().size() - (i + 1);

		if (i == 0)
		{
			for (std::vector<Base*>::const_iterator
					j = _game->getSavedGame()->getBases()->begin();
					j != _game->getSavedGame()->getBases()->end();
					++j)
			{
				baseIncomes += (*j)->getCashIncome();
				baseExpenses += (*j)->getCashSpent();
			}

			income[i] = baseIncomes / 1000; // perhaps add Country funding
			expenditure[i] = baseExpenses / 1000;
			maintenance[i] = _game->getSavedGame()->getBaseMaintenances() / 1000; // use current
		}
		else
		{
			income[i] = static_cast<int>(_game->getSavedGame()->getIncomeList().at(rit)) / 1000; // perhaps add Country funding
			expenditure[i] = static_cast<int>(_game->getSavedGame()->getExpenditureList().at(rit)) / 1000;
			maintenance[i] = static_cast<int>(_game->getSavedGame()->getMaintenanceList().at(rit)) / 1000;
		}

		balance[i] = static_cast<int>(_game->getSavedGame()->getFundsList().at(rit)) / 1000; // note: these (int)casts render int64_t useless.
		score[i] = _game->getSavedGame()->getResearchScores().at(rit);


		for (std::vector<Region*>::const_iterator
				j = _game->getSavedGame()->getRegions()->begin();
				j != _game->getSavedGame()->getRegions()->end();
				++j)
		{
			score[i] += (*j)->getActivityXCom().at(rit) - (*j)->getActivityAlien().at(rit);
		}

		if (i == 0) // values are stored backwards. So take 1st value for last.
			_txtScore->setText(Text::intWide(score[i]));


		if (_financeToggles.at(0) == true) // INCOME
		{
			if (income[i] > scaleHigh)
				scaleHigh = income[i];

			if (income[i] < scaleLow)
				scaleLow = income[i];
		}

		if (_financeToggles.at(1) == true) // EXPENDITURE
		{
			if (expenditure[i] > scaleHigh)
				scaleHigh = expenditure[i];

			if (expenditure[i] < scaleLow)
				scaleLow = expenditure[i];
		}

		if (_financeToggles.at(2) == true) // MAINTENANCE
		{
			if (maintenance[i] > scaleHigh)
				scaleHigh = maintenance[i];

			if (maintenance[i] < scaleLow)
				scaleLow = maintenance[i];
		}

		if (_financeToggles.at(3) == true) // BALANCE
		{
			if (balance[i] > scaleHigh)
				scaleHigh = balance[i];

			if (balance[i] < scaleLow)
				scaleLow = balance[i];
		}

		if (_financeToggles.at(4) == true) // SCORE
		{
			if (score[i] > scaleHigh)
				scaleHigh = score[i];

			if (score[i] < scaleLow)
				scaleLow = score[i];
		}
	}

	for (size_t // toggle screens
			i = 0;
			i != 5u;
			++i)
	{
		_financeLines.at(i)->setVisible(_financeToggles.at(i));
		_financeLines.at(i)->clear();
	}


	const int low = scaleLow; // adjust the scale to fit the upward maximum
	int delta = scaleHigh - scaleLow;

	switch (recallFactor)
	{
		case GUF_QUARTER:
			delta >>= 1; // no break.
		case GUF_HALF:
			delta >>= 1;
	}

	int test = 100;
	while (delta > GRIDCELLS_y * test)
		test += 100;

	scaleLow = 0;
	scaleHigh = GRIDCELLS_y * test;

	while (low < scaleLow)
	{
		scaleLow -= test;
		scaleHigh -= test;
	}

	// Figure out how many units to the pixel then plot the points for the graph
	// and connect the dots.
	const float pixelUnits = static_cast<float>(scaleHigh - scaleLow) / PIXELS_y;

	Uint8 color;
	Sint16
		reduction,
		x,y;

	std::vector<Sint16> lineVector;

	for (size_t
			i = 0;
			i != 5u;
			++i)
	{
		lineVector.clear();

		for (size_t
				j = 0;
				j != MONTHS_u;
				++j)
		{
			y = 175 + static_cast<Sint16>(Round(static_cast<float>(scaleLow) / pixelUnits));

			switch (i)
			{
				case 0:
					reduction = static_cast<Sint16>(Round(static_cast<float>(income[j]) / pixelUnits));
				break;
				case 1:
					reduction = static_cast<Sint16>(Round(static_cast<float>(expenditure[j]) / pixelUnits));
				break;
				case 2:
					reduction = static_cast<Sint16>(Round(static_cast<float>(maintenance[j]) / pixelUnits));
				break;
				case 3:
					reduction = static_cast<Sint16>(Round(static_cast<float>(balance[j]) / pixelUnits));
				break;
				case 4:
				default: // avoid vc++ linker warning.
					reduction = static_cast<Sint16>(Round(static_cast<float>(score[j]) / pixelUnits));
			}

			y -= reduction;

			lineVector.push_back(y);

			if (lineVector.size() > 1)
			{
				if (i % 2 != 0)
					color = 8;
				else
					color = 0;

				Uint8 multer; // switch colors for Income (was yellow) and Maintenance (was green)
				switch (i)
				{
					case 0: multer = 2; break;
					case 2: multer = 0; break;
					default:
						multer = static_cast<Uint8>(i);
				}

				color = Palette::blockOffset((multer / 2) + 1) + color;

				x = 312 - static_cast<Sint16>(j) * 17;
				_financeLines.at(i)->drawLine(
											x,y,
											x + 17,
											lineVector.at(lineVector.size() - 2),
											color);

				switch (recallFactor)
				{
					case GUF_HALF:
					case GUF_QUARTER:
						boxLines(_financeLines.at(i));
				}
			}
		}
	}

	updateScale(scaleLow, scaleHigh);
	_txtFactor->setVisible();
}

/**
 * Shift the buttons to display only GRAPH_BUTTONS - reset their state from toggles.
 * @param action - pointer to an Action
 */
void GraphsState::shiftButtons(Action* action) // private.
{
	if (_finance == false)
	{
		if (_country == true)
		{
			if (_countryToggles.size() > GRAPH_BUTTONS)
			{
				int dirVal;
				if (action->getDetails()->button.button == SDL_BUTTON_WHEELUP)
				{
					dirVal = -1;
					if (static_cast<int>(_btnCountryOffset) + dirVal < 0)
						return;
				}
				else if (action->getDetails()->button.button == SDL_BUTTON_WHEELDOWN)
				{
					dirVal = 1;
					if (static_cast<int>(_btnCountryOffset + GRAPH_BUTTONS)
							+ dirVal >= static_cast<int>(_countryToggles.size()))
					{
						return;
					}
				}
				else
				{
					switch (action->getDetails()->key.keysym.sym)
					{
						case SDLK_UP:
						case SDLK_KP8:
							dirVal = -1;
							if (static_cast<int>(_btnCountryOffset) + dirVal < 0)
								return;
							break;

						case SDLK_DOWN:
						case SDLK_KP2:
							dirVal = 1;
							if (static_cast<int>(_btnCountryOffset + GRAPH_BUTTONS)
									+ dirVal >= static_cast<int>(_countryToggles.size()))
							{
								return;
							}
							break;

						case SDLK_PAGEUP:
						case SDLK_KP9:
							dirVal = std::max(
										-(static_cast<int>(GRAPH_BUTTONS)),
										-(static_cast<int>(_btnCountryOffset)));
							break;

						case SDLK_PAGEDOWN:
						case SDLK_KP3:
							dirVal = std::min(
										static_cast<int>(GRAPH_BUTTONS),
										static_cast<int>(_countryToggles.size())
											- static_cast<int>(GRAPH_BUTTONS)
											- static_cast<int>(_btnCountryOffset) - 1);
							break;

						case SDLK_HOME:
						case SDLK_KP7:
							dirVal = -(static_cast<int>(_btnCountryOffset));
							break;

						case SDLK_END:
						case SDLK_KP1:
							dirVal = static_cast<int>(_countryToggles.size())
										- static_cast<int>(GRAPH_BUTTONS)
										- static_cast<int>(_btnCountryOffset) - 1;
							break;

						default:
							dirVal = 0;
					}
				}

				if (dirVal != 0)
					scrollButtons(dirVal);
			}
		}
/*		else // _region -> not needed unless quantity of Regions increases over GRAPH_BUTTONS. Ain't likely to happen.
		{
			if (_regionToggles.size() > GRAPH_BUTTONS)
			{
				int dirVal = 0;
				if (action->getDetails()->button.button == SDL_BUTTON_WHEELUP)
					dirVal = -1;
				else if (action->getDetails()->button.button == SDL_BUTTON_WHEELDOWN)
					dirVal = 1;

				if (dirVal != 0)
					scrollButtons(dirVal);
			}
		} */
	}
}

/**
 * Helper for shiftButtons().
 * @param dirVal	- quantity of rows and direction to shift by (neg. up/ pos. down)
 * @param init		- true to shift buttons to the recall-row (default false)
 */
void GraphsState::scrollButtons( // private.
		int dirVal,
		bool init)
{
	if (dirVal + static_cast<int>(_btnCountryOffset) > -1 // -> final safety.
		&& dirVal
			+ static_cast<int>(_btnCountryOffset)
			+ static_cast<int>(GRAPH_BUTTONS) < static_cast<int>(_countryToggles.size()))
	{
		_forceVis = true; // do not blink while scrolling.
		blink();

		if (init == true)
			_btnCountryOffset = recallCountry;
		else
			recallCountry =
			_btnCountryOffset = static_cast<size_t>(static_cast<int>(_btnCountryOffset) + dirVal);

		std::vector<ToggleTextButton*>::const_iterator pBtn = _btnCountries.begin();
		std::vector<Text*>::const_iterator pActA = _txtCountryActA.begin();
		std::vector<Text*>::const_iterator pActX = _txtCountryActX.begin();
		std::vector<bool>::iterator pBlinkA = _blinkCountryAlien.begin();
		std::vector<bool>::iterator pBlinkX = _blinkCountryXCom.begin();

		size_t row = 0;
		for (std::vector<GraphBtnInfo*>::const_iterator
				i = _countryToggles.begin();
				i != _countryToggles.end();
				++i, ++row)
		{
			if (row >= _btnCountryOffset)
			{
				if (row < _btnCountryOffset + GRAPH_BUTTONS)
				{
					*pBlinkA = (*i)->_blinkA;
					*pBlinkX = (*i)->_blinkX;
					++pBlinkA; // note that all these incrementors are for the iterators - not their values.
					++pBlinkX;

					updateButton(
							*i,
							*pBtn++,
							*pActA++,
							*pActX++);
				}
				else
					return;
			}
		}
	}
}

/**
 * Updates button appearances when scrolling the lists.
 * @note Helper for scrollButtons().
 */
void GraphsState::updateButton( // private.
		GraphBtnInfo* info,
		ToggleTextButton* btn,
		Text* aLiens,
		Text* xCom)
{
	btn->setText(info->_label);
	btn->setInvertColor(info->_colorPushed);
	btn->setPressed(info->_pushed);

	aLiens->setText(Text::intWide(info->_actA));
	aLiens->setColor(info->_colorText);

	xCom->setText(Text::intWide(info->_actX));
	xCom->setColor(info->_colorText);
}

}
