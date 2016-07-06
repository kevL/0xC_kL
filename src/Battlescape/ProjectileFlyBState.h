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

#ifndef OPENXCOM_PROJECTILEFLYBSTATE_H
#define OPENXCOM_PROJECTILEFLYBSTATE_H

#include "BattleState.h"
//#include "Position.h"

#include "../Ruleset/MapData.h"


namespace OpenXcom
{

class BattleItem;
class BattlescapeGame;
class BattleUnit;
class Projectile;
class SavedBattleGame;
class Tile;


/**
 * A projectile state.
 */
class ProjectileFlyBState
	:
		public BattleState
{

private:
	bool
		_forced,
		_init,
		_targetFloor;
	int _initUnitAni;

	BattleItem
		* _load,
		* _prjItem;
	BattleUnit* _unit;
	Projectile* _prj;
	SavedBattleGame* _battleSave;

	Position
		_posOrigin,
		_originVoxel,
		_targetVoxel,
		_prjVector;
	VoxelType _prjImpact;

	/// Tries to create a projectile.
	bool createProjectile();

	/// Peforms a melee attack.
	void performMeleeAttack();


	public:
		/// Creates a new ProjectileFlyB state.
		ProjectileFlyBState(
				BattlescapeGame* const parent,
				BattleAction action,
				Position origin = Position(0,0,-1));
		/// Cleans up the ProjectileFlyB state.
		~ProjectileFlyBState();

		/// Gets the name of the BattleState.
		std::string getBattleStateLabel() const override;

		/// Initializes the BattleState.
		void init() override;
		/// Handles a cancel request.
		void cancel() override;
		/// Runs BattleState functionality every cycle.
		void think() override;
};

}

#endif
