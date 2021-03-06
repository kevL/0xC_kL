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

#ifndef OPENXCOM_CRAFTERRORSTATE_H
#define OPENXCOM_CRAFTERRORSTATE_H

//#include <string>

#include "../Engine/State.h"


namespace OpenXcom
{

class GeoscapeState;
class Text;
class TextButton;
class Window;


/**
 * Window used to notify the player when an error occurs with a craft procedure.
 */
class CraftErrorState
	:
		public State
{
private:
	GeoscapeState* const _geoState;
	Text* _txtMessage;
	TextButton
		* _btnOk,
		* _btnOk5Secs;
	Window* _window;


	public:
		/// Creates a CraftError state.
		CraftErrorState(
				GeoscapeState* const geoState,
				const std::wstring& wst);
		/// Cleans up the CraftError state.
		~CraftErrorState();

		/// Initializes the state.
		void init() override;

		/// Handler for clicking the Ok button.
		void btnOkClick(Action* action);
		/// Handler for clicking the OK 5 Secs button.
		void btnOk5SecsClick(Action* action);
};

}

#endif
