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

#ifndef OPENXCOM_DEBRIEFEXTRASTATE_H
#define OPENXCOM_DEBRIEFEXTRASTATE_H

#include "../Engine/State.h"

#include <map>
//#include <string>


namespace OpenXcom
{

class Base;
class RuleItem;

class Text;
class TextButton;
class TextList;
class Timer;
class Window;


/**
 * Extra detail during Debriefings.
 */
class DebriefExtraState
	:
		public State
{

private:
	enum DebriefExtraScreen
	{
		DES_SOL_STATS,		// 0
		DES_LOOT_GAINED,	// 1
		DES_LOOT_LOST		// 2
	} _curScreen;

	static const Uint8
		YELLOW	= 138u,
		BROWN	= 143u,
		GRAY	= 159u,
		GREEN	= 239u;

	int _costTotal;
	size_t _sel;

	double _storeSize;

	std::wstring _error;

	std::vector<int>
		_qtysBuy,
		_qtysSell;

	std::map<const RuleItem*, int>
		_itemsGained,
		_itemsLost;
	std::map<std::wstring, std::vector<int>> _solStatDeltas;

	Text
		* _txtBaseLabel,
		* _txtScreen,
		* _txtTitle;
	TextButton* _btnOk;
	TextList
		* _lstLost,
		* _lstGained,
		* _lstSolStats;
	Timer
		* _timerDec,
		* _timerInc;
	Window* _window;

	Base* _base;

	/// Handler for pressing an increase-arrow in the list.
	void lstLeftArrowPress(Action* action);
	/// Handler for releasing an increase-arrow in the list.
	void lstLeftArrowRelease(Action* action);
	/// Handler for pressing a decrease-arrow in the list.
	void lstRightArrowPress(Action* action);
	/// Handler for releasing a decrease-arrow in the list.
	void lstRightArrowRelease(Action* action);

	/// Increases the quantity of an item by one.
	void increase();
	/// Increases the quantity of an item by the given value.
	void increaseByValue(int qtyDelta);
	/// Decreases the quantity of an item by one.
	void decrease();
	/// Decreases the quantity of an item by the given value.
	void decreaseByValue(int qtyDelta);

	/// Updates the buy/sell quantities.
	void update();

	/// Gets the rule for the currently selected item.
	const RuleItem* getRule(const std::map<const RuleItem*, int>& list) const;

	/// Runs the timers.
	void think() override;

	/// Builds the soldier-stat-changes screen.
	void buildSoldierStats();

	/// Formats mapped input to a TextList.
	void styleList(
			const std::map<const RuleItem*, int>& input,
			TextList* const list);


	public:
		/// Creates a DebriefExtra state.
		DebriefExtraState(
				Base* const base,
				std::wstring operation,
				std::map<const RuleItem*, int> itemsLost,
				std::map<const RuleItem*, int> itemsGained,
				std::map<std::wstring, std::vector<int>> solStatDeltas);
		/// Cleans up the DebriefExtra state.
		~DebriefExtraState();

		/// Handler for clicking the OK button.
		void btnOkClick(Action* action);
};

}

#endif
