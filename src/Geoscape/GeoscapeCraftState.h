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

#ifndef OPENXCOM_GEOSCAPECRAFTSTATE_H
#define OPENXCOM_GEOSCAPECRAFTSTATE_H

#include "../Engine/State.h"


namespace OpenXcom
{

class Craft;
class GeoscapeState;
class Text;
class TextButton;
class Waypoint;
class Window;


/**
 * Craft window that displays info about a specific craft out on the Geoscape.
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
		* _txtDamage,
		* _txtFuel,
		* _txtHWP,
		* _txtMaxSpeed,
		* _txtRedirect,
		* _txtSoldier,
		* _txtSpeed,
		* _txtStatus,
		* _txtTitle,
		* _txtW1Ammo,
		* _txtW1Name,
		* _txtW2Ammo,
		* _txtW2Name,
		* _txtKills;
	TextButton
		* _btnRebase,
		* _btnCancel,
		* _btnCenter,
		* _btnPatrol,
		* _btnTarget;
	Waypoint* _waypoint;
	Window* _window;

	/// Hides various screen-elements to reveal the globe & Craft.
	void transposeWindow();
	/// Applies the targeter-graphic to the last-known UFO coordinates.
	void targeter();


	public:
		/// Creates the Geoscape Craft state.
		GeoscapeCraftState(
				Craft* const craft,
				GeoscapeState* const geoState,
				Waypoint* const waypoint = nullptr,
				bool doublePop = false,
				bool transpose = false);
		/// Cleans up the Geoscape Craft state.
		~GeoscapeCraftState();

		/// Handler for clicking the Center button.
		void btnCenterClick(Action* action);
		/// Handler for clicking the Return To Base button.
		void btnBaseClick(Action* action);
		/// Handler for clicking the Select New Target button.
		void btnTargetClick(Action* action);
		/// Handler for clicking the Patrol button.
		void btnPatrolClick(Action* action);
		/// Handler for clicking the Cancel button.
		void btnCancelClick(Action* action);
};

}

#endif
