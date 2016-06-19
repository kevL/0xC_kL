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
		* _btnEngineerDown,
		* _btnEngineerUp,
		* _btnUnitDown,
		* _btnUnitUp;
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
		* _timerDecEngineers,
		* _timerDecUnits,
		* _timerIncEngineers,
		* _timerIncUnits;
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

	/// Updates display of assigned/available engineers and workshop space.
	void assignEngineers();
	/// Calculates the monthly change in funds due to profit/expenses.
	int calcProfit();
	/// Formats the profit-value.
	bool formatProfit(
			int profit,
			std::wostringstream& woststr);

	/// Adds given number of engineers to the project if possible.
	void incEngineers(int change);
	/// Handler for pressing the more engineer button.
	void incEngineersPress(Action* action);
	/// Handler for releasing the more engineer button.
	void incEngineersRelease(Action* action);
	/// Handler for clicking the more engineer button.
	void incEngineersClick(Action* action);

	/// Removes the given number of engineers from the project if possible.
	void decEngineers(int change);
	/// Handler for pressing the less engineer button.
	void decEngineersPress(Action* action);
	/// Handler for releasing the less engineer button.
	void decEngineersRelease(Action* action);
	/// Handler for clicking the less engineer button.
	void decEngineersClick(Action* action);

	/// Adds given number of units to produce to the project if possible.
	void incUnits(int change);
	/// Handler for pressing the more unit button.
	void incUnitsPress(Action* action);
	/// Handler for releasing the more unit button.
	void incUnitsRelease(Action* action);
	/// Handler for clicking the more unit button.
	void incUnitsClick(Action* action);

	/// Removes the given number of units to produce from the project if possible.
	void decUnits(int change);
	/// Handler for pressing the less unit button.
	void decUnitsPress(Action* action);
	/// Handler for releasing the less unit button.
	void decUnitsRelease(Action* action);
	/// Handler for clicking the less unit button.
	void decUnitsClick(Action* action);

	/// Adds engineers to the production (if possible).
	void onIncEngineers();
	/// Removes engineers from the production (if possible).
	void onDecEngineers();

	/// Increases quantity of units to make.
	void onIncUnits();
	/// Decreases quantity of units to make (if possible).
	void onDecUnits();

	/// Gets the quantity by which to increase/decrease.
	int stepDelta() const;

	/// Runs state functionality every cycle.
	void think() override;


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
