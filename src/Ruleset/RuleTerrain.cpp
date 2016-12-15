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
 * @param type - reference to the type of terrain
 */
RuleTerrain::RuleTerrain(const std::string& type)
	:
		_type(type),
		_script("DEFAULT"),
		_basicArmor("STR_ARMOR_NONE_UC")
{}

/**
 * dTor.
 * @note RuleTerrain only holds MapBlocks. Map datafiles are referenced.
 */
RuleTerrain::~RuleTerrain()
{
	for (std::vector<MapBlock*>::const_iterator
			i = _blocks.begin();
			i != _blocks.end();
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
		_dataSets.clear();
		for (YAML::const_iterator
				i = mapDataSets.begin();
				i != mapDataSets.end();
				++i)
		{
			_dataSets.push_back(rules->getMapDataSet(i->as<std::string>()));
		}
	}

	if (const YAML::Node& mapBlocks = node["mapBlocks"])
	{
		_blocks.clear();
		for (YAML::const_iterator
				i = mapBlocks.begin();
				i != mapBlocks.end();
				++i)
		{
			MapBlock* const mapBlock (new MapBlock((*i)["type"].as<std::string>()));
			mapBlock->load(*i);
			_blocks.push_back(mapBlock);
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

	_basicArmor = node["basicArmor"].as<std::string>(_basicArmor);
}

/**
 * Gets the array of MapBlocks.
 * @return, pointer to a vector of pointers as an array of MapBlocks
 */
const std::vector<MapBlock*>* RuleTerrain::getMapBlocks() const
{
	return &_blocks;
}

/**
 * Gets the array of MapDataSets (MCDs).
 * @return, pointer to a vector of pointers as an array of MapDataSets
 */
const std::vector<MapDataSet*>* RuleTerrain::getMapDataSets() const
{
	return &_dataSets;
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
MapBlock* RuleTerrain::getTerrainBlock(
		int sizeX,
		int sizeY,
		int group,
		bool force) const
{
	//Log(LOG_INFO) << "getTerrainBlock()";
	//Log(LOG_INFO) << "sizeX = " << sizeX << " sizeY = " << sizeY << " group = " << group << " force = " << force;
	std::vector<MapBlock*> blocks;

	for (std::vector<MapBlock*>::const_iterator
			i = _blocks.begin();
			i != _blocks.end();
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
 * @param type - reference to the type of a MapBlock
 * @return, pointer to a MapBlock or nullptr if not found
 */
MapBlock* RuleTerrain::getTerrainBlock(const std::string& type) const
{
	for (std::vector<MapBlock*>::const_iterator
			i = _blocks.begin();
			i != _blocks.end();
			++i)
	{
		if ((*i)->getType() == type)
			return *i;
	}
	return nullptr;
}

/**
 * Gets a MapData object.
 * @param partId	- pointer to the ID of the part
 * @param partSetId	- pointer to the ID of the tileset
 * @return, pointer to MapData object
 */
MapData* RuleTerrain::getTerrainPart(
		unsigned int* partId,
		int* partSetId) const
{
	MapDataSet* partSet;

	std::vector<MapDataSet*>::const_iterator pDataSet (_dataSets.begin());
	for (
			;
			pDataSet != _dataSets.end();
			++pDataSet)
	{
		partSet = *pDataSet;

		if (*partId < partSet->getRecordsQty()) // found.
			break;

		*partId -= partSet->getRecordsQty();
		++(*partSetId);
	}

	if (pDataSet == _dataSets.end()) // Set this broken part-reference to "BLANKS" id-0.
	{
		partSet = _dataSets.front();
		*partId = 0u;
		*partSetId = 0;
	}

	return partSet->getRecords()->at(*partId);
}

/**
 * Gets the list of civilian-types to use on this RuleTerrain.
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
 * Gets the list of music-tracks this RuleTerrain can choose from.
 * @return, list of music-tracks
 */
const std::vector<std::string>& RuleTerrain::getTerrainMusics() const
{
	return _musics;
}

/**
 * Gets the pyjama-type.
 * @note Used in BattlescapeGenerator::setTacticalSprites() to outfit soldiers
 * in basic camoflage that's suitable for this RuleTerrain.
 * @return, the pyjama-type
 */
const std::string& RuleTerrain::getBasicArmorType() const
{
	return _basicArmor;	// TODO: Add camo-outfits to Terrains.rul ....
}						// STR_STREET_ARCTIC_UC	(EqualTerms_kL.rul)
						// STR_STREET_JUNGLE_UC		"
}						// STR_STREET_URBAN_UC		"
						// STR_ARMOR_NONE_UC	(Armor.rul, default)
