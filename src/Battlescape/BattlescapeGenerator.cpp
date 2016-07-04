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

#include "../Ruleset/MapBlock.h"
#include "../Ruleset/MapData.h"
#include "../Ruleset/MapDataSet.h"
#include "../Ruleset/MCDPatch.h"
#include "../Ruleset/RuleAlienDeployment.h"
#include "../Ruleset/RuleAlienRace.h"
#include "../Ruleset/RuleArmor.h"
#include "../Ruleset/RuleBaseFacility.h"
//#include "../Ruleset/RuleCraft.h"
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
#include "../Savegame/SoldierLayout.h"
#include "../Savegame/ItemContainer.h"
#include "../Savegame/Node.h"
#include "../Savegame/SavedBattleGame.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/Soldier.h"
#include "../Savegame/TerrorSite.h"
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
		_unitList(game->getSavedGame()->getBattleSave()->getUnits()),
		_itemList(game->getSavedGame()->getBattleSave()->getItems()),
		_rules(game->getRuleset()),
		_res(game->getResourcePack()),
		_craft(nullptr),
		_ufo(nullptr),
		_base(nullptr),
		_terrorSite(nullptr),
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
		_isFakeInventory(false),
		_battleOrder(0),
		_blocksLeft(0),
		_testBlock(nullptr)
//		_error(false)
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
	_seg.clear();
	_drillMap.clear();

	_blocks.resize(
			_mapsize_x / 10,
			std::vector<MapBlock*>(_mapsize_y / 10));
	_landingzone.resize(
			_mapsize_x / 10,
			std::vector<bool>(_mapsize_y / 10, false));
	_seg.resize(
			_mapsize_x / 10,
			std::vector<int>(_mapsize_y / 10, 0));
	_drillMap.resize(
			_mapsize_x / 10,
			std::vector<int>(_mapsize_y / 10, MD_NONE));

	_blocksLeft = (_mapsize_x / 10) * (_mapsize_y / 10);

	_battleSave->initMap(
					_mapsize_x,
					_mapsize_y,
					_mapsize_z);		// creates the tile-parts
	_battleSave->initUtilities(_res);	// creates Pathfinding and TileEngine.
}

/**
 * Sets the xCom Craft involved in the battle.
 * @param craft - pointer to Craft
 */
void BattlescapeGenerator::setCraft(Craft* const craft)
{
	_craft = craft;
	_craft->setTactical();
}

/**
 * Sets the UFO involved in the battle.
 * @param ufo - pointer to Ufo
 */
void BattlescapeGenerator::setUfo(Ufo* const ufo)
{
	_ufo = ufo;
	_ufo->setTactical();
}

/**
 * Sets the xCom Base involved in the battle.
 * @param base - pointer to Base
 */
void BattlescapeGenerator::setBase(Base* const base)
{
	_base = base;
	_base->setTactical();
}

/**
 * Sets the aLien TerrorSite involved in the battle.
 * @param terrorSite - pointer to TerrorSite
 */
void BattlescapeGenerator::setTerrorSite(TerrorSite* const terrorSite)
{
	_terrorSite = terrorSite;
	_terrorSite->setTactical();
}

/**
 * Sets the AlienBase involved in the battle.
 * @param base - pointer to AlienBase
 */
void BattlescapeGenerator::setAlienBase(AlienBase* const base)
{
	_alienBase = base;
	_alienBase->setTactical();
}

/**
 * Sets the terrain-rule based on where a UFO crashed/landed as per
 * ConfirmLandingState or nextStage().
 * @param texture - pointer to RuleTerrain
 */
void BattlescapeGenerator::setTerrain(RuleTerrain* const terrain)
{
	_terrainRule = terrain;
}

/**
 * Sets the tactical shade based on where a UFO crashed or landed.
 * @note This is used to determine the Battlescape light-level.
 * @param shade - current shade of a Polygon on the Globe
 */
void BattlescapeGenerator::setShade(int shade)
{
	_shade = std::max(0, std::min(15, shade));
}

/**
 * Sets the alien-race for tactical.
 * @note This is used to determine the various alien-types to spawn.
 * @param alienRace - reference to the alien-race family
 */
void BattlescapeGenerator::setAlienRace(const std::string& alienRace)
{
	_alienRace = alienRace;
	_battleSave->setAlienRace(alienRace);
}

/**
 * Sets the alien-item-level.
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
 * Starts the Generator.
 * @note This fills the SavedBattleGame with data.
 */
void BattlescapeGenerator::run()
{
	_unitSequence = BattleUnit::MAX_SOLDIER_ID; // geoscape Soldier IDs should stay below this number

	const RuleAlienDeployment* ruleDeploy;
	if (_ufo != nullptr)
		ruleDeploy = _rules->getDeployment(_ufo->getRules()->getType());
	else
		ruleDeploy = _rules->getDeployment(_battleSave->getTacticalType());

	_battleSave->setTurnLimit(ruleDeploy->getTurnLimit());
	_battleSave->setChronoResult(ruleDeploy->getChronoResult());
	_battleSave->setCheatTurn(ruleDeploy->getCheatTurn());

	ruleDeploy->getDimensions(
						&_mapsize_x,
						&_mapsize_y,
						&_mapsize_z);

	if (_terrainRule == nullptr) // '_terrainRule' NOT set for Cydonia, Base assault/defense. Already set for NewBattleState ...... & UFO, & terrorSite.
	{
		_terrainRule = _rules->getTerrain(ruleDeploy->getDeployTerrains().at(RNG::pick(ruleDeploy->getDeployTerrains().size())));
		if (_terrainRule == nullptr)
		{
			// trouble: no texture and no deployment terrain, most likely scenario is a UFO landing on water: use the first available terrain.
			_terrainRule = _game->getRuleset()->getTerrain(_game->getRuleset()->getTerrainList().front());
			Log(LOG_WARNING) << "bGen::run() Could not find a terrain rule. Use first terrainType: "
							 << _terrainRule->getType();
		}
	}
/* Theirs:
	if (_terrainRule == nullptr)
	{
		if (_texture == nullptr
			|| _texture->getTextureDetail()->empty() == true
			|| ruleDeploy->getDeployTerrains().empty() == false)
		{
			size_t pick = RNG::generate(0, ruleDeploy->getDeployTerrains().size() - 1);
			_terrainRule = _rules->getTerrain(ruleDeploy->getDeployTerrains().at(pick));
		}
		else // UFO crashed/landed or TerrorSite
		{
			const Target* target;
			if (_terrorSite != nullptr) target = _terrorSite;
			else target = _ufo;
			_terrainRule = _rules->getTerrain(_texture->getTextureTerrain(target));
		}
	} */
/*	if (ruleDeploy->getDeployTerrains().empty() == true) // UFO crashed/landed
	{
		Log(LOG_INFO) << "bGen::run() deployment-terrains NOT valid";
		if (_terrorSiteTerrain == nullptr) // kL
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
		Log(LOG_INFO) << "bGen::run() Choose terrain from deployment, qty = " << ruleDeploy->getDeployTerrains().size();
		const size_t pick = RNG::generate(0, ruleDeploy->getDeployTerrains().size() - 1);
		_terrainRule = _rules->getTerrain(ruleDeploy->getDeployTerrains().at(pick));
	} */


	const std::vector<MapScript*>* directives (nullptr); // alienDeployment-script overrides terrain-script <-

	std::string script (ruleDeploy->getScriptType());
	//Log(LOG_INFO) << "bgen: script = " << script;
	if (script.empty() == false)
	{
		directives = _rules->getMapScripts(script);
		if (directives == nullptr) Log(LOG_WARNING) << "bGen::run() - There is a Deployment script defined ["
													<< script << "] but could not find its rule.";
	}

	if (directives == nullptr)
	{
		script = _terrainRule->getScriptType();
		//Log(LOG_INFO) << "bgen: script = " << script;
		if (script.empty() == false)
		{
			directives = _rules->getMapScripts(script);
			if (directives == nullptr) Log(LOG_WARNING) << "bGen::run() - There is a Terrain script defined ["
														<< script << "] but could not find its rule.";
		}
	}

	if (directives == nullptr)
	{
		throw Exception("bGen:run() No map-script found. See logfile for details.");
	}

	generateMap(directives);						// <--|| BATTLEFIELD GENERATION. <--|||
	setupObjectives(ruleDeploy);

	if (ruleDeploy->getShade() != -1)
		setShade(ruleDeploy->getShade());

	_battleSave->setTacticalShade(_shade);

	_battleSave->setBattleTerrain(_terrainRule->getType());
	setTacticalSprites();

	deployXcom();									// <--|| XCOM DEPLOYMENT. <--|||

	const size_t qtyUnits_pre (_unitList->size());

	deployAliens(ruleDeploy);						// <--|| ALIEN DEPLOYMENT. <--|||

	if (qtyUnits_pre == _unitList->size())
	{
		throw Exception("bGen:run() No alien units could be placed on the map.");
	}

	deployCivilians(ruleDeploy->getCivilians());	// <--|| CIVILIAN DEPLOYMENT. <--|||

	if (_generateFuel == true)
		fuelPowerSources();

	if (_ufo != nullptr && _ufo->getUfoStatus() == Ufo::CRASHED)
		explodePowerSources();

/*	if (_missionType == "STR_BASE_DEFENSE")
	{
		for (int i = 0; i < _battleSave->getMapSizeXYZ(); ++i)
			_battleSave->getTiles()[i]->setDiscovered(true, 2);
		_battleSave->calcBaseDestruct();
	}
	if (_missionType == "STR_ALIEN_BASE_ASSAULT"
		|| _missionType == "STR_MARS_THE_FINAL_ASSAULT")
	{
		for (int i = 0; i < _battleSave->getMapSizeXYZ(); ++i)
		{
			if (_battleSave->getTiles()[i]->getMapData(O_FLOOR)
				&& (_battleSave->getTiles()[i]->getMapData(O_FLOOR)->getTileType() == START_POINT
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
				&& _battleSave->getTiles()[i]->getMapData(O_FLOOR)->getTileType() == START_POINT)
//					|| (_battleSave->getTiles()[i]->getPosition().z == _mapsize_z - 1
//						&& _battleSave->getTiles()[i]->getMapData(O_FLOOR)->isGravLift()
//						&& _battleSave->getTiles()[i]->getMapData(O_OBJECT))
			{
				_battleSave->getTiles()[i]->setDiscovered(true, 2);
			}
		}
	} */

	std::vector<std::string> tracks (ruleDeploy->getDeploymentMusics());
	if (tracks.empty() == false)
		_battleSave->setMusic(tracks.at(RNG::pick(tracks.size(), true)));
	else
	{
		tracks = _terrainRule->getTerrainMusics();
		if (tracks.empty() == false)
			_battleSave->setMusic(tracks.at(RNG::pick(tracks.size(), true)));
	}

	_battleSave->getTileEngine()->calculateSunShading();
	_battleSave->getTileEngine()->calculateTerrainLighting();
	_battleSave->getTileEngine()->calculateUnitLighting();

//	_battleSave->getTileEngine()->calcFovAll(false, true); // NOTE: Also done in BattlescapeGame::init(). Are both needed. no.

	_battleSave->getShuffleUnits()->assign(
										_unitList->size(),
										nullptr);
}

/**
 * Re-initializes an existing SavedBattleGame for a next-stage tactical battle.
 */
void BattlescapeGenerator::nextStage()
{
	_battleSave->resetTurnCounter();

	const Tile* tile;
	Position pos;

	int unitSize;

	bool fullSuccess (true); // all aLiens were put down on previous stage regardless of 'aborted'.

	// NOTE: The next section decides if each unit should be set Latent.
	// Player units are never set latent unless the 1st-stage is aborted; all
	// non-player units will have a Latency set, either Latent_Start or just
	// Latent depending on what tile they're on at the end of the 1st-stage. If
	// aborted, player-units will be set latent unless they're on an End_Point.
	const Position posBogus (Position(-1,-1,-1));
	for (std::vector<BattleUnit*>::const_iterator	// set all living hostile/neutral units Latent
			i = _unitList->begin();					// and deal with player-units not in endpoint-area if aborted
			i != _unitList->end();					// plus break all unit-tile links and unit-positions.
			++i)
	{
		if ((*i)->getUnitStatus() != STATUS_DEAD)
		{
			(*i)->setAIState();

			switch ((*i)->getOriginalFaction())
			{
				case FACTION_PLAYER:
					if (_battleSave->isAborted() == true)
						setUnitLatency(*i);
					break;

				case FACTION_HOSTILE:
					if ((*i)->getUnitStatus() == STATUS_STANDING)
						fullSuccess = false;
					// no break;
				case FACTION_NEUTRAL:
					setUnitLatency(*i);
			}
		}

		if ((*i)->getUnitTile() != nullptr) // break Tiles' links to unit.
		{
			switch (unitSize = (*i)->getArmor()->getSize())
			{
				case 1:
					(*i)->getUnitTile()->setTileUnit();
					break;

				case 2:
				{
					pos = (*i)->getPosition();
					for (int
							x = 0;
							x != unitSize;
							++x)
					{
						for (int
								y = 0;
								y != unitSize;
								++y)
						{
							_battleSave->getTile(pos + Position(x,y,0))->setTileUnit();
						}
					}
				}
			}
		}
		(*i)->setUnitTile();				// break unit's link to Tile.
		(*i)->setPosition(posBogus, false);	// and give unit a bogus Position(-1,-1,-1)
	}

	// Remove all items not belonging to player-units from the map.
	// Sort items into two categories:
	// - the ones that are guaranteed to be able to take home barring complete
	//	 failure - ie: stuff on the transport craft
	// - and those that are scattered about on the ground that will be recovered
	//	 ONLY on success.
	// This does not include items in player-units' hands.
	// whatever.
	// NOTE: The one rule through all of this is that unit-corpses should never
	// get deleted; they need to be used for Scoring and possible resurrection
	// after the second stage - when the Debriefing runs finally.
	std::vector<BattleItem*>
		* const guaranteed (_battleSave->guaranteedRecover()),
		* const conditional (_battleSave->conditionalRecover()),
		forwardGround,
		forwardCarried,
		deletable;

	for (std::vector<BattleItem*>::const_iterator
			i = _itemList->begin();
			i != _itemList->end();
			++i)
	{
		std::vector<BattleItem*>* dst; // assign the item a destination container ->

		if ((*i)->getRules()->isRecoverable() == true)
		{
			if ((*i)->getItemUnit() != nullptr							// IMPORTANT: Do not load weapons with battle-corpses.
				&& (*i)->getItemUnit()->getUnitStatus() != STATUS_DEAD)	// if unit is Dead just treat its corpse as any other object.
			{
				if ((*i)->getOwner() == nullptr
					|| (*i)->getItemUnit()->isHealable() == false)
				{
					switch ((*i)->getItemUnit()->getOriginalFaction())
					{
						case FACTION_PLAYER:
							if (fullSuccess == true) // aborted or not.
							{
								if ((*i)->getItemUnit()->getUnitStatus() == STATUS_UNCONSCIOUS
									&& (*i)->getItemUnit()->isHealable() == true
									&& (_battleSave->isAborted() == false
										|| ((tile = (*i)->getItemTile()) != nullptr
											&& tile->getMapData(O_FLOOR) != nullptr
											&& tile->getMapData(O_FLOOR)->getTileType() == END_POINT)))
								{
									dst = &forwardGround;
								}
								else
									dst = guaranteed;
							}
							else // all aLiens were NOT put down; implies that previous stage was Aborted.
							{
								switch ((*i)->getItemUnit()->getUnitStatus())
								{
									case STATUS_UNCONSCIOUS:
										if ((*i)->getItemUnit()->isHealable() == true)
										{
											dst = &forwardGround;	// tricky bit of logic here.
											break;					// Relies on the result of setUnitLatency().
										}
										// no break;

									case STATUS_LATENT: dst = conditional;
										break;

									case STATUS_LATENT_START: dst = guaranteed;
								}
							}
							break;

						case FACTION_HOSTILE:
						case FACTION_NEUTRAL:
							if (fullSuccess == true
								|| (*i)->getItemUnit()->getUnitStatus() == STATUS_LATENT_START)
							{
								dst = guaranteed;
							}
							else
								dst = conditional;
					}
				}
				else // body is carried and healable
					dst = &forwardCarried;
			}
			else if ((*i)->isLoad() == false)
			{
				if ((*i)->getOwner() == nullptr)
				{
					(*i)->setFuse(-1);

					if (fullSuccess == true) // all aLiens were put down on previous stage
					{
						if (_gameSave->isResearched((*i)->getRules()->getRequirements()) == true)
							dst = &forwardGround;
						else
							dst = guaranteed;
					}
					else if ((tile = (*i)->getItemTile()) != nullptr // definitely aborted.
						&& tile->getMapData(O_FLOOR) != nullptr)
					{
						switch (tile->getMapData(O_FLOOR)->getTileType())
						{
							case START_POINT: dst = guaranteed;
								break;

							case END_POINT:
								if (_gameSave->isResearched((*i)->getRules()->getRequirements()) == true)
								{
									dst = &forwardGround;
									break;
								}
								// no break;

							default: dst = conditional;
						}
					}
					else // safety catch-all.
						dst = conditional;
				}
				else // carried:
				{
					if (fullSuccess == true) // no hostiles left standing so they shouldn't be carrying anything; neutrals might still be packing an apple or two ...
					{
						if (_gameSave->isResearched((*i)->getRules()->getRequirements()) == true)
							dst = &forwardCarried;
						else
							dst = guaranteed;
					}
					else // carried & aborted:
					{
						switch ((*i)->getOwner()->getFaction())
						{
							case FACTION_PLAYER:
								switch ((*i)->getOwner()->getUnitStatus())
								{
									case STATUS_LATENT_START: dst = guaranteed;
										break;

									default:
										if (_gameSave->isResearched((*i)->getRules()->getRequirements()) == true)
										{
											dst = &forwardCarried;	// Not so sure that I really want unresearched stuff getting shuttled around automagically !
											break;
										}
										// no break;				// In fact I don't; aborting player-units should have to hold onto their stuff and drag it out manually.

									case STATUS_LATENT: dst = conditional;
								}
								break;

							case FACTION_HOSTILE:
							case FACTION_NEUTRAL:
//								dst = &deletable;	// aLiens and Civies run away with their stuff ... but, uh, their bodies are counted in Debriefing.
								dst = conditional;	// So make it conditional -- fun: Two-stage routines that I'm not even going to use ......
						}
					}
				}


				if ((*i)->selfPowered() == false)
				{
					BattleItem* const load ((*i)->getAmmoItem());
					if (load != nullptr)
					{
						if (load->getRules()->isRecoverable() == true)
							dst->push_back(load);
						else
							dst = &deletable;
					}
				}
			}
		}
		else
			dst = &deletable;

		(*i)->setItemTile();
		dst->push_back(*i); // -> put the item in its destination container
	}


	for (std::vector<BattleItem*>::const_iterator
			i = guaranteed->begin();
			i != guaranteed->end();
			++i)
	{
		Log(LOG_INFO) << "bGen:nextStage() guaranteed " << (*i)->getRules()->getType();
		Log(LOG_INFO) << ". ItemID-" << (*i)->getId();
		if ((*i)->getItemUnit() != nullptr) Log(LOG_INFO) << ". UnitID-" << (*i)->getItemUnit()->getId()
													  << " status= " << (int)(*i)->getItemUnit()->getUnitStatus();
	}

	for (std::vector<BattleItem*>::const_iterator
			i = conditional->begin();
			i != conditional->end();
			++i)
	{
		Log(LOG_INFO) << "bGen:nextStage() conditional " << (*i)->getRules()->getType();
		Log(LOG_INFO) << ". ItemID-" << (*i)->getId();
		if ((*i)->getItemUnit() != nullptr) Log(LOG_INFO) << ". UnitID-" << (*i)->getItemUnit()->getId()
													  << " status= " << (int)(*i)->getItemUnit()->getUnitStatus();
	}

	for (std::vector<BattleItem*>::const_iterator
			i = deletable.begin();
			i != deletable.end();
			++i)
	{
		Log(LOG_INFO) << "bGen:nextStage() deletable " << (*i)->getRules()->getType();
		Log(LOG_INFO) << ". ItemID-" << (*i)->getId();
		if ((*i)->getItemUnit() != nullptr) Log(LOG_INFO) << ". UnitID-" << (*i)->getItemUnit()->getId()
													  << " status= " << (int)(*i)->getItemUnit()->getUnitStatus();
		_battleSave->toDeleteItem(*i);
	}

	_itemList->clear();
	for (std::vector<BattleItem*>::const_iterator
			i = forwardCarried.begin();
			i != forwardCarried.end();
			++i)
	{
		Log(LOG_INFO) << "bGen:nextStage() forwardCarried " << (*i)->getRules()->getType();
		Log(LOG_INFO) << ". ItemID-" << (*i)->getId();
		if ((*i)->getItemUnit() != nullptr) Log(LOG_INFO) << ". UnitID-" << (*i)->getItemUnit()->getId()
													  << " status= " << (int)(*i)->getItemUnit()->getUnitStatus();
		_itemList->push_back(*i);
	}
	// NOTE: forwardGround vector is placed below after _tileEquipt is assigned.


	const RuleAlienDeployment* const ruleDeploy (_rules->getDeployment(_battleSave->getTacticalType()));

	_battleSave->setTurnLimit(ruleDeploy->getTurnLimit());
	_battleSave->setChronoResult(ruleDeploy->getChronoResult());
	_battleSave->setCheatTurn(ruleDeploy->getCheatTurn());

	ruleDeploy->getDimensions(
						&_mapsize_x,
						&_mapsize_y,
						&_mapsize_z);

	_terrainRule = _rules->getTerrain(ruleDeploy->getDeployTerrains().at(RNG::pick(ruleDeploy->getDeployTerrains().size())));


	const std::vector<MapScript*>* directives (nullptr); // alienDeployment-script overrides terrain-script <-

	std::string script (ruleDeploy->getScriptType());
	//Log(LOG_INFO) << "bgen: script = " << script;
	if (script.empty() == false)
	{
		directives = _rules->getMapScripts(script);
		if (directives == nullptr) Log(LOG_WARNING) << "bGen::nextStage() - There is a Deployment script defined ["
													<< script << "] but could not find its rule.";
	}

	if (directives == nullptr)
	{
		script = _terrainRule->getScriptType();
		//Log(LOG_INFO) << "bgen: script = " << script;
		if (script.empty() == false)
		{
			directives = _rules->getMapScripts(script);
			if (directives == nullptr) Log(LOG_WARNING) << "bGen::nextStage() - There is a Terrain script defined ["
														<< script << "] but could not find its rule.";
		}
	}

	if (directives == nullptr)
	{
		throw Exception("bGen:nextStage() No map-script found. See logfile for details.");
	}

	generateMap(directives);							// <--|| BATTLEFIELD GENERATION. <--|||
	setupObjectives(ruleDeploy);

	setShade(ruleDeploy->getShade()); // NOTE: 2nd stage must have deployment-shade set, else 0 (bright).
	_battleSave->setTacticalShade(_shade);
	_battleSave->setBattleTerrain(_terrainRule->getType());
//	setTacticalSprites();
	_battleSave->isAborted(false);

	bool soldierFound (false);
	for (std::vector<BattleUnit*>::const_iterator		// <--|| XCOM DEPLOYMENT. <--|||
			i = _unitList->begin();
			i != _unitList->end();
			++i)
	{
		if ((*i)->getOriginalFaction() == FACTION_PLAYER)
		{
			(*i)->setFaction(FACTION_PLAYER);
			(*i)->setExposed(-1);
			(*i)->kneelUnit(false);
//			(*i)->getVisibleTiles()->clear();

			if ((*i)->getUnitStatus() == STATUS_STANDING)
			{
				if (soldierFound == false && (*i)->getGeoscapeSoldier() != nullptr)
				{
					soldierFound = true;
					_battleSave->setSelectedUnit(*i);
				}

				const Node* const node (_battleSave->getSpawnNode(NR_XCOM, *i));
				if (node != nullptr || placeUnitNearFaction(*i) == true)
				{
					if (node != nullptr)
						_battleSave->setUnitPosition(*i, node->getPosition());

					if (_tileEquipt == nullptr)
						_battleSave->setBattleInventory(_tileEquipt = (*i)->getUnitTile());

					_tileEquipt->setTileUnit(*i);	// Set all of player's units non-visible on tileEquipt for pre-battle inventory.
					(*i)->setUnitVisible(false);	// InventoryState::btnOkClick() calls resetUnitsOnTiles() to correct this for battle.
				}
				else
				{
					(*i)->setUnitStatus(STATUS_LATENT);
					Log(LOG_WARNING) << "bGen:nextStage() Could not place xCom unit id-" << (*i)->getId() << " Set to STATUS_LATENT.";
				}
			}
		}
	}

	if (soldierFound == false)
	{
		throw Exception("bGen:nextStage() No soldier-unit found.");
	}

//	const BattleUnit* const selUnit (_battleSave->getSelectedUnit());
//	if (selUnit == nullptr
//		|| selUnit->getUnitStatus() != STATUS_STANDING
//		|| selUnit->getFaction() != FACTION_PLAYER)
//	{
//		_battleSave->selectNextFactionUnit(); // NOTE: This runs only if the only player-unit still conscious is a support-unit.
//	}

	const RuleInventory* const grdRule (_game->getRuleset()->getInventoryRule(ST_GROUND));
	for (std::vector<BattleItem*>::const_iterator
		i = forwardGround.begin();
		i != forwardGround.end();
		++i)
	{
		Log(LOG_INFO) << "bGen:nextStage() forwardGround " << (*i)->getRules()->getType();
		Log(LOG_INFO) << ". ItemID-" << (*i)->getId();
		_itemList->push_back(*i);

		(*i)->setInventorySection(grdRule);
		_tileEquipt->addItem(*i);

		if ((*i)->getItemUnit() != nullptr)
		{
			Log(LOG_INFO) << ". UnitID-" << (*i)->getItemUnit()->getId()
						  << " status= " << (int)(*i)->getItemUnit()->getUnitStatus();
			(*i)->getItemUnit()->setPosition(_tileEquipt->getPosition());
		}
	}


	_alienRace = ruleDeploy->getRace();
	if (_alienRace.empty() == true)
	{
		for (std::vector<TerrorSite*>::const_iterator
				i = _gameSave->getTerrorSites()->begin();
				i != _gameSave->getTerrorSites()->end();
				++i)
		{
			if ((*i)->getTactical() == true)
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
				if ((*i)->getTactical() == true)
				{
					_alienRace = (*i)->getAlienRace();
					break;
				}
			}
		}
	}

	if (_alienRace.empty() == true)
	{
		throw Exception("bGen:nextStage() No alien race specified.");
	}


	_unitSequence = _unitList->back()->getId() + 1;

	const size_t qtyUnits_pre (_unitList->size());

	deployAliens(ruleDeploy);							// <--|| ALIEN DEPLOYMENT. <--|||

	if (qtyUnits_pre == _unitList->size())
	{
		throw Exception("bGen:nextStage() No alien units deployed.");
	}

	deployCivilians(ruleDeploy->getCivilians());		// <--|| CIVILIAN DEPLOYMENT. <--|||

/*	// Probly don't need this anymore; it's done via "revealedFloors" in MapScripting ... but not quite.
	for (int i = 0; i < _battleSave->getMapSizeXYZ(); ++i)
	{
		if (_battleSave->getTiles()[i]->getMapData(O_FLOOR) != nullptr
			&& _battleSave->getTiles()[i]->getMapData(O_FLOOR)->getTileType() == START_POINT)
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

//	_battleSave->getTileEngine()->calcFovAll(false, true); // NOTE: Also done in BattlescapeGame::init(). Are both needed. no.

//	if (_battleSave->getBattleGame() != nullptr) // safety. Shall be valid for nextStage() start here.
	_battleSave->getBattleGame()->reinit();

	_battleSave->getShuffleUnits()->assign(
										_unitList->size(),
										nullptr);
}

/**
 * Determines and sets the latency-status of a specified BattleUnit at the start
 * of next-stage tactical battles.
 * @param unit - pointer to a BattleUnit
 */
void BattlescapeGenerator::setUnitLatency(BattleUnit* const unit) // private.
{
	const Position pos (unit->getPosition());
	switch (unit->getUnitStatus())
	{
		case STATUS_UNCONSCIOUS:			// has no Tile, need to get one
			if (pos == Position(-1,-1,-1))	// is carried
			{
				bool found (false);
				for (std::vector<BattleUnit*>::const_iterator		// find carrier
						i = _unitList->begin();
						i != _unitList->end() && found == false;
						++i)
				{
					for (std::vector<BattleItem*>::const_iterator	// find body in carrier's inventory
							j = (*i)->getInventory()->begin();
							j != (*i)->getInventory()->end() && found == false;
							++j)
					{
						if (/*(*j)->getItemUnit() != nullptr &&*/ (*j)->getItemUnit() == *i)
						{
							found = true;

							if ((*i)->isOnTiletype(START_POINT) == true)
								unit->setUnitStatus(STATUS_LATENT_START);
							else
							{
								switch (unit->getOriginalFaction())
								{
									case FACTION_PLAYER:
										if ((*i)->isOnTiletype(END_POINT) == true	// the unit's carrier is on an End_Tile and is carrying said
											&& unit->isHealable() == true)			// unit to the next stage. So remain Unconscious @ pos(-1,-1,-1)
										{
											break;
										}
										// no break;
									case FACTION_HOSTILE:
									case FACTION_NEUTRAL:
										unit->setUnitStatus(STATUS_LATENT);
								}
							}
						}
					}
				}
			}
			else // is NOT carried
			{
				const Tile* const tile (_battleSave->getTile(pos));
				if (tile != nullptr && tile->getMapData(O_FLOOR) != nullptr)
				{
					switch (tile->getMapData(O_FLOOR)->getTileType())
					{
						case START_POINT:
							unit->setUnitStatus(STATUS_LATENT_START);
							break;

						case END_POINT:
							if (unit->getOriginalFaction() == FACTION_PLAYER	// if Faction_Player the unit is lying Unconscious on an End_Tile and will
								&& unit->isHealable() == true)					// appear on the next stage's equipt-tile. So just stay Unconscious
							{
								break;
							}
							// no break;
						default:
							unit->setUnitStatus(STATUS_LATENT);
							break;
					}
				}
				else // safety catch-all.
					unit->setUnitStatus(STATUS_LATENT);
			}
			break;

		case STATUS_STANDING: // TODO: Check that MC'd units are reset to original faction ....
			if (unit->isOnTiletype(START_POINT) == true)
				unit->setUnitStatus(STATUS_LATENT_START);
			else
			{
				switch (unit->getOriginalFaction())
				{
					case FACTION_PLAYER:
						if (unit->isOnTiletype(END_POINT) == true)
							break;
						// no break;
					case FACTION_HOSTILE:
					case FACTION_NEUTRAL:
						unit->setUnitStatus(STATUS_LATENT);
				}
			}
	}
}

/**
 * Deploys the player's units and equipment based on the Craft and/or Base.
 */
void BattlescapeGenerator::deployXcom() // private.
{
	//Log(LOG_INFO) << "";
	//Log(LOG_INFO) << "BattlescapeGenerator::deployXcom()";
	// A vehicle is actually an item that you will never see since it is
	// converted to a unit -- the item itself however becomes its weapon.
	std::vector<Vehicle*>* pSupports;

	if (_craft != nullptr)
	{
		//Log(LOG_INFO) << ". craft VALID";
		_base = _craft->getBase();

		if (_isFakeInventory == false)
			pSupports = _craft->getVehicles();
		else
			pSupports = nullptr;
	}
	else if (_base != nullptr && _isFakeInventory == false)
	{
		//Log(LOG_INFO) << ". base VALID";
		std::vector<Vehicle*> supports;
		prepBaseVehicles(supports);
		pSupports = &supports;
	}
	else
		pSupports = nullptr;

	if (pSupports != nullptr)
	{
		for (std::vector<Vehicle*>::const_iterator
				i = pSupports->begin();
				i != pSupports->end();
				++i)
		{
			//Log(LOG_INFO) << ". . convertVehicle " << (int)*i;
			BattleUnit* const unit (convertVehicle(*i));
			if (unit != nullptr && _battleSave->getSelectedUnit() == nullptr)
				_battleSave->setSelectedUnit(unit);
		}
	}


	for (std::vector<Soldier*>::const_iterator // Add Soldiers that are in the Craft or Base.
			i = _base->getSoldiers()->begin();
			i != _base->getSoldiers()->end();
			++i)
	{
		if ((_craft != nullptr
				&& (*i)->getCraft() == _craft)
			|| (_craft == nullptr
				&& (*i)->getSickbay() == 0
				&& ((*i)->getCraft() == nullptr
					|| (*i)->getCraft()->getCraftStatus() != CS_OUT)))
		{
			//Log(LOG_INFO) << ". . addPlayerUnit " << (*i)->getId();
			BattleUnit* const unit (addPlayerUnit(new BattleUnit(*i, _gameSave->getDifficulty())));
			if (unit != nullptr)
			{
				//Log(LOG_INFO) << "bGen::deployXcom() ID " << unit->getId() << " battleOrder = " << _battleOrder + 1;
				unit->setBattleOrder(++_battleOrder);

				if (_battleSave->getSelectedUnit() == nullptr)
					_battleSave->setSelectedUnit(unit);
			}
		}
	}

	if (_unitList->empty() == true)
	{
		throw Exception("bGen:deployXcom() No player units deployed.");
	}

	//Log(LOG_INFO) << ". addPlayerUnit(s) DONE";

	for (std::vector<BattleUnit*>::const_iterator // pre-battle Equip; give all xCom units access to the inventory tile.
			i = _unitList->begin();
			i != _unitList->end();
			++i)
	{
		_tileEquipt->setTileUnit(*i);	// Set all of player's units non-visible on tileEquipt for pre-battle inventory.
		(*i)->setUnitVisible(false);	// InventoryState::btnOkClick() calls resetUnitsOnTiles() to correct this for battle.
	}
	//Log(LOG_INFO) << ". setTileUnit(s) DONE";


	const RuleInventory* const grdRule (_rules->getInventoryRule(ST_GROUND));

	if (_craft != nullptr) // UFO or Base Assault or Craft-equip.
	{
		//Log(LOG_INFO) << "";
		//Log(LOG_INFO) << ". . addCraftItems";
		BattleItem* item;
		for (std::map<std::string, int>::const_iterator // Add items that are in the Craft.
				i = _craft->getCraftItems()->getContents()->begin();
				i != _craft->getCraftItems()->getContents()->end();
				++i)
		{
			//Log(LOG_INFO) << ". . . item = " << i->first << " (" << i->second << ")";
			for (int
					j = 0;
					j != i->second;
					++j)
			{
				//Log(LOG_INFO) << ". . . . addItem() ToTile iter = " << (j + 1);
				item = new BattleItem(
									_rules->getItemRule(i->first),
									_battleSave->getCanonicalBattleId());
				item->setInventorySection(grdRule);
				_tileEquipt->addItem(item);
			}
		}
		//Log(LOG_INFO) << ". . addCraftItems DONE";
	}
	else // Base Defense or Base-equip.
	{
		BattleItem* item;

		// Add only items in Craft that are at the Base for skirmish mode; ie.
		// Do NOT add items from the Base itself in skirmish mode.
		if (_gameSave->getMonthsPassed() != -1)									// Add items that are at the Base.
		{
			//Log(LOG_INFO) << "";
			//Log(LOG_INFO) << ". . addBaseItems";
			const RuleItem* itRule;
			for (std::map<std::string, int>::const_iterator						// Add items that are in the Base's storage.
					i = _base->getStorageItems()->getContents()->begin();
					i != _base->getStorageItems()->getContents()->end();
					)
			{
				//Log(LOG_INFO) << ". . . item = " << i->first << " (" << i->second << ")";
				itRule = _rules->getItemRule(i->first);
				switch (itRule->getBattleType())
				{
					case BT_FIREARM:
					case BT_AMMO:
					case BT_MELEE:
					case BT_GRENADE:
					case BT_PROXYGRENADE:
					case BT_MEDIKIT:
					case BT_SCANNER:
					case BT_MINDPROBE:
					case BT_PSIAMP:
					case BT_FLARE:
						if (itRule->getBigSprite() > -1 // See also CraftEquipmentState cTor. Inventory also uses this "bigSprite" trick.
							&& itRule->isFixed() == false
							&& _gameSave->isResearched(itRule->getRequirements()) == true)
						{
							//Log(LOG_INFO) << ". . . item = " << i->first << " (" << i->second << ")";
							for (int
									j = 0;
									j != i->second;
									++j)
							{
								//Log(LOG_INFO) << ". . . . addItem() ToTile iter = " << (j + 1);
								item = new BattleItem(
													itRule,
													_battleSave->getCanonicalBattleId());
								item->setInventorySection(grdRule);
								_tileEquipt->addItem(item);
							}

							if (_isFakeInventory == false)
							{
								//Log(LOG_INFO) << ". . . . remove item from Stores";
								i = _base->getStorageItems()->getContents()->erase(i);
								break;
							}
						}
						// no break;
					case BT_NONE:
					case BT_CORPSE:
					case BT_FUEL:
						++i;
				}
			}
		}
		//Log(LOG_INFO) << "";
		//Log(LOG_INFO) << ". . addBaseBaseItems DONE, add BaseCraftItems";

		for (std::vector<Craft*>::const_iterator								// Add items that are in the Crafts at the Base.
				i = _base->getCrafts()->begin();
				i != _base->getCrafts()->end();
				++i)
		{
			//Log(LOG_INFO) << ". . . check if Craft at base";
			if ((*i)->getCraftStatus() != CS_OUT)
			{
				//Log(LOG_INFO) << ". . . Craft IS at base";
				for (std::map<std::string, int>::const_iterator
						j = (*i)->getCraftItems()->getContents()->begin();
						j != (*i)->getCraftItems()->getContents()->end();
						++j)
				{
					//Log(LOG_INFO) << ". . . . item = " << j->first << " (" << j->second << ")";
					for (int
							k = 0;
							k != j->second;
							++k)
					{
						//Log(LOG_INFO) << ". . . . . addItem() ToTile iter = " << (k + 1);
						item = new BattleItem(
											_rules->getItemRule(j->first),
											_battleSave->getCanonicalBattleId());
						item->setInventorySection(grdRule);
						_tileEquipt->addItem(item);
					}
				}
			}
		}
		//Log(LOG_INFO) << ". . addBaseCraftItems DONE";
	}
	//Log(LOG_INFO) << ". addItem(s) DONE";


	// kL_note: ALL ITEMS STAY ON THE GROUNDTILE, _tileEquipt,
	// IN THAT INVENTORY(vector) UNTIL EVERYTHING IS EQUIPPED & LOADED. Then
	// the inventory-tile is cleaned up at the end of this function....
	//Log(LOG_INFO) << "";
	//Log(LOG_INFO) << ". placeLayout Pt I";
	for (std::vector<BattleItem*>::const_iterator								// Equip soldiers based on equipment-layout Part I.
			i = _tileEquipt->getInventory()->begin();
			i != _tileEquipt->getInventory()->end();
			++i)
	{
		//Log(LOG_INFO) << ". . try to place NON-AMMO tileItem = " << (*i)->getRules()->getType();
		if ((*i)->getInventorySection() == grdRule
			&& (*i)->getRules()->getBattleType() != BT_AMMO)
		{
			//Log(LOG_INFO) << ". . place " << (*i)->getRules()->getType();
			placeLayout(*i);
		}
	}

	//Log(LOG_INFO) << "";
	//Log(LOG_INFO) << ". placeLayout Pt II";
	for (std::vector<BattleItem*>::const_iterator								// Equip soldiers based on equipment-layout Part II.
			i = _tileEquipt->getInventory()->begin();
			i != _tileEquipt->getInventory()->end();
			++i)
	{
		//Log(LOG_INFO) << ". . try to place AMMO tileItem = " << (*i)->getRules()->getType();
		if ((*i)->getInventorySection() == grdRule
			&& (*i)->getRules()->getBattleType() == BT_AMMO)
		{
			//Log(LOG_INFO) << ". . . place " << (*i)->getRules()->getType();
			placeLayout(*i);
		}
	}
	//Log(LOG_INFO) << ". placeLayout all DONE";

	//Log(LOG_INFO) << "";
	//Log(LOG_INFO) << ". Load Ground Weapons...";
	for (std::vector<BattleItem*>::const_iterator								// Load ground weapons.
			i = _tileEquipt->getInventory()->begin();
			i != _tileEquipt->getInventory()->end();
			++i)
	{
		if ((*i)->getInventorySection() == grdRule
			&& (*i)->getRules()->getCompatibleAmmo()->empty() == false)
//			&& (*i)->getRules()->isFixed() == false
//			&& (*i)->getAmmoItem() == nullptr
//			&& (*i)->selfPowered() == false
//			&& (*i)->getRules()->getBattleType() == BT_FIREARM
//			&& (*i)->getRules()->getBattleType() == BT_MELEE
		{
			//Log(LOG_INFO) << ". . load " << (*i)->getRules()->getType();
			loadGroundWeapon(*i);
		}
	}

	//Log(LOG_INFO) << "";
	//Log(LOG_INFO) << ". Clean up";
	for (std::vector<BattleItem*>::const_iterator								// Clean up placed items.
			i = _tileEquipt->getInventory()->begin();
			i != _tileEquipt->getInventory()->end();
			)
	{
		(*i)->setProperty();
		_itemList->push_back(*i);
		if ((*i)->getInventorySection() != grdRule)
		{
			//Log(LOG_INFO) << ". . erase tileItem : " << (*i)->getRules()->getType();
			i = _tileEquipt->getInventory()->erase(i);
		}
		else
		{
			//Log(LOG_INFO) << ". . add to battleSave : " << (*i)->getRules()->getType();
			++i;
		}
	}
	//Log(LOG_INFO) << "BattlescapeGenerator::deployXcom() EXIT";
}

/**
 * Constructs a vector of Vehicles that can participate in BaseDefense tacticals.
 * @param vehicles - reference to a vector of pointers to Vehicles
 */
void BattlescapeGenerator::prepBaseVehicles(std::vector<Vehicle*>& vehicles) // private.
{
	for (std::vector<Craft*>::const_iterator // add Vehicles that are in Crafts at the Base.
			i = _base->getCrafts()->begin();
			i != _base->getCrafts()->end();
			++i)
	{
		if ((*i)->getCraftStatus() != CS_OUT)
		{
			for (std::vector<Vehicle*>::const_iterator
					j = (*i)->getVehicles()->begin();
					j != (*i)->getVehicles()->end();
					++j)
			{
				vehicles.push_back(*j);
			}
		}
	}

	const RuleItem* itRule;
	int quadrants;

	ItemContainer* const baseStores (_base->getStorageItems());
	for (std::map<std::string, int>::const_iterator // add Vehicles in Base's stores.
			i = baseStores->getContents()->begin();
			i != baseStores->getContents()->end();
			)
	{
		itRule = _rules->getItemRule(i->first);
		if (itRule->isFixed() == true)
		{
			quadrants = _rules->getArmor(_rules->getUnitRule(i->first)->getArmorType())->getSize();
			quadrants *= quadrants;

			if (itRule->getFullClip() < 1)
			{
				for (int
						j = 0;
						j != i->second;
						++j)
				{
					vehicles.push_back(new Vehicle(
												itRule,
												itRule->getFullClip(),
												quadrants));
				}
				baseStores->removeItem(i->first, i->second);

				i = baseStores->getContents()->begin();
				continue;
			}
			else
			{
				const std::string type (itRule->getCompatibleAmmo()->front());
				const int
					clipsRequired (itRule->getFullClip()),
					baseClips (baseStores->getItemQuantity(type)),

					tanks = std::min(i->second,
									 baseClips / clipsRequired);
				if (tanks != 0)
				{
					for (int
							j = 0;
							j != tanks;
							++j)
					{
						vehicles.push_back(new Vehicle(
													itRule,
													clipsRequired,
													quadrants));
					}
					baseStores->removeItem(i->first, tanks);
					baseStores->removeItem(type, clipsRequired * tanks);

					i = baseStores->getContents()->begin();
					continue;
				}
			}
		}
		++i;
	}
}

/**
 * Converts a Vehicle to a player-support-unit that's about to be added to the
 * Battlescape.
 * @note Sets the correct turret depending on its item-type [and adds auxilliary
 * weapons if any - not at present, kL].
 * @param vehicle - pointer to Vehicle
 * @return, pointer to the spawned unit; nullptr if unable to instantiate and equip itself
 */
BattleUnit* BattlescapeGenerator::convertVehicle(Vehicle* const vehicle) // private.
{
	std::string type (vehicle->getRules()->getType());				// Convert this item-type ...
	RuleUnit* const unitRule (_rules->getUnitRule(type));			// ... to a unitRule. tata!

	BattleUnit* const supportUnit (addPlayerUnit(new BattleUnit(	// then add Vehicle as a unit.
															unitRule,
															FACTION_PLAYER,
															_unitSequence++,
															_rules->getArmor(unitRule->getArmorType()))));
	if (supportUnit != nullptr)
	{
		supportUnit->setTurretType(vehicle->getRules()->getTurretType());

		BattleItem* const weapon (new BattleItem(					// add Vehicle as a weapon-item and assign the unit itself as the owner of the weapon.
											_rules->getItemRule(type),
											_battleSave->getCanonicalBattleId()));
		if (placeGeneric(weapon, supportUnit) == false)
		{
			Log(LOG_WARNING) << "bGen:convertVehicle() Could not add " << type;

			--_unitSequence;
			delete weapon;
			delete supportUnit;

			return nullptr;
		}

		if (weapon->getRules()->getFullClip() > 0)
		{
			type = vehicle->getRules()->getCompatibleAmmo()->front();
			BattleItem* const load (new BattleItem(					// add load and assign the weapon as its owner.
												_rules->getItemRule(type),
												_battleSave->getCanonicalBattleId()));
			if (placeGeneric(load, supportUnit) == false)
			{
				Log(LOG_WARNING) << "bGen:convertVehicle() Could not add " << type << " to " << vehicle->getRules()->getType();

				--_unitSequence;
				delete load;
				delete weapon;
				delete supportUnit;

				return nullptr;
			}

			load->setAmmoQuantity(vehicle->getLoad());
		}

/*		if (unitRule->getBuiltInWeapons().empty() == false) // add item(builtInWeapon) -- what about ammo
		{
			for (std::vector<std::string>::const_iterator
					i = unitRule->getBuiltInWeapons().begin();
					i != unitRule->getBuiltInWeapons().end();
					++i)
			{
				RuleItem* const itRule (_rules->getItemRule(*i));
				if (itRule != nullptr)
				{
					weapon = new BattleItem(itRule, _battleSave->getCanonicalBattleId());
					if (placeGeneric(weapon, supportUnit) == false)
					{
						Log(LOG_WARNING) << "bGen:convertVehicle() Could not add " << itRule->getType() << " to " << vhclType;
						delete weapon;
					}
				}
			}
		} */
	}
	return supportUnit;
}

/**
 * Adds a player-unit to the Battlescape and places it at an unallocated
 * spawn-point.
 * @note Spawn-points are Tiles in case of an xCom Craft that landed or they are
 * resource-defined mapnodes if there is no Craft.
 * @param unit - pointer to an xCom BattleUnit
 * @return, pointer to the spawned unit if successful else nullptr
 */
BattleUnit* BattlescapeGenerator::addPlayerUnit(BattleUnit* const unit) // private.
{
	//Log(LOG_INFO) << "bsg:addPlayerUnit()";
	if ((_craft == nullptr || _craftDeployed == false)	// (_missionType == "STR_ALIEN_BASE_ASSAULT"
		&& _isFakeInventory == false)					// || _missionType == "STR_MARS_THE_FINAL_ASSAULT")
	{													// ^ taken care of in MapScripting.
		//Log(LOG_INFO) << ". no Craft";
		const Node* const node (_battleSave->getSpawnNode(NR_XCOM, unit));
		if (node != nullptr)
		{
			//Log(LOG_INFO) << ". . spawnNode valid";
			_battleSave->setBattleInventory(_tileEquipt = _battleSave->getTile(node->getPosition()));

			_battleSave->setUnitPosition(unit, node->getPosition());
			unit->setUnitDirection(RNG::generate(0,7));

			_unitList->push_back(unit);
			return unit;
		}
		//Log(LOG_INFO) << ". . spawnNode NOT valid - try not baseDefense";

		if (_battleSave->getTacType() != TCT_BASEDEFENSE // quah
			&& placeUnitNearFaction(unit) == true)
		{
			//Log(LOG_INFO) << ". . . placeUnitNearFaction() TRUE";
			_battleSave->setBattleInventory(_tileEquipt = _battleSave->getTile(unit->getPosition()));

			unit->setUnitDirection(RNG::generate(0,7));

			_unitList->push_back(unit);
			return unit;
		}
	}
	else if (_craft != nullptr // Transport Craft deployments (Lightning & Avenger)
		&& _craft->getRules()->getCraftDeployment().empty() == false
		&& _isFakeInventory == false)
	{
		//Log(LOG_INFO) << ". Craft valid - use Deployment";
		bool canPlace;
		int unitSize (unit->getArmor()->getSize());

		for (std::vector<std::vector<int>>::const_iterator
				i = _craft->getRules()->getCraftDeployment().begin();
				i != _craft->getRules()->getCraftDeployment().end();
				++i)
		{
			//Log(LOG_INFO) << ". getCraftDeployment()";
			const Position pos (Position(
									(*i)[0u] + (_craftPos.x * 10),
									(*i)[1u] + (_craftPos.y * 10),
									(*i)[2u] +  _craftZ));
			canPlace = true;
			for (int
					x = 0;
					x != unitSize && canPlace == true;
					++x)
			{
				for (int
						y = 0;
						y != unitSize && canPlace == true;
						++y)
				{
					canPlace = canPlacePlayerUnit(_battleSave->getTile(pos + Position(x,y,0)));
				}
			}

			if (canPlace == true
				&& _battleSave->setUnitPosition(unit, pos) == true)
			{
				//Log(LOG_INFO) << ". canPlacePlayerUnit()";
				//Log(LOG_INFO) << ". setUnitPosition()";
				if (_tileEquipt == nullptr)
					_battleSave->setBattleInventory(_tileEquipt = _battleSave->getTile(pos));

				unit->setUnitDirection((*i)[3u]);

				_unitList->push_back(unit);
				return unit;
			}
		}
	}
	else // mission w/ transport craft that does not have ruleset Deployments. Or it's a craft/base Equip.
	{
		//Log(LOG_INFO) << ". baseEquip OR Craft w/out Deployment rule";
		Tile* tile;
		int supportOrder (0);
		for (size_t
				i = 0u;
				i != _battleSave->getMapSizeXYZ();
				++i)
		{
			if (canPlacePlayerUnit(tile = _battleSave->getTiles()[i]) == true)
			{
				if (unit->getGeoscapeSoldier() == nullptr)
				{
					if ((unit->getArmor()->getSize() == 1
							|| tile->getPosition().x == _tileEquipt->getPosition().x)
						&& ++supportOrder == 3
						&& _battleSave->setUnitPosition(
													unit,
													tile->getPosition()) == true)
					{
						if (_tileEquipt == nullptr)
							_battleSave->setBattleInventory(_tileEquipt = tile);

						_unitList->push_back(unit);
						return unit;
					}
				}
				else if (_battleSave->setUnitPosition(
													unit,
													tile->getPosition()) == true)
				{
					if (_tileEquipt == nullptr)
						_battleSave->setBattleInventory(_tileEquipt = tile);

					_unitList->push_back(unit);
					return unit;
				}
			}
		}
	}

	delete unit; // fallthrough if above fails.
	return nullptr;
}

/**
 * Checks if a soldier/support can be placed on a given Tile.
 * @note Helper for addPlayerUnit().
 * @param tile - pointer to a Tile
 * @return, true if unit can be placed there
 */
bool BattlescapeGenerator::canPlacePlayerUnit(Tile* const tile) // private.
{
	if (tile != nullptr												// is a tile
		&& tile->getMapData(O_FLOOR) != nullptr						// has a floor
		&& tile->getMapData(O_FLOOR)->getTileType() == START_POINT	// is a 'start point', ie. cargo tile
		&& tile->getMapData(O_OBJECT) == nullptr					// no object content
		&& tile->getMapData(O_FLOOR)->getTuCostPart(MT_WALK) < 255	// is walkable.
		&& tile->getTileUnit() == nullptr)							// and no unit on Tile.
	{
		return true;
	}
	return false;
}

/**
 * Loads a weapon on the equipment Tile.
 * @note Helper for deployXcom().
 * @param item - pointer to a BattleItem
 */
void BattlescapeGenerator::loadGroundWeapon(BattleItem* const item) // private.
{
	const RuleInventory* const grdRule (_rules->getInventoryRule(ST_GROUND));
	for (std::vector<BattleItem*>::const_iterator
			i = _tileEquipt->getInventory()->begin();
			i != _tileEquipt->getInventory()->end();
			++i)
	{
		if ((*i)->getInventorySection() == grdRule
			&& item->setAmmoItem(*i) == 0)
		{
			//Log(LOG_INFO) << ". . . " << item->getRules()->getType() << " loaded w/ " << (*i)->getRules()->getType();
			return;
		}
	}
}

/**
 * Places an item on an xCom Soldier based on his/her equipment-layout.
 * @param item - pointer to a BattleItem
 */
void BattlescapeGenerator::placeLayout(BattleItem* const item) // private.
{
	//Log(LOG_INFO) << "placeLayout item " << item->getRules()->getType();
	const RuleInventory* const grdRule (_rules->getInventoryRule(ST_GROUND));
	std::string loadType;

	bool loaded;
	for (std::vector<BattleUnit*>::const_iterator
			i = _unitList->begin();
			i != _unitList->end();
			++i)
	{
		if ((*i)->getGeoscapeSoldier() != nullptr)
		{
			//Log(LOG_INFO) << ". on " << (*i)->getGeoscapeSoldier()->getName().c_str();
			const std::vector<SoldierLayout*>* const layout ((*i)->getGeoscapeSoldier()->getLayout());
			for (std::vector<SoldierLayout*>::const_iterator
					j = layout->begin();
					j != layout->end();
					++j)
			{
				//Log(LOG_INFO) << ". . iterate layout items";
				if ((*j)->getItemType() == item->getRules()->getType()
					&& (*i)->getItem(
								(*j)->getLayoutSection(),
								(*j)->getSlotX(),
								(*j)->getSlotY()) == nullptr)
				{
					//Log(LOG_INFO) << ". . . type match!";
					loadType = (*j)->getAmmoType();
					if (loadType.empty() == true)
						loaded = true;
					else
					{
						loaded = false;
						for (std::vector<BattleItem*>::const_iterator
								k = _tileEquipt->getInventory()->begin();
								k != _tileEquipt->getInventory()->end();
								++k)
						{
							if ((*k)->getInventorySection() == grdRule
								&& (*k)->getRules()->getType() == loadType
								&& item->setAmmoItem(*k) == 0)
							{
								loaded = true;
								break;
							}
						}
					}

					if (loaded == true)
					{
						//Log(LOG_INFO) << ". . . . PLACED";
						item->changeOwner(*i);
						item->setInventorySection(_rules->getInventory((*j)->getLayoutSection()));
						item->setSlotX((*j)->getSlotX());
						item->setSlotY((*j)->getSlotY());

						switch (item->getRules()->getBattleType())
						{
							case BT_GRENADE:
							case BT_PROXYGRENADE:
							case BT_FLARE:
								if (Options::includePrimeStateInSavedLayout == true)
									item->setFuse((*j)->getFuse());
						}
					}
					return;
				}
			}
		}
	}
}

/**
 * Sets xCom soldiers' combat clothing style - spritesheets & paperdolls.
 * @note Uses EqualTerms v1 graphics to replace stock resources. Affects soldiers
 * wearing pyjamas (STR_ARMOR_NONE_UC) only. This is done by switching in/out
 * equivalent Armors.
 */
void BattlescapeGenerator::setTacticalSprites() const // private.
{
	RuleArmor* const armorRule (_rules->getArmor(_terrainRule->getPyjamaType()));

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
 * Places an item on an xCom soldier (auto-equip ONLY). kL_note: I don't use that part.
 * Or to an xCom tank, also adds items & terrorWeapons to aLiens, deployAliens()!
 * @param item - pointer to a BattleItem
 * @param unit - pointer to a BattleUnit
 * @return, true if item was placed
 */
bool BattlescapeGenerator::placeGeneric( // private.
		BattleItem* const item,
		BattleUnit* const unit) const
{
	int placed (0);

	const RuleInventory
		* const rhRule (_rules->getInventoryRule(ST_RIGHTHAND)),
		* const lhRule (_rules->getInventoryRule(ST_LEFTHAND));
	BattleItem
		* const rhWeapon (unit->getItem(ST_RIGHTHAND)),
		* const lhWeapon (unit->getItem(ST_LEFTHAND));

	const RuleItem* const itRule (item->getRules());
	if (itRule->isFixed() == true)
	{
		item->setInventorySection(rhRule);
		placed = 1;
/*		if (rhWeapon == nullptr) // not needed at present.
		{
			item->setInventorySection(rhRule);
			placed = 1;
		}
		else if (lhWeapon == nullptr)
		{
			item->setInventorySection(lhRule);
			placed = 1;
		} */
	}
	else
	{
		switch (itRule->getBattleType())
		{
			case BT_FIREARM:
			case BT_MELEE:
				if (rhWeapon == nullptr)
				{
					item->setInventorySection(rhRule);
					placed = 1;
					break;
				}

				if (lhWeapon == nullptr)
				{
					item->setInventorySection(lhRule);
					placed = 1;
					break;
				} // no break.
			case BT_AMMO:
				if (rhWeapon != nullptr
					&& rhWeapon->getAmmoItem() == nullptr
					&& rhWeapon->setAmmoItem(item) == 0)
				{
//					item->setInventorySection(rhRule);
					placed = 2;
					break;
				}

				if (lhWeapon != nullptr
					&& lhWeapon->getAmmoItem() == nullptr
					&& lhWeapon->setAmmoItem(item) == 0)
				{
//					item->setInventorySection(lhRule);
					placed = 2;
					break;
				} // no break.

			default:
			{
				std::vector<const RuleInventory*> inTypes;
				inTypes.push_back(_rules->getInventoryRule(ST_BELT));
				inTypes.push_back(_rules->getInventoryRule(ST_BACKPACK));

				for (std::vector<const RuleInventory*>::const_iterator
						i = inTypes.begin();
						i != inTypes.end() && placed == 0;
						++i)
				{
					for (std::vector<RuleSlot>::const_iterator
							j = (*i)->getSlots()->begin();
							j != (*i)->getSlots()->end() && placed == 0;
							++j)
					{
						if (Inventory::isOverlap(
											unit, item, *i,
											j->x, j->y) == false
							&& (*i)->fitItemInSlot(itRule, j->x, j->y) == true)
						{
							item->setInventorySection(*i);
							item->setSlotX(j->x);
							item->setSlotY(j->y);
							placed = 1;
						}
					}
				}
			}
		}
	}

	switch (placed)
	{
		case 1:
			item->changeOwner(unit); // no break.
		case 2:
			_itemList->push_back(item);
			return true;
	}
	return false; // If not placed the item will be deleted.
}

/**
 * Deploys the aLiens according to the RuleAlienDeployment rule.
 * @param ruleDeploy - pointer to the RuleAlienDeployment rule
 */
void BattlescapeGenerator::deployAliens(const RuleAlienDeployment* const ruleDeploy) // private.
{
	int elapsed (_gameSave->getMonthsPassed());
	if (elapsed != -1)
	{
		const int levelHigh (static_cast<int>(_rules->getAlienItemLevels().size()) - 1);
		if (elapsed > levelHigh)
			elapsed = levelHigh;

		if (ruleDeploy->getRace().empty() == false) // race re-defined by Deployment if there is one.
			_alienRace = ruleDeploy->getRace();
	}
	else
		elapsed = _alienItemLevel;

	const RuleAlienRace* const raceRule (_game->getRuleset()->getAlienRace(_alienRace));
	if (raceRule == nullptr)
	{
		throw Exception("bGen:deployAliens() No rule for " + _alienRace);
	}


	std::string aLien;
	bool outside;
	int qty;
	size_t level;

	const RuleItem* itRule;
	BattleItem* item;
	RuleUnit* unitRule;
	BattleUnit* unit;

	for (std::vector<DeploymentData>::const_iterator
			i = ruleDeploy->getDeploymentData()->begin();
			i != ruleDeploy->getDeploymentData()->end();
			++i)
	{
		aLien = raceRule->getMember((*i).alienRank);

		switch (_gameSave->getDifficulty())
		{
			case DIFF_BEGINNER:
			case DIFF_EXPERIENCED:
				qty = (*i).lowQty
					+ RNG::generate(0, (*i).dQty);
				break;

			case DIFF_VETERAN:
			case DIFF_GENIUS:
				qty = (*i).lowQty
					+ (((*i).highQty - (*i).lowQty) >> 1u)
					+ RNG::generate(0, (*i).dQty);
				break;

			default: // void g++ warning.
			case DIFF_SUPERHUMAN:
				qty = (*i).highQty
					+ RNG::generate(0, (*i).dQty);
		}

		qty += RNG::generate(0, (*i).extraQty);

		if (_base != nullptr && _base->getDefenseResult() != 0)
			qty = std::max(qty >> 1u,
						   qty - (qty * _base->getDefenseResult() / 100));

		for (int
				j = 0;
				j != qty;
				++j)
		{
			if (_ufo != nullptr)
				outside = RNG::percent((*i).pctOutsideUfo);
			else
				outside = false;

			unitRule = _rules->getUnitRule(aLien);
			unit = addAlien(
						unitRule,
						(*i).alienRank,
						outside);

			if (unit != nullptr)
			{
				// Built in weapons: the unit has this weapon regardless of loadout or what have you.
//				if (unitRule->getBuiltInWeapons().empty() == false)
//				{
//					for (std::vector<std::string>::const_iterator
//							j = unitRule->getBuiltInWeapons().begin();
//							j != unitRule->getBuiltInWeapons().end();
//							++j)
//					{
//						if ((itRule = _rules->getItemRule(*j)) != nullptr)
//						{
//							item = new BattleItem(itRule, _battleSave->getCanonicalBattleId());
//							if (placeGeneric(item, unit) == false)
//							{
//								Log(LOG_WARNING) << "bGen:deployAliens() Could not add " << itRule->getType() << " to " << unit->getType();
//								delete item;
//							}
//						}
//					}
//				}

				if (unitRule->isLivingWeapon() == true)
				{
					const std::string terrorWeapon (unitRule->getRace().substr(4) + "_WEAPON");
					if ((itRule = _rules->getItemRule(terrorWeapon)) != nullptr)
					{
						item = new BattleItem( // terror aLiens add their weapons
											itRule,
											_battleSave->getCanonicalBattleId());
						if (placeGeneric(item, unit) == false)
						{
							Log(LOG_WARNING) << "bGen:deployAliens() [1] Could not add " << itRule->getType() << " to " << unit->getType();
							delete item;
						}
//						else // -> there are no aLien turret-types at present. I don't do MiB. i do aLiens.
//							unit->setTurretType(item->getRules()->getTurretType());
					}
				}
				else
				{
					if ((*i).itemSets.size() == 0)
					{
						throw Exception("bGen:deployAliens() No itemSets defined.");
					}

					level = static_cast<size_t>(_rules->getAlienItemLevels().at(static_cast<size_t>(elapsed)).at(RNG::generate(0,9)));
					if (level > (*i).itemSets.size() - 1u)
						level = (*i).itemSets.size() - 1u;
					// Relax item level requirements
					// <- Yankes; https://github.com/Yankes/OpenXcom/commit/4c252470aa2e261b0f449a56aaea5d5b0cb2229c
//					if (level > (*i).itemSets.size() - 1u)
//					{
//						std::ostringstream ststr;
//						ststr	<< "Unit generator encountered an error: not enough item sets defined, expected: "
//								<< (level + 1) << " found: " << (*i).itemSets.size();
//						throw Exception(ststr.str());
//					}

					for (std::vector<std::string>::const_iterator
							type = (*i).itemSets.at(level).items.begin();
							type != (*i).itemSets.at(level).items.end();
							++type)
					{
						if ((itRule = _rules->getItemRule(*type)) != nullptr)
						{
							item = new BattleItem( // aLiens add items
												itRule,
												_battleSave->getCanonicalBattleId());
							if (placeGeneric(item, unit) == false)
							{
								Log(LOG_WARNING) << "bGen:deployAliens() [2] Could not add " << itRule->getType() << " to " << unit->getType();
								delete item;
							}
						}
					}
				}
			}
		}
	}

	if (_base != nullptr)
		_base->clearDefenseResult();
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
	const DifficultyLevel diff (_gameSave->getDifficulty());
	BattleUnit* const unit (new BattleUnit(
										unitRule,
										FACTION_HOSTILE,
										_unitSequence++,
										_rules->getArmor(unitRule->getArmorType()),
										diff,
										_gameSave->getMonthsPassed()));

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

	const size_t ranks_2 ((sizeof(Node::nodeRank) / sizeof(Node::nodeRank[0u][0u]))
						/ (sizeof(Node::nodeRank) / sizeof(Node::nodeRank[0u])));
	if (node == nullptr) // ie. if not spawning on a Civ-Scout node
	{
		for (size_t
				i = 0u;
				i != ranks_2 && node == nullptr; // =8, 2nd dimension of nodeRank[][], ref Node.cpp
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
		|| placeUnitNearFaction(unit) == true)
	{
		unit->setAIState(new AlienBAIState(
										_battleSave,
										unit,
										node));
		unit->setRankInt(aLienRank);

		const Position posCraft (_unitList->at(0u)->getPosition()); // aLiens face Craft
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

		unit->preBattleMotion();

		_unitList->push_back(unit);
	}
	else
	{
		Log(LOG_WARNING) << "bGen:addAlien() Could not add " << unit->getType();
		--_unitSequence;
		delete unit;
		return nullptr;
	}

	return unit;
}

/**
 * Places a specified BattleUnit near a friendly unit.
 * @param unit - pointer to the BattleUnit in question
 * @return, true if @a unit was successfully placed
 */
bool BattlescapeGenerator::placeUnitNearFaction(BattleUnit* const unit) // private.
{
	if (unit != nullptr && _unitList->empty() == false)
	{
		const BattleUnit* ally;
		int t (100);
		while (t--)
		{
			ally = _unitList->at(RNG::pick(_unitList->size()));
			if (ally->getFaction() == unit->getFaction()
				&& ally->getPosition() != Position(-1,-1,-1)
				&& ally->getArmor()->getSize() >= unit->getArmor()->getSize())
			{
				if (_battleSave->placeUnitNearPosition(
													unit,
													ally->getPosition(),
													ally->getArmor()->getSize() == 2) == true)
				{
					return true;
				}
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
		const int qty (std::max(1,
								RNG::generate(civilians >> 1u,
											  civilians)));
		for (int
				i = 0;
				i != qty;
				++i)
		{
			addCivilian(_rules->getUnitRule(_terrainRule->getCivilianTypes()
						.at(RNG::pick(_terrainRule->getCivilianTypes().size()))));
		}
	}
}

/**
 * Adds a civilian to the game and places him on a free spawnpoint.
 * @param unitRule - pointer to the Unit rule that holds info about civilians
 */
void BattlescapeGenerator::addCivilian(RuleUnit* const unitRule) // private.
{
	BattleUnit* const unit (new BattleUnit(
										unitRule,
										FACTION_NEUTRAL,
										_unitSequence++,
										_rules->getArmor(unitRule->getArmorType())));

	Node* const node (_battleSave->getSpawnNode(NR_SCOUT, unit));
	if ((node != nullptr
			&& _battleSave->setUnitPosition(unit, node->getPosition()) == true)
		|| placeUnitNearFaction(unit) == true)
	{
		unit->setAIState(new CivilianBAIState(_battleSave, unit, node));
		unit->setUnitDirection(RNG::generate(0,7));
		unit->preBattleMotion();

		_unitList->push_back(unit);
	}
	else
	{
		Log(LOG_WARNING) << "bGen:addCivilian() Could not add " << unit->getType();
		--_unitSequence;
		delete unit;
	}
}

/**
 * Loads a MAP file into the tiles of the BattleGame.
 * @param block				- pointer to MapBlock
 * @param offset_x			- Mapblock offset in x-direction
 * @param offset_y			- Mapblock offset in y-direction
 * @param terraRule			- pointer to RuleTerrain
 * @param dataSetIdOffset	- (default 0)
 * @param revealed			- true if this MapBlock is revealed (eg. landingsite of the Skyranger) (default false)
 * @param craft				- true if xCom Craft has landed on the MAP (default false)
 * @return, height of the loaded Mapblock (needed for spawn-point calculation)
 * @sa http://www.ufopaedia.org/index.php?title=MAPS
 * @note Y-axis is in reverse order.
 */
int BattlescapeGenerator::loadMAP( // private.
		MapBlock* const block,
		int offset_x,
		int offset_y,
		const RuleTerrain* const terraRule,
		int dataSetIdOffset,
		bool revealed,
		bool craft)
{
	std::ostringstream file;
	file << "MAPS/" << block->getType() << ".MAP";
	std::ifstream ifstr (
						CrossPlatform::getDataFile(file.str()).c_str(),
						std::ios::in | std::ios::binary);
	if (ifstr.fail() == true)
	{
		throw Exception("bGen:loadMAP() " + file.str() + " not found.");
	}

	char array_Map[3u];
	ifstr.read(
			reinterpret_cast<char*>(&array_Map),
			sizeof(array_Map));
	const int
		size_x (static_cast<int>(array_Map[1u])), // note X-Y switch!
		size_y (static_cast<int>(array_Map[0u])), // note X-Y switch!
		size_z (static_cast<int>(array_Map[2u]));

	block->setSizeZ(size_z);

	std::ostringstream oststr;
	if (size_z > _battleSave->getMapSizeZ())
	{
		oststr << "bGen:loadMAP() Height of " + file.str() + " is too big for the mission. Block is "
			   << size_z << " expected: " << _battleSave->getMapSizeZ();
		throw Exception(oststr.str());
	}
	else if (size_x != block->getSizeX() || size_y != block->getSizeY())
	{
		oststr << "bGen:loadMAP() Map block is not of the size specified. " + file.str() + " is "
			   << size_x << "x" << size_y << " expected: " << block->getSizeX() << "x" << block->getSizeY();
		throw Exception(oststr.str());
	}

	int
		x (offset_x),
		y (offset_y),
		z (size_z - 1);

	for (int // check if there is already a layer - if so, move Z up
			i = _mapsize_z - 1;
			i != 0;
			--i)
	{
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
		throw Exception("bGen:loadMAP() Craft/UFO map is too tall.");
	}

	unsigned char array_Parts[Tile::PARTS_TILE];
	unsigned partId;

	bool revealDone;
	while (ifstr.read(
					reinterpret_cast<char*>(&array_Parts),
					sizeof(array_Parts)))
	{
		revealDone = false;

		for (size_t
				i = 0u;
				i != Tile::PARTS_TILE;
				++i)
		{
			partId = static_cast<unsigned>(array_Parts[i]);

			// Remove natural terrain that is inside Craft or Ufo.
			if (i != 0u						// not if it's a floor since Craft/Ufo part will overwrite it anyway
				&& partId == 0u				// and only if no Craft/Ufo part would overwrite the part
				&& array_Parts[0u] != 0u)	// but only if there *is* a floor-part to the Craft/Ufo so it would (have) be(en) inside the Craft/Ufo
			{
				_battleSave->getTile(Position(x,y,z))->setMapData(nullptr,-1,-1, static_cast<MapDataType>(i));
			}

			// Then overwrite previous terrain with Craft or Ufo terrain.
			// nb. See sequence of map-loading in generateMap() (1st terrain, 2nd Ufo, 3rd Craft) <- preMapScripting.
			if (partId > 0u)
			{
				unsigned int dataId (partId);
				int dataSetId (dataSetIdOffset);

				MapData* const data (terraRule->getMapData(&dataId, &dataSetId));
//				if (dataSetIdOffset > 0) // ie: ufo or craft.
//					_battleSave->getTile(Position(x,y,z))->setMapData(nullptr,-1,-1,3); // erase content-object

				_battleSave->getTile(Position(x,y,z))->setMapData(
																data,
																static_cast<int>(dataId),
																dataSetId,
																static_cast<MapDataType>(i));
			}

			// If the part is not a floor and is empty, remove it; this prevents growing grass in UFOs.
			// note: And outside UFOs. so remark it
//			if (part == 3 && partId == 0)
//				_battleSave->getTile(Position(x,y,z))->setMapData(nullptr,-1,-1, part);

			if (craft == true // Reveal only tiles inside the Craft.
				&& partId != 0u
				&& z != _craftZ)
			{
				revealDone = true;
				_battleSave->getTile(Position(x,y,z))->setRevealed();
			}
		}

//		if (craft && _craftZ == z)
//		{
//			for (int z2 = _battleSave->getMapSizeZ() - 1; z2 >= _craftZ; --z2)
//				_battleSave->getTile(Position(x,y,z2))->setDiscovered(true, 2);
//		}

		if (revealDone == false)
			_battleSave->getTile(Position(x,y,z))->setRevealed(
															ST_CONTENT,
															revealed == true || block->isFloorRevealed(z) == true);
		if (++x == size_x + offset_x)
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

	if (ifstr.eof() == false)
	{
		throw Exception("bGen:loadMAP() Invalid MAP file: " + file.str());
	}

	ifstr.close();

	if (_generateFuel == true) // if one of the mapBlocks has an items array defined, don't deploy fuel algorithmically
		_generateFuel = (block->getItems()->empty() == true);

	const RuleItem* itRule;
	BattleItem* item;
	for (std::map<std::string, std::vector<Position>>::const_iterator
			i = block->getItems()->begin();
			i != block->getItems()->end();
			++i)
	{
		itRule = _rules->getItemRule((*i).first);
		for (std::vector<Position>::const_iterator
				j = (*i).second.begin();
				j != (*i).second.end();
				++j)
		{
			item = new BattleItem(
								itRule,
								_battleSave->getCanonicalBattleId());
			item->setInventorySection(_rules->getInventoryRule(ST_GROUND));
			_battleSave->getTile((*j) + Position(
												offset_x,
												offset_y,
												0))->addItem(item);
			_itemList->push_back(item);
		}
	}
	return size_z;
}

/**
 * Loads an XCom format RMP file as battlefield Nodes.
 * @param block		- pointer to MapBlock
 * @param offset_x	- Mapblock offset in x-direction
 * @param offset_y	- Mapblock offset in y-direction
 * @param segment	- Mapblock segment
 * @sa http://www.ufopaedia.org/index.php?title=ROUTES
 */
void BattlescapeGenerator::loadRMP( // private.
		MapBlock* const block,
		int offset_x,
		int offset_y,
		int segment)
{
	char dataArray[24u];

	std::ostringstream file;
	file << "ROUTES/" << block->getType() << ".RMP";

	std::ifstream ifstr (
						CrossPlatform::getDataFile(file.str()).c_str(),
						std::ios::in | std::ios::binary);
	if (ifstr.fail() == true)
	{
		throw Exception("bGen:loadRMP() " + file.str() + " not found");
	}

	int
		pos_x,
		pos_y,
		pos_z,

		unitType,
		nodeRank,
		ptrlPriority,
		aLienTarget,
		spPriority,

		linkId,
		nodeVal (0);

	const int nodeOffset (static_cast<int>(_battleSave->getNodes()->size()));
	Node* node;
	Position pos;

	while (ifstr.read(
					reinterpret_cast<char*>(&dataArray),
					sizeof(dataArray)))
	{
		pos_x = static_cast<int>(dataArray[1u]); // note: Here is where x-y values get reversed
		pos_y = static_cast<int>(dataArray[0u]); // vis-a-vis values in .RMP files vs. loaded values.
		pos_z = static_cast<int>(dataArray[2u]);

		if (   pos_x > -1 && pos_x < block->getSizeX()
			&& pos_y > -1 && pos_y < block->getSizeY()
			&& pos_z > -1 && pos_z < _mapsize_z)
		{
			pos = Position(
						offset_x + pos_x,
						offset_y + pos_y,
						block->getSizeZ() - pos_z - 1);

			unitType		= static_cast<int>(dataArray[19u]); // -> Any=0; Flying=1; Small=2; FlyingLarge=3; Large=4
			nodeRank		= static_cast<int>(dataArray[20u]);
			ptrlPriority	= static_cast<int>(dataArray[21u]);
			aLienTarget		= static_cast<int>(dataArray[22u]);
			spPriority		= static_cast<int>(dataArray[23u]);

			// TYPE_FLYING		= 0x01 -> ref Savegame/Node.h
			// TYPE_SMALL		= 0x02
			// TYPE_LARGEFLYING	= 0x04
			// TYPE_LARGE		= 0x08
			// TYPE_DANGEROUS	= 0x10 <- not in RMP file.
			if		(unitType == 3) unitType = 4; // for bit-wise
			else if (unitType == 4) unitType = 8;

			if (_battleSave->getTacType() != TCT_BASEDEFENSE)
				aLienTarget = 0; // ensure these get zero'd for nonBaseDefense battles; cf. Node::isAlienTarget()

			node = new Node(
						_battleSave->getNodes()->size(),
						pos,
						segment,
						unitType,
						nodeRank,
						ptrlPriority,
						aLienTarget,
						spPriority);

			for (size_t // create nodeLinks ->
					j = 0u;
					j != 5u; // Max links that a node can have.
					++j)
			{
				linkId = static_cast<int>(dataArray[(j * 3u) + 4u]); // <- 4[5,6],7[8,9],10[11,12],13[14,15],16[17,18] -> [unitType & distance of linked nodes are not used]

				if (linkId < 251) // do not offset special values; ie. links to N,S,E,West, or none.
					linkId += nodeOffset;
				else
					linkId -= 256;	// 255 -> -1 = unused
									// 254 -> -2 = north
									// 253 -> -3 = east
									// 252 -> -4 = south
									// 251 -> -5 = west

				std::vector<int>* const nodeLinks (node->getNodeLinks());
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
		else
		{
//			_error = true;
			Log(LOG_WARNING) << "bGen:loadRMP() Error in RMP file: " << file.str()
							 << " node #" << nodeVal << " is outside map boundaries at"
							 << " (" << pos_x << "," << pos_y << "," << pos_z << ")";
		}
		++nodeVal;
	}

	if (ifstr.eof() == false)
	{
		throw Exception("bGen:loadRMP() Invalid RMP file: " + file.str());
	}
	ifstr.close();
}

/**
 * Fill power sources with an alien fuel object.
 */
void BattlescapeGenerator::fuelPowerSources() // private.
{
	BattleItem* alienFuel;
	const Tile* tile;
	for (size_t
			i = 0u;
			i != _battleSave->getMapSizeXYZ();
			++i)
	{
		tile = _battleSave->getTiles()[i];
		if (tile->getMapData(O_OBJECT)
			&& tile->getMapData(O_OBJECT)->getTileType() == UFO_POWER_SOURCE)
		{
			alienFuel = new BattleItem(
									_rules->getItemRule(_rules->getAlienFuelType()),
									_battleSave->getCanonicalBattleId());
			alienFuel->setInventorySection(_rules->getInventoryRule(ST_GROUND));
			_battleSave->getTiles()[i]->addItem(alienFuel);
			_itemList->push_back(alienFuel);
		}
	}
}

/**
 * When a UFO crashes there is a chance for each aLien power-source to explode.
 */
void BattlescapeGenerator::explodePowerSources() // private.
{
	const Tile* tile;
	for (size_t
			i = 0u;
			i != _battleSave->getMapSizeXYZ();
			++i)
	{
		tile = _battleSave->getTiles()[i];
		if (tile->getMapData(O_OBJECT) != nullptr
			&& tile->getMapData(O_OBJECT)->getTileType() == UFO_POWER_SOURCE
			&& RNG::percent(80) == true)
		{
			double power (static_cast<double>(_ufo->getUfoDamagePct()));	// range: ~50+ to ~100-
			if (RNG::percent(static_cast<int>(power) >> 1u) == true)		// chance for full range Explosion (even if crash took low damage)
				power = RNG::generate(1.,100.);

			power *= RNG::generate(0.1,2.);
			power += std::pow(power, 2) / 160.;

			if (power > 0.5)
				_battleSave->getTileEngine()->explode(
													Position::toVoxelSpaceCentered(tile->getPosition(), 10),
													static_cast<int>(std::ceil(power)),
													DT_HE,
													21);
		}
	}

	while ((tile = _battleSave->getTileEngine()->checkForTerrainExplosives()) != nullptr)
		_battleSave->getTileEngine()->explode(
											Position::toVoxelSpaceCentered(tile->getPosition(), 10),
											tile->getExplosive(),
											DT_HE,
											tile->getExplosive() / 10);
}

/**
 * Creates a mini-battleSave for managing inventory from the Geoscape's
 * CraftEquip or BaseEquip and some other screens.
 * @note kids, don't try this at home! yer tellin' me.
 * @param craft		- pointer to Craft to handle
 * @param base		- pointer to Base to handle (default nullptr)
 * @param selUnitId	- soldier to display in battle pre-equip inventory (default 0)
 */
void BattlescapeGenerator::runInventory(
		Craft* const craft,
		Base* const base,
		size_t selUnitId)
{
	_isFakeInventory = true;

	if (craft != nullptr)
	{
		setCraft(craft);

		if (selUnitId != 0 && static_cast<int>(selUnitId) <= craft->getQtySoldiers())
		{
			size_t j (0u);
			for (std::vector<BattleUnit*>::const_iterator
					i = _unitList->begin();
					i != _unitList->end();
					++i)
			{
				if (++j == selUnitId)
				{
					_battleSave->setSelectedUnit(*i);
					break;
				}
			}
		}
	}
	else
		setBase(base);

	_battleSave->initMap(			// -> fake a Map for soldier placement ->
					_mapsize_x = 1,	// All units go on the same tile
					_mapsize_y = 1,	// and that tile is the equipt-Tile.
					_mapsize_z = 1);

	MapDataSet* const dataSet (new MapDataSet("blank", _game));
	MapData* const data (new MapData(dataSet));
	Tile* const tile (_battleSave->getTiles()[0u]);
	tile->setMapData(
				data,
				0,0,
				O_FLOOR);
	tile->getMapData(O_FLOOR)->setTileType(START_POINT);
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
	deployXcom(); // generate BattleItems for the equipt-Tile and place all units on that tile.

	delete data;
	delete dataSet;
}

/**
 * Processes a set of map-modules (sets of Tiles) into a Battlescape.
 * @param directives - the directives to use to build the Map
 */
void BattlescapeGenerator::generateMap(const std::vector<MapScript*>* const directives) // private.
{
	//Log(LOG_INFO) << "generateMap, terraRule = " << _terrainRule->getType() << " script = " << _terrainRule->getScriptType();
//	_error = false;
	_testBlock = new MapBlock("testBlock");

	init(); // set up map-generation vars

	MapBlock* craftBlock (nullptr);
	std::vector<MapBlock*> ufoBlocks;

	int
		blockDataSetIdOffset (0),
		craftDataSetIdOffset (0);

	std::map<int, bool> conditions; // create an array to track command success/failure

	for (std::vector<MapDataSet*>::const_iterator
			i = _terrainRule->getMapDataSets()->begin();
			i != _terrainRule->getMapDataSets()->end();
			++i)
	{
		(*i)->loadData();

		if (_rules->getMCDPatch((*i)->getType()) != nullptr)
			_rules->getMCDPatch((*i)->getType())->modifyData(*i);

		_battleSave->getMapDataSets()->push_back(*i);
		++blockDataSetIdOffset;
	}

	RuleTerrain* ufoTerrain (nullptr); // generate the map now and store it inside the tile objects

	if (_battleSave->getTacType() == TCT_BASEDEFENSE) // this mission-type is "hard-coded" in terms of map-layout
		generateBaseMap();

	bool success;

	for (std::vector<MapScript*>::const_iterator // process script-directives
			i = directives->begin();
			i != directives->end();
			++i)
	{
		//Log(LOG_INFO) << "do script Command type = " << (int)(*i)->getType();
		if ((*i)->getLabel() != 0
			&& conditions.find((*i)->getLabel()) != conditions.end())
		{
			throw Exception("bGen:generateMap() Multiple directives share the same label.");
		}

		success = conditions[static_cast<size_t>((*i)->getLabel())] = false;

		bool metConditions (true);
		if ((*i)->getConditions()->empty() == false) // if this command runs conditionally on the failures or successes of previous commands
		{
			for (std::vector<int>::const_iterator // compare the corresponding entries in the success/failure vector
					j = (*i)->getConditions()->begin();
					j != (*i)->getConditions()->end();
					++j)
			{
				if (conditions.find(std::abs(*j)) != conditions.end())	// positive numbers indicate conditional on success, negative means conditional on failure
				{														// ie: [1,-2] means this command only runs if command 1 succeeded and command 2 failed.
					if ((*j > 0 && conditions[*j] == false)
						|| (*j < 0 && conditions[std::abs(*j)] == true))
					{
						metConditions = false;
						break;
					}
				}
				else
				{
					throw Exception("bGen:generateMap() Conditional directive expected a label that did not exist before the directive.");
				}
			}
		}

		if (metConditions == true
			&& RNG::percent((*i)->chanceOfExecution()) == true) // if there's a chance a command won't execute by design take that into account here
		{
			//Log(LOG_INFO) << " execution TRUE";
			(*i)->init(); // initialize the block selection arrays

			for (int // each command can be attempted multiple times since randomization within the rects may occur
					j = 0;
					j != (*i)->getExecutions();
					++j)
			{
				MapBlock* block (nullptr);
				int
					x,y;

				switch ((*i)->getType())
				{
					case MSC_ADDBLOCK:
						if ((block = (*i)->getNextBlock(_terrainRule)) != nullptr
							&& selectPosition( // select x/y-position within the rects using an even distribution
											(*i)->getRects(),
											x,y,
											block->getSizeX(),
											block->getSizeY()) == true)
						{
							success |= addBlock(x,y, block) == true;
						}
						break;

					case MSC_ADDLINE:
						success = addLine(
										static_cast<MapDirection>((*i)->getDirection()),
										(*i)->getRects()) == true;
						break;

					case MSC_ADDCRAFT:
						if (_craft != nullptr)
						{
							craftBlock = _craft->getRules()->getTacticalTerrainData()->getMapBlockRand(999,999, 0, false);
							if (addCraft(craftBlock, *i, _craftPos) == true)
							{
								// By default addCraft adds blocks from group 1.
								// This can be overwritten in the command by defining specific groups or blocks
								// or this behaviour can be suppressed by leaving group 1 empty.
								// This is intentional to allow for TFTD's cruise liners/etc.
								// In this situation you can end up with ANYTHING under your craft so be careful.
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
						// As above note that the craft and the ufo will never be allowed to overlap.
						// The significant difference here is that you accept a UFO-type string here to
						// choose the UFO map and store the UFO positions in a vector, which are iterated
						// later when actually loading the map- and route-data. This makes it possible
						// to add multiple UFOs to a single map.
						// IMPORTANT: All the UFOs must use *exactly* the same MCD set.
						// This is fine for most UFOs but it does mean that small scouts can't be
						// combined with larger ones unless some major alterations are done to the MCD-sets
						// and to the maps themselves beforehand.
						// This is because serializing all the MCDs is an implementational nightmare from
						// your perspective and modders can take care of all that manually on their end.
						//
						// - Warboy opus (c.2015)
						if (_rules->getUfo((*i)->getUfoType()) != nullptr)
							ufoTerrain = _rules->getUfo((*i)->getUfoType())->getTacticalTerrainData();
						else if (_ufo != nullptr)
							ufoTerrain = _ufo->getRules()->getTacticalTerrainData();

						if (ufoTerrain != nullptr)
						{
							MapBlock* const ufoBlock (ufoTerrain->getMapBlockRand(999,999, 0, false));
							Log(LOG_INFO) << "bGen:generateMap() ufo-type " << ufoBlock->getType();

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
						block = (*i)->getNextBlock(_terrainRule);
						while (block != nullptr)
						{
							if (selectPosition(
											(*i)->getRects(),
											x,y,
											block->getSizeX(),
											block->getSizeY()) == true)
							{
								success |= addBlock(x,y, block) == true; // fill area will succeed if even one block is added
							}
							else
								break;

							block = (*i)->getNextBlock(_terrainRule);
						}
						break;

					case MSC_CHECKBLOCK:
						for (std::vector<SDL_Rect*>::const_iterator
								k = (*i)->getRects()->begin();
								k != (*i)->getRects()->end() && success == false;
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
									else // wildcard - don't care what block it is, just wanna know if there's a block here
										success = (_blocks[x][y] != nullptr);
								}
							}
						}
						break;

					case MSC_REMOVE:
						success = clearBlocks(*i);
						break;

					case MSC_RESIZE:
						if (_battleSave->getTacType() == TCT_BASEDEFENSE)
						{
							throw Exception("bGen:generateMap() Base defense map cannot be resized.");
						}

						if (_blocksLeft < (_mapsize_x / 10) * (_mapsize_y / 10))
						{
							throw Exception("bGen:generateMap() Map cannot be resized after adding blocks.");
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
	//Log(LOG_INFO) << ". . done Directives";

	if (_blocksLeft != 0)
	{
		throw Exception("bGen:generateMap() Map failed to fully generate.");
	}

	loadNodes();

	if (_battleSave->getTacType() == TCT_BASEASSAULT)
		assignStoragePositions();


	if (ufoTerrain != nullptr && ufoBlocks.empty() == false)
	{
		for (std::vector<MapDataSet*>::const_iterator
				i = ufoTerrain->getMapDataSets()->begin();
				i != ufoTerrain->getMapDataSets()->end();
				++i)
		{
			(*i)->loadData();

			if (_rules->getMCDPatch((*i)->getType()) != nullptr)
				_rules->getMCDPatch((*i)->getType())->modifyData(*i);

			_battleSave->getMapDataSets()->push_back(*i);
			++craftDataSetIdOffset;
		}

		for (size_t
				i = 0u;
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
					_seg[static_cast<size_t>(_ufoPos[i].x) + static_cast<size_t>(j)]
						[static_cast<size_t>(_ufoPos[i].y) + static_cast<size_t>(k)] = Node::SEG_UFO;
				}
			}
		}
	}

	if (craftBlock != nullptr)
	{
		for (std::vector<MapDataSet*>::const_iterator
				i = _craft->getRules()->getTacticalTerrainData()->getMapDataSets()->begin();
				i != _craft->getRules()->getTacticalTerrainData()->getMapDataSets()->end();
				++i)
		{
			(*i)->loadData();

			if (_rules->getMCDPatch((*i)->getType()) != nullptr)
				_rules->getMCDPatch((*i)->getType())->modifyData(*i);

			_battleSave->getMapDataSets()->push_back(*i);
		}

		loadMAP(
				craftBlock,
				static_cast<int>(_craftPos.x * 10),
				static_cast<int>(_craftPos.y * 10),
				_craft->getRules()->getTacticalTerrainData(),
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
				_seg[static_cast<size_t>(_craftPos.x) + static_cast<size_t>(i)]
					[static_cast<size_t>(_craftPos.y) + static_cast<size_t>(j)] = Node::SEG_CRAFT;
			}
		}

//		for (int
//				i = _craftPos.x * 10 - 1;
//				i <= _craftPos.x * 10 + craftBlock->getSizeX();
//				++i)
//		{
//			for (int
//					j = _craftPos.y * 10 - 1;
//					j <= _craftPos.y * 10 + craftBlock->getSizeY();
//					++j)
//			{
//				for (int
//						k = _mapsize_z - 1;
//						k >= _craftZ;
//						--k)
//				{
//					if (_battleSave->getTile(Position(i,j,k)))
//						_battleSave->getTile(Position(i,j,k))->setDiscovered(true, 2);
//				}
//			}
//		}
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

//	if (_error == true)
//	{
//		throw Exception("bGen:generateMap() Map failed to fully generate. See logfile for details.");
//	}
}

/**
 * Generates a battlefield based on the Base's layout.
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
				xLimit ((*i)->getX() + static_cast<int>((*i)->getRules()->getSize()) - 1),
				yLimit ((*i)->getY() + static_cast<int>((*i)->getRules()->getSize()) - 1);
			int num (0);

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
					// lots of crazy stuff here, which is for the hangars or other large base facilities
					// TODO: clean this mess up, make the mapNames a vector in the base module defs
					// also figure out how to do the terrain sets on a per-block basis.
					const std::string mapname ((*i)->getRules()->getMapName());
					std::ostringstream newname;
					newname << mapname.substr(
											0u,
											mapname.size() - 2u); // strip off last 2 digits

					int mapnum (std::atoi(mapname.substr( // get number
														mapname.size() - 2u,
														2u).c_str()));
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
						int grdLevel;
						for (
								grdLevel = _mapsize_z - 1;
								grdLevel > -1;
								--grdLevel)
						{
							if (_battleSave->getTile(Position(
															x * 10,
															y * 10,
															grdLevel))->hasNoFloor() == false)
							{
								break;
							}
						}
						if (grdLevel < 0) grdLevel = 0; // safety.

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
								if (((k + l) & 1) == 0) // use only every other tile giving a checkerboard pattern
								{
									const Tile
										* const tile      (_battleSave->getTile(Position(k,     l,     grdLevel))),
										* const tileEast  (_battleSave->getTile(Position(k + 1, l,     grdLevel))),
										* const tileSouth (_battleSave->getTile(Position(k,     l + 1, grdLevel)));

									if (tile != nullptr
										&& tile->getMapData(O_FLOOR) != nullptr
										&& tile->getMapData(O_OBJECT) == nullptr
										&& tileEast != nullptr
										&& tileEast->getMapData(O_WESTWALL) == nullptr
										&& tileSouth != nullptr
										&& tileSouth->getMapData(O_NORTHWALL) == nullptr)
									{
										_battleSave->storagePositions().push_back(Position(k,l, grdLevel));
									}
								}
							}
						}

						if (_tileEquipt == nullptr) // put the inventory tile on the lowest floor, jic.
							_tileEquipt = _battleSave->getTile(Position(
																	x * 10 + 5,
																	y * 10 + 5,
																	std::max(0,
																			 grdLevel - 1)));
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

	_battleSave->calcBaseDestruct();
}

/**
 * Finds AlienBase start-modules and assigns possible positions for player's
 * starting equipment.
 */
void BattlescapeGenerator::assignStoragePositions() // private.
{
	const size_t
		xSize (static_cast<size_t>(_mapsize_x / 10)),
		ySize (static_cast<size_t>(_mapsize_y / 10));

	for (size_t
			y = 0u;
			y != ySize;
			++y)
	{
		for (size_t
				x = 0u;
				x != xSize;
				++x)
		{
			if (_blocks[x][y]->isInGroup(MBT_START) == true)
				_battleSave->storagePositions().push_back(Position(
															static_cast<int>(x) * 10 + 5,
															static_cast<int>(y) * 10 + 5,
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
	Tile* tile;
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
				tile = _battleSave->getTile(Position(dx,dy,z));
				for (size_t
						partType = 0u;
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
	int segOffset (0);

	const size_t
		blocks_x (static_cast<size_t>(_mapsize_x / 10)),
		blocks_y (static_cast<size_t>(_mapsize_y / 10));

	for (size_t
			y = 0u;
			y != blocks_y;
			++y)
	{
		for (size_t
				x = 0u;
				x != blocks_x;
				++x)
		{
			_seg[x][y] = segOffset;

			if (_blocks[x][y] != nullptr && _blocks[x][y] != _testBlock)
			{
				if (_blocks[x][y]->isInGroup(MBT_LANDPAD) == false
					|| _landingzone[x][y] == false) // TODO: look closer at this.
				{
					loadRMP(
						_blocks[x][y],
						static_cast<int>(x) * 10,
						static_cast<int>(y) * 10,
						segOffset++);
				}
			}
		}
	}
}

/**
 * Attaches all the nodes together in an intimate web of C++.
 * @note I'd suppose this attaches the N/E/S/W node-links to other MapBlocks.
 */
void BattlescapeGenerator::attachNodeLinks() // private.
{
	const int
		borDirs[4u]			{-2,-3,-4,-5},
		borDirs_invert[4u]	{-4,-5,-2,-3};
	int borSegs[4u];

	size_t
		x,y;

	for (std::vector<Node*>::const_iterator
			i = _battleSave->getNodes()->begin();
			i != _battleSave->getNodes()->end();
			++i)
	{
		x = static_cast<size_t>((*i)->getPosition().x / 10),
		y = static_cast<size_t>((*i)->getPosition().y / 10);

		if (static_cast<int>(x) == (_mapsize_x / 10) - 1)
			borSegs[0u] = -1;
		else
			borSegs[0u] = _seg[x + 1][y];

		if (static_cast<int>(y) == (_mapsize_y / 10) - 1)
			borSegs[1u] = -1;
		else
			borSegs[1u] = _seg[x][y + 1];

		if (x == 0u)
			borSegs[2u] = -1;
		else
			borSegs[2u] = _seg[x - 1][y];

		if (y == 0u)
			borSegs[3u] = -1;
		else
			borSegs[3u] = _seg[x][y - 1];

		for (std::vector<int>::iterator
				j = (*i)->getNodeLinks()->begin();
				j != (*i)->getNodeLinks()->end();
				++j)
		{
			for (size_t
					dir = 0u;
					dir != 4u;
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
 * @param ret_x		- reference to the x position for the block - gets stored in this variable
 * @param ret_y		- reference to the y position for the block - gets stored in this variable
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
	std::vector<std::pair<int, int>> validPairs;
	bool add;

	for (std::vector<SDL_Rect*>::const_iterator
			i = available.begin();
			i != available.end();
			++i)
	{
		//Log(LOG_INFO) << ". iter";
		const int
			width		(static_cast<int>((*i)->w)),
			height		(static_cast<int>((*i)->h)),
			xPos		(static_cast<int>((*i)->x)),
			yPos		(static_cast<int>((*i)->y)),
			widthField	(static_cast<int>(rectField.w)),
			heightField	(static_cast<int>(rectField.h));

		if (width >= size_x && height >= size_y)
		{
			//Log(LOG_INFO) << ". . found area";
			for (int
					x = xPos;
					x + size_x <= xPos + width && x + size_x <= widthField;
					++x)
			{
				//Log(LOG_INFO) << ". . . fits X";
				for (int
						y = yPos;
						y + size_y <= yPos + height && y + size_y <= heightField;
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
 * @param rect		- reference to the position of the craft and store it here
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
			rC_x (static_cast<size_t>(posX)),
			rC_y (static_cast<size_t>(posY)),
			rC_w (static_cast<size_t>(rect.w)),
			rC_h (static_cast<size_t>(rect.h));

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
		const std::vector<SDL_Rect*>* const rects)
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

	int
		roadX (0), // avoid vc++ linker warnings
		roadY (0),
		limit,
		* pRoad;
	MapBlockType
		testType,
		blockType;

	switch (dir)
	{
		case MD_VERTICAL:
			limit = _mapsize_y / 10;
			pRoad = &roadY;
			testType = MBT_EWROAD;
			blockType = MBT_NSROAD;
			break;

		default:
			limit = _mapsize_x / 10;
			pRoad = &roadX;
			testType = MBT_NSROAD;
			blockType = MBT_EWROAD;
	}

	int t (0);
	bool placed (false);
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
				&& _blocks[roadX][roadY]->isInGroup(testType) == false)
			{
				placed = false;
				break;
			}
		}
		if (++t > 20) return false; // forget it ...
	}

	*pRoad = 0;
	while (*pRoad != limit)
	{
		if (_blocks[roadX][roadY] == nullptr)
			addBlock(
					roadX, roadY,
					_terrainRule->getMapBlockRand(10,10, blockType));
		else if (_blocks[roadX][roadY]->isInGroup(testType) == true)
		{
			_blocks[roadX][roadY] = _terrainRule->getMapBlockRand(10,10, MBT_CROSSROAD);
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
 * @param block	- pointer to the MapBlock to add
 * @return, true if the block was added
 */
bool BattlescapeGenerator::addBlock( // private.
		int x,
		int y,
		MapBlock* const block)
{
	const size_t
		xSize ((block->getSizeX() - 1) / 10),
		ySize ((block->getSizeY() - 1) / 10),
		xt (static_cast<size_t>(x)),
		yt (static_cast<size_t>(y));

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
					   [yt + yd] != nullptr)
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
 * @param dir	- the direction to drill (MapScript.h)
 */
void BattlescapeGenerator::drillModules( // private.
		TunnelData* const info,
		const std::vector<SDL_Rect*>* const rects,
		MapDirection dir)
{
	const MCDReplacement
		* const westMcd		(info->getMcdReplacement("west")),
		* const northMcd	(info->getMcdReplacement("north")),
		* const cornerMcd	(info->getMcdReplacement("corner")),
		* const floorMcd	(info->getMcdReplacement("floor"));

	SDL_Rect rect;
	rect.x =
	rect.y = 3;
	rect.w =
	rect.h = 3u;

	if (rects->empty() == false)
		rect = *rects->front();

	Tile* tile;
	MapData* data;
	const MapData* objPart;

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
					if (i < _mapsize_x / 10 - 1
						&& (_drillMap[i][j] == MD_HORIZONTAL || _drillMap[i][j] == MD_BOTH)
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
									data = _terrainRule->getMapDataSets()->at(floorMcd->dataSet)->getRecords()->at(floorMcd->entry);
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

								if ((objPart = tile->getMapData(O_OBJECT)) != nullptr
									&& objPart->getTuCostPart(MT_WALK) == 0)
								{
									tile->setMapData(nullptr,-1,-1, O_OBJECT);
								}
							}
						}

						if (northMcd != nullptr)
						{
							data = _terrainRule->getMapDataSets()->at(northMcd->dataSet)->getRecords()->at(northMcd->entry);
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
							data = _terrainRule->getMapDataSets()->at(cornerMcd->dataSet)->getRecords()->at(cornerMcd->entry);
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
					if (j < _mapsize_y / 10 - 1
						&& (_drillMap[i][j] == MD_VERTICAL || _drillMap[i][j] == MD_BOTH)
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
									data = _terrainRule->getMapDataSets()->at(floorMcd->dataSet)->getRecords()->at(floorMcd->entry);
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

								if ((objPart = tile->getMapData(O_OBJECT)) != nullptr
									&& objPart->getTuCostPart(MT_WALK) == 0)
								{
									tile->setMapData(nullptr,-1,-1, O_OBJECT);
								}
							}
						}

						if (westMcd != nullptr)
						{
							data = _terrainRule->getMapDataSets()->at(westMcd->dataSet)->getRecords()->at(westMcd->entry);
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
							data = _terrainRule->getMapDataSets()->at(cornerMcd->dataSet)->getRecords()->at(cornerMcd->entry);
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
 * Clears all MapBlocks in a given set of rects as defined by a specified
 * directive.
 * @param directive - contains all the info needed
 * @return, true if success
 * @feel clever & self-important(!)
 * @reality WoT..
 */
bool BattlescapeGenerator::clearBlocks(const MapScript* const directive) // private.
{
	std::vector<std::pair<int,int>> deleted;

	for (std::vector<SDL_Rect*>::const_iterator
			i = directive->getRects()->begin();
			i != directive->getRects()->end();
			++i)
	{
		const int
			xPos	(static_cast<int>((*i)->x)),
			yPos	(static_cast<int>((*i)->y)),
			width	(static_cast<int>((*i)->w)),
			height	(static_cast<int>((*i)->h));

		for (int
				x = xPos;
				x != xPos + width && x != _mapsize_x / 10;
				++x)
		{
			for (int
					y = yPos;
					y != yPos + height && y != _mapsize_y / 10;
					++y)
			{
				if (_blocks[x][y] != nullptr && _blocks[x][y] != _testBlock)
				{
					std::pair<int,int> pos (x,y);

					if (directive->getGroups()->empty() == false)
					{
						for (std::vector<int>::const_iterator
								j = directive->getGroups()->begin();
								j != directive->getGroups()->end();
								++j)
							if (_blocks[x][y]->isInGroup(*j) == true)
								if (std::find( // the deleted vector should only contain unique entries
											deleted.begin(),
											deleted.end(),
											pos) == deleted.end())
									deleted.push_back(pos);
					}
					else if (directive->getBlocks()->empty() == false)
					{
						for (std::vector<int>::const_iterator
								j = directive->getBlocks()->begin();
								j != directive->getBlocks()->end();
								++j)
							if (*j < static_cast<int>(_terrainRule->getMapBlocks()->size()))
								if (std::find( // the deleted vector should only contain unique entries
											deleted.begin(),
											deleted.end(),
											pos) == deleted.end())
									deleted.push_back(pos);
					}
					else if (std::find( // the deleted vector should only contain unique entries
									deleted.begin(),
									deleted.end(),
									pos) == deleted.end())
						deleted.push_back(pos);
				}
			}
		}
	}

	bool success (false);
	for (std::vector<std::pair<int,int>>::const_iterator
			i = deleted.begin();
			i != deleted.end();
			++i)
	{
		const int
			x ((*i).first),
			y ((*i).second);

		clearModule(
				x * 10,
				y * 10,
				_blocks[x][y]->getSizeX(),
				_blocks[x][y]->getSizeY());

		const int
			delx (_blocks[x][y]->getSizeX() / 10),
			dely (_blocks[x][y]->getSizeY() / 10);

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
		success = true; // this command succeeds if even one block is removed.
	}
	return success;
}

/**
 * Sets up the Player's objectives for the battle.
 * @param ruleDeploy - deployment-rule from which to fetch relevant data
 */
void BattlescapeGenerator::setupObjectives(const RuleAlienDeployment* const ruleDeploy) // private.
{
	const TileType tileType (ruleDeploy->getPlayerObjective());
	if (tileType != TILE)
	{
		int
			req (ruleDeploy->getObjectivesRequired()),
			inSitu (0);
		MapDataType partType;
		const Tile* tile;

		for (size_t
				i = 0u;
				i != _battleSave->getMapSizeXYZ();
				++i)
		{
			for (size_t
					j = 0u;
					j != Tile::PARTS_TILE;
					++j)
			{
				partType = static_cast<MapDataType>(j);

				tile = _battleSave->getTiles()[i];
				if (tile->getMapData(partType) != nullptr
					&& tile->getMapData(partType)->getTileType() == tileType)
				{
					++inSitu;
				}
			}
		}

		if (inSitu != 0)
		{
			_battleSave->setObjectiveTileType(tileType);

			if (req == 0 || inSitu < req)
				req = inSitu;

			_battleSave->setObjectiveTotal(req);
		}
	}
}

}
