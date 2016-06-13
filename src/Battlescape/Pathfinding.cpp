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
		_tuCostTotal(0),
		_ctrl(false),
		_alt(false),
		_zPath(false),
		_mType(MT_WALK),
		_doorCost(0)
{
	//Log(LOG_INFO) << "Create Pathfinding";
	_nodes.reserve(_battleSave->getMapSizeXYZ()); // reserve one node per tactical tile.

	Position pos;
	for (size_t // initialize one node per tile.
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
		_mType = MT_WALK;
}

/**
 * Sets the movement type for the path.
 */
void Pathfinding::setMoveType() // private.
{
	if ((_mType = _unit->getMoveTypeUnit()) == MT_FLY
		&& _alt == true // this forces Soldiers in flightsuits to walk on (or fall to) the ground.
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
		_alt  = (SDL_GetModState() & KMOD_ALT)  != 0;

		const Uint8* const keystate (SDL_GetKeyState(nullptr));
		if (keystate[SDLK_z] != 0)
			_zPath = true;
		else
			_zPath = false;
	}
}

/**
 * Aborts the current path - clears the path vector.
 */
void Pathfinding::abortPath()
{
	_path.clear();
	_tuCostTotal = 0;

	setInputModifiers();
}

/**
 * Calculates the shortest path; tries bresenham then A* algorithms.
 * @note 'launchTarget' is required only when called by AlienBAIState::pathWaypoints().
 * @param unit				- pointer to a BattleUnit
 * @param posStop			- destination Position
 * @param maxTuCost			- maximum time units this path can cost (default TU_INFINITE)
 * @param launchTarget		- pointer to a targeted BattleUnit, used only by
 *							  AlienBAIState::pathWaypoints() (default nullptr)
 * @param strafeRejected	- true if path needs to be recalculated w/out strafe (default false)
 */
void Pathfinding::calculatePath(
		const BattleUnit* const unit, // -> should not need 'unit' here anymore; done in setPathingUnit() unless FACTION_PLAYER ...
		Position posStop,
		int maxTuCost,
		const BattleUnit* const launchTarget,
		bool strafeRejected)
{
	//Log(LOG_INFO) << "Pathfinding::calculatePath()";
	// i'm DONE with these out of bounds errors.
	// kL_note: I really don't care what you're "DONE" with ..... if you're going
	// to cry like a babby, at least make it humorous -- like
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

	if (launchTarget != nullptr)
	{
		_mType = MT_FLY;
		strafeRejected = true;
	}
	else
		setMoveType(); // redundant in some cases ...

	if (unit->getFaction() != FACTION_PLAYER)
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
			if ((tileStop = _battleSave->getTile(posStop + Position(x,y,0))) != nullptr)
			{
				if (isBlocked(
							tileStop,
							O_FLOOR,
							launchTarget) == true
					|| isBlocked(
							tileStop,
							O_OBJECT,
							launchTarget) == true
					|| tileStop->getTuCostTile(O_OBJECT, _mType) == FAIL)
				{
					return;
				}
			}
			else
				return; // <- probly also done again below_ Pertains only to large units.
		}
	}

	static Position posStop_cache; // for keeping things straight if strafeRejected happens.
	posStop_cache = posStop;

	tileStop = _battleSave->getTile(posStop);
	while (tileStop->getTerrainLevel() == -24
		&& posStop.z != _battleSave->getMapSizeZ())
	{
		++posStop.z;
		tileStop = _battleSave->getTile(posStop);
	}

	if (_mType != MT_FLY)
	{
		//Log(LOG_INFO) << ". drop tileStop " << tileStop->getPosition();
		while (canFallDown(tileStop, unitSize))
		{
			--posStop.z;
			tileStop = _battleSave->getTile(posStop);

			//Log(LOG_INFO) << ". . drop to " << tileStop->getPosition();
		}
	}


	if (isBlocked( // TODO: Check all quadrants. See above^.
			tileStop,
			O_FLOOR,
			launchTarget) == false
		&& isBlocked(
				tileStop,
				O_OBJECT,
				launchTarget) == false)
	{
		if (unitSize == 2)
		{
			const Tile* tileTest;
			const BattleUnit* unitTest;

			static const int dir[3u] {4,2,3};

			size_t i (0);
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
						if (isBlockedPath(
										tileStop,
										dir[i],
										unit) == true
							&& isBlockedPath(
										tileStop,
										dir[i],
										launchTarget) == true)
						{
							return;
						}

						tileTest = _battleSave->getTile(posStop + Position(x,y,0));
						if (x != 0 && y != 0
							&& ((tileTest->getMapData(O_NORTHWALL) != nullptr
									&& tileTest->getMapData(O_NORTHWALL)->isHingeDoor() == true)
								|| (tileTest->getMapData(O_WESTWALL) != nullptr
									&& tileTest->getMapData(O_WESTWALL)->isHingeDoor() == true)))
						{
							return;
						}

						if ((unitTest = tileTest->getTileUnit()) != nullptr
							&& unitTest != unit
							&& unitTest != launchTarget
							&& unitTest->getUnitVisible() == true)
						{
							return;
						}

						++i;
					}
				}
			}
		}


		const Position posStart (unit->getPosition());

		const bool isMech (unit->getUnitRules() != nullptr
						&& unit->getUnitRules()->isMechanical());

		_pathAction->strafe =
		_strafe = strafeRejected == false
			   && Options::battleStrafe == true
			   && (    (_ctrl == true && isMech == false)
					|| (_alt == true  && isMech == true))
			   && (std::abs(
						(_battleSave->getTile(posStop)->getTerrainLevel()  - posStop.z  * 24)
					  - (_battleSave->getTile(posStart)->getTerrainLevel() - posStart.z * 24)) < 9)
			   && std::abs(posStop.x - posStart.x) < 2
			   && std::abs(posStop.y - posStart.y) < 2;

//		const bool sneak (Options::sneakyAI == true
//					   && unit->getFaction() == FACTION_HOSTILE);

		if (posStart.z == posStop.z
			&& bresenhamPath(
						posStart,
						posStop,
						launchTarget) == true)
//						sneak) == true)
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
						launchTarget,
						maxTuCost) == false)
//						sneak) == false)
			{
				abortPath();
			}
		}

		if (_path.empty() == false)
		{
			if (_path.size() > 2u && _strafe == true)
			{
				calculatePath( // iterate this function ONCE ->
							unit,
							posStop_cache,
							maxTuCost,
							nullptr,
							true); // <- sets '_strafe' FALSE so loop never gets back in here.
			}
			else if (Options::battleStrafe == true
				&& _ctrl == true
				&& unit->getGeoscapeSoldier() != nullptr
				&& (_strafe == false
					|| (_path.size() == 1u
						&& unit->getUnitDirection() == _path.front())))
			{
				_strafe =
				_pathAction->strafe = false;
				_pathAction->dash = true;
				if (_pathAction->actor != nullptr)
					_pathAction->actor->setDashing();
			}
			else
			{
				_pathAction->dash = false;
				if (_pathAction->actor != nullptr)
					_pathAction->actor->setDashing(false);
			}
		}
	}

	// TEST:
//	for (std::vector<int>::const_reverse_iterator
//			rit = _path.rbegin();
//			rit != _path.rend();
//			++rit)
//	{
//		Log(LOG_INFO) << ". dir= " << *rit;
//	}
}

/**
 * Calculates the shortest path using Brensenham path algorithm.
 * @note This only works in the x/y-plane.
 * @param origin		- reference to the Position to start from
 * @param target		- reference to the Position to end at
 * @param launchTarget	- pointer to targeted BattleUnit
// * @param sneak		- true if unit is sneaking
 * @return, true if a path is found
 */
bool Pathfinding::bresenhamPath( // private.
		const Position& origin,
		const Position& target,
		const BattleUnit* const launchTarget)
//		bool sneak)
{
	return false; // TEST.


	//Log(LOG_INFO) << "Pathfinding::bresenhamPath()";
	static const size_t DIR_TOTAL (8u);
	static const int
		stock_xd[DIR_TOTAL]	{ 0, 1, 1, 1, 0,-1,-1,-1}, // stock values
		stock_yd[DIR_TOTAL]	{-1,-1, 0, 1, 1, 1, 0,-1},
		alt_xd[DIR_TOTAL]	{ 0,-1,-1,-1, 0, 1, 1, 1}, // alt values
		alt_yd[DIR_TOTAL]	{ 1, 1, 0,-1,-1,-1, 0, 1};

	int
		xd[DIR_TOTAL],
		yd[DIR_TOTAL];
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

		tuCostLast (-1);

	size_t dir;

	Position
		posStart (origin),
		posStop,
		posStopTest;

//	_tuCostTotal = 0;

	// start and end points
	x0 = origin.x; x1 = target.x;
	y0 = origin.y; y1 = target.y;
	z0 = origin.z; z1 = target.z;

	// 'steep' x/y Line, make longest delta x-plane
	swap_xy = std::abs(y1 - y0) > std::abs(x1 - x0);
	if (swap_xy)
	{
		std::swap(x0,y0);
		std::swap(x1,y1);
	}

	// do same for x/z
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
	drift_xy = delta_x >> 1u;
	drift_xz = delta_x >> 1u;

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
			posStopTest =
			posStop = Position(cx,cy,cz);

			// get direction
			for (
					dir = 0;
					dir != DIR_TOTAL;
					++dir)
			{
				if (   xd[dir] == cx - posStart.x
					&& yd[dir] == cy - posStart.y)
				{
					break;
				}
			}

			const int tuCost (getTuCostPf(
										posStart,
										static_cast<int>(dir),
										&posStop,
										launchTarget));
			//Log(LOG_INFO) << ". TU Cost = " << tuCost;

//			if (sneak == true && _battleSave->getTile(posStop)->getTileVisible())
//				return false;

			// delete the following <- wtf does that stand for. Apart from the fact that this is an extremely kludgy approximation of wall-costs:
			const bool isDiagonal ((dir & 1u) == 1u);
			const int
				tuCostDiagonalLast (tuCostLast + (tuCostLast >> 1u)),
				tuCostDiagonal (tuCost + (tuCost >> 1u));

			if (posStopTest == posStop
				&& tuCost < FAIL
				&& (tuCost == tuCostLast
					|| (isDiagonal == true  && tuCost == tuCostDiagonalLast)
					|| (isDiagonal == false && tuCostLast == tuCostDiagonal)
					|| tuCostLast == -1)
				&& isBlockedPath(
							_battleSave->getTile(posStart),
							static_cast<int>(dir),
							launchTarget) == false)
			{
				_path.push_back(static_cast<int>(dir));
				//Log(LOG_INFO) << ". " << dir;
			}
			else
				return false;

			if (tuCost != FAIL && launchTarget == nullptr)
			{
				tuCostLast = tuCost;
				_tuCostTotal += tuCost;
			}

			posStart = Position(cx,cy,cz);
		}

		// update progress in other planes
		drift_xy = drift_xy - delta_y;
		drift_xz = drift_xz - delta_z;

		// step in y-plane
		if (drift_xy < 0)
		{
			y = y + step_y;
			drift_xy = drift_xy + delta_x;
		}

		// step in z-plane
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
 * path information is set only if a valid path is found. See also findReachable().
 * @param posOrigin		- reference to the position to start from
 * @param posTarget		- reference to the position to end at
 * @param launchTarget	- pointer to targeted BattleUnit
 * @param maxTuCost		- maximum time units this path can cost
// * @param sneak		- true if the unit is sneaking
 * @return, true if a path is found
 */
bool Pathfinding::aStarPath( // private.
		const Position& posOrigin,
		const Position& posTarget,
		const BattleUnit* const launchTarget,
		int maxTuCost)
//		bool sneak)
{
	//Log(LOG_INFO) << "";
	//Log(LOG_INFO) << "Pathfinding::aStarPath()";
	for (std::vector<PathfindingNode>::iterator
			i = _nodes.begin();
			i != _nodes.end();
			++i)
	{
		i->resetNode();
	}

	PathfindingNode
		* nodeStart (getNode(posOrigin)),
		* nodeStop;

	nodeStart->linkNode(0, nullptr, 0, posTarget);

	PathfindingOpenSet nodes;
	nodes.addNode(nodeStart);

	Position posStop;
	int tuCost;

	while (nodes.isNodeSetEmpty() == false)
	{
		nodeStart = nodes.getNode();
		const Position& posStart (nodeStart->getPosition());
		//Log(LOG_INFO) << ". posStart " << (nodeStart->getPosition());
		nodeStart->setChecked();

		if (posStart == posTarget)
		{
			_path.clear();

			while (nodeStart->getPrevNode() != nullptr)
			{
				_path.push_back(nodeStart->getPrevDir());
				//Log(LOG_INFO) << ". . " << nodeStart->getPrevDir();
				nodeStart = nodeStart->getPrevNode();
			}

			if (_strafe == true
				&& _unit->getUnitRules() != nullptr
				&& _unit->getUnitRules()->isMechanical() == true)
			{
				const int delta (std::abs((_path.back() + 4) % 8 - _unit->getUnitDirection())); // no u.
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
			tuCost = getTuCostPf(
							posStart,
							dir,
							&posStop,
							launchTarget,
							false);
			//Log(LOG_INFO) << ". dir= " << dir << " tuCost=" << tuCost;
			if (tuCost < FAIL)
			{
//				if (sneak == true && _battleSave->getTile(posStop)->getTileVisible() == true)
//					tuCost *= 2;

				nodeStop = getNode(posStop);
				if (nodeStop->getChecked() == false)
				{
					_tuCostTotal = tuCost + nodeStart->getTuCostNode(launchTarget != nullptr);
					//Log(LOG_INFO) << ". tuCostTotal= " << _tuCostTotal;

					if ((nodeStop->inOpenSet() == false
							|| nodeStop->getTuCostNode(launchTarget != nullptr) > _tuCostTotal)
						&& _tuCostTotal <= maxTuCost)
					{
						nodeStop->linkNode(
										_tuCostTotal,
										nodeStart,
										dir,
										posTarget);
						nodes.addNode(nodeStop);
					}
				}
			}
		}
	}
	return false;
}

/**
 * Locates all tiles reachable to @a unit with a TU cost no more than @a maxTuCost.
 * @note Uses Dijkstra's algorithm.
 * @sa aStarPath().
 * @param unit		- pointer to a BattleUnit
 * @param maxTuCost	- the maximum cost of the path to each tile
 * @return, vector of reachable tile-indices sorted in ascending order of cost;
 *			the first tile is the start-location itself
 */
std::vector<size_t> Pathfinding::findReachable(
		const BattleUnit* const unit,
		int maxTuCost)
{
	for (std::vector<PathfindingNode>::iterator
			i = _nodes.begin();
			i != _nodes.end();
			++i)
	{
		i->resetNode();
	}

	PathfindingNode
		* nodeStart (getNode(unit->getPosition())),
		* nodeStop;

	nodeStart->linkNode(0, nullptr, 0);

	PathfindingOpenSet nodes;
	nodes.addNode(nodeStart);

	std::vector<PathfindingNode*> set; // note these are not route-nodes perse: *every Tile* is a PathfindingNode.

	Position posStop;
	int
		tuCost,
		tuCostTotal;

	while (nodes.isNodeSetEmpty() == false)
	{
		nodeStart = nodes.getNode();
		const Position& posStart (nodeStart->getPosition());

		for (int
				dir = 0;
				dir != 10;
				++dir)
		{
			tuCost = getTuCostPf(
							posStart,
							dir,
							&posStop);

			if (tuCost < FAIL)
			{
				tuCostTotal = nodeStart->getTuCostNode() + tuCost;
				if (   tuCostTotal <= maxTuCost
					&& tuCostTotal <= unit->getEnergy())
				{
					nodeStop = getNode(posStop);
					if (nodeStop->getChecked() == false)
					{
						if (nodeStop->inOpenSet() == false
							|| nodeStop->getTuCostNode() > tuCostTotal)
						{
							nodeStop->linkNode(
											tuCostTotal,
											nodeStart,
											dir);
							nodes.addNode(nodeStop);
						}
					}
				}
			}
		}

		nodeStart->setChecked();
		set.push_back(nodeStart);
	}

	std::sort(
			set.begin(),
			set.end(),
			MinNodeCosts());

	std::vector<size_t> tileIds;
	tileIds.reserve(set.size());
	for (std::vector<PathfindingNode*>::const_iterator
			i = set.begin();
			i != set.end();
			++i)
	{
		//Log(LOG_INFO) << "pf: " << _battleSave->getTileIndex((*i)->getPosition());
		tileIds.push_back(_battleSave->getTileIndex((*i)->getPosition()));
	}
	return tileIds;
}

/**
 * Gets the Node on a given position on the map.
 * @param pos - reference the Position
 * @return, pointer to PathfindingNode
 */
PathfindingNode* Pathfinding::getNode(const Position& pos) // private.
{
	return &_nodes[_battleSave->getTileIndex(pos)];
}

/**
 * Gets the TU cost to move from 1 tile to another - ONE STEP ONLY!
 * @note But also updates the destination Position because it is possible that
 * the unit uses stairs or falls while moving.
 * @param posStart		- reference to the start-position
 * @param dir			- direction of movement
 * @param posStop		- pointer to destination-position
 * @param launchTarget	- pointer to targeted BattleUnit (default nullptr)
 * @param bresenh		- true if calc'd by Breshenham pathing (default true)
 * @return, TU cost or 255 if movement is impossible
 */
int Pathfinding::getTuCostPf(
		const Position& posStart,
		int dir,
		Position* const posStop,
		const BattleUnit* const launchTarget,
		bool bresenh)
{
	//Log(LOG_INFO) << "";
	//Log(LOG_INFO) << "Pathfinding::getTuCostPf()";// << _unit->getId();
	directionToVector(dir, posStop);
	*posStop += posStart;

	bool
		fall   (false),
		stairs (false);

	int
		partsGoingUp	(0),
		partsGoingDown	(0),
		partsFalling	(0),
		partsOnAir		(0),

		cost,
		costTotal (0);

	_doorCost = 0;

	const Tile
		* tileStart,
		* tileStartBelow,
		* tileStop,
		* tileStopBelow,
		* tileStopAbove;

	Position posOffset;

	const int
		unitSize (_unit->getArmor()->getSize()),
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
// first, CHECK FOR BLOCKAGE ->> then calc TU cost after.
			posOffset = Position(x,y,0);

			if (   (tileStart = _battleSave->getTile(posStart + posOffset)) == nullptr
				|| (tileStop  = _battleSave->getTile(*posStop + posOffset)) == nullptr)
			{
				return FAIL;
			}

			if (_mType != MT_FLY)
			{
				if (x == 0 && y == 0
					&& canFallDown(
								tileStart,
								unitSize) == true)
				{
					if (dir != DIR_DOWN) return FAIL;

					fall = true;
					++partsOnAir;
				}
				else
				{
					tileStartBelow = _battleSave->getTile(posStart + posOffset + Position(0,0,-1));
					if (tileStart->hasNoFloor(tileStartBelow) == true
						&& ++partsOnAir == quadrants)
					{
						if (dir != DIR_DOWN) return FAIL;

						fall = true;
					}
				}
			}

			if (x != 0 && y != 0 // don't let large units phase through doors
				&& ((tileStop->getMapData(O_NORTHWALL) != nullptr
						&& tileStop->getMapData(O_NORTHWALL)->isHingeDoor() == true)
					|| (tileStop->getMapData(O_WESTWALL) != nullptr
						&& tileStop->getMapData(O_WESTWALL)->isHingeDoor() == true)))
			{
				return FAIL;
			}

			if (dir < DIR_UP && tileStart->getTerrainLevel() > -16
				&& (tileStart->getTerrainLevel() - tileStop->getTerrainLevel() > 8
					|| isBlockedPath(
								tileStart,
								dir,
								launchTarget) == true))
			{
				return FAIL;
			}


			Position posOffsetVertical (0,0,0); // this will later be used to re-init the start Tile

			tileStopBelow = _battleSave->getTile(*posStop + posOffset + Position(0,0,-1)),
			tileStopAbove = _battleSave->getTile(*posStop + posOffset + Position(0,0, 1));

			if (dir < DIR_UP
				&& stairs == false
				&& tileStart->getTerrainLevel() < -15
				&& tileStopAbove->hasNoFloor(tileStop) == false)
			{
				if (++partsGoingUp >= unitSize)
				{
					stairs = true;
					++posOffsetVertical.z;
					++posStop->z;
					tileStop = _battleSave->getTile(*posStop + posOffset);
				}
			}
			else if (dir < DIR_UP
				&& fall == false
				&& _mType != MT_FLY
				&& canFallDown(tileStop) == true
				&& tileStopBelow != nullptr
				&& tileStopBelow->getTerrainLevel() < -11)
			{
				if (++partsGoingDown == quadrants)
				{
					fall = true;
					--posStop->z;
					tileStop = _battleSave->getTile(*posStop + posOffset);
				}
			}
			else if (_mType == MT_FLY
				&& launchTarget == nullptr // TODO: <-- Think about that.
				&& tileStopBelow != nullptr
				&& tileStopBelow->getTileUnit() != nullptr
				&& tileStopBelow->getTileUnit() != _unit
				&& tileStopBelow->getTileUnit()->getHeight(true) - tileStopBelow->getTerrainLevel() > UNIT_HEIGHT) // cf. UnitWalkBState::doStatusStand()
			{
				return FAIL;
			}

			if (tileStop == nullptr)
				return FAIL;

			cost = 0;

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
								return FAIL;

							case FLY_GRAVLIFT:
							case FLY_GOOD:
								cost = 8;

								if (tileStop->getMapData(O_OBJECT) != nullptr)
								{
									switch (tileStop->getMapData(O_OBJECT)->getBigwall())
									{
										case BIGWALL_NONE:
										case BIGWALL_BLOCK:
										case BIGWALL_NESW:
										case BIGWALL_NWSE:
											cost += tileStop->getTuCostTile(O_OBJECT, _mType);
											// TODO: Early-exit if cost>FAIL.
									}
								}
						}
					}
					break;

				default:
					if (posStop->z == tileStart->getPosition().z
						&& (tileStart->getTerrainLevel() - tileStop->getTerrainLevel() > 8
							|| isBlockedPath(
										tileStart,
										dir,
										launchTarget) == true))
					{
						return FAIL;
					}
			}

			if (_mType != MT_FLY
				&& dir != DIR_DOWN
				&& fall == false
				&& canFallDown(tileStart) == true
				&& ++partsFalling == quadrants)
			{
				dir = DIR_DOWN;
				fall = true;
				*posStop = posStart + Position(0,0,-1);
				tileStop = _battleSave->getTile(*posStop + posOffset);
			}

			tileStart = _battleSave->getTile(tileStart->getPosition() + posOffsetVertical);

			if (dir < DIR_UP && partsGoingUp != 0
				&& (tileStart->getTerrainLevel() - tileStop->getTerrainLevel() > 8
					|| isBlockedPath(
								tileStart,
								dir,
								launchTarget) == true))
			{
				return FAIL;
			}

			if (isBlocked(
						tileStop,
						O_FLOOR,
						launchTarget) == true
				|| isBlocked(
							tileStop,
							O_OBJECT,
							launchTarget) == true)
			{
				return FAIL;
			}
// CHECK FOR BLOCKAGE_end.

// Calculate TU costage ->
			if (dir < DIR_UP && fall == false)
			{
				if (tileStop->getMapData(O_FLOOR) != nullptr)
					cost = tileStop->getTuCostTile(O_FLOOR, _mType);
				else
					cost = 4;

				if (stairs == false && tileStop->getMapData(O_OBJECT) != nullptr)
					cost += tileStop->getTuCostTile(O_OBJECT, _mType);

				if ((dir & 1) == 1)
					cost += (cost + 1) >> 1u;

				// TODO: Early-exit if cost>FAIL.
//				if (cost >= FAIL) return FAIL; // quick outs ->
//				else if (launchTarget != nullptr) return 0; // <- provided walls have actually been blocked already, not based on TU > FAIL

//				if (posOffsetVertical.z > 0) ++cost;

				if (tileStart->getPosition().z == tileStop->getPosition().z)	// Do not charge wall-cost if moving up or down a level.
					cost += getWallTuCost(dir, tileStart, tileStop);			// NOTE: Custom mapblocks could flaunt that but it's unlikely.
			}

			if (tileStop->getFire() != 0)
			{
				if (_unit->getSpecialAbility() != SPECAB_BURN)
					cost += 2 + (dir & 1);

				if (_unit->avoidsFire() == true)
					cost += TU_FIRE_AVOID;	// cf. UnitWalkBState::doStatusStand() - this gets subtracted.
			}

			// Propose: if flying then no extra TU cost
			//Log(LOG_INFO) << ". pathSize = " << (int)_path.size();
			if (_strafe == true)
			{
				// Extra TU for strafe-moves ->	1 0 1
				//								2 ^ 2
				//								3 2 3
				int delta (std::abs((dir + 4) % 8 - _unit->getUnitDirection())); // no u.

				if (delta > 1 && delta < 7
					&& _unit->getUnitRules() != nullptr
					&& _unit->getUnitRules()->isMechanical() == true)
				{
					if (bresenh == true) // NOTE: A* happily denies an illegal tank-strafe completely.
					{
						_strafe = false; // illegal direction for tank-strafe.
						_pathAction->strafe = false;
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
				}
			}
			costTotal += cost;
		}
	}

	if (unitSize == 2) // only for Large units ->
	{
		// - check the path between part 0,0 and part 1,1 at destination position
		const Tile* const ulTile (_battleSave->getTile(*posStop));
		if (isBlockedPath(
					ulTile,
					3,
					launchTarget) == true)
		{
			return FAIL;
		}

		// - then check the path between part 1,0 and part 0,1 at destination position
		const Tile* const urTile (_battleSave->getTile(*posStop + Position(1,0,0)));
		if (isBlockedPath(
					urTile,
					5,
					launchTarget) == true)
		{
			return FAIL;
		}

		if (fall == false)
		{
			const Tile
				* const lrTile (_battleSave->getTile(*posStop + Position(1,1,0))),
				* const llTile (_battleSave->getTile(*posStop + Position(0,1,0)));
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
				return FAIL;
		}

		costTotal = (costTotal + quadrants - 1) / quadrants; // round up.
	} // largeUnits_end.

	//Log(LOG_INFO) << ". costTotal= " << costTotal;
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
//				tile = _battleSave->getTile(tileStart->getPosition() + Position(0,0,1));
//			else
//				tile = tileStart;

//			if (tileStart->getPosition().z == tileStop->getPosition().z)
			{
				tile = tileStart;
				if ((partCost = tile->getTuCostTile(O_NORTHWALL, _mType)) > 0)
				{
					tuTotal += partCost;

					if (tile->getMapData(O_NORTHWALL)->isDoor() == true)
						if (partCost > _doorCost) _doorCost = partCost; // don't let large unit parts reset _doorCost prematurely.
				}
			}
			break;

		case 1:
//			if (tileStart->getPosition().z == tileStop->getPosition().z)
			{
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

				if (walls != 0) // && _doorCost == 0
					tuTotal = ((tuTotal + (tuTotal >> 1u)) + walls - 1) / walls; // add 50% and round up.
			}
			break;

		case 2:
//			if (tileStart->getPosition().z > tileStop->getPosition().z)
//				tile = _battleSave->getTile(tileStop->getPosition() + Position(0,0,1));
//			else
//				tile = tileStop;

//			if (tileStart->getPosition().z == tileStop->getPosition().z)
			{
				tile = tileStop;
				if ((partCost = tile->getTuCostTile(O_WESTWALL, _mType)) > 0)
				{
					tuTotal += partCost;

					if (tile->getMapData(O_WESTWALL)->isDoor() == true)
						if (partCost > _doorCost) _doorCost = partCost;
				}
			}
			break;

		case 3:
//			if (tileStart->getPosition().z == tileStop->getPosition().z)
			{
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

				if (walls != 0)
					tuTotal = ((tuTotal + (tuTotal >> 1u)) + walls - 1) / walls;
			}
			break;

		case 4:
//			if (tileStart->getPosition().z > tileStop->getPosition().z)
//				tile = _battleSave->getTile(tileStop->getPosition() + Position(0,0,1));
//			else
//				tile = tileStop;

//			if (tileStart->getPosition().z == tileStop->getPosition().z)
			{
				tile = tileStop;
				if ((partCost = tile->getTuCostTile(O_NORTHWALL, _mType)) > 0)
				{
					tuTotal += partCost;

					if (tile->getMapData(O_NORTHWALL)->isDoor() == true)
						if (partCost > _doorCost) _doorCost = partCost;
				}
			}
			break;

		case 5:
//			if (tileStart->getPosition().z == tileStop->getPosition().z)
			{
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

				if (walls != 0)
					tuTotal = ((tuTotal + (tuTotal >> 1u)) + walls - 1) / walls;
			}
			break;

		case 6:
//			if (tileStart->getPosition().z < tileStop->getPosition().z)
//				tile = _battleSave->getTile(tileStart->getPosition() + Position(0,0,1));
//			else
//				tile = tileStart;

//			if (tileStart->getPosition().z == tileStop->getPosition().z)
			{
				tile = tileStart;
				if ((partCost = tile->getTuCostTile(O_WESTWALL, _mType)) > 0)
				{
					tuTotal += partCost;

					if (tile->getMapData(O_WESTWALL)->isDoor() == true)
						if (partCost > _doorCost) _doorCost = partCost;
				}
			}
			break;

		case 7:
//			if (tileStart->getPosition().z == tileStop->getPosition().z)
			{
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

				if (walls != 0)
					tuTotal = ((tuTotal + (tuTotal >> 1u)) + walls - 1) / walls;
			}
	}

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
bool Pathfinding::isBlockedPath(
		const Tile* const startTile,
		const int dir,
		const BattleUnit* const launchTarget) const
{
	//Log(LOG_INFO) << "Pathfinding::isBlockedPath()";
	const Position pos (startTile->getPosition());

	static const Position
		posNorth	(Position( 0,-1, 0)),
		posEast		(Position( 1, 0, 0)),
		posSouth	(Position( 0, 1, 0)),
		posWest		(Position(-1, 0, 0));


	switch (dir)
	{
		case 0:	// north
			//Log(LOG_INFO) << ". try North";
			if (isBlocked(
						startTile,
						O_NORTHWALL,
						launchTarget) == true)
			{
				return true;
			}
			break;

		case 1: // north-east
			//Log(LOG_INFO) << ". try NorthEast";
			if (isBlocked(
						startTile,
						O_NORTHWALL,
						launchTarget) == true
				|| isBlocked(
						_battleSave->getTile(pos + posEast),
						O_WESTWALL,
						launchTarget) == true
				|| isBlocked(
						_battleSave->getTile(pos + posEast),
						O_NORTHWALL,
						launchTarget) == true
				|| isBlocked(
						_battleSave->getTile(pos + posEast),
						O_OBJECT,
						launchTarget,
						BIGWALL_NESW) == true
				|| isBlocked(
						_battleSave->getTile(pos + posEast + posNorth),
						O_WESTWALL,
						launchTarget) == true
				|| isBlocked(
						_battleSave->getTile(pos + posNorth),
						O_OBJECT,
						launchTarget,
						BIGWALL_NESW) == true)
			{
				return true;
			}
			break;

		case 2: // east
			//Log(LOG_INFO) << ". try East";
			if (isBlocked(
						_battleSave->getTile(pos + posEast),
						O_WESTWALL,
						launchTarget) == true)
			{
				return true;
			}
			break;

		case 3: // south-east
			//Log(LOG_INFO) << ". try SouthEast";
			if (isBlocked(
						_battleSave->getTile(pos + posEast),
						O_WESTWALL,
						launchTarget) == true
				|| isBlocked(
						_battleSave->getTile(pos + posEast),
						O_OBJECT,
						launchTarget,
						BIGWALL_NWSE) == true
				|| isBlocked(
						_battleSave->getTile(pos + posEast + posSouth),
						O_NORTHWALL,
						launchTarget) == true
				|| isBlocked(
						_battleSave->getTile(pos + posEast + posSouth),
						O_WESTWALL,
						launchTarget) == true
				|| isBlocked(
						_battleSave->getTile(pos + posSouth),
						O_NORTHWALL,
						launchTarget) == true
				|| isBlocked(
						_battleSave->getTile(pos + posSouth),
						O_OBJECT,
						launchTarget,
						BIGWALL_NWSE) == true)
			{
				return true;
			}
			break;

		case 4: // south
			//Log(LOG_INFO) << ". try South";
			if (isBlocked(
						_battleSave->getTile(pos + posSouth),
						O_NORTHWALL,
						launchTarget) == true)
			{
				return true;
			}
			break;

		case 5: // south-west
			//Log(LOG_INFO) << ". try SouthWest";
			if (isBlocked(
						startTile,
						O_WESTWALL,
						launchTarget) == true
				|| isBlocked(
						_battleSave->getTile(pos + posSouth),
						O_WESTWALL,
						launchTarget) == true
				|| isBlocked(
						_battleSave->getTile(pos + posSouth),
						O_NORTHWALL,
						launchTarget) == true
				|| isBlocked(
						_battleSave->getTile(pos + posSouth),
						O_OBJECT,
						launchTarget,
						BIGWALL_NESW) == true
				|| isBlocked(
						_battleSave->getTile(pos + posSouth + posWest),
						O_NORTHWALL,
						launchTarget) == true
				|| isBlocked(
						_battleSave->getTile(pos + posWest),
						O_OBJECT,
						launchTarget,
						BIGWALL_NESW) == true)
			{
				return true;
			}
			break;

		case 6: // west
			//Log(LOG_INFO) << ". try West";
			if (isBlocked(
						startTile,
						O_WESTWALL,
						launchTarget) == true)
			{
				return true;
			}
			break;

		case 7: // north-west
			//Log(LOG_INFO) << ". try NorthWest";
			if (isBlocked(
						startTile,
						O_WESTWALL,
						launchTarget) == true
				|| isBlocked(
						startTile,
						O_NORTHWALL,
						launchTarget) == true
				|| isBlocked(
						_battleSave->getTile(pos + posWest),
						O_NORTHWALL,
						launchTarget) == true
				|| isBlocked(
						_battleSave->getTile(pos + posWest),
						O_OBJECT,
						launchTarget,
						BIGWALL_NWSE) == true
				|| isBlocked(
						_battleSave->getTile(pos + posNorth),
						O_WESTWALL,
						launchTarget) == true
				|| isBlocked(
						_battleSave->getTile(pos + posNorth),
						O_OBJECT,
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
bool Pathfinding::isBlocked( // private.
		const Tile* const tile,
		const MapDataType partType,
		const BattleUnit* const launchTarget,
		const BigwallType diagExclusion) const
{
	//Log(LOG_INFO) << "Pathfinding::isBlocked()";
	if (tile == nullptr) return true;

	const MapData* part;

	switch (partType)
	{
		case O_OBJECT:
			//Log(LOG_INFO) << ". part is Bigwall/object " << tile->getPosition();
			if ((part = tile->getMapData(O_OBJECT)) != nullptr)
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

			if ((part = tile->getMapData(O_OBJECT)) != nullptr)
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

			if ((part = tileWest->getMapData(O_OBJECT)) != nullptr)
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

			if ((part = tile->getMapData(O_OBJECT)) != nullptr)
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

			if ((part = tileNorth->getMapData(O_OBJECT)) != nullptr)
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
				if (blockUnit == _unit
					|| blockUnit == launchTarget
					|| blockUnit->isOut_t(OUT_STAT) == true)
				{
					return false;
				}

				if (launchTarget != nullptr // <- isAI
					&& launchTarget != blockUnit
					&& blockUnit->getFaction() == FACTION_HOSTILE)
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

						default:
						case FACTION_HOSTILE:
						case FACTION_NEUTRAL:
							if (std::find(
									_unit->getHostileUnitsThisTurn().begin(),
									_unit->getHostileUnitsThisTurn().end(),
									blockUnit) != _unit->getHostileUnitsThisTurn().end())
							{
								return true;
							}
					}
				}
			}
			else if (tile->hasNoFloor() == true	// This section is devoted to ensuring that large units
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

						if (blockUnit != launchTarget								// don't let any units fall on large units
							&& blockUnit->isOut_t(OUT_STAT) == false
							&& blockUnit->getArmor()->getSize() == 2)
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

	static const int TU_LARGEBLOCK (5); // stop large units from going through hedges and over fences

	const int partCost (tile->getTuCostTile(partType, _mType));
	if (partCost == FAIL
		|| (partCost > TU_LARGEBLOCK
			&& _unit != nullptr
			&& _unit->getArmor()->getSize() == 2))
	{
		//Log(LOG_INFO) << "isBlocked() EXIT true, partType = " << partType << " MT = " << (int)_mType;
		return true;
	}
	//Log(LOG_INFO) << "isBlocked() EXIT false, partType = " << partType << " MT = " << (int)_mType;
	return false;
}

/**
 * Marks Tiles with pathfinding-info for the path-preview.
 * @param discard - true removes preview (default false)
 * @return,	true if the preview is created or discarded even if non-previewed;
 *			false if there is no path or an existing preview remains static
 */
bool Pathfinding::previewPath(bool discard)
{
	//Log(LOG_INFO) << "Pathfinding::previewPath()";
	if (_path.empty() == true							// <- no current path at all
		|| (_previewed == true && discard == false))	// <- already previewed; won't change the preview (preview must be discarded before calling funct.)
	{
		return false;
	}

	Tile* tile;
	if ((_previewed = !discard) == false)
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
			agility (_unit->getArmor()->getAgility());
		int
			unitTu (_unit->getTimeUnits()),
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

//		bool switchBack;
//		if (_battleSave->getBattleGame()->getReservedAction() == BA_NONE)
//		{
//			switchBack = true;
//			_battleSave->getBattleGame()->setReservedAction(BA_SNAPSHOT);
//		}
//		else switchBack = false;

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
				&& canFallDown(
							_battleSave->getTile(posStart),
							unitSize);
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
//		if (switchBack == true)
//			_battleSave->getBattleGame()->setReservedAction(BA_NONE);
	}
	return true;
}

/**
 * Clears the tiles used for path-preview.
 * @return, true if preview gets removed
 */
bool Pathfinding::clearPreview()
{
	if (_previewed == true)	// something smells ...
	{
		previewPath(true);	// ... redundant here
		return true;
	}
	return false;			// Ie. is this function any different than "return previewPath(true)"
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
 * Checks if large units can fall down a level.
 * @note Wrapper for canFallDown(Tile*).
 * @param tile		- pointer to a tile
 * @param unitSize	- size of the unit
 * @return, true if unit on @a tile can fall to a lower level
 */
bool Pathfinding::canFallDown( // private
		const Tile* const tile,
		int unitSize) const
{
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
			if (canFallDown(_battleSave->getTile(tile->getPosition() + Position(x,y,0))) == false)
				return false;
		}
	}
	return true;
}

/**
 * Checks if a unit can fall down a level.
 * @param tile - pointer to a tile
 * @return, true if unit on @a tile can fall to a lower level
 */
bool Pathfinding::canFallDown(const Tile* const tile) const // private
{
	return tile->hasNoFloor(_battleSave->getTile(tile->getPosition() + Position(0,0,-1)));
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


	if (stopTile->getMapData(O_OBJECT) != nullptr)	// NOTE: This is effectively an early-out for up/down movement; getTuCostPf()
	{												// will also prevent movement if TU-cost(O_OBJECT) is more than the unit has.
		switch (stopTile->getMapData(O_OBJECT)->getBigwall())
		{
			case BIGWALL_NONE:
				if (stopTile->getMapData(O_OBJECT)->getTuCostPart(_mType) != 255)
					break;
				// 'else' no break;

			case BIGWALL_BLOCK:
			case BIGWALL_NESW:
			case BIGWALL_NWSE:
				return FLY_BLOCKED;
		}
	}

	if (startTile->getMapData(O_FLOOR) != nullptr
		&& startTile->getMapData(O_FLOOR)->isGravLift()
		&& stopTile->getMapData(O_FLOOR) != nullptr
		&& stopTile->getMapData(O_FLOOR)->isGravLift())
	{
		return FLY_GRAVLIFT;
	}

	if (_mType == MT_FLY
		|| (dir == DIR_DOWN && _alt == true))
	{
		if ((dir == DIR_UP
				&& stopTile->hasNoFloor(startTile))
			|| (dir == DIR_DOWN
				&& startTile->hasNoFloor(_battleSave->getTile(posStart + Position(0,0,-1)))))
		{
//			if (launch == true)
//			{
//				if ((dir == DIR_UP && stopTile->getMapData(O_FLOOR)->getLoftId(0) != 0)
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
 * @return, direction where the unit needs to go next, -1 if it's the end of the path
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
