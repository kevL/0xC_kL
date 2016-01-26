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

#include "Pathfinding.h"

//#include <list>

#include "PathfindingOpenSet.h"

#include "../fmath.h"

#include "../Battlescape/BattlescapeGame.h"

//#include "../Engine/Logger.h"
#include "../Engine/Options.h"

#include "../Ruleset/RuleArmor.h"

#include "../Savegame/BattleUnit.h"
#include "../Savegame/SavedBattleGame.h"
#include "../Savegame/Tile.h"


namespace OpenXcom
{

Uint8 // static not const.
	Pathfinding::red	=  3, // defaults ->
	Pathfinding::green	=  4, // overridden by Interfaces.rul
	Pathfinding::yellow	= 10; // when BattlescapeState loads.


/**
 * Sets up a Pathfinding object.
 * @note There is only one Pathfinding object per tactical so calls to this
 * object should generally be preceded w/ setPathingUnit() in order to setup
 * '_unit' and '_mType' etc.
 * @param battleSave - pointer to SavedBattleGame
 */
Pathfinding::Pathfinding(SavedBattleGame* const battleSave)
	:
		_battleSave(battleSave),
		_unit(nullptr),
		_battleAction(nullptr),
		_previewed(false),
		_strafe(false),
		_tuCostTotal(0),
		_ctrl(false),
		_alt(false),
		_zPath(false),
		_mType(MT_WALK),
		_openDoor(0)
{
	//Log(LOG_INFO) << "Create Pathfinding";
	_nodes.reserve(_battleSave->getMapSizeXYZ()); // reserve one node per tactical tile.

	Position pos;
	for (size_t // initialize one node per tile.
			i = 0;
			i != _battleSave->getMapSizeXYZ();
			++i)
	{
		//Log(LOG_INFO) << ". fill NodePosition #" << (int)i;
		_battleSave->tileCoords(
							i,
							&pos.x,
							&pos.y,
							&pos.z);
		_nodes.push_back(PathfindingNode(pos));
	}
	//Log(LOG_INFO) << "Create Pathfinding EXIT";
}

/**
 * Deletes the Pathfinding.
 * @internal This is required to be here because it requires the PathfindingNode
 * class definition.
 */
Pathfinding::~Pathfinding()
{}

/**
 * Gets the Node on a given position on the map.
 * @param pos - reference the Position
 * @return, pointer to PathfindingNode
 */
PathfindingNode* Pathfinding::getNode(const Position& pos) // private.
{
	return &_nodes[static_cast<size_t>(_battleSave->getTileIndex(pos))];
}

/**
 * Calculates the shortest path; tries bresenham then A* algorithms.
 * @note 'missileTarget' is required only when called by AlienBAIState::pathWaypoints().
 * @param unit				- pointer to a BattleUnit
 * @param posStop			- destination Position
 * @param missileTarget		- pointer to a targeted BattleUnit (default nullptr)
 * @param maxTuCost			- maximum time units this path can cost (default 1000)
 * @param strafeRejected	- true if path needs to be recalculated w/out strafe (default false)
 */
void Pathfinding::calculate(
		const BattleUnit* const unit, // -> should not need 'unit' here anymore; done in setPathingUnit() unless FACTION_PLAYER ...
		Position posStop,
		const BattleUnit* const missileTarget,
		int maxTuCost,
		bool strafeRejected)
{
	//Log(LOG_INFO) << "Pathfinding::calculate()";
	_path.clear();
	_tuCostTotal = 0;

	// i'm DONE with these out of bounds errors.
	// kL_note: I really don't care what you're "DONE" with .....
	int armorSize = unit->getArmor()->getSize();
	if (posStop.x < 0
		|| posStop.y < 0
		|| posStop.x > _battleSave->getMapSizeX() - armorSize
		|| posStop.y > _battleSave->getMapSizeY() - armorSize)
	{
		return;
	}

	setInputModifiers();
	setMoveType(); // redundant in some cases ...

	if (missileTarget != nullptr	// pathfinding for missile; not sure how 'missileTarget' affects initialization yet.
		&& maxTuCost == -1)			// TODO: figure how 'missileTarget' and 'maxTuCost' work together or not. -> are they redudant, and if so how redundant.
									// ... Completely redundant, it seems ... in fact it seems like one of those things that was never thoroughly thought
									// through: bresenham always uses its own default of 1000, while aStar never sets its missile=true boolean because it
									// needs -1 passed in ...... plus it does further checks directly against maxTuCost.
	{
		_mType = MT_FLY;
		maxTuCost = 1000;
		strafeRejected = true;
	}

	if (unit->getFaction() != FACTION_PLAYER)
		strafeRejected = true;


	const Tile* tileStop = _battleSave->getTile(posStop);

	if (isBlocked(			// check if destination is blocked.
				tileStop,	// <- note these aren't the actual destTiles yet.
				O_FLOOR,
				missileTarget) == true
		|| isBlocked(
				tileStop,
				O_OBJECT,
				missileTarget) == true)
	{
		return;
	}


	Position posStop2; // for keeping things straight if strafeRejected happens.
	if (strafeRejected == false)
		posStop2 = posStop;

	// The following check avoids causing the unit to walk behind the stairs if
	// the player clicks behind the stairs to make it go up the stairs. It works
	// only if the unit is on one of the 2 tiles on the stairs or on the tile
	// right in front of the stairs.
	// kL_note: I don't want this: (the function below can be removed too).
/*	if (isOnStairs(posStart, posStop))
	{
		posStop.z++;
		tileStop = _battleSave->getTile(posStop);
	} */

	while (tileStop->getTerrainLevel() == -24
		&& posStop.z != _battleSave->getMapSizeZ())
	{
		++posStop.z;
		tileStop = _battleSave->getTile(posStop);
		//Log(LOG_INFO) << ". raising destTile 1 level, posStop = " << posStop;
	}

	// Check if there's a floor else lower the destination. This allows click in
	// the air for non-flyers and they target the ground tile below that tile.
	if (_mType != MT_FLY)
	{
		while (canFallDown(tileStop, armorSize))
		{
			--posStop.z;
			tileStop = _battleSave->getTile(posStop);
			//Log(LOG_INFO) << ". canFallDown() -1 level, posStop = " << posStop;
		}
	}


	if (isBlocked(		// recheck if destination is blocked.
			tileStop,	// <- these are the actual destTiles.
			O_FLOOR,
			missileTarget) == false
		&& isBlocked(
				tileStop,
				O_OBJECT,
				missileTarget) == false)
	{
		if (armorSize != 1)
		{
			--armorSize;

			const int dir[3] = {4,2,3};
			size_t i = 0;

			const Tile* tileTest;
			const BattleUnit* unitTest;

			for (int
					x = 0;
					x <= armorSize;
					x += armorSize)
			{
				for (int
						y = 0;
						y <= armorSize;
						y += armorSize)
				{
					if (x != 0 || y != 0)
					{
						tileTest = _battleSave->getTile(posStop + Position(x,y,0));
						if (x != 0 && y != 0
							&& ((tileTest->getMapData(O_NORTHWALL) != nullptr
									&& tileTest->getMapData(O_NORTHWALL)->isDoor() == true)
								|| (tileTest->getMapData(O_WESTWALL) != nullptr
									&& tileTest->getMapData(O_WESTWALL)->isDoor() == true)))
						{
							return;
						}
						else if (isBlockedPath(
										tileStop,
										dir[i],
										unit) == true
							&& isBlockedPath(
										tileStop,
										dir[i],
										missileTarget) == true)
						{
							return;
						}
						else if (tileTest->getTileUnit() != nullptr)
						{
							unitTest = tileTest->getTileUnit();
							if (unitTest != unit
								&& unitTest != missileTarget
								&& unitTest->getUnitVisible() == true)
							{
								return;
							}
						}

						++i;
					}
				}
			}
		}


		const Position posStart = unit->getPosition();

		const bool isMech = unit->getUnitRules() != nullptr
						 && unit->getUnitRules()->isMechanical();

		_strafe = strafeRejected == false
			   && Options::battleStrafe == true
			   && ((_ctrl == true && isMech == false)
					|| (_alt == true && isMech == true))
			   && (//posStart.z == posStop.z ||
					std::abs(
							(_battleSave->getTile(posStop)->getTerrainLevel() - posStop.z * 24)
						  - (_battleSave->getTile(posStart)->getTerrainLevel() - posStart.z * 24)) < 9)
			   && std::abs(posStop.x - posStart.x) < 2
			   && std::abs(posStop.y - posStart.y) < 2;

		_battleAction->strafe = _strafe;


		const bool sneak = unit->getFaction() == FACTION_HOSTILE
						&& Options::sneakyAI == true;

		if (posStart.z == posStop.z
			&& bresenhamPath(
						posStart,
						posStop,
						missileTarget,
						sneak
						/*maxTuCost*/) == true)
		{
			std::reverse(
					_path.begin(),
					_path.end());
		}
		else
		{
			abortPath();

			if (aStarPath(
						posStart,
						posStop,
						missileTarget,
						sneak,
						maxTuCost) == false)
			{
				abortPath();
			}
		}

		if (_path.empty() == false)
		{
			if (_path.size() > 2
				&& _strafe == true)
			{
				calculate( // iterate this function ONCE ->
						unit,
						posStop2,
						missileTarget,
						maxTuCost,
						true); // <- sets '_strafe' FALSE so loop never gets back in here.
			}
			else if (Options::battleStrafe == true
				&& _ctrl == true
				&& unit->getGeoscapeSoldier() != nullptr
				&& (_strafe == false
					|| (_path.size() == 1
						&& unit->getUnitDirection() == _path.front())))
			{
				_strafe = false;

				if (_battleAction != nullptr) // this is a safety.
				{
					_battleAction->strafe = false;
					_battleAction->dash = true;
					if (_battleAction->actor != nullptr)	// but this sometimes happens via AlienBAIState::setupAmbush() at least
						_battleAction->actor->setDashing();	// if end turn is done w/out a selected unit. (not sure)
				}
			}
			else //if (_strafe == false)
			{
				if (_battleAction != nullptr) // this is a safety.
				{
					_battleAction->dash = false;
					if (_battleAction->actor != nullptr)			// but this sometimes happens via AlienBAIState::setupAmbush() at least.
						_battleAction->actor->setDashing(false);	// if end turn is done w/out a selected unit. (for sure)
				}
			}
		}
	}
}

/**
 * Calculates the shortest path using Brensenham path algorithm.
 * @note This only works in the X/Y plane.
 * @param origin		- reference the Position to start from
 * @param target		- reference the Position to end at
 * @param missileTarget	- pointer to targeted BattleUnit
 * @param sneak			- true if unit is sneaking (default false)
 * @param maxTuCost		- maximum time units the path can cost (default 1000)
 * @return, true if a path is found
 */
bool Pathfinding::bresenhamPath( // private.
		const Position& origin,
		const Position& target,
		const BattleUnit* const missileTarget,
		bool sneak,
		int maxTuCost)
{
	//Log(LOG_INFO) << "Pathfinding::bresenhamPath()";
	static const int
		FAIL = 255,
		DIRZ = 8,
		stock_xd[DIRZ]	= { 0, 1, 1, 1, 0,-1,-1,-1}, // stock values
		stock_yd[DIRZ]	= {-1,-1, 0, 1, 1, 1, 0,-1},
		alt_xd[DIRZ]	= { 0,-1,-1,-1, 0, 1, 1, 1}, // alt values
		alt_yd[DIRZ]	= { 1, 1, 0,-1,-1,-1, 0, 1};

	int
		xd[DIRZ],
		yd[DIRZ];

	if (_zPath == false)
	{
		//std::copy(std::begin(src), std::end(src), std::begin(dest));
		std::copy(stock_xd, stock_xd + 8, xd);
		std::copy(stock_yd, stock_yd + 8, yd);
	}
	else
	{
		std::copy(alt_xd, alt_xd + 8, xd);
		std::copy(alt_yd, alt_yd + 8, yd);
	}

	int
		x,x0,x1, delta_x, step_x,
		y,y0,y1, delta_y, step_y,
		z,z0,z1, delta_z, step_z,

		swap_xy, swap_xz,
		drift_xy, drift_xz,

		cx,cy,cz,

		dir,
		tuCostLast = -1;

	Position posLast (origin);
	Position posNext;

	_tuCostTotal = 0;

	// start and end points
	x0 = origin.x; x1 = target.x;
	y0 = origin.y; y1 = target.y;
	z0 = origin.z; z1 = target.z;

	// 'steep' xy Line, make longest delta x plane
	swap_xy = std::abs(y1 - y0) > std::abs(x1 - x0);
	if (swap_xy)
	{
		std::swap(x0,y0);
		std::swap(x1,y1);
	}

	// do same for xz
	swap_xz = std::abs(z1 - z0) > std::abs(x1 - x0);
	if (swap_xz)
	{
		std::swap(x0,z0);
		std::swap(x1,z1);
	}

	// delta is Length in each plane
	delta_x = std::abs(x1 - x0);
	delta_y = std::abs(y1 - y0);
	delta_z = std::abs(z1 - z0);

	// drift controls when to step in 'shallow' planes
	// starting value keeps Line centred
	drift_xy = (delta_x / 2);
	drift_xz = (delta_x / 2);

	// direction of line
	step_x =
	step_y =
	step_z = 1;
	if (x0 > x1) step_x = -1;
	if (y0 > y1) step_y = -1;
	if (z0 > z1) step_z = -1;

	// starting point
	y = y0;
	z = z0;

	// step through longest delta that was swapped to x
	for (
			x = x0;
			x != (x1 + step_x);
			x += step_x)
	{
		// copy position
		cx = x; cy = y; cz = z;

		// unswap (in reverse)
		if (swap_xz) std::swap(cx,cz);
		if (swap_xy) std::swap(cx,cy);

		if (x != x0 || y != y0 || z != z0)
		{
			Position posNextReal = Position(cx,cy,cz);
			posNext = posNextReal;

			// get direction
			for (
					dir = 0;
					dir != DIRZ;
					++dir)
			{
				if (   xd[dir] == cx - posLast.x
					&& yd[dir] == cy - posLast.y)
				{
					break;
				}
			}

			const int tuCost = getTuCostPf(
										posLast,
										dir,
										&posNext,
										missileTarget,
										missileTarget != nullptr && maxTuCost == 1000);
			//Log(LOG_INFO) << ". TU Cost = " << tuCost;

			if (sneak == true
				&& _battleSave->getTile(posNext)->getTileVisible())
			{
				return false;
			}

			// delete the following
			const bool isDiagonal = (dir & 1);
			const int
				tuCostLastDiagonal = tuCostLast + tuCostLast / 2,
				tuCostDiagonal = tuCost + tuCost / 2;

			if (posNext == posNextReal
				&& tuCost < FAIL
				&& (tuCost == tuCostLast
					|| (isDiagonal == true && tuCost == tuCostLastDiagonal)
					|| (isDiagonal == false && tuCostDiagonal == tuCostLast)
					|| tuCostLast == -1)
				&& isBlockedPath(
							_battleSave->getTile(posLast),
							dir,
							missileTarget) == false)
			{
				_path.push_back(dir);
			}
			else
				return false;

			if (missileTarget == nullptr
				&& tuCost != FAIL)
			{
				tuCostLast = tuCost;
				_tuCostTotal += tuCost;
			}

			posLast = Position(cx,cy,cz);
		}

		// update progress in other planes
		drift_xy = drift_xy - delta_y;
		drift_xz = drift_xz - delta_z;

		// step in y plane
		if (drift_xy < 0)
		{
			y = y + step_y;
			drift_xy = drift_xy + delta_x;
		}

		// same in z
		if (drift_xz < 0)
		{
			z = z + step_z;
			drift_xz = drift_xz + delta_x;
		}
	}

	return true;
}

/**
 * Calculates the shortest path using a simple A-Star algorithm.
 * @note The unit information and movement type must have already been set. The
 * path information is set only if a valid path is found.
 * @param origin		- reference the position to start from
 * @param target		- reference the position to end at
 * @param missileTarget	- pointer to targeted BattleUnit
 * @param sneak			- true if the unit is sneaking (default false)
 * @param maxTuCost		- maximum time units this path can cost (default 1000)
 * @return, true if a path is found
 */
bool Pathfinding::aStarPath( // private.
		const Position& origin,
		const Position& target,
		const BattleUnit* const missileTarget,
		bool sneak,
		int maxTuCost)
{
	//Log(LOG_INFO) << "Pathfinding::aStarPath()";

	for (std::vector<PathfindingNode>::iterator // reset every node, so have to check them all
			i = _nodes.begin();
			i != _nodes.end();
			++i)
	{
		i->reset();
	}

	PathfindingNode
		* const nodeStart = getNode(origin), // start position is the first Node in the OpenSet
		* nodeTest,
		* node,
		* nodeStop;

	nodeStart->linkNode(0, nullptr, 0, target);

	PathfindingOpenSet openList;
	openList.addNode(nodeStart);

	Position posStop;
	int tuCost;

	const bool missile = missileTarget != nullptr
					  && maxTuCost == -1;

	while (openList.isNodeSetEmpty() == false) // if the openList is empty, reached the end
	{
		nodeTest = openList.getNode();
		const Position& posStart = nodeTest->getPosition();
		nodeTest->setChecked();

		if (posStart == target) // found the target.
		{
			_path.clear();

			node = nodeTest;
			while (node->getPrevNode() != nullptr)
			{
				_path.push_back(node->getPrevDir());
				node = node->getPrevNode();
			}

			if (_strafe == true
				&& _unit->getUnitRules() != nullptr
				&& _unit->getUnitRules()->isMechanical() == true)
			{
				const int delta = std::abs((_path.back() + 4) % 8 - _unit->getUnitDirection());
				if (delta > 1 && delta < 7)
				{
					_strafe = false; // illegal direction for tank-strafe.
					_battleAction->strafe = false;

					return false;
				}
			}

			return true;
		}

		for (int // try all reachable neighbours.
				dir = 0;
				dir != 10; // dir 0 thro 7, up/down
				++dir)
		{
			//Log(LOG_INFO) << ". try dir ... " << dir;
			tuCost = getTuCostPf(
							posStart,
							dir,
							&posStop,
							missileTarget,
							missile,
							false);
			//Log(LOG_INFO) << ". TU Cost = " << tuCost;
			if (tuCost >= 255) // Skip unreachable / blocked
				continue;

			if (sneak == true
				&& _battleSave->getTile(posStop)->getTileVisible() == true)
			{
				tuCost *= 2; // avoid being seen
			}

			nodeStop = getNode(posStop);
			if (nodeStop->getChecked() == true) // algorithm means this node is already at minimum cost
			{
				//Log(LOG_INFO) << ". node already Checked ... cont.";
				continue;
			}

			_tuCostTotal = nodeTest->getTuCostNode(missile) + tuCost;

			if ((nodeStop->inOpenSet() == false // if this node is unvisited or has only been visited from inferior paths...
					|| nodeStop->getTuCostNode(missile) > _tuCostTotal)
				&& _tuCostTotal <= maxTuCost)
			{
				//Log(LOG_INFO) << ". nodeChecked(dir) = " << dir << " totalCost = " << _tuCostTotal;
				nodeStop->linkNode(
								_tuCostTotal,
								nodeTest,
								dir,
								target);

				openList.addNode(nodeStop);
			}
		}
	}

	return false; // unable to reach the target
}

/**
 * Gets the TU cost to move from 1 tile to another - ONE STEP ONLY!
 * @note But also updates the destination Position because it is possible that
 * the unit uses stairs or falls while moving.
 * @param posStart		- reference to the start position
 * @param dir			- direction of movement
 * @param posStop		- pointer to destination Position
 * @param missileTarget	- pointer to targeted BattleUnit (default nullptr)
 * @param missile		- true if a guided missile (default false)
 * @param bresenh		- true if calc'd by Breshenham pathing (default true)
 * @return, TU cost or 255 if movement is impossible
 */
int Pathfinding::getTuCostPf(
		const Position& posStart,
		int dir,
		Position* const posStop,
		const BattleUnit* const missileTarget,
		bool missile,
		bool bresenh)
{
	//Log(LOG_INFO) << "Pathfinding::getTuCostPf() " << _unit->getId();
	static const int FAIL = 255;

	directionToVector(
					dir,
					posStop);
	*posStop += posStart;

	bool
		fall = false,
		stairs = false;

	int
		partsGoingUp = 0,
		partsGoingDown = 0,
		partsFalling = 0,
		partsChangingHeight = 0,
//		partsOnAir = 0,

		cost,
		costTotal = 0,

		wallTu,
		wallTotal,
		edges;

	_openDoor = 0;

	Tile
		* tileStart,
//		* tileStartBelow,
		* tileStop,
		* tileStopBelow,
		* tileStopAbove,
		* tileStopTest;

	Position posOffset;

	const int armorSize = _unit->getArmor()->getSize() - 1;
	for (int
			x = 0;
			x <= armorSize;
			++x)
	{
		for (int
				y = 0;
				y <= armorSize;
				++y)
		{
			// first, CHECK FOR BLOCKAGE ->> then calc TU cost down below.
			posOffset = Position(x,y,0);

			tileStart = _battleSave->getTile(posStart + posOffset);
			if (tileStart == nullptr)
				return FAIL;

			tileStop = _battleSave->getTile(*posStop + posOffset);
			if (tileStop == nullptr)
				return FAIL;

/*			if (_mType != MT_FLY)
			{
				tileStartBelow = _battleSave->getTile(posStart + posOffset + Position(0,0,-1));
				if (dir == DIR_DOWN)
				{
					if (tileStart->hasNoFloor(tileStartBelow) == true)
					{
						++partsOnAir;
						if (partsOnAir == (armorSize + 1) * (armorSize + 1))
							fall = true;
					}
				}
				else if (tileStart->hasNoFloor(tileStartBelow) == true)
				{
					++partsOnAir;
					if (partsOnAir == (armorSize + 1) * (armorSize + 1))
						return FAIL; // cannot walk on air
				}
			} */
/*			if (x == 0 && y == 0
				&& _mType != MT_FLY
				&& canFallDown(
							tileStart,
							armorSize + 1)) // kL_note: keep at it!! You'll get that ego you've been striving so hard for. Try Masonry.
			{
				if (direction != DIR_DOWN)
					return FAIL; // cannot walk on air

				fall = true;
			} */ // end Take II ... i still don't trust that crap.

			// don't let tanks phase through doors
			if (x != 0 && y != 0)
			{
				if ((tileStop->getMapData(O_NORTHWALL) != nullptr
						&& tileStop->getMapData(O_NORTHWALL)->isDoor() == true)
					|| (tileStop->getMapData(O_WESTWALL) != nullptr
						&& tileStop->getMapData(O_WESTWALL)->isDoor() == true))
				{
					//if (debug) Log(LOG_INFO) << "door bisects";
					return FAIL;
				}
			}

			// 'terrainLevel' starts at 0 (floor) and goes up to -24 *cuckoo**
			if (dir < DIR_UP
				&& tileStart->getTerrainLevel() > -16) // lower than 2-thirds up.
			{
				if (isBlockedPath(
							tileStart,
							dir,
							missileTarget) == true)
				{
					//if (debug) Log(LOG_INFO) << "too far up[1]";
					return FAIL;
				}

				if (tileStart->getTerrainLevel() - tileStop->getTerrainLevel() > 8) // greater than 1/3 step up.
				{
					//if (debug) Log(LOG_INFO) << "too far up[2]";
					return FAIL;
				}
			}


			// this will later be used to re-init the start Tile
			Position posOffsetVertical (0,0,0); // init.

			tileStopBelow = _battleSave->getTile(*posStop + posOffset + Position(0,0,-1)),
			tileStopAbove = _battleSave->getTile(*posStop + posOffset + Position(0,0, 1));

			// if unit is on stairs try to go up a level
			if (dir < DIR_UP
				&& stairs == false
				&& tileStart->getTerrainLevel() < -15 // higher than 2-thirds up.
				&& tileStopAbove->hasNoFloor(tileStop) == false)
			{
				++partsGoingUp;
				//Log(LOG_INFO) << "partsUp = " << partsGoingUp;

				if (partsGoingUp > armorSize)
				{
					//Log(LOG_INFO) << "partsUp > armorSize";
					stairs = true;

					++posOffsetVertical.z;
					++posStop->z;
					tileStop = _battleSave->getTile(*posStop + posOffset);
//					tileStopBelow = _battleSave->getTile(*posStop + Position(x,y,-1)); // NOT USED.
				}
			}
			else if (dir < DIR_UP
				&& fall == false // for safely walking down ramps or stairs ...
				&& _mType != MT_FLY
				&& canFallDown(tileStop) == true
				&& tileStopBelow != nullptr
				&& tileStopBelow->getTerrainLevel() < -11)	// higher than 1-half up.
			{
				++partsGoingDown;
				//Log(LOG_INFO) << "partsDown = " << partsGoingDown;

				if (partsGoingDown == (armorSize + 1) * (armorSize + 1))
				{
					//Log(LOG_INFO) << "partsDown == armorSize";
					fall = true;

					--posStop->z;
					tileStop = _battleSave->getTile(*posStop + posOffset);
//					tileStopBelow = _battleSave->getTile(*posStop + Position(x,y,-1)); // NOT USED.
				}
			}
			else if (_mType == MT_FLY
				&& tileStopBelow != nullptr
				&& tileStopBelow->getTileUnit() != nullptr
				&& tileStopBelow->getTileUnit() != _unit)
			{
				// 2 or more voxels poking into this tile -> no go
				if (tileStopBelow->getTileUnit()->getHeight(true) - tileStopBelow->getTerrainLevel() > 26)
				{
					//if (debug) Log(LOG_INFO) << "head too large";
					return FAIL;
				}
			}

			// this means the destination is probably outside the map
			if (tileStop == nullptr)
			{
				//if (debug) Log(LOG_INFO) << "dest outside Map";
				return FAIL;
			}

			cost = 0;

			if (dir < DIR_UP)
			{
				if (posStop->z == tileStart->getPosition().z)
				{
					if (isBlockedPath( // check if path can go this way
								tileStart,
								dir,
								missileTarget) == true)
					{
						//Log(LOG_INFO) << "same Z, blocked, dir = " << dir;
						return FAIL;
					}

					if (tileStart->getTerrainLevel() - tileStop->getTerrainLevel() > 8) // greater than 1/3 step up.
					{
						//if (debug) Log(LOG_INFO) << "same Z too high";
						return FAIL;
					}
				}
			}
			else //if (fall == false) // <- this can probly stay <--
			{
				if (validateUpDown(
								posStart + posOffset,
								dir) < 1)
				{
					return FAIL;
				}

				cost = 8; // vertical movement by flying suit or grav lift
			}

			// check if there's a floor else fall down; for walking off roofs and pathing in mid-air ...
			if (fall == false
				&& _mType != MT_FLY
				&& canFallDown(tileStart) == true)
			{
				++partsFalling;

				if (//dir != DIR_DOWN && // <- this can probly stay <--
					partsFalling == (armorSize + 1) * (armorSize + 1))
				{
//					return false; // <- fuck you.
//					return FAIL;

					fall = true;
					*posStop = posStart + Position(0,0,-1);
					tileStop = _battleSave->getTile(*posStop + posOffset);
//					tileStopBelow = _battleSave->getTile(*posStop + Position(x,y,-1)); // NOT USED.
					dir = DIR_DOWN;
				}
			}

			tileStart = _battleSave->getTile(tileStart->getPosition() + posOffsetVertical);

			if (dir < DIR_UP
				&& partsGoingUp != 0)
			{
				if (isBlockedPath(
							tileStart,
							dir,
							missileTarget) == true)
				{
					//if (debug) Log(LOG_INFO) << "partsUp blocked";
					return FAIL;
				}

				if (tileStart->getTerrainLevel() - tileStop->getTerrainLevel() > 8) // greater than 1/3 step up.
				{
					//if (debug) Log(LOG_INFO) << "partsUp blocked, too high";
					return FAIL;
				}
			}

			if (isBlocked(
						tileStop,
						O_FLOOR,
						missileTarget) == true
				|| isBlocked(
							tileStop,
							O_OBJECT,
							missileTarget) == true)
			{
				//if (debug) Log(LOG_INFO) << "just blocked";
				return FAIL;
			}
// CHECK FOR BLOCKAGE_end.


			if (missile == true) // if missile GO.
				return 0;


			// Calculate TU costage ->
			if (dir < DIR_UP)
			{
				// if not fell down and there is no floor can't know the TUs so it defaults to 4
				if (fall == false
					&& tileStop->hasNoFloor() == true)
				{
					cost = 4;
				}
				else
				{
					cost += tileStop->getTuCostTile(O_FLOOR, _mType);

					if (fall == false
						&& stairs == false
						&& tileStop->getMapData(O_OBJECT) != nullptr)
					{
						cost += tileStop->getTuCostTile(O_OBJECT, _mType);

						if (tileStop->getMapData(O_FLOOR) == nullptr)
							cost += 4;
					}
				}

				// climbing up a level costs one extra. Not.
//				if (posOffsetVertical.z > 0) ++cost;

				wallTotal =	// walking over rubble walls
				wallTu =	// differentiates wall-Total from wall-current and keeps the '_openDoor' value straight
				edges = 0;	// quantity of walls crossed if moving diagonally

				if (   dir == 7
					|| dir == 0
					|| dir == 1)
				{
					//Log(LOG_INFO) << ". from " << tileStart->getPosition() << " to " << tileStop->getPosition() << " dir = " << dir;
					wallTu = tileStart->getTuCostTile(O_NORTHWALL, _mType); // ( 'walkover' bigWalls not incl. -- none exists )
					if (wallTu > 0)
					{
//						if (dir & 1) // would use this to increase diagonal wall-crossing by +50%
//							wallTu += wallTu / 2;

						wallTotal += wallTu;
						++edges;

						if (tileStart->getMapData(O_NORTHWALL)->isDoor() == true
							|| tileStart->getMapData(O_NORTHWALL)->isUfoDoor() == true)
						{
							//Log(LOG_INFO) << ". . . _openDoor[N] = TRUE, wallTu = " << wallTu;
							if (wallTu > _openDoor) // don't let large unit parts reset _openDoor prematurely
								_openDoor = wallTu;
						}
					}
				}

				if (   dir == 1 // && !fall
					|| dir == 2
					|| dir == 3)
				{
					//Log(LOG_INFO) << ". from " << tileStart->getPosition() << " to " << tileStop->getPosition() << " dir = " << dir;

					if (tileStart->getPosition().z > tileStop->getPosition().z) // don't count wallCost if it's on the floor below.
						tileStopTest = _battleSave->getTile(tileStop->getPosition() + Position(0,0,1)); // no safety req'd.
					else
						tileStopTest = tileStop;

					wallTu = tileStopTest->getTuCostTile(O_WESTWALL, _mType);
					if (wallTu > 0)
					{
						wallTotal += wallTu;
						++edges;

						if (tileStopTest->getMapData(O_WESTWALL)->isDoor() == true
							|| tileStopTest->getMapData(O_WESTWALL)->isUfoDoor() == true)
						{
							if (wallTu > _openDoor)
								_openDoor = wallTu;
						}
					}
				}

				if (   dir == 3 // && !fall
					|| dir == 4
					|| dir == 5)
				{
					//Log(LOG_INFO) << ". from " << tileStart->getPosition() << " to " << tileStop->getPosition() << " dir = " << dir;

					if (tileStart->getPosition().z > tileStop->getPosition().z) // don't count wallCost if it's on the floor below.
						tileStopTest = _battleSave->getTile(tileStop->getPosition() + Position(0,0,1)); // no safety req'd.
					else
						tileStopTest = tileStop;

					wallTu = tileStopTest->getTuCostTile(O_NORTHWALL, _mType);
					if (wallTu > 0)
					{
						wallTotal += wallTu;
						++edges;

						if (tileStopTest->getMapData(O_NORTHWALL)->isDoor() == true
							|| tileStopTest->getMapData(O_NORTHWALL)->isUfoDoor() == true)
						{
							//Log(LOG_INFO) << ". . . _openDoor[S] = TRUE, wallTu = " << wallTu;
							if (wallTu > _openDoor)
								_openDoor = wallTu;
						}
					}
				}

				if (   dir == 5
					|| dir == 6
					|| dir == 7)
				{
					//Log(LOG_INFO) << ". from " << tileStart->getPosition() << " to " << tileStop->getPosition() << " dir = " << dir;
					wallTu = tileStart->getTuCostTile(O_WESTWALL, _mType); // ( bigWalls not incl. yet )
					if (wallTu > 0)
					{
						wallTotal += wallTu;
						++edges;

						if (tileStart->getMapData(O_WESTWALL)->isDoor() == true
							|| tileStart->getMapData(O_WESTWALL)->isUfoDoor() == true)
						{
							//Log(LOG_INFO) << ". . . _openDoor[W] = TRUE, wallTu = " << wallTu;
							if (wallTu > _openDoor)
								_openDoor = wallTu;
						}
					}
				}

				// diagonal walking (uneven directions) costs 50% more tu's
				// kL_note: this is moved up so that objects don't cost +150% tu;
				// instead, let them keep a flat +100% to step onto
				// -- note that Walls also do not take +150 tu to step over diagonally....
				if (dir & 1)
				{
					cost = static_cast<int>(static_cast<float>(cost) * 1.5f);

					if (wallTotal > 0
						&& _openDoor == 0)
					{
						if (edges > 0)
							wallTotal /= edges; // average of the walls crossed

						if ((wallTotal - edges) % 2 == 1) // round wallCost up.
							wallTotal += 1;
					}
				}

				//Log(LOG_INFO) << ". wallTotal = " << wallTotal;
				//Log(LOG_INFO) << ". cost[0] = " << cost;
				cost += wallTotal;
				//Log(LOG_INFO) << ". cost[1] = " << cost;
			}


			if (tileStop->getFire() != 0)
			{
				// TFTD thing: tiles on fire are cost 2 TU more for whatever reason.
				// kL_note: Let's make it a UFO thing, too.
				cost += 2;
				if (_unit->avoidsFire() == true)
					cost += 32;	// try to find a better path, but don't exclude this path entirely.
								// See UnitWalkBState::doStatusStand(), where this is subtracted again.
			}

			// Propose: if flying then no extra TU cost
			//Log(LOG_INFO) << ". pathSize = " << (int)_path.size();
			if (_strafe == true)
			{
				// kL_begin: extra TU for strafe-moves ->	1 0 1
				//											2 ^ 2
				//											3 2 3
				int delta = std::abs((dir + 4) % 8 - _unit->getUnitDirection());

				if (delta > 1 && delta < 7
					&& _unit->getUnitRules() != nullptr
					&& _unit->getUnitRules()->isMechanical() == true)
				{
					if (bresenh == true)
					{
						_strafe = false; // illegal direction for tank-strafe.
						_battleAction->strafe = false;
					}
				}
				else if (_unit->getUnitDirection() != dir) // if not dashing straight ahead 1 tile.
				{
					delta = std::min(
									std::abs(8 + dir - _unit->getUnitDirection()),
									std::min(
										std::abs(_unit->getUnitDirection() - dir),
										std::abs(8 + _unit->getUnitDirection() - dir)));
					if (delta == 4) delta = 2;

					cost += delta;
				} // kL_end.
			}

			costTotal += cost;
		}
	}

	if (armorSize > 0) // only for Large units ->
	{
		//Log(LOG_INFO) << "getTuCostPf() armorSize > 0 " << (*posStop);
		// - check the path between part 0,0 and part 1,1 at destination position
		const Tile* const ulTile = _battleSave->getTile(*posStop);
		if (isBlockedPath(
					ulTile,
					3,
					missileTarget) == true)
		{
			//Log(LOG_INFO) << "blocked uL,lR " << (*posStop);
			return FAIL;
		}

		// - then check the path between part 1,0 and part 0,1 at destination position
		const Tile* const urTile = _battleSave->getTile(*posStop + Position(1,0,0));
		if (isBlockedPath(
					urTile,
					5,
					missileTarget) == true)
		{
			//Log(LOG_INFO) << "blocked uR,lL " << (*posStop);
			return FAIL;
		}

		if (fall == false)
		{
			const Tile
				* const lrTile = _battleSave->getTile(*posStop + Position(1,1,0)),
				* const llTile = _battleSave->getTile(*posStop + Position(0,1,0));
			const int
				levels[4] =
				{
					ulTile->getTerrainLevel(),
					urTile->getTerrainLevel(),
					llTile->getTerrainLevel(),
					lrTile->getTerrainLevel()
				},

				minLevel = *std::min_element(levels, levels + 4),
				maxLevel = *std::max_element(levels, levels + 4);

			if (std::abs(maxLevel - minLevel) > 8)
			{
				//Log(LOG_INFO) << "blocked by levels " << (*posStop) << " " << std::abs(maxLevel - minLevel);
				return FAIL;
			}
		}

		// If the unit changed level check that there are two parts changing
		// level so a large unit can not go up a small sized stairs.
		if (partsChangingHeight == 1)
		{
			//Log(LOG_INFO) << "blocked - not enough parts changing level";
			return FAIL;
		}

		costTotal = static_cast<int>(Round(std::ceil( // ok, round those tanks up!
					static_cast<double>(costTotal) / static_cast<double>((armorSize + 1) * (armorSize + 1)))));

		//Log(LOG_INFO) << ". large unit costTotal = " << costTotal;
	} // largeUnits_end.

	//Log(LOG_INFO) << "Pathfinding::getTuCostPf() ret = " << costTotal;
	return costTotal;
}

/**
 * Converts direction to a unit-vector.
 * @note Direction starts north = 0 and goes clockwise.
 * @param dir		- source direction
 * @param posVect	- pointer to a Position
 */
void Pathfinding::directionToVector( // static.
		const int dir,
		Position* const posVect)
{
	static const int
		x[10] = { 0, 1, 1, 1, 0,-1,-1,-1, 0, 0},
		y[10] = {-1,-1, 0, 1, 1, 1, 0,-1, 0, 0},
		z[10] = { 0, 0, 0, 0, 0, 0, 0, 0, 1,-1};

	posVect->x = x[dir];
	posVect->y = y[dir];
	posVect->z = z[dir];
}

/**
 * Converts unit-vector to a direction.
 * @note Direction starts north = 0 and goes clockwise.
 * @param posVect	- reference to a Position
 * @param dir		- reference to the resulting direction (up/down & same-tile sets dir -1)
 */
void Pathfinding::vectorToDirection( // static.
		const Position& posVect,
		int& dir)
{
	static const int
		x[8] = { 0, 1, 1, 1, 0,-1,-1,-1},
		y[8] = {-1,-1, 0, 1, 1, 1, 0,-1};

	for (size_t
			i = 0;
			i != 8;
			++i)
	{
		if (x[i] == posVect.x
			&& y[i] == posVect.y)
		{
			dir = static_cast<int>(i);
			return;
		}
	}

	dir = -1;
}

/**
 * Checks whether a path is ready and gives the first direction.
 * @return, direction where the unit needs to go next, -1 if it's the end of the path
 */
int Pathfinding::getStartDirection()
{
	if (_path.empty() == false)
		return _path.back();

	return -1;
}

/**
 * Dequeues the next path direction. Ie, returns the direction and removes it from queue.
 * @return, direction where the unit needs to go next, -1 if it's the end of the path
 */
int Pathfinding::dequeuePath()
{
	if (_path.empty() == true)
		return -1;

	const int last_element = _path.back();
	_path.pop_back();

	return last_element;
}

/**
 * Aborts the current path - clears the path vector.
 */
void Pathfinding::abortPath()
{
	setInputModifiers();

	_tuCostTotal = 0;
	_path.clear();
}

/**
 * Determines whether going from one Tile to another is blocked.
 * @param startTile		- pointer to start tile
 * @param dir			- direction of movement
 * @param missileTarget	- pointer to targeted BattleUnit (default nullptr)
 * @return, true if path is blocked
 */
bool Pathfinding::isBlockedPath( // public
		const Tile* const startTile,
		const int dir,
		const BattleUnit* const missileTarget) const
{
	//Log(LOG_INFO) << "Pathfinding::isBlocked() #1";

	// check if the difference in height between start and destination is not too high
	// so we can not jump to the highest part of the stairs from the floor
	// stairs terrainlevel goes typically -8 -16 (2 steps) or -4 -12 -20 (3 steps)
	// this "maximum jump height" is therefore set to 8

	const Position pos = startTile->getPosition();

	static const Position
		posNorth	= Position( 0,-1, 0),
		posEast		= Position( 1, 0, 0),
		posSouth	= Position( 0, 1, 0),
		posWest		= Position(-1, 0, 0);


	switch (dir)
	{
		case 0:	// north
			//Log(LOG_INFO) << ". try North";
			if (isBlocked(
						startTile,
						O_NORTHWALL,
						missileTarget))
			{
				return true;
			}
			return false;

		case 1: // north-east
			//Log(LOG_INFO) << ". try NorthEast";
			if (isBlocked(
						startTile,
						O_NORTHWALL,
						missileTarget)
				|| isBlocked(
						_battleSave->getTile(pos + posEast),
						O_WESTWALL,
						missileTarget)
				|| isBlocked(
						_battleSave->getTile(pos + posEast),
						O_NORTHWALL,
						missileTarget)
				|| isBlocked(
						_battleSave->getTile(pos + posEast),
						O_OBJECT,
						missileTarget,
						BIGWALL_NESW)
				|| isBlocked(
						_battleSave->getTile(pos + posEast + posNorth),
						O_WESTWALL,
						missileTarget)
				|| isBlocked(
						_battleSave->getTile(pos + posNorth),
						O_OBJECT,
						missileTarget,
						BIGWALL_NESW))
			{
				return true;
			}
			return false;

		case 2: // east
			//Log(LOG_INFO) << ". try East";
			if (isBlocked(
						_battleSave->getTile(pos + posEast),
						O_WESTWALL,
						missileTarget))
			{
				return true;
			}
			return false;

		case 3: // south-east
			//Log(LOG_INFO) << ". try SouthEast";
			if (isBlocked(
						_battleSave->getTile(pos + posEast),
						O_WESTWALL,
						missileTarget)
				|| isBlocked(
						_battleSave->getTile(pos + posEast),
						O_OBJECT,
						missileTarget,
						BIGWALL_NWSE)
				|| isBlocked(
						_battleSave->getTile(pos + posEast + posSouth),
						O_NORTHWALL,
						missileTarget)
				|| isBlocked(
						_battleSave->getTile(pos + posEast + posSouth),
						O_WESTWALL,
						missileTarget)
				|| isBlocked(
						_battleSave->getTile(pos + posSouth),
						O_NORTHWALL,
						missileTarget)
				|| isBlocked(
						_battleSave->getTile(pos + posSouth),
						O_OBJECT,
						missileTarget,
						BIGWALL_NWSE))
			{
				return true;
			}
			return false;

		case 4: // south
			//Log(LOG_INFO) << ". try South";
			if (isBlocked(
						_battleSave->getTile(pos + posSouth),
						O_NORTHWALL,
						missileTarget))
			{
				return true;
			}
			return false;

		case 5: // south-west
			//Log(LOG_INFO) << ". try SouthWest";
			if (isBlocked(
						startTile,
						O_WESTWALL,
						missileTarget)
				|| isBlocked(
						_battleSave->getTile(pos + posSouth),
						O_WESTWALL,
						missileTarget)
				|| isBlocked(
						_battleSave->getTile(pos + posSouth),
						O_NORTHWALL,
						missileTarget)
				|| isBlocked(
						_battleSave->getTile(pos + posSouth),
						O_OBJECT,
						missileTarget,
						BIGWALL_NESW)
				|| isBlocked(
						_battleSave->getTile(pos + posSouth + posWest),
						O_NORTHWALL,
						missileTarget)
				|| isBlocked(
						_battleSave->getTile(pos + posWest),
						O_OBJECT,
						missileTarget,
						BIGWALL_NESW))
			{
				return true;
			}
			return false;

		case 6: // west
			//Log(LOG_INFO) << ". try West";
			if (isBlocked(
						startTile,
						O_WESTWALL,
						missileTarget))
			{
				return true;
			}
			return false;

		case 7: // north-west
			//Log(LOG_INFO) << ". try NorthWest";
			if (isBlocked(
						startTile,
						O_WESTWALL,
						missileTarget)
				|| isBlocked(
						startTile,
						O_NORTHWALL,
						missileTarget)
				|| isBlocked(
						_battleSave->getTile(pos + posWest),
						O_NORTHWALL,
						missileTarget)
				|| isBlocked(
						_battleSave->getTile(pos + posWest),
						O_OBJECT,
						missileTarget,
						BIGWALL_NWSE)
				|| isBlocked(
						_battleSave->getTile(pos + posNorth),
						O_WESTWALL,
						missileTarget)
				|| isBlocked(
						_battleSave->getTile(pos + posNorth),
						O_OBJECT,
						missileTarget,
						BIGWALL_NWSE))
			{
				return true;
			}
	}

	return false;
}

/**
 * Determines whether a certain part of a tile blocks movement.
 * @param tile			- pointer to a specified Tile, can be nullptr
 * @param partType		- part of the tile (MapData.h)
 * @param missileTarget	- pointer to targeted BattleUnit (default nullptr)
 * @param diagExclusion	- to exclude diagonal bigWalls (default BIGWALL_NONE) (Pathfinding.h)
 * @return, true if path is blocked
 */
bool Pathfinding::isBlocked( // private.
		const Tile* const tile,
		const MapDataType partType,
		const BattleUnit* const missileTarget,
		const BigwallType diagExclusion) const
{
	//Log(LOG_INFO) << "Pathfinding::isBlocked() #2";
	if (tile == nullptr) return true;

//	if (tile->getMapData(O_OBJECT) != nullptr
//		&& tile->getMapData(O_OBJECT)->getBigwall() == BIGWALL_BLOCK)
//	{
//		return true;
//	}

	switch (partType)
	{
		case O_OBJECT:
			//Log(LOG_INFO) << ". part is Bigwall/object";
			if (tile->getMapData(O_OBJECT) != nullptr
				&& tile->getMapData(O_OBJECT)->getBigwall() != diagExclusion)
			{
				switch (tile->getMapData(O_OBJECT)->getBigwall())
				{
					case BIGWALL_BLOCK:
					case BIGWALL_NESW:
					case BIGWALL_NWSE:
						return true;
				}
			}
			return false;

		case O_WESTWALL:
			{
				//Log(LOG_INFO) << ". part is Westwall";
				if (missileTarget != nullptr					// missiles can't pathfind through closed doors.
					&& tile->getMapData(O_WESTWALL) != nullptr	// ... neither can proxy mines.
					&& (tile->getMapData(O_WESTWALL)->isDoor() == true
						|| (tile->getMapData(O_WESTWALL)->isUfoDoor() == true
							&& tile->isUfoDoorOpen(O_WESTWALL) == false)))
				{
					return true;
				}

				if (tile->getMapData(O_OBJECT) != nullptr)
				{
					switch (tile->getMapData(O_OBJECT)->getBigwall())
					{
						case BIGWALL_WEST:
						case BIGWALL_W_N:
							return true;
					}
				}

				const Tile* const tileWest = _battleSave->getTile(tile->getPosition() + Position(-1,0,0));
				if (tileWest == nullptr)
					return true;

				if (tileWest->getMapData(O_OBJECT) != nullptr)
				{
					switch (tileWest->getMapData(O_OBJECT)->getBigwall())
					{
						case BIGWALL_EAST:
						case BIGWALL_E_S:
							return true;
					}
				}
			}
			break;

		case O_NORTHWALL:
			{
				//Log(LOG_INFO) << ". part is Northwall";
				if (missileTarget != nullptr					// missiles can't pathfind through closed doors.
					&& tile->getMapData(O_NORTHWALL) != nullptr	// ... neither can proxy mines.
					&& (tile->getMapData(O_NORTHWALL)->isDoor() == true
						|| (tile->getMapData(O_NORTHWALL)->isUfoDoor() == true
							&& tile->isUfoDoorOpen(O_NORTHWALL) == false)))
				{
					return true;
				}

				if (tile->getMapData(O_OBJECT) != nullptr)
				{
					switch (tile->getMapData(O_OBJECT)->getBigwall())
					{
						case BIGWALL_NORTH:
						case BIGWALL_W_N:
							return true;
					}
				}

				const Tile* const tileNorth = _battleSave->getTile(tile->getPosition() + Position(0,-1,0));
				if (tileNorth == nullptr)
					return true;

				if (tileNorth->getMapData(O_OBJECT) != nullptr)
				{
					switch (tileNorth->getMapData(O_OBJECT)->getBigwall())
					{
						case BIGWALL_SOUTH:
						case BIGWALL_E_S:
							return true;
					}
				}
			}
			break;

		case O_FLOOR:
			{
				//Log(LOG_INFO) << ". part is Floor";
				const BattleUnit* targetUnit = tile->getTileUnit();

				if (targetUnit != nullptr)
				{
					if (targetUnit == _unit
						|| targetUnit == missileTarget
						|| targetUnit->isOut_t(OUT_STAT) == true)
					{
						return false;
					}

					if (_unit != nullptr)
					{
						if (_unit->getFaction() == targetUnit->getFaction())
							return true;

						switch (_unit->getFaction())
						{
							case FACTION_PLAYER:
								if (targetUnit->getUnitVisible() == true)
									return true;
								break;

							case FACTION_HOSTILE:
								if (std::find(
									_unit->getHostileUnitsThisTurn().begin(),
									_unit->getHostileUnitsThisTurn().end(),
									targetUnit) != _unit->getHostileUnitsThisTurn().end())
								{
									return true;
								}
						}
					}
				}
				else if (tile->hasNoFloor() == true	// This section is devoted to ensuring that large units
					&& _mType != MT_FLY)			// do not take part in any kind of falling behaviour.
				{
					Position pos = tile->getPosition();
					while (pos.z > -1)
					{
						const Tile* const testTile = _battleSave->getTile(pos);
						targetUnit = testTile->getTileUnit();

						if (targetUnit != nullptr
							&& targetUnit != _unit)
						{
							if (_unit != nullptr // don't let large units fall on other units
								&& _unit->getArmor()->getSize() > 1)
							{
								return true;
							}

							if (targetUnit != missileTarget // don't let any units fall on large units
								&& targetUnit->isOut_t(OUT_STAT) == false
								&& targetUnit->getArmor()->getSize() > 1)
							{
								return true;
							}
						}

						if (testTile->hasNoFloor() == false)
							break;

						--pos.z;
					}
				}
			}
	}

	if (tile->getTuCostTile(partType, _mType) == 255)
	{
		//Log(LOG_INFO) << "isBlocked() EXIT true, partType = " << partType << " MT = " << (int)_mType;
		return true;
	}

	//Log(LOG_INFO) << "isBlocked() EXIT false, partType = " << partType << " MT = " << (int)_mType;
	return false;
}

/**
 * Determines whether a unit can fall down from this tile.
 * True if the current position is higher than 0 (the tileBelow does not exist)
 * or if the tile has no floor (and there is no unit standing below).
 * @param tile - the current tile
 * @return, true if a unit on @a tile can fall to a lower level
 */
bool Pathfinding::canFallDown(const Tile* const tile) const // private
{
	if (tile->getPosition().z == 0) // already on lowest maplevel
		return false;

	return tile->hasNoFloor(_battleSave->getTile(tile->getPosition() + Position(0,0,-1)));
}

/**
 * Wrapper for canFallDown() above.
 * @param tile		- pointer to the current Tile
 * @param armorSize	- the size of the unit
 * @return, true if a unit on @a tile can fall to a lower level
 */
bool Pathfinding::canFallDown( // private
		const Tile* const tile,
		int armorSize) const
{
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
			if (canFallDown(_battleSave->getTile(tile->getPosition() + Position(x,y,0))) == false)
				return false;
		}
	}

	return true;
}

/**
 * Checks if vertical movement is valid.
 * @note There is a grav lift or the unit can fly and there are no obstructions.
 * @param posStart	- start position
 * @param dir		- up or down
 * @return,	-1 can't fly
//			-1 kneeling (stop unless on gravLift)
			 0 blocked (stop)
			 1 gravLift (go)
			 2 flying (go unless blocked)
 */
int Pathfinding::validateUpDown(
		const Position& posStart,
		const int dir)
{
	Position posStop;
	directionToVector(
					dir,
					&posStop);
	posStop += posStart;

	const Tile
		* const startTile = _battleSave->getTile(posStart),
		* const destTile = _battleSave->getTile(posStop);

	if (destTile == nullptr)
		return 0;

	const bool gravLift = startTile->getMapData(O_FLOOR) != nullptr
					   && startTile->getMapData(O_FLOOR)->isGravLift()
					   && destTile->getMapData(O_FLOOR) != nullptr
					   && destTile->getMapData(O_FLOOR)->isGravLift();

	if (gravLift == true)
		return 1;

	if (_mType == MT_FLY //_unit->getMoveTypeUnit() == MT_FLY
		|| (dir == DIR_DOWN
			&& _alt == true))
	{
		if ((dir == DIR_UP
				&& destTile->hasNoFloor(startTile))
			|| (dir == DIR_DOWN
				&& startTile->hasNoFloor(_battleSave->getTile(posStart + Position(0,0,-1)))))
		{
			return 2; // flying.
		}
		else
			return 0; // blocked.
	}

	return -1; // no flying suit.
}

/**
 * Marks tiles for the path preview.
 * @param discard - true removes preview (default false)
 * @return, true if the preview is created or discarded; false if nothing happens
 */
bool Pathfinding::previewPath(bool discard)
{
	if (_path.empty() == true
		|| (discard == false && _previewed == true))
	{
		return false;
	}
	_previewed = !discard;


	Tile* tile;
	if (discard == true)
	{
		for (size_t
				i = 0;
				i != _battleSave->getMapSizeXYZ();
				++i)
		{
			tile = _battleSave->getTiles()[i];
			tile->setPreviewDir(-1);
			tile->setPreviewTu(-1);
			tile->setPreviewColor(0);
		}
	}
	else
	{
		const int
			armorSize = _unit->getArmor()->getSize() - 1,
			agility = _unit->getArmor()->getAgility();
		int
			unitTu = _unit->getTimeUnits(),
			unitEnergy = _unit->getEnergy(),
			tileTu,			// cost per tile
			tuSpent = 0,	// only for soldiers reserving TUs
			energyLimit,
			dir;
		bool
			hathStood = false,
			gravLift,
			reserveOk,
			falling;
		Uint8 color;

		Position
			start = _unit->getPosition(),
			dest;

/*		bool switchBack;
		if (_battleSave->getBattleGame()->getReservedAction() == BA_NONE)
		{
			switchBack = true;
			_battleSave->getBattleGame()->setReservedAction(BA_SNAPSHOT);
		}
		else
			switchBack = false; */

		for (std::vector<int>::reverse_iterator
				i = _path.rbegin();
				i != _path.rend();
				++i)
		{
			dir = *i;
			tileTu = getTuCostPf( // gets tu cost but also gets the destination position.
							start,
							dir,
							&dest);

			energyLimit = unitEnergy;

			falling = _mType != MT_FLY
				   && canFallDown(
							_battleSave->getTile(start),
							armorSize + 1);
			if (falling == false)
			{
				gravLift = dir >= DIR_UP
						&& _battleSave->getTile(start)->getMapData(O_FLOOR) != nullptr
						&& _battleSave->getTile(start)->getMapData(O_FLOOR)->isGravLift() == true;
				if (gravLift == false)
				{
					if (hathStood == false
						&& _unit->isKneeled() == true)
					{
						hathStood = true;

						tuSpent += 10;
						unitTu -= 10; // 10 tu + 3 energy to stand up.
						unitEnergy -= 3;
					}

					if (_battleAction->dash == true)
					{
						tileTu -= _openDoor;
						unitEnergy -= tileTu * 3 / 2;

						tileTu = (tileTu * 3 / 4) + _openDoor;
					}
					else
						unitEnergy += _openDoor - tileTu;

					unitEnergy += agility;

					if (unitEnergy > energyLimit)
						unitEnergy = energyLimit;
				}

				unitTu -= tileTu;
			}

			tuSpent += tileTu;
			reserveOk = _battleSave->getBattleGame()->checkReservedTu(
																_unit,
																tuSpent);
//																true);
			start = dest;

			for (int
					x = armorSize;
					x != -1;
					--x)
			{
				for (int
						y = armorSize;
						y != -1;
						--y)
				{
					tile = _battleSave->getTile(start + Position(x,y,0));
					Tile* const tileAbove = _battleSave->getTile(start + Position(x,y,1));

					if (i == _path.rend() - 1)
						tile->setPreviewDir(10);
					else
						tile->setPreviewDir(*(i + 1)); // next dir

					if ((x && y)
						|| armorSize == 0)
					{
						tile->setPreviewTu(unitTu);
					}

					if (tileAbove != nullptr
						&& tileAbove->getPreviewDir() == 0
						&& tileTu == 0
						&& _mType != MT_FLY)				// unit fell down
					{
						tileAbove->setPreviewDir(DIR_DOWN);	// retroactively set tileAbove's direction
					}

					if (unitTu > -1
						&& unitEnergy > -1)
					{
						if (reserveOk == true)
							color = Pathfinding::green;
						else
							color = Pathfinding::yellow;
					}
					else
						color = Pathfinding::red;

					tile->setPreviewColor(color);
				}
			}
		}

//		if (switchBack == true)
//			_battleSave->getBattleGame()->setReservedAction(BA_NONE);
	}

	return true;
}

/**
 * Unmarks the tiles used for path preview.
 * @return, true if preview gets removed
 */
bool Pathfinding::removePreview()
{
	if (_previewed == false)
		return false;

	previewPath(true);

	return true;
}

/**
 * Gets the path preview setting.
 * @return, true if path is previewed
 */
bool Pathfinding::isPathPreviewed() const
{
	return _previewed;
}

/**
 * Locates all tiles reachable to @a unit with a TU cost no more than @a tuMax.
 * @note Uses Dijkstra's algorithm.
 * @param unit	- pointer to a BattleUnit
 * @param tuMax	- the maximum cost of the path to each tile
 * @return, vector of reachable tile indices sorted in ascending order of cost;
 * the first tile is the start location itself
 */
std::vector<int> Pathfinding::findReachable(
		const BattleUnit* const unit,
		int tuMax)
{
	for (std::vector<PathfindingNode>::iterator
			i = _nodes.begin();
			i != _nodes.end();
			++i)
	{
		i->reset();
	}

//	setPathingUnit(unit); // <- done in BattlescapeGame::handleUnitAI() as well as the AI states themselves.

	PathfindingNode
		* const startNode = getNode(unit->getPosition()),
		* currentNode,
		* nextNode;

	startNode->linkNode(0,nullptr,0);

	PathfindingOpenSet unvisited;
	unvisited.addNode(startNode);

	std::vector<PathfindingNode*> reachable;	// note these are not route-nodes perse;
												// *every Tile* is a PathfindingNode.
	Position nextPos;
	int
		tuCost,
		totalTuCost,
		staMax = unit->getEnergy();

	while (unvisited.isNodeSetEmpty() == false) // 'unvisited' -> 'openList'
	{
		currentNode = unvisited.getNode();
		const Position& currentPos = currentNode->getPosition();

		for (int // Try all reachable neighbours.
				dir = 0;
				dir != 10;
				++dir)
		{
			tuCost = getTuCostPf(
							currentPos,
							dir,
							&nextPos);

			if (tuCost != 255) // Skip unreachable / blocked
			{
				totalTuCost = currentNode->getTuCostNode(false) + tuCost;
				if (totalTuCost <= tuMax
					&& totalTuCost <= staMax)
				{
					nextNode = getNode(nextPos);
					if (nextNode->getChecked() == false) // the algorithm means this node is already at minimum cost.
					{
						if (nextNode->inOpenSet() == false
							|| nextNode->getTuCostNode(false) > totalTuCost) // if this node is unvisited or visited from a better path.
						{
							nextNode->linkNode(
											totalTuCost,
											currentNode,
											dir);

							unvisited.addNode(nextNode);
						}
					}
				}
			}
		}

		currentNode->setChecked();
		reachable.push_back(currentNode);
	}

	std::sort(
			reachable.begin(),
			reachable.end(),
			MinNodeCosts());

	std::vector<int> tileIndices;
	tileIndices.reserve(reachable.size());

	for (std::vector<PathfindingNode*>::const_iterator
			i = reachable.begin();
			i != reachable.end();
			++i)
	{
		tileIndices.push_back(_battleSave->getTileIndex((*i)->getPosition()));
	}

	return tileIndices;
}

/**
 * Sets unit etc in order to exploit pathing functions.
 * @note This has evolved into an initialization routine.
 * @param unit - pointer to a BattleUnit
 */
void Pathfinding::setPathingUnit(BattleUnit* const unit)
{
	_unit = unit;

	if (_battleSave->getBattleState() == nullptr) // safety for battlescape generation.
		_battleAction = nullptr;
	else
		_battleAction = _battleSave->getBattleGame()->getCurrentAction();

	if (unit != nullptr)
		setMoveType();
	else
		_mType = MT_WALK;
}

/**
 * Sets the movement type for the path.
 */
void Pathfinding::setMoveType() // private.
{
	_mType = _unit->getMoveTypeUnit();

	if (_alt == true // this forces soldiers in flyingsuits to walk on (or fall to) the ground.
		&& _mType == MT_FLY
		&& _unit->getGeoscapeSoldier() != nullptr)
//			|| _unit->getUnitRules()->isMechanical() == false)	// hovertanks & cyberdiscs always hover.
//		&& _unit->getRaceString() != "STR_FLOATER"				// floaters always float
//		&& _unit->getRaceString() != "STR_CELATID"				// celatids always .. float.
//		&& _unit->getRaceString() != "STR_ETHEREAL")			// Ethereals *can* walk, but they don't like to.
	{															// Should turn this into Ruleset param: 'alwaysFloat'
		_mType = MT_WALK;										// or use floatHeight > 0 or something-like-that
	}
}

/**
 * Sets keyboard input modifiers.
 */
void Pathfinding::setInputModifiers()
{
	if (_battleSave->getSide() != FACTION_PLAYER
		|| _battleSave->getBattleGame()->playerPanicHandled() == false)
	{
		_ctrl =
		_alt =
		_zPath = false;
	}
	else
	{
		_ctrl = (SDL_GetModState() & KMOD_CTRL) != 0;
		_alt = (SDL_GetModState() & KMOD_ALT) != 0;

		const Uint8* const keystate = SDL_GetKeyState(nullptr);
		if (keystate[SDLK_z] != 0)
			_zPath = true;
		else
			_zPath = false;
	}
}

/**
 * Checks whether a modifier key was used to enable strafing or running.
 * @return, true if modifier was used
 */
bool Pathfinding::isModCtrl() const
{
	return _ctrl;
}

/**
 * Checks whether a modifier key was used to enable forced walking.
 * @return, true if modifier was used
 */
bool Pathfinding::isModAlt() const
{
	return _alt;
}

/**
 * Gets the zPath modifier setting.
 * @return, true if modifier was used
 */
bool Pathfinding::isZPath() const
{
	return _zPath;
}

/**
 * Gets the current movementType.
 * @return, the currently pathing unit's movementType (MapData.h)
 */
MovementType Pathfinding::getMoveTypePf() const
{
	return _mType;
}

/**
 * Gets TU cost for opening a door.
 * @note Used to conform TU costs in UnitWalkBState.
 * @return, TU cost for opening a specific door
 */
int Pathfinding::getOpenDoor() const
{
	return _openDoor;
}

/**
 * Gets a reference to the current path.
 * @return, reference to a vector of directions
 */
const std::vector<int>& Pathfinding::getPath()
{
	return _path;
}

/**
 * Makes a copy of the current path.
 * @return, copy of the path
 */
std::vector<int> Pathfinding::copyPath() const
{
	return _path;
}

/*
 * Checks if going one step from start to destination in the given direction
 * requires going through a closed UFO door.
 * @param direction The direction of travel.
 * @param start The starting position of the travel.
 * @param destination Where the travel ends.
 * @return The TU cost of opening the door. 0 if no UFO door opened.
 *
int Pathfinding::getOpeningUfoDoorCost(int direction, Position start, Position destination) // private.
{
	Tile* s = _battleSave->getTile(start);
	Tile* d = _battleSave->getTile(destination);

	switch (direction)
	{
		case 0:
			if (s->getMapData(O_NORTHWALL)
				&& s->getMapData(O_NORTHWALL)->isUfoDoor()
				&& !s->isUfoDoorOpen(O_NORTHWALL))
			return s->getMapData(O_NORTHWALL)->getTuCost(_mType);
		break;
		case 1:
			if (s->getMapData(O_NORTHWALL)
				&& s->getMapData(O_NORTHWALL)->isUfoDoor()
				&& !s->isUfoDoorOpen(O_NORTHWALL))
			return s->getMapData(O_NORTHWALL)->getTuCost(_mType);
			if (d->getMapData(O_WESTWALL)
				&& d->getMapData(O_WESTWALL)->isUfoDoor()
				&& !d->isUfoDoorOpen(O_WESTWALL))
			return d->getMapData(O_WESTWALL)->getTuCost(_mType);
		break;
		case 2:
			if (d->getMapData(O_WESTWALL)
				&& d->getMapData(O_WESTWALL)->isUfoDoor()
				&& !d->isUfoDoorOpen(O_WESTWALL))
			return d->getMapData(O_WESTWALL)->getTuCost(_mType);
		break;
		case 3:
			if (d->getMapData(O_NORTHWALL)
				&& d->getMapData(O_NORTHWALL)->isUfoDoor()
				&& !d->isUfoDoorOpen(O_NORTHWALL))
			return d->getMapData(O_NORTHWALL)->getTuCost(_mType);
			if (d->getMapData(O_WESTWALL)
				&& d->getMapData(O_WESTWALL)->isUfoDoor()
				&& !d->isUfoDoorOpen(O_WESTWALL))
			return d->getMapData(O_WESTWALL)->getTuCost(_mType);
		break;
		case 4:
			if (d->getMapData(O_NORTHWALL)
				&& d->getMapData(O_NORTHWALL)->isUfoDoor()
				&& !d->isUfoDoorOpen(O_NORTHWALL))
			return d->getMapData(O_NORTHWALL)->getTuCost(_mType);
		break;
		case 5:
			if (d->getMapData(O_NORTHWALL)
				&& d->getMapData(O_NORTHWALL)->isUfoDoor()
				&& !d->isUfoDoorOpen(O_NORTHWALL))
			return d->getMapData(O_NORTHWALL)->getTuCost(_mType);
			if (s->getMapData(O_WESTWALL)
				&& s->getMapData(O_WESTWALL)->isUfoDoor()
				&& !s->isUfoDoorOpen(O_WESTWALL))
			return s->getMapData(O_WESTWALL)->getTuCost(_mType);
		break;
		case 6:
			if (s->getMapData(O_WESTWALL)
				&& s->getMapData(O_WESTWALL)->isUfoDoor()
				&& !s->isUfoDoorOpen(O_WESTWALL))
			return s->getMapData(O_WESTWALL)->getTuCost(_mType);
		break;
		case 7:
			if (s->getMapData(O_NORTHWALL)
				&& s->getMapData(O_NORTHWALL)->isUfoDoor()
				&& !s->isUfoDoorOpen(O_NORTHWALL))
			return s->getMapData(O_NORTHWALL)->getTuCost(_mType);
			if (s->getMapData(O_WESTWALL)
				&& s->getMapData(O_WESTWALL)->isUfoDoor()
				&& !s->isUfoDoorOpen(O_WESTWALL))
			return s->getMapData(O_WESTWALL)->getTuCost(_mType);
		break;

		default:
			return 0;
	}

	return 0;
} */

/*
 * Determines whether the unit is going up a stairs.
 * @param startPosition The position to start from.
 * @param endPosition The position we wanna reach.
 * @return True if the unit is going up a stairs.
 *
bool Pathfinding::isOnStairs(const Position& startPosition, const Position& endPosition)
{
	// condition 1 : endposition has to the south a terrainlevel -16 object (upper part of the stairs)
	if (_battleSave->getTile(endPosition + Position(0, 1, 0))
		&& _battleSave->getTile(endPosition + Position(0, 1, 0))->getTerrainLevel() == -16)
	{
		// condition 2 : one position further to the south there has to be a terrainlevel -8 object (lower part of the stairs)
		if (_battleSave->getTile(endPosition + Position(0, 2, 0))
			&& _battleSave->getTile(endPosition + Position(0, 2, 0))->getTerrainLevel() != -8)
		{
			return false;
		}

		// condition 3 : the start position has to be on either of the 3 tiles to the south of the endposition
		if (startPosition == endPosition + Position(0, 1, 0)
			|| startPosition == endPosition + Position(0, 2, 0)
			|| startPosition == endPosition + Position(0, 3, 0))
		{
			return true;
		}
	}

	// same for the east-west oriented stairs.
	if (_battleSave->getTile(endPosition + Position(1, 0, 0))
		&& _battleSave->getTile(endPosition + Position(1, 0, 0))->getTerrainLevel() == -16)
	{
		if (_battleSave->getTile(endPosition + Position(2, 0, 0))
			&& _battleSave->getTile(endPosition + Position(2, 0, 0))->getTerrainLevel() != -8)
		{
			return false;
		}

		if (startPosition == endPosition + Position(1, 0, 0)
			|| startPosition == endPosition + Position(2, 0, 0)
			|| startPosition == endPosition + Position(3, 0, 0))
		{
			return true;
		}
	}

	//TFTD stairs 1 : endposition has to the south a terrainlevel -18 object (upper part of the stairs)
	if (_battleSave->getTile(endPosition + Position(0, 1, 0)) && _battleSave->getTile(endPosition + Position(0, 1, 0))->getTerrainLevel() == -18)
	{
		// condition 2 : one position further to the south there has to be a terrainlevel -8 object (lower part of the stairs)
		if (_battleSave->getTile(endPosition + Position(0, 2, 0)) && _battleSave->getTile(endPosition + Position(0, 2, 0))->getTerrainLevel() != -12)
		{
			return false;
		}

		// condition 3 : the start position has to be on either of the 3 tiles to the south of the endposition
		if (startPosition == endPosition + Position(0, 1, 0) || startPosition == endPosition + Position(0, 2, 0) || startPosition == endPosition + Position(0, 3, 0))
		{
			return true;
		}
	}

	// same for the east-west oriented stairs.
	if (_battleSave->getTile(endPosition + Position(1, 0, 0)) && _battleSave->getTile(endPosition + Position(1, 0, 0))->getTerrainLevel() == -18)
	{
		if (_battleSave->getTile(endPosition + Position(2, 0, 0)) && _battleSave->getTile(endPosition + Position(2, 0, 0))->getTerrainLevel() != -12)
		{
			return false;
		}
		if (startPosition == endPosition + Position(1, 0, 0) || startPosition == endPosition + Position(2, 0, 0) || startPosition == endPosition + Position(3, 0, 0))
		{
			return true;
		}
	}
	return false;
} */

}
