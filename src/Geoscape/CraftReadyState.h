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

#ifndef OPENXCOM_CRAFTREADYSTATE_H
#define OPENXCOM_CRAFTREADYSTATE_H

//#include <string>

#include "../Engine/State.h"


namespace OpenXcom
{

class Craft;
class GeoscapeState;

class Text;
class TextButton;
class Window;


/**
 * Window used to notify the player when a Craft is ready for take-off.
 */
class CraftReadyState
	:
		public State
{

private:
	Craft* _craft;
	GeoscapeState* _geoState;

	Text* _txtMessage;
	TextButton
		* _btnCraftInfo,
		* _btnBase,
		* _btnOk,
		* _btnOk5Secs;
	Window* _window;


	public:
		/// Constructs the CraftReady state.
		CraftReadyState(
				GeoscapeState* const geoState,
				Craft* const craft,
				const std::wstring& wst);
		/// Cleans up the CraftReady state.
		~CraftReadyState();

		/// Initializes the state.
		void init() override;

		/// Handler for clicking the OK button.
		void btnOkClick(Action* action);
		/// Handler for clicking the OK 5 Secs button.
		void btnOk5SecsClick(Action* action);
		/// Handler for clicking the GoToBase button.
		void btnGoToBaseClick(Action* action);
		/// Handler for clicking the CraftInfo button.
		void btnCraftInfoClick(Action* action);
};

}

#endif
