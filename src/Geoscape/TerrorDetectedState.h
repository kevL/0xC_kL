/*
 * Copyright 2010-2018 OpenXcom Developers.
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

#ifndef OPENXCOM_TERRORDETECTEDSTATE_H
#define OPENXCOM_TERRORDETECTEDSTATE_H

#include "../Engine/State.h"


namespace OpenXcom
{

class GeoscapeState;
class TerrorSite;
class Text;
class TextButton;
class Window;


/**
 * Displays info on a detected terror-site.
 */
class TerrorDetectedState
	:
		public State
{

private:

	GeoscapeState* _geoState;
	const TerrorSite* _terrorSite;
	TextButton
		* _btnCancel,
		* _btnCenter,
		* _btnIntercept;
	Text
		* _txtCity,
		* _txtTitle;
	Window* _window;


	public:

		/// Creates a TerrorDetected state.
		TerrorDetectedState(
				const TerrorSite* const terrorSite,
				GeoscapeState* const geoState);
		/// Cleans up the TerrorDetected state.
		~TerrorDetectedState();

		/// Handler for clicking the Intercept button.
		void btnInterceptClick(Action* action);
		/// Handler for clicking the Center on Site button.
		void btnCenterClick(Action* action);
		/// Handler for clicking the Cancel button.
		void btnCancelClick(Action* action);
};

}

#endif
