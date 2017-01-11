/*
 * Copyright 2010-2017 OpenXcom Developers.
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

#include "Tile.h"

//#include <algorithm>

#include "../fmath.h"

#include "BattleItem.h"
#include "SerializationHelper.h"

#include "../Battlescape/Pathfinding.h"

#include "../Engine/RNG.h"
#include "../Engine/SurfaceSet.h"

#include "../Ruleset/MapDataSet.h"
#include "../Ruleset/RuleArmor.h"

#include "../Savegame/SavedBattleGame.h"


namespace OpenXcom
{

/// How many bytes various fields use in a serialized tile. See header.
Tile::SerializationKey Tile::serializationKey
{
	4, // index
	2, // _partSetId, four of these
	2, // _partId, four of these
	1, // _fire
	1, // _smoke
	1, // _aniOffset
	1, // one 8-bit bool field
	4 + (2 * 4) + (2 * 4) + 1 + 1 + 1 + 1 // total bytes to save one tile
};


/**
 * Creates the Tile at a specified Position.
 * @param pos - reference to a position
 */
Tile::Tile(const Position& pos)
	:
		_pos(pos),
		_smoke(0),
		_fire(0),
		_explosive(0),
		_explosiveType(DT_NONE),
		_unit(nullptr),
		_aniOffset(0),
		_visible(false),
		_previewColor(0u),
		_previewDir(-1),
		_previewTu(-1),
		_danger(false)
{
	size_t i;
	for (
			i = 0u;
			i != PARTS_TILE;
			++i)
	{
		_parts[i]		=  nullptr;
		_partId[i]		= -1;
		_partSetId[i]	= -1;
		_curFrame[i]	=  0;
	}

	for (
			i = 0u;
			i != SECTIONS;
			++i)
	{
		_revealed[i] = false;
	}

	for (
			i = 0u;
			i != LIGHTLAYERS;
			++i)
	{
		_light[i] = 0;
	}
}

/**
 * dTor.
 */
Tile::~Tile()
{
	_inventory.clear();
}

/**
 * Loads this Tile from a YAML node.
 * @param node - reference a YAML node
 */
void Tile::load(const YAML::Node& node)
{
	for (size_t
			i = 0u;
			i != PARTS_TILE;
			++i)
	{
		_partId[i]		= node["mapDataID"][i]		.as<int>(_partId[i]);
		_partSetId[i]	= node["mapDataSetID"][i]	.as<int>(_partSetId[i]);
	}

	_fire		= node["fire"]		.as<int>(_fire);
	_smoke		= node["smoke"]		.as<int>(_smoke);
	_aniOffset	= node["aniOffset"]	.as<int>(_aniOffset);

	if (node["discovered"])
	{
		for (size_t
				i = 0u;
				i != SECTIONS;
				++i)
		{
			_revealed[i] = node["discovered"][i].as<bool>();
		}
	}

	if (node["openDoorWest"])
		_curFrame[1u] = 7;

	if (node["openDoorNorth"])
		_curFrame[2u] = 7;
}

/**
 * Loads this Tile from binary.
 * @param buffer - pointer to buffer
 * @param serKey - reference to the serialization key
 */
void Tile::loadBinary(
		Uint8* buffer,
		Tile::SerializationKey& serKey)
{
	_partId[O_FLOOR]		= unserializeInt(&buffer, serKey._partId);
	_partId[O_WESTWALL]		= unserializeInt(&buffer, serKey._partId);
	_partId[O_NORTHWALL]	= unserializeInt(&buffer, serKey._partId);
	_partId[O_OBJECT]		= unserializeInt(&buffer, serKey._partId);

	_partSetId[O_FLOOR]		= unserializeInt(&buffer, serKey._partSetId);
	_partSetId[O_WESTWALL]	= unserializeInt(&buffer, serKey._partSetId);
	_partSetId[O_NORTHWALL]	= unserializeInt(&buffer, serKey._partSetId);
	_partSetId[O_OBJECT]	= unserializeInt(&buffer, serKey._partSetId);

	_smoke		= unserializeInt(&buffer, serKey._smoke);
	_fire		= unserializeInt(&buffer, serKey._fire);
	_aniOffset	= unserializeInt(&buffer, serKey._aniOffset);

	const int boolFields (unserializeInt(
									&buffer,
									serKey.boolFields));

	_revealed[ST_WEST]		= (boolFields & 0x01) ? true : false;
	_revealed[ST_NORTH]		= (boolFields & 0x02) ? true : false;
	_revealed[ST_CONTENT]	= (boolFields & 0x04) ? true : false;

	_curFrame[O_WESTWALL]	= (boolFields & 0x08) ? 7 : 0;
	_curFrame[O_NORTHWALL]	= (boolFields & 0x10) ? 7 : 0;

//	if (_fire || _smoke) _animationOffset = std::rand() % 4;
}

/**
 * Saves this Tile to a YAML node.
 * @return, YAML node
 */
YAML::Node Tile::save() const
{
	YAML::Node node;

	node["position"] = _pos;

	for (size_t
			i = 0u;
			i != PARTS_TILE;
			++i)
	{
		node["mapDataID"].push_back(_partId[i]);
		node["mapDataSetID"].push_back(_partSetId[i]);
	}

	if (_smoke != 0)		node["smoke"]		= _smoke;
	if (_fire != 0)			node["fire"]		= _fire;
	if (_aniOffset != 0)	node["aniOffset"]	= _aniOffset;

	if (   _revealed[ST_WEST]		== true
		|| _revealed[ST_NORTH]		== true
		|| _revealed[ST_CONTENT]	== true)
	{
		for (size_t
				i = 0u;
				i != SECTIONS;
				++i)
		{
			node["discovered"].push_back(_revealed[i]);
		}
	}

	if (isSlideDoorOpen(O_WESTWALL) == true)
		node["openDoorWest"] = true;
	if (isSlideDoorOpen(O_NORTHWALL) == true)
		node["openDoorNorth"] = true;

	return node;
}

/**
 * Saves this Tile to binary.
 * @param buffer - pointer to pointer to buffer
 */
void Tile::saveBinary(Uint8** buffer) const
{
	serializeInt(buffer, serializationKey._partId, _partId[O_FLOOR]);
	serializeInt(buffer, serializationKey._partId, _partId[O_WESTWALL]);
	serializeInt(buffer, serializationKey._partId, _partId[O_NORTHWALL]);
	serializeInt(buffer, serializationKey._partId, _partId[O_OBJECT]);

	serializeInt(buffer, serializationKey._partSetId, _partSetId[O_FLOOR]);
	serializeInt(buffer, serializationKey._partSetId, _partSetId[O_WESTWALL]);
	serializeInt(buffer, serializationKey._partSetId, _partSetId[O_NORTHWALL]);
	serializeInt(buffer, serializationKey._partSetId, _partSetId[O_OBJECT]);

	serializeInt(buffer, serializationKey._smoke,		_smoke);
	serializeInt(buffer, serializationKey._fire,		_fire);
	serializeInt(buffer, serializationKey._aniOffset,	_aniOffset);

	int boolFields ((_revealed[ST_WEST] ? 0x01 : 0x0) + (_revealed[ST_NORTH] ? 0x02 : 0x0) + (_revealed[ST_CONTENT] ? 0x04 : 0x0));

	boolFields |= isSlideDoorOpen(O_WESTWALL)  ? 0x08 : 0x0;
	boolFields |= isSlideDoorOpen(O_NORTHWALL) ? 0x10 : 0x0;

	serializeInt(
				buffer,
				serializationKey.boolFields,
				boolFields);
}

/**
 * Sets the MapData references of parts 0 to 3.
 * @param part		- pointer to MapData
 * @param partId	- data-ID
 * @param partSetId	- dataSet-ID
 * @param partType	- the part-type (MapData.h)
 */
void Tile::setMapData(
		MapData* const part,
		const int partId,
		const int partSetId,
		const MapDataType partType)
{
	_parts[partType]		= part;
	_partId[partType]		= partId;
	_partSetId[partType]	= partSetId;
}

/**
 * Gets the MapData references of parts 0 to 3.
 * @param partId	- pointer to data-ID
 * @param partSetId	- pointer to dataSet-ID
 * @param partType	- the part-type (MapData.h)
 */
void Tile::getMapData(
		int* partId,
		int* partSetId,
		MapDataType partType) const
{
	*partId		= _partId[partType];
	*partSetId	= _partSetId[partType];
}

/**
 * Gets whether this Tile has no objects.
 * @note The function does not check for a BattleUnit in this Tile.
 * @param testInventory - true to check for inventory items (default true)
 * @param testVolatiles - true to check for smoke and/or fire (default true)
 * @return, true if there is nothing but air on this Tile
 */
bool Tile::isVoid(
		const bool testInventory,
		const bool testVolatiles) const
{
	bool ret (_parts[O_FLOOR]		== nullptr
		   && _parts[O_WESTWALL]	== nullptr
		   && _parts[O_NORTHWALL]	== nullptr
		   && _parts[O_OBJECT]		== nullptr);

	if (testInventory == true)
		ret &= (_inventory.empty() == true);

	if (testVolatiles == true)
		ret &= (_smoke == 0); // -> fireTiles always have smoke.

	return ret;
}

/**
 * Gets the TU cost to move over a partType of this Tile.
 * @param partType	- the MapDataType (MapData.h)
 * @param type		- the MoveType (MapData.h)
 * @return, TU cost
 */
int Tile::getTuCostTile(
		MapDataType partType,
		MoveType type) const
{
	if (    _parts[partType] != nullptr
		&& (_parts[partType]->isSlideDoor() == false || _curFrame[partType] < 2))
	{
		switch (partType)
		{
			case O_FLOOR:
			case O_WESTWALL:
			case O_NORTHWALL:
				return _parts[partType]->getTuCostPart(type);

			case O_OBJECT:
				switch (_parts[O_OBJECT]->getBigwall())
				{
					case BIGWALL_NONE:
					case BIGWALL_BLOCK:
					case BIGWALL_NESW:
					case BIGWALL_NWSE:
						return _parts[partType]->getTuCostPart(type);
				}
		}
	}
	return 0;
}

/**
 * Checks whether or not this Tile has a reasonable floor.
 * @param tileBelow - the tile below this Tile (default nullptr)
 * @return, true if tile has an effective floor
 */
bool Tile::isFloored(const Tile* const tileBelow) const
{
	if (_pos.z == 0
		|| (tileBelow != nullptr && tileBelow->getTerrainLevel() == -24))
	{
		return true;
	}

	if (_parts[O_FLOOR] != nullptr)
		return (_parts[O_FLOOR]->isNoFloor() == false);

	return false;
}

/**
 * Gets the Tile below this Tile.
 * @param battleSave - pointer to the SavedBattleGame
 * @return, pointer to tile-below (can be nullptr)
 */
Tile* Tile::getTileBelow(const SavedBattleGame* const battleSave) const
{
	static const Position& posBelow (Position(0,0,-1));

	return battleSave->getTile(_pos + posBelow);
}

/**
 * Gets the Tile above this Tile.
 * @param battleSave - pointer to the SavedBattleGame
 * @return, pointer to tile-above (can be nullptr)
 */
Tile* Tile::getTileAbove(const SavedBattleGame* const battleSave) const
{
	static const Position& posAbove (Position(0,0,+1));

	return battleSave->getTile(_pos + posAbove);
}

/**
 * Gets whether this Tile has a bigwall.
 * @return, true if the content-object in this Tile is a bigwall (BigwallType in Pathfinding.h)
 *
bool Tile::isBigWall() const
{
	if (_parts[O_OBJECT] != nullptr)
		return (_parts[O_OBJECT]->getBigwall() != BIGWALL_NONE);

	return false;
} */

/**
 * Gets the terrain-level of this Tile.
 * @note For graphical Y offsets etc. Terrain-level starts and 0 and goes
 * upwards to -24; negative values are higher.
 * @return, the level in pixels
 */
int Tile::getTerrainLevel() const
{
	int level (0);
	if (_parts[O_FLOOR] != nullptr)
		level = _parts[O_FLOOR]->getTerrainLevel();

	if (_parts[O_OBJECT] != nullptr)
		level = std::min(level,
						_parts[O_OBJECT]->getTerrainLevel());

	return level;
}

/**
 * Gets this Tile's footstep-sound.
 * @param tileBelow - pointer to the Tile below this Tile
 * @return, sound-ID
 *			0 - none
 *			1 - metal
 *			2 - wood/stone
 *			3 - dirt
 *			4 - mud
 *			5 - sand
 *			6 - snow (mars)
 */
int Tile::getFootstepSound(const Tile* const tileBelow) const
{
	if (_parts[O_OBJECT] != nullptr
		&& _parts[O_OBJECT]->getFootstepSound() != 0)
	{
		switch (_parts[O_OBJECT]->getBigwall())
		{
			case BIGWALL_NONE:
			case BIGWALL_BLOCK:
				return _parts[O_OBJECT]->getFootstepSound();
		}
	}

	if (_parts[O_FLOOR] != nullptr)
		return _parts[O_FLOOR]->getFootstepSound();

	if (_parts[O_OBJECT] == nullptr
		&& tileBelow != nullptr
		&& tileBelow->getTerrainLevel() == -24)
	{
		return tileBelow->getMapData(O_OBJECT)->getFootstepSound();
	}
	return 0;
}

/**
 * Opens a door on this Tile.
 * @note It's inadvisable to set an MCD-entry as both a door and a ufo-door.
 * @param partType		- a tile-part type (MapData.h)
 * @param unit			- pointer to a BattleUnit (default nullptr)
// * @param reserved	- BattleActionType (BattlescapeGame.h) (default BA_NONE)
 * @return, DoorResult (Tile.h)
 *			-1 no door opened
 *			 0 wood door
 *			 1 ufo door
 *			 2 ufo door is still opening (animation playing)
 *			 3 not enough TUs
 */
DoorResult Tile::openDoor(
		const MapDataType partType,
		const BattleUnit* const unit)
//		const BattleActionType reserved)
{
	if (_parts[partType] != nullptr)
	{
		if (_parts[partType]->isHingeDoor() == true)
		{
			if (_unit != nullptr
				&& _unit != unit
				&& _unit->getPosition() != getPosition())
			{
				return DR_NONE;
			}

			if (unit != nullptr
				&& unit->getTu() < _parts[partType]->getTuCostPart(unit->getMoveTypeUnit()))
//									+ unit->getActionTu(reserved, unit->getMainHandWeapon()))
			{
				return DR_ERR_TU;
			}

			setMapData(
					_parts[partType]->getDataset()->getRecords()->at(static_cast<size_t>(_parts[partType]->getAltMCD())),
					_parts[partType]->getAltMCD(),
					_partSetId[partType],
					_parts[partType]->getDataset()->getRecords()->at(static_cast<size_t>(_parts[partType]->getAltMCD()))->getPartType());
			setMapData(nullptr,-1,-1, partType);

			return DR_WOOD_OPEN;
		}

		if (_parts[partType]->isSlideDoor() == true)
		{
			if (_curFrame[partType] == 0) // ufo door part 0 - door is closed
			{
				if (unit != nullptr
					&& unit->getTu() < _parts[partType]->getTuCostPart(unit->getMoveTypeUnit()))
//										+ unit->getActionTu(reserved, unit->getMainHandWeapon()))
				{
					return DR_ERR_TU;
				}

				_curFrame[partType] = 1; // start opening door
				return DR_UFO_OPEN;
			}

			if (_curFrame[partType] != 7) // ufo door != part 7 -> door is still opening
				return DR_UFO_WAIT;
		}
	}
	return DR_NONE;
}

/**
 * Opens a ufo-door without checks.
 * @param partType - a tile-part type (MapData.h)
 */
void Tile::openAdjacentDoor(const MapDataType partType)
{
	_curFrame[partType] = 1;
}

/**
 * Closes ufo-door(s) on this Tile.
 * @return, true if a door closed
 */
bool Tile::closeSlideDoor()
{
	int ret (false);
	for (size_t
			i = 0u;
			i != PARTS_TILE;
			++i)
	{
		if (isSlideDoorOpen(static_cast<MapDataType>(i)) == true)
		{
			_curFrame[i] = 0;
			ret = true;
		}
	}
	return ret;
}

/**
 * Sets this Tile's sections' revealed flags.
 * @note Also re-caches the sprites for any unit on this Tile if the value changes.
 * @param section - the SectionType (Tile.h) (default ST_CONTENT)
 *					0 westwall
 *					1 northwall
 *					2 object+floor
 * @param revealed - true if revealed (default true)
 */
void Tile::setRevealed(
		SectionType section,
		bool revealed)
{
	if (_revealed[section] != revealed)
	{
		if ((_revealed[section] = revealed) == true
			&& section == ST_CONTENT)
		{
			if (    _parts[O_OBJECT] == nullptr
				|| (_parts[O_OBJECT]->getBigwall() & 0x6) == 0) // NeSw, NwSe: Try no-reveal (walls) if content is diag BigWall.
			{
				_revealed[ST_WEST]  = // if object+floor is revealed set west- & north-walls revealed also.
				_revealed[ST_NORTH] = true;
			}
		}
		if (_unit != nullptr) _unit->setCacheInvalid();
	}
}

/**
 * Gets the black fog-of-war/revealed status of this Tile.
 * @param section - the SectionType (Tile.h) (default ST_CONTENT)
 *					0 westwall
 *					1 northwall
 *					2 object+floor
 * @return, true if revealed
 */
bool Tile::isRevealed(SectionType section) const
{
	return _revealed[section];
}

/**
 * Sets the light on this Tile to zero.
 * @note This is done before a light-power recalculation.
 * @param layer - light is separated in 3 layers: Ambient, Static and Dynamic
 */
void Tile::resetLight(size_t layer)
{
	_light[layer] = 0;
}

/**
 * Adds light on this Tile.
 * @note Only add light if the current light-power is lower.
 * @param light - amount of light to add
 * @param layer - light is separated in 3 layers: Ambient, Static and Dynamic
 */
void Tile::addLight(
		int light,
		size_t layer)
{
	if (light > _light[layer])
		_light[layer] = (light > LIGHT_FULL) ? LIGHT_FULL : light;
}

/**
 * Gets this Tile's shade: 0-15.
 * @note Gets the brightest of all light-layers.
 * @note Shade is the inverse of light-level so a maximum amount of light '15'
 * returns shade-level '0'.
 * @return, the current shade
 */
int Tile::getShade() const
{
	int light (0);
	for (size_t
			i = 0u;
			i != LIGHTLAYERS;
			++i)
	{
		if (_light[i] > light)
			light = _light[i];
	}
	return LIGHT_FULL - light;
}

/**
 * Destroys a specified part-type on this Tile.
 * @note First remove the old part then replace it with the destroyed one
 * because the part-type of the old and new are not necessarily the same. If
 * the destroyed part is an explosive set the tile's explosive value which will
 * trigger a chained explosion.
 * @param partType		- the tile-part for destruction (MapData.h)
 * @param battleSave	- pointer to the SavedBattleGame
 * @param obliterate	- true to bypass the death-part (default false)
 * @return, the power that was required to destroy the part (-1 not destroyed)
 */
int Tile::destroyTilepart(
		MapDataType partType,
		SavedBattleGame* const battleSave,
		bool obliterate)
{
	//Log(LOG_INFO) << "";
	//Log(LOG_INFO) << "Tile::destroyTilepart() " << _pos;

	int
		ret,
		tLevel (0);

	const MapData* const part (_parts[partType]);
	if (part != nullptr)
	{
		if (part->getArmorPoints() != 255 && part->isGravLift() == false)
		{
			ret = part->getArmorPoints();

			if (part->getTileType() == battleSave->getObjectiveTileType())
				battleSave->addDestroyedObjective();

			if (partType == O_OBJECT)
				tLevel = _parts[O_OBJECT]->getTerrainLevel();

			setMapData(nullptr,-1,-1, partType); // destroy current part.

			if (obliterate == false)
			{
				const int deadId (part->getDieMCD());
				if (deadId != 0) // instantiate a death-part if there is one.
				{
					MapData* const partDead (part->getDataset()->getRecords()->at(static_cast<size_t>(deadId)));
					setMapData(
							partDead,
							deadId,
							_partSetId[partType],
							partDead->getPartType());
				}

				if (part->getExplosive() != 0) // do not explode if 'obliterate' is TRUE.
					setExplosive(
							part->getExplosive(),
							part->getExplosiveType());
			}
		}
		else // indestructable.
			return -1;
	}
	else
		ret = -1;

	if (partType == O_FLOOR) // check if the floor-part on the ground-level is gone.
	{
		if (_pos.z == 0 && _parts[O_FLOOR] == nullptr) // replace with scorched earth
			setMapData(
					MapDataSet::getScorchedEarth(),
					1,0, O_FLOOR);

		if (   _parts[O_OBJECT] != nullptr // destroy object-part if the floor-part was just destroyed.
			&& _parts[O_OBJECT]->getBigwall() == BIGWALL_NONE)
		{
			destroyTilepart(O_OBJECT, battleSave, true); // stop floating haybales.
		}
	}

	if (tLevel == -24) // destroy the object-part above if all its support was destroyed.
	{
		Tile* const tileAbove (battleSave->getTile(_pos + Position(0,0,1)));
		if (   tileAbove != nullptr
			&& tileAbove->getMapData(O_FLOOR)  == nullptr
			&& tileAbove->getMapData(O_OBJECT) != nullptr
			&& tileAbove->getMapData(O_OBJECT)->getBigwall() == BIGWALL_NONE)
		{
			tileAbove->destroyTilepart(O_OBJECT, battleSave, true); // stop floating lamposts. Trees would be more difficult.
		}
	}

	return ret;
}

/**
 * Damages terrain (check against terrain-part armor).
 * @note Called by TileEngine::hit().
 * @param partType		- part of tile to check (MapData.h)
 * @param power			- power of the hit
 * @param battleSave	- pointer to SavedBattleGame
 */
void Tile::hitTile(
		MapDataType partType,
		int power,
		SavedBattleGame* const battleSave)
{
	//Log(LOG_INFO) << "Tile::hitTile() partType= "	<< partType
	//			  << " hp= "						<< _parts[partType]->getArmor()
	//			  << " power= "						<< power;
	int expend;
	while (power > 0
		&& _parts[partType] != nullptr // early out + safety: Also handled in destroyTilepart().
		&& _parts[partType]->getArmorPoints() <= power)
	{
		if ((expend = destroyTilepart(partType, battleSave)) != -1)
			power -= expend;
		else
			break;
	}
	// NOTE: A part's death-part can cycle or loop back to itself -- which if so
	// should be corrected in the part's Tileset itself.
}

/**
 * Sets a "virtual" explosive on this Tile.
 * @note Mark a tile this way to detonate it later because the same tile can be
 * visited multiple times by "explosion rays". The explosive power that gets set
 * on a tile is that of the most powerful ray that passes through it -- see
 * TileEngine::explode().
 * @param power		- how big the BOOM will be / how much tile-destruction
 * @param explType	- the type of this Tile's explosion (set in MCD) (RuleItem.h)
 */
void Tile::setExplosive(
		int power,
		DamageType explType)
{
	if (_explosive < power)
	{
		_explosive = power;
		_explosiveType = explType;
	}
}

/**
 * Resets this Tile's explosive to zero.
 */
void Tile::clearExplosive()
{
	_explosive = 0;
	_explosiveType = DT_NONE;
}

/**
 * Gets if & how powerfully this Tile will explode.
 * @note Don't confuse this with a tile's inherent explosive power. This value
 * is set by explosions external to the tile itself.
 * @return, how big the BOOM will be / how much tile-destruction
 */
int Tile::getExplosive() const
{
	return _explosive;
}

/**
 * Gets explosive type of this Tile.
 * @return, explosive type
 */
DamageType Tile::getExplosiveType() const
{
	return _explosiveType;
}

/**
 * Flammability of a tile is the flammability of its most flammable tile-part.
 * @note I now decree that this returns the inverse of 0..255 as a percentage!
 * @param partType - the part to check or O_NULPART to test all parts (default O_NULPART) (MapData.h)
 * @return, the lower the value the higher the chance the tile-part catches fire - BSZAAST!!!
 */
int Tile::getBurnable(MapDataType partType) const
{
	if (partType == O_NULPART)
	{
		int burn (255); // not burnable. <- lower is better :)
		for (size_t
				i = 0u;
				i != PARTS_TILE;
				++i)
		{
			if (   _parts[i] != nullptr
				&& _parts[i]->getFlammable() < burn)
			{
				burn = _parts[i]->getFlammable();
			}
		}
		return convertBurnToPct(burn);
	}
	return convertBurnToPct(_parts[partType]->getFlammable());
}

/**
 * Converts obscure inverse MCD notation to understandable percentages.
 * @note Chance can be increased by the power of the spark.
 * @param burn - flammability from an MCD file (see MapData)
 * @return, basic percent chance that this stuff burns
 */
int Tile::convertBurnToPct(int burn) // private/static.
{
	if (burn <   1) return 100;
	if (burn > 254) return 0;

	return Vicegrip(100 - static_cast<int>(std::floor(static_cast<float>(burn) / 255.f * 100.f)), 1,100);
}

/**
 * Gets the fuel of a tile-part.
 * @note Fuel of a tile is the highest fuel of its parts/objects. This is NOT
 * the sum of the fuel of the objects!
 * @param partType - the part to check or O_NULPART to test all parts (default O_NULPART) (MapData.h)
 * @return, turns to burn
 */
int Tile::getFuel(MapDataType partType) const
{
	if (partType == O_NULPART)
	{
		int fuel (0);
		for (size_t
				i = 0u;
				i != PARTS_TILE;
				++i)
		{
			if (   _parts[i] != nullptr
				&& _parts[i]->getFuel() > fuel)
			{
				fuel = _parts[i]->getFuel();
			}
		}
		return fuel;
	}
	return _parts[partType]->getFuel();
}

/**
 * Tries to start fire on this Tile.
 * @note If true it will add its fuel as turns-to-burn if tile is burning at a
 * lesser intensity. Called by floor-burning Silacoids and fire spreading @
 * turnovers and by TileEngine::detonateTile() after HE explosions. The tile
 * needs to both be flammable and have internal fuel or else it won't even
 * attempt to catch fire.
 * @param power - rough chance to get things going
 * @return, true if tile catches fire or even gets smoked
 */
bool Tile::igniteTile(int power)
{
	//Log(LOG_INFO) << "";
	//Log(LOG_INFO) << "Tile::igniteTile() " << _pos << " power= " << power;
	if (power != 0 && allowSmoke() == true)
	{
		const int burn (getBurnable());
		//Log(LOG_INFO) << ". burn= " << burn;
		if (burn != 0)
		{
			const int fuel (getFuel());
			//Log(LOG_INFO) << ". . fuel= " << fuel;
			if (fuel != 0)
			{
				power = ((power + 4) / 5) + ((burn + 7) >> 3u) + (((fuel << 1u) + 6) / 7);
				//Log(LOG_INFO) << ". . . power= " << power;
				if (RNG::percent(power) == true)	// unfortunately the state-machine may cause an unpredictable quantity of calls to this
				{									// ... via ExplosionBState::think().
					//Log(LOG_INFO) << ". . . . turns= " << ((burn + 15) >> 4u);
					addSmoke((burn + 15) >> 4u);	// TODO: Add smoke to tileAbove also.

					if (allowFire() == true && _fire < fuel + 1)	// TODO: pass in tileBelow and check its terrainLevel for -24;
						addFire(fuel + 1);							// drop fire through to any tileBelow ...

					return true;
				}
			}
		}
	}
	return false;
}

/**
 * Adds fire to this Tile.
 * @param turns - turns to burn
 * @return, true if fire was added
 */
bool Tile::addFire(int turns)
{
	if (turns != 0 && allowFire() == true)
	{
		if (_smoke == 0 && _fire == 0)
			_aniOffset = RNG::seedless(0,3);

		_fire += turns;

		if (_fire > 12) _fire = 12;

		if (_smoke < _fire + 2)
			_smoke = _fire + RNG::generate(2,3);

		return true;
	}
	return false;
}

/**
 * Reduces the number of turns this Tile will burn.
 * @return, fire after decrease
 */
int Tile::decreaseFire()
{
	if (--_fire < 1)
	{
		_fire = 0;
		if (_smoke == 0) _aniOffset = 0;
	}
	return _fire;
}

/**
 * Gets the number of turns left for this Tile to be on fire.
 * @return, turns left
 */
int Tile::getFire() const
{
	return _fire;
}

/**
 * Adds smoke to this Tile.
 * @param turns - turns to smoke for
 */
void Tile::addSmoke(int turns)
{
	if (turns != 0 && allowSmoke() == true)
	{
		if (_smoke == 0 && _fire == 0)
			_aniOffset = RNG::seedless(0,3);

		if ((_smoke += turns) > 17)
			_smoke = 17;
	}
}

/**
 * Reduces the number of turns this Tile will smoke for.
 * @return, smoke after decrease
 */
int Tile::decreaseSmoke()
{
	if (_fire != 0) // don't let smoke deplete faster than fire depletes.
		--_smoke;
	else
		_smoke -= (RNG::generate(1, _smoke) + 2) / 3;

	if (_smoke < 1)
	{
		_smoke = 0;
		if (_fire == 0) _aniOffset = 0;
	}
	return _smoke;
}

/**
 * Gets the number of turns left for this Tile to smoke.
 * @return, turns left
 */
int Tile::getSmoke() const
{
	return _smoke;
}

/**
 * Checks if this Tile accepts smoke.
 * @note Only the object is checked: diagonal bigWalls that have their
 * '_blockSmoke' flag set TRUE never smoke.
 * @return, true if smoke can occupy this Tile
 */
bool Tile::allowSmoke() const // private.
{
	if (_parts[O_OBJECT] != nullptr)
	{
		switch (_parts[O_OBJECT]->getBigwall())
		{
			case BIGWALL_NESW:
			case BIGWALL_NWSE:
				if (_parts[O_OBJECT]->blockSmoke() == true)
					return false;
		}
	}
	return true;
}

/**
 * Checks if this Tile accepts fire.
 * @note Only the floor and object are checked: diagonal bigWalls and floors
 * that have their '_blockFire' flag set TRUE never fire.
 * @return, true if fire can occupy this Tile
 */
bool Tile::allowFire() const // private.
{
	if (_parts[O_FLOOR] == nullptr && _parts[O_OBJECT] == nullptr)
		return false;

	if (   _parts[O_FLOOR] != nullptr
		&& _parts[O_FLOOR]->blockFire() == true)
	{
		return false;
	}

	if (_parts[O_OBJECT] != nullptr)
	{
		switch (_parts[O_OBJECT]->getBigwall())
		{
			case BIGWALL_NONE:
			case BIGWALL_BLOCK:
			case BIGWALL_NESW:
			case BIGWALL_NWSE:
				if (_parts[O_OBJECT]->blockFire() == true)
					return false;
		}
	}
	return true;
}

/**
 * Ends this Tile's turn. Units catch on fire.
 * @note Separated from resolveOverlaps() above so that units take damage before
 * smoke/fire spreads to them; this is so that units would have to end their
 * turn on a tile before smoke/fire damages them. That is they get a chance to
 * get off the tile during their turn.
 * @param battleSave - pointer to the current SavedBattleGame (default nullptr hits units)
 */
void Tile::hitTileContent(SavedBattleGame* const battleSave)
{
	int
		powerSmoke,
		powerFire;

	if (_smoke != 0)
		powerSmoke = 1 + ((_smoke + 3) >> 2u);
	else
		powerSmoke = 0;

	if (_fire != 0)
		powerFire = _fire + RNG::generate(3,9);
	else
		powerFire = 0;

	float vulnr;
	if (battleSave == nullptr) // damage standing units at end of faction's turn-phase. Notice this hits only the primary quadrant! ... perhaps.
	{
		if (powerSmoke != 0 && _unit->isHealable() == true
			&& (vulnr = _unit->getArmor()->getDamageModifier(DT_SMOKE)) > 0.f) // try to knock _unit out.
		{
			_unit->takeDamage(
							Position(0,0,0),
							static_cast<int>(Round(static_cast<float>(powerSmoke) * vulnr)),
							DT_SMOKE, // -> DT_STUN
							true);
		}

		if (powerFire != 0
			&& (vulnr = _unit->getArmor()->getDamageModifier(DT_IN)) > 0.f)
		{
			_unit->takeDamage(
							Position(0,0,0),
							static_cast<int>(Round(static_cast<float>(powerFire) * vulnr)),
							DT_IN,
							true);

			if (RNG::percent(static_cast<int>(Round(40.f * vulnr))) == true) // try to set _unit on fire. Do damage from fire here, too.
			{
				const int dur (RNG::generate(1,
											 static_cast<int>(Round(5.f * vulnr))));
				if (dur > _unit->getUnitFire())
					_unit->setUnitFire(dur);
			}
		}
	}
	else // battleSave VALID -> check to destroy items & kill unconscious units at end of full-turns
	{
		BattleUnit* unit;

		if (powerSmoke != 0)
		{
			for (std::vector<BattleItem*>::const_iterator // handle unconscious units on this Tile vs. DT_SMOKE
					i = _inventory.begin();
					i != _inventory.end();
					++i)
			{
				if ((unit = (*i)->getBodyUnit()) != nullptr
					&& unit->getUnitStatus() == STATUS_UNCONSCIOUS
					&& unit->getTakenExpl() == false)
				{
					unit->setTakenExpl();

					if ((vulnr = unit->getArmor()->getDamageModifier(DT_SMOKE)) > 0.f)
						unit->takeDamage(
									Position(0,0,0),
									static_cast<int>(Round(static_cast<float>(powerSmoke) * vulnr)),
									DT_SMOKE,
									true);
				}
			}
		}

		if (powerFire != 0) // TODO: Cook-off grenades (question: primed or not). Cf, TileEngine::explode() case: DT_IN.
		{
			bool done (false);
			while (done == false && _inventory.empty() == false) // handle items including unconscious or dead units on this Tile vs. DT_IN
			{
				for (std::vector<BattleItem*>::const_iterator
						i = _inventory.begin();
						i != _inventory.end();
						)
				{
					if ((unit = (*i)->getBodyUnit()) != nullptr
						&& unit->getUnitStatus() == STATUS_UNCONSCIOUS
						&& unit->getTakenFire() == false)
					{
						unit->setTakenFire();

						if ((vulnr = unit->getArmor()->getDamageModifier(DT_IN)) > 0.f)
						{
							unit->takeDamage(
										Position(0,0,0),
										static_cast<int>(static_cast<float>(powerFire) * vulnr),
										DT_IN,
										true);

							if (unit->getHealth() == 0)
							{
								unit->putdown(true);
								unit->killerFaction(FACTION_NONE);
							}
						}
						done = (++i == _inventory.end());
					}
					else if (powerFire > (*i)->getRules()->getArmorPoints() // no modifier when destroying items, not even corpse in bodyarmor.
						&& (unit == nullptr || unit->getUnitStatus() == STATUS_DEAD))
					{
						battleSave->toDeleteItem(*i);	// This should not kill *and* remove a unit's corpse on the same
						break;							// tilePhase; but who knows, I haven't traced it comprehensively.
					}
					else
						done = (++i == _inventory.end());
				}
			}
		}
	}
}

/**
 * Animates the tile.
 * @note This means to advance the current frame for every part. Ufo-doors are a
 * bit special - they animate only when triggered; when ufo-doors are on frame 0
 * (closed) or frame 7 (open) they are not animated further. A ufo-door on an
 * XCOM craft has only 4 frames; when it hits frame 3 it jumps to frame 7 (open).
 */
void Tile::animateTile()
{
	int nextFrame;
	for (size_t
			i = 0u;
			i != PARTS_TILE;
			++i)
	{
		if (_parts[i] != nullptr)
		{
			switch (_parts[i]->getPsychedelic())
			{
				default:
				case 0:
					if (_parts[i]->isSlideDoor() == false
						|| (_curFrame[i] != 0
							&& _curFrame[i] != 7)) // ufo-door is currently static
					{
						nextFrame = _curFrame[i] + 1;

						if (_parts[i]->isSlideDoor() == true // special handling for Avenger & Lightning doors
							&& _parts[i]->getTileType() == START_TILE
							&& nextFrame == 3)
						{
							nextFrame = 7;
						}

						if (nextFrame == 8)
							nextFrame = 0;

						_curFrame[i] = nextFrame;
					}
					break;

				case 1:
					if (RNG::seedless(0,2) != 0) // 66%
						_curFrame[i] = RNG::seedless(0,7);
					break;

				case 2:
					if (RNG::seedless(0,2) == 0) // 33%
						_curFrame[i] = RNG::seedless(0,7);
			}
		}
	}
}

/**
 * Gets the number of frames the fire or smoke animation is off-sync.
 * @note To void fire and smoke animations of different tiles moving nice in
 * sync - that'd look fake.
 * @return, offset
 */
int Tile::getAnimationOffset() const
{
	return _aniOffset;
}

/**
 * Gets the sprite of a certain part of this Tile.
 * @param partType - tile-part to get a sprite for
 * @return, pointer to the sprite
 */
Surface* Tile::getSprite(MapDataType partType) const
{
	const MapData* const data (_parts[partType]);
	if (data != nullptr)
		return data->getDataset()->getSurfaceset()->getFrame(data->getSprite(_curFrame[partType]));

	return nullptr;
}

/**
 * Sets a BattleUnit on this Tile.
 * @param unit - pointer to a BattleUnit (default nullptr)
 */
void Tile::setTileUnit(BattleUnit* const unit)
{
	_unit = unit;
}

/**
 * Adds an item on this Tile.
 * @param item - pointer to a BattleItem
 */
void Tile::addItem(BattleItem* const item)
{
	item->setItemTile(this);
	_inventory.push_back(item);
}

/**
 * Removes an item from this Tile.
 * @param item - pointer to a BattleItem
 */
void Tile::removeItem(BattleItem* const item)
{
	for (std::vector<BattleItem*>::const_iterator
			i = _inventory.begin();
			i != _inventory.end();
			++i)
	{
		if (*i == item)
		{
			_inventory.erase(i);
			break;
		}
	}
	item->setItemTile();
}

/**
 * Gets a corpse-sprite to draw on the battlefield.
 * @param fire - pointer to set fire true
 * @return, sprite-ID in floorobs (-1 none)
 */
int Tile::getCorpseSprite(bool* fire) const
{
	*fire = false;

	int spriteId (-1);
	if (_inventory.empty() == false)
	{
		int
			weight (-1),
			weightTest;

		for (std::vector<BattleItem*>::const_iterator // 1. soldier body
				i = _inventory.begin();
				i != _inventory.end();
				++i)
		{
			if ((*i)->getRules()->getBattleType() == BT_CORPSE
				&& (*i)->getBodyUnit() != nullptr
				&& (*i)->getBodyUnit()->getGeoscapeSoldier() != nullptr
				&& (weightTest = (*i)->getRules()->getWeight()) > weight)
			{
				weight = weightTest;
				spriteId = (*i)->getRules()->getFloorSprite();

				if ((*i)->getBodyUnit()->getUnitFire() != 0)
					*fire = true;
			}
		}

		if (spriteId == -1)
		{
			weight = -1;
			for (std::vector<BattleItem*>::const_iterator // 2. non-soldier body
					i = _inventory.begin();
					i != _inventory.end();
					++i)
			{
				if ((*i)->getRules()->getBattleType() == BT_CORPSE
					&& (*i)->getBodyUnit() != nullptr
					&& (weightTest = (*i)->getRules()->getWeight()) > weight)
				{
					weight = weightTest;
					spriteId = (*i)->getRules()->getFloorSprite();

					if ((*i)->getBodyUnit()->getUnitFire() != 0)
						*fire = true;
				}
			}

			if (spriteId == -1)
			{
				weight = -1;
				for (std::vector<BattleItem*>::const_iterator // 3. corpse
						i = _inventory.begin();
						i != _inventory.end();
						++i)
				{
					if ((*i)->getRules()->getBattleType() == BT_CORPSE
						&& (weightTest = (*i)->getRules()->getWeight()) > weight)
					{
						weight = weightTest;
						spriteId = (*i)->getRules()->getFloorSprite();
					}
				}
			}
		}
	}
	return spriteId;
}

/**
 * Gets the topmost floorob-sprite to draw on the battlefield.
 * @param fuse - pointer to set fuse true
 * @return, sprite-ID in floorobs (-1 none)
 */
int Tile::getTopSprite(bool* fuse) const
{
	*fuse = false;

	int spriteId (-1);
	if (_inventory.empty() == false)
	{
		const BattleItem* grenade (nullptr);

		for (std::vector<BattleItem*>::const_iterator
				i = _inventory.begin();
				i != _inventory.end();
				++i)
		{
			if ((*i)->getRules()->getBattleType() != BT_CORPSE
				&& (*i)->getFuse() > -1)
			{
				switch ((*i)->getRules()->getBattleType())
				{
					case BT_GRENADE:
						*fuse = true;
						grenade = *i;
						break;

					case BT_PROXYGRENADE: // proxy has precedence over grenades
						*fuse = true;
						return (*i)->getRules()->getFloorSprite();
				}
			}
		}

		if (grenade != nullptr)
			return grenade->getRules()->getFloorSprite(); // TODO: Find heaviest fused grenade if more than one.

		int
			weight (-1),
			weightTest;

		for (std::vector<BattleItem*>::const_iterator
				i = _inventory.begin();
				i != _inventory.end();
				++i)
		{
			if ((*i)->getRules()->getBattleType() != BT_CORPSE
				&& (weightTest = (*i)->getRules()->getWeight()) > weight)
			{
				weight = weightTest;
				spriteId = (*i)->getRules()->getFloorSprite();
			}
		}
	}
	return spriteId;
}

/**
 * Gets if this Tile has an unconscious unit in its inventory.
 * @param playerOnly - true to check for only xCom units (default true)
 * @return,	0 - no living Soldier
 *			1 - stunned Soldier
 *			2 - stunned and wounded Soldier
 */
int Tile::hasUnconsciousUnit(bool playerOnly) const
{
	int ret (0);
	for (std::vector<BattleItem*>::const_iterator
			i = _inventory.begin();
			i != _inventory.end();
			++i)
	{
		const BattleUnit* const unit ((*i)->getBodyUnit());

		if (unit != nullptr
			&& unit->getUnitStatus() == STATUS_UNCONSCIOUS
			&& (unit->getOriginalFaction() == FACTION_PLAYER
				|| playerOnly == false))
		{
			if (playerOnly == true && unit->getFatalsTotal() == 0)
				ret = 1;
			else
				return 2;
		}
	}
	return ret;
}

/**
 * Checks this Tile for a primed grenade.
 * return, true if primed grendades lies here
 */
bool Tile::hasPrimedGrenade() const
{
	for (std::vector<BattleItem*>::const_iterator
			i = _inventory.begin();
			i != _inventory.end();
			++i)
	{
		if ((*i)->getRules()->isGrenade() == true && (*i)->getFuse() != -1)
			return true;
	}
	return false;
}

/**
 * Gets the inventory on this Tile.
 * @return, pointer to a vector of pointers to BattleItems
 */
std::vector<BattleItem*>* Tile::getInventory()
{
	return &_inventory;
}

/**
 * Sets this Tile's visible-flag.
 * @note Used only by sneakyAI.
 * @param vis - true if visible (default true)
 *
void Tile::setTileVisible(bool vis)
{
	_visible = vis;
} */
/**
 * Gets this Tile's visible-flag.
 * @note Used only by sneakyAI.
 * @return, true if visible
 *
bool Tile::getTileVisible() const
{
	return _visible;
} */

/**
 * Sets the direction of path-preview-arrows.
 * @param dir - a direction
 */
void Tile::setPreviewDir(int dir)
{
	_previewDir = dir;
}

/**
 * Gets the direction of path-preview-arrows.
 * @return, preview direction
 */
int Tile::getPreviewDir() const
{
	return _previewDir;
}

/**
 * Sets a number to be displayed by path-preview.
 * @param tu - quantity of TUs left if/when this Tile is reached
 */
void Tile::setPreviewTu(int tu)
{
	_previewTu = tu;
}

/**
 * Gets the number to be displayed for path-preview.
 * @return, quantity of TUs left if/when this Tile is reached
 */
int Tile::getPreviewTu() const
{
	return _previewTu;
}

/**
 * Sets the path-preview-marker- color on this Tile.
 * @param color - color of marker
 */
void Tile::setPreviewColor(Uint8 color)
{
	_previewColor = color;
}

/**
 * Gets the path-preview-marker-color on this Tile.
 * @return, color of marker
 */
int Tile::getPreviewColor() const
{
	return static_cast<int>(_previewColor);
}

/**
 * Sets the danger-flag on this Tile.
 * @param danger - true if the AI regards the tile as dangerous (default true)
 */
void Tile::setDangerous(bool danger)
{
	_danger = danger;
}

/**
 * Gets the danger-flag on this Tile.
 * @return, true if the AI regards the tile as dangerous
 */
bool Tile::getDangerous() const
{
	return _danger;
}

}
