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

#ifndef OPENXCOM_MAPBLOCK_H
#define OPENXCOM_MAPBLOCK_H

//#include <map>	// std::map
//#include <string>	// std::string
//#include <vector>	// std::vector

#include <yaml-cpp/yaml.h>


namespace OpenXcom
{

enum MapBlockType // These define 'groups' ... more or less.
{
	MBT_UNDEFINED = -1,	// -1
	MBT_DEFAULT,		//  0
	MBT_LANDPAD,		//  1
	MBT_EWROAD,			//  2
	MBT_NSROAD,			//  3
	MBT_CROSSROAD,		//  4
	MBT_START,			//  5 kL_add. For AlienBase starting equipment spawn
	MBT_CONTROL,		//  6
	MBT_BRAIN			//  7
};


class Position;


/**
 * Represents a battlefield MapBlock.
 * @note It contains constant info about this MapBlock, like its type,
 * dimensions, attributes .... Map blocks are stored in RuleTerrain objects.
 * @sa http://www.ufopaedia.org/index.php?title=MAPS_Terrain
 */
class MapBlock
{

private:
	std::string _type;
	int
		_size_x,
		_size_y,
		_size_z;

	std::vector<int>
		_groups,
		_revealedFloors;

	std::map<std::string, std::vector<Position>> _items;


	public:
		/// Constructs a MapBlock object.
		explicit MapBlock(const std::string& type);
		/// Destructs the MapBlock object.
		~MapBlock();

		/// Loads the MapBlock from YAML.
		void load(const YAML::Node& node);

		/// Gets the MapBlock's type - used for MAP generation.
		const std::string& getType() const;

		/// Gets the MapBlock's x-size.
		int getSizeX() const;
		/// Gets the MapBlock's y-size.
		int getSizeY() const;
		/// Gets the MapBlock's z-size.
		int getSizeZ() const;
		/// Sets the MapBlock's z-size.
		void setSizeZ(int size_z);

		/// Gets if the MapBlock is from the group specified.
		bool isInGroup(int group) const;
		/// Gets if the MapBlock's floor-tiles should be revealed or not.
		bool isFloorRevealed(int reveal) const;

		/// Gets the Positions for items that instantiate in the MapBlock.
		const std::map<std::string, std::vector<Position>>& getBlockItems();
};

}

#endif
