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

#ifndef OPENXCOM_MANUFACTURECOMPLETESTATE_H
#define OPENXCOM_MANUFACTURECOMPLETESTATE_H

//#include <string>

#include "../Engine/State.h"

#include "../Savegame/ManufactureProject.h"


namespace OpenXcom
{

class Base;
class GeoscapeState;

class Text;
class TextButton;
class Window;


/**
 * Window used to notify the player when a Manufacture project is completed.
 */
class ManufactureCompleteState
	:
		public State
{

private:
	Base* _base;
	GeoscapeState* _geoState;

	Text* _txtMessage;
	TextButton
		* _btnBase,
		* _btnOk,
		* _btnOk5Secs;
	Window* _window;


	public:
		/// Creates a ManufactureComplete state.
		ManufactureCompleteState(
				Base* const base,
				const std::wstring& item,
				GeoscapeState* const geoState,
				bool allocate,
				ManufactureProgress doneType);
		/// Cleans up the ManufactureComplete state.
		~ManufactureCompleteState();

		/// Initializes the state.
		void init() override;

		/// Handler for clicking the Ok button.
		void btnOkClick(Action* action);
		/// Handler for clicking the Ok 5sec button.
		void btnOk5SecsClick(Action* action);
		/// Handler for clicking the Go To Base button.
		void btnGotoBaseClick(Action* action);
};

}

#endif
