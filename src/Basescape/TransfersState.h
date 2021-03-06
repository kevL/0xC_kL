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

#ifndef OPENXCOM_TRANSFERSSTATE_H
#define OPENXCOM_TRANSFERSSTATE_H

#include "../Engine/State.h"


namespace OpenXcom
{

class Base;
class Text;
class TextButton;
class TextList;
class Window;


/**
 * Transfers window that displays all the items currently in-transit to a Base.
 */
class TransfersState
	:
		public State
{

private:
	Base* _base;
	Text
		* _txtArrivalTime,
		* _txtBaseLabel,
		* _txtItem,
		* _txtQuantity,
		* _txtTitle;
	TextButton* _btnOk;
	TextList* _lstTransfers;
	Window* _window;


	public:
		/// Creates a Transfers state.
		explicit TransfersState(Base* const base);
		/// Cleans up the Transfers state.
		~TransfersState();

		/// Handler for clicking the Ok button.
		void btnOkClick(Action* action);
};

}

#endif
