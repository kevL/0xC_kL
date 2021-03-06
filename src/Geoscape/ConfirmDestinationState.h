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
 * along with OpenXcom. If not, see <http:///www.gnu.org/licenses/>.
 */

#ifndef OPENXCOM_CONFIRMDESTINATIONSTATE_H
#define OPENXCOM_CONFIRMDESTINATIONSTATE_H

#include "../Engine/State.h"


namespace OpenXcom
{

class Craft;
class Target;
class Text;
class TextButton;
class Waypoint;
class Window;


/**
 * Window that allows the player to confirm a Craft's destination.
 */
class ConfirmDestinationState
	:
		public State
{

private:
	Craft* _craft;
	Target* _target;
	Waypoint* _waypoint;

	Text* _txtTarget;
	TextButton
		* _btnCancel,
		* _btnOk;
	Window* _window;


	public:
		/// Creates the Confirm Destination state.
		ConfirmDestinationState(
				Craft* const craft,
				Target* const target);
		/// Cleans up the Confirm Destination state.
		~ConfirmDestinationState();

		/// Handler for clicking the Ok button.
		void btnOkClick(Action* action);
		/// Handler for clicking the Cancel button.
		void btnCancelClick(Action* action);
};

}

#endif
