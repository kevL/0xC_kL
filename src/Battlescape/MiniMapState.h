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

#ifndef OPENXCOM_MINIMAP_H
#define OPENXCOM_MINIMAP_H

#include "../Engine/State.h"


namespace OpenXcom
{

class BattlescapeButton;
class MiniMapView;
class Text;
class Timer;


/**
 * The MiniMap is an overhead representation of the battlefield, a strategic
 * view that allows the player to see more of the Map.
 */
class MiniMapState
	:
		public State
{

private:
	BattlescapeButton
		* _btnLvlDown,
		* _btnLvlUp,
		* _btnOk;
	MiniMapView* _miniView;
	Surface* _bgScanbord;
	Text* _txtLevel;
	Timer* _timerAnimate;

	/// Handles Minimap animation.
	void animate();


	public:
		/// Creates a MiniMap state.
		MiniMapState();
		/// Cleans up the MiniMap state.
		~MiniMapState();

		/// Handler for the Ok button.
		void btnOkClick(Action* action);
		/// Handler for the one level up button.
		void btnLevelUpClick(Action* action);
		/// Handler for the one level down button.
		void btnLevelDownClick(Action* action);
		/// Handler for centering on the selected unit.
		void keyCenterUnitPress(Action* action);

		/// Handler for right-clicking anything.
		void handle(Action* action) override;

		/// Handles timers.
		void think() override;
};

}

#endif
