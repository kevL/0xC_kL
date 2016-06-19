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

#ifndef OPENXCOM_MANUFACTUREINFOSTATE_H
#define OPENXCOM_MANUFACTUREINFOSTATE_H

#include "../Engine/State.h"


namespace OpenXcom
{

class ArrowButton;
class Base;
class Production;
class RuleManufacture;
class Text;
class TextButton;
class Timer;
class ToggleTextButton;
class Window;


/**
 * Screen that allows changing of Manufacture settings.
*/
class ManufactureInfoState
	:
		public State
{

private:
	int _producedValue;

	ArrowButton
		* _btnEngineerLess,
		* _btnEngineerMore,
		* _btnUnitLess,
		* _btnUnitMore;
	Base* _base;
	Production* _production;
	const RuleManufacture* _manfRule;
	Text
		* _txtFreeEngineer,
		* _txtFreeSpace,
		* _txtEngineers,
		* _txtEngineersDesc,
		* _txtProfit,
		* _txtTimeDesc,
		* _txtTimeLeft,
		* _txtTitle,
		* _txtTotal,
		* _txtTotalDesc;
	TextButton
		* _btnOk,
		* _btnStop;
	Timer
		* _timerEngineersLess,
		* _timerEngineersMore,
		* _timerUnitsLess,
		* _timerUnitsMore;
	ToggleTextButton* _btnSell;
	Window* _window;

	/// Builds the User Interface.
	void buildUi();
	/// Caches production-value for profit calculations.
	void initProfit();

	/// Handler for the Stop button.
	void btnStopClick(Action* action);
	/// Handler for the OK button.
	void btnOkClick(Action* action);
	/// Helper to exit the State.
	void exitState();

	/// Handler for releasing the Sell button.
	void btnSellRelease(Action* action);

	/// Updates display of assigned/available engineers and workshop-space.
	void assignEngineers();
	/// Calculates the monthly change in funds due to profit/expenses.
	int calcProfit();
	/// Formats the profit-value.
	bool formatProfit(
			int profit,
			std::wostringstream& woststr);

	/// Handler for pressing the more engineer button.
	void engineersMorePress(Action* action);
	/// Handler for releasing the more engineer button.
	void engineersMoreRelease(Action* action);

	/// Handler for pressing the less engineer button.
	void engineersLessPress(Action* action);
	/// Handler for releasing the less engineer button.
	void engineersLessRelease(Action* action);

	/// Handler for pressing the more unit button.
	void unitsMorePress(Action* action);
	/// Handler for releasing the more unit button.
	void unitsMoreRelease(Action* action);
	/// Handler for clicking the more unit button.
	void unitsMoreClick(Action* action);

	/// Handler for pressing the less unit button.
	void unitsLessPress(Action* action);
	/// Handler for releasing the less unit button.
	void unitsLessRelease(Action* action);
	/// Handler for clicking the less unit button.
	void unitsLessClick(Action* action);

	/// Runs state functionality every cycle.
	void think() override;

	/// Adds engineers to the production.
	void onEngineersMore();
	/// Adds a given quantity of engineers to the production if possible.
	void engineersMoreByValue(int delta);
	/// Subtracts engineers from the production.
	void onEngineersLess();
	/// Subtracts a given quantity of engineers from the production if possible.
	void engineersLessByValue(int delta);

	/// Increases quantity of units to produce.
	void onUnitsMore();
	/// Adds a given quantity of units to produce if possible.
	void unitsMoreByValue(int delta);
	/// Decreases quantity of units to produce.
	void onUnitsLess();
	/// Subtracts a given quantity of units to produce if possible.
	void unitsLessByValue(int delta);

	/// Gets the quantity by which to increase/decrease.
	int stepDelta() const;


	public:
		/// Creates a ManufactureInfo state (new production).
		ManufactureInfoState(
				Base* const base,
				const RuleManufacture* const _manfRule);
		/// Creates a ManufactureInfo (modify production).
		ManufactureInfoState(
				Base* const base,
				Production* const production);
		/// Cleans up the ManufactureInfo state.
		~ManufactureInfoState();
};

}

#endif
