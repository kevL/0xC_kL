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

#ifndef OPENXCOM_MONTHLYCOSTSSTATE_H
#define OPENXCOM_MONTHLYCOSTSSTATE_H

#include "../Engine/State.h"


namespace OpenXcom
{

class Base;
class Text;
class TextButton;
class TextList;
class Window;


/**
 * A screen that displays all the maintenance costs of a particular Base.
 */
class MonthlyCostsState
	:
		public State
{

private:
	Text
		* _txtUnitCost,
		* _txtIncome,
		* _txtCost,
		* _txtQuantity,
		* _txtRental,
		* _txtSalaries,
		* _txtTitle;
	TextButton* _btnOk;
	TextList
		* _lstBaseCost,
		* _lstCrafts,
		* _lstMaintenance,
		* _lstSalaries,
		* _lstTotal;
	Window *_window;


	public:
		/// Creates a MonthlyCosts state.
		explicit MonthlyCostsState(Base* base);
		/// Cleans up the MonthlyCosts state.
		~MonthlyCostsState();

		/// Handler for clicking the Ok button.
		void btnOkClick(Action* action);
};

}

#endif
