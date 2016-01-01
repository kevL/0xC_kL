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

#include "MapData.h"

#include "MapDataSet.h"

//#include "../Engine/Logger.h"


namespace OpenXcom
{

/**
 * Creates a new MapData tile-part.
 * @note yes, This is a tile-part.
 * @param dataSet - pointer to the MapDataSet this tile-part belongs to
 */
MapData::MapData(MapDataSet* const dataSet)
	:
		_dataSet(dataSet),
		_specialType(TILE),
		_isUfoDoor(false),
		_stopLOS(false),
		_isNoFloor(false),
		_isGravLift(false),
		_isDoor(false),
		_blockFire(false),
		_blockSmoke(false),
		_baseModule(false),
		_yOffset(0),
		_tuWalk(0),
		_tuSlide(0),
		_tuFly(0),
		_terrainLevel(0),
		_footstepSound(0),
		_dieMCD(0),
		_altMCD(0),
		_partType(O_FLOOR),
		_lightSource(0),
		_armor(0),
		_flammable(0),
		_fuel(0),
		_explosive(0),
		_explosiveType(DT_NONE),
		_bigWall(BIGWALL_NONE),
		_miniMapIndex(0),
		_isPsychedelic(0)
{
	//Log(LOG_INFO) << "MapData cTor dataSet = " << _dataSet->getType();
	std::fill_n(_sprite, 8,0);
	std::fill_n(_block, 6,0);
	std::fill_n(_loftId, 12,0);
}

/**
 * Destroys the MapData.
 */
MapData::~MapData()
{
	//Log(LOG_INFO) << "MapData dTor dataSet = " << _dataSet->getType();
}

/**
 * Gets the MapDataSet/tileset this tile-part belongs to.
 * @return, pointer to MapDataSet
 */
MapDataSet* MapData::getDataset() const
{
	return _dataSet;
}

/**
 * Gets the sprite index.
 * @param aniFrame - animation frame 0-7
 * @return, the original sprite index
 */
int MapData::getSprite(int aniFrame) const
{
	return _sprite[static_cast<size_t>(aniFrame)];
}

/**
 * Sets the sprite index for a certain frame.
 * @param aniFrame	- animation frame
 * @param id		- the sprite index in the surfaceset of the mapdataset
 */
void MapData::setSprite(
		size_t aniFrame,
		int id)
{
	_sprite[aniFrame] = id;
}

/**
 * Gets whether this is an animated ufo door.
 * @return, true if this is an animated ufo door
 */
bool MapData::isUfoDoor() const
{
	return _isUfoDoor;
}

/**
 * Gets whether this stops LoS.
 * @return, true if this stops LoS
 */
bool MapData::stopLOS() const
{
	return _stopLOS;
}

/**
 * Gets whether this is a floor.
 * @return, true if this is not a floor
 */
bool MapData::isNoFloor() const
{
	return _isNoFloor;
}

/**
 * Gets whether this is a big wall that blocks all surrounding diagonal paths.
 * @note Return value key:
 * 0: not a bigWall
 * 1: regular bigWall
 * 2: allows movement in ne/sw direction
 * 3: allows movement in nw/se direction
 * 4: acts as a west wall
 * 5: acts as a north wall
 * 6: acts as an east wall
 * 7: acts as a south wall
 * 8: acts as a south and east wall
 * 9: acts as a north and west wall
 * @return, BigwallType (MapData.h)
 */
BigwallType MapData::getBigwall() const
{
	return _bigWall;
}

/**
 * Gets whether this is a normal door.
 * @return, true if this is a normal door
 */
bool MapData::isDoor() const
{
	return _isDoor;
}

/**
 * Gets whether this is a grav lift.
 * @return, true if this is a grav lift
 */
bool MapData::isGravLift() const
{
	return _isGravLift;
}

/**
 * Gets whether this tile part blocks smoke.
 * @return, true if it blocks smoke
 */
bool MapData::blockSmoke() const
{
	return _blockSmoke;
}

/**
 * Gets whether this tile part blocks fire.
 * @return, true if it blocks fire
 */
bool MapData::blockFire() const
{
	return _blockFire;
}

/**
 * Sets whether this tile part stops LoS.
 * @return, true if stops LoS (default true)
 */
void MapData::setStopLOS(bool stopLOS)
{
	_stopLOS = stopLOS;
	_block[1] = stopLOS ? 100 : 0;
}

/**
 * Sets a bunch of flags.
 * @param isUfoDoor		- true if this is a ufo door
 * @param stopLOS		- true if this stops line of sight
 * @param isNoFloor		- true if this is *not* a floor
 * @param bigWall		- type of a bigWall
 * @param isGravLift	- true if this is a grav lift
 * @param isDoor		- true if this is a normal door
 * @param blockFire		- true if this blocks fire
 * @param blockSmoke	- true if this blocks smoke
 * @param baseModule	- true if this is a base module (objective) item
 */
void MapData::setFlags(
		bool isUfoDoor,
		bool stopLOS,
		bool isNoFloor,
		int  bigWall,
		bool isGravLift,
		bool isDoor,
		bool blockFire,
		bool blockSmoke,
		bool baseModule)
{
	_isUfoDoor = isUfoDoor;
	_stopLOS = stopLOS;
	_isNoFloor = isNoFloor;
	_bigWall = static_cast<BigwallType>(bigWall);
	_isGravLift = isGravLift;
	_isDoor = isDoor;
	_blockFire = blockFire;
	_blockSmoke = blockSmoke;
	_baseModule = baseModule;
}

/**
 * Gets the amount of blockage of a certain type.
 * @param dType - DamageType (RuleItem.h)
 * @return, the blockage (0-255)
 */
int MapData::getBlock(DamageType dType) const
{
	switch (dType)
	{
/*		case DT_NONE:	return _block[1];
		case DT_HE:		return _block[2];
		case DT_SMOKE:	return _block[3];
		case DT_IN:		return _block[4];
		case DT_STUN:	return _block[5]; */
											// see setBlock() below.
		case DT_NONE:	return _block[1];	// stop LoS: [0 or 100], was [0 or 255]
		case DT_HE:
		case DT_IN:
		case DT_STUN:	return _block[2];	// HE block [int]
		case DT_SMOKE:	return _block[3];	// block smoke: try (bool), was [0 or 256]
	}

	return 0;
}

/**
 * Sets the amount of blockage for all types.
 * @param lightBlock	- light blockage			- Light_Block
 * @param visionBlock	- vision blockage			- Stop_LOS
 * @param heBlock		- high explosive blockage	- HE_Block
 * @param smokeBlock	- smoke blockage			- Block_Smoke
 * @param fireBlock		- fire blockage				- Flammable (lower = more flammable)
 * @param gasBlock		- gas blockage				- HE_Block
 */
void MapData::setBlock(
		int lightBlock,
		int visionBlock,
		int heBlock,
		int smokeBlock,
		int fireBlock,
		int gasBlock)
{
/*	_block[0] = lightBlock; // not used...
	_block[1] = visionBlock == 1? 255: 0;
	_block[2] = heBlock;
	_block[3] = smokeBlock == 1? 255: 0;
	_block[4] = fireBlock == 1? 255: 0;
	_block[5] = gasBlock == 1? 255: 0; */

	_block[0] = lightBlock; // not used
//	_block[1] = visionBlock; // kL
//	_block[1] = visionBlock == 1? 255: 0; // <- why? kL_note. haha
	_block[1] = visionBlock == 1 ? 100 : 0; // kL
		// stopLoS==true needs to be a significantly large integer (only about 10+ really)
		// so that if a directionally opposite Field of View check includes a "-1",
		// meaning block by bigWall or other content-object, the result is not reduced
		// to zero (no block at all) when added to regular stopLoS by a standard wall.
		//
		// It would be unnecessary to use that jigger-pokery if TileEngine::
		// horizontalBlockage() & blockage() were coded differently [verticalBlockage()
		// too, perhaps]
	_block[2] = heBlock;
//	_block[3] = smokeBlock == 1? 256: 0; // <- why? kL_note. I basically use visionBlock for smoke ....
	_block[3] = smokeBlock;
	_block[4] = fireBlock; // this is Flammable, NOT Block_Fire.
	_block[5] = gasBlock;
}

/**
 * Sets the amount of HE blockage.
 * @param heBlock - the high explosive blockage
 */
void MapData::setHEBlock(int heBlock)
{
	_block[2] = heBlock;
}

/**
 * Gets the Y offset for drawing.
 * @return, the height in pixels
 */
int MapData::getYOffset() const
{
	return _yOffset;
}

/**
 * Sets the offset on the Y axis for drawing this tile-part.
 * @param value - the offset
 */
void MapData::setYOffset(int value)
{
	_yOffset = value;
}

/**
 * Gets the MapDataType/part-type of this tile-part.
 * @return, the tile-part type (0-3)
 */
MapDataType MapData::getPartType() const
{
	return _partType;
}

/**
 * Sets the MapDataType/part-type of this tile-part.
 * @param type - the tile-part type (0-3)
 */
void MapData::setPartType(MapDataType type)
{
	_partType = type;
}

/**
 * Gets the SpecialTileType.
 * @return, the SpecialTileType
 */
SpecialTileType MapData::getSpecialType() const
{
	return _specialType;
}

/**
 * Sets the SpecialTileType.
 * @param value	- SpecialTileType (MapData.h)
 */
void MapData::setSpecialType(SpecialTileType type)
{
	_specialType = type;
}

/**
 * Gets the TU cost to move over/through the tile-part.
 * @param moveType - the movement type
 * @return, the TU cost
 */
int MapData::getTuCostPart(MovementType moveType) const
{
	switch (moveType)
	{
		case MT_WALK:	return _tuWalk;
		case MT_SLIDE:	return _tuSlide;
		case MT_FLY:	return _tuFly;
	}

	return 0;
}

/**
 * Sets the TU cost to move over the tile-part.
 * @param walk	- the walking TU cost
 * @param fly	- the flying TU cost
 * @param slide	- the sliding TU cost
 */
void MapData::setTUCosts(
		int walk,
		int fly,
		int slide)
{
	_tuWalk = walk;
	_tuFly = fly;
	_tuSlide = slide;
}

/**
 * Adds to the graphical Y-offset of units or objects on this tile-part.
 * @return, Y offset
 */
int MapData::getTerrainLevel() const
{
	return _terrainLevel;
}

/**
 * Sets the Y-offset for units or objects on this tile-part.
 * @param value - Y offset
 */
void MapData::setTerrainLevel(int value)
{
	_terrainLevel = value;
}

/**
 * Gets the index to this tile-part's footstep sound.
 * @return, the sound ID
 */
int MapData::getFootstepSound() const
{
	return _footstepSound;
}

/**
 * Sets the index to this tile-part's footstep sound.
 * @param value - the sound ID
 */
void MapData::setFootstepSound(int value)
{
	_footstepSound = value;
}

/**
 * Gets the alternate tile-part ID.
 * @return, the alternate tile-part ID
 */
int MapData::getAltMCD() const
{
	return _altMCD;
}

/**
 * Sets the alternate tile-part ID.
 * @param value - the alternate tile-part ID
 */
void MapData::setAltMCD(int value)
{
	_altMCD = value;
}

/**
 * Gets the dead tile-part ID.
 * @return, the dead tile-part ID
 */
int MapData::getDieMCD() const
{
	return _dieMCD;
}

/**
 * Sets the dead tile-part ID.
 * @param value - the dead tile-part ID
 */
void MapData::setDieMCD(int value)
{
	_dieMCD = value;
}

/**
 * Gets the amount of light this tile-part emits.
 * @return, the amount of light
 */
int MapData::getLightSource() const
{
	if (_lightSource == 1)	// lamp posts have 1
		return 15;			// but they should emit more light

	return _lightSource - 1;
}

/**
 * Sets the amount of light this tile-part emits.
 * @param value - the amount of light
 */
void MapData::setLightSource(int value)
{
	_lightSource = value;
}

/**
 * Gets the amount of armor.
 * @note Total hitpoints of the tile-part before destroyed.
 * @return, the amount of armor
 */
int MapData::getArmor() const
{
	return _armor;
}

/**
 * Sets the amount of armor.
 * @note Total hitpoints of the tile-part before destroyed.
 * @param value - the amount of armor
 */
void MapData::setArmor(int value)
{
	_armor = value;
}

/**
 * Gets the amount of flammable (how flammable this tile-part is).
 * @return, the amount of flammable
 */
int MapData::getFlammable() const
{
	return _flammable;
}

/**
 * Sets the amount of flammable (how flammable this tile-part is).
 * @param value - the amount of flammable
 */
void MapData::setFlammable(int value)
{
	_flammable = value;
}

/**
 * Gets the amount of fuel.
 * @return, the amount of fuel
 */
int MapData::getFuel() const
{
	return _fuel;
}

/**
 * Sets the amount of fuel.
 * @param value - the amount of fuel
 */
void MapData::setFuel(int value)
{
	_fuel = value;
}

/**
 * Gets the loft index for a certain layer.
 * @param layer - the layer
 * @return, the LOFT index
 */
size_t MapData::getLoftId(size_t layer) const
{
	return _loftId[layer];
}

/**
 * Sets the loft index for a certain layer.
 * @param loft	- the LOFT index
 * @param layer	- the layer (0..11)
 */
void MapData::setLoftId(
		size_t loft,
		size_t layer)
{
	_loftId[layer] = loft;
}

/**
 * Gets the amount of explosive.
 * @return, the amount of explosive
 */
int MapData::getExplosive() const
{
	return _explosive;
}

/**
 * Sets the amount of explosive.
 * @param value - the amount of explosive
 */
void MapData::setExplosive(int value)
{
	_explosive = value;
}

/**
 * Gets the type of explosive.
 * @return, the amount of explosive
 */
DamageType MapData::getExplosiveType() const
{
	return _explosiveType;
}

/**
 * Sets the type of explosive.
 * @param value - the type of explosive
 */
void MapData::setExplosiveType(int value)
{
	switch (value) // account for (HE_Type)MCD vs. (RuleItem.h enum)DamageType mismatch
	{
		case 0: _explosiveType = DT_HE;		return;
		default:
		case 1: _explosiveType = DT_SMOKE;	return;
		case 5: _explosiveType = DT_IN;		return;
		case 6: _explosiveType = DT_STUN;
	}
}

/**
 * Sets the SCANG.DAT index for the minimap.
 * @param i - the minimap index
 */
void MapData::setMiniMapIndex(unsigned short id)
{
	_miniMapIndex = static_cast<int>(id);
}

/**
 * Gets the SCANG.DAT index for the minimap.
 * @return, the minimap index
 */
int MapData::getMiniMapIndex() const
{
	return _miniMapIndex;
}

/**
 * Sets the bigWall value.
 * @param bigWall - the new bigWall value (MapData.h)
 */
void MapData::setBigWall(int bigWall)
{
	_bigWall = static_cast<BigwallType>(bigWall);
}

/**
 * Sets the TUWalk value.
 * @param TUWalk - the new TUWalk value
 */
void MapData::setTUWalk(const int TUWalk)
{
	_tuWalk = TUWalk;
}

/**
 * Sets the TUFly value.
 * @param TUFly - the new TUFly value
 */
void MapData::setTUFly(const int TUFly)
{
	_tuFly = TUFly;
}

/**
 * Sets the TUSlide value.
 * @param TUSlide - the new TUSlide value
 */
void MapData::setTUSlide(const int TUSlide)
{
	_tuSlide = TUSlide;
}

/**
 * Sets the "no floor" flag.
 * @param isNoFloor - true if the tile has no floor part
 */
void MapData::setNoFloor(bool isNoFloor)
{
	_isNoFloor = isNoFloor;
}

/**
 * Check if this is an xcom base tile-part.
 * @return, true if it is a base tile-part
 */
bool MapData::isBaseModule() const
{
	return _baseModule;
}

/**
 * Sets if this tilepart is psychedelic.
 * @param psycho - true if psycho
 */
void MapData::setPsychedelic(int psycho)
{
	_isPsychedelic = psycho;
}

/**
 * Gets if this tilepart is psychedelic.
 * @return, true if psycho
 */
int MapData::getPsychedelic() const
{
	return _isPsychedelic;
}

}
