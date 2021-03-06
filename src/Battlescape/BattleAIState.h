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

#ifndef OPENXCOM_BATTLEAISTATE_H
#define OPENXCOM_BATTLEAISTATE_H

#include <yaml-cpp/yaml.h>

#include "Pathfinding.h"
#include "Position.h"
#include "TileEngine.h"

#include "../Engine/RNG.h"

#include "../Savegame/BattleUnit.h"
#include "../Savegame/Node.h"
#include "../Savegame/SavedBattleGame.h"
#include "../Savegame/Tile.h"


namespace OpenXcom
{

class BattleUnit;
class Node;
class SavedBattleGame;

struct BattleAction;


enum AIMode
{
	AI_PATROL,	// 0
	AI_AMBUSH,	// 1
	AI_COMBAT,	// 2
	AI_ESCAPE	// 3
};


/**
 * This class is used by the BattleUnit AI.
 */
class BattleAIState
{

protected:
	static const int
		FAST_PASS_THRESHOLD     = 100,
		BASE_SUCCESS_SYSTEMATIC = 100,
		BASE_SUCCESS_DESPERATE  = 115,
		BASE_SUCCESS            =  73,
		EXPOSURE_PENALTY        =  12,
		FIRE_PENALTY            =  37,
		COVER_BONUS             =  29,
		SMOKE_BONUS             =   3,
		CUR_TILE_PREF           =  10,

		ESCAPE_FAIL             = -100000,

		TRACE_DIR               = 10,

		CAP_DIST                = 1000,
		CAP_DIST_SQR            = 1000000;

	static const Uint8
		TRACE_RED    =  3u,
		TRACE_GREEN  =  4u,
		TRACE_LIME   =  5u,
		TRACE_ORANGE =  7u,
		TRACE_BLUE   =  8u,
		TRACE_YELLOW = 10u,
		TRACE_BROWN  = 11u,
		TRACE_PURPLE = 13u;

	int
		_spottersOrigin,
		_traceAI,
		_tuEscape;

	BattleUnit
		* _unit,
		* _unitAggro;
	Node
		* _startNode,
		* _stopNode;
	Pathfinding* _pf;
	SavedBattleGame* _battleSave;
	TileEngine* _te;

	AIMode _AIMode;

	std::vector<size_t> _reachable;


	public:
		/// Creates a BattleAIState linked to the SavedBattleGame and a specific BattleUnit.
		BattleAIState(
				SavedBattleGame* const battleSave,
				BattleUnit* const unit,
				Node* const startNode);
		/// Cleans up the BattleAIState.
		virtual ~BattleAIState();

		/// Loads the AI-state from YAML.
		virtual void load(const YAML::Node& node);
		/// Saves the AI-state to YAML.
		virtual YAML::Node save() const;

		/// Enters the state.
//		virtual void enter();
		/// Exits the state.
//		virtual void exit();
		/// Initializes Pathfinding and TileEngine.
		virtual void init();
		/// Runs state functionality every AI-cycle.
		virtual void thinkAi(BattleAction* const action);

		/// Accesses the reachable-tiles vector.
		std::vector<size_t>& reachableTiles();

		/// Resets the unit's saved parameters.
		void resetAI();

		/// Gets the current AIMode setting.
		AIMode getAIMode();
		/// Converts the AIMode into a string for debugging.
		static std::string debugAiMode(AIMode mode);
		/// Gets a color representative of AI-movement calculations.
		static Uint8 debugTraceColor(
				bool chosen,
				int score);
};

}

#endif
