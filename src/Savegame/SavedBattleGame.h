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

#ifndef OPENXCOM_SAVEDBATTLEGAME_H
#define OPENXCOM_SAVEDBATTLEGAME_H

//#include <string>
//#include <vector>
#include <yaml-cpp/yaml.h>

#include "BattleUnit.h"
//#include "../Battlescape/BattlescapeGame.h" // BattleActionType (included in BattleUnit.h)

#include "../Ruleset/RuleAlienDeployment.h"


namespace OpenXcom
{

enum TacticalType
{
	TCT_DEFAULT = -1,	// -1 init.
	TCT_UFOCRASHED,		//  0
	TCT_UFOLANDED,		//  1
	TCT_BASEASSAULT,	//  2
	TCT_BASEDEFENSE,	//  3
	TCT_TERRORSITE,		//  4
	TCT_MARS1,			//  5
	TCT_MARS2			//  6
};


class BattleItem;
class BattlescapeState;
class Game;
class MapDataSet;
class Node;
class OperationPool;
class Pathfinding;
class Position;
class Ruleset;
class SavedGame;
class State;
class Tile;
class TileEngine;


/**
 * The battlescape data that gets written to disk when the game is saved.
 * @note This holds all the variable info in a battlegame.
 */
class SavedBattleGame
{

private:
	static const size_t SEARCH_DIST = 11u;

	bool
		_aborted,
		_cheatAI,
		_controlDestroyed,
		_debugTac,
//		_kneelReserved,
		_pacified,
//		_preBattle,
		_unitsFalling;
	int
		_cheatTurn,
		_groundLevel,
		_initTu,
		_itemId,
		_mapsize_x,
		_mapsize_y,
		_mapsize_z,
		_objectivesDestroyed,
		_objectivesReq,
		_tacticalShade,
		_turn,
		_turnLimit;
	size_t _qtyTilesTotal;

//	BattleActionType _batReserved;
	ChronoResult _chronoResult;
	Position _rfTriggerPosition;
	TileType _objectiveType;
	TacticalType _tacType;
	UnitFaction _side;

	BattlescapeState* _battleState;
	BattleUnit
		* _selectedUnit,
		* _lastSelectedUnit;
	const BattleUnit* _walkUnit;
	Pathfinding* _pf;
	Tile
		* _equiptTile,
		** _tiles;
	TileEngine* _te;

	std::string
		_alienRace,
		_music,
		_tacticalType,
		_terrain;
	std::wstring _operationTitle;

	std::list<BattleUnit*> _fallingUnits;

//	std::set<Tile*> _detonationTiles;

	std::vector<BattleItem*>
		_toDelete,
		_items,
		_recoverConditional,
		_recoverGuaranteed;
	std::vector<BattleUnit*>
		_units,
		_shuffleUnits;
	std::vector<MapDataSet*> _mapDataSets;
	std::vector<Node*> _nodes;
	std::vector<Position>
		_storageSpace,
		_tileSearch;
	std::vector<std::vector<std::pair<int,int>>> _baseModules;

	std::vector<std::pair<int,int>> _scanDots;

//	Uint8 _dragButton;			// this is a cache for Options::getString("battleScrollDragButton")
//	bool _dragInvert;			// this is a cache for Options::getString("battleScrollDragInvert")
//	int
//		_dragTimeTolerance,		// this is a cache for Options::getInt("battleScrollDragTimeTolerance")
//		_dragPixelTolerance;	// this is a cache for Options::getInt("battleScrollDragPixelTolerance")

	/// Selects one of the Player's units when he/she begins each turn.
	void selectPlayerUnit();

	/// Selects a BattleUnit.
	BattleUnit* selectFactionUnit(
			int dir,
			bool checkReselect = false,
			bool dontReselect = false,
			bool checkInventory = false);

	/// Sets the TacticalType based on a specified mission-type.
	void setTacType(const std::string& type);


	public:
		static const size_t SEARCH_SIZE = SEARCH_DIST * SEARCH_DIST;

		/// Creates a new battle save based on the current generic save.
		explicit SavedBattleGame(
				const std::vector<OperationPool*>* const titles = nullptr,
				const Ruleset* const rules = nullptr);
		/// Cleans up the saved game.
		~SavedBattleGame();

		/// Loads a saved battle game from YAML.
		void load(
				const YAML::Node& node,
				Ruleset* const rules,
				const SavedGame* const gameSave);
		/// Load map resources.
		void loadMapResources(const Game* const game);
		/// Saves a saved battle game to YAML.
		YAML::Node save() const;

		/// Gets a pointer to the tiles.
		Tile** getTiles() const;

		/// Sets the dimensions of the map and initializes it.
		void initMap(
				const int mapsize_x,
				const int mapsize_y,
				const int mapsize_z);
		/// Initialises the Pathfinding and TileEngine.
		void initUtilities(const ResourcePack* const res);

		/// Gets the TacticalType of this battle.
		TacticalType getTacType() const;
		/// Sets the mission type.
		void setTacticalType(const std::string& type);
		/// Gets the mission type.
		std::string getTacticalType() const;

		/// Sets the global shade.
		void setTacticalShade(int shade);
		/// Gets the global shade.
		int getTacticalShade() const;

		/// Gets terrain size x.
		int getMapSizeX() const;
		/// Gets terrain size y.
		int getMapSizeY() const;
		/// Gets terrain size z.
		int getMapSizeZ() const;
		/// Gets terrain x*y*z.
		size_t getMapSizeXYZ() const;

		/// Sets the type of terrain for the mission.
		void setBattleTerrain(const std::string& terrain);
		/// Gets the type of terrain for the mission.
		std::string getBattleTerrain() const;

		/**
		 * Converts coordinates into a unique index.
		 * getTile() calls this every time, so should be inlined along with it.
		 * @note Are not functions that are defined inside the class def'n here
		 * supposedly assumed as 'inlined' ...... And the true result of keyword
		 * "inline" is actually something quite different. ah C++ clear as mud.
		 * @param pos - a position to convert
		 * @return, the unique index
		 */
		inline size_t getTileIndex(const Position& pos) const
		{ return static_cast<size_t>((pos.z * _mapsize_y * _mapsize_x) + (pos.y * _mapsize_x) + pos.x); }

		/// Converts a tile index to its coordinates.
		void tileCoords(
				size_t index,
				int* x,
				int* y,
				int* z) const;

		/**
		 * Gets the Tile at a given position on the map.
		 * @note This method is called over 50mil+ times per turn so it seems
		 * useful to inline it.
		 * @note Are not functions that are defined inside the class def'n here
		 * supposedly assumed as 'inlined' ...... And the true result of keyword
		 * "inline" is actually something quite different. ah C++ clear as mud.
		 * @param pos - reference a Map position
		 * @return, pointer to the tile at that position
		 */
		inline Tile* getTile(const Position& pos) const
		{
			if (   pos.x < 0
				|| pos.y < 0
				|| pos.z < 0
				|| pos.x >= _mapsize_x
				|| pos.y >= _mapsize_y
				|| pos.z >= _mapsize_z)
			{
				return nullptr;
			}
			return _tiles[getTileIndex(pos)];
		}

		/// Gets the currently selected BattleUnit.
		BattleUnit* getSelectedUnit() const;
		/// Sets the currently selected BattleUnit.
		void setSelectedUnit(BattleUnit* const unit = nullptr);
		/// Selects the next BattleUnit.
		BattleUnit* selectNextFactionUnit(
				bool checkReselect = false,
				bool dontReselect = false,
				bool checkInventory = false);
		/// Selects the previous BattleUnit.
		BattleUnit* selectPreviousFactionUnit(
				bool checkReselect = false,
				bool dontReselect = false,
				bool checkInventory = false);
		/// Selects the BattleUnit at a specified Position.
		BattleUnit* selectUnit(const Position& pos);

		/// Gets a pointer to the list of nodes.
		std::vector<Node*>* getNodes();
		/// Gets a pointer to the list of units.
		std::vector<BattleUnit*>* getUnits();
		/// Gets a pointer to the list of shuffled units.
		std::vector<BattleUnit*>* getShuffleUnits();
		/// Gets a pointer to the list of items.
		std::vector<BattleItem*>* getItems();

		/// Gets the pathfinding object.
		Pathfinding* getPathfinding() const;
		/// Gets a pointer to the tileengine.
		TileEngine* getTileEngine() const;

		/// Gets the game's mapdatafiles.
		std::vector<MapDataSet*>* getMapDataSets();

		/// Gets the playing side.
		UnitFaction getSide() const;

		/// Gets the turn.
		int getTurn() const;
		/// Ends the turn.
		bool endFactionTurn();

		/// Sets debug mode.
		void debugTac();
		/// Gets debug mode.
		bool getDebugTac() const;

		/// Gets a pointer to the SavedGame.
		SavedGame* getSavedGame() const;

		/// Gets a pointer to the BattlescapeGame.
		BattlescapeGame* getBattleGame() const;
		/// Gets a pointer to the BattlescapeState.
		BattlescapeState* getBattleState() const;
		/// Sets the pointer to the BattlescapeState.
		void setBattleState(BattlescapeState* const battleState);

		/// Sets all BattleUnits onto their start Tile(s) at the end of pre-battle equip.
		void resetUnitsOnTiles();

		/// Gives access to the storage-tiles vector.
		std::vector<Position>& getStorageSpace();
		/// Moves all the leftover items to random locations in the storage-tiles vector.
		void distributeEquipment(Tile* const tile);

		/// Removes a BattleItem from the battlefield.
		std::vector<BattleItem*>::const_iterator toDeleteItem(BattleItem* const item);
		/// Gives read-only access to the deleted-items vector.
		const std::vector<BattleItem*>& getDeletedItems() const;

		/// Sets if the mission was aborted or not.
		void setAborted(bool abort = true);
		/// Checks if the mission was aborted.
		bool isAborted() const;

		/// Sets the objective-tiletype for this mission.
		void setObjectiveTileType(TileType type);
		/// Gets the objective-tiletype of this mission.
		TileType getObjectiveTileType() const;
		/// Sets how many objective-tiles need to be destroyed.
		void setObjectiveTotal(int qty);
		/// Increments the objective-tiles destroyed counter.
		void addDestroyedObjective();
		/// Checks if enough objective-tiles are destroyed.
		bool allObjectivesDestroyed() const;

		/// Sets the next available item-ID value.
		void setCanonicalBattleId();
		/// Gets the next available item-ID value.
		int* getCanonicalBattleId();

		/// Gets a spawn-node.
		Node* getSpawnNode(
				int unitRank,
				BattleUnit* const unit);
		/// Gets a patrol-node.
		Node* getPatrolNode(
				bool scout,
				BattleUnit* const unit,
				Node* startNode);
		/// Gets the Node considered nearest to a BattleUnit.
		Node* getNearestNode(const BattleUnit* const unit) const;
		/// Gets if a BattleUnit can use a particular Node.
		bool isNodeType(
				const Node* const node,
				const BattleUnit* const unit) const;

		/// Carries out full-turn preparations for Tiles.
		void tileVolatiles();

		/// Revives unconscious units of @a faction.
//		void reviveUnits(const UnitFaction faction);
		/// Revives unconscious units.
		void reviveUnit(
				BattleUnit* const unit,
				bool turnOver = false);
		/// Sends the body-item that corresponds to a BattleUnit to the deleted vector.
		void deleteBody(const BattleUnit* const unit);

		/// Sets or tries to set a unit of a certain size on a certain position of the map.
		bool setUnitPosition(
				BattleUnit* const unit,
				const Position& pos,
				bool test = false) const;
		/// Attempts to place a unit on or near a specified Position.
		bool placeUnitNearPosition(
				BattleUnit* const unit,
				const Position& pos,
				bool isLargeUnit) const;

		/// Adds this unit to the list of falling BattleUnits.
		bool addFallingUnit(BattleUnit* const unit);
		/// Gets the list of falling BattleUnits.
		std::list<BattleUnit*>* getFallingUnits();
		/// Toggles the flag that says "there are units falling - start the fall state".
		void setUnitsFalling(bool fall);
		/// Checks the status of the flag that says "there are units falling".
		bool getUnitsFalling() const;

		/// Gets the highest ranked, living unit of faction.
		const BattleUnit* getHighestRanked(
				int& qtyAllies,
				bool isXcom = true) const;
		/// Gets the morale modifier based on the highest ranked, living xcom/alien unit, or for a unit passed into this function.
		int getMoraleModifier(
				const BattleUnit* const unit = nullptr,
				bool isXcom = true) const;

		/// Resets the turn-counter.
		void resetTurnCounter();
		/// Resets the visibility of all tiles on the battlefield.
		void blackTiles();

		/// Gets an 11x11 grid of Positions (-10 to +10 x/y).
		const std::vector<Position>& getTileSearch() const;

		/// Checks if the AI has engaged cheat-mode.
		bool isCheating() const;

		/// Gets a reference to xCom's base-modules.
		std::vector<std::vector<std::pair<int, int>>>& baseDestruct();
		/// Calculates the number of xCom's base-modules remaining.
		void calcBaseDestruct();

		/// Gets the list of items guaranteed to be recovered.
		std::vector<BattleItem*>* guaranteedItems();
		/// Gets the list of items that MIGHT get recovered.
		std::vector<BattleItem*>* conditionalItems();

		/// Sets the player's inventory-tile when BattlescapeGenerator runs.
		void setBattleInventory(Tile* const equiptTile);
		/// Gets the player's inventory-tile for preBattle InventoryState Ok-click.
		Tile* getBattleInventory() const;

		/// Sets the alien-race for the SavedBattleGame.
		void setAlienRace(const std::string& alienRace);
		/// Gets the alien-race participating in the SavedBattleGame.
		const std::string& getAlienRace() const;

		/// Sets ground-level.
		void setGroundLevel(const int level);
		/// Gets ground-level.
		int getGroundLevel() const;

		/// Gets the operation-title of the SavedBattleGame.
		const std::wstring& getOperation() const;

		/// Tells player that an aLienBase control has been destroyed.
//		void setControlDestroyed();
		/// Gets if an aLienBase control has been destroyed.
		bool getControlDestroyed() const;

		/// Get the name of the music-track to play.
//		std::string& getMusic();
		/// Set the name of the music-track to play.
		void setMusic(std::string& track);
		/// Sets variables for what music-track to play in a specified terrain.
		void calibrateMusic(
				std::string& music,
				std::string& terrain) const;

		/// Sets the aLiens as having been pacified.
		void setPacified(bool pacified = true);
		/// Gets whether the aLiens have been pacified yet.
		bool getPacified() const;

		/// Stores the camera-position where the last RF-trigger happened.
		void cacheRfTriggerPosition(const Position& pos);
		/// Gets the camera-position where the last RF-trigger happened.
		const Position& getRfTriggerPosition() const;

		/// Gets a ref to the scanner-dots vector.
		std::vector<std::pair<int,int>>& scannerDots();
		/// Gets a read-only ref to the scanner-dots vector.
		const std::vector<std::pair<int,int>>& scannerDots() const;

		/// Gets the minimum TU that a unit has at start of its turn.
		int getInitTu() const;

		/// Sets the previous walking unit.
		void setWalkUnit(const BattleUnit* const unit);
		/// Gets the previous walking unit.
		const BattleUnit* getWalkUnit() const;

		/// Sets the turn-limit for tactical.
		void setTurnLimit(int limit);
		/// Gets the turn-limit for tactical.
		int getTurnLimit() const;
		/// Sets the result when the turn-timer runs out.
		void setChronoResult(ChronoResult result);
		/// Gets the result when the turn-timer runs out.
		ChronoResult getChronoResult() const;

		/// Sets the turn for the AI to start the cheating aLiens.
		void setCheatTurn(int turn);

		/// Access to the set of detonation-tiles.
//		std::set<Tile*>& detonationTiles();

		/// Checks if tactical has yet to start.
//		bool preBattle() const;

		/// Gets the reserved fire mode.
//		BattleActionType getBatReserved() const;
		/// Sets the reserved fire mode.
//		void setBatReserved(BattleActionType reserved);
		/// Gets whether we are reserving TUs to kneel.
//		bool getKneelReserved() const;
		/// Sets whether we are reserving TUs to kneel.
//		void setKneelReserved(bool reserved);

		/// Checks whether a particular faction has eyes on *unit (whether any unit on that faction sees *unit).
//		bool eyesOnTarget(UnitFaction faction, BattleUnit* unit);
};

}

#endif
