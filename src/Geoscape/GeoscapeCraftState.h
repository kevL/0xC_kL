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

#ifndef OPENXCOM_GEOSCAPECRAFTSTATE_H
#define OPENXCOM_GEOSCAPECRAFTSTATE_H

#include "../Engine/State.h"


namespace OpenXcom
{

class Craft;
class GeoscapeState;
class Text;
class TextButton;
class Ufo;
class Waypoint;
class Window;


/**
 * State that displays info about a specific Craft on the Geoscape.
 */
class GeoscapeCraftState
	:
		public State
{

private:
	bool
		_delayPop,
		_doublePop;

	Craft* _craft;
	GeoscapeState* _geoState;
	Surface
		* _sprite,
		* _srfTarget;
	Text
		* _txtAltitude,
		* _txtBase,
		* _txtFuel,
		* _txtHull,
		* _txtHwp,
		* _txtMaxSpeed,
		* _txtRedirect,
		* _txtSoldier,
		* _txtSpeed,
		* _txtStatus,
		* _txtTitle,
		* _txtLoad1,
		* _txtWeapon1,
		* _txtLoad2,
		* _txtWeapon2,
		* _txtKills;
	TextButton
		* _btnRebase,
		* _btnCancel,
		* _btnCenter,
		* _btnPatrol,
		* _btnTarget,
		* _btnIntercept;
	Ufo* _ufo;
	Waypoint* _waypoint;
	Window* _window;

	/// Handler for clicking the Center button.
	void btnCenterPauseClick(Action* action);
	/// Handler for clicking the Return To Base button.
	void btnBaseClick(Action* action);
	/// Handler for clicking the Select New Target button.
	void btnTargetClick(Action* action);
	/// Handler for clicking the Patrol button.
	void btnPatrolClick(Action* action);
	/// Handler for clicking the Cancel button.
	void btnCancelOrRedirectClick(Action* action);
	/// Handler for clicking the Intercept button.
	void btnInterceptClick(Action* action);

	/// Hides various screen-elements to reveal the globe & Craft.
	void transposeWindow();
	/// Applies the Globe's targeter-graphic to the last-known UFO coordinates.
	void targeter();


	public:
		/// Creates a GeoscapeCraft state.
		GeoscapeCraftState(
				Craft* const craft,
				GeoscapeState* const geoState,
				Waypoint* const waypoint = nullptr,
				bool doublePop = false,
				bool transpose = false,
				Ufo* const ufo = nullptr);
		/// Cleans up the GeoscapeCraft state.
		~GeoscapeCraftState();
};

}

#endif
