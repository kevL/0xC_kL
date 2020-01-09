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

namespace ItemPlacedType
{
	enum kalafradjalistic
	{
		FAILED,			// 0
		SUCCESS,		// 1
		SUCCESS_LOAD	// 2
	};
}

class BattleItem;
class BattleUnit;
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
		_bonk;
	int
		_cheatTurn,
		_groundLevel,
		_dropTu,
		_itemId,
		_mapsize_x,
		_mapsize_y,
		_mapsize_z,
		_objectivesDestroyed,
		_objectivesRequired,
		_tacticalShade,
		_turn,
		_turnLimit;
	size_t _qtyTilesTotal;

//	BattleActionType _batReserved;
	ChronoResult _chronoResult;
	Position _rfTriggerOffset;
	TilepartSpecial _objectiveType;
	TacticalType _tacType;
	UnitFaction _side;

	BattlescapeState* _battleState;
	BattleUnit
		* _selectedUnit,
		* _recallUnit;
	Pathfinding* _pf;
	SavedGame* _playSave;
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

	std::list<BattleUnit*> _bonkers;

//	std::set<Tile*> _detonationTiles;

	std::vector<BattleItem*>
		_deletedProperty,
		_items,
		_recoverConditional,
		_recoverGuaranteed;
	std::vector<BattleUnit*>
		_units,
		_shuffleUnits;
	std::vector<MapDataSet*> _battleDataSets;
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

	/// Selects a BattleUnit.
	BattleUnit* selectUnit(
			int dir,
			bool dontReselect,
			bool checkReselect,
			bool checkInventory);

	/// Sets the TacticalType based on a specified mission-type.
	void setTacType(const std::string& type);

	/// Carries out full-turn preparations for Tiles.
	void tileVolatiles();


	public:
		static const size_t SEARCH_SIZE = (SEARCH_DIST * SEARCH_DIST);

		/// Creates a SavedBattleGame based on the current SavedGame.
		SavedBattleGame(
				SavedGame* const playSave,
				const std::vector<OperationPool*>* const titles = nullptr,
				const Ruleset* const rules = nullptr);
		/// Cleans up the SavedBattleGame.
		~SavedBattleGame();

		/// Loads the SavedBattleGame from YAML.
		void load(
				const YAML::Node& node,
				Ruleset* const rules);
		/// Load map resources.
		void loadMapResources(const Game* const game);
		/// Saves the SavedBattleGame to YAML.
		YAML::Node save() const;

		/// Gets a pointer to the tiles.
		Tile** getTiles() const;

		/// Sets the dimensions of the map and initializes Tiles.
		void initMap(
				const int mapsize_x,
				const int mapsize_y,
				const int mapsize_z);
		/// Initializes the Pathfinding and the TileEngine.
		void initUtilities(const ResourcePack* const res);

		/// Sets the tactical-type.
		void setTacticalType(const std::string& type);
		/// Gets the tactical-type.
		std::string getTacticalType() const;
		/// Gets the TacticalType of this battle.
		TacticalType getTacType() const;

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
		BattleUnit* selectNextUnit(
				bool dontReselect = false,
				bool checkReselect = false,
				bool checkInventory = false);
		/// Selects the previous BattleUnit.
		BattleUnit* selectPrevUnit(
				bool dontReselect = false,
				bool checkReselect = false,
				bool checkInventory = false);

		/// Gets the playing side.
		UnitFaction getSide() const;
		/// Gets the turn.
		int getTurn() const;
		/// Advances play to the next faction.
		bool factionEndTurn();
		/// Selects the first BattleUnit of faction.
		BattleUnit* firstFactionUnit(UnitFaction faction);

		/// Sets debug mode.
		void debugTac();
		/// Gets debug mode.
		bool getDebugTac() const;

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

		/// Gets the battle's MCD sets.
		std::vector<MapDataSet*>* getBattleDataSets();

		/// Gets a pointer to the SavedGame.
		SavedGame* getSavedGame() const;

		/// Gets a pointer to the BattlescapeGame.
		BattlescapeGame* getBattleGame() const;
		/// Gets a pointer to the BattlescapeState.
		BattlescapeState* getBattleState() const;
		/// Sets the pointer to the BattlescapeState.
		void setBattleState(BattlescapeState* const battleState);

		/// Places all BattleUnits at their start-Tile(s) at the end of pre-battle Equip.
		void positionUnits();

		/// Gives access to the storage-tiles vector.
		std::vector<Position>& storagePositions();
		/// Moves all the leftover items to random locations in the storage-tiles vector.
		void distributeEquipt(Tile* const tile);

		/// Removes a BattleItem from the battlefield.
		std::vector<BattleItem*>::const_iterator sendItemToDelete(BattleItem* const it);
		/// Gives read-only access to the deleted-property vector.
		const std::vector<BattleItem*>& deletedProperty() const;

		/// Sets if the mission was aborted.
		void isAborted(bool abort);
		/// Gets if the mission was aborted.
		bool isAborted() const;

		/// Sets the objective-tiletype for this mission.
		void setObjectiveTilepartType(TilepartSpecial specialType);
		/// Gets the objective-tiletype of this mission.
		TilepartSpecial getObjectiveTilepartType() const;
		/// Sets how many objective-tiles need to be destroyed.
		void initObjectives(int qty);
		/// Increments the objective-tiles destroyed counter.
		void addDestroyedObjective();
		/// Checks if enough objective-tiles are destroyed.
		bool allObjectivesDestroyed() const;

		/// Sets the highest available item-ID value.
		void setCanonicalBattleId();
		/// Gets the highest available item-ID value.
		int* getCanonicalBattleId();

		/// Gets a spawn-node.
		Node* getSpawnNode(
				int unitRank,
				BattleUnit* const unit);
		/// Gets a patrol-node.
		Node* getPatrolNode(
				bool scout,
				BattleUnit* const unit,
				Node* start);
		/// Gets the Node considered nearest to a BattleUnit.
		Node* getStartNode(const BattleUnit* const unit) const;
		/// Gets if a BattleUnit can use a particular Node.
		bool isNodeType(
				const Node* const node,
				const BattleUnit* const unit) const;

		/// Revives unconscious units of @a faction.
//		void reviveUnits(const UnitFaction faction);
		/// Revives unconscious units.
		void checkUnitRevival(BattleUnit* const unit);
		/// Sends the body-item that corresponds to a BattleUnit to the deleted vector.
		void deleteBody(const BattleUnit* const unit);

		/// Sets or tries to set a BattleUnit of a certain size on a certain Position of the battlefield.
		bool setUnitPosition(
				BattleUnit* const unit,
				const Position& pos,
				bool test = false) const;
		/// Attempts to place a unit on or near a specified Position.
		bool placeUnitByPosition(
				BattleUnit* const unit,
				const Position& pos,
				bool large) const;

		/// Adds this unit to the list of bonking BattleUnits.
		bool addBonker(BattleUnit* const unit);
		/// Gets the list of bonking BattleUnits.
		std::list<BattleUnit*>* getBonkers();
		/// Accesses the '_bonk' bool.
		bool& doBonks();

		/// Gets the highest ranked, living unit of faction.
		const BattleUnit* getHighestRanked(
				int& allies,
				bool xcom = true) const;
		/// Gets the morale modifier based on the highest ranked, living xcom/alien unit, or for a unit passed into this function.
		int getMoraleModifier(
				const BattleUnit* const unit = nullptr,
				bool xcom = true) const;

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
		std::vector<BattleItem*>* recoverGuaranteed();
		/// Gets the list of items that MIGHT get recovered.
		std::vector<BattleItem*>* recoverConditional();

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

		/// Gets the music-track to play.
//		std::string& getMusic();
		/// Sets the music-track to play.
		void setMusic(std::string& track);
		/// Sets variables for what music-track to play in a specified terrain.
		void calibrateMusic(
				std::string& track,
				std::string& terrain) const;

		/// Sets the aLiens as having been pacified.
		void setPacified(bool pacified = true);
		/// Gets whether the aLiens have been pacified yet.
		bool getPacified() const;

		/// Sets the camera-offset of when the last RF-trigger happened.
		void rfTriggerOffset(const Position& offset);
		/// Gets the camera-offset of when the last RF-trigger happened.
		const Position& rfTriggerOffset() const;

		/// Gets a ref to the scanner-dots vector.
		std::vector<std::pair<int,int>>& scannerDots();
		/// Gets a read-only ref to the scanner-dots vector.
		const std::vector<std::pair<int,int>>& scannerDots() const;

		/// Gets the minimum TU that a unit has at the start of its turn.
		int getDropTu() const;

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
