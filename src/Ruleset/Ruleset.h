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

#ifndef OPENXCOM_RULESET_H
#define OPENXCOM_RULESET_H

#ifndef _USE_MATH_DEFINES
#	define _USE_MATH_DEFINES
#endif

#include <cmath>
#include <map>
#include <string>
#include <vector>

//#include <SDL.h>

#include <yaml-cpp/yaml.h>

#include "RuleAlienMission.h"
#include "RuleInventory.h"

#include "../Savegame/GameTime.h"


namespace OpenXcom
{

class AlienDeployment;
class AlienRace;
class ArticleDefinition;
class Base;
class ExtraSounds;
class ExtraSprites;
class ExtraStrings;
class Game;
class MapDataSet;
class MapScript;
class MCDPatch;
class OperationPool;
class ResourcePack;
class RuleAlienMission;
class RuleArmor;
class RuleAward;
class RuleBaseFacility;
class RuleCountry;
class RuleCraft;
class RuleCraftWeapon;
class RuleGlobe;
class RuleInterface;
class RuleInventory;
class RuleItem;
class RuleManufacture;
class RuleMissionScript;
class RuleMusic;
class RuleRegion;
class RuleResearch;
class RuleSoldier;
class RuleTerrain;
class RuleUfo;
class RuleUnit;
class RuleVideo;
class SavedGame;
class Soldier;
class SoldierNamePool;
//class SoundDefinition;
//class StatString;
class UfoTrajectory;


/**
 * Set of rules and stats for a game.
 * @note A ruleset holds all the constant info that never changes throughout a
 * game, like stats of all the in-game items, countries, research tree, soldier
 * names, starting base, etc.
 *
 * HOWEVER, RuleUnit can have its UnitStats substruct modified.
 */
class Ruleset
{

protected:
	int
//		_initialFunding,
		_firstGrenade,
		_retalCoef,

		_costEngineer,
		_costScientist,
		_timePersonnel,
		_radarCutoff,

		_modIndex,

		_craftListOrder,
		_facilityListOrder,
		_invListOrder,
		_itemListOrder,
		_manufactureListOrder,
		_researchListOrder,
		_ufopaediaListOrder;

	GameTime _startingTime;
	YAML::Node _startingBase;

	const Game* const _game;
	RuleGlobe* _globe;

	std::string
		_finalResearch,
		_font;

	std::vector<std::string>
		_alienMissionsIndex,
		_aliensIndex,
		_armorsIndex,
		_countriesIndex,
		_craftsIndex,
		_craftWeaponsIndex,
		_deploymentsIndex,
		_extraSoundsIndex,
		_extraSpritesIndex,
		_extraStringsIndex,
		_facilitiesIndex,
		_invsIndex,
		_itemsIndex,
		_manufactureIndex,
		_MCDPatchesIndex,
		_missionScriptIndex,
		_musicIndex,
		_regionsIndex,
		_researchIndex,
		_soldiersIndex,
		_terrainIndex,
		_ufopaediaIndex,
		_ufosIndex;
//		_psiRequirements; // it's a cache for psiStrengthEval ...

	std::vector<std::vector<int>> _alienItemLevels;

	std::vector<OperationPool*> _operationTitles;
	std::vector<SoldierNamePool*> _names;
//	std::vector<StatString*> _statStrings;

	std::map<std::string, AlienDeployment*>		_alienDeployments;
	std::map<std::string, AlienRace*>			_alienRaces;
	std::map<std::string, RuleArmor*>			_armors;
	std::map<std::string, ArticleDefinition*>	_ufopaediaArticles;
	std::map<std::string, ExtraStrings*>		_extraStrings;
	std::map<std::string, MapDataSet*>			_mapDataSets;
	std::map<std::string, MCDPatch*>			_MCDPatches;
	std::map<std::string, RuleAlienMission*>	_alienMissions;
	std::map<std::string, RuleAward*>			_awards;
	std::map<std::string, RuleBaseFacility*>	_facilities;
	std::map<std::string, RuleCountry*>			_countries;
	std::map<std::string, RuleCraft*>			_crafts;
	std::map<std::string, RuleCraftWeapon*>		_craftWeapons;
	std::map<std::string, RuleInterface*>		_interfaces;
	std::map<std::string, RuleInventory*>		_inventories;
	std::map<std::string, RuleItem*>			_items;
	std::map<std::string, RuleManufacture*>		_manufacture;
	std::map<std::string, RuleMissionScript*>	_missionScripts;
	std::map<std::string, RuleRegion*>			_regions;
	std::map<std::string, RuleResearch*>		_research;
	std::map<std::string, RuleSoldier*>			_soldiers;
	std::map<std::string, RuleTerrain*>			_terrains;
	std::map<std::string, RuleUfo*>				_ufos;
	std::map<std::string, RuleVideo*>			_videos;
//	std::map<std::string, SoundDefinition*>		_soundDefs; // TFTD.
	std::map<std::string, UfoTrajectory*>		_ufoTrajectories;
	std::map<std::string, RuleUnit*>			_units;

	std::map<std::string, std::vector<MapScript*>>	_mapScripts;

	std::vector<std::pair<std::string, ExtraSounds*>>	_extraSounds;
	std::vector<std::pair<std::string, ExtraSprites*>>	_extraSprites;
	std::vector<std::pair<std::string, RuleMusic*>>		_music;

	std::pair<std::string, int> _alienFuel;

	std::map<InventorySection, RuleInventory*> _inventories_ST;

	/// Loads all ruleset-files from a directory.
	void loadFiles(const std::string& dir);
	/// Loads a ruleset-file that's in YAML.
	void loadFile(const std::string& file);

	/// Loads a ruleset-element.
	template<typename T>
	T* loadRule(
			const YAML::Node& node,
			std::map<std::string, T*>* types,
			std::vector<std::string>* index = nullptr,
			const std::string& keyId = "type");


	public:
		/// Creates a blank Ruleset.
		explicit Ruleset(const Game* const game);
		/// Cleans up the Ruleset.
		~Ruleset();

		/// Reloads the country lines.
		void reloadCountryLines() const;

		/// Checks to ensure MissionScripts are okay.
		void validateMissions() const;

		/// Loads the Ruleset from a given source.
		void load(const std::string& src);

		/// Generates a starting SavedGame.
		SavedGame* newSave() const;

		/// Gets the pool list for Soldier names.
		const std::vector<SoldierNamePool*>& getPools() const;
		/// Gets the pool list for Operation titles.
		const std::vector<OperationPool*>& getOperations() const;

		/// Gets the rules for a Country type.
		RuleCountry* getCountry(const std::string& id) const;
		/// Gets the available Countries.
		const std::vector<std::string>& getCountriesList() const;
		/// Gets the rules for a Region type.
		RuleRegion* getRegion(const std::string& id) const;
		/// Gets the available Regions.
		const std::vector<std::string>& getRegionsList() const;

		/// Gets the rules for a Facility type.
		RuleBaseFacility* getBaseFacility(const std::string& id) const;
		/// Gets the available Facilities.
		const std::vector<std::string>& getBaseFacilitiesList() const;

		/// Gets the rules for a Craft type.
		RuleCraft* getCraft(const std::string& id) const;
		/// Gets the available Crafts.
		const std::vector<std::string>& getCraftsList() const;
		/// Gets the rules for a CraftWeapon type.
		RuleCraftWeapon* getCraftWeapon(const std::string& id) const;
		/// Gets the available CraftWeapons.
		const std::vector<std::string>& getCraftWeaponsList() const;

		/// Gets the rules for an Item type.
		RuleItem* getItemRule(const std::string& id) const;
		/// Gets the available Items.
		const std::vector<std::string>& getItemsList() const;

		/// Gets the rules for a UFO type.
		RuleUfo* getUfo(const std::string& id) const;
		/// Gets the available UFOs.
		const std::vector<std::string>& getUfosList() const;

		/// Gets rules for Terrain types.
		RuleTerrain* getTerrain(const std::string& type) const;
		/// Gets the available Terrains.
		const std::vector<std::string>& getTerrainList() const;

		/// Gets the MapDataSet for the battlefield.
		MapDataSet* getMapDataSet(const std::string& name);

		/// Gets the rules for Awards.
		std::map<std::string, RuleAward*> getAwardsList() const;

		/// Gets the rules for a Soldier type.
		RuleSoldier* getSoldier(const std::string& type) const;
		/// Returns a list of all soldier-types provided by the Ruleset.
		const std::vector<std::string>& getSoldiersList() const;
		/// Gets non-Soldier-unit rules.
		RuleUnit* getUnitRule(const std::string& type) const;

		/// Gets the rules for AlienRace types.
		AlienRace* getAlienRace(const std::string& type) const;
		/// Gets the available AlienRaces.
		const std::vector<std::string>& getAlienRacesList() const;
		/// Gets the rules for AlienDeployment types.
		AlienDeployment* getDeployment(const std::string& name) const;
		/// Gets the available AlienDeployments.
		const std::vector<std::string>& getDeploymentsList() const;

		/// Gets the rules for Armor types.
		RuleArmor* getArmor(const std::string& name) const;
		/// Gets the available Armors.
		const std::vector<std::string>& getArmorsList() const;

		/// Gets the cost of an engineer.
		int getEngineerCost() const;
		/// Gets the cost of a scientist.
		int getScientistCost() const;
		/// Gets the transfer time of personnel.
		int getPersonnelTime() const;

		/// Gets a Ufopaedia ArticleDefinition.
		ArticleDefinition* getUfopaediaArticle(const std::string& article_id) const;
		/// Gets the available Articles.
		const std::vector<std::string>& getUfopaediaList() const;

		/// Gets a map of the Inventories.
		std::map<std::string, RuleInventory*>* getInventories();
		/// Gets the rules for a specific Inventory from a type.
		const RuleInventory* getInventory(const std::string& type) const;
		/// Gets the rules for a specific Inventory from a section-ID.
		const RuleInventory* getInventoryRule(const InventorySection sectionId) const;
		/// Converts all inventory-mappings from string-keys to enumerated-keys.
		void convertInventories();
		/// Returns the sorted list of inventories.
//		const std::vector<std::string>& getInventoryList() const;
		/// Determines the highest TU-value used in Inventory rules.
		int detHighTuInventoryCost() const;

		/// Gets the rules for a specific research-project.
		const RuleResearch* getResearch(const std::string& id) const;
		/// Gets the list of all research-projects.
		const std::vector<std::string>& getResearchList() const;
		/// Gets the rules for a specific manufacture-project.
		RuleManufacture* getManufacture(const std::string& id) const;
		/// Gets the list of all manufacture-projects.
		const std::vector<std::string>& getManufactureList() const;

		/// Gets Facilities for custom-bases.
		std::vector<OpenXcom::RuleBaseFacility*> getCustomBaseFacilities() const;

		/// Gets a specific UfoTrajectory.
		const UfoTrajectory* getUfoTrajectory(const std::string& id) const;

		/// Gets the rules for a specific AlienMission.
		const RuleAlienMission* getAlienMission(const std::string& id) const;
		/// Gets the rules for a random AlienMission.
		const RuleAlienMission* getRandomMission(
				MissionObjective objective,
				size_t monthsPassed) const;
		/// Gets a list of all AlienMissions.
		const std::vector<std::string>& getAlienMissionList() const;

		/// Gets the AlienItemLevel table.
		const std::vector<std::vector<int>>& getAlienItemLevels() const;

		/// Gets the default start-base.
		const YAML::Node& getStartingBase() const;
		/// Gets the default start-time.
		const GameTime& getStartingTime() const;

		/// Gets an MCDPatch.
		MCDPatch* getMCDPatch(const std::string& name) const;

		/// Gets the music rules.
		std::vector<std::pair<std::string, RuleMusic*>> getMusicTracks() const;

		/// Gets a list of external Sprites.
		std::vector<std::pair<std::string, ExtraSprites*>> getExtraSprites() const;
		/// Gets a list of external Sounds.
		std::vector<std::pair<std::string, ExtraSounds*>> getExtraSounds() const;
		/// Gets a list of external Strings.
		std::map<std::string, ExtraStrings*> getExtraStrings() const;

		/// Gets the list of StatStrings.
//		std::vector<StatString*> getStatStrings() const;

		/// Sorts all the lists according to their weights.
		void sortLists();

		/// Gets the research-requirements for Psi-Lab (it's a cache for psiStrengthEval)
//		std::vector<std::string> getPsiRequirements() const;

		/// Generates a Soldier.
		Soldier* genSoldier(
				SavedGame* const save,
				std::string type = "") const;

		/// Gets the item-type to be used as fuel for UFOs.
		const std::string& getAlienFuelType() const;
		/// Gets the quantity of alien-fuel to recover per fuel-item found.
		int getAlienFuelQuantity() const;

		/// Gets the Font name.
		std::string getFontName() const;

		/// Gets the minimum radar-range.
//		int getMinRadarRange() const;
		/// Gets maximum radar-range of all Facilities.
		int getMaxRadarRange() const;
		/// Gets the cutoff between small & large radars.
		int getRadarCutoffRange() const;

		/// Gets the turn that aLiens are allowed to throw their first grenades.
		int getFirstGrenade() const;

		/// Gets the basic retaliation chance.
		int getRetaliation() const;

		/// Gets the rules for an Interface.
		RuleInterface* getInterface(const std::string& id) const;

		/// Gets the rules for the Globe.
		RuleGlobe* getGlobe() const;

		/// Gets the list of selective files for insertion into internal Cat files.
//		const std::map<std::string, SoundDefinition*>* getSoundDefinitions() const;

		/// Gets a list of videos for intro/outro etc.
		const std::map<std::string, RuleVideo*>* getVideos() const;

		/// Gets a list of MissionScripts.
		const std::vector<std::string>* getMissionScriptList() const;
		/// Gets a specific MissionScript.
		RuleMissionScript* getMissionScript(const std::string& type) const;

		/// Gets a list of MapScripts.
		const std::vector<MapScript*>* getMapScript(const std::string& type) const;

		/// Gets the final-research-type.
		const std::string& getFinalResearch() const;

		/// Gets a pointer to the current Game.
		const Game* getGame() const;
};

}

#endif
