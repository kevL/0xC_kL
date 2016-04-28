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

#include "Tile.h"

//#include <algorithm>

#include "BattleItem.h"
#include "SerializationHelper.h"

#include "../fmath.h"

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
	2, // _mapDataSetId, four of these
	2, // _mapDataId, four of these
	1, // _fire
	1, // _smoke
	1, // _animOffset
	1, // one 8-bit bool field
	4 + (2 * 4) + (2 * 4) + 1 + 1 + 1 + 1 // total bytes to save one tile
};


/**
 * Creates a Tile.
 * @param pos - reference to a Position
 */
Tile::Tile(const Position& pos)
	:
		_pos(pos),
		_smoke(0),
		_fire(0),
		_explosive(0),
		_explosiveType(DT_NONE),
		_unit(nullptr),
		_animOffset(0),
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
		_parts[i]			=  nullptr;
		_mapDataId[i]		= -1;
		_mapDataSetId[i]	= -1;
		_curFrame[i]		=  0;
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
 * Load this Tile from a YAML node.
 * @param node - reference a YAML node
 */
void Tile::load(const YAML::Node& node)
{
	for (size_t
			i = 0u;
			i != PARTS_TILE;
			++i)
	{
		_mapDataId[i]		= node["mapDataID"][i]		.as<int>(_mapDataId[i]);
		_mapDataSetId[i]	= node["mapDataSetID"][i]	.as<int>(_mapDataSetId[i]);
	}

	_fire		= node["fire"]		.as<int>(_fire);
	_smoke		= node["smoke"]		.as<int>(_smoke);
	_animOffset	= node["animOffset"].as<int>(_animOffset);

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
 * Load the tile from binary.
 * @param buffer - pointer to buffer
 * @param serKey - reference the serialization key
 */
void Tile::loadBinary(
		Uint8* buffer,
		Tile::SerializationKey& serKey)
{
	_mapDataId[O_FLOOR]		= unserializeInt(&buffer, serKey._mapDataId);
	_mapDataId[O_WESTWALL]	= unserializeInt(&buffer, serKey._mapDataId);
	_mapDataId[O_NORTHWALL]	= unserializeInt(&buffer, serKey._mapDataId);
	_mapDataId[O_OBJECT]	= unserializeInt(&buffer, serKey._mapDataId);

	_mapDataSetId[O_FLOOR]		= unserializeInt(&buffer, serKey._mapDataSetId);
	_mapDataSetId[O_WESTWALL]	= unserializeInt(&buffer, serKey._mapDataSetId);
	_mapDataSetId[O_NORTHWALL]	= unserializeInt(&buffer, serKey._mapDataSetId);
	_mapDataSetId[O_OBJECT]		= unserializeInt(&buffer, serKey._mapDataSetId);

	_smoke		= unserializeInt(&buffer, serKey._smoke);
	_fire		= unserializeInt(&buffer, serKey._fire);
	_animOffset	= unserializeInt(&buffer, serKey._animOffset);

	const int boolFields (unserializeInt(
									&buffer,
									serKey.boolFields));

	_revealed[ST_WEST]		= (boolFields & 0x01) ? true : false;
	_revealed[ST_NORTH]		= (boolFields & 0x02) ? true : false;
	_revealed[ST_CONTENT]	= (boolFields & 0x04) ? true : false;

	_curFrame[O_WESTWALL]	= (boolFields & 0x08) ? 7 : 0;
	_curFrame[O_NORTHWALL]	= (boolFields & 0x10) ? 7 : 0;

//	if (_fire || _smoke)
//		_animationOffset = std::rand() %4;
}

/**
 * Saves the tile to a YAML node.
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
		node["mapDataID"].push_back(_mapDataId[i]);
		node["mapDataSetID"].push_back(_mapDataSetId[i]);
	}

	if (_smoke != 0)		node["smoke"]		= _smoke;
	if (_fire != 0)			node["fire"]		= _fire;
	if (_animOffset != 0)	node["animOffset"]	= _animOffset;

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

	if (isUfoDoorOpen(O_WESTWALL) == true)
		node["openDoorWest"] = true;
	if (isUfoDoorOpen(O_NORTHWALL) == true)
		node["openDoorNorth"] = true;

	return node;
}

/**
 * Saves the tile to binary.
 * @param buffer - pointer to pointer to buffer
 */
void Tile::saveBinary(Uint8** buffer) const
{
	serializeInt(buffer, serializationKey._mapDataId, _mapDataId[O_FLOOR]);
	serializeInt(buffer, serializationKey._mapDataId, _mapDataId[O_WESTWALL]);
	serializeInt(buffer, serializationKey._mapDataId, _mapDataId[O_NORTHWALL]);
	serializeInt(buffer, serializationKey._mapDataId, _mapDataId[O_OBJECT]);

	serializeInt(buffer, serializationKey._mapDataSetId, _mapDataSetId[O_FLOOR]);
	serializeInt(buffer, serializationKey._mapDataSetId, _mapDataSetId[O_WESTWALL]);
	serializeInt(buffer, serializationKey._mapDataSetId, _mapDataSetId[O_NORTHWALL]);
	serializeInt(buffer, serializationKey._mapDataSetId, _mapDataSetId[O_OBJECT]);

	serializeInt(buffer, serializationKey._smoke,		_smoke);
	serializeInt(buffer, serializationKey._fire,		_fire);
	serializeInt(buffer, serializationKey._animOffset,	_animOffset);

	int boolFields ((_revealed[ST_WEST] ? 0x01 : 0x0) + (_revealed[ST_NORTH] ? 0x02 : 0x0) + (_revealed[ST_CONTENT] ? 0x04 : 0x0));

	boolFields |= isUfoDoorOpen(O_WESTWALL)  ? 0x08 : 0x0;
	boolFields |= isUfoDoorOpen(O_NORTHWALL) ? 0x10 : 0x0;

	serializeInt(
				buffer,
				serializationKey.boolFields,
				boolFields);
}

/**
 * Sets the MapData references of parts 0 to 3.
 * @param data		- pointer to MapData
 * @param dataId	- dataID
 * @param dataSetId	- dataSetID
 * @param partType	- the part type (MapData.h)
 */
void Tile::setMapData(
		MapData* const data,
		const int dataId,
		const int dataSetId,
		const MapDataType partType)
{
	_parts[partType] = data;
	_mapDataId[partType] = dataId;
	_mapDataSetId[partType] = dataSetId;
}

/**
 * Gets the MapData references of parts 0 to 3.
 * @param dataId	- pointer to dataID
 * @param dataSetId	- pointer to dataSetID
 * @param partType	- the part type (MapData.h)
 */
void Tile::getMapData(
		int* dataId,
		int* dataSetId,
		MapDataType partType) const
{
	*dataId = _mapDataId[partType];
	*dataSetId = _mapDataSetId[partType];
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
	bool ret (_parts[O_FLOOR] == nullptr
		   && _parts[O_WESTWALL] == nullptr
		   && _parts[O_NORTHWALL] == nullptr
		   && _parts[O_OBJECT] == nullptr);

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
	if (_parts[partType] != nullptr
		&& (_parts[partType]->isUfoDoor() == false
			|| _curFrame[partType] < 2))
	{
		switch (partType)
		{
			case O_OBJECT:
				switch (_parts[O_OBJECT]->getBigwall())
				{
					case BIGWALL_NONE:
					case BIGWALL_BLOCK:
					case BIGWALL_NESW:
					case BIGWALL_NWSE:
						return _parts[partType]->getTuCostPart(type); // question: Why do side-bigwalls return 0.
				}
				break;

			default:
				return _parts[partType]->getTuCostPart(type);
		}
	}
	return 0;
}

/**
 * Checks whether this Tile has a reasonable floor or not.
 * @param tileBelow - the tile below this Tile (default nullptr)
 * @return, true if tile has no floor
 */
bool Tile::hasNoFloor(const Tile* const tileBelow) const
{
	if (_pos.z == 0
		|| (tileBelow != nullptr
			&& tileBelow->getTerrainLevel() == -24))
	{
		return false;
	}

	if (_parts[O_FLOOR] != nullptr)
		return _parts[O_FLOOR]->isNoFloor();

	return true;
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
 * Gets the terrain level of this Tile.
 * @note For graphical Y offsets etc. Terrain level starts and 0 and goes
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
 * Gets this Tile's footstep sound.
 * @param tileBelow - pointer to the Tile below this Tile
 * @return, sound ID
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
 * Open a door on this Tile.
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
		if (_parts[partType]->isDoor() == true)
		{
			if (_unit != nullptr
				&& _unit != unit
				&& _unit->getPosition() != getPosition())
			{
				return DR_NONE;
			}

			if (unit != nullptr
				&& unit->getTimeUnits() < _parts[partType]->getTuCostPart(unit->getMoveTypeUnit()))
//											+ unit->getActionTu(reserved, unit->getMainHandWeapon()))
			{
				return DR_ERR_TU;
			}

			setMapData(
					_parts[partType]->getDataset()->getRecords()->at(_parts[partType]->getAltMCD()),
					_parts[partType]->getAltMCD(),
					_mapDataSetId[partType],
					_parts[partType]->getDataset()->getRecords()->at(_parts[partType]->getAltMCD())->getPartType());

			setMapData(nullptr,-1,-1, partType);

			return DR_WOOD_OPEN;
		}

		if (_parts[partType]->isUfoDoor() == true)
		{
			if (_curFrame[partType] == 0) // ufo door part 0 - door is closed
			{
				if (unit != nullptr
					&& unit->getTimeUnits() < _parts[partType]->getTuCostPart(unit->getMoveTypeUnit()))
//												+ unit->getActionTu(reserved, unit->getMainHandWeapon()))
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
 * Opens a door without checks.
 * @param partType - a tile-part type (MapData.h)
 */
void Tile::openDoorAuto(const MapDataType partType)
{
//	if (_parts[partType]->isDoor() == true)
//	{
//		setMapData(
//				_parts[partType]->getDataset()->getRecords()->at(_parts[partType]->getAltMCD()),
//				_parts[partType]->getAltMCD(),
//				_mapDataSetId[partType],
//				_parts[partType]->getDataset()->getRecords()->at(_parts[partType]->getAltMCD())->getPartType());
//
//		setMapData(nullptr,-1,-1, partType);
//	}
//	else if (_parts[partType]->isUfoDoor() == true)

	_curFrame[partType] = 1; // start opening door
}

/**
 * Closes a ufo-door on this Tile.
 * @return, true if a door closed
 */
bool Tile::closeUfoDoor()
{
	int ret (false);
	for (size_t
			i = 0u;
			i != PARTS_TILE;
			++i)
	{
		if (isUfoDoorOpen(static_cast<MapDataType>(i)) == true)
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
 * @param section - the SectionType (Tile.h)
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
		_revealed[section] = revealed;

		if (revealed == true && section == ST_CONTENT) // NOTE: Try no-reveal (walls) if content is diag BigWall.
		{
			if (_parts[O_OBJECT] == nullptr
				|| (_parts[O_OBJECT]->getBigwall() & 0x6) == 0) // NeSw, NwSe
			{
				_revealed[ST_WEST] = // if object+floor is revealed set west- & north-walls revealed also.
				_revealed[ST_NORTH] = true;
			}
		}

		if (_unit != nullptr) _unit->flagCache();
	}
}

/**
 * Gets the black fog-of-war/revealed status of this Tile.
 * @param section - the SectionType (Tile.h)
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
 * @note Returns the brightest of all light-layers.
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
 * Destroys a part on this Tile.
 * @note First remove the old object then replace it with the destroyed one
 * because the object type of the old and new are not necessarily the same. If
 * the destroyed part is an explosive set the tile's explosive value which will
 * trigger a chained explosion.
 * @param partType		- this Tile's part for destruction (MapData.h)
 * @param battleSave	- pointer to the SavedBattleGame
 */
void Tile::destroyTilepart(
		MapDataType partType,
		SavedBattleGame* const battleSave)
{
	int tLevel (0);

	if (_parts[partType] != nullptr)
	{
		if (_parts[partType]->isGravLift() == true
			|| _parts[partType]->getArmor() == 255) // <- set to 255 in MCD for Truly Indestructability.
		{
			return;
		}

		if (partType == O_OBJECT)
			tLevel = _parts[O_OBJECT]->getTerrainLevel();

		if (_parts[partType]->getSpecialType() == battleSave->getObjectiveType())
			battleSave->addDestroyedObjective();

		const MapData* const data (_parts[partType]);
		const int dataSetId (_mapDataSetId[partType]);

		setMapData(nullptr,-1,-1, partType);

		if (data->getDieMCD() != 0)
		{
			MapData* const dataDead (data->getDataset()->getRecords()->at(data->getDieMCD()));
			setMapData(
					dataDead,
					data->getDieMCD(),
					dataSetId,
					dataDead->getPartType());
		}

		if (data->getExplosive() != 0)
			setExplosive(
					data->getExplosive(),
					data->getExplosiveType());
	}

	if (partType == O_FLOOR) // check if the floor on the lowest level is gone.
	{
		if (_pos.z == 0 && _parts[O_FLOOR] == nullptr)
			setMapData( // replace with scorched earth
					MapDataSet::getScorchedEarthTile(),
					1,0, O_FLOOR);

		if (_parts[O_OBJECT] != nullptr // destroy the object if floor is gone.
			&& _parts[O_OBJECT]->getBigwall() == BIGWALL_NONE)
		{

			destroyTilepart(O_OBJECT, battleSave); // stop floating haybales.
		}
	}

	if (tLevel == -24) // destroy the object-above if its support is gone.
	{
		const Tile* const tileAbove (battleSave->getTile(_pos + Position(0,0,1)));
		if (tileAbove != nullptr
			&& tileAbove->getMapData(O_FLOOR) == nullptr
			&& tileAbove->getMapData(O_OBJECT) != nullptr
			&& tileAbove->getMapData(O_OBJECT)->getBigwall() == BIGWALL_NONE)
		{
			destroyTilepart(O_OBJECT, battleSave); // stop floating lampposts.
		}
	}
}

/**
 * Damages terrain (check against terrain-part armor/hitpoints/constitution).
 * @param partType		- part of tile to check (MapData.h)
 * @param power			- power of the damage
 * @param battleSave	- pointer to the SavedBattleGame
 */
void Tile::hitTile(
		MapDataType partType,
		int power,
		SavedBattleGame* const battleSave)
{
	//Log(LOG_INFO) << "Tile::damage() vs partType = " << partType << ", hp = " << _parts[partType]->getArmor();
	if (power >= _parts[partType]->getArmor())
		destroyTilepart(partType, battleSave);
}

/**
 * Sets a "virtual" explosive on this Tile.
 * @note Mark a tile this way to detonate it later because the same tile can be
 * visited multiple times by "explosion rays". The explosive power that gets set
 * on a tile is that of the most powerful ray that passes through it -- see
 * TileEngine::explode().
 * @param power		- how big the BOOM will be / how much tile-destruction
 * @param explType	- the type of this Tile's explosion (set in MCD) (RuleItem.h)
 * @param force		- forces value even if lower (default false)
 */
void Tile::setExplosive(
		int power,
		DamageType explType,
		bool force)
{
	if (force == true || _explosive < power)
	{
		_explosive = power;
		_explosiveType = explType;
	}
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
 * @return, the lower the value the higher the chance the tile catches fire - BSZAAST!!!
 */
int Tile::getFlammability() const
{
	int burn (255); // not burnable. <- lower is better :)
	for (size_t
			i = 0u;
			i != PARTS_TILE;
			++i)
	{
		if (_parts[i] != nullptr
			&& _parts[i]->getFlammable() < burn)
		{
			burn = _parts[i]->getFlammable();
		}
	}
	return convertBurnToPct(burn);
}

/**
 * Gets the flammability of a tile-part.
 * @note I now decree that this returns the inverse of 0..255 as a percentage!
 * @param partType - the part to check (MapData.h)
 * @return, the lower the value the higher the chance the tile-part catches fire - BSZAAST!!!
 */
int Tile::getFlammability(MapDataType partType) const
{
	return convertBurnToPct(_parts[partType]->getFlammable());
}

/**
 * Converts obscure inverse MCD notation to understandable percentages.
 * @note Chance can be increased by the power of the spark.
 * @param burn - flammability from an MCD file (see MapData)
 * @return, basic percent chance that this stuff burns
 */
int Tile::convertBurnToPct(int burn) const // private.
{
	if (burn > 254)
		return 0;

	burn = 255 - burn;
	burn = std::max(1,
					std::min(100,
							static_cast<int>(std::ceil(
							static_cast<double>(burn) / 255. * 100.))));

	return burn;
}

/**
 * Gets the fuel of a tile-part.
 * @note Fuel of a tile is the highest fuel of its parts/objects. This is NOT
 * the sum of the fuel of the objects!
 * @param partType - the part to check or O_NULPART to check all parts (default O_NULPART) (MapData.h)
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
			if (_parts[i] != nullptr
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
 * @note If true it will add its fuel as turns to burn.
 * @note Called by floor-burning Silacoids and fire spreading @ turnovers and
 * by TileEngine::detonate() after HE explosions.
 * @param power - rough chance to get things going
 * @return, true if tile catches fire
 */
bool Tile::ignite(int power)
{
	if (power != 0 && allowSmoke() == true)
	{
		const int fuel (getFuel());
		if (fuel != 0)
		{
			const int burn (getFlammability());
			if (burn != 0)
			{
				power = ((((power + 4) / 5) + ((burn + 7) / 8) + (fuel * 3) + 6) / 7);
				if (RNG::percent(power) == true)
				{
					addSmoke((burn + 15) / 16);

					// TODO: pass in tileBelow and check its terrainLevel for -24; drop fire through to any tileBelow ...
					if (allowFire() == true)
						addFire(fuel + 1);

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
			_animOffset = RNG::seedless(0,3);

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
 */
void Tile::decreaseFire()
{
	if (--_fire < 1)
	{
		_fire = 0;
		if (_smoke == 0)
			_animOffset = 0;
	}
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
			_animOffset = RNG::seedless(0,3);

		if ((_smoke += turns) > 17)
			_smoke = 17;
	}
}

/**
 * Reduces the number of turns this Tile will smoke for.
 */
void Tile::decreaseSmoke()
{
	if (_fire != 0) // don't let smoke deplete faster than fire depletes.
		--_smoke;
	else
		_smoke -= (RNG::generate(1, _smoke) + 2) / 3;

	if (_smoke < 1)
	{
		_smoke = 0;
		if (_fire == 0)
			_animOffset = 0;
	}
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
	if (_parts[O_FLOOR] != nullptr
		&& _parts[O_FLOOR]->blockFire() == true)
	{
		return false;
	}

	if (_parts[O_OBJECT] != nullptr)
	{
		switch (_parts[O_OBJECT]->getBigwall())
		{
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
 * @param battleSave - pointer to the current SavedBattleGame (default nullptr= hits units)
 */
void Tile::hitTileInventory(SavedBattleGame* const battleSave)
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
				if (dur > _unit->getFireUnit())
					_unit->setFireUnit(dur);
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
				if ((unit = (*i)->getUnit()) != nullptr
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
					if ((unit = (*i)->getUnit()) != nullptr
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
								unit->instaKill();
								unit->killerFaction(unit->getFaction()); // killed by self ....
							}
						}
						done = (++i == _inventory.end());
					}
					else if (powerFire > (*i)->getRules()->getArmor() // no modifier when destroying items, not even corpse in bodyarmor.
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
 * Animate the tile.
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
					if (_parts[i]->isUfoDoor() == false
						|| (_curFrame[i] != 0
							&& _curFrame[i] != 7)) // ufo-door is currently static
					{
						nextFrame = _curFrame[i] + 1;

						if (_parts[i]->isUfoDoor() == true // special handling for Avenger & Lightning doors
							&& _parts[i]->getSpecialType() == START_POINT
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
 * Get the number of frames the fire or smoke animation is off-sync.
 * @note To void fire and smoke animations of different tiles moving nice in
 * sync - that'd look fake.
 * @return, offset
 */
int Tile::getAnimationOffset() const
{
	return _animOffset;
}

/**
 * Get the sprite of a certain part of this Tile.
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
 * Sets a unit on this Tile.
 * @param unit		- pointer to a BattleUnit (default nullptr)
 * @param tileBelow	- pointer to the Tile below this Tile (default nullptr)
 */
void Tile::setUnit(
		BattleUnit* const unit,
		const Tile* const tileBelow)
{
	if (unit != nullptr)
		unit->setTile(this, tileBelow);

	_unit = unit;
}

/**
 * Sets a unit transitorily on this Tile.
 * @param unit - pointer to a BattleUnit
 */
void Tile::setTransitUnit(BattleUnit* const unit)
{
	_unit = unit;
}

/**
 * Adds an item on this Tile.
 * @param item - pointer to a BattleItem
 */
void Tile::addItem(BattleItem* const item)
{
	item->setTile(this);
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
	item->setTile();
}

/**
 * Gets a corpse-sprite to draw on the battlefield.
 * @param fired - pointer to set fire true
 * @return, sprite ID in floorobs (-1 none)
 */
int Tile::getCorpseSprite(bool* fired) const
{
	int sprite (-1);
	if (_inventory.empty() == false)
	{
		*fired = false;
		int
			weight (-1),
			weightTest;

		for (std::vector<BattleItem*>::const_iterator // 1. soldier body
				i = _inventory.begin();
				i != _inventory.end();
				++i)
		{
			if ((*i)->getUnit() != nullptr
				&& (*i)->getUnit()->getGeoscapeSoldier() != nullptr)
			{
				weightTest = (*i)->getRules()->getWeight();
				if (weightTest > weight)
				{
					weight = weightTest;
					sprite = (*i)->getRules()->getFloorSprite();

					if ((*i)->getUnit()->getFireUnit() != 0)
						*fired = true;
				}
			}
		}

		if (sprite == -1)
		{
			weight = -1;
			for (std::vector<BattleItem*>::const_iterator // 2. non-soldier body
					i = _inventory.begin();
					i != _inventory.end();
					++i)
			{
				if ((*i)->getUnit() != nullptr)
				{
					weightTest = (*i)->getRules()->getWeight();
					if (weightTest > weight)
					{
						weight = weightTest;
						sprite = (*i)->getRules()->getFloorSprite();

						if ((*i)->getUnit()->getFireUnit() != 0)
							*fired = true;
					}
				}
			}

			if (sprite == -1)
			{
				weight = -1;
				for (std::vector<BattleItem*>::const_iterator // 3. corpse
						i = _inventory.begin();
						i != _inventory.end();
						++i)
				{
					if ((*i)->getRules()->getBattleType() == BT_CORPSE)
					{
						weightTest = (*i)->getRules()->getWeight();
						if (weightTest > weight)
						{
							weight = weightTest;
							sprite = (*i)->getRules()->getFloorSprite();
						}
					}
				}
			}
		}
	}

	return sprite;
}

/**
 * Get the topmost item sprite to draw on the battlefield.
 * @param primed - pointer to set primed true
 * @return, sprite ID in floorobs (-1 none)
 */
int Tile::getTopSprite(bool* primed) const
{
	int sprite (-1);
	if (_inventory.empty() == false)
	{
		const BattleItem* grenade (nullptr);
		*primed = false;
		BattleType bType;

		for (std::vector<BattleItem*>::const_iterator
				i = _inventory.begin();
				i != _inventory.end();
				++i)
		{
			if ((*i)->getFuse() > -1)
			{
				bType = (*i)->getRules()->getBattleType();
				if (bType == BT_PROXYGRENADE)
				{
					*primed = true;
					return (*i)->getRules()->getFloorSprite();
				}
				else if (bType == BT_GRENADE)
				{
					*primed = true;
					grenade = *i;
				}
			}
		}

		if (grenade != nullptr)
			return grenade->getRules()->getFloorSprite();

		int
			weight (-1),
			weightTest;

		for (std::vector<BattleItem*>::const_iterator
				i = _inventory.begin();
				i != _inventory.end();
				++i)
		{
			weightTest = (*i)->getRules()->getWeight();
			if (weightTest > weight)
			{
				weight = weightTest;
				sprite = (*i)->getRules()->getFloorSprite();
			}
		}
	}

	return sprite;
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
		const BattleUnit* const unit ((*i)->getUnit());

		if (unit != nullptr
			&& unit->getUnitStatus() == STATUS_UNCONSCIOUS
			&& (unit->getOriginalFaction() == FACTION_PLAYER
				|| playerOnly == false))
		{
			if (playerOnly == true && unit->getFatalWounds() == 0)
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
