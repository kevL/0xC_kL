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

#include "MCDPatch.h"

#include "MapDataSet.h"

//#include "../Engine/Logger.h"


namespace OpenXcom
{

/**
 * Initializes the MCDPatch.
 */
MCDPatch::MCDPatch()
{
	//Log(LOG_INFO) << "MCDPatch cTor";
}

/**
 * dTor.
 */
MCDPatch::~MCDPatch()
{
	//Log(LOG_INFO) << "MCDPatch dTor";
}

/**
 * Loads the MCDPatch from a YAML file.
 * @param node - reference a YAML node
 */
void MCDPatch::load(const YAML::Node& node) // TODO: fill this out with more data as needed.
{
	//Log(LOG_INFO) << "MCDPatch: load()";
	YAML::Node data (node["data"]);

	for (YAML::const_iterator
			i  = data.begin();
			i != data.end();
			++i)
	{
		const size_t mcdIndex ((*i)["MCDIndex"].as<size_t>());
		//Log(LOG_INFO) << ". index = " << mcdIndex;

		if ((*i)["bigWall"])
		{
			const int bigWall ((*i)["bigWall"].as<int>());
			//Log(LOG_INFO) << ". . bigWall = " << bigWall;
			_bigWalls.push_back(std::make_pair(
											mcdIndex,
											bigWall));
		}

		if ((*i)["TUWalk"])
		{
			const int tuWalk ((*i)["TUWalk"].as<int>());
			_tuWalks.push_back(std::make_pair(
											mcdIndex,
											tuWalk));
		}

		if ((*i)["TUSlide"])
		{
			const int tuSlide ((*i)["TUSlide"].as<int>());
			_tuSlides.push_back(std::make_pair(
											mcdIndex,
											tuSlide));
		}

		if ((*i)["TUFly"])
		{
			const int tuFly ((*i)["TUFly"].as<int>());
			_tuFlys.push_back(std::make_pair(
										mcdIndex,
										tuFly));
		}

		if ((*i)["deathTile"])
		{
			const int deathTile ((*i)["deathTile"].as<int>());
			_deathTiles.push_back(std::make_pair(
											mcdIndex,
											deathTile));
		}

		if ((*i)["terrainHeight"])
		{
			const int terrainHeight ((*i)["terrainHeight"].as<int>());
			_terrainHeight.push_back(std::make_pair(
												mcdIndex,
												terrainHeight));
		}

		if ((*i)["explosive"])
		{
			const int explosive ((*i)["explosive"].as<int>());
			_explosives.push_back(std::make_pair(
											mcdIndex,
											explosive));
		}

		if ((*i)["armor"])
		{
			const int armor ((*i)["armor"].as<int>());
			_armors.push_back(std::make_pair(
										mcdIndex,
										armor));
		}

		if ((*i)["flammability"])
		{
			const int flammability ((*i)["flammability"].as<int>());
			_flammabilities.push_back(std::make_pair(
												mcdIndex,
												flammability));
		}

		if ((*i)["fuel"])
		{
			const int fuel ((*i)["fuel"].as<int>());
			_fuels.push_back(std::make_pair(
										mcdIndex,
										fuel));
		}

		if ((*i)["footstepSound"])
		{
			const int footstepSound ((*i)["footstepSound"].as<int>());
			_footstepSounds.push_back(std::make_pair(
												mcdIndex,
												footstepSound));
		}

		if ((*i)["HEBlock"])
		{
			const int HEBlock ((*i)["HEBlock"].as<int>());
			_heBlocks.push_back(std::make_pair(
											mcdIndex,
											HEBlock));
		}

		if ((*i)["noFloor"])
		{
			const bool noFloor ((*i)["noFloor"].as<bool>());
			_noFloors.push_back(std::make_pair(
											mcdIndex,
											noFloor));
		}

		if ((*i)["stopLOS"])
		{
			const bool stopLOS ((*i)["stopLOS"].as<bool>());
			_stopLOSes.push_back(std::make_pair(
											mcdIndex,
											stopLOS));
		}

		if ((*i)["LOFTS"])
		{
			const std::vector<size_t> lofts ((*i)["LOFTS"].as<std::vector<size_t>>());
			_lofts.push_back(std::make_pair(
										mcdIndex,
										lofts));
		}

		if ((*i)["objectType"])
		{
			const MapDataType objectType (static_cast<MapDataType>((*i)["objectType"].as<int>()));
			_objectTypes.push_back(std::make_pair(
												mcdIndex,
												objectType));
		}

		if ((*i)["specialType"])
		{
			const TilepartSpecial specialType (static_cast<TilepartSpecial>((*i)["specialType"].as<int>()));
			_specialTypes.push_back(std::make_pair(
												mcdIndex,
												specialType));
		}

		if ((*i)["scanG"])
		{
			const int scanG ((*i)["scanG"].as<int>());
			_scanG.push_back(std::make_pair(
										mcdIndex,
										scanG));
		}

		if ((*i)["psycho"])
		{
			const int psychedelic ((*i)["psycho"].as<int>());
			_psychedelic.push_back(std::make_pair(
												mcdIndex,
												psychedelic));
		}
	}
}

/**
 * Applies an MCD patch to a MapDataSet/Tileset.
 * @param dataSet - pointer to the MapDataSet to modify
 */
void MCDPatch::patchData(MapDataSet* const dataSet) const
{
	//Log(LOG_INFO) << "MCDPatch::patchData() dataSet= " << dataSet->getType();
	for (std::vector<std::pair<size_t, int>>::const_iterator
			i  = _bigWalls.begin();
			i != _bigWalls.end();
			++i)
	{
		//Log(LOG_INFO) << ". id= " << (i->first) << " bigWall= " << (i->second);
		int bigwall;
		switch (i->second) // convert decimal-based patches to hexadecimal BigwallTypes.
		{
			default:
			case 0: bigwall = 0x00; break;	// BIGWALL_NONE
			case 1: bigwall = 0x01; break;	// BIGWALL_BLOCK
			case 2: bigwall = 0x02; break;	// BIGWALL_NESW
			case 3: bigwall = 0x04; break;	// BIGWALL_NWSE
			case 4: bigwall = 0x08; break;	// BIGWALL_WEST
			case 5: bigwall = 0x10; break;	// BIGWALL_NORTH
			case 6: bigwall = 0x20; break;	// BIGWALL_EAST
			case 7: bigwall = 0x40; break;	// BIGWALL_SOUTH
			case 8: bigwall = 0x80;			// BIGWALL_E_S

//			case 9:							// BIGWALL_W_N - not used in stock UFO.
		}
		dataSet->getRecords()->at(i->first)->setBigWall(bigwall);
	}

	for (std::vector<std::pair<size_t, int>>::const_iterator
			i  = _tuWalks.begin();
			i != _tuWalks.end();
			++i)
	{
		dataSet->getRecords()->at(i->first)->setTuWalk(i->second);
	}

	for (std::vector<std::pair<size_t, int>>::const_iterator
			i  = _tuSlides.begin();
			i != _tuSlides.end();
			++i)
	{
		dataSet->getRecords()->at(i->first)->setTuSlide(i->second);
	}

	for (std::vector<std::pair<size_t, int>>::const_iterator
			i  = _tuFlys.begin();
			i != _tuFlys.end();
			++i)
	{
		dataSet->getRecords()->at(i->first)->setTuFly(i->second);
	}

	for (std::vector<std::pair<size_t, int>>::const_iterator
			i  = _deathTiles.begin();
			i != _deathTiles.end();
			++i)
	{
		dataSet->getRecords()->at(i->first)->setDiePart(i->second);
	}

	for (std::vector<std::pair<size_t, int>>::const_iterator
			i  = _terrainHeight.begin();
			i != _terrainHeight.end();
			++i)
	{
		dataSet->getRecords()->at(i->first)->setTerrainLevel(i->second);
	}

	for (std::vector<std::pair<size_t, int>>::const_iterator
			i  = _explosives.begin();
			i != _explosives.end();
			++i)
	{
		dataSet->getRecords()->at(i->first)->setExplosive(i->second);
	}

	for (std::vector<std::pair<size_t, int>>::const_iterator
			i  = _armors.begin();
			i != _armors.end();
			++i)
	{
		dataSet->getRecords()->at(i->first)->setArmorPoints(i->second);
	}

	for (std::vector<std::pair<size_t, int>>::const_iterator
			i  = _flammabilities.begin();
			i != _flammabilities.end();
			++i)
	{
		dataSet->getRecords()->at(i->first)->setFlammable(i->second);
	}

	for (std::vector<std::pair<size_t, int>>::const_iterator
			i  = _fuels.begin();
			i != _fuels.end();
			++i)
	{
		dataSet->getRecords()->at(i->first)->setFuel(i->second);
	}

	for (std::vector<std::pair<size_t, int>>::const_iterator
			i  = _footstepSounds.begin();
			i != _footstepSounds.end();
			++i)
	{
		dataSet->getRecords()->at(i->first)->setFootstepSound(i->second);
	}

	for (std::vector<std::pair<size_t, int>>::const_iterator
			i  = _heBlocks.begin();
			i != _heBlocks.end();
			++i)
	{
		dataSet->getRecords()->at(i->first)->setHeBlock(i->second);
	}

	for (std::vector<std::pair<size_t, bool>>::const_iterator
			i  = _noFloors.begin();
			i != _noFloors.end();
			++i)
	{
		dataSet->getRecords()->at(i->first)->setNoFloor(i->second);
	}

	for (std::vector<std::pair<size_t, bool>>::const_iterator
			i  = _stopLOSes.begin();
			i != _stopLOSes.end();
			++i)
	{
		dataSet->getRecords()->at(i->first)->setStopLos(i->second);
	}

	for (std::vector<std::pair<size_t, std::vector<size_t>>>::const_iterator
			i  = _lofts.begin();
			i != _lofts.end();
			++i)
	{
		size_t layer (0u);
		for (std::vector<size_t>::const_iterator
				j  = i->second.begin();
				j != i->second.end();
				++j)
		{
			dataSet->getRecords()->at(i->first)->setLoftId(*j, layer++);
		}
	}

	for (std::vector<std::pair<size_t, MapDataType>>::const_iterator
			i  = _objectTypes.begin();
			i != _objectTypes.end();
			++i)
	{
		dataSet->getRecords()->at(i->first)->setPartType(static_cast<MapDataType>(i->second));
	}

	for (std::vector<std::pair<size_t, TilepartSpecial>>::const_iterator
			i  = _specialTypes.begin();
			i != _specialTypes.end();
			++i)
	{
		dataSet->getRecords()->at(i->first)->setSpecialType(static_cast<TilepartSpecial>(i->second));
	}

	for (std::vector<std::pair<size_t, int>>::const_iterator
			i  = _scanG.begin();
			i != _scanG.end();
			++i)
	{
		dataSet->getRecords()->at(i->first)->setMiniMapIndex(static_cast<unsigned short>(i->second));
	}

	for (std::vector<std::pair<size_t, int>>::const_iterator
			i  = _psychedelic.begin();
			i != _psychedelic.end();
			++i)
	{
		dataSet->getRecords()->at(i->first)->setPsychedelic(i->second);
	}
}

}
