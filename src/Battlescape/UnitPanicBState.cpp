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

#include "UnitPanicBState.h"

#include "../Engine/RNG.h"
#include "../Battlescape/TileEngine.h"

#include "../Savegame/BattleUnit.h"
#include "../Savegame/SavedBattleGame.h"


namespace OpenXcom
{

/**
 * Sets up the UnitPanicBState.
 * @param parent	- pointer to the BattlescapeGame
 * @param unit		- pointer to a panicking BattleUnit
 */
UnitPanicBState::UnitPanicBState(
		BattlescapeGame* const parent,
		BattleUnit* const unit)
	:
		BattleState(parent),
		_unit(unit)
{}

/**
 * Deletes this UnitPanicBState.
 */
UnitPanicBState::~UnitPanicBState()
{}

/**
 * Gets the name of this BattleState.
 * @return, label of the substate
 */
std::string UnitPanicBState::getBattleStateLabel() const
{
	return "UnitPanicBState";
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
	if (_unit->isOut_t(OUT_STAT) == false)
	{
		_unit->setUnitStatus(STATUS_STANDING);
		_unit->moraleChange(10 + RNG::generate(0,10));
	}

	_unit->setTimeUnits(0);
	_unit->setDashing(false);

	if (_parent->getBattleSave()->getSide() == FACTION_PLAYER
		|| _parent->getBattleSave()->getDebugTac() == true)
	{
		_parent->setupSelector();
	}
	_parent->popState();
}

/**
 * Panicking cannot be cancelled.
 */
//void UnitPanicBState::cancel(){}

}
