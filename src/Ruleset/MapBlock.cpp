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

#include "MapBlock.h"

#include <algorithm> // std::find()

#include "../Battlescape/Position.h"

#include "../Engine/Exception.h"


namespace OpenXcom
{

/**
 * This is MapBlock construction.
 * @param type - reference to the type of the MapBlock
 */
MapBlock::MapBlock(const std::string& type)
	:
		_type(type),
		_size_x(10),
		_size_y(10),
		_size_z(4)
{
	_groups.push_back(0);
}

/**
 * MapBlock destruction.
 */
MapBlock::~MapBlock()
{}

/**
 * Loads the MapBlock from a YAML file.
 * @param node - reference a YAML node
 */
void MapBlock::load(const YAML::Node& node)
{
	_type   = node["type"]  .as<std::string>(_type);
	_size_x = node["width"] .as<int>(_size_x);
	_size_y = node["length"].as<int>(_size_y);
	_size_z = node["height"].as<int>(_size_z);

	if (_size_x % 10 != 0 || _size_y % 10 != 0)
	{
		std::string error ("MapBlock.load() " + _type + ": x/y must be evenly divisible by ten");
		throw Exception(error);
	}

	if (const YAML::Node& group = node["groups"])
	{
		_groups.clear();

		if (group.Type() == YAML::NodeType::Sequence)
			_groups = group.as<std::vector<int>>(_groups);
		else
			_groups.push_back(group.as<int>(0));
	}

	if (const YAML::Node& floor = node["revealedFloors"])
	{
		_revealedFloors.clear();

		if (floor.Type() == YAML::NodeType::Sequence)
			_revealedFloors = floor.as<std::vector<int>>(_revealedFloors);
		else
			_revealedFloors.push_back(floor.as<int>(0));
	}

	_items = node["items"].as<std::map<std::string, std::vector<Position>>>(_items);
}

/**
 * Gets the MapBlock type.
 * @return, the type of this block
 */
const std::string& MapBlock::getType() const
{
	return _type;
}

/**
 * Gets the MapBlock size-x.
 * @return, x in tile-space
 */
int MapBlock::getSizeX() const
{
	return _size_x;
}

/**
 * Gets the MapBlock size-y.
 * @return, y in tile-space
 */
int MapBlock::getSizeY() const
{
	return _size_y;
}

/**
 * Sets the MapBlock size-z.
 * @param size_z - z in tile-space
 */
void MapBlock::setSizeZ(int size_z)
{
	_size_z = size_z;
}

/**
 * Gets the MapBlock size-z.
 * @return, z in tile-space
 */
int MapBlock::getSizeZ() const
{
	return _size_z;
}

/**
 * Gets whether this MapBlock is a member of a particular group.
 * @param group - the group to check
 * @return, true if block is defined in @a group
 */
bool MapBlock::isInGroup(int group) const
{
	return std::find(
				_groups.begin(),
				_groups.end(),
				group) != _groups.end();
}

/**
 * Gets if this floor-level should be revealed or not.
 * @param reveal - the floor-level to check reveal value of
 * @return, true if floor-level will be revealed
 */
bool MapBlock::isFloorRevealed(int reveal) const
{
	return std::find(
				_revealedFloors.begin(),
				_revealedFloors.end(),
				reveal) != _revealedFloors.end();
}

/**
 * Gets any items associated with this MapBlock and their Positions.
 * @return, reference to a map of item-types and their positions
 */
const std::map<std::string, std::vector<Position>>& MapBlock::getPlacedItems()
{
	return _items;
}

}
