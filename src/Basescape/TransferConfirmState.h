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

#ifndef OPENXCOM_TRANSFERCONFIRMSTATE_H
#define OPENXCOM_TRANSFERCONFIRMSTATE_H

#include "../Engine/State.h"


namespace OpenXcom
{

class Base;
class Text;
class TextButton;
class TransferItemsState;
class Window;


/**
 * Window to confirm a transfer between bases.
 */
class TransferConfirmState
	:
		public State
{

private:
	Base* _base;
	Text
		* _txtTitle,
		* _txtCost,
		* _txtTotal;
	TextButton
		* _btnCancel,
		* _btnOk;
	TransferItemsState* _state;
	Window* _window;


	public:
		/// Creates the Transfer Confirm state.
		TransferConfirmState(
				Base* base,
				TransferItemsState* state);
		/// Cleans up the Transfer Confirm state.
		~TransferConfirmState();

		/// Handler for clicking the Cancel button.
		void btnCancelClick(Action* action);
		/// Handler for clicking the Ok button.
		void btnOkClick(Action* action);
};

}

#endif
