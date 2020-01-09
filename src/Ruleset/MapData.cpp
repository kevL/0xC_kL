/*
 * Copyright 2010-2020 OpenXcom Developers.
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
 * Creates the MapData tilepart.
 * @note yes, This is a tilepart.
 * @param dataSet - pointer to the MapDataSet (aka MCD) this tilepart belongs to
 */
MapData::MapData(MapDataSet* const dataSet)
	:
		_dataSet(dataSet),
		_specialType(TILE),
		_door(false),
		_hingeDoor(false),
		_slideDoor(false),
		_stopLos(false),
		_noFloor(false),
		_gravLift(false),
		_disallowFire(false),
		_disallowSmoke(false),
		_baseObject(false),
		_yOffset(0),
		_tuWalk(0),
		_tuSlide(0),
		_tuFly(0),
		_level(0),
		_sound(0),
		_dieId(0),
		_altId(0),
		_partType(O_FLOOR),
		_light(0),
		_armor(0),
		_flammable(0),
		_fuel(0),
		_explosive(0),
		_explosiveType(DT_NONE),
		_bigWall(BIGWALL_NONE),
		_miniId(0),
		_psychedelic(0)
{
	//Log(LOG_INFO) << "MapData cTor dataSet = " << _dataSet->getType();
	std::fill_n(_sprites,  8u,0u);
	std::fill_n(_blocks,   6u,0u);
	std::fill_n(_loftIds, 12u,0u);
}

/**
 * Destroys this MapData.
 */
MapData::~MapData()
{
	//Log(LOG_INFO) << "MapData dTor dataSet = " << _dataSet->getType();
}

/**
 * Gets the MapDataSet/tileset this tilepart belongs to.
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
	return _sprites[static_cast<size_t>(aniCycle)];
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
	_sprites[aniCycle] = id;
}

/**
 * Gets if this tilepart is either a normal door or a ufo-door.
 * @return, true if door
 */
bool MapData::isDoor() const
{
	return _door;
}

/**
 * Gets whether this tilepart is a normal door.
 * @return, true if normal door
 */
bool MapData::isHingeDoor() const
{
	return _hingeDoor;
}

/**
 * Gets whether this tilepart is an animated ufo-door.
 * @return, true if ufo-door
 */
bool MapData::isSlideDoor() const
{
	return _slideDoor;
}

/**
 * Gets whether this tilepart stops LoS.
 * @return, true if stops LoS
 */
bool MapData::stopsLos() const
{
	return _stopLos;
}

/**
 * Sets whether this tilepart acts as (not) a floor.
 * @param noFloor - true if the tile does not act as a floor
 */
void MapData::setNoFloor(bool noFloor)
{
	_noFloor = noFloor;
}

/**
 * Gets whether this tilepart acts as (not) a floor.
 * @return, true if the tile does not act as a floor
 */
bool MapData::isNoFloor() const
{
	return _noFloor;
}

/**
 * Gets whether this tilepart is a bigwall that blocks surrounding diagonal
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
 * Gets whether this tilepart is a grav lift.
 * @return, true if a grav lift
 */
bool MapData::isGravLift() const
{
	return _gravLift;
}

/**
 * Gets whether this tilepart blocks smoke.
 * @return, true if blocks smoke
 */
bool MapData::blocksSmoke() const
{
	return _disallowSmoke;
}

/**
 * Gets whether this tilepart blocks fire.
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
	// TODO: If gravlifts are truly indestructible then they ought have their
	// armor value set to 255 when their dataset loads and be done with it.
	// see Tile::destroyTilepart()

	_slideDoor     = slideDoor;
	_stopLos       = stopLOS;
	_noFloor       = noFloor;
	_bigWall       = static_cast<BigwallType>(bigWall);
	_gravLift      = gravLift;
	_hingeDoor     = hingeDoor;
	_disallowFire  = disallowFire;
	_disallowSmoke = disallowSmoke;
	_baseObject    = baseObject;

	_door = _slideDoor
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
//		case DT_NONE:  return _block[BT_LOS];
//		case DT_HE:    return _block[BT_HE];
//		case DT_SMOKE: return _block[BT_SMOKE];
//		case DT_IN:    return _block[BT_FIRE];
//		case DT_STUN:  return _block[BT_GAS];
													// see setBlock() below_
		case DT_NONE:  return _blocks[BT_LOS];		// stop LoS: [0 or 100], was [0 or 255]
		case DT_HE:
		case DT_IN:
		case DT_STUN:  return _blocks[BT_HE];		// HE block [int] // TODO: Just fix all this crap.
		case DT_SMOKE: return _blocks[BT_SMOKE];	// block smoke: try (bool), was [0 or 256]
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
void MapData::setBlocks(
		int light,
		int vision,
		int he,
		int smoke,
		int fire,
		int gas)
{
	_blocks[BT_LIGHT] = light;					// not used.
	_blocks[BT_LOS]   = vision == 1 ? 100 : 0;	// stopLoS==true needs to be a significantly large integer (only about 10+ really)
												// so that if a directionally opposite Field of View check includes a "-1",
												// meaning block by bigWall or other content-object, the result is not reduced
												// to zero (no block at all) when added to regular stopLoS by a standard wall.
												//
												// It would be unnecessary to use that jigger-pokery if TileEngine::horizontalBlockage()
												// and TileEngine::blockage() were coded differently - verticalBlockage() too, perhaps.
	_blocks[BT_HE]    = he;
	_blocks[BT_SMOKE] = smoke;					// this is the same as the _blockSmoke flag.
	_blocks[BT_FIRE]  = fire;					// IMPORTANT: this is Flammable, NOT Block_Fire per se.
	_blocks[BT_GAS]   = gas;					// probably not used.
}

/**
 * Sets default blockage values if none are assigned in an MCD.
 * @param armor - armor value of the tilepart
 */
void MapData::setDefaultBlocks(int armor)
{
	_blocks[BT_LIGHT] = 1;
	_blocks[BT_LOS]   = 1;
	_blocks[BT_HE]    = armor;
	_blocks[BT_SMOKE] = 1;
	_blocks[BT_FIRE]  = 1;
	_blocks[BT_GAS]   = 1;
}

/**
 * Sets whether this tilepart stops LoS.
 * @param, true if stops LoS
 */
void MapData::setStopLos(bool stopLos)
{
	_stopLos = stopLos;
	_blocks[BT_LOS] = (stopLos == true) ? 100 : 0;
}

/**
 * Sets the amount of HE blockage.
 * @param heBlock - the high explosive blockage
 */
void MapData::setHeBlock(int he)
{
	_blocks[BT_HE] = he;
}

/**
 * Gets the offset on the y-axis for drawing this tilepart.
 * @note this value is positive (cf. getTerrainLevel())
 * @return, the inverse-offset of the sprite in pixels
 */
int MapData::getOffsetY() const
{
	return _yOffset;
}

/**
 * Sets the offset on the y-axis for drawing this tilepart.
 * @note this value is positive (cf. setTerrainLevel())
 * @param offset - the inverse-offset of the sprite in pixels
 */
void MapData::setOffsetY(int offset)
{
	_yOffset = offset;
}

/**
 * Gets the MapDataType (part-type) of this tilepart.
 * @return, the tilepart type (0-3)
 */
MapDataType MapData::getPartType() const
{
	return _partType;
}

/**
 * Sets the MapDataType (part-type) of this tilepart.
 * @param type - the tilepart type (0-3)
 */
void MapData::setPartType(MapDataType type)
{
	_partType = type;
}

/**
 * Gets this tilepart's TilepartSpecial.
 * @return, the TilepartSpecial (MapData.h)
 */
TilepartSpecial MapData::getSpecialType() const
{
	return _specialType;
}

/**
 * Sets this tilepart's TilepartSpecial.
 * @param type - a TilepartSpecial (MapData.h)
 */
void MapData::setSpecialType(TilepartSpecial type)
{
	_specialType = type;
}

/**
 * Gets the TU-cost to move over/through this tilepart.
 * @param type - the MoveType (MapData.h)
 * @return, the TU-cost
 */
int MapData::getTuCostPart(MoveType type) const
{
	switch (type)
	{
		case MT_WALK:  return _tuWalk;
		case MT_SLIDE: return _tuSlide;
		case MT_FLY:   return _tuFly;
	}
	return 0;
}

/**
 * Sets the TU-cost to move over this tilepart.
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
 * Adds to the graphical Y-offset of units or objects on this tilepart if this
 * tilepart is a floor or content-object.
 * @note this value is negative (cf. getOffsetY())
 * @return, y-offset in pixels
 */
int MapData::getTerrainLevel() const
{
	return _level;
}

/**
 * Sets the Y-offset for units or objects on this tilepart.
 * @note this value is negative (cf. setOffsetY())
 * @param offset - y-offset in pixels
 */
void MapData::setTerrainLevel(int offset)
{
	_level = offset;
}

/**
 * Gets the index to this tilepart's footstep sound.
 * @return, the sound ID
 */
int MapData::getFootstepSound() const
{
	return _sound;
}

/**
 * Sets the index to this tilepart's footstep sound.
 * @param sound - the sound ID
 */
void MapData::setFootstepSound(int sound)
{
	_sound = sound;
}

/**
 * Gets the alternate tilepart ID.
 * @return, the alternate tilepart ID
 */
int MapData::getAltPart() const
{
	return _altId;
}

/**
 * Sets the alternate tilepart ID.
 * @param id - the alternate tilepart ID
 */
void MapData::setAltPart(int id)
{
	_altId = id;
}

/**
 * Gets the dead tilepart ID.
 * @return, the dead tilepart ID
 */
int MapData::getDiePart() const
{
	return _dieId;
}

/**
 * Sets the dead tilepart ID.
 * @param id - the dead tilepart ID
 */
void MapData::setDiePart(int id)
{
	_dieId = id;
}

/**
 * Gets the amount of light this tilepart emits.
 * @return, the amount of light
 */
int MapData::getLightSource() const
{
//	if (_lightSource == 1)	// lamp posts have 1
//		return 15;			// but they should emit more light -> Fixed in MCD.

	return _light;// - 1;
}

/**
 * Sets the amount of light this tilepart emits.
 * @param light - the amount of light
 */
void MapData::setLightSource(int light)
{
	_light = light;
}

/**
 * Gets the amount of armor.
 * @note Total hitpoints of the tilepart before destroyed. "255" shall be
 * considered indestructible.
 * @return, the amount of armor
 */
int MapData::getArmorPoints() const
{
	return _armor;
}

/**
 * Sets the amount of armor.
 * @note Total hitpoints of the tilepart before destroyed. "255" shall be
 * considered indestructible.
 * @param armor - the amount of armor
 */
void MapData::setArmorPoints(int armor)
{
	_armor = armor;
}

/**
 * Gets the amount of flammable (how flammable this tilepart is).
 * @note "255" shall be considered not flammable.
 * @return, the amount of flammable
 */
int MapData::getFlammable() const
{
	return _flammable;
}

/**
 * Sets the amount of flammable (how flammable this tilepart is).
 * @note "255" shall be considered not flammable.
 * @param flammable - the amount of flammable
 */
void MapData::setFlammable(int flammable)
{
	_flammable = flammable;
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
 * @param fuel - the amount of fuel
 */
void MapData::setFuel(int fuel)
{
	_fuel = fuel;
}

/**
 * Gets the LoFT-index of a specified layer.
 * @param layer - the layer (0..11)
 * @return, the LoFT-index
 */
size_t MapData::getLoftId(size_t layer) const
{
	return _loftIds[layer];
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
	_loftIds[layer] = loft;
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
	_miniId = static_cast<int>(id);
}

/**
 * Gets the SCANG.DAT index for the minimap.
 * @return, the minimap index
 */
int MapData::getMiniMapIndex() const
{
	return _miniId;
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
 * Checks if this is an aLien-objective tilepart.
 * @return, true if so
 */
bool MapData::isBaseObject() const
{
	return _baseObject;
}

/**
 * Sets if this tilepart is psychedelic.
 * @param psycho - true if psycho
 */
void MapData::setPsychedelic(int psycho)
{
	_psychedelic = psycho;
}

/**
 * Gets if this tilepart is psychedelic.
 * @return, true if psycho
 */
int MapData::getPsychedelic() const
{
	return _psychedelic;
}

}
