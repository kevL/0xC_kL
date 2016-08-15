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

#ifndef OPENXCOM_EXPLOSIONBSTATE_H
#define OPENXCOM_EXPLOSIONBSTATE_H

#include "BattleState.h"
#include "Position.h"


namespace OpenXcom
{

class BattlescapeGame;
class BattleUnit;
class RuleItem;
class SavedBattleGame;
class Tile;


/**
 * Explosion state not only handles explosions, but also bullet impacts!
 * Refactoring tip : ImpactBState.
 */
class ExplosionBState
	:
		public BattleState
{

private:
	bool
		_areaOfEffect,
		_buttHurt,
		_forceCamera,
		_isLaunched,
		_lowerWeapon,
		_melee,
		_meleeSuccess;
	int _power;
//		_extend,

	BattleUnit* _unit;
	const RuleItem* _itRule;
	SavedBattleGame* _battleSave;
	Tile* _tile;

	const Position _centerVoxel;


	/// Calculates the effects of the explosion.
	void explode();


	public:
		/// Creates an ExplosionBState object.
		ExplosionBState(
				BattlescapeGame* const parent,
				const Position centerVoxel,
				const RuleItem* const itRule,
				BattleUnit* const unit,
				Tile* const tile = nullptr,
				bool lowerWeapon = false,
				bool meleeSuccess = false,
				bool forceCamera = false,
				bool isLaunched = false);
		/// Cleans up the ExplosionBState.
		~ExplosionBState();

		/// Gets the label of the BattleState.
		std::string getBattleStateLabel() const override;

		/// Initializes the BattleState.
		void init() override;
		/// Runs BattleState functionality every cycle.
		void think() override;
		/// Handles a cancel request.
//		void cancel();
};

}

#endif
