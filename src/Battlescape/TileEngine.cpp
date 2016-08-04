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

#include "TileEngine.h"

//#include <climits>
//#include <set>
//#include <assert.h>
//#include <SDL.h>

#include "../fmath.h"

#include "AlienBAIState.h"
#include "BattlescapeState.h"
#include "Camera.h"
#include "ExplosionBState.h"
#include "InfoboxDialogState.h"
#include "Map.h"
#include "Pathfinding.h"
#include "ProjectileFlyBState.h"
#include "UnitTurnBState.h"

#include "../Engine/Game.h"
#include "../Engine/Language.h"
#include "../Engine/Logger.h"
//#include "../Engine/Options.h"
#include "../Engine/RNG.h"
#include "../Engine/Sound.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/MapDataSet.h"
#include "../Ruleset/RuleArmor.h"

#include "../Savegame/BattleItem.h"
#include "../Savegame/BattleUnit.h"
#include "../Savegame/SavedBattleGame.h"
#include "../Savegame/SavedGame.h"


namespace OpenXcom
{

//bool TileEngine::_debug = false;

const int TileEngine::scanOffsetZ[11u] // static/private.
{
		  0,
	 -2,  2,
	 -4,  4,
	 -6,  6,
	 -8,  8,
	-12, 12
};


/**
 * Sets up the TileEngine.
 * @param battleSave	- pointer to SavedBattleGame
 * @param voxelData		- pointer to a vector of voxel-data
 */
TileEngine::TileEngine(
		SavedBattleGame* const battleSave,
		const std::vector<Uint16>* const voxelData)
	:
		_battleSave(battleSave),
		_voxelData(voxelData),
		_unitLighting(true),
		_powerE(-1),
		_powerT(-1),
		_spotSound(true),
		_trueTile(nullptr),
		_dirRay(-1),
		_isReaction(false)
//		_missileDirection(-1)
{
	_rfAction = new BattleAction();
}

/**
 * Deletes the TileEngine.
 */
TileEngine::~TileEngine()
{
	delete _rfAction;
}

/**
 * Calculates sun shading for the whole terrain.
 * @note Wrapper for calculateSunShading().
 */
void TileEngine::calculateSunShading() const
{
	Tile* tile;
	for (size_t // reset and re-calculate sunlight.
			i = 0u;
			i != _battleSave->getMapSizeXYZ();
			++i)
	{
		tile = _battleSave->getTiles()[i];
		tile->resetLight(LIGHT_LAYER_AMBIENT);
		calculateSunShading(tile);
	}
}

/**
 * Calculates sun shading for 1 tile.
 * @note Sun comes from above and is blocked by floors and/or objects.
 * TODO: angle the shadow according to the time - link to Options::globeSeasons
 * (or whatever the realistic lighting thing is).
 * @param tile - pointer to a tile to calculate the sun shading for
 */
void TileEngine::calculateSunShading(Tile* const tile) const
{
	//Log(LOG_INFO) << "TileEngine::calculateSunShading()";
	int light (LIGHT_SUN - _battleSave->getTacticalShade());

	if (_battleSave->getTacticalShade() < 5) // Sun doesn't drop shadows in broad daylight.
	{
		// kL: old code
		if (verticalBlockage(
						_battleSave->getTile(Position(
													tile->getPosition().x,
													tile->getPosition().y,
													_battleSave->getMapSizeZ() - 1)),
						tile,
						DT_NONE) != 0)
		// kL_note: new code
//		int
//			block = 0,
//			x = tile->getPosition().x,
//			y = tile->getPosition().y;
//		for (int
//				z = _save->getMapSizeZ() - 1;
//				z > tile->getPosition().z;
//				--z)
//		{
//			block += blockage(
//							_save->getTile(Position(x, y, z)),
//							O_FLOOR,
//							DT_NONE);
//			block += blockage(
//							_save->getTile(Position(x, y, z)),
//							O_OBJECT,
//							DT_NONE,
//							Pathfinding::DIR_DOWN);
//		}
//		if (block > 0)
		{
			light -= 2;
		}
	}

	tile->addLight(
				light,
				LIGHT_LAYER_AMBIENT);
	//Log(LOG_INFO) << "TileEngine::calculateSunShading() EXIT";
}

/**
 * Recalculates lighting for the terrain: parts, items, fire, flares.
 */
void TileEngine::calculateTerrainLighting() const
{
	for (size_t // reset.
			i = 0u;
			i != _battleSave->getMapSizeXYZ();
			++i)
	{
		_battleSave->getTiles()[i]->resetLight(LIGHT_LAYER_STATIC);
	}

	int light;
	Tile* tile;

	for (size_t
			i = 0u;
			i != _battleSave->getMapSizeXYZ();
			++i)
	{
		light = 0;
		tile = _battleSave->getTiles()[i];

		if (tile->getMapData(O_FLOOR) != nullptr					// add lighting of Floor-part
			&& tile->getMapData(O_FLOOR)->getLightSource() != 0)
		{
			light = tile->getMapData(O_FLOOR)->getLightSource();
		}

		if (tile->getMapData(O_OBJECT) != nullptr					// add lighting of Object-part
			&& tile->getMapData(O_OBJECT)->getLightSource() != 0)
		{
			light = std::max(light,
							 tile->getMapData(O_OBJECT)->getLightSource());
		}

		if (tile->getFire() != 0)									// add lighting from Tile-fire
			light = std::max(light,
							 static_cast<int>(LIGHT_FIRE)); // kludgy STL::nerf

		for (std::vector<BattleItem*>::const_iterator
				j = tile->getInventory()->begin();
				j != tile->getInventory()->end();
				++j)
		{
			if ((*j)->getRules()->getBattleType() == BT_FLARE		// add lighting of battle-flares
				&& (*j)->getFuse() != -1)
			{
				light = std::max(light,
								 (*j)->getRules()->getPower());
			}
		}

		addLight(
				tile->getPosition(),
				light,
				LIGHT_LAYER_STATIC);
	}
}

/**
 * Recalculates lighting for the units.
 */
void TileEngine::calculateUnitLighting() const
{
	for (size_t // reset all light to 0 first
			i = 0u;
			i != _battleSave->getMapSizeXYZ();
			++i)
	{
		_battleSave->getTiles()[i]->resetLight(LIGHT_LAYER_DYNAMIC);
	}

	int light;
	for (std::vector<BattleUnit*>::const_iterator
			i = _battleSave->getUnits()->begin();
			i != _battleSave->getUnits()->end();
			++i)
	{
		light = 0;
		if (_unitLighting == true		// add lighting of player-units
			&& (*i)->getFaction() == FACTION_PLAYER
			&& (*i)->isOut_t(OUT_STAT) == false)
		{
			light = LIGHT_UNIT;
		}

		if ((*i)->getUnitFire() != 0)	// add lighting of any units on fire
			light = std::max(light,
							 static_cast<int>(LIGHT_FIRE)); // kludgy STL::nerf

		addLight(
				(*i)->getPosition(),
				light,
				LIGHT_LAYER_DYNAMIC);
	}
}

/**
 * Toggles personal lighting on/off.
 */
void TileEngine::togglePersonalLighting()
{
	_unitLighting = !_unitLighting;
	calculateUnitLighting();
}

/**
 * Adds a circular light-pattern starting from @a pos and losing power
 * proportional to distance.
 * @param pos	- reference to the center-position in tile-space
 * @param power	- power of light
 * @param layer	- light is separated in 3 layers: Ambient, Static, and Dynamic
 */
void TileEngine::addLight( // private.
		const Position& pos,
		int power,
		size_t layer) const
{
	Tile* tile;
	int light;
	double dZ;

	for (int // loop through the positive quadrant only - reflect that onto the other quadrants.
			x = 0;
			x <= power;
			++x)
	{
		for (int
				y = 0;
				y <= power;
				++y)
		{
			for (int
					z = 0;
					z != _battleSave->getMapSizeZ();
					++z)
			{
				dZ = static_cast<double>(std::abs(pos.z - z)) * 1.5;
				light = power
					  - static_cast<int>(Round(std::sqrt(static_cast<double>(x * x + y * y) + dZ * dZ)));

				if ((tile = _battleSave->getTile(Position(
														pos.x + x,
														pos.y + y,
														z))) != nullptr) tile->addLight(light, layer);

				if ((tile = _battleSave->getTile(Position(
														pos.x - x,
														pos.y - y,
														z))) != nullptr) tile->addLight(light, layer);

				if ((tile = _battleSave->getTile(Position(
														pos.x + x,
														pos.y - y,
														z))) != nullptr) tile->addLight(light, layer);

				if ((tile = _battleSave->getTile(Position(
														pos.x - x,
														pos.y + y,
														z))) != nullptr) tile->addLight(light, layer);
			}
		}
	}
}

/**
 * Calculates FoV vs units for a single BattleUnit.
 * @param unit - pointer to a BattleUnit
 * @return, true if previously concealed units are spotted
 */
bool TileEngine::calcFovUnits(BattleUnit* const unit) const
{
	unit->clearHostileUnits();

	bool
		spotByPlayer  (false),
		spotByHostile (false);

	// NOTE: Lift by terrain-level hasn't been done yet; see before tile-reveals below_

	if (_battleSave->getBattleGame() == nullptr // pre-battle power-source explosion.
		|| _battleSave->getBattleGame()->playerPanicHandled() == true)
	{
		int soundId;
		if (_spotSound == true)
			soundId = unit->getAggroSound();
		else
			soundId = -1;

		Position
			posOther,
			pos;

		int otherSize;

		switch (unit->getFaction())
		{
			case FACTION_PLAYER:
				for (std::vector<BattleUnit*>::const_iterator
						i = _battleSave->getUnits()->begin(), j = _battleSave->getUnits()->end();
						i != j;
						++i)
				{
					if ((*i)->getFaction() != FACTION_PLAYER
						&& (*i)->getUnitTile() != nullptr) // otherUnit is standing.
					{
						posOther = (*i)->getPosition();

						otherSize = (*i)->getArmor()->getSize();
						for (int
								x = 0;
								x != otherSize;
								++x)
						{
							for (int
									y = 0;
									y != otherSize;
									++y)
							{
								pos = posOther + Position(x,y,0);

								if (unit->checkViewSector(pos) == true
									&& visible(unit, _battleSave->getTile(pos)) == true)
								{
									if ((*i)->getUnitVisible() == false)
									{
										(*i)->setUnitVisible();
										spotByPlayer = true; // NOTE: This will halt a player's moving-unit when spotting a new Civie even.
									}

									if ((*i)->getFaction() == FACTION_HOSTILE)
									{
										unit->addToHostileUnits((*i)); // adds spottedUnit to '_hostileUnits' and to '_hostileUnitsThisTurn'

										if (soundId != -1
											&& spotByPlayer == true) // play aggro-sound if non-MC'd [huh] xCom unit spots a not-previously-visible hostile.
//											&& unit->getOriginalFaction() == FACTION_PLAYER	// NOTE: Mind-control zhing clashes with aggroSound; put
										{													// that back to prevent it or pass in isMC-reveal somehow.
											const BattlescapeGame* const battle (_battleSave->getBattleGame());
											battle->getResourcePack()->getSound("BATTLE.CAT", static_cast<unsigned>(soundId))
																		->play(-1, battle->getMap()->getSoundAngle(unit->getPosition()));
											soundId = -1; // play once only.
										}
									}

									x =
									y = otherSize - 1; // stop.
								}
							}
						}
					}
				}
				break;

			case FACTION_HOSTILE:
				for (std::vector<BattleUnit*>::const_iterator
						i = _battleSave->getUnits()->begin(), j = _battleSave->getUnits()->end();
						i != j;
						++i)
				{
					if ((*i)->getFaction() != FACTION_HOSTILE
						&& (*i)->getUnitTile() != nullptr) // otherUnit is standing.
					{
						posOther = (*i)->getPosition();

						otherSize = (*i)->getArmor()->getSize();
						for (int
								x = 0;
								x != otherSize;
								++x)
						{
							for (int
									y = 0;
									y != otherSize;
									++y)
							{
								pos = posOther + Position(x,y,0);

								if (unit->checkViewSector(pos) == true
									&& visible(unit, _battleSave->getTile(pos)) == true)
								{
									spotByHostile = unit->addToHostileUnits(*i); // adds spottedUnit to '_hostileUnits' and to '_hostileUnitsThisTurn'

									if (_battleSave->getSide() == FACTION_HOSTILE)
										(*i)->setExposed();	// NOTE: xCom agents can be seen by enemies but *not* become Exposed.
															// Only potential reactionFire should set them Exposed during xCom's turn.

									x =
									y = otherSize - 1; // stop.
								}
							}
						}
					}
				}
				break;
		}
	}

	switch (unit->getFaction())
	{
		case FACTION_PLAYER:
			return spotByPlayer;

		default:
		case FACTION_HOSTILE:
			return spotByHostile;
	}
}

/**
 * Calculates FoV vs Tiles for a single BattleUnit.
 * @note Only Player faction ever needs to use this; check Faction_Player TRUE
 * before a call.
 * @param unit - pointer to a BattleUnit
 */
void TileEngine::calcFovTiles(const BattleUnit* const unit) const
{
//	unit->clearVisibleTiles();
//	unit->getTile()->setTileVisible();
//	unit->getTile()->setDiscovered(true, 2);

	int dir;
	switch (unit->getTurretType())
	{
		case TRT_NONE: dir = unit->getUnitDirection(); break;
		default:
//			if (Options::battleStrafe == false)
//				dir = unit->getUnitDirection();
//			else
			dir = unit->getTurretDirection();
	}

	bool swapXY;
	switch (dir)
	{
		case 0:
		case 4:  swapXY = true; break;
		default: swapXY = false;
	}

	static const int
		sign_x[8u] { 1, 1, 1, 1,-1,-1,-1,-1},
		sign_y[8u] {-1,-1, 1, 1, 1, 1,-1,-1};

	int
		y1 (0),
		y2 (0),
		unitSize (unit->getArmor()->getSize());
	size_t trjLength;

	bool diag;
	if ((dir & 1) == 1)
	{
		diag = true;
		y2 = SIGHTDIST_TSp;
	}
	else
		diag = false;

	VoxelType blockType;

	std::vector<Position> trj;

	Position
		posUnit (unit->getPosition()),
		posTest,
		posTrj;

	Tile
		* tile,
		* tileEdge;
	const MapData
		* object,
		* objectEdge;

	if (unit->getHeight(true) - _battleSave->getTile(posUnit)->getTerrainLevel() > 31) // arbitrary 24+8, could use Pathfinding::UNIT_HEIGHT
	{
		const Tile* const tileAbove (_battleSave->getTile(posUnit + Position(0,0,1)));
		if (tileAbove != nullptr && tileAbove->hasNoFloor() == true)
			++posUnit.z;
	}

	for (int
			x = 0; // does the unit itself really need checking ... Yes, marks own Tile as discovered.
			x <= SIGHTDIST_TSp;
			++x)
	{
		if (diag == false)
		{
			y1 = -x;
			y2 =  x;
		}

		for (int
				y = y1;
				y <= y2;
				++y)
		{
			for (int
					z = 0;
					z != _battleSave->getMapSizeZ();
					++z)
			{
				posTest.z = z;

				if (x * x + y * y <= SIGHTDIST_TSp_Sqr)
				{
					posTest.x = posUnit.x + (sign_x[dir] * (swapXY ? y : x)); // NOTE: posUnit is a large unit's primary quadrant.
					posTest.y = posUnit.y + (sign_y[dir] * (swapXY ? x : y));

					if (_battleSave->getTile(posTest) != nullptr)
					{
						// this sets tiles to discovered if they are in FoV ->
						// NOTE: Tile visibility is calculated not in voxel-space but tile-space.
						for (int
								dX = 0;
								dX != unitSize;
								++dX)
						{
							for (int
									dY = 0;
									dY != unitSize;
									++dY)
							{
								trj.clear();
								blockType = plotLine(
													posUnit + Position(dX,dY, 0),
													posTest,
													true,
													&trj,
													unit,
													false);
								trjLength = trj.size();

								if (blockType == TRJ_DECREASE) // NOTE: Not a voxel-type here, just a return-value.
									--trjLength;

								for (size_t
										i = 0u;
										i != trjLength;
										++i)
								{
									posTrj = trj.at(i);

									// mark every tile of line as visible ->
									tile = _battleSave->getTile(posTrj);
									if (tile->isRevealed() == false) // NOTE: Keep an eye on this.
									{
										tile->setRevealed();	// sprite caching for floor+content, ergo + west & north walls.
//										tile->setTileVisible();	// Used only by sneakyAI.

										// walls to the east or south of a visible tile, reveal that too
										// NOTE: yeh, If there's walls or an appropriate BigWall object!
										// tile-parts:
										//		#0 - floor
										//		#1 - westwall
										//		#2 - northwall
										//		#3 - object
										// revealable sections:
										//		#0 - westwall
										//		#1 - northwall
										//		#2 - floor + content (reveals both walls also)

										if ((object = tile->getMapData(O_OBJECT)) == nullptr
											|| (object->getBigwall() & 0xa1) == 0)				// [0xa1 = Block/East/ES]
										{
											tileEdge = _battleSave->getTile(Position(			// show Tile EAST
																				posTrj.x + 1,
																				posTrj.y,
																				posTrj.z));
											if (tileEdge != nullptr)
											{
												if ((objectEdge = tileEdge->getMapData(O_OBJECT)) != nullptr
													&& (objectEdge->getBigwall() & 0x9) != 0)	// [0x9 = Block/West]
												{
													tileEdge->setRevealed();					// reveal entire TileEast
												}
												else if (tileEdge->getMapData(O_WESTWALL) != nullptr)
													tileEdge->setRevealed(ST_WEST);				// reveal only westwall
											}
										}

										if (object == nullptr
											|| (object->getBigwall() & 0xc1) == 0)				// [0xc1 = Block/South/ES]
										{
											tileEdge = _battleSave->getTile(Position(			// show Tile SOUTH
																				posTrj.x,
																				posTrj.y + 1,
																				posTrj.z));
											if (tileEdge != nullptr)
											{
												if ((objectEdge = tileEdge->getMapData(O_OBJECT)) != nullptr
													&& (objectEdge->getBigwall() & 0x11) != 0)	// [0x11 = Block/North]
												{
													tileEdge->setRevealed();					// reveal entire TileSouth
												}
												else if (tileEdge->getMapData(O_NORTHWALL) != nullptr)
													tileEdge->setRevealed(ST_NORTH);			// reveal only northwall
											}
										}

										if (tile->getMapData(O_WESTWALL) == nullptr
											&& (object == nullptr
												|| (object->getBigwall() & 0x9) == 0))			// [0x9 = Block/West]
										{
											tileEdge = _battleSave->getTile(Position(			// show Tile WEST
																				posTrj.x - 1,
																				posTrj.y,
																				posTrj.z));
											if (tileEdge != nullptr
												&& (objectEdge = tileEdge->getMapData(O_OBJECT)) != nullptr)
											{
												switch (objectEdge->getBigwall())
												{
													case BIGWALL_BLOCK:
													case BIGWALL_EAST:
													case BIGWALL_E_S:
														tileEdge->setRevealed();				// reveal entire TileWest
												}
											}
										}

										if (tile->getMapData(O_NORTHWALL) == nullptr
											&& (object == nullptr
												|| (object->getBigwall() & 0x11) == 0))			// [0x11 = Block/North]
										{
											tileEdge = _battleSave->getTile(Position(			// show Tile NORTH
																				posTrj.x,
																				posTrj.y - 1,
																				posTrj.z));
											if (tileEdge != nullptr
												&& (objectEdge = tileEdge->getMapData(O_OBJECT)) != nullptr)
											{
												switch (objectEdge->getBigwall())
												{
													case BIGWALL_BLOCK:
													case BIGWALL_SOUTH:
													case BIGWALL_E_S:
														tileEdge->setRevealed();				// reveal entire TileNorth
												}
											}
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}
}

/**
 * Calculates FoV vs units for conscious units within range of a specified Position.
 * @note Used when a unit is walking or terrain has changed which can reveal
 * unseen units and/or parts of terrain. Spotsound triggers:
 * - convert unit
 * - kneel click
 * - hit changes unit/terrain
 * - door opened
 * - psi-control
 * - unit dies
 * - fallBstate
 * - walkBstate end
 * - walkBstate post-path
 * - unit revives
 * @param pos		- reference to the position of the changed unit/terrain
 * @param spotSound	- true to play aggro-sound (default false)
 * @param faction	- faction to calculate for (BattleUnit.h) (default FACTION_NONE)
 */
void TileEngine::calcFovUnits_pos(
		const Position& pos,
		bool spotSound,
		UnitFaction faction)
{
	_spotSound = spotSound;
	for (std::vector<BattleUnit*>::const_iterator
			i = _battleSave->getUnits()->begin(), j = _battleSave->getUnits()->end();
			i != j;
			++i)
	{
		if ((*i)->getUnitTile() != nullptr
			&& ((*i)->getFaction() == faction
				|| ((*i)->getFaction() != FACTION_NEUTRAL && faction == FACTION_NONE))
			&& distSqr((*i)->getPosition(), pos) <= SIGHTDIST_TSp_Sqr)
		{
			calcFovUnits(*i);
		}
	}
	_spotSound = true;
}

/**
 * Calculates FoV vs Tiles for conscious units within range of a specified Position.
 * @note Used when a unit is walking or terrain has changed which can reveal
 * unseen units and/or parts of terrain.
 */
void TileEngine::calcFovTiles_pos(const Position& pos)
{
	for (std::vector<BattleUnit*>::const_iterator
			i = _battleSave->getUnits()->begin(), j = _battleSave->getUnits()->end();
			i != j;
			++i)
	{
		if ((*i)->getFaction() == FACTION_PLAYER
			&& (*i)->getUnitTile() != nullptr
			&& distSqr((*i)->getPosition(), pos) <= SIGHTDIST_TSp_Sqr)
		{
			calcFovTiles(*i);
		}
	}
}

/**
 * Calculates FoV vs units for all conscious BattleUnits on the battlefield.
 * @note Spotsound triggers:
 * - drop an item
 * - explosion changes unit/terrain
 * @param spotSound - true to play aggro-sound (default false)
 */
void TileEngine::calcFovUnits_all(bool spotSound)
{
	_spotSound = spotSound;
	for (std::vector<BattleUnit*>::const_iterator
			i = _battleSave->getUnits()->begin(), j = _battleSave->getUnits()->end();
			i != j;
			++i)
	{
		if ((*i)->getUnitTile() != nullptr
			&& (*i)->getFaction() != FACTION_NEUTRAL)
		{
			calcFovUnits(*i);
		}
	}
	_spotSound = true;
}

/**
 * Calculates FoV vs Tiles for all conscious BattleUnits on the battlefield.
 */
void TileEngine::calcFovTiles_all()
{
	for (std::vector<BattleUnit*>::const_iterator
			i = _battleSave->getUnits()->begin(), j = _battleSave->getUnits()->end();
			i != j;
			++i)
	{
		if ((*i)->getFaction() == FACTION_PLAYER
			&& (*i)->getUnitTile() != nullptr)
		{
			calcFovTiles(*i);
		}
	}
}

/**
 * Checks visibility of a BattleUnit on @a tile to @a unit.
 * @param unit - pointer to a BattleUnit that's looking at @a tile
 * @param tile - pointer to a Tile that @a unit is looking at
 * @return, true if a BattleUnit on @a tile is seen
 */
bool TileEngine::visible(
		const BattleUnit* const unit,
		const Tile* const tile) const
{
	//return false;
	//bool debug (false);
	if (tile != nullptr)
	{
		//debug = unit->getId() == 1000020 || unit->getId() == 167;
		//if (debug) Log(LOG_INFO) << "TileEngine::visible() id-" << unit->getId();

		const BattleUnit* const targetUnit (getTargetUnit(tile));
		if (targetUnit != nullptr && targetUnit->getUnitTile() != nullptr) //targetUnit->isOut_t() == false)
		{
			//Log(LOG_INFO) << ". try to sight id-" << targetUnit->getId();

			if (unit->getFaction() == targetUnit->getFaction())
			{
				//Log(LOG_INFO) << ". . faction - ret TRUE";
				return true;
			}

			const int sqrDist (distSqr(
									unit->getPosition(),
									targetUnit->getPosition()));
			//if (debug) Log(LOG_INFO) << ". distSqr= " << sqrDist;

			if (sqrDist <= SIGHTDIST_TSp_Sqr + unit->getArmor()->getSize())	// include armor-size to account for vagaries
			{																// between tile-space and voxel-space
				//if (debug) Log(LOG_INFO) << ". passed SIGHTDIST_TSp_Sqr";
				switch (unit->getFaction())
				{
					case FACTION_PLAYER:
					case FACTION_NEUTRAL:
						if (tile->getShade() > MAX_SHADE_TO_SEE_UNITS)
						{
							const int shade (_battleSave->getTacticalShade());
							if (sqrDist > SIGHTDIST_TSp_Sqr + 9 - (shade * shade))
								break;
						}
						//if (debug) Log(LOG_INFO) << ". passed SIGHTDIST_TSp_Sqr + 9 - shade^2";
						// no break;

					case FACTION_HOSTILE:
					{
						const Position originVoxel (getSightOriginVoxel(unit));
						Position scanVoxel;
						if (canTargetUnit(
										&originVoxel,
										tile,
										&scanVoxel,
										unit) == true)
						{
							//if (debug) Log(LOG_INFO) << ". . CanTargetUnit() TRUE";
							std::vector<Position> trj;
							plotLine(
									originVoxel,
									scanVoxel,
									true,
									&trj,
									unit);

							float distLimit (static_cast<float>(trj.size()));
							const Tile* tileScan (nullptr);

							for (size_t
									i = 0u;
									i != trj.size();
									++i)
							{
								tileScan = _battleSave->getTile(Position::toTileSpace(trj.at(i)));

								distLimit += static_cast<float>(tileScan->getSmoke() + tileScan->getFire()) / 3.f;
								if (static_cast<int>(std::ceil(distLimit * distLimit)) > SIGHTDIST_VSp_Sqr)
								{
									//if (debug) Log(LOG_INFO) << ". . . failed SIGHTDIST_VSp_Sqr - ret FALSE";
									return false;
								}
							}

							if (tileScan != nullptr // safety.
								&& getTargetUnit(tileScan) == targetUnit)
							{
								//if (debug) Log(LOG_INFO) << ". . Tile has targetUnit - ret TRUE";
								return true;
							}
						}
					}
				}
			}
		}
	}
	//if (debug) Log(LOG_INFO) << ". ret FALSE";
	return false;
}

/**
 * Gets a valid target-unit given a Tile.
 * @param tile - pointer to a tile
 * @return, pointer to a unit (nullptr if none)
 */
const BattleUnit* TileEngine::getTargetUnit(const Tile* const tile) const	// now I love const; it does absolutely nothing other than
{																			// cause problems and make a pretty blue color in the IDE.
	if (tile != nullptr) // safety for canTargetUnit().
	{
		if (tile->getTileUnit() != nullptr) // warning: Careful not to use this when UnitWalkBState has transient units placed.
			return tile->getTileUnit();

		if (tile->getPosition().z > 0 && tile->hasNoFloor() == true)
		{
			const Tile* const tileBelow (_battleSave->getTile(tile->getPosition() + Position(0,0,-1)));
			if (tileBelow->getTileUnit() != nullptr)
				return tileBelow->getTileUnit();
		}
	}
	return nullptr;
}

/**
 * Gets the origin-voxel of a unit's LoS.
 * @param unit	- pointer to the watcher
 * @param pos	- pointer to a hypothetical Position (default nullptr to use @a unit's current position)
 * @return, approximately an eyeball-voxel
 */
Position TileEngine::getSightOriginVoxel(
		const BattleUnit* const unit,
		const Position* pos) const
{
	if (pos == nullptr)
		pos = &unit->getPosition();

	Position originVoxel (Position::toVoxelSpaceCentered(	// TODO: Large units get an origin in the very northwest corner of quadrant #4.
													*pos,	// It will not be accurate.
													unit->getHeight(true) + EYE_OFFSET
														- _battleSave->getTile(*pos)->getTerrainLevel(),
													unit->getArmor()->getSize()));
	const int ceilingZ ((*pos).z * 24 + 23);
	if (ceilingZ < originVoxel.z)
	{
		const Tile* const tileAbove (_battleSave->getTile(*pos + Position(0,0,1)));
		if (tileAbove == nullptr || tileAbove->hasNoFloor() == false)
			originVoxel.z = ceilingZ; // careful with that ceiling, Eugene.
	}

	return originVoxel;
}

/**
 * Gets the origin-voxel of a shot/throw or missile.
 * @param action	- reference to a BattleAction
 * @param tile		- pointer to a start-tile (default nullptr)
 * @return, position of the origin in voxel-space
 */
Position TileEngine::getOriginVoxel(
		const BattleAction& action,
		const Tile* const tile) const
{
	if (action.type == BA_LAUNCH)
	{
		Position pos;
		if (tile != nullptr)
			pos = tile->getPosition();
		else
			pos = action.actor->getUnitTile()->getPosition();

		if (action.actor->getPosition() != pos)				// NOTE: Don't consider unit height or terrain-level if the Prj
			return Position::toVoxelSpaceCentered(pos, 16);	// is not being launched - ie, if it originates from a waypoint.
	}

	return getSightOriginVoxel(action.actor);
}
/*	const int dirYshift[8] = {1, 1, 8,15,15,15, 8, 1};
	const int dirXshift[8] = {8,14,15,15, 8, 1, 1, 1};
	int dir = getDirectionTo(pos, action.target);
	originVoxel.x += dirXshift[dir] * action.actor->getArmor()->getSize();
	originVoxel.y += dirYshift[dir] * action.actor->getArmor()->getSize(); */

/**
 * Checks for a BattleUnit available for targeting and at what particular voxel.
 * @param originVoxel	- pointer to voxel of trace origin (eg. gun's barrel)
 * @param tileTarget	- pointer to Tile to check against
 * @param scanVoxel		- pointer to voxel that is returned coordinate of hit
 * @param excludeUnit	- pointer to unitSelf (to not hit self)
 * @param targetUnit	- pointer to a hypothetical unit to draw a virtual LoF
 *						  for AI-ambush usage; if left nullptr this function
 *						  behaves normally (default nullptr to use @a tileTarget's
 *						  current unit)
 * @param force			- pointer to bool that stores whether shot should be
 *						  automatically verified in Projectile instantiation (default nullptr not used)
 * @return, true if a unit can be targeted in @a tileTarget
 */
bool TileEngine::canTargetUnit(
		const Position* const originVoxel,
		const Tile* const tileTarget,
		Position* const scanVoxel,
		const BattleUnit* const excludeUnit,
		const BattleUnit* targetUnit,
		bool* const force) const
{
/*	static bool debugged = false;
	bool debug;
	if (debugged == false
		&& _battleSave->getTile(Position::toTileSpace(*originVoxel))->getTileUnit() != nullptr
		&& _battleSave->getTile(Position::toTileSpace(*originVoxel))->getTileUnit()->getId() == 388
//		&& _battleSave->getSelectedUnit() != nullptr
//		&& _battleSave->getSelectedUnit()->getId() == 189
		&& tileTarget->getPosition() == Position(28,18,0))
	{
		debugged =
		debug = true;
		Log(LOG_INFO) << "";
		Log(LOG_INFO) << "TileEngine::canTargetUnit() shooter id-" << _battleSave->getSelectedUnit()->getId();
	}
	else debug = false; */

	int
		offsetX,
		offsetY;

	bool hypothetical;
	if (targetUnit == nullptr)
	{
//		if ((targetUnit = tileTarget->getTileUnit()) == nullptr)
		if ((targetUnit = getTargetUnit(tileTarget)) == nullptr)
		{
			//if (debug) Log(LOG_INFO) << ". no Unit, ret FALSE";
			return false; // no unit in the tileTarget even if it's elevated and appearing in it. TODO: Could use getTargetUnit().
		}
		hypothetical = false;
		offsetX =
		offsetY = 0;
	}
	else
	{
		hypothetical = true;
		offsetX = targetUnit->getPosition().x - tileTarget->getPosition().x;
		offsetY = targetUnit->getPosition().y - tileTarget->getPosition().y;
	}

	if (targetUnit == excludeUnit)
	{
		//if (debug) Log(LOG_INFO) << ". hit vs Self, ret FALSE";
		return false;
	}

/*	if (debug)
	{
		Log(LOG_INFO) << ". offsetX = " << offsetX;
		Log(LOG_INFO) << ". offsetY = " << offsetY;
	} */

	float radius; // radius = LoFT-id
	if (targetUnit->getArmor()->getSize() == 1)
		radius = static_cast<float>(targetUnit->getLoft());
//		radius = static_cast<float>(targetUnit->getLoft() * 2u + 1u); // <- this is the actual voxel-dimension in the LoFT.
	else
		radius = 3.f;
	//if (debug) Log(LOG_INFO) << ". radius= " << radius;

	const Position
		targetVoxel (Position::toVoxelSpaceCentered(tileTarget->getPosition())),
		relVoxel (targetVoxel - *originVoxel);
	const float theta (radius / std::sqrt(static_cast<float>(relVoxel.x * relVoxel.x + relVoxel.y * relVoxel.y)));
	const int
//		relX (static_cast<int>(Round(static_cast<float>( relVoxel.y) * theta))),
//		relY (static_cast<int>(Round(static_cast<float>(-relVoxel.x) * theta))),
		relX (static_cast<int>(std::ceil(static_cast<float>( relVoxel.y) * theta))),
		relY (static_cast<int>(std::ceil(static_cast<float>(-relVoxel.x) * theta))),
		scanOffsetXY[10u]
		{
			 0,		 0,
			 relX,	 relY,
			-relX,	-relY,
			 relY,	-relX,
			-relY,	 relX
		};

/*	if (debug)
	{
		Log(LOG_INFO) << ". originVoxel " << *originVoxel << " ts " << (*originVoxel / Position(16,16,24));
		Log(LOG_INFO) << ". targetVoxel " << targetVoxel << " ts " << (targetVoxel / Position(16,16,24));
		Log(LOG_INFO) << ". relVoxel " << relVoxel << " ts " << (relVoxel / Position(16,16,24));
		Log(LOG_INFO) << ". theta= " << theta;
		Log(LOG_INFO) << ". relX= " << relX;
		Log(LOG_INFO) << ". relY= " << relY;
	} */

	const int
		targetLow (targetVoxel.z
				 - tileTarget->getTerrainLevel()
				 + targetUnit->getFloatHeight());
	int
		targetHigh (targetLow),
		targetMid,
		height;

	if (targetUnit->isOut_t(OUT_STAT) == false)
		height = targetUnit->getHeight();
	else
		height = 12; // whats this even for.

	targetHigh += (height - 1);
	targetMid = (targetHigh + targetLow) >> 1u;
	height = std::min(11, height >> 1u); // don't exceed array-size of scanOffsetZ[]

/*	if (debug)
	{
		Log(LOG_INFO) << ". height= " << height;
		Log(LOG_INFO) << ". targetHigh= " << targetHigh;
		Log(LOG_INFO) << ". targetMid= " << targetMid;
		Log(LOG_INFO) << ". targetLow= " << targetLow;
	} */

	std::vector<Position> trj;
	std::vector<int>
		targetable_x,
		targetable_y,
		targetable_z;

	VoxelType voxelType;

	for (size_t // scan from a horizontal center-line up and down using scanOffsetZ[]
			i = 0u;
			i != static_cast<size_t>(height);
			++i)
	{
		if ((scanVoxel->z = targetMid + scanOffsetZ[i]) > -1) // account for probable truncation of (int)'targetMid'.
		{
/*			if (debug)
			{
				Log(LOG_INFO) << "";
				Log(LOG_INFO) << ". . i= " << i << " scan.Z= " << scanVoxel->z << " (" << (scanVoxel->z / 24) << ")";
			} */

			for (size_t // scan from vertical center-line outwards left and right using scanOffsetXY[]
					j = 0u;
					j != 5u;
					++j)
			{
				scanVoxel->x = targetVoxel.x + scanOffsetXY[j * 2u];
				scanVoxel->y = targetVoxel.y + scanOffsetXY[j * 2u + 1u];
/*				if (debug)
				{
					Log(LOG_INFO) << ". . . j= " << j;
					Log(LOG_INFO) << ". . . scan.X= " << scanVoxel->x << " (" << (scanVoxel->x >> 4) << ")";
					Log(LOG_INFO) << ". . . scan.Y= " << scanVoxel->y << " (" << (scanVoxel->y >> 4) << ")";
				} */

				trj.clear();
				voxelType = plotLine(
									*originVoxel,
									*scanVoxel,
									false,
									&trj,
									excludeUnit);
/*				if (debug)
				{
					if (trj.empty() == false) Log(LOG_INFO) << ". . . . impact " << trj.at(0u) << " ts " << (trj.at(0u) / Position(16,16,24));
					Log(LOG_INFO) << ". . . . impactType= " << MapData::debugVoxelType(voxelType);
				} */

				switch (voxelType)
				{
					case VOXEL_UNIT:
//						for (int // voxel of hit must be inside of scanned tileTarget(s)
//								x = 0;
//								x <= unitSize;
//								++x)
//						{
//							if (debug) Log(LOG_INFO) << ". . . . iterate x-Size";
//							for (int
//									y = 0;
//									y <= unitSize;
//									++y)
//						{
//							if (debug) Log(LOG_INFO) << ". . . . . iterate y-Size";
						if (   (trj.at(0u).x >> 4) == (scanVoxel->x >> 4) /*+ x*/ + offsetX
							&& (trj.at(0u).y >> 4) == (scanVoxel->y >> 4) /*+ y*/ + offsetY
							&&  trj.at(0u).z >= targetLow
							&&  trj.at(0u).z <= targetHigh)
						{
							//if (debug) Log(LOG_INFO) << ". . . . . . TargetUnit found @ scanVoxel " << (*scanVoxel);
							targetable_x.push_back(scanVoxel->x);
							targetable_y.push_back(scanVoxel->y);
							targetable_z.push_back(scanVoxel->z);
						}
//							}
//						}
						break;

					case VOXEL_EMPTY:
						if (hypothetical == true && trj.empty() == false)
						{
							//if (debug) Log(LOG_INFO) << ". . . hypothetical Found, ret TRUE";
							return true;
						}
				}
			}
		}
	}

	if (targetable_x.empty() == false) // find the voxel centered on targetable area
	{
		int
			minVal (*std::min_element(targetable_x.begin(), targetable_x.end())),
			maxVal (*std::max_element(targetable_x.begin(), targetable_x.end()));
		const int target_x ((minVal + maxVal) >> 1u);

		minVal = *std::min_element(targetable_y.begin(), targetable_y.end()),
		maxVal = *std::max_element(targetable_y.begin(), targetable_y.end());
		const int target_y ((minVal + maxVal) >> 1u);

		minVal = *std::min_element(targetable_z.begin(), targetable_z.end()),
		maxVal = *std::max_element(targetable_z.begin(), targetable_z.end());
		const int target_z ((minVal + maxVal) >> 1u);

		*scanVoxel = Position(target_x, target_y, target_z);
		if (force != nullptr) *force = true;

		//if (debug) Log(LOG_INFO) << ". scanVoxel RET " << (*scanVoxel) << " " << (*scanVoxel / Position(16,16,24)) << " area=" << targetable_x.size();
		return true;
	}

	// Couldn't establish LoF so just set *scanVoxel to the centre of the target
	// in case caller wants to use it anyway -- See ProjectileFlyBState::init().
//	*scanVoxel = Position::toVoxelSpaceCentered(tileTarget->getPosition(), targetMid);
	// kL_note: Except I'm not so sure that I haven't fixed the glitch. If I
	// have that could throw my fix off again.

	//if (debug) Log(LOG_INFO) << "TileEngine::canTargetUnit() exit FALSE";
	return false;
}

/**
 * Checks for a tile-part available for targeting and what particular voxel.
 * @param originVoxel	- pointer to the Position voxel of trace origin (eg. gun's barrel)
 * @param tileTarget	- pointer to the Tile to check against
 * @param tilePart		- tile-part to check for (MapData.h)
 * @param scanVoxel		- pointer to voxel that is returned coordinate of hit
 * @param excludeUnit	- pointer to unitSelf (to not hit self)
 * @return, true if tile-part can be targeted
 */
bool TileEngine::canTargetTilepart(
		const Position* const originVoxel,
		const Tile* const tileTarget,
		const MapDataType tilePart,
		Position* const scanVoxel,
		const BattleUnit* const excludeUnit) const
{
	static const int
		objectSpiral[82u]
		{
			8,8,  8,6, 10,6, 10,8, 10,10, 8,10,  6,10,  6,8,  6,6,											// first circle
			8,4, 10,4, 12,4, 12,6, 12,8, 12,10, 12,12, 10,12, 8,12, 6,12, 4,12, 4,10, 4,8, 4,6, 4,4, 6,4,	// second circle
			8,1, 12,1, 15,1, 15,4, 15,8, 15,12, 15,15, 12,15, 8,15, 4,15, 1,15, 1,12, 1,8, 1,4, 1,1, 4,1	// third circle
		},
		northSpiral[14u]
		{
			7,0, 9,0, 6,0, 11,0, 4,0, 13,0, 2,0
		},
		westSpiral[14u]
		{
			0,7, 0,9, 0,6, 0,11, 0,4, 0,13, 0,2
		};

	Position targetVoxel (Position::toVoxelSpace(tileTarget->getPosition()));

	std::vector<Position> trj;

	const int* spiral;
	int
		zMin,
		zMax;
	size_t spiralSize;
	bool
		foundMinZ,
		foundMaxZ;

	switch (tilePart)
	{
		case O_FLOOR:
			spiral = objectSpiral;
			spiralSize = 41u;
			foundMinZ =
			foundMaxZ = true;
			zMin =
			zMax = 0;
			break;

		case O_WESTWALL:
			spiral = westSpiral;
			spiralSize = 7u;
			foundMinZ =
			foundMaxZ = false;
			break;

		case O_NORTHWALL:
			spiral = northSpiral;
			spiralSize = 7u;
			foundMinZ =
			foundMaxZ = false;
			break;

		case O_OBJECT:
			spiral = objectSpiral;
			spiralSize = 41u;
			foundMinZ =
			foundMaxZ = false;
			break;

		default:
			//Log(LOG_INFO) << "TileEngine::canTargetTilepart() EXIT, ret False (tilePart is O_NULPART)";
			return false;
	}


	for (int // find out height range
			j = 1;
			j != 12 && foundMinZ == false;
			++j)
	{
		for (size_t
				i = 0u;
				i != spiralSize && foundMinZ == false;
				++i)
		{
			int
				tX (spiral[i * 2u]),
				tY (spiral[i * 2u + 1u]);

			if (voxelCheck(
						Position(
								targetVoxel.x + tX,
								targetVoxel.y + tY,
								targetVoxel.z + j * 2),
						nullptr,
						true) == static_cast<VoxelType>(tilePart)) // bingo. Note MapDataType & VoxelType correspond.
			{
				zMin = j * 2;
				foundMinZ = true;
			}
		}
	}
	//Log(LOG_INFO) << "canTargetTilepart minZ = " << zMin;

	if (foundMinZ == false)
		return false; // empty object!!!

	for (int
			j = 10;
			j != -1 && foundMaxZ == false;
			--j)
	{
		for (size_t
				i = 0u;
				i != spiralSize && foundMaxZ == false;
				++i)
		{
			int
				tX (spiral[i * 2u]),
				tY (spiral[i * 2u + 1u]);

			if (voxelCheck(
						Position(
								targetVoxel.x + tX,
								targetVoxel.y + tY,
								targetVoxel.z + j * 2),
						nullptr,
						true) == static_cast<VoxelType>(tilePart)) // bingo. Note MapDataType & VoxelType correspond.
			{
				zMax = j * 2;
				foundMaxZ = true;
			}
		}
	}

	//Log(LOG_INFO) << "canTargetTilepart maxZ = " << zMax;
	if (foundMaxZ == true)
	{
		if (zMin > zMax) zMin = zMax;

		const size_t zRange (static_cast<size_t>(std::min(11, // stay within bounds of scanOffsetZ[].
														  zMax - zMin + 1)));
		const int zCenter ((zMax + zMin) / 2);

		VoxelType voxelType;

		for (size_t
				j = 0u;
				j != zRange;
				++j)
		{
			scanVoxel->z = targetVoxel.z + zCenter + scanOffsetZ[j];
			//Log(LOG_INFO) << "j=" << j << " targetVoxel.z = " << targetVoxel.z << " zCenter = " << zCenter << " heightCenter = " << scanOffsetZ[j];
			//Log(LOG_INFO) << "scanVoxel.z = " << scanVoxel->z;

			for (size_t
					i = 0u;
					i != spiralSize;
					++i)
			{
				scanVoxel->x = targetVoxel.x + spiral[i * 2u];
				scanVoxel->y = targetVoxel.y + spiral[i * 2u + 1u];

				trj.clear();
				voxelType = plotLine(
									*originVoxel,
									*scanVoxel,
									false,
									&trj,
									excludeUnit);
				if (voxelType == static_cast<VoxelType>(tilePart)								// bingo. MapDataType & VoxelType correspond
					&& Position::toTileSpace(trj.at(0u)) == Position::toTileSpace(*scanVoxel))	// so do Tiles.
				{
					//Log(LOG_INFO) << "ret TRUE";
					return true;
				}
			}
		}
	}

	//Log(LOG_INFO) << "ret FALSE";
	return false;
}

/**
 * Checks for how exposed unit is to another unit.
 * @param originVoxel	- voxel of trace origin (eye or gun's barrel)
 * @param tile			- pointer to Tile to check against
 * @param excludeUnit	- is self (not to hit self)
 * @param excludeAllBut	- [optional] is unit which is the only one to be considered for ray hits
 * @return, degree of exposure (as percent)
 *
int TileEngine::checkVoxelExposure(
		Position* originVoxel,
		Tile* tile,
		BattleUnit* excludeUnit,
		BattleUnit* excludeAllBut)
{
	Position targetVoxel = Position(
			tile->getPosition().x * 16 + 7,
			tile->getPosition().y * 16 + 8,
			tile->getPosition().z * 24);
	Position scanVoxel;
	std::vector<Position> _trajectory;

	BattleUnit* otherUnit = tile->getTileUnit();

	if (otherUnit == 0)
		return 0; // no unit in this tile, even if it elevated and appearing in it.

	if (otherUnit == excludeUnit)
		return 0; // skip self

	int targetMinHeight = targetVoxel.z - tile->getTerrainLevel();
	if (otherUnit)
		 targetMinHeight += otherUnit->getFloatHeight();

	// if there is an other unit on target tile, we assume we want to check against this unit's height
	int heightRange;

	int unitRadius = otherUnit->getLoft(); // width == loft in default loftemps set
	if (otherUnit->getArmor()->getSize() > 1)
	{
		unitRadius = 3;
	}

	// vector manipulation to make scan work in view-space
	Position relPos = targetVoxel - *originVoxel;
	float normal = static_cast<float>(unitRadius) / sqrt(static_cast<float>(relPos.x * relPos.x + relPos.y * relPos.y));
	int relX = static_cast<int>(floor(static_cast<float>(relPos.y) * normal + 0.5f));
	int relY = static_cast<int>(floor(static_cast<float>(-relPos.x) * normal + 0.5f));

	int targetSlices[10] = // looks like [6] to me.. it is 6. so what this function isn't used.
	{
		0,		0,
		relX,	relY,
		-relX,	-relY
	};
//	int targetSlices[10] = // taken from "canTargetUnit()"
//	{
//		0,		0,
//		relX,	relY,
//		-relX,	-relY,
//		relY,	-relX,
//		-relY,	relX
//	};

	if (!otherUnit->isOut())
	{
		heightRange = otherUnit->getHeight();
	}
	else
	{
		heightRange = 12;
	}

	int targetMaxHeight = targetMinHeight+heightRange;
	// scan ray from top to bottom plus different parts of target cylinder
	int total = 0;
	int visible = 0;

	for (int i = heightRange; i >= 0; i -= 2)
	{
		++total;

		scanVoxel.z = targetMinHeight + i;
		for (int j = 0; j != 3; ++j)
		{
			scanVoxel.x = targetVoxel.x + targetSlices[j * 2];
			scanVoxel.y = targetVoxel.y + targetSlices[j * 2 + 1];

			_trajectory.clear();

			int test = plotLine(*originVoxel, scanVoxel, false, &_trajectory, excludeUnit, true, false, excludeAllBut);
			if (test == VOXEL_UNIT)
			{
				// voxel of hit must be inside of scanned box
				if (_trajectory.at(0).x / 16 == scanVoxel.x / 16
					&& _trajectory.at(0).y / 16 == scanVoxel.y / 16
					&& _trajectory.at(0).z >= targetMinHeight
					&& _trajectory.at(0).z <= targetMaxHeight)
				{
					++visible;
				}
			}
		}
	}

	return visible * 100 / total;
} */

/**
 * Checks if a 'sniper' from the opposing faction sees this unit.
 * @note The unit with the highest reaction score will be compared with the
 * triggering unit's reaction score. If it's higher a shot is fired when enough
 * time units plus a weapon and ammo are available.
 * @note The tuSpent parameter is needed because popState() doesn't subtract TU
 * until after the Initiative has been calculated or called from
 * ProjectileFlyBState.
 * @param triggerUnit	- pointer to a unit to check RF against
 * @param tuSpent		- the unit's triggering expenditure of TU if firing or throwing (default 0)
 * @param autoSpot		- true if RF was not triggered by a melee atk (default true)
 * @return, true if reaction fire took place
 */
bool TileEngine::checkReactionFire(
		BattleUnit* const triggerUnit,
		int tuSpent,
		bool autoSpot)
{
	//Log(LOG_INFO) << "";
	//Log(LOG_INFO) << "TileEngine::checkReactionFire() vs id-" << triggerUnit->getId();
	//Log(LOG_INFO) << ". tuSpent = " << tuSpent;
	bool ret (false);

	if (_battleSave->getSide() != FACTION_NEUTRAL						// no reaction on civilian turn.
		&& triggerUnit->getFaction() == _battleSave->getSide()			// spotted unit must be current side's faction
		&& triggerUnit->getUnitTile() != nullptr						// and must be on map
		&& triggerUnit->isOut_t(OUT_HLTH_STUN) == false					// and must be conscious
//		&& _battleSave->getBattleGame()->playerPanicHandled() == true)	// and ... nahhh. Note that doesn't affect aLien RF anyway.
		&& (triggerUnit->getFaction() == FACTION_PLAYER					// Mc'd aLiens do not RF and Xcom does not RF on Mc'd Xcom-units.
			|| triggerUnit->isMindControlled() == false))
	{
		// NOTE: If RF is triggered by melee (or walking/kneeling), a target that is
		// a potential RF-spotter will not be damaged yet and hence the damage +
		// checkCasualties() happens later; but if RF is triggered by a firearm,
		// a target that is a potential RF-spotter *will* be damaged when this runs
		// since damage + checkCasualties() has already been called. fucko*
		//Log(LOG_INFO) << ". Target = VALID";
		std::vector<BattleUnit*> spotters (getSpottingUnits(triggerUnit));
		//Log(LOG_INFO) << ". # spotters = " << spotters.size();

		BattleUnit* reactorUnit (getReactor( // get the first actor up to bat.
										spotters,
										triggerUnit,
										tuSpent,
										autoSpot));
		// start iterating through the possible reactors until
		// the current unit is the one with the highest score.
		while (reactorUnit != triggerUnit)
		{
			// !!!!!SHOOT!!!!!!!!
			if (reactionShot(reactorUnit, triggerUnit) == false)
			{
				//Log(LOG_INFO) << ". . no Snap by id-" << reactorUnit->getId();
				// can't make a reaction shot for whatever reason then boot this guy from the vector.
				for (std::vector<BattleUnit*>::const_iterator
						i = spotters.begin();
						i != spotters.end();
						++i)
				{
					if (*i == reactorUnit)
					{
						spotters.erase(i);
						break;
					}
				}
			}
			else
			{
				//Log(LOG_INFO) << ". . Snap by id-" << reactorUnit->getId();
				ret = true;

				if (reactorUnit->getGeoscapeSoldier() != nullptr
					&& reactorUnit->isMindControlled() == false)
				{
					//Log(LOG_INFO) << ". . reactionXP to " << reactorUnit->getId();
					reactorUnit->addReactionExp();
				}
			}

			reactorUnit = getReactor( // nice shot, Buckwheat.
								spotters,
								triggerUnit,
								tuSpent,
								autoSpot);
			//Log(LOG_INFO) << ". . NEXT AT BAT id-" << reactorUnit->getId();
		}
//		spotters.clear();
	}

	if (ret == false)
		_battleSave->rfTriggerOffset(Position(0,0,-1));

	return ret;
}

/**
 * Creates a vector of BattleUnits that can spot @a unit.
 * @param unit - pointer to a BattleUnit to spot
 * @return, vector of pointers to BattleUnits that can see the trigger-unit
 */
std::vector<BattleUnit*> TileEngine::getSpottingUnits(const BattleUnit* const unit)
{
	//Log(LOG_INFO) << "TileEngine::getSpottingUnits() vs. id-" << unit->getId() << " " << unit->getPosition();
	const Tile* const tile (unit->getUnitTile());
	std::vector<BattleUnit*> spotters;

	for (std::vector<BattleUnit*>::const_iterator
			i = _battleSave->getUnits()->begin();
			i != _battleSave->getUnits()->end();
			++i)
	{
		//Log(LOG_INFO) << ". check id-" << (*i)->getId();
		if (   (*i)->getFaction() != _battleSave->getSide()
			&& (*i)->getFaction() != FACTION_NEUTRAL
			&& (*i)->getUnitStatus() == STATUS_STANDING
			&& (*i)->getTu() != 0)
//			&& (*i)->getSpawnType().empty() == true)
		{
			if ((((*i)->getFaction() == FACTION_HOSTILE							// Mc'd xCom units will RF on loyal xCom units
						&& (*i)->getOriginalFaction() != FACTION_PLAYER			// but Mc'd aLiens won't RF on other aLiens ...
						&& (*i)->isZombie() == false)
					|| (((*i)->getOriginalFaction() == FACTION_PLAYER			// Also - aLiens get to see in all directions
							|| (*i)->isZombie() == true)
						&& (*i)->checkViewSector(unit->getPosition()) == true))	// but xCom & zombies must checkViewSector() even when MC'd
				&& visible(*i, tile) == true)
			{
				//Log(LOG_INFO) << ". . add spotter " << (*i)->getPosition();
				//Log(LOG_INFO) << ". . distSqr= " << distSqr(unit->getPosition(), (*i)->getPosition());
				spotters.push_back(*i);
			}
		}
	}
	return spotters;
}

/**
 * Gets the unit with the highest reaction-score from the spotters-vector.
 * @param spotters	- vector of the pointers to spotting BattleUnits
 * @param defender	- pointer to the defending BattleUnit to check reaction
 *					  scores against
 * @param tuSpent	- defending BattleUnit's expenditure of TU that had caused
 *					  reaction checks. This is needed because popState() doesn't
 *					  subtract TU until after the initiative has been calculated
 *					  and shots have been done by ProjectileFlyBState.
 * @param autoSpot	- true if RF was triggered by a projectile-shot, ie. NOT a
 *					  melee-atk. This is so that if an aLien in the spotters-
 *					  vector gets put down by the trigger-shot it won't tell its
 *					  buds. (default true)
 * @return, pointer to the BattleUnit with initiative (next up!)
 */
BattleUnit* TileEngine::getReactor(
		std::vector<BattleUnit*> spotters,
		BattleUnit* const defender,
		const int tuSpent,
		bool autoSpot) const
{
	//Log(LOG_INFO) << "TileEngine::getReactor() vs id-" << defender->getId();
	//Log(LOG_INFO) << ". tuSpent = " << tuSpent;
	BattleUnit* nextReactor (nullptr);
	int
		init (-1),
		initTest;

	for (std::vector<BattleUnit*>::const_iterator
			i = spotters.begin();
			i != spotters.end();
			++i)
	{
		//Log(LOG_INFO) << ". . test spotter id-" << (*i)->getId() << " initi= " << (*i)->getInitiative();
		if ((*i)->isOut_t() == false
			&& (initTest = (*i)->getInitiative()) > init)
		{
			init = initTest;
			nextReactor = *i;
		}
	}

	//Log(LOG_INFO) << ". trigger id-" << defender->getId() << " initi= " << defender->getInitiative(tuSpent);

	// nextReactor has to *best* defender.Init to get initiative
	// Analysis: It appears that defender's tu for firing/throwing
	// are not subtracted before getInitiative() is called.

	if (nextReactor == nullptr
		|| init <= defender->getInitiative(tuSpent))
	{
		nextReactor = defender;
	}

	if (nextReactor != defender
		&& nextReactor->getFaction() == FACTION_HOSTILE)
	{
		//Log(LOG_INFO) << "getReactor() id-" << nextReactor->getId() << " spots id-" << defender->getId();
		if (autoSpot == true)
		{
			//Log(LOG_INFO) << ". after a trajectory-shot";
			defender->setExposed();								// defender has been spotted on Player turn.
		}
		else
		{
			//Log(LOG_INFO) << ". after a melee-attack, wait for checkCasualties";
			defender->getRfSpotters()->push_back(nextReactor);	// let BG::checkCasualties() figure it out
		}
	}

	//Log(LOG_INFO) << ". init = " << init;
	return nextReactor;
}

/**
 * Fires off a reaction shot if possible.
 * @param unit			- pointer to the spotting unit
 * @param targetUnit	- pointer to the spotted unit
 * @return, true if a shot happens
 */
bool TileEngine::reactionShot(
		BattleUnit* const unit,
		const BattleUnit* const targetUnit)
{
	//Log(LOG_INFO) << "TileEngine::reactionShot() " << unit->getId();
	_rfAction->actor = unit;

	if (unit->getFaction() == FACTION_PLAYER
		&& unit->isMindControlled() == false)
	{
		switch (unit->getActiveHand())
		{
			case AH_RIGHT:
				_rfAction->weapon = unit->getItem(ST_RIGHTHAND);
				break;
			case AH_LEFT:
				_rfAction->weapon = unit->getItem(ST_LEFTHAND);
				break;

			default:
			case AH_NONE:
				_rfAction->weapon = unit->getMainHandWeapon(true,true,true);
		}
	}
	else // aLien .....
	{
		_rfAction->weapon = unit->getMainHandWeapon(true,true,true); // will get Rifle OR Melee OR Fist

//		if (_rfAction->weapon == nullptr)
//			_rfAction->weapon = unit->getMeleeWeapon(); // will get Melee OR Fist
/*		if (_rfAction->weapon == nullptr
			&& _rfAction->actor->getUnitRules() != nullptr
			&& _rfAction->actor->getUnitRules()->getMeleeWeapon() == "STR_FIST")
		{
			_rfAction->weapon = _battleSave->getBattleGame()->getFist();
		} */
	}

	if (_rfAction->weapon == nullptr // lasers & melee are their own ammo-items and return INT_MAX for ammo-qty.
		|| _rfAction->weapon->getAmmoItem() == nullptr
		|| _rfAction->weapon->getRules()->canReactionFire() == false
		|| (_rfAction->actor->getFaction() != FACTION_HOSTILE
			&& _battleSave->getSavedGame()->isResearched(_rfAction->weapon->getRules()->getRequirements()) == false))
	{
		return false;
	}


	_rfAction->posTarget = targetUnit->getPosition();
	_rfAction->type = BA_NONE;
	_rfAction->TU = 0;

	switch (_rfAction->weapon->getRules()->getBattleType())
	{
		case BT_MELEE:
		{
			_rfAction->type = BA_MELEE;
			_rfAction->TU = _rfAction->actor->getActionTu(BA_MELEE, _rfAction->weapon);
			if (_rfAction->TU == 0
				|| _rfAction->TU > _rfAction->actor->getTu())
			{
				return false;
			}

			bool canMelee (false);
			for (int
					i = 0;
					i != 8 && canMelee == false;
					++i)
			{
				canMelee = validMeleeRange(unit, i, targetUnit);
			}
			if (canMelee == false) return false;
			break;
		}

		default:
			chooseFireMethod();
			if (_rfAction->TU == 0 || _rfAction->type == BA_NONE)
				return false;
	}

	_rfAction->targeting = true;

	if (unit->getFaction() == FACTION_HOSTILE)
	{
		AlienBAIState* aiState (dynamic_cast<AlienBAIState*>(unit->getAIState()));
		if (aiState == nullptr)
		{
			aiState = new AlienBAIState(_battleSave, unit);
			aiState->init();
			unit->setAIState(aiState);
		}

		if (_rfAction->weapon->getAmmoItem()->getRules()->getExplosionRadius() > 0
			&& aiState->explosiveEfficacy(
								_rfAction->posTarget,
								unit,
								_rfAction->weapon->getAmmoItem()->getRules()->getExplosionRadius(),
								_battleSave->getBattleState()->getSavedGame()->getDifficulty()) == false)
		{
			_rfAction->targeting = false;
		}
	}

	if (_rfAction->targeting == true
		&& _rfAction->actor->expendTu(_rfAction->TU) == true)
	{
		//Log(LOG_INFO) << "TE: rf by Actor id-" << _rfAction->actor->getId();
		_rfAction->TU = 0;
		_rfAction->value = -1;

		_battleSave->getBattleGame()->stateBPushBack(new UnitTurnBState(
																_battleSave->getBattleGame(),
																*_rfAction,
																false));
		_battleSave->getBattleGame()->stateBPushBack(new ProjectileFlyBState(
																	_battleSave->getBattleGame(),
																	*_rfAction));
		return true;
	}

	return false;
}

/**
 * Selects a reaction-fire-method based on TU and distance.
 */
void TileEngine::chooseFireMethod()
{
	const int dist (_battleSave->getTileEngine()->distance(
														_rfAction->actor->getPosition(),
														_rfAction->posTarget));
	const RuleItem* const itRule (_rfAction->weapon->getRules());
	if (dist <= itRule->getMaxRange() && dist >= itRule->getMinRange())
	{
		const int tu (_rfAction->actor->getTu());

		if (dist <= itRule->getAutoRange())
		{
			if (itRule->getAutoTu() != 0
				&& tu >= _rfAction->actor->getActionTu(BA_AUTOSHOT, _rfAction->weapon))
			{
				_rfAction->type = BA_AUTOSHOT;
				_rfAction->TU = _rfAction->actor->getActionTu(BA_AUTOSHOT, _rfAction->weapon);
			}
			else if (itRule->getSnapTu() != 0
				&& tu >= _rfAction->actor->getActionTu(BA_SNAPSHOT, _rfAction->weapon))
			{
				_rfAction->type = BA_SNAPSHOT;
				_rfAction->TU = _rfAction->actor->getActionTu(BA_SNAPSHOT, _rfAction->weapon);
			}
			else if (itRule->getAimedTu() != 0
				&& tu >= _rfAction->actor->getActionTu(BA_AIMEDSHOT, _rfAction->weapon))
			{
				_rfAction->type = BA_AIMEDSHOT;
				_rfAction->TU = _rfAction->actor->getActionTu(BA_AIMEDSHOT, _rfAction->weapon);
			}
		}
		else if (dist <= itRule->getSnapRange())
		{
			if (itRule->getSnapTu() != 0
				&& tu >= _rfAction->actor->getActionTu(BA_SNAPSHOT, _rfAction->weapon))
			{
				_rfAction->type = BA_SNAPSHOT;
				_rfAction->TU = _rfAction->actor->getActionTu(BA_SNAPSHOT, _rfAction->weapon);
			}
			else if (itRule->getAimedTu() != 0
				&& tu >= _rfAction->actor->getActionTu(BA_AIMEDSHOT, _rfAction->weapon))
			{
				_rfAction->type = BA_AIMEDSHOT;
				_rfAction->TU = _rfAction->actor->getActionTu(BA_AIMEDSHOT, _rfAction->weapon);
			}
			else if (itRule->getAutoTu() != 0
				&& tu >= _rfAction->actor->getActionTu(BA_AUTOSHOT, _rfAction->weapon))
			{
				_rfAction->type = BA_AUTOSHOT;
				_rfAction->TU = _rfAction->actor->getActionTu(BA_AUTOSHOT, _rfAction->weapon);
			}
		}
		else // if (dist <= itRule->getAimRange())
		{
			if (itRule->getAimedTu() != 0
				&& tu >= _rfAction->actor->getActionTu(BA_AIMEDSHOT, _rfAction->weapon))
			{
				_rfAction->type = BA_AIMEDSHOT;
				_rfAction->TU = _rfAction->actor->getActionTu(BA_AIMEDSHOT, _rfAction->weapon);
			}
			else if (itRule->getSnapTu() != 0
				&& tu >= _rfAction->actor->getActionTu(BA_SNAPSHOT, _rfAction->weapon))
			{
				_rfAction->type = BA_SNAPSHOT;
				_rfAction->TU = _rfAction->actor->getActionTu(BA_SNAPSHOT, _rfAction->weapon);
			}
			else if (itRule->getAutoTu() != 0
				&& tu >= _rfAction->actor->getActionTu(BA_AUTOSHOT, _rfAction->weapon))
			{
				_rfAction->type = BA_AUTOSHOT;
				_rfAction->TU = _rfAction->actor->getActionTu(BA_AUTOSHOT, _rfAction->weapon);
			}
		}
	}
}

/**
 * Accesses a boolean that flags reaction-fire for Camera repositioning.
 * @return, reference to the flag
 */
bool& TileEngine::isReaction()
{
	return _isReaction;
}

/**
 * Handles bullet/weapon hits. A bullet/weapon hits a voxel.
 * @note Called from ExplosionBState::explode().
 * @param targetVoxel	- reference to the center of hit in voxel-space
 * @param power			- power of the hit/explosion
 * @param dType			- damage type of the hit (RuleItem.h)
 * @param attacker		- pointer to BattleUnit that caused the hit
 * @param melee			- true if no projectile, trajectory, etc. is needed (default false)
 * @param shotgun		- true if hit by shotgun pellet(s) (default false)
 * @param infection		- reference to a spawn-unit (default "")
 */
void TileEngine::hit(
		const Position& targetVoxel,
		int power,
		DamageType dType,
		BattleUnit* const attacker,
		bool melee,
		bool shotgun,
		const std::string& infection)
{
	if (dType == DT_NONE) return; // bypass Psi-attacks. Psi-attacks don't get this far anymore ... but safety.

	const Position& posTarget (Position::toTileSpace(targetVoxel));
	Tile* const tile (_battleSave->getTile(posTarget));
	if (tile == nullptr) return;

	BattleUnit* targetUnit (tile->getTileUnit());

	VoxelType voxelType;
	if (melee == true)
		voxelType = VOXEL_UNIT;
	else
		voxelType = voxelCheck(targetVoxel, attacker);

	switch (voxelType)
	{
		case VOXEL_FLOOR:
		case VOXEL_WESTWALL:
		case VOXEL_NORTHWALL:
		case VOXEL_OBJECT:
		{
			switch (dType)
			{
				case DT_STUN: // workaround for Stunrod.
				case DT_SMOKE:
					return;
			}

			power = RNG::generate( // 25% to 75% linear.
								(power		>> 2u),
								(power * 3)	>> 2u);
			// This is where to adjust damage based on effectiveness of weapon vs Terrain!
			// DT_NONE,		// 0
			// DT_AP,		// 1
			// DT_IN,		// 2
			// DT_HE,		// 3
			// DT_LASER,	// 4
			// DT_PLASMA,	// 5
			// DT_STUN,		// 6
			// DT_MELEE,	// 7
			// DT_ACID,		// 8
			// DT_SMOKE		// 9
			const MapDataType partType (static_cast<MapDataType>(voxelType)); // Note that MapDataType & VoxelType correspond.

			switch (dType) // round up.
			{
				case DT_AP:
					power = ((power * 3) + 19) / 20;	// 15%
					break;
				case DT_LASER:
					if (   tile->getMapData(partType)->getTileType() != ALIEN_ALLOYS
						&& tile->getMapData(partType)->getTileType() != DEAD_TILE)
					{
						power = (power + 4) / 5;		// 20% // problem: Fusion Torch; fixed, heh.
					}
					break;
				case DT_IN:
					power = (power + 3) / 4;			// 25%
					break;
				case DT_PLASMA:
					power = (power + 2) / 3;			// 33%
					break;
				case DT_MELEE:							// TODO: define 2 terrain types, Soft & Hard; so that edged weapons do good vs. Soft, blunt weapons do good vs. Hard
					power = (power + 1) / 2;			// 50% TODO: allow melee attacks vs. objects.
					break;
				case DT_HE:								// question: do HE & IN ever get in here - hit() or explode() below
					power += power / 10;				// 110%
//					break;
//				case DT_ACID: // 100% damage
//				default: // [DT_NONE],[DT_STUN,DT_SMOKE]
//					return nullptr;
			}

			if (power > 0)
			{
				if (partType == O_OBJECT
					&& _battleSave->getTacType() == TCT_BASEDEFENSE
					&& tile->getMapData(O_OBJECT)->isBaseObject() == true
					&& tile->getMapData(O_OBJECT)->getArmor() <= power)
				{
					_battleSave->baseDestruct()[static_cast<size_t>((targetVoxel.x >> 4u) / 10)]
											   [static_cast<size_t>((targetVoxel.y >> 4u) / 10)].second--;
				}
				tile->hitTile(partType, power, _battleSave);
			}
			break;
		}

		case VOXEL_UNIT: // BattleUnit voxelType HIT SUCCESS.
		{
			if (targetUnit == nullptr
				&& _battleSave->getTile(posTarget)->hasNoFloor() == true)
			{
				const Tile* const tileBelow (_battleSave->getTile(posTarget + Position(0,0,-1)));
				if (tileBelow != nullptr && tileBelow->getTileUnit() != nullptr)
					targetUnit = tileBelow->getTileUnit();
			}

			if (targetUnit != nullptr)
			{
				const int antecedentWounds (targetUnit->getFatalWounds());

				if (attacker != nullptr
					&& attacker->getSpecialAbility() == SPECAB_BURN) // Silacoids can set targets on fire!!
				{
					const float vulnr (targetUnit->getArmor()->getDamageModifier(DT_IN));
					if (vulnr > 0.f)
					{
						int fire (attacker->getUnitRules()->getSpecabPower());
						fire = RNG::generate( // 12.5% to 37.5%
										(fire	   >> 3u),
										(fire * 3) >> 3u);
						targetUnit->takeDamage(
											Position(0,0,0),
											fire, DT_IN, true);

						const int burn (RNG::generate(0,
													  static_cast<int>(Round(vulnr * 5.f))));
						if (targetUnit->getUnitFire() < burn)
							targetUnit->setUnitFire(burn); // catch fire and burn!!
					}
				}

				const Position
					centerUnitVoxel (Position::toVoxelSpaceCentered(
																targetUnit->getPosition(),
																(targetUnit->getHeight() >> 1u)
																	+ targetUnit->getFloatHeight()
																	- tile->getTerrainLevel(),
																targetUnit->getArmor()->getSize())),
					relationalVoxel (targetVoxel - centerUnitVoxel);

				double delta;
				if (dType == DT_HE || Options::battleTFTDDamage == true)
					delta = 50.;
				else
					delta = 100.;

				const int
					power1 (static_cast<int>(std::floor(static_cast<double>(power) * (100. - delta) / 100.)) + 1),
					power2 (static_cast<int>(std::ceil (static_cast<double>(power) * (100. + delta) / 100.)));

				int extraPower (0);
				if (attacker != nullptr) // bonus to damage per Accuracy (TODO: use ranks also for xCom or aLien)
				{
					switch (dType)
					{
						case DT_AP:
						case DT_LASER:
						case DT_PLASMA:
						case DT_ACID:
							extraPower = attacker->getBattleStats()->firing;
							break;

						case DT_MELEE:
							extraPower = attacker->getBattleStats()->melee;
					}
					extraPower = static_cast<int>(Round(static_cast<double>(power * extraPower) / 1000.));
				}

				power = RNG::generate(power1, power2) // bell curve
					  + RNG::generate(power1, power2);
				power >>= 1u;
				power += extraPower;

				power = targetUnit->takeDamage(
											relationalVoxel,
											power, dType,
											dType == DT_STUN || dType == DT_SMOKE);	// stun ignores armor... does now! UHM.... note it
																					// still gets Vuln.modifier, but not armorReduction.
				if (shotgun == true) targetUnit->hasCried(true);

				if (power != 0)
				{
					if (attacker != nullptr
						&& (antecedentWounds < targetUnit->getFatalWounds()
							|| targetUnit->isOut_t(OUT_HEALTH) == true))	// ... just do this here and bDone with it.
					{														// Regularly done in BattlescapeGame::checkCasualties()
						targetUnit->killerFaction(attacker->getFaction());
					}
					// NOTE: Not so sure that's been setup right (cf. other kill-credit code as well as DebriefingState)
					// I mean, shouldn't that be checking that the thing actually DIES.
					// And, probly don't have to state if killed by aLiens: probly assumed in DebriefingState.

					if (targetUnit->getSpecialAbility() == SPECAB_EXPLODE // cyberdiscs, usually. Also, cybermites ... (and Zombies, on fire).
						&& targetUnit->isOut_t(OUT_HLTH_STUN) == true
						&& (targetUnit->isZombie() == true
							|| (   dType != DT_STUN		// don't explode if stunned. Maybe... see above.
								&& dType != DT_SMOKE
								&& dType != DT_HE		// don't explode if taken down w/ explosives -> wait a sec, this is hit() not explode() ...
								&& dType != DT_IN)))	//&& dType != DT_MELEE
					{
						_battleSave->getBattleGame()->stateBPushNext(new ExplosionBState(
																					_battleSave->getBattleGame(),
																					centerUnitVoxel,
																					nullptr,
																					targetUnit));
					}
				}

				if (infection.empty() == false
					&& (targetUnit->getGeoscapeSoldier() != nullptr
						|| targetUnit->getUnitRules()->getRace() == "STR_CIVILIAN")
					&& targetUnit->getSpawnType().empty() == true)
				{
					targetUnit->setSpawnUnit(infection);
				}

				if (melee == false
					&& _battleSave->getBattleGame()->getTacticalAction()->takenXp == false
					&& targetUnit->getFaction() == FACTION_HOSTILE
					&& attacker != nullptr
					&& attacker->getGeoscapeSoldier() != nullptr
					&& attacker->isMindControlled() == false
					&& _battleSave->getBattleGame()->playerPanicHandled() == true)
				{
					_battleSave->getBattleGame()->getTacticalAction()->takenXp = true;
					attacker->addFiringExp();
				}
			}
		}
	}

	applyGravity(tile);

	calculateSunShading();		// roofs could have been destroyed
	calculateTerrainLighting();	// fires could have been started
//	calculateUnitLighting();	// units could have collapsed <- done in UnitDieBState

	calcFovTiles_pos(posTarget);
	calcFovUnits_pos(posTarget, true);
}

/**
 * Handles blast-propagation of explosions.
 * @note Called from ExplosionBState.
 * HE/smoke/fire explodes in a circular pattern on 1 level only.
 * HE however damages floor tiles of the above level. Not the units on it.
 * HE destroys an object if its power is higher than the object's armor
 * then HE blockage is applied for further propagation.
 * See http://www.ufopaedia.org/index.php?title=Explosions for more info.
 * @param targetVoxel	- reference to the center of explosion in voxelspace
 * @param power			- power of explosion
 * @param dType			- damage type of explosion (RuleItem.h)
 * @param radius		- maximum radius of explosion
 * @param attacker		- pointer to a unit that caused explosion (default nullptr)
 * @param grenade		- true if explosion is caused by a grenade for throwing XP (default false)
 * @param defusePulse	- true if explosion item caused an electo-magnetic pulse that defuses primed grenades (default false)
 * @param isLaunched	- true if shot by a Launcher (default false)
 */
void TileEngine::explode(
		const Position& targetVoxel,
		int power,
		DamageType dType,
		int radius,
		BattleUnit* const attacker,
		bool grenade,
		bool defusePulse,
		bool isLaunched)
{
//	int iFalse (0);
//	for (int i = 0; i < 1000; ++i)
//	{
//		int iTest (RNG::generate(0,1));
//		if (iTest == 0) ++iFalse;
//	}
//	Log(LOG_INFO) << "RNG:TEST = " << iFalse;

	//Log(LOG_INFO) << "TileEngine::explode() power = " << power << ", dType = " << (int)dType << ", radius = " << radius;
	if (dType == DT_IN)
	{
		power >>= 1u;
		//Log(LOG_INFO) << ". DT_IN power = " << power;
	}

	if (power < 1) // quick out.
		return;

	BattleUnit* targetUnit (nullptr);

	std::set<Tile*> tilesAffected;
//	std::set<Tile*>& tilesToDetonate (_battleSave->detonationTiles());
	std::pair<std::set<Tile*>::const_iterator, bool> tilePair;

	int z_Dec;
	switch (Options::battleExplosionHeight)
	{
		case 3: z_Dec = 10; break; // makes things easy for AlienBAIState::explosiveEfficacy()
		case 2: z_Dec = 20; break;
		case 1: z_Dec = 30; break;

		default:
		case 0:
			z_Dec = 0; // default flat explosion
	}

	Tile
		* tileStart (nullptr),
		* tileStop (nullptr);

	int // convert voxel-space to tile-space
		centerX (targetVoxel.x >> 4u),
		centerY (targetVoxel.y >> 4u),
		centerZ (targetVoxel.z / 24),

		tileX,
		tileY,
		tileZ;

	double
		r,
		r_Max (static_cast<double>(radius)),

		vect_x,
		vect_y,
		vect_z,

		sin_te,
		cos_te,
		sin_fi,
		cos_fi;

	bool takenXp (false);

//	int testIter (0); // TEST.
	//Log(LOG_INFO) << ". r_Max = " << r_Max;

//	for (int fi = 0; fi == 0; ++fi)		// kL_note: Looks like a TEST ray. ( 0 == horizontal )
//	for (int fi = 90; fi == 90; ++fi)	// vertical: UP
//	for (int fi = -90; fi == -90; ++fi)	// vertical: DOWN
	for (int
			fi = -90;
			fi <  91;
			fi +=  5) // ray-tracing every 5 degrees makes sure all tiles are covered within a sphere.
	{
//		for (int te = 0; te == 0; ++te)			// kL_note: Looks like a TEST ray. ( 0 == south, 180 == north, goes CounterClock-wise )
//		for (int te = 180; te == 180; ++te)		// N
//		for (int te = 90; te < 360; te += 180)	// E & W
//		for (int te = 45; te < 360; te += 180)	// SE & NW
//		for (int te = 225; te < 420; te += 180)	// NW & SE
//		for (int te = 135; te == 135; ++te)		// NE
//		for (int te = 315; te == 315; ++te)		// SW
//		for (int te = 135; te < 360; te += 180)	// NE & SW
		for (int
				te =   0;
				te < 360;
				te +=  3) // ray-tracing every 3 degrees makes sure all tiles are covered within a circle.
		{
			_dirRay = te;

			//Log(LOG_INFO) << "te = " << te << " fi = " << fi;
			sin_te = std::sin(static_cast<double>(te) * M_PI / 180.);
			cos_te = std::cos(static_cast<double>(te) * M_PI / 180.);
			sin_fi = std::sin(static_cast<double>(fi) * M_PI / 180.);
			cos_fi = std::cos(static_cast<double>(fi) * M_PI / 180.);

			tileStart = _battleSave->getTile(Position(
													centerX,
													centerY,
													centerZ));

			_powerE =
			_powerT = power;	// initialize _powerE & _powerT for each ray.
			r = 0.;				// initialize radial length, also.

			while (_powerE > 0
				&& r - 0.5 < r_Max) // kL_note: Allows explosions of 0 radius(!), single tile only hypothetically.
									// the idea is to show an explosion animation but affect only that one tile.
			{
				//Log(LOG_INFO) << "";
				//++testIter;
				//Log(LOG_INFO) << ". i = " << testIter;
				//Log(LOG_INFO) << ". r = " << r << " _powerE = " << _powerE;

				vect_x = static_cast<double>(centerX) + (r * sin_te * cos_fi);
				vect_y = static_cast<double>(centerY) + (r * cos_te * cos_fi);
				vect_z = static_cast<double>(centerZ) + (r * sin_fi);

				tileX = static_cast<int>(std::floor(vect_x));
				tileY = static_cast<int>(std::floor(vect_y));
				tileZ = static_cast<int>(std::floor(vect_z));

				tileStop = _battleSave->getTile(Position(
														tileX,
														tileY,
														tileZ));
				if (tileStop == nullptr) // out of map!
				{
					//Log(LOG_INFO) << ". test : tileStart " << tileStart->getPosition() << ". tileStop NOT Valid " << Position(tileX, tileY, tileZ);
					break;
				}
				//else Log(LOG_INFO) << ". test : tileStart " << tileStart->getPosition() << " tileStop " << tileStop->getPosition();


				if (r > 0.5						// don't block epicentrum.
					&& tileStart != tileStop)	// don't double blockage from the same tiles (when diagonal that happens).
				{
					int dir;
					Pathfinding::vectorToDirection(
												tileStop->getPosition() - tileStart->getPosition(),
												dir);
					if (dir != -1 && (dir & 1) == 1)
					{
//						_powerE = static_cast<int>(static_cast<float>(_powerE) * 0.70710678f);
//						_powerE = static_cast<int>(static_cast<double>(_powerE) * RNG::generate(0.895,0.935));
						_powerE = static_cast<int>(static_cast<double>(_powerE) * RNG::generate(0.69,0.87));
					}

					if (radius > 0)
					{
						_powerE -= ((power + radius - 1) / radius); // round up.
						//Log(LOG_INFO) << "radius > 0, " << power << "/" << radius << "=" << _powerE;
					}
					//else Log(LOG_INFO) << "radius <= 0";


					if (_powerE < 1)
					{
						//Log(LOG_INFO) << ". _powerE < 1 BREAK[hori] " << Position(tileX, tileY, tileZ) << "\n";
						break;
					}

					if (tileStart->getPosition().z != tileZ // up/down explosion decrease
						&& (z_Dec == 0 || (_powerE -= z_Dec) < 1))
					{
						//Log(LOG_INFO) << ". _powerE < 1 BREAK[vert] " << Position(tileX, tileY, tileZ) << "\n";
						break;
					}

					_powerT = _powerE;

					const int horiBlock (horizontalBlockage(
														tileStart,
														tileStop,
														dType));
					//if (horiBlock != 0) Log(LOG_INFO) << ". horiBlock = " << horiBlock;

					const int vertBlock (verticalBlockage(
														tileStart,
														tileStop,
														dType));
					//if (vertBlock != 0) Log(LOG_INFO) << ". vertBlock = " << vertBlock;

					if (horiBlock < 0 && vertBlock < 0) // only visLike will return < 0 for this break here.
					{
						break; // WAIT A SECOND ... oh, Stun &tc.
					}
					else
					{
						if (horiBlock > 0) // only !visLike will return > 0 for these breaks here.
						{
							if ((_powerT -= horiBlock) < 1) // terrain takes 200% power to destruct. <- But this isn't for destruction.
							{
								//Log(LOG_INFO) << ". horiBlock BREAK " << Position(tileX, tileY, tileZ) << "\n";
								break;
							}
						}

						if (vertBlock > 0) // only !visLike will return > 0 for these breaks here.
						{
							if ((_powerT -= vertBlock) < 1) // terrain takes 200% power to destruct. <- But this isn't for destruction.
							{
								//Log(LOG_INFO) << ". vertBlock BREAK " << Position(tileX, tileY, tileZ) << "\n";
								break;
							}
						}
					}
				}

				// set this to the power-value *before* BLOCK reduces it, and *after* distance is accounted for!
				// ..... not necessarily.
				if (dType == DT_HE) // explosions do 50% damage to terrain and 50% to 150% damage to units
				{
					//Log(LOG_INFO) << ". setExplosive() _powerE = " << _powerE;
					tileStop->setExplosive(_powerE, DT_HE);	// try powerT to prevent smoke/fire appearing behind intact walls etc.
															// although that might gimp true damage vs parts calculations .... NOPE.
//					tilesToDetonate.insert(tileStop);
				}

				_powerE = _powerT; // note: These two are becoming increasingly redundant !!!


				// ** DAMAGE begins w/ _powerE ***

				tilePair = tilesAffected.insert(tileStop);	// check if this tile was hit already
				if (tilePair.second == true)				// true if a new tile was inserted.
				{
					//Log(LOG_INFO) << ". > tile TRUE : tileStart " << tileStart->getPosition() << " tileStop " << tileStop->getPosition() << " _powerE = " << _powerE << " r = " << r;
					//Log(LOG_INFO) << ". > _powerE = " << _powerE;

					if ((targetUnit = tileStop->getTileUnit()) != nullptr
						&& targetUnit->getTakenExpl() == true) // hit large units only once ... stop experience exploitation near the end of this loop, also. Lulz
					{
						//Log(LOG_INFO) << ". . targetUnit id-" << targetUnit->getId() << " set Unit NULL";
						targetUnit = nullptr;
					}

					int
						power_OnUnit,
						antecedentWounds;

					if (attacker != nullptr && targetUnit != nullptr)
						antecedentWounds = targetUnit->getFatalWounds();
					else
						antecedentWounds = 0;

					switch (dType)
					{
						case DT_STUN: // power 0 - 200%
						{
							if (targetUnit != nullptr)
							{
								power_OnUnit = RNG::generate(1, _powerE << 1u) // bell curve
											 + RNG::generate(1, _powerE << 1u);
								power_OnUnit >>= 1u;
								//Log(LOG_INFO) << ". . . power_OnUnit = " << power_OnUnit << " DT_STUN";
								targetUnit->takeDamage(Position(0,0,0), power_OnUnit, DT_STUN, true);
							}

							BattleUnit* bu;
							for (std::vector<BattleItem*>::const_iterator
									i = tileStop->getInventory()->begin();
									i != tileStop->getInventory()->end();
									++i)
							{
								if ((bu = (*i)->getItemUnit()) != nullptr
									&& bu->getUnitStatus() == STATUS_UNCONSCIOUS
									&& bu->getTakenExpl() == false)
								{
									bu->setTakenExpl();
									power_OnUnit = RNG::generate(1, _powerE << 1u) // bell curve
												 + RNG::generate(1, _powerE << 1u);
									power_OnUnit >>= 1u;
									//Log(LOG_INFO) << ". . . . power_OnUnit (corpse) = " << power_OnUnit << " DT_STUN";
									bu->takeDamage(Position(0,0,0), power_OnUnit, DT_STUN, true);
								}
							}
							break;
						}

						case DT_HE: // power 50 - 150%. 70% of that if kneeled, 85% if kneeled @ GZ
						{
							//Log(LOG_INFO) << ". . dType == DT_HE";
							if (targetUnit != nullptr)
							{
								//Log(LOG_INFO) << ". . powerE = " << _powerE << " vs. " << targetUnit->getId();
								const double
									power0 (static_cast<double>(_powerE)),
									power1 (power0 * 0.5),
									power2 (power0 * 1.5);

								power_OnUnit = static_cast<int>(RNG::generate(power1, power2)) // bell curve
											 + static_cast<int>(RNG::generate(power1, power2));
								power_OnUnit >>= 1u;
								//Log(LOG_INFO) << ". . DT_HE = " << power_OnUnit; // << ", vs ID " << targetUnit->getId();

								if (power_OnUnit > 0)
								{
									Position relVoxel;	// units above the explosion will be hit in the legs
									if (distance(		// units lateral to or below will be hit in the torso
												tileStop->getPosition(),
												Position(
														centerX,
														centerY,
														centerZ)) < 2)
									{
										//Log(LOG_INFO) << ". . . power_OnUnit = " << power_OnUnit << " DT_HE, GZ";
										relVoxel = Position(0,0,0); // Ground zero effect is in effect
										if (targetUnit->isKneeled() == true)
										{
											power_OnUnit = power_OnUnit * 17 / 20; // 85% damage
											//Log(LOG_INFO) << ". . . power_OnUnit(kneeled) = " << power_OnUnit << " DT_HE, GZ";
										}
									}
									else
									{
										//Log(LOG_INFO) << ". . . power_OnUnit = " << power_OnUnit << " DT_HE, not GZ";
										relVoxel = Position( // Directional damage relative to explosion position.
														(centerX << 4u) - (tileStop->getPosition().x << 4u),
														(centerY << 4u) - (tileStop->getPosition().y << 4u),
														(centerZ *  24) - (tileStop->getPosition().z *  24));
										if (targetUnit->isKneeled() == true)
										{
											power_OnUnit = power_OnUnit * 7 / 10; // 70% damage
											//Log(LOG_INFO) << ". . . power_OnUnit(kneeled) = " << power_OnUnit << " DT_HE, not GZ";
										}
									}

									if (power_OnUnit > 0)
									{
										targetUnit->takeDamage(relVoxel, power_OnUnit, DT_HE);
										//Log(LOG_INFO) << ". . . realDamage = " << damage << " DT_HE";
									}
								}
							}

							//Log(LOG_INFO) << " ";
							BattleUnit* bu;

							bool done (false);
							while (done == false && tileStop->getInventory()->empty() == false)
							{
								for (std::vector<BattleItem*>::const_iterator
										i = tileStop->getInventory()->begin();
										i != tileStop->getInventory()->end();
										)
								{
									//Log(LOG_INFO) << "pos " << tileStop->getPosition();
									//Log(LOG_INFO) << ". . INVENTORY: Item = " << (*i)->getRules()->getType();
									if ((bu = (*i)->getItemUnit()) != nullptr
										&& bu->getUnitStatus() == STATUS_UNCONSCIOUS
										&& bu->getTakenExpl() == false)
									{
										//Log(LOG_INFO) << ". . . vs Unit unconscious";
										bu->setTakenExpl();

										const double
											power0 (static_cast<double>(_powerE)),
											power1 (power0 * 0.5),
											power2 (power0 * 1.5);

										power_OnUnit = static_cast<int>(RNG::generate(power1, power2)) // bell curve
													 + static_cast<int>(RNG::generate(power1, power2));
										power_OnUnit >>= 1u;
										//Log(LOG_INFO) << ". . . INVENTORY: power = " << power_OnUnit;
										bu->takeDamage(Position(0,0,0), power_OnUnit, DT_HE);
										//Log(LOG_INFO) << ". . . INVENTORY: damage = " << dam;

										if (bu->isOut_t(OUT_HEALTH) == true)
										{
											//Log(LOG_INFO) << ". . . . INVENTORY: instaKill";
//											bu->instaKill(); // TODO: Log the kill in Soldier's Diary.
											if (attacker != nullptr)
											{
												bu->killerFaction(attacker->getFaction());
												//Log(LOG_INFO) << "TE::explode() " << bu->getId() << " killedByFaction = " << (int)attacker->getFaction();
											}

											if (bu->getGeoscapeSoldier() != nullptr // send Death notice.
												&& Options::battleNotifyDeath == true)
											{
												Game* const game (_battleSave->getBattleState()->getGame());
												game->pushState(new InfoboxDialogState(game->getLanguage()->getString( // "has exploded ..."
																													"STR_HAS_BEEN_KILLED",
																													bu->getGender())
																												.arg(bu->getName(game->getLanguage()))));
											}
										}
									}
									else if (_powerE > (*i)->getRules()->getArmorPoints()
										&& (bu == nullptr
											|| (bu->getUnitStatus() == STATUS_DEAD && bu->getTakenExpl() == false)))
									{
										//Log(LOG_INFO) << ". . . vs Item armor";
										if ((*i)->getRules()->isGrenade() == true && (*i)->getFuse() > -1)
										{
											//Log(LOG_INFO) << ". . . . INVENTORY: primed grenade";
											const Position explVoxel (Position::toVoxelSpaceCentered(tileStop->getPosition(), FLOOR_TLEVEL));
											_battleSave->getBattleGame()->stateBPushNext(new ExplosionBState(
																										_battleSave->getBattleGame(),
																										explVoxel,
																										(*i)->getRules(),
																										attacker));
										}
										//Log(LOG_INFO) << ". . . . INVENTORY: removeItem = " << (*i)->getRules()->getType();
										_battleSave->toDeleteItem(*i);
										break;
/*										if ((*i)->getRules()->isGrenade() == true && (*i)->getFuse() > -1)
										{
											//Log(LOG_INFO) << ". . . . INVENTORY: primed grenade";
											(*i)->setFuse(-2);
											const Position explVoxel (Position::toVoxelSpaceCentered(tileStop->getPosition(), FLOOR_TLEVEL));
											_battleSave->getBattleGame()->stateBPushNext(new ExplosionBState(
																										_battleSave->getBattleGame(),
																										explVoxel,
																										(*i)->getRules(),
																										attacker));
										}
										else if ((*i)->getFuse() != -2)
										{
											//Log(LOG_INFO) << ". . . . INVENTORY: removeItem = " << (*i)->getRules()->getType();
											_battleSave->toDeleteItem(*i);
											break;
										} */
									}
									//else Log(LOG_INFO) << ". . . INVENTORY: bypass item = " << (*i)->getRules()->getType();
									done = (++i == tileStop->getInventory()->end());
								}
							}
							break;
						}

						case DT_SMOKE:
						{
							if (tileStop->getTerrainLevel() > -24)
							{
								const int powerSmoke (static_cast<int>(std::ceil(
													 (static_cast<double>(_powerE) / static_cast<double>(power)) * 10.)));
								tileStop->addSmoke(powerSmoke + RNG::generate(0,7));
							}

							if (targetUnit != nullptr)
							{
								power_OnUnit = RNG::generate( // 10% to 20%
														_powerE / 10,
														_powerE / 5);
								targetUnit->takeDamage(Position(0,0,0), power_OnUnit, DT_SMOKE, true);
								//Log(LOG_INFO) << ". . DT_IN : " << targetUnit->getId() << " takes " << firePower << " firePower";
							}

							BattleUnit* bu;
							for (std::vector<BattleItem*>::const_iterator
									i = tileStop->getInventory()->begin();
									i != tileStop->getInventory()->end();
									++i)
							{
								if ((bu = (*i)->getItemUnit()) != nullptr
									&& bu->getUnitStatus() == STATUS_UNCONSCIOUS
									&& bu->getTakenExpl() == false)
								{
									bu->setTakenExpl();
									power_OnUnit = RNG::generate( // 10% to 20%
															_powerE / 10,
															_powerE / 5);
									bu->takeDamage(Position(0,0,0), power_OnUnit, DT_SMOKE, true);
								}
							}
							break;
						}

						case DT_IN:
						{
							if (targetUnit != nullptr)
							{
								targetUnit->setTakenExpl();
								power_OnUnit = RNG::generate( // 25% - 75%
														(_powerE	  >> 2u),
														(_powerE * 3) >> 2u);
								targetUnit->takeDamage(Position(0,0,0), power_OnUnit, DT_IN, true);
								//Log(LOG_INFO) << ". . DT_IN : " << targetUnit->getId() << " takes " << firePower << " firePower";

								const float vulnr (targetUnit->getArmor()->getDamageModifier(DT_IN));
								if (vulnr > 0.f)
								{
									const int burn (RNG::generate(0,
																  static_cast<int>(Round(5.f * vulnr))));
									if (targetUnit->getUnitFire() < burn)
										targetUnit->setUnitFire(burn); // catch fire and burn!!
								}
							}

							Tile // NOTE: Should check if tileBelow's have already had napalm drop on them from this explosion ....
								* tileFire (tileStop),
								* tileBelow (_battleSave->getTile(tileFire->getPosition() + Position(0,0,-1)));

							while (tileFire != nullptr				// safety.
								&& tileFire->getPosition().z > 0	// safety.
								&& (tileFire->getMapData(O_OBJECT) == nullptr
									|| (tileFire->getMapData(O_OBJECT)->getBigwall() & 0xfe) == 1)
//								&& tileFire->getMapData(O_FLOOR) == nullptr
								&& tileFire->hasNoFloor(tileBelow) == true)
							{
								tileFire = tileBelow;
								tileBelow = _battleSave->getTile(tileFire->getPosition() + Position(0,0,-1));
							}

//							if (tileFire->isVoid() == false)
//							{
								// kL_note: So, this just sets a tile on fire/smoking regardless of its content.
								// cf. Tile::igniteTile() -> well, not regardless, but automatically. That is,
								// igniteTile() checks for Flammability first: if (getFlammability() == 255) don't do it.
								// So this is, like, napalm from an incendiary round, while igniteTile() is for parts
								// of the tile itself self-igniting.

							if (tileFire != nullptr) // safety.
							{
								const int fire (std::max(1,
														 static_cast<int>(std::ceil(
														(static_cast<float>(_powerE) / static_cast<float>(power)) * 10.))));
								if (tileFire->addFire(fire + tileFire->getFuel() + 2 / 3) == false)
									tileFire->addSmoke(std::max(tileFire->getFuel() + fire,
															  ((tileFire->getFlammability() + 9) / 10) + fire));
							}
//							}

							if ((targetUnit = tileFire->getTileUnit()) != nullptr
								&& targetUnit->getTakenExpl() == false)
							{
								power_OnUnit = RNG::generate( // 25% - 75%
														(_powerE	  >> 2u),
														(_powerE * 3) >> 2u);
								targetUnit->takeDamage(Position(0,0,0), power_OnUnit, DT_IN, true);
								//Log(LOG_INFO) << ". . DT_IN : " << targetUnit->getId() << " takes " << firePower << " firePower";

								const float vulnr (targetUnit->getArmor()->getDamageModifier(DT_IN));
								if (vulnr > 0.f)
								{
									const int burn (RNG::generate(0,
																  static_cast<int>(Round(5.f * vulnr))));
									if (targetUnit->getUnitFire() < burn)
										targetUnit->setUnitFire(burn); // catch fire and burn!!
								}
							}

							BattleUnit* bu;

							bool done (false);
							while (done == false && tileStop->getInventory()->empty() == false)
							{
								for (std::vector<BattleItem*>::const_iterator
										i = tileFire->getInventory()->begin();
										i != tileFire->getInventory()->end();
										)
								{
									if ((bu = (*i)->getItemUnit()) != nullptr
										&& bu->getUnitStatus() == STATUS_UNCONSCIOUS
										&& bu->getTakenExpl() == false)
									{
										bu->setTakenExpl();
										power_OnUnit = RNG::generate( // 25% - 75%
																(_powerE	  >> 2u),
																(_powerE * 3) >> 2u);
										bu->takeDamage(Position(0,0,0), power_OnUnit, DT_IN, true);

										if (bu->isOut_t(OUT_HEALTH) == true)
										{
//											bu->instaKill(); // TODO: Log the kill in Soldier's Diary.
											if (attacker != nullptr)
											{
												bu->killerFaction(attacker->getFaction());
												//Log(LOG_INFO) << "TE::explode() " << bu->getId() << " killedByFaction = " << (int)attacker->getFaction();
											}

											if (bu->getGeoscapeSoldier() != nullptr // send Death notice.
												&& Options::battleNotifyDeath == true)
											{
												Game* const game (_battleSave->getBattleState()->getGame());
												game->pushState(new InfoboxDialogState(game->getLanguage()->getString( // "has been killed with Fire ..."
																													"STR_HAS_BEEN_KILLED",
																													bu->getGender())
																												.arg(bu->getName(game->getLanguage()))));
											}
										}
									}
									else if (_powerE > (*i)->getRules()->getArmorPoints()
										&& (bu == nullptr
											|| (bu->getUnitStatus() == STATUS_DEAD && bu->getTakenExpl() == false)))
									{
										if ((*i)->getRules()->isGrenade() == true && (*i)->getFuse() > -1)
										{
											const Position explVoxel (Position::toVoxelSpaceCentered(tileStop->getPosition(), FLOOR_TLEVEL));
											_battleSave->getBattleGame()->stateBPushNext(new ExplosionBState(
																										_battleSave->getBattleGame(),
																										explVoxel,
																										(*i)->getRules(),
																										attacker));
										}
										_battleSave->toDeleteItem(*i);
										break;
/*										if ((*i)->getRules()->isGrenade() == true && (*i)->getFuse() > -1)
										{
											(*i)->setFuse(-2);
											const Position explVoxel (Position::toVoxelSpaceCentered(tileStop->getPosition(), FLOOR_TLEVEL));
											_battleSave->getBattleGame()->stateBPushNext(new ExplosionBState(
																										_battleSave->getBattleGame(),
																										explVoxel,
																										(*i)->getRules(),
																										attacker));
										}
										else if ((*i)->getFuse() != -2)
										{
											_battleSave->toDeleteItem(*i);
											break;
										} */
									}
									done = (++i == tileStop->getInventory()->end());
								}
							}
						}
					} // End switch().


					if (targetUnit != nullptr)
					{
						//Log(LOG_INFO) << ". . targetUnit ID " << targetUnit->getId() << ", setTaken TRUE";
						targetUnit->setTakenExpl();
						// if it's going to bleed to death and it's not a player give credit for the kill.
						// kL_note: See Above^
						if (attacker != nullptr)
						{
//							if (targetUnit->getHealth() == 0
							if (targetUnit->isOut_t(OUT_HEALTH) == true
								|| antecedentWounds < targetUnit->getFatalWounds())
							{
								targetUnit->killerFaction(attacker->getFaction()); // kL .. just do this here and bDone with it. Normally done in BattlescapeGame::checkCasualties()
								//Log(LOG_INFO) << "TE::explode() " << targetUnit->getId() << " killedByFaction = " << (int)attacker->getFaction();
							}

							if (takenXp == false
								&& targetUnit->getFaction() == FACTION_HOSTILE
								&& attacker->getGeoscapeSoldier() != nullptr
								&& attacker->isMindControlled() == false
								&& dType != DT_SMOKE
								&& _battleSave->getBattleGame()->playerPanicHandled() == true)
							{
								takenXp = true;
								if (grenade == true)
									attacker->addThrowingExp();
								else if (isLaunched == false)
									attacker->addFiringExp();
							}
						}
					}
				}

				tileStart = tileStop;
				r += 1.;
				//Log(LOG_INFO) << "";
			}
		}
	}

	_powerE =
	_powerT =
	_dirRay = -1;

	for (std::vector<BattleUnit*>::const_iterator
			i = _battleSave->getUnits()->begin();
			i != _battleSave->getUnits()->end();
			++i)
	{
		//Log(LOG_INFO) << ". . unitTaken ID " << (*i)->getId() << ", reset Taken";
		(*i)->setTakenExpl(false);
	}


	if (dType == DT_HE) // detonate tiles affected with HE
	{
		if (_trueTile != nullptr)		// special case for when a diagonal bigwall is directly targetted.
		{								// The explosion is moved out a tile so give a full-power hit to the true target-tile.
			_trueTile->setExplosive(power, DT_HE);
			detonateTile(_trueTile);	// I doubt this needs any *further* consideration ...
		}								// although it would be nice to have the explosion 'kick in' a bit.

		//Log(LOG_INFO) << ". explode Tiles, size = " << tilesAffected.size();
		for (std::set<Tile*>::const_iterator
				i = tilesAffected.begin();
				i != tilesAffected.end();
				++i)
		{
			if (*i != _trueTile)
			{
				detonateTile(*i);
				applyGravity(*i);
				Tile* const tileAbove (_battleSave->getTile((*i)->getPosition() + Position(0,0,1)));
				if (tileAbove != nullptr)
					applyGravity(tileAbove);
			}
		}
		//Log(LOG_INFO) << ". explode Tiles DONE";
	}
	_trueTile = nullptr;

//	if (_trueTile != nullptr)
//	{
//		if (dType == DT_HE)	// special case for when a diagonal bigwall is directly targetted.
//		{					// The explosion is moved out a tile so give a full-power hit to the true target-tile.
//			_trueTile->setExplosive(power, DT_HE);
//			tilesToDetonate.insert(_trueTile);
//		}
//		_trueTile = nullptr;
//	}


	if (defusePulse == true)
	{
		for (std::vector<BattleItem*>::const_iterator
				i = _battleSave->getItems()->begin();
				i != _battleSave->getItems()->end();
				++i)
		{
			switch ((*i)->getRules()->getBattleType())
			{
				case BT_GRENADE:
				case BT_PROXYGRENADE:
				case BT_FLARE:
					(*i)->setFuse(-1); // TODO: Grenades and proxies EXPLODE.
			}
		}
	}


	calculateSunShading();		// roofs could have been destroyed
	calculateTerrainLighting();	// fires could have been started
//	calculateUnitLighting();	// units could have collapsed <- done in UnitDieBState

	calcFovTiles_all();
	calcFovUnits_all();
	//Log(LOG_INFO) << "TileEngine::explode() EXIT";
}

/**
 * Calculates the quantity of power that is blocked as it passes from one Tile
 * to another on the same z-level.
 * @param tileStart	- pointer to tile where the power starts
 * @param tileStop	- pointer to adjacent tile where the power ends
 * @param dType		- type of power (RuleItem.h)
 * @return, 0			- no block
 *			1+			- variable power blocked
 *			HARD_BLOCK	- hard-block for power or vision
 */
int TileEngine::horizontalBlockage(
		const Tile* const tileStart,
		const Tile* const tileStop,
		const DamageType dType) const
{
	//Log(LOG_INFO) << "TileEngine::horizontalBlockage()";
	if (   tileStart == nullptr // safety checks
		|| tileStop  == nullptr
		|| tileStart->getPosition().z != tileStop->getPosition().z)
	{
		return 0;
	}

	int dir;
	Pathfinding::vectorToDirection(
							tileStop->getPosition() - tileStart->getPosition(),
							dir);
	if (dir == -1) return 0; // tileStart == tileStop


	bool visLike;
	switch (dType)
	{
		case DT_NONE:
		case DT_IN:
		case DT_STUN:
		case DT_SMOKE:
			visLike = true;
			break;

		default:
			visLike = false;
	}

	static const Position
		posNorth	(Position( 0,-1, 0)),
		posEast		(Position( 1, 0, 0)),
		posSouth	(Position( 0, 1, 0)),
		posWest		(Position(-1, 0, 0));

	int block (0);
	switch (dir)
	{
		case 0:	// north
			block = blockage(
							tileStart,
							O_NORTHWALL,
							dType);

			if (visLike == false) // visLike does this after the switch()
				block += blockage(
								tileStart,
								O_OBJECT,
								dType,
								0,
								true)
						+ blockage(
								tileStop,
								O_OBJECT,
								dType,
								4);
			break;

		case 1: // north east
			if (visLike == true)
			{
				block = blockage( // up+right
								tileStart,
								O_NORTHWALL,
								dType)
						+ blockage(
								tileStop,
								O_WESTWALL,
								dType)
						+ blockage(
								_battleSave->getTile(tileStart->getPosition() + posNorth),
								O_OBJECT,
								dType,
								3); // checks Content/bigWalls

				if (block == 0) break; // this way is opened

				block = blockage( // right+up
								_battleSave->getTile(tileStart->getPosition() + posEast),
								O_NORTHWALL,
								dType)
						+ blockage(
								_battleSave->getTile(tileStart->getPosition() + posEast),
								O_WESTWALL,
								dType)
						+ blockage(
								_battleSave->getTile(tileStart->getPosition() + posEast),
								O_OBJECT,
								dType,
								7); // checks Content/bigWalls
			}
			else // dt_NE
			{
				block = (blockage(
								tileStart,
								O_NORTHWALL,
								dType) >> 1u)
						+ (blockage(
								tileStop,
								O_WESTWALL,
								dType) >> 1u)
						+ (blockage(
								_battleSave->getTile(tileStart->getPosition() + posEast),
								O_NORTHWALL,
								dType) >> 1u)
						+ (blockage(
								_battleSave->getTile(tileStart->getPosition() + posEast),
								O_WESTWALL,
								dType) >> 1u)

						+ (blockage(
								tileStart,
								O_OBJECT,
								dType,
								0,
								true) >> 1u)
						+ (blockage(
								tileStart,
								O_OBJECT,
								dType,
								2,
								true) >> 1u)

						+ (blockage(
								_battleSave->getTile(tileStart->getPosition() + posNorth),
								O_OBJECT,
								dType,
								2) >> 1u) // checks Content/bigWalls
						+ (blockage(
								_battleSave->getTile(tileStart->getPosition() + posNorth),
								O_OBJECT,
								dType,
								4) >> 1u) // checks Content/bigWalls

						+ (blockage(
								_battleSave->getTile(tileStart->getPosition() + posEast),
								O_OBJECT,
								dType,
								0) >> 1u) // checks Content/bigWalls
						+ (blockage(
								_battleSave->getTile(tileStart->getPosition() + posEast),
								O_OBJECT,
								dType,
								6) >> 1u) // checks Content/bigWalls

						+ (blockage(
								tileStop,
								O_OBJECT,
								dType,
								4) >> 1u) // checks Content/bigWalls
						+ (blockage(
								tileStop,
								O_OBJECT,
								dType,
								6) >> 1u); // checks Content/bigWalls
			}
			break;

		case 2: // east
			block = blockage(
							tileStop,
							O_WESTWALL,
							dType);

			if (visLike == false) // visLike does this after the switch()
				block += blockage(
								tileStop,
								O_OBJECT,
								dType,
								6)
						+  blockage(
								tileStart,
								O_OBJECT,
								dType,
								2,
								true);
			break;

		case 3: // south east
			if (visLike == true)
			{
				block = blockage( // down+right
								_battleSave->getTile(tileStart->getPosition() + posSouth),
								O_NORTHWALL,
								dType)
						+ blockage(
								tileStop,
								O_WESTWALL,
								dType)
						+ blockage(
								_battleSave->getTile(tileStart->getPosition() + posSouth),
								O_OBJECT,
								dType,
								1); // checks Content/bigWalls

				if (block == 0) break; // this way is opened

				block = blockage( // right+down
								_battleSave->getTile(tileStart->getPosition() + posEast),
								O_WESTWALL,
								dType)
						+ blockage(
								tileStop,
								O_NORTHWALL,
								dType)
						+ blockage(
								_battleSave->getTile(tileStart->getPosition() + posEast),
								O_OBJECT,
								dType,
								5); // checks Content/bigWalls
			}
			else // dt_SE
			{
				block = (blockage(
								tileStop,
								O_WESTWALL,
								dType) >> 1u)
						+ (blockage(
								tileStop,
								O_NORTHWALL,
								dType) >> 1u)
						+ (blockage(
								_battleSave->getTile(tileStart->getPosition() + posEast),
								O_WESTWALL,
								dType) >> 1u)
						+ (blockage(
								_battleSave->getTile(tileStart->getPosition() + posSouth),
								O_NORTHWALL,
								dType) >> 1u)

						+ (blockage(
								tileStart,
								O_OBJECT,
								dType,
								2,
								true) >> 1u) // checks Content/bigWalls
						+ (blockage(
								tileStart,
								O_OBJECT,
								dType,
								4,
								true) >> 1u) // checks Content/bigWalls

						+ (blockage(
								_battleSave->getTile(tileStart->getPosition() + posSouth),
								O_OBJECT,
								dType,
								0) >> 1u) // checks Content/bigWalls
						+ (blockage(
								_battleSave->getTile(tileStart->getPosition() + posSouth),
								O_OBJECT,
								dType,
								2) >> 1u) // checks Content/bigWalls

						+ (blockage(
								_battleSave->getTile(tileStart->getPosition() + posEast),
								O_OBJECT,
								dType,
								4) >> 1u) // checks Content/bigWalls
						+ (blockage(
								_battleSave->getTile(tileStart->getPosition() + posEast),
								O_OBJECT,
								dType,
								6) >> 1u) // checks Content/bigWalls

						+ (blockage(
								tileStop,
								O_OBJECT,
								dType,
								0) >> 1u) // checks Content/bigWalls
						+ (blockage(
								tileStop,
								O_OBJECT,
								dType,
								6) >> 1u); // checks Content/bigWalls
			}
			break;

		case 4: // south
			block = blockage(
							tileStop,
							O_NORTHWALL,
							dType);

			if (visLike == false) // visLike does this after the switch()
				block += blockage(
								tileStop,
								O_OBJECT,
								dType,
								0)
						+ blockage(
								tileStart,
								O_OBJECT,
								dType,
								4,
								true);
			break;

		case 5: // south west
			if (visLike == true)
			{
				block = blockage( // down+left
								_battleSave->getTile(tileStart->getPosition() + posSouth),
								O_NORTHWALL,
								dType)
						+ blockage(
								_battleSave->getTile(tileStart->getPosition() + posSouth),
								O_WESTWALL,
								dType)
						+ blockage(
								_battleSave->getTile(tileStart->getPosition() + posSouth),
								O_OBJECT,
								dType,
								7); // checks Content/bigWalls

				if (block == 0) break; // this way is opened

				block = blockage( // left+down
								tileStart,
								O_WESTWALL,
								dType)
						+ blockage(
								tileStop,
								O_NORTHWALL,
								dType)
						+ blockage(
								_battleSave->getTile(tileStart->getPosition() + posWest),
								O_OBJECT,
								dType,
								3);
			}
			else // dt_SW
			{
				block = (blockage(
								tileStop,
								O_NORTHWALL,
								dType) >> 1u)
						+ (blockage(
								tileStart,
								O_WESTWALL,
								dType) >> 1u)
						+ (blockage(
								_battleSave->getTile(tileStart->getPosition() + posSouth),
								O_WESTWALL,
								dType) >> 1u)
						+ (blockage(
								_battleSave->getTile(tileStart->getPosition() + posSouth),
								O_NORTHWALL,
								dType) >> 1u)

						+ (blockage(
								tileStart,
								O_OBJECT,
								dType,
								4,
								true) >> 1u) // checks Content/bigWalls
						+ (blockage(
								tileStart,
								O_OBJECT,
								dType,
								6,
								true) >> 1u) // checks Content/bigWalls

						+ (blockage(
								_battleSave->getTile(tileStart->getPosition() + posSouth),
								O_OBJECT,
								dType,
								0) >> 1u) // checks Content/bigWalls
						+ (blockage(
								_battleSave->getTile(tileStart->getPosition() + posSouth),
								O_OBJECT,
								dType,
								6) >> 1u) // checks Content/bigWalls

						+ (blockage(
								_battleSave->getTile(tileStart->getPosition() + posWest),
								O_OBJECT,
								dType,
								2) >> 1u) // checks Content/bigWalls
						+ (blockage(
								_battleSave->getTile(tileStart->getPosition() + posWest),
								O_OBJECT,
								dType,
								4) >> 1u) // checks Content/bigWalls

						+ (blockage(
								tileStop,
								O_OBJECT,
								dType,
								0) >> 1u) // checks Content/bigWalls
						+ (blockage(
								tileStop,
								O_OBJECT,
								dType,
								2) >> 1u); // checks Content/bigWalls
			}
			break;

		case 6: // west
			block = blockage(
							tileStart,
							O_WESTWALL,
							dType);

			if (visLike == false) // visLike does this after the switch()
			{
				block += blockage(
								tileStart,
								O_OBJECT,
								dType,
								6,
								true)
						+ blockage(
								tileStop,
								O_OBJECT,
								dType,
								2);
			}
			break;

		case 7: // north west
			if (visLike == true)
			{
				block = blockage( // up+left
								tileStart,
								O_NORTHWALL,
								dType)
						+ blockage(
								_battleSave->getTile(tileStart->getPosition() + posNorth),
								O_WESTWALL,
								dType)
						+ blockage(
								_battleSave->getTile(tileStart->getPosition() + posNorth),
								O_OBJECT,
								dType,
								5);

				if (block == 0) break; // this way is opened

				block = blockage( // left+up
								tileStart,
								O_WESTWALL,
								dType)
						+ blockage(
								_battleSave->getTile(tileStart->getPosition() + posWest),
								O_NORTHWALL,
								dType)
						+ blockage(
								_battleSave->getTile(tileStart->getPosition() + posWest),
								O_OBJECT,
								dType,
								1);
			}
			else // dt_NW
			{
				block = (blockage(
								tileStart,
								O_WESTWALL,
								dType) >> 1u)
						+ (blockage(
								tileStart,
								O_NORTHWALL,
								dType) >> 1u)
						+ (blockage(
								_battleSave->getTile(tileStart->getPosition() + posNorth),
								O_WESTWALL,
								dType) >> 1u)
						+ (blockage(
								_battleSave->getTile(tileStart->getPosition() + posWest),
								O_NORTHWALL,
								dType) >> 1u)

						+ (blockage(
								tileStart,
								O_OBJECT,
								dType,
								0,
								true) >> 1u)
						+ (blockage(
								tileStart,
								O_OBJECT,
								dType,
								6,
								true) >> 1u)

						+ (blockage(
								_battleSave->getTile(tileStart->getPosition() + posNorth),
								O_OBJECT,
								dType,
								4) >> 1u)
						+ (blockage(
								_battleSave->getTile(tileStart->getPosition() + posNorth),
								O_OBJECT,
								dType,
								6) >> 1u)

						+ (blockage(
								_battleSave->getTile(tileStart->getPosition() + posWest),
								O_OBJECT,
								dType,
								0) >> 1u)
						+ (blockage(
								_battleSave->getTile(tileStart->getPosition() + posWest),
								O_OBJECT,
								dType,
								2) >> 1u)

						+ (blockage(
								tileStop,
								O_OBJECT,
								dType,
								2) >> 1u)
						+ (blockage(
								tileStop,
								O_OBJECT,
								dType,
								4) >> 1u);
			}
	}

	if (visLike == true)
	{
		block += blockage(
						tileStart,
						O_OBJECT,
						dType,
						dir, // checks Content/bigWalls
						true,
						true);

		if (block == 0		// if, no visBlock yet ...
			&& blockage(	// so check for content @tileStop & reveal it by not cutting trajectory.
					tileStop,
					O_OBJECT,
					dType,
					(dir + 4) % 8) != 0)	// opposite direction
		{									// should always be, < 1; ie. this condition checks [if -1]
			switch (dType)
			{
				case DT_NONE:
					//if (_debug) Log(LOG_INFO) << ". ret -1";
					return -1;

				default:
					//if (_debug) Log(LOG_INFO) << ". ret HARD_BLOCK";
					return HARD_BLOCK;
			}
		}
	}

	//Log(LOG_INFO) << "TileEngine::horizontalBlockage() EXIT, ret = " << block;
	return block;
}

/**
 * Calculates the quantity of power that is blocked going from one tile to
 * another on a different z-level.
 * @note Can cross more than one level (used for lighting). Only floor- and
 * object-parts are taken into account ... not really!
 * @param tileStart	- pointer to Tile where the power starts
 * @param tileStop	- pointer to adjacent Tile where the power ends
 * @param dType		- DamageType of power (RuleItem.h)
 * @return, 0			- no block
 *			1+			- variable power blocked
 *			HARD_BLOCK	- hard-block for power or vision
 */
int TileEngine::verticalBlockage(
		const Tile* const tileStart,
		const Tile* const tileStop,
		const DamageType dType) const
{
	//Log(LOG_INFO) << "TileEngine::verticalBlockage()";
	if (   tileStart == nullptr // safeties.
		|| tileStop == nullptr
		|| tileStart == tileStop)
	{
		return 0;
	}

	const int dirZ (tileStop->getPosition().z - tileStart->getPosition().z);
	if (dirZ == 0) return 0;

	int
		x (tileStart->getPosition().x),
		y (tileStart->getPosition().y),
		z (tileStart->getPosition().z),
		block (0);

	if (dirZ > 0) // up
	{
		if (   x == tileStop->getPosition().x
			&& y == tileStop->getPosition().y)
		{
			for ( // this checks directly up.
					z += 1;
					z <= tileStop->getPosition().z;
					++z)
			{
				block += blockage( // these check the stopTile
								_battleSave->getTile(Position(x,y,z)),
								O_FLOOR,
								dType)
					   + blockage(
								_battleSave->getTile(Position(x,y,z)),
								O_OBJECT,
								dType,
								Pathfinding::DIR_UP);
			}
			return block;
		}

		x = tileStop->getPosition().x;
		y = tileStop->getPosition().y;
		z = tileStart->getPosition().z;

		block += horizontalBlockage( // this checks for ANY Block horizontally to a tile beneath the tileStop
								tileStart,
								_battleSave->getTile(Position(x,y,z)),
								dType);

		for (
				z += 1;
				z <= tileStop->getPosition().z;
				++z)
		{
			block += blockage( // these check the stopTile
							_battleSave->getTile(Position(x,y,z)),
							O_FLOOR,
							dType)
				   + blockage(
							_battleSave->getTile(Position(x,y,z)),
							O_OBJECT,
							dType, // note: no Dir vs typeOBJECT ... added:
							Pathfinding::DIR_UP);
		}
	}
	else // down
	{
		if (   x == tileStop->getPosition().x
			&& y == tileStop->getPosition().y)
		{
			for ( // this checks directly down.
					;
					z > tileStop->getPosition().z;
					--z)
			{
				block += blockage( // these check the startTile
								_battleSave->getTile(Position(x,y,z)),
								O_FLOOR,
								dType)
					   + blockage(
								_battleSave->getTile(Position(x,y,z)),
								O_OBJECT,
								dType,
								Pathfinding::DIR_DOWN,
								true); // kL_add. ( should be false for LoS, btw ) huh
			}
			return block;
		}

		x = tileStop->getPosition().x;
		y = tileStop->getPosition().y;
		z = tileStart->getPosition().z;

		block += horizontalBlockage( // this checks for ANY Block horizontally to a tile above the tileStop
								tileStart,
								_battleSave->getTile(Position(x,y,z)),
								dType);

		for (
				;
				z > tileStop->getPosition().z;
				--z)
		{
			block += blockage( // these check the startTile
							_battleSave->getTile(Position(x,y,z)),
							O_FLOOR,
							dType)
				   + blockage(
							_battleSave->getTile(Position(x,y,z)),
							O_OBJECT,
							dType, // note: no Dir vs typeOBJECT ... added:
							Pathfinding::DIR_DOWN,
							true);
		}
	}

	//Log(LOG_INFO) << "TileEngine::verticalBlockage() EXIT ret = " << block;
	return block;
}

/**
 * Calculates the amount of power that is blocked going from one tile to another on a different level.
 * @param tileStart The tile where the power starts.
 * @param tileStop The adjacent tile where the power ends.
 * @param dType The type of power/damage.
 * @param skipObject
 * @return Amount of blockage of this power.
 *
int TileEngine::verticalBlockage(Tile *tileStart, Tile *tileStop, DamageType dType, bool skipObject)
{
	int block = 0;

	// safety check
	if (tileStart == 0 || tileStop == 0) return 0;
	int direction = tileStop->getPosition().z - tileStart->getPosition().z;

	if (direction == 0 ) return 0;

	int x = tileStart->getPosition().x;
	int y = tileStart->getPosition().y;
	int z = tileStart->getPosition().z;

	if (direction < 0) // down
	{
		block += blockage(_save->getTile(Position(x, y, z)), O_FLOOR, dType);
		if (!skipObject)
			block += blockage(_save->getTile(Position(x, y, z)), O_OBJECT, dType, Pathfinding::DIR_DOWN);
		if (x != tileStop->getPosition().x || y != tileStop->getPosition().y)
		{
			x = tileStop->getPosition().x;
			y = tileStop->getPosition().y;
			int z = tileStart->getPosition().z;
			block += horizontalBlockage(tileStart, _save->getTile(Position(x, y, z)), dType, skipObject);
			block += blockage(_save->getTile(Position(x, y, z)), O_FLOOR, dType);
			if (!skipObject)
				block += blockage(_save->getTile(Position(x, y, z)), O_OBJECT, dType, Pathfinding::DIR_DOWN);
		}
	}
	else if (direction > 0) // up
	{
		block += blockage(_save->getTile(Position(x, y, z+1)), O_FLOOR, dType);
		if (!skipObject)
			block += blockage(_save->getTile(Position(x, y, z+1)), O_OBJECT, dType, Pathfinding::DIR_UP);
		if (x != tileStop->getPosition().x || y != tileStop->getPosition().y)
		{
			x = tileStop->getPosition().x;
			y = tileStop->getPosition().y;
			int z = tileStart->getPosition().z+1;
			block += horizontalBlockage(tileStart, _save->getTile(Position(x, y, z)), dType, skipObject);
			block += blockage(_save->getTile(Position(x, y, z)), O_FLOOR, dType);
			if (!skipObject)
				block += blockage(_save->getTile(Position(x, y, z)), O_OBJECT, dType, Pathfinding::DIR_UP);
		}
	}
	return block;
} */

/**
 * Calculates the quantity of power or LoS/FoV/LoF blocked by various types of
 * walls or floors or object-parts/bigwalls of a Tile.
 * @param startTile		- pointer to tile where the power starts
 * @param partType		- the part of the tile that the power tries to go through (MapData.h)
 * @param dType			- the type of power (RuleItem.h) DT_NONE if line-of-vision
 * @param dir			- direction the power travels	-1	walls & floors (default -1)
 *														 0+	big-walls & content
 *						  Note: Think of 'dir' as the edge or side of the current Tile
 *						  (not as a direction of travel).
 * @param isStartTile	- true if the start tile is being examined for bigWalls;
 *						  used only when dir is specified (default false)
 * @param isTrueDir		- for checking if dir is *really* from the direction of sight (true)
 *						  or, in the case of some bigWall determinations, perpendicular to it (false);
 *						  used only when dir is specified (default false)
 * @return, 0			- no block
 *			1+			- variable power blocked
 *			HARD_BLOCK	- hard-block for power or vision
 */
int TileEngine::blockage( // private.
		const Tile* const tile,
		const MapDataType partType,
		/*const*/ DamageType dType,
		const int dir,
		const bool isStartTile,
		const bool isTrueDir) const
{
	if (tile != nullptr && tile->isSlideDoorOpen(partType) == false)
	{
		const MapData* const part (tile->getMapData(partType));
		if (part != nullptr)
		{
			if (dType == DT_STUN) dType = DT_SMOKE; // TODO: Workaround until get MapData/MapDataSets are sorted out properly.


			bool visLike;
			switch (dType)
			{
				case DT_NONE:
				case DT_SMOKE:
				case DT_STUN:
				case DT_IN:
					visLike = true;
					break;

				default:
					visLike = false;
			}

			bool diagBigwallPass (false); // spaghetti strand #397

			//if (_debug) Log(LOG_INFO) << ". dir = " << dir << " getMapData(partType) stopLOS() = " << part->stopLOS();
			switch (dir)
			{
				case -1: // regular north/west wall (not BigWall), or it's a floor, or an object (incl. BigWall) vs up/down-diagonal.
					//if (_debug) Log(LOG_INFO) << ". dir -1";
					if (visLike == true)
					{
						switch (partType)
						{
							case O_WESTWALL:
							case O_NORTHWALL:
							case O_OBJECT:		// object-part is for verticalBlockage() only.
								switch (dType)	// TODO: Needs Gas/Stun dType added.
								{
									default:
									case DT_NONE:
										if (part->stopLOS() == true)
											return HARD_BLOCK;
										break;

									case DT_SMOKE:
										if (part->blockSmoke() == true)
											return HARD_BLOCK;
										break;

									case DT_IN:
										if (part->blockFire() == true)
											return HARD_BLOCK;
								}
								break;

							case O_FLOOR:			// Might want to check hasNoFloor() flags:
								return HARD_BLOCK;	// all floors that block LoS should have their stopLOS flag set true if not a gravLift-floor.

							//if (_debug) Log(LOG_INFO) << ". . . . dir = " << dir << " Ret 1000[0] partType = " << partType << " " << tile->getPosition();
						}
					}
					else if (part->stopLOS() == true // use stopLOS to hinder explosions from propagating through BigWalls freely.
						&& _powerE > -1
						&& _powerE < (part->getArmor() << 1u)) // terrain absorbs 200% damage from DT_HE!
					{
						//if (_debug) Log(LOG_INFO) << ". . . . dir = " << dir << " Ret 1000[1] partType = " << partType << " " << tile->getPosition();
						return HARD_BLOCK;
					}
					break;

				default: // (dir > -1) -> VALID object-part (incl. BigWalls) *always* gets passed in here and *with* a direction.
				{
					const MapData* const object (tile->getMapData(O_OBJECT));
					const BigwallType bigType (object->getBigwall()); // 0..9 or per MCD.
					//if (_debug) Log(LOG_INFO) << ". dir = " << dir << " bigWall = " << bigType;

					if (_powerE != -1) // allow explosions to move along diagonal bigwalls
					{
						switch (dir)
						{
							case 0:
								switch (bigType)
								{
									case BIGWALL_NESW:
										if (_dirRay > 134 && _dirRay < 316)
											diagBigwallPass = true;
										break;
									case BIGWALL_NWSE:
										if (_dirRay > 44 && _dirRay < 226)
											diagBigwallPass = true;
								}
								break;
							case 2:
								switch (bigType)
								{
									case BIGWALL_NESW:
										if ((_dirRay > -1 && _dirRay < 136) || (_dirRay > 314 && _dirRay < 361))
											diagBigwallPass = true;
										break;
									case BIGWALL_NWSE:
										if (_dirRay > 44 && _dirRay < 226)
											diagBigwallPass = true;
								}
								break;
							case 4:
								switch (bigType)
								{
									case BIGWALL_NESW:
										if ((_dirRay > -1 && _dirRay < 136) || (_dirRay > 314 && _dirRay < 361))
											diagBigwallPass = true;
										break;
									case BIGWALL_NWSE:
										if ((_dirRay > 224 && _dirRay < 361) || (_dirRay > -1 && _dirRay < 46))
											diagBigwallPass = true;
								}
								break;
							case 6:
								switch (bigType)
								{
									case BIGWALL_NESW:
										if (_dirRay > 134 && _dirRay < 316)
											diagBigwallPass = true;
										break;
									case BIGWALL_NWSE:
										if ((_dirRay > 224 && _dirRay < 361) || (_dirRay > -1 && _dirRay < 46))
											diagBigwallPass = true;
								}
						}
					}

					if (isStartTile == true) // the object already got hit as the previous StopTile but can still block LoS when looking down.
					{
						//if (_debug) Log(LOG_INFO) << ". . isStartTile";
//						bool diagStop = true; // <- superceded by ProjectileFlyBState::_prjVector ->
//						if (dType == DT_HE && _missileDirection != -1)
//						{
//							const int dirDelta = std::abs(8 + _missileDirection - dir) % 8;
//							diagStop = (dirDelta < 2 || dirDelta > 6);
//						}
//						else diagStop = true;
						// this needs to check which side the *missile* is coming from,
						// although grenades that land on a diagonal BigWall are exempt regardless!!!
//							|| (diagStop == false && (bigType == BIGWALL_NESW || bigType == BIGWALL_NWSE))

						if (bigType == BIGWALL_NONE) // for non-visLike ... but if (only non-BigWall object) no dTypes are blocked here because, origin.
						{
							//if (_debug) Log(LOG_INFO) << ". . . Bigwall_None ret 0";
							return 0;
						}

						if (visLike == true && dir == Pathfinding::DIR_DOWN) // check if object blocks visLike
						{
							//if (_debug) Log(LOG_INFO) << ". . . visLike & Dir_Down";
							switch (dType) // TODO: Needs Gas/Stun dType added.
							{
								case DT_NONE:
									if (object->stopLOS() == false)
									{
										//if (_debug) Log(LOG_INFO) << ". . . . DT_None/no stopLOS ret 0";
										return 0;
									}
									break;

								case DT_SMOKE:
									if (object->blockSmoke() == false)
									{
										//if (_debug) Log(LOG_INFO) << ". . . . DT_Smoke/no blockSmoke ret 0";
										return 0;
									}
									break;

								case DT_IN:
									if (object->blockFire() == false)
									{
										//if (_debug) Log(LOG_INFO) << ". . . . DT_INC/no blockFire ret 0";
										return 0;
									}
							}
						}
						else if (visLike == false && diagBigwallPass == false) // check diagonal BigWall HE blockage ...
						{
							//if (_debug) Log(LOG_INFO) << ". . . NOT visLike & no DiagPass";
							switch (bigType)
							{
								case BIGWALL_NESW:
								case BIGWALL_NWSE:
									if (object->stopLOS() == true // use stopLOS to hinder explosions from propagating through BigWalls freely.
										&& _powerE > -1
										&& _powerE < (object->getArmor() << 1u))
									{
										//if (_debug) Log(LOG_INFO) << ". . . . dir = " << dir
										//		<< " HARD_BLOCK partType = " << partType << " " << tile->getPosition();
										return HARD_BLOCK;
									}
							}
						}
					}

					if (visLike == true && bigType == BIGWALL_NONE) // hardblock for visLike against non-BigWall object-part.
					{
						//if (_debug) Log(LOG_INFO) << ". . visLike & Bigwall_None";
						switch (dType) // TODO: Needs Gas/Stun dType added.
						{
							case DT_NONE:
								if (object->stopLOS() == true)
								{
									//if (_debug) Log(LOG_INFO) << ". . . DT_None/stopLOS ret HARD_BLOCK";
									return HARD_BLOCK;
								}
								break;

							case DT_SMOKE:
								if (object->blockSmoke() == true)
								{
									//if (_debug) Log(LOG_INFO) << ". . . DT_Smoke/blockSmoke ret HARD_BLOCK";
									return HARD_BLOCK;
								}
								break;

							case DT_IN:
								if (object->blockFire() == true)
								{
									//if (_debug) Log(LOG_INFO) << ". . . DT_INC/blockFire ret HARD_BLOCK";
									return HARD_BLOCK;
								}

							//if (_debug) Log(LOG_INFO) << ". . . . dir = " << dir << " Ret 1000[3] partType = " << partType << " " << tile->getPosition();
						}
					}


					switch (dir) // -> object-part (incl. BigWalls)
					{
						case 0: // north
							if (diagBigwallPass == true)
								return 0; // partType By-passed.

							switch (bigType)
							{
								case BIGWALL_WEST:
								case BIGWALL_EAST:
								case BIGWALL_SOUTH:
								case BIGWALL_E_S:
									//if (_debug) Log(LOG_INFO) << "TileEngine::blockage() EXIT, ret 0 ( dir 0 north )";
									return 0; // partType By-passed.
							}
							break;

						case 1: // north east
							switch (bigType)
							{
								case BIGWALL_NWSE:
									if (isTrueDir == true) break;
								case BIGWALL_WEST:
								case BIGWALL_SOUTH:
									//if (_debug) Log(LOG_INFO) << "TileEngine::blockage() EXIT, ret 0 ( dir 1 northeast )";
									return 0;
							}
							break;

						case 2: // east
							if (diagBigwallPass == true)
								return 0;

							switch (bigType)
							{
								case BIGWALL_NORTH:
								case BIGWALL_SOUTH:
								case BIGWALL_WEST:
//								case BIGWALL_W_N: // NOT USED in stock UFO.
									//if (_debug) Log(LOG_INFO) << "TileEngine::blockage() EXIT, ret 0 ( dir 2 east )";
									return 0;
							}
							break;

						case 3: // south east
							switch (bigType)
							{
								case BIGWALL_NESW:
									if (isTrueDir == true) break;
								case BIGWALL_NORTH:
								case BIGWALL_WEST:
//								case BIGWALL_W_N: // NOT USED in stock UFO.
									//if (_debug) Log(LOG_INFO) << "TileEngine::blockage() EXIT, ret 0 ( dir 3 southeast )";
									return 0;
							}
							break;

						case 4: // south
							if (diagBigwallPass == true)
								return 0;

							switch (bigType)
							{
								case BIGWALL_WEST:
								case BIGWALL_EAST:
								case BIGWALL_NORTH:
//								case BIGWALL_W_N: // NOT USED in stock UFO.
									//if (_debug) Log(LOG_INFO) << "TileEngine::blockage() EXIT, ret 0 ( dir 4 south )";
									return 0;
							}
							break;

						case 5: // south west
							switch (bigType)
							{
								case BIGWALL_NWSE:
									if (isTrueDir == true) break;
								case BIGWALL_NORTH:
								case BIGWALL_EAST:
									//if (_debug) Log(LOG_INFO) << "TileEngine::blockage() EXIT, ret 0 ( dir 5 southwest )";
									return 0;
							}
							break;

						case 6: // west
							if (diagBigwallPass == true)
								return 0;

							switch (bigType)
							{
								case BIGWALL_NORTH:
								case BIGWALL_SOUTH:
								case BIGWALL_EAST:
								case BIGWALL_E_S:
									//if (_debug) Log(LOG_INFO) << "TileEngine::blockage() EXIT, ret 0 ( dir 6 west )";
									return 0;
							}
							break;

						case 7: // north west
							switch (bigType)
							{
								case BIGWALL_NESW:
									if (isTrueDir == true) break;
								case BIGWALL_SOUTH:
								case BIGWALL_EAST:
								case BIGWALL_E_S:
									//if (_debug) Log(LOG_INFO) << "TileEngine::blockage() EXIT, ret 0 ( dir 7 northwest )";
									return 0;
							}
							break;

						case Pathfinding::DIR_UP:	// #8
						case Pathfinding::DIR_DOWN:	// #9
							switch (bigType)
							{
//								BIGWALL_NONE		// 0 // let object-parts Block explosions
//								BIGWALL_BLOCK		// 1 // includes stopLoS (floors handled above under non-directional condition)
								case BIGWALL_NESW:	// 2
								case BIGWALL_NWSE:	// 3
								case BIGWALL_WEST:	// 4
								case BIGWALL_NORTH:	// 5
								case BIGWALL_EAST:	// 6
								case BIGWALL_SOUTH:	// 7
								case BIGWALL_E_S:	// 8
//								case BIGWALL_W_N	// 9 NOT USED in stock UFO.
									//if (_debug) Log(LOG_INFO) << "TileEngine::blockage() EXIT, ret 0 ( dir 8/9 up/down )";
									return 0;
							}

							if (visLike == true)
							{
								switch (dType) // TODO: Needs Gas/Stun dType added.
								{
									case DT_NONE:
										if (object->stopLOS() == false)
										{
											//if (_debug) Log(LOG_INFO) << ". . DT_None/noStopLOS ret 0 ( dir 8/9 up/down )";
											return 0;
										}
										break;

									case DT_SMOKE:
										if (object->blockSmoke() == false)
										{
											//if (_debug) Log(LOG_INFO) << ". . DT_Smoke/no blockSmoke ret 0 ( dir 8/9 up/down )";
											return 0;
										}
										break;

									case DT_IN:
										if (object->blockFire() == false)
										{
											//if (_debug) Log(LOG_INFO) << ". . DT_Fire/no blockFire ret 0 ( dir 8/9 up/down )";
											return 0;
										}

									//if (_debug) Log(LOG_INFO) << "TileEngine::blockage() EXIT, ret 0 ( dir 8,9 up,down )";
								}
							}
//							break;
//
//						default:
//							return 0; // .....
					}


					// could be object-part or BigWalls block here
					if (visLike == true
						|| (diagBigwallPass == false
							&& _powerE > -1
							&& _powerE < (object->getArmor() << 1u))) // terrain absorbs 200% damage from DT_HE.
					{
						switch (dType)
						{
							case DT_SMOKE:
								if (object->blockSmoke() == true)
								{
									//if (_debug) Log(LOG_INFO) << ". DT_Smoke/blockSmoke HARD_BLOCK";
									return HARD_BLOCK;
								}
								break;

							case DT_IN:
								if (object->blockFire() == true)
								{
									//if (_debug) Log(LOG_INFO) << ". DT_INC/blockFire HARD_BLOCK";
									return HARD_BLOCK;
								}
								break;

							default: // use stopLOS to hinder explosions from propagating through BigWalls freely.
								if (object->stopLOS() == true)
								{
									//if (_debug) Log(LOG_INFO) << ". default/no stopLOS HARD_BLOCK";
									return HARD_BLOCK;
								}

							//if (_debug) Log(LOG_INFO) << ". . . . dir = " << dir << " isTrueDir = " << isTrueDir
							//		<< " Ret 1000[4] partType = " << partType << " " << tile->getPosition();
						}
					}
				}
			}

			if (visLike == false && diagBigwallPass == false)	// Only non-visLike can get partly blocked; other dTypes
			{													// are either completely blocked above^ or get a pass here.
				//if (_debug) Log(LOG_INFO) << "TileEngine::blockage() EXIT, ret = " << part->getBlock(dType);
				return part->getBlock(dType);
			}
		}
	}
	//if (_debug) Log(LOG_INFO) << "TileEngine::blockage() EXIT, (no tile OR no valid partType OR ufo-door open) ret 0";
	return 0; // no Valid [partType].
}

/**
 * Sets the final direction from which a missile or thrown-object came;
 * for use in determining blast propagation against diagonal BigWalls.
 * This needs to be stored because the Projectile itself is long gone
 * once ExplosionBState starts.
 * @param dir - the direction as calculated in Projectile
 *
void TileEngine::setProjectileDirection(const int dir)
{
	_missileDirection = dir;
} */

/**
 * Applies the explosive power to tile-parts.
 * @note This is where the actual destruction takes place.
 * 9 parts are affected:
 * - 2 walls
 * - floors top & bottom
 * - up to 4 bigWalls around the perimeter
 * - plus the content-object in the center
 * @param tile - pointer to Tile affected
 */
void TileEngine::detonateTile(Tile* const tile) const
{
	int power (tile->getExplosive());	// <- power that hit tile.
	if (power == 0) return;				// <- no explosive applied to tile

	//bool debug (tile->getPosition() == Position(20,30,3));
	//Log(LOG_INFO) << "";
	//if (debug) Log(LOG_INFO) << "TileEngine::detonateTile() " << tile->getPosition() << " power = " << power;
	tile->clearExplosive(); // reset Tile's '_explosive' value to 0


	const Position pos (tile->getPosition());

	Tile* tiles[9u];
	tiles[0u] = _battleSave->getTile(Position(		// tileAbove, do floor
										pos.x,
										pos.y,
										pos.z + 1));
	tiles[1u] = _battleSave->getTile(Position(		// tileEast, do westwall
										pos.x + 1,
										pos.y,
										pos.z));
	tiles[2u] = _battleSave->getTile(Position(		// tileSouth, do northwall
										pos.x,
										pos.y + 1,
										pos.z));
	tiles[3u] =										// central tile, do floor
	tiles[4u] =										// central tile, do westwall
	tiles[5u] =										// central tile, do northwall
	tiles[6u] = tile;								// central tile, do object

	tiles[7u] = _battleSave->getTile(Position(		// tileNorth, do bigwall south
										pos.x,
										pos.y - 1,
										pos.z));
	tiles[8u] = _battleSave->getTile(Position(		// tileWest, do bigwall east
										pos.x - 1,
										pos.y,
										pos.z));
	static const MapDataType parts[9u]
	{
		O_FLOOR,		// 0 - in tileAbove
		O_WESTWALL,		// 1 - in tileEast
		O_NORTHWALL,	// 2 - in tileSouth
		O_FLOOR,		// 3 - in central tile
		O_WESTWALL,		// 4 - in central tile
		O_NORTHWALL,	// 5 - in central tile
		O_OBJECT,		// 6 - in central tile
		O_OBJECT,		// 7 - in tileNorth, bigwall south
		O_OBJECT		// 8 - in tileWest, bigwall east
	};

	int
		powerTest,
		density (0),
		dieMCD;

	MapDataType
		partTest,
		partT;

	bool diagWallDestroyed (true);

	MapData* part;

	Tile* tileTest;
	for (size_t
			i = 8u;
			i != std::numeric_limits<size_t>::max();
			--i)
	{
		if ((tileTest = tiles[i]) == nullptr
			|| ((partTest = parts[i]) == O_FLOOR && diagWallDestroyed == false) // don't hit Floor if there's still a BigWall
			|| (part = tileTest->getMapData(partTest)) == nullptr)
		{
			continue;
		}

		if (tile->getMapData(O_OBJECT) != nullptr) // if central tile has object-part
		{
			switch (tile->getMapData(O_OBJECT)->getBigwall())
			{
				case BIGWALL_E_S: // don't hit tileEast westwall or tileSouth northwall if s/e bigWall exists
					if (i == 1u || i == 2u) continue;
					break;

				case BIGWALL_EAST: // don't hit tileEast westwall if eastern bigWall exists
					if (i == 1u) continue;
					break;

				case BIGWALL_SOUTH: // don't hit tileSouth northwall if southern bigWall exists
					if (i == 2u) continue;
			}
		}

		const BigwallType bigWall (part->getBigwall());

		switch (i) // don't hit a tile-part that's not supposed to get hit in that tile.
		{
			case 7u: // tileNorth bigwall south
			case 8u: // tileWest bigwall east
				switch (bigWall)
				{
//					BIGWALL_BLOCK	// <- always hit this
//					BIGWALL_E_S		// <- always hit this

					case BIGWALL_NONE: // never hit these if (i= 7 or i= 8)
					case BIGWALL_NESW:
					case BIGWALL_NWSE:
					case BIGWALL_WEST:
					case BIGWALL_NORTH:
//					case BIGWALL_W_N: // NOT USED in stock UFO.
						continue;

					case BIGWALL_SOUTH:
						if (i != 7u) continue;
						break;

					case BIGWALL_EAST:
						if (i != 8u) continue;
				}
		}


		powerTest = power;

		if (i == 6u)
		{
			switch (bigWall)
			{
				case BIGWALL_NESW: // diagonals
				case BIGWALL_NWSE:
					if ((part->getArmor() << 1u) > powerTest) // not enough to destroy
						diagWallDestroyed = false;
			}
		}

		// Check tile-part's HP then iterate through and destroy its death-tiles if enough powerTest.
		while (part != nullptr
			&& part->getArmor() != 255
			&& (part->getArmor() << 1u) <= powerTest)
		{
			if (powerTest == power) // only once per initial part destroyed.
			{
				for (size_t // get a yes/no volume for the object by checking its LoFT-layers.
						j = 0u;
						j != LOFT_LAYERS;
						++j)
				{
					if (part->getLoftId(j) != 0)
						++density;
				}
			}

			powerTest -= part->getArmor() << 1u;

			if (i == 6u)
			{
				switch (bigWall)
				{
					case BIGWALL_NESW: // diagonals for the central tile
					case BIGWALL_NWSE:
						diagWallDestroyed = true;
				}
			}

			if (_battleSave->getTacType() == TCT_BASEDEFENSE
				&& part->isBaseObject() == true)
			{
				_battleSave->baseDestruct()[static_cast<size_t>(tile->getPosition().x / 10)]
										   [static_cast<size_t>(tile->getPosition().y / 10)].second--;
			}

			// This tracks dead object-parts (object-part can become a floor-part ... unless your MCDs are correct!)
			if ((dieMCD = part->getDieMCD()) != 0)
				partT = part->getDataset()->getRecords()->at(static_cast<size_t>(dieMCD))->getPartType();
			else
				partT = partTest;

			tileTest->destroyTilepart(partTest, _battleSave); // DESTROY HERE <-|||

			part = tileTest->getMapData(partTest = partT);
		}
	}


	power = (power + 29) / 30;

	if (tile->igniteTile((power + 1) >> 1u) == false)
		tile->addSmoke((power + density + 1) >> 1u);

	if (tile->getSmoke() != 0) // add smoke to tiles above
	{
		Tile* const tileAbove (_battleSave->getTile(tile->getPosition() + Position(0,0,1)));
		if (tileAbove != nullptr
			&& tileAbove->hasNoFloor(tile) == true // TODO: use verticalBlockage() instead
			&& RNG::percent(tile->getSmoke() << 3u) == true) // unfortunately the state-machine may cause an unpredictable quantity of calls to this ... via ExplosionBState::think().
		{
			tileAbove->addSmoke(tile->getSmoke() / 3);
		}
	}
}

/**
 * Checks for chained explosions.
 * @note Chained explosions are explosions which occur after an explosive object
 * is destroyed. May be due a direct hit, other explosion or fire.
 * @return, tile on which an explosion is about to occur
 */
Tile* TileEngine::checkForTerrainExplosives() const
{
	Tile* tile;
	for (size_t
			i = 0u;
			i != _battleSave->getMapSizeXYZ();
			++i)
	{
		tile = _battleSave->getTiles()[i];
		if (tile->getExplosive() != 0)
			return tile;
	}
	return nullptr;
}

/**
 * Opens a door if any by rightclick or by walking through it.
 * @note The unit has to face in the right direction.
 * @param unit		- pointer to a BattleUnit trying the door
 * @param rtClick	- true if the player right-clicked (default true)
 * @param dir		- direction to check for a door (default -1)
 * @return, DoorResult (Tile.h)
 *			-1 there is no door or you're a tank and can't do sweet shit except blast the fuck out of it
 *			 0 wood door opened so make a squeaky sound and walk through
 *			 1 ufo door is starting to open so make a whoosh sound but don't walk through yet
 *			 2 ufo door is still opening so don't walk through yet (have patience futuristic technology)
 *			 3 not enough TU
 *			 4 would contravene reserve TU
 */
DoorResult TileEngine::unitOpensDoor(
		BattleUnit* const unit,
		const bool rtClick,
		int dir)
{
	//Log(LOG_INFO) << "unitOpensDoor()";
	if (dir == -1)
		dir = unit->getUnitDirection();

	if (rtClick == true // RMB works only for cardinal directions, and not for dogs.
		&& (dir % 2 == 1
			|| (unit->getUnitRules() != nullptr
				&& unit->getUnitRules()->hasHands() == false)))
	{
		return DR_NONE;
	}

	Tile
		* tile,
		* tileDoor (nullptr);
	Position
		pos,
		posDoor;
	MapDataType partType (O_FLOOR); // avoid vc++ linker warning.
	bool calcTu (false);
	int
		tuCost (0),
		z;
	DoorResult ret (DR_NONE);

	if (unit->getUnitTile()->getTerrainLevel() < -12)
		z = 1; // if standing on stairs check the tile above instead
	else
		z = 0;

	const int unitSize (unit->getArmor()->getSize());
	for (int
			x = 0;
			x != unitSize && ret == DR_NONE;
			++x)
	{
		for (int
				y = 0;
				y != unitSize && ret == DR_NONE;
				++y)
		{
			pos = unit->getPosition() + Position(x,y,z);
			tile = _battleSave->getTile(pos);
			////Log(LOG_INFO) << ". iter unitSize " << pos;

			if (tile != nullptr)
			{
				std::vector<std::pair<Position, MapDataType>> wallCheck;
				switch (dir)
				{
					case 0: // north
							wallCheck.push_back(std::make_pair(Position(0, 0, 0), O_NORTHWALL));	// origin
						if (x != 0)
							wallCheck.push_back(std::make_pair(Position(0,-1, 0), O_WESTWALL));		// one tile north
						break;

					case 1: // north east
							wallCheck.push_back(std::make_pair(Position(0, 0, 0), O_NORTHWALL));	// origin
							wallCheck.push_back(std::make_pair(Position(1,-1, 0), O_WESTWALL));		// one tile north-east
						break;

					case 2: // east
							wallCheck.push_back(std::make_pair(Position(1, 0, 0), O_WESTWALL));		// one tile east
						break;

					case 3: // south-east
						if (y == 0)
							wallCheck.push_back(std::make_pair(Position(1, 1, 0), O_WESTWALL));		// one tile south-east
						if (x == 0)
							wallCheck.push_back(std::make_pair(Position(1, 1, 0), O_NORTHWALL));	// one tile south-east
						break;

					case 4: // south
							wallCheck.push_back(std::make_pair(Position(0, 1, 0), O_NORTHWALL));	// one tile south
						break;

					case 5: // south-west
							wallCheck.push_back(std::make_pair(Position( 0, 0, 0), O_WESTWALL));	// origin
							wallCheck.push_back(std::make_pair(Position(-1, 1, 0), O_NORTHWALL));	// one tile south-west
						break;

					case 6: // west
							wallCheck.push_back(std::make_pair(Position( 0, 0, 0), O_WESTWALL));	// origin
						if (y != 0)
							wallCheck.push_back(std::make_pair(Position(-1, 0, 0), O_NORTHWALL));	// one tile west
						break;

					case 7: // north-west
							wallCheck.push_back(std::make_pair(Position( 0, 0, 0), O_WESTWALL));	// origin
							wallCheck.push_back(std::make_pair(Position( 0, 0, 0), O_NORTHWALL));	// origin
						if (x != 0)
							wallCheck.push_back(std::make_pair(Position(-1,-1, 0), O_WESTWALL));	// one tile north
						if (y != 0)
							wallCheck.push_back(std::make_pair(Position(-1,-1, 0), O_NORTHWALL));	// one tile north
				}

				partType = O_FLOOR; // just a reset for 'partType'.

				for (std::vector<std::pair<Position, MapDataType>>::const_iterator
						i = wallCheck.begin();
						i != wallCheck.end();
						++i)
				{
					tileDoor = _battleSave->getTile(posDoor = pos + i->first);
					//Log(LOG_INFO) << ". . iter wallCheck " << posDoor << " partType = " << i->second;
					if (tileDoor != nullptr)
					{
						partType = i->second;
//						ret = tileDoor->openDoor(partType, unit); //_battleSave->getBatReserved());
						//Log(LOG_INFO) << ". . . openDoor = " << ret;

						switch (ret = tileDoor->openDoor(partType, unit)) //_battleSave->getBatReserved());
						{
							case DR_WOOD_OPEN:
								if (rtClick == true)
									calcTu = true;
									switch (partType)
									{
										default:
										case O_WESTWALL:
											partType = O_NORTHWALL;
											break;
										case O_NORTHWALL:
											partType = O_WESTWALL;
									}
								break;

							case DR_UFO_OPEN:
								openAdjacentDoors(posDoor, partType); // no break.
							case DR_ERR_TU:
								calcTu = true;
						}

						if (ret != DR_NONE) break;
					}
				}
			}
		}
	}

	//Log(LOG_INFO) << "tuCost = " << tuCost;
	if (calcTu == true)
	{
		tuCost = tileDoor->getTuCostTile(partType, unit->getMoveTypeUnit());
		//Log(LOG_INFO) << ". . ret = " << ret << ", partType = " << partType << ", tuCost = " << tuCost;

		if (unit->getFaction() == FACTION_PLAYER // <- no Reserve tolerance.
			|| _battleSave->getBattleGame()->checkReservedTu(unit, tuCost) == true)
		{
			//Log(LOG_INFO) << "check reserved tu";
			if (unit->expendTu(tuCost) == true)
			{
				//Log(LOG_INFO) << "spend tu";
				if (rtClick == true) // try this one ...... <-- let UnitWalkBState handle FoV & new unit visibility when walking (ie, not for RMB here).
				{
					//Log(LOG_INFO) << "RMB -> calcFoV";
					_battleSave->getBattleGame()->checkProxyGrenades(unit);

					pos = unit->getPosition();
					calcFovTiles_pos(pos); // calculate FoV for everyone within sight-range, incl. unit.
					calcFovUnits_pos(pos, true);

					// look from the other side, may need to check reaction fire
					// This seems redundant but hey maybe it removes now-unseen units from a unit's visible-units vector ....
					//
					// It is redundant. And instead of calcFoV for position it should be calcFoV for all.
//					const std::vector<BattleUnit*>* const hostileUnits (unit->getHostileUnits());
//					for (size_t
//							i = 0;
//							i != hostileUnits->size();
//							++i)
//					{
//						//Log(LOG_INFO) << "calcFoV hostile";
//						calcFov(hostileUnits->at(i)); // calculate FoV for all hostile units that are visible to this unit.
//					}
				}
			}
			else // not enough TU
			{
				//Log(LOG_INFO) << "unitOpensDoor() ret DR_ERR_TU";
				return DR_ERR_TU;
			}
		}
		else // reserved TU
		{
			//Log(LOG_INFO) << "unitOpensDoor() ret DR_ERR_RESERVE";
			return DR_ERR_RESERVE;
		}
	}

	//Log(LOG_INFO) << "unitOpensDoor() ret = " << ret;
	return ret;
}
/*				switch (dir)
				{
					case 0: // north
							checkPos.push_back(std::make_pair(Position(0, 0, 0), O_NORTHWALL));		// origin
						if (x != 0)
							checkPos.push_back(std::make_pair(Position(0,-1, 0), O_WESTWALL));		// one tile north
					break;

					case 1: // north east
							checkPos.push_back(std::make_pair(Position(0, 0, 0), O_NORTHWALL));		// origin
							checkPos.push_back(std::make_pair(Position(1,-1, 0), O_WESTWALL));		// one tile north-east
//						if (rtClick == true)
//						{
//							checkPos.push_back(std::make_pair(Position(1, 0, 0), O_WESTWALL));		// one tile east
//							checkPos.push_back(std::make_pair(Position(1, 0, 0), O_NORTHWALL));		// one tile east
//						} */
/*						if (rtClick
							|| testAdjacentDoor(posUnit, O_NORTHWALL, 1)) // kL
						{
							checkPos.push_back(std::make_pair(Position(0, 0, 0), O_NORTHWALL));		// origin
							checkPos.push_back(std::make_pair(Position(1,-1, 0), O_WESTWALL));		// one tile north-east
							checkPos.push_back(std::make_pair(Position(1, 0, 0), O_WESTWALL));		// one tile east
							checkPos.push_back(std::make_pair(Position(1, 0, 0), O_NORTHWALL));		// one tile east
						} *
					break;

					case 2: // east
							checkPos.push_back(std::make_pair(Position(1, 0, 0), O_WESTWALL));		// one tile east
					break;

					case 3: // south-east
						if (y == 0)
							checkPos.push_back(std::make_pair(Position(1, 1, 0), O_WESTWALL));		// one tile south-east
						if (x == 0)
							checkPos.push_back(std::make_pair(Position(1, 1, 0), O_NORTHWALL));		// one tile south-east
//						if (rtClick == true)
//						{
//							checkPos.push_back(std::make_pair(Position(1, 0, 0), O_WESTWALL));		// one tile east
//							checkPos.push_back(std::make_pair(Position(0, 1, 0), O_NORTHWALL));		// one tile south
//						} */
/*						if (rtClick
							|| testAdjacentDoor(posUnit, O_NORTHWALL, 3)) // kL
						{
							checkPos.push_back(std::make_pair(Position(1, 0, 0), O_WESTWALL));		// one tile east
							checkPos.push_back(std::make_pair(Position(0, 1, 0), O_NORTHWALL));		// one tile south
							checkPos.push_back(std::make_pair(Position(1, 1, 0), O_WESTWALL));		// one tile south-east
							checkPos.push_back(std::make_pair(Position(1, 1, 0), O_NORTHWALL));		// one tile south-east
						} *
					break;

					case 4: // south
							checkPos.push_back(std::make_pair(Position(0, 1, 0), O_NORTHWALL));		// one tile south
					break;

					case 5: // south-west
							checkPos.push_back(std::make_pair(Position( 0, 0, 0), O_WESTWALL));		// origin
							checkPos.push_back(std::make_pair(Position(-1, 1, 0), O_NORTHWALL));	// one tile south-west
//						if (rtClick == true)
//						{
//							checkPos.push_back(std::make_pair(Position(0, 1, 0), O_WESTWALL));		// one tile south
//							checkPos.push_back(std::make_pair(Position(0, 1, 0), O_NORTHWALL));		// one tile south
//						} */
/*						if (rtClick
							|| testAdjacentDoor(posUnit, O_NORTHWALL, 5)) // kL
						{
							checkPos.push_back(std::make_pair(Position( 0, 0, 0), O_WESTWALL));		// origin
							checkPos.push_back(std::make_pair(Position( 0, 1, 0), O_WESTWALL));		// one tile south
							checkPos.push_back(std::make_pair(Position( 0, 1, 0), O_NORTHWALL));	// one tile south
							checkPos.push_back(std::make_pair(Position(-1, 1, 0), O_NORTHWALL));	// one tile south-west
						} *
					break;

					case 6: // west
							checkPos.push_back(std::make_pair(Position( 0, 0, 0), O_WESTWALL));		// origin
						if (y != 0)
							checkPos.push_back(std::make_pair(Position(-1, 0, 0), O_NORTHWALL));	// one tile west
					break;

					case 7: // north-west
							checkPos.push_back(std::make_pair(Position( 0, 0, 0), O_WESTWALL));		// origin
							checkPos.push_back(std::make_pair(Position( 0, 0, 0), O_NORTHWALL));	// origin
						if (x != 0)
							checkPos.push_back(std::make_pair(Position(-1,-1, 0), O_WESTWALL));		// one tile north
						if (y != 0)
							checkPos.push_back(std::make_pair(Position(-1,-1, 0), O_NORTHWALL));	// one tile north
//						if (rtClick == true)
//						{
//							checkPos.push_back(std::make_pair(Position( 0,-1, 0), O_WESTWALL));		// one tile north
//							checkPos.push_back(std::make_pair(Position(-1, 0, 0), O_NORTHWALL));	// one tile west
//						} */
/*						if (rtClick
							|| testAdjacentDoor(posUnit, O_NORTHWALL, 7)) // kL
						{
							//Log(LOG_INFO) << ". north-west";
							checkPos.push_back(std::make_pair(Position( 0, 0, 0), O_WESTWALL));		// origin
							checkPos.push_back(std::make_pair(Position( 0, 0, 0), O_NORTHWALL));	// origin
							checkPos.push_back(std::make_pair(Position( 0,-1, 0), O_WESTWALL));		// one tile north
							checkPos.push_back(std::make_pair(Position(-1, 0, 0), O_NORTHWALL));	// one tile west
						} *
				} */

/**
 * Checks for a door connected to a wall at this position,
 * so that units can open double doors diagonally.
 * @param pos	- the starting position
 * @param part	- the wall to test
 * @param dir	- the direction to check out
 *
bool TileEngine::testAdjacentDoor(
		Position pos,
		int part,
		int dir)
{
	Position offset;
	switch (dir)
	{
		// only Northwall-doors are handled at present
		case 1: offset = Position( 1, 0, 0); break;	// northwall in tile to east
		case 3: offset = Position( 1, 1, 0); break;	// northwall in tile to south-east
		case 5: offset = Position(-1, 1, 0); break;	// northwall in tile to south-west
		case 7: offset = Position(-1, 0, 0);		// northwall in tile to west
	}

	const Tile* const tile (_battleSave->getTile(pos + offset));
	if (tile != nullptr
		&& tile->getMapData(part) != nullptr
		&& tile->getMapData(part)->isSlideDoor() == true)
	{
		return true;
	}

	return false;
} */

/**
 * Opens any doors connected to this wall at this position,
 * @note Keeps processing till it hits a non-ufo-door.
 * @param pos		- reference to the starting position
 * @param partType	- the wall to open (defines which direction to check)
 */
void TileEngine::openAdjacentDoors( // private.
		const Position& pos,
		MapDataType partType) const
{
	Tile* tile;
	Position offset;
	const bool westSide (partType == O_WESTWALL);

	for (int
			i = 1;
			;
			++i)
	{
		offset = (westSide == true) ? Position(0,i,0) : Position(i,0,0);
		if ((tile = _battleSave->getTile(pos + offset)) != nullptr
			&& tile->getMapData(partType) != nullptr
			&& tile->getMapData(partType)->isSlideDoor() == true)
		{
			tile->openAdjacentDoor(partType);
		}
		else
			break;
	}

	for (int
			i = -1;
			;
			--i)
	{
		offset = (westSide == true) ? Position(0,i,0) : Position(i,0,0);
		if ((tile = _battleSave->getTile(pos + offset)) != nullptr
			&& tile->getMapData(partType) != nullptr
			&& tile->getMapData(partType)->isSlideDoor() == true)
		{
			tile->openAdjacentDoor(partType);
		}
		else
			break;
	}
}

/**
 * Closes ufo-doors.
 * @return, true if a door closed
 */
bool TileEngine::closeSlideDoors() const
{
	int ret (false);
	Tile* tile;
	const Tile
		* tileNorth,
		* tileWest;
	const BattleUnit* unit;

	for (size_t
			i = 0u;
			i != _battleSave->getMapSizeXYZ();
			++i)
	{
		tile = _battleSave->getTiles()[i];

		if ((unit = tile->getTileUnit()) != nullptr
			&& unit->getArmor()->getSize() == 2)
		{
			if (tile->isSlideDoorOpen(O_NORTHWALL) == true
				&& (tileNorth = _battleSave->getTile(tile->getPosition() + Position(0,-1,0))) != nullptr
				&& tileNorth->getTileUnit() != nullptr
				&& tileNorth->getTileUnit() == unit)
			{
				continue;
			}

			if (tile->isSlideDoorOpen(O_WESTWALL) == true
				&& (tileWest = _battleSave->getTile(tile->getPosition() + Position(-1,0,0))) != nullptr
				&& tileWest->getTileUnit() != nullptr
				&& tileWest->getTileUnit() == unit)
			{
				continue;
			}
		}

		ret |= tile->closeSlideDoor();
	}
	return ret;
}

/**
 * Calculates a line trajectory using bresenham algorithm in 3D.
 * @note Accuracy is NOT considered; this is a true path/trajectory.
 * @param origin		- reference to the origin (voxel-space for 'doVoxelCheck'; tile-space otherwise)
 * @param target		- reference to the target (voxel-space for 'doVoxelCheck'; tile-space otherwise)
 * @param storeTrj		- true will store the whole trajectory; otherwise only the last position gets stored
 * @param trj			- pointer to a vector of Positions in which the trajectory will be stored
 * @param excludeUnit	- pointer to a BattleUnit to be excluded from collision detection
 * @param doVoxelCheck	- true to check against a voxel; false to check tile-blocking for FoV
 *						  (true for unit visibility and LoS/LoF; false for terrain visibility) (default true)
 * @param onlyVisible	- true to skip invisible units (default false) [used in FPS view]
 * @param excludeAllBut	- pointer to a unit that's to be considered exclusively for targeting (default nullptr)
 * @return, VoxelType (MapData.h)
 *			 -1 hit nothing
 *			0-3 tile-part
 *			  4 unit
 *			  5 out-of-map
 *		    6/7 special cases for calcFov() to remove or not the last tile in the trajectory
 * VOXEL_EMPTY			// -1
 * VOXEL_FLOOR			//  0
 * VOXEL_WESTWALL		//  1
 * VOXEL_NORTHWALL		//  2
 * VOXEL_OBJECT			//  3
 * VOXEL_UNIT			//  4
 * VOXEL_OUTOFBOUNDS	//  5
 * TRJ_STANDARD			//  6
 * TRJ_DECREASE			//  7
 */
VoxelType TileEngine::plotLine(
		const Position& origin,
		const Position& target,
		const bool storeTrj,
		std::vector<Position>* const trj,
		const BattleUnit* const excludeUnit,
		const bool doVoxelCheck, // false is used only for calcFov()
		const bool onlyVisible,
		const BattleUnit* const excludeAllBut) const
{
	//if (_debug) Log(LOG_INFO) << "TileEngine::plotLine()";
	VoxelType voxelType;

	int
		x,x0,x1,
		y,y0,y1,
		z,z0,z1,

		drift_xy,
		drift_xz,

		cx,cy,cz,

		horiBlock,
		vertBlock;

	Position posStart (origin); // for FoV only ->
	Tile
		* tileStart,
		* tileStop;

	x0 = origin.x; // start & end points
	x1 = target.x;

	y0 = origin.y;
	y1 = target.y;

	z0 = origin.z;
	z1 = target.z;

	const bool swap_xy (std::abs(y1 - y0) > std::abs(x1 - x0)); // step x/y plane, make longest delta along the x-axis
	if (swap_xy == true)
	{
		std::swap(x0,y0);
		std::swap(x1,y1);
	}

	const bool swap_xz (std::abs(z1 - z0) > std::abs(x1 - x0)); // step x/z plane, make longest delta along the x-axis
	if (swap_xz == true)
	{
		std::swap(x0,z0);
		std::swap(x1,z1);
	}

	const int
		delta_x (std::abs(x1 - x0)), // delta is length in each plane
		delta_y (std::abs(y1 - y0)),
		delta_z (std::abs(z1 - z0)),

		step_x ((x0 > x1) ? -1 : 1), // direction of line
		step_y ((y0 > y1) ? -1 : 1),
		step_z ((z0 > z1) ? -1 : 1);

	drift_xy =					// drift controls when to step in shallow planes
	drift_xz = (delta_x >> 1u);	// starting value keeps line centered

	x = x0; y = y0; z = z0;		// starting point
	for (
			;					// step through longest delta that has been swapped to x-axis
			x != x1 + step_x;
			x += step_x)
	{
		cx = x; cy = y; cz = z;					// copy x/y/z

		if (swap_xz == true) std::swap(cx,cz);	// unswap - in reverse
		if (swap_xy == true) std::swap(cx,cy);

		if (storeTrj == true) // && trj != nullptr)
			trj->push_back(Position(cx,cy,cz));

		if (doVoxelCheck == true) // passes through this voxel, for Unit visibility & LoS/LoF
		{
			voxelType = voxelCheck(
								Position(cx,cy,cz),
								excludeUnit,
								false,
								onlyVisible,
								excludeAllBut);

			if (voxelType != VOXEL_EMPTY) // hit.
			{
				//if (_debug) Log(LOG_INFO) << "pL() ret[1] " << MapData::debugVoxelType(voxelType) << " vs"
				//						  << Position(cx,cy,cz) << " ts" << Position::toTileSpace(Position(cx,cy,cz));

//				if (trj != nullptr)					// store the position of impact
				trj->push_back(Position(cx,cy,cz));	// NOTE: This stores the final position twice if storeTrj=TRUE.
													// Cf. plotParabola() where that is explicitly not done.
				return voxelType;
			}
		}
		else // for Terrain visibility, ie. FoV / Fog of War.
		{
			tileStart = _battleSave->getTile(posStart);
			tileStop  = _battleSave->getTile(posStart = Position(cx,cy,cz));

			//if (_debug) Log(LOG_INFO) << "pL() tileStart" << posStart << " tileStop" << Position(cx,cy,cz);
			horiBlock = horizontalBlockage(
									tileStart,
									tileStop,
									DT_NONE);
			vertBlock = verticalBlockage(
									tileStart,
									tileStop,
									DT_NONE);
			//if (_debug) {
			//	Log(LOG_INFO) << ". horiBlock= " << horiBlock;
			//	Log(LOG_INFO) << ". vertBlock= " << vertBlock; }

			if (horiBlock < 0) // hit object-part
			{
				if (vertBlock < 1) return TRJ_STANDARD;
				horiBlock = 0;
			}
			if (horiBlock + vertBlock != 0) return TRJ_DECREASE;
		}

		if ((drift_xy -= delta_y) < 0) // step along y-axis
		{
			y += step_y;

			if (doVoxelCheck == true) // check for xy diagonal intermediate voxel step, for Unit visibility
			{
				cx = x; cy = y; cz = z;					// copy x/y/z

				if (swap_xz == true) std::swap(cx,cz);
				if (swap_xy == true) std::swap(cx,cy);

				voxelType = voxelCheck(
									Position(cx,cy,cz),
									excludeUnit,
									false,
									onlyVisible,
									excludeAllBut);

				if (voxelType != VOXEL_EMPTY)
				{
					//if (_debug) Log(LOG_INFO) << "pL() ret[2] " << MapData::debugVoxelType(voxelType) << " vs"
					//							<< Position(cx,cy,cz) << " ts" << Position::toTileSpace(Position(cx,cy,cz));

//					if (trj != nullptr)
					trj->push_back(Position(cx,cy,cz)); // store the position of impact

					return voxelType;
				}
			}
			drift_xy += delta_x;
		}

		if ((drift_xz -= delta_z) < 0) // step along z-axis
		{
			z += step_z;

			if (doVoxelCheck == true) // check for x/z diagonal intermediate voxel step
			{
				cx = x; cy = y; cz = z;					// copy x/y/z

				if (swap_xz == true) std::swap(cx,cz);
				if (swap_xy == true) std::swap(cx,cy);

				voxelType = voxelCheck(
									Position(cx,cy,cz),
									excludeUnit,
									false,
									onlyVisible,
									excludeAllBut);

				if (voxelType != VOXEL_EMPTY)
				{
					//if (_debug) Log(LOG_INFO) << "pL() ret[3] " << MapData::debugVoxelType(voxelType) << " vs"
					//						  << Position(cx,cy,cz) << " ts" << Position::toTileSpace(Position(cx,cy,cz));

//					if (trj != nullptr)
					trj->push_back(Position(cx,cy,cz));	// store the position of impact

					return voxelType;
				}
			}
			drift_xz += delta_x;
		}
	}

	//if (_debug) Log(LOG_INFO) << "ret VOXEL_EMPTY";
	return VOXEL_EMPTY;
}

/**
 * Calculates a parabolic trajectory for thrown items and arcing shots.
 * @note Accuracy is NOT considered; this is a true path/trajectory.
 * @param originVoxel	- reference to the origin in voxelspace
 * @param targetVoxel	- reference to the target in voxelspace
 * @param storeTrj		- true will store the whole trajectory,
 *						  otherwise it stores the last position only
 * @param trj			- pointer to a vector of Positions in which the
 *						  trajectory will be stored
 * @param excludeUnit	- pointer to a unit to exclude, makes sure the
 *						  trajectory does not hit the shooter itself
 * @param arc			- how high the parabola goes:
 *						  1.0 is almost straight throw
 *						  3.0 is a very high throw to throw over a fence eg.
 * @param allowCeil		- true to allow arching shots to hit a ceiling ... (default false)
 * @param deltaVoxel	- reference to the deviation of the angles that should be taken
 *						  into account (0,0,0) is perfection (default Position(0,0,0))
 * @return, VoxelType (MapData.h)
 *			 -1 hit nothing
 *			0-3 tile-part (floor / westwall / northwall / object)
 *			  4 unit
 *			  5 out-of-map
 * VOXEL_EMPTY			// -1
 * VOXEL_FLOOR			//  0
 * VOXEL_WESTWALL		//  1
 * VOXEL_NORTHWALL		//  2
 * VOXEL_OBJECT			//  3
 * VOXEL_UNIT			//  4
 * VOXEL_OUTOFBOUNDS	//  5
 */
VoxelType TileEngine::plotParabola(
		const Position& originVoxel,
		const Position& targetVoxel,
		const bool storeTrj,
		std::vector<Position>* const trj,
		const BattleUnit* const excludeUnit,
		const double arc,
		const bool allowCeil,
		const Position& deltaVoxel) const
{
	//if (_debug) Log(LOG_INFO) << "TileEngine::plotParabola()";

	const double ro (std::sqrt(static_cast<double>(
					(targetVoxel.x - originVoxel.x) * (targetVoxel.x - originVoxel.x)
				  + (targetVoxel.y - originVoxel.y) * (targetVoxel.y - originVoxel.y)
				  + (targetVoxel.z - originVoxel.z) * (targetVoxel.z - originVoxel.z))));

	if (AreSame(ro, 0.)) // jic.
		return VOXEL_EMPTY;

	double
		fi (std::acos(static_cast<double>(targetVoxel.z - originVoxel.z) / ro)),
		te (std::atan2(
				static_cast<double>(targetVoxel.y - originVoxel.y),
				static_cast<double>(targetVoxel.x - originVoxel.x)));

	te += deltaVoxel.x * M_PI / (ro * 2.);							// horizontal
	fi += (deltaVoxel.z + deltaVoxel.y) * M_PI * arc / (ro * 14.);	// vertical

	const double
		zA (std::sqrt(ro) * arc),
		zK (4. * zA / (ro * ro));

	int
		x (originVoxel.x),
		y (originVoxel.y),
		z (originVoxel.z);
	double d (8.);

	Position
		startVoxel (Position(x,y,z)),
		stopVoxel,
		stopPlotLine;

	const Position posTarget (Position::toTileSpace(targetVoxel));
	const int posVoxelZ (posTarget.z * 24 + FLOOR_TLEVEL);

	VoxelType voxelType;

	std::vector<Position> trjPlotLine;

	while (z > -1) // while airborne ->
	{
		x = static_cast<int>(static_cast<double>(originVoxel.x) + d * std::cos(te) * std::sin(fi));
		y = static_cast<int>(static_cast<double>(originVoxel.y) + d * std::sin(te) * std::sin(fi));
		z = static_cast<int>(static_cast<double>(originVoxel.z) + d * std::cos(fi)
			- zK * (d - ro / 2.) * (d - ro / 2.)
			+ zA);

		stopVoxel = Position(x,y,z);
		//if (_debug) Log(LOG_INFO) << "pP() stopVoxel " << stopVoxel;

		if (storeTrj == true) trj->push_back(stopVoxel); // add current voxel.

		trjPlotLine.clear();
		//if (_debug) Log(LOG_INFO) << "pP() do plotLine";
		voxelType = plotLine(
							startVoxel,
							stopVoxel,
							false,
							&trjPlotLine, // NOTE: This already handles karadoc's core fix.
							excludeUnit);
		//if (_debug) Log(LOG_INFO) << "pP() plotLine DONE";

		if (trjPlotLine.empty() == false)
			stopPlotLine = trjPlotLine.at(0u);
		else
			stopPlotLine = stopVoxel;

		//if (_debug) Log(LOG_INFO) << "pP() stopPlotLine set";

		if (voxelType != VOXEL_EMPTY
			|| (   stopPlotLine.z < startVoxel.z
				&& stopPlotLine.z < posVoxelZ
				&& Position::toTileSpace(stopPlotLine) == posTarget))
		{
//			if (_debug)
//			{
//				Log(LOG_INFO) << "pP() (voxelType != VOXEL_EMPTY) = " << (voxelType != VOXEL_EMPTY);
//				Log(LOG_INFO) << "pP() (stopVoxel.z < startVoxel.z) = " << (stopPlotLine.z < startVoxel.z);
//				Log(LOG_INFO) << "pP() (stopVoxel.z < posVoxelZ) = " << (stopPlotLine.z < posVoxelZ);
//				Log(LOG_INFO) << "pP() (Position::toTileSpace(stopVoxel) == posTarget) = " << (Position::toTileSpace(stopPlotLine) == posTarget);
//			}

			if (allowCeil == false
				&& startVoxel.z < stopPlotLine.z)
			{
				//if (_debug) Log(LOG_INFO) << "pP() hit Ceiling set voxelType VOXEL_OUTOFBOUNDS " << Position::toTileSpace(stopPlotLine);
				voxelType = VOXEL_OUTOFBOUNDS; // <- do not stick to ceilings ....
			}

			if (storeTrj == false) trj->push_back(stopPlotLine); // final voxel only.

			//if (_debug) Log(LOG_INFO) << "pP() ret " << MapData::debugVoxelType(voxelType) << " vs"
			//						    << stopPlotLine << " ts" << Position::toTileSpace(stopPlotLine);
			return voxelType;
		}

		startVoxel = stopVoxel;
		d += 1.;
	}

	if (storeTrj == false) trj->push_back(Position(x,y,z)); // final voxel only.

	//if (_debug) Log(LOG_INFO) << "pP() EXIT - ret VOXEL_EMPTY " << Position::toTileSpace(Position(x,y,z));
	return VOXEL_EMPTY;
}

/**
 * Checks if a throw action is permissible.
 * @note Accuracy is NOT considered; this checks for a true path/trajectory.
 * @sa Projectile::calculateThrow()
 * @param action		- reference to the action to validate
 * @param originVoxel	- reference to the origin point of the action
 * @param targetVoxel	- reference to the target point of the action
 * @param pArc			- pointer to a curvature of the throw (default nullptr)
 * @param pType			- pointer to a type of voxel at which the trajectory terminates (default nullptr)
 * @return, true if throw is valid
 */
bool TileEngine::validateThrow(
		const BattleAction& action,
		const Position& originVoxel,
		const Position& targetVoxel,
		double* const pArc,
		VoxelType* const pType) const
{
//	if (_debug == true) _debug = false;
//	else if (Position::toTileSpace(targetVoxel) == Position(41,22,2)) _debug = true;
//	else _debug = false;
//	_debug = true;

//	if (_debug)
//	{
//		Log(LOG_INFO) << "";
//		Log(LOG_INFO) << "TileEngine::validateThrow() to vs" << targetVoxel << " ts" << Position::toTileSpace(targetVoxel);
//	}

	if (action.type == BA_THROW) // ie. Do not check the following for acid-spit, grenade-launcher, etc.
	{
		const Tile* const tile (_battleSave->getTile(action.posTarget)); // safety Off.

		if (validThrowRange(&action, originVoxel, tile) == false)
		{
			//if (_debug) Log(LOG_INFO) << ". range not valid - ret FALSE";
			return false;
		}

		// Prevent Grenades from landing on diagonal bigWalls.
		// See also Projectile::calculateThrow().
		if (tile->getMapData(O_OBJECT) != nullptr)
//			&& tile->getMapData(O_OBJECT)->getTuCostPart(MT_WALK) == 255
//			&& action.weapon->getRules()->isGrenade() == true)
		{
			switch (tile->getMapData(O_OBJECT)->getBigwall())
			{
				case BIGWALL_NESW:
				case BIGWALL_NWSE:
					//if (_debug) Log(LOG_INFO) << ". diag BigWalls not allowed - ret FALSE";
					return false;
			}
		}

		// This had me hunting through throwing-algorithms for hours. It turns
		// out that an item *can* be throw onto the upper tile of a gravLift if
		// there is a BattleUnit standing on it ... interesting quirk that.
		if (tile->getPosition().z != 0
			&& tile->getTileUnit() == nullptr // <---||
			&& tile->getMapData(O_FLOOR) != nullptr
			&& tile->getMapData(O_FLOOR)->isGravLift() == true)
		{
			const Tile* const tileBelow (_battleSave->getTile(tile->getPosition() + Position(0,0,-1)));
			if (tileBelow != nullptr
				&& tileBelow->getMapData(O_FLOOR) != nullptr
				&& tileBelow->getMapData(O_FLOOR)->isGravLift() == true)
			{
				//if (_debug) Log(LOG_INFO) << ". upper GravLift floor not allowed - ret FALSE";
				return false;
			}
		}
	}

	static const double ARC_DELTA (0.1);

	const Position posTarget (Position::toTileSpace(targetVoxel));
	double parabolicCoefficient_low; // higher parabolicCoefficient means higher arc IG. eh ......

	if (posTarget != Position::toTileSpace(originVoxel))
	{
		if (action.actor->isKneeled() == false)
			parabolicCoefficient_low = 1.; // 8 tiles when standing in a base corridor
		else
			parabolicCoefficient_low = 1.1; // 14 tiles, raise arc when kneeling else range in corridors is too far.
	}
	else
		parabolicCoefficient_low = 0.;
	//Log(LOG_INFO) << ". starting arc = " << parabolicCoefficient_low;
	//Log(LOG_INFO) << ". origin " << originVoxel << " target " << targetVoxel;


	// check for voxelType up from the lowest arc
	VoxelType voxelType;
	std::vector<Position> trj;

	bool arcGood (false);
	while (arcGood == false && parabolicCoefficient_low < 10.) // find an arc to destination
	{
//		if (_debug)
//		{
//			Log(LOG_INFO) << "";
//			Log(LOG_INFO) << "vT() arc[1] = " << parabolicCoefficient_low;
//		}
		trj.clear();
		voxelType = plotParabola(
							originVoxel,
							targetVoxel,
							false,
							&trj,
							action.actor,
							parabolicCoefficient_low,
							action.type != BA_THROW);
		//if (_debug) Log(LOG_INFO) << "vT() plotParabola()[1] = " << MapData::debugVoxelType(voxelType);

		switch (voxelType)
		{
			case VOXEL_EMPTY:
			case VOXEL_FLOOR:
			case VOXEL_OBJECT:
			case VOXEL_UNIT:
				if (Position::toTileSpace(trj.at(0u)) == posTarget)
				{
					//if (_debug) Log(LOG_INFO) << "vT() stop[1] pos" << Position::toTileSpace(trj.at(0u));
					arcGood = true;
					if (pType != nullptr) *pType = voxelType;
					break;
				}
				// no break;
			case VOXEL_WESTWALL:
			case VOXEL_NORTHWALL:
			case VOXEL_OUTOFBOUNDS:
				parabolicCoefficient_low += ARC_DELTA;
		}
	}

	if (parabolicCoefficient_low >= 10.)
	{
		//if (_debug) Log(LOG_INFO) << ". arc > 10 - ret FALSE";
		return false;
	}

	if (pArc != nullptr)
	{
		// arc continues rising to find upper limit
		double parabolicCoefficient_high (parabolicCoefficient_low + ARC_DELTA);
		arcGood = true;
		while (arcGood == true && parabolicCoefficient_high < 10.)	// TODO: should use (pC2 < pC+2.0) or so; this just needs to get
		{															// over the lower limit with some leeway - not 'to the moon'.
			//if (_debug) Log(LOG_INFO) << ". . arc[2] = " << parabolicCoefficient_high;
			trj.clear();
			voxelType = plotParabola(
								originVoxel,
								targetVoxel,
								false,
								&trj,
								action.actor,
								parabolicCoefficient_high,
								action.type != BA_THROW);
			//if (_debug) Log(LOG_INFO) << ". . plotParabola()[2] = " << MapData::debugVoxelType(voxelType);

			switch (voxelType)
			{
				case VOXEL_WESTWALL:
				case VOXEL_NORTHWALL:
				case VOXEL_OUTOFBOUNDS:
					arcGood = false;
					break;

				case VOXEL_EMPTY:
				case VOXEL_FLOOR:
				case VOXEL_OBJECT:
				case VOXEL_UNIT:
					if (Position::toTileSpace(trj.at(0u)) != posTarget)
					{
						//if (_debug) Log(LOG_INFO) << ". . . stop[2] pos" << Position::toTileSpace(trj.at(0u));
						arcGood = false;
					}
					else
						parabolicCoefficient_high += ARC_DELTA;
			}
		}

		// use the average of upper & lower limits:
		// Lessens chance of bouncing a thrown item back off a wall by barely skimming overtop once accuracy is applied.
		*pArc = (parabolicCoefficient_low + parabolicCoefficient_high - ARC_DELTA) / 2.; // back off from the upper limit
	}

	//if (pArc != nullptr) Log(LOG_INFO) << ". vT() ret TRUE arc = " << *pArc;
	//else Log(LOG_INFO) << ". vT() ret TRUE no arc requested";
	//if (_debug) Log(LOG_INFO) << "vT() EXIT - ret TRUE";
	return true;
}

/**
 * Validates the throwing range.
 * @param action		- pointer to BattleAction (BattlescapeGame.h)
 * @param originVoxel	- reference to the origin in voxel-space
 * @param tile			- pointer to the targeted tile
 * @return, true if the range is valid
 */
bool TileEngine::validThrowRange( // static.
		const BattleAction* const action,
		const Position& originVoxel,
		const Tile* const tile)
{
	const int
		delta_x (action->actor->getPosition().x - action->posTarget.x),
		delta_y (action->actor->getPosition().y - action->posTarget.y),
		distThrow (static_cast<int>(std::sqrt(static_cast<double>((delta_x * delta_x) + (delta_y * delta_y)))));

	int weight (action->weapon->getRules()->getWeight());
	if (action->weapon->getAmmoItem() != nullptr
		&& action->weapon->getAmmoItem() != action->weapon)
	{
		weight += action->weapon->getAmmoItem()->getRules()->getWeight();
	}

	const int deltaZ (originVoxel.z // throw up is neg./ throw down is pos.
				   - (action->posTarget.z * 24) + tile->getTerrainLevel());
	int dist ((getThrowDistance(
							weight,
							action->actor->getStrength(),
							deltaZ) + 8) >> 4u); // center & convert to tile-space.
	if (action->actor->isKneeled() == true)
		dist = (dist * 3) >> 2u;

	return (distThrow <= dist);
}

/**
 * Helper for validThrowRange().
 * @param weight	- the weight of the object
 * @param strength	- the strength of the thrower
 * @param elevation	- the difference in height between the target and the thrower (voxel-space)
 * @return, the maximum throwing range
 */
int TileEngine::getThrowDistance( // private/static.
		int weight,
		int strength,
		int elevation)
{
	double
		z (static_cast<double>(elevation) + 0.5),
		dZ (1.);

	int ret (0);
	while (ret < 4000) // jic.
	{
		ret += 8;

		if (dZ < -1.)
			z -= 8.;
		else
			z += dZ * 8.;

		if (z < 0. && dZ < 0.) // roll back
		{
			dZ = std::max(dZ, -1.);
			if (std::abs(dZ) > 1e-10) // rollback horizontal
				ret -= static_cast<int>(z / dZ);

			break;
		}

		dZ -= static_cast<double>(weight * 50 / strength) / 100.;
		if (dZ <= -2.) // become falling
			break;
	}
	return ret;
}

/**
 * Validates the melee-range between two BattleUnits.
 * @note Wrapper for validMeleeRange().
 * @param actor			- pointer to acting unit
 * @param dir			- direction of action (default -1)
 * @param targetUnit	- pointer to targetUnit (default nullptr)
 * @return, true if within one tile
 */
bool TileEngine::validMeleeRange(
		const BattleUnit* const actor,
		int dir,
		const BattleUnit* const targetUnit) const
{
	if (dir == -1)
		dir = actor->getUnitDirection();

	return validMeleeRange(
						actor->getPosition(),
						dir,
						actor,
						targetUnit);
}

/**
 * Validates the melee-range between a Position and a BattleUnit.
 * @param pos			- reference to the position of action
 * @param dir			- direction to check
 * @param actor			- pointer to acting unit
 * @param targetUnit	- pointer to targetUnit (default nullptr)
 * @return, true if within one tile
 */
bool TileEngine::validMeleeRange(
		const Position& pos,
		const int dir,
		const BattleUnit* const actor,
		const BattleUnit* const targetUnit) const
{
	if (dir < 0 || dir > 7)
		return false;

	const Tile
		* tileOrigin,
		* tileTarget;
	Position
		posOrigin,
		posTarget,
		posVector,
		voxelOrigin,
		voxelTarget; // not used.

	Pathfinding::directionToVector(dir, &posVector);

	const int armorSize (actor->getArmor()->getSize());
	for (int
			x = 0;
			x != armorSize;
			++x)
	{
		for (int
				y = 0;
				y != armorSize;
				++y)
		{
			posOrigin = pos + Position(x,y,0);
			posTarget = posOrigin + posVector;

			tileOrigin = _battleSave->getTile(posOrigin);
			tileTarget = _battleSave->getTile(posTarget);

			if (tileOrigin != nullptr && tileTarget != nullptr)
			{
				if (tileTarget->getTileUnit() == nullptr)
					tileTarget = getVerticalTile(posOrigin, posTarget);

				if (tileTarget != nullptr
					&& tileTarget->getTileUnit() != nullptr
					&& (targetUnit == nullptr
						|| targetUnit == tileTarget->getTileUnit()))
				{
					voxelOrigin = Position::toVoxelSpaceCentered( // note this is not center of large unit, rather the center of each quadrant.
															posOrigin,
															actor->getHeight(true) + EYE_OFFSET
																- tileOrigin->getTerrainLevel());
					if (canTargetUnit(
								&voxelOrigin,
								tileTarget,
								&voxelTarget,
								actor) == true)
					{
						return true;
					}
				}
			}
		}
	}

	return false;
}

/**
 * Gets an adjacent Position that can be attacked with melee.
 * @param actor - pointer to a BattleUnit
 * @return, position in direction that unit faces
 */
Position TileEngine::getMeleePosition(const BattleUnit* const actor) const
{
	const Tile* tileOrigin;
	Tile* tileTarget;
	const Position pos (actor->getPosition());
	Position
		posOrigin,
		posTarget,
		posVector,
		voxelOrigin,
		voxelTarget; // not used.

	const int
		unitSize (actor->getArmor()->getSize()),
		dir (actor->getUnitDirection());

	Pathfinding::directionToVector(dir, &posVector);

	for (int
			x = 0;
			x != unitSize;
			++x)
	{
		for (int
				y = 0;
				y != unitSize;
				++y)
		{
			posOrigin = pos + Position(x,y,0);
			posTarget = posOrigin + posVector;

			tileOrigin = _battleSave->getTile(posOrigin);
			tileTarget = _battleSave->getTile(posTarget);

			if (tileOrigin != nullptr && tileTarget != nullptr)
			{
				if (tileTarget->getTileUnit() == nullptr
					|| tileTarget->getTileUnit() == actor)
				{
					tileTarget = getVerticalTile(posOrigin, posTarget);
				}

				if (tileTarget != nullptr
					&& tileTarget->getTileUnit() != nullptr
					&& tileTarget->getTileUnit() != actor)
				{
					voxelOrigin = Position::toVoxelSpaceCentered( // note this is not center of large unit, rather the center of each quadrant.
															posOrigin,
															actor->getHeight(true) + EYE_OFFSET
																- tileOrigin->getTerrainLevel());
					if (canTargetUnit(
									&voxelOrigin,
									tileTarget,
									&voxelTarget,
									actor) == true)
					{
						return tileTarget->getPosition();
//						return posTarget;	// TODO: conform this to the fact Reapers can melee vs. 2 tiles
					}						// or three tiles if their direction is diagonal.
				}
			}
		}
	}

	return Position(-1,-1,-1); // this should simply never happen because the call is made after validMeleeRange()
}

/**
 * Gets an adjacent tile with an unconscious unit if any.
 * @param actor - pointer to a BattleUnit
 * @return, pointer to a Tile
 */
Tile* TileEngine::getExecutionTile(const BattleUnit* const actor) const
{
	const Tile* tileOrigin;
	Tile* tileTarget;
	const Position pos (actor->getPosition());
	Position
		posOrigin,
		posTarget,
		posVector,
		voxelOrigin,
		voxelTarget; // not used.

	const int
		unitSize (actor->getArmor()->getSize()),
		dir (actor->getUnitDirection());

	Pathfinding::directionToVector(dir, &posVector);

	for (int
			x = 0;
			x != unitSize;
			++x)
	{
		for (int
				y = 0;
				y != unitSize;
				++y)
		{
			posOrigin = pos + Position(x,y,0);
			posTarget = posOrigin + posVector;

			tileOrigin = _battleSave->getTile(posOrigin);
			tileTarget = _battleSave->getTile(posTarget);

			if (tileOrigin != nullptr && tileTarget != nullptr)
			{
				if (tileTarget->hasUnconsciousUnit(false) == 0)
					tileTarget = getVerticalTile(posOrigin, posTarget);

				if (tileTarget != nullptr
					&& tileTarget->hasUnconsciousUnit(false) != 0)
				{
					voxelOrigin = Position::toVoxelSpaceCentered( // note this is not center of large unit, rather the center of each quadrant.
															posOrigin,
															actor->getHeight(true) + EYE_OFFSET
																- tileOrigin->getTerrainLevel());
					if (canTargetTilepart(
										&voxelOrigin,
										tileOrigin,
										O_FLOOR,
										&voxelTarget,
										actor) == true)
					{
						return tileTarget;
					}
				}
			}
		}
	}

	return nullptr;
}

/**
 * Gets a Tile within melee range.
 * @param posOrigin - reference a position origin
 * @param posTarget - reference a position target
 * @return, pointer to a tile within melee range
 */
Tile* TileEngine::getVerticalTile( // private.
		const Position& posOrigin,
		const Position& posTarget) const
{
	Tile
		* tileOrigin (_battleSave->getTile(posOrigin)),

		* tileTargetAbove (_battleSave->getTile(posTarget + Position(0,0, 1))),
		* tileTargetBelow (_battleSave->getTile(posTarget + Position(0,0,-1)));

	if (tileTargetAbove != nullptr
		&& std::abs(tileTargetAbove->getTerrainLevel() - (tileOrigin->getTerrainLevel() + 24)) < 9)
	{
		return tileTargetAbove;
	}

	if (tileTargetBelow != nullptr
		&& std::abs((tileTargetBelow->getTerrainLevel() + 24) + tileOrigin->getTerrainLevel()) < 9)
	{
		return tileTargetBelow;
	}

	return nullptr;
}

/**
 * Calculates 'ground' z-value beneath a particular voxel - used for casting
 * projectile shadow.
 * @param originVoxel - reference to the voxel-position to trace down
 * @return, z-position of 'ground'
 */
int TileEngine::castShadow(const Position& originVoxel) const
{
	int ret (originVoxel.z);

	Position posTile (Position::toTileSpace(originVoxel));
	const Tile* tile (_battleSave->getTile(posTile));
	while (tile != nullptr
		&& tile->isVoid(false, false) == true
		&& tile->getTileUnit() == nullptr)
	{
		ret = (posTile.z + 1) * 24;

		--posTile.z;
		tile = _battleSave->getTile(posTile);
	}

	Position targetVoxel (originVoxel);
	for (
			;
			ret != 0;
			--ret)
	{
		targetVoxel.z = ret;
		if (voxelCheck(targetVoxel) != VOXEL_EMPTY)
			break;
	}
	return ret;
}

/**
 * Traces voxel visibility.
 * @param voxel - reference to the voxel-position
 * @return, true if visible
 *
bool TileEngine::isVoxelVisible(const Position& voxel) const
{
	const int start_z = voxel.z + 3; // slight Z adjust
	if (start_z / 24 != voxel.z / 24)
		return true; // visible!

	Position testVoxel = voxel;

	const int end_z = (start_z / 24) * 24 + 24;
	for (int // only OBJECT can cause additional occlusion (because of any shape)
			z = start_z;
			z < end_z;
			++z)
	{
		testVoxel.z = z;
		if (voxelCheck(testVoxel, nullptr) == VOXEL_OBJECT)
			return false;

		++testVoxel.x;
		if (voxelCheck(testVoxel, nullptr) == VOXEL_OBJECT)
			return false;

		++testVoxel.y;
		if (voxelCheck(testVoxel, nullptr) == VOXEL_OBJECT)
			return false;
	}

	return true;
} */

/**
 * Checks for a voxel-type in voxel-space.
 * @param targetVoxel		- reference to the Position to check in voxel-space
 * @param excludeUnit		- pointer to unit NOT to do checks for (default nullptr)
 * @param excludeAllUnits	- true to NOT do checks on any unit (default false)
 * @param onlyVisible		- true to consider only visible units (default false)
 * @param excludeAllBut		- pointer to an only unit to be considered (default nullptr)
 * @return, VoxelType (MapData.h)
 *			 -1 hit nothing
 *			0-3 tile-part (floor / westwall / northwall / object)
 *			  4 unit
 *			  5 out-of-map
 * VOXEL_EMPTY			// -1
 * VOXEL_FLOOR			//  0
 * VOXEL_WESTWALL		//  1
 * VOXEL_NORTHWALL		//  2
 * VOXEL_OBJECT			//  3
 * VOXEL_UNIT			//  4
 * VOXEL_OUTOFBOUNDS	//  5
 */
VoxelType TileEngine::voxelCheck(
		const Position& targetVoxel,
		const BattleUnit* const excludeUnit,
		const bool excludeAllUnits,
		const bool onlyVisible,
		const BattleUnit* const excludeAllBut) const
{
//	if (_battleSave->preBattle() == true)
//		excludeAllUnits = true;	// don't start unit spotting before pre-game inventory stuff since
								// large units on the craftInventory-tile will cause a crash if they're "spotted"
								// but I wouldn't be surprised if this is simply more code-bloat
								// after the changes made herein to calcFov.
								// TODO: Trace that because this function gets called 50-bazillion times a second.
								// And if any voxel-checks *are* being done during pre-battle, stop it back there.

	//if (_debug) Log(LOG_INFO) << "TileEngine::voxelCheck() targetVoxel " << targetVoxel;
	const Tile
		* const tile (_battleSave->getTile(Position::toTileSpace(targetVoxel))),
		* tileBelow;
	//Log(LOG_INFO) << ". tile " << tile->getPosition();
	// check if we are out of the map <- we. It's a voxel-check, not a 'we'.
	if (tile == nullptr)
//		|| targetVoxel.x < 0
//		|| targetVoxel.y < 0
//		|| targetVoxel.z < 0)
	{
		//Log(LOG_INFO) << ". vC() ret VOXEL_OUTOFBOUNDS " << Position::toTileSpace(targetVoxel) << " " << targetVoxel;
		return VOXEL_OUTOFBOUNDS;
	}

	if (tile->isVoid(false, false) == true
		&& tile->getTileUnit() == nullptr // TODO: tie this into the boolean-input parameters
		&& ((tileBelow = _battleSave->getTile(tile->getPosition() + Position(0,0,-1))) == nullptr
			|| tileBelow->getTileUnit() == nullptr))
	{
		//if (_debug) Log(LOG_INFO) << ". vC() ret VOXEL_EMPTY";
		return VOXEL_EMPTY;
	}

	if (targetVoxel.z % 24 < 2 // NOTE: This should allow items to be thrown through a gravLift down to the floor below.
		&& tile->getMapData(O_FLOOR) != nullptr
		&& tile->getMapData(O_FLOOR)->isGravLift() == true)
	{
		//Log(LOG_INFO) << "voxelCheck() isGravLift";
		//Log(LOG_INFO) << ". level = " << tile->getPosition().z;
		if (tile->getPosition().z == 0
			|| ((tileBelow = _battleSave->getTile(tile->getPosition() + Position(0,0,-1))) != nullptr
				&& tileBelow->getMapData(O_FLOOR) != nullptr
				&& tileBelow->getMapData(O_FLOOR)->isGravLift() == false))
		{
			//Log(LOG_INFO) << ". vC() ret VOXEL_FLOOR";
			return VOXEL_FLOOR;
		}
	}

	// first check tile-part voxel-data
	MapDataType partType;
	const MapData* partData;
	size_t
		loftId,
		layer ((static_cast<size_t>(targetVoxel.z) % 24) >> 1u),
		x (15u - static_cast<size_t>(targetVoxel.x) % 16);		// x-axis is reversed for tileParts, standard for battleUnit.
	const size_t y (static_cast<size_t>(targetVoxel.y) % 16);	// y-axis is standard

	for (size_t
			i = 0u; // terrain parts [0=floor, 1/2=walls, 3=content-object]
			i != Tile::PARTS_TILE;
			++i)
	{
		if (tile->isSlideDoorOpen(partType = static_cast<MapDataType>(i)) == false
			&& (partData = tile->getMapData(partType)) != nullptr
			&& (loftId = (partData->getLoftId(layer) << 4u) + y) < _voxelData->size() // davide, http://openxcom.org/forum/index.php?topic=2934.msg32146#msg32146 (x2 _below)
			&& (_voxelData->at(loftId) & (1 << x))) // if the voxelData at loftId is "1" solid:
		{
//			if (_debug)
//			{
//				Log(LOG_INFO) << "vC() ret " << i;
//				Log(LOG_INFO) << "vC() targetTile " << Position::toTileSpace(targetVoxel);
//				Log(LOG_INFO) << "vC() targetVoxel.x " << targetVoxel.x % 16;
//				Log(LOG_INFO) << "vC() targetVoxel.y " << targetVoxel.y % 16;
//				Log(LOG_INFO) << "vC() targetVoxel.z " << targetVoxel.z % 24;
//			}
			return static_cast<VoxelType>(partType); // NOTE: MapDataType & VoxelType correspond.
		}
	}

	// second check UNIT voxel-data
	if (excludeAllUnits == false)
	{
		const BattleUnit* targetUnit (tile->getTileUnit());

		if (targetUnit == nullptr
			&& tile->hasNoFloor() == true
			&& (tileBelow = _battleSave->getTile(tile->getPosition() + Position(0,0,-1))) != nullptr)
		{
			targetUnit = tileBelow->getTileUnit();
		}

		if (targetUnit != nullptr && targetUnit != excludeUnit
			&& (excludeAllBut == nullptr || targetUnit == excludeAllBut)
			&& (onlyVisible == false || targetUnit->getUnitVisible() == true))
		{
			const Position posUnit (targetUnit->getPosition());
			const int target_z (posUnit.z * 24 // get foot-level voxel
							  + targetUnit->getFloatHeight()
							  - tile->getTerrainLevel());

			if (targetVoxel.z > target_z
				&& targetVoxel.z <= target_z + targetUnit->getHeight()) // if hit is between foot- and hair-level voxel-layers (z-axis)
			{
				switch (targetUnit->getArmor()->getSize())
				{
					case 2: // for large units...
					{
						const Position posTile (tile->getPosition());
						layer = static_cast<size_t>(posTile.x - posUnit.x + ((posTile.y - posUnit.y) << 1u));
						//Log(LOG_INFO) << ". vC, large unit, LoFT entry = " << layer;
						break;
					}

					case 1: layer = 0u;
				}

//				if (layer > -1)
//				{
				x = static_cast<size_t>(targetVoxel.x % 16);
				// That should be (8,8,10) as per BattlescapeGame::handleNonTargetAction() if (_tacAction.type == BA_MELEE)

				//Log(LOG_INFO) << "loftId = " << loftId << " vD-size = " << (int)_voxelData->size();
				if ((loftId = (targetUnit->getLoft(layer) << 4u) + y) < _voxelData->size() // davide, http://openxcom.org/forum/index.php?topic=2934.msg32146#msg32146 (x2 ^above)
					&& (_voxelData->at(loftId) & (1 << x))) // if the voxelData at loftId is "1" solid:
				{
					//Log(LOG_INFO) << ". vC() ret VOXEL_UNIT";
					return VOXEL_UNIT;
				}
//				}
//				else Log(LOG_INFO) << "ERROR TileEngine::voxelCheck() LoFT entry = " << layer;
			}
		}
	}

	//if (_debug) Log(LOG_INFO) << ". vC() EXIT - ret VOXEL_EMPTY";
	return VOXEL_EMPTY;
}

/**
 * Performs a psionic BattleAction.
 * @param action - pointer to a BattleAction (BattlescapeGame.h)
 * @return, true if attack succeeds
 */
bool TileEngine::psiAttack(BattleAction* const action)
{
	static const bool debug (false);
	if (debug) {
		Log(LOG_INFO) << "";
		Log(LOG_INFO) << "TileEngine::psiAttack() id-" << action->actor->getId();
	}

	const Tile* const tile (_battleSave->getTile(action->posTarget));
	if (tile != nullptr)
	{
		if (debug) Log(LOG_INFO) << ". vs " << action->posTarget;
		BattleUnit* const victim (tile->getTileUnit());
		if (victim != nullptr)
		{
			if (debug) Log(LOG_INFO) << ". . vs ID " << victim->getId();
			if (victim->psiBlock() == false)
			{
				if (action->type == BA_PSICOURAGE)
				{
					if (debug) Log(LOG_INFO) << ". . . . BA_PSICONTROL";
					const int morale (10 + RNG::generate(0,20) + ((action->actor->getBattleStats()->psiSkill + 9) / 10)); // round up.
					action->value = morale;
					victim->moraleChange(morale);
					return true;
				}

				if (action->actor->getOriginalFaction() == FACTION_PLAYER) // NOTE: No exp for Psicourage.
					action->actor->addPsiSkillExp();
				else if (victim->getOriginalFaction() == FACTION_PLAYER) //&& Options::allowPsiStrengthImprovement
					victim->addPsiStrengthExp();

				if (action->type == BA_PSICONTROL
					&& victim->getOriginalFaction() == FACTION_HOSTILE // aLiens should be reduced to 50- Morale before MC.
					&& RNG::percent(victim->getMorale() - 50) == true)
				{
					if (debug) Log(LOG_INFO) << ". . . . RESIST vs " << (victim->getMorale() - 50);
					_battleSave->getBattleState()->warning("STR_PSI_RESIST");
					return false;
				}

				const UnitStats
					* const statsActor (action->actor->getBattleStats()),
					* const statsVictim (victim->getBattleStats());

				int
					psiStrength,
					psiSkill (0);

				if (victim->getFaction() == FACTION_HOSTILE
					&& victim->isMindControlled() == true)
				{
					victim->hostileMcValues(psiStrength, psiSkill);
				}
				else
				{
					psiStrength = statsVictim->psiStrength;
					psiSkill = statsVictim->psiSkill;
				}

				const float
					defense (static_cast<float>(psiStrength) + (static_cast<float>(psiSkill) / 5.f)),
					dist (static_cast<float>(distance(
													action->actor->getPosition(),
													action->posTarget)));

				int bonusSkill; // add to psiSkill when using aLien to Panic another aLien/civie ....
				if (action->actor->getFaction() == FACTION_PLAYER
					&& action->actor->isMindControlled() == true)
				{
					bonusSkill = 21; // ... arbitrary
				}
				else
					bonusSkill = 0;

				float attack (static_cast<float>(statsActor->psiStrength * (statsActor->psiSkill + bonusSkill)) / 50.f);

				if (debug) {
					Log(LOG_INFO) << ". . . defense=\t"	<< (int)defense;
					Log(LOG_INFO) << ". . . attack=\t"	<< (int)attack;
					Log(LOG_INFO) << ". . . dist=\t\t"	<< (int)dist;
				}

				attack -= dist * 2.f;
				attack -= defense;
				switch (action->type)
				{
					case BA_PSICONFUSE:	attack += 60.f; break;
					case BA_PSIPANIC:	attack += 45.f; break;
					case BA_PSICONTROL:	attack += 15.f;
				}
				attack *= 100.f;
				attack /= 56.f;

				const int success (static_cast<int>(attack));
				action->value = success;

				if (debug) Log(LOG_INFO) << ". . . success=\t" << success;
				if (RNG::percent(success) == true)
				{
					if (debug) Log(LOG_INFO) << ". . . . SUCCESS";
					if (action->actor->getOriginalFaction() == FACTION_PLAYER)
					{
						int xp;
						switch (action->type)
						{
							default:
							case BA_PSIPANIC: xp = 1;
								break;

							case BA_PSICONTROL:
								switch (victim->getOriginalFaction())
								{
									default:
									case FACTION_HOSTILE:	xp = 2; break;
									case FACTION_NEUTRAL:	xp = 1; break;
									case FACTION_PLAYER:	xp = 0;
								}
								break;

							case BA_PSICONFUSE: xp = 0;
						}
						action->actor->addPsiSkillExp(xp);
					}

					if (debug) Log(LOG_INFO) << ". . . . victim morale[0]= " << victim->getMorale();
					switch (action->type)
					{
						default:
						case BA_PSIPANIC:
						{
							if (debug) Log(LOG_INFO) << ". . . . . BA_PSIPANIC";
							int morale (100);
							switch (action->actor->getOriginalFaction())
							{
								case FACTION_HOSTILE:
									morale += statsActor->psiStrength * 7 / 20;		// +35% effect for non-Player actors.
									break;

								case FACTION_PLAYER:
									morale += (statsActor->psiStrength * 13) / 20;	// +65% effect for Player's actors.
							}
							morale -= (statsVictim->bravery * 3) >> 1u;				// -150% of the victim's Bravery.
							if (morale > 0)
								victim->moraleChange(-morale);

							if (debug) {
								Log(LOG_INFO) << ". . . . . morale reduction= " << morale;
								Log(LOG_INFO) << ". . . . . victim morale[1]= " << victim->getMorale();
							}
							break;
						}

						case BA_PSICONTROL:
						{
							if (debug) Log(LOG_INFO) << ". . . . . BA_PSICONTROL";
							int morale (statsVictim->bravery);
							switch (action->actor->getFaction())
							{
								default:
								case FACTION_HOSTILE:
									{
										morale = std::min(0,
														 (_battleSave->getMoraleModifier() / 10) + (morale >> 1u) - 110);

										int // store a representation of the aLien's psyche in its victim.
											str (statsActor->psiStrength),
											skl (statsActor->psiSkill);
										victim->hostileMcValues(str, skl);
									}
									break;

								case FACTION_PLAYER:
									switch (victim->getOriginalFaction())
									{
										default:
										case FACTION_HOSTILE: // aLien Morale loss for getting Mc'd by Player.
											morale = std::min(0,
															 (_battleSave->getMoraleModifier(nullptr, false) / 10) + ((morale * 3) >> 2u) - 110);
											break;

										case FACTION_NEUTRAL: // Morale change for civies (-10) unless already Mc'd by aLiens.
											if (victim->isMindControlled() == false)
											{
												morale = -10;
												break;
											} // no break;
										case FACTION_PLAYER: // xCom and civies' Morale gain for getting Mc'd back to xCom.
										{
											morale >>= 1u;
											victim->setExposed(-1);	// bonus Exposure removal.
										}
									}
							}
							victim->moraleChange(morale);
							if (debug) Log(LOG_INFO) << ". . . . . victim morale[2]= " << victim->getMorale();

							if (victim->getAIState() != nullptr)
							{
								switch (victim->getOriginalFaction())
								{
									case FACTION_PLAYER:  victim->setAIState(); break;
									case FACTION_HOSTILE:
									case FACTION_NEUTRAL: victim->getAIState()->resetAI();
								}
							}

							victim->setFaction(action->actor->getFaction());
							victim->setUnitVisible(victim->getFaction() == FACTION_PLAYER);
							victim->setUnitStatus(STATUS_STANDING);
							victim->prepTuEnergy();
							victim->setReselect();

							calculateUnitLighting();
							if (victim->getFaction() == FACTION_PLAYER)
								calcFovTiles(victim);
							calcFovUnits_pos(action->posTarget, true);
							break;
						}

						case BA_PSICONFUSE:
						{
							const int tuLoss ((statsActor->psiSkill + 2) / 3);
							if (debug) {
								Log(LOG_INFO) << ". . . . . BA_PSICONFUSE";
								Log(LOG_INFO) << ". . . . . tuLoss= " << tuLoss;
							}
							victim->setTu(victim->getTu() - tuLoss);
						}
					}
					if (debug) Log(LOG_INFO) << "TileEngine::psiAttack() ret TRUE";
					return true;
				}

				std::string info; // psi Fail. ->
				switch (action->type)
				{
					default:
					case BA_PSIPANIC:
						info = "STR_PANIC_";
						break;

					case BA_PSICONFUSE:
						info = "STR_CONFUSE_";
						break;

					case BA_PSICONTROL:
						info = "STR_CONTROL_";
				}
				_battleSave->getBattleState()->warning(info, success);

				if (victim->getOriginalFaction() == FACTION_PLAYER)
				{
					switch (action->actor->getFaction())
					{
						case FACTION_HOSTILE:
							victim->addPsiStrengthExp(2); // xCom resisted an aLien
							break;

						case FACTION_PLAYER:
							victim->addPsiStrengthExp(1); // xCom resisted an xCom attempt
					}
				}
			}
			else if (action->actor->getFaction() == FACTION_PLAYER)
				_battleSave->getBattleState()->warning(BattlescapeGame::PLAYER_ERROR[11]); // psi-immune
		}
	}

	if (debug) Log(LOG_INFO) << "TileEngine::psiAttack() ret FALSE";
	return false;
}

/**
 * Applies gravity to a tile - causes items and units to drop.
 * @param tile - pointer to a Tile to check
 * @return, pointer to the Tile where stuff ends up
 */
Tile* TileEngine::applyGravity(Tile* const tile) const
{
	if (tile == nullptr)
		return nullptr;

	const Position pos (tile->getPosition());
	if (pos.z == 0)
		return tile;


	const bool nulItems (tile->getInventory()->empty());
	BattleUnit* const unit (tile->getTileUnit());

	if (unit == nullptr && nulItems == true)
		return tile;


	Tile
		* deltaTile (tile),
		* deltaTileBelow (nullptr);
	Position posBelow (pos);

	if (unit != nullptr)
	{
		const int unitSize (unit->getArmor()->getSize());
		while (posBelow.z > 0)
		{
			bool canFall (true);
			for (int
					y = 0;
					y != unitSize && canFall == true;
					++y)
			{
				for (int
						x = 0;
						x != unitSize && canFall == true;
						++x)
				{
					deltaTile = _battleSave->getTile(Position(
															posBelow.x + x,
															posBelow.y + y,
															posBelow.z));
					deltaTileBelow = _battleSave->getTile(Position(
															posBelow.x + x,
															posBelow.y + y,
															posBelow.z - 1));
					if (deltaTile->hasNoFloor(deltaTileBelow) == false)	// note: polar water has no floor, so units that die on them ... uh, sink.
						canFall = false;								// ... before I changed the loop condition to > 0, that is
				}
			}

			if (canFall == false)
				break;

			--posBelow.z;
		}

		if (posBelow != pos)
		{
			if (unit->isOut_t(OUT_STAT) == true)
			{
				for (int
						y = unitSize - 1;
						y != -1;
						--y)
				{
					for (int
							x = unitSize - 1;
							x != -1;
							--x)
					{
						_battleSave->getTile(pos + Position(x,y,0))->setTileUnit();
					}
				}
				unit->setPosition(posBelow);
			}
			else
			{
				switch (unit->getMoveTypeUnit())
				{
					case MT_FLY:
						if (unit->isKneeled() == true)
							unit->flagCache();

						unit->startWalking(							// move to the position you're already in.
										unit->getUnitDirection(),	// this will unset the kneeling flag, set the floating flag, etc.
										unit->getPosition(),
										_battleSave->getTile(unit->getPosition() + Position(0,0,-1)));
						unit->setUnitStatus(STATUS_STANDING);		// and set Status_Standing rather than _Walking or _Flying to avoid weirdness.
						break;

					case MT_WALK:
					case MT_SLIDE:
						unit->startWalking(
										Pathfinding::DIR_DOWN,
										unit->getPosition() + Position(0,0,-1),
										_battleSave->getTile(unit->getPosition() + Position(0,0,-1)));
						//Log(LOG_INFO) << "TileEngine::applyGravity(), addFallingUnit() ID " << unit->getId();
						_battleSave->addFallingUnit(unit);
				}
			}
		}
	}

	deltaTile = tile;
	posBelow = pos;

	while (posBelow.z > 0)
	{
		deltaTile = _battleSave->getTile(posBelow);
		deltaTileBelow = _battleSave->getTile(Position(
													posBelow.x,
													posBelow.y,
													posBelow.z - 1));

		if (deltaTile->hasNoFloor(deltaTileBelow) == false)
			break;

		--posBelow.z;
	}

	if (posBelow != pos)
	{
		deltaTile = _battleSave->getTile(posBelow);
		if (nulItems == false)
		{
			for (std::vector<BattleItem*>::const_iterator
					i = tile->getInventory()->begin();
					i != tile->getInventory()->end();
					++i)
			{
				if ((*i)->getItemUnit() != nullptr
					&& tile->getPosition() == (*i)->getItemUnit()->getPosition())
				{
					(*i)->getItemUnit()->setPosition(deltaTile->getPosition());
				}
				deltaTile->addItem(*i);
			}
			tile->getInventory()->clear();
		}
	}
	return deltaTile;
}

/**
 * Gets the AI to look through a window.
 * @param pos - reference to the current Position
 * @return, direction or -1 if no window found
 */
int TileEngine::faceWindow(const Position& pos) const
{
	const Tile* tile;
	if ((tile = _battleSave->getTile(pos)) != nullptr)
	{
		if (tile->getMapData(O_NORTHWALL) != nullptr
			&& tile->getMapData(O_NORTHWALL)->stopLOS() == false)
		{
			if (pos.y != 0)
				return 0;
			else
				return -1; // do not look into the Void.
		}

		if (tile->getMapData(O_WESTWALL) != nullptr
			&& tile->getMapData(O_WESTWALL)->stopLOS() == false)
		{
			if (pos.x != 0)
				return 6;
			else
				return -1;
		}
	}

	if ((tile = _battleSave->getTile(pos + Position(1,0,0))) != nullptr
		&& tile->getMapData(O_WESTWALL) != nullptr
		&& tile->getMapData(O_WESTWALL)->stopLOS() == false)
	{
		if (pos.x != _battleSave->getMapSizeX() - 1)
			return 2;
		else
			return -1;
	}

	if ((tile = _battleSave->getTile(pos + Position(0,1,0))) != nullptr
		&& tile->getMapData(O_NORTHWALL) != nullptr
		&& tile->getMapData(O_NORTHWALL)->stopLOS() == false)
	{
		if (pos.y != _battleSave->getMapSizeY() - 1)
			return 4;
//		else
//			return -1;
	}

	return -1;
}

/**
 * Marks a region of the map as "dangerous to aliens" for a turn.
 * @param pos		- reference to the epicenter of the explosion in tile-space
 * @param radius	- how far to spread out
 * @param unit		- pointer to the BattleUnit that triggered this battle-action
 */
void TileEngine::setDangerZone(
		const Position& pos,
		const int radius,
		const BattleUnit* const unit) const
{
	Tile* tile (_battleSave->getTile(pos));
	if (tile != nullptr)
	{
		tile->setDangerous(); // set the epicenter as dangerous

		const Position originVoxel (Position::toVoxelSpaceCentered(
																pos,
																12 - tile->getTerrainLevel())); // what.
		Position
			targetVoxel,
			posTest;

		for (int
				x = -radius;
				x != radius;
				++x)
		{
			for (int
					y = -radius;
					y != radius;
					++y)
			{
				if (x != 0 || y != 0) // skip the epicenter
				{
					if ((x * x) + (y * y) <= radius * radius) // make sure it's within the radius
					{
						posTest = pos + Position(x,y,0);
						tile = _battleSave->getTile(posTest);
						if (tile != nullptr)
						{
							targetVoxel = Position::toVoxelSpaceCentered(
																	posTest,
																	12 - tile->getTerrainLevel()); // what.
							std::vector<Position> trj;
							// trace a line here ignoring all units to check if the
							// explosion will reach this point; granted this won't
							// properly account for explosions tearing through walls,
							// but then you can't really know that kind of
							// information before the fact so let the AI assume that
							// the wall (or tree) is enough of a shield.
							if (plotLine(
										originVoxel,
										targetVoxel,
										false,
										&trj,
										unit,
										true,
										false,
										unit) == VOXEL_EMPTY)
							{
								if (trj.size() != 0u
									&& Position::toTileSpace(trj.back()) == posTest)
								{
									tile->setDangerous();
								}
							}
						}
					}
				}
			}
		}
	}
}

/**
 * Calculates the distance between 2 points rounded to nearest integer.
 * @param pos1 - reference to the first Position
 * @param pos2 - reference to the second Position
// * @param considerZ	- true to consider the z coordinate (default true)
 * @return, distance
 */
int TileEngine::distance( // static.
		const Position& pos1,
		const Position& pos2)
{
	const int
		x (pos1.x - pos2.x),
		y (pos1.y - pos2.y),
		z (pos1.z - pos2.z);

	return static_cast<int>(Round(
		   std::sqrt(static_cast<double>(x * x + y * y + z * z))));
}
/* int TileEngine::distance( // static.
		const Position& pos1,
		const Position& pos2,
		const bool considerZ)
{
	const int
		x (pos1.x - pos2.x),
		y (pos1.y - pos2.y);

	int z;
	if (considerZ == true)
		z = pos1.z - pos2.z;
	else
		z = 0;

	return static_cast<int>(Round(
		   std::sqrt(static_cast<double>(x * x + y * y + z * z))));
} */

/**
 * Calculates the distance squared between 2 points.
 * @note No square-root and no floating-point math and sometimes it's all that's
 * really needed.
 * @param pos1		- to reference to the first Position
 * @param pos2		- to reference to the second Position
// * @param considerZ	- true to consider the z coordinate (default true)
 * @return, distance
 */
int TileEngine::distSqr( // static.
		const Position& pos1,
		const Position& pos2)
{
	const int
		x (pos1.x - pos2.x),
		y (pos1.y - pos2.y),
		z (pos1.z - pos2.z);

	return x * x + y * y + z * z;
}
/* int TileEngine::distSqr( // static.
		const Position& pos1,
		const Position& pos2,
		const bool considerZ)
{
	const int
		x (pos1.x - pos2.x),
		y (pos1.y - pos2.y);

	int z;
	if (considerZ == true)
		z = pos1.z - pos2.z;
	else
		z = 0;

	return x * x + y * y + z * z;
} */

/**
 * Calculates the distance between 2 points as a floating-point value.
 * @param pos1 - reference to the first Position
 * @param pos2 - reference to the second Position
 * @return, distance
 *
double TileEngine::distancePrecise( // static.
		const Position& pos1,
		const Position& pos2) const
{
	const int
		x = pos1.x - pos2.x,
		y = pos1.y - pos2.y,
		z = pos1.z - pos2.z;

	return std::sqrt(static_cast<double>(x * x + y * y + z * z));
} */

/**
 * Gets the direction from origin to target.
 * @param posOrigin - reference to the origin-point of the action
 * @param posTarget - reference to the target-point of the action
 * @return, direction
 */
int TileEngine::getDirectionTo( // static.
		const Position& posOrigin,
		const Position& posTarget)
{
	if (posOrigin == posTarget) // safety.
		return 0;

	const double
		theta (std::atan2( // radians: + = y > 0; - = y < 0;
						static_cast<double>(posOrigin.y - posTarget.y),
						static_cast<double>(posTarget.x - posOrigin.x))),

		// divide the pie in 4 thetas each at 1/8th before each quarter
		pi_8 (M_PI / 8.),				// a circle divided into 16 sections (rads)-> 22.5 deg
		d (0.),							// a bias toward cardinal directions. (~0.1)
		pie[4u]
		{
			M_PI - pi_8 - d,			// 2.7488935718910690836548129603696	-> 157.5 deg
			M_PI * 3. / 4. - pi_8 + d,	// 1.9634954084936207740391521145497	-> 112.5 deg
			M_PI_2 - pi_8 - d,			// 1.1780972450961724644234912687298	->  67.5 deg
			pi_8 + d					// .39269908169872415480783042290994	->  22.5 deg
		};

	if (theta > pie[0u] || theta < -pie[0u])
		return 6;
	if (theta > pie[1u])
		return 7;
	if (theta > pie[2u])
		return 0;
	if (theta > pie[3u])
		return 1;
	if (theta < -pie[1u])
		return 5;
	if (theta < -pie[2u])
		return 4;
	if (theta < -pie[3u])
		return 3;
	return 2;
}

/**
 * Sets a tile with a diagonal bigwall as the true epicenter of an explosion.
 * @param tile - tile to set (default nullptr)
 */
void TileEngine::setTrueTile(Tile* const tile)
{
	_trueTile = tile;
}

}
