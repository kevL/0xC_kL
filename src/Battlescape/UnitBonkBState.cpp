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

#include "UnitBonkBState.h"

//#include <algorithm>	// std::find()
//#include <list>		// std::list<>

#include "BattlescapeState.h"
#include "Camera.h"
#include "Map.h"
#include "Pathfinding.h"
#include "TileEngine.h"

//#include "../Engine/Options.h"

#include "../Ruleset/RuleArmor.h"

#include "../Savegame/BattleUnit.h"
#include "../Savegame/SavedBattleGame.h"
#include "../Savegame/Tile.h"


namespace OpenXcom
{

// TODO: GET RID OF THIS STATE and disallow units from walking over/falling onto
// other units via Pathfinding and AI.


/**
 * Sets up the UnitBonkBState.
 * @param battle - pointer to the BattlescapeGame
 */
UnitBonkBState::UnitBonkBState(BattlescapeGame* const battle)
	:
		BattleState(battle),
		_te(battle->getTileEngine()),
		_battleSave(battle->getBattleSave()),
		_unitsBonking(battle->getBattleSave()->getBonkers())
{
	//Log(LOG_INFO) << "UnitBonkBState:cTor";
}

/**
 * Deletes the UnitBonkBState.
 */
UnitBonkBState::~UnitBonkBState()
{}

/**
 * Gets the label of this BattleState.
 * @return, label of the substate
 */
std::string UnitBonkBState::getBattleStateLabel() const
{
	std::ostringstream oststr;
	oststr << "UnitBonkBState";
	if (_action.actor != nullptr) oststr << " ActorId-" << _action.actor->getId();
	return oststr.str();
}

/**
 * Initializes this BattleState.
 */
void UnitBonkBState::init()
{
	Uint32 interval;
	switch (_battleSave->getSide())
	{
		default:
		case FACTION_PLAYER:
			interval = _battle->getBattlescapeState()->STATE_INTERVAL_XCOM;
			break;

		case FACTION_HOSTILE:
		case FACTION_NEUTRAL:
			interval = _battle->getBattlescapeState()->STATE_INTERVAL_ALIEN;
	}

	//Log(LOG_INFO) << "unitFallB: init() set interval= " << interval;
	_battle->setStateInterval(interval);
}

/**
 * Runs BattleState functionality every cycle.
 * @note Progresses the bonk and updates the battlescape etc.
 */
void UnitBonkBState::think()
{
	//Log(LOG_INFO) << "UnitBonkBState::think()";
	BattleUnit* unitBelow;
	const Tile* tile;
	Tile
		* tileBelow,
		* tileStart;
	Position
		pos,
		posStart,
		posQuadStop;

	for (std::list<BattleUnit*>::const_iterator
			i  = _unitsBonking->begin();
			i != _unitsBonking->end();
			)
	{
		//Log(LOG_INFO) << ". falling id-" << (*i)->getId();
		if ((*i)->getHealth() != 0 && (*i)->isStunned() == false)
		{
			pos = (*i)->getPosition();

			switch ((*i)->getUnitStatus()) // sort out unit-status - must be STATUS_STANDING to proceed
			{
				case STATUS_WALKING:
				case STATUS_FLYING:
					//Log(LOG_INFO) << ". . call keepWalking()";
					(*i)->keepWalking(_battleSave->getTile(pos + Position::POS_BELOW), true);
					_battle->getMap()->cacheUnitSprite(*i);

					++i;
					continue;

				case STATUS_TURNING:
					//Log(LOG_INFO) << ". Status_Turning -> Status_Standing";
					(*i)->setUnitStatus(STATUS_STANDING);
			}


			const int quads ((*i)->getArmor()->getSize() - 1);

			if (canFall(*i) == true)
			{
				for (int // tile(s) that unit is falling into:
						x = quads;
						x != -1;
						--x)
				{
					for (int
							y = quads;
							y != -1;
							--y)
					{
						tileBelow = _battleSave->getTile(pos + Position(x,y,-1));
						_tilesToBonkInto.push_back(tileBelow);
					}
				}

				for (std::vector<Tile*>::const_iterator // check each tile for units that need moving out of the way.
						j  = _tilesToBonkInto.begin();
						j != _tilesToBonkInto.end();
						++j)
				{
					if ((unitBelow = (*j)->getTileUnit()) != nullptr
						&& std::find( // ignore falling units (including self)
								_unitsBonking->begin(),
								_unitsBonking->end(),
								unitBelow) == _unitsBonking->end()
						&& std::find( // ignore already added units
								_unitsBonked.begin(),
								_unitsBonked.end(),
								unitBelow) == _unitsBonked.end())
					{
						//Log(LOG_INFO) << ". . . Move, ID " << unitBelow->getId();
						_unitsBonked.push_back(unitBelow);
					}
				}
			}

			if ((posStart = (*i)->getStartPosition()) != pos)	// the unit has moved from one tile to the other.
			{													// kL_note: Can prob. use _tileSwitchDone around here ... plus use transient tile(s).
				for (int // reset tiles moved from
						x = quads;
						x != -1;
						--x)
				{
					for (int
							y = quads;
							y != -1;
							--y)
					{
						tileStart = _battleSave->getTile(posStart + Position(x,y,0));
						if (*i == tileStart->getTileUnit())
						{
							//Log(LOG_INFO) << ". clear startTile's unit";
							tileStart->setTileUnit();
						}
					}
				}

				(*i)->setUnitTile(
							_battleSave->getTile(pos),
							_battleSave->getTile(pos + Position::POS_BELOW));
				for (int // update tiles moved to.
						x = quads;
						x != -1;
						--x)
				{
					for (int
							y = quads;
							y != -1;
							--y)
					{
						//Log(LOG_INFO) << ". setTileUnit to belowTile";
						_battleSave->getTile(pos + Position(x,y,0))->setTileUnit(*i);
					}
				}

				switch ((*i)->getUnitStatus())
				{
					case STATUS_STANDING: // done falling, just standing around.
						//Log(LOG_INFO) << ". STATUS_STANDING";
						// My God, this was all so badly coded!!!!
						if (canFall(*i) == true)
						{
							//Log(LOG_INFO) << ". . still falling -> startWalking()";
							(*i)->startWalking(
											Pathfinding::DIR_DOWN,
											pos + Position::POS_BELOW);

							(*i)->setCacheInvalid();
							_battle->getMap()->cacheUnitSprite(*i);

							++i; // <- are you sure. why not let unit continue falling
						}
						else // done falling just standing around ...
						{
							//Log(LOG_INFO) << ". . burnFloors, checkProxies, Erase.i";
							if ((*i)->getSpecialAbility() == SPECAB_BURN) // burn floortiles
							{
								// Put burnedBySilacoid() here! etc
								(*i)->burnTile((*i)->getUnitTile());

								if ((*i)->getUnitStatus() != STATUS_STANDING)	// ie. burned a hole in the floor and fell through it
									_battle->getPathfinding()->abortPath();	// TODO: trace this.
							}

							_te->calculateUnitLighting();

							(*i)->setCacheInvalid();
							_battle->getMap()->cacheUnitSprite(*i);

							if ((*i)->getFaction() == FACTION_PLAYER)
								_te->calcFovTiles(*i);
							_te->calcFovUnits_pos(pos, true);

							_battle->checkProxyGrenades(*i);
							// kL_add: Put checkForSilacoid() here!

							if ((*i)->getUnitStatus() == STATUS_STANDING)
							{
								if (_battle->getTileEngine()->checkReactionFire(*i) == true)	// TODO: Not so sure I want RF on these guys ....
								{
									if ((*i)->getFaction() == _battleSave->getSide())			// Eg. this would need a vector to be accurate.
										_battleSave->rfTriggerOffset(_battle->getMap()->getCamera()->getMapOffset());

									_battle->getPathfinding()->abortPath();						// In fact this whole state should be bypassed.
								}
								i = _unitsBonking->erase(i);
							}
						}
						break;

					default:
						//Log(LOG_INFO) << ". not STATUS_STANDING, next unit";
						++i;
				}
			}
		}
		else // dead or stunned ->
		{
			//Log(LOG_INFO) << ". dead OR stunned, Erase & cont";
			i = _unitsBonking->erase(i);
		}
	}


	if (_unitsBonked.empty() == false) // find somewhere to move the unit(s) in danger of being squashed.
	{
		//Log(LOG_INFO) << ". unitsToMove not empty";
		std::vector<Tile*> escapeTiles;

		for (std::vector<BattleUnit*>::const_iterator
				i  = _unitsBonked.begin();
				i != _unitsBonked.end();
				)
		{
			//Log(LOG_INFO) << ". moving unit ID " << (*i)->getId();
			bool escape (false);

			const int belowSize ((*i)->getArmor()->getSize() - 1); // need to move all sections of unitBelow out of the way.
			std::vector<Position> posQuadrants;
			for (int
					x = belowSize;
					x != -1;
					--x)
			{
				for (int
						y = belowSize;
						y != -1;
						--y)
				{
					//Log(LOG_INFO) << ". body size + 1";
					posQuadrants.push_back((*i)->getPosition() + Position(x,y,0));
				}
			}

			for (int // check in each direction.
					dir = 0;
					dir != Pathfinding::DIR_UP && escape == false;
					++dir)
			{
				//Log(LOG_INFO) << ". . checking directions to move";
				Position posVect;
				Pathfinding::directionToVector(dir, &posVect);

				for (std::vector<Position>::const_iterator
						j  = posQuadrants.begin();
						j != posQuadrants.end();
						)
				{
					//Log(LOG_INFO) << ". . . checking quads";
					posQuadStop = *j + posVect;
					tile = _battleSave->getTile(posQuadStop);
					tileBelow = _battleSave->getTile(posQuadStop + Position::POS_BELOW);

					if (tile == nullptr
						|| (tile->getTileUnit() != nullptr && tile->getTileUnit() != *i)
						|| (tile->isFloored(tileBelow) == false && (*i)->getMoveTypeUnit() != MT_FLY)
						|| std::find(
								escapeTiles.begin(),
								escapeTiles.end(),
								tile) != escapeTiles.end()
						|| std::find(
								_tilesToBonkInto.begin(),
								_tilesToBonkInto.end(),
								tile) != _tilesToBonkInto.end()
						|| _battleSave->getPathfinding()->getTuCostPf(*j, dir, &posQuadStop) == Pathfinding::PF_FAIL_TU)
//						|| _battleSave->getPathfinding()->isBlockedDir(_battleSave->getTile(*j), dir, *i)
					{
						break; // no go -> try next direction
					}
					// else check next quad of the unit ->

					// if all sections of the unit-fallen-onto can be moved then move it
					if (++j == posQuadrants.end())
					{
						//Log(LOG_INFO) << ". . . . move unit";
						if (_battleSave->addBonker(*i) == true)
						{
							//Log(LOG_INFO) << ". . . . . add Falling Unit";
							escape = true;

							for (int // now ensure no other unit escapes here too.
									x = belowSize;
									x != -1;
									--x)
							{
								for (int
										y = belowSize;
										y != -1;
										--y)
								{
									//Log(LOG_INFO) << ". . . . . . check for more escape units?";
									escapeTiles.push_back(_battleSave->getTile(tile->getPosition() + Position(x,y,0)));
								}
							}

							//Log(LOG_INFO) << ". . . . startWalking() out of the way?";
							(*i)->startWalking(
											dir,
											(*i)->getPosition() + posVect,
											_battleSave->getTile(*j + Position::POS_BELOW));

							i = _unitsBonked.erase(i);
						}
					}
				}
			}

			if (escape == false)
			{
				//Log(LOG_INFO) << ". . . NOT escape";
//				(*i)->knockOut(); // needs conversion check. THIS FUNCTION HAS BEEN REMOVED.
				i = _unitsBonked.erase(i);
			}
		}

		//Log(LOG_INFO) << ". . checkCasualties()";
		_battle->checkCasualties();
	}


	//Log(LOG_INFO) << ". done main recursion";
	if (_unitsBonking->empty() == true)
	{
		//Log(LOG_INFO) << ". Falling units EMPTY";
//		_tilesToFallInto.clear();
//		_unitsUnder.clear();

		_battle->popBattleState();
	}
	//Log(LOG_INFO) << "UnitBonkBState::think() EXIT";
}

/**
 * Checks if a BattleUnit can fall to the next lower level.
 * @param unit - pointer to a unit
 */
bool UnitBonkBState::canFall(const BattleUnit* const unit)
{
	if (unit->getWalkPhase() == 0 && unit->getMoveTypeUnit() != MT_FLY)
	{
		const Position& pos (unit->getPosition());
		if (pos.z != 0)
		{
			const Tile* tile;
			const int quads (unit->getArmor()->getSize() - 1);
			for (int
					x = quads;
					x != -1;
					--x)
			{
				for (int
						y = quads;
						y != -1;
						--y)
				{
					tile = _battleSave->getTile(pos + Position(x,y,0));
					if (tile->isFloored(tile->getTileBelow(_battleSave)) == true)
						return false;
				}
			}
			return true;
		}
	}
	return false;
}

}
