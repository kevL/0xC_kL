/*
 * Copyright 2010-2018 OpenXcom Developers.
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

#include "SavedBattleGame.h"

//#include <vector>
//#include <assert.h>
#include <cstring>

#include "BattleItem.h"
#include "Node.h"
#include "SavedGame.h"
#include "SerializationHelper.h"
#include "Tile.h"

#include "../Battlescape/AlienBAIState.h"
#include "../Battlescape/CivilianBAIState.h"
#include "../Battlescape/BattlescapeGame.h"
#include "../Battlescape/BattlescapeState.h"
#include "../Battlescape/ExplosionBState.h"
#include "../Battlescape/Map.h"
#include "../Battlescape/Pathfinding.h"
#include "../Battlescape/Position.h"
#include "../Battlescape/TileEngine.h"

#include "../Engine/Game.h"
#include "../Engine/Language.h"
#include "../Engine/Logger.h"
//#include "../Engine/Options.h"
#include "../Engine/RNG.h"

#include "../Interface/Text.h"

#include "../Resource/XcomResourcePack.h"

#include "../Ruleset/MapDataSet.h"
#include "../Ruleset/MCDPatch.h"
#include "../Ruleset/OperationPool.h"
#include "../Ruleset/RuleArmor.h"
#include "../Ruleset/RuleInventory.h"
#include "../Ruleset/Ruleset.h"


namespace OpenXcom
{

/**
 * Creates and initializes a SavedBattleGame.
 * @param playSave	- pointer to the SavedGame
 * @param titles	- pointer to a vector of pointers to OperationPool (default nullptr)
 * @param rules		- pointer to the Ruleset (default nullptr)
 */
SavedBattleGame::SavedBattleGame(
		SavedGame* const playSave,
		const std::vector<OperationPool*>* const titles,
		const Ruleset* const rules)
	:
		_playSave(playSave),
		_battleState(nullptr),
		_mapsize_x(0),
		_mapsize_y(0),
		_mapsize_z(0),
		_qtyTilesTotal(0u),
		_selectedUnit(nullptr),
		_recallUnit(nullptr),
		_pf(nullptr),
		_te(nullptr),
		_tacticalShade(0),
		_side(FACTION_PLAYER),
		_turn(1),
		_debugTac(false),
		_aborted(false),
		_itemId(0),
		_objectiveType(TILE),
		_objectivesRequired(0),
		_objectivesDestroyed(0),
		_bonk(false),
		_cheatAI(false),
//		_batReserved(BA_NONE),
//		_kneelReserved(false),
		_equiptTile(nullptr),
		_groundLevel(-1),
		_tacType(TCT_DEFAULT),
		_controlDestroyed(false),
		_tiles(nullptr),
		_pacified(false),
		_rfTriggerOffset(0,0,-1),
		_dropTu(0),
		_turnLimit(0),
		_chronoResult(FORCE_LOSE),
		_cheatTurn(CHEAT_TURN_DEFAULT)
//		_preBattle(true)
//		_dragInvert(false),
//		_dragTimeTolerance(0),
//		_dragPixelTolerance(0)
{
/*	// debug TEST ->
	Position pos;
	int size1, size2;
	Log(LOG_INFO) << "size 1 large FALSE";
	size1 = -1;//(-unit->getArmor()->getSize()),
	size2 =  1;//((large == true) ? 2 : 1),
	int
		xArray1[8u] {    0, size2, size2, size2,     0, size1, size1, size1},
		yArray1[8u] {size1, size1,     0, size2, size2, size2,     0, size1};
	for (unsigned
			i = 0u;
			i != 8u;
			++i)
	{
		pos = Position(xArray1[i], yArray1[i], 0);
		Log(LOG_INFO) << ". i= " << i << (pos);
	}

	Log(LOG_INFO) << "size 2 large FALSE";
	size1 = -2;//(-unit->getArmor()->getSize()),
	size2 =  1;//((large == true) ? 2 : 1),
	int
		xArray2[8u] {    0, size2, size2, size2,     0, size1, size1, size1},
		yArray2[8u] {size1, size1,     0, size2, size2, size2,     0, size1};
	for (unsigned
			i = 0u;
			i != 8u;
			++i)
	{
		pos = Position(xArray2[i], yArray2[i], 0);
		Log(LOG_INFO) << ". i= " << i << (pos);
	}

	Log(LOG_INFO) << "size 1 large TRUE";
	size1 = -1;//(-unit->getArmor()->getSize()),
	size2 =  2;//((large == true) ? 2 : 1),
	int
		xArray3[8u] {    0, size2, size2, size2,     0, size1, size1, size1},
		yArray3[8u] {size1, size1,     0, size2, size2, size2,     0, size1};
	for (unsigned
			i = 0u;
			i != 8u;
			++i)
	{
		pos = Position(xArray3[i], yArray3[i], 0);
		Log(LOG_INFO) << ". i= " << i << (pos);
	}

	Log(LOG_INFO) << "size 2 large TRUE";
	size1 = -2;//(-unit->getArmor()->getSize()),
	size2 =  2;//((large == true) ? 2 : 1),
	int
		xArray4[8u] {    0, size2, size2, size2,     0, size1, size1, size1},
		yArray4[8u] {size1, size1,     0, size2, size2, size2,     0, size1};
	for (unsigned
			i = 0u;
			i != 8u;
			++i)
	{
		pos = Position(xArray4[i], yArray4[i], 0);
		Log(LOG_INFO) << ". i= " << i << (pos);
	} // end_TEST.
*/


	//Log(LOG_INFO) << "\nCreate SavedBattleGame";
	if (rules != nullptr) // ie. not craft- or base-equip screen.
		_dropTu = rules->getHighestDropCost();

	_tileSearch.resize(SEARCH_SIZE);
	for (size_t
			i = 0u;
			i != SEARCH_SIZE;
			++i)
	{
		_tileSearch[i].x = (static_cast<int>(i % SEARCH_DIST)) - 5; // ie. -5 to +5
		_tileSearch[i].y = (static_cast<int>(i / SEARCH_DIST)) - 5; // ie. -5 to +5
	}
	/*	The '_tileSearch' array (Position):
		0		1		2		3		4		5		6		7		8		9		10
	[	-5,-5	-4,-5	-3,-5	-2,-5	-1,-5	0,-5	1,-5	2,-5	3,-5	4,-5	5,-5	//  0
		-5,-4	-4,-4	-3,-4	-2,-4	-1,-4	0,-4	1,-4	2,-4	3,-4	4,-4	5,-4	//  1
		-5,-3	-4,-3	-3,-3	-2,-3	-1,-3	0,-3	1,-3	2,-3	3,-3	4,-3	5,-3	//  2
		-5,-2	-4,-2	-3,-2	-2,-2	-1,-2	0,-2	1,-2	2,-2	3,-2	4,-2	5,-2	//  3
		-5,-1	-4,-1	-3,-1	-2,-1	-1,-1	0,-1	1,-1	2,-1	3,-1	4,-1	5,-1	//  4
		-5, 0	-4, 0	-3, 0	-2, 0	-1, 0	0, 0	1, 0	2, 0	3, 0	4, 0	5, 0	//  5
		-5, 1	-4, 1	-3, 1	-2, 1	-1, 1	0, 1	1, 1	2, 1	3, 1	4, 1	5, 1	//  6
		-5, 2	-4, 2	-3, 2	-2, 2	-1, 2	0, 2	1, 2	2, 2	3, 2	4, 2	5, 2	//  7
		-5, 3	-4, 3	-3, 3	-2, 3	-1, 3	0, 3	1, 3	2, 3	3, 3	4, 3	5, 3	//  8
		-5, 4	-4, 4	-3, 4	-2, 4	-1, 4	0, 4	1, 4	2, 4	3, 4	4, 4	5, 4	//  9
		-5, 5	-4, 5	-3, 5	-2, 5	-1, 5	0, 5	1, 5	2, 5	3, 5	4, 5	5, 5 ]	// 10
	*/

	if (titles != nullptr)
	{
		const size_t pool (0u); //RNG::generate(0, titles->size()-1 // <- in case I want to expand this for different missionTypes. eg, Cydonia -> "Blow Hard"
		_operationTitle = titles->at(pool)->genOperation();
	}
}

/**
 * Deletes the battlescape content from memory.
 */
SavedBattleGame::~SavedBattleGame()
{
	//Log(LOG_INFO) << "";
	//Log(LOG_INFO) << "Delete SavedBattleGame";
	if (_tiles != nullptr)
	{
		for (size_t
				i = 0u;
				i != _qtyTilesTotal;
				++i)
			delete _tiles[i];

		delete[] _tiles;
	}

	for (std::vector<MapDataSet*>::const_iterator
			i  = _battleDataSets.begin();
			i != _battleDataSets.end();
			++i)
		(*i)->unloadData();

	for (std::vector<Node*>::const_iterator
			i  = _nodes.begin();
			i != _nodes.end();
			++i)
		delete *i;

	for (std::vector<BattleUnit*>::const_iterator
			i  = _units.begin();
			i != _units.end();
			++i)
		delete *i;

	for (std::vector<BattleItem*>::const_iterator
			i  = _items.begin();
			i != _items.end();
			++i)
		delete *i;

	for (std::vector<BattleItem*>::const_iterator
			i  = _recoverGuaranteed.begin();
			i != _recoverGuaranteed.end();
			++i)
		delete *i;

	for (std::vector<BattleItem*>::const_iterator
			i  = _recoverConditional.begin();
			i != _recoverConditional.end();
			++i)
		delete *i;

	for (std::vector<BattleItem*>::const_iterator
			i  = _deletedProperty.begin();
			i != _deletedProperty.end();
			++i)
		delete *i;

	delete _pf;
	delete _te;
}

/**
 * Loads the SavedBattleGame from a YAML file.
 * @param node	- reference a YAML node
 * @param rules	- pointer to the Ruleset
 */
void SavedBattleGame::load(
		const YAML::Node& node,
		Ruleset* const rules)
{
	//Log(LOG_INFO) << "SavedBattleGame::load()";
	_terrain       = node["terrain"].as<std::string>(_terrain);
	_turn          = node["turn"]   .as<int>(_turn);
	_tacticalShade = node["shade"]  .as<int>(_tacticalShade);

	setTacType(_tacticalType = node["type"].as<std::string>(_tacticalType));


	Log(LOG_INFO) << ". init map";
	initMap(									// NOTE: This clears '_battleDataSets' (as well as '_nodes' and '_tiles')
		node["width"] .as<int>(_mapsize_x),		// therefore it should run *before* loading '_battleDataSets' etc.
		node["length"].as<int>(_mapsize_y),		// It runs when creating (or resizing) a tactical (1st & 2nd stages) in
		node["height"].as<int>(_mapsize_z));	// BattlescapeGenerator::generateMap() -- also runs for fake-inventories.
												// NOTE: The vars '_mapsize_x' '_mapsize_y' '_mapsize_z' and '_qtyTilesTotal' are set in initMap().

	Log(LOG_INFO) << ". load battle data sets";
	for (YAML::const_iterator
			i  = node["battleDataSets"].begin();
			i != node["battleDataSets"].end();
			++i)
	{
		_battleDataSets.push_back(rules->getMapDataSet(i->as<std::string>())); // NOTE: Ruleset must be non-const to push_back().
	}


	if (!node["tileTotalBytesPer"]) // binary tile data not found, load old-style text tiles :(
	{
		Log(LOG_INFO) << ". load tiles [text]";
		for (YAML::const_iterator
				i  = node["tiles"].begin();
				i != node["tiles"].end();
				++i)
		{
			const Position pos ((*i)["position"].as<Position>());
			getTile(pos)->load((*i));
		}
	}
	else // load binary Tiles.
	{
		Log(LOG_INFO) << ". load tiles [binary]";
		// load key to how the tile data was saved
		Tile::SerializationKey serKey;
		// WARNING: Don't trust extracting integers from YAML as anything other than 'int' ...
		// NOTE: Many load sequences use '.as<size_t>' .....
		const size_t totalTiles (node["totalTiles"].as<size_t>());

		std::memset(
				&serKey,
				0,
				sizeof(Tile::SerializationKey));

		// WARNING: Don't trust extracting integers from YAML as anything other than 'int' ...
		serKey.index      = node["tileIndexSize"]     .as<Uint8>(serKey.index);
		serKey.totalBytes = node["tileTotalBytesPer"] .as<Uint32>(serKey.totalBytes);
		serKey._fire      = node["tileFireSize"]      .as<Uint8>(serKey._fire);
		serKey._smoke     = node["tileSmokeSize"]     .as<Uint8>(serKey._smoke);
		serKey._aniOffset = node["tileOffsetSize"]    .as<Uint8>(serKey._aniOffset);
		serKey._partId    = node["tileIDSize"]        .as<Uint8>(serKey._partId);
		serKey._partSetId = node["tileSetIDSize"]     .as<Uint8>(serKey._partSetId);
		serKey.boolFields = node["tileBoolFieldsSize"].as<Uint8>(1u); // boolean flags used to be stored in an unmentioned byte (Uint8) :|

		// load binary tile data!
		const YAML::Binary binTiles (node["binTiles"].as<YAML::Binary>());

		Uint8
			* readBuffer (const_cast<Uint8*>(binTiles.data())),
			* const dataEnd (readBuffer + totalTiles * serKey.totalBytes);

		while (readBuffer < dataEnd)
		{
			const int index (unserializeInt(&readBuffer, serKey.index));
			assert(
				index > -1
				&& index < static_cast<int>(_qtyTilesTotal));

			_tiles[static_cast<size_t>(index)]->loadBinary(readBuffer, serKey); // loadBinary's privileges to advance *readBuffer have been revoked
			readBuffer += serKey.totalBytes - serKey.index;	// readBuffer is now incremented strictly by totalBytes in case there are obsolete fields present in the data
		}
	}

	if (_tacType == TCT_BASEDEFENSE)
	{
		Log(LOG_INFO) << ". load xcom base";
		if (node["moduleMap"])
			_baseModules = node["moduleMap"].as<std::vector<std::vector<std::pair<int,int>>>>();
	}

	Log(LOG_INFO) << ". load nodes";
	for (YAML::const_iterator
			i  = node["nodes"].begin();
			i != node["nodes"].end();
			++i)
	{
		Node* const routeNode (new Node());
		routeNode->load(*i);
		_nodes.push_back(routeNode);
	}


	const int selUnitId (node["selectedUnit"].as<int>());

	int id;

	BattleUnit* unit;
	UnitFaction
		faction,
		factionOrg;

	Log(LOG_INFO) << ". load units";
	for (YAML::const_iterator
			i  = node["units"].begin();
			i != node["units"].end();
			++i)
	{
		id         = (*i)["id"]                                      .as<int>();
		faction    = static_cast<UnitFaction>((*i)["faction"]        .as<int>());
		factionOrg = static_cast<UnitFaction>((*i)["originalFaction"].as<int>(faction)); // .. technically, static_cast<int>(faction).

		if (id < BattleUnit::MAX_SOLDIER_ID)			// instance a BattleUnit from a geoscape-soldier
			unit = new BattleUnit(
							_playSave->getSoldier(id),
							this);
		else											// instance a BattleUnit as an aLien, civie, or support-unit
		{
			const std::string
				type  ((*i)["genUnitType"] .as<std::string>()),
				armor ((*i)["genUnitArmor"].as<std::string>());

			if (rules->getUnitRule(type) != nullptr && rules->getArmor(armor) != nullptr) // safeties.
				unit = new BattleUnit(
									rules->getUnitRule(type),
									factionOrg,
									id,
									rules->getArmor(armor),
									this);
			else
				unit = nullptr;
		}

		if (unit != nullptr)
		{
			Log(LOG_INFO) << ". . load unit " << id;
			unit->load(*i);
			_units.push_back(unit);

			switch (faction)
			{
				case FACTION_PLAYER:
					if ((_selectedUnit == nullptr || unit->getId() == selUnitId)
						&& unit->getUnitStatus() == STATUS_STANDING)
					{
						_selectedUnit = unit;
					}
					break;

				case FACTION_HOSTILE:
				case FACTION_NEUTRAL:
					switch (unit->getUnitStatus())
					{
						case STATUS_DEAD:
						case STATUS_LATENT:
						case STATUS_LATENT_START:
							break;

						default:
							if (const YAML::Node& ai = (*i)["AI"])
							{
								BattleAIState* aiState;
								switch (faction)
								{
									case FACTION_HOSTILE: aiState = new AlienBAIState(this, unit); break;
									case FACTION_NEUTRAL: aiState = new CivilianBAIState(this, unit);
								}
								aiState->load(ai);			// NOTE: Cannot init() AI-state here, Pathfinding
								unit->setAIState(aiState);	// and TileEngine haven't been instantiated yet. So
							}								// it's done in BattlescapeGame::cTor.
					}
			}
		}
	}

	for (size_t // load _hostileUnitsThisTurn here
			i = 0u;
			i != _units.size();
			++i)
	{
		if (_units.at(i)->getUnitStatus() == STATUS_STANDING) // safety.
			_units.at(i)->loadSpotted(); // convert unit-ID's into pointers to BattleUnits
	}

	_shuffleUnits.assign(
					_units.size(),
					nullptr);


	Log(LOG_INFO) << ". reset tiles";
	positionUnits(); // matches up tiles and units

	Log(LOG_INFO) << ". load items";
	BattleItem* it;
	Position pos;
	int
		owner,
		uId;
	std::string st;
	bool ground;

	static const size_t LIST_TYPE (3u);
	static const std::string itLists_st[LIST_TYPE]
	{
		"items",
		"recoverConditional",
		"recoverGuaranteed"
	};
	std::vector<BattleItem*>* const itLists[LIST_TYPE]
	{
		&_items,
		&_recoverConditional,
		&_recoverGuaranteed
	};

	for (size_t
			i = 0u;
			i != LIST_TYPE;
			++i)
	{
		for (YAML::const_iterator
				j  = node[itLists_st[i]].begin();
				j != node[itLists_st[i]].end();
				++j)
		{
			st = (*j)["type"].as<std::string>();
			if (rules->getItemRule(st) != nullptr)
			{
				id = (*j)["id"].as<int>(-1); // NOTE: 'id' should always be valid here.
				it = new BattleItem(
								rules->getItemRule(st),
								nullptr,
								id);

				it->load(*j);

				if ((*j)["section"]) // TODO: why is this done here instead of by BattleItem::load()
				{
					st = (*j)["section"].as<std::string>();	// NOTE: the given 'section' should always be valid. Unless it's a loaded Ammo-item.
//					if (st.empty() == false) //!= "NONE")	// cf. BattleItem::save()
					it->setInventorySection(rules->getInventory(st));
				}

				ground = it->getInventorySection() != nullptr
					  && it->getInventorySection()->getCategory() == IC_GROUND;

				owner = (*j)["owner"].as<int>(-1); // cf. BattleItem::save() ->
				uId   = (*j)["unit"] .as<int>(-1);

				for (std::vector<BattleUnit*>::const_iterator // match up some item-variables with units
						k  = _units.begin();
						k != _units.end();
						++k)
				{
					const int testUnit ((*k)->getId());
					if (testUnit == owner)
					{
						if (ground == false)
							it->changeOwner(*k);	// ie. add to unit-inventory
						else
							it->setOwner(*k);		// ie. do NOT add to unit-inventory
					}
					else if (testUnit == uId)
						it->setBodyUnit(*k);
				}

				if (ground == true) // match up items and tiles
				{
					if ((*j)["position"])
					{
						pos = (*j)["position"].as<Position>();
						it->setInventorySection(rules->getInventoryRule(ST_GROUND));
						getTile(pos)->addItem(it);
					}
					else
						pos = Position::POS_BELOW; // cf. BattleItem::save()
				}

				itLists[i]->push_back(it);
			}
			else Log(LOG_ERROR) << "Failed to load item " << st;
		}
	}

	Log(LOG_INFO) << ". load weapons w/ ammo";
	// iterate through the items again and tie ammo-items to their weapons
	std::vector<BattleItem*>::const_iterator pWeapon (_items.begin());
	for (YAML::const_iterator
			i  = node["items"].begin();
			i != node["items"].end();
			++i)
	{
		if (rules->getItemRule((*i)["type"].as<std::string>()) != nullptr)
		{
			const int clip ((*i)["clip"].as<int>(-1)); // cf. BattleItem::save()
			if (clip != -1)
			{
				for (std::vector<BattleItem*>::const_iterator
						j  = _items.begin();
						j != _items.end();
						++j)
				{
					if ((*j)->getId() == clip)
					{
						(*pWeapon)->setClip(*j, true);
						break;
					}
				}
			}
			++pWeapon;
		}
	}

	for (YAML::const_iterator
			i  = node["delete"].begin();
			i != node["delete"].end();
			++i)
	{
		st = (*i)["type"].as<std::string>();
		if (rules->getItemRule(st) != nullptr)
		{
			id = (*i)["id"].as<int>(-1); // NOTE: 'id' should always be valid here.
			it = new BattleItem(
							rules->getItemRule(st),
							nullptr,
							id);
			it->loadDeleted();
			_deletedProperty.push_back(it);
		}
	}

	Log(LOG_INFO) << ". set some vars";

	_objectiveType       = static_cast<TilepartSpecial>(node["objectiveType"]      .as<int>(_objectiveType));
	_objectivesRequired  =                              node["objectivesRequired"] .as<int>(_objectivesRequired);
	_objectivesDestroyed =                              node["objectivesDestroyed"].as<int>(_objectivesDestroyed);

	_turnLimit = node["turnLimit"].as<int>(_turnLimit);
	_chronoResult = static_cast<ChronoResult>(node["chronoResult"].as<int>(_chronoResult));

	_cheatAI   = node["cheatAI"]  .as<bool>(_cheatAI);
	_cheatTurn = node["cheatTurn"].as<int>(_cheatTurn);
	_alienRace = node["alienRace"].as<std::string>(_alienRace);
//	_kneelReserved = node["kneelReserved"].as<bool>(_kneelReserved);

//	_batReserved = static_cast<BattleActionType>(node["batReserved"].as<int>(_batReserved));
	_operationTitle = Language::utf8ToWstr(node["operationTitle"].as<std::string>(""));

	_controlDestroyed = node["controlDestroyed"].as<bool>(_controlDestroyed);

	_music = node["music"].as<std::string>(_music);

	if (node["scanDots"]) _scanDots = node["scanDots"].as<std::vector<std::pair<int,int>>>();

	Log(LOG_INFO) << ". set item ID";
	setCanonicalBattleId();
	//Log(LOG_INFO) << "SavedBattleGame::load() EXIT";

	// TEST, reveal all tiles
//	for (size_t i = 0; i != _qtyTilesTotal; ++i)
//		_tiles[i]->setDiscovered(true, 2);
}

/**
 * Loads the resources required by the Map in this SavedBattleGame.
 * @param game - pointer to Game
 */
void SavedBattleGame::loadMapResources(const Game* const game)
{
	for (std::vector<MapDataSet*>::const_iterator
			i  = _battleDataSets.begin();
			i != _battleDataSets.end();
			++i)
	{
		(*i)->loadData();

		if (game->getRuleset()->getMCDPatch((*i)->getType()) != nullptr)
			game->getRuleset()->getMCDPatch((*i)->getType())->patchData(*i);
	}

	int
		record,  // id of data for a tilepart
		dataset; // id of the terrain's MCD data (within the total datasets aka MCD terrains)
	MapDataType partType;

	Tile* tile;
	for (size_t
			i = 0u;
			i != _qtyTilesTotal;
			++i)
	{
		tile = _tiles[i];
		for (size_t
				j = 0u;
				j != Tile::TILE_PARTS;
				++j)
		{
			partType = static_cast<MapDataType>(j);
			tile->getMapData(
						&record,
						&dataset,
						partType);

			if (record != -1 && dataset != -1)
				tile->setMapData(
							_battleDataSets[static_cast<size_t>(dataset)]->getRecords()->at(static_cast<size_t>(record)),
							record,
							dataset,
							partType);
		}
	}

	initUtilities(game->getResourcePack());

	_te->calculateSunShading();
	_te->calculateTerrainLighting();
	_te->calculateUnitLighting();
}

/**
 * Saves this SavedBattleGame to a YAML file.
 * @return, YAML node
 */
YAML::Node SavedBattleGame::save() const
{
	YAML::Node node;

	if (_objectivesRequired != 0)
	{
		node["objectiveType"]       = static_cast<int>(_objectiveType);
		node["objectivesRequired"]  = _objectivesRequired;
		node["objectivesDestroyed"] = _objectivesDestroyed;
	}

	node["width"]        = _mapsize_x;
	node["length"]       = _mapsize_y;
	node["height"]       = _mapsize_z;
	node["type"]         = _tacticalType;
	node["shade"]        = _tacticalShade;
	node["turn"]         = _turn;
	node["terrain"]      = _terrain;
	node["selectedUnit"] = (_selectedUnit != nullptr) ? _selectedUnit->getId() : -1;

	for (std::vector<MapDataSet*>::const_iterator
			i  = _battleDataSets.begin();
			i != _battleDataSets.end();
			++i)
	{
		node["battleDataSets"].push_back((*i)->getType());
	}

#if 0 // <- change to '1' to save Tiles in a human-readable non-binary format.
	for (size_t
			i = 0u;
			i != _qtyTilesTotal;
			++i)
	{
		if (_tiles[i]->isVoid() == false)
			node["tiles"].push_back(_tiles[i]->save());
	}
#else
	// write out the field sizes used to write the tile data
	node["tileIndexSize"]      = Tile::serializationKey.index;
	node["tileTotalBytesPer"]  = Tile::serializationKey.totalBytes;
	node["tileFireSize"]       = Tile::serializationKey._fire;
	node["tileSmokeSize"]      = Tile::serializationKey._smoke;
	node["tileOffsetSize"]     = Tile::serializationKey._aniOffset;
	node["tileIDSize"]         = Tile::serializationKey._partId;
	node["tileSetIDSize"]      = Tile::serializationKey._partSetId;
	node["tileBoolFieldsSize"] = Tile::serializationKey.boolFields;

	size_t tilesDataSize (Tile::serializationKey.totalBytes * _qtyTilesTotal);
	Uint8
		* const tilesData (static_cast<Uint8*>(calloc(tilesDataSize, 1u))),
		* writeBuffer (tilesData);

	for (size_t
			i = 0u;
			i != _qtyTilesTotal;
			++i)
	{
		serializeInt( // <- save ALL Tiles. (Stop void tiles returning undiscovered postReload.)
				&writeBuffer,
				Tile::serializationKey.index,
				static_cast<int>(i));
		_tiles[i]->saveBinary(&writeBuffer);
//		if (_tiles[i]->isVoid() == false)	// NOTE: Save *all* tiles because otherwise it creates problems
//		{									// with ... stuff.
//			serializeInt(
//					&writeBuffer,
//					Tile::serializationKey.index,
//					static_cast<int>(i));
//			_tiles[i]->saveBinary(&writeBuffer);
//		}
//		else
//			tilesDataSize -= Tile::serializationKey.totalBytes;
	}

	node["totalTiles"] = tilesDataSize / Tile::serializationKey.totalBytes; // not strictly necessary, just convenient
	node["binTiles"]   = YAML::Binary(tilesData, tilesDataSize);

	std::free(tilesData);
#endif

	for (std::vector<Node*>::const_iterator
			i  = _nodes.begin();
			i != _nodes.end();
			++i)
	{
		node["nodes"].push_back((*i)->save());
	}

	if (_tacType == TCT_BASEDEFENSE)
		node["moduleMap"] = _baseModules;

	for (std::vector<BattleUnit*>::const_iterator
			i  = _units.begin();
			i != _units.end();
			++i)
	{
		node["units"].push_back((*i)->save());
	}

	for (std::vector<BattleItem*>::const_iterator
			i  = _items.begin();
			i != _items.end();
			++i)
	{
		node["items"].push_back((*i)->save());
	}

	for (std::vector<BattleItem*>::const_iterator
			i  = _recoverGuaranteed.begin();
			i != _recoverGuaranteed.end();
			++i)
	{
		node["recoverGuaranteed"].push_back((*i)->save());
	}

	for (std::vector<BattleItem*>::const_iterator
			i  = _recoverConditional.begin();
			i != _recoverConditional.end();
			++i)
	{
		node["recoverConditional"].push_back((*i)->save());
	}

	for (std::vector<BattleItem*>::const_iterator
			i  = _deletedProperty.begin();
			i != _deletedProperty.end();
			++i)
	{
		node["delete"].push_back((*i)->saveDeleted());
	}

//	node["batReserved"]   = static_cast<int>(_batReserved);
//	node["kneelReserved"] = _kneelReserved;
	node["alienRace"]     = _alienRace;

	if (_operationTitle.empty() == false)
		node["operationTitle"] = Language::wstrToUtf8(_operationTitle);

	if (_controlDestroyed == true)
		node["controlDestroyed"] = _controlDestroyed;

	if (_music.empty() == false)
		node["music"] = _music;

	if (_turnLimit != 0)
	{
		node["turnLimit"] = _turnLimit;
		node["chronoResult"] = static_cast<int>(_chronoResult);
	}

	if (_cheatAI == true)
		node["cheatAI"] = _cheatAI;

	if (_cheatTurn != CHEAT_TURN_DEFAULT)
		node["cheatTurn"] = _cheatTurn;

	if (_scanDots.empty() == false)
		node["scanDots"] = _scanDots;

	return node;
}

/**
 * Gets the array of Tiles.
 * @return, pointer to a pointer to the Tile array
 */
Tile** SavedBattleGame::getTiles() const
{
	return _tiles;
}

/**
 * Deletes the old and initializes a new array of Tiles after clearing MCD types
 * and deleting any/all old Nodes.
 * @param mapsize_x - width on x-axis
 * @param mapsize_y - length on y-axis
 * @param mapsize_z - height on z-axis
 * @note If borks occur for the MapScript RESIZE directive, see
 *		 "fix save/load on resized maps"
 *		 https://github.com/SupSuper/OpenXcom/commit/2c88d4e5f5eacaffeae8042c5b941f1f12ca42aa
 *		 "remove debugging code that i should not have committed."
 *		 https://github.com/SupSuper/OpenXcom/commit/7ea43162665bd2de9ae5fd14142bf1de2194c4da
 *		 "fix multi stage saves"
 *		 https://github.com/SupSuper/OpenXcom/commit/068fc539a3928763c98120e60cd0660559457bdc
 */
void SavedBattleGame::initMap(
		const int mapsize_x,
		const int mapsize_y,
		const int mapsize_z)
{
	_battleDataSets.clear(); // clear MCD types.

	for (std::vector<Node*>::const_iterator // delete Nodes ->
			i  = _nodes.begin();
			i != _nodes.end();
			++i)
	{
		delete *i;
	}
	_nodes.clear();

	if (_tiles != nullptr) // delete Tiles ->
	{
		for (size_t
				i = 0u;
				i != _qtyTilesTotal;
				++i)
			delete _tiles[i];

		delete[] _tiles;
	}

	_qtyTilesTotal = static_cast<size_t>( // create Tiles ->
					 (_mapsize_x = mapsize_x)
				   * (_mapsize_y = mapsize_y)
				   * (_mapsize_z = mapsize_z));

	_tiles = new Tile*[_qtyTilesTotal];

	Position pos;
	for (size_t
			i = 0u;
			i != _qtyTilesTotal;
			++i)
	{
		tileCoords(
				i,
				&pos.x,
				&pos.y,
				&pos.z);

		_tiles[i] = new Tile(pos);
	}
}

/**
 * Initializes the battlefield-utilities.
 * @param res - pointer to ResourcePack
 */
void SavedBattleGame::initUtilities(const ResourcePack* const res)
{
	delete _pf;
	delete _te;

	_pf = new Pathfinding(this);
	_te = new TileEngine(
					this,
					res->getVoxelData());
}

/**
 * Sets the TacticalType based on a specified mission-type.
 * @WARNING: This is *not* ready to use arbitrary/customized mission-strings.
 * @param type - reference to a mission-type
 */
void SavedBattleGame::setTacType(const std::string& type) // private.
{
	if      (type.compare("STR_UFO_CRASH_RECOVERY")     == 0) _tacType = TCT_UFOCRASHED;
	else if (type.compare("STR_UFO_GROUND_ASSAULT")     == 0) _tacType = TCT_UFOLANDED;
	else if (type.compare("STR_BASE_DEFENSE")           == 0) _tacType = TCT_BASEDEFENSE;
	else if (type.compare("STR_ALIEN_BASE_ASSAULT")     == 0) _tacType = TCT_BASEASSAULT;
	else if (type.compare("STR_TERROR_MISSION")         == 0
		||   type.compare("STR_PORT_ATTACK")            == 0) _tacType = TCT_TERRORSITE;
	else if (type.compare("STR_MARS_CYDONIA_LANDING")   == 0) _tacType = TCT_MARS1;
	else if (type.compare("STR_MARS_THE_FINAL_ASSAULT") == 0) _tacType = TCT_MARS2;
	else                                                      _tacType = TCT_DEFAULT;	// <- the default should probly be TCT_UFOCRASHED.
}																						// Or even TCT_ARBITRARY/CUSTOM.

/**
 * Sets the tactical-type.
 * @param type - reference to a tactical-type
 */
void SavedBattleGame::setTacticalType(const std::string& type)
{
	setTacType(_tacticalType = type);
}

/**
 * Gets the tactical-type as a string.
 * @note This should return a const ref except perhaps when there's a nextStage
 * that deletes this SavedBattleGame ... and creates a new one wherein the ref
 * is no longer valid.
 * @return, the tactical-type
 */
std::string SavedBattleGame::getTacticalType() const
{
	return _tacticalType;
}

/**
 * Gets the TacticalType of this battle.
 * @return, the TacticalType (SavedBattleGame.h)
 */
TacticalType SavedBattleGame::getTacType() const
{
	return _tacType;
}

/**
 * Sets the tactical shade.
 * @param shade - the tactical shade
 */
void SavedBattleGame::setTacticalShade(int shade)
{
	_tacticalShade = shade;
}

/**
 * Gets the tactical shade.
 * @return, the tactical shade
 */
int SavedBattleGame::getTacticalShade() const
{
	return _tacticalShade;
}

/**
 * Gets the map-width.
 * @return, the map-width (Size X) in tiles
 */
int SavedBattleGame::getMapSizeX() const
{
	return _mapsize_x;
}

/**
 * Gets the map-length.
 * @return, the map-length (Size Y) in tiles
 */
int SavedBattleGame::getMapSizeY() const
{
	return _mapsize_y;
}

/**
 * Gets the map-height.
 * @return, the map-height (Size Z) in layers
 */
int SavedBattleGame::getMapSizeZ() const
{
	return _mapsize_z;
}

/**
 * Gets the total quantity of individual Tiles on the battlefield.
 * @return, the total map-size in tiles
 */
size_t SavedBattleGame::getMapSizeXYZ() const
{
	return _qtyTilesTotal;
}

/**
 * Sets the terrain-type string.
 * @param terrain - the terrain-type
 */
void SavedBattleGame::setBattleTerrain(const std::string& terrain)
{
	_terrain = terrain;
}

/**
 * Gets the terrain-type string.
 * @return, the terrain-type
 */
std::string SavedBattleGame::getBattleTerrain() const
{
	return _terrain;
}

/**
 * Converts a tile-index to coordinates.
 * @param index	- the unique tileindex
 * @param x		- pointer to the x-coordinate
 * @param y		- pointer to the y-coordinate
 * @param z		- pointer to the z-coordinate
 */
void SavedBattleGame::tileCoords(
		size_t index,
		int* x,
		int* y,
		int* z) const
{
	const int
		i (static_cast<int>(index)),
		area (_mapsize_x * _mapsize_y);

	*z =  i / area;
	*y = (i % area) / _mapsize_x;
	*x = (i % area) % _mapsize_x;
}

/**
 * Gets the currently selected BattleUnit.
 * @return, pointer to a BattleUnit
 */
BattleUnit* SavedBattleGame::getSelectedUnit() const
{
	return _selectedUnit;
}

/**
 * Sets the currently selected BattleUnit.
 * @param unit - pointer to a BattleUnit (default nullptr)
 */
void SavedBattleGame::setSelectedUnit(BattleUnit* const unit)
{
	_selectedUnit = unit;
}

/**
 * Selects the next BattleUnit.
 * @note Also used for/by the AI in BattlescapeGame::think(), popState(),
 * handleUnitAI(), and in SavedBattleGame::factionEndTurn().
 * @param dontReselect		- true to set the current unit's reselectable flag FALSE (default false)
 * @param checkReselect		- true to check the next unit's reselectable flag (default false)
 * @param checkInventory	- true to check if the next unit has no inventory (default false)
 * @return, pointer to newly selected BattleUnit or nullptr if none can be selected
 * @sa selectUnit
 */
BattleUnit* SavedBattleGame::selectNextUnit(
		bool dontReselect,
		bool checkReselect,
		bool checkInventory)
{
	return selectUnit(
					+1,
					dontReselect,
					checkReselect,
					checkInventory);
}

/**
 * Selects the previous BattleUnit.
 * @param dontReselect		- true to set the current unit's reselectable flag FALSE (default false)
 * @param checkReselect		- true to check the next unit's reselectable flag (default false)
 * @param checkInventory	- true to check if the next unit has no inventory (default false)
 * @return, pointer to newly selected BattleUnit or nullptr if none can be selected
 * @sa selectUnit
 */
BattleUnit* SavedBattleGame::selectPrevUnit(
		bool dontReselect,
		bool checkReselect,
		bool checkInventory)
{
	return selectUnit(
					-1,
					dontReselect,
					checkReselect,
					checkInventory);
}

/**
 * Selects the next BattleUnit in a certain direction.
 * @param dir				- direction to iterate (+1 for next and -1 for previous)
 * @param dontReselect		- true to set the current unit's reselectable flag FALSE
 * @param checkReselect		- true to check the next unit's reselectable flag
 * @param checkInventory	- true to check if the next unit has no inventory
 * @return, pointer to newly selected BattleUnit or nullptr if none can be selected
 */
BattleUnit* SavedBattleGame::selectUnit( // private.
		int dir,
		bool dontReselect,
		bool checkReselect,
		bool checkInventory)
{
	//Log(LOG_INFO) << "sbg:selectUnit()";
	if (dontReselect == true && _selectedUnit != nullptr)
	{
		//Log(LOG_INFO) << ". set dontReselect id-" << _selectedUnit->getId();
		_selectedUnit->setReselect(false);
	}


	std::vector<BattleUnit*>* units;
	switch (_side)
	{
		default:
		case FACTION_PLAYER:  units = &_units; break;
		case FACTION_HOSTILE:
		case FACTION_NEUTRAL: units = &_shuffleUnits;
	}

	std::vector<BattleUnit*>::const_iterator
		iterFirst,
		iterLast,
		iterNext;

	switch (dir)
	{
		case +1:
			iterFirst = units->begin();
			iterLast  = units->end() - 1;
			break;

		case -1:
			iterFirst = units->end() - 1;
			iterLast  = units->begin();
	}

	iterNext = std::find(
					units->begin(),
					units->end(),
					_selectedUnit);
	//Log(LOG_INFO) << ". sel id-" << ((iterNext != units->end()) ? (*iterNext)->getId() : 0);
	do
	{
		//Log(LOG_INFO) << ". . do (is NOT Selectable)";
		if (iterNext != units->end())
		{
			//Log(LOG_INFO) << ". . . sel is VALID id-" << (*iterNext)->getId();
			if (iterNext != iterLast)
				iterNext += dir;
			else
				iterNext = iterFirst;
			//Log(LOG_INFO) << ". . . next id-" << (*iterNext)->getId();

			if (*iterNext == _selectedUnit) // iter returned to itself
			{
				//Log(LOG_INFO) << ". . . . iter returned to itself";
				if (checkReselect == true && _selectedUnit->getReselect() == false)
				{
					//Log(LOG_INFO) << ". . . . . unit cannot be reselected - set sel to NULL";
					_selectedUnit = nullptr;
				}

				//Log(LOG_INFO) << ". . . . ret id-" << (_selectedUnit ? _selectedUnit->getId() : 0);
				return _selectedUnit;
			}
			else if (_selectedUnit == nullptr && iterNext == iterFirst) // no units can be selected
			{
				//Log(LOG_INFO) << ". . . . no units can be selected";
				return nullptr;
			}
		}
		else
		{
			//Log(LOG_INFO) << ". . . next is NOT Valid - set iter to begin()";
			iterNext = iterFirst;
		}
	}
	while ((*iterNext)->isSelectable(
								_side,
								checkReselect,
								checkInventory) == false);

	//Log(LOG_INFO) << ". Ret id-" << (*iterNext)->getId();
	return (_selectedUnit = *iterNext);
}

/**
 * Gets the faction-side currently playing.
 * @return, the faction currently playing (BattleUnit.h)
 */
UnitFaction SavedBattleGame::getSide() const
{
	return _side;
}

/**
 * Gets the current turn.
 * @return, the current turn
 */
int SavedBattleGame::getTurn() const
{
	return _turn;
}

/**
 * Ends the current faction's turn and progresses to the next.
 * @note Called by BattlescapeGame::endTurn().
 * @return, true if the turn rolls-over to Faction_Player
 */
bool SavedBattleGame::factionEndTurn()
{
	//Log(LOG_INFO) << "sbg:factionEndTurn() side= " << _side;

	if (_side == FACTION_PLAYER) // ie. End of Player turn
	{
//		std::string file = "tac_END_" + std::to_string(_turn);
//		_playSave->setLabel(L"tac_END_" + Text::intWide(_turn));
//		_playSave->save(file + SavedGame::SAVE_ExtDot);

		_playSave->setLabel(SavedGame::SAVELABEL_TacTurnEnd);	// this Save is done auto at the end of Player's turn
		_playSave->save(SavedGame::SAVE_TacTurnEnd);			// vid BattlescapeGame::endTurn() for start of Player turn
	}															// vid InventoryState::btnOkClick() for start of tactical


	for (std::vector<BattleUnit*>::const_iterator	// set *all* units non-selectable
			i  = _units.begin();					// Units of the upcoming turn's faction are
			i != _units.end();						// set selectable at the end.
			++i)
	{
		(*i)->setReselect(false);
	}

	switch (_side)
	{
		case FACTION_PLAYER: // end of Player turn.
			if (_selectedUnit != nullptr
				&& _selectedUnit->isMindControlled() == false)
			{
				_recallUnit = _selectedUnit;
			}
			else
				_recallUnit = nullptr;

			_selectedUnit = nullptr;

			_scanDots.clear();

			_shuffleUnits = _units; // Only Faction_Player turns use the regular '_units' vector to keep their Battle Order correct;
			RNG::shuffle(_shuffleUnits.begin(), _shuffleUnits.end()); // _Hostile and _Neutral use '_shuffleUnits'.
			// TODO: It no longer seems necessary to NULL '_shuffleUnits' at the start of Player turns below_

			_side = FACTION_HOSTILE;
			break;

		case FACTION_HOSTILE: // end of Alien turn.
			if (firstFactionUnit(FACTION_NEUTRAL) != nullptr)
			{
				_side = FACTION_NEUTRAL;
				break;
			}
			// no break;

		case FACTION_NEUTRAL: // end of Civilian turn.
		default:
			_side = FACTION_PLAYER;
	}
	//Log(LOG_INFO) << ". side= " << _side;



	// ** _side HAS ADVANCED to next faction after here!!! ** //


	int aLienIntel (0);
	switch (_side)
	{
		case FACTION_PLAYER:
		{
			++_turn;

			std::fill(
					_shuffleUnits.begin(),
					_shuffleUnits.end(),
					nullptr);

			tileVolatiles(); // do Tile stuff

			firstFactionUnit(FACTION_PLAYER); // set '_selectedUnit'

			int aLienIntelTest;
			for (std::vector<BattleUnit*>::const_iterator
					i  = _units.begin();
					i != _units.end();
					++i)
			{
				if ((*i)->getOriginalFaction() == FACTION_HOSTILE)
				{
					switch ((*i)->getUnitStatus())	// set non-aLien units not-Exposed if their current
					{								// exposure exceeds aLien's max-intel. See below_
						case STATUS_STANDING:
						case STATUS_UNCONSCIOUS:	// NOTE: Status_Unconscious does not break exposure. psycho aLiens!
							if ((aLienIntelTest = (*i)->getIntelligence()) > aLienIntel)
								aLienIntel = aLienIntelTest;
					}
				}
			}
			break;
		}

		case FACTION_HOSTILE:
			if (_cheatAI == false // pseudo the Turn-20 / less-than-3-aliens-left Exposure rule.
				&& _turn > (_cheatTurn >> 2u))
			{
				for (std::vector<BattleUnit*>::const_iterator // find a conscious non-MC'd aLien ...
						i  = _units.begin();
						i != _units.end() && _cheatAI == false;
						++i)
				{
					if ((*i)->getOriginalFaction() == FACTION_HOSTILE
						&& (*i)->isMindControlled() == false)
					{
						switch ((*i)->getUnitStatus())
						{
							case STATUS_STANDING:
							case STATUS_UNCONSCIOUS:
								const int d (RNG::generate(0,5));
								if (_turn > _cheatTurn - 3 + d
									|| _battleState->getBattleGame()->tallyHostiles() < d - 1)
								{
									_cheatAI = true;
								}
						}
					}
				}
			}
	}

	for (std::vector<BattleUnit*>::const_iterator
			i  = _units.begin();
			i != _units.end();
			++i)
	{
		switch ((*i)->getUnitStatus())
		{
			case STATUS_LATENT:
			case STATUS_LATENT_START:
				break;

			case STATUS_DEAD: // burning corpses eventually sputter out.
				if ((*i)->getFaction() == _side && (*i)->getUnitFire() != 0)
					(*i)->setUnitFire((*i)->getUnitFire() - 1);
				break;

			default:
			case STATUS_WALKING:
			case STATUS_FLYING:
			case STATUS_TURNING:
			case STATUS_AIMING:
			case STATUS_COLLAPSING:
			case STATUS_PANICKING:
			case STATUS_BERSERK:
				(*i)->setUnitStatus(STATUS_STANDING); // safety.
				// no break;

			case STATUS_STANDING:
				if ((_cheatAI == true && (*i)->psiBlock() == false)		// aLiens know where xCom is when cheating ~turn20
					|| (*i)->getFaction() == FACTION_HOSTILE			// aLiens always know where their buddies are,
					|| (*i)->getOriginalFaction() == FACTION_HOSTILE)	// Mc'd or not.
				{
					(*i)->setExposed();
				}
				else if (_side == FACTION_PLAYER)
				{
					int exposure ((*i)->getExposed());
					if (exposure != -1)
					{
						if ((*i)->psiBlock() == true || ++exposure > aLienIntel)
							(*i)->setExposed(-1);
						else
							(*i)->setExposed(exposure);
					}
				}
				// no break;

			case STATUS_UNCONSCIOUS:
				if ((*i)->getOriginalFaction() == _side)
					(*i)->hitUnitFire();			// the return to BattlescapeGame::endTurn() will handle casualties.

				if ((*i)->getFaction() == _side)	// This causes an Mc'd unit to lose its turn.
				{
					(*i)->prepareUnit();			// set Reselectable, REVERT FACTION, do tu/stun/fire recovery, determine panic, etc.
					checkUnitRevival(*i);
				}
				// if newSide=XCOM, xCom agents DO NOT revert to xCom; MC'd aLiens revert to aLien.
				// if newSide=Alien, xCom agents revert to xCom; MC'd aLiens DO NOT revert to aLien.
				// etc.

				switch ((*i)->getUnitStatus())	// NOTE: prepareUnit() can change status to Panic or Berserk.
				{								// NOTE: checkUnitRevival() can change status to Standing.
					case STATUS_STANDING:
					case STATUS_PANICKING:
					case STATUS_BERSERK:
						(*i)->setUnitVisible((*i)->getFaction() == FACTION_PLAYER);

						(*i)->setDashing(false);	// - no longer dashing; dash is effective vs. Reaction Fire only and
													// is/ought be reset/removed every time BattlescapeGame::primaryAction()
													// uses the Pathfinding object. Other, more ideal places for this
													// are UnitWalkBState dTor and/or BattlescapeGame::popState().
													//
													// NOTE: Panic also uses dash; but UnitPanicBState turns it off again.
				}
		}
	}

	_te->calculateSunShading();
	_te->calculateTerrainLighting();
	_te->calculateUnitLighting(); // turn off MC'd aLien-lighting.

	_te->calcFovUnits_all(); // do calcFov() *after* aLiens & civies have been set non-visible above^

	return (_side == FACTION_PLAYER);
}

/**
 * Selects the first BattleUnit of faction at the start of each faction-turn.
 * @note This does NOT set '_selectedUnit' unless it's the beginning of a Player
 * turn or no unit of faction can be found -- otherwise bg:think() handles it.
 * @param faction - faction of unit to select (BattleUnit.h)
 * @return, pointer to the selected unit or nullptr
 */
BattleUnit* SavedBattleGame::firstFactionUnit(UnitFaction faction)
{
	if (_side == FACTION_PLAYER && _recallUnit != nullptr
		&& _recallUnit->isSelectable(FACTION_PLAYER) == true)
	{
		return (_selectedUnit = _recallUnit);
	}

	std::vector<BattleUnit*>* units;
	switch (_side)
	{
		default:
		case FACTION_PLAYER:  units = &_units; break;
		case FACTION_HOSTILE:
		case FACTION_NEUTRAL: units = &_shuffleUnits;
	}

	for (std::vector<BattleUnit*>::const_iterator
			i = units->begin();
			i != units->end();
			++i)
	{
		if ((*i)->isSelectable(faction) == true)
		{
			if (_side == FACTION_PLAYER)
				_selectedUnit = *i;
			return *i;
		}
	}

	return (_selectedUnit = nullptr);
}

/**
 * Turns on tactical debug-mode.
 */
void SavedBattleGame::debugTac()
{
	_debugTac = true;

	for (size_t // reveal tiles.
			i = 0u;
			i != _qtyTilesTotal;
			++i)
	{
		_tiles[i]->setRevealed(
							ST_CONTENT,
							true,
							true); // force
	}
}

/**
 * Gets the current tactical debug-mode setting.
 * @return, debug mode
 */
bool SavedBattleGame::getDebugTac() const
{
	return _debugTac;
}

/**
 * Gets the list of Nodes.
 * @return, pointer to a vector of pointers to the Nodes
 */
std::vector<Node*>* SavedBattleGame::getNodes()
{
	return &_nodes;
}

/**
 * Gets the list of BattleUnits.
 * @return, pointer to a vector of pointers to the BattleUnits
 */
std::vector<BattleUnit*>* SavedBattleGame::getUnits()
{
	return &_units;
}

/**
 * Gets the list of shuffled BattleUnits.
 * @return, pointer to a vector of pointers to the BattleUnits
 */
std::vector<BattleUnit*>* SavedBattleGame::getShuffleUnits()
{
	return &_shuffleUnits;
}

/**
 * Gets the list of BattleItems.
 * @return, pointer to a vector of pointers to the BattleItems
 */
std::vector<BattleItem*>* SavedBattleGame::getItems()
{
	return &_items;
}

/**
 * Gets the Pathfinding object.
 * @return, pointer to Pathfinding
 */
Pathfinding* SavedBattleGame::getPathfinding() const
{
	return _pf;
}

/**
 * Gets the terrain-modifier object.
 * @return, pointer to the TileEngine
 */
TileEngine* SavedBattleGame::getTileEngine() const
{
	return _te;
}

/**
 * Gets the list of MCDs for the battlefield.
 * @return, pointer to a vector of pointers to MapDataSets
 */
std::vector<MapDataSet*>* SavedBattleGame::getBattleDataSets()
{
	return &_battleDataSets;
}

/**
 * Gets a pointer to the Geoscape save.
 * @return, pointer to SavedGame
 */
SavedGame* SavedBattleGame::getSavedGame() const
{
	return _playSave;
}

/**
 * Gets the BattlescapeGame.
 * @note During battlescape-generation BattlescapeState is not valid yet.
 * @return, pointer to the BattlescapeGame
 */
BattlescapeGame* SavedBattleGame::getBattleGame() const
{
	if (_battleState != nullptr)
		return _battleState->getBattleGame();

	return nullptr;
}

/**
 * Gets the BattlescapeState.
 * @note During battlescape-generation BattlescapeState is not valid yet.
 * @return, pointer to the BattlescapeState
 */
BattlescapeState* SavedBattleGame::getBattleState() const
{
	return _battleState;
}

/**
 * Sets the BattlescapeState.
 * @param battleState - pointer to the BattlescapeState
 */
void SavedBattleGame::setBattleState(BattlescapeState* const battleState)
{
	_battleState = battleState;
}

/**
 * Places all BattleUnits at their start-Tile(s) at the end of pre-battle Equip.
 */
void SavedBattleGame::positionUnits()
{
	Position pos;
	for (size_t
			i = 0u;
			i != _qtyTilesTotal;
			++i)
	{
		_tiles[i]->setTileUnit(); // first break all Tile->BattleUnit links.
	}

	for (std::vector<BattleUnit*>::const_iterator
			i = _units.begin();
			i != _units.end();
			++i)
	{
		if ((*i)->getUnitStatus() == STATUS_STANDING)
		{
			if ((*i)->getFaction() == FACTION_PLAYER)
				(*i)->setUnitVisible();

			pos = (*i)->getPosition();
			(*i)->setUnitTile(
							getTile(pos),
							getTile(pos + Position::POS_BELOW)); // second set BattleUnit->Tile link for the unit's primary quadrant

			const int unitSize ((*i)->getArmor()->getSize() - 1);
			for (int
					x = unitSize;
					x != -1;
					--x)
			{
				for (int
						y = unitSize;
						y != -1;
						--y)
				{
					getTile(pos + Position(x,y,0))->setTileUnit(*i); // third set Tile->BattleUnit links for each unit-quadrant
				}
			}
		}
	}
}

/**
 * Gives access to the storagePositions vector for distribution of items in base
 * defense missions.
 * @return, reference a vector of storage positions
 */
std::vector<Position>& SavedBattleGame::storagePositions()
{
	return _storageSpace;
}

/**
 * Move all the leftover items in base defense missions to random locations in
 * the storage facilities.
 * @param tile - pointer to a tile where all the goodies are placed
 */
void SavedBattleGame::distributeEquipt(Tile* const tile)
{
	if (_storageSpace.empty() == false)
	{
		for (std::vector<BattleItem*>::const_iterator
				i = tile->getInventory()->begin();
				i != tile->getInventory()->end();
				)
		{
			if ((*i)->getInventorySection()->getSectionType() == ST_GROUND)
			{
				getTile(_storageSpace.at(RNG::pick(_storageSpace.size())))->addItem(*i);
				i = tile->getInventory()->erase(i);
			}
			else
				++i;
		}
	}
}

/**
 * Removes an item from the game - when ammo item is depleted for example.
 * @note Items need to be checked for removal from three vectors:
 *		- tile inventory
 *		- battleunit inventory
 *		- battlescape-items container
 * If the item is xComProperty upon removal the pointer to the item is kept in
 * the '_deletedProperty' vector which is flushed and destroyed in the
 * SavedBattleGame dTor.
 * @note See note for BattleItem::changeOwner().
 * @param it - pointer to an item to remove
 * @return, const_iterator to the next item in the BattleItems list
 */
std::vector<BattleItem*>::const_iterator SavedBattleGame::sendItemToDelete(BattleItem* const it)
{
	it->changeOwner();

	Tile* const tile (it->getTile());
	if (tile != nullptr)
		tile->removeItem(it);

	for (std::vector<BattleItem*>::const_iterator
			i  = _items.begin();
			i != _items.end();
			++i)
	{
		if (*i == it)
		{
			if ((*i)->isProperty() == true)
				_deletedProperty.push_back(it);
			else
				delete *i;

			return _items.erase(i);
		}
	}
	return _items.end();
}

/**
 * Gives read-only access to the deleted-property vector.
 * @return, const-ref to a vector of pointers to deleted BattleItems
 */
const std::vector<BattleItem*>& SavedBattleGame::deletedProperty() const
{
	return _deletedProperty;
}

/**
 * Sets whether the mission was aborted or successful.
 * @param abort - true if tactical was aborted or false if successful
 */
void SavedBattleGame::isAborted(bool abort)
{
	_aborted = abort;
}

/**
 * Gets whether the mission was aborted or successful.
 * @return, true if the mission was aborted or false if successful
 */
bool SavedBattleGame::isAborted() const
{
	return _aborted;
}

/**
 * Sets the objective-tiletype for the current battle.
 * @param type - objective-tiletype (RuleItem.h)
 */
void SavedBattleGame::setObjectiveTilepartType(TilepartSpecial specialType)
{
	_objectiveType = specialType;
}

/**
 * Gets the objective-tiletype for the current battle.
 * @return, objective-tiletype (RuleItem.h)
 */
TilepartSpecial SavedBattleGame::getObjectiveTilepartType() const
{
	return _objectiveType;
}

/**
 * Initializes the objective-tiles needed quantity.
 * @note Used only to initialize the objective counter; cf addDestroyedObjective()
 * below. Objectives were tile-parts marked w/ OBJECTIVE_TILE in their MCD but now
 * can be any specially marked tile. See elsewhere ha.
 * @param qty - quantity of objective-tiletype tile-parts that need to be
 *				destroyed for a/the tactical-objective to be achieved
 */
void SavedBattleGame::initObjectives(int qty)
{
	_objectivesRequired = qty;
	_objectivesDestroyed = 0;
}

/**
 * Increments the objective-tiles destroyed and checks if the necessary quantity
 * has been destroyed.
 */
void SavedBattleGame::addDestroyedObjective()
{
	if (allObjectivesDestroyed() == false)
	{
		++_objectivesDestroyed;
		if (allObjectivesDestroyed() == true)
		{
			_controlDestroyed = true;
			_battleState->getBattleGame()->objectiveSuccess();
		}
	}
}

/**
 * Checks if enough objective-tiles have been destroyed.
 * @return, true if enough for a win-condition
 */
bool SavedBattleGame::allObjectivesDestroyed() const
{
	return _objectivesRequired != 0
		&& _objectivesDestroyed >= _objectivesRequired;
}

/**
 * Sets the highest available item-ID.
 * @note Used only at the finish of loading a SavedBattleGame.
 * @note ItemIDs start at 0.
 */
void SavedBattleGame::setCanonicalBattleId()
{
	int
		id (-1),
		idTest;

	for (std::vector<BattleItem*>::const_iterator
			i  = _items.begin();
			i != _items.end();
			++i)
	{
		if ((idTest = (*i)->getId()) > id)
			id = idTest;
	}
	_itemId = ++id;
}

/**
 * Gets the highest available item-ID value.
 * @note It is incremented by BattleItem() cTor.
 * @return, pointer to the highest available id#
 */
int* SavedBattleGame::getCanonicalBattleId()
{
	return &_itemId;
}

/**
 * Finds a suitable Node where a specified BattleUnit can spawn.
 * @note bgen.addAlien() uses a fallback mechanism to test assorted nodeRanks.
 * @param unitRank	- rank of the unit attempting to spawn
 * @param unit		- pointer to the unit (to test-set its position)
 * @return, pointer to the Node or nullptr
 */
Node* SavedBattleGame::getSpawnNode(
		int unitRank,
		BattleUnit* const unit)
{
	std::vector<Node*> nodes;
	for (std::vector<Node*>::const_iterator
			i  = _nodes.begin();
			i != _nodes.end();
			++i)
	{
		if ((*i)->getSpawnWeight() != 0			// spawn-priority 0 is not spawnplace
			&& (*i)->getNodeRank() == unitRank	// ranks must match
			&& isNodeType(*i, unit)				// unit's size and walk-type must match node's
			&& setUnitPosition(					// check if unit can be set at this node
							unit,					// ie. node is big enough
							(*i)->getPosition(),	// and there's not already a unit there.
							true) == true)			// test-only: runs again w/ FALSE on return to bgen::addAlien()
		{
			for (int // weight each eligible node by its Priority.
					j = (*i)->getSpawnWeight();
					j != 0;
					--j)
			{
				nodes.push_back(*i);
			}
		}
	}

	if (nodes.empty() == false)
		return nodes[RNG::pick(nodes.size())];

	return nullptr;
}

/**
 * Finds a suitable Node where a specified BattleUnit can patrol to.
 * @param scout	- true if the unit is scouting
 * @param unit	- pointer to a BattleUnit
 * @param start	- pointer to the node that unit is currently at
 * @return, pointer to the destination Node
 */
Node* SavedBattleGame::getPatrolNode(
		bool scout,
		BattleUnit* const unit,
		Node* start)
{
	if (start == nullptr)
		start = getStartNode(unit);

	std::vector<Node*>
		nodesScout,
		nodesOfficer;
	Node* node;

	size_t qtyNodes;
	if (scout == true)
		qtyNodes = getNodes()->size();
	else
		qtyNodes = Node::NODE_LINKS;

	for (size_t
			i = 0u;
			i != qtyNodes;
			++i)
	{
		if (scout == true || start->getLinks()->at(i) > -1)	// non-scouts need Links to travel along.
		{													// N-E-S-W directions are never used (linkId's -2,-3,-4,-5).
			if (scout == true)								// Meaning that non-scouts never leave their spawn-block ...
				node = getNodes()->at(i);
			else
				node = getNodes()->at(static_cast<size_t>(start->getLinks()->at(i)));

			if ((node->getPatrolPriority() != 0								// for non-scouts find a node with a desirability above 0
					|| scout == true
					|| node->getNodeRank() > NR_SCOUT)
				&& node->isAllocated() == false								// check if not allocated
				&& isNodeType(node, unit)
				&& setUnitPosition(											// check if unit can be set at this node
								unit,											// ie. it's big enough
								node->getPosition(),							// and there's not already a unit there.
								true) == true									// but don't actually set the unit...
				&& getTile(node->getPosition()) != nullptr					// the node is on a valid tile
				&& getTile(node->getPosition())->getFire() == 0				// you are not a firefighter; do not patrol into fire
				&& (getTile(node->getPosition())->getDangerous() == false	// aliens don't run into a grenade blast
					|| unit->getFaction() != FACTION_HOSTILE)					// but civies do!
				&& (node != start											// scouts push forward
					|| scout == false))											// others can mill around.. ie, stand there.
			{
				for (int
						j = node->getPatrolPriority(); // weight each eligible node by its patrol-Flags.
						j != -1;
						--j)
				{
					nodesScout.push_back(node);

					if (scout == false
						&& node->getNodeRank() == Node::nodeRank[static_cast<size_t>(unit->getRankInt())]
																[0u]) // high-class node here.
					{
						nodesOfficer.push_back(node);
					}
				}
			}
		}
	}

	if (nodesScout.empty() == true)
	{
		if (scout == false && unit->getArmor()->getSize() == 2)
		{
//			return Sectopod::CTD();
			return getPatrolNode(true, unit, start); // recurse w/ scout=true
		}
		return nullptr;
	}

	if (scout == true // picks a random destination
		|| nodesOfficer.empty() == true
		|| RNG::percent(17) == true) // officers can go for a stroll ...
	{
		return nodesScout[RNG::pick(nodesScout.size())];
	}
	return nodesOfficer[RNG::pick(nodesOfficer.size())];
}

/**
 * Gets a Node considered nearest to a specified BattleUnit.
 * @note Assume closest node is on same level to avoid strange things. The node
 * has to match unit-size or the AI will freeze.
 * @param unit - pointer to a BattleUnit
 * @return, the nearest Node
 */
Node* SavedBattleGame::getStartNode(const BattleUnit* const unit) const
{
	Node* node (nullptr);
	int
		dist (1000000),
		distTest;

	const int posZ (unit->getPosition().z);
	const bool onequad (unit->getArmor()->getSize() == 1);

	// NOTE/TODO: This doesn't check for Flying unit/nodetype - that is it
	// assumes that non-flying units can start from a flying-only nodetype.

	for (std::vector<Node*>::const_iterator
			i  = _nodes.begin();
			i != _nodes.end();
			++i)
	{
		if ((*i)->getPosition().z == posZ
			&& (onequad == true
				|| (((*i)->getUnitType() & (Node::TYPE_SMALL | Node::TYPE_SMALLFLYING)) == 0)))
		{
			distTest = TileEngine::distSqr(
									(*i)->getPosition(),
									unit->getPosition());
			if (distTest < dist)
			{
				dist = distTest;
				node = *i;
			}
		}
	}
	return node;
}

/**
 * Gets if a specified BattleUnit can use a specified Node.
 * @note Small units are allowed to use Large nodes and flying units are
 * allowed to use nonFlying nodes. The basic node-types are set in
 * BattlescapeGenerator::loadRouteFile().
 * @param node - pointer to a node
 * @param unit - pointer to a unit trying to use the node
 * @return, true if unit can use node
 */
bool SavedBattleGame::isNodeType(
		const Node* const node,
		const BattleUnit* const unit) const
{
	const int type (node->getUnitType());

	if ((type & Node::TYPE_DANGEROUS) == 0)					// +16	Only Type_Dangerous is ever added to a
	{														//		stock nodeType in the code currently.
		switch (type)
		{
			case Node::TYPE_SMALLFLYING:					// 1
				return unit->getArmor()->getSize() == 1
					&& unit->getMoveTypeUnit() == MT_FLY;

			case Node::TYPE_SMALL:							// 2
				return unit->getArmor()->getSize() == 1;

			case Node::TYPE_LARGEFLYING:					// 4
				return unit->getMoveTypeUnit() == MT_FLY;

//			case 0:											// Any.
//			case Node::TYPE_LARGE:							// 8 All units can use Type_Large.
//				break;
		}
		return true;
	}
	return false;
}

/**
 * Carries out full-turn preparations such as fire and smoke spreading.
 * @note Also explodes any explosive Tiles that get destroyed by fire.
 */
void SavedBattleGame::tileVolatiles() // private.
{
	std::vector<Tile*>
		tilesFired,
		tilesSmoked;

	Tile* tile;
	for (size_t
			i = 0u;
			i != _qtyTilesTotal;
			++i)
	{
		tile = getTiles()[i];
		if (tile->getFire() != 0)
			tilesFired.push_back(tile);

		if (tile->getSmoke() != 0)
			tilesSmoked.push_back(tile);

		tile->setDangerous(false); // reset.
	}

	int var;
	for (std::vector<Tile*>::const_iterator // TODO: Spread fires upward similar to smoke below_
			i  = tilesFired.begin();
			i != tilesFired.end();
			++i)
	{
		if ((var = (*i)->decreaseFire()) != 0)
		{
			var <<= 3u;
			for (int
					dir = 0;
					dir != 8;
					dir += 2)
			{
				Position pos;
				Pathfinding::directionToVector(dir, &pos);

				if ((tile = getTile((*i)->getPosition() + pos)) != nullptr
					&& _te->horizontalBlockage(*i, tile, DT_IN) == 0)
				{
					tile->igniteTile(var);
				}
			}
		}
		else // try to destroy object- and/or floor-parts when Fire goes out.
		{
			if ((*i)->getMapData(O_OBJECT) != nullptr)
			{
				if (   (*i)->getMapData(O_OBJECT)->getFlammable()   != 255
					&& (*i)->getMapData(O_OBJECT)->getArmorPoints() != 255) // NOTE: Also checked in destroyTilepart().
				{
					(*i)->destroyTilepart(O_OBJECT, this);
					(*i)->destroyTilepart(O_FLOOR, this);	// NOTE: There is no assurance that the current floor-part is actually 'flammable';
				}											// but it ensures that if there's not a current floor-part then
			}												// at least a scorched-earth floor-part gets laid down.
			else if ((*i)->getMapData(O_FLOOR) != nullptr)
			{
				if (   (*i)->getMapData(O_FLOOR)->getFlammable()   != 255
					&& (*i)->getMapData(O_FLOOR)->getArmorPoints() != 255) // NOTE: Also checked in destroyTilepart().
				{
					(*i)->destroyTilepart(O_FLOOR, this);
				}
			}
			_te->applyGravity(*i);
		}
	}

	for (std::vector<Tile*>::const_iterator
			i  = tilesSmoked.begin();
			i != tilesSmoked.end();
			++i)
	{
		if ((var = (*i)->decreaseSmoke() / 3) > 1)
		{
			if ((tile = (*i)->getTileAbove(this)) != nullptr
				&& tile->isFloored(*i) == false) // TODO: Use verticalBlockage() instead.
			{
				tile->addSmoke(var);
			}

			for (int
					dir = 0;
					dir != 8;
					dir += 2)
			{
				if (RNG::percent(var << 3u) == true)
				{
					Position pos;
					Pathfinding::directionToVector(dir, &pos);

					if ((tile = getTile((*i)->getPosition() + pos)) != nullptr
						&& _te->horizontalBlockage(*i, tile, DT_SMOKE) == 0)
					{
						tile->addSmoke(var);
					}
				}
			}
		}
	}
}

/**
 * Checks for and revives unconscious BattleUnits.
 * @param faction - the faction to check
 *
void SavedBattleGame::reviveUnits(const UnitFaction faction)
{
	for (std::vector<BattleUnit*>::const_iterator
			i = _units.begin();
			i != _units.end();
			++i)
	{
		if ((*i)->getOriginalFaction() == faction
			&& (*i)->getUnitStatus() != STATUS_DEAD) // etc. See below_
		{
			checkUnitRevival(*i, true);
		}
	}
} */

/**
 * Checks if unit is unconscious and revives it if it shouldn't be.
 * @note Revived units need a tile to stand on. If the unit's current position
 * is occupied then all directions around the tile are searched for a free tile
 * to place the unit on. If no free tile is found the unit stays unconscious.
 * @param unit - pointer to a BattleUnit to try to revive
 */
void SavedBattleGame::checkUnitRevival(BattleUnit* const unit)
{
	if (unit->isRevivable() == true && unit->isStunned() == false)
	{
		Position pos (unit->getPosition());

		if (pos == Position::POS_BOGUS) // if carried
		{
			for (std::vector<BattleItem*>::const_iterator
					i  = _items.begin();
					i != _items.end();
					++i)
			{
				if ((*i)->getBodyUnit() == unit)
				{
					pos = (*i)->getOwner()->getPosition();
					break;
				}
			}
		}

		const Tile* const tile (getTile(pos));
		bool largeOther (tile != nullptr
					  && tile->getTileUnit() != nullptr
					  && tile->getTileUnit() != unit
					  && tile->getTileUnit()->getArmor()->getSize() == 2);

		if (placeUnitByPosition(unit, pos, largeOther) == true)
		{
			switch (unit->getFaction()) // faction will be Original here due to death/stun sequence.
			{
				case FACTION_HOSTILE:
					unit->setExposed();
					break;

				case FACTION_PLAYER:
					unit->setUnitVisible();
					if (unit->getGeoscapeSoldier() != nullptr)
						unit->kneelUnit(true);
					// no break;

				case FACTION_NEUTRAL:
					unit->setExposed(-1);
			}

			unit->setUnitStatus(STATUS_STANDING);
			unit->setUnitDirection(RNG::generate(0,7));
			unit->setReselect(unit->getFaction() == _side);

			unit->setCacheInvalid();
			_battleState->getMap()->cacheUnitSprite(unit);

			deleteBody(unit);

			_te->applyGravity(unit->getUnitTile()); // if unit was carried by a flying unit.

			_te->calculateUnitLighting();
			if (unit->getFaction() == FACTION_PLAYER)
				_te->calcFovTiles(unit);
			_te->calcFovUnits_pos(unit->getPosition());

			_battleState->updateMedicIcons();
		}
	}
}

/**
 * Sends the body-item that corresponds to a BattleUnit to the deleted vector.
 * @param unit - pointer to a BattleUnit
 */
void SavedBattleGame::deleteBody(const BattleUnit* const unit)
{
	int quads (unit->getArmor()->getSize() * unit->getArmor()->getSize());
	for (std::vector<BattleItem*>::const_iterator
			i  = _items.begin();
			i != _items.end();
			)
	{
		if ((*i)->getBodyUnit() == unit)
		{
			i = sendItemToDelete(*i);
			if (--quads == 0) return;
		}
		else
			++i;
	}
}

/**
 * Sets or tries to set a BattleUnit of a certain size on a certain Position of
 * the battlefield.
 * @note Also handles large units that are placed on multiple tiles unlike
 * BattleUnit::setPosition().
 * @param unit	- pointer to a unit to be placed
 * @param pos	- reference to the position to try to place the unit
 * @param test	- true only checks if unit can be placed at @a pos (default false)
 * @return, true if unit was placed successfully
 */
bool SavedBattleGame::setUnitPosition(
		BattleUnit* const unit,
		const Position& pos,
		bool test) const
{
//	if (unit != nullptr)
//	{
//	_pf->setPathingUnit(unit); // <- this is not valid when doing fake-inventory. NOTE: Craft/Base-equip & pre-battle equip prob. don't call this anymore.
	Position pos0 (pos); // strip const.
	const Tile* tile;

	const int unitSize (unit->getArmor()->getSize() - 1);
	for (int
			x = unitSize;
			x != -1;
			--x)
	{
		for (int
				y = unitSize;
				y != -1;
				--y)
		{
			if ((tile = getTile(pos0 + Position(x,y,0))) != nullptr)
			{
				if (tile->getTerrainLevel() == -24) // NOTE: This never runs ... hopefully.
				{
					pos0 += Position::POS_ABOVE;
					x =
					y = unitSize + 1; // start over.
					break;
				}

				if ((tile->getTileUnit() != nullptr && tile->getTileUnit() != unit)
					|| tile->getTuCostTile(
										O_OBJECT,
										unit->getMoveTypeUnit()) == 255
					|| (unit->getMoveTypeUnit() != MT_FLY // <- so just use the unit's moveType.
						&& tile->isFloored(tile->getTileBelow(this)) == false))
				{
					return false;
				}

				if (tile->getMapData(O_OBJECT) != nullptr)
				{
					switch (tile->getMapData(O_OBJECT)->getBigwall())
					{
						case BIGWALL_BLOCK:
						case BIGWALL_NESW:
						case BIGWALL_NWSE:
							return false;
					}
				}

				const Tile* const tileAbove (tile->getTileAbove(this)); // TODO: check for ceiling also.
				if (tileAbove != nullptr
					&& tileAbove->getTileUnit() != nullptr
					&& tileAbove->getTileUnit() != unit
					&& unit->getHeight(true) - tile->getTerrainLevel() > Pathfinding::UNIT_HEIGHT) // don't stuck yer head up someone's flying arse.
				{
					return false;
				}
			}
			else
				return false;
		}
	}

	if (unitSize == 1) // -> however, large units never use base equip, so _pf is valid here.
	{
		_pf->setPathingUnit(unit);
		for (int
				dir = 2;
				dir != 5;
				++dir)
		{
			if (_pf->isBlockedDir(getTile(pos0), dir) == true)
				return false;
		}
	}

	if (test == false)
	{
		unit->setPosition(pos0);
		unit->setUnitTile(
						getTile(pos0),
						getTile(pos0 + Position::POS_BELOW));
		for (int
				x = unitSize;
				x != -1;
				--x)
		{
			for (int
					y = unitSize;
					y != -1;
					--y)
			{
				getTile(pos0 + Position(x,y,0))->setTileUnit(unit);
			}
		}
	}
	return true;
//	}
//	return false;
}

/**
 * Places a specified BattleUnit at or near a specified Position.
 * @param unit	- pointer to a BattleUnit to place
 * @param pos	- reference to the position at or around which to attempt to place @a unit
 * @param large	- true to account for large unit at @a pos
 * @return, true if placed
 */
bool SavedBattleGame::placeUnitByPosition(
		BattleUnit* const unit,
		const Position& pos,
		bool large) const
{
	if (setUnitPosition(unit, pos) == true)
		return true;

	int
		size1 (-unit->getArmor()->getSize()),
		size2 ((large == true) ? 2 : 1),
		xArray[8u] {    0, size2, size2, size2,     0, size1, size1, size1},
		yArray[8u] {size1, size1,     0, size2, size2, size2,     0, size1};

	const Tile* tile;
	Position pos0;

	const unsigned dir (static_cast<unsigned>(RNG::generate(0,7)));
	for (unsigned
			i = dir;
			i != dir + 8u;
			++i)
	{
		pos0 = pos + Position(				// TODO: If unit is large and/or there is a large unit at pos
							xArray[i % 8],	// this algorithm skips Tiles that should be valid to check.
							yArray[i % 8],
							0);
		if ((tile = getTile(pos0)) != nullptr
			&& getPathfinding()->isBlockedDir(tile, static_cast<int>(dir)) == false
			&& setUnitPosition(unit, pos0) == true)
		{
			return true;
		}
	}
	return false;
}
//	if (unit->getMovementType() == MT_FLY) // uhh no.
//	{
//		Tile* tile (getTile(pos + Position::POS_ABOVE));
//		if (tile
//			&& tile->isFloored(getTile(pos)) == false
//			&& setUnitPosition(unit, pos + Position::POS_ABOVE))
//		{
//			return true;
//		}
//	}

/**
 * Adds this unit to the vector of bonking units if it's not already there.
 * @param unit - pointer to the unit to add
 * @return, true if the unit was added
 */
bool SavedBattleGame::addBonker(BattleUnit* const unit)
{
	for (std::list<BattleUnit*>::const_iterator
			i  = _bonkers.begin();
			i != _bonkers.end();
			++i)
	{
		if (unit == *i) return false;
	}
	_bonkers.push_front(unit);
	return (_bonk = true);
}

/**
 * Gets all units in the battlescape that are bonking.
 * @return, pointer to the list of pointers to the bonking BattleUnits
 */
std::list<BattleUnit*>* SavedBattleGame::getBonkers()
{
	return &_bonkers;
}

/**
 * Accesses the '_bonk' bool.
 * @note The flag that says: Units are bonking, start UnitBonkBState.
 * @return, reference to the toggle
 */
bool& SavedBattleGame::doBonks()
{
	return _bonk;
}

/**
 * Gets the highest ranked, living, non Mc'd unit of faction.
 * @param allies	- reference to the number of allied units that will be conscious and not MC'd
 * @param xcom		- true if examining Faction_Player, false for Faction_Hostile (default true)
 * @return, pointer to highest ranked BattleUnit of faction
 */
const BattleUnit* SavedBattleGame::getHighestRanked(
		int& allies,
		bool xcom) const
{
	//Log(LOG_INFO) << "SavedBattleGame::getHighestRanked() xcom = " << xcom;
	const BattleUnit* leader (nullptr);
	allies = 0;

	for (std::vector<BattleUnit*>::const_iterator
			i  = _units.begin();
			i != _units.end();
			++i)
	{
		if ((*i)->isOut_t(OUT_STAT) == false)
		{
			if (xcom == true)
			{
				//Log(LOG_INFO) << "SavedBattleGame::getHighestRanked(), side is Xcom";
				if ((*i)->getFaction() == FACTION_PLAYER
					&& (*i)->isMindControlled() == false)
				{
					++allies;

					if (leader == nullptr || (*i)->getRankInt() > leader->getRankInt())
						leader = *i;
				}
			}
			else if ((*i)->getFaction() == FACTION_HOSTILE
				&& (*i)->isMindControlled() == false)
			{
				//Log(LOG_INFO) << "SavedBattleGame::getHighestRanked(), side is aLien";
				++allies;

				if (leader == nullptr || (*i)->getRankInt() < leader->getRankInt())
					leader = *i;
			}
		}
	}
	//if (leader) Log(LOG_INFO) << ". leaderID = " << leader->getId();
	//else Log(LOG_INFO) << ". leaderID = 0";
	return leader;
}

/**
 * Gets a morale modifier.
 * Either a bonus/penalty for faction based on the highest ranked living unit
 * of the faction or a penalty for a single deceased BattleUnit.
 * @param unit - pointer to BattleUnit deceased; higher rank is higher penalty (default nullptr)
 * @param xcom - if no unit is passed in this determines whether penalty applies to xCom or aLiens (default true)
 * @return, morale modifier
 */
int SavedBattleGame::getMoraleModifier( // note: Add bonus to aLiens for Cydonia & Final Assault.
		const BattleUnit* const unit,
		bool xcom) const
{
	//Log(LOG_INFO) << "SavedBattleGame::getMoraleModifier()";
	int modifier (100);

	if (unit != nullptr) // morale Loss if 'unit' is slain
	{
		//Log(LOG_INFO) << "SavedBattleGame::getMoraleModifier(), unit slain Penalty";
		switch (unit->getOriginalFaction())
		{
			case FACTION_NEUTRAL:
				return modifier;

			case FACTION_PLAYER: // xCom dies. MC'd or not
				switch (unit->getRankInt())
				{
					case 5: modifier += 30; // 200 commander
					case 4: modifier += 25; // 170 colonel
					case 3: modifier += 20; // 145 captain
					case 2: modifier += 10; // 125 sergeant
					case 1: modifier += 15; // 115 squaddie
					//Log(LOG_INFO) << ". . xCom lossModifi = " << ret;
				}
				break;

			case FACTION_HOSTILE:
				if (unit->isMindControlled() == false) // aLien dies. MC'd aliens return 100 or 50 on Mars
				{
					switch (unit->getRankInt()) // soldiers are rank #5, terrorists are ranks #6 and #7
					{
						case 0: modifier += 30; // 200 commander
						case 1: modifier += 25; // 170 leader
						case 2: modifier += 20; // 145 engineer
						case 3: modifier += 10; // 125 medic
						case 4: modifier += 15; // 115 navigator
					}

					switch (_tacType)
					{
						case TCT_MARS1: // "STR_MARS_CYDONIA_LANDING"
						case TCT_MARS2: // "STR_MARS_THE_FINAL_ASSAULT"
							modifier >>= 1u; // less hit for losing a unit on Cydonia.
					}
				}
				//Log(LOG_INFO) << ". . aLien lossModifi = " << ret;
		}
	}
	else // leadership Bonus
	{
		//Log(LOG_INFO) << "SavedBattleGame::getMoraleModifier(), leadership Bonus";
		const BattleUnit* leader;
		int allies;

		if (xcom == true)
		{
			if ((leader = getHighestRanked(allies)) != nullptr)
			{
				switch (leader->getRankInt())
				{
					case 5: modifier += 15; // 135, was 150	// commander
					case 4: modifier +=  5; // 120, was 125	// colonel
					case 3: modifier +=  5; // 115			// captain
					case 2: modifier += 10; // 110			// sergeant
					case 1: modifier += 15; // 100			// squaddie
					case 0: modifier -= 15; //  85			// rookies ...
				}
			}
			//Log(LOG_INFO) << ". . xCom leaderModifi = " << ret;
		}
		else // aLien
		{
			if ((leader = getHighestRanked(allies, false)) != nullptr)
			{
				switch (leader->getRankInt()) // terrorists are ranks #6 and #7
				{
					case 0: modifier += 25; // 150 commander
					case 1: modifier += 10; // 125 leader
					case 2: modifier +=  5; // 115 engineer
					case 3: modifier += 10; // 110 medic
					case 4: modifier += 10; // 100 navigator
					case 5: modifier -= 10; //  90 soldiers ...
				}
			}

			switch (_tacType)
			{
				case TCT_TERRORSITE:	// "STR_TERROR_MISSION"
				case TCT_BASEASSAULT:	// "STR_ALIEN_BASE_ASSAULT"
				case TCT_BASEDEFENSE:	// "STR_BASE_DEFENSE"
					modifier += 50;			// higher morale.
					break;

				case TCT_MARS1: // "STR_MARS_CYDONIA_LANDING"
				case TCT_MARS2: // "STR_MARS_THE_FINAL_ASSAULT"
					modifier += 100; // higher morale.
			}
			//Log(LOG_INFO) << ". . aLien leaderModifi = " << ret;
		}
		modifier += allies - 9; // use 9 allies as Unity.
	}
	//Log(LOG_INFO) << ". totalModifier = " << ret;
	return modifier;
}

/**
 * Resets the turn counter.
 */
void SavedBattleGame::resetTurnCounter()
{
	_turn = 1;
	_cheatAI = false;
	_side = FACTION_PLAYER;
//	_preBattle = true;
}

/**
 * Resets visibility of all the tiles on the map.
 */
void SavedBattleGame::blackTiles()
{
	for (size_t
			i = 0u;
			i != _qtyTilesTotal;
			++i)
	{
		_tiles[i]->setRevealed(ST_WEST,    false);
		_tiles[i]->setRevealed(ST_NORTH,   false);
		_tiles[i]->setRevealed(ST_CONTENT, false);
	}
}

/**
 * Gets an 11x11 grid of positions (-10 to +10 x/y).
 * @return, the tilesearch vector for use in AI functions
 */
const std::vector<Position>& SavedBattleGame::getTileSearch() const
{
	return _tileSearch;
}

/**
 * Gets if the AI has started to cheat.
 * @return, true if AI is cheating
 */
bool SavedBattleGame::isCheating() const
{
	return _cheatAI;
}

/**
 * Gets a reference to the base-module destruction map.
 * @note This map contains information on how many destructible base-modules
 * remain at any given grid reference in the basescape, using [x][y] format.
 * -1 - no objects
 *  0 - destroyed
 *  # - quantity
 * @return, reference to a vector of vectors containing pairs of ints that
 *			represent the base-modules that are intact/destroyed by aLiens
 */
std::vector<std::vector<std::pair<int,int>>>& SavedBattleGame::baseDestruct()
{
	return _baseModules;
}

/**
 * Calculates the number of map modules remaining by counting the map objects on
 * the top floor that have the baseModule flag set.
 * @note Store this data in the grid - as outlined in the comments above - in
 * pairs that represent initial and current values.
 */
void SavedBattleGame::calcBaseDestruct()
{
	_baseModules.resize(
					static_cast<size_t>(_mapsize_x / 10), // resize 1st dimension in array.
							std::vector<std::pair<int,int>>(
					static_cast<size_t>(_mapsize_y / 10), // resize 2nd dimension in array.
							std::make_pair(-1,-1)));

	const Tile* tile;
	for (int // need a bunch of size_t ->
			x = 0;
			x != _mapsize_x;
			++x)
	{
		for (int
				y = 0;
				y != _mapsize_y;
				++y)
		{
			for (int
					z = 0;
					z != _mapsize_z;
					++z)
			{
				if ((tile = getTile(Position(x,y,z))) != nullptr
					&& tile->getMapData(O_OBJECT) != nullptr
					&& tile->getMapData(O_OBJECT)->isBaseObject() == true)
				{
					_baseModules[static_cast<size_t>(x / 10)]
								[static_cast<size_t>(y / 10)].first += _baseModules[static_cast<size_t>(x / 10)]
																				   [static_cast<size_t>(y / 10)].first > 0 ? 1 : 2;
					_baseModules[static_cast<size_t>(x / 10)]
								[static_cast<size_t>(y / 10)].second = _baseModules[static_cast<size_t>(x / 10)]
																				   [static_cast<size_t>(y / 10)].first;
				}
			}
		}
	}
}

/**
 * Gets the list of items that are guaranteed to be recovered.
 * @note These items are in the transport.
 * @return, pointer to a vector of pointers to BattleItems
 */
std::vector<BattleItem*>* SavedBattleGame::recoverGuaranteed()
{
	return &_recoverGuaranteed;
}

/**
 * Gets the list of items that are NOT guaranteed to be recovered.
 * @note These items are NOT in the transport.
 * @return, pointer to a vector of pointers to BattleItems
 */
std::vector<BattleItem*>* SavedBattleGame::recoverConditional()
{
	return &_recoverConditional;
}

/**
 * Sets the player's inventory-tile when BattlescapeGenerator runs.
 * @note For use in BaseAssault/Defense tacticals to randomize item locations.
 * Also used to resolve positions for Status_Latent_Start units after a
 * 2nd-stage tactical.
 * @param equiptTile - pointer to the Tile where tactical equip't goes
 */
void SavedBattleGame::setBattleInventory(Tile* const equiptTile)
{
	_equiptTile = equiptTile;
}

/**
 * Gets the player's inventory-tile for preBattle InventoryState Ok-click.
 * @return, pointer to the tactical equip't Tile
 */
Tile* SavedBattleGame::getBattleInventory() const
{
	return _equiptTile;
}

/**
 * Sets the aLien-race for this SavedBattleGame.
 * @note Currently used only for BaseDefense missions but should fill for other
 * missions also.
 * @param alienRace - reference to an alien-race string
 */
void SavedBattleGame::setAlienRace(const std::string& alienRace)
{
	_alienRace = alienRace;
}

/**
 * Gets the aLien-race participating in this SavedBattleGame.
 * @note Currently used only to get the aLien-race for SoldierDiary statistics
 * after a BaseDefense mission.
 * @return, reference to the alien-race string
 */
const std::string& SavedBattleGame::getAlienRace() const
{
	return _alienRace;
}

/**
 * Sets ground-level.
 * @param level - ground-level as determined in BattlescapeGenerator
 */
void SavedBattleGame::setGroundLevel(const int level)
{
	_groundLevel = level;
}

/**
 * Gets ground-level.
 * @return, ground-level
 */
int SavedBattleGame::getGroundLevel() const
{
	return _groundLevel;
}

/**
 * Gets the operation-title of this SavedBattleGame.
 * @return, reference to the title
 */
const std::wstring& SavedBattleGame::getOperation() const
{
	return _operationTitle;
}

/**
 * Tells player that an aLienBase-control has been destroyed.
 *
void SavedBattleGame::setControlDestroyed()
{
	_controlDestroyed = true;
} */

/**
 * Gets if an aLienBase-control has been destroyed.
 * @return, true if destroyed
 */
bool SavedBattleGame::getControlDestroyed() const
{
	return _controlDestroyed;
}

/**
 * Gets the music-track for the current battle.
 * @return, address of the title of the music track
 *
std::string& SavedBattleGame::getMusic()
{
	return _music;
} */

/**
 * Sets the music-track for this SavedBattleGame.
 * @note The track-string is const but I don't want to deal with it.
 * @param track - reference to the track's type
 */
void SavedBattleGame::setMusic(std::string& track)
{
	_music = track;
}

/**
 * Sets variables for what music to play in a specified terrain or lack thereof.
 * @note The music-string and terrain-string are both const but I don't want to
 * deal with it.
 * @param track		- reference to the music category to play
 * @param terrain	- reference to the terrain to choose music for
 */
void SavedBattleGame::calibrateMusic(
		std::string& track,
		std::string& terrain) const
{
	if (_music.empty() == false)
		track = _music;
	else
	{
		switch (_tacType)
		{
			case TCT_UFOCRASHED:									// 0 - STR_UFO_CRASH_RECOVERY
				track = OpenXcom::res_MUSIC_TAC_BATTLE_UFOCRASHED;
				terrain = _terrain;
				break;
			case TCT_UFOLANDED:										// 1 - STR_UFO_GROUND_ASSAULT
				track = OpenXcom::res_MUSIC_TAC_BATTLE_UFOLANDED;
				terrain = _terrain;
				break;
			case TCT_BASEASSAULT:									// 2 - STR_ALIEN_BASE_ASSAULT
				track = OpenXcom::res_MUSIC_TAC_BATTLE_BASEASSAULT;
				break;
			case TCT_BASEDEFENSE:									// 3 - STR_BASE_DEFENSE
				track = OpenXcom::res_MUSIC_TAC_BATTLE_BASEDEFENSE;
				break;
			case TCT_TERRORSITE:									// 4 - STR_TERROR_MISSION and STR_PORT_ATTACK, see setTacType()
				track = OpenXcom::res_MUSIC_TAC_BATTLE_TERRORSITE;
				break;
			case TCT_MARS1:											// 5 - STR_MARS_CYDONIA_LANDING
				track = OpenXcom::res_MUSIC_TAC_BATTLE_MARS1;
				break;
			case TCT_MARS2:											// 6 - STR_MARS_THE_FINAL_ASSAULT
				track = OpenXcom::res_MUSIC_TAC_BATTLE_MARS2;
				break;

			default:
				track = OpenXcom::res_MUSIC_TAC_BATTLE; // safety.
//				terrain = "CULTA"; // remarked in Music.rul
		}
	}
	//Log(LOG_INFO) << "SBG:calibrateMusic track= " << track << " terrain= " << terrain;
}

/**
 * Sets the aLiens as having been pacified.
// * @note Experience gains are no longer allowed once this is set.
 * @param pacified - true if pacified (default true)
 */
void SavedBattleGame::setPacified(bool pacified)
{
	_pacified = pacified;
}

/**
 * Gets whether the aLiens have been pacified yet.
// * @note Experience gains are no longer allowed if this is set.
 * @return, true if pacified
 */
bool SavedBattleGame::getPacified() const
{
	return _pacified;
}

/**
 * Sets the camera-offset of when the last RF-trigger happened.
 * @param offset - reference to a camera-offset
 */
void SavedBattleGame::rfTriggerOffset(const Position& offset)
{
	_rfTriggerOffset = offset;
}

/**
 * Gets the camera-offset of when the last RF-trigger happened.
 * @return, reference to the camera-offset
 */
const Position& SavedBattleGame::rfTriggerOffset() const
{
	return _rfTriggerOffset;
}

/**
 * Gets a ref to the scanner-dots vector.
 * @return, reference to a vector of pairs of ints which are positions of the current Turn's scanner-dots.
 */
std::vector<std::pair<int,int>>& SavedBattleGame::scannerDots()
{
	return _scanDots;
}

/**
 * Gets a read-only ref to the scanner-dots vector.
 * @return, reference to a vector of pairs of ints which are positions of the current Turn's scanner-dots.
 */
const std::vector<std::pair<int,int>>& SavedBattleGame::scannerDots() const
{
	return _scanDots;
}

/**
 * Gets the minimum TU that a unit has at the start of its turn.
 * @return, min TU value
 */
int SavedBattleGame::getDropTu() const
{
	return _dropTu;
}

/**
 * Sets the maximum number of turns before tactical ends.
 * @param limit - the turn limit
 */
void SavedBattleGame::setTurnLimit(int limit)
{
	_turnLimit = limit;
}

/**
 * Gets the maximum number of turns before tactical ends.
 * @return, the turn limit
 */
int SavedBattleGame::getTurnLimit() const
{
	return _turnLimit;
}

/**
 * Sets the result to occur when the turn-timer runs out.
 * @param result - the result to perform (RuleAlienDeployment.h)
 */
void SavedBattleGame::setChronoResult(ChronoResult result)
{
	_chronoResult = result;
}

/**
 * Gets the result to perform when the turn-timer runs out.
 * @return, the result to perform (RuleAlienDeployment.h)
 */
ChronoResult SavedBattleGame::getChronoResult() const
{
	return _chronoResult;
}

/**
 * Sets the turn at which the player's units become exposed to the AI.
 * @param turn - the turn for the AI to start cheating
 */
void SavedBattleGame::setCheatTurn(int turn)
{
	_cheatTurn = turn;
}

/**
 * Access to the set of detonation-tiles.
 * @note The detonation-tiles are stored here instead of in BattlescapeGame
 * because during pre-battle powersource explosions the latter is invalid.
 * @return, reference to the set of detonation-tiles
 *
std::set<Tile*>& SavedBattleGame::detonationTiles()
{
	return _detonationTiles;
} */

/**
 * Checks if tactical has yet to start.
 * @return, true if pre-battle
 *
bool SavedBattleGame::preBattle() const
{
	return _preBattle;
} */

}

/**
 * Gets the TU reserved type.
 * @return, a BattleActionType
 *
BattleActionType SavedBattleGame::getBatReserved() const
{
	return _batReserved;
} */
/**
 * Sets the TU reserved type.
 * @param reserved - a BattleActionType
 *
void SavedBattleGame::setBatReserved(BattleActionType reserved)
{
	_batReserved = reserved;
} */
/**
 * Gets the kneel reservation setting.
 * @return, true if an extra 4 TUs should be reserved to kneel
 *
bool SavedBattleGame::getKneelReserved() const
{
	return _kneelReserved;
} */
/**
 * Sets the kneel reservation setting.
 * @param reserved - true if an extra 4 TUs should be reserved to kneel
 *
void SavedBattleGame::setKneelReserved(bool reserved)
{
	_kneelReserved = reserved;
} */

/**
 * @brief Checks whether anyone on a particular faction is looking at the unit.
 * Similar to getSpottingUnits() but returns a bool and stops searching if one positive hit is found.
 * @param faction Faction to check through.
 * @param unit Whom to spot.
 * @return True when the unit can be seen
 *
bool SavedBattleGame::eyesOnTarget(UnitFaction faction, BattleUnit* unit)
{
	for (std::vector<BattleUnit*>::const_iterator i = getUnits()->begin(); i != getUnits()->end(); ++i)
	{
		if ((*i)->getFaction() != faction) continue;
		std::vector<BattleUnit*>* vis = (*i)->getHostileUnits();
		if (std::find(vis->begin(), vis->end(), unit) != vis->end())
		{
			return true;
			// aliens know the location of all XCom agents sighted by all other
			// aliens due to sharing locations over their space-walkie-talkies
		}
	}
	return false;
} */
