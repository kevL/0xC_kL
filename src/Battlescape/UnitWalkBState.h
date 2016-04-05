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

#ifndef OPENXCOM_UNITWALKBSTATE_H
#define OPENXCOM_UNITWALKBSTATE_H

//#include <climits>

#include "BattlescapeGame.h"
#include "BattleState.h"


namespace OpenXcom
{

class BattleUnit;
class Camera;
class Pathfinding;
class SavedBattleGame;
class TileEngine;


/**
 * State for walking units.
 */
class UnitWalkBState
	:
		public BattleState
{

private:
	bool
		_falling,
		_kneelCheck,
		_isVisible,
		_playFly,
		_tileSwitchDone,
		_preStepTurn;
	int
		_preStepCost,
		_dirStart;
	size_t _antecedentOpponents;

	BattleUnit* _unit;
	Camera* _walkCam;
	Pathfinding* _pf;
	SavedBattleGame* _battleSave;
	TileEngine* _te;

	/// Begins unit-walk and may also end unit-walk.
	bool doStatusStand();
	/// Continues unit-walk.
	bool doStatusWalk();
	/// Ends unit-walk.
	bool doStatusStand_end();
	/// Swivels unit.
	void doStatusTurn();

	/// Resets the unit-cache, aborts the path and the State.
	void abortState(bool recache = true);

	/// Handles some calculations when the path is finished.
	void postPathProcedures();
	/// Gets a suitable final facing direction for aLiens.
	int getFinalDirection() const;

	/// Checks visibility against new opponents.
	bool visForUnits() const;

	/// Sets animation speed for the unit.
	void setWalkSpeed(bool gravLift) const;

	/// Handles the stepping sounds.
	void playMoveSound();

	/// For determining if a flying-unit turns flight off at start of movement.
	void doFallCheck();
	/// Checks if there is ground below when unit is falling.
	bool groundCheck() const;

	/// Establishes unit's transient link(s) to its destination Tile(s).
	void establishTilesLink() const;
	/// Clears unit's transient link(s) to other Tile(s).
	void clearTilesLink(bool origin) const;


	public:
		/// Creates a UnitWalkBState class.
		UnitWalkBState(
				BattlescapeGame* const parent,
				BattleAction _action);
		/// Cleans up the UnitWalkBState.
		~UnitWalkBState();

		/// Initializes the state.
		void init() override;
		/// Runs state functionality every cycle.
		void think() override;
		/// Handles a cancel request.
		void cancel() override;
};

}

#endif
