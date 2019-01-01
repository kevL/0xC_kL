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

#ifndef OPENXCOM_PATHFINDINGOPENSET_H
#define OPENXCOM_PATHFINDINGOPENSET_H

#include <queue>


namespace OpenXcom
{

class PathfindingNode;


struct OpenSetNode
{
	PathfindingNode* _node;
	float _tuTotal;
};


/**
 * Helper struct to compare two OpenSetNodes' TU-cost.
 */
struct IsCheaperOS
{
	/**
	 * Compares entries @a node1 and @a node2.
	 * @param node1 - pointer to first OpenSetNode
	 * @param node2 - pointer to second OpenSetNode
	 * @return, true if entry @a node2 should go before @a node1 (false if equal)
	 */
	bool operator ()(const OpenSetNode* const node1, const OpenSetNode* const node2) const
	{
		return node2->_tuTotal < node1->_tuTotal;
	}
};


/**
 * The openset nodes that need to be examined by A* pathfinding.
 */
class PathfindingOpenSet
{

private:
	std::priority_queue<OpenSetNode*, std::vector<OpenSetNode*>, IsCheaperOS> _frontier;

	public:
		/// Cleans up the PathfindingOpenSet.
		~PathfindingOpenSet();

		/// Adds a node to the frontier.
		void addNode(PathfindingNode* const node);
		/// Gets the next node to check.
		PathfindingNode* processNodeTop();

		/// Gets if the frontier is empty.
		bool isOpenSetEmpty() const
		{ return (_frontier.empty() == true); }
};

}

#endif
