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

#include "Pathfinding.h"

//#include <algorithm>	// std::copy(), std::find(), std::min(), std::max(), std::min_element(), std::max_element(), std::reverse(), std::sort(), std::swap()
//#include <cmath>		// std::abs(), std::ceil()

#include "../fmath.h"

#include "BattlescapeGame.h"
#include "PathfindingOpenSet.h"

#include "../Engine/Options.h"

#include "../Ruleset/RuleArmor.h"

#include "../Savegame/BattleUnit.h"
#include "../Savegame/SavedBattleGame.h"
#include "../Savegame/Tile.h"


namespace OpenXcom
{

bool Pathfinding::_debug = false; // static.

Uint8 // static not const.
	Pathfinding::red	=  3u, // defaults ->
	Pathfinding::green	=  4u, // overridden by Interfaces.rul when BattlescapeState loads
	Pathfinding::yellow	= 10u;


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
		_pathAction(nullptr),
		_previewed(false),
		_strafe(false),
		_tuCostTally(0),
		_ctrl(false),
		_alt(false),
		_zPath(false),
		_mType(MT_WALK),
		_doorCost(0)
//		_tuFirst(-1)
{
	//Log(LOG_INFO) << "Create Pathfinding";
	_nodes.reserve(_battleSave->getMapSizeXYZ()); // reserve one PathfindingNode per tactical tile.

	Position pos;
	for (size_t // create one PathfindingNode per tile across the entire battlefield.
			i = 0u;
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
 * Sets unit etc in order to exploit pathing functions.
 * @note This has evolved into an initialization routine.
 * @param unit - pointer to a BattleUnit
 */
void Pathfinding::setPathingUnit(BattleUnit* const unit)
{
	_unit = unit;

	if (_battleSave->getBattleState() == nullptr) // safety for battlescape generation.
		_pathAction = nullptr;
	else
		_pathAction = _battleSave->getBattleGame()->getTacticalAction();

	if (unit != nullptr)
		setMoveType();
	else
		_mType = MT_WALK; // I really don't think this ever happens.
}

/**
 * Sets the movement type for the path.
 */
void Pathfinding::setMoveType() // private.
{
	if ((_mType = _unit->getMoveTypeUnit()) == MT_FLY
		&& _alt == true // this forces Soldiers in flight-suits to walk on (or fall to) the ground.
		&& _unit->getGeoscapeSoldier() != nullptr)
	{
		_mType = MT_WALK;
	}
}
//			|| _unit->getUnitRules()->isMechanical() == false)	// hovertanks & cyberdiscs always hover.
//		&& _unit->getRaceString() != "STR_FLOATER"				// floaters always float
//		&& _unit->getRaceString() != "STR_CELATID"				// celatids always .. float.
//		&& _unit->getRaceString() != "STR_ETHEREAL")			// Ethereals *can* walk, but they don't like to.
// Should turn this into Ruleset param: 'alwaysFloat'
// or use floatHeight > 0 or something-like-that

/**
 * Sets keyboard input modifiers.
 * @param missile - true if a launch-target is valid (default false)
 */
void Pathfinding::setInputModifiers(bool missile)
{
	if (_battleSave->getSide() != FACTION_PLAYER
		|| _battleSave->getBattleGame()->playerPanicHandled() == false
		|| missile == true)
	{
		_ctrl  =
		_alt   =
		_zPath = false;
	}
	else
	{
		_ctrl = (SDL_GetModState() & KMOD_CTRL) != 0;
		_alt  = (SDL_GetModState() & KMOD_ALT)  != 0;

		const Uint8* const keystate (SDL_GetKeyState(nullptr));
		_zPath = (keystate[SDLK_z] != 0);
	}
}

/**
 * Aborts the current path - clears the path vector.
 */
void Pathfinding::abortPath()
{
	//Log(LOG_INFO) << "";
	//Log(LOG_INFO) << "pf:abortPath() id-" << _unit->getId();
	_path.clear();
	_tuCostTally = 0;

	setInputModifiers();
}

/**
 * Calculates the shortest path w/ A* algorithm.
 * @note 'launchTarget' is required only when called by AlienBAIState::pathWaypoints().
 * @param unit				- pointer to a BattleUnit
 * @param posStop			- destination Position
 * @param tuCap				- maximum time units this path can cost (default TU_INFINITE)
 * @param launchTarget		- pointer to a targeted BattleUnit, used only by
 *							  AlienBAIState::pathWaypoints() (default nullptr)
 * @param strafeRejected	- true if path needs to be recalculated w/out strafe (default false)
 */
void Pathfinding::calculatePath(
		BattleUnit* const unit, // -> should not need 'unit' here anymore; done in setPathingUnit() unless FACTION_PLAYER ...
		Position posStop,
		int tuCap,
		const BattleUnit* const launchTarget,
		bool strafeRejected)
{
	//Log(LOG_INFO) << "";
	//Log(LOG_INFO) << "pf:calculatePath() id-" << unit->getId();
	//Log(LOG_INFO) << ". posStop " << posStop;
	// i'm DONE with these out of bounds errors.
	// kL_note: I really don't care what you're "DONE" with ..... if you're
	// going to cry like a babby, at least make it humorous - like
	// GREAT FLAMING TOADS OF HOLY RABBIT SHIT i can't stand it anymore!!1! PS. fixing out of bounds checks
	const int unitSize (unit->getArmor()->getSize());
	if (   posStop.x < 0
		|| posStop.y < 0
		|| posStop.x > _battleSave->getMapSizeX() - unitSize
		|| posStop.y > _battleSave->getMapSizeY() - unitSize)
	{
		return;
	}

	// WARNING: If the AI ever needs to path a guided-missile that's fired by a
	// large unit the 'unitSize' would need to be forced to "1" above^ and below_.


	abortPath();

	_unit = unit; // Or, setPathingUnit()

	if (launchTarget != nullptr)
	{
		_mType = MT_FLY;
		setInputModifiers(strafeRejected = true);
	}
	else
		setMoveType(); // redundant in some cases ...

	if (_unit->getFaction() != FACTION_PLAYER)
		strafeRejected = true;

	const Tile* tileStop;
	for (int // NOTE: Is this code-block even necessary since it's done basically again below_.
			x = 0;
			x != unitSize;
			++x)
	{
		for (int
				y = 0;
				y != unitSize;
				++y)
		{
			if ((tileStop = _battleSave->getTile(posStop + Position(x,y,0))) == nullptr)
			{
				//Log(LOG_INFO) << ". early out [1]";
				return; // <- probly also done again below_ Pertains only to large units.
			}

			if (isBlockedTile(
						tileStop,
						O_FLOOR,
						launchTarget) == true
				|| isBlockedTile(
						tileStop,
						O_CONTENT,
						launchTarget) == true
				|| tileStop->getTuCostTile(O_CONTENT, _mType) == PF_FAIL_TU)
			{
				//Log(LOG_INFO) << ". early out [2]";
				return;
			}
		}
	}

	static Position posStop_cache; // for keeping things straight if strafeRejected happens.
	posStop_cache = posStop;


	tileStop = _battleSave->getTile(posStop);
	while (tileStop->getTerrainLevel() == -24 && posStop.z != _battleSave->getMapSizeZ())
	{
		++posStop.z;
		//Log(LOG_INFO) << ". raise tileStop " << posStop;

		// ensure that 'posStop.z' wasn't adjusted outside the map.
		// This occurs in rare circumstances where an object has terrainLevel
		// -24 on the top floor and is considered passable terrain for whatever
		// reason (usually bigwall type objects)
		//
		// Example pls. cite MCD chapter & verse.
		if (posStop.z == _battleSave->getMapSizeZ())
		{
			//Log(LOG_INFO) << ". . icarus FAIL " << posStop;
			return; // Icarus is a bad role model for XCom soldiers.
		}

		tileStop = _battleSave->getTile(posStop);
	}

	if (_mType != MT_FLY)
	{
		//Log(LOG_INFO) << ". check drop tileStop";
		while (isUnitFloored(tileStop, unitSize) == false)
		{
			--posStop.z;
			tileStop = _battleSave->getTile(posStop);
			//Log(LOG_INFO) << ". . drop to " << posStop;
		}
	}


	if (isBlockedTile( // TODO: Check all quadrants. See above^.
			tileStop,
			O_FLOOR,
			launchTarget) == false
		&& isBlockedTile(
				tileStop,
				O_CONTENT,
				launchTarget) == false)
	{
		if (unitSize == 2)
		{
			const Tile* tileTest;
			const BattleUnit* unitTest;

			static const int dir[3u] {4,2,3};

			size_t i (0u);
			for (int
					x = 0;
					x != 2;
					++x)
			{
				for (int
						y = 0;
						y != 2;
						++y)
				{
					if (x != 0 || y != 0)
					{
						if (isBlockedDir(
										tileStop,
										dir[i],
										_unit) == true
							&& isBlockedDir(
										tileStop,
										dir[i],
										launchTarget) == true)
						{
							//Log(LOG_INFO) << ". early out [3]";
							return;
						}

						tileTest = _battleSave->getTile(posStop + Position(x,y,0));
						if (x == 1 && y == 1
							&& ((tileTest->getMapData(O_NORTHWALL) != nullptr
									&& tileTest->getMapData(O_NORTHWALL)->isHingeDoor() == true)
								|| (tileTest->getMapData(O_WESTWALL) != nullptr
									&& tileTest->getMapData(O_WESTWALL)->isHingeDoor() == true)))
						{
							//Log(LOG_INFO) << ". early out [4]";
							return;
						}

						if ((unitTest = tileTest->getTileUnit()) != nullptr
							&& unitTest != _unit
							&& unitTest != launchTarget
							&& unitTest->getUnitVisible() == true)
						{
							//Log(LOG_INFO) << ". early out [5]";
							return;
						}

						++i;
					}
				}
			}
		}


		const Position posStart (_unit->getPosition());
		//Log(LOG_INFO) << "posStart " << posStart;
		//Log(LOG_INFO) << "posStop  " << posStop;

		const bool isMech (_unit->isMechanical() == true);

		_pathAction->strafe =
		_strafe = strafeRejected == false
			   && Options::battleStrafe == true
			   && (    (_ctrl == true && isMech == false)
					|| (_alt  == true && isMech == true))
			   && (std::abs(
						findTerrainLevel(_battleSave->getTile(posStop), posStart.z)
					  - findTerrainLevel(_battleSave->getTile(posStart), posStop.z)) < 9)
			   && std::abs(posStop.x - posStart.x) < 2
			   && std::abs(posStop.y - posStart.y) < 2;
		//Log(LOG_INFO) << "strafe= " << _strafe;
		//Log(LOG_INFO) << "tLevel stop= " << findTerrainLevel(_battleSave->getTile(posStop), posStart.z);
		//Log(LOG_INFO) << "tLevel start= " << findTerrainLevel(_battleSave->getTile(posStart), posStop.z);
		//Log(LOG_INFO) << "tLevel diff= " << std::abs(findTerrainLevel(_battleSave->getTile(posStop), posStart.z)
		//										   - findTerrainLevel(_battleSave->getTile(posStart), posStop.z));

//		const bool sneak (Options::sneakyAI == true
//					   && _unit->getFaction() == FACTION_HOSTILE);

		if (aStarPath(
					posStart,
					posStop,
					launchTarget,
					tuCap) == true)
//					sneak) == true)
		{
			if (_path.empty() == false)
			{
				if (_strafe == true && _path.size() > 2u)
				{
					//Log(LOG_INFO) << ". recalc Path - strafeRejected";
					calculatePath( // iterate this function ONCE ->
								_unit,
								posStop_cache,
								tuCap,
								nullptr,
								true); // <- sets '_strafe' FALSE so recursion never gets back in here.
				}
				else if (Options::battleStrafe == true
					&& _ctrl == true
					&& _unit->getGeoscapeSoldier() != nullptr
					&& (_strafe == false
						|| (_path.size() == 1u
							&& _unit->getUnitDirection() == _path.front())))
				{
					//Log(LOG_INFO) << ". strafeRejected - dashing";
					_strafe =
					_pathAction->strafe = false;
					_pathAction->dash = true;
					if (_pathAction->actor != nullptr)
						_pathAction->actor->setDashing();
				}
				else
				{
					//Log(LOG_INFO) << ". regular.";
					_pathAction->dash = false;
					if (_pathAction->actor != nullptr)
						_pathAction->actor->setDashing(false);
				}
			}
		}
		else
		{
			//Log(LOG_INFO) << "ABORTED";
			abortPath();
		}
	}
}

/**
 * Finds the effective terrain-level of a specified Tile.
 * @note Helper for calculatePath(). Used in the odd situation of a large unit
 * reversing south and up a level, or north and down a level; the primary
 * quadrant will be in the air and so its real terrain-level will be
 * misrepresented when determining whether a tank-strafe is valid. This corrects
 * that inaccurate z-level discrepancy.
 * @param pos		- pointer to a tile
 * @param levelZ	- z-level of other tile
 * @return, terrain-level
 */
int Pathfinding::findTerrainLevel( // private.
		const Tile* tile,
		int levelZ) const
{
	if (tile->getPosition().z != levelZ)
	{
		while (tile->isFloored() == false)
			tile = tile->getTileBelow(_battleSave);
	}
	return tile->getTerrainLevel() - tile->getPosition().z * 24;
}

/**
 * Calculates the shortest path using a simple A* algorithm.
 * @note The unit information and movement type must have already been set. The
 * path information is set only if a valid path is found. See also findReachable().
 * @param posOrigin		- reference to the position to start from
 * @param posTarget		- reference to the position to end at
 * @param launchTarget	- pointer to targeted BattleUnit
 * @param tuCap			- maximum turn units this path can cost
// * @param sneak		- true if the unit is sneaking
 * @return, true if a path is found
 */
bool Pathfinding::aStarPath( // private.
		const Position& posOrigin,
		const Position& posTarget,
		const BattleUnit* const launchTarget,
		int tuCap)
//		bool sneak)
{
	//Log(LOG_INFO) << "";
	//Log(LOG_INFO) << "pf:aStarPath() id-" << _unit->getId();
	for (std::vector<PathfindingNode>::iterator
			i  = _nodes.begin();
			i != _nodes.end();
			++i)
	{
		i->resetNode();
	}

	PathfindingNode
		* nodeCurrent (getPfNode(posOrigin)),
		* nodeStep;

	nodeCurrent->linkNode(0, nullptr, 0, posTarget);

	PathfindingOpenSet openset;
	openset.addNode(nodeCurrent);

	Position posStep;
	int tuCost;
//	_tuFirst = -1; // ... could go in abortPath()

	while (openset.isOpenSetEmpty() == false)
	{
		nodeCurrent = openset.processNodeTop();
		nodeCurrent->setVisited();

		const Position& posCurrent (nodeCurrent->getPosition());
		//Log(LOG_INFO) << ". posCurrent " << (nodeCurrent->getPosition());

		if (posCurrent == posTarget) // finished.
		{
			_path.clear();

			while (nodeCurrent->getPriorNode() != nullptr)
			{
				_path.push_back(nodeCurrent->getPriorDir());
				//Log(LOG_INFO) << ". . " << nodeCurrent->getPriorDir();
				nodeCurrent = nodeCurrent->getPriorNode();
			}

			if (_strafe == true && _unit->isMechanical() == true) // is Tracked vehicle actually.
			{
				const int delta (std::abs((_path.back() + 4) % 8 - _unit->getUnitDirection()));
				if (delta > 1 && delta != 7)
				{
					_path.clear(); // safety.

					_strafe =
					_pathAction->strafe = false; // illegal direction for tank-strafe.
					return false;
				}
			}
			return true;
		}


		for (int
				dir = 0;
				dir != 10;
				++dir)
		{
			//Log(LOG_INFO) << ". dir= " << dir << " tuCost=" << tuCost;
			if ((tuCost = getTuCostPf(
								posCurrent,
								dir,
								&posStep,
								launchTarget)) < PF_FAIL_TU)
			{
//				if (sneak == true && _battleSave->getTile(posCurrent)->getTileVisible() == true)
//					tuCost *= 2;

				nodeStep = getPfNode(posStep);
				if (nodeStep->getVisited() == false)
				{
					if (_zPath == true && dir % 2 == 0)
						tuCost += 1;

					_tuCostTally = nodeCurrent->getTuCostTill(launchTarget != nullptr) + tuCost;
					//Log(LOG_INFO) << ". tuCostTotal= " << _tuCostTotal;

					if (_tuCostTally <= tuCap
						&& (nodeStep->inOpenSet() == false
							|| nodeStep->getTuCostTill(launchTarget != nullptr) > _tuCostTally)) // eg. costTotal is still 0.
					{
						nodeStep->linkNode(
										_tuCostTally,
										nodeCurrent,
										dir,
										posTarget);
						openset.addNode(nodeStep);

//						if (_tuFirst == -1) _tuFirst = tuCost;
					}
				}
			}
		}
	}
	return false;
}

/**
 * Gets the TU-cost for the first tile of motion.
 * @return, TU-cost for the first tile of motion.
 *
int Pathfinding::getTuFirst() const
{
	return _tuFirst;
} */

/**
 * Locates all tiles reachable to @a unit with a TU-cost no more than @a tuCap.
 * @note Uses Dijkstra's algorithm.
 * @sa aStarPath().
 * @param unit	- pointer to a BattleUnit
 * @param tuCap	- maximum turn units the path can cost
 * @return, vector of reachable tile-indices sorted in ascending order of cost;
 *			the first tile is the start-location itself
 */
std::vector<size_t> Pathfinding::findReachable(
		const BattleUnit* const unit,
		int tuCap)
{
	for (std::vector<PathfindingNode>::iterator
			i = _nodes.begin();
			i != _nodes.end();
			++i)
	{
		i->resetNode();
	}

	PathfindingNode
		* nodeCurrent (getPfNode(unit->getPosition())),
		* nodeStep;

	nodeCurrent->linkNode(0, nullptr, 0);

	PathfindingOpenSet openset;
	openset.addNode(nodeCurrent);

	std::vector<PathfindingNode*> nodes;	// NOTE: These are not route-nodes,
											// *every Tile* is a PathfindingNode.
	Position posStep;
	int
		tuCost,
		tuCostTotal;

	while (openset.isOpenSetEmpty() == false)
	{
		nodeCurrent = openset.processNodeTop();
		const Position& posCurrent (nodeCurrent->getPosition());

		for (int
				dir = 0;
				dir != 10;
				++dir)
		{
			tuCost = getTuCostPf(
							posCurrent,
							dir,
							&posStep);

			if (tuCost < PF_FAIL_TU)
			{
				tuCostTotal = nodeCurrent->getTuCostTill() + tuCost;
				if (   tuCostTotal <= tuCap
					&& tuCostTotal <= unit->getEnergy())	// NOTE: This does not account for less energy-expenditure
				{											// on gravLifts or for opening doors.
					nodeStep = getPfNode(posStep);
					if (nodeStep->getVisited() == false)
					{
						if (nodeStep->inOpenSet() == false
							|| nodeStep->getTuCostTill() > tuCostTotal)
						{
							nodeStep->linkNode(
											tuCostTotal,
											nodeCurrent,
											dir);
							openset.addNode(nodeStep);
						}
					}
				}
			}
		}
		nodeCurrent->setVisited();
		nodes.push_back(nodeCurrent);
	}

	std::sort(
			nodes.begin(),
			nodes.end(),
			IsCheaperPF());

	std::vector<size_t> nodeList;
	nodeList.reserve(nodes.size());
	for (std::vector<PathfindingNode*>::const_iterator
			i  = nodes.begin();
			i != nodes.end();
			++i)
	{
		//Log(LOG_INFO) << "pf: " << _battleSave->getTileIndex((*i)->getPosition());
		nodeList.push_back(_battleSave->getTileIndex((*i)->getPosition()));
	}
	return nodeList;
}

/**
 * Gets the PathfindingNode at a specified Position.
 * @param pos - reference to a Position
 * @return, pointer to the PathfindingNode
 */
PathfindingNode* Pathfinding::getPfNode(const Position& pos) // private.
{
	return &_nodes[_battleSave->getTileIndex(pos)];
}

/**
 * Gets the TU-cost to move from 1 tile to another - ONE STEP ONLY!
 * @note But also updates the destination Position because it is possible that
 * the unit uses stairs or falls while moving.
 * @param posStart		- reference to the start-position
 * @param dir			- direction of movement
 * @param posStop		- pointer to destination-position that will be set
 * @param launchTarget	- pointer to targeted BattleUnit (default nullptr)
 * @return, TU-cost or 255 if movement is impossible
 */
int Pathfinding::getTuCostPf(
		const Position& posStart,
		int dir,
		Position* const posStop,
		const BattleUnit* const launchTarget)
{
//	bool _debug;
//	if (   (posStart.z == 1 && posStart.x == 5 && posStart.y == 48)
//		|| (posStart.z == 1 && posStart.x == 5 && posStart.y == 47)) _debug = true;
//	else _debug = false;
//
//	if (_debug) {
//		Log(LOG_INFO) << "";
//		Log(LOG_INFO) << "pf:getTuCostPf() id-" << _unit->getId();
//		Log(LOG_INFO) << ". dir= " << dir;
//		Log(LOG_INFO) << ". posStart " << posStart;
//	}

	directionToVector(dir, posStop);
	*posStop += posStart;

//	if (_debug == true
//		&& (   (posStop->z == 1 && posStop->x == 5 && posStop->y == 47)
//			|| (posStop->z == 0 && posStop->x == 5 && posStop->y == 47)))
//	{
//		_debug = true;
//	}
//	else _debug = false;
//
//	if (_debug) Log(LOG_INFO) << ". posStop  " << (*posStop);

	bool
		fall (false),
		rise (false);

	int
		quadsGoingUp   (0),
		quadsGoingDown (0),	// TODO: 3 vars all denoting essentially the same thing ->
		quadsFalling   (0),	// The problem is they were trying to account for a unit
		quadsOnAir     (0),	// standing on nothing and a unit stepping onto nothing
							// simultaneously (during the same 'step'/call/loop).
		cost,
		costTotal (0);

	_doorCost = 0;

	const Tile
		* tileStart,
		* tileStop,
		* tileStopBelow,
		* tileStopAbove;

	Position posOffset;

	const int
		unitSize  (_unit->getArmor()->getSize()),
		quadrants (unitSize * unitSize);

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
// first, CHECK FOR BLOCKAGE ->> then calc TU-cost after.
			//if (_debug) Log(LOG_INFO) << ". . (" << x << "," << y << ")";
			cost = 0;

			posOffset = Position(x,y,0);

			if (   (tileStart = _battleSave->getTile(posStart + posOffset)) == nullptr
				|| (tileStop  = _battleSave->getTile(*posStop + posOffset)) == nullptr)
			{
				return PF_FAIL_TU;
			}

			if (_mType != MT_FLY)
			{
				if (x == 0 && y == 0
					&& isUnitFloored(
								tileStart,
								unitSize) == false)
				{
					if (dir != DIR_DOWN) return PF_FAIL_TU;

					fall = true;
					++quadsOnAir;
					//if (_debug) Log(LOG_INFO) << ". . . not Fly not Floored fall TRUE partsOnAir= " << partsOnAir;
				}
				else if (tileStart->isFloored(tileStart->getTileBelow(_battleSave)) == false
					&& ++quadsOnAir == quadrants)
				{
					if (dir != DIR_DOWN) return PF_FAIL_TU;

					fall = true;
					//if (_debug) Log(LOG_INFO) << ". . . not Fly no Floor fall TRUE partsOnAir= " << partsOnAir;
				}
			}

			if (x == 1 && y == 1 // don't let large units phase through doors
				&& (      (tileStop->getMapData(O_NORTHWALL) != nullptr
						&& tileStop->getMapData(O_NORTHWALL)->isHingeDoor() == true)
					||    (tileStop->getMapData(O_WESTWALL) != nullptr
						&& tileStop->getMapData(O_WESTWALL)->isHingeDoor() == true)))
			{
				//if (_debug) Log(LOG_INFO) << ". . . clipped by hinged door PF_FAIL_TU";
				return PF_FAIL_TU;
			}

			if (dir < DIR_UP && tileStart->getTerrainLevel() > -16
				&& (tileStart->getTerrainLevel() - tileStop->getTerrainLevel() > 8
					|| isBlockedDir(
								tileStart,
								dir,
								launchTarget) == true))
			{
				//if (_debug) Log(LOG_INFO) << ". . . tLevel too great Or tile block PF_FAIL_TU [1]";
				return PF_FAIL_TU;
			}


			Position posOffsetVertical; // this will later be used to re-init the startTile [inits to (0,0,0)]

			tileStopBelow = _battleSave->getTile(*posStop + posOffset + Position::POS_BELOW),
			tileStopAbove = _battleSave->getTile(*posStop + posOffset + Position::POS_ABOVE);

			if (rise == false
				&& dir < DIR_UP
				&& tileStart->getTerrainLevel() < -15
				&& tileStopAbove != nullptr
				&& tileStopAbove->isFloored(tileStop) == true)
			{
				//if (_debug) Log(LOG_INFO) << ". . . going up";
				if (++quadsGoingUp == unitSize)
				{
					//if (_debug) Log(LOG_INFO) << ". . . . going up all quadrants";
					rise = true;

					++posStop->z;
					++posOffsetVertical.z;
					tileStop = _battleSave->getTile(*posStop + posOffset);
				}
			}
			else if (fall == false
				&& dir < DIR_UP
				&& _mType != MT_FLY
				&& tileStop->isFloored(tileStop->getTileBelow(_battleSave)) == false
				&& tileStopBelow != nullptr
				&& tileStopBelow->getTerrainLevel() < -11)
			{
				//if (_debug) Log(LOG_INFO) << ". . . going down";
				if (++quadsGoingDown == quadrants)
				{
					//if (_debug) Log(LOG_INFO) << ". . . . going down all quadrants";
					fall = true;

					--posStop->z;
					tileStop = _battleSave->getTile(*posStop + posOffset);

					if (tileStopBelow->getMapData(O_CONTENT) != nullptr)		// copied from below_
						cost = 4 + tileStopBelow->getTuCostTile(O_CONTENT, _mType);
					else if (tileStopBelow->getMapData(O_FLOOR) != nullptr)
						cost = tileStopBelow->getTuCostTile(O_FLOOR, _mType);
					else
						cost = 4;

					if ((dir & 1) == 1)
						cost += (cost + 1) >> 1u;								// end_Copy.
				}
			}
			else if (_mType == MT_FLY
				&& launchTarget == nullptr // TODO: <-- Think about that.
				&& tileStopBelow != nullptr
				&& tileStopBelow->getTileUnit() != nullptr
				&& tileStopBelow->getTileUnit() != _unit
				&& tileStopBelow->getTileUnit()->getHeight(true) - tileStopBelow->getTerrainLevel() > UNIT_HEIGHT) // cf. UnitWalkBState::statusStand()
			{
				//if (_debug) Log(LOG_INFO) << ". . . flight clipped by a unit below PF_FAIL_TU";
				return PF_FAIL_TU;
			}

			if (tileStop == nullptr)
			{
				//if (_debug) Log(LOG_INFO) << ". . . tileStop Invalid PF_FAIL_TU";
				return PF_FAIL_TU;
			}

			switch (dir)
			{
				case DIR_UP:
				case DIR_DOWN:
					if (fall == false || _alt == true)
					{
						switch (validateUpDown(
											posStart + posOffset,
											dir))
//											launchTarget != nullptr))
						{
							case FLY_CANT:
							case FLY_BLOCKED:
								//if (_debug) Log(LOG_INFO) << ". . . fly blocked PF_FAIL_TU";
								return PF_FAIL_TU;

							case FLY_GRAVLIFT:
							case FLY_GOOD:
								cost = 8;

								if (tileStop->getMapData(O_CONTENT) != nullptr)
								{
									switch (tileStop->getMapData(O_CONTENT)->getBigwall())
									{
										case BIGWALL_NONE:
										case BIGWALL_BLOCK:
										case BIGWALL_NESW:
										case BIGWALL_NWSE:
											cost += tileStop->getTuCostTile(O_CONTENT, _mType);
											// TODO: Early-exit if cost>PF_FAIL_TU.
									}
								}
						}
					}
					break;

				default:
					if (quadsGoingUp == 0 // Warboy_add. 2017 oct 31
						&& posStop->z == tileStart->getPosition().z
						&& (tileStart->getTerrainLevel() - tileStop->getTerrainLevel() > 8
							|| isBlockedDir(
										tileStart,
										dir,
										launchTarget) == true))
					{
						//if (_debug) Log(LOG_INFO) << ". . . tLevel too great Or tile block PF_FAIL_TU [2]";
						return PF_FAIL_TU;
					}
			}

			if (fall == false
				&& dir != DIR_DOWN
				&& _mType != MT_FLY
				&& tileStart->isFloored(tileStart->getTileBelow(_battleSave)) == false)
			{
				//if (_debug) Log(LOG_INFO) << ". . . falling";
				if (++quadsFalling == quadrants)
				{
					//if (_debug) Log(LOG_INFO) << ". . . . falling all quadrants";
					fall = true;
					dir = DIR_DOWN;

					*posStop = posStart + Position::POS_BELOW;
					tileStop = _battleSave->getTile(*posStop + posOffset);
				}
			}

			tileStart = _battleSave->getTile(tileStart->getPosition() + posOffsetVertical);
			//if (_debug) Log(LOG_INFO) << ". . . tileStart " << (tileStart->getPosition() + posOffsetVertical);

			if (dir < DIR_UP
				&& quadsGoingUp != 0
				&& (tileStart->getTerrainLevel() - tileStop->getTerrainLevel() > 8
					|| isBlockedDir(
								tileStart,
								dir,
								launchTarget) == true))
			{
				//if (_debug) Log(LOG_INFO) << ". . . tLevel too great Or tile block PF_FAIL_TU [3]";
				return PF_FAIL_TU;
			}

			if (isBlockedTile(
						tileStop,
						O_FLOOR,
						launchTarget) == true
				|| isBlockedTile(
							tileStop,
							O_CONTENT,
							launchTarget) == true
				|| tileStop->getTuCostTile(O_CONTENT, _mType) == PF_FAIL_TU)
			{
				//if (_debug) Log(LOG_INFO) << ". . . tile blocked or tileCost PF_FAIL_TU";
				return PF_FAIL_TU;
			}
// CHECK FOR BLOCKAGE_end.

			//if (_debug) Log(LOG_INFO) << ". . dir= " << dir;
			//if (_debug) Log(LOG_INFO) << ". . posStop= " << (*posStop);
			//if (_debug) Log(LOG_INFO) << ". . fall= " << fall;

// Calculate TU costage ->
			if (dir < DIR_UP && fall == false)
			{
//				if (tileStop->getMapData(O_FLOOR) != nullptr)					// copied above^
//					cost = tileStop->getTuCostTile(O_FLOOR, _mType);
//				else
//					cost = 4;
//
//				if (tileStop->getMapData(O_CONTENT) != nullptr) //&& rise == false
//					cost += tileStop->getTuCostTile(O_CONTENT, _mType);

				if (tileStop->getMapData(O_CONTENT) != nullptr) //&& rise == false
					cost = 4 + tileStop->getTuCostTile(O_CONTENT, _mType);
				else if (tileStop->getMapData(O_FLOOR) != nullptr)
					cost = tileStop->getTuCostTile(O_FLOOR, _mType);
				else
					cost = 4;

				if ((dir & 1) == 1)
					cost += (cost + 1) >> 1u;									// end_Copy.

				// TODO: Early-exit if cost>PF_FAIL_TU.
//				if (cost >= PF_FAIL_TU) return PF_FAIL_TU; // quick outs ->
//				else if (launchTarget != nullptr) return 0; // <- provided walls have actually been blocked already, not based on TU > PF_FAIL_TU

//				if (posOffsetVertical.z > 0) ++cost;

				if (tileStart->getPosition().z == tileStop->getPosition().z)	// Do not charge wall-cost if moving up or down a level.
					cost += getWallTuCost(dir, tileStart, tileStop);			// NOTE: Custom mapblocks could flaunt that but it's unlikely.
			}

			if (tileStop->getFire() != 0)
			{
				if (_unit->getSpecialAbility() != SPECAB_BURN)
					cost += 2 + (dir & 1);

				if (_unit->avoidsFire() == true)	// NOTE: Uses vulnr instead of Specab_Burn.
					cost += TU_FIRE_AVOID;			// cf. UnitWalkBState::statusStand() - this gets subtracted.
			}

			// Propose: if flying then no extra TU-cost
			//Log(LOG_INFO) << ". pathSize = " << (int)_path.size();
			if (_strafe == true && _unit->getUnitDirection() != dir) // if not dashing straight ahead 1 tile.
			{
				// Extra TU for strafe-moves ->	1 0 1
				//								2 ^ 2
				//								3 2 3
				int delta (std::min(
								std::abs(dir - _unit->getUnitDirection() + 8),
								std::min(
									std::abs(_unit->getUnitDirection() - dir),
									std::abs(_unit->getUnitDirection() - dir + 8))));
				if (delta == 4) delta = 2;

				cost += delta;
			}
			costTotal += cost;
		}
	}

	if (unitSize == 2) // only for Large units ->
	{
		// - check the path between part 0,0 and part 1,1 at destination position
//		if (   (posStop->z == 0 || posStop->z == 1)
//			&& (posStop->x >  3 && posStop->x <  6)
//			&& (posStop->y > 40 && posStop->y < 50))
//		{
//			if (_debug) Log(LOG_INFO) << ". large posStop " << (*posStop);
//		}

		const Tile* const ulTile (_battleSave->getTile(*posStop));
		if (isBlockedDir(
					ulTile,
					3,
					launchTarget) == true)
		{
			//if (_debug) Log(LOG_INFO) << ". . large blocked [1]";
			return PF_FAIL_TU;
		}

		// - then check the path between part 1,0 and part 0,1 at destination position
		const Tile* const urTile (_battleSave->getTile(*posStop + Position(1,0,0)));
		if (isBlockedDir(
					urTile,
					5,
					launchTarget) == true)
		{
			//if (_debug) Log(LOG_INFO) << ". . large blocked [2]";
			return PF_FAIL_TU;
		}

		if (fall == false)
		{
			const Tile
				* const llTile (_battleSave->getTile(*posStop + Position(0,1,0))),
				* const lrTile (_battleSave->getTile(*posStop + Position(1,1,0)));
			const int
				levels[4u]
				{
					ulTile->getTerrainLevel(),
					urTile->getTerrainLevel(),
					llTile->getTerrainLevel(),
					lrTile->getTerrainLevel()
				},

				minLevel (*std::min_element(levels, levels + 4)),
				maxLevel (*std::max_element(levels, levels + 4));

			if (std::abs(maxLevel - minLevel) > 8)
			{
				//if (_debug) Log(LOG_INFO) << ". . large blocked [3]";
//				return PF_FAIL_TU;
			}
		}

		costTotal = (costTotal + quadrants - 1) / quadrants; // round up.
	} // largeUnits_end.

	//if (_debug) Log(LOG_INFO) << ". COST TU= " << costTotal;
	return costTotal;
}

/**
 * Gets the TU-cost for crossing over walls.
 * @note Helper for getTuCostPf().
 * @param dir		- direction of travel
 * @param tileStart	- pointer to a startTile
 * @param tileStop	- pointer to a stopTile
 * @return, TU-cost for crossing over walls
 */
int Pathfinding::getWallTuCost( // private.
		int dir,
		const Tile* const tileStart,
		const Tile* const tileStop)
{
	int
		tuTotal	 (0), // walking over fences, hedges, and rubble walls
		partCost (0), // differentiates wall-Total from wall-current and keeps the '_doorCost' value straight
		walls	 (0); // quantity of walls crossed if moving diagonally

	const Tile* tile;

	switch (dir)
	{
		case 0:
//			if (tileStart->getPosition().z < tileStop->getPosition().z) // don't count wallCosts on the floor below.
//				tile = _battleSave->getTile(tileStart->getPosition() + Position::POS_ABOVE);
//			else
//				tile = tileStart;

//			if (tileStart->getPosition().z == tileStop->getPosition().z)
//			{
			tile = tileStart;
			if ((partCost = tile->getTuCostTile(O_NORTHWALL, _mType)) > 0)
			{
				tuTotal += partCost;

				if (tile->getMapData(O_NORTHWALL)->isDoor() == true)
					if (partCost > _doorCost) _doorCost = partCost; // don't let large unit parts reset _doorCost prematurely.
			}
//			}
			break;

		case 1:
//			if (tileStart->getPosition().z == tileStop->getPosition().z)
//			{
			tile = tileStart;
			if ((partCost = tile->getTuCostTile(O_NORTHWALL, _mType)) > 0)
			{
				tuTotal += partCost;
				++walls;

				if (tile->getMapData(O_NORTHWALL)->isDoor() == true)
					if (partCost > _doorCost) _doorCost = partCost;
			}

			tile = _battleSave->getTile(tileStart->getPosition() + Position(1,0,0)); // tileEast
			if ((partCost = tile->getTuCostTile(O_NORTHWALL, _mType)) > 0)
			{
				tuTotal += partCost;
				++walls;

				if (tile->getMapData(O_NORTHWALL)->isDoor() == true)
					if (partCost > _doorCost) _doorCost = partCost;
			}

			if ((partCost = tile->getTuCostTile(O_WESTWALL, _mType)) > 0)
			{
				tuTotal += partCost;
				++walls;

				if (tile->getMapData(O_WESTWALL)->isDoor() == true)
					if (partCost > _doorCost) _doorCost = partCost;
			}

			tile = tileStop; //_battleSave->getTile(tileStart->getPosition() + Position(1,-1,0)); // tileNorthEast
			if ((partCost = tile->getTuCostTile(O_WESTWALL, _mType)) > 0)
			{
				tuTotal += partCost;
				++walls;

				if (tile->getMapData(O_WESTWALL)->isDoor() == true)
					if (partCost > _doorCost) _doorCost = partCost;
			}
//			}
			break;

		case 2:
//			if (tileStart->getPosition().z > tileStop->getPosition().z)
//				tile = _battleSave->getTile(tileStop->getPosition() + Position::POS_ABOVE);
//			else
//				tile = tileStop;

//			if (tileStart->getPosition().z == tileStop->getPosition().z)
//			{
			tile = tileStop;
			if ((partCost = tile->getTuCostTile(O_WESTWALL, _mType)) > 0)
			{
				tuTotal += partCost;

				if (tile->getMapData(O_WESTWALL)->isDoor() == true)
					if (partCost > _doorCost) _doorCost = partCost;
			}
//			}
			break;

		case 3:
//			if (tileStart->getPosition().z == tileStop->getPosition().z)
//			{
			tile = _battleSave->getTile(tileStart->getPosition() + Position(1,0,0)); // tileEast
			if ((partCost = tile->getTuCostTile(O_WESTWALL, _mType)) > 0)
			{
				tuTotal += partCost;
				++walls;

				if (tile->getMapData(O_WESTWALL)->isDoor() == true)
					if (partCost > _doorCost) _doorCost = partCost;
			}

			tile = tileStop; //_battleSave->getTile(tileStart->getPosition() + Position(1,1,0)); // tileSouthEast
			if ((partCost = tile->getTuCostTile(O_WESTWALL, _mType)) > 0)
			{
				tuTotal += partCost;
				++walls;

				if (tile->getMapData(O_WESTWALL)->isDoor() == true)
					if (partCost > _doorCost) _doorCost = partCost;
			}

			if ((partCost = tile->getTuCostTile(O_NORTHWALL, _mType)) > 0)
			{
				tuTotal += partCost;
				++walls;

				if (tile->getMapData(O_NORTHWALL)->isDoor() == true)
					if (partCost > _doorCost) _doorCost = partCost;
			}

			tile = _battleSave->getTile(tileStart->getPosition() + Position(0,1,0)); // tileSouth
			if ((partCost = tile->getTuCostTile(O_NORTHWALL, _mType)) > 0)
			{
				tuTotal += partCost;
				++walls;

				if (tile->getMapData(O_NORTHWALL)->isDoor() == true)
					if (partCost > _doorCost) _doorCost = partCost;
			}
//			}
			break;

		case 4:
//			if (tileStart->getPosition().z > tileStop->getPosition().z)
//				tile = _battleSave->getTile(tileStop->getPosition() + Position::POS_ABOVE);
//			else
//				tile = tileStop;

//			if (tileStart->getPosition().z == tileStop->getPosition().z)
//			{
			tile = tileStop;
			if ((partCost = tile->getTuCostTile(O_NORTHWALL, _mType)) > 0)
			{
				tuTotal += partCost;

				if (tile->getMapData(O_NORTHWALL)->isDoor() == true)
					if (partCost > _doorCost) _doorCost = partCost;
			}
//			}
			break;

		case 5:
//			if (tileStart->getPosition().z == tileStop->getPosition().z)
//			{
			tile = tileStart;
			if ((partCost = tile->getTuCostTile(O_WESTWALL, _mType)) > 0)
			{
				tuTotal += partCost;
				++walls;

				if (tile->getMapData(O_WESTWALL)->isDoor() == true)
					if (partCost > _doorCost) _doorCost = partCost;
			}

			tile = _battleSave->getTile(tileStart->getPosition() + Position(0,1,0)); // tileSouth
			if ((partCost = tile->getTuCostTile(O_WESTWALL, _mType)) > 0)
			{
				tuTotal += partCost;
				++walls;

				if (tile->getMapData(O_WESTWALL)->isDoor() == true)
					if (partCost > _doorCost) _doorCost = partCost;
			}

			if ((partCost = tile->getTuCostTile(O_NORTHWALL, _mType)) > 0)
			{
				tuTotal += partCost;
				++walls;

				if (tile->getMapData(O_NORTHWALL)->isDoor() == true)
					if (partCost > _doorCost) _doorCost = partCost;
			}

			tile = tileStop; //_battleSave->getTile(tileStart->getPosition() + Position(-1,1,0)); // tileSouthWest
			if ((partCost = tile->getTuCostTile(O_NORTHWALL, _mType)) > 0)
			{
				tuTotal += partCost;
				++walls;

				if (tile->getMapData(O_NORTHWALL)->isDoor() == true)
					if (partCost > _doorCost) _doorCost = partCost;
			}
//			}
			break;

		case 6:
//			if (tileStart->getPosition().z < tileStop->getPosition().z)
//				tile = _battleSave->getTile(tileStart->getPosition() + Position::POS_ABOVE);
//			else
//				tile = tileStart;

//			if (tileStart->getPosition().z == tileStop->getPosition().z)
//			{
			tile = tileStart;
			if ((partCost = tile->getTuCostTile(O_WESTWALL, _mType)) > 0)
			{
				tuTotal += partCost;

				if (tile->getMapData(O_WESTWALL)->isDoor() == true)
					if (partCost > _doorCost) _doorCost = partCost;
			}
//			}
			break;

		case 7:
//			if (tileStart->getPosition().z == tileStop->getPosition().z)
//			{
			tile = tileStart;
			if ((partCost = tile->getTuCostTile(O_WESTWALL, _mType)) > 0)
			{
				tuTotal += partCost;
				++walls;

				if (tile->getMapData(O_WESTWALL)->isDoor() == true)
					if (partCost > _doorCost) _doorCost = partCost;
			}

			if ((partCost = tile->getTuCostTile(O_NORTHWALL, _mType)) > 0)
			{
				tuTotal += partCost;
				++walls;

				if (tile->getMapData(O_NORTHWALL)->isDoor() == true)
					if (partCost > _doorCost) _doorCost = partCost;
			}

			tile = _battleSave->getTile(tileStart->getPosition() + Position(-1,0,0)); // tileWest
			if ((partCost = tile->getTuCostTile(O_NORTHWALL, _mType)) > 0)
			{
				tuTotal += partCost;
				++walls;

				if (tile->getMapData(O_NORTHWALL)->isDoor() == true)
					if (partCost > _doorCost) _doorCost = partCost;
			}

			tile = _battleSave->getTile(tileStart->getPosition() + Position(0,-1,0)); // tileNorth
			if ((partCost = tile->getTuCostTile(O_WESTWALL, _mType)) > 0)
			{
				tuTotal += partCost;
				++walls;

				if (tile->getMapData(O_WESTWALL)->isDoor() == true)
					if (partCost > _doorCost) _doorCost = partCost;
			}
//			}
	}

	if (walls != 0) // && _doorCost == 0
		tuTotal = ((tuTotal + (tuTotal >> 1u)) + walls - 1) / walls;

//	if ((dir & 1) == 1)
//	{
//		if (tuTotal != 0 && _doorCost == 0)
//		{
//			if (walls != 0)
//				tuTotal /= walls; // average of the walls crossed
//
//			if (((tuTotal - walls) & 1) == 1) // round wallCost up.
//				++tuTotal;
//		}
//	}
	return tuTotal;
}

/**
 * Determines whether going from one Tile to another is blocked.
 * @param startTile		- pointer to start-tile
 * @param dir			- direction of movement
 * @param launchTarget	- pointer to targeted BattleUnit (default nullptr)
 * @return, true if path is blocked
 */
bool Pathfinding::isBlockedDir(
		const Tile* const startTile,
		const int dir,
		const BattleUnit* const launchTarget) const
{
	//Log(LOG_INFO) << "Pathfinding::isBlockedDir()";
	const Position pos (startTile->getPosition());

	static const Position
		posNorth (Position( 0,-1, 0)),
		posEast  (Position( 1, 0, 0)),
		posSouth (Position( 0, 1, 0)),
		posWest  (Position(-1, 0, 0));


	switch (dir)
	{
		case 0:	// north
			//Log(LOG_INFO) << ". try North";
			if (isBlockedTile(
						startTile,
						O_NORTHWALL,
						launchTarget) == true)
			{
				return true;
			}
			break;

		case 1: // north-east
			//Log(LOG_INFO) << ". try NorthEast";
			if (isBlockedTile(
						startTile,
						O_NORTHWALL,
						launchTarget) == true
				|| isBlockedTile(
						_battleSave->getTile(pos + posEast),
						O_WESTWALL,
						launchTarget) == true
				|| isBlockedTile(
						_battleSave->getTile(pos + posEast),
						O_NORTHWALL,
						launchTarget) == true
				|| isBlockedTile(
						_battleSave->getTile(pos + posEast),
						O_CONTENT,
						launchTarget,
						BIGWALL_NESW) == true
				|| isBlockedTile(
						_battleSave->getTile(pos + posEast + posNorth),
						O_WESTWALL,
						launchTarget) == true
				|| isBlockedTile(
						_battleSave->getTile(pos + posNorth),
						O_CONTENT,
						launchTarget,
						BIGWALL_NESW) == true)
			{
				return true;
			}
			break;

		case 2: // east
			//Log(LOG_INFO) << ". try East";
			if (isBlockedTile(
						_battleSave->getTile(pos + posEast),
						O_WESTWALL,
						launchTarget) == true)
			{
				return true;
			}
			break;

		case 3: // south-east
			//Log(LOG_INFO) << ". try SouthEast";
			if (isBlockedTile(
						_battleSave->getTile(pos + posEast),
						O_WESTWALL,
						launchTarget) == true
				|| isBlockedTile(
						_battleSave->getTile(pos + posEast),
						O_CONTENT,
						launchTarget,
						BIGWALL_NWSE) == true
				|| isBlockedTile(
						_battleSave->getTile(pos + posEast + posSouth),
						O_NORTHWALL,
						launchTarget) == true
				|| isBlockedTile(
						_battleSave->getTile(pos + posEast + posSouth),
						O_WESTWALL,
						launchTarget) == true
				|| isBlockedTile(
						_battleSave->getTile(pos + posSouth),
						O_NORTHWALL,
						launchTarget) == true
				|| isBlockedTile(
						_battleSave->getTile(pos + posSouth),
						O_CONTENT,
						launchTarget,
						BIGWALL_NWSE) == true)
			{
				return true;
			}
			break;

		case 4: // south
			//Log(LOG_INFO) << ". try South";
			if (isBlockedTile(
						_battleSave->getTile(pos + posSouth),
						O_NORTHWALL,
						launchTarget) == true)
			{
				return true;
			}
			break;

		case 5: // south-west
			//Log(LOG_INFO) << ". try SouthWest";
			if (isBlockedTile(
						startTile,
						O_WESTWALL,
						launchTarget) == true
				|| isBlockedTile(
						_battleSave->getTile(pos + posSouth),
						O_WESTWALL,
						launchTarget) == true
				|| isBlockedTile(
						_battleSave->getTile(pos + posSouth),
						O_NORTHWALL,
						launchTarget) == true
				|| isBlockedTile(
						_battleSave->getTile(pos + posSouth),
						O_CONTENT,
						launchTarget,
						BIGWALL_NESW) == true
				|| isBlockedTile(
						_battleSave->getTile(pos + posSouth + posWest),
						O_NORTHWALL,
						launchTarget) == true
				|| isBlockedTile(
						_battleSave->getTile(pos + posWest),
						O_CONTENT,
						launchTarget,
						BIGWALL_NESW) == true)
			{
				return true;
			}
			break;

		case 6: // west
			//Log(LOG_INFO) << ". try West";
			if (isBlockedTile(
						startTile,
						O_WESTWALL,
						launchTarget) == true)
			{
				return true;
			}
			break;

		case 7: // north-west
			//Log(LOG_INFO) << ". try NorthWest";
			if (isBlockedTile(
						startTile,
						O_WESTWALL,
						launchTarget) == true
				|| isBlockedTile(
						startTile,
						O_NORTHWALL,
						launchTarget) == true
				|| isBlockedTile(
						_battleSave->getTile(pos + posWest),
						O_NORTHWALL,
						launchTarget) == true
				|| isBlockedTile(
						_battleSave->getTile(pos + posWest),
						O_CONTENT,
						launchTarget,
						BIGWALL_NWSE) == true
				|| isBlockedTile(
						_battleSave->getTile(pos + posNorth),
						O_WESTWALL,
						launchTarget) == true
				|| isBlockedTile(
						_battleSave->getTile(pos + posNorth),
						O_CONTENT,
						launchTarget,
						BIGWALL_NWSE) == true)
			{
				return true;
			}
	}
	return false;
}

/**
 * Determines whether a specified part of a Tile blocks movement.
 * @param tile			- pointer to a tile can be nullptr
 * @param partType		- part of the tile (MapData.h)
 * @param launchTarget	- pointer to targeted BattleUnit (default nullptr)
 * @param diagExclusion	- to exclude diagonal bigWalls (Pathfinding.h) (default BIGWALL_NONE)
 * @return, true if path is blocked
 */
bool Pathfinding::isBlockedTile( // private.
		const Tile* const tile,
		const MapDataType partType,
		const BattleUnit* const launchTarget,
		const BigwallType diagExclusion) const
{
	//Log(LOG_INFO) << "Pathfinding::isBlockedTile()";
	if (tile == nullptr) return true;

	const MapData* part;

	switch (partType)
	{
		case O_CONTENT:
			//Log(LOG_INFO) << ". part is Bigwall/object " << tile->getPosition();
			if ((part = tile->getMapData(O_CONTENT)) != nullptr)
			{
				const BigwallType bigType (part->getBigwall());
				switch (bigType)
				{
					case BIGWALL_BLOCK:
					case BIGWALL_NESW:
					case BIGWALL_NWSE:
						if (bigType != diagExclusion)
							return true;
				}
			}
			return false; // NOTE: I know this is here for a good reason but it keeps getting in my way.

		case O_WESTWALL:
		{
			//Log(LOG_INFO) << ". part is Westwall";
			if (launchTarget != nullptr								// missiles can't pathfind through closed doors.
				&& (part = tile->getMapData(O_WESTWALL)) != nullptr	// ... neither can proxy mines.
				&& (part->isHingeDoor() == true
					|| (part->isSlideDoor() == true
						&& tile->isSlideDoorOpen(O_WESTWALL) == false)))
			{
				return true;
			}

			if ((part = tile->getMapData(O_CONTENT)) != nullptr)
			{
				switch (part->getBigwall())
				{
					case BIGWALL_WEST:
//					case BIGWALL_W_N: // NOT USED in stock UFO.
						return true;
				}
			}

			const Tile* const tileWest (_battleSave->getTile(tile->getPosition() + Position(-1,0,0)));
			if (tileWest == nullptr)
				return true;

			if ((part = tileWest->getMapData(O_CONTENT)) != nullptr)
			{
				switch (part->getBigwall())
				{
					case BIGWALL_EAST:
					case BIGWALL_E_S:
						return true;
				}
			}
			break;
		}

		case O_NORTHWALL:
		{
			//Log(LOG_INFO) << ". part is Northwall";
			if (launchTarget != nullptr									// missiles can't pathfind through closed doors.
				&& (part = tile->getMapData(O_NORTHWALL)) != nullptr	// ... neither can proxy mines.
				&& (part->isHingeDoor() == true
					|| (part->isSlideDoor() == true
						&& tile->isSlideDoorOpen(O_NORTHWALL) == false)))
			{
				return true;
			}

			if ((part = tile->getMapData(O_CONTENT)) != nullptr)
			{
				switch (part->getBigwall())
				{
					case BIGWALL_NORTH:
//					case BIGWALL_W_N: // NOT USED in stock UFO.
						return true;
				}
			}

			const Tile* const tileNorth (_battleSave->getTile(tile->getPosition() + Position(0,-1,0)));
			if (tileNorth == nullptr)
				return true;

			if ((part = tileNorth->getMapData(O_CONTENT)) != nullptr)
			{
				switch (part->getBigwall())
				{
					case BIGWALL_SOUTH:
					case BIGWALL_E_S:
						return true;
				}
			}
			break;
		}

		case O_FLOOR:
		{
			//Log(LOG_INFO) << ". part is Floor";
			const BattleUnit* blockUnit (tile->getTileUnit());

			if (blockUnit != nullptr)
			{
				if (   blockUnit == _unit
					|| blockUnit == launchTarget
					|| blockUnit->getUnitStatus() != STATUS_STANDING)
//					|| blockUnit->isOut_t(OUT_STAT) == true)
				{
					return false;
				}

				if (   launchTarget != nullptr // <- isAI
					&& launchTarget != blockUnit
					&& blockUnit->getFaction() == FACTION_HOSTILE) // huh. ||isNeutral && isVis. i'd guess
				{
					return true;
				}

				if (_unit != nullptr)
				{
					if (_unit->getFaction() == blockUnit->getFaction())
						return true;

					switch (_unit->getFaction())
					{
						case FACTION_PLAYER:
							if (blockUnit->getUnitVisible() == true)
								return true;
							break;

						case FACTION_HOSTILE:
						case FACTION_NEUTRAL: // TODO: Perhaps use (exposed<intelligence) instead of hostileUnitsThisTurn.
//							if (std::find(
//									_unit->getHostileUnitsThisTurn().begin(),				// TEST
//									_unit->getHostileUnitsThisTurn().end(),					// TODO: The ai hangs (loops) if there is a blocking unit that it hasn't spotted
//									blockUnit) != _unit->getHostileUnitsThisTurn().end())	// after AlienBAIState::findFirePosition() -> do Move. So fix the AI ...
							{
								return true;
							}
					}
				}
			}
			else if (tile->isFloored() == false	// This section is devoted to ensuring that large units
				&& _mType != MT_FLY)			// do not take part in any kind of falling behaviour.
			{
				Position pos (tile->getPosition());
				while (pos.z != -1)
				{
					const Tile* const testTile (_battleSave->getTile(pos));
					blockUnit = testTile->getTileUnit();

					if (blockUnit != nullptr && blockUnit != _unit)
					{
						if (_unit != nullptr && _unit->getArmor()->getSize() == 2)	// don't let large units fall on other units
							return true;

						if (   blockUnit != launchTarget							// don't let any units fall on large units
							&& blockUnit->getUnitStatus() == STATUS_STANDING
							&& blockUnit->getArmor()->getSize() == 2)
						{
							return true;
						}
					}

					if (testTile->isFloored() == true)
						break;

					--pos.z;
				}
			}
		}
	}

	static const int TU_LARGEBLOCK (6); // stop large units from going through hedges and over fences

	const int partCost (tile->getTuCostTile(partType, _mType));
	if (partCost == PF_FAIL_TU
		|| (partCost > TU_LARGEBLOCK
			&& _unit != nullptr
			&& _unit->getArmor()->getSize() == 2))
	{
		//Log(LOG_INFO) << "isBlockedTile() EXIT true, partType = " << partType << " MT = " << (int)_mType;
		return true;
	}
	//Log(LOG_INFO) << "isBlockedTile() EXIT false, partType = " << partType << " MT = " << (int)_mType;
	return false;
}

/**
 * Marks Tiles with pathfinding-info for the path-preview.
 * @param clear - true clears the preview (default false)
 * @return,	true if a preview-path is created or discarded
 *			false if nothing changed
 */
bool Pathfinding::previewPath(bool clear)
{
	//Log(LOG_INFO) << "";
	//Log(LOG_INFO) << "pf:previewPath() id-" << _unit->getId();

	if (_path.empty() == false						// path is valid.
		&& (_previewed == false || clear == true))	// either create a preview or discard the old one.
	{
		Tile* tile;
		if ((_previewed = !clear) == false)
		{
			for (size_t
					i = 0u;
					i != _battleSave->getMapSizeXYZ();
					++i)
			{
				tile = _battleSave->getTiles()[i];
				tile->setPreviewColor(0);
				tile->setPreviewDir(-1);
				tile->setPreviewTu(-1);
			}
		}
		else
		{
			const int
				unitSize (_unit->getArmor()->getSize()),
				agility  (_unit->getArmor()->getAgility());
			int
				unitTu (_unit->getTu()),
				unitEn (_unit->getEnergy()),
				tuCost,			// cost per tile
				tuTally (0),	// only for soldiers reserving TUs
				energyLimit,
				dir;
			bool
				hathStood (false),
				gravLift,
				reserveOk,
				fall;
			Uint8 color;

			Position
				posStart (_unit->getPosition()),
				posStop;

//			bool switchBack;
//			if (_battleSave->getBattleGame()->getReservedAction() == BA_NONE)
//			{
//				switchBack = true;
//				_battleSave->getBattleGame()->setReservedAction(BA_SNAPSHOT);
//			}
//			else switchBack = false;

			Tile* tileAbove;
			for (std::vector<int>::const_reverse_iterator
					rit = _path.rbegin();
					rit != _path.rend();
					++rit)
			{
				dir = *rit;
				//Log(LOG_INFO) << ". dir= " << dir;
				tuCost = getTuCostPf( // gets tu-cost but also gets the destination position.
								posStart,
								dir,
								&posStop);
				//Log(LOG_INFO) << ". tuCost= " << tuCost;

				energyLimit = unitEn;

				fall = _mType != MT_FLY
					&& isUnitFloored(
								_battleSave->getTile(posStart),
								unitSize) == false;
				//Log(LOG_INFO) << ". fall= " << (int)fall;
				if (fall == false)
				{
					gravLift = dir >= DIR_UP
							&& _battleSave->getTile(posStart)->getMapData(O_FLOOR) != nullptr
							&& _battleSave->getTile(posStart)->getMapData(O_FLOOR)->isGravLift() == true;
					if (gravLift == false)
					{
						if (hathStood == false && _unit->isKneeled() == true)
						{
							hathStood = true;
							tuTally += TU_STAND;
							unitTu -= TU_STAND;
							unitEn -= std::max(0, EN_STAND - agility);
						}

						if (_pathAction->dash == true)
						{
							unitEn -= (((tuCost -= _doorCost) * 3) >> 1u);
							tuCost = ((tuCost * 3) >> 2u) + _doorCost;
						}
						else
							unitEn -= tuCost - _doorCost;

						unitEn += agility;

						if (unitEn > energyLimit)
							unitEn = energyLimit;
					}

					unitTu -= tuCost;
				}

				tuTally += tuCost;
				reserveOk = _battleSave->getBattleGame()->checkReservedTu(_unit, tuTally);

				posStart = posStop;
				for (int
						x = unitSize - 1;
						x != -1;
						--x)
				{
					for (int
							y = unitSize - 1;
							y != -1;
							--y)
					{
						tile = _battleSave->getTile(posStart + Position(x,y,0));
						//Log(LOG_INFO) << ". pos " << (posStart + Position(x,y,0));

						if (rit == _path.rend() - 1)
							tile->setPreviewDir(10);
						else
							tile->setPreviewDir(*(rit + 1)); // next dir

						if (unitSize == 1 || (x == 1 && y == 1))
							tile->setPreviewTu(unitTu);

						if ((tileAbove = _battleSave->getTile(posStart + Position(x,y,1))) != nullptr
							&& tileAbove->getPreviewDir() == 0
							&& tuCost == 0
							&& _mType != MT_FLY) // unit fell down
						{
							//Log(LOG_INFO) << ". unit fell down, retroactively set tileAbove's direction " << (posStart + Position(x,y,1));
							tileAbove->setPreviewDir(DIR_DOWN);	// retroactively set tileAbove's direction
						}

						if (unitTu > -1 && unitEn > -1)
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
//			if (switchBack == true) _battleSave->getBattleGame()->setReservedAction(BA_NONE);
		}
		return true;
	}
	return false;
}

/**
 * Clears the tiles used for path-preview.
 * @return, true if preview is cleared
 *			false if there is no preview
 */
bool Pathfinding::clearPreview()
{
	if (_previewed == true)
	{
		previewPath(true);
		return true;
	}
	return false;
}

/**
 * Gets the path-preview setting.
 * @return, true if path is previewed
 */
bool Pathfinding::isPathPreviewed() const
{
	return _previewed;
}

/**
 * Checks if a unit can fall down from a specified Tile.
 * @param tile		- pointer to a tile
 * @param unitSize	- size of the unit
 * @return, true if unit on @a tile has at least one quadrant on a solidly floored tile
 */
bool Pathfinding::isUnitFloored( // private
		const Tile* const tile,
		int unitSize) const
{
	const Tile* tileTest;
	const Position& pos (tile->getPosition());
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
			tileTest = _battleSave->getTile(pos + Position(x,y,0));
			if (tileTest->isFloored(tileTest->getTileBelow(_battleSave)) == true)
				return true;
		}
	}
	return false;
}

/**
 * Checks if vertical movement is valid.
 * @note Checks if there is a gravlift or the unit can fly and there are no
 * obstructions.
 * @param posStart	- reference to a start position
 * @param dir		- up or down
// * @param launch	- true if pathing a waypoint-launcher/guided missile (default false)
 * @return, UpDownCheck (Pathfinding.h)
 */
UpDownCheck Pathfinding::validateUpDown(
		const Position& posStart,
		const int dir) const
//		const bool launch)	// <- not so sure this is needed.
{							// ie, AI Blaster Launches have found holes in floor just fine so far.
	Position posStop;
	directionToVector(dir, &posStop);
	posStop += posStart;

	const Tile
		* const startTile (_battleSave->getTile(posStart)),
		* const stopTile  (_battleSave->getTile(posStop));

	if (stopTile == nullptr) return FLY_BLOCKED;


	if (stopTile->getMapData(O_CONTENT) != nullptr)	// NOTE: This is effectively an early-out for up/down movement; getTuCostPf()
	{												// will also prevent movement if TU-cost(O_CONTENT) is more than the unit has.
		switch (stopTile->getMapData(O_CONTENT)->getBigwall())
		{
			case BIGWALL_NONE:
				if (stopTile->getMapData(O_CONTENT)->getTuCostPart(_mType) != 255)
					break;
				// no break;

			case BIGWALL_BLOCK:
			case BIGWALL_NESW:
			case BIGWALL_NWSE:
				return FLY_BLOCKED;
		}
	}

	if (   startTile->getMapData(O_FLOOR) != nullptr
		&& startTile->getMapData(O_FLOOR)->isGravLift()
		&& stopTile ->getMapData(O_FLOOR) != nullptr
		&& stopTile ->getMapData(O_FLOOR)->isGravLift())
	{
		return FLY_GRAVLIFT;
	}

	if (_mType == MT_FLY
		|| (dir == DIR_DOWN && _alt == true))
	{
		if ((dir == DIR_UP
				&& stopTile->isFloored(startTile) == false)
			|| (dir == DIR_DOWN
				&& startTile->isFloored(startTile->getTileBelow(_battleSave)) == false))
		{
//			if (launch == true)
//			{
//				if (   (dir == DIR_UP   && stopTile-> getMapData(O_FLOOR)->getLoftId(0) != 0)
//					|| (dir == DIR_DOWN && startTile->getMapData(O_FLOOR)->getLoftId(0) != 0))
//				{
//					return FLY_BLOCKED;
//				}
//			}
			return FLY_GOOD;
		}
		return FLY_BLOCKED;
	}
	return FLY_CANT;
}

/**
 * Checks whether a modifier key was used to enable strafing or running.
 * @return, true if CTRL was used
 */
bool Pathfinding::isModCtrl() const
{
	return _ctrl;
}

/**
 * Checks whether a modifier key was used to enable forced walking.
 * @return, true if ALT was used
 */
bool Pathfinding::isModAlt() const
{
	return _alt;
}

/**
 * Gets the zPath-modifier setting.
 * @return, true if key-Z was used
 */
bool Pathfinding::isZPath() const
{
	return _zPath;
}

/**
 * Gets the current movement-type.
 * @return, the currently pathing unit's MoveType (MapData.h)
 */
MoveType Pathfinding::getMoveTypePf() const
{
	return _mType;
}

/**
 * Tracks TU-cost if opening a door.
 * @note Used to conform TU-costs in UnitWalkBState.
 * @return, TU-cost for opening a door
 */
int Pathfinding::getDoorCost() const
{
	return _doorCost;
}

/**
 * Checks whether a path is ready and returns the first direction.
 * @return, direction where the unit needs to go next
 *			-1 if it's the end of the path or there is no valid path
 */
int Pathfinding::getStartDirection() const
{
	if (_path.empty() == false)
		return _path.back();

	return -1;
}

/**
 * Dequeues the next path-direction and returns that direction.
 * @return, direction where the unit needs to go next, -1 if it's the end of the path
 */
int Pathfinding::dequeuePath()
{
	if (_path.empty() == true)
		return -1;

	const int last_element (_path.back());
	_path.pop_back();

	return last_element;
}

/**
 * Gets a read-only reference to the current path.
 * @return, reference to a vector of directions
 */
const std::vector<int>& Pathfinding::getPath() const
{
	return _path;
}

/**
 * Gets a copy of the current path.
 * @return, copy of the path
 */
std::vector<int> Pathfinding::copyPath() const
{
	return _path;
}

/**
 * Converts direction to a unit-vector.
 * @note Direction starts north = 0 and goes clockwise.
 * @param dir		- source direction
 * @param posVect	- pointer to a Position
 */
void Pathfinding::directionToVector( // static.
		int dir,
		Position* const posVect)
{
	static const int
		x[10u] { 0, 1, 1, 1, 0,-1,-1,-1, 0, 0},
		y[10u] {-1,-1, 0, 1, 1, 1, 0,-1, 0, 0},
		z[10u] { 0, 0, 0, 0, 0, 0, 0, 0, 1,-1};

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
		x[8u] { 0, 1, 1, 1, 0,-1,-1,-1},
		y[8u] {-1,-1, 0, 1, 1, 1, 0,-1};

	for (size_t
			i = 0u;
			i != 8u;
			++i)
	{
		if (   x[i] == posVect.x
			&& y[i] == posVect.y)
		{
			dir = static_cast<int>(i);
			return;
		}
	}
	dir = -1;
}

}
