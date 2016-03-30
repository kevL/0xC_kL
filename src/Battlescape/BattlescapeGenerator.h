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

#ifndef OPENXCOM_BATTLESCAPEGENERATOR_H
#define OPENXCOM_BATTLESCAPEGENERATOR_H

//#include <vector>

#include "../Ruleset/MapScript.h"


namespace OpenXcom
{

class AlienBase;
class AlienDeployment;
class AlienRace;
class Base;
class BattleItem;
class BattleUnit;
class Craft;
class Game;
class MissionSite;
class ResourcePack;
class RuleItem;
class Ruleset;
class RuleUnit;
class SavedBattleGame;
class SavedGame;
class Tile;
class Ufo;
class Vehicle;


/**
 * A utility class that generates the initial battlescape data.
 * @note Taking into account mission type, craft and ufo involved, terrain type
 * ... etc.
 */
class BattlescapeGenerator
{

private:
	bool
		_baseEquiptMode,
		_craftDeployed,
		_generateFuel;
//		_error;
	int
		_alienItemLevel,
		_blocksLeft,
		_craftZ,
		_mapsize_x,
		_mapsize_y,
		_mapsize_z,
		_shade,
		_unitSequence;
	size_t _battleOrder;

	SDL_Rect _craftPos;
	std::vector<SDL_Rect> _ufoPos;

	std::vector<std::vector<bool>> _landingzone;
	std::vector<std::vector<int>>
		_drillMap,
		_segments;
	std::vector<std::vector<MapBlock*>> _blocks;

	AlienBase* _alienBase;
	Base* _base;
	Craft* _craft;
	Game* _game;
	MapBlock* _testBlock;
	MissionSite* _site;
	ResourcePack* _res;
	Ruleset* _rules;
	RuleTerrain* _terrainRule;
	SavedBattleGame* _battleSave;
	SavedGame* _gameSave;
	Tile* _tileEquipt;
	Ufo* _ufo;

	std::string _alienRace;


	/// Sets the map-size and associated vars.
	void init();

	/// Deploys XCOM units for the mission.
	void deployXcom();
	/// Constructs a vector of Vehicles that can participate in a Base Defense tactical.
	void prepareBaseVehicles(std::vector<Vehicle*>& vehicles);
	/// Prepares a player-support-unit to be added to the Battlescape.
	BattleUnit* convertVehicle(Vehicle* const vehicle);
	/// Adds a player-unit to the Battlescape.
	BattleUnit* addPlayerUnit(BattleUnit* const unit);
	/// Runs necessary checks before setting a unit's position.
	bool canPlacePlayerUnit(Tile* const tile);
	/// Loads a weapon that's lying on the XCOM-equipment Tile.
	void loadGroundWeapon(BattleItem* const item);
	/// Places a BattleItem on a Soldier based on that Soldier's equipment-layout.
	void placeLayout(BattleItem* const item);
	/// Sets XCOM Soldiers' combat-clothing style - ie. spritesheets & paperdolls.
	void setTacticalSprites() const;

	/// Places a BattleItem on a BattleUnit and adds it to the Battlescape.
	bool placeGeneric(
			BattleItem* const item,
			BattleUnit* const unit) const;

	/// Deploys the aLiens according to an AlienDeployment rule.
	void deployAliens(const AlienDeployment* const ruleDeploy);
	/// Adds an aLien to the battlescape.
	BattleUnit* addAlien(
			RuleUnit* const unitRule,
			int aLienRank,
			bool outside);
	/// Finds a spot near a friend to spawn at.
	bool placeUnitNearFaction(BattleUnit* const unit);

	/// Spawns civilians for a terror-mission.
	void deployCivilians(int civilians);
	/// Adds a civilian to the Battlescape.
	void addCivilian(RuleUnit* const unitRule);

	/// Loads an XCom MAP file.
	int loadMAP(
			MapBlock* const block,
			int offset_x,
			int offset_y,
			const RuleTerrain* const terraRule,
			int dataSetIdOffset = 0,
			bool revealed = false,
			bool craft = false);
	/// Loads an XCom RMP file.
	void loadRMP(
			MapBlock* const block,
			int offset_x,
			int offset_y,
			int segment);

	/// Fills power sources with an alien fuel object.
	void fuelPowerSources();
	/// Possibly explodes ufo powersources.
	void explodePowerSources();

	/// Generates a new battlescape map.
	void generateMap(const std::vector<MapScript*>* const script);
	/// Generates a map from base modules.
	void generateBaseMap();

	/// Finds Alien Base start modules for Xcom equipment spawning.
	void placeXcomProperty();

	/// Clears a module from the map.
	void clearModule(
			int x,
			int y,
			int sizeX,
			int sizeY);
	/// Load the nodes from the associated map blocks.
	void loadNodes();
	/// Connects all the nodes together.
	void attachNodeLinks();
	/// Selects an unused position on the map of a given size.
	bool selectPosition(
			const std::vector<SDL_Rect*>* const rects,
			int& ret_x,
			int& ret_y,
			int size_x,
			int size_y);
	/// Adds a craft (either a ufo or an xcom craft) somewhere on the map.
	bool addCraft(
			const MapBlock* const block,
			MapScript* const script,
			SDL_Rect& rect);
	/// Adds a line (generally a road) to the map.
	bool addLine(
			MapDirection dir,
			const std::vector<SDL_Rect*>* const rects);
	/// Adds a single block at a given position.
	bool addBlock(
			int x,
			int y,
			MapBlock* const block);
	/// Drills some tunnels between map blocks.
	void drillModules(
			TunnelData* const info,
			const std::vector<SDL_Rect*>* const rects,
			MapDirection dir);
	/// Clears all modules in a rect from a command.
	bool removeBlocks(const MapScript* const directive);


	public:
		/// Creates a BattlescapeGenerator class
		explicit BattlescapeGenerator(Game* const game);
		/// Cleans up the BattlescapeGenerator.
		~BattlescapeGenerator();

		/// Sets the XCom craft.
		void setCraft(Craft* const craft);
		/// Sets the ufo.
		void setUfo(Ufo* const ufo);
		/// Sets the XCom base.
		void setBase(Base* const base);
		/// Sets the mission-site.
		void setMissionSite(MissionSite* const site);
		/// Sets the alien base
		void setAlienBase(AlienBase* const base);
		/// Sets the terrain.
		void setTerrain(RuleTerrain* const terrain);
		/// Sets the polygon shade.
		void setShade(int shade);
		/// Sets the alien race.
		void setAlienRace(const std::string& alienRace);
		/// Sets the alien item level.
		void setAlienItemlevel(int alienItemLevel);

		/// Runs the generator.
		void run();
		/// Sets up the next stage (for Cydonia/TFTD missions).
		void nextStage();

		/// Generates a fake battlescape for Craft & Base soldier-inventory.
		void runInventory(
				Craft* const craft,
				Base* const base = nullptr,
				size_t selUnitId = 0);

		/// Sets up the objectives for the map.
		void setupObjectives(const AlienDeployment* const ruleDeploy);
};

}

#endif
