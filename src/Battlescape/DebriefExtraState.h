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
		YELLOW	= 138,
		PURPLE	= 176, // needs high contrast. Still not right ...
		GREEN	= 239;

	std::map<const RuleItem*, int>
		_itemsGained,
		_itemsLost;

	Text
		* _txtBaseLabel,
		* _txtTitle;
	TextButton* _btnOk;
	TextList
		* _lstLost,
		* _lstGained,
		* _lstSolStats;
	Window* _window;

	/// Builds the Soldier Stat increases screen.
	void buildSoldierStats();

	/// Adds a row-entry to a TextList.
	void styleList(
			const std::map<const RuleItem*, int>& input,
			TextList* const list);


	public:
		/// Creates the DebriefExtra state.
		DebriefExtraState(
				const Base* const base,
				std::wstring operation,
				std::map<const RuleItem*, int> itemsLost,
				std::map<const RuleItem*, int> itemsGained);
		/// Cleans up the DebriefExtra state.
		~DebriefExtraState();

		/// Handler for clicking the OK button.
		void btnOkClick(Action* action);
};

}

#endif
