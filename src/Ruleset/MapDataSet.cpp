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

#include "MapDataSet.h"

#include <fstream>

//#include <SDL/SDL_endian.h>

#include "MapData.h"

#include "../Engine/CrossPlatform.h"
#include "../Engine/Exception.h"
#include "../Engine/Game.h"
#include "../Engine/SurfaceSet.h"

#include "../Resource/ResourcePack.h"


namespace OpenXcom
{

MapData // static.
	* MapDataSet::_floorBlank  (nullptr),
	* MapDataSet::_floorScorch (nullptr);


/**
 * MapDataSet construction.
 * @note This is a tileset.
 * @param type - reference to the type of the MCD
 * @param game - pointer to the Game
 */
MapDataSet::MapDataSet(
		const std::string& type,
		const Game* const game)
	:
		_type(type),
		_game(game),
		_surfaceSet(nullptr),
		_loaded(false)
{}

/**
 * MapDataSet destruction.
 */
MapDataSet::~MapDataSet()
{
	unloadData();
}

/**
 * Loads the MapDataSet from a YAML file.
 * @param node - reference a YAML node
 */
void MapDataSet::load(const YAML::Node& node)
{
	for (YAML::const_iterator
			i = node.begin();
			i != node.end();
			++i)
		_type = i->as<std::string>(_type);
}

/**
 * Gets the MapDataSet type string - eg. "AVENGER"
 * @return, the MapDataSet type
 */
std::string MapDataSet::getType() const
{
	return _type;
}

/**
 * Gets the MapDataSet size.
 * @return, the quantity of records
 */
size_t MapDataSet::getRecordsQty() const
{
	return _records.size();
}

/**
 * Gets the objects in this dataset.
 * @return, pointer to a vector of pointers to MapData objects
 */
std::vector<MapData*>* MapDataSet::getRecords()
{
	return &_records;
}

/**
 * Gets the surfaces in this dataset.
 * @return, pointer to the SurfaceSet graphics
 */
SurfaceSet* MapDataSet::getSurfaceset() const
{
	return _surfaceSet;
}

/**
 * Loads terrain data in XCom format - MCD & PCK files.
 * @sa http://www.ufopaedia.org/index.php?title=MCD
 */
void MapDataSet::loadData()
{
	if (_loaded == false) // prevents loading twice
	{
		_loaded = true;

#pragma pack(push, 1)	// align the incoming MCD-values with 1-byte boundaries
		struct MCD		// This struct helps to read the .MCD file format.
		{
			unsigned char	Frame[8u];
			unsigned char	LOFT[12u];
			unsigned short	ScanG;
			unsigned char	u23;
			unsigned char	u24;
			unsigned char	u25;
			unsigned char	u26;
			unsigned char	u27;
			unsigned char	u28;
			unsigned char	u29;
			unsigned char	u30;
			unsigned char	UFO_Door;
			unsigned char	Stop_LOS;
			unsigned char	No_Floor;
			unsigned char	Big_Wall;
			unsigned char	Gravlift;
			unsigned char	Door;
			unsigned char	Block_Fire;
			unsigned char	Block_Smoke;
			unsigned char	u39; // NOTE: so-called 'Start_Phase' is around here. See MCDEdit v1.17i
			unsigned char	TU_Walk;
			unsigned char	TU_Slide;
			unsigned char	TU_Fly;
			unsigned char	Armor;
			unsigned char	HE_Block;
			unsigned char	Die_MCD;
			unsigned char	Flammable;
			unsigned char	Alt_MCD;
			unsigned char	u48;
			signed char		T_Level;
			unsigned char	P_Level;
			unsigned char	u51;
			unsigned char	Light_Block;
			unsigned char	Footstep;
			unsigned char	Tile_Type;
			unsigned char	HE_Type;
			unsigned char	HE_Strength;
			unsigned char	Smoke_Blockage;
			unsigned char	Fuel;
			unsigned char	Light_Source;
			unsigned char	Target_Type;
			unsigned char	Xcom_Base;
			unsigned char	u62;
		};
#pragma pack(pop) // revert to standard byte-alignment

		MCD mcd;

		// Load all terrain-part-data from the MCD-file '_type'.
		std::string file ("TERRAIN/" + _type + ".MCD");
		std::ifstream ifstr (
						CrossPlatform::getDataFile(file).c_str(),
						std::ios::in | std::ios::binary);
		if (ifstr.fail() == true)
		{
			throw Exception("MapDataSet::loadData() " + file + " not found");
		}


		MapData* part;
		int partId (0); // <- used only for BLANKS tileset.
		while (ifstr.read(
						reinterpret_cast<char*>(&mcd),
						sizeof(MCD)))
		{
			part = new MapData(this);
			_records.push_back(part);

			// Set all the terrain-tilepart properties:
			for (size_t
					i = 0u;
					i != 8u; // sprite-frames (battlescape tactical animations)
					++i)
			{
				part->setSprite(i, static_cast<int>(mcd.Frame[i]));
			}

			part->setPartType(static_cast<MapDataType>(mcd.Tile_Type));
			part->setTileType(static_cast<TileType>(mcd.Target_Type));
			part->setOffsetY(static_cast<int>(mcd.P_Level));
			part->setTuCosts(
					static_cast<int>(mcd.TU_Walk),
					static_cast<int>(mcd.TU_Slide),
					static_cast<int>(mcd.TU_Fly));
			part->setFlags(
					mcd.UFO_Door != 0,
					mcd.Stop_LOS != 0,
					mcd.No_Floor != 0,
					static_cast<int>(mcd.Big_Wall),
					mcd.Gravlift != 0,
					mcd.Door != 0,
					mcd.Block_Fire != 0,
					mcd.Block_Smoke != 0,
					mcd.Xcom_Base != 0);
			part->setTerrainLevel(static_cast<int>(mcd.T_Level));
			part->setFootstepSound(static_cast<int>(mcd.Footstep));
			part->setAltMCD(static_cast<int>(mcd.Alt_MCD));
			part->setDieMCD(static_cast<int>(mcd.Die_MCD));
			part->setBlock(
					static_cast<int>(mcd.Light_Block),
					static_cast<int>(mcd.Stop_LOS),
					static_cast<int>(mcd.HE_Block),
					static_cast<int>(mcd.Block_Smoke),
					static_cast<int>(mcd.Flammable),
					static_cast<int>(mcd.HE_Block));
			part->setLightSource(static_cast<int>(mcd.Light_Source));
			part->setArmor(static_cast<int>(mcd.Armor));
			part->setFlammable(static_cast<int>(mcd.Flammable));
			part->setFuel(static_cast<int>(mcd.Fuel));
			part->setExplosiveType(static_cast<int>(mcd.HE_Type));
			part->setExplosive(static_cast<int>(mcd.HE_Strength));

			mcd.ScanG = SDL_SwapLE16(mcd.ScanG);
			part->setMiniMapIndex(mcd.ScanG);

			for (size_t
					loft = 0u;
					loft != 12u; // LoFT layers (each layer is doubled to give a total height of 24 voxels)
					++loft)
			{
				part->setLoftId(
							static_cast<size_t>(mcd.LOFT[loft]),
							loft);
			}

			// Load the two tiles of the BLANKS dataset as static so that they
			// are accessible to all MapDataSet-instantiations.
			if (_type.compare("BLANKS") == 0)
			{
				switch (partId)
				{
					case 0: MapDataSet::_floorBlank  = part; break; // not used.
					case 1: MapDataSet::_floorScorch = part;
				}
			}
			++partId;
		}


		if (ifstr.eof() == false)
		{
			throw Exception("MapDataSet::loadData() Invalid MCD file");
		}

		ifstr.close();

		// process the MapDataSet to put 'block' values on floortiles (as they don't exist in UFO::Orig)
		for (std::vector<MapData*>::const_iterator
				i = _records.begin();
				i != _records.end();
				++i)
		{
			if ((*i)->getPartType() == O_FLOOR && (*i)->getBlock(DT_HE) == 0)
			{
				const int armor ((*i)->getArmor());
				(*i)->setBlock(
							1,		// light
							1,		// LoS
							armor,	// HE
							1,		// smoke
							1,		// fire
							1);		// gas

				if ((*i)->getDieMCD() != 0)
					_records.at(static_cast<size_t>((*i)->getDieMCD()))->setBlock(
																				1,
																				1,
																				armor,
																				1,
																				1,
																				1);
				if ((*i)->isGravLift() == false) (*i)->setStopLOS();
			}
		}

		// Load terrain sprites/surfaces/PCK files into a SurfaceSet.
		// Let any defined extra-sprites overrule the stock terrain-sprites.
		SurfaceSet* const srt (_game->getResourcePack()->getSurfaceSet(_type + ".PCK"));
		if (srt != nullptr)
			_surfaceSet = srt;
		else
		{
			_surfaceSet = new SurfaceSet(32,40);
			_surfaceSet->loadPck(
							CrossPlatform::getDataFile("TERRAIN/" + _type + ".PCK"),
							CrossPlatform::getDataFile("TERRAIN/" + _type + ".TAB"));
		}
	}
}

/**
 * Unloads the terrain data.
 */
void MapDataSet::unloadData()
{
	if (_loaded == true)
	{
		_loaded = false;

		for (std::vector<MapData*>::const_iterator
				i = _records.begin();
				i != _records.end();
				)
		{
			delete *i;
			i = _records.erase(i);
		}

		// But don't delete the extraSprites for terrain!!!
		if (_game->getResourcePack()->getSurfaceSet(_type + ".PCK") == nullptr)
			delete _surfaceSet;
	}
}

/**
 * Loads the LOFTEMPS.DAT into the ruleset voxeldata.
 * @param file		- reference to the filename of the DAT file
 * @param voxelData	- pointer to a vector of voxelDatums
 */
void MapDataSet::loadLoft( // static.
		const std::string& file,
		std::vector<Uint16>* const voxelData)
{
	std::ifstream ifstr (file.c_str(), std::ios::in | std::ios::binary);
	if (ifstr.fail() == true)
	{
		throw Exception("MapDataSet::loadData() " + file + " not found");
	}

	Uint16 value;

	while (ifstr.read(
					reinterpret_cast<char*>(&value),
					sizeof(value)))
	{
		value = SDL_SwapLE16(value);
		voxelData->push_back(value);
	}

	if (ifstr.eof() == false)
	{
		throw Exception("MapDataSet::loadData() Invalid LOFTEMPS");
	}

	ifstr.close();
}

/**
 * Gets the universal blank-floor part.
 * @note Not used.
 * @return, pointer to blank-floor part
 *
MapData* MapDataSet::getBlankFloor() // static.
{
	return MapDataSet::_floorBlank;
} */

/**
 * Gets the universal scorched-earth part.
 * @return, pointer to scorched-earth part
 */
MapData* MapDataSet::getScorchedEarth() // static.
{
	return MapDataSet::_floorScorch;
}

}
