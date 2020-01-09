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

#ifndef OPENXCOM_PATHFINDINGNODE_H
#define OPENXCOM_PATHFINDINGNODE_H

#include "Position.h"


namespace OpenXcom
{

class PathfindingOpenSet;

struct OpenSetNode;


/**
 * Pathfinding-info for a tile on the battlefield.
 */
class PathfindingNode
{

private:
	friend class PathfindingOpenSet;

	bool _visited;
	int _dirPrior;
	float
		_tuTill, // true TU-cost from start to node
		_tuLeft; // estimated TU-cost to reach destination

	OpenSetNode* _openSetNode; // used by PathfindingOpenSet
	PathfindingNode* _nodePrior;

	const Position _pos;


	public:
		/// Creates a PathfindingNode.
		explicit PathfindingNode(const Position& pos);
		/// Cleans up the PathfindingNode.
		~PathfindingNode();

		/// Gets the Node's position.
		const Position& getPosition() const;

		/// Resets the Node.
		void resetNode();

		/// Checks if the Node has been visited.
		bool getVisited() const
		{ return _visited; }
		/// Marks the Node as visited.
		void setVisited()
		{ _visited = true; }

		/// Gets the TU-cost to reach the Node.
		int getTuCostTill(bool missile = false) const;
		/// Gets the estimated TU-cost to reach the destination.
		int getTuCostLeft() const
		{ return static_cast<int>(_tuLeft); }

		/// Gets the Node's previous Node.
		PathfindingNode* getPriorNode() const
		{ return _nodePrior; }
		/// Gets the previous walking direction.
		int getPriorDir() const
		{ return _dirPrior; }

		/// Gets if the Node is already in a PathfindingOpenSet.
		bool inOpenSet() const
		{ return (_openSetNode != nullptr); }

//#ifdef __MORPHOS__
//	#undef connect
//#endif

		/// Connects to previous Node along the path.
		void linkNode(
				int tuTill,
				PathfindingNode* const nodePrior,
				int dirPrior,
				const Position& posTarget);
		/// Connects to previous Node along a visit.
		void linkNode(
				int tuTill,
				PathfindingNode* const nodePrior,
				int dirPrior);
};


/**
 * Helper struct to compare two PathfindingNodes' TU-cost.
 * @note Used only by Pathfinding::findReachable().
 */
struct IsCheaperPF
{
	/**
	 * Compares nodes @a node1 and @a node2.
	 * @param node1 - pointer to first PathfindingNode
	 * @param node2 - pointer to second PathfindingNode
	 * @return, true if node @a node1 must come before @a node2
	 */
	bool operator ()(const PathfindingNode* const node1, const PathfindingNode* const node2) const
	{
		return (node1->getTuCostTill() < node2->getTuCostTill());
	}
};

}

#endif
