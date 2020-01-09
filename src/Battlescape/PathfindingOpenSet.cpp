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

#include "PathfindingOpenSet.h"

//#include <cassert>

#include "PathfindingNode.h"


namespace OpenXcom
{

/**
 * Cleans up all the entries still in the OpenSet.
 */
PathfindingOpenSet::~PathfindingOpenSet()
{
	while (_frontier.empty() == false)
	{
		const OpenSetNode* const node (_frontier.top());
		_frontier.pop();
		delete node;
	}
}

/**
 * Gets the PathfindingNode with the cheapest TU-cost.
 * @note After this call the node is no longer in the openset.
 * @note It is an error to call this when the openset is empty.
 * @return, pointer to the PathfindingNode which had the least cost
 */
PathfindingNode* PathfindingOpenSet::processNodeTop()
{
//	assert(isOpenSetEmpty() == false);

	const OpenSetNode* nodeOs (_frontier.top());
	PathfindingNode* const nodePf (nodeOs->_node);
	_frontier.pop();
	delete nodeOs;
	nodePf->_openSetNode = nullptr;

	// non-eligible entries might be visible so clear them from the top of the queue.
	while (_frontier.empty() == false && _frontier.top()->_node == nullptr)
	{
		nodeOs = _frontier.top();
		_frontier.pop();
		delete nodeOs;
	}

	return nodePf;
}

/**
 * Adds a specified PathfindingNode to the openset.
 * @note If the node was already in the set the previous entry is discarded. It
 * is the caller's responsibility to never re-add a node with a higher cost.
 * @param nodePf - pointer to the PathfindingNode to add
 */
void PathfindingOpenSet::addNode(PathfindingNode* const nodePf)
{
	OpenSetNode* const nodeOs (new OpenSetNode);
	nodeOs->_node = nodePf;
	nodeOs->_tuTotal = nodePf->_tuTill + nodePf->_tuLeft;

	nodePf->_openSetNode = nodeOs;
	_frontier.push(nodeOs);
}

}
