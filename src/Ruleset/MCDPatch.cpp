/*
 * Copyright 2010-2015 OpenXcom Developers.
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

#include "MCDPatch.h"

#include "MapDataSet.h"


namespace OpenXcom
{

/**
 * Initializes an MCD Patch.
 */
MCDPatch::MCDPatch()
{}

/**
 * dTor.
 */
MCDPatch::~MCDPatch()
{}

/**
 * Loads the MCD Patch from a YAML file.
 * TODO: fill this out with more data.
 * @param node - YAML node
 */
void MCDPatch::load(const YAML::Node& node)
{
	YAML::Node data = node["data"];

	for (YAML::const_iterator
			i = data.begin();
			i != data.end();
			++i)
	{
		const size_t MCDIndex = (*i)["MCDIndex"].as<size_t>();

		if ((*i)["bigWall"])
		{
			const int bigWall = (*i)["bigWall"].as<int>();
			_bigWalls.push_back(std::make_pair(
											MCDIndex,
											bigWall));
		}

		if ((*i)["TUWalk"])
		{
			const int TUWalk = (*i)["TUWalk"].as<int>();
			_TUWalks.push_back(std::make_pair(
											MCDIndex,
											TUWalk));
		}

		if ((*i)["TUFly"])
		{
			const int TUFly = (*i)["TUFly"].as<int>();
			_TUFlys.push_back(std::make_pair(
										MCDIndex,
										TUFly));
		}

		if ((*i)["TUSlide"])
		{
			const int TUSlide = (*i)["TUSlide"].as<int>();
			_TUSlides.push_back(std::make_pair(
											MCDIndex,
											TUSlide));
		}

		if ((*i)["deathTile"])
		{
			const int deathTile = (*i)["deathTile"].as<int>();
			_deathTiles.push_back(std::make_pair(
											MCDIndex,
											deathTile));
		}

		if ((*i)["terrainHeight"])
		{
			const int terrainHeight = (*i)["terrainHeight"].as<int>();
			_terrainHeight.push_back(std::make_pair(
												MCDIndex,
												terrainHeight));
		}

		if ((*i)["explosive"])
		{
			const int explosive = (*i)["explosive"].as<int>();
			_explosives.push_back(std::make_pair(
											MCDIndex,
											explosive));
		}

		if ((*i)["armor"])
		{
			const int armor = (*i)["armor"].as<int>();
			_armors.push_back(std::make_pair(
										MCDIndex,
										armor));
		}

		if ((*i)["flammability"])
		{
			const int flammability = (*i)["flammability"].as<int>();
			_flammabilities.push_back(std::make_pair(
												MCDIndex,
												flammability));
		}

		if ((*i)["fuel"])
		{
			const int fuel = (*i)["fuel"].as<int>();
			_fuels.push_back(std::make_pair(
										MCDIndex,
										fuel));
		}

		if ((*i)["footstepSound"])
		{
			const int footstepSound = (*i)["footstepSound"].as<int>();
			_footstepSounds.push_back(std::make_pair(
												MCDIndex,
												footstepSound));
		}

		if ((*i)["HEBlock"])
		{
			const int HEBlock = (*i)["HEBlock"].as<int>();
			_HEBlocks.push_back(std::make_pair(
											MCDIndex,
											HEBlock));
		}

		if ((*i)["noFloor"])
		{
			const bool noFloor = (*i)["noFloor"].as<bool>();
			_noFloors.push_back(std::make_pair(
											MCDIndex,
											noFloor));
		}

		if ((*i)["stopLOS"])
		{
			const bool stopLOS = (*i)["stopLOS"].as<bool>();
			_stopLOSses.push_back(std::make_pair(
											MCDIndex,
											stopLOS));
		}

		if ((*i)["LOFTS"])
		{
			const std::vector<size_t> lofts = (*i)["LOFTS"].as<std::vector<size_t>>();
			_LOFTS.push_back(std::make_pair(
										MCDIndex,
										lofts));
		}

		if ((*i)["objectType"])
		{
			const MapDataType objectType = static_cast<MapDataType>((*i)["objectType"].as<int>());
			_objectTypes.push_back(std::make_pair(
												MCDIndex,
												objectType));
		}

		if ((*i)["specialType"])
		{
			const SpecialTileType specialType = static_cast<SpecialTileType>((*i)["specialType"].as<int>());
			_specialTypes.push_back(std::make_pair(
												MCDIndex,
												specialType));
		}

		if ((*i)["scanG"])
		{
			const int scanG = (*i)["scanG"].as<int>();
			_scanG.push_back(std::make_pair(
										MCDIndex,
										scanG));
		}
	}
}

/**
 * Applies an MCD patch to a mapDataSet.
 * @param dataSet The MapDataSet we want to modify.
 */
void MCDPatch::modifyData(MapDataSet* const dataSet) const
{
	for (std::vector<std::pair<size_t, int>>::const_iterator
			i = _bigWalls.begin();
			i != _bigWalls.end();
			++i)
	{
		dataSet->getObjects()->at(i->first)->setBigWall(i->second);
	}

	for (std::vector<std::pair<size_t, int>>::const_iterator
			i = _TUWalks.begin();
			i != _TUWalks.end();
			++i)
	{
		dataSet->getObjects()->at(i->first)->setTUWalk(i->second);
	}

	for (std::vector<std::pair<size_t, int>>::const_iterator
			i = _TUFlys.begin();
			i != _TUFlys.end();
			++i)
	{
		dataSet->getObjects()->at(i->first)->setTUFly(i->second);
	}

	for (std::vector<std::pair<size_t, int>>::const_iterator
			i = _TUSlides.begin();
			i != _TUSlides.end();
			++i)
	{
		dataSet->getObjects()->at(i->first)->setTUSlide(i->second);
	}

	for (std::vector<std::pair<size_t, int>>::const_iterator
			i = _deathTiles.begin();
			i != _deathTiles.end();
			++i)
	{
		dataSet->getObjects()->at(i->first)->setDieMCD(i->second);
	}

	for (std::vector<std::pair<size_t, int>>::const_iterator
			i = _terrainHeight.begin();
			i != _terrainHeight.end();
			++i)
	{
		dataSet->getObjects()->at(i->first)->setTerrainLevel(i->second);
	}

	for (std::vector<std::pair<size_t, int>>::const_iterator
			i = _explosives.begin();
			i != _explosives.end();
			++i)
	{
		dataSet->getObjects()->at(i->first)->setExplosive(i->second);
	}

	for (std::vector<std::pair<size_t, int>>::const_iterator
			i = _armors.begin();
			i != _armors.end();
			++i)
	{
		dataSet->getObjects()->at(i->first)->setArmor(i->second);
	}

	for (std::vector<std::pair<size_t, int>>::const_iterator
			i = _flammabilities.begin();
			i != _flammabilities.end();
			++i)
	{
		dataSet->getObjects()->at(i->first)->setFlammable(i->second);
	}

	for (std::vector<std::pair<size_t, int>>::const_iterator
			i = _fuels.begin();
			i != _fuels.end();
			++i)
	{
		dataSet->getObjects()->at(i->first)->setFuel(i->second);
	}

	for (std::vector<std::pair<size_t, int>>::const_iterator
			i = _footstepSounds.begin();
			i != _footstepSounds.end();
			++i)
	{
		dataSet->getObjects()->at(i->first)->setFootstepSound(i->second);
	}

	for (std::vector<std::pair<size_t, int>>::const_iterator
			i = _HEBlocks.begin();
			i != _HEBlocks.end();
			++i)
	{
		dataSet->getObjects()->at(i->first)->setHEBlock(i->second);
	}

	for (std::vector<std::pair<size_t, bool>>::const_iterator
			i = _noFloors.begin();
			i != _noFloors.end();
			++i)
	{
		dataSet->getObjects()->at(i->first)->setNoFloor(i->second);
	}

	for (std::vector<std::pair<size_t, bool>>::const_iterator
			i = _stopLOSses.begin();
			i != _stopLOSses.end();
			++i)
	{
		dataSet->getObjects()->at(i->first)->setStopLOS(i->second);
	}

	for (std::vector<std::pair<size_t, std::vector<size_t>>>::const_iterator
			i = _LOFTS.begin();
			i != _LOFTS.end();
			++i)
	{
		size_t layer = 0;
		for (std::vector<size_t>::const_iterator
				j = i->second.begin();
				j != i->second.end();
				++j)
		{
			dataSet->getObjects()->at(i->first)->setLoftId(*j, layer++);
		}
	}

	for (std::vector<std::pair<size_t, MapDataType>>::const_iterator
			i = _objectTypes.begin();
			i != _objectTypes.end();
			++i)
	{
		dataSet->getObjects()->at(i->first)->setObjectType(static_cast<MapDataType>(i->second));
	}

	for (std::vector<std::pair<size_t, SpecialTileType>>::const_iterator
			i = _specialTypes.begin();
			i != _specialTypes.end();
			++i)
	{
		dataSet->getObjects()->at(i->first)->setSpecialType(static_cast<SpecialTileType>(i->second));
	}

	for (std::vector<std::pair<size_t, int>>::const_iterator
			i = _scanG.begin();
			i != _scanG.end();
			++i)
	{
		dataSet->getObjects()->at(i->first)->setMiniMapIndex(static_cast<unsigned short>(i->second));
	}
}

}
