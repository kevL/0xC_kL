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

#ifndef OPENXCOM_CRAFTPATROLSTATE_H
#define OPENXCOM_CRAFTPATROLSTATE_H

#include "../Engine/State.h"


namespace OpenXcom
{

class Craft;
class GeoscapeState;
class Text;
class TextButton;
class Window;


/**
 * Window displayed when a craft starts patrolling at a waypoint.
 */
class CraftPatrolState
	:
		public State
{

private:
	bool _delayPop;

	Craft* _craft;
	GeoscapeState* _geoState;
	Surface
		* _sprite,
		* _srfTarget;
	Text* _txtDestination;
//		* _txtPatrolling;
	TextButton
		* _btn5s,
		* _btnBase,
		* _btnCenter,
		* _btnInfo,
		* _btnOk,
		* _btnRedirect;
	Window* _window;

	/// Hides various screen-elements to reveal the globe & Craft.
	void transposeWindow();


	public:
		/// Creates the Geoscape CraftPatrol state.
		CraftPatrolState(
				Craft* const craft,
				GeoscapeState* const geoState);
		/// Cleans up the Geoscape CraftPatrol state.
		~CraftPatrolState();

		/// Initializes the state.
		void init() override;

		/// Handler for clicking the Ok button.
		void btnOkClick(Action* action);
		/// Handler for clicking the Ok button.
		void btn5sClick(Action* action);
		/// Handler for clicking the Craft Info button.
		void btnInfoClick(Action* action);
		/// Handler for clicking the Return To Base button.
		void btnBaseClick(Action* action);
		/// Handler for clicking the Center button.
		void btnCenterClick(Action* action);
		/// Handler for clicking the Redirect Craft button.
		void btnRedirectClick(Action* action);
};

}

#endif
