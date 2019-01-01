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

#ifndef OPENXCOM_NODE_H
#define OPENXCOM_NODE_H

//#include <yaml-cpp/yaml.h>

#include "../Battlescape/Position.h"


namespace OpenXcom
{

enum NodeRank
{
	NR_SCOUT,		// 0
	NR_XCOM,		// 1
	NR_SOLDIER,		// 2
	NR_NAVIGATOR,	// 3
	NR_LEADER,		// 4
	NR_ENGINEER,	// 5
	NR_MISC1,		// 6
	NR_MEDIC,		// 7
	NR_MISC2		// 8
};


/**
 * Represents a node/spawnpoint in the battlescape loaded from RMP files.
 * @sa http://www.ufopaedia.org/index.php?title=ROUTES
 */
class Node
{

private:
	bool _allocated;
	int
		_id,				// unique identifier
		_unittype,			// usability by small/large/flying units
		_noderank,			// aLien rank that can spawn or path here
		_patrolpriority,	// desirability of patrolling to
		_spawnweight,		// desirability of spawning at
		_segment,			// something to do with nodeLinks between MapBlocks; see BattlescapeGenerator::attachNodeLinks() etc.
		_attackfacility;	// lures aLiens to shoot objectives in BaseDefense missions

	Position _pos;

	std::vector<int> _links;


	public:
		static const int
			SEG_CRAFT = 1000,
			SEG_UFO   = 2000,

			// -> Any=0; Flying=1; Small=2; FlyingLarge=3; Large=4 <- loaded in BattlescapeGenerator::loadRouteFile()
			TYPE_SMALLFLYING = 0x01, // non-flying unit cannot spawn here when this bit is set; see SavedBattleGame::getSpawnNode()
			TYPE_SMALL       = 0x02, // large unit cannot spawn here when this bit is set; see SavedBattleGame::getSpawnNode()
									 // NOTE: getNodeType() is also used in SavedBattleGame::getPatrolNode() <- ie, it's not about spawning only; it affects patrolling also.
			TYPE_LARGEFLYING = 0x04, // kL_add
//			TYPE_LARGE       = 0x08, // kL_add (not used... equivalent to Any)
			TYPE_DANGEROUS   = 0x10, // kL_changed from 0x04[ie.large] -> an aLien was shot here, stop patrolling to it like an idiot with a deathwish

			nodeRank[8u][8u]; // maps node-ranks (.RMP) to aLiens' ranks

		static const size_t NODE_LINKS = 5u;

		/// Creates a Node.
		Node();
		/// Also creates a Node.
		Node(
				int id,
				Position pos,
				int segment,
				int unittype,
				int noderank,
				int patrolpriority,
				int attackfacility,
				int spawnweight);
		/// Cleans up the Node.
		~Node();

		/// Loads the Node from YAML.
		void load(const YAML::Node& node);
		/// Saves the Node to YAML.
		YAML::Node save() const;

		/// Sets the Node's ID.
		void setId(int id);
		/// Gets the Node's ID.
		int getId() const;

		/// Gets the Node's paths.
		std::vector<int>* getLinks();

		/// Gets the Node's rank.
		NodeRank getNodeRank() const;

		/// Gets the Node's priority.
		int getSpawnWeight() const;

		/// Gets the Node's position.
		const Position& getPosition() const;

		/// Gets the Node's segment.
		int getSegment() const;

		/// Sets the Node's type. SURPRISE!! (not)
		void setDangerous();
		/// Gets the Node's type.
		int getUnitType() const;

		/// Gets the 'flags' variable which is really the patrol-priority value.
		int getPatrolPriority() const
		{ return _patrolpriority; }

		// kL_note: in SavedBattleGame::getPatrolNodes() I changed less-than to greater-than ...
		// wonder if that matters here. So: REVERTED.
		// correction: REMOVED. Just use getPatrol() directly. Tks, anyway
		/// Compares the 'flags' variables of the nodes for the purpose of patrol decisions.
//		bool operator <(Node& b)
//		{ return _patrol < b.getPatrol(); }
		/// Compares the 'flags' variables of the nodes for the purpose of patrol decisions.
//		bool operator >(Node& b)
//		{ return _patrol > b.getPatrol(); }


		/// Checks if the Node is allocated.
		bool isAllocated() const;
		/// Sets the Node as allocated.
		void allocate(bool allocated = true);

		/// Gets if the Node is suitable for an aLien to target an xCom Base's targets/objective-parts.
		bool isAlienTarget() const;
};

}

#endif
