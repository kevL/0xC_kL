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

#include "PathfindingOpenSet.h"

//#include <assert.h>

#include "PathfindingNode.h"


namespace OpenXcom
{

/**
 * Cleans up all the entries still in the OpenSet.
 */
PathfindingOpenSet::~PathfindingOpenSet()
{
	while (_queue.empty() == false)
	{
		const OpenSetEntry* const entry (_queue.top());
		_queue.pop();

		delete entry;
	}
}

/**
 * Discards entries that have come to the top of the queue.
 */
void PathfindingOpenSet::discard() // private.
{
	while (_queue.empty() == false && _queue.top()->_node == nullptr)
	{
		const OpenSetEntry* const entry (_queue.top());
		_queue.pop();

		delete entry;
	}
}

/**
 * Gets the PathfindingNode with the least cost.
 * @note After this call the node is no longer in the set. It is an error to
 * call this when the set is empty.
 * @return, pointer to the node which had the least cost
 */
PathfindingNode* PathfindingOpenSet::getNode()
{
	assert(isNodeSetEmpty() == false);

	const OpenSetEntry* const entry (_queue.top());
	PathfindingNode* const node (entry->_node);
	_queue.pop();

	delete entry;
	node->_openSetEntry = nullptr;

	discard(); // non-eligible entries might be visible.

	return node;
}

/**
 * Places a specified PathfindingNode in this OpenSet.
 * @note If the node was already in the set the previous entry is discarded. It
 * is the caller's responsibility to never re-add a node with a higher cost.
 * @param node - pointer to the node to add
 */
void PathfindingOpenSet::addNode(PathfindingNode* const node)
{
	OpenSetEntry* const entry (new OpenSetEntry);
	entry->_node = node;
	entry->_cost = node->getTuCostNode() + node->getTuGuess();

	if (node->_openSetEntry != nullptr)
		node->_openSetEntry->_node = nullptr;

	node->_openSetEntry = entry;
	_queue.push(entry);
}

}
