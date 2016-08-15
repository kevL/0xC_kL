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

#include "UnitFallBState.h"

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

/**
 * Sets up the UnitFallBState.
 * @param parent - pointer to the BattlescapeGame
 */
UnitFallBState::UnitFallBState(BattlescapeGame* const parent)
	:
		BattleState(parent),
		_te(parent->getTileEngine()),
		_battleSave(parent->getBattleSave())
{
	//Log(LOG_INFO) << "UnitFallBState:cTor";
}

/**
 * Deletes the UnitWalkBState.
 */
UnitFallBState::~UnitFallBState()
{}

/**
 * Gets the label of this BattleState.
 * @return, label of the substate
 */
std::string UnitFallBState::getBattleStateLabel() const
{
	std::ostringstream oststr;
	oststr << "UnitFallBState";
	if (_action.actor != nullptr) oststr << " id-" << _action.actor->getId();
	else oststr << " - Actor INVALID";

	return oststr.str();
}

/**
 * Initializes this BattleState.
 */
void UnitFallBState::init()
{
	Uint32 interval;
	switch (_battleSave->getSide())
	{
		default:
		case FACTION_PLAYER:
			interval = _parent->getBattlescapeState()->STATE_INTERVAL_XCOM;
			break;

		case FACTION_HOSTILE:
		case FACTION_NEUTRAL:
			interval = _parent->getBattlescapeState()->STATE_INTERVAL_ALIEN;
	}

	//Log(LOG_INFO) << "unitFallB: init() set interval= " << interval;
	_parent->setStateInterval(interval);
}

/**
 * Runs BattleState functionality every cycle.
 * @note Progresses the fall and updates the battlescape etc.
 */
void UnitFallBState::think()
{
	//Log(LOG_INFO) << "UnitFallBState::think()";
	BattleUnit* unitBelow;
	const Tile* tile;
	Tile
		* tileBelow,
		* tileStart;
	Position
		pos,
		posStart,
		posStop,
		posQuad;
	const Position& posBelow (Position(0,0,-1));

	for (std::list<BattleUnit*>::const_iterator
			i = _battleSave->getFallingUnits()->begin();
			i != _battleSave->getFallingUnits()->end();
			)
	{
		//Log(LOG_INFO) << ". falling id-" << (*i)->getId();
		if ((*i)->getHealth() == 0 || (*i)->isStunned() == true)
		{
			//Log(LOG_INFO) << ". dead OR stunned, Erase & cont";
			i = _battleSave->getFallingUnits()->erase(i);
			continue;
		}

		if ((*i)->getUnitStatus() == STATUS_TURNING)
		{
			//Log(LOG_INFO) << ". Status_Turning -> Status_Standing";
			(*i)->setUnitStatus(STATUS_STANDING);
		}

		bool
			fallCheck (true),
			fall;
//			onScreen = (*i)->getUnitVisible()
//					&& _parent->getMap()->getCamera()->isOnScreen(posStart);
//		bool onScreen = ((*i)->getVisible() && _parent->getMap()->getCamera()->isOnScreen(posStart, true, size, false));

		pos = (*i)->getPosition();

		const int unitSize ((*i)->getArmor()->getSize() - 1);
		for (int
				x = unitSize;
				x != -1;
				--x)
		{
			for (int
					y = unitSize;
					y != -1;
					--y)
			{
				tileBelow = _battleSave->getTile(pos + Position(x,y,-1));
				if (_battleSave->getTile(pos + Position(x,y,0))->isFloored(tileBelow) == true
					|| (*i)->getMoveTypeUnit() == MT_FLY)
				{
					//Log(LOG_INFO) << ". . fallCheck set FALSE";
					fallCheck = false;
				}
			}
		}

		tileBelow = _battleSave->getTile(pos + posBelow);

		fall = fallCheck == true // why are there two 'fall' determinations. See below_
			&& pos.z != 0
			&& (*i)->getUnitTile()->isFloored(tileBelow) == false
//			&& (*i)->getMoveTypeUnit() != MT_FLY // done above in fallCheck
			&& (*i)->getWalkPhase() == 0;

		if (fall == true)
		{
			for (int // tile(s) that unit is falling into:
					x = unitSize;
					x != -1;
					--x)
			{
				for (int
						y = unitSize;
						y != -1;
						--y)
				{
					tileBelow = _battleSave->getTile(pos + Position(x,y,-1));
					_tilesToFallInto.push_back(tileBelow);
				}
			}

			for (std::vector<Tile*>::const_iterator // check each tile for units that need moving out of the way.
					j = _tilesToFallInto.begin();
					j != _tilesToFallInto.end();
					++j)
			{
				if ((unitBelow = (*j)->getTileUnit()) != nullptr
					&& *i != unitBelow	// falling units do not fall on themselves
					&& std::find(
							_unitsToMove.begin(),
							_unitsToMove.end(),
							unitBelow) == _unitsToMove.end())
				{
					//Log(LOG_INFO) << ". . . Move, ID " << unitBelow->getId();
					_unitsToMove.push_back(unitBelow);
				}
			}
		}

		switch ((*i)->getUnitStatus())
		{
			case STATUS_WALKING:
			case STATUS_FLYING:
				//Log(LOG_INFO) << ". . call keepWalking()";
				(*i)->keepWalking(tileBelow, true);		// advances the phase
				_parent->getMap()->cacheUnitSprite(*i);	// make sure the fallUnit sprites are up to date
		}

		fall = fallCheck == true // why are there two 'fall' determinations. See above^
			&& pos.z != 0
			&& (*i)->getUnitTile()->isFloored(tileBelow) == false
//			&& (*i)->getMovementType() != MT_FLY // done above in fallCheck
			&& (*i)->getWalkPhase() == 0;

		//Log(LOG_INFO) << ". new fallCheck = " << fallCheck;


		if ((posStart = (*i)->getStartPosition()) != pos)	// the unit has moved from one tile to the other.
		{													// kL_note: Can prob. use _tileSwitchDone around here ... plus use transient tile(s).
			for (int // reset tiles moved from
					x = unitSize;
					x != -1;
					--x)
			{
				for (int
						y = unitSize;
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
							_battleSave->getTile(pos + Position(0,0,-1)));
			for (int // update tiles moved to.
					x = unitSize;
					x != -1;
					--x)
			{
				for (int
						y = unitSize;
						y != -1;
						--y)
				{
					//Log(LOG_INFO) << ". setTileUnit to belowTile";
					_battleSave->getTile(pos + Position(x,y,0))->setTileUnit(*i);
				}
			}

			if (_unitsToMove.empty() == false) // find somewhere to move the unit(s) in danger of being squashed.
			{
				//Log(LOG_INFO) << ". unitsToMove not empty";
				std::vector<Tile*> escapeTiles;

				for (std::vector<BattleUnit*>::const_iterator
						j = _unitsToMove.begin();
						j != _unitsToMove.end();
						)
				{
					//Log(LOG_INFO) << ". moving unit ID " << (*j)->getId();
					unitBelow = *j;
					bool escape (false);

					const int belowSize (unitBelow->getArmor()->getSize() - 1); // need to move all sections of unitBelow out of the way.
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
							posQuadrants.push_back(unitBelow->getPosition() + Position(x,y,0));
						}
					}

					for (int // Check in each direction.
							dir = 0;
							dir != Pathfinding::DIR_UP && escape == false;
							++dir)
					{
						//Log(LOG_INFO) << ". . checking directions to move";
						Position posVect;
						Pathfinding::directionToVector(dir, &posVect);

						for (std::vector<Position>::const_iterator
								k = posQuadrants.begin();
								k != posQuadrants.end();
								)
						{
							//Log(LOG_INFO) << ". . . checking bodysections";
							posQuad = *k;
							tile = _battleSave->getTile(posQuad + posVect);
							tileBelow = _battleSave->getTile(posQuad + posVect + posBelow);

							bool
								aboutToBeOccupiedFromAbove (tile != nullptr
														 && std::find(
																	_tilesToFallInto.begin(),
																	_tilesToFallInto.end(),
																	tile) != _tilesToFallInto.end()),
								alreadyTaken (tile != nullptr
										   && std::find(
													escapeTiles.begin(),
													escapeTiles.end(),
													tile) != escapeTiles.end()),
								alreadyOccupied (tile != nullptr
											  && tile->getTileUnit() != nullptr
											  && tile->getTileUnit() != unitBelow),
								hasFloor (tile != nullptr
									   && tile->isFloored(tileBelow) == true),
								blocked (_battleSave->getPathfinding()->isBlockedPath(
																					_battleSave->getTile(posQuad),
																					dir,
																					unitBelow)),
								unitCanFly (unitBelow->getMoveTypeUnit() == MT_FLY),
								canMoveToTile (tile != nullptr
											&& alreadyOccupied == false
											&& alreadyTaken == false
											&& aboutToBeOccupiedFromAbove == false
											&& blocked == false
											&& (hasFloor == true || unitCanFly == true));

							if (canMoveToTile == true)
								++k; // Check next section of the unit.
							else
								break; // Try next direction.


							// If all sections of the unit-fallen-onto can be moved then move it.
							if (k == posQuadrants.end())
							{
								//Log(LOG_INFO) << ". . . . move unit";
								if (_battleSave->addFallingUnit(unitBelow) == true)
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
									unitBelow->startWalking(
														dir,
														unitBelow->getPosition() + posVect,
														_battleSave->getTile(posQuad + posBelow));

									j = _unitsToMove.erase(j);
								}
							}
						}
					}

					if (escape == false)
					{
						//Log(LOG_INFO) << ". . . NOT escape";
						unitBelow->knockOut();
						j = _unitsToMove.erase(j);
					}
				}

				//Log(LOG_INFO) << ". . checkCasualties()";
				_parent->checkCasualties(nullptr, *i);
			}
		}

		switch ((*i)->getUnitStatus())
		{
			case STATUS_STANDING: // done falling, just standing around.
				//Log(LOG_INFO) << ". STATUS_STANDING";
				if (fall == true)
				{
					//Log(LOG_INFO) << ". . still falling -> startWalking()";
					posStop = pos + posBelow;

					tileBelow = _battleSave->getTile(posStop);
					(*i)->startWalking(
									Pathfinding::DIR_DOWN,
									posStop,
									tileBelow);

					(*i)->setCacheInvalid();
					_parent->getMap()->cacheUnitSprite(*i);

					++i;
				}
				else // done falling just standing around ...
				{
					//Log(LOG_INFO) << ". . burnFloors, checkProxies, Erase.i";
					if ((*i)->getSpecialAbility() == SPECAB_BURN) // burn floortiles
					{
						// Put burnedBySilacoid() here! etc
						(*i)->burnTile((*i)->getUnitTile());

//						const int power ((*i)->getUnitRules()->getSpecabPower());
//						(*i)->getTile()->igniteTile(power / 10);
//						const Position targetVoxel (Position::toVoxelSpaceCentered(
//																			pos,
//																			-(*i)->getTile()->getTerrainLevel()));
//						_parent->getTileEngine()->hit(
//													targetVoxel,
//													power,
//													DT_IN,
//													*i);
						if ((*i)->getUnitStatus() != STATUS_STANDING) // ie. burned a hole in the floor and fell through it
							_parent->getPathfinding()->abortPath();
					}

					_te->calculateUnitLighting();

					(*i)->setCacheInvalid();
					_parent->getMap()->cacheUnitSprite(*i);

					if ((*i)->getFaction() == FACTION_PLAYER)
						_te->calcFovTiles(*i);
					_te->calcFovUnits_pos(pos, true);

					_parent->checkProxyGrenades(*i);
					// kL_add: Put checkForSilacoid() here!

					if ((*i)->getUnitStatus() == STATUS_STANDING)
					{
						if (_parent->getTileEngine()->checkReactionFire(*i) == true)	// TODO: Not so sure I want RF on these guys ....
						{
							if ((*i)->getFaction() == _battleSave->getSide())			// Eg. this would need a vector to be accurate.
								_battleSave->rfTriggerOffset(_parent->getMap()->getCamera()->getMapOffset());

							_parent->getPathfinding()->abortPath();						// In fact this whole state should be bypassed.
						}
						i = _battleSave->getFallingUnits()->erase(i);
					}
				}
				break;

			default:
				//Log(LOG_INFO) << ". not STATUS_STANDING, next unit";
				++i;
		}
	}


	//Log(LOG_INFO) << ". done main recursion";
	if (_battleSave->getFallingUnits()->empty() == true)
	{
		//Log(LOG_INFO) << ". Falling units EMPTY";
//		_tilesToFallInto.clear();
//		_unitsToMove.clear();

		_parent->popBattleState();
//		return;
	}
	//Log(LOG_INFO) << "UnitFallBState::think() EXIT";
}

}
