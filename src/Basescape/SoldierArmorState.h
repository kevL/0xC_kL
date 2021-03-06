/*
 * Copyright 2010-2020 OpenXcom Developers.
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

#ifndef OPENXCOM_SOLDIERARMORSTATE_H
#define OPENXCOM_SOLDIERARMORSTATE_H

//#include <vector>

#include "../Engine/State.h"


namespace OpenXcom
{

class Base;
class RuleArmor;
class Soldier;
class Text;
class TextButton;
class TextList;
class Window;


/**
 * SoldierArmor window that allows changing the Armor of a Soldier.
 */
class SoldierArmorState
	:
		public State
{

private:
	Base* _base;
	Soldier* _sol;
	Text
		* _txtQuantity,
		* _txtSoldier,
		* _txtType;
	TextButton* _btnCancel;
	TextList* _lstArmor;
	Window* _window;

	std::vector<RuleArmor*> _armors;


	public:
		/// Creates a SoldierArmor state.
		SoldierArmorState(
				Base* const base,
				size_t solId);
		/// Cleans up the SoldierArmor state.
		~SoldierArmorState();

		/// Handler for clicking the Cancel button.
		void btnCancelClick(Action* action);
		/// Handler for clicking the Weapons list.
		void lstArmorClick(Action* action);
};

}

#endif
