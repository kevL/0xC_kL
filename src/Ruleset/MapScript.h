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

#ifndef OPENXCOM_MAPSCRIPT_H
#define OPENXCOM_MAPSCRIPT_H

#include <map>
//#include <string>
#include <vector>

#include <SDL_video.h>

#include <yaml-cpp/yaml.h>


namespace OpenXcom
{

struct MCDReplacement
{
	int
		dataSet,
		entry;
};

struct TunnelData
{
	int level;
	std::map<std::string, MCDReplacement> replacements;

	///
	const MCDReplacement* getMcdReplacement(const std::string& type)
	{
		if (replacements.find(type) != replacements.end())
			return &replacements[type];

		return nullptr;
	}
};

enum MapScriptDirective
{
	MSC_UNDEFINED = -1,	// -1
	MSC_ADDBLOCK,		//  0
	MSC_ADDLINE,		//  1
	MSC_ADDCRAFT,		//  2
	MSC_ADDUFO,			//  3
	MSC_DIGTUNNEL,		//  4
	MSC_FILLAREA,		//  5
	MSC_CHECKBLOCK,		//  6
	MSC_REMOVE,			//  7
	MSC_RESIZE			//  8
};

enum MapDirection
{
	MD_NONE,		// 0
	MD_VERTICAL,	// 1
	MD_HORIZONTAL,	// 2
	MD_BOTH			// 3
};

class MapBlock;
class RuleTerrain;


/**
 * A class for handling battlefield designs.
 */
class MapScript
{

private:
	int
		_sizeX,
		_sizeY,
		_sizeZ,
		_executionChance,
		_executions,
		_cumulativeFrequency,
		_label;

	TunnelData* _tunnelData;

	MapDirection _direction;
	MapScriptDirective _type;

	std::string _ufoType;

	std::vector<int>
		_blocks,
		_blocksTemp,
		_conditions,
		_frequencies,
		_frequenciesTemp,
		_groups,
		_groupsTemp,
		_maxUses,
		_maxUsesTemp;
	std::vector<SDL_Rect*> _rects;

	/// Randomly generate a group from within the array.
	int getGroupNumber();
	/// Randomly generate a block number from within the array.
	int getBlockNumber();


	public:
		/// Constructs a MapScript.
		MapScript();
		/// Destructs the MapScript.
		~MapScript();

		/// Loads information from a ruleset-file.
		void load(const YAML::Node& node);

		/// Initializes all the variables and stuff for a MapScript directive.
		void init();

		/// Gets what type of directive this is.
		MapScriptDirective getType() const
		{ return _type; };

		/// Gets the rects, describing the areas this directive applies to.
		const std::vector<SDL_Rect*>* getRects() const
		{ return &_rects; };

		/// Gets the X size for this directive.
		int getSizeX() const
		{ return _sizeX; };
		/// Gets the Y size for this directive.
		int getSizeY() const
		{ return _sizeY; };
		/// Gets the Z size for this directive.
		int getSizeZ() const
		{ return _sizeZ; };

		/// Get the chances of this directive executing.
		int chanceOfExecution() const
		{ return _executionChance; };

		/// Gets the label for this directive.
		int getLabel() const
		{ return _label; };

		/// Gets how many times this directive repeats (1 repeat means 2 executions)
		int getExecutions() const
		{ return _executions; };

		/// Gets what conditions apply to this directive.
		const std::vector<int>* getConditions() const
		{ return &_conditions; };

		/// Gets the groups-vector for iteration.
		const std::vector<int>* getGroups() const
		{ return &_groups; };
		/// Gets the blocks-vector for iteration.
		const std::vector<int>* getBlocks() const
		{ return &_blocks; };

		/// Gets the direction this directive goes (for lines and tunnels).
		MapDirection getDirection() const
		{ return _direction; };

		/// Gets the MCD-replacement-data for tunnel-replacements.
		TunnelData* getTunnelData() const
		{ return _tunnelData; };

		/// Gets a MapBlock from an array of either groups or blocks.
		MapBlock* getNextBlock(const RuleTerrain* const terrainRule);

		/// Gets the ufo-type for the ADD-UFO directive.
		const std::string& getUfoType() const;
};

}

#endif
