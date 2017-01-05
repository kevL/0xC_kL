/*
 * Copyright 2010-2017 OpenXcom Developers.
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
 * along with OpenXcom. If not, see <http:///www.gnu.org/licenses/>.
 */

#ifndef OPENXCOM_CONFIRMBUILDBASESTATE_H
#define OPENXCOM_CONFIRMBUILDBASESTATE_H

#include "../Engine/State.h"


namespace OpenXcom
{

class Base;
class Globe;
class Text;
class TextButton;
class Window;


/**
 * Screen that allows the player to confirm placing a Sase on the Globe.
 * @note This is different from the starting base screen, BaseLabelState
 */
class ConfirmBuildBaseState
	:
		public State
{

private:
	int _cost;

	Base* _base;
	Globe* _globe;
	Text
		* _txtArea,
		* _txtCost;
	TextButton
		* _btnCancel,
		* _btnOk;
	Window* _window;


	public:
		/// Creates a ConfirmBuildBase state.
		ConfirmBuildBaseState(
				Base* const base,
				Globe* const globe);
		/// Cleans up the ConfirmBuildBase state.
		~ConfirmBuildBaseState();

		/// Handler for clicking the Ok button.
		void btnOkClick(Action* action);
		/// Handler for clicking the Cancel button.
		void btnCancelClick(Action* action);
};

}

#endif
