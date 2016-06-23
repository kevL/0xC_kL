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

//#include <SDL_endian.h>

#include "MapData.h"

#include "../Engine/CrossPlatform.h"
#include "../Engine/Exception.h"
#include "../Engine/Game.h"
#include "../Engine/SurfaceSet.h"

#include "../Resource/ResourcePack.h"


namespace OpenXcom
{

MapData // static.
	* MapDataSet::_blankTile (nullptr),
	* MapDataSet::_scorchedTile (nullptr);


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
 * @return, the size in quantity of records
 */
size_t MapDataSet::getSize() const
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
	if (_loaded == true) // prevents loading twice
		return;

	_loaded = true;

#pragma pack(push, 1)	// align the incoming MCD-values with 1-byte boundaries
	struct MCD			// This struct helps to read the .MCD file format.
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
		unsigned char	u39;
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

	// Load all terrain-data in from an MCD file.
	std::string file ("TERRAIN/" + _type + ".MCD");
	std::ifstream ifstr ( // load file
						CrossPlatform::getDataFile(file).c_str(),
						std::ios::in | std::ios::binary);
	if (ifstr.fail() == true)
	{
		throw Exception("MapDataSet::loadData() " + file + " not found");
	}


	MapData* to;
	int objNumber (0);
	while (ifstr.read(
					(char*)&mcd,
					sizeof(MCD)))
	{
		to = new MapData(this);
		_records.push_back(to);

		// Set all the terrain/tile-part properties:
		for (size_t
				i = 0u;
				i != 8u; // sprite-frames (Battlescape tactical animations)
				++i)
		{
			to->setSprite(i, (int)mcd.Frame[i]);
		}

		to->setPartType(static_cast<MapDataType>(mcd.Tile_Type));
		to->setTileType(static_cast<TileType>(mcd.Target_Type));
		to->setOffsetY((int)mcd.P_Level);
		to->setTUCosts(
				(int)mcd.TU_Walk,
				(int)mcd.TU_Fly,
				(int)mcd.TU_Slide);
		to->setFlags(
				mcd.UFO_Door != 0,
				mcd.Stop_LOS != 0,
				mcd.No_Floor != 0,
				(int)mcd.Big_Wall,
				mcd.Gravlift != 0,
				mcd.Door != 0,
				mcd.Block_Fire != 0,
				mcd.Block_Smoke != 0,
				mcd.Xcom_Base != 0);
		to->setTerrainLevel((int)mcd.T_Level);
		to->setFootstepSound((int)mcd.Footstep);
		to->setAltMCD((int)(mcd.Alt_MCD));
		to->setDieMCD((int)(mcd.Die_MCD));
		to->setBlock(
				(int)mcd.Light_Block,
				(int)mcd.Stop_LOS,
				(int)mcd.HE_Block,
				(int)mcd.Block_Smoke,
				(int)mcd.Flammable,
				(int)mcd.HE_Block);
		to->setLightSource((int)mcd.Light_Source);
		to->setArmor((int)mcd.Armor);
		to->setFlammable((int)mcd.Flammable);
		to->setFuel((int)mcd.Fuel);
		to->setExplosiveType((int)mcd.HE_Type);
		to->setExplosive((int)mcd.HE_Strength);

		mcd.ScanG = SDL_SwapLE16(mcd.ScanG);
		to->setMiniMapIndex(mcd.ScanG);

		for (size_t
				loft = 0u;
				loft != 12u; // LoFT layers (each layer is doubled to give a total height of 24 voxels)
				++loft)
		{
			to->setLoftId(
						static_cast<size_t>(mcd.LOFT[loft]),
						static_cast<size_t>(loft));
		}

		// Store the 2 tiles of 'blanks' as static so they are accessible to all MapData-instantiations.
		if ((objNumber == 0 || objNumber == 1)
			&& _type.compare("BLANKS") == 0)
		{
			switch (objNumber)
			{
				case 0:
					MapDataSet::_blankTile = to;
					break;
				case 1:
					MapDataSet::_scorchedTile = to;
			}
		}
		++objNumber;
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
			if ((*i)->isGravLift() == false)
				(*i)->setStopLOS();
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
					(char*)&value,
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
 * Gets this part's blank-floor part.
 * @return, pointer to a blank-floor part
 */
MapData* MapDataSet::getBlankFloorTile() // static.
{
	return MapDataSet::_blankTile;
}

/**
 * Gets this part's scorched-earth part.
 * @return, pointer to scorched-earth part
 */
MapData* MapDataSet::getScorchedEarthTile() // static.
{
	return MapDataSet::_scorchedTile;
}

}
