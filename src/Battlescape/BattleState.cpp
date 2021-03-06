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

#include "BattleState.h"

#include "../Savegame/BattleUnit.h"


namespace OpenXcom
{

/**
 * Sets up the BattleState.
 * @param battle - pointer to BattlescapeGame
 * @param action - struct containing info about the current BattleAction
 */
BattleState::BattleState(
		BattlescapeGame* const battle,
		BattleAction action)
	:
		_battle(battle),
		_action(action)
{}

/**
 * Deletes this BattleState.
 */
BattleState::~BattleState() // virtual.
{}

/**
 * Gets the label of this BattleState.
 * @return, label of the substate if any else "BattleState"
 */
std::string BattleState::getBattleStateLabel() const // virtual.
{
	std::ostringstream oststr;
	oststr << "BattleState";
	if (_action.actor != nullptr) oststr << " ActorId-" << _action.actor->getId();
	return oststr.str();
}

/**
 * Starts the current BattleState.
 */
void BattleState::init() // virtual.
{}

/**
 * Cancels the current BattleState.
 */
void BattleState::cancel() // virtual.
{}

/**
 * Runs any code the current BattleState needs to keep updating every
 * engine-cycle.
 */
void BattleState::think() // virtual.
{}

/**
 * Gets a copy of the BattleAction.
 * @return, the BattleAction struct (BattlescapeGame.h)
 */
BattleAction BattleState::getAction() const
{
	return _action;
}

}
