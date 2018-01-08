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

#ifndef OPENXCOM_GRAPHSSTATE_H
#define OPENXCOM_GRAPHSSTATE_H

#include "../Engine/State.h"


namespace OpenXcom
{

enum GraphsExpansionFactor
{
	GF_DEFAULT,	// 0
	GF_HALF,	// 1
	GF_QUARTER	// 2
};

class Country;
class InteractiveSurface;
class Region;
class SavedGame;
class Surface;
class Text;
class TextButton;
class TextList;
class Timer;
class ToggleTextButton;

struct GraphBtnInfo;


/**
 * Graphs screen for displaying graphs of various monthly game data like
 * activity and funding.
 */
class GraphsState
	:
		public State
{

private:
	static const size_t
		GRAPH_BUTTONS	= 19u, // # visible btns. Does not include TOTAL btn.
		TEXTS_y			= 10u,
		MONTHS_u		= 12u;
	static const int
		MONTHS			= 12,
		YEARS			=  6,
		GRIDCELLS_y		=  9,
		HEIGHT_btn		= 10;

	static const float PIXELS_y;

	bool
		_alien,
		_country,
		_finance,
		_income,
		_init,
		_forceVis,	// true to ensure values are displayed when scrolling buttons
		_reset,		// true to stop buttons blinking & reset activity
		_lock;		// true to lock the scale

	int
	_lockRegionsHigh,
	_lockRegionsLow,
	_lockCountriesHigh,
	_lockCountriesLow,
	_lockFinanceHigh,
	_lockFinanceLow;

	size_t _btnCountryOffset;
//		_btnRegionOffset;

	std::vector<Country*>* _countries;
	std::vector<Region*>* _regions;

	InteractiveSurface
		* _bg,
		* _isfFinance,
		* _isfGeoscape,
		* _isfIncome,
		* _isfUfoCountry,
		* _isfUfoRegion,
		* _isfXcomCountry,
		* _isfXcomRegion;
	const RuleInterface* _uiGraphs;
	SavedGame* _playSave;
	Surface* _srfPageLine;
	Text
		* _txtScore,
		* _txtThous,
		* _txtTitle;
	TextButton
		* _btnReset,
		* _btnFactor1,
		* _btnFactor2,
		* _btnFactor4,
		* _userFactor;
	TextList
		* _lstMonths,
		* _lstYears;
	ToggleTextButton
		* _btnCountryTotal,
		* _btnRegionTotal,
		* _btnToggleAll,
		* _btnLockScale;

	Timer* _timerBlink;

	std::vector<bool>
		_blinkCountryAlien,
		_blinkCountryXCom,
		_blinkRegionAlien,
		_blinkRegionXCom,
		_financeToggles;

	std::vector<GraphBtnInfo*>
		_countryToggles,
		_regionToggles;
	std::vector<Surface*>
		_alienCountryLines,
		_alienRegionLines,
		_financeLines,
		_incomeLines,
		_xcomCountryLines,
		_xcomRegionLines;
	std::vector<Text*>
		_txtCountryActA,
		_txtCountryActX,
		_txtRegionActA,
		_txtRegionActX,
		_txtScale;
	std::vector<ToggleTextButton*>
		_btnCountries,
		_btnFinances,
		_btnRegions;

	/// Recalls buttons to their pre-Graph cTor row.
	void initButtons();

	/// Blinks recent activity-values.
	void blink();

	/// Changes the current page.
	void changePage();

	/// Locks the vertical scale to current values.
	void btnLockPress(Action*);
	/// Resets aLien/xCom activity and the blink indicators.
	void btnResetPress(Action* action);
	/// Initializes the toggle-all stuff.
	void prepToggleAll();
	/// Toggles all region/country buttons.
	void btnToggleAllPress(Action* action);
	/// Sets the graphs to an expansion by mouse-click.
	void btnFactorPress(Action* action);
	/// Sets the graphs to an expansion by hot-key.
	void keyFactorPress(Action* action);

	/// Resets all the elements on screen.
	void resetScreen();

	/// Clears pixels of lines that would otherwise draw overtop the title area.
	void boxLines(Surface* const srf);
	/// Updates the scale.
	void updateScale(
			int valLow,
			int valHigh);
	/// Decides which line-drawing-routine to call.
	void drawLines(bool reset = true);
	/// Draws Region lines.
	void drawRegionLines();
	/// Draws Country lines.
	void drawCountryLines();
	/// Draws Finances lines.
	void drawFinanceLines();

	/// Mouse-wheel handler for shifting up/down the buttons.
	void shiftButtons(Action* action);
	/// Scrolls button lists - scroll and repaint buttons' functions.
	void scrollButtons(
			int dirVal,
			bool init = false);
	/// Updates button appearances when scrolling the lists.
	void updateButton(
			const GraphBtnInfo* const info,
			ToggleTextButton* const btn,
			Text* const aLiens,
			Text* const xCom);


	public:
		/// Creates the Graphs state.
		GraphsState();
		/// Cleans up the Graphs state.
		~GraphsState();

		/// Handles state thinking.
		void think() override;

		/// Handler for clicking the Geoscape icon.
		void btnGeoscapeClick(Action* action);
		/// Handler for clicking the ufo region icon.
		void btnUfoRegionClick(Action* action);
		/// Handler for clicking the xcom region icon.
		void btnXcomRegionClick(Action* action);
		/// Handler for clicking the ufo country icon.
		void btnUfoCountryClick(Action* action);
		/// Handler for clicking the xcom country icon.
		void btnXcomCountryClick(Action* action);
		/// Handler for clicking the income icon.
		void btnIncomeClick(Action* action);
		/// Handler for clicking the finance icon.
		void btnFinanceClick(Action* action);
		/// Handler for clicking on a region button.
		void btnRegionListPress(Action* action);
		/// Handler for clicking on a country button.
		void btnCountryListPress(Action* action);
		/// Handler for clicking  on a finances button.
		void btnFinanceListPress(Action* action);
};

}

#endif
