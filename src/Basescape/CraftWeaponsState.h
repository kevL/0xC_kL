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

#ifndef OPENXCOM_CRAFTWEAPONSSTATE_H
#define OPENXCOM_CRAFTWEAPONSSTATE_H

//#include <vector>

#include "../Engine/State.h"


namespace OpenXcom
{

class Base;
class Craft;
class RuleCraftWeapon;
class Text;
class TextButton;
class TextList;
class Window;


/**
 * Selection window for changing the weapon equipped on a Craft.
 */
class CraftWeaponsState
	:
		public State
{

private:
	Base* _base;
	Craft* _craft;
	Text
		* _txtAmmunition,
		* _txtArmament,
		* _txtQuantity,
		* _txtTitle;
	TextButton* _btnCancel;
	TextList* _lstWeapons;
	Window* _window;

	size_t _pod;

	std::vector<const RuleCraftWeapon*> _cwRules;


	public:
		/// Creates a CraftWeapons state.
		CraftWeaponsState(
				Base* const base,
				size_t craftId,
				size_t pod);
		/// Cleans up the CraftWeapons state.
		~CraftWeaponsState();

		/// Handler for clicking the Cancel button.
		void btnCancelClick(Action* action);
		/// Handler for clicking the CraftWeapons list.
		void lstWeaponsClick(Action* action);
};

}

#endif
