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

#ifndef OPENXCOM_DERANKSOLDIERSTATE_H
#define OPENXCOM_DERANKSOLDIERSTATE_H

#include "../Engine/State.h"


namespace OpenXcom
{

class Base;
class Text;
class TextButton;
class Window;

/**
 * Window shown when the player wants to demote a Soldier.
 */
class DerankSoldierState
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
		/// Creates a DerankSoldier state.
		DerankSoldierState(
				Base* const base,
				size_t solId,
				bool isPlayerError);
		/// Cleans up the DerankSoldier state.
		~DerankSoldierState();

		/// Handler for clicking the Ok button.
		void btnOkClick(Action* action);
		/// Handler for clicking the Cancel button.
		void btnCancelClick(Action* action);
};

}

#endif
