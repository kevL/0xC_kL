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

#ifndef OPENXCOM_CIVILIANBAISTATE_H
#define OPENXCOM_CIVILIANBAISTATE_H

//#include <yaml-cpp/yaml.h>

#include "BattleAIState.h"


namespace OpenXcom
{

/**
 * This is the initial AI state of civilian BattleUnits walking around and
 * looking to get killed.
 */
class CivilianBAIState
	:
		public BattleAIState
{

private:
	int _targetsHostile;

	BattleAction
		* _escapeAction,
		* _patrolAction;

	/// Counts how many aLiens spot this unit.
	int tallySpotters(const Position& pos) const;
	/// Counts the quantity of Hostiles that the civilian sees.
	int tallyHostiles();

	/// Sets up an escape objective.
	void setupEscape();
	/// Sets up a patrol objective.
	void setupPatrol();

	/// Re-evaluates the situation and makes a decision from available options.
	void evaluateAIMode();


	public:
		/// Creates a new BattleAIState linked to the game and a specific unit.
		CivilianBAIState(
				SavedBattleGame* const battleSave,
				BattleUnit* const unit,
				Node* const startNode = nullptr);
		/// Cleans up the BattleAIState.
		~CivilianBAIState();

		/// Loads the AI state from YAML.
		void load(const YAML::Node& node) override;
		/// Saves the AI state to YAML.
		YAML::Node save() const override;

		/// Enters the state.
//		void enter();
		/// Exits the state.
//		void exit();

		/// Runs state functionality every AI cycle.
		void think(BattleAction* const action) override;
};

}

#endif
