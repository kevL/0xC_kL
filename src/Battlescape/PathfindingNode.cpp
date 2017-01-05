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

#include "PathfindingNode.h"

//#include <cmath> // std::sqrt()


namespace OpenXcom
{

/**
 * Sets up a PathfindingNode.
 * @param pos - Position on the battlefield
 */
PathfindingNode::PathfindingNode(const Position& pos)
	:
		_pos(pos),
		_checked(false),
		_tuCost(0),
		_tuGuess(0.f),
		_prevNode(nullptr),
		_prevDir(0),
		_openSetEntry(nullptr)
{}

/**
 * Deletes the PathfindingNode.
 */
PathfindingNode::~PathfindingNode()
{}

/**
 * Gets this Node's Position.
 * @return - reference to position
 */
const Position& PathfindingNode::getPosition() const
{
	return _pos;
}

/**
 * Resets this Node.
 */
void PathfindingNode::resetNode()
{
	_checked = false;
	_openSetEntry = nullptr;
}

/**
 * Gets the checked status of this Node.
 * @return, true if node has been checked
 */
bool PathfindingNode::getChecked() const
{
	return _checked;
}

/**
 * Gets this Node's TU cost.
 * @param missile - true if this is a missile (default false)
 * @return, TU cost
 */
int PathfindingNode::getTuCostNode(bool missile) const
{
	if (missile == false)
		return _tuCost;

	return 0;
}

/**
 * Gets this Node's previous Node.
 * @return, pointer to the previous node
 */
PathfindingNode* PathfindingNode::getPrevNode() const
{
	return _prevNode;
}

/**
 * Gets the previous walking direction for how a unit got to this Node.
 * @return, previous dir
 */
int PathfindingNode::getPrevDir() const
{
	return _prevDir;
}

/**
 * Connects this Node.
 * @note This will connect the Node to the previous Node along the path to its
 * @a target and update Pathfinding information.
 * @param tuCost	- the total cost of the path so far
 * @param prevNode	- pointer to the previous node along the path
 * @param prevDir	- the direction to this node FROM the previous node
 * @param target	- reference to the target-position (used to update the '_tuGuess' cost)
 */
void PathfindingNode::linkNode(
		int tuCost,
		PathfindingNode* const prevNode,
		int prevDir,
		const Position& target)
{
	_tuCost = tuCost;
	_prevNode = prevNode;
	_prevDir = prevDir;

	if (_openSetEntry == nullptr) // otherwise this has been done already
	{
		Position pos (target - _pos);
		pos *= pos;
		_tuGuess = std::sqrt(static_cast<float>(pos.x + pos.y + pos.z)) * 4.f;
	}
}

/**
 * Connects this Node.
 * @note This will connect the Node to the previous Node along the path.
 * @param tuCost	- the total cost of the path so far
 * @param prevNode	- pointer to the previous node along the path
 * @param prevDir	- the direction to this node FROM the previous node
 */
void PathfindingNode::linkNode(
		int tuCost,
		PathfindingNode* const prevNode,
		int prevDir)
{
	_tuCost = tuCost;
	_prevNode = prevNode;
	_prevDir = prevDir;
	_tuGuess = 0.f;
}

}
