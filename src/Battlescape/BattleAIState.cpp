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

#include "BattleAIState.h"

//#include "../Engine/Options.h"


namespace OpenXcom
{

/**
 * Sets up a BattleAIState.
 * @param battleSave	- pointer to the SavedBattleGame
 * @param unit			- pointer to a BattleUnit
 * @param startNode		- pointer to the unit's start Node
 */
BattleAIState::BattleAIState(
		SavedBattleGame* const battleSave,
		BattleUnit* const unit,
		Node* const startNode)
	:
		_battleSave(battleSave),
		_unit(unit),
		_startNode(startNode),
		_unitAggro(nullptr),
		_AIMode(AI_PATROL),
		_stopNode(nullptr),
		_spottersOrigin(0),
		_tuEscape(0)
{
//	_traceAI = Options::traceAI;
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
	_AIMode = static_cast<AIMode>(node["AIMode"].as<int>(0));

	const int
		startNodeId	= node["startNode"]	.as<int>(-1),
		stopNodeId	= node["stopNode"]	.as<int>(-1);

	if (startNodeId != -1)
		_startNode = _battleSave->getNodes()->at(static_cast<size_t>(startNodeId));

	if (stopNodeId != -1)
		_stopNode = _battleSave->getNodes()->at(static_cast<size_t>(stopNodeId));
}

/**
 * Saves the AI state to a YAML file.
 * @return, YAML node
 */
YAML::Node BattleAIState::save() const // virtual.
{
	int
		startNodeId,
		stopNodeId;

	if (_startNode != nullptr)
		startNodeId	= _startNode->getId();
	else
		startNodeId = -1;

	if (_stopNode != nullptr)
		stopNodeId = _stopNode->getId();
	else
		stopNodeId = -1;

	YAML::Node node;

	node["startNode"]	= startNodeId;
	node["stopNode"]	= stopNodeId;
	node["AIMode"]		= static_cast<int>(_AIMode);

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
 * Runs any code the state needs to keep updating every AI cycle.
 * @param action - (possible) AI BattleAction to execute after thinking is done.
 */
void BattleAIState::think(BattleAction* const) // virtual.
{}

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
 * Gets the AI Mode for debug-readout.
 * @return, AI Mode as a string
 */
std::string BattleAIState::getAIMode() const // virtual.
{
	switch (_AIMode)
	{
		case 0: return "Patrol";
		case 1: return "Ambush";
		case 2: return "Combat";
		case 3: return "Escape";
	}

	return "error - no AI Mode";
}

}
