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
 */
BattleAIState::BattleAIState(
		SavedBattleGame* const battleSave,
		BattleUnit* const unit)
	:
		_battleSave(battleSave),
		_unit(unit),
		_unitAggro(nullptr),
		_AIMode(AI_PATROL),
		_startNode(nullptr),
		_stopNode(nullptr),
		_spottersOrigin(0)
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
void BattleAIState::load(const YAML::Node&)
{}

/**
 * Saves the AI state to a YAML file.
 * @return, YAML node
 */
YAML::Node BattleAIState::save() const // virtual.
{
	return YAML::Node();
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
