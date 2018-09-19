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

#include "MapData.h"

#include "MapDataSet.h"

//#include "../Engine/Logger.h"


namespace OpenXcom
{

/**
 * Creates the MapData tile-part.
 * @note yes, This is a tile-part.
 * @param dataSet - pointer to the MapDataSet (aka MCD) this tile-part belongs to
 */
MapData::MapData(MapDataSet* const dataSet)
	:
		_dataSet(dataSet),
		_specialType(TILE),
		_isDoor(false),
		_hingeDoor(false),
		_slideDoor(false),
		_stopLOS(false),
		_noFloor(false),
		_gravLift(false),
		_disallowFire(false),
		_disallowSmoke(false),
		_baseObject(false),
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
	std::fill_n(_sprite,  8u,0u);
	std::fill_n(_block,   6u,0u);
	std::fill_n(_loftId, 12u,0u);
}

/**
 * Destroys this MapData.
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
 * @param aniCycle - animation frame 0-7
 * @return, the original sprite index
 */
int MapData::getSprite(int aniCycle) const
{
	return _sprite[static_cast<size_t>(aniCycle)];
}

/**
 * Sets the sprite index for a certain frame.
 * @param aniCycle	- animation frame
 * @param id		- the sprite index in the surfaceset of the mapdataset
 */
void MapData::setSprite(
		size_t aniCycle,
		int id)
{
	_sprite[aniCycle] = id;
}

/**
 * Gets if this tile-part is either a normal door or a ufo-door.
 * @return, true if door
 */
bool MapData::isDoor() const
{
	return _isDoor;
}

/**
 * Gets whether this tile-part is a normal door.
 * @return, true if normal door
 */
bool MapData::isHingeDoor() const
{
	return _hingeDoor;
}

/**
 * Gets whether this tile-part is an animated ufo-door.
 * @return, true if ufo-door
 */
bool MapData::isSlideDoor() const
{
	return _slideDoor;
}

/**
 * Gets whether this tile-part stops LoS.
 * @return, true if stops LoS
 */
bool MapData::stopLOS() const
{
	return _stopLOS;
}

/**
 * Gets whether this tile-part is a floor.
 * @return, true if a floor
 */
bool MapData::isNoFloor() const
{
	return _noFloor;
}

/**
 * Gets whether this tile-part is a bigwall that blocks surrounding diagonal
 * paths.
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
 * 9: acts as a north and west wall // NOTE: Not used in UFO.
 * @return, BigwallType (MapData.h)
 */
BigwallType MapData::getBigwall() const
{
	return _bigWall;
}

/**
 * Gets whether this tile-part is a grav lift.
 * @return, true if a grav lift
 */
bool MapData::isGravLift() const
{
	return _gravLift;
}

/**
 * Gets whether this tile-part blocks smoke.
 * @return, true if blocks smoke
 */
bool MapData::blocksSmoke() const
{
	return _disallowSmoke;
}

/**
 * Gets whether this tile-part blocks fire.
 * @return, true if blocks fire
 */
bool MapData::blocksFire() const
{
	return _disallowFire;
}

/**
 * Sets a bunch of flags.
 * @param slideDoor		- true if a ufo door
 * @param stopLOS		- true if stops line of sight
 * @param noFloor		- true if *not* a floor
 * @param bigWall		- type if a bigWall
 * @param gravLift		- true if a grav lift
 * @param hingeDoor		- true if a normal door
 * @param disallowFire	- true if blocks fire
 * @param disallowSmoke	- true if blocks smoke
 * @param baseObject	- true if a base-object/aLien-objective
 */
void MapData::setFlags(
		bool slideDoor,
		bool stopLOS,
		bool noFloor,
		int  bigWall,
		bool gravLift,
		bool hingeDoor,
		bool disallowFire,
		bool disallowSmoke,
		bool baseObject)
{
	_slideDoor     = slideDoor;
	_stopLOS       = stopLOS;
	_noFloor       = noFloor;
	_bigWall       = static_cast<BigwallType>(bigWall);
	_gravLift      = gravLift;
	_hingeDoor     = hingeDoor;
	_disallowFire  = disallowFire;
	_disallowSmoke = disallowSmoke;
	_baseObject    = baseObject;

	_isDoor = _slideDoor
		   || _hingeDoor;
}

/**
 * Gets the quantity of blockage of a specified type.
 * @param dType - DamageType (RuleItem.h)
 * @return, the blockage (0-255)
 */
int MapData::getBlock(DamageType dType) const
{
	switch (dType)
	{
//		case DT_NONE:  return _block[1];
//		case DT_HE:    return _block[2];
//		case DT_SMOKE: return _block[3];
//		case DT_IN:    return _block[4];
//		case DT_STUN:  return _block[5];
											// see setBlock() below_
		case DT_NONE:  return _block[1u];	// stop LoS: [0 or 100], was [0 or 255]
		case DT_HE:
		case DT_IN:
		case DT_STUN:  return _block[2u];	// HE block [int] // TODO: Just fix all this crap.
		case DT_SMOKE: return _block[3u];	// block smoke: try (bool), was [0 or 256]
	}
	return 0;
}

/**
 * Sets the quantity of blockage for all types.
 * @param light		- light blockage			- Light_Block
 * @param vision	- vision blockage			- Stop_LOS
 * @param he		- high explosive blockage	- HE_Block
 * @param smoke		- smoke blockage			- Block_Smoke
 * @param fire		- fire blockage				- Flammable (lower = more flammable)
 * @param gas		- gas blockage				- HE_Block
 */
void MapData::setBlock(
		int light,
		int vision,
		int he,
		int smoke,
		int fire,
		int gas)
{
	_block[0u] = light;					// not used.
	_block[1u] = vision == 1 ? 100 : 0;	// stopLoS==true needs to be a significantly large integer (only about 10+ really)
										// so that if a directionally opposite Field of View check includes a "-1",
										// meaning block by bigWall or other content-object, the result is not reduced
										// to zero (no block at all) when added to regular stopLoS by a standard wall.
										//
										// It would be unnecessary to use that jigger-pokery if TileEngine::horizontalBlockage()
										// and TileEngine::blockage() were coded differently - verticalBlockage() too, perhaps.
	_block[2u] = he;
	_block[3u] = smoke;					// this is the same as the _blockSmoke flag.
	_block[4u] = fire;					// IMPORTANT: this is Flammable, NOT Block_Fire per se.
	_block[5u] = gas;					// probably not used.
}

/**
 * Sets default blockage values if none are assigned in an MCD.
 * @param he - armor value of the tilepart
 */
void MapData::setDefaultBlock(int he)
{
	_block[0u] = 1;
	_block[1u] = 1;
	_block[2u] = he;
	_block[3u] = 1;
	_block[4u] = 1;
	_block[5u] = 1;
}

/**
 * Sets whether this tile-part stops LoS.
 * @param, true if stops LoS
 */
void MapData::setStopLOS(bool stopLOS)
{
	_stopLOS = stopLOS;
	_block[1u] = (stopLOS == true) ? 100 : 0;
}

/**
 * Sets the amount of HE blockage.
 * @param heBlock - the high explosive blockage
 */
void MapData::setHEBlock(int heBlock)
{
	_block[2u] = heBlock;
}

/**
 * Gets the offset on the y-axis for drawing this tile-part.
 * @return, the offset in pixels
 */
int MapData::getOffsetY() const
{
	return _yOffset;
}

/**
 * Sets the offset on the y-axis for drawing this tile-part.
 * @param offset - the offset in pixels
 */
void MapData::setOffsetY(int offset)
{
	_yOffset = offset;
}

/**
 * Gets the MapDataType (part-type) of this tile-part.
 * @return, the tile-part type (0-3)
 */
MapDataType MapData::getPartType() const
{
	return _partType;
}

/**
 * Sets the MapDataType (part-type) of this tile-part.
 * @param type - the tile-part type (0-3)
 */
void MapData::setPartType(MapDataType type)
{
	_partType = type;
}

/**
 * Gets this tile-part's TilepartSpecial.
 * @return, the TilepartSpecial (MapData.h)
 */
TilepartSpecial MapData::getSpecialType() const
{
	return _specialType;
}

/**
 * Sets this tile-part's TilepartSpecial.
 * @param value	- a TilepartSpecial (MapData.h)
 */
void MapData::setSpecialType(TilepartSpecial specialType)
{
	_specialType = specialType;
}

/**
 * Gets the TU-cost to move over/through this tile-part.
 * @param type - the MoveType (MapData.h)
 * @return, the TU-cost
 */
int MapData::getTuCostPart(MoveType type) const
{
	switch (type)
	{
		case MT_WALK:  return _tuWalk;
		case MT_SLIDE: return _tuSlide;
		case MT_FLOAT: // wft.
		case MT_FLY:   return _tuFly;
	}
	return 0;
}

/**
 * Sets the TU-cost to move over this tile-part.
 * @param walk	- the walking TU-cost
 * @param slide	- the sliding TU-cost
 * @param fly	- the flying TU-cost
 */
void MapData::setTuCosts(
		int walk,
		int slide,
		int fly)
{
	_tuWalk  = walk;
	_tuSlide = slide;
	_tuFly   = fly;
}

/**
 * Adds to the graphical Y-offset of units or objects on this tile-part.
 * @return, y-offset in pixels
 */
int MapData::getTerrainLevel() const
{
	return _terrainLevel;
}

/**
 * Sets the Y-offset for units or objects on this tile-part.
 * @param offset - y-offset in pixels
 */
void MapData::setTerrainLevel(int offset)
{
	_terrainLevel = offset;
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
//	if (_lightSource == 1)	// lamp posts have 1
//		return 15;			// but they should emit more light -> Fixed in MCD.

	return _lightSource;// - 1;
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
 * @note Total hitpoints of the tile-part before destroyed. "255" shall be
 * considered indestructible.
 * @return, the amount of armor
 */
int MapData::getArmorPoints() const
{
	return _armor;
}

/**
 * Sets the amount of armor.
 * @note Total hitpoints of the tile-part before destroyed. "255" shall be
 * considered indestructible.
 * @param value - the amount of armor
 */
void MapData::setArmorPoints(int value)
{
	_armor = value;
}

/**
 * Gets the amount of flammable (how flammable this tile-part is).
 * @note "255" shall be considered not flammable.
 * @return, the amount of flammable
 */
int MapData::getFlammable() const
{
	return _flammable;
}

/**
 * Sets the amount of flammable (how flammable this tile-part is).
 * @note "255" shall be considered not flammable.
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
 * Gets the LoFT-index of a specified layer.
 * @param layer - the layer (0..11)
 * @return, the LoFT-index
 */
size_t MapData::getLoftId(size_t layer) const
{
	return _loftId[layer];
}

/**
 * Sets the LoFT-index of a specified layer.
 * @param loft	- the LoFT-index
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
 * @param type - the type of explosive
 */
void MapData::setExplosiveType(int type)
{
	switch (type) // account for (HE_Type)MCD vs. (RuleItem.h)DamageType mismatch
	{
		case 0: _explosiveType = DT_HE;
			break;

		default:
		case 1: _explosiveType = DT_SMOKE;
			break;

		case 5: _explosiveType = DT_IN;
			break;

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
 * Sets the TU-walk value.
 * @param tu - the TU value
 */
void MapData::setTuWalk(int tu)
{
	_tuWalk = tu;
}

/**
 * Sets the TU-slide value.
 * @param tu - the TU value
 */
void MapData::setTuSlide(int tu)
{
	_tuSlide = tu;
}

/**
 * Sets the TU-fly value.
 * @param tu - the TU value
 */
void MapData::setTuFly(int tu)
{
	_tuFly = tu;
}

/**
 * Sets the "no floor" flag.
 * @param isNoFloor - true if the tile has no floor part
 */
void MapData::setNoFloor(bool isNoFloor)
{
	_noFloor = isNoFloor;
}

/**
 * Checks if this is an aLien-objective tile-part.
 * @return, true if so
 */
bool MapData::isBaseObject() const
{
	return _baseObject;
}

/**
 * Sets if this tile-part is psychedelic.
 * @param psycho - true if psycho
 */
void MapData::setPsychedelic(int psycho)
{
	_isPsychedelic = psycho;
}

/**
 * Gets if this tile-part is psychedelic.
 * @return, true if psycho
 */
int MapData::getPsychedelic() const
{
	return _isPsychedelic;
}

}
