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
class ManufactureProject;
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
	bool _start;

	int _valueProduct;

	ArrowButton
		* _btnEngineerLess,
		* _btnEngineerMore,
		* _btnUnitLess,
		* _btnUnitMore;
	Base* _base;
	ManufactureProject* _project;
	const RuleManufacture* _mfRule;
	Text
		* _txtFreeEngineer,
		* _txtFreeSpace,
		* _txtEngineers,
		* _txtEngineersDesc,
		* _txtProfit,
		* _txtDurationDesc,
		* _txtDuration,
		* _txtTitle,
		* _txtUnits,
		* _txtUnitsDesc;
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
	void build();
	/// Caches manufacture-value for profit calculations.
	void initProfit();

	/// Handler for the Stop button.
	void btnStopClick(Action* action);
	/// Handler for the Ok button.
	void btnOkClick(Action* action);
	/// Helper to exit the State.
	void exitState();

	/// Handler for releasing the Sell button.
	void btnSellRelease(Action* action);

	/// Updates the display.
	void updateInfo();
	/// Calculates the monthly change in funds due to profit/expenses.
	int calcProfit();
	/// Formats the profit-value.
	static bool formatProfit(
			int profit,
			std::wostringstream& woststr);

	/// Handler for pressing the more-engineers button.
	void engineersMorePress(Action* action);
	/// Handler for releasing the more-engineers button.
	void engineersMoreRelease(Action* action);

	/// Handler for pressing the less-engineers button.
	void engineersLessPress(Action* action);
	/// Handler for releasing the less-engineers button.
	void engineersLessRelease(Action* action);

	/// Handler for pressing the more-units button.
	void unitsMorePress(Action* action);
	/// Handler for releasing the more-units button.
	void unitsMoreRelease(Action* action);
	/// Handler for clicking the more-units button.
	void unitsMoreClick(Action* action);

	/// Handler for pressing the less-units button.
	void unitsLessPress(Action* action);
	/// Handler for releasing the less-units button.
	void unitsLessRelease(Action* action);
	/// Handler for clicking the less-units button.
	void unitsLessClick(Action* action);

	/// Runs state functionality every cycle.
	void think() override;

	/// Adds engineers to the project.
	void onEngineersMore();
	/// Adds a given quantity of engineers to the proejct if possible.
	void engineersMoreByValue(int delta);
	/// Subtracts engineers from the project.
	void onEngineersLess();
	/// Subtracts a given quantity of engineers from the project if possible.
	void engineersLessByValue(int delta);

	/// Increases quantity of units.
	void onUnitsMore();
	/// Adds a given quantity of units if possible.
	void unitsMoreByValue(int delta);
	/// Decreases quantity of units.
	void onUnitsLess();
	/// Subtracts a given quantity of units if possible.
	void unitsLessByValue(int delta);


	public:
		/// Creates a ManufactureInfo state (start manufacture).
		ManufactureInfoState(
				Base* const base,
				const RuleManufacture* const _mfRule);
		/// Creates a ManufactureInfo state (adjust manufacture).
		ManufactureInfoState(
				Base* const base,
				ManufactureProject* const project);
		/// Cleans up the ManufactureInfo state.
		~ManufactureInfoState();
};

}

#endif
