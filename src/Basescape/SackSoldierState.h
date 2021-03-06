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

#ifndef OPENXCOM_SACKSOLDIERSTATE_H
#define OPENXCOM_SACKSOLDIERSTATE_H

#include "../Engine/State.h"


namespace OpenXcom
{

class Base;
class Text;
class TextButton;
class Window;

/**
 * Window shown when the player wants to sack a Soldier.
 */
class SackSoldierState
	:
		public State
{

private:
	size_t _solId;

	Base* _base;

	Text
		* _txtSoldier,
		* _txtTitle;
	TextButton
		* _btnCancel,
		* _btnOk;
	Window* _window;


	public:
		/// Creates a SackSoldier state.
		SackSoldierState(
				Base* const base,
				size_t solId);
		/// Cleans up the SackSoldier state.
		~SackSoldierState();

		/// Handler for clicking the Ok button.
		void btnOkClick(Action* action);
		/// Handler for clicking the Cancel button.
		void btnCancelClick(Action* action);
};

}

#endif
