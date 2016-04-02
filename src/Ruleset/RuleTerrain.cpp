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

#include "RuleTerrain.h"

#include "MapBlock.h"
#include "MapDataSet.h"
#include "Ruleset.h"

#include "../Engine/RNG.h"


namespace OpenXcom
{

/**
 * RuleTerrain construction.
 * @param type - reference the type of terrain
 */
RuleTerrain::RuleTerrain(const std::string& type)
	:
		_type(type),
		_script("DEFAULT"),
		_pyjamaType("STR_ARMOR_NONE_UC")
{}

/**
 * dTor.
 * @note RuleTerrain only holds MapBlocks. Map datafiles are referenced.
 */
RuleTerrain::~RuleTerrain()
{
	for (std::vector<MapBlock*>::const_iterator
			i = _mapBlocks.begin();
			i != _mapBlocks.end();
			++i)
		delete *i;
}

/**
 * Loads the RuleTerrain from a YAML file.
 * @param node	- reference a YAML node
 * @param rules	- pointer to Ruleset
 */
void RuleTerrain::load(
		const YAML::Node& node,
		Ruleset* const rules)
{
	_type	= node["type"]	.as<std::string>(_type);
	_script	= node["script"].as<std::string>(_script);

	if (const YAML::Node& mapDataSets = node["mapDataSets"])
	{
		_mapDataSets.clear();
		for (YAML::const_iterator
				i = mapDataSets.begin();
				i != mapDataSets.end();
				++i)
		{
			_mapDataSets.push_back(rules->getMapDataSet(i->as<std::string>()));
		}
	}

	if (const YAML::Node& mapBlocks = node["mapBlocks"])
	{
		_mapBlocks.clear();
		for (YAML::const_iterator
				i = mapBlocks.begin();
				i != mapBlocks.end();
				++i)
		{
			MapBlock* const mapBlock (new MapBlock((*i)["type"].as<std::string>()));
			mapBlock->load(*i);
			_mapBlocks.push_back(mapBlock);
		}
	}

	if (const YAML::Node& civs = node["civilianTypes"])
		_civilianTypes = civs.as<std::vector<std::string>>(_civilianTypes);
	else
	{
		_civilianTypes.push_back("MALE_CIVILIAN");
		_civilianTypes.push_back("FEMALE_CIVILIAN");
	}

	for (YAML::const_iterator
			i = node["music"].begin();
			i != node["music"].end();
			++i)
	{
		_musics.push_back((*i).as<std::string>()); // NOTE: might not be compatible w/ sza_MusicRules.
	}

	_pyjamaType = node["pyjamaType"].as<std::string>(_pyjamaType);
}

/**
 * Gets the array of MapBlocks.
 * @return, pointer to a vector of pointers as an array of MapBlocks
 */
const std::vector<MapBlock*>* RuleTerrain::getMapBlocks()
{
	return &_mapBlocks;
}

/**
 * Gets the array of MapDataSets (MCDs).
 * @return, pointer to a vector of pointers as an array of MapDataSets
 */
const std::vector<MapDataSet*>* RuleTerrain::getMapDataSets()
{
	return &_mapDataSets;
}

/**
 * Gets the terrain-type.
 * @return, the terrain-type
 */
const std::string& RuleTerrain::getType() const
{
	return _type;
}

/**
 * Gets a random MapBlock within the given constraints.
 * @param sizeX - the maximum x-size of the mapblock
 * @param sizeY - the maximum y-size of the mapblock
 * @param group - the group-type
 * @param force - true to enforce block-size at the max-size (default true)
 * @return, pointer to a MapBlock or nullptr if none found
 */
MapBlock* RuleTerrain::getMapBlockRand(
		int sizeX,
		int sizeY,
		int group,
		bool force) const
{
	//Log(LOG_INFO) << "getMapBlockRand()";
	//Log(LOG_INFO) << "sizeX = " << sizeX << " sizeY = " << sizeY << " group = " << group << " force = " << force;
	std::vector<MapBlock*> blocks;

	for (std::vector<MapBlock*>::const_iterator
			i = _mapBlocks.begin();
			i != _mapBlocks.end();
			++i)
	{
		//Log(LOG_INFO) << ". try [" << (*i)->getType() << "]";
		if ((*i)->isInGroup(group) == true
			&& ((*i)->getSizeX() == sizeX
				|| (force == false && (*i)->getSizeX() < sizeX))
			&& ((*i)->getSizeY() == sizeY
				|| (force == false && (*i)->getSizeY() < sizeY)))
		{
			//Log(LOG_INFO) << ". . PUSH BLOC";
			blocks.push_back(*i);
		}
	}

	if (blocks.empty() == false)
		return blocks[RNG::pick(blocks.size())];

	//Log(LOG_INFO) << "ret nullptr";
	return nullptr;
}

/**
 * Gets a MapBlock of a specified type.
 * @param type - reference the type of a MapBlock
 * @return, pointer to a MapBlock or nullptr if not found
 */
MapBlock* RuleTerrain::getMapBlock(const std::string& type) const
{
	for (std::vector<MapBlock*>::const_iterator
			i = _mapBlocks.begin();
			i != _mapBlocks.end();
			++i)
	{
		if ((*i)->getType() == type)
			return *i;
	}
	return nullptr;
}

/**
 * Gets a MapData object.
 * @param id			- pointer to the ID of the terrain
 * @param mapDataSetId	- pointer to the ID of the MapDataSet
 * @return, pointer to MapData object
 */
MapData* RuleTerrain::getMapData(
		unsigned int* id,
		int* mapDataSetId) const
{
	MapDataSet* dataSet (nullptr);

	std::vector<MapDataSet*>::const_iterator pDataSet (_mapDataSets.begin());
	for (
			;
			pDataSet != _mapDataSets.end();
			++pDataSet)
	{
		dataSet = *pDataSet;

		if (*id < dataSet->getSize())
			break;

		*id -= dataSet->getSize();
		++(*mapDataSetId);
	}

	if (pDataSet == _mapDataSets.end())
	{
		// oops! someone at microprose made an error in the map!
		// set this broken tile reference to BLANKS 0.
		dataSet = _mapDataSets.front();
		*id = 0;
		*mapDataSetId = 0;
	}

	return dataSet->getRecords()->at(*id);
}

/**
 * Gets the list of civilian-types to use on this terrain.
 * @return, list of civilian-types (default MALE_CIVILIAN and FEMALE_CIVILIAN)
 */
const std::vector<std::string>& RuleTerrain::getCivilianTypes() const
{
	return _civilianTypes;
}

/**
 * Gets the generation-script.
 * @return, the script to use
 */
const std::string& RuleTerrain::getScriptType() const
{
	return _script;
}

/**
 * Gets the list of music-tracks this terrain can choose from.
 * @return, list of music-tracks
 */
const std::vector<std::string>& RuleTerrain::getTerrainMusics() const
{
	return _musics;
}

/**
 * Gets the pyjama-type.
 * @note Used in BattlescapeGenerator::setTacticalSprites() to outfit soldiers
 * in camo suitable for the RuleTerrain.
 * @return, the pyjama-type
 */
const std::string& RuleTerrain::getPyjamaType() const
{
	return _pyjamaType;
}

}
