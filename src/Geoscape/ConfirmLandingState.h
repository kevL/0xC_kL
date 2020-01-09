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

#ifndef OPENXCOM_CONFIRMLANDINGSTATE_H
#define OPENXCOM_CONFIRMLANDINGSTATE_H

#include "../Engine/State.h"


namespace OpenXcom
{

class Craft;
class RuleTerrain;
class RuleTexture;
class Text;
class TextButton;
class Window;


/**
 * Window that asks the player to confirm a Craft landing at its destination.
 */
class ConfirmLandingState
	:
		public State
{

private:
	int _shade;

	Craft* _craft;
	RuleTerrain* _terrainRule;
	Text
		* _txtBegin,
		* _txtMessage,
		* _txtMessage2,

		* _txtBase,
		* _txtShade,
		* _txtTexture;
	TextButton
		* _btnPatrol,
		* _btnYes;
	Window* _window;


	public:
		/// Creates a ConfirmLanding state.
		ConfirmLandingState(
				Craft* const craft,
				const RuleTexture* const texRule = nullptr,
				const int shade = -1,
				const bool allowTactical = true);
		/// Cleans up the ConfirmLanding state.
		~ConfirmLandingState();

		/// Initializes the State and makes a sanity check.
		void init() override;

		/// Handler for clicking the Yes button.
		void btnYesClick(Action* action);
		/// Handler for clicking the Patrol button.
		void btnPatrolClick(Action* action);
		/// Handler for clicking the Intercept button.
		void btnInterceptClick(Action* action);
		/// Handler for clicking the Return To Base button.
		void btnBaseClick(Action* action);

		/// Selects a terrain-type for crashed or landed UFOs.
//		RuleTerrain* selectTerrain(const double lat);
		/// Selects a terrain-type for missions at cities.
//		RuleTerrain* selectCityTerrain(const double lat);
};

}

#endif
