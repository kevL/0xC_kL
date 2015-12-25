/*
 * Copyright 2010-2015 OpenXcom Developers.
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

#ifndef OPENXCOM_ALIENBAISTATE_H
#define OPENXCOM_ALIENBAISTATE_H

//#include <vector>

#include "BattleAIState.h"

#include "../Battlescape/BattlescapeGame.h"


namespace OpenXcom
{

/**
 * This class is used by the BattleUnit AI.
 */
class AlienBAIState
	:
		public BattleAIState
{

private:
	bool
		_blaster,
		_didPsi,
		_grenade, // kL
		_melee,
		_rifle;
	int
		_closestDist,
		_intelligence,
		_targetsExposed,
		_targetsVisible,
		_spottersHostile,
		_tuAmbush,
		_tuEscape;
//kL	_reserveTUs;

	std::vector<int>
		_reachable,
		_reachableAttack;
//		_wasHitBy;

	BattleAction
		* _ambushAction,
		* _attackAction,
		* _escapeAction,
		* _patrolAction,
		* _psiAction;

	BattleActionType _reserve;

	/// Sets up a patrol objective.
	void setupPatrol();
	/// Sets up an ambush objective.
	void setupAmbush();
	/// Sets up a combat objective.
	void setupAttack();
	/// Sets up an escape objective.
	void setupEscape();

	/// Counts how many xcom/civilian units are known to this unit.
	int countExposedTargets() const;
	/// Counts how many known XCom units are able to see this unit.
	int countSpotters(const Position& pos) const;

	/// Selects the nearest target we can see, and return the number of viable targets.
	int selectNearestTarget();
	/// Selects the closest known xcom unit for ambushing.
	bool selectClosestKnownEnemy();
	/// Selects a random known target.
	bool selectRandomTarget();
	/// Selects the nearest reachable point relative to a target.
	bool selectPositionNearTarget(
			const BattleUnit* const targetUnit,
			int tuMax) const;

	/// Re-evaluates the situation and makes a decision from available options.
	void evaluateAIMode();
	/// Selects a suitable position from which to attack.
	bool findFirePoint();

	/// Attempts to take a melee attack/charge an enemy we can see.
	void meleeAction();
	/// Performs a melee attack action.
	void meleeAttack();
	/// Attempts to fire a waypoint projectile at an enemy we, or one of our teammates sees.
	void wayPointAction();
	/// Constructs a waypoint path for weapons like Blaster Launcher.
	bool pathWaypoints();
	/// Attempts to fire at an enemy we can see.
	void projectileAction();
	/// Selects a fire method.
	void selectFireMethod();
	/// Attempts to throw a grenade at an enemy (or group of enemies) we can see.
	void grenadeAction();
	/// Performs a psionic attack.
	bool psiAction();

	/// Checks to make sure a target is valid given the parameters.
	bool validTarget(
			const BattleUnit* const unit,
			bool assessDanger = false,
			bool includeCivs = false) const;

	/// Assumes the aLien has both a ranged and a melee weapon, and selects one.
	void selectMeleeOrRanged();


	public:
		/// Creates a new AlienBAIState linked to the game and a certain unit.
		AlienBAIState(
				SavedBattleGame* const battleSave,
				BattleUnit* const unit,
				Node* const node = nullptr);
		/// Cleans up the AlienBAIState.
		~AlienBAIState();

		/// Loads the AI state from YAML.
		void load(const YAML::Node& node);
		/// Saves the AI state to YAML.
		YAML::Node save() const override;

		/// Enters the state.
//		void enter();
		/// Exits the state.
//		void exit();
		/// Runs state functionality every AI cycle.
		void think(BattleAction* const action) override;

		/// Sets the "unit was hit" flag true.
//		void setWasHitBy(BattleUnit *attacker);
		/// Gets whether the unit was hit.
//		getWasHitBy(int attacker) const;

		/// Decides if we should throw a grenade/launch a missile to this position.
		bool explosiveEfficacy(
				const Position& posTarget,
				const BattleUnit* const attacker,
				const int explRadius,
				const int diff) const;
//				bool grenade = false) const;
//		bool getNodeOfBestEfficacy(BattleAction* action);

		/// Checks the alien's TU reservation setting.
		BattleActionType getReservedAIAction() const;

		/// Gets the current target-unit.
//		BattleUnit* getTarget();
};

}

#endif
