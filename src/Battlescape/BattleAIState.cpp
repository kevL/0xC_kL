/*
 * Copyright 2010-2018 OpenXcom Developers.
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

#include "BattleAIState.h"

#include "../Engine/Logger.h"
#include "../Engine/Options.h"


namespace OpenXcom
{

/**
 * Sets up a BattleAIState.
 * @param battleSave	- pointer to the SavedBattleGame
 * @param unit			- pointer to a BattleUnit
 * @param startNode		- pointer to the unit's start-node
 */
BattleAIState::BattleAIState(
		SavedBattleGame* const battleSave,
		BattleUnit* const unit,
		Node* const startNode)
	:
		_battleSave(battleSave),
		_unit(unit),
		_startNode(startNode),
		_stopNode(nullptr),
		_AIMode(AI_PATROL),
		_unitAggro(nullptr),
		_spottersOrigin(0),
		_tuEscape(-1),
		_traceAI(Options::traceAI)
{
	if (_traceAI) Log(LOG_INFO) << "Create BattleAIState id-" << _unit->getId();
}

/**
 * Deletes the BattleAIState.
 */
BattleAIState::~BattleAIState() // virtual.
{}

/**
 * Loads the AI state from a YAML file.
 * @param node - reference a YAML node
 */
void BattleAIState::load(const YAML::Node& node) // virtual
{
	_AIMode = static_cast<AIMode>(node["ai"].as<int>(0));

	int nodeId (node["start"].as<int>(-1));
	if (nodeId != -1)
		_startNode = _battleSave->getNodes()->at(static_cast<size_t>(nodeId));

	nodeId = node["stop"].as<int>(-1);

	if (nodeId != -1)
		_stopNode = _battleSave->getNodes()->at(static_cast<size_t>(nodeId));
}

/**
 * Saves the AI state to a YAML file.
 * @return, YAML node
 */
YAML::Node BattleAIState::save() const // virtual.
{
	YAML::Node node;

	node["ai"] = static_cast<int>(_AIMode);

	int nodeId;

	if (_startNode != nullptr)
		nodeId = _startNode->getId();
	else
		nodeId = -1;

	node["start"] = nodeId;

	if (_stopNode != nullptr)
		nodeId = _stopNode->getId();
	else
		nodeId = -1;

	node["stop"] = nodeId;

	return node;
}

/**
 * Enters the current AI state.
 */
//void BattleAIState::enter(){}
/**
 * Exits the current AI state.
 */
//void BattleAIState::exit(){}

/**
 * Initializes Pathfinding and TileEngine.
 */
void BattleAIState::init() // virtual.
{
	_pf = _battleSave->getPathfinding();
	_te = _battleSave->getTileEngine();
}

/**
 * Runs any code the state needs to keep updating every AI-cycle.
 * @note Called by BattleUnit::thinkAi().
 * @param action - pointer to a BattleAction to fill w/ data (BattlescapeGame.h)
 */
void BattleAIState::thinkAi(BattleAction* const) // virtual.
{}

/**
 * Accesses the reachable-tiles vector.
 * @return, reference to a vector of tile-IDs
 */
std::vector<size_t>& BattleAIState::reachableTiles()
{
	return _reachable;
}

/**
 * Resets the unit's saved parameters.
 */
void BattleAIState::resetAI()
{
	_AIMode = AI_PATROL;
	_startNode =
	_stopNode = nullptr;
}

/**
 * Gets the current AIMode setting.
 * @return, the AIMode (BattleAIState.h)
 */
AIMode BattleAIState::getAIMode()
{
	return _AIMode;
}

/**
 * Converts an AIMode into a string for debugging.
 * @return, AIMode as string
 */
std::string BattleAIState::debugAiMode(AIMode mode) // static.
{
	switch (mode)
	{
		case AI_PATROL: return "PATROL";
		case AI_AMBUSH: return "AMBUSH";
		case AI_COMBAT: return "COMBAT";
		case AI_ESCAPE: return "ESCAPE";
	}
	return "error - no AI Mode";
}

/**
 * Gets a color representative of AI-movement calculations.
 * @param chosen	- true to color the tile as 'chosen'
 * @param score		- the value to display
 * @return, color
 */
Uint8 BattleAIState::debugTraceColor( // static
		bool chosen,
		int score)
{
	if (score < 0)
	{
		if (chosen == true)
			return TRACE_ORANGE;
		return TRACE_RED;
	}

	if (score < (FAST_PASS_THRESHOLD >> 1u))
	{
		if (chosen == true)
			return TRACE_BROWN;
		return TRACE_BLUE;
	}

	if (score < FAST_PASS_THRESHOLD)
	{
		if (chosen == true)
			return TRACE_GREEN;
		return TRACE_YELLOW;
	}

	return TRACE_LIME;
}

}
