/*
 * Copyright 2010-2019 OpenXcom Developers.
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

#ifndef OPENXCOM_ALIENBASEDETECTEDSTATE_H
#define OPENXCOM_ALIENBASEDETECTEDSTATE_H

#include "../Engine/State.h"


namespace OpenXcom
{

class AlienBase;
class GeoscapeState;
class Text;
class TextButton;
class Window;


/**
 * Displays text to the effect that an aLien-base has been detected.
 */
class AlienBaseDetectedState
	:
		public State
{

private:
	AlienBase* _alienBase;
	GeoscapeState* _geoState;
	Text* _txtTitle;
	TextButton
		* _btnCenter,
		* _btnOk;
	Window* _window;


	public:
		/// Creates the AlienBaseDetected state.
		AlienBaseDetectedState(
				AlienBase* const alienBase,
				GeoscapeState* const geoState,
				bool recon = true);
		/// Cleans up the AlienBaseDetected state.
		~AlienBaseDetectedState();

		/// Handler for clicking the Ok button.
		void btnOkClick(Action* action);
		/// Handler for clicking the Center button.
		void btnCenterClick(Action* action);
};

}

#endif
