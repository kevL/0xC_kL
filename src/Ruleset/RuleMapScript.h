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

#ifndef OPENXCOM_RULEMAPSCRIPT_H
#define OPENXCOM_RULEMAPSCRIPT_H

#include <map>
//#include <string>
#include <vector>

#include <SDL/SDL_video.h>

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
	MSD_UNDEFINED = -1,	// -1
	MSD_ADDBLOCK,		//  0
	MSD_ADDLINE,		//  1
	MSD_ADDCRAFT,		//  2
	MSD_ADDUFO,			//  3
	MSD_DIGTUNNEL,		//  4
	MSD_FILLAREA,		//  5
	MSD_CHECKBLOCK,		//  6
	MSD_REMOVE,			//  7
	MSD_RESIZE			//  8
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
class RuleMapScript
{

private:
	size_t _id;
	int
		_sizeX,
		_sizeY,
		_sizeZ,
		_percent,
		_iterations,
		_frequency;

	TunnelData* _tunnelData;

	MapDirection _direction;
	MapScriptDirective _type;

	std::string _ufoType;

	std::vector<int>
		_blocks,
		_blocksTest,
		_freqs,
		_freqsTest,
		_groups,
		_groupsTest,
		_limit,
		_limitTest,
		_prereqs;
	std::vector<SDL_Rect*> _rects;

	/// Randomly generates a group from the array.
	int getGroup();
	/// Randomly generates a block number from the array.
	int getBlock();


	public:
		/// Constructs a RuleMapScript.
		RuleMapScript();
		/// Destructs the RuleMapScript.
		~RuleMapScript();

		/// Loads information from a ruleset-file.
		void load(const YAML::Node& node);

		/// Initializes all the variables and stuff for a RuleMapScript directive.
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
		int getPercent() const
		{ return _percent; };

		/// Gets the ID of this directive.
		size_t getId() const
		{ return _id; };

		/// Gets how many times this directive repeats (1 repeat means 2 executions)
		int getIterations() const
		{ return _iterations; };

		/// Gets what conditions apply to this directive.
		const std::vector<int>* getPrereqs() const
		{ return &_prereqs; };

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
