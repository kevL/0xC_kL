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
 * @param gameSave	- pointer to the SavedGame
 * @param titles	- pointer to a vector of pointers to OperationPool (default nullptr)
 * @param rules		- pointer to the Ruleset (default nullptr)
 */
SavedBattleGame::SavedBattleGame(
		SavedGame* const gameSave,
		const std::vector<OperationPool*>* const titles,
		const Ruleset* const rules)
	:
		_gameSave(gameSave),
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
		_objectiveTile(TILE),
		_objectivesRequired(0),
		_objectivesDestroyed(0),
		_unitsFalling(false),
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
	for (size_t
			i = 0u;
			i != _qtyTilesTotal;
			++i)
		delete _tiles[i];

	delete[] _tiles;

	for (std::vector<MapDataSet*>::const_iterator
			i = _mapDataSets.begin();
			i != _mapDataSets.end();
			++i)
		(*i)->unloadData();

	for (std::vector<Node*>::const_iterator
			i = _nodes.begin();
			i != _nodes.end();
			++i)
		delete *i;

	for (std::vector<BattleUnit*>::const_iterator
			i = _units.begin();
			i != _units.end();
			++i)
		delete *i;

	for (std::vector<BattleItem*>::const_iterator
			i = _items.begin();
			i != _items.end();
			++i)
		delete *i;

	for (std::vector<BattleItem*>::const_iterator
			i = _recoverGuaranteed.begin();
			i != _recoverGuaranteed.end();
			++i)
		delete *i;

	for (std::vector<BattleItem*>::const_iterator
			i = _recoverConditional.begin();
			i != _recoverConditional.end();
			++i)
		delete *i;

	for (std::vector<BattleItem*>::const_iterator
			i = _deletedProperty.begin();
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
	_mapsize_x		= node["width"]		.as<int>(_mapsize_x);
	_mapsize_y		= node["length"]	.as<int>(_mapsize_y);
	_mapsize_z		= node["height"]	.as<int>(_mapsize_z);
	_tacticalType	= node["type"]		.as<std::string>(_tacticalType);
	_tacticalShade	= node["shade"]		.as<int>(_tacticalShade);
	_turn			= node["turn"]		.as<int>(_turn);
	_terrain		= node["terrain"]	.as<std::string>(_terrain);

	setTacType(_tacticalType);

	Log(LOG_INFO) << ". load mapdatasets";
	for (YAML::const_iterator
			i = node["mapdatasets"].begin();
			i != node["mapdatasets"].end();
			++i)
	{
		_mapDataSets.push_back(rules->getMapDataSet(i->as<std::string>()));
	}

	Log(LOG_INFO) << ". init map";
	initMap(
		_mapsize_x,
		_mapsize_y,
		_mapsize_z);

	if (!node["tileTotalBytesPer"]) // binary tile data not found, load old-style text tiles :(
	{
		Log(LOG_INFO) << ". load tiles [text]";
		for (YAML::const_iterator
				i = node["tiles"].begin();
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
		serKey.index		= node["tileIndexSize"]		.as<Uint8>(serKey.index);
		serKey.totalBytes	= node["tileTotalBytesPer"]	.as<Uint32>(serKey.totalBytes);
		serKey._fire		= node["tileFireSize"]		.as<Uint8>(serKey._fire);
		serKey._smoke		= node["tileSmokeSize"]		.as<Uint8>(serKey._smoke);
		serKey._aniOffset	= node["tileOffsetSize"]	.as<Uint8>(serKey._aniOffset);
		serKey._partId		= node["tileIDSize"]		.as<Uint8>(serKey._partId);
		serKey._partSetId	= node["tileSetIDSize"]		.as<Uint8>(serKey._partSetId);
		serKey.boolFields	= node["tileBoolFieldsSize"].as<Uint8>(1u); // boolean flags used to be stored in an unmentioned byte (Uint8) :|

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
			i = node["nodes"].begin();
			i != node["nodes"].end();
			++i)
	{
		Node* const pfNode (new Node());
		pfNode->load(*i);
		_nodes.push_back(pfNode);
	}


	const int selUnitId (node["selectedUnit"].as<int>());

	int id;

	BattleUnit* unit;
	UnitFaction
		faction,
		factionOrg;

	Log(LOG_INFO) << ". load units";
	for (YAML::const_iterator
			i = node["units"].begin();
			i != node["units"].end();
			++i)
	{
		id			= (*i)["id"]										.as<int>();
		faction		= static_cast<UnitFaction>((*i)["faction"]			.as<int>());
		factionOrg	= static_cast<UnitFaction>((*i)["originalFaction"]	.as<int>(faction)); // .. technically, static_cast<int>(faction).

		if (id < BattleUnit::MAX_SOLDIER_ID)			// instance a BattleUnit from a geoscape-soldier
			unit = new BattleUnit(
							_gameSave->getSoldier(id),
							this);
		else											// instance a BattleUnit as an aLien, civie, or support-unit
		{
			const std::string
				type	((*i)["genUnitType"]	.as<std::string>()),
				armor	((*i)["genUnitArmor"]	.as<std::string>());

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
	static const size_t LIST_TYPE (3u);
	static const std::string itList_saved[LIST_TYPE]
	{
		"items",
		"recoverConditional",
		"recoverGuaranteed"
	};
	std::vector<BattleItem*>* const itList_battle[LIST_TYPE]
	{
		&_items,
		&_recoverConditional,
		&_recoverGuaranteed
	};

	std::string st;
	int
		owner,
		ownerPre,
		unitId;
	Position pos;
	BattleItem* item;

	for (size_t
			i = 0u;
			i != LIST_TYPE;
			++i)
	{
		for (YAML::const_iterator
				j = node[itList_saved[i]].begin();
				j != node[itList_saved[i]].end();
				++j)
		{
			st = (*j)["type"].as<std::string>();
			if (rules->getItemRule(st) != nullptr)
			{
				id = (*j)["id"].as<int>(-1); // NOTE: 'id' should always be valid here.
				item = new BattleItem(
									rules->getItemRule(st),
									nullptr,
									id);

				item->load(*j);

				if ((*j)["section"])
				{
					st = (*j)["section"].as<std::string>();	// NOTE: the given 'section' should always be valid. Unless it's a loaded Ammo-item.
//					if (st.empty() == false) //!= "NONE")	// cf. BattleItem::save()
					item->setInventorySection(rules->getInventory(st));
				}

				owner		= (*j)["owner"]		.as<int>(-1); // cf. BattleItem::save() ->
				ownerPre	= (*j)["ownerPre"]	.as<int>(-1);
				unitId		= (*j)["unit"]		.as<int>(-1);

				if (ownerPre == -1) ownerPre = owner;

				for (std::vector<BattleUnit*>::const_iterator // match up some item-variables with units
						k = _units.begin();
						k != _units.end();
						++k)
				{
					const int testId ((*k)->getId());
					if (testId == owner)
					{
						item->setOwner(*k); // call before setPriorOwner()
						(*k)->getInventory()->push_back(item);
					}

					if (testId == ownerPre)
						item->setPriorOwner(*k);

					if (testId == unitId)
						item->setItemUnit(*k);
				}

				if (item->getInventorySection() != nullptr						// match up items and tiles
					&& item->getInventorySection()->getCategory() == IC_GROUND)	// NOTE: 'section' should always be valid unless it's a loaded Ammo-item.
				{
					if ((*j)["position"])
					{
						pos = (*j)["position"].as<Position>();
//						if (pos.z != -1) // was not saved if pos.z= -1
						item->setInventorySection(rules->getInventoryRule(ST_GROUND));
						getTile(pos)->addItem(item);
					}
					else
						pos = Position(0,0,-1); // cf. BattleItem::save()
				}

				itList_battle[i]->push_back(item);
			}
			else Log(LOG_ERROR) << "Failed to load item " << st;
		}
	}

	Log(LOG_INFO) << ". load weapons w/ ammo";
	// iterate through the items again and tie ammo-items to their weapons
	std::vector<BattleItem*>::const_iterator pWeapon (_items.begin());
	for (YAML::const_iterator
			i = node["items"].begin();
			i != node["items"].end();
			++i)
	{
		if (rules->getItemRule((*i)["type"].as<std::string>()) != nullptr)
		{
			const int ammo ((*i)["ammoItem"].as<int>(-1)); // cf. BattleItem::save()
			if (ammo != -1)
			{
				for (std::vector<BattleItem*>::const_iterator
						j = _items.begin();
						j != _items.end();
						++j)
				{
					if ((*j)->getId() == ammo)
					{
						(*pWeapon)->setAmmoItem(*j, true);
						break;
					}
				}
			}
			++pWeapon;
		}
	}

	for (YAML::const_iterator
			i = node["toDelete"].begin();
			i != node["toDelete"].end();
			++i)
	{
		st = (*i)["type"].as<std::string>();
		if (rules->getItemRule(st) != nullptr)
		{
			id = (*i)["id"].as<int>(-1); // NOTE: 'id' should always be valid here.
			item = new BattleItem(
								rules->getItemRule(st),
								nullptr,
								id);
			item->loadDeleted();
			_deletedProperty.push_back(item);
		}
	}

	Log(LOG_INFO) << ". set some vars";

	_objectiveTile = static_cast<TileType>(node["objectiveTile"].as<int>(_objectiveTile));
	_objectivesRequired		= node["objectivesRequired"]	.as<int>(_objectivesRequired);
	_objectivesDestroyed	= node["objectivesDestroyed"]	.as<int>(_objectivesDestroyed);

	_turnLimit = node["turnLimit"].as<int>(_turnLimit);
	_chronoResult = static_cast<ChronoResult>(node["chronoResult"].as<int>(_chronoResult));

	_cheatAI	= node["cheatAI"]	.as<bool>(_cheatAI);
	_cheatTurn	= node["cheatTurn"]	.as<int>(_cheatTurn);
	_alienRace	= node["alienRace"]	.as<std::string>(_alienRace);
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
			i = _mapDataSets.begin();
			i != _mapDataSets.end();
			++i)
	{
		(*i)->loadData();

		if (game->getRuleset()->getMCDPatch((*i)->getType()) != nullptr)
			game->getRuleset()->getMCDPatch((*i)->getType())->modifyData(*i);
	}

	int
		dataId,
		dataSetId;
	MapDataType partType;

	Tile* tile;
	for (size_t
			i = 0u;
			i != _qtyTilesTotal;
			++i)
	{
		for (size_t
				j = 0u;
				j != Tile::PARTS_TILE;
				++j)
		{
			partType = static_cast<MapDataType>(j);

			tile = _tiles[i];
			tile->getMapData(
						&dataId,
						&dataSetId,
						partType);

			if (dataId != -1 && dataSetId != -1)
				tile->setMapData(
							_mapDataSets[static_cast<size_t>(dataSetId)]->getRecords()->at(static_cast<size_t>(dataId)),
							dataId,
							dataSetId,
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
		node["objectiveTile"]		= static_cast<int>(_objectiveTile);
		node["objectivesRequired"]	= _objectivesRequired;
		node["objectivesDestroyed"]	= _objectivesDestroyed;
	}

	node["width"]			= _mapsize_x;
	node["length"]			= _mapsize_y;
	node["height"]			= _mapsize_z;
	node["type"]			= _tacticalType;
	node["shade"]			= _tacticalShade;
	node["turn"]			= _turn;
	node["terrain"]			= _terrain;
	node["selectedUnit"]	= (_selectedUnit != nullptr) ? _selectedUnit->getId() : -1;

	for (std::vector<MapDataSet*>::const_iterator
			i = _mapDataSets.begin();
			i != _mapDataSets.end();
			++i)
	{
		node["mapdatasets"].push_back((*i)->getType());
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
	node["tileIndexSize"]		= Tile::serializationKey.index;
	node["tileTotalBytesPer"]	= Tile::serializationKey.totalBytes;
	node["tileFireSize"]		= Tile::serializationKey._fire;
	node["tileSmokeSize"]		= Tile::serializationKey._smoke;
	node["tileOffsetSize"]		= Tile::serializationKey._aniOffset;
	node["tileIDSize"]			= Tile::serializationKey._partId;
	node["tileSetIDSize"]		= Tile::serializationKey._partSetId;
	node["tileBoolFieldsSize"]	= Tile::serializationKey.boolFields;

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

	node["totalTiles"]	= tilesDataSize / Tile::serializationKey.totalBytes; // not strictly necessary, just convenient
	node["binTiles"]	= YAML::Binary(tilesData, tilesDataSize);

	std::free(tilesData);
#endif

	for (std::vector<Node*>::const_iterator
			i = _nodes.begin();
			i != _nodes.end();
			++i)
	{
		node["nodes"].push_back((*i)->save());
	}

	if (_tacType == TCT_BASEDEFENSE)
		node["moduleMap"] = _baseModules;

	for (std::vector<BattleUnit*>::const_iterator
			i = _units.begin();
			i != _units.end();
			++i)
	{
		node["units"].push_back((*i)->save());
	}

	for (std::vector<BattleItem*>::const_iterator
			i = _items.begin();
			i != _items.end();
			++i)
	{
		node["items"].push_back((*i)->save());
	}

	for (std::vector<BattleItem*>::const_iterator
			i = _recoverGuaranteed.begin();
			i != _recoverGuaranteed.end();
			++i)
	{
		node["recoverGuaranteed"].push_back((*i)->save());
	}

	for (std::vector<BattleItem*>::const_iterator
			i = _recoverConditional.begin();
			i != _recoverConditional.end();
			++i)
	{
		node["recoverConditional"].push_back((*i)->save());
	}

	for (std::vector<BattleItem*>::const_iterator
			i = _deletedProperty.begin();
			i != _deletedProperty.end();
			++i)
	{
		node["toDelete"].push_back((*i)->saveDeleted());
	}

//	node["batReserved"]		= static_cast<int>(_batReserved);
//	node["kneelReserved"]	= _kneelReserved;
	node["alienRace"]		= _alienRace;

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
 * Deletes the old and initializes a new array of Tiles.
 * @param mapsize_x -
 * @param mapsize_y -
 * @param mapsize_z -
 */
void SavedBattleGame::initMap(
		const int mapsize_x,
		const int mapsize_y,
		const int mapsize_z)
{
	if (_nodes.empty() == false) // Delete old stuff,
	{
		_qtyTilesTotal = static_cast<size_t>(_mapsize_x * _mapsize_y * _mapsize_z);
		for (size_t
				i = 0u;
				i != _qtyTilesTotal;
				++i)
		{
			delete _tiles[i];
		}

		delete[] _tiles;

		for (std::vector<Node*>::const_iterator
				i = _nodes.begin();
				i != _nodes.end();
				++i)
		{
			delete *i;
		}

		_nodes.clear();
		_mapDataSets.clear();
	}

	_mapsize_x = mapsize_x; // Create Tile objects.
	_mapsize_y = mapsize_y;
	_mapsize_z = mapsize_z;
	_qtyTilesTotal = static_cast<size_t>(mapsize_z * mapsize_y * mapsize_x);

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
 * Initializes the map-utilities.
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
	if		(type.compare("STR_UFO_CRASH_RECOVERY")		== 0)	_tacType = TCT_UFOCRASHED;
	else if	(type.compare("STR_UFO_GROUND_ASSAULT")		== 0)	_tacType = TCT_UFOLANDED;
	else if	(type.compare("STR_BASE_DEFENSE")			== 0)	_tacType = TCT_BASEDEFENSE;
	else if	(type.compare("STR_ALIEN_BASE_ASSAULT")		== 0)	_tacType = TCT_BASEASSAULT;
	else if	(type.compare("STR_TERROR_MISSION")			== 0
		||	 type.compare("STR_PORT_ATTACK")			== 0)	_tacType = TCT_TERRORSITE;
	else if	(type.compare("STR_MARS_CYDONIA_LANDING")	== 0)	_tacType = TCT_MARS1;
	else if	(type.compare("STR_MARS_THE_FINAL_ASSAULT")	== 0)	_tacType = TCT_MARS2;
	else														_tacType = TCT_DEFAULT;	// <- the default should probly be TCT_UFOCRASHED.
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
 * handleUnitAI(), and in SavedBattleGame::endFactionTurn().
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
 * @note Called from BattlescapeGame::endTurn().
 * @return, true if the turn rolls-over to Faction_Player
 */
bool SavedBattleGame::endFactionTurn()
{
	//Log(LOG_INFO) << "sbg:endFactionTurn() side= " << _side;
	for (std::vector<BattleUnit*>::const_iterator	// set *all* units non-selectable
			i = _units.begin();						// Units of the upcoming turn's faction are
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
			std::fill(
					_shuffleUnits.begin(),
					_shuffleUnits.end(),
					nullptr);

			tileVolatiles(); // do Tile stuff

			++_turn;

			firstFactionUnit(FACTION_PLAYER); // set '_selectedUnit'

			int aLienIntelTest;
			for (std::vector<BattleUnit*>::const_iterator
					i = _units.begin();
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
						i = _units.begin();
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
								const int r (RNG::generate(0,5));
								if (_turn > _cheatTurn - 3 + r
									|| _battleState->getBattleGame()->tallyHostiles() < r - 1)
								{
									_cheatAI = true;
								}
						}
					}
				}
			}
	}

	for (std::vector<BattleUnit*>::const_iterator
			i = _units.begin();
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
				if (_cheatAI == true									// aLiens know where xCom is when cheating ~turn20
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
						if (++exposure > aLienIntel)
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
		_tiles[i]->setRevealed();
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
 * Gets the array of MCDs.
 * @return, pointer to a vector of pointers to MapDataSets
 */
std::vector<MapDataSet*>* SavedBattleGame::getMapDataSets()
{
	return &_mapDataSets;
}

/**
 * Gets a pointer to the Geoscape save.
 * @return, pointer to SavedGame
 */
SavedGame* SavedBattleGame::getSavedGame() const
{
	return _gameSave;
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
	const Position& posBelow (Position(0,0,-1));

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
							getTile(pos + posBelow)); // second set BattleUnit->Tile link for the unit's primary quadrant

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
 * Upon removal the pointer to the item is kept in the '_deletedProperty' vector which
 * is flushed and destroyed in the SavedBattleGame dTor.
 * @param item - pointer to an item to remove
 * @return, const_iterator to the next item in the BattleItems list
 */
std::vector<BattleItem*>::const_iterator SavedBattleGame::toDeleteItem(BattleItem* const item)
{
	Tile* const tile (item->getItemTile());
	if (tile != nullptr)
	{
		for (std::vector<BattleItem*>::const_iterator
				i = tile->getInventory()->begin();
				i != tile->getInventory()->end();
				++i)
		{
			if (*i == item)
			{
				tile->getInventory()->erase(i);
				break;
			}
		}
	}

	BattleUnit* const unit (item->getOwner());
	if (unit != nullptr)
	{
		for (std::vector<BattleItem*>::const_iterator
				i = unit->getInventory()->begin();
				i != unit->getInventory()->end();
				++i)
		{
			if (*i == item)
			{
				unit->getInventory()->erase(i);
				break;
			}
		}
	}

	for (std::vector<BattleItem*>::const_iterator
			i = _items.begin();
			i != _items.end();
			++i)
	{
		if (*i == item)
		{
			if ((*i)->getProperty() == true)
				_deletedProperty.push_back(item);
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
void SavedBattleGame::setObjectiveTileType(TileType type)
{
	_objectiveTile = type;
}

/**
 * Gets the objective-tiletype for the current battle.
 * @return, objective-tiletype (RuleItem.h)
 */
TileType SavedBattleGame::getObjectiveTileType() const
{
	return _objectiveTile;
}

/**
 * Initializes the objective-tiles needed quantity.
 * @note Used only to initialize the objective counter; cf addDestroyedObjective()
 * below. Objectives were tile-parts marked w/ OBJECT_TILE in their MCD but now
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
 * Sets the next available item-ID value.
 * @note Used only at the finish of loading a SavedBattleGame.
 * @note ItemIDs start at 0.
 */
void SavedBattleGame::setCanonicalBattleId()
{
	int
		id (-1),
		idTest;

	for (std::vector<BattleItem*>::const_iterator
			i = _items.begin();
			i != _items.end();
			++i)
	{
		if ((idTest = (*i)->getId()) > id)
			id = idTest;
	}
	_itemId = ++id;
}

/**
 * Gets the next available item-ID value.
 * @return, pointer to the highest available value
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
	std::vector<Node*> spawnNodes;
	for (std::vector<Node*>::const_iterator
			i = _nodes.begin();
			i != _nodes.end();
			++i)
	{
		if ((*i)->getPriority() != 0			// spawn-priority 0 is not spawnplace
			&& (*i)->getNodeRank() == unitRank	// ranks must match
			&& isNodeType(*i, unit)				// unit's size and walk-type must match node's
			&& setUnitPosition(					// check if unit can be set at this node
							unit,					// ie. node is big enough
							(*i)->getPosition(),	// and there's not already a unit there.
							true) == true)			// test-only: runs again w/ FALSE on return to bgen::addAlien()
		{
			for (int // weight each eligible node by its Priority.
					j = (*i)->getPriority();
					j != 0;
					--j)
			{
				spawnNodes.push_back(*i);
			}
		}
	}

	if (spawnNodes.empty() == false)
		return spawnNodes[RNG::pick(spawnNodes.size())];

	return nullptr;
}

/**
 * Finds a suitable Node where a specified BattleUnit can patrol to.
 * @param scout		- true if the unit is scouting
 * @param unit		- pointer to a BattleUnit
 * @param startNode	- pointer to the node that unit is currently at
 * @return, pointer to the destination Node
 */
Node* SavedBattleGame::getPatrolNode(
		bool scout,
		BattleUnit* const unit,
		Node* startNode)
{
	if (startNode == nullptr)
		startNode = getNearestNode(unit);

	std::vector<Node*>
		scoutNodes,
		officerNodes;
	Node* node;

	size_t qtyNodes;
	if (scout == true)
		qtyNodes = getNodes()->size();
	else
		qtyNodes = startNode->getNodeLinks()->size();

	for (size_t
			i = 0u;
			i != qtyNodes;
			++i)
	{
		if (scout == true || startNode->getNodeLinks()->at(i) > -1)	// non-scouts need Links to travel along.
		{															// N-E-S-W directions are never used (linkId's -2,-3,-4,-5).
			if (scout == true)										// Meaning that non-scouts never leave their spawn-block ...
				node = getNodes()->at(i);
			else
				node = getNodes()->at(static_cast<size_t>(startNode->getNodeLinks()->at(i)));

			if ((node->getPatrol() != 0										// for non-scouts find a node with a desirability above 0
					|| node->getNodeRank() > NR_SCOUT
					|| scout == true)
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
				&& (node != startNode										// scouts push forward
					|| scout == false))											// others can mill around.. ie, stand there.
			{
				for (int
						j = node->getPatrol(); // weight each eligible node by its patrol-Flags.
						j != -1;
						--j)
				{
					scoutNodes.push_back(node);

					if (scout == false
						&& node->getNodeRank() == Node::nodeRank[static_cast<size_t>(unit->getRankInt())]
																[0u]) // high-class node here.
					{
						officerNodes.push_back(node);
					}
				}
			}
		}
	}

	if (scoutNodes.empty() == true)
	{
		if (scout == false && unit->getArmor()->getSize() == 2)
		{
//			return Sectopod::CTD();
			return getPatrolNode(true, unit, startNode);
		}
		return nullptr;
	}

	if (scout == true // picks a random destination
		|| officerNodes.empty() == true
		|| RNG::percent(17) == true) // officers can go for a stroll ...
	{
		return scoutNodes[RNG::pick(scoutNodes.size())];
	}
	return officerNodes[RNG::pick(officerNodes.size())];
}

/**
 * Gets a Node considered nearest to a specified BattleUnit.
 * @note Assume closest node is on same level to avoid strange things. The node
 * has to match unit-size or the AI will freeze.
 * @param unit - pointer to a BattleUnit
 * @return, the nearest Node
 */
Node* SavedBattleGame::getNearestNode(const BattleUnit* const unit) const
{
	Node* node (nullptr);
	int
		dist (1000000),
		distTest;

	for (std::vector<Node*>::const_iterator
			i = _nodes.begin();
			i != _nodes.end();
			++i)
	{
		if (unit->getPosition().z == (*i)->getPosition().z
			&& (unit->getArmor()->getSize() == 1
				|| !((*i)->getNodeType() & Node::TYPE_SMALL)))
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
 * BattlescapeGenerator::loadRMP().
 * @param node - pointer to a node
 * @param unit - pointer to a unit trying to use the node
 * @return, true if unit can use node
 */
bool SavedBattleGame::isNodeType(
		const Node* const node,
		const BattleUnit* const unit) const
{
	const int type (node->getNodeType());

	if (type & Node::TYPE_DANGEROUS)					// +16	Only Type_Dangerous is ever added to a
		return false;									//		stock nodeType in the code currently.

	switch (type)
	{
		case Node::TYPE_FLYING:							// 1
			return unit->getMoveTypeUnit() == MT_FLY
				&& unit->getArmor()->getSize() == 1;

		case Node::TYPE_SMALL:							// 2
			return unit->getArmor()->getSize() == 1;

		case Node::TYPE_LARGEFLYING:					// 4
			return unit->getMoveTypeUnit() == MT_FLY;

//		case 0:											// Any.
//			break;
//		case Node::TYPE_LARGE:							// 8 All units can use Type_Large.
//			break;
	}
	return true;
}

/**
 * Carries out full-turn preparations such as fire and smoke spreading.
 * @note Also explodes any explosive Tiles that get destroyed by fire.
 */
void SavedBattleGame::tileVolatiles()
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
			i = tilesFired.begin();
			i != tilesFired.end();
			++i)
	{
		if ((*i)->decreaseFire() != 0)
		{
			var = (*i)->getFire() << 4u;
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
				if (   (*i)->getMapData(O_OBJECT)->getFlammable() != 255
					&& (*i)->getMapData(O_OBJECT)->getArmor()     != 255) // NOTE: Also checked in destroyTilepart().
				{
					(*i)->destroyTilepart(O_OBJECT, this);
					(*i)->destroyTilepart(O_FLOOR, this);	// NOTE: There is no assurance that the current floor-part is actually 'flammable';
				}											// but it ensures that if there's not a current floor-part then
			}												// at least a scorched-earth floor-part gets laid down.
			else if ((*i)->getMapData(O_FLOOR) != nullptr)
			{
				if (   (*i)->getMapData(O_FLOOR)->getFlammable() != 255
					&& (*i)->getMapData(O_FLOOR)->getArmor()     != 255) // NOTE: Also checked in destroyTilepart().
				{
					(*i)->destroyTilepart(O_FLOOR, this);
				}
			}
			_te->applyGravity(*i);
		}
	}

	for (std::vector<Tile*>::const_iterator
			i = tilesSmoked.begin();
			i != tilesSmoked.end();
			++i)
	{
		if ((*i)->decreaseSmoke() > 1)
		{
			if ((var = (*i)->getSmoke() >> 1u) > 2
				&& (tile = getTile((*i)->getPosition() + Position(0,0,1))) != nullptr
				&& tile->isFloored(*i) == false) // TODO: Use verticalBlockage() instead.
			{
				tile->addSmoke(var / 3);
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
						tile->addSmoke(var >> 1u);
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

		if (pos == Position(-1,-1,-1)) // if carried
		{
			for (std::vector<BattleItem*>::const_iterator
					i = _items.begin();
					i != _items.end();
					++i)
			{
				if ((*i)->getBodyUnit() != nullptr
					&& (*i)->getBodyUnit() == unit
					&& (*i)->getOwner() != nullptr) // safety.
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
	int quadrants (unit->getArmor()->getSize() * unit->getArmor()->getSize());
	for (std::vector<BattleItem*>::const_iterator
			i = _items.begin();
			i != _items.end();
			)
	{
		if ((*i)->getBodyUnit() == unit)
		{
			i = toDeleteItem(*i);
			if (--quadrants == 0) return;
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
 * @param pos	- reference to the position to place the unit
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
					pos0 += Position(0,0,1);
					x =
					y = unitSize + 1; // start over.
					break;
				}

				if ((tile->getTileUnit() != nullptr && tile->getTileUnit() != unit)
					|| tile->getTuCostTile(
										O_OBJECT,
										unit->getMoveTypeUnit()) == 255
					|| (unit->getMoveTypeUnit() != MT_FLY // <- so just use the unit's moveType.
						&& tile->isFloored(getTile(pos0 + Position(x,y,-1))) == false))
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

				const Tile* const tileAbove (getTile(pos0 + Position(x,y,1))); // TODO: check for ceiling also.
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
						getTile(pos0 + Position(0,0,-1)));
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
//		Tile* tile = getTile(pos + Position(0,0,1));
//		if (tile
//			&& tile->isFloored(getTile(pos)) == false
//			&& setUnitPosition(unit, pos + Position(0,0,1)))
//		{
//			return true;
//		}
//	}

/**
 * Adds this unit to the vector of falling units if it doesn't already exist there.
 * @param unit - pointer to the unit to add
 * @return, true if the unit was added
 */
bool SavedBattleGame::addFallingUnit(BattleUnit* const unit)
{
	for (std::list<BattleUnit*>::const_iterator
			i = _fallingUnits.begin();
			i != _fallingUnits.end();
			++i)
	{
		if (unit == *i) return false;
	}
	_fallingUnits.push_front(unit);
	return (_unitsFalling = true);
}

/**
 * Gets all units in the battlescape that are falling.
 * @return, pointer to the list of pointers to the falling BattleUnits
 */
std::list<BattleUnit*>* SavedBattleGame::getFallingUnits()
{
	return &_fallingUnits;
}

/**
 * Accesses the '_unitsFalling' bool.
 * @note The flag that says: Units Falling, start UnitFallBState.
 * @return, reference to the toggle
 */
bool& SavedBattleGame::unitsFalling()
{
	return _unitsFalling;
}

/**
 * Gets the highest ranked, living, non Mc'd unit of faction.
 * @param qtyAllies	- reference to the number of allied units that will be conscious and not MC'd
 * @param isXcom	- true if examining Faction_Player, false for Faction_Hostile (default true)
 * @return, pointer to highest ranked BattleUnit of faction
 */
const BattleUnit* SavedBattleGame::getHighestRanked(
		int& qtyAllies,
		bool isXcom) const
{
	//Log(LOG_INFO) << "SavedBattleGame::getHighestRanked() xcom = " << xcom;
	const BattleUnit* leader (nullptr);
	qtyAllies = 0;

	for (std::vector<BattleUnit*>::const_iterator
			i = _units.begin();
			i != _units.end();
			++i)
	{
		if ((*i)->isOut_t(OUT_STAT) == false)
		{
			if (isXcom == true)
			{
				//Log(LOG_INFO) << "SavedBattleGame::getHighestRanked(), side is Xcom";
				if ((*i)->getFaction() == FACTION_PLAYER
					&& (*i)->isMindControlled() == false)
				{
					++qtyAllies;

					if (leader == nullptr || (*i)->getRankInt() > leader->getRankInt())
						leader = *i;
				}
			}
			else if ((*i)->getFaction() == FACTION_HOSTILE
				&& (*i)->isMindControlled() == false)
			{
				//Log(LOG_INFO) << "SavedBattleGame::getHighestRanked(), side is aLien";
				++qtyAllies;

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
 * @param unit		- pointer to BattleUnit deceased; higher rank is higher penalty (default nullptr)
 * @param isXcom	- if no unit is passed in this determines whether penalty applies to xCom or aLiens (default true)
 * @return, morale modifier
 */
int SavedBattleGame::getMoraleModifier( // note: Add bonus to aLiens for Cydonia & Final Assault.
		const BattleUnit* const unit,
		bool isXcom) const
{
	//Log(LOG_INFO) << "SavedBattleGame::getMoraleModifier()";
	int ret (100);

	if (unit != nullptr) // morale Loss if 'unit' is slain
	{
		//Log(LOG_INFO) << "SavedBattleGame::getMoraleModifier(), unit slain Penalty";
		switch (unit->getOriginalFaction())
		{
			case FACTION_NEUTRAL:
				return ret;

			case FACTION_PLAYER: // xCom dies. MC'd or not
				switch (unit->getRankInt())
				{
					case 5: ret += 30; // 200 commander
					case 4: ret += 25; // 170 colonel
					case 3: ret += 20; // 145 captain
					case 2: ret += 10; // 125 sergeant
					case 1: ret += 15; // 115 squaddie
					//Log(LOG_INFO) << ". . xCom lossModifi = " << ret;
				}
				break;

			case FACTION_HOSTILE:
				if (unit->isMindControlled() == false) // aLien dies. MC'd aliens return 100 or 50 on Mars
				{
					switch (unit->getRankInt()) // soldiers are rank #5, terrorists are ranks #6 and #7
					{
						case 0: ret += 30;	// 200 commander
						case 1: ret += 25;	// 170 leader
						case 2: ret += 20;	// 145 engineer
						case 3: ret += 10;	// 125 medic
						case 4: ret += 15;	// 115 navigator
					}

					switch (_tacType)
					{
						case TCT_MARS1:	// "STR_MARS_CYDONIA_LANDING"
						case TCT_MARS2:	// "STR_MARS_THE_FINAL_ASSAULT"
							ret >>= 1u;	// less hit for losing a unit on Cydonia.
					}
				}
				//Log(LOG_INFO) << ". . aLien lossModifi = " << ret;
		}
	}
	else // leadership Bonus
	{
		//Log(LOG_INFO) << "SavedBattleGame::getMoraleModifier(), leadership Bonus";
		const BattleUnit* leader;
		int qtyAllies;

		if (isXcom == true)
		{
			if ((leader = getHighestRanked(qtyAllies)) != nullptr)
			{
				switch (leader->getRankInt())
				{
					case 5: ret += 15; // 135, was 150	// commander
					case 4: ret +=  5; // 120, was 125	// colonel
					case 3: ret +=  5; // 115			// captain
					case 2: ret += 10; // 110			// sergeant
					case 1: ret += 15; // 100			// squaddie
					case 0: ret -= 15; //  85			// rookies ...
				}
			}
			//Log(LOG_INFO) << ". . xCom leaderModifi = " << ret;
		}
		else // aLien
		{
			if ((leader = getHighestRanked(qtyAllies, false)) != nullptr)
			{
				switch (leader->getRankInt()) // terrorists are ranks #6 and #7
				{
					case 0: ret += 25; // 150 commander
					case 1: ret += 10; // 125 leader
					case 2: ret +=  5; // 115 engineer
					case 3: ret += 10; // 110 medic
					case 4: ret += 10; // 100 navigator
					case 5: ret -= 10; //  90 soldiers ...
				}
			}

			switch (_tacType)
			{
				case TCT_TERRORSITE:	// "STR_TERROR_MISSION"
				case TCT_BASEASSAULT:	// "STR_ALIEN_BASE_ASSAULT"
				case TCT_BASEDEFENSE:	// "STR_BASE_DEFENSE"
					ret += 50;			// higher morale.
					break;

				case TCT_MARS1:	// "STR_MARS_CYDONIA_LANDING"
				case TCT_MARS2:	// "STR_MARS_THE_FINAL_ASSAULT"
					ret += 100;	// higher morale.
			}
			//Log(LOG_INFO) << ". . aLien leaderModifi = " << ret;
		}
		ret += qtyAllies - 9; // use 9 allies as Unity.
	}
	//Log(LOG_INFO) << ". totalModifier = " << ret;
	return ret;
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
		_tiles[i]->setRevealed(ST_WEST, false);
		_tiles[i]->setRevealed(ST_NORTH, false);
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
 * @param music		- reference to the music category to play
 * @param terrain	- reference to the terrain to choose music for
 */
void SavedBattleGame::calibrateMusic(
		std::string& music,
		std::string& terrain) const
{
	if (_music.empty() == false)
		music = _music;
	else
	{
		switch (_tacType)
		{
			case TCT_UFOCRASHED:	// 0 - STR_UFO_CRASH_RECOVERY
				music = OpenXcom::res_MUSIC_TAC_BATTLE_UFOCRASHED;
				terrain = _terrain;
				break;
			case TCT_UFOLANDED:		// 1 - STR_UFO_GROUND_ASSAULT
				music = OpenXcom::res_MUSIC_TAC_BATTLE_UFOLANDED;
				terrain = _terrain;
				break;
			case TCT_BASEASSAULT:	// 2 - STR_ALIEN_BASE_ASSAULT
				music = OpenXcom::res_MUSIC_TAC_BATTLE_BASEASSAULT;
				break;
			case TCT_BASEDEFENSE:	// 3 - STR_BASE_DEFENSE
				music = OpenXcom::res_MUSIC_TAC_BATTLE_BASEDEFENSE;
				break;
			case TCT_TERRORSITE:	// 4 - STR_TERROR_MISSION and STR_PORT_ATTACK, see setTacType()
				music = OpenXcom::res_MUSIC_TAC_BATTLE_TERRORSITE;
				break;
			case TCT_MARS1:			// 5 - STR_MARS_CYDONIA_LANDING
				music = OpenXcom::res_MUSIC_TAC_BATTLE_MARS1;
				break;
			case TCT_MARS2:			// 6 - STR_MARS_THE_FINAL_ASSAULT
				music = OpenXcom::res_MUSIC_TAC_BATTLE_MARS2;
				break;

			default:
				music = OpenXcom::res_MUSIC_TAC_BATTLE; // safety.
//				terrain = "CULTA"; // remarked in Music.rul
		}
	}
	//Log(LOG_INFO) << "SBG:calibrateMusic music= " << music << " terrain= " << terrain;
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
