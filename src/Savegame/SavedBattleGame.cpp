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
 * @param titles	- pointer to a vector of pointers to OperationPool (default nullptr)
 * @param rules		- pointer to the Ruleset (default nullptr)
 */
SavedBattleGame::SavedBattleGame(
		const std::vector<OperationPool*>* const titles,
		const Ruleset* const rules)
	:
		_battleState(nullptr),
		_mapsize_x(0),
		_mapsize_y(0),
		_mapsize_z(0),
		_qtyTilesTotal(0),
		_selectedUnit(nullptr),
		_lastSelectedUnit(nullptr),
		_pf(nullptr),
		_te(nullptr),
		_tacticalShade(0),
		_side(FACTION_PLAYER),
		_turn(1),
		_debugTac(false),
		_aborted(false),
		_itemId(0),
		_objectiveType(STT_NONE), // -1
		_objectivesDestroyed(0),
		_objectivesNeeded(0),
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
		_rfTriggerPosition(0,0,-1),
		_initTu(20),
		_walkUnit(nullptr),
		_turnLimit(0),
		_chronoResult(FORCE_LOSE),
		_cheatTurn(CHEAT_DEFAULT)
//		_dragInvert(false),
//		_dragTimeTolerance(0),
//		_dragPixelTolerance(0)
{
	//Log(LOG_INFO) << "\nCreate SavedBattleGame";
	if (rules != nullptr) // ie. not craft- or base-equip screen.
		_initTu = rules->detHighTuInventoryCost();

	_tileSearch.resize(SEARCH_SIZE);
	for (size_t
			i = 0;
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
		const size_t pool = 0; //RNG::generate(0, titles->size()-1 // <- in case I want to expand this for different missionTypes. eg, Cydonia -> "Blow Hard"
		_operationTitle = titles->at(pool)->genOperation();
	}
}

/**
 * Deletes the game content from memory.
 */
SavedBattleGame::~SavedBattleGame()
{
	//Log(LOG_INFO) << "";
	//Log(LOG_INFO) << "Delete SavedBattleGame";
	for (size_t
			i = 0;
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
			i = _toDelete.begin();
			i != _toDelete.end();
			++i)
		delete *i;

	delete _pf;
	delete _te;
}

/**
 * Loads the SavedBattleGame from a YAML file.
 * @param node		- reference a YAML node
 * @param rules		- pointer to the Ruleset
 * @param savedGame	- pointer to the SavedGame
 */
void SavedBattleGame::load(
		const YAML::Node& node,
		Ruleset* const rules,
		const SavedGame* const savedGame)
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
		Log(LOG_INFO) << ". load tiles [1]";
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
		Log(LOG_INFO) << ". load tiles [2]";
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
		serKey.index			= node["tileIndexSize"]		.as<Uint8>(serKey.index);
		serKey.totalBytes		= node["tileTotalBytesPer"]	.as<Uint32>(serKey.totalBytes);
		serKey._fire			= node["tileFireSize"]		.as<Uint8>(serKey._fire);
		serKey._smoke			= node["tileSmokeSize"]		.as<Uint8>(serKey._smoke);
		serKey._animOffset		= node["tileOffsetSize"]	.as<Uint8>(serKey._animOffset);
		serKey._mapDataId		= node["tileIDSize"]		.as<Uint8>(serKey._mapDataId);
		serKey._mapDataSetId	= node["tileSetIDSize"]		.as<Uint8>(serKey._mapDataSetId);
		serKey.boolFields		= node["tileBoolFieldsSize"].as<Uint8>(1); // boolean flags used to be stored in an unmentioned byte (Uint8) :|

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


	const int selectedUnit (node["selectedUnit"].as<int>());

	int id;

	BattleUnit* unit;
	UnitFaction
		faction,
		originalFaction;

	Log(LOG_INFO) << ". load units";
	for (YAML::const_iterator
			i = node["units"].begin();
			i != node["units"].end();
			++i)
	{
		id				= (*i)["id"]										.as<int>();
		faction			= static_cast<UnitFaction>((*i)["faction"]			.as<int>());
		originalFaction	= static_cast<UnitFaction>((*i)["originalFaction"]	.as<int>(faction)); // .. technically, static_cast<int>(faction).

		const GameDifficulty diff (savedGame->getDifficulty());
		if (id < BattleUnit::MAX_SOLDIER_ID)	// BattleUnit is linked to a geoscape soldier
		{
			unit = new BattleUnit(				// look up the matching soldier
							savedGame->getSoldier(id),
							diff);				// kL_add: For VictoryPts value per death.
		}
		else									// create a new Unit, not-soldier but Vehicle, Civie, or aLien.
		{
			const std::string
				type	((*i)["genUnitType"]	.as<std::string>()),
				armor	((*i)["genUnitArmor"]	.as<std::string>());

			if (rules->getUnitRule(type) != nullptr && rules->getArmor(armor) != nullptr) // safeties.
				unit = new BattleUnit(
									rules->getUnitRule(type),
									originalFaction,
									id,
									rules->getArmor(armor),
									diff,
									savedGame->getMonthsPassed());
			else
				unit = nullptr;
		}

		if (unit != nullptr)
		{
			Log(LOG_INFO) << ". . load unit " << id;
			unit->load(*i);
			_units.push_back(unit);

			if (faction == FACTION_PLAYER)
			{
				if (unit->getId() == selectedUnit
					|| (_selectedUnit == nullptr && unit->isOut_t(OUT_STAT) == false))
				{
					_selectedUnit = unit;
				}
			}
			else if (unit->getUnitStatus() != STATUS_DEAD)
			{
				if (const YAML::Node& ai = (*i)["AI"])
				{
					BattleAIState* aiState;

					if (faction == FACTION_HOSTILE)
						aiState = new AlienBAIState(this, unit);
					else
						aiState = new CivilianBAIState(this, unit);

					aiState->load(ai);
					unit->setAIState(aiState);
				}
			}
		}
	}

	for (size_t // load _hostileUnitsThisTurn here
			i = 0;
			i != _units.size();
			++i)
	{
		if (_units.at(i)->isOut_t(OUT_STAT) == false)
			_units.at(i)->loadSpotted(this); // convert unitID's into pointers to BattleUnits
	}

	_shuffleUnits.assign(
					_units.size(),
					nullptr);



	Log(LOG_INFO) << ". reset tiles";
	resetUnitsOnTiles(); // matches up tiles and units

	Log(LOG_INFO) << ". load items";
	static const size_t LIST_TYPE = 3;
	std::string itLists_saved[LIST_TYPE] =
	{
		"items",
		"recoverConditional",
		"recoverGuaranteed"
	};
	std::vector<BattleItem*>* itLists_battle[LIST_TYPE] =
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
			i = 0;
			i != LIST_TYPE;
			++i)
	{
		for (YAML::const_iterator
				j = node[itLists_saved[i]].begin();
				j != node[itLists_saved[i]].end();
				++j)
		{
			st = (*j)["type"].as<std::string>();
			if (rules->getItem(st) != nullptr)
			{
				id = (*j)["id"].as<int>(-1); // note: 'id' should always be valid here.
				item = new BattleItem(
									rules->getItem(st),
									nullptr,
									id);

				item->load(*j);

				if ((*j)["section"])
				{
					st = (*j)["section"].as<std::string>(); // note: the given 'section' should always be valid. Unless it's a loaded Ammo-item.
//					if (st.empty() == false) //!= "NONE") // cf. BattleItem::save()
					item->setInventorySection(rules->getInventory(st));
				}

				owner		= (*j)["owner"]		.as<int>(-1); // cf. BattleItem::save() ->
				ownerPre	= (*j)["ownerPre"]	.as<int>(-1);
				unitId		= (*j)["unit"]		.as<int>(-1);

				if (ownerPre == -1 && owner != -1)
					ownerPre = owner;

				for (std::vector<BattleUnit*>::const_iterator // match up items and units
						k = _units.begin();
						k != _units.end();
						++k)
				{
					if ((*k)->getId() == owner)
						item->changeOwner(*k);

					if ((*k)->getId() == ownerPre)
						item->setPriorOwner(*k);

					if ((*k)->getId() == unitId)
						item->setUnit(*k);
				}

				if (item->getInventorySection() != nullptr // match up items and tiles // note: 'section' should always be valid. Unless it's a loaded Ammo-item.
					&& item->getInventorySection()->getCategory() == IC_GROUND)
				{
					if ((*j)["position"])
					{
						pos = (*j)["position"].as<Position>();
//						if (pos.z != -1)
						getTile(pos)->addItem(
											item,
											rules->getInventoryRule(ST_GROUND));
					}
					else
						pos = Position(0,0,-1); // cf. BattleItem::save()
				}

				itLists_battle[i]->push_back(item);
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
		if (rules->getItem((*i)["type"].as<std::string>()) != nullptr)
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

	Log(LOG_INFO) << ". set some vars";

	_objectiveType = static_cast<SpecialTileType>(node["objectiveType"].as<int>(_objectiveType));
	_objectivesDestroyed	= node["objectivesDestroyed"]	.as<int>(_objectivesDestroyed);
	_objectivesNeeded		= node["objectivesNeeded"]		.as<int>(_objectivesNeeded);

	_turnLimit = node["turnLimit"].as<int>(_turnLimit);
	_chronoResult = static_cast<ChronoResult>(node["chronoResult"].as<int>(_chronoResult));

	_cheatTurn				= node["cheatTurn"]				.as<int>(_cheatTurn);
	_alienRace				= node["alienRace"]				.as<std::string>(_alienRace);
//	_kneelReserved			= node["kneelReserved"]			.as<bool>(_kneelReserved);

//	_batReserved = static_cast<BattleActionType>(node["batReserved"].as<int>(_batReserved));
	_operationTitle = Language::utf8ToWstr(node["operationTitle"].as<std::string>());


	if (node["controlDestroyed"])
		_controlDestroyed = node["controlDestroyed"].as<bool>();


	_music = node["music"].as<std::string>(_music);

	Log(LOG_INFO) << ". set item ID";
	setNextItemId();
	//Log(LOG_INFO) << "SavedBattleGame::load() EXIT";

	// TEST, reveal all tiles
//	for (size_t i = 0; i != _qtyTilesTotal; ++i)
//		_tiles[i]->setDiscovered(true, 2);
}

/**
 * Loads the resources required by the map in the battle save.
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
		mapDataId,
		mapDataSetId;
	const int parts = static_cast<int>(Tile::PARTS_TILE);
	MapDataType partType;

	for (size_t
			i = 0;
			i != _qtyTilesTotal;
			++i)
	{
		for (int
				j = 0;
				j != parts;
				++j)
		{
			partType = static_cast<MapDataType>(j);
			_tiles[i]->getMapData(
								&mapDataId,
								&mapDataSetId,
								partType);

			if (mapDataId != -1
				&& mapDataSetId != -1)
			{
				_tiles[i]->setMapData(
								_mapDataSets[static_cast<size_t>(mapDataSetId)]->getRecords()->at(static_cast<size_t>(mapDataId)),
								mapDataId,
								mapDataSetId,
								partType);
			}
		}
	}

	initUtilities(game->getResourcePack());

	_te->calculateSunShading();
	_te->calculateTerrainLighting();
	_te->calculateUnitLighting();
//	_te->recalculateFOV(); // -> moved to BattlescapeGame::init()
}

/**
 * Saves the saved battle game to a YAML file.
 * @return, YAML node
 */
YAML::Node SavedBattleGame::save() const
{
	YAML::Node node;

	if (_objectivesNeeded != 0)
	{
		node["objectiveType"]		= static_cast<int>(_objectiveType);
		node["objectivesDestroyed"]	= _objectivesDestroyed;
		node["objectivesNeeded"]	= _objectivesNeeded;
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
			i = 0;
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
	node["tileOffsetSize"]		= Tile::serializationKey._animOffset;
	node["tileIDSize"]			= Tile::serializationKey._mapDataId;
	node["tileSetIDSize"]		= Tile::serializationKey._mapDataSetId;
	node["tileBoolFieldsSize"]	= Tile::serializationKey.boolFields;

	size_t tilesDataSize = static_cast<size_t>(Tile::serializationKey.totalBytes) * _qtyTilesTotal;
	Uint8
		* const tilesData = static_cast<Uint8*>(calloc(tilesDataSize, 1)),
		* writeBuffer = tilesData;

	for (size_t
			i = 0;
			i != _qtyTilesTotal;
			++i)
	{
		serializeInt( // <- save ALL Tiles. (Stop void tiles returning undiscovered postReload.)
				&writeBuffer,
				Tile::serializationKey.index,
				static_cast<int>(i));
		_tiles[i]->saveBinary(&writeBuffer);
/*		if (_tiles[i]->isVoid() == false)
		{
			serializeInt(
					&writeBuffer,
					Tile::serializationKey.index,
					static_cast<int>(i));
			_tiles[i]->saveBinary(&writeBuffer);
		}
		else
			tilesDataSize -= Tile::serializationKey.totalBytes; */
	}

	node["totalTiles"]	= tilesDataSize / static_cast<size_t>(Tile::serializationKey.totalBytes); // not strictly necessary, just convenient
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

	if (_tacticalType == "STR_BASE_DEFENSE")
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

//	node["batReserved"]		= static_cast<int>(_batReserved);
//	node["kneelReserved"]	= _kneelReserved;
	node["alienRace"]		= _alienRace;
	node["operationTitle"]	= Language::wstrToUtf8(_operationTitle);

	if (_controlDestroyed == true)
		node["controlDestroyed"] = _controlDestroyed;

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

	node["music"] = _music;

	if (_turnLimit != 0)
	{
		node["turnLimit"] = _turnLimit;
		node["chronoResult"] = static_cast<int>(_chronoResult);
	}

	if (_cheatTurn != CHEAT_DEFAULT)
		node["cheatTurn"] = _cheatTurn;

	return node;
}

/**
 * Gets the array of tiles.
 * @return, pointer to a pointer to the Tile array
 */
Tile** SavedBattleGame::getTiles() const
{
	return _tiles;
}

/**
 * Deletes the old and initializes a new array of tiles.
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
				i = 0;
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
			i = 0;
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
 * Initializes the map utilities.
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
 * Sets the TacticalType based on the battle Type.
 * @param type - reference the battle type
 */
void SavedBattleGame::setTacType(const std::string& type) // private.
{
	if (type.compare("STR_UFO_CRASH_RECOVERY") == 0)
		_tacType = TCT_UFOCRASHED;
	else if (type.compare("STR_UFO_GROUND_ASSAULT") == 0)
		_tacType = TCT_UFOLANDED;
	else if (type.compare("STR_BASE_DEFENSE") == 0)
		_tacType = TCT_BASEDEFENSE;
	else if (type.compare("STR_ALIEN_BASE_ASSAULT") == 0)
		_tacType = TCT_BASEASSAULT;
	else if (type.compare("STR_TERROR_MISSION") == 0
		|| type.compare("STR_PORT_ATTACK") == 0)
	{
		_tacType = TCT_MISSIONSITE;
	}
	else if (type.compare("STR_MARS_CYDONIA_LANDING") == 0)
		_tacType = TCT_MARS1;
	else if (type.compare("STR_MARS_THE_FINAL_ASSAULT") == 0)
		_tacType = TCT_MARS2;
	else
		_tacType = TCT_DEFAULT; // <- the default should probly be TCT_UFOCRASHED.
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
 * Sets the mission type.
 * @param type - reference a mission type
 */
void SavedBattleGame::setTacticalType(const std::string& type)
{
	_tacticalType = type;
	setTacType(_tacticalType);
}

/**
 * Gets the mission type.
 * @note This should return a const ref
 * except perhaps when there's a nextStage that deletes this SavedBattleGame ...
 * and creates a new one wherein the ref is no longer valid.
 * @return, the mission type
 */
std::string SavedBattleGame::getTacticalType() const
{
	return _tacticalType;
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
 * Gets the map width.
 * @return, the map width (Size X) in tiles
 */
int SavedBattleGame::getMapSizeX() const
{
	return _mapsize_x;
}

/**
 * Gets the map length.
 * @return, the map length (Size Y) in tiles
 */
int SavedBattleGame::getMapSizeY() const
{
	return _mapsize_y;
}

/**
 * Gets the map height.
 * @return, the map height (Size Z) in layers
 */
int SavedBattleGame::getMapSizeZ() const
{
	return _mapsize_z;
}

/**
 * Gets the qty of tiles on the battlefield.
 * @return, the total map-size in tiles
 */
size_t SavedBattleGame::getMapSizeXYZ() const
{
	return _qtyTilesTotal;
}

/**
 * Sets the terrain-type string.
 * @param terrain - the terrain
 */
void SavedBattleGame::setBattleTerrain(const std::string& terrain)
{
	_terrain = terrain;
}

/**
 * Gets the terrainType string.
 * @return, the terrain
 */
std::string SavedBattleGame::getBattleTerrain() const
{
	return _terrain;
}

/**
 * Converts a tile index to coordinates.
 * @param index	- the unique tileindex
 * @param x		- pointer to the X coordinate
 * @param y		- pointer to the Y coordinate
 * @param z		- pointer to the Z coordinate
 */
void SavedBattleGame::tileCoords(
		size_t index,
		int* x,
		int* y,
		int* z) const
{
	const int
		i = static_cast<int>(index),
		area = _mapsize_x * _mapsize_y;

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
 * @param unit - pointer to a BattleUnit
 */
void SavedBattleGame::setSelectedUnit(BattleUnit* const unit)
{
	_selectedUnit = unit;
}

/**
 * Selects the next BattleUnit.
 * @note Also used for/by the AI in BattlescapeGame::think(), popState(),
 * handleUnitAI(), and in SavedBattleGame::endFactionTurn().
 * @param checkReselect		- true to check the reselectable flag (default false)
 * @param dontReselect		- true to set the reselectable flag FALSE (default false)
 * @param checkInventory	- true to check if the unit has an inventory (default false)
 * @return, pointer to newly selected BattleUnit or nullptr if none can be selected
 * @sa selectFactionUnit
 */
BattleUnit* SavedBattleGame::selectNextFactionUnit(
		bool checkReselect,
		bool dontReselect,
		bool checkInventory)
{
	return selectFactionUnit(
						+1,
						checkReselect,
						dontReselect,
						checkInventory);
}

/**
 * Selects the previous BattleUnit.
 * @param checkReselect		- true to check the reselectable flag (default false)
 * @param dontReselect		- true to set the reselectable flag FALSE (default false)
 * @param checkInventory	- true to check if the unit has an inventory (default false)
 * @return, pointer to newly selected BattleUnit or nullptr if none can be selected
* @sa selectFactionUnit
*/
BattleUnit* SavedBattleGame::selectPreviousFactionUnit(
		bool checkReselect,
		bool dontReselect,
		bool checkInventory)
{
	return selectFactionUnit(
						-1,
						checkReselect,
						dontReselect,
						checkInventory);
}

/**
 * Selects the next BattleUnit in a certain direction.
 * @param dir				- direction to iterate (+1 for next and -1 for previous)
 * @param checkReselect		- true to check the reselectable flag (default false)
 * @param dontReselect		- true to set the reselectable flag FALSE (default false)
 * @param checkInventory	- true to check if the unit has an inventory (default false)
 * @return, pointer to newly selected BattleUnit or nullptr if none can be selected
 */
BattleUnit* SavedBattleGame::selectFactionUnit( // private.
		int dir,
		bool checkReselect,
		bool dontReselect,
		bool checkInventory)
{
	if (_units.empty() == true)
		return (_selectedUnit = _lastSelectedUnit = nullptr);

	if (_selectedUnit != nullptr && dontReselect == true)
		_selectedUnit->dontReselect();


	std::vector<BattleUnit*>::const_iterator
		iterFirst,
		iterLast,
		iterUnit;

	std::vector<BattleUnit*>* units;
	if (_shuffleUnits.empty() == true // < needed for Base/craft Equip.
		|| _shuffleUnits[0] == nullptr)
	{
		units = &_units;
	}
	else // non-player turn Use shuffledUnits. See endFactionTurn() ....
		units = &_shuffleUnits;

	if (dir > 0)
	{
		iterFirst = units->begin();
		iterLast = units->end() - 1;
	}
	else
	{
		iterFirst = units->end() - 1;
		iterLast = units->begin();
	}

	iterUnit = std::find(
					units->begin(),
					units->end(),
					_selectedUnit);
	do
	{
		if (iterUnit != units->end())
		{
			if (iterUnit != iterLast)
				iterUnit += dir;
			else
				iterUnit = iterFirst;

			if (*iterUnit == _selectedUnit)
			{
				if (checkReselect == true
					&& _selectedUnit->reselectAllowed() == false)
				{
					_selectedUnit = nullptr;
				}

				return _selectedUnit;
			}
			else if (_selectedUnit == nullptr
				&& iterUnit == iterFirst)
			{
				return nullptr;
			}
		}
		else
			iterUnit = iterFirst;
	}
	while ((*iterUnit)->isSelectable(
								_side,
								checkReselect,
								checkInventory) == false);

	return (_selectedUnit = *iterUnit);
}

/**
 * Gets the unit at Position if it's valid and conscious.
 * @param pos - reference a Position
 * @return, pointer to the BattleUnit or nullptr
 */
BattleUnit* SavedBattleGame::selectUnit(const Position& pos)
{
	BattleUnit* const unit (getTile(pos)->getTileUnit());
	if (unit != nullptr && unit->isOut_t(OUT_STAT) == false)
		return unit;

	return nullptr;
}

/**
 * Gets the list of nodes.
 * @return, pointer to a vector of pointers to the Nodes
 */
std::vector<Node*>* SavedBattleGame::getNodes()
{
	return &_nodes;
}

/**
 * Gets the list of units.
 * @return, pointer to a vector of pointers to the BattleUnits
 */
std::vector<BattleUnit*>* SavedBattleGame::getUnits()
{
	return &_units;
}

/**
 * Gets the list of shuffled units.
 * @return, pointer to a vector of pointers to the BattleUnits
 */
std::vector<BattleUnit*>* SavedBattleGame::getShuffleUnits()
{
	return &_shuffleUnits;
}

/**
 * Gets the list of items.
 * @return, pointer to a vector of pointers to the BattleItems
 */
std::vector<BattleItem*>* SavedBattleGame::getItems()
{
	return &_items;
}

/**
 * Gets the pathfinding object.
 * @return, pointer to Pathfinding
 */
Pathfinding* SavedBattleGame::getPathfinding() const
{
	return _pf;
}

/**
 * Gets the terrain modifier object.
 * @return, pointer to the TileEngine
 */
TileEngine* SavedBattleGame::getTileEngine() const
{
	return _te;
}

/**
* Gets the array of mapblocks.
* @return, pointer to a vector of pointers to the MapDataSet
*/
std::vector<MapDataSet*>* SavedBattleGame::getMapDataSets()
{
	return &_mapDataSets;
}

/**
 * Gets the side currently playing.
 * @return, the unit faction currently playing
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
 * Finishes up Alien or Civilian turns and prepares the Player turn.
 */
void SavedBattleGame::prepPlayerTurn() // private.
{
	std::fill(
			_shuffleUnits.begin(),
			_shuffleUnits.end(),
			nullptr);

	tileVolatiles(); // do Tile stuff
	++_turn;

	_side = FACTION_PLAYER;

	if (_lastSelectedUnit != nullptr
		&& _lastSelectedUnit->isSelectable(FACTION_PLAYER) == true)
	{
		//Log(LOG_INFO) << ". . last Selected = " << _lastSelectedUnit->getId();
		_selectedUnit = _lastSelectedUnit;
	}
	else
	{
		//Log(LOG_INFO) << ". . select next";
		selectNextFactionUnit();
	}

	while (_selectedUnit != nullptr
		&& _selectedUnit->getFaction() != FACTION_PLAYER)
	{
		//Log(LOG_INFO) << ". . select next loop";
		selectNextFactionUnit(true);
	}

	//if (_selectedUnit != nullptr) Log(LOG_INFO) << ". -> selected Unit = " << _selectedUnit->getId();
	//else Log(LOG_INFO) << ". -> NO UNIT TO SELECT FOUND";
}

/**
 * Ends the current faction-turn and progresses to the next one.
 * @note Called from BattlescapeGame::endTurn()
 * @return, true if the turn rolls-over back to faction Player
 */
bool SavedBattleGame::endFactionTurn()
{
	//Log(LOG_INFO) << "sbg:endFactionTurn()";
	for (std::vector<BattleUnit*>::const_iterator // -> would it be safe to exclude Dead & Unconscious units
			i = _units.begin();
			i != _units.end();
			++i)
	{
		if ((*i)->getFaction() == _side)
		{
			(*i)->setRevived(false);
			if (_side == FACTION_PLAYER)
				(*i)->dontReselect();
		}
	}

	bool ret;
	switch (_side)
	{
		case FACTION_PLAYER: // end of Player turn.
			_side = FACTION_HOSTILE;
			ret = false;

			if (_selectedUnit != nullptr
				&& _selectedUnit->isMindControlled() == false)
			{
				_lastSelectedUnit = _selectedUnit;
			}
			_selectedUnit = nullptr;

			_scanDots.clear();

			_shuffleUnits = _units;
			RNG::shuffle(_shuffleUnits.begin(), _shuffleUnits.end());
			break;

		case FACTION_HOSTILE: // end of Alien turn.
			_side = FACTION_NEUTRAL;
			if (selectNextFactionUnit() == nullptr)
				ret = true;
			else
				ret = false;
			break;

		default:
		case FACTION_NEUTRAL: // end of Civilian turn.
			//Log(LOG_INFO) << ". end Neutral phase -> PLAYER";
			ret = true;
	}

	if (ret == true)
		prepPlayerTurn();

	// ** _side HAS ADVANCED to next faction after here!!! ** //


	if (_cheatAI == false // pseudo the Turn-20 / less-than-3-aliens-left Reveal rule.
		&& _side == FACTION_HOSTILE
		&& _turn > _cheatTurn / 4)
	{
		for (std::vector<BattleUnit*>::const_iterator
				i = _units.begin();
				i != _units.end();
				++i)
		{
			if ((*i)->isOut_t(OUT_STAT) == false // a conscious non-MC'd aLien ...
				&& (*i)->getFaction() == FACTION_HOSTILE
				&& (*i)->isMindControlled() == false)
			{
				const int r = RNG::generate(0,5);
				if (_turn > _cheatTurn - 3 + r
					|| (_turn > (_cheatTurn / 4)
						&& _battleState->getBattleGame()->tallyHostiles() < r - 1))
				{
					_cheatAI = true;
				}

				break;
			}
		}
	}

	//Log(LOG_INFO) << ". side = " << (int)_side;
	for (std::vector<BattleUnit*>::const_iterator // -> would it be safe to exclude Dead & Unconscious units
			i = _units.begin();
			i != _units.end();
			++i)
	{
		if ((*i)->getUnitStatus() != STATUS_LIMBO)
		{
			if ((*i)->isOut_t(OUT_HLTH) == false)
			{
				(*i)->setDashing(false);	// Safety. no longer dashing; dash is effective
											// vs. Reaction Fire only and is/ought be
											// reset/removed every time BattlescapeGame::primaryAction()
											// uses the Pathfinding object. Other, more ideal
											// places for this safety are UnitWalkBState dTor
											// and/or BattlescapeGame::popState().
				if ((*i)->getOriginalFaction() == _side)
				{
					reviveUnit(*i, true);
					(*i)->takeFire();
				}

				if ((*i)->getFaction() == _side)	// This causes an Mc'd unit to lose its turn.
					(*i)->prepUnit();				// REVERTS FACTION, does tu/stun recovery, Fire damage, etc.
				// if newSide=XCOM, xCom agents DO NOT revert to xCom; MC'd aLiens revert to aLien.
				// if newSide=Alien, xCom agents revert to xCom; MC'd aLiens DO NOT revert to aLien.

				if ((*i)->getFaction() == FACTION_HOSTILE				// aLiens always know where their buddies are,
					|| (*i)->getOriginalFaction() == FACTION_HOSTILE	// Mc'd or not.
					|| _cheatAI == true)								// aLiens know where xCom is when cheating ~turn20
				{
					(*i)->setExposed();
				}
				else if ((*i)->getExposed() != -1
					&& _side == FACTION_PLAYER)
				{
					(*i)->setExposed((*i)->getExposed() + 1);
				}

				if ((*i)->getFaction() != FACTION_PLAYER)
					(*i)->setUnitVisible(false);
			}
			else if ((*i)->getFaction() == _side
				&& (*i)->getFireUnit() != 0)
			{
				(*i)->setFireUnit((*i)->getFireUnit() - 1); // dead burning bodies eventually go out.
			}
		}
	}

	_te->calculateSunShading();
	_te->calculateTerrainLighting();
	_te->calculateUnitLighting(); // turn off MCed alien lighting.

	// redo calculateFOV() *after* aliens & civies have been set notVisible
	// -> AND *only after* a calcLighting has been done!
	_te->recalculateFOV();

	if (_side != FACTION_PLAYER)
		selectNextFactionUnit();

	return ret;
}

/**
 * Turns on debug mode.
 */
void SavedBattleGame::debugTac()
{
	_debugTac = true;

	for (size_t // reveal tiles.
			i = 0;
			i != _qtyTilesTotal;
			++i)
	{
		_tiles[i]->setRevealed(ST_CONTENT);
	}
}

/**
 * Gets the current debug mode.
 * @return, debug mode
 */
bool SavedBattleGame::getDebugTac() const
{
	return _debugTac;
}

/**
 * Gets the BattlescapeGame.
 * @note There can be cases when BattlescapeGame is valid but BattlescapeState
 * is not; during battlescape generation for example -> CTD. So fix it ....
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
 * @return, pointer to the BattlescapeState
 */
BattlescapeState* SavedBattleGame::getBattleState() const
{
	return _battleState;
}

/**
 * Sets the BattlescapeState.
 * @param bs - pointer to the BattlescapeState
 */
void SavedBattleGame::setBattleState(BattlescapeState* bs)
{
	_battleState = bs;
}

/**
 * Resets all the units to their current standing tile(s).
 */
void SavedBattleGame::resetUnitsOnTiles()
{
	for (std::vector<BattleUnit*>::const_iterator
			i = _units.begin();
			i != _units.end();
			++i)
	{
		if ((*i)->isOut_t(OUT_STAT) == false)
		{
			const int armorSize = (*i)->getArmor()->getSize() - 1;

			if ((*i)->getTile() != nullptr // remove unit from its current tile
				&& (*i)->getTile()->getTileUnit() == *i) // wtf, is this super-safety ......
			{
				for (int
						x = armorSize;
						x != -1;
						--x)
				{
					for (int
							y = armorSize;
							y != -1;
							--y)
					{
						getTile((*i)->getTile()->getPosition() + Position(x,y,0))->setUnit(nullptr);
					}
				}
			}

			for (int // set unit onto its proper tile
					x = armorSize;
					x != -1;
					--x)
			{
				for (int
						y = armorSize;
						y != -1;
						--y)
				{
					Tile* const tile = getTile((*i)->getPosition() + Position(x,y,0));
					tile->setUnit(
								*i,
								getTile(tile->getPosition() + Position(0,0,-1)));
				}
			}
		}

		if ((*i)->getFaction() == FACTION_PLAYER)
			(*i)->setUnitVisible();
	}
}

/**
 * Gives access to the storageSpace vector for distribution of items in base
 * defense missions.
 * @return, reference a vector of storage positions
 */
std::vector<Position>& SavedBattleGame::getStorageSpace()
{
	return _storageSpace;
}

/**
 * Move all the leftover items in base defense missions to random locations in
 * the storage facilities.
 * @param tile - pointer to a tile where all the goodies are placed
 */
void SavedBattleGame::randomizeItemLocations(Tile* const tile)
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
				getTile(_storageSpace.at(RNG::pick(_storageSpace.size())))->addItem(*i, (*i)->getInventorySection());
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
 *		- battlegame-items container
 * Upon removal the pointer to the item is kept in the '_toDelete' vector which
 * is flushed and destroyed in the SavedBattleGame dTor.
 * @param item - pointer to an item to remove
 */
void SavedBattleGame::toDeleteItem(BattleItem* const item)
{
	Tile* const tile (item->getTile());
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
			_items.erase(i);
			break;
		}
	}

	_toDelete.push_back(item);
}

/**
 * Sets whether the mission was aborted or successful.
 * @param flag - true if the mission was aborted, or false if the mission was successful (default true)
 */
void SavedBattleGame::setAborted(bool flag)
{
	_aborted = flag;
}

/**
 * Returns whether the mission was aborted or successful.
 * @return, true if the mission was aborted, or false if the mission was successful
 */
bool SavedBattleGame::isAborted() const
{
	return _aborted;
}

/**
 * Sets the objective type for the current battle.
 * @param type - the objective type (RuleItem.h)
 */
void SavedBattleGame::setObjectiveType(SpecialTileType type)
{
	_objectiveType = type;
}

/**
 * Gets the objective type for the current battle.
 * @return, the objective type (RuleItem.h)
 */
SpecialTileType SavedBattleGame::getObjectiveType() const
{
	return _objectiveType;
}

/**
 * Initializes the objectives-needed count.
 * @note Used only to initialize the objective counter; cf addDestroyedObjective() below.
 * @note Objectives were tile-parts marked w/ MUST_DESTROY in their MCD but now
 * can be any specially marked tile. See elsewhere.
 * @param qty - quantity of objective-tileparts that need to be destroyed
 */
void SavedBattleGame::setObjectiveTotal(int qty)
{
	_objectivesNeeded = qty;
	_objectivesDestroyed = 0;
}

/**
 * Increments the objectives-destroyed count and checks whether the necessary
 * quantity of objectives have been destroyed.
 */
void SavedBattleGame::addDestroyedObjective()
{
	if (allObjectivesDestroyed() == false)
	{
		++_objectivesDestroyed;
		if (allObjectivesDestroyed() == true)
		{
			_controlDestroyed = true;
			_battleState->getBattleGame()->objectiveDone();
		}
	}
}

/**
 * Returns whether or not enough objectives have been destroyed.
 * @return, true if the objectives are destroyed
 */
bool SavedBattleGame::allObjectivesDestroyed() const
{
	return _objectivesNeeded != 0
		&& _objectivesNeeded <= _objectivesDestroyed;
}

/**
 * Sets the next available item ID value.
 * @note Used only at the finish of loading a SavedBattleGame.
 * @note ItemIDs start at 0.
 */
void SavedBattleGame::setNextItemId()
{
	int
		id = -1,
		idTest;

	for (std::vector<BattleItem*>::const_iterator
			i = _items.begin();
			i != _items.end();
			++i)
	{
		idTest = (*i)->getId();
		if (idTest > id)
			id = idTest;
	}

	_itemId = ++id;
}

/**
 * Gets the next available item ID value.
 * @return, pointer to the highest available value
 */
int* SavedBattleGame::getNextItemId()
{
	return &_itemId;
}

/**
 * Finds a fitting node where a unit can spawn.
 * @note bgen.addAlien() uses a fallback mechanism to test assorted nodeRanks.
 * @param unitRank	- rank of the unit attempting to spawn
 * @param unit		- pointer to the unit (to test-set its position)
 * @return, pointer to the chosen node
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
		if ((*i)->getPriority() != 0						// spawn-priority 0 is not spawnplace
			&& (*i)->getNodeRank() == unitRank				// ranks must match
			&& isNodeType(*i, unit)
//			&& (!((*i)->getNodeType() & Node::TYPE_SMALL)	// the small unit bit is not set on the node
//				|| unit->getArmor()->getSize() == 1)			// or the unit is small
//			&& (!((*i)->getNodeType() & Node::TYPE_FLYING)	// the flying unit bit is not set on the node
//				|| unit->getMovementType() == MT_FLY)			// or the unit can fly
			&& setUnitPosition(								// check if unit can be set at this node
							unit,								// ie. it's big enough
							(*i)->getPosition(),				// and there's not already a unit there.
							true) == true)					// testOnly, runs again w/ FALSE on return to bgen::addAlien()
		{
			for (int										// weight each eligible node by its Priority.
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
 * Finds a fitting node where a given unit can patrol to.
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
			i = 0;
			i != qtyNodes;
			++i)
	{
		if (scout == true || startNode->getNodeLinks()->at(i) > -1)	// non-scouts need Links to travel along.
		{															// N-E-S-W directions are never used (linkId's -2,-3,-4,-5).
			if (scout == true)										// Meaning that non-scouts never leave their spawn-block ...
				node = getNodes()->at(i);
			else
				node = getNodes()->at(static_cast<size_t>(startNode->getNodeLinks()->at(i)));

			if ((node->getPatrol() != 0
					|| node->getNodeRank() > NR_SCOUT
					|| scout == true)										// for non-scouts find a node with a desirability above 0
				&& node->isAllocated() == false								// check if not allocated
				&& isNodeType(node, unit)
				&& setUnitPosition(											// check if unit can be set at this node
								unit,											// ie. it's big enough
								node->getPosition(),							// and there's not already a unit there.
								true) == true									// but don't actually set the unit...
				&& getTile(node->getPosition()) != nullptr						// the node is on a valid tile
				&& getTile(node->getPosition())->getFire() == 0				// you are not a firefighter; do not patrol into fire
				&& (getTile(node->getPosition())->getDangerous() == false	// aliens don't run into a grenade blast
					|| unit->getFaction() != FACTION_HOSTILE)					// but civies do!
				&& (node != startNode										// scouts push forward
					|| scout == false))											// others can mill around.. ie, stand there
//				&& node->getPosition().x > -1								// x-pos valid
//				&& node->getPosition().y > -1)								// y-pos valid
			{
				for (int
						j = node->getPatrol(); // weight each eligible node by its patrol-Flags.
						j != -1;
						--j)
				{
					scoutNodes.push_back(node);

					if (scout == false
						&& node->getNodeRank() == Node::nodeRank[static_cast<size_t>(unit->getRankInt())]
																[0]) // high-class node here.
					{
						officerNodes.push_back(node);
					}
				}
			}
		}
	}

	if (scoutNodes.empty() == true)
	{
		if (scout == false && unit->getArmor()->getSize() > 1)
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
 * Gets the node considered nearest to a BattleUnit.
 * @note Assume closest node is on same level to avoid strange things.
 * @note The node has to match unit size or the AI will freeze.
 * @param unit - pointer to a BattleUnit
 * @return, the nearest node
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
 * Gets if a BattleUnit can use a particular Node.
 * @note Small units are allowed to use Large nodes and flying units are
 * allowed to use nonFlying nodes.
 * @param node - pointer to a node
 * @param unit - pointer to a unit trying to use the node
 * @return, true if unit can use node
 */
bool SavedBattleGame::isNodeType(
		const Node* const node,
		const BattleUnit* const unit) const
{
	const int type (node->getNodeType());
	if (type == 0)
		return true;

	if (type & Node::TYPE_DANGEROUS)
		return false;

	if (type & Node::TYPE_FLYING)
		return unit->getMoveTypeUnit() == MT_FLY
			&& unit->getArmor()->getSize() == 1;

	if (type & Node::TYPE_SMALL)
		return unit->getArmor()->getSize() == 1;

	if (type & Node::TYPE_LARGEFLYING)
		return unit->getMoveTypeUnit() == MT_FLY;

	return true;
}

/**
 * Carries out new turn preparations such as fire and smoke spreading.
 * @note Also explodes any explosive tiles that get destroyed by fire.
 */
void SavedBattleGame::tileVolatiles()
{
	std::vector<Tile*>
		tilesFired,
		tilesSmoked;

	Tile* tile;
	for (size_t
			i = 0;
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
	for (std::vector<Tile*>::const_iterator
			i = tilesFired.begin();
			i != tilesFired.end();
			++i)
	{
		(*i)->decreaseFire();

		var = (*i)->getFire() << 4;

		if (var != 0)
		{
			for (int
					dir = 0;
					dir != 8;
					dir += 2)
			{
				Position spreadPos;
				Pathfinding::directionToVector(dir, &spreadPos);
				tile = getTile((*i)->getPosition() + spreadPos);

				if (tile != nullptr && _te->horizontalBlockage(*i, tile, DT_IN) == 0)
					tile->ignite(var);
			}
		}
		else
		{
			if ((*i)->getMapData(O_OBJECT) != nullptr)
			{
				if ((*i)->getMapData(O_OBJECT)->getFlammable() != 255
					&& (*i)->getMapData(O_OBJECT)->getArmor() != 255)
				{
					(*i)->destroyTilepart(O_OBJECT, this);
					(*i)->destroyTilepart(O_FLOOR, this);
				}
			}
			else if ((*i)->getMapData(O_FLOOR) != nullptr)
			{
				if ((*i)->getMapData(O_FLOOR)->getFlammable() != 255
					&& (*i)->getMapData(O_FLOOR)->getArmor() != 255)
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
		(*i)->decreaseSmoke();

		var = (*i)->getSmoke() >> 1;

		if (var > 1)
		{
			tile = getTile((*i)->getPosition() + Position(0,0,1));
			if (tile != nullptr && tile->hasNoFloor(*i) == true) // TODO: use verticalBlockage() instead
				tile->addSmoke(var / 3);

			for (int
					dir = 0;
					dir != 8;
					dir += 2)
			{
				if (RNG::percent(var * 8) == true)
				{
					Position posSpread;
					Pathfinding::directionToVector(dir, &posSpread);
					tile = getTile((*i)->getPosition() + posSpread);
					if (tile != nullptr && _te->horizontalBlockage(*i, tile, DT_SMOKE) == 0)
						tile->addSmoke(var >> 1);
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
			reviveUnit(*i, true);
		}
	}
} */

/**
 * Checks if unit is unconscious and revives it if it shouldn't be.
 * @note Revived units need a tile to stand on. If the unit's current position
 * is occupied then all directions around the tile are searched for a free tile
 * to place the unit on. If no free tile is found the unit stays unconscious.
 * @param atTurnOver - true if called from SavedBattleGame::endFactionTurn (default false)
 */
void SavedBattleGame::reviveUnit(
		BattleUnit* const unit,
		bool atTurnOver)
{
	if (unit->getUnitStatus() == STATUS_UNCONSCIOUS
		&& unit->getStun() < unit->getHealth() + static_cast<int>(atTurnOver) // do health=stun if unit is about to get healed in Prep Turn.
		&& (unit->getGeoscapeSoldier() != nullptr
			|| (unit->getUnitRules()->isMechanical() == false
				&& unit->getArmor()->getSize() == 1)))
	{
		if (unit->getFaction() == FACTION_HOSTILE)	// faction will be Original here
			unit->setExposed();						// due to death/stun sequence.
		else
			unit->setExposed(-1);


		Position posCorpse (unit->getPosition());

		if (posCorpse == Position(-1,-1,-1)) // if carried
		{
			for (std::vector<BattleItem*>::const_iterator
					i = _items.begin();
					i != _items.end();
					++i)
			{
				if ((*i)->getUnit() != nullptr
					&& (*i)->getUnit() == unit
					&& (*i)->getOwner() != nullptr)
				{
					posCorpse = (*i)->getOwner()->getPosition();
					break;
				}
			}
		}

		const Tile* const tileCorpse = getTile(posCorpse);
		bool largeUnit = tileCorpse != nullptr
					  && tileCorpse->getTileUnit() != nullptr
					  && tileCorpse->getTileUnit() != unit
					  && tileCorpse->getTileUnit()->getArmor()->getSize() == 2;

		if (placeUnitNearPosition(unit, posCorpse, largeUnit) == true)
		{
			unit->setUnitStatus(STATUS_STANDING);

			if (unit->getGeoscapeSoldier() != nullptr)
				unit->kneel(true);

			unit->clearCache();
			unit->setUnitDirection(RNG::generate(0,7));
			unit->setTimeUnits(0);
			unit->setEnergy(0);
			unit->setRevived();

			_te->calculateUnitLighting();
			_te->calculateFOV(unit->getPosition(), true);
			removeCorpse(unit);

			_battleState->hotWoundsRefresh();
		}
	}
}

/**
 * Removes the body item (corpse) that corresponds to a unit.
 * @param unit - pointer to a BattleUnit
 */
void SavedBattleGame::removeCorpse(const BattleUnit* const unit)
{
	int quad (unit->getArmor()->getSize() * unit->getArmor()->getSize());

	for (std::vector<BattleItem*>::const_iterator
			i = _items.begin();
			i != _items.end();
			++i)
	{
		if ((*i)->getUnit() == unit)
		{
			toDeleteItem(*i);
			--i;

			if (--quad == 0) return;
		}
	}
}

/**
 * Places units on the map.
 * @note Also handles large units that are placed on multiple tiles.
 * @param unit	- pointer to a unit to be placed
 * @param pos	- reference the position to place the unit
 * @param test	- true only checks if unit can be placed at the position (default false)
 * @return, true if unit was placed successfully
 */
bool SavedBattleGame::setUnitPosition(
		BattleUnit* const unit,
		const Position& pos,
		bool test) const
{
	if (unit != nullptr)
	{
//		_pf->setPathingUnit(unit); // <- this is not valid when doing base equip.
		Position posTest = pos; // strip const.

		const int armorSize = unit->getArmor()->getSize() - 1;
		for (int
				x = armorSize;
				x != -1;
				--x)
		{
			for (int
					y = armorSize;
					y != -1;
					--y)
			{
				const Tile* const tile = getTile(posTest + Position(x,y,0));
				if (tile != nullptr)
				{
					if (tile->getTerrainLevel() == -24)
					{
						posTest += Position(0,0,1);
						x =
						y = armorSize + 1; // start over.

						break;
					}

					if ((tile->getTileUnit() != nullptr
							&& tile->getTileUnit() != unit)
						|| tile->getTuCostTile(
											O_OBJECT,
											unit->getMoveTypeUnit()) == 255
						|| (unit->getMoveTypeUnit() != MT_FLY // <- so just use the unit's moveType.
							&& tile->hasNoFloor(getTile(posTest + Position(x,y,-1))) == true))
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

					// TODO: check for ceiling also.
					const Tile* const tileAbove = getTile(posTest + Position(x,y,1));
					if (tileAbove != nullptr
						&& tileAbove->getTileUnit() != nullptr
						&& tileAbove->getTileUnit() != unit
						&& unit->getHeight(true) - tile->getTerrainLevel() > 26) // don't stuck yer head up someone's flying arse.
					{
						return false;
					}
				}
				else
					return false;
			}
		}

		if (armorSize != 0) // -> however, large units never use base equip, so _pf is valid here.
		{
			_pf->setPathingUnit(unit);
			for (int
					dir = 2;
					dir != 5;
					++dir)
			{
				if (_pf->isBlockedPath(getTile(posTest), dir) == true)
					return false;
			}
		}

		if (test == false)
		{
			unit->setPosition(posTest);

			for (int
					x = armorSize;
					x != -1;
					--x)
			{
				for (int
						y = armorSize;
						y != -1;
						--y)
				{
					getTile(posTest + Position(x,y,0))->setUnit(
															unit,
															getTile(posTest + Position(x,y,-1)));
				}
			}
		}

		return true;
	}

	return false;
}

/**
 * Places a unit on or near a position.
 * @param unit		- pointer to a BattleUnit to place
 * @param pos		- reference the position around which to attempt to place @a unit
 * @param isLarge	- true if @a unit is large
 * @return, true if unit is placed
 */
bool SavedBattleGame::placeUnitNearPosition(
		BattleUnit* const unit,
		const Position& pos,
		bool isLarge) const
{
	if (unit == nullptr)
		return false;

	if (setUnitPosition(unit, pos) == true)
		return true;


	int
		size1 = 0 - unit->getArmor()->getSize(),
		size2 = isLarge ? 2 : 1,
		xArray[8] = {    0, size2, size2, size2,     0, size1, size1, size1},
		yArray[8] = {size1, size1,     0, size2, size2, size2,     0, size1};

	const Tile* tile;
	const int dir = RNG::generate(0,7);
	for (int
			i = dir;
			i != dir + 8;
			++i)
	{
		Position posOffset = Position(
									xArray[i % 8],
									yArray[i % 8],
									0);
//		getPathfinding()->directionToVector(
//										i % 8,
//										&posOffset);

		tile = getTile(pos + (posOffset / 2));
//		tile = getTile(pos + posOffset);
		if (tile != nullptr
			&& getPathfinding()->isBlockedPath(tile, dir, nullptr) == false
//			&& getPathfinding()->isBlockedPath(getTile(pos), i) == false
			&& setUnitPosition(unit, pos + posOffset) == true)
		{
			return true;
		}
	}

/*	if (unit->getMovementType() == MT_FLY) // uhh no.
	{
		Tile* tile = getTile(pos + Position(0,0,1));
		if (tile
			&& tile->hasNoFloor(getTile(pos))
			&& setUnitPosition(unit, pos + Position(0,0,1)))
		{
			return true;
		}
	} */

	return false;
}

/**
 * Adds this unit to the vector of falling units if it doesn't already exist there.
 * @param unit - the unit to add
 * @return, true if the unit was added
 */
bool SavedBattleGame::addFallingUnit(BattleUnit* const unit)
{
	for (std::list<BattleUnit*>::const_iterator
			i = _fallingUnits.begin();
			i != _fallingUnits.end();
			++i)
	{
		if (unit == *i)
			return false;
	}

	_fallingUnits.push_front(unit);
	_unitsFalling = true;

	return true;
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
 * Toggles the switch that says "there are units falling, start the fall state".
 * @param fall - true if there are any units falling in the battlescap
 */
void SavedBattleGame::setUnitsFalling(bool fall)
{
	_unitsFalling = fall;
}

/**
 * Returns whether there are any units falling in the battlescape.
 * @return, true if there are any units falling in the battlescape
 */
bool SavedBattleGame::getUnitsFalling() const
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
	const BattleUnit* leader = nullptr;
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

					if (leader == nullptr
						|| (*i)->getRankInt() > leader->getRankInt())
					{
						leader = *i;
					}
				}
			}
			else if ((*i)->getFaction() == FACTION_HOSTILE
				&& (*i)->isMindControlled() == false)
			{
				//Log(LOG_INFO) << "SavedBattleGame::getHighestRanked(), side is aLien";
				++qtyAllies;

				if (leader == nullptr
					|| (*i)->getRankInt() < leader->getRankInt())
				{
					leader = *i;
				}
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
	if (unit != nullptr
		&& unit->getOriginalFaction() == FACTION_NEUTRAL)
	{
		return 100;
	}

	int ret = 100;

	if (unit != nullptr) // morale Loss when 'unit' slain
	{
		//Log(LOG_INFO) << "SavedBattleGame::getMoraleModifier(), unit slain Penalty";
		if (unit->getOriginalFaction() == FACTION_PLAYER) // xCom dies. MC'd or not
		{
			switch (unit->getRankInt())
			{
				case 5:			// commander
					ret += 30;	// 200
				case 4:			// colonel
					ret += 25;	// 170
				case 3:			// captain
					ret += 20;	// 145
				case 2:			// sergeant
					ret += 10;	// 125
				case 1:			// squaddie
					ret += 15;	// 115
			}
			//Log(LOG_INFO) << ". . xCom lossModifi = " << ret;
		}
		else if (unit->getFaction() == FACTION_HOSTILE) // aLien dies. MC'd aliens return 100, or 50 on Mars
		{
			switch (unit->getRankInt()) // soldiers are rank #5, terrorists are ranks #6 and #7
			{
				case 0:			// commander
					ret += 30;	// 200
				case 1:			// leader
					ret += 25;	// 170
				case 2:			// engineer
					ret += 20;	// 145
				case 3:			// medic
					ret += 10;	// 125
				case 4:			// navigator
					ret += 15;	// 115
			}

			if (_tacticalType == "STR_MARS_CYDONIA_LANDING"
				|| _tacticalType == "STR_MARS_THE_FINAL_ASSAULT")
			{
				ret /= 2; // less hit for losing a unit on Cydonia.
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
			leader = getHighestRanked(qtyAllies);

			if (leader != nullptr)
			{
				switch (leader->getRankInt())
				{
					case 5:			// commander
						ret += 15;	// 135, was 150
					case 4:			// colonel
						ret += 5;	// 120, was 125
					case 3:			// captain
						ret += 5;	// 115
					case 2:			// sergeant
						ret += 10;	// 110
					case 1:			// squaddie
						ret += 15;	// 100
					case 0:			// rookies...
						ret -= 15;	// 85
				}
			}
			//Log(LOG_INFO) << ". . xCom leaderModifi = " << ret;
		}
		else // aLien
		{
			leader = getHighestRanked(qtyAllies, false);
			if (leader != nullptr)
			{
				switch (leader->getRankInt()) // terrorists are ranks #6 and #7
				{
					case 0:			// commander
						ret += 25;	// 150
					case 1:			// leader
						ret += 10;	// 125
					case 2:			// engineer
						ret += 5;	// 115
					case 3:			// medic
						ret += 10;	// 110
					case 4:			// navigator
						ret += 10;	// 100
					case 5:			// soldiers...
						ret -= 10;	// 90
				}
			}

			if (_tacticalType == "STR_TERROR_MISSION"
				|| _tacticalType == "STR_ALIEN_BASE_ASSAULT"
				|| _tacticalType == "STR_BASE_DEFENSE")
			{
				ret += 50; // higher morale.
			}
			else if (_tacticalType == "STR_MARS_CYDONIA_LANDING"
						|| _tacticalType == "STR_MARS_THE_FINAL_ASSAULT")
			{
				ret += 100; // higher morale.
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
}

/**
 * Resets visibility of all the tiles on the map.
 */
void SavedBattleGame::blackTiles()
{
	for (size_t
			i = 0;
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
const std::vector<Position> SavedBattleGame::getTileSearch()
{
	return _tileSearch;
}

/**
 * Gets if the AI has started to cheat.
 * @return, true if AI is cheating
 */
bool SavedBattleGame::isCheating()
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
					static_cast<size_t>(_mapsize_x / 10),
					std::vector<std::pair<int,int>>(_mapsize_y / 10, std::make_pair(-1,-1)));

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
					_baseModules[x / 10]
								[y / 10].first += _baseModules[x / 10]
															  [y / 10].first > 0 ? 1 : 2;
					_baseModules[x / 10]
								[y / 10].second = _baseModules[x / 10]
															  [y / 10].first;
				}
			}
		}
	}
}

/**
 * Gets a pointer to the geoscape save.
 * @return, pointer to SavedGame
 */
SavedGame* SavedBattleGame::getGeoscapeSave() const
{
	return _battleState->getGame()->getSavedGame();
}

/**
 * Gets the list of items that are guaranteed to be recovered.
 * @note These items are in the transport.
 * @return, pointer to a vector of pointers to BattleItems
 */
std::vector<BattleItem*>* SavedBattleGame::guaranteedItems()
{
	return &_recoverGuaranteed;
}

/**
 * Gets the list of items that are NOT guaranteed to be recovered.
 * @note These items are NOT in the transport.
 * @return, pointer to a vector of pointers to BattleItems
 */
std::vector<BattleItem*>* SavedBattleGame::conditionalItems()
{
	return &_recoverConditional;
}

/**
 * Sets the battlescape inventory tile when BattlescapeGenerator runs.
 * For use in base missions to randomize item locations.
 * @param equiptTile - pointer to the Tile where tactical equip't should be
 */
void SavedBattleGame::setBattleInventory(Tile* const equiptTile)
{
	_equiptTile = equiptTile;
}

/**
 * Gets the inventory tile for preBattle InventoryState OK click.
 * @return, pointer to the tactical equip't Tile
 */
Tile* SavedBattleGame::getBattleInventory() const
{
	return _equiptTile;
}

/**
 * Sets the aLien race for this battle.
 * @note Currently used only for Base Defense missions but should fill for other
 * missions also.
 * @param alienRace - reference to an alien-race string
 */
void SavedBattleGame::setAlienRace(const std::string& alienRace)
{
	_alienRace = alienRace;
}

/**
 * Gets the aLien race participating in this battle.
 * @note Currently used only to get the aLien race for SoldierDiary statistics
 * after a Base Defense mission.
 * @return, reference to the alien-race string
 */
const std::string& SavedBattleGame::getAlienRace() const
{
	return _alienRace;
}

/**
 * Sets the ground level.
 * @param level - ground level as determined in BattlescapeGenerator
 */
void SavedBattleGame::setGroundLevel(const int level)
{
	_groundLevel = level;
}

/**
 * Gets the ground level.
 * @return, ground level
 */
int SavedBattleGame::getGroundLevel() const
{
	return _groundLevel;
}

/**
 * Gets the operation title of the mission.
 * @return, reference to the title
 */
const std::wstring& SavedBattleGame::getOperation() const
{
	return _operationTitle;
}

/**
 * Tells player that an aLienBase control has been destroyed.
 *
void SavedBattleGame::setControlDestroyed()
{
	_controlDestroyed = true;
} */

/**
 * Gets if an aLienBase control has been destroyed.
 * @return, true if destroyed
 */
bool SavedBattleGame::getControlDestroyed() const
{
	return _controlDestroyed;
}

/**
 * Gets the music track for the current battle.
 * @return, address of the title of the music track
 *
std::string& SavedBattleGame::getMusic()
{
	return _music;
} */

/**
 * Sets the music track for this battle.
 * @note The track-string is const but I don't want to deal with it.
 * @param track - reference the track's name
 */
void SavedBattleGame::setMusic(std::string& track)
{
	_music = track;
}

/**
 * Sets variables for what music to play in a particular terrain or lack thereof.
 * @note The music-string and terrain-string are both const but I don't want to
 * deal with it.
 * @param music		- reference the music category to play
 * @param terrain	- reference the terrain to choose music for
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
			case TCT_MISSIONSITE:	// 4 - STR_TERROR_MISSION and STR_PORT_ATTACK, see setTacType()
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
 * @note Experience gains are no longer allowed once this is set.
 */
void SavedBattleGame::setPacified()
{
	_pacified = true;
}

/**
 * Gets whether the aLiens have been pacified yet.
 * @note Experience gains are no longer allowed if this is set.
 * @return, true if pacified
 */
bool SavedBattleGame::getPacified() const
{
	return _pacified;
}

/**
 * Stores the camera-position where the last RF-trigger happened.
 * @param pos - position
 */
void SavedBattleGame::cacheRfTriggerPosition(const Position& pos)
{
	_rfTriggerPosition = pos;
}

/**
 * Gets the camera-position where the last RF-trigger happened.
 * @return, position
 */
const Position& SavedBattleGame::getRfTriggerPosition() const
{
	return _rfTriggerPosition;
}

/**
 * Gets a ref to the scanner dots vector.
 * @return, reference to a vector of pairs of ints which are positions of current Turn's scanner dots.
 */
std::vector<std::pair<int,int>>& SavedBattleGame::scannerDots()
{
	return _scanDots;
}
const std::vector<std::pair<int,int>>& SavedBattleGame::scannerDots() const
{
	return _scanDots;
}

/**
 * Gets the minimum TU that a unit has at start of its turn.
 * @return, min TU value
 */
int SavedBattleGame::getInitTu() const
{
	return _initTu;
}

/**
 * Sets the previous walking unit.
 * @note Used for controlling the Camera during aLien movement incl/ panic.
 * Stops the Camera from recentering on a unit that just moved and so is already
 * nearly centered but is getting another slice from the AI-engine.
 * @param unit - pointer to a BattleUnit
 */
void SavedBattleGame::setWalkUnit(const BattleUnit* const unit)
{
	_walkUnit = unit;
}

/**
 * Gets the previous walking unit.
 * @note Used for controlling the Camera during aLien movement incl/ panic.
 * Stops the Camera from recentering on a unit that just moved and so is already
 * nearly centered but is getting another slice from the AI-engine.
 * @return, pointer to a BattleUnit
 */
const BattleUnit* SavedBattleGame::getWalkUnit() const
{
	return _walkUnit;
}

/**
 * Sets the turn limit for tactical.
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
const int SavedBattleGame::getTurnLimit() const
{
	return _turnLimit;
}

/**
 * Sets the result to occur when the timer runs out.
 * @param result - the result to perform (AlienDeployment.h)
 */
void SavedBattleGame::setChronoResult(ChronoResult result)
{
	_chronoResult = result;
}

/**
 * Gets the result to perform when the timer expires.
 * @return, the result to perform (AlienDeployment.h)
 */
const ChronoResult SavedBattleGame::getChronoResult() const
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
