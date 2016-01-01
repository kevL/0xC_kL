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

#ifndef OPENXCOM_NEWPOSSIBLEMANUFACTURESTATE
#define OPENXCOM_NEWPOSSIBLEMANUFACTURESTATE

#include "../Engine/State.h"


namespace OpenXcom
{

class Base;
class RuleManufacture;
class Text;
class TextButton;
class TextList;
class Window;


/**
 * Window which inform the player of new possible manufacture projects.
 * @note Also lets the player to go to ManufactureState to dispatch engineers.
 */
class NewPossibleManufactureState
	:
		public State
{

private:
	Base* _base;
	Text* _txtTitle;
	TextButton
		* _btnManufacture,
		* _btnOk;
	TextList* _lstPossibilities;
	Window* _window;


	public:
		/// Creates the NewPossibleManufacture state.
		NewPossibleManufactureState(
				Base* const base,
				const std::vector<const RuleManufacture*>& possibilities,
				bool showManufactureButton);
		// Deconstructs the NewPossibleManufactureState.
		~NewPossibleManufactureState();

		/// Handler for clicking the OK button.
		void btnOkClick(Action* action);
		/// Handler for clicking the Allocate Manufacture button.
		void btnManufactureClick(Action* action);
};

}

#endif
