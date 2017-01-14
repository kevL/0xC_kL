/*
 * Copyright 2010-2017 OpenXcom Developers.
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

#include "GraphsState.h"

//#include <sstream>

#include "../Engine/Action.h"
#include "../Engine/Game.h"
#include "../Engine/InteractiveSurface.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Options.h"
#include "../Engine/Palette.h"
#include "../Engine/Screen.h"
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

static int recallPage (-1);
static size_t recallCountry (0u);
static GraphsExpansionFactor recallExpansion (GF_DEFAULT);

static int
	SCREEN_OFFSET_y,
	TOGGLEALL_yCou,
	TOGGLEALL_yReg;

const float GraphsState::PIXELS_y (126.f);


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
//		_btnRegionOffset(0u),
		_btnCountryOffset(0u),
		_init(true),
		_reset(false),
		_forceVis(true),
		_lock(false),
		_lockRegionsHigh(-1),
		_lockRegionsLow(-1),
		_lockCountriesHigh(-1),
		_lockCountriesLow(-1),
		_lockFinanceHigh(-1),
		_lockFinanceLow(-1),
		_uiGraphs(_game->getRuleset()->getInterface("graphs")),
		_playSave(_game->getSavedGame()),
		_regions(_game->getSavedGame()->getRegions()),
		_countries(_game->getSavedGame()->getCountries())
{
	const int offsetX ((Options::baseXResolution - 320) >> 1u);
	SCREEN_OFFSET_y  = (Options::baseYResolution - 200) >> 1u;

	_bg = new InteractiveSurface(
							Options::baseXResolution,
							Options::baseYResolution,
							-offsetX,
							-SCREEN_OFFSET_y);
	_bg->onMousePress(		static_cast<ActionHandler>(&GraphsState::shiftButtons),
							SDL_BUTTON_WHEELUP);
	_bg->onMousePress(		static_cast<ActionHandler>(&GraphsState::shiftButtons),
							SDL_BUTTON_WHEELDOWN);
	_bg->onKeyboardPress(	static_cast<ActionHandler>(&GraphsState::shiftButtons),
							SDLK_UP);
	_bg->onKeyboardPress(	static_cast<ActionHandler>(&GraphsState::shiftButtons),
							SDLK_KP8);
	_bg->onKeyboardPress(	static_cast<ActionHandler>(&GraphsState::shiftButtons),
							SDLK_DOWN);
	_bg->onKeyboardPress(	static_cast<ActionHandler>(&GraphsState::shiftButtons),
							SDLK_KP2);
	_bg->onKeyboardPress(	static_cast<ActionHandler>(&GraphsState::shiftButtons),
							SDLK_PAGEUP);
	_bg->onKeyboardPress(	static_cast<ActionHandler>(&GraphsState::shiftButtons),
							SDLK_KP9);
	_bg->onKeyboardPress(	static_cast<ActionHandler>(&GraphsState::shiftButtons),
							SDLK_PAGEDOWN);
	_bg->onKeyboardPress(	static_cast<ActionHandler>(&GraphsState::shiftButtons),
							SDLK_KP3);
	_bg->onKeyboardPress(	static_cast<ActionHandler>(&GraphsState::shiftButtons),
							SDLK_HOME);
	_bg->onKeyboardPress(	static_cast<ActionHandler>(&GraphsState::shiftButtons),
							SDLK_KP7);
	_bg->onKeyboardPress(	static_cast<ActionHandler>(&GraphsState::shiftButtons),
							SDLK_END);
	_bg->onKeyboardPress(	static_cast<ActionHandler>(&GraphsState::shiftButtons),
							SDLK_KP1);
	_bg->onMousePress(		static_cast<ActionHandler>(&GraphsState::btnGeoscapeClick),
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

	_btnLockScale	= new ToggleTextButton(24, 14, 136, 28);
	_btnReset		= new TextButton(40, 16, 96, 26);
	_btnToggleAll	= new ToggleTextButton(24, HEIGHT_btn, 66, 0); // y is set according to page.

	_btnFactor1		= new TextButton(16, 16, 272, 26);
	_btnFactor2		= new TextButton(16, 16, 288, 26);
	_btnFactor4		= new TextButton(16, 16, 304, 26);

	switch (recallExpansion)
	{
		default:
		case GF_DEFAULT: _userFactor = _btnFactor1; break;
		case GF_HALF:    _userFactor = _btnFactor2; break;
		case GF_QUARTER: _userFactor = _btnFactor4;
	}

	_txtTitle	= new Text(220, 16, 100, 28);
	_txtFactor	= new Text( 35,  9,  96, 28);

	_lstMonths	= new TextList(215, 9, 117, 182); // NOTE: These go beyond 320px.
	_lstYears	= new TextList(215, 9, 117, 191);

	_txtScore	= new Text(36, 9, 46, 82);


	setInterface("graphs");

	add(_bg);
	add(_isfUfoRegion);
	add(_isfUfoCountry);
	add(_isfXcomRegion);
	add(_isfXcomCountry);
	add(_isfIncome);
	add(_isfFinance);
	add(_isfGeoscape);
	add(_btnLockScale,	"button",	"graphs");
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
			i = 0u;
			i != TEXTS_y;
			++i)
	{
		_txtScale.push_back(new Text(
									32,9,
									91,
									171 - (static_cast<int>(i) * 14)));
		add(_txtScale.at(i), "scale", "graphs");
	}

	static const Uint8
		C_OFFSET_btn (13u),
		C_OFFSET_txt (16u);

	size_t btnOffset (0u);
	unsigned colorOffset (0u);
	Uint8 color;
	int
		actA,
		actX;
	bool
		blinkA,
		blinkX;

	/* REGIONS */
	for (std::vector<Region*>::const_iterator
			i = _regions->begin();
			i != _regions->end();
			++i, ++colorOffset, ++btnOffset)
	{
//		if (colorOffset == 17) colorOffset = 0; // <- only 15 regions IG.
		color = static_cast<Uint8>(colorOffset * 8u);

		actA = (*i)->getActivityAlien().back();
		actX = (*i)->getActivityXCom().back();
		blinkA = (*i)->recentActivityAlien(false, true);
		blinkX = (*i)->recentActivityXCom(false, true);

		// put all the regions into toggles
		_regionToggles.push_back(new GraphBtnInfo(
												tr((*i)->getRules()->getType()), // name of Region
												static_cast<Uint8>(color + C_OFFSET_btn),
												actA,
												actX,
												static_cast<Uint8>(color + C_OFFSET_txt),
												blinkA,
												blinkX));

		// first add the GRAPH_BUTTONS (having the respective region's information)
		if (btnOffset < GRAPH_BUTTONS) // leave a slot for the TOTAL btn.
		{
			_btnRegions.push_back(new ToggleTextButton(
													65, HEIGHT_btn,
													0,
													static_cast<int>(btnOffset) * HEIGHT_btn));
			_btnRegions.at(btnOffset)->setText(tr((*i)->getRules()->getType())); // name of Region
			_btnRegions.at(btnOffset)->setColorInvert(static_cast<Uint8>(color + C_OFFSET_btn));
			_btnRegions.at(btnOffset)->onMousePress(static_cast<ActionHandler>(&GraphsState::btnRegionListPress),
													SDL_BUTTON_LEFT);
			add(_btnRegions.at(btnOffset), "button", "graphs");

			_txtRegionActA.push_back(new Text(
											27,9,
											66,
											(static_cast<int>(btnOffset) * HEIGHT_btn) + 1));
			_txtRegionActA.at(btnOffset)->setColor(static_cast<Uint8>(color + C_OFFSET_txt));
			_txtRegionActA.at(btnOffset)->setText(Text::intWide(actA));
			add(_txtRegionActA.at(btnOffset));

			_txtRegionActX.push_back(new Text(
											27,9,
											66,
											(static_cast<int>(btnOffset) * HEIGHT_btn) + 1));
			_txtRegionActX.at(btnOffset)->setColor(static_cast<Uint8>(color + C_OFFSET_txt));
			_txtRegionActX.at(btnOffset)->setText(Text::intWide(actX));
			add(_txtRegionActX.at(btnOffset));

			_blinkRegionAlien.push_back(blinkA);
			_blinkRegionXCom.push_back(blinkX);
		}


		_alienRegionLines.push_back(new Surface());
		add(_alienRegionLines.at(btnOffset));

		_xcomRegionLines.push_back(new Surface());
		add(_xcomRegionLines.at(btnOffset));
	}


	int btnTotal_y;
	if (_regionToggles.size() < GRAPH_BUTTONS)
		btnTotal_y = static_cast<int>(_regionToggles.size());
	else
		btnTotal_y = static_cast<int>(GRAPH_BUTTONS);

	_btnRegionTotal = new ToggleTextButton( // TOTAL btn.
										65, HEIGHT_btn,
										0,
										btnTotal_y * HEIGHT_btn);

	color = static_cast<Uint8>(_uiGraphs->getElement("regionTotal")->color);

	_regionToggles.push_back(new GraphBtnInfo( // TOTAL btn is the last button in the vector.
											tr("STR_TOTAL_UC"),
											color,
											0,0,0,
											false,false));
	_btnRegionTotal->setColorInvert(color);
	_btnRegionTotal->setText(tr("STR_TOTAL_UC"));
	_btnRegionTotal->onMousePress(	static_cast<ActionHandler>(&GraphsState::btnRegionListPress),
									SDL_BUTTON_LEFT);

	add(_btnRegionTotal, "button", "graphs");

	_alienRegionLines.push_back(new Surface());
	add(_alienRegionLines.at(btnOffset));

	_xcomRegionLines.push_back(new Surface());
	add(_xcomRegionLines.at(btnOffset));


	btnOffset =
	colorOffset = 0u;

	/* COUNTRIES */
	for (std::vector<Country*>::const_iterator
			i = _countries->begin();
			i != _countries->end();
			++i, ++colorOffset, ++btnOffset)
	{
		if (colorOffset == 17u) colorOffset = 0u;
		color = static_cast<Uint8>(colorOffset * 8u);

		actA = (*i)->getActivityAlien().back();
		actX = (*i)->getActivityXCom().back();
		blinkA = (*i)->recentActivityAlien(false, true);
		blinkX = (*i)->recentActivityXCom(false, true);

		// put all the countries into toggles
		_countryToggles.push_back(new GraphBtnInfo(
												tr((*i)->getRules()->getType()), // name of Country
												static_cast<Uint8>(color + C_OFFSET_btn),
												actA,
												actX,
												static_cast<Uint8>(color + C_OFFSET_txt),
												blinkA,
												blinkX));

		// first add the GRAPH_BUTTONS (having the respective country's information)
		if (btnOffset < GRAPH_BUTTONS) // leave a slot for the TOTAL btn.
		{
			_btnCountries.push_back(new ToggleTextButton(
													65, HEIGHT_btn,
													0,
													static_cast<int>(btnOffset) * HEIGHT_btn));
			_btnCountries.at(btnOffset)->setText(tr((*i)->getRules()->getType())); // name of Country
			_btnCountries.at(btnOffset)->setColorInvert(static_cast<Uint8>(color + C_OFFSET_btn));
			_btnCountries.at(btnOffset)->onMousePress(	static_cast<ActionHandler>(&GraphsState::btnCountryListPress),
														SDL_BUTTON_LEFT);
			add(_btnCountries.at(btnOffset), "button", "graphs");

			_txtCountryActA.push_back(new Text(
											27,9,
											66,
											(static_cast<int>(btnOffset) * HEIGHT_btn) + 1));
			_txtCountryActA.at(btnOffset)->setColor(static_cast<Uint8>(color + C_OFFSET_txt));
			_txtCountryActA.at(btnOffset)->setText(Text::intWide(actA));
			add(_txtCountryActA.at(btnOffset));

			_txtCountryActX.push_back(new Text(
											27,9,
											66,
											(static_cast<int>(btnOffset) * HEIGHT_btn) + 1));
			_txtCountryActX.at(btnOffset)->setColor(static_cast<Uint8>(color + C_OFFSET_txt));
			_txtCountryActX.at(btnOffset)->setText(Text::intWide(actX));
			add(_txtCountryActX.at(btnOffset));

			_blinkCountryAlien.push_back(blinkA);
			_blinkCountryXCom.push_back(blinkX);
		}


		_alienCountryLines.push_back(new Surface());
		add(_alienCountryLines.at(btnOffset));

		_xcomCountryLines.push_back(new Surface());
		add(_xcomCountryLines.at(btnOffset));

		_incomeLines.push_back(new Surface());
		add(_incomeLines.at(btnOffset));
	}


	if (_countryToggles.size() < GRAPH_BUTTONS)
		btnTotal_y = static_cast<int>(_countryToggles.size());
	else
		btnTotal_y = static_cast<int>(GRAPH_BUTTONS);

	_btnCountryTotal = new ToggleTextButton( // TOTAL btn.
										65, HEIGHT_btn,
										0,
										btnTotal_y * HEIGHT_btn);

	color = static_cast<Uint8>(_uiGraphs->getElement("countryTotal")->color);

	_countryToggles.push_back(new GraphBtnInfo( // TOTAL btn is the last button in the vector.
											tr("STR_TOTAL_UC"),
											color,
											0,0,0,
											false,false));

	_btnCountryTotal->setColorInvert(color);
	_btnCountryTotal->setText(tr("STR_TOTAL_UC"));
	_btnCountryTotal->onMousePress(	static_cast<ActionHandler>(&GraphsState::btnCountryListPress),
									SDL_BUTTON_LEFT);

	add(_btnCountryTotal, "button", "graphs");

	_alienCountryLines.push_back(new Surface());
	add(_alienCountryLines.at(btnOffset));

	_xcomCountryLines.push_back(new Surface());
	add(_xcomCountryLines.at(btnOffset));

	_incomeLines.push_back(new Surface());
	add(_incomeLines.at(btnOffset));


	/* FINANCE */
	unsigned hueFactor;
	for (size_t
			i = 0u;
			i != 5u;
			++i)
	{
		_btnFinances.push_back(new ToggleTextButton(
												82,16,
												0,
												static_cast<int>(i) * 16));
		_financeToggles.push_back(false);

		switch (i) // switch colors for Income (was yellow) and Maintenance (was green)
		{
			case 0: hueFactor = 2u; break;
			case 2: hueFactor = 0u; break;
			default:
				hueFactor = i;
		}

		_btnFinances.at(i)->setColorInvert(static_cast<Uint8>((hueFactor << 3u) + C_OFFSET_btn));
		_btnFinances.at(i)->onMousePress(static_cast<ActionHandler>(&GraphsState::btnFinanceListPress));

		add(_btnFinances.at(i), "button", "graphs");

		_financeLines.push_back(new Surface());
		add(_financeLines.at(i));
	}

	_btnFinances.at(0u)->setText(tr("STR_INCOME"));
	_btnFinances.at(1u)->setText(tr("STR_EXPENDITURE"));
	_btnFinances.at(2u)->setText(tr("STR_MAINTENANCE"));
	_btnFinances.at(3u)->setText(tr("STR_BALANCE"));
	_btnFinances.at(4u)->setText(tr("STR_SCORE"));

	_txtScore->setColor(49u);
	_txtScore->setAlign(ALIGN_RIGHT);


	// Load back all the buttons' toggled states from SavedGame!

	/* REGION TOGGLES */
	std::string graphRegionToggles (_playSave->getGraphRegionToggles());
	while (graphRegionToggles.size() < _regionToggles.size())
		graphRegionToggles.push_back('0');

	for (size_t
			i = 0u;
			i != _regionToggles.size();
			++i)
	{
		_regionToggles[i]->_pushed = (graphRegionToggles[i] == '0') ? false : true;

		if (_regionToggles.size() - 1u == i)
			_btnRegionTotal->setPressed(_regionToggles[i]->_pushed);
		else if (i < GRAPH_BUTTONS)
			_btnRegions.at(i)->setPressed(_regionToggles[i]->_pushed);
	}

	/* COUNTRY TOGGLES */
	std::string graphCountryToggles (_playSave->getGraphCountryToggles());
	while (graphCountryToggles.size() < _countryToggles.size())
		graphCountryToggles.push_back('0');

	for (size_t
			i = 0u;
			i != _countryToggles.size();
			++i)
	{
		_countryToggles[i]->_pushed = (graphCountryToggles[i] == '0') ? false : true;

		if (_countryToggles.size() - 1u == i)
			_btnCountryTotal->setPressed(_countryToggles[i]->_pushed);
		else if (i < GRAPH_BUTTONS)
			_btnCountries.at(i)->setPressed(_countryToggles[i]->_pushed);
	}

	/* FINANCE TOGGLES */
	std::string graphFinanceToggles (_playSave->getGraphFinanceToggles());
	while (graphFinanceToggles.size() < _financeToggles.size())
		graphFinanceToggles.push_back('0');

	for (size_t
			i = 0u;
			i != _financeToggles.size();
			++i)
	{
		_financeToggles[i] = (graphFinanceToggles[i] == '0') ? false : true;
		_btnFinances.at(i)->setPressed(_financeToggles[i]);
	}


//	_btnLockScale->setText(L"lock"); // TODO: use tr("STR_GRAPHS_LOCK_SCALE")
	_btnLockScale->onMousePress(static_cast<ActionHandler>(&GraphsState::btnLockPress),
								SDL_BUTTON_LEFT);

	_btnReset->setText(tr("STR_RESET_UC"));
	_btnReset->onMousePress(static_cast<ActionHandler>(&GraphsState::btnResetPress),
							SDL_BUTTON_LEFT);

//	_btnToggleAll->setText(L"all"); // TODO: use tr("STR_GRAPHS_TOGGLE_ALL")
	_btnToggleAll->onMousePress(static_cast<ActionHandler>(&GraphsState::btnTogglePress),
								SDL_BUTTON_LEFT);

	_btnFactor1->setText(L"1");
	_btnFactor1->setGroup(&_userFactor);
	_btnFactor1->setSilent();
	_btnFactor1->onMousePress(		static_cast<ActionHandler>(&GraphsState::btnFactorPress),
									SDL_BUTTON_LEFT);
	_btnFactor1->onKeyboardPress(	static_cast<ActionHandler>(&GraphsState::keyFactorPress),
									SDLK_1);

	_btnFactor2->setText(L"2");
	_btnFactor2->setGroup(&_userFactor);
	_btnFactor2->setSilent();
	_btnFactor2->onMousePress(		static_cast<ActionHandler>(&GraphsState::btnFactorPress),
									SDL_BUTTON_LEFT);
	_btnFactor2->onKeyboardPress(	static_cast<ActionHandler>(&GraphsState::keyFactorPress),
									SDLK_2);

	_btnFactor4->setText(L"3");
	_btnFactor4->setGroup(&_userFactor);
	_btnFactor4->setSilent();
	_btnFactor4->onMousePress(		static_cast<ActionHandler>(&GraphsState::btnFactorPress),
									SDL_BUTTON_LEFT);
	_btnFactor4->onKeyboardPress(	static_cast<ActionHandler>(&GraphsState::keyFactorPress),
									SDLK_3);

	Surface* const icons (_game->getResourcePack()->getSurface("GRAPHS.SPK"));
	icons->setX(offsetX);
	icons->setY(SCREEN_OFFSET_y);
	icons->blit(_bg);

	const Uint8 colorGrid (static_cast<Uint8>(_uiGraphs->getElement("graph")->color));

	_bg->drawRect( // set up the grid
				static_cast<Sint16>(offsetX + 125),
				static_cast<Sint16>(SCREEN_OFFSET_y + 49),
				188,127,
				colorGrid);

	for (int
			i = 0;
			i != 5;
			++i)
	{
		for (Sint16
				y = static_cast<Sint16>(SCREEN_OFFSET_y +  50 + i);
				y < static_cast<Sint16>(SCREEN_OFFSET_y + 164 + i);
				y = static_cast<Sint16>(y + 14))
		{
			for (Sint16
					x = static_cast<Sint16>(offsetX + 126 + i);
					x < static_cast<Sint16>(offsetX + 298 + i);
					x = static_cast<Sint16>(x + 17))
			{
				switch (i)
				{
					case 4:
						color = 0u;
						break;
					default:
						color = static_cast<Uint8>(colorGrid + 1 + i);
				}

				_bg->drawRect(
							x,y,
							static_cast<Sint16>(16 - (i * 2)),
							static_cast<Sint16>(13 - (i * 2)),
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


	const GameTime* const gt (_playSave->getTime());
	const int yr (gt->getYear());
	size_t th (static_cast<size_t>(gt->getMonth()) - 1u);

	for (size_t
			i = 0u;
			i != MONTHS_u;
			++i)
	{
		if (i == 1u)
		{
			if (th == 11u)
				_lstYears->setCellText(0u,0u, Text::intWide(yr));
			else
				_lstYears->setCellText(0u,0u, Text::intWide(yr - 1));
		}
		else if (th == 0u)
			_lstYears->setCellText(0u, i / 2u, Text::intWide(yr));

		if (++th == 12u) th = 0u;

		_lstMonths->setCellText(0u, i, tr(GameTime::GAME_MONTHS[th]));
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

	_txtFactor->setText(tr("STR_FINANCE_THOUSANDS"));


	_isfUfoRegion->onMousePress(	static_cast<ActionHandler>(&GraphsState::btnUfoRegionClick),
									SDL_BUTTON_LEFT);
	_isfXcomRegion->onMousePress(	static_cast<ActionHandler>(&GraphsState::btnXcomRegionClick),
									SDL_BUTTON_LEFT);
	_isfUfoCountry->onMousePress(	static_cast<ActionHandler>(&GraphsState::btnUfoCountryClick),
									SDL_BUTTON_LEFT);
	_isfXcomCountry->onMousePress(	static_cast<ActionHandler>(&GraphsState::btnXcomCountryClick),
									SDL_BUTTON_LEFT);

	_isfIncome->onMousePress(	static_cast<ActionHandler>(&GraphsState::btnIncomeClick),
								SDL_BUTTON_LEFT);
	_isfFinance->onMousePress(	static_cast<ActionHandler>(&GraphsState::btnFinanceClick),
								SDL_BUTTON_LEFT);

	_isfGeoscape->onMousePress(		static_cast<ActionHandler>(&GraphsState::btnGeoscapeClick),
									SDL_BUTTON_LEFT);
	_isfGeoscape->onKeyboardPress(	static_cast<ActionHandler>(&GraphsState::btnGeoscapeClick),
									Options::keyCancel);
	_isfGeoscape->onKeyboardPress(	static_cast<ActionHandler>(&GraphsState::btnGeoscapeClick),
									Options::keyOk);
	_isfGeoscape->onKeyboardPress(	static_cast<ActionHandler>(&GraphsState::btnGeoscapeClick),
									Options::keyOkKeypad);
	_isfGeoscape->onKeyboardPress(	static_cast<ActionHandler>(&GraphsState::btnGeoscapeClick),
									Options::keyGeoGraphs);

	centerSurfaces();


	_timerBlink = new Timer(250u);
	_timerBlink->onTimer(static_cast<StateHandler>(&GraphsState::blink));

	initButtons();

	TOGGLEALL_yReg = static_cast<int>(_btnRegions.size())   * HEIGHT_btn;
	TOGGLEALL_yCou = static_cast<int>(_btnCountries.size()) * HEIGHT_btn;

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
	delete _timerBlink;

	std::string toggles;
	for (size_t
			i = 0u;
			i != _regionToggles.size();
			++i)
	{
		toggles.push_back(_regionToggles[i]->_pushed ? '1' : '0');
		delete _regionToggles[i];
	}
	_playSave->setGraphRegionToggles(toggles);

	toggles.clear();
	for (size_t
			i = 0u;
			i != _countryToggles.size();
			++i)
	{
		toggles.push_back(_countryToggles[i]->_pushed ? '1' : '0');
		delete _countryToggles[i];
	}
	_playSave->setGraphCountryToggles(toggles);

	toggles.clear();
	for (size_t
			i = 0u;
			i != _financeToggles.size();
			++i)
	{
		toggles.push_back(_financeToggles[i] ? '1' : '0');
	}
	_playSave->setGraphFinanceToggles(toggles);
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
			_timerBlink->start();
			return;
		}
	}

//	for (std::vector<GraphBtnInfo*>::const_iterator	// not needed because Country-areas are all subsumed within Regions;
//			i = _countryToggles.begin();			// that is, if a country is blinking its region will already be blinking.
//			i != _countryToggles.end();				// So just start the gosh-darned Timer.
//			++i)
//	{
//		if ((*i)->_blinkA == true || (*i)->_blinkX == true)
//		{
//			_timerBlink->start();
//			return;
//		}
//	}
}

/**
 * Handles State thinking.
 */
void GraphsState::think()
{
	switch (recallPage)
	{
		case 0: case 1: case 2: case 3:
			_timerBlink->think(this, nullptr);
	}
}

/**
 * Makes recent activity Text blink.
 */
void GraphsState::blink() // private.
{
	static bool vis (true);

	if (_reset == true)
	{
		_timerBlink->stop();
		vis = true;
	}
	else if (_forceVis == true)
	{
		_forceVis = false;
		vis = true;
	}
	else
		vis = !vis;

	size_t id (0u);

	switch (recallPage)
	{
		case 0: // region UFO
			for (std::vector<bool>::const_iterator
					i = _blinkRegionAlien.begin();
					i != _blinkRegionAlien.end();
					++i, ++id)
			{
				if (*i == true)
					_txtRegionActA.at(id)->setVisible(vis);
			}
			break;

		case 1: // region XCOM
			for (std::vector<bool>::const_iterator
					i = _blinkRegionXCom.begin();
					i != _blinkRegionXCom.end();
					++i, ++id)
			{
				if (*i == true)
					_txtRegionActX.at(id)->setVisible(vis);
			}
			break;

		case 2: // country UFO
			for (std::vector<bool>::const_iterator
					i = _blinkCountryAlien.begin();
					i != _blinkCountryAlien.end();
					++i, ++id)
			{
				if (*i == true)
					_txtCountryActA.at(id)->setVisible(vis);
			}
			break;

		case 3: // country XCOM
			for (std::vector<bool>::const_iterator
					i = _blinkCountryXCom.begin();
					i != _blinkCountryXCom.end();
					++i, ++id)
			{
				if (*i == true)
					_txtCountryActX.at(id)->setVisible(vis);
			}
	}
}

/**
 * Exits to the previous screen.
 * @param action - pointer to an Action
 */
void GraphsState::btnGeoscapeClick(Action*)
{
	SDL_EnableKeyRepeat(0, SDL_DEFAULT_REPEAT_INTERVAL);

	_game->getScreen()->fadeScreen();

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

		_btnReset->setVisible(_timerBlink->isRunning() == true);
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

		_btnReset->setVisible(_timerBlink->isRunning() == true);
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

		_btnToggleAll->setY(TOGGLEALL_yCou + SCREEN_OFFSET_y);
		_btnToggleAll->setVisible();
		initToggleAll();

		_btnReset->setVisible(_timerBlink->isRunning() == true);
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

		_btnToggleAll->setY(TOGGLEALL_yCou + SCREEN_OFFSET_y);
		_btnToggleAll->setVisible();
		initToggleAll();

		_btnReset->setVisible(_timerBlink->isRunning() == true);
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
void GraphsState::btnRegionListPress(Action* action)
{
	ToggleTextButton* const btn (dynamic_cast<ToggleTextButton*>(action->getSender()));
	size_t btnId;

	if (btn == _btnRegionTotal)
		btnId = _regionToggles.size() - 1u;
	else
	{
		for (
				btnId = 0u;
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
void GraphsState::btnCountryListPress(Action* action)
{
	ToggleTextButton* const btn (dynamic_cast<ToggleTextButton*>(action->getSender()));
	size_t btnId;

	if (btn == _btnCountryTotal)
		btnId = _countryToggles.size() - 1u;
	else
	{
		for (
				btnId = 0u;
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
void GraphsState::btnFinanceListPress(Action* action)
{
	ToggleTextButton* const btn (dynamic_cast<ToggleTextButton*>(action->getSender()));
	size_t btnId;

	for (
			btnId = 0u;
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
 * Locks the vertical scale to current values.
 * @param action - pointer to an Action
 */
void GraphsState::btnLockPress(Action*) // private.
{
	if (!(_lock = _btnLockScale->getPressed()))
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
			i = _regions->begin();
			i != _regions->end();
			++i)
		(*i)->resetActivity();

	for (std::vector<Country*>::const_iterator
			i = _countries->begin();
			i != _countries->end();
			++i)
		(*i)->resetActivity();
}

/**
 * Initializes the toggle-all stuff.
 */
void GraphsState::initToggleAll() // private.
{
	bool allPressed (true);
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
	const bool vis (_btnToggleAll->getPressed() == true);

	if (_country == true)
	{
		for (size_t
				i = 0u;
				i != _countries->size();
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
				i = 0u;
				i != _regions->size();
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
 * Sets the graph-lines to an expansion-factor by mouse-click.
 * @param action - pointer to an Action
 */
void GraphsState::btnFactorPress(Action* action) // private.
{
	TextButton* const sender (dynamic_cast<TextButton*>(action->getSender()));
	if (sender != nullptr) // check that this isn't a fake-call by keyFactorPress()
	{
		if		(sender == _btnFactor1) recallExpansion = GF_DEFAULT;	// NOTE: The 'sender' can't be checked against the '_userFactor' group first
		else if	(sender == _btnFactor2) recallExpansion = GF_HALF;		// because the ActionHandler has already switched '_userFactor' to 'sender'.
		else if	(sender == _btnFactor4) recallExpansion = GF_QUARTER;	// So trying to pre-determine if a click was made on an already-active TextButton
																		// won't work unless changes are made to ActionHandler.
		drawLines(false);
	}
}

/**
 * Sets the graph-lines to an expansion-factor by hot-key.
 * @param action - pointer to an Action
 */
void GraphsState::keyFactorPress(Action* action) // private.
{
	TextButton* const sender (dynamic_cast<TextButton*>(action->getSender()));
	switch (action->getDetails()->key.keysym.sym)
	{
		case SDLK_1:
			if (sender != _userFactor)
			{
				sender->mousePress(_game->getFakeMouseActionD(), this);
				recallExpansion = GF_DEFAULT;
			}
			break;

		case SDLK_2:
			if (sender != _userFactor)
			{
				sender->mousePress(_game->getFakeMouseActionD(), this);
				recallExpansion = GF_HALF;
			}
			break;

		case SDLK_3:
			if (sender != _userFactor)
			{
				sender->mousePress(_game->getFakeMouseActionD(), this);
				recallExpansion = GF_QUARTER;
			}
	}
	drawLines(false);
}

/**
 * Removes all elements from view.
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
			srf->setPixelColor(x,y, 0u);
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
	const int delta (std::max(10,
							 (valHigh - valLow) / GRIDCELLS_y));
	for (size_t
			i = 0u;
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

	if (_finance == false)
	{
		if (_country == false)
			drawRegionLines();
		else
			drawCountryLines();
	}
	else
		drawFinanceLines();
}

/**
 * Sets up the screens and draws the lines for region buttons to toggle on and off.
 */
void GraphsState::drawRegionLines() // private.
{
	int // calculate totals and set the upward maximum
		scaleHigh (0),
		scaleLow  (0),
		total,
		act,

		totals[MONTHS_u] {0,0,0,0,0,0,0,0,0,0,0,0};

	for (size_t
			i = 0u;
			i != _playSave->getFundsList().size();
			++i)
	{
		total = 0;

		for (size_t
				j = 0u;
				j != _regions->size();
				++j)
		{
			if (_alien == true)
				act = _regions->at(j)->getActivityAlien().at(i);
			else
				act = _regions->at(j)->getActivityXCom().at(i);

			total += act;

			if (_regionToggles.at(j)->_pushed == true)
			{
				if (act > scaleHigh)
					scaleHigh = act;
				else if (act < scaleLow && _alien == false) // aLiens never get into negative scores.
					scaleLow = act;
			}
		}

		if (_regionToggles.back()->_pushed == true
			&& total > scaleHigh)
		{
			scaleHigh = total;
		}
	}


	const int low (scaleLow); // adjust the scale to fit the upward maximum
	int delta (scaleHigh - scaleLow);

	switch (recallExpansion)
	{
		case GF_QUARTER: delta >>= 1u; // no break.
		case GF_HALF:    delta >>= 1u;
	}

	int test (10);
	while (delta > GRIDCELLS_y * test)
		test += 10;

	scaleLow = 0;
	scaleHigh = GRIDCELLS_y * test;

	while (low < scaleLow)
	{
		scaleLow  -= test;
		scaleHigh -= test;
	}

	if (_lock == false || _lockRegionsHigh == -1)
	{
		_lockRegionsHigh = scaleHigh;
		_lockRegionsLow  = scaleLow;
	}
	else
	{
		scaleHigh = _lockRegionsHigh;
		scaleLow  = _lockRegionsLow;
	}

	// Figure out how many units to the pixel then plot the points for the graph
	// and connect the dots.
	const float pixelUnits (static_cast<float>(scaleHigh - scaleLow) / PIXELS_y);

	int reduction;
	Sint16
		x,y;
	std::vector<Sint16> lineVector;
	Region* region;

	for (size_t // draw region lines
			i = 0u;
			i != _regions->size();
			++i)
	{
		region = _regions->at(i);

		_alienRegionLines.at(i)->clear();
		_xcomRegionLines.at(i)->clear();

		lineVector.clear();

		for (size_t
				j = 0u;
				j != MONTHS_u;
				++j)
		{
			y = static_cast<Sint16>(175 + static_cast<int>(Round(static_cast<float>(scaleLow) / pixelUnits)));

			if (_alien == true)
			{
				if (j < region->getActivityAlien().size())
				{
					reduction = region->getActivityAlien().at(region->getActivityAlien().size() - (j + 1u));
					y = static_cast<Sint16>(y - static_cast<int>(Round(static_cast<float>(reduction) / pixelUnits)));
					totals[j] += reduction;
				}
			}
			else
			{
				if (j < region->getActivityXCom().size())
				{
					reduction = region->getActivityXCom().at(region->getActivityXCom().size() - (j + 1u));
					y = static_cast<Sint16>(y - static_cast<int>(Round(static_cast<float>(reduction) / pixelUnits)));
					totals[j] += reduction;
				}
			}

			if (y > 175) y = 175;

			lineVector.push_back(y);

			if (lineVector.size() > 1u)
			{
				x = static_cast<Sint16>(312 - static_cast<int>(j) * 17);

				if (_alien == true)
				{
					_alienRegionLines.at(i)->drawLine(
													x,y,
													static_cast<Sint16>(x + 17),
													lineVector.at(lineVector.size() - 2u),
													static_cast<Uint8>(_regionToggles.at(i)->_colorPushed + 4u));
					boxLines(_alienRegionLines.at(i));
				}
				else
				{
					_xcomRegionLines.at(i)->drawLine(
													x,y,
													static_cast<Sint16>(x + 17),
													lineVector.at(lineVector.size() - 2u),
													static_cast<Uint8>(_regionToggles.at(i)->_colorPushed + 4u));
					boxLines(_xcomRegionLines.at(i));
				}
			}
		}

		if (_alien == true)
			_alienRegionLines.at(i)->setVisible(_regionToggles.at(i)->_pushed);
		else
			_xcomRegionLines.at(i)->setVisible(_regionToggles.at(i)->_pushed);
	}


	if (_alien == true) // set up the TOTAL line ->
		_alienRegionLines.back()->clear();
	else
		_xcomRegionLines.back()->clear();

	Uint8 color (static_cast<Uint8>(_uiGraphs->getElement("regionTotal")->color2));
	lineVector.clear();

	for (size_t
			i = 0u;
			i != MONTHS_u;
			++i)
	{
		y = static_cast<Sint16>(175 + static_cast<int>(Round(static_cast<float>(scaleLow) / pixelUnits)));

		if (totals[i] > 0)
			y = static_cast<Sint16>(y - static_cast<int>(Round(static_cast<float>(totals[i]) / pixelUnits)));

		lineVector.push_back(y);

		if (lineVector.size() > 1u)
		{
			x = static_cast<Sint16>(312 - static_cast<int>(i) * 17);

			if (_alien == true)
			{
				_alienRegionLines.back()->drawLine(
												x,y,
												static_cast<Sint16>(x + 17),
												lineVector.at(lineVector.size() - 2u),
												color);
				boxLines(_alienRegionLines.back());
			}
			else
			{
				_xcomRegionLines.back()->drawLine(
												x,y,
												static_cast<Sint16>(x + 17),
												lineVector.at(lineVector.size() - 2u),
												color);
				boxLines(_xcomRegionLines.back());
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
		scaleHigh (0),
		scaleLow  (0),
		total,
		act,

		totals[MONTHS_u] {0,0,0,0,0,0,0,0,0,0,0,0};

	for (size_t
			i = 0u;
			i != _playSave->getFundsList().size();
			++i)
	{
		total = 0;

		for (size_t
				j = 0u;
				j != _countries->size();
				++j)
		{
			if (_alien == true)
				act = _countries->at(j)->getActivityAlien().at(i);
			else if (_income == true)
				act = _countries->at(j)->getFunding().at(i);
			else
				act = _countries->at(j)->getActivityXCom().at(i);

			total += act;

			if (_countryToggles.at(j)->_pushed == true)
			{
				if (act > scaleHigh)
					scaleHigh = act;
				else if (act < scaleLow && _alien == false && _income == false) // NOTE: aLien & Income never go into negative values.
					scaleLow = act;
			}
		}

		if (_countryToggles.back()->_pushed == true
			&& total > scaleHigh)
		{
			scaleHigh = total;
		}
	}


	const int low (scaleLow); // adjust the scale to fit the upward maximum
	int delta (scaleHigh - scaleLow);

	switch (recallExpansion)
	{
		case GF_QUARTER: delta >>= 1u; // no break.
		case GF_HALF:    delta >>= 1u;
	}

	int test (10);
	while (delta > GRIDCELLS_y * test)
		test += 10;

	scaleLow = 0;
	scaleHigh = GRIDCELLS_y * test;

	while (low < scaleLow)
	{
		scaleLow  -= test;
		scaleHigh -= test;
	}

	if (_lock == false || _lockCountriesHigh == -1)
	{
		_lockCountriesHigh = scaleHigh;
		_lockCountriesLow  = scaleLow;
	}
	else
	{
		scaleHigh = _lockCountriesHigh;
		scaleLow  = _lockCountriesLow;
	}

	// Figure out how many units to the pixel then plot the points for the graph
	// and connect the dots.
	const float pixelUnits (static_cast<float>(scaleHigh - scaleLow) / PIXELS_y);

	int reduction;
	Sint16
		x,y;
	std::vector<Sint16> lineVector;
	Country* country;

	for (size_t // draw country lines
			i = 0u;
			i != _countries->size();
			++i)
	{
		country = _countries->at(i);

		_alienCountryLines.at(i)->clear();
		_xcomCountryLines.at(i)->clear();
		_incomeLines.at(i)->clear();

		lineVector.clear();

		for (size_t
				j = 0u;
				j != MONTHS_u;
				++j)
		{
			y = static_cast<Sint16>(175 + static_cast<int>(Round(static_cast<float>(scaleLow) / pixelUnits)));

			if (_alien == true)
			{
				if (j < country->getActivityAlien().size())
				{
					reduction = country->getActivityAlien().at(country->getActivityAlien().size() - (j + 1u));
					y = static_cast<Sint16>(y - static_cast<int>(Round(static_cast<float>(reduction) / pixelUnits)));
					totals[j] += reduction;
				}
			}
			else if (_income == true)
			{
				if (j < country->getFunding().size())
				{
					reduction = country->getFunding().at(country->getFunding().size() - (j + 1u));
					y = static_cast<Sint16>(y - static_cast<int>(Round(static_cast<float>(reduction) / pixelUnits)));
					totals[j] += reduction;
				}
			}
			else
			{
				if (j < country->getActivityXCom().size())
				{
					reduction = country->getActivityXCom().at(country->getActivityXCom().size() - (j + 1u));
					y = static_cast<Sint16>(y - static_cast<int>(Round(static_cast<float>(reduction) / pixelUnits)));
					totals[j] += reduction;
				}
			}

			if (y > 175) y = 175;

			lineVector.push_back(y);

			if (lineVector.size() > 1u)
			{
				x = static_cast<Sint16>(312 - static_cast<int>(j) * 17);

				if (_alien == true)
				{
					_alienCountryLines.at(i)->drawLine(
													x,y,
													static_cast<Sint16>(x + 17),
													lineVector.at(lineVector.size() - 2u),
													static_cast<Uint8>(_countryToggles.at(i)->_colorPushed + 4u));
					boxLines(_alienCountryLines.at(i));
				}
				else if (_income == true)
				{
					_incomeLines.at(i)->drawLine(
												x,y,
												static_cast<Sint16>(x + 17),
												lineVector.at(lineVector.size() - 2u),
												static_cast<Uint8>(_countryToggles.at(i)->_colorPushed + 4u));
					boxLines(_incomeLines.at(i));
				}
				else
				{
					_xcomCountryLines.at(i)->drawLine(
													x,y,
													static_cast<Sint16>(x + 17),
													lineVector.at(lineVector.size() - 2u),
													static_cast<Uint8>(_countryToggles.at(i)->_colorPushed + 4u));
					boxLines(_xcomCountryLines.at(i));
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


	if (_alien == true) // set up the TOTAL line ->
		_alienCountryLines.back()->clear();
	else if (_income == true)
		_incomeLines.back()->clear();
	else
		_xcomCountryLines.back()->clear();

	Uint8 color (static_cast<Uint8>(_uiGraphs->getElement("countryTotal")->color2));
	lineVector.clear();

	for (size_t
			i = 0u;
			i != MONTHS_u;
			++i)
	{
		y = static_cast<Sint16>(175 + static_cast<int>(Round(static_cast<float>(scaleLow) / pixelUnits)));

		if (totals[i] > 0)
			y = static_cast<Sint16>(y - static_cast<int>(Round(static_cast<float>(totals[i]) / pixelUnits)));

		lineVector.push_back(y);

		if (lineVector.size() > 1u)
		{
			x = static_cast<Sint16>(312 - static_cast<int>(i) * 17);

			if (_alien == true)
			{
				_alienCountryLines.back()->drawLine(
												x,y,
												static_cast<Sint16>(x + 17),
												lineVector.at(lineVector.size() - 2u),
												color);
				boxLines(_alienCountryLines.back());
			}
			else if (_income == true)
			{
				_incomeLines.back()->drawLine(
											x,y,
											static_cast<Sint16>(x + 17),
											lineVector.at(lineVector.size() - 2u),
											color);
				boxLines(_incomeLines.back());
			}
			else
			{
				_xcomCountryLines.back()->drawLine(
												x,y,
												static_cast<Sint16>(x + 17),
												lineVector.at(lineVector.size() - 2u),
												color);
				boxLines(_xcomCountryLines.back());
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
	_txtFactor->setVisible(_income == true);
}

/**
 * Sets up the screens and draws the lines for the finance buttons to toggle on and off.
 */
void GraphsState::drawFinanceLines() // private. // Council Analytics
{
	int
		scaleHigh (0),
		scaleLow  (0),

		income[MONTHS_u]		{0,0,0,0,0,0,0,0,0,0,0,0},
		expenditure[MONTHS_u]	{0,0,0,0,0,0,0,0,0,0,0,0},
		maintenance[MONTHS_u]	{0,0,0,0,0,0,0,0,0,0,0,0},
		balance[MONTHS_u]		{0,0,0,0,0,0,0,0,0,0,0,0},
		score[MONTHS_u]			{0,0,0,0,0,0,0,0,0,0,0,0},

		baseIncomes  (0),
		baseExpenses (0);

	// start filling those arrays with score values;
	// determine which is the highest one being displayed, so the scale can be adjusted
	size_t rit;
	for (size_t
			i = 0u;
			i != _playSave->getFundsList().size(); // use Balance as template.
			++i)
	{
		rit = _playSave->getFundsList().size() - (i + 1u);

		if (i == 0u)
		{
			for (std::vector<Base*>::const_iterator
					j = _playSave->getBases()->begin();
					j != _playSave->getBases()->end();
					++j)
			{
				baseIncomes  += (*j)->getCashIncome();
				baseExpenses += (*j)->getCashSpent();
			}

			income[i]		= baseIncomes  / 1000; // perhaps add Country funding
			expenditure[i]	= baseExpenses / 1000;
			maintenance[i]	= _playSave->getBaseMaintenances() / 1000; // use current
		}
		else
		{
			income[i]		= static_cast<int>(_playSave->getIncomeList().at(rit)      / 1000); // perhaps add Country funding
			expenditure[i]	= static_cast<int>(_playSave->getExpenditureList().at(rit) / 1000);
			maintenance[i]	= static_cast<int>(_playSave->getMaintenanceList().at(rit) / 1000);
		}

		balance[i] = static_cast<int>(_playSave->getFundsList().at(rit) / 1000);
		score[i] = _playSave->getResearchScores().at(rit);


		for (std::vector<Region*>::const_iterator
				j = _regions->begin();
				j != _regions->end();
				++j)
		{
			score[i] += (*j)->getActivityXCom().at(rit) - (*j)->getActivityAlien().at(rit);
		}

		if (i == 0u) // values are stored backwards. So take 1st value for last.
			_txtScore->setText(Text::intWide(score[i]));


		if (_financeToggles.at(0u) == true) // INCOME
		{
			if (income[i] > scaleHigh)
				scaleHigh = income[i];

			if (income[i] < scaleLow)
				scaleLow = income[i];
		}

		if (_financeToggles.at(1u) == true) // EXPENDITURE
		{
			if (expenditure[i] > scaleHigh)
				scaleHigh = expenditure[i];

			if (expenditure[i] < scaleLow)
				scaleLow = expenditure[i];
		}

		if (_financeToggles.at(2u) == true) // MAINTENANCE
		{
			if (maintenance[i] > scaleHigh)
				scaleHigh = maintenance[i];

			if (maintenance[i] < scaleLow)
				scaleLow = maintenance[i];
		}

		if (_financeToggles.at(3u) == true) // BALANCE
		{
			if (balance[i] > scaleHigh)
				scaleHigh = balance[i];

			if (balance[i] < scaleLow)
				scaleLow = balance[i];
		}

		if (_financeToggles.at(4u) == true) // SCORE
		{
			if (score[i] > scaleHigh)
				scaleHigh = score[i];

			if (score[i] < scaleLow)
				scaleLow = score[i];
		}
	}

	for (size_t // toggle screens
			i = 0u;
			i != 5u;
			++i)
	{
		_financeLines.at(i)->setVisible(_financeToggles.at(i));
		_financeLines.at(i)->clear();
	}


	const int low (scaleLow); // adjust the scale to fit the upward maximum
	int delta (scaleHigh - scaleLow);

	switch (recallExpansion)
	{
		case GF_QUARTER: delta >>= 1; // no break.
		case GF_HALF:    delta >>= 1;
	}

	int test (100);
	while (delta > GRIDCELLS_y * test)
		test += 100;

	scaleLow = 0;
	scaleHigh = GRIDCELLS_y * test;

	while (low < scaleLow)
	{
		scaleLow  -= test;
		scaleHigh -= test;
	}

	if (_lock == false || _lockFinanceHigh == -1)
	{
		_lockFinanceHigh = scaleHigh;
		_lockFinanceLow  = scaleLow;
	}
	else
	{
		scaleHigh = _lockFinanceHigh;
		scaleLow  = _lockFinanceLow;
	}

	// Figure out how many units to the pixel then plot the points for the graph
	// and connect the dots.
	const float pixelUnits (static_cast<float>(scaleHigh - scaleLow) / PIXELS_y);

	Uint8 color;
	unsigned hueFactor;
	Sint16
		reduction,
		x,y;

	std::vector<Sint16> lineVector;

	for (size_t
			i = 0u;
			i != 5u;
			++i)
	{
		lineVector.clear();

		for (size_t
				j = 0u;
				j != MONTHS_u;
				++j)
		{
			y = static_cast<Sint16>(175 + static_cast<int>(Round(static_cast<float>(scaleLow) / pixelUnits)));

			switch (i)
			{
				case 0u:
					reduction = static_cast<Sint16>(Round(static_cast<float>(income[j]) / pixelUnits));
					break;
				case 1u:
					reduction = static_cast<Sint16>(Round(static_cast<float>(expenditure[j]) / pixelUnits));
					break;
				case 2u:
					reduction = static_cast<Sint16>(Round(static_cast<float>(maintenance[j]) / pixelUnits));
					break;
				case 3u:
					reduction = static_cast<Sint16>(Round(static_cast<float>(balance[j]) / pixelUnits));
					break;
				default: // avoid vc++ linker warning.
				case 4u:
					reduction = static_cast<Sint16>(Round(static_cast<float>(score[j]) / pixelUnits));
			}

			y = static_cast<Sint16>(y - reduction);

			lineVector.push_back(y);

			if (lineVector.size() > 1u)
			{
				if (i % 2 != 0u)
					color = 8u;
				else
					color = 0u;

				switch (i) // switch colors for Income (was yellow) and Maintenance (was green)
				{
					case 0u: hueFactor = 2u; break;
					case 2u: hueFactor = 0u; break;
					default:
						hueFactor = i;
				}

				color = static_cast<Uint8>(Palette::blockOffset(static_cast<Uint8>((hueFactor >> 1u) + 1u)) + color);

				x = static_cast<Sint16>(312 - static_cast<int>(j) * 17);
				_financeLines.at(i)->drawLine(
											x,y,
											static_cast<Sint16>(x + 17),
											lineVector.at(lineVector.size() - 2u),
											color);
				boxLines(_financeLines.at(i));
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
				switch (action->getDetails()->button.button)
				{
					case SDL_BUTTON_WHEELUP:
						dirVal = -1;
						if (static_cast<int>(_btnCountryOffset) + dirVal < 0)
							return;
						break;

					case SDL_BUTTON_WHEELDOWN:
						dirVal = 1;
						if (static_cast<int>(_btnCountryOffset + GRAPH_BUTTONS) + dirVal >= static_cast<int>(_countryToggles.size()))
							return;
						break;

					default:
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
//		else // _region -> not needed unless quantity of Regions increases over GRAPH_BUTTONS. Ain't likely to happen.
//		{
//			if (_regionToggles.size() > GRAPH_BUTTONS)
//			{
//				int dirVal = 0;
//				if (action->getDetails()->button.button == SDL_BUTTON_WHEELUP)
//					dirVal = -1;
//				else if (action->getDetails()->button.button == SDL_BUTTON_WHEELDOWN)
//					dirVal = 1;
//
//				if (dirVal != 0)
//					scrollButtons(dirVal);
//			}
//		}
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

		std::vector<ToggleTextButton*>::const_iterator pBtn (_btnCountries.begin());
		std::vector<Text*>::const_iterator pActA (_txtCountryActA.begin());
		std::vector<Text*>::const_iterator pActX (_txtCountryActX.begin());
		std::vector<bool>::iterator pBlinkA (_blinkCountryAlien.begin());
		std::vector<bool>::iterator pBlinkX (_blinkCountryXCom.begin());

		size_t row (0u);
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
		const GraphBtnInfo* const info,
		ToggleTextButton* const btn,
		Text* const aLiens,
		Text* const xCom)
{
	btn->setText(info->_label);
	btn->setColorInvert(info->_colorPushed);
	btn->setPressed(info->_pushed);

	aLiens->setText(Text::intWide(info->_actA));
	aLiens->setColor(info->_colorText);

	xCom->setText(Text::intWide(info->_actX));
	xCom->setColor(info->_colorText);
}

}
