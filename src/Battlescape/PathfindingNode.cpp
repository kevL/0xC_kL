/*
 * Copyright 2010-2019 OpenXcom Developers.
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
 * @param pos - reference to a Position on the battlefield
 */
PathfindingNode::PathfindingNode(const Position& pos)
	:
		_pos(pos),
		_visited(false),
		_tuTill(0.f),
		_tuLeft(0.f),
		_nodePrior(nullptr),
		_dirPrior(0),
		_openSetNode(nullptr)
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
	_visited = false;
	_openSetNode = nullptr;
}

/**
 * Gets this Node's TU-cost.
 * @param missile - true if this is a missile (default false)
 * @return, TU-cost
 */
int PathfindingNode::getTuCostTill(bool missile) const
{
	if (missile == true)
		return 0;

	return static_cast<int>(_tuTill);
}

/**
 * Connects this Node.
 * @note This will connect the Node to the previous Node along the path to its
 * @a posTarget and update Pathfinding information.
 * @param tuTill	- the TU-cost of the path so far
 * @param nodePrior	- pointer to the previous node along the path
 * @param dirPrior	- the direction to this node FROM the previous node
 * @param posTarget	- reference to the target-position to set the '_tuDestination' cost
 */
void PathfindingNode::linkNode(
		int tuTill,
		PathfindingNode* const nodePrior,
		int dirPrior,
		const Position& posTarget)
{
	_tuTill = static_cast<float>(tuTill);
	_nodePrior = nodePrior;
	_dirPrior = dirPrior;

	if (_openSetNode == nullptr) // otherwise this has been done already
	{
		Position pos (posTarget - _pos);
		pos *= pos;
		_tuLeft = std::sqrt(static_cast<float>(pos.x + pos.y + pos.z)) * 4.f;
	}
}

/**
 * Connects this Node.
 * @note This will connect the Node to the previous Node along the path.
 * @param tuReached	- the TU-cost of the path so far
 * @param nodePrior	- pointer to the previous node along the path
 * @param dirPrior	- the direction to this node FROM the previous node
 */
void PathfindingNode::linkNode(
		int tuTill,
		PathfindingNode* const nodePrior,
		int dirPrior)
{
	_tuTill = static_cast<float>(tuTill);
	_nodePrior = nodePrior;
	_dirPrior = dirPrior;
	_tuLeft = 0.f;
}

}
