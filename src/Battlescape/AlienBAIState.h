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
	static const int
		PSI_LOS_WEIGHT     = 52, // the chance for an aLien to do a psi-attack against a target is increased by this amount if it has LoS to that target.
		PSI_SWITCH_TARGET  = 30, // a delta (weighted roll) that can cause an aLien to switch its preferred target (ie, anti-lightning rod device).
		PSI_CUTOFF         = 23, // roughly the minimum pct an aLien needs before it even bothers with a possible psi-attack.
		PSI_OR_BLASTER_PCT = 87, // chance that an aLien will try a psi- or blaster attack (one roll includes both).
		PSI_TRIED_LIMIT    =  1; // how many times each aLien gets to try a psi-attack per turn.

	bool
		_hasBlaster,
		_hasMelee,
		_hasRifle,
		_doGrenade;
	int
		_aggression,
		_distClosest,
		_targetsExposed,
		_targetsVisible,
		_tuAmbush;

	std::vector<size_t> _reachableAttack;
//		_wasHitBy;

	BattleAction
		* _escapeAction,
		* _patrolAction,
		* _ambushAction,
		* _attackAction,
		* _psiAction;

	BattleActionType _reserve;

	/// Sets up an AI_PATROL BattleAction.
	void setupPatrol();
	/// Sets up an AI_COMBAT BattleAction.
	void setupAttack();
	/// Sets up an AI_AMBUSH BattleAction.
	void setupAmbush();
	/// Sets up an AI_ESCAPE BattleAction.
	void setupEscape();

	/// Selects the AI Mode for BattlescapeGame::handleUnitAI().
	void evaluateAiMode();

	/// Counts valid targets Player and neutral.
	int tallyTargets() const;
	/// Counts Player units that spot a position.
	int tallySpotters(const Position& pos) const;

	/// Selects the nearest target seen and returns the quantity of viable targets.
	int selectNearestTarget();
	/// Selects the closest exposed Player unit.
	bool selectPlayerTarget();
	/// Selects an exposed Player or neutral unit.
	bool selectTarget();

	/// Selects a suitable position from which to shoot.
	bool findFirePosition();
	/// Selects the nearest reachable position relative to a target.
	bool findMeleePosition(
			const BattleUnit* const targetUnit,
			int tuCap) const;

	/// Sets up a melee/charge sub-action of AI_COMBAT.
	void meleeAction();
	/// Finishes setting up a melee/charge.
	void faceMelee();
	/// Sets up a launcher sub-action of AI_COMBAT.
	bool wayPointAction();
	/// Constructs a waypoint path for a guided projectile.
	bool pathWaypoints(const BattleUnit* const unit);
	/// Sets up a shot-projectile sub-action of AI_COMBAT.
	void rifleAction();
	/// Selects a fire method for a shot-projectile action.
	void chooseFireMethod();
	/// Sets up a throw-grenade sub-action of AI_COMBAT.
	bool grenadeAction();
	/// Sets up a psionic sub-action of AI_COMBAT.
	bool psiAction();

	/// Checks to ensure that a particular BattleUnit is a valid target.
	bool validTarget(
			const BattleUnit* const unit,
			bool dangerTile = false,
			bool includeCivs = false) const;

	/// Chooses between a melee or ranged attack if both are available.
	void chooseMeleeOrRanged();


	public:
		/// Creates an AlienBAIState for a specified BattleUnit.
		AlienBAIState(
				SavedBattleGame* const battleSave,
				BattleUnit* const unit,
				Node* const startNode = nullptr);
		/// Cleans up the AlienBAIState.
		~AlienBAIState();

		/// Loads the BAI-state from YAML.
		void load(const YAML::Node& node) override;
		/// Saves the BAI-state to YAML.
		YAML::Node save() const override;

		/// Runs state functionality every AI-cycle.
		void thinkAi(BattleAction* const aiAction) override;

		/// Decides if it's okay to create an explosion.
		bool explosiveEfficacy(
				const Position& pos,
				const BattleUnit* const attacker,
				const int radius,
				const int diff) const;
//				bool grenade = false) const;

		/// Gets the TU-reservation setting.
		BattleActionType getReservedAiAction() const;

		/// Enters the state.
//		void enter();
		/// Exits the state.
//		void exit();

		/// Gets the current target-unit.
//		BattleUnit* getTarget();

		///
//		bool getNodeOfBestEfficacy(BattleAction* action);

		/// Sets the "unit was hit" flag true.
//		void setWasHitBy(BattleUnit *attacker);
		/// Gets whether the unit was hit.
//		getWasHitBy(int attacker) const;
};

}

#endif
