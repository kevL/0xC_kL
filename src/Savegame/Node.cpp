/*
 * Copyright 2010-2018 OpenXcom Developers.
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

#include "Node.h"


namespace OpenXcom
{

// The following table presents the order in which aLien ranks choose nodes of
// alternate ranks to spawn on ('_priority' here, "Spawn" in mapView) or path to
// ('_patrol' here, "Flags" or "Importance" in mapView).
// NOTE: All aLiens can fall back to rank 0 nodes - which is civ-scout (outside ufo).
// NOTE: node rank 1 is for XCOM spawning and hence is not used by aLiens.
/* const int Node::nodeRank[8][7] = // stock table->
{
	{4, 3, 5, 8, 7, 2, 0},	// commander
	{4, 3, 5, 8, 7, 2, 0},	// leader
	{5, 4, 3, 2, 7, 8, 0},	// engineer
	{7, 6, 2, 8, 3, 4, 0},	// medic
	{3, 4, 5, 2, 7, 8, 0},	// navigator
	{2, 5, 3, 4, 6, 8, 0},	// soldier
	{2, 5, 3, 4, 6, 8, 0},	// terrorist1
	{2, 5, 3, 4, 6, 8, 0}	// terrorist2
}; */
/* const int Node::nodeRank[8][8] = // kL_begin:
{
	{4, 3, 5, 2, 7, 8, 6, 0},	// commander
	{4, 3, 5, 2, 7, 8, 6, 0},	// leader
	{5, 4, 3, 2, 7, 8, 6, 0},	// engineer
	{7, 3, 5, 4, 2, 8, 6, 0},	// medic
	{3, 4, 5, 2, 7, 8, 6, 0},	// navigator
	{2, 5, 3, 7, 4, 8, 6, 0},	// soldier
	{6, 8, 2, 5, 3, 4, 7, 0},	// terrorist1
	{8, 6, 2, 5, 3, 4, 7, 0}	// terrorist2
}; // */
// note: The 2nd dimension holds fallbacks:
	// 0:Civ-Scout
	// 1:XCom
	// 2:Soldier
	// 3:Navigator
	// 4:Leader/Commander
	// 5:Engineer
	// 6:Misc1
	// 7:Medic
	// 8:Misc2
// see Node.h, enum NodeRank ->

const int Node::nodeRank[8u][8u]
{
	{ NR_LEADER,	NR_NAVIGATOR,	NR_ENGINEER,	NR_SOLDIER,		NR_MEDIC,		NR_MISC2,	NR_MISC1, NR_SCOUT },	// commander
	{ NR_LEADER,	NR_NAVIGATOR,	NR_ENGINEER,	NR_SOLDIER,		NR_MEDIC,		NR_MISC2,	NR_MISC1, NR_SCOUT },	// leader
	{ NR_ENGINEER,	NR_LEADER,		NR_NAVIGATOR,	NR_SOLDIER,		NR_MEDIC,		NR_MISC2,	NR_MISC1, NR_SCOUT },	// engineer
	{ NR_MEDIC,		NR_NAVIGATOR,	NR_ENGINEER,	NR_LEADER,		NR_SOLDIER,		NR_MISC2,	NR_MISC1, NR_SCOUT },	// medic
	{ NR_NAVIGATOR,	NR_LEADER,		NR_ENGINEER,	NR_SOLDIER,		NR_MEDIC,		NR_MISC2,	NR_MISC1, NR_SCOUT },	// navigator
	{ NR_SOLDIER,	NR_ENGINEER,	NR_NAVIGATOR,	NR_MEDIC,		NR_LEADER,		NR_MISC2,	NR_MISC1, NR_SCOUT },	// soldier
	{ NR_MISC1,		NR_MISC2,		NR_SOLDIER,		NR_ENGINEER,	NR_NAVIGATOR,	NR_LEADER,	NR_MEDIC, NR_SCOUT },	// terrorist1
	{ NR_MISC2,		NR_MISC1,		NR_SOLDIER,		NR_ENGINEER,	NR_NAVIGATOR,	NR_LEADER,	NR_MEDIC, NR_SCOUT }	// terrorist2
}; // kL_end.


/**
 * Quick constructor for SavedBattleGame::load().
 */
Node::Node()
	:
		_id(0),
		_unittype(0),
		_noderank(0),
		_patrolpriority(0),
		_spawnweight(0),
		_segment(0),
		_attackfacility(0),
		_allocated(false)
{}

/**
 * Initializes the Node for the battlescape.
 * @param id				- node's ID
 * @param pos				- node's Position
 * @param segment			- for linking nodes
 * @param unittype			- size and movement type of allowable units (used for spawns only)
 * @param nodeank			- rank of allowable units (used for spawns and patrols)
 * @param patrolpriority	- preferability of the node for patrolling
 * @param attackfacility	- lures aLiens to attack their BaseDefense objectives
 * @param spawnweight		- weight of the node for spawning
 */
Node::Node(
		int id,
		Position pos,
		int segment,
		int unittype,
		int noderank,
		int patrolpriority,
		int attackfacility,
		int spawnweight)
	:
		_id(id),
		_pos(pos),
		_segment(segment),
		_unittype(unittype),
		_noderank(noderank),
		_patrolpriority(patrolpriority),
		_attackfacility(attackfacility),
		_spawnweight(spawnweight),
		_allocated(false)
{}

/**
 * Cleans up this Node.
 */
Node::~Node()
{}

/**
 * Loads this Node from a YAML file.
 * @param node - reference a YAML node
 */
void Node::load(const YAML::Node& node)
{
	_id             = node["id"]       .as<int>(_id);
	_pos            = node["pos"]      .as<Position>(_pos);
	_unittype       = node["type"]     .as<int>(_unittype);
	_noderank       = node["rank"]     .as<int>(_noderank);
	_patrolpriority = node["patrol"]   .as<int>(_patrolpriority);
	_attackfacility = node["attack"]   .as<int>(_attackfacility);
	_spawnweight	= node["spawn"]    .as<int>(_spawnweight);
	_allocated      = node["allocated"].as<bool>(_allocated);
	_links          = node["links"]    .as<std::vector<int>>(_links);
//	_segment        = node["segment"]  .as<int>(_segment);
}

/**
 * Saves this Node to a YAML file.
 * @return, YAML node
 */
YAML::Node Node::save() const
{
	YAML::Node node;

	node["id"]        = _id;
	node["pos"]       = _pos;
	node["type"]      = _unittype;
	node["rank"]      = _noderank;
	node["patrol"]    = _patrolpriority;
	node["attack"]    = _attackfacility;
	node["spawn"]     = _spawnweight;
	node["allocated"] = _allocated;
	node["links"]     = _links;
//	node["segment"]   = _segment;

	return node;
}

/**
 * Sets this Node's ID.
 * @param id - the ID
 */
void Node::setId(int id)
{
	_id = id;
}

/**
 * Gets this Node's ID.
 * @return, the ID
 */
int Node::getId() const
{
	return _id;
}

/**
 * Gets the rank of units that can spawn on this Node.
 * @return, NodeRank enum
 */
NodeRank Node::getNodeRank() const
{
	return static_cast<NodeRank>(_noderank);
}

/**
 * Gets the priority of this Node as a spawnpoint.
 * @return, priority
 */
int Node::getSpawnWeight() const
{
	return _spawnweight;
}

/**
 * Gets this Node's position.
 * @return, reference to Position
 */
const Position& Node::getPosition() const
{
	return _pos;
}

/**
 * Gets this Node's segment.
 * @return, segment
 */
int Node::getSegment() const
{
	return _segment;
}

/**
 * Gets this Node's linked Nodes for pathing.
 * @return, pointer to a vector of node-IDs
 */
std::vector<int>* Node::getLinks()
{
	return &_links;
}

/**
 * Sets this Node's type as dangerous to aLiens.
 */
void Node::setDangerous()
{
	_unittype |= TYPE_DANGEROUS;
}

/**
 * Gets this Node's unittype.
 * @note Also stores bit-dangerous which disallows its use by aLiens.
 * @return, type
 */
int Node::getUnitType() const
{
	return _unittype;
}

/**
 * Gets if this Node is allocated.
 * @return, true if allocated
 */
bool Node::isAllocated() const
{
	return _allocated;
}

/**
 * Sets this Node as allocated.
 * @param allocated - true to allocate the node (default true)
 */
void Node::allocate(bool allocated)
{
	_allocated = allocated;
}

/**
 * Gets if this Node is suitable for an aLien to target xCom-base targets/objectives.
 * @return, true if this is a Destroy-XCOM Base node
 */
bool Node::isAlienTarget() const
{
	return (_attackfacility != 0);
}

}
