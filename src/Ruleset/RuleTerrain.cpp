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
			i  = _blocks.begin();
			i != _blocks.end();
			++i)
		delete *i;
}

/**
 * Loads this terrain-type from YAML.
 * @param node	- reference a YAML node
 * @param rules	- pointer to Ruleset
 */
void RuleTerrain::load(
		const YAML::Node& node,
		Ruleset* const rules)
{
	_type   = node["type"]  .as<std::string>(_type);
	_script = node["script"].as<std::string>(_script);

	if (const YAML::Node& dataSets = node["dataSets"])
	{
		_dataSets.clear();

		_dataSets.push_back(rules->getMapDataSet("BLANKS")); // NOTE: The rule for every terrain-type gets "BLANKS".

		for (YAML::const_iterator
				i  = dataSets.begin();
				i != dataSets.end();
				++i)
		{
			_dataSets.push_back(rules->getMapDataSet(i->as<std::string>()));
		}
	}

	if (const YAML::Node& blocks = node["blocks"])
	{
		_blocks.clear();
		for (YAML::const_iterator
				i  = blocks.begin();
				i != blocks.end();
				++i)
		{
			MapBlock* const block (new MapBlock((*i)["type"].as<std::string>()));
			block->load(*i);
			_blocks.push_back(block);
		}
	}

	if (const YAML::Node& civTypes = node["civTypes"])
		_civTypes = civTypes.as<std::vector<std::string>>(_civTypes);
	else
	{
		_civTypes.push_back("MALE_CIVILIAN");
		_civTypes.push_back("FEMALE_CIVILIAN");
	}

	for (YAML::const_iterator
			i  = node["music"].begin();
			i != node["music"].end();
			++i)
	{
		_musics.push_back((*i).as<std::string>()); // NOTE: might not be compatible w/ sza_MusicRules.
	}

	_basicArmor = node["basicArmor"].as<std::string>(_basicArmor);
}

/**
 * Gets this terrain-type's list of MapBlocks.
 * @return, pointer to a vector of pointers as an array of MapBlocks
 */
const std::vector<MapBlock*>* RuleTerrain::getMapBlocks() const
{
	return &_blocks;
}

/**
 * Gets this terrain-type's list of MapDataSets (MCDs).
 * @return, pointer to a vector of pointers as an array of MapDataSets
 */
const std::vector<MapDataSet*>* RuleTerrain::getMapDataSets() const
{
	return &_dataSets;
}

/**
 * Gets the terrain's type.
 * @return, this terrain-type
 */
const std::string& RuleTerrain::getType() const
{
	return _type;
}

/**
 * Gets a MapBlock in this terrain-type within specified constraints.
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
	//Log(LOG_INFO) << "sizeX= " << sizeX << " sizeY= " << sizeY << " group= " << group << " force= " << force;
	std::vector<MapBlock*> blocks;

	for (std::vector<MapBlock*>::const_iterator
			i  = _blocks.begin();
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
 * Gets a MapBlock of a specified type in this terrain-type.
 * @param type - reference to the type of a MapBlock
 * @return, pointer to a MapBlock or nullptr if not found
 */
MapBlock* RuleTerrain::getTerrainBlock(const std::string& type) const
{
	for (std::vector<MapBlock*>::const_iterator
			i  = _blocks.begin();
			i != _blocks.end();
			++i)
	{
		if ((*i)->getType() == type)
			return *i;
	}
	return nullptr;
}

/**
 * Gets a MapData object (MCD tile-part) in this terrain-type.
 * @param partId	- pointer to the ID of the part
 * @param partSetId	- pointer to the ID of the tileset
 * @return, pointer to MapData object
 */
MapData* RuleTerrain::getTerrainPart(
		size_t* partId,
		int* partSetId) const
{
	MapDataSet* dataSet;

	std::vector<MapDataSet*>::const_iterator i (_dataSets.begin());
	for (
			;
			i != _dataSets.end();
			++i)
	{
		dataSet = *i;

		if (*partId < dataSet->getRecordsQty()) // found.
			break;

		*partId -= dataSet->getRecordsQty();
		++(*partSetId);
	}

	if (i == _dataSets.end()) // Set this broken part-reference to "BLANKS" id-0.
	{
		dataSet = _dataSets.front(); // "BLANKS"
		*partSetId = 0;
		*partId = 0u;
	}

	return dataSet->getRecords()->at(*partId);
}

/**
 * Gets the civilian-types that can appear in this terrain-type.
 * @return, list of civilian-types (default MALE_CIVILIAN and FEMALE_CIVILIAN)
 */
const std::vector<std::string>& RuleTerrain::getCivilianTypes() const
{
	return _civTypes;
}

/**
 * Gets the generation script for this terrain-type.
 * @return, the script to use
 */
const std::string& RuleTerrain::getScriptType() const
{
	return _script;
}

/**
 * Gets the list of music that can play in this terrain-type.
 * @return, list of music-tracks
 */
const std::vector<std::string>& RuleTerrain::getTerrainMusics() const
{
	return _musics;
}

/**
 * Gets the basic-armor-type of this terrain-type.
 * @note Used in BattlescapeGenerator::setTacticalSprites() to outfit soldiers
 * in basic camoflage that's suitable for this RuleTerrain.
 * @return, the basic-armor-type
 */
const std::string& RuleTerrain::getBasicArmorType() const
{
	return _basicArmor;	// TODO: Add camo-outfits to Terrains.rul ....
}						// STR_STREET_ARCTIC_UC	(EqualTerms_kL.rul)
						// STR_STREET_JUNGLE_UC		"
}						// STR_STREET_URBAN_UC		"
						// STR_ARMOR_NONE_UC	(Armor.rul, default)
