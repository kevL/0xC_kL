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

#include "UnitPanicBState.h"

#include "../Engine/RNG.h"
#include "../Battlescape/TileEngine.h"

#include "../Savegame/BattleUnit.h"
#include "../Savegame/SavedBattleGame.h"


namespace OpenXcom
{

/**
 * Sets up the UnitPanicBState.
 * @note It's rather silly that this is a full BattleState.
 * @param battle	- pointer to the BattlescapeGame
 * @param unit		- pointer to a panicking BattleUnit
 */
UnitPanicBState::UnitPanicBState(
		BattlescapeGame* const battle,
		BattleUnit* const unit)
	:
		BattleState(battle),
		_unit(unit)
{}

/**
 * Deletes this UnitPanicBState.
 */
UnitPanicBState::~UnitPanicBState()
{}

/**
 * Gets the label of this BattleState.
 * @return, label of the substate
 */
std::string UnitPanicBState::getBattleStateLabel() const
{
	std::ostringstream oststr;
	oststr << "UnitPanicBState";
	if (_action.actor != nullptr) oststr << " ActorId-" << _action.actor->getId();
	if (_unit         != nullptr) oststr << " UnitId-"  << _unit        ->getId();
	return oststr.str();
}

/**
 *
 */
//void UnitPanicBState::init(){}

/**
 * Runs BattleState functionality every cycle.
 * @note Ends panicking for the BattleUnit.
 */
void UnitPanicBState::think()
{
	if (_unit->getUnitStatus() == STATUS_STANDING)
		_unit->moraleChange(10 + RNG::generate(0,10));

	_unit->setTu();
	_unit->setEnergy();
	_unit->setDashing(false);

	if (   _battle->getBattleSave()->getSide() == FACTION_PLAYER
		|| _battle->getBattleSave()->getDebugTac() == true)
	{
		_battle->setupSelector();
	}
	_battle->popBattleState();
}

/**
 * Panicking cannot be cancelled.
 */
//void UnitPanicBState::cancel(){}

}
