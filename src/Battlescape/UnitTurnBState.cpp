/*
 * Copyright 2010-2017 OpenXcom Developers.
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

#include "UnitTurnBState.h"

#include "BattlescapeState.h"
#include "Map.h"
#include "TileEngine.h"

#include "../Engine/Sound.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/RuleArmor.h"

#include "../Savegame/BattleUnit.h"
#include "../Savegame/SavedBattleGame.h"


namespace OpenXcom
{

/**
 * Sets up the UnitTurnBState.
 * @param battleGame	- pointer to the BattlescapeGame
 * @param action		- the current BattleAction
 * @param chargeTu		- true if there is TU cost, false for reaction fire and panic (default true)
 */
UnitTurnBState::UnitTurnBState(
		BattlescapeGame* const battleGame,
		BattleAction action,
		bool chargeTu)
	:
		BattleState(battleGame, action),
		_unit(action.actor),
		_chargeTu(chargeTu),
		_turret(false),
		_tu(0)
{}

/**
 * Deletes this UnitTurnBState.
 */
UnitTurnBState::~UnitTurnBState()
{}

/**
 * Gets the label of this BattleState.
 * @return, label of the substate
 */
std::string UnitTurnBState::getBattleStateLabel() const
{
	std::ostringstream oststr;
	oststr << "UnitTurnBState";
	if (_action.actor != nullptr) oststr << " id-" << _action.actor->getId();
	else oststr << " - Actor INVALID";

	return oststr.str();
}

/**
 * Initializes this BattleState.
 */
void UnitTurnBState::init()
{
//	if (_unit->isOut_t(OUT_STAT) == false)
	switch (_unit->getUnitStatus())
	{
		case STATUS_STANDING:
			_action.TU = 0;
			_unit->setStopShot(false);

			// if unit has a turret and it's either strafing or turning during
			// targeting then only the turret turns
			_turret = _unit->getTurretType() != TRT_NONE
				   && (_action.strafe == true || _action.targeting == true);

			switch (_action.value)
			{
				case -1:
					if (   _unit->getPosition().x != _action.posTarget.x
						|| _unit->getPosition().y != _action.posTarget.y)
					{
						_unit->setDirectionTo(_action.posTarget, _turret); // -> STATUS_TURNING
					}
					break;

				default:
					_unit->setDirectionTo(_action.value, _turret); // -> STATUS_TURNING
			}

			switch (_unit->getUnitStatus())
			{
				case STATUS_TURNING:
					if (_chargeTu == true)						// reaction fire & panic permit free turning
					{
						if (_unit->getTurretType() != TRT_NONE	// if turreted vehicle
							&& _action.strafe == false			// but not swivelling turret
							&& _action.targeting == false)		// and not taking a shot at something...
						{
							switch (_unit->getMoveTypeUnit())
							{
								case MT_FLY: _tu = 2; break;	// hover vehicles cost 2 per facing change
								default:	 _tu = 3;			// large tracked vehicles cost 3 per facing change
							}
						}
						else
							_tu = 1;							// one tu per facing change
					}

					Uint32 interval;
					if (_unit->getFaction() == FACTION_PLAYER)
						interval = _battleGame->getBattlescapeState()->STATE_INTERVAL_XCOM;
					else
						interval = _battleGame->getBattlescapeState()->STATE_INTERVAL_ALIEN;

					_battleGame->setStateInterval(interval);
					break;

				case STATUS_STANDING: // try to open a door
					if (_chargeTu == true && _action.type == BA_NONE)
					{
						int soundId;
						switch (_battleGame->getTileEngine()->unitOpensDoor(_unit))
						{
							case DR_WOOD_OPEN:
								soundId = static_cast<int>(ResourcePack::DOOR_OPEN);
								break;
							case DR_UFO_OPEN:
								soundId = static_cast<int>(ResourcePack::SLIDING_DOOR_OPEN);
								break;
							case DR_ERR_TU:
								_action.result = BattlescapeGame::PLAYER_ERROR[0u];
								soundId = -1;
								break;
							case DR_ERR_RESERVE:
								_action.result = "STR_TUS_RESERVED"; // no break;

							default:
								soundId = -1;
						}

						if (soundId != -1)
							_battleGame->getResourcePack()->getSound("BATTLE.CAT", static_cast<unsigned>(soundId))
															->play(-1, _battleGame->getMap()->getSoundAngle(_unit->getPosition()));
					} // no break;

				default: // safety.
					_unit->clearTurnDirection();
					_battleGame->popBattleState();
			}
			break;

		default:
			_unit->clearTurnDirection();
			_battleGame->popBattleState();
	}
}

/**
 * Runs BattleState functionality every cycle.
 */
void UnitTurnBState::think()
{
	bool pop;

	if (_chargeTu == true
		&& _action.targeting == false
		&& _unit->getFaction() != FACTION_PLAYER // <- no Reserve tolerance.
		&& _battleGame->checkReservedTu(_unit, _tu) == false)
	{
		_unit->setUnitStatus(STATUS_STANDING);
		pop = true;
	}
	else if (_unit->expendTu(_tu) == true)
	{
		_unit->turn(_turret); // done-> STATUS_STANDING
		_battleGame->getMap()->cacheUnitSprite(_unit);

		bool spot;
		switch (_unit->getFaction())
		{
			case FACTION_PLAYER:
				_battleGame->getTileEngine()->calcFovTiles(_unit);
				// no break;
			case FACTION_HOSTILE:
				spot = _battleGame->playerPanicHandled() == true // short-circuit of calcFovUnits() is intentional.
					&& _battleGame->getTileEngine()->calcFovUnits(_unit);
				break;

			case FACTION_NEUTRAL:
			default:
				spot = false;
		}

		if (_chargeTu == true)
		{
			switch (_unit->getFaction())
			{
				case FACTION_PLAYER:
					if (spot == true)
					{
						_unit->setUnitStatus(STATUS_STANDING);

						if (_action.targeting == true)
							_unit->setStopShot(); // NOTE: keep this for Faction_Player only till I intuit the AI better.
					}
					break;

				case FACTION_HOSTILE:
					if (_action.type == BA_NONE && spot == true)
						_unit->setUnitStatus(STATUS_STANDING);
			}
		}

		switch (_unit->getUnitStatus())
		{
			case STATUS_STANDING:
				pop = true;
				break;

			default:
				pop = false;
				if (_chargeTu == true && _unit->getFaction() == FACTION_PLAYER)
				{
					_battleGame->getBattlescapeState()->clearHostileIcons();

					if (_battleGame->playerPanicHandled() == true)
						_battleGame->getBattlescapeState()->updateHostileIcons();
				}
		}
	}
	else
	{
		_action.result = BattlescapeGame::PLAYER_ERROR[0u];
		_unit->setUnitStatus(STATUS_STANDING);
		pop = true;
	}

	if (pop == true)
	{
		_unit->clearTurnDirection();
		_battleGame->popBattleState();
	}
}

/**
 * Unit turning cannot be cancelled.
 */
//void UnitTurnBState::cancel(){}

}
