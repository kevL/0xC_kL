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
class Base;
class BattleItem;
class BattleUnit;
class Craft;
class Game;
class TerrorSite;
class ResourcePack;
class RuleAlienDeployment;
class RuleAlienRace;
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
		_isFakeInventory,
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
		_seg;
	std::vector<std::vector<MapBlock*>> _blocks;

	std::vector<BattleItem*>* _itemList;
	std::vector<BattleUnit*>* _unitList;

	AlienBase* _alienBase;
	Base* _base;
	Craft* _craft;
	Game* _game;
	MapBlock* _testBlock;
	TerrorSite* _terrorSite;
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

	/// Sets the latency-status of a specified BattleUnit.
	void setUnitLatency(BattleUnit* const unit);

	/// Deploys XCOM units and equipment for tactical.
	void deployXcom();
	/// Constructs a vector of Vehicles that can participate in BaseDefense tacticals.
	void prepBaseVehicles(std::vector<Vehicle*>& vehicles);
	/// Prepares a support-unit to be added to the Battlescape.
	BattleUnit* convertVehicle(Vehicle* const vehicle);
	/// Adds a unit to the Battlescape.
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

	/// Deploys the aLiens according to an RuleAlienDeployment rule.
	void deployAliens(const RuleAlienDeployment* const ruleDeploy);
	/// Adds an aLien to the Battlescape.
	BattleUnit* addAlien(
			RuleUnit* const unitRule,
			int aLienRank,
			bool outside);
	/// Finds a tile near an ally to spawn at.
	bool placeUnitNearFaction(BattleUnit* const unit);

	/// Spawns civilians for a terror-mission.
	void deployCivilians(int civilians);
	/// Adds a civilian to the Battlescape.
	void addCivilian(RuleUnit* const unitRule);

	/// Loads a MAP file.
	int loadMAP(
			MapBlock* const block,
			int offset_x,
			int offset_y,
			const RuleTerrain* const terraRule,
			int dataSetIdOffset = 0,
			bool revealed = false,
			bool craft = false);
	/// Loads an RMP file.
	void loadRMP(
			MapBlock* const block,
			int offset_x,
			int offset_y,
			int segment);

	/// Fills power-sources with aLien-fuel objects.
	void fuelPowerSources();
	/// Checks and explodes UFO power-sources.
	void explodePowerSources();

	/// Generates the tactical Map of a battlefield.
	void generateMap(const std::vector<MapScript*>* const directives);
	/// Generates the tactical Map of a battlefield based on the player's Base.
	void generateBaseMap();

	/// Uses aLienBase start-modules for spawning xCom equipment.
	void placeXcomProperty();

	/// Clears a module from the Map.
	void clearModule(
			int x,
			int y,
			int sizeX,
			int sizeY);
	/// Loads the Nodes from associated MapBlocks.
	void loadNodes();
	/// Connects all the Nodes together.
	void attachNodeLinks();
	/// Selects an unused rectangle of a specified size on the Map.
	bool selectPosition(
			const std::vector<SDL_Rect*>* const rects,
			int& ret_x,
			int& ret_y,
			int size_x,
			int size_y);
	/// Adds a Craft (either a UFO or an XCOM Craft) somewhere on the Map.
	bool addCraft(
			const MapBlock* const block,
			MapScript* const script,
			SDL_Rect& rect);
	/// Adds a line (generally a road) to the Map.
	bool addLine(
			MapDirection dir,
			const std::vector<SDL_Rect*>* const rects);
	/// Adds a single MapBlock at a specified position.
	bool addBlock(
			int x,
			int y,
			MapBlock* const block);
	/// Drills tunnels between MapBlocks.
	void drillModules(
			TunnelData* const info,
			const std::vector<SDL_Rect*>* const rects,
			MapDirection dir);
	/// Clears all modules in a rect according to a specified directive.
	bool clearBlocks(const MapScript* const directive);

	/// Sets up the player's objectives for the upcoming battle.
	void setupObjectives(const RuleAlienDeployment* const ruleDeploy);


	public:
		/// Creates a BattlescapeGenerator class.
		explicit BattlescapeGenerator(Game* const game);
		/// Cleans up the BattlescapeGenerator.
		~BattlescapeGenerator();

		/// Sets the xCom Craft.
		void setCraft(Craft* const craft);
		/// Sets the UFO.
		void setUfo(Ufo* const ufo);
		/// Sets the xCom Base.
		void setBase(Base* const base);
		/// Sets the aLien TerrorSite.
		void setTerrorSite(TerrorSite* const terrorSite);
		/// Sets the AlienBase.
		void setAlienBase(AlienBase* const base);
		/// Sets the terrain-rule.
		void setTerrain(RuleTerrain* const terrain);
		/// Sets the tactical shade.
		void setShade(int shade);
		/// Sets the alien-race.
		void setAlienRace(const std::string& alienRace);
		/// Sets the alien-item-level.
		void setAlienItemlevel(int alienItemLevel);

		/// Runs the Generator.
		void run();
		/// Sets up the next stage (for Cydonia/2-stage missions).
		void nextStage();

		/// Generates a fake battlescape for Craft & Base soldier-inventory.
		void runInventory(
				Craft* const craft,
				Base* const base = nullptr,
				size_t selUnitId = 0u);
};

}

#endif
