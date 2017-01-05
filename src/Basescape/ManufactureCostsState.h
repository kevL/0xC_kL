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
 * along with OpenXcom. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef OPENXCOM_MANUFACTURECOSTSSTATE_H
#define OPENXCOM_MANUFACTURECOSTSSTATE_H

#include "../Engine/State.h"


namespace OpenXcom
{

class Text;
class TextButton;
class TextList;
class Window;


/**
 * Screen that displays a table of manufacturing costs.
 */
class ManufactureCostsState
	:
		public State
{

private:
	static const Uint8
		GREEN	=  48u,
		BROWN	=  80u,
		YELLOW	= 213u;

	Text
		* _txtTitle,
		* _txtItem,
		* _txtManHours,
		* _txtSpace,
		* _txtCost,
		* _txtRequired;
	TextButton* _btnCancel;
	TextList* _lstProduction;
	Window* _window;


	public:
		/// Creates a ManufactureCosts state.
		ManufactureCostsState();
		/// dTor.
		~ManufactureCostsState();

		/// Populates the table with Manufacture info.
		void init() override;

		/// Handler for the Cancel button.
		void btnCancelClick(Action* action);
};

}

#endif
