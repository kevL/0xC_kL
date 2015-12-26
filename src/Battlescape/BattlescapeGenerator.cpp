/*
 * Copyright 2010-2015 OpenXcom Developers.
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

#include "BattlescapeGenerator.h"

#include <fstream>
//#include <sstream>
//#include <assert.h>

#include "AlienBAIState.h"
#include "CivilianBAIState.h"
#include "Inventory.h"
#include "Pathfinding.h"
#include "TileEngine.h"

#include "../Engine/CrossPlatform.h"
#include "../Engine/Exception.h"
#include "../Engine/Game.h"
//#include "../Engine/LocalizedText.h"
#include "../Engine/Logger.h"
#include "../Engine/Options.h"
#include "../Engine/RNG.h"

//#include "../Resource/XcomResourcePack.h"

#include "../Ruleset/AlienDeployment.h"
#include "../Ruleset/AlienRace.h"
#include "../Ruleset/MapBlock.h"
#include "../Ruleset/MapData.h"
#include "../Ruleset/MapDataSet.h"
#include "../Ruleset/MCDPatch.h"
#include "../Ruleset/RuleArmor.h"
#include "../Ruleset/RuleBaseFacility.h"
#include "../Ruleset/RuleCraft.h"
#include "../Ruleset/RuleGlobe.h"
#include "../Ruleset/RuleInventory.h"
#include "../Ruleset/Ruleset.h"
#include "../Ruleset/RuleTerrain.h"
#include "../Ruleset/RuleUfo.h"
#include "../Ruleset/RuleUnit.h"

#include "../Savegame/AlienBase.h"
#include "../Savegame/Base.h"
#include "../Savegame/BaseFacility.h"
#include "../Savegame/BattleItem.h"
#include "../Savegame/BattleUnit.h"
#include "../Savegame/Craft.h"
#include "../Savegame/EquipmentLayoutItem.h"
#include "../Savegame/ItemContainer.h"
#include "../Savegame/MissionSite.h"
#include "../Savegame/Node.h"
#include "../Savegame/SavedBattleGame.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/Soldier.h"
#include "../Savegame/Tile.h"
#include "../Savegame/Ufo.h"
#include "../Savegame/Vehicle.h"


namespace OpenXcom
{

/**
 * Sets up a BattlescapeGenerator.
 * @param game - pointer to Game
 */
BattlescapeGenerator::BattlescapeGenerator(Game* const game)
	:
		_game(game),
		_gameSave(game->getSavedGame()),
		_battleSave(game->getSavedGame()->getBattleSave()),
		_rules(game->getRuleset()),
		_res(game->getResourcePack()),
		_craft(nullptr),
		_ufo(nullptr),
		_base(nullptr),
		_mission(nullptr),
		_alienBase(nullptr),
		_terrainRule(nullptr),
		_shade(0),
		_alienItemLevel(0),
		_mapsize_x(0),
		_mapsize_y(0),
		_mapsize_z(0),
		_unitSequence(0),
		_tileEquipt(nullptr),
		_generateFuel(true),
		_craftDeployed(false),
		_craftZ(0),
		_baseEquipScreen(false),
		_battleOrder(0),
		_blocksLeft(0),
		_testBlock(nullptr)
{}

/**
 * Deletes the BattlescapeGenerator.
 */
BattlescapeGenerator::~BattlescapeGenerator()
{}

/**
 * Sets up the various arrays and whatnot according to the size of the battlefield.
 */
void BattlescapeGenerator::init() // private.
{
	_blocks.clear();
	_landingzone.clear();
	_segments.clear();
	_drillMap.clear();

	_blocks.resize(
				_mapsize_x / 10,
				std::vector<MapBlock*>(_mapsize_y / 10));
	_landingzone.resize(
					_mapsize_x / 10,
					std::vector<bool>(
								_mapsize_y / 10,
								false));
	_segments.resize(
				_mapsize_x / 10,
				std::vector<int>(
							_mapsize_y / 10,
							0));
	_drillMap.resize(
				_mapsize_x / 10,
				std::vector<int>(
							_mapsize_y / 10,
							MD_NONE));

	_blocksLeft = (_mapsize_x / 10) * (_mapsize_y / 10);
	//Log(LOG_INFO) << "bgen: _blocksLeft = " << _blocksLeft;

	_battleSave->initMap( // creates the tile objects
					_mapsize_x,
					_mapsize_y,
					_mapsize_z);
	_battleSave->initUtilities(_res);
}

/**
 * Sets the XCom craft involved in the battle.
 * @param craft - pointer to Craft
 */
void BattlescapeGenerator::setCraft(Craft* craft)
{
	_craft = craft;
	_craft->setInBattlescape();
}

/**
 * Sets the ufo involved in the battle.
 * @param ufo - pointer to Ufo
 */
void BattlescapeGenerator::setUfo(Ufo* ufo)
{
	_ufo = ufo;
	_ufo->setInBattlescape();
}

/**
 * Sets the XCom base involved in the battle.
 * @param base - pointer to Base
 */
void BattlescapeGenerator::setBase(Base* base)
{
	_base = base;
	_base->setInBattlescape();
}

/**
 * Sets the mission site involved in the battle.
 * @param mission - pointer to MissionSite
 */
void BattlescapeGenerator::setMissionSite(MissionSite* mission)
{
	_mission = mission;
	_mission->setInBattlescape();
}

/**
 * Sets the alien base involved in the battle.
 * @param base - pointer to AlienBase
 */
void BattlescapeGenerator::setAlienBase(AlienBase* base)
{
	_alienBase = base;
	_alienBase->setInBattlescape();
}

/**
 * Sets the terrain of where a ufo crashed/landed as per ConfirmLandingState
 * or nextStage().
 * @param texture - pointer to RuleTerrain
 */
void BattlescapeGenerator::setTerrain(RuleTerrain* terrain)
{
	_terrainRule = terrain;
}

/**
 * Sets the world shade where a ufo crashed or landed.
 * @note This is used to determine the battlescape light level.
 * @param shade - shade of the polygon on the globe
 */
void BattlescapeGenerator::setShade(int shade)
{
	_shade = std::max(0, std::min(15, shade));
}

/**
 * Sets the alien race on the mission.
 * @note This is used to determine the various alien types to spawn.
 * @param alienRace - reference the alien race family
 */
void BattlescapeGenerator::setAlienRace(const std::string& alienRace)
{
	_alienRace = alienRace;
	_battleSave->setAlienRace(alienRace);
}

/**
 * Sets the alien item level.
 * @note This is used to determine how advanced the equipment of the aliens will
 * be. It applies only to 'New Battle' games and is usually overridden by the
 * current month. This value ought be between 0 and the size of the itemLevel
 * array in the ruleset inclusive. At specified months higher itemLevels appear
 * more often and lower ones gradually disappear.
 * @param alienItemLevel - alienItemLevel
 */
void BattlescapeGenerator::setAlienItemlevel(int alienItemLevel)
{
	_alienItemLevel = alienItemLevel;
}

/**
 * Starts the generator.
 * @note This fills the SavedBattleGame with data.
 */
void BattlescapeGenerator::run()
{
	_unitSequence = BattleUnit::MAX_SOLDIER_ID; // geoscape soldier IDs should stay below this number

	AlienDeployment* deployRule;
	if (_ufo != nullptr)
		deployRule = _rules->getDeployment(_ufo->getRules()->getType());
	else
		deployRule = _rules->getDeployment(_battleSave->getTacticalType());

	deployRule->getDimensions(
						&_mapsize_x,
						&_mapsize_y,
						&_mapsize_z);
	if (_terrainRule == nullptr)	// '_terrainRule' NOT set for Cydonia, Base assault/defense. Already set for NewBattleState ...... & UFO, & missionSite.
	{
		_terrainRule = _rules->getTerrain(deployRule->getDeployTerrains().at(RNG::pick(deployRule->getDeployTerrains().size())));
		if (_terrainRule == nullptr)
		{
			// trouble: no texture and no deployment terrain, most likely scenario is a UFO landing on water: use the first available terrain.
			_terrainRule = _game->getRuleset()->getTerrain(_game->getRuleset()->getTerrainList().front());
			Log(LOG_WARNING) << "bGen::run() - Could not find a terrain rule ... using first terrainType: "
							 << _terrainRule->getType();
		}
	}
/* Theirs:
	if (_terrainRule == nullptr)
	{
		if (_texture == nullptr
			|| _texture->getTextureDetail()->empty() == true
			|| deployRule->getDeployTerrains().empty() == false)
		{
			size_t pick = RNG::generate(0, deployRule->getDeployTerrains().size() - 1);
			_terrainRule = _rules->getTerrain(deployRule->getDeployTerrains().at(pick));
		}
		else // UFO crashed/landed or MissionSite
		{
			const Target* target;
			if (_mission != nullptr) target = _mission;
			else target = _ufo;
			_terrainRule = _rules->getTerrain(_texture->getTextureTerrain(target));
		}
	} */
/*	if (deployRule->getDeployTerrains().empty() == true) // UFO crashed/landed
	{
		Log(LOG_INFO) << "bGen::run() deployment-terrains NOT valid";
		if (_siteTerrain == nullptr) // kL
		{
			Log(LOG_INFO) << ". siteTexture = " << _texture;
			double lat;
			if (_ufo != nullptr) lat = _ufo->getLatitude();
			else lat = 0.;
			_terrainRule = getTerrain(_texture, lat);
		}
		else
		{
			Log(LOG_INFO) << ". ufo mission siteTerrain = " << _siteTerrain->getName();
			_terrainRule = _siteTerrain; // kL
		}
	}
	else if (_siteTerrain != nullptr // kL ->
		&& _missionType == "STR_TERROR_MISSION")
	{
		Log(LOG_INFO) << ". terror mission siteTerrain = " << _siteTerrain->getName();
		_terrainRule = _siteTerrain; // kL_end.
	}
	else // set-piece battle like Cydonia or Terror site or Base assault/defense
	{
		Log(LOG_INFO) << "bGen::run() Choose terrain from deployment, qty = " << deployRule->getDeployTerrains().size();
		const size_t pick = RNG::generate(0, deployRule->getDeployTerrains().size() - 1);
		_terrainRule = _rules->getTerrain(deployRule->getDeployTerrains().at(pick));
	} */

	const std::vector<MapScript*>* script = nullptr; // alienDeployment script overrides terrain script <-
	const std::string scriptDeploy = deployRule->getScript();
	//Log(LOG_INFO) << "bgen: scriptDeploy = " << scriptDeploy;
	if (scriptDeploy.empty() == false)
	{
		script = _rules->getMapScript(scriptDeploy);
		if (script == nullptr) Log(LOG_WARNING) << "bGen::nextStage() - There is a Deployment script defined ["
												<< scriptDeploy << "] but could not find its rule.";
	}

	if (script == nullptr)
	{
		const std::string scriptTerra = _terrainRule->getScript();
		//Log(LOG_INFO) << "bgen: scriptTerra = " << scriptTerra;
		if (scriptTerra.empty() == false)
		{
			script = _rules->getMapScript(scriptTerra);
			if (script == nullptr) Log(LOG_WARNING) << "bGen::nextStage() - There is a Terrain script defined ["
													<< scriptTerra << "] but could not find its rule.";
		}
	}

	if (script == nullptr)
	{
		throw Exception("Map generator encountered an error: no script found. See log for detail.");
	}

	generateMap(script); // <-- BATTLE MAP GENERATION.
	setupObjectives(deployRule);

	if (deployRule->getShade() != -1)
		setShade(deployRule->getShade());

	_battleSave->setTacticalShade(_shade);

	_battleSave->setBattleTerrain(_terrainRule->getType());
	setTacticalSprites();

	deployXCOM(); // <-- XCOM DEPLOYMENT.

	const size_t unitCount = _battleSave->getUnits()->size();

	deployAliens(deployRule); // <-- ALIEN DEPLOYMENT.

	if (unitCount == _battleSave->getUnits()->size())
	{
		throw Exception("Map generator encountered an error: no alien units could be placed on the map.");
	}

	deployCivilians(deployRule->getCivilians()); // <-- CIVILIAN DEPLOYMENT.

	if (_generateFuel == true)
		fuelPowerSources();

	if (_ufo != nullptr && _ufo->getUfoStatus() == Ufo::CRASHED)
		explodePowerSources();

/*	if (_missionType == "STR_BASE_DEFENSE")
	{
		for (int i = 0; i < _battleSave->getMapSizeXYZ(); ++i)
			_battleSave->getTiles()[i]->setDiscovered(true, 2);
		_battleSave->calculateModuleMap();
	}
	if (_missionType == "STR_ALIEN_BASE_ASSAULT"
		|| _missionType == "STR_MARS_THE_FINAL_ASSAULT")
	{
		for (int i = 0; i < _battleSave->getMapSizeXYZ(); ++i)
		{
			if (_battleSave->getTiles()[i]->getMapData(O_FLOOR)
				&& (_battleSave->getTiles()[i]->getMapData(O_FLOOR)->getSpecialType() == START_POINT
					|| (_battleSave->getTiles()[i]->getPosition().z == 1
						&& _battleSave->getTiles()[i]->getMapData(O_FLOOR)->isGravLift()
						&& _battleSave->getTiles()[i]->getMapData(O_OBJECT))))
			{
				_battleSave->getTiles()[i]->setDiscovered(true, 2);
			}
		}
	} */
/*	if (_craftDeployed == false)
	{
		for (int i = 0; i < _battleSave->getMapSizeXYZ(); ++i)
		{
			if (_battleSave->getTiles()[i]->getMapData(O_FLOOR) != nullptr
				&& _battleSave->getTiles()[i]->getMapData(O_FLOOR)->getSpecialType() == START_POINT)
//					|| (_battleSave->getTiles()[i]->getPosition().z == _mapsize_z - 1
//						&& _battleSave->getTiles()[i]->getMapData(O_FLOOR)->isGravLift()
//						&& _battleSave->getTiles()[i]->getMapData(O_OBJECT))
			{
				_battleSave->getTiles()[i]->setDiscovered(true, 2);
			}
		}
	} */

	std::vector<std::string> tracks (deployRule->getDeploymentMusics());
	if (tracks.empty() == false)
		_battleSave->setMusic(tracks.at(RNG::pick(tracks.size())));
	else
	{
		tracks = _terrainRule->getTerrainMusics();
		if (tracks.empty() == false)
			_battleSave->setMusic(tracks.at(RNG::pick(tracks.size())));
	}

	_battleSave->getTileEngine()->calculateSunShading();
	_battleSave->getTileEngine()->calculateTerrainLighting();
	_battleSave->getTileEngine()->calculateUnitLighting();

	_battleSave->getTileEngine()->recalculateFOV();
}

/**
 * Switches an existing SavedBattleGame to a new stage.
 */
void BattlescapeGenerator::nextStage()
{
	_battleSave->resetTurnCounter();

	int aliensAlive (0);

	const Position posBogus (Position(-1,-1,-1));
	for (std::vector<BattleUnit*>::const_iterator // kill all enemy units or those not in endpoint area if aborted
			i = _battleSave->getUnits()->begin();
			i != _battleSave->getUnits()->end();
			++i)
	{
		if ((*i)->getUnitStatus() != STATUS_DEAD
			&& ((*i)->getOriginalFaction() != FACTION_PLAYER
				|| (_battleSave->isAborted() == true
					&& (*i)->isInExitArea(END_POINT) == false)))
		{
			if ((*i)->getOriginalFaction() == FACTION_HOSTILE
				&& (*i)->isOut_t(OUT_STAT) == false)
			{
				++aliensAlive;
			}

			(*i)->setUnitStatus(STATUS_LIMBO);
		}

		if ((*i)->getTile() != nullptr) // break old Tile's link to unit.
			(*i)->getTile()->setUnit(nullptr);

		(*i)->setTile(nullptr);				// break unit's link to all Tiles.
		(*i)->setPosition(posBogus, false);	// and give it a bogus Position
	}

	// Remove all items not belonging to soldiers from the map;
	// sort items into two categories:
	// - the ones that are guaranteed to be able to take home barring complete failure - ie: stuff on the transport craft
	// - and those that are scattered about on the ground that will be recovered ONLY on success.
	// This does not include items in soldiers' hands.
	std::vector<BattleItem*>
		* takeHomeGuaranteed (_battleSave->getGuaranteedRecoveredItems()),
		* takeHomeConditional (_battleSave->getConditionalRecoveredItems()),
		takeToNextStage,
		carryToNextStage,
		removeFromGame;

	for (std::vector<BattleItem*>::const_iterator
			i = _battleSave->getItems()->begin();
			i != _battleSave->getItems()->end();
			++i)
	{
		if ((*i)->isLoadedAmmo() == false)
		{
			std::vector<BattleItem*>* toContainer;

			// assign the item a destination container ->
			if ((*i)->getOwner() == nullptr
				&& (*i)->getRules()->isRecoverable() == true)
			{
				(*i)->setFuse(-1);

				if (aliensAlive == 0)
				{
					if ((*i)->getUnit() != nullptr
						|| _gameSave->isResearched((*i)->getRules()->getRequirements()) == false)
					{
						toContainer = takeHomeGuaranteed;
					}
					else
						toContainer = &takeToNextStage;
				}
				else
				{
					const Tile* const tile ((*i)->getTile());
					if (tile != nullptr)
					{
						if (tile->getMapData(O_FLOOR) != nullptr
							&& tile->getMapData(O_FLOOR)->getSpecialType() == START_POINT)
						{
							toContainer = takeHomeGuaranteed;
						}
						else if (tile->getMapData(O_FLOOR) != nullptr
							&& tile->getMapData(O_FLOOR)->getSpecialType() == END_POINT)
						{
							toContainer = &takeToNextStage;
						}
						else
							toContainer = takeHomeConditional;
					}
					else
						toContainer = &removeFromGame;
				}
			}
			else if ((*i)->getOwner() != nullptr
				&& (*i)->getOwner()->getFaction() == FACTION_PLAYER)
			{
				toContainer = &carryToNextStage;
			}
			else
				toContainer = &removeFromGame;

			// move the item to its destination container ->
			BattleItem* const load = (*i)->getAmmoItem();
			if (load != nullptr && *i != load)
			{
				load->setTile(nullptr);
				toContainer->push_back(load);
			}

			(*i)->setTile(nullptr);
			toContainer->push_back(*i);
		}
	}

	for (std::vector<BattleItem*>::const_iterator
			i = removeFromGame.begin();
			i != removeFromGame.end();
			++i)
	{
		if ((*i)->getOwner() != nullptr)
		{
			for (std::vector<BattleItem*>::const_iterator
					j = (*i)->getOwner()->getInventory()->begin();
					j != (*i)->getOwner()->getInventory()->end();
					++j)
			{
				if (*i == *j)
				{
					(*i)->getOwner()->getInventory()->erase(j);
					break;
				}
			}
		}

		delete *i;
	}

	_battleSave->getItems()->clear();

	for (std::vector<BattleItem*>::const_iterator
			i = carryToNextStage.begin();
			i != carryToNextStage.end();
			++i)
	{
		_battleSave->getItems()->push_back(*i);
	}


	AlienDeployment* const deployRule (_rules->getDeployment(_battleSave->getTacticalType()));
	deployRule->getDimensions(
						&_mapsize_x,
						&_mapsize_y,
						&_mapsize_z);

	_terrainRule = _rules->getTerrain(deployRule->getDeployTerrains().at(RNG::pick(deployRule->getDeployTerrains().size())));


	const std::vector<MapScript*>* scripts (nullptr); // rule: Deployment takes priority over Terrain.

	std::string script (deployRule->getScript());
	if (script.empty() == false)
	{
		scripts = _rules->getMapScript(script);
		if (scripts == nullptr)
		{
			Log(LOG_WARNING) << "BattlescapeGenerator::nextStage() - There is a Deployment script defined ["
							 << script << "] but could not find its rule.";
		}
	}

	if (scripts == nullptr)
	{
		script = _terrainRule->getScript();
		if (script.empty() == false)
		{
			scripts = _rules->getMapScript(script);
			if (scripts == nullptr)
				Log(LOG_WARNING) << "BattlescapeGenerator::nextStage() - There is a Terrain script defined ["
								 << script << "] but could not find its rule.";
		}
	}

	if (scripts == nullptr)
	{
		throw Exception("Map generator encountered an error: no script found. See log for detail.");
	}

	generateMap(scripts); // <-- BATTLE MAP GENERATION.
	setupObjectives(deployRule);

	setShade(deployRule->getShade()); // note: 2nd stage must have deployment-shade set, else 0 (bright).
	_battleSave->setTacticalShade(_shade);
	_battleSave->setBattleTerrain(_terrainRule->getType());
//	setTacticalSprites();
	_battleSave->setAborted(false);

	bool selectDone = false;
	for (std::vector<BattleUnit*>::const_iterator // <-- XCOM DEPLOYMENT.
			i = _battleSave->getUnits()->begin();
			i != _battleSave->getUnits()->end();
			++i)
	{
		if ((*i)->getOriginalFaction() == FACTION_PLAYER
			&& (*i)->isOut_t(OUT_STAT) == false)
		{
			(*i)->setFaction(FACTION_PLAYER);
			(*i)->setExposed(-1);
//			(*i)->getVisibleTiles()->clear();

			if (selectDone == false
				&& (*i)->getGeoscapeSoldier() != nullptr)
			{
				_battleSave->setSelectedUnit(*i);
				selectDone = true;
			}

			const Node* const node = _battleSave->getSpawnNode(NR_XCOM, *i);
			if (node != nullptr
				|| placeUnitNearFriend(*i) == true)
			{
				if (node != nullptr)
					_battleSave->setUnitPosition(*i, node->getPosition());

				if (_tileEquipt == nullptr)
				{
					_tileEquipt = (*i)->getTile();
					_battleSave->setBattleInventory(_tileEquipt);
				}

				_tileEquipt->setUnit(*i);		// bogus Tile until resetUnitsOnTiles() runs
				(*i)->setUnitVisible(false);	// when exiting preBattle's InventoryState.
				(*i)->prepUnit(false);
			}
			else
			{
				(*i)->setUnitStatus(STATUS_LIMBO);
				Log(LOG_WARNING) << "BattlescapeGenerator::nextStage() - Could not place xCom unit ["
								 << (*i)->getId() << "] Send to Limbo.";
			}
		}
	}

	if (_battleSave->getSelectedUnit() == nullptr
		|| _battleSave->getSelectedUnit()->getFaction() != FACTION_PLAYER)
	{
		_battleSave->selectNextFactionUnit();
	}

	RuleInventory* const grndRule = _game->getRuleset()->getInventory("STR_GROUND");
	for (std::vector<BattleItem*>::const_iterator
		i = takeToNextStage.begin();
		i != takeToNextStage.end();
		++i)
	{
		_battleSave->getItems()->push_back(*i);
		_tileEquipt->addItem(*i, grndRule);
	}


	_alienRace = deployRule->getRace();
	if (_alienRace.empty() == true)
	{
		for (std::vector<MissionSite*>::const_iterator
				i = _gameSave->getMissionSites()->begin();
				i != _gameSave->getMissionSites()->end();
				++i)
		{
			if ((*i)->isInBattlescape() == true)
			{
				_alienRace = (*i)->getAlienRace();
				break;
			}
		}

		if (_alienRace.empty() == true)
		{
			for (std::vector<AlienBase*>::const_iterator
					i = _gameSave->getAlienBases()->begin();
					i != _gameSave->getAlienBases()->end();
					++i)
			{
				if ((*i)->isInBattlescape() == true)
				{
					_alienRace = (*i)->getAlienRace();
					break;
				}
			}
		}
	}

	if (_alienRace.empty() == true)
	{
		throw Exception("Map generator encountered an error: no alien race could be determined.");
	}


	_unitSequence = _battleSave->getUnits()->back()->getId() + 1;

	const size_t unitCount = _battleSave->getUnits()->size();

	deployAliens(deployRule); // <-- ALIEN DEPLOYMENT.

	if (unitCount == _battleSave->getUnits()->size())
	{
		throw Exception("Map generator encountered an error: no alien units could be placed on the map.");
	}

	deployCivilians(deployRule->getCivilians()); // <-- CIVILIAN DEPLOYMENT.

/*	// Probly don't need this anymore; it's done via "revealedFloors" in MapScripting ... but not quite.
	for (int i = 0; i < _battleSave->getMapSizeXYZ(); ++i)
	{
		if (_battleSave->getTiles()[i]->getMapData(O_FLOOR) != nullptr
			&& _battleSave->getTiles()[i]->getMapData(O_FLOOR)->getSpecialType() == START_POINT)
//				|| (_battleSave->getTiles()[i]->getPosition().z == 1
//					&& _battleSave->getTiles()[i]->getMapData(O_FLOOR)->isGravLift()
//					&& _battleSave->getTiles()[i]->getMapData(O_OBJECT))
		{
			_battleSave->getTiles()[i]->setDiscovered(true, 2);
		}
	} */

	_battleSave->getTileEngine()->calculateSunShading();
	_battleSave->getTileEngine()->calculateTerrainLighting();
	_battleSave->getTileEngine()->calculateUnitLighting();

	_battleSave->getTileEngine()->recalculateFOV();
}

/**
* Deploys all the X-COM units and equipment based on the Geoscape base / craft.
*/
void BattlescapeGenerator::deployXCOM() // private.
{
	//Log(LOG_INFO) << "BattlescapeGenerator::deployXCOM()";
	if (_craft != nullptr)
	{
		_base = _craft->getBase();

		if (_baseEquipScreen == false)
		{
			//Log(LOG_INFO) << ". craft VALID";
			// add all vehicles that are in the craft - a vehicle is actually an item,
			// which you will never see as it is converted to a unit;
			// however the item itself becomes the weapon it equips.
			for (std::vector<Vehicle*>::const_iterator
					i = _craft->getVehicles()->begin();
					i != _craft->getVehicles()->end();
					++i)
			{
				//Log(LOG_INFO) << ". . isCraft: addXCOMVehicle " << (int)*i;
				BattleUnit* const unit = addXCOMVehicle(*i);
				if (unit != nullptr && _battleSave->getSelectedUnit() == nullptr)
					_battleSave->setSelectedUnit(unit);
			}
		}
	}
	else if (_base != nullptr && _baseEquipScreen == false)
	{
		// add vehicles that are in Base inventory.
		for (std::vector<Vehicle*>::const_iterator
				i = _base->getVehicles()->begin();
				i != _base->getVehicles()->end();
				++i)
		{
			//Log(LOG_INFO) << ". . isBase: addXCOMVehicle " << (int)*i;
			BattleUnit* const unit = addXCOMVehicle(*i);
			if (unit != nullptr && _battleSave->getSelectedUnit() == nullptr)
				_battleSave->setSelectedUnit(unit);
		}

		// Only add vehicles from the craft in new battle mode
		// otherwise the base's vehicle vector will already contain these
		// due to the geoscape calling base->setupDefenses().
		if (_gameSave->getMonthsPassed() == -1)
		{
			// add vehicles from Crafts at Base.
			for (std::vector<Craft*>::const_iterator
				i = _base->getCrafts()->begin();
				i != _base->getCrafts()->end();
				++i)
			{
				for (std::vector<Vehicle*>::const_iterator
					j = (*i)->getVehicles()->begin();
					j != (*i)->getVehicles()->end();
					++j)
				{
					BattleUnit* const unit = addXCOMVehicle(*j);
					if (unit != nullptr && _battleSave->getSelectedUnit() == nullptr)
						_battleSave->setSelectedUnit(unit);
				}
			}
		}
	}

	// add soldiers that are in the Craft or Base.
	for (std::vector<Soldier*>::const_iterator
			i = _base->getSoldiers()->begin();
			i != _base->getSoldiers()->end();
			++i)
	{
		if ((_craft != nullptr
				&& (*i)->getCraft() == _craft)
			|| (_craft == nullptr
				&& (*i)->getRecovery() == 0
				&& ((*i)->getCraft() == nullptr
					|| (*i)->getCraft()->getCraftStatus() != "STR_OUT")))
		{
			//Log(LOG_INFO) << ". . addXCOMUnit " << (*i)->getId();
			BattleUnit* const unit = addXCOMUnit(new BattleUnit(*i, _gameSave->getDifficulty())); // for VictoryPts value per death.
			if (unit != nullptr)
			{
				//Log(LOG_INFO) << "bGen::deployXCOM() ID " << unit->getId() << " battleOrder = " << _battleOrder + 1;
				unit->setBattleOrder(++_battleOrder);

				if (_battleSave->getSelectedUnit() == nullptr)
					_battleSave->setSelectedUnit(unit);
			}
		}
	}

	if (_battleSave->getUnits()->empty() == true)
	{
		throw Exception("Map generator encountered an error: no xcom units could be placed on the map.");
	}

	//Log(LOG_INFO) << ". addXCOMUnit(s) DONE";

	for (std::vector<BattleUnit*>::const_iterator // pre-battle equip; give all xCom Soldiers access to the inventory tile.
			i = _battleSave->getUnits()->begin();
			i != _battleSave->getUnits()->end();
			++i)
	{
		if ((*i)->getFaction() == FACTION_PLAYER) // not really necessary because only xCom is on the field atm. Could exclude tanks ....
		{
			_tileEquipt->setUnit(*i);
			(*i)->setUnitVisible(false);
		}
	}
	//Log(LOG_INFO) << ". setUnit(s) DONE";

	RuleInventory* const grndRule = _rules->getInventory("STR_GROUND");

	if (_craft != nullptr) // add items that are in the Craft.
	{
		//Log(LOG_INFO) << ". . addCraftItems";
		for (std::map<std::string, int>::const_iterator
				i = _craft->getCraftItems()->getContents()->begin();
				i != _craft->getCraftItems()->getContents()->end();
				++i)
		{
			//Log(LOG_INFO) << ". . . *i = _craft->getItems()->getContents()";
			for (int
					j = 0;
					j != i->second;
					++j)
			{
				//Log(LOG_INFO) << ". . . j+";
				_tileEquipt->addItem(
								new BattleItem(
											_rules->getItem(i->first),
											_battleSave->getNextItemId()),
								grndRule);
				//Log(LOG_INFO) << ". . . j cycle";
			}
		}
		//Log(LOG_INFO) << ". . addCraftItems DONE";
	}
	else // add items that are in the Base.
	{
		// add only items in Craft for skirmish mode;
		// ie. Do NOT add items from Base in skirmish mode:
		if (_gameSave->getMonthsPassed() != -1)
		{
			//Log(LOG_INFO) << ". . addBaseItems";
			for (std::map<std::string, int>::const_iterator // add items from stores in base
					i = _base->getStorageItems()->getContents()->begin();
					i != _base->getStorageItems()->getContents()->end();
					)
			{
				// only add items to the battlescape that make sense;
				// if item has a sprite it's probably ok
				const RuleItem* const itRule = _rules->getItem(i->first);
				if (itRule->getBigSprite() != -1
					&& itRule->getBattleType() != BT_NONE
					&& itRule->getBattleType() != BT_CORPSE
					&& itRule->isFixed() == false
					&& _gameSave->isResearched(itRule->getRequirements()) == true)
				{
					for (int
							j = 0;
							j != i->second;
							++j)
					{
						_tileEquipt->addItem(
										new BattleItem(
													_rules->getItem(i->first),
													_battleSave->getNextItemId()),
										grndRule);
					}

					const std::map<std::string, int>::const_iterator baseItem = i++;
					if (_baseEquipScreen == false)
						_base->getStorageItems()->removeItem(
														baseItem->first,
														baseItem->second);
				}
				else
					++i;
			}
		}
		//Log(LOG_INFO) << ". . addBaseBaseItems DONE, add BaseCraftItems";

		for (std::vector<Craft*>::const_iterator // add items from Crafts at Base.
				i = _base->getCrafts()->begin();
				i != _base->getCrafts()->end();
				++i)
		{
			if ((*i)->getCraftStatus() != "STR_OUT")
			{
				for (std::map<std::string, int>::const_iterator
						j = (*i)->getCraftItems()->getContents()->begin();
						j != (*i)->getCraftItems()->getContents()->end();
						++j)
				{
					for (int
							k = 0;
							k != j->second;
							++k)
					{
						_tileEquipt->addItem(
										new BattleItem(
													_rules->getItem(j->first),
													_battleSave->getNextItemId()),
										grndRule);
					}
				}
			}
		}
		//Log(LOG_INFO) << ". . addBaseCraftItems DONE";
	}
	//Log(LOG_INFO) << ". addItem(s) DONE";


	// kL_note: ALL ITEMS SEEM TO STAY ON THE GROUNDTILE, _tileEquipt,
	// IN THAT INVENTORY(vector) UNTIL EVERYTHING IS EQUIPPED & LOADED. Then
	// the inventory-tile is cleaned up at the end of this function....
	//
	// equip soldiers based on equipment-layout
	//Log(LOG_INFO) << ". placeItemByLayout Start";
	for (std::vector<BattleItem*>::const_iterator
			i = _tileEquipt->getInventory()->begin();
			i != _tileEquipt->getInventory()->end();
			++i)
	{
		//Log(LOG_INFO) << ". placeItemByLayout() NOT ammo allocation";
		// don't let the soldiers take extra ammo yet
		if ((*i)->getRules()->getBattleType() != BT_AMMO)
			placeItemByLayout(*i);
	}

	for (std::vector<BattleItem*>::const_iterator
			i = _tileEquipt->getInventory()->begin();
			i != _tileEquipt->getInventory()->end();
			++i)
	{
		//Log(LOG_INFO) << ". placeItemByLayout() DO ammo allocation";
		// only need to distribute extra ammo at this point.
		if ((*i)->getRules()->getBattleType() == BT_AMMO)
			placeItemByLayout(*i);
	}
	//Log(LOG_INFO) << ". placeItemByLayout all DONE";

	//Log(LOG_INFO) << ". Load Weapons..."; -> Cf. loadWeapons()
	for (std::vector<BattleItem*>::const_iterator
			i = _tileEquipt->getInventory()->begin();
			i != _tileEquipt->getInventory()->end();
			++i)
	{
		//Log(LOG_INFO) << ". loading."; // from loadWeapons() ->
/*		if ((*i)->getRules()->isFixed() == false
			&& (*i)->getRules()->getCompatibleAmmo()->empty() == false
			&& (*i)->getAmmoItem() == nullptr
			&& ((*i)->getRules()->getBattleType() == BT_FIREARM
				|| (*i)->getRules()->getBattleType() == BT_MELEE)) */
		if ((*i)->selfPowered() == false) // or do it My way.
			loadGroundWeapon(*i);
	}
	//Log(LOG_INFO) << ". loading DONE";

	// clean up moved items
	for (std::vector<BattleItem*>::const_iterator
			i = _tileEquipt->getInventory()->begin();
			i != _tileEquipt->getInventory()->end();
			)
	{
		if ((*i)->getSlot() == grndRule)
		{
//			(*i)->setXcomProperty();
			_battleSave->getItems()->push_back(*i);

			++i;
		}
		else
			i = _tileEquipt->getInventory()->erase(i);
	}
	//Log(LOG_INFO) << "BattlescapeGenerator::deployXCOM() EXIT";
}

/**
 * Adds an XCom vehicle to the game.
 * @note Sets the correct turret depending on the ammo type and adds auxilliary
 * weapons if any.
 * @param vehicle - pointer to Vehicle
 * @return, pointer to the spawned unit; nullptr if unable to create and equip
 */
BattleUnit* BattlescapeGenerator::addXCOMVehicle(Vehicle* const vehicle) // private.
{
	const std::string vhclType = vehicle->getRules()->getType();
	RuleUnit* const unitRule = _rules->getUnit(vhclType);

	BattleUnit* const supportUnit = addXCOMUnit(new BattleUnit( // add Vehicle as a unit.
															unitRule,
															FACTION_PLAYER,
															_unitSequence++,
															_rules->getArmor(unitRule->getArmor()),
															DIFF_BEGINNER)); // <- but do not upgrade tanks
	if (supportUnit != nullptr)
	{
		supportUnit->setTurretType(vehicle->getRules()->getTurretType());

		BattleItem* item = new BattleItem( // add Vehicle as an item and assign the unit as its owner.
									_rules->getItem(vhclType),
									_battleSave->getNextItemId());
		if (addItem(
				item,
				supportUnit) == false)
		{
			Log(LOG_WARNING) << "BattlescapeGenerator could not add: " << vhclType;
			--_unitSequence;

			delete item;
			delete supportUnit;

			return nullptr;
		}

		if (vehicle->getRules()->getCompatibleAmmo()->empty() == false)
		{
			const std::string ammoType = vehicle->getRules()->getCompatibleAmmo()->front();
			BattleItem* const ammoItem = new BattleItem(
													_rules->getItem(ammoType),
													_battleSave->getNextItemId());
			if (addItem( // add 'ammoItem' and assign the Vehicle-ITEM as its owner.
					ammoItem,
					supportUnit) == false)
			{
				Log(LOG_WARNING) << "BattlescapeGenerator could not add [" << ammoType << "] to " << vhclType;
				--_unitSequence;

				delete ammoItem;
				delete item;
				delete supportUnit;

				return nullptr;
			}

			ammoItem->setAmmoQuantity(vehicle->getAmmo());
		}


		if (unitRule->getBuiltInWeapons().empty() == false) // add item(builtInWeapon) -- what about ammo
		{
			for (std::vector<std::string>::const_iterator
					i = unitRule->getBuiltInWeapons().begin();
					i != unitRule->getBuiltInWeapons().end();
					++i)
			{
				RuleItem* const itRule = _rules->getItem(*i);
				if (itRule != nullptr)
				{
					item = new BattleItem(itRule, _battleSave->getNextItemId());
					if (addItem(item, supportUnit) == false)
					{
						Log(LOG_WARNING) << "BattlescapeGenerator could not add [" << itRule->getType() << "] to " << vhclType;
						delete item;
					}
				}
			}
		}
	}

	return supportUnit;
}

/**
 * Adds a soldier to the game and places him on a free spawnpoint.
 * Spawnpoints are either tiles in case of an XCom craft that landed
 * or they are mapnodes in case there's no craft.
 * @param unit - pointer to an xCom BattleUnit
 * @return, pointer to the spawned unit if successful else nullptr
 */
BattleUnit* BattlescapeGenerator::addXCOMUnit(BattleUnit* const unit) // private.
{
	//Log(LOG_INFO) << "bsg:addXCOMUnit()";
	if ((_craft == nullptr || _craftDeployed == false) // (_missionType == "STR_ALIEN_BASE_ASSAULT" || _missionType == "STR_MARS_THE_FINAL_ASSAULT") <- taken care of in MapScripting.
		&& _baseEquipScreen == false)
	{
		//Log(LOG_INFO) << ". no Craft";
		const Node* const node = _battleSave->getSpawnNode(NR_XCOM, unit);
		if (node != nullptr)
		{
			//Log(LOG_INFO) << ". . spawnNode valid";
			_battleSave->getUnits()->push_back(unit); // add unit to vector of Units.

			_battleSave->setUnitPosition(unit, node->getPosition());
			unit->setUnitDirection(RNG::generate(0,7));

			_tileEquipt = _battleSave->getTile(node->getPosition());
			_battleSave->setBattleInventory(_tileEquipt);

			return unit;
		}
		else if (_battleSave->getTacType() != TCT_BASEDEFENSE)
		{
			//Log(LOG_INFO) << ". . spawnNode NOT valid - not baseDefense";
			if (placeUnitNearFriend(unit) == true)
			{
				//Log(LOG_INFO) << ". . . placeUnitNearFriend() TRUE";
				_battleSave->getUnits()->push_back(unit); // add unit to vector of Units.

				unit->setUnitDirection(RNG::generate(0,7));

				_tileEquipt = _battleSave->getTile(unit->getPosition());
				_battleSave->setBattleInventory(_tileEquipt);

				return unit;
			}
		}
	}
	else if (_craft != nullptr // Transport craft deployments (Lightning & Avenger)
		&& _craft->getRules()->getCraftDeployment().empty() == false
		&& _baseEquipScreen == false)
	{
		//Log(LOG_INFO) << ". Craft valid - use Deployment";
		for (std::vector<std::vector<int> >::const_iterator
				i = _craft->getRules()->getCraftDeployment().begin();
				i != _craft->getRules()->getCraftDeployment().end();
				++i)
		{
			//Log(LOG_INFO) << ". getCraftDeployment()";
			const Position pos = Position(
										(*i)[0] + (_craftPos.x * 10),
										(*i)[1] + (_craftPos.y * 10),
										(*i)[2] + _craftZ);
			bool canPlace = true;
			for (int
					x = 0;
					x != unit->getArmor()->getSize()
						&& canPlace == true;
					++x)
			{
				for (int
						y = 0;
						y != unit->getArmor()->getSize()
							&& canPlace == true;
						++y)
				{
					canPlace = canPlaceXComUnit(_battleSave->getTile(pos + Position(x,y,0)));
				}
			}

			if (canPlace == true)
			{
				//Log(LOG_INFO) << ". canPlaceXComUnit()";
				if (_battleSave->setUnitPosition(unit, pos) == true)
				{
					//Log(LOG_INFO) << ". setUnitPosition()";
					_battleSave->getUnits()->push_back(unit); // add unit to vector of Units.

					unit->setUnitDirection((*i)[3]);

					return unit;
				}
			}
		}
	}
	else // mission w/ transport craft that does not have ruleset Deployments. Or it's a craft/base Equip.
	{
		//Log(LOG_INFO) << ". baseEquip OR Craft w/out Deployment rule";
		int tankPos = 0;

		const size_t mapSize = static_cast<size_t>(_mapsize_x * _mapsize_y * _mapsize_z);
		for (size_t
				i = 0;
				i != mapSize;
				++i)
		{
			if (canPlaceXComUnit(_battleSave->getTiles()[i]) == true)
			{
				if (unit->getGeoscapeSoldier() == nullptr)
				{
					if ((unit->getArmor()->getSize() == 1
							|| _battleSave->getTiles()[i]->getPosition().x == _tileEquipt->getPosition().x)
						&& ++tankPos == 3
						&& _battleSave->setUnitPosition(
													unit,
													_battleSave->getTiles()[i]->getPosition()) == true)
					{
						_battleSave->getUnits()->push_back(unit); // add unit to vector of Units.
						return unit;
					}
				}
				else if (_battleSave->setUnitPosition(
													unit,
													_battleSave->getTiles()[i]->getPosition()) == true)
				{
					_battleSave->getUnits()->push_back(unit); // add unit to vector of Units.
					return unit;
				}
			}
		}
	}

	delete unit; // fallthrough if above fails.
	return nullptr;
}

/**
 * Checks if a soldier/tank can be placed on a given tile.
 * @param tile - the given tile
 * @return, true if unit can be placed on Tile
 */
bool BattlescapeGenerator::canPlaceXComUnit(Tile* const tile) // private.
{
	// to spawn an xcom soldier there has to be a tile with a floor
	// with the starting point attribute and no objects in the way
	if (tile != nullptr													// is a tile
		&& tile->getMapData(O_FLOOR) != nullptr							// has a floor
		&& tile->getMapData(O_FLOOR)->getSpecialType() == START_POINT	// is a 'start point', ie. cargo tile
		&& tile->getMapData(O_OBJECT) == nullptr						// no object content
		&& tile->getMapData(O_FLOOR)->getTuCostPart(MT_WALK) < 255		// is walkable.
		&& tile->getUnit() == nullptr)									// and no unit on Tile.
	{
		// ground inventory goes where the first xCom unit spawns
		if (_tileEquipt == nullptr)
		{
			_tileEquipt = tile;
			_battleSave->setBattleInventory(_tileEquipt);
		}

		return true;
	}

	return false;
}

/**
 * Loads a weapon on the inventoryTile.
 * @param item - pointer to a BattleItem
 */
void BattlescapeGenerator::loadGroundWeapon(BattleItem* const item) // private.
{
	const RuleInventory* const ground = _rules->getInventory("STR_GROUND");
	RuleInventory* const righthand = _rules->getInventory("STR_RIGHT_HAND");

	for (std::vector<BattleItem*>::const_iterator
			i = _tileEquipt->getInventory()->begin();
			i != _tileEquipt->getInventory()->end();
			++i)
	{
		if ((*i)->getSlot() == ground
			&& item->setAmmoItem(*i) == 0)
		{
//			(*i)->setXcomProperty();
			(*i)->setSlot(righthand); // trick to remove ammo from ground-slot
			_battleSave->getItems()->push_back(*i);
		}
	}
}

/**
 * Places an item on an XCom soldier based on equipment layout.
 * @param item - pointer to a BattleItem
 * @return, true if item is placed successfully
 */
bool BattlescapeGenerator::placeItemByLayout(BattleItem* const item) // private.
{
	const RuleInventory* const ground = _rules->getInventory("STR_GROUND");
	if (item->getSlot() == ground)
	{
		RuleInventory* const righthand = _rules->getInventory("STR_RIGHT_HAND");
		bool loaded = false;

		for (std::vector<BattleUnit*>::const_iterator // find the first soldier with a matching layout-slot
				i = _battleSave->getUnits()->begin();
				i != _battleSave->getUnits()->end();
				++i)
		{
			if ((*i)->getGeoscapeSoldier() != nullptr // handle only with xCom soldiers that have a layout
				&& (*i)->getGeoscapeSoldier()->getEquipmentLayout()->empty() == false)
			{
				const std::vector<EquipmentLayoutItem*>* const layoutItems = (*i)->getGeoscapeSoldier()->getEquipmentLayout();
				for (std::vector<EquipmentLayoutItem*>::const_iterator
						j = layoutItems->begin();
						j != layoutItems->end();
						++j)
				{
					if ((*j)->getItemType() == item->getRules()->getType() // find the first matching layout-slot which is not already occupied
						&& (*i)->getItem(
									(*j)->getSlot(),
									(*j)->getSlotX(),
									(*j)->getSlotY()) == nullptr)
					{
						if ((*j)->getAmmoItem() == "NONE")
							loaded = true;
						else
						{
							loaded = false;

							for (std::vector<BattleItem*>::const_iterator		// maybe we find the layout-ammo on the ground to load it with
									k = _tileEquipt->getInventory()->begin();	// Yeh maybe: as in maybe this works maybe it doesn't
									k != _tileEquipt->getInventory()->end();
									++k)
							{
								if ((*k)->getRules()->getType() == (*j)->getAmmoItem()
									&& (*k)->getSlot() == ground		// why the redundancy?
																		// WHAT OTHER _tileEquipt IS THERE BUT THE GROUND TILE!!??!!!1
									&& item->setAmmoItem(*k) == 0)		// okay, so load the damn item.
								{
//									(*k)->setXcomProperty();
									(*k)->setSlot(righthand);			// why are you putting ammo in his right hand.....
																		// maybe just to get it off the ground so it doesn't get loaded into another weapon later.
									_battleSave->getItems()->push_back(*k);

									loaded = true;
									break;
									// note: soldier is not owner of the ammo, this fact is relevant to the saved layouts
								}
							}
						}

						if (loaded == true)	// only place the item onto the soldier when it's loaded with its layout-ammo if any
						{
//							item->setXcomProperty();

							item->moveToOwner(*i);

							item->setSlot(_rules->getInventory((*j)->getSlot()));
							item->setSlotX((*j)->getSlotX());
							item->setSlotY((*j)->getSlotY());

							if (item->getRules()->isGrenade() == true
								&& Options::includePrimeStateInSavedLayout == true)
							{
								item->setFuse((*j)->getFuse());
							}

							_battleSave->getItems()->push_back(item);

							return true;
						}
					}
				}
			}
		}
	}

	return false;
}

/**
 * Sets xCom soldiers' combat clothing style - spritesheets & paperdolls.
 * @note Uses EqualTerms v1 graphics to replace stock resources. Affects soldiers
 * wearing pyjamas (STR_ARMOR_NONE_UC) only. This is done by switching in/out
 * equivalent Armors.
 */
void BattlescapeGenerator::setTacticalSprites() const // private.
{
	RuleArmor* const armorRule = _rules->getArmor(_terrainRule->getPyjamaType());

	Base* base;
	if (_craft != nullptr)
		base = _craft->getBase();
	else
		base = _base;

	for (std::vector<Soldier*>::const_iterator
			i = base->getSoldiers()->begin();
			i != base->getSoldiers()->end();
			++i)
	{
		if ((_craft == nullptr || (*i)->getCraft() == _craft)
			&& (*i)->getArmor()->isBasic() == true)
		{
			(*i)->setArmor(armorRule);
		}
	}
}
/*
// base defense, craft nullptr "STR_BASE_DEFENSE"
// ufo, base nullptr "STR_UFO_CRASH_RECOVERY" "STR_UFO_GROUND_ASSAULT" "STR_TERROR_MISSION" "STR_ALIEN_BASE_ASSAULT"
// cydonia "STR_MARS_CYDONIA_LANDING" "STR_MARS_THE_FINAL_ASSAULT"

	if ((_craft == nullptr // both Craft & Base are nullptr for the 2nd of a 2-part mission.
			&& _base == nullptr)
		|| _missionType == "STR_BASE_DEFENSE")
	{
		return;
	}

	std::string stArmor = "STR_ARMOR_NONE_UC";

	if (_isCity == true)
		stArmor = "STR_STREET_URBAN_UC";
	else if (_missionType == "STR_MARS_CYDONIA_LANDING"
		|| _missionType == "STR_MARS_THE_FINAL_ASSAULT")
	{
		stArmor = "STR_STREET_ARCTIC_UC";
	}
	else if (_missionType == "STR_TERROR_MISSION"
		|| _missionType == "STR_ALIEN_BASE_ASSAULT")
	{
		stArmor = "STR_STREET_URBAN_UC";
	}
	else
	{
		if ((_texture > -1 && _texture < 7)
			|| (_texture > 9 && _texture < 12))
		{
			stArmor = "STR_STREET_JUNGLE_UC";
		}
		else if (_texture > 6 && _texture < 10
			|| _texture == 12)
		{
			stArmor = "STR_STREET_ARCTIC_UC";
		}
	} */

/**
 * Adds an item to an XCom soldier (auto-equip ONLY). kL_note: I don't use that part.
 * Or to an XCom tank, also adds items & terrorWeapons to aLiens, deployAliens()!
 * @param item - pointer to the BattleItem
 * @param unit - pointer to the BattleUnit
 * @return, true if item was placed
 */
bool BattlescapeGenerator::addItem( // private.
		BattleItem* const item,
		BattleUnit* const unit) const
{
	RuleInventory
		* const rightHand = _rules->getInventory("STR_RIGHT_HAND"),
		* const leftHand = _rules->getInventory("STR_LEFT_HAND");
	BattleItem
		* const rhWeapon = unit->getItem("STR_RIGHT_HAND"),
		* const lhWeapon = unit->getItem("STR_LEFT_HAND");

	RuleItem* const itRule = item->getRules();

	int placed = 0;

	if (itRule->isFixed() == true)
	{
		if (rhWeapon == nullptr)
		{
			item->setSlot(rightHand);
			placed = 1;
		}
		else if (lhWeapon == nullptr)
		{
			item->setSlot(leftHand);
			placed = 1;
		}
	}
	else
	{
		switch (itRule->getBattleType())
		{
			case BT_FIREARM:
			case BT_MELEE:
				if (rhWeapon == nullptr)
				{
					item->setSlot(rightHand);
					placed = 1;
				}
				else if (lhWeapon == nullptr)
				{
					item->setSlot(leftHand);
					placed = 1;
				}
			break;

			case BT_AMMO:
			{
				if (rhWeapon != nullptr
					&& (rhWeapon->getRules()->isFixed() == true
						|| unit->getFaction() != FACTION_PLAYER)
					&& rhWeapon->getAmmoItem() == nullptr
					&& rhWeapon->setAmmoItem(item) == 0)
				{
					item->setSlot(rightHand);
					placed = 2;
					break;
				}

				if (lhWeapon != nullptr
					&& (lhWeapon->getRules()->isFixed() == true
						|| unit->getFaction() != FACTION_PLAYER)
					&& lhWeapon->getAmmoItem() == nullptr
					&& lhWeapon->setAmmoItem(item) == 0)
				{
					item->setSlot(leftHand);
					placed = 2;
					break;
				}

				for (int // else put the clip in Belt or Backpack
						i = 0;
						i != 4;
						++i)
				{
					if (unit->getItem("STR_BELT", i) == false
						&& _rules->getInventory("STR_BELT")->fitItemInSlot(itRule, i, 0))
					{
						item->setSlot(_rules->getInventory("STR_BELT"));
						item->setSlotX(i);
						placed = 1;
						break;
					}
				}

				if (placed == 0)
				{
					for (int
							i = 0;
							i != 3;
							++i)
					{
						if (unit->getItem("STR_BACK_PACK", i) == false
							&& _rules->getInventory("STR_BACK_PACK")->fitItemInSlot(itRule, i, 0))
						{
							item->setSlot(_rules->getInventory("STR_BACK_PACK"));
							item->setSlotX(i);
							placed = 1;
							break;
						}
					}
				}
			}
			break;

			case BT_GRENADE: // includes AlienGrenades & SmokeGrenades & HE-Packs.
			case BT_PROXYGRENADE:
				for (int
						i = 0;
						i != 4;
						++i)
				{
					if (unit->getItem("STR_BELT", i) == false)
					{
						item->setSlot(_rules->getInventory("STR_BELT"));
						item->setSlotX(i);
						placed = 1;
						break;
					}
				}
			break;

			case BT_MINDPROBE:
			case BT_MEDIKIT:
			case BT_SCANNER:
				if (unit->getItem("STR_BACK_PACK") == false)
				{
					item->setSlot(_rules->getInventory("STR_BACK_PACK"));
					placed = 1;
				}
		}
	}

	switch (placed)
	{
		case 1:
			item->moveToOwner(unit); // no break.
		case 2:
//			item->setXcomProperty(unit->getFaction() == FACTION_PLAYER);
			_battleSave->getItems()->push_back(item);
			return true;
	}

	return false; // If not placed the item will be deleted.
}

/**
 * Deploys the aLiens according to the AlienDeployment rules.
 * @param deployRule - pointer to the AlienDeployment rule
 */
void BattlescapeGenerator::deployAliens(AlienDeployment* const deployRule) // private.
{
	int month = _gameSave->getMonthsPassed();
	if (month != -1)
	{
		const int itemLevel_top = static_cast<int>(_rules->getAlienItemLevels().size()) - 1;
		if (month > itemLevel_top)
			month = itemLevel_top;

		if (deployRule->getRace().empty() == false) // race re-defined by deployment if there is one.
			_alienRace = deployRule->getRace();
	}
	else
		month = _alienItemLevel;

	const AlienRace* const raceRule = _game->getRuleset()->getAlienRace(_alienRace);
	if (raceRule == nullptr)
	{
		throw Exception("Map generator encountered an error: Unknown race: ["
				+ _alienRace + "] defined in deployAliens()");
	}


	std::string aLien;
	bool outside;
	int qty;
	size_t itemLevel;

	RuleItem* itRule;
	BattleItem* item;
	RuleUnit* unitRule;
	BattleUnit* unit;

	for (std::vector<DeploymentData>::const_iterator
			data = deployRule->getDeploymentData()->begin();
			data != deployRule->getDeploymentData()->end();
			++data)
	{
		aLien = raceRule->getMember((*data).alienRank);

		if (_gameSave->getDifficulty() < DIFF_VETERAN)
			qty = (*data).lowQty
				+ RNG::generate(0, (*data).dQty); // beginner/experienced
		else if (_gameSave->getDifficulty() < DIFF_SUPERHUMAN)
			qty = (*data).lowQty
				+ (((*data).highQty - (*data).lowQty) / 2)
				+ RNG::generate(0, (*data).dQty); // veteran/genius
		else
			qty = (*data).highQty
				+ RNG::generate(0, (*data).dQty); // super (and beyond)

		qty += RNG::generate(0, (*data).extraQty);

		if (_base != nullptr
			&& _base->getDefenseResult() != 0)
		{
			qty = std::max(
						qty / 2,
						qty - (qty * _base->getDefenseResult() / 100));
		}

		for (int
				i = 0;
				i != qty;
				++i)
		{
			if (_ufo != nullptr)
				outside = RNG::percent((*data).pctOutsideUfo);
			else
				outside = false;

			unitRule = _rules->getUnit(aLien);
			unit = addAlien(
						unitRule,
						(*data).alienRank,
						outside);

			if (unit != nullptr)
			{
				// Built in weapons: the unit has this weapon regardless of loadout or what have you.
				if (unitRule->getBuiltInWeapons().empty() == false)
				{
					for (std::vector<std::string>::const_iterator
							j = unitRule->getBuiltInWeapons().begin();
							j != unitRule->getBuiltInWeapons().end();
							++j)
					{
						itRule = _rules->getItem(*j);
						if (itRule != nullptr)
						{
							item = new BattleItem(
												itRule,
												_battleSave->getNextItemId());
							if (addItem(item, unit) == false)
							{
								Log(LOG_WARNING) << "BattlescapeGenerator could not add ["
												 << itRule->getType() << "] to " << unit->getType();
								delete item;
							}
						}
					}
				}

				// terrorist aliens' equipment is a special case - they are fitted
				// with a weapon which is the alien's name with suffix _WEAPON
				if (unitRule->isLivingWeapon() == true)
				{
					std::string terrorWeapon = unitRule->getRace().substr(4);
					terrorWeapon += "_WEAPON";

					itRule = _rules->getItem(terrorWeapon);
					if (itRule != nullptr)
					{
						item = new BattleItem( // terror aLiens add their weapons
											itRule,
											_battleSave->getNextItemId());
						if (addItem(item, unit) == false)
						{
							Log(LOG_WARNING) << "BattlescapeGenerator could not add ["
											 << itRule->getType() << "] to " << unit->getType();
							delete item;
						}
						else
							unit->setTurretType(item->getRules()->getTurretType());
					}
				}
				else
				{
					if ((*data).itemSets.size() == 0)
					{
						throw Exception("Unit generator encountered an error: item set not defined");
					}

					itemLevel = static_cast<size_t>(_rules->getAlienItemLevels().at(static_cast<size_t>(month)).at(RNG::generate(0,9)));
					if (itemLevel > (*data).itemSets.size() - 1)
						itemLevel = (*data).itemSets.size() - 1;
					// Relax item level requirements
					// <- Yankes; https://github.com/Yankes/OpenXcom/commit/4c252470aa2e261b0f449a56aaea5d5b0cb2229c
/*					if (itemLevel > (*data).itemSets.size() - 1)
					{
						std::ostringstream ststr;
						ststr	<< "Unit generator encountered an error: not enough item sets defined, expected: "
								<< (itemLevel+1) << " found: " << (*data).itemSets.size();
						throw Exception(ststr.str());
					} */

					for (std::vector<std::string>::const_iterator
							setItem = (*data).itemSets.at(itemLevel).items.begin();
							setItem != (*data).itemSets.at(itemLevel).items.end();
							++setItem)
					{
						itRule = _rules->getItem(*setItem);
						if (itRule != nullptr)
						{
							item = new BattleItem( // aLiens add items
												itRule,
												_battleSave->getNextItemId());
							if (addItem(item, unit) == false)
							{
								Log(LOG_WARNING) << "BattlescapeGenerator could not add ["
												 << itRule->getType() << "] to " << unit->getType();
								delete item;
							}
						}
					}
				}
			}
		}
	}

	if (_base != nullptr)
		_base->setDefenseResult(0);
}

/**
 * Adds an alien to the game and places him on a free spawnpoint.
 * @param unitRule	- pointer to the Unit rule which holds info about aLiens
 * @param aLienRank	- rank of the alien used for spawn point search
 * @param outside	- true if the alien should spawn outside the UFO
 * @return, pointer to the created BattleUnit
 */
BattleUnit* BattlescapeGenerator::addAlien( // private.
		RuleUnit* const unitRule,
		int aLienRank,
		bool outside)
{
	const GameDifficulty diff = _gameSave->getDifficulty();
	BattleUnit* const unit = new BattleUnit(
										unitRule,
										FACTION_HOSTILE,
										_unitSequence++,
										_rules->getArmor(unitRule->getArmor()),
										diff,
										_gameSave->getMonthsPassed());

	if (aLienRank > 7) // safety to avoid index out of bounds errors
		aLienRank = 7;

	// following data is the order in which certain alien ranks spawn on certain node ranks;
	// note that they all can fall back to rank 0 nodes - which is scout (outside ufo)
	Node* node;
	if (outside == true)
		node = _battleSave->getSpawnNode( // Civ-Scout spawnpoints <- 'outside'
									NR_SCOUT,
									unit);
	else
		node = nullptr;

	const size_t ranks_2 = (sizeof(Node::nodeRank) / sizeof(Node::nodeRank[0][0]))
						 / (sizeof(Node::nodeRank) / sizeof(Node::nodeRank[0]));
	if (node == nullptr) // ie. if not spawning on a Civ-Scout node
	{
		for (size_t
				i = 0;
				i != ranks_2 // =8, 2nd dimension of nodeRank[][], ref Node.cpp
					&& node == nullptr;
				++i)
		{
			node = _battleSave->getSpawnNode(
										Node::nodeRank[aLienRank][i],
										unit);
		}
	}

	if ((node != nullptr
			&& _battleSave->setUnitPosition(
										unit,
										node->getPosition()) == true)
		|| placeUnitNearFriend(unit) == true)
	{
		unit->setAIState(new AlienBAIState(
										_battleSave,
										unit,
										node));
		unit->setRankInt(aLienRank);

		Position posCraft = _battleSave->getUnits()->at(0)->getPosition(); // aLiens face Craft
		int dir;
		if (RNG::percent((diff + 1) * 20) == true
			&& TileEngine::distance(
								node->getPosition(),
								posCraft) < 25)
		{
			dir = TileEngine::getDirectionTo(unit->getPosition(), posCraft);
		}
		else
			dir = _battleSave->getTileEngine()->faceWindow(node->getPosition());

		if (dir == -1)
			dir = RNG::generate(0,7);
		unit->setUnitDirection(dir);

		int tu = unit->getTimeUnits();
		tu = static_cast<int>(ceil(
			 static_cast<double>(tu) * RNG::generate(0.,1.)));
		unit->setTimeUnits(tu);

		_battleSave->getUnits()->push_back(unit);
	}
	else
	{
		Log(LOG_WARNING) << "BattlescapeGenerator could not add ["
						 << unit->getType() << "]";
		--_unitSequence;
		delete unit;
		return nullptr;
	}

	return unit;
}

/**
 * Places a unit near a friendly unit.
 * @param unit - pointer to the BattleUnit in question
 * @return, true if @a unit was successfully placed
 */
bool BattlescapeGenerator::placeUnitNearFriend(BattleUnit* const unit) // private.
{
	if (unit != nullptr
		&& _battleSave->getUnits()->empty() == false)
	{
		const BattleUnit* friendUnit;
		Position posEntry;
		int tries;
		bool isLarge;

		for (int
				i = 0;
				i != 10;
				++i)
		{
			posEntry = Position(-1,-1,-1);
			tries = 100;
			isLarge = false;

			while (posEntry == Position(-1,-1,-1)
				&& tries != 0)
			{
				friendUnit = _battleSave->getUnits()->at(RNG::pick(_battleSave->getUnits()->size()));
				if (friendUnit->getFaction() == unit->getFaction()
					&& friendUnit->getPosition() != Position(-1,-1,-1)
					&& friendUnit->getArmor()->getSize() >= unit->getArmor()->getSize())
				{
					posEntry = friendUnit->getPosition();
					isLarge = (friendUnit->getArmor()->getSize() == 2);
				}

				--tries;
			}

			if (tries != 0
				&& _battleSave->placeUnitNearPosition(unit, posEntry, isLarge) == true)
			{
				return true;
			}
		}
	}

	return false;
}

/**
 * Spawns civilians on a terror mission.
 * @param civilians - maximum number of civilians to spawn
 */
void BattlescapeGenerator::deployCivilians(int civilians) // private.
{
	if (civilians != 0)
	{
		const int qty = std::max(
							1,
							RNG::generate(
										civilians / 2,
										civilians));
		for (int
				i = 0;
				i != qty;
				++i)
		{
			addCivilian(_rules->getUnit(_terrainRule->getCivilianTypes().at(RNG::pick(_terrainRule->getCivilianTypes().size()))));
		}
	}
}

/**
 * Adds a civilian to the game and places him on a free spawnpoint.
 * @param unitRule - pointer to the Unit rule that holds info about civilians
 */
void BattlescapeGenerator::addCivilian(RuleUnit* const unitRule) // private.
{
	BattleUnit* const unit = new BattleUnit(
										unitRule,
										FACTION_NEUTRAL,
										_unitSequence++,
										_rules->getArmor(unitRule->getArmor()),
										DIFF_BEGINNER); // <- do not upgrade civilians

	Node* const node = _battleSave->getSpawnNode(NR_SCOUT, unit);
	if ((node != nullptr
			&& _battleSave->setUnitPosition(unit, node->getPosition()) == true)
		|| placeUnitNearFriend(unit) == true)
	{
		unit->setAIState(new CivilianBAIState(_battleSave, unit, node));
		unit->setUnitDirection(RNG::generate(0,7));

		_battleSave->getUnits()->push_back(unit);
	}
	else
	{
		Log(LOG_WARNING) << "BattlescapeGenerator could not add: " << unit->getType();
		--_unitSequence;
		delete unit;
	}
}

/**
 * Loads a MAP file into the tiles of the BattleGame.
 * @param block				- pointer to MapBlock
 * @param offset_x			- Mapblock offset in X direction
 * @param offset_y			- Mapblock offset in Y direction
 * @param terraRule			- pointer to RuleTerrain
 * @param dataSetIdOffset	- (default 0)
 * @param discovered		- true if this MapBlock is discovered (eg. landingsite of the Skyranger) (default false)
 * @param craft				- true if xCom Craft has landed on the MAP (default false)
 * @return, height of the loaded Mapblock (needed for spawnpoint calculation)
 * @sa http://www.ufopaedia.org/index.php?title=MAPS
 * @note Y-axis is in reverse order.
 */
int BattlescapeGenerator::loadMAP( // private.
		MapBlock* const block,
		int offset_x,
		int offset_y,
		const RuleTerrain* const terraRule,
		int dataSetIdOffset,
		bool discovered,
		bool craft)
{
	std::ostringstream file;
	file << "MAPS/" << block->getType() << ".MAP";
	std::ifstream mapFile ( // Load file
						CrossPlatform::getDataFile(file.str()).c_str(),
						std::ios::in | std::ios::binary);
	if (mapFile.fail() == true)
	{
		throw Exception(file.str() + " not found");
	}

	char array_Map[3];
	mapFile.read(
			(char*)&array_Map,
			sizeof(array_Map));
	const int
		size_x = static_cast<int>(array_Map[1]), // note X-Y switch!
		size_y = static_cast<int>(array_Map[0]), // note X-Y switch!
		size_z = static_cast<int>(array_Map[2]);

	block->setSizeZ(size_z);

	std::ostringstream oststr;
	if (size_z > _battleSave->getMapSizeZ())
	{
		oststr << "Height of map " + file.str() + " too big for this mission, block is " << size_z << ", expected: " << _battleSave->getMapSizeZ();
		throw Exception(oststr.str());
	}
	else if (size_x != block->getSizeX()
		|| size_y != block->getSizeY())
	{
		oststr << "Map block is not of the size specified " + file.str() + " is " << size_x << "x" << size_y << " , expected: " << block->getSizeX() << "x" << block->getSizeY();
		throw Exception(oststr.str());
	}

	int
		x = offset_x,
		y = offset_y,
		z = size_z - 1;

	for (int
			i = _mapsize_z - 1;
			i != 0;
			--i)
	{
		// check if there is already a layer - if so, move Z up
		if (_battleSave->getTile(Position(x,y,i))->getMapData(O_FLOOR) != nullptr)
		{
			z += i;

			if (craft == true)
			{
				_craftZ = i;
				_battleSave->setGroundLevel(i);
			}

			break;
		}
	}

	if (z > _battleSave->getMapSizeZ() - 1)
	{
		throw Exception("Something is wrong in the map definitions, craft/ufo map is too tall.");
	}

	unsigned char array_Parts[Tile::PARTS_TILE];
	unsigned partId;

	bool revealDone;
	while (mapFile.read(
					(char*)&array_Parts,
					sizeof(array_Parts)))
	{
		revealDone = false;

		for (size_t
				partType = 0;
				partType != Tile::PARTS_TILE;
				++partType)
		{
			partId = static_cast<unsigned>(array_Parts[partType]);

			// Remove natural terrain that is inside Craft or Ufo.
			if (partType != 0				// not if it's a floor since Craft/Ufo part will overwrite it anyway
				&& partId == 0			// and only if no Craft/Ufo part would overwrite the part
				&& array_Parts[0] != 0)	// but only if there *is* a floor-part to the Craft/Ufo so it would (have) be(en) inside the Craft/Ufo
			{
				_battleSave->getTile(Position(x,y,z))->setMapData(nullptr,-1,-1, static_cast<MapDataType>(partType));
			}

			// Then overwrite previous terrain with Craft or Ufo terrain.
			// nb. See sequence of map-loading in generateMap() (1st terrain, 2nd Ufo, 3rd Craft) <- preMapScripting.
			if (partId > 0)
			{
				unsigned int dataId = partId;
				int dataSetId = dataSetIdOffset;

				MapData* const data = terraRule->getMapData(&dataId, &dataSetId);
//				if (dataSetIdOffset > 0) // ie: ufo or craft.
//					_battleSave->getTile(Position(x,y,z))->setMapData(nullptr,-1,-1,3); // erase content-object

				_battleSave->getTile(Position(x,y,z))->setMapData(
																data,
																static_cast<int>(dataId),
																dataSetId,
																static_cast<MapDataType>(partType));
			}

/*			// If the part is not a floor and is empty, remove it; this prevents growing grass in UFOs.
			// note: And outside UFOs. so remark it
			if (part == 3 && partId == 0)
			{
				_battleSave->getTile(Position(x,y,z))->setMapData(nullptr,-1,-1, part);
			} */

			if (craft == true // Reveal only tiles inside the Craft.
				&& partId != 0
				&& z != _craftZ)
			{
				revealDone = true;
				_battleSave->getTile(Position(x,y,z))->setDiscovered(true, 2);
			}
		}

/*		if (craft && _craftZ == z)
		{
			for (int z2 = _battleSave->getMapSizeZ() - 1; z2 >= _craftZ; --z2)
				_battleSave->getTile(Position(x,y,z2))->setDiscovered(true, 2);
		} */

		if (revealDone == false)
			_battleSave->getTile(Position(x,y,z))->setDiscovered(
															discovered == true || block->isFloorRevealed(z) == true,
															2);
		++x;

		if (x == size_x + offset_x)
		{
			x = offset_x;
			++y;
		}

		if (y == size_y + offset_y)
		{
			y = offset_y;
			--z;
		}
	}

	if (mapFile.eof() == false)
	{
		throw Exception("Invalid MAP file: " + file.str());
	}

	mapFile.close();

	if (_generateFuel == true) // if one of the mapBlocks has an items array defined, don't deploy fuel algorithmically
		_generateFuel = (block->getItems()->empty() == true);

	for (std::map<std::string, std::vector<Position> >::const_iterator
			i = block->getItems()->begin();
			i != block->getItems()->end();
			++i)
	{
		RuleItem* const itRule = _rules->getItem((*i).first);
		for (std::vector<Position>::const_iterator
				j = (*i).second.begin();
				j != (*i).second.end();
				++j)
		{
			BattleItem* const item = new BattleItem(
												itRule,
												_battleSave->getNextItemId());
			_battleSave->getItems()->push_back(item);
			_battleSave->getTile((*j) + Position(
												offset_x,
												offset_y,
												0))
											->addItem(
													item,
													_rules->getInventory("STR_GROUND"));
		}
	}

	return size_z;
}

/**
 * Loads an XCom format RMP file as battlefield Nodes.
 * @param block		- pointer to MapBlock
 * @param offset_x	- Mapblock offset in X direction
 * @param offset_y	- Mapblock offset in Y direction
 * @param segment	- Mapblock segment
 * @sa http://www.ufopaedia.org/index.php?title=ROUTES
 */
void BattlescapeGenerator::loadRMP( // private.
		MapBlock* const block,
		int offset_x,
		int offset_y,
		int segment)
{
	char dataArray[24];

	std::ostringstream file;
	file << "ROUTES/" << block->getType() << ".RMP";

	std::ifstream mapFile ( // init. Load file
					CrossPlatform::getDataFile(file.str()).c_str(),
					std::ios::in | std::ios::binary);
	if (mapFile.fail() == true)
	{
		throw Exception(file.str() + " not found");
	}

	int
		pos_x,
		pos_y,
		pos_z,

		unitType,
		nodeRank,
		ptrlPriority,
		aLienObject,
		spPriority,

		linkId;

	const int nodeOffset = static_cast<int>(_battleSave->getNodes()->size());
	Node* node;
	Position pos;

	while (mapFile.read(
					(char*)&dataArray,
					sizeof(dataArray)))
	{
		pos_x = static_cast<int>(dataArray[1]); // note: Here is where x-y values get reversed
		pos_y = static_cast<int>(dataArray[0]); // vis-a-vis values in .RMP files vs. loaded values.
		pos_z = static_cast<int>(dataArray[2]);

		if (pos_x < block->getSizeX()
			&& pos_y < block->getSizeY()
			&& pos_z < _mapsize_z)
		{
			pos = Position(
						offset_x + pos_x,
						offset_y + pos_y,
						block->getSizeZ() - pos_z - 1);

			unitType		= static_cast<int>(dataArray[19]); // -> Any=0; Flying=1; Small=2; FlyingLarge=3; Large=4
			nodeRank		= static_cast<int>(dataArray[20]);
			ptrlPriority	= static_cast<int>(dataArray[21]);
			aLienObject		= static_cast<int>(dataArray[22]);
			spPriority		= static_cast<int>(dataArray[23]);

			// TYPE_FLYING		= 0x01 -> ref Savegame/Node.h
			// TYPE_SMALL		= 0x02
			// TYPE_LARGEFLYING	= 0x04
			// TYPE_LARGE		= 0x08
			// TYPE_DANGEROUS	= 0x10 <- not in RMP file.
			if		(unitType == 3) unitType = 4; // for bit-wise
			else if (unitType == 4) unitType = 8;

			if (_battleSave->getTacType() != TCT_BASEDEFENSE)
				aLienObject = 0; // ensure these get zero'd for nonBaseDefense battles; cf. Node::isTarget()

			node = new Node(
						_battleSave->getNodes()->size(),
						pos,
						segment,
						unitType,
						nodeRank,
						ptrlPriority,
						aLienObject,
						spPriority);

			for (size_t // create nodeLinks ->
					j = 0;
					j != 5;
					++j)
			{
				linkId = static_cast<int>(dataArray[(j * 3) + 4]); // <- 4[5,6],7[8,9],10[11,12],13[14,15],16[17,18] -> [unitType & distance of linked nodes are not used]

				if (linkId < 251) // do not offset special values; ie. links to N,S,E,West, or none.
					linkId += nodeOffset;
				else
					linkId -= 256;	// 255 -> -1 = unused
									// 254 -> -2 = north
									// 253 -> -3 = east
									// 252 -> -4 = south
									// 251 -> -5 = west

				std::vector<int>* nodeLinks = node->getNodeLinks();
				if (std::find(
							nodeLinks->rbegin(),
							nodeLinks->rend(),
							linkId) != nodeLinks->rend())
				{
					linkId = -1; // prevent multiple identical links on nodes.
				}

				nodeLinks->push_back(linkId);
			}

			_battleSave->getNodes()->push_back(node);
		}
	}

	if (mapFile.eof() == false)
	{
		throw Exception("Invalid RMP file");
	}

	mapFile.close();
}

/**
 * Fill power sources with an alien fuel object.
 */
void BattlescapeGenerator::fuelPowerSources() // private.
{
	for (size_t
			i = 0;
			i != _battleSave->getMapSizeXYZ();
			++i)
	{
		if (_battleSave->getTiles()[i]->getMapData(O_OBJECT)
			&& _battleSave->getTiles()[i]->getMapData(O_OBJECT)->getSpecialType() == UFO_POWER_SOURCE)
		{
			BattleItem* const alienFuel = new BattleItem(
													_rules->getItem(_rules->getAlienFuelType()),
													_battleSave->getNextItemId());

			_battleSave->getItems()->push_back(alienFuel);
			_battleSave->getTiles()[i]->addItem(
											alienFuel,
											_rules->getInventory("STR_GROUND"));
		}
	}
}

/**
 * When a UFO crashes there is a chance for each powersource to explode.
 */
void BattlescapeGenerator::explodePowerSources() // private.
{
	Position voxel;

	for (size_t
			i = 0;
			i != _battleSave->getMapSizeXYZ();
			++i)
	{
		if (_battleSave->getTiles()[i]->getMapData(O_OBJECT) != nullptr
			&& _battleSave->getTiles()[i]->getMapData(O_OBJECT)->getSpecialType() == UFO_POWER_SOURCE
			&& RNG::percent(80) == true)
		{
			voxel = Position::toVoxelSpaceCentered(
												_battleSave->getTiles()[i]->getPosition(),
												10);

			double power = static_cast<double>(_ufo->getUfoDamagePct());	// range: ~50+ to ~100-
			if (RNG::percent(static_cast<int>(power) / 2) == true)			// chance for full range Explosion (even if crash took low damage)
				power = RNG::generate(1.,100.);

			power *= RNG::generate(0.1,2.);
			power += std::pow(power, 2) / 160.;

			if (power > 0.5)
				_battleSave->getTileEngine()->explode(
													voxel,
													static_cast<int>(std::ceil(power)),
													DT_HE,
													21);
		}
	}

	const Tile* tile = _battleSave->getTileEngine()->checkForTerrainExplosions();
	while (tile != nullptr)
	{
		voxel = Position::toVoxelSpaceCentered(tile->getPosition(), 10);
		_battleSave->getTileEngine()->explode(
											voxel,
											tile->getExplosive(),
											DT_HE,
											tile->getExplosive() / 10);

		tile = _battleSave->getTileEngine()->checkForTerrainExplosions();
	}
}

/**
 * Creates a mini-battle-save for managing inventory from the Geoscape's CraftEquip or BaseEquip screen.
 * Kids, don't try this at home! yer tellin' me.
 * @param craft		- pointer to Craft to handle
 * @param base		- pointer to Base to handle (default nullptr)
 * @param selUnit	- soldier to display in battle pre-equip inventory (default 0)
 */
void BattlescapeGenerator::runInventory(
		Craft* craft,
		Base* base,
		size_t selUnit)
{
	_baseEquipScreen = true;

	int qtySoldiers;
	if (craft != nullptr)
		qtySoldiers = craft->getNumSoldiers();
	else
		qtySoldiers = base->getAvailableSoldiers(true);

	_mapsize_x = qtySoldiers; // fake a map for soldier placement
	_mapsize_y =
	_mapsize_z = 1;

	_battleSave->initMap(
					_mapsize_x,
					_mapsize_y,
					_mapsize_z);

	MapDataSet* const dataSet = new MapDataSet("blank", _game);
	MapData* const data = new MapData(dataSet);

	for (size_t
			i = 0;
			i != static_cast<size_t>(qtySoldiers);
			++i)
	{
		Tile* const tile = _battleSave->getTiles()[i];
		tile->setMapData(
					data,
					0,0,
					O_FLOOR);
		tile->getMapData(O_FLOOR)->setSpecialType(START_POINT);
		tile->getMapData(O_FLOOR)->setTUWalk(0);
		tile->getMapData(O_FLOOR)->setFlags(
										false,
										false,
										false,
										0,
										false,
										false,
										false,
										false,
										false);
	}

	if (craft != nullptr)
		setCraft(craft); // generate the battleitems for inventory
	else
		setBase(base);

	deployXCOM();

	if (craft != nullptr
		&& selUnit != 0
		&& static_cast<int>(selUnit) <= qtySoldiers)
	{
		size_t j = 0;
		for (std::vector<BattleUnit*>::const_iterator
				i = _battleSave->getUnits()->begin();
				i != _battleSave->getUnits()->end();
				++i)
		{
			if (++j == selUnit)
			{
				_battleSave->setSelectedUnit(*i);
				break;
			}
		}
	}

	delete data;
	delete dataSet;
}

/**
 * Generates a map of modules (sets of tiles) for a new Battlescape game.
 * @param script - the scripts to use to build the map
 */
void BattlescapeGenerator::generateMap(const std::vector<MapScript*>* const script) // private.
{
	//Log(LOG_INFO) << "generateMap, terraRule = " << _terrainRule->getType() << " script = " << _terrainRule->getScript();
	// set up map generation vars
	_testBlock = new MapBlock("testBlock");

	init();

	MapBlock* craftBlock = nullptr;
	std::vector<MapBlock*> ufoBlocks;

	int
		blockDataSetIdOffset = 0,
		craftDataSetIdOffset = 0;

	// create an array to track command success/failure
	std::map<int, bool> conditions;

	for (std::vector<MapDataSet*>::const_iterator
			i = _terrainRule->getMapDataSets()->begin();
			i != _terrainRule->getMapDataSets()->end();
			++i)
	{
		(*i)->loadData();

		if (_rules->getMCDPatch((*i)->getName()) != nullptr)
			_rules->getMCDPatch((*i)->getName())->modifyData(*i);

		_battleSave->getMapDataSets()->push_back(*i);
		++blockDataSetIdOffset;
	}

	// generate the map now and store it inside the tile objects
	RuleTerrain* ufoTerrain = nullptr;

	// this mission type is "hard-coded" in terms of map layout
	if (_battleSave->getTacType() == TCT_BASEDEFENSE)
		generateBaseMap();

	bool success;

	// process script
	for (std::vector<MapScript*>::const_iterator
			i = script->begin();
			i != script->end();
			++i)
	{
		//Log(LOG_INFO) << "do script Command type = " << (int)(*i)->getType();
		if ((*i)->getLabel() != 0
			&& conditions.find((*i)->getLabel()) != conditions.end())
		{
			throw Exception("Map generator encountered an error: multiple commands are sharing the same label.");
		}

		success = conditions[static_cast<size_t>((*i)->getLabel())] = false;


		// if this command runs conditionally on the failures or successes of previous commands
		if ((*i)->getConditions()->empty() == false)
		{
			bool go = true;

			// compare the corresponding entries in the success/failure vector
			for (std::vector<int>::const_iterator
					j = (*i)->getConditions()->begin();
					j != (*i)->getConditions()->end();
					++j)
			{
				// positive numbers indicate conditional on success, negative means conditional on failure
				// ie: [1, -2] means this command only runs if command 1 succeeded and command 2 failed.
				if (conditions.find(std::abs(*j)) != conditions.end())
				{
					if ((*j > 0 && conditions[*j] == false)
						|| (*j < 0 && conditions[std::abs(*j)] == true))
					{
						go = false;
						break;
					}
				}
				else
				{
					throw Exception("Map generator encountered an error: conditional command expected a label that did not exist before this command.");
				}
			}

			if (go == false)
				continue;
		}

		// if there's a chance a command won't execute by design take that into account here
		if (RNG::percent((*i)->chanceOfExecution()) == true)
		{
			//Log(LOG_INFO) << " execution TRUE";
			// initialize the block selection arrays
			(*i)->init();

			// each command can be attempted multiple times since randomization within the rects may occur
			for (int
					j = 0;
					j != (*i)->getExecutions();
					++j)
			{
				MapBlock* block = nullptr;
				int
					x,y;

				switch ((*i)->getType())
				{
					case MSC_ADDBLOCK:
						block = (*i)->getNextBlock(_terrainRule);
						// select an X and Y position from within the rects, using an even distribution
						if (block != nullptr
							&& selectPosition(
											(*i)->getRects(),
											x,y,
											block->getSizeX(),
											block->getSizeY()) == true)
						{
							success = addBlock(x,y, block) == true
								   || success;
						}
					break;

					case MSC_ADDLINE:
						success = addLine(
										static_cast<MapDirection>((*i)->getDirection()),
										(*i)->getRects()) == true;
					break;

					case MSC_ADDCRAFT:
						//Log(LOG_INFO) << "MSC_ADDCRAFT ->";
						if (_craft != nullptr)
						{
							craftBlock = _craft->getRules()->getBattlescapeTerrainData()->getMapBlockRand(999, 999, 0, false);
							//Log(LOG_INFO) << ". craftBlock = " << craftBlock->getType();
							if (addCraft(craftBlock, *i, _craftPos) == true)
							{
								// by default addCraft adds blocks from group 1.
								// this can be overwritten in the command by defining specific groups or blocks
								// or this behaviour can be suppressed by leaving group 1 empty
								// this is intentional to allow for TFTD's cruise liners/etc
								// in this situation, you can end up with ANYTHING under your craft, so be careful
								for (
										x = static_cast<int>(_craftPos.x);
										x != static_cast<int>(_craftPos.x) + static_cast<int>(_craftPos.w);
										++x)
								{
									for (
											y = static_cast<int>(_craftPos.y);
											y != static_cast<int>(_craftPos.y) + static_cast<int>(_craftPos.h);
											++y)
									{
										if (_blocks[x][y] != nullptr)
											loadMAP(
												_blocks[x][y],
												x * 10, y * 10,
												_terrainRule);
									}
								}

								success = _craftDeployed = true;
							}
						}
					break;

					case MSC_ADDUFO:
						// as above, note that the craft and the ufo will never be allowed to overlap.
						// significant difference here is that we accept a UFOName string here to choose the UFO map
						// and we store the UFO positions in a vector, which we iterate later when actually loading the
						// map and route data. this makes it possible to add multiple UFOs to a single map
						// IMPORTANTLY: all the UFOs must use _exactly_ the same MCD set.
						// this is fine for most UFOs but it does mean small scouts can't be combined with larger ones
						// unless some major alterations are done to the MCD sets and maps themselves beforehand
						// this is because serializing all the MCDs is an implementational nightmare from my perspective,
						// and modders can take care of all that manually on their end. - Warboy opus
						//Log(LOG_INFO) << "MSC_ADDUFO ->";
						if (_rules->getUfo((*i)->getUfoType()) != nullptr)
							ufoTerrain = _rules->getUfo((*i)->getUfoType())->getBattlescapeTerrainData();
						else if (_ufo != nullptr)
							ufoTerrain = _ufo->getRules()->getBattlescapeTerrainData();

						if (ufoTerrain != nullptr)
						{
							MapBlock* const ufoBlock = ufoTerrain->getMapBlockRand(999, 999, 0, false);
							//Log(LOG_INFO) << ". ufoBlock = " << ufoBlock->getType();

							SDL_Rect ufoPosTest;
							if (addCraft(ufoBlock, *i, ufoPosTest) == true)
							{
								_ufoPos.push_back(ufoPosTest);
								ufoBlocks.push_back(ufoBlock);

								for (
										x = static_cast<int>(ufoPosTest.x);
										x != static_cast<int>(ufoPosTest.x) + static_cast<int>(ufoPosTest.w);
										++x)
								{
									for (
											y = static_cast<int>(ufoPosTest.y);
											y != static_cast<int>(ufoPosTest.y) + static_cast<int>(ufoPosTest.h);
											++y)
									{
										if (_blocks[x][y])
											loadMAP(
												_blocks[x][y],
												x * 10, y * 10,
												_terrainRule);
									}
								}

								success = true;
							}
						}
					break;

					case MSC_DIGTUNNEL:
						drillModules(
									(*i)->getTunnelData(),
									(*i)->getRects(),
									(*i)->getDirection());
						success = true; // this command is fail-proof
					break;

					case MSC_FILLAREA:
						//Log(LOG_INFO) << "MSC_FILLAREA ->";
						block = (*i)->getNextBlock(_terrainRule);
						//Log(LOG_INFO) << ". block = " << block->getType();
						while (block != nullptr)
						{
							//Log(LOG_INFO) << ". . iter";
							if (selectPosition(
											(*i)->getRects(),
											x,y,
											block->getSizeX(),
											block->getSizeY()) == true)
							{
								//Log(LOG_INFO) << ". . . pos selected";
								// fill area will succeed if even one block is added
								success = addBlock(x,y, block) == true
									   || success;
							}
							else
							{
								//Log(LOG_INFO) << ". . . pos NOT selected";
								break;
							}

							block = (*i)->getNextBlock(_terrainRule);
							//Log(LOG_INFO) << ". . block = " << block->getType();
						}
					break;

					case MSC_CHECKBLOCK:
						for (std::vector<SDL_Rect*>::const_iterator
								k = (*i)->getRects()->begin();
								k != (*i)->getRects()->end()
									&& success == false;
								++k)
						{
							for (
									x = (*k)->x;
									x != (*k)->x + (*k)->w && x != _mapsize_x / 10 && success == false;
									++x)
							{
								for (
										y = (*k)->y;
										y != (*k)->y + (*k)->h && y != _mapsize_y / 10 && success == false;
										++y)
								{
									if ((*i)->getGroups()->empty() == false)
									{
										for (std::vector<int>::const_iterator
												z = (*i)->getGroups()->begin();
												z != (*i)->getGroups()->end() && success == false;
												++z)
										{
											success = (_blocks[x][y]->isInGroup(*z) == true);
										}
									}
									else if ((*i)->getBlocks()->empty() == false)
									{
										for (std::vector<int>::const_iterator
												z = (*i)->getBlocks()->begin();
												z != (*i)->getBlocks()->end() && success == false;
												++z)
										{
											if (*z < static_cast<int>(_terrainRule->getMapBlocks()->size()))
												success = (_blocks[x][y] == _terrainRule->getMapBlocks()->at(*z));
										}
									}
									else // wildcard, don't care what block it is, just wanna know if there's a block here
										success = (_blocks[x][y] != nullptr);
								}
							}
						}
					break;

					case MSC_REMOVE:
						success = removeBlocks(*i);
					break;

					case MSC_RESIZE:
						if (_battleSave->getTacType() == TCT_BASEDEFENSE)
						{
							throw Exception("Map Generator encountered an error: Base defense map cannot be resized.");
						}

						if (_blocksLeft < (_mapsize_x / 10) * (_mapsize_y / 10))
						{
							throw Exception("Map Generator encountered an error: The map cannot be resized after adding blocks.");
						}

						if ((*i)->getSizeX() > 0 && (*i)->getSizeX() != _mapsize_x / 10)
							_mapsize_x = (*i)->getSizeX() * 10;

						if ((*i)->getSizeY() > 0 && (*i)->getSizeY() != _mapsize_y / 10)
							_mapsize_y = (*i)->getSizeY() * 10;

						if ((*i)->getSizeZ() > 0 && (*i)->getSizeZ() != _mapsize_z)
							_mapsize_z = (*i)->getSizeZ();

						init();
				}
			}
		}
	}
	//Log(LOG_INFO) << ". . done Commands";

	if (_blocksLeft != 0)
	{
		throw Exception("Map failed to fully generate.");
	}

	loadNodes();

	if (_battleSave->getTacType() == TCT_BASEASSAULT)
		placeXComProperty();


	if (ufoTerrain != nullptr && ufoBlocks.empty() == false)
	{
		for (std::vector<MapDataSet*>::const_iterator
				i = ufoTerrain->getMapDataSets()->begin();
				i != ufoTerrain->getMapDataSets()->end();
				++i)
		{
			(*i)->loadData();

			if (_rules->getMCDPatch((*i)->getName()))
				_rules->getMCDPatch((*i)->getName())->modifyData(*i);

			_battleSave->getMapDataSets()->push_back(*i);
			++craftDataSetIdOffset;
		}

		for (size_t
				i = 0;
				i != ufoBlocks.size();
				++i)
		{
			loadMAP(
				ufoBlocks[i],
				static_cast<int>(_ufoPos[i].x) * 10, static_cast<int>(_ufoPos[i].y) * 10,
				ufoTerrain,
				blockDataSetIdOffset);
			loadRMP(
				ufoBlocks[i],
				static_cast<int>(_ufoPos[i].x) * 10, static_cast<int>(_ufoPos[i].y) * 10,
				Node::SEG_UFO);

			for (int
					j = 0;
					j != ufoBlocks[i]->getSizeX() / 10;
					++j)
			{
				for (int
						k = 0;
						k != ufoBlocks[i]->getSizeY() / 10;
						++k)
				{
					_segments[static_cast<size_t>(_ufoPos[i].x) + static_cast<size_t>(j)]
							 [static_cast<size_t>(_ufoPos[i].y) + static_cast<size_t>(k)] = Node::SEG_UFO;
				}
			}
		}
	}

	if (craftBlock != nullptr)
	{
		for (std::vector<MapDataSet*>::const_iterator
				i = _craft->getRules()->getBattlescapeTerrainData()->getMapDataSets()->begin();
				i != _craft->getRules()->getBattlescapeTerrainData()->getMapDataSets()->end();
				++i)
		{
			(*i)->loadData();

			if (_rules->getMCDPatch((*i)->getName()))
				_rules->getMCDPatch((*i)->getName())->modifyData(*i);

			_battleSave->getMapDataSets()->push_back(*i);
		}

		loadMAP(
			craftBlock,
			static_cast<int>(_craftPos.x * 10),
			static_cast<int>(_craftPos.y * 10),
			_craft->getRules()->getBattlescapeTerrainData(),
			blockDataSetIdOffset + craftDataSetIdOffset,
			false, // was true
			true);
		loadRMP(
			craftBlock,
			static_cast<int>(_craftPos.x * 10),
			static_cast<int>(_craftPos.y * 10),
			Node::SEG_CRAFT);

		for (int
				i = 0;
				i != craftBlock->getSizeX() / 10;
				++i)
		{
			for (int
					j = 0;
					j != craftBlock->getSizeY() / 10;
					++j)
			{
				_segments[static_cast<size_t>(_craftPos.x) + static_cast<size_t>(i)]
						 [static_cast<size_t>(_craftPos.y) + static_cast<size_t>(j)] = Node::SEG_CRAFT;
			}
		}

/*		for (int
				i = _craftPos.x * 10 - 1;
				i <= _craftPos.x * 10 + craftBlock->getSizeX();
				++i)
		{
			for (int
					j = _craftPos.y * 10 - 1;
					j <= _craftPos.y * 10 + craftBlock->getSizeY();
					++j)
			{
				for (int
						k = _mapsize_z - 1;
						k >= _craftZ;
						--k)
				{
					if (_battleSave->getTile(Position(i,j,k)))
						_battleSave->getTile(Position(i,j,k))->setDiscovered(true, 2);
				}
			}
		} */
	}

	delete _testBlock;

	Tile* tile; // safety that ensures there's always a floor-tile on level 0
	for (int
			x = 0;
			x != _mapsize_x;
			++x)
	{
		for (int
				y = 0;
				y != _mapsize_y;
				++y)
		{
			tile = _battleSave->getTile(Position(x,y,0));
			if (tile->getMapData(O_FLOOR) == nullptr)
				tile->setMapData(
							MapDataSet::getScorchedEarthTile(),
							1,0,
							O_FLOOR);
		}
	}

	attachNodeLinks();
}

/**
 * Generates a map based on the base's layout.
 * @note This doesn't drill or fill with dirt - the script must do that.
 */
void BattlescapeGenerator::generateBaseMap() // private.
{
	// add modules based on the base's layout
	for (std::vector<BaseFacility*>::const_iterator
			i = _base->getFacilities()->begin();
			i != _base->getFacilities()->end();
			++i)
	{
		if ((*i)->buildFinished() == true)
		{
			const int
				xLimit = (*i)->getX() + (*i)->getRules()->getSize() - 1,
				yLimit = (*i)->getY() + (*i)->getRules()->getSize() - 1;
			int num = 0;

			for (int
					y = (*i)->getY();
					y <= yLimit;
					++y)
			{
				for (int
						x = (*i)->getX();
						x <= xLimit;
						++x)
				{
					// lots of crazy stuff here, which is for the hangars (or other large base facilities one may create)
					// TODO: clean this mess up, make the mapNames a vector in the base module defs
					// also figure out how to do the terrain sets on a per-block basis.
					const std::string mapname = (*i)->getRules()->getMapName();
					std::ostringstream newname;
					newname << mapname.substr(
											0,
											mapname.size() - 2); // strip off last 2 digits

					int mapnum = std::atoi(mapname.substr(
														mapname.size() - 2,
														2).c_str()); // get number
					mapnum += num;
					if (mapnum < 10)
						newname << 0;
					newname << mapnum;

					addBlock(
						x,y,
						_terrainRule->getMapBlock(newname.str()));

					_drillMap[x][y] = MD_NONE;
					++num;

					// general stores - there is where the items are put
					if ((*i)->getRules()->getStorage() > 0) // <- hmm Raises questions about buying and transfering Craft ....
					{
						int groundLevel;
						for (
								groundLevel = _mapsize_z - 1;
								groundLevel > -1;
								--groundLevel)
						{
							if (_battleSave->getTile(Position(
															x * 10,
															y * 10,
															groundLevel))->hasNoFloor() == false)
							{
								break;
							}
						}
						if (groundLevel < 0)
							groundLevel = 0; // safety.

						for (int
								k = x * 10;
								k != (x + 1) * 10;
								++k)
						{
							for (int
									l = y * 10;
									l != (y + 1) * 10;
									++l)
							{
								// use only every other tile, giving a "checkerboard" pattern
								if ((k + l) % 2 == 0)
								{
									Tile
										* const tile = _battleSave->getTile(Position(
																				k,
																				l,
																				groundLevel)),
										* const tileEast = _battleSave->getTile(Position(
																				k + 1,
																				l,
																				groundLevel)),
										* const tileSouth = _battleSave->getTile(Position(
																				k,
																				l + 1,
																				groundLevel));

									if (tile != nullptr
										&& tile->getMapData(O_FLOOR) != nullptr
										&& tile->getMapData(O_OBJECT) == nullptr
										&& tileEast != nullptr
										&& tileEast->getMapData(O_WESTWALL) == nullptr
										&& tileSouth != nullptr
										&& tileSouth->getMapData(O_NORTHWALL) == nullptr)
									{
										_battleSave->getStorageSpace().push_back(Position(k,l, groundLevel));
									}
								}
							}
						}

						// put the inventory tile on the lowest floor, jic.
						if (_tileEquipt == nullptr)
							_tileEquipt = _battleSave->getTile(Position(
																	x * 10 + 5,
																	y * 10 + 5,
																	std::max(
																		0,
																		groundLevel - 1)));
					}
				}
			}

			for (int
					x = (*i)->getX();
					x <= xLimit;
					++x)
			{
				_drillMap[x][yLimit] = MD_VERTICAL;
			}

			for (int
					y = (*i)->getY();
					y <= yLimit;
					++y)
			{
				_drillMap[xLimit][y] = MD_HORIZONTAL;
			}

			_drillMap[xLimit][yLimit] = MD_BOTH;
		}
	}

	_battleSave->calculateModuleMap();
}

/**
 * Finds Alien Base start modules and iterates possible positions for Xcom's
 * starting equipment.
 */
void BattlescapeGenerator::placeXComProperty() // private.
{
	const size_t
		xSize = static_cast<size_t>(_mapsize_x / 10),
		ySize = static_cast<size_t>(_mapsize_y / 10);

	for (size_t
			y = 0;
			y != ySize;
			++y)
	{
		for (size_t
				x = 0;
				x != xSize;
				++x)
		{
			if (_blocks[x][y]->isInGroup(MBT_START))
				_battleSave->getStorageSpace().push_back(Position(
																x * 10 + 5,
																y * 10 + 5,
																1));
		}
	}
}

/**
 * Clears a module from the map.
 * @param x		- the x offset
 * @param y		- the y offset
 * @param sizeX	- how far along the x axis to clear
 * @param sizeY	- how far along the y axis to clear
 */
void BattlescapeGenerator::clearModule( // private.
		int x,
		int y,
		int sizeX,
		int sizeY)
{
	for (int
			z = 0;
			z != _mapsize_z;
			++z)
	{
		for (int
				dx = x;
				dx != x + sizeX;
				++dx)
		{
			for (int
					dy = y;
					dy != y + sizeY;
					++dy)
			{
				Tile* const tile = _battleSave->getTile(Position(dx,dy,z));
				for (size_t
						partType = 0;
						partType != Tile::PARTS_TILE;
						++partType)
				{
					tile->setMapData(nullptr,-1,-1, static_cast<MapDataType>(partType));
				}
			}
		}
	}
}

/**
 * Loads all the nodes from the map modules.
 */
void BattlescapeGenerator::loadNodes() // private.
{
	int segment (0);

	const size_t
		blocks_x (static_cast<size_t>(_mapsize_x / 10)),
		blocks_y (static_cast<size_t>(_mapsize_y / 10));

	for (size_t
			y = 0;
			y != blocks_y;
			++y)
	{
		for (size_t
				x = 0;
				x != blocks_x;
				++x)
		{
			_segments[x][y] = segment;

			if (_blocks[x][y] != nullptr && _blocks[x][y] != _testBlock)
			{
				if (_blocks[x][y]->isInGroup(MBT_LANDPAD) == false
					|| _landingzone[x][y] == false) // TODO: look closer at this.
				{
					loadRMP(
						_blocks[x][y],
						static_cast<int>(x) * 10,
						static_cast<int>(y) * 10,
						segment++);
				}
			}
		}
	}
}

/**
 * Attaches all the nodes together in an intimate web of C++.
 */
void BattlescapeGenerator::attachNodeLinks() // private.
{
	for (std::vector<Node*>::const_iterator
			i = _battleSave->getNodes()->begin();
			i != _battleSave->getNodes()->end();
			++i)
	{
		const size_t
			segX = (*i)->getPosition().x / 10,
			segY = (*i)->getPosition().y / 10;
		const int
			borDirs[4]			= {-2,-3,-4,-5},
			borDirs_invert[4]	= {-4,-5,-2,-3};
		int borSegs[4];

		if (static_cast<int>(segX) == (_mapsize_x / 10) - 1)
			borSegs[0] = -1;
		else
			borSegs[0] = _segments[segX + 1]
								  [segY];

		if (static_cast<int>(segY) == (_mapsize_y / 10) - 1)
			borSegs[1] = -1;
		else
			borSegs[1] = _segments[segX]
								  [segY + 1];

		if (segX == 0)
			borSegs[2] = -1;
		else
			borSegs[2] = _segments[segX - 1]
								  [segY];

		if (segY == 0)
			borSegs[3] = -1;
		else
			borSegs[3] = _segments[segX]
								  [segY - 1];

		for (std::vector<int>::iterator
				j = (*i)->getNodeLinks()->begin();
				j != (*i)->getNodeLinks()->end();
				++j)
		{
			for (size_t
					dir = 0;
					dir != 4;
					++dir)
			{
				if (*j == borDirs[dir])
				{
					for (std::vector<Node*>::const_iterator
							k = _battleSave->getNodes()->begin();
							k != _battleSave->getNodes()->end();
							++k)
					{
						if ((*k)->getSegment() == borSegs[dir])
						{
							for (std::vector<int>::iterator
									l = (*k)->getNodeLinks()->begin();
									l != (*k)->getNodeLinks()->end();
									++l )
							{
								if (*l == borDirs_invert[dir])
								{
									*l = (*i)->getId();
									*j = (*k)->getId();
								}
							}
						}
					}
				}
			}
		}
	}
}

/**
 * Selects a position for a MapBlock.
 * @param rects		- pointer to a vector of pointers representing the positions to select from - none means the whole map
 * @param ret_x		- reference the x position for the block - gets stored in this variable
 * @param ret_y		- reference the y position for the block - gets stored in this variable
 * @param size_x	- the x size of the block to add
 * @param size_y	- the y size of the block to add
 * @return, true if a position was selected
 */
bool BattlescapeGenerator::selectPosition( // private.
		const std::vector<SDL_Rect*>* const rects,
		int& ret_x,
		int& ret_y,
		int size_x,
		int size_y)
{
	//Log(LOG_INFO) << "bgen::selectPosition()";
	SDL_Rect rectField;
	rectField.x =
	rectField.y = 0;
	rectField.w = static_cast<Uint16>(_mapsize_x / 10);
	rectField.h = static_cast<Uint16>(_mapsize_y / 10);

	std::vector<SDL_Rect*> available;
	if (rects->empty() == true)
		available.push_back(&rectField);
	else
		available = *rects;

	size_x /= 10;
	size_y /= 10;

	std::pair<int, int> select;
	std::vector<std::pair<int, int> > validPairs;
	bool add;

	for (std::vector<SDL_Rect*>::const_iterator
			i = available.begin();
			i != available.end();
			++i)
	{
		//Log(LOG_INFO) << ". iter";
		const int
			width = static_cast<int>((*i)->w),
			height = static_cast<int>((*i)->h),
			xPos = static_cast<int>((*i)->x),
			yPos = static_cast<int>((*i)->y),
			widthField = static_cast<int>(rectField.w),
			heightField = static_cast<int>(rectField.h);

		if (width >= size_x && height >= size_y)
		{
			//Log(LOG_INFO) << ". . found area";
			for (int
					x = xPos;
					x + size_x <= xPos + width
						&& x + size_x <= widthField;
					++x)
			{
				//Log(LOG_INFO) << ". . . fits X";
				for (int
						y = yPos;
						y + size_y <= yPos + height
							&& y + size_y <= heightField;
						++y)
				{
					//Log(LOG_INFO) << ". . . fits Y";
					select = std::make_pair(x,y);

					if (std::find(
								validPairs.begin(),
								validPairs.end(),
								select) == validPairs.end())
					{
						//Log(LOG_INFO) << ". . . . Test add";
						add = true;

						for (int
								xTest = x;
								xTest != x + size_x;
								++xTest)
						{
							//Log(LOG_INFO) << ". . . . . Test add X";
							for (int
									yTest = y;
									yTest != y + size_y;
									++yTest)
							{
								//Log(LOG_INFO) << ". . . . . Test add Y";
								if (_blocks[static_cast<size_t>(xTest)]
										   [static_cast<size_t>(yTest)] != nullptr)
								{
									//Log(LOG_INFO) << ". . . . . . forget it.";
									add = false;
								}
							}
						}

						if (add == true)
						{
							//Log(LOG_INFO) << ". . . . ADDING";
							validPairs.push_back(select);
						}
					}
				}
			}
		}
	}
	//Log(LOG_INFO) << ". done selections";

	if (validPairs.empty() == false)
	{
		//Log(LOG_INFO) << ". . pick selection, ret TRUE";
		select = validPairs.at(RNG::pick(validPairs.size()));
		ret_x = select.first;
		ret_y = select.second;

		return true;
	}

	//Log(LOG_INFO) << ". ret FALSE";
	return false;
}

/**
 * Adds a craft or UFO to the map and tries to add a landing zone type block
 * underneath it.
 * @param block		- pointer to the MapBlock for the craft in question
 * @param script	- pointer to the script command to pull info from
 * @param rect		- reference the position of the craft and store it here
 * @return, true if the craft was placed
 */
bool BattlescapeGenerator::addCraft( // private.
		const MapBlock* const block,
		MapScript* const script,
		SDL_Rect& rect)
{
	rect.w = static_cast<Uint16>(block->getSizeX());
	rect.h = static_cast<Uint16>(block->getSizeY());

	int
		posX,
		posY;

	if (selectPosition(
					script->getRects(),
					posX, posY,
					static_cast<int>(rect.w),
					static_cast<int>(rect.h)) == true)
	{
		rect.x = static_cast<Sint16>(posX);
		rect.y = static_cast<Sint16>(posY);
		rect.w /= 10;
		rect.h /= 10;

		const size_t
			rC_x = static_cast<size_t>(posX),
			rC_y = static_cast<size_t>(posY),
			rC_w = static_cast<size_t>(rect.w),
			rC_h = static_cast<size_t>(rect.h);

		MapBlock* blockTest;

		for (size_t
				x = 0;
				x != rC_w;
				++x)
		{
			for (size_t
					y = 0;
					y != rC_h;
					++y)
			{
				_landingzone[rC_x + x]
							[rC_y + y] = true;

				blockTest = script->getNextBlock(_terrainRule);
				if (blockTest != nullptr
					&& _blocks[rC_x + x]
							  [rC_y + y] == nullptr)
				{
					_blocks[rC_x + x]
						   [rC_y + y] = blockTest;

					--_blocksLeft;
				}
			}
		}

		return true;
	}

	return false;
}

/**
 * Draws a line along the Map - horizontally/vertically/both.
 * @param dir	- the direction to draw the line (MapScript.h)
 * @param rects	- the positions to allow the line to be drawn in
 * @return, true if the blocks were added
 */
bool BattlescapeGenerator::addLine( // private.
		MapDirection dir,
		const std::vector<SDL_Rect*>* rects)
{
	if (dir == MD_BOTH)
	{
		if (addLine(MD_VERTICAL, rects) == true)
		{
			addLine(MD_HORIZONTAL, rects);
			return true;
		}

		return false;
	}

	int iter = 0,
		roadX = 0, // avoid VC++ linker warnings
		roadY = 0,
		* pRoad,
		limit;
	MapBlockType
		comparator,
		blockType;

	if (dir == MD_VERTICAL)
	{
		pRoad = &roadY;
		comparator = MBT_EWROAD;
		blockType = MBT_NSROAD;
		limit = _mapsize_y / 10;
	}
	else
	{
		pRoad = &roadX;
		comparator = MBT_NSROAD;
		blockType = MBT_EWROAD;
		limit = _mapsize_x / 10;
	}

	bool placed = false;
	while (placed == false)
	{
		placed = true;
		selectPosition(
					rects,
					roadX, roadY,
					10,10);

		for (
				*pRoad = 0;
				*pRoad != limit;
				++(*pRoad))
		{
			if (   _blocks[roadX][roadY] != nullptr
				&& _blocks[roadX][roadY]->isInGroup(comparator) == false)
			{
				placed = false;
				break;
			}
		}

		if (++iter > 20)
			return false; // forget it ...
	}

	*pRoad = 0;
	while (*pRoad != limit)
	{
		if (_blocks[roadX][roadY] == nullptr)
			addBlock(
					roadX, roadY,
					_terrainRule->getMapBlockRand(10, 10, blockType));
		else if (_blocks[roadX][roadY]->isInGroup(comparator) == true)
		{
			_blocks[roadX][roadY] = _terrainRule->getMapBlockRand(10, 10, MBT_CROSSROAD);
			clearModule(
					roadX * 10,
					roadY * 10,
					10,10);
			loadMAP(
				_blocks[roadX][roadY],
				roadX * 10,
				roadY * 10,
				_terrainRule);
		}

		++(*pRoad);
	}

	return true;
}

/**
 * Adds a single block to the map.
 * @param x		- the x position to add the block
 * @param y		- the y position to add the block
 * @param block	- the MapBlock to add
 * @return, true if the block was added
 */
bool BattlescapeGenerator::addBlock( // private.
		int x,
		int y,
		MapBlock* const block)
{
	const size_t
		xSize = (block->getSizeX() - 1) / 10,
		ySize = (block->getSizeY() - 1) / 10,
		xt = static_cast<size_t>(x),
		yt = static_cast<size_t>(y);

	for (size_t
			xd = 0;
			xd <= xSize;
			++xd)
	{
		for (size_t
				yd = 0;
				yd != ySize;
				++yd)
		{
			if (_blocks[xt + xd]
					   [yt + yd])
			{
				return false;
			}
		}
	}

	for (size_t
			xd = 0;
			xd <= xSize;
			++xd)
	{
		for (size_t
				yd = 0;
				yd <= ySize;
				++yd)
		{
			_blocks[xt + xd]
				   [yt + yd] = _testBlock;

			--_blocksLeft;
		}
	}

	for (size_t // mark the south edge of the block for drilling
			xd = 0;
			xd <= xSize;
			++xd)
	{
		_drillMap[xt + xd]
				 [yt + ySize] = MD_VERTICAL;
	}

	for (size_t // then the east edge
			yd = 0;
			yd <= ySize;
			++yd)
	{
		_drillMap[xt + xSize]
				 [yt + yd] = MD_HORIZONTAL;
	}


	_drillMap[xt + xSize]				// then the far corner gets marked for both
			 [yt + ySize] = MD_BOTH;	// this also marks 1x1 modules
	_blocks[xt][yt] = block;

	loadMAP(
		_blocks[xt][yt],
		x * 10,
		y * 10,
		_terrainRule,
		0,
		_battleSave->getTacType() == TCT_BASEDEFENSE);

	return true;
}

/**
 * Drills a tunnel between existing map modules.
 * @note This drills all modules currently on the map so it should take place
 * BEFORE the dirt is added in base defenses.
 * @param info	- pointer to the wall replacements and level to dig on
 * @param rects	- pointer to a vector of pointers defining the length/width of the tunnels themselves
 * @param dir	- the direction to drill
 */
void BattlescapeGenerator::drillModules( // private.
		TunnelData* info,
		const std::vector<SDL_Rect*>* rects,
		MapDirection dir)
{
	const MCDReplacement
		* const westMcd = info->getMCDReplacement("westWall"),
		* const northMcd = info->getMCDReplacement("northWall"),
		* const cornerMcd = info->getMCDReplacement("corner"),
		* const floorMcd = info->getMCDReplacement("floor");

	SDL_Rect rect;
	rect.x =
	rect.y = 3;
	rect.w =
	rect.h = 3;

	if (rects->empty() == false)
		rect = *rects->front();

	Tile* tile;
	MapData* data;

	for (int
			i = 0;
			i != (_mapsize_x / 10);
			++i)
	{
		for (int
				j = 0;
				j != (_mapsize_y / 10);
				++j)
		{
			if (_blocks[i][j] != 0)
			{
				tile = nullptr;
				data = nullptr;

				if (dir != MD_VERTICAL) // drill east
				{
					if (i < (_mapsize_x / 10) - 1
						&& (_drillMap[i][j] == MD_HORIZONTAL
							|| _drillMap[i][j] == MD_BOTH)
						&& _blocks[i + 1][j] != nullptr)
					{
						for (int
								k = rect.y;
								k != rect.y + rect.h;
								++k)
						{
							tile = _battleSave->getTile(Position(
															(i * 10) + 9,
															(j * 10) + k,
															info->level));
							if (tile != nullptr)
							{
								tile->setMapData(0,-1,-1, O_WESTWALL);
								tile->setMapData(0,-1,-1, O_OBJECT);

								if (floorMcd != nullptr)
								{
									data = _terrainRule->getMapDataSets()->at(floorMcd->dataSet)->getObjects()->at(floorMcd->entry);
									tile->setMapData(
												data,
												floorMcd->entry,
												floorMcd->dataSet,
												O_FLOOR);
								}

								tile = _battleSave->getTile(Position(
																(i + 1) * 10,
																(j * 10) + k,
																info->level));
								tile->setMapData(nullptr,-1,-1, O_WESTWALL);

								const MapData* const obj = tile->getMapData(O_OBJECT);
								if (obj != nullptr
									&& obj->getTuCostPart(MT_WALK) == 0)
								{
									tile->setMapData(nullptr,-1,-1, O_OBJECT);
								}
							}
						}

						if (northMcd != nullptr)
						{
							data = _terrainRule->getMapDataSets()->at(northMcd->dataSet)->getObjects()->at(northMcd->entry);
							tile = _battleSave->getTile(Position(
															(i * 10) + 9,
															(j * 10) + rect.y,
															info->level));
							tile->setMapData(
										data,
										northMcd->entry,
										northMcd->dataSet,
										O_NORTHWALL);
							tile = _battleSave->getTile(Position(
															(i * 10) + 9,
															(j * 10) + rect.y + rect.h,
															info->level));
							tile->setMapData(
										data,
										northMcd->entry,
										northMcd->dataSet,
										O_NORTHWALL);
						}

						if (cornerMcd != nullptr)
						{
							data = _terrainRule->getMapDataSets()->at(cornerMcd->dataSet)->getObjects()->at(cornerMcd->entry);
							tile = _battleSave->getTile(Position(
															(i + 1) * 10,
															(j * 10) + rect.y,
															info->level));

							if (tile->getMapData(O_NORTHWALL) == nullptr)
								tile->setMapData(
											data,
											cornerMcd->entry,
											cornerMcd->dataSet,
											O_NORTHWALL);
						}
					}
				}

				if (dir != MD_HORIZONTAL) // drill south
				{
					if (j < (_mapsize_y / 10) - 1
						&& (_drillMap[i][j] == MD_VERTICAL
							|| _drillMap[i][j] == MD_BOTH)
						&& _blocks[i][j + 1] != nullptr)
					{
						for (int
								k = rect.x;
								k != rect.x + rect.w;
								++k)
						{
							tile = _battleSave->getTile(Position(
															(i * 10) + k,
															(j * 10) + 9,
															info->level));
							if (tile != nullptr)
							{
								tile->setMapData(nullptr,-1,-1, O_NORTHWALL);
								tile->setMapData(nullptr,-1,-1, O_OBJECT);

								if (floorMcd != nullptr)
								{
									data = _terrainRule->getMapDataSets()->at(floorMcd->dataSet)->getObjects()->at(floorMcd->entry);
									tile->setMapData(
												data,
												floorMcd->entry,
												floorMcd->dataSet,
												O_FLOOR);
								}

								tile = _battleSave->getTile(Position(
																(i * 10) + k,
																(j + 1) * 10,
																info->level));
								tile->setMapData(nullptr,-1,-1, O_NORTHWALL);

								const MapData* const obj = tile->getMapData(O_OBJECT);
								if (obj != nullptr
									&& obj->getTuCostPart(MT_WALK) == 0)
								{
									tile->setMapData(nullptr,-1,-1, O_OBJECT);
								}
							}
						}

						if (westMcd != nullptr)
						{
							data = _terrainRule->getMapDataSets()->at(westMcd->dataSet)->getObjects()->at(westMcd->entry);
							tile = _battleSave->getTile(Position(
															(i * 10) + rect.x,
															(j * 10) + 9,
															info->level));
							tile->setMapData(
										data,
										westMcd->entry,
										westMcd->dataSet,
										O_WESTWALL);
							tile = _battleSave->getTile(Position(
															(i * 10) + rect.x + rect.w,
															(j * 10) + 9,
															info->level));
							tile->setMapData(
										data,
										westMcd->entry,
										westMcd->dataSet,
										O_WESTWALL);
						}

						if (cornerMcd != nullptr)
						{
							data = _terrainRule->getMapDataSets()->at(cornerMcd->dataSet)->getObjects()->at(cornerMcd->entry);
							tile = _battleSave->getTile(Position(
															(i * 10) + rect.x,
															(j + 1) * 10,
															info->level));

							if (tile->getMapData(O_WESTWALL) == nullptr)
								tile->setMapData(
											data,
											cornerMcd->entry,
											cornerMcd->dataSet,
											O_WESTWALL);
						}
					}
				}
			}
		}
	}
}

/**
 * Removes all blocks within a given set of rects as defined in the command.
 * @param directive - contains all the info needed
 * @return, true if success
 * @feel clever & self-important(!)
 * @reality WoT..
 */
bool BattlescapeGenerator::removeBlocks(MapScript* const directive) // private.
{
	std::vector<std::pair<int, int> > deleted;
	bool success = false;

	for (std::vector<SDL_Rect*>::const_iterator
			i = directive->getRects()->begin();
			i != directive->getRects()->end();
			++i)
	{
		const int
			xPos = static_cast<int>((*i)->x),
			yPos = static_cast<int>((*i)->y),
			width = static_cast<int>((*i)->w),
			height = static_cast<int>((*i)->h);

		for (int
				x = xPos;
				x != xPos + width
					&& x != _mapsize_x / 10;
				++x)
		{
			for (int
					y = yPos;
					y != yPos + height
						&& y != _mapsize_y / 10;
					++y)
			{
				if (_blocks[x][y] != nullptr
					&& _blocks[x][y] != _testBlock)
				{
					std::pair<int, int> pos (x,y); // init.

					if (directive->getGroups()->empty() == false)
					{
						for (std::vector<int>::const_iterator
								j = directive->getGroups()->begin();
								j != directive->getGroups()->end();
								++j)
						{
							if (_blocks[x][y]->isInGroup(*j) == true)
							{
								// the deleted vector should only contain unique entries
								if (std::find(
											deleted.begin(),
											deleted.end(),
											pos) == deleted.end())
								{
									deleted.push_back(pos);
								}
							}
						}
					}
					else if (directive->getBlocks()->empty() == false)
					{
						for (std::vector<int>::const_iterator
								j = directive->getBlocks()->begin();
								j != directive->getBlocks()->end();
								++j)
						{
							if (*j < static_cast<int>(_terrainRule->getMapBlocks()->size()))
							{
								// the deleted vector should only contain unique entries
								if (std::find(
											deleted.begin(),
											deleted.end(),
											pos) == deleted.end())
								{
									deleted.push_back(pos);
								}
							}
						}
					}
					else
					{
						// the deleted vector should only contain unique entries
						if (std::find(
									deleted.begin(),
									deleted.end(),
									pos) == deleted.end())
						{
							deleted.push_back(pos);
						}
					}
				}
			}
		}
	}

	for (std::vector<std::pair<int, int> >::const_iterator
			i = deleted.begin();
			i != deleted.end();
			++i)
	{
		const int
			x = (*i).first,
			y = (*i).second;

		clearModule(
				x * 10,
				y * 10,
				_blocks[x][y]->getSizeX(),
				_blocks[x][y]->getSizeY());

		const int
			delx = _blocks[x][y]->getSizeX() / 10,
			dely = _blocks[x][y]->getSizeY() / 10;

		for (int
				dx = x;
				dx != x + delx;
				++dx)
		{
			for (int
					dy = y;
					dy != y + dely;
					++dy)
			{
				_blocks[dx][dy] = nullptr;
				++_blocksLeft;
			}
		}

		// this command succeeds if even one block is removed.
		success = true;
	}

	return success;
}

/**
 * Sets up the objectives for the map.
 * @param deployRule - deployment data from which to fetch data
 */
void BattlescapeGenerator::setupObjectives(const AlienDeployment* const deployRule)
{
	const SpecialTileType specialType = deployRule->getObjectiveType();
	if (specialType != STT_NONE)
	{
		int
			req = deployRule->getObjectivesRequired(),
			inSitu = 0;
		const int parts = static_cast<int>(Tile::PARTS_TILE);
		MapDataType partType;

		for (size_t
				i = 0;
				i != _battleSave->getMapSizeXYZ();
				++i)
		{
			for (int
					j = 0;
					j != parts;
					++j)
			{
				partType = static_cast<MapDataType>(j);
				if (_battleSave->getTiles()[i]->getMapData(partType)
					&& _battleSave->getTiles()[i]->getMapData(partType)->getSpecialType() == specialType)
				{
					++inSitu;
				}
			}
		}

		if (inSitu != 0)
		{
			_battleSave->setObjectiveType(specialType);

			if (req == 0 || inSitu < req)
				req = inSitu;

			_battleSave->setObjectiveTotal(req);
		}
	}
}

}
