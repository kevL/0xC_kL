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

#ifndef OPENXCOM_TILE_H
#define OPENXCOM_TILE_H

//#include <list>
//#include <vector>
//#include <SDL_types.h>

#include "BattleUnit.h"

#include "../Battlescape/Position.h"

#include "../Ruleset/MapData.h"
//#include "../Ruleset/RuleItem.h" // DamageType enum, already in MapData.h


namespace OpenXcom
{

enum DoorResult
{
	DR_NONE = -1,	// -1
	DR_WOOD_OPEN,	//  0
	DR_UFO_OPEN,	//  1
	DR_UFO_WAIT,	//  2
	DR_ERR_TU,		//  3
	DR_ERR_RESERVE	//  4
};

enum SectionType
{
	ST_WEST,	// 0
	ST_NORTH,	// 1
	ST_CONTENT	// 2
};


class BattleItem;
class BattleUnit;
class MapData;
class RuleInventory;
class SavedBattleGame;
class Surface;


/**
 * Basic element of which a battle map is built.
 * @sa http://www.ufopaedia.org/index.php?title=MAPS
 */
class Tile
{

	public:
		static const size_t PARTS_TILE = 4u;

private:
	static const int LIGHT_FULL = 15;

	static const size_t
		LIGHTLAYERS	= 3u,
		SECTIONS	= 3u;

	bool
		_danger,
		_revealed[SECTIONS],
		_visible;
	int
		_animOffset,
		_curFrame[PARTS_TILE],
		_explosive,
		_fire,
		_light[LIGHTLAYERS],
		_mapDataId[PARTS_TILE],
		_mapDataSetId[PARTS_TILE],
		_previewDir,
		_previewTu,
		_smoke;
	Uint8 _previewColor;

	DamageType _explosiveType;

	BattleUnit* _unit;
	MapData* _parts[PARTS_TILE];

	Position _pos;

	std::vector<BattleItem*> _inventory;

	/// Checks if the Tile accepts smoke.
	bool allowSmoke() const;
	/// Checks if the Tile accepts fire.
	bool allowFire() const;

	/// Converts obscure inverse MCD notation to understandable percentages.
	int convertBurnToPct(int burn) const;


	public:
		static struct SerializationKey
		{
			// how many bytes to store for each variable or each member of array of the same name
			Uint8 index; // for indexing the actual tile array
			Uint8 _mapDataSetId;
			Uint8 _mapDataId;
			Uint8 _smoke;
			Uint8 _fire;
			Uint8 _animOffset;
			Uint8 boolFields;
			Uint32 totalBytes; // per structure including any data not mentioned here and accounting for all array members
		} serializationKey;

		/// Creates a Tile.
		explicit Tile(const Position& pos);
		/// Cleans up the Tile.
		~Tile();

		/// Loads the tile from yaml.
		void load(const YAML::Node& node);
		/// Loads the tile from binary buffer in memory.
		void loadBinary(
				Uint8* buffer,
				Tile::SerializationKey& serializationKey);
		/// Saves the tile to yaml.
		YAML::Node save() const;
		/// Saves the tile to binary
		void saveBinary(Uint8** buffer) const;

		/**
		 * Gets a pointer to MapData for a part of the Tile.
		 * @param part - the part 0-3
		 * @return, pointer to MapData
		 */
		MapData* getMapData(MapDataType partType) const
		{ return _parts[partType]; }

		/// Sets the pointer to the mapdata for a specific part of the Tile.
		void setMapData(
				MapData* const data,
				const int dataID,
				const int dataSetID,
				const MapDataType partType);
		/// Gets the IDs of the mapdata for a specific part of the Tile.
		void getMapData(
				int* dataID,
				int* dataSetID,
				MapDataType partType) const;

		/// Gets whether the Tile has no objects
		bool isVoid(
				const bool testInventory = true,
				const bool testVolatiles = true) const;

		/// Gets the TU cost to walk over a certain part of the Tile.
		int getTuCostTile(
				MapDataType partType,
				MoveType type) const;

		/// Checks if the Tile has a floor.
		bool hasNoFloor(const Tile* const tileBelow = nullptr) const;

		/// Checks if the Tile is a big wall.
//		bool isBigWall() const;

		/// Gets terrain level.
		int getTerrainLevel() const;

		/**
		 * Gets the Tile's position.
		 * @return, position
		 */
		const Position& getPosition() const
		{ return _pos; }

		/// Gets the floor object footstep sound.
		int getFootstepSound(const Tile* const tileBelow) const;

		/// Opens a door.
		DoorResult openDoor(
				const MapDataType partType,
				const BattleUnit* const unit = nullptr);
//				const BattleActionType reserved = BA_NONE);
		/// Opens a door without checks.
		void openDoorAuto(const MapDataType partType);
		/**
		 * Checks if a ufo-door is open or opening.
		 * @note Used for visibility/light blocking checks. This function
		 * assumes that there are never 2 doors on 1 tile or a door and another
		 * wall on 1 tile.
		 * @param partType - the tile-part to consider
		 * @return, true if ufo-door is valid and not closed
		 */
		bool isUfoDoorOpen(MapDataType partType) const
		{	return _parts[partType] != nullptr
				&& _parts[partType]->isUfoDoor() == true
				&& _curFrame[partType] != 0; }
		/// Closes a ufo-door.
		bool closeUfoDoor();

		/// Sets the black fog-of-war status of a tile-section.
		void setRevealed(
				SectionType section,
				bool revealed = true);
		/// Gets the black fog-of-war status of a tile-section.
		bool isRevealed(SectionType section) const;

		/// Resets light to zero for the Tile.
		void resetLight(size_t layer);
		/// Adds light to the Tile.
		void addLight(
				int light,
				size_t layer);
		/// Gets the shade amount.
		int getShade() const;

		/// Destroys a tile-part.
		void destroyTilepart(
				MapDataType partType,
				SavedBattleGame* const battleSave);
		/// Damages a tile-part.
		void hitTile(
				MapDataType partType,
				int power,
				SavedBattleGame* const battleSave);

		/// Sets a virtual explosive on the Tile to detonate later.
		void setExplosive(
				int power,
				DamageType explType,
				bool force = false);
		/// Gets explosive power of the Tile.
		int getExplosive() const;
		/// Gets explosive type of the Tile.
		DamageType getExplosiveType() const;

		/// Gets flammability.
		int getFlammability() const;
		/// Gets flammability of part.
		int getFlammability(MapDataType partType) const;

		/// Gets turns to burn of part.
		int getFuel(MapDataType partType = O_NULPART) const;

		/// Tries to start fire on the Tile.
		bool ignite(int power);
		/// Adds fire to the Tile.
		bool addFire(int turns);
		/// Reduces the number of turns the Tile will burn.
		void decreaseFire();
		/// Gets fire.
		int getFire() const;	// kL_note: Made this inline, but may result in UB if say BattleUnit->getFire() conflicts.
//		{ return _fire; }		// So ... don't. ie: change function names, THANKS c++
								// ps. I changed the BattleUnit class function identifier to "getFireUnit" .....

		/// Adds smoke to the Tile.
		void addSmoke(int turns);
		/// Reduces the number of turns the Tile will smoke.
		void decreaseSmoke();
		/// Gets smoke.
		int getSmoke() const; // kL_note: Made this inline, but may result in UB if say BattleUnit->getFire() conflicts. So ... don't.
//		{ return _smoke; }

		/// Ends the Tile's turn. Units catch on fire.
		void hitTileInventory(SavedBattleGame* const battleSave = nullptr);

		/// Animates the tile parts.
		void animateTile();
		/// Gets fire and smoke animation offset.
		int getAnimationOffset() const;

		/// Gets object sprites.
		Surface* getSprite(MapDataType partType) const;

		/// Sets a unit on the Tile.
		void setUnit(
				BattleUnit* const unit = nullptr,
				const Tile* const tileBelow = nullptr);
		/// Sets a unit transitorily on the Tile.
		void setTransitUnit(BattleUnit* const unit);
		/**
		 * Gets the (alive) unit on the Tile.
		 * @return, pointer to a BattleUnit
		 */
		BattleUnit* getTileUnit() const
		{ return _unit; }

		/// Adds item
		void addItem(BattleItem* const item);
		/// Removes item
		void removeItem(BattleItem* const item);

		/// Gets corpse-sprite.
		int getCorpseSprite(bool* fired) const;
		/// Gets top-most item-sprite.
		int getTopSprite(bool* primed) const;
		/// Gets if the Tile has an unconscious unit in its inventory.
		int hasUnconsciousUnit(bool playerOnly = true) const;

		/// Checks this Tile for a primed grenade.
		bool hasPrimedGrenade() const;

		/// Gets inventory on the Tile.
		std::vector<BattleItem*>* getInventory();

		/// Sets the Tile visible flag.
//		void setTileVisible(bool vis = true);
		/// Gets the Tile visible flag.
//		bool getTileVisible() const;

		/// Sets the direction of path preview arrows.
		void setPreviewDir(int dir);
		/// Gets the direction of path preview arrows.
		int getPreviewDir() const;
		/// Sets the number to be displayed for path preview.
		void setPreviewTu(int tu);
		/// Gets the number to be displayed for path preview.
		int getPreviewTu() const;
		/// Sets the preview Tile marker color.
		void setPreviewColor(Uint8 color);
		/// Gets the preview Tile marker color.
		int getPreviewColor() const;

		/// Sets the danger flag on the Tile so the AI may avoid it.
		void setDangerous(bool danger = true);
		/// Checks the danger flag on the Tile.
		bool getDangerous() const;
};

}

#endif
