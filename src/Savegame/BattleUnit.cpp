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

#include "BattleUnit.h"

//#include <sstream>

#include "../fmath.h"

#include "BattleItem.h"
#include "BattleUnitStatistics.h"
#include "SavedGame.h"
#include "SavedBattleGame.h"
#include "Soldier.h"
#include "Tile.h"

#include "../Battlescape/BattleAIState.h"
#include "../Battlescape/BattlescapeGame.h"
#include "../Battlescape/BattlescapeState.h"
#include "../Battlescape/Map.h"
#include "../Battlescape/Pathfinding.h"
#include "../Battlescape/TileEngine.h"

#include "../Engine/Language.h"
//#include "../Engine/Logger.h"
//#include "../Engine/Options.h"
#include "../Engine/RNG.h"
#include "../Engine/Sound.h"
#include "../Engine/Surface.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/RuleArmor.h"
#include "../Ruleset/RuleInventory.h"
#include "../Ruleset/Ruleset.h"
#include "../Ruleset/RuleSoldier.h"
#include "../Ruleset/RuleUnit.h"


namespace OpenXcom
{

//bool BattleUnit::_debug = false; // static.


/**
 * Initializes the BattleUnit from a specified Soldier.
 * @param sol	- pointer to a geoscape Soldier
 * @param diff	- for negative VictoryPts value at death
 */
BattleUnit::BattleUnit(
		Soldier* const sol,
		const DifficultyLevel diff)
	:
		_geoscapeSoldier(sol),
		_unitRule(nullptr),
		_faction(FACTION_PLAYER),
		_originalFaction(FACTION_PLAYER),
		_killerFaction(FACTION_NONE),
		_murdererId(0),
		_battleGame(nullptr),
		_turretType(TRT_NONE),
		_tile(nullptr),
//		_pos(Position()),
//		_posStart(Position()),
//		_posStop(Position()),
		_dir(0),
		_dirTo(0),
		_dirTurret(0),
		_dirToTurret(0),
		_dirVertical(Pathfinding::DIR_VERT_NONE),
		_dirFace(-1),
		_status(STATUS_STANDING),
		_walkPhase(0),
		_walkPhaseHalf(0),
		_walkPhaseFull(0),
		_walkBackwards(false),
		_fallPhase(0),
		_spinPhase(-1),
		_aimPhase(0),
		_kneeled(false),
		_floating(false),
		_dontReselect(false),
		_fire(0),
		_unitAIState(nullptr),
		_visible(false),
		_cacheInvalid(true),
		_expBravery(0),
		_expReactions(0),
		_expFiring(0),
		_expThrowing(0),
		_expPsiSkill(0),
		_expPsiStrength(0),
		_expMelee(0),
		_takedowns(0),
		_motionPoints(0),
		_coverReserve(0),
		_charging(nullptr),
		_turnsExposed(-1),
		_hidingForTurn(false),
		_battleOrder(0),
		_stopShot(false),
		_dashing(false),
		_takenExpl(false),
		_takenFire(false),
		_diedByFire(false),
		_dirTurn(0),
		_mcStrength(0),
		_mcSkill(0),
		_drugDose(0),
		_isZombie(false),
		_hasCried(false),
		_hasBeenStunned(false),
		_psiBlock(false),

		_deathSound(-1),
		_aggroSound(-1),
		_moveSound(-1),
		_intelligence(0),
		_aggression(0),
		_specab(SPECAB_NONE),
		_morale(100),
		_stunLevel(0),
		_aboutToCollapse(false),
		_type("SOLDIER"),
//		_race("STR_HUMAN"), // not used.
		_activeHand(AH_NONE),
		_fist(nullptr),

		_name(sol->getName()),
		_id(sol->getId()),
		_rank(sol->getRankString()),
		_armor(sol->getArmor()),
		_loftSet(sol->getArmor()->getLoftSet()),
		_mType(sol->getArmor()->getMoveTypeArmor()),
		_standHeight(sol->getRules()->getStandHeight()),
		_kneelHeight(sol->getRules()->getKneelHeight()),
		_floatHeight(sol->getRules()->getFloatHeight()),
		_gender(sol->getGender()),

		_stats(*sol->getCurrentStats()),

		_lastCover(-1,-1,-1)
{
	//Log(LOG_INFO) << "Create BattleUnit 1 : soldier ID = " << getId();
	_stats += *_armor->getStats();

	int rankValue;
	switch (sol->getRank())
	{
		case RANK_SQUADDIE:		rankValue =	 2;	break; // was 0
		case RANK_SERGEANT:		rankValue =	 5;	break; // was 1
		case RANK_CAPTAIN:		rankValue =	15;	break; // was 3
		case RANK_COLONEL:		rankValue =	30;	break; // was 6
		case RANK_COMMANDER:	rankValue =	50;	break; // was 10

		default:
			rankValue = 0;
	}

	_value = 20 + ((sol->getMissions() + rankValue) * (diff + 1));

	_tu = _stats.tu;
	_energy = _stats.stamina;
	_health = _stats.health;

	_armorHp[SIDE_FRONT]	= _armor->getFrontArmor();
	_armorHp[SIDE_LEFT]		=
	_armorHp[SIDE_RIGHT]	= _armor->getSideArmor();
	_armorHp[SIDE_REAR]		= _armor->getRearArmor();
	_armorHp[SIDE_UNDER]	= _armor->getUnderArmor();

	for (size_t
			i = 0u;
			i != PARTS_ARMOR;
			++i)
	{
		_cache[i] = nullptr;
	}

	for (size_t
			i = 0u;
			i != PARTS_BODY;
			++i)
	{
		_fatalWounds[i] = 0;
	}

//	for (int i = 0; i < SPEC_WEAPON_MAX; ++i)
//		_specWeapon[i] = 0;

	deriveRank(); // -> '_rankInt'

	const int look ((static_cast<int>(sol->getLook()) << 1u)
				   + static_cast<int>(sol->getGender()));
	setRecolor(
			look,
			look,
			_rankInt);

	_statistics = new BattleUnitStatistics(); // Soldier Diary
	//Log(LOG_INFO) << "Create BattleUnit 1, DONE";
}

/**
 * Creates the BattleUnit from a (non-Soldier) Unit-rule object.
 * @param unitRule		- pointer to RuleUnit
 * @param faction		- faction the unit belongs to
 * @param id			- the unit's unique-ID
 * @param armor			- pointer to unit's armor
 * @param diff			- the current game's difficulty setting (for aLien stat adjustment) (default DIFF_BEGINNER)
 * @param month			- the current month (default 0)
 * @param battleGame	- pointer to the BattlescapeGame (default nullptr)
 */
BattleUnit::BattleUnit(
		RuleUnit* const unitRule,
		const UnitFaction faction,
		const int id,
		RuleArmor* const armor,
		const DifficultyLevel diff,
		const int month,
		BattlescapeGame* const battleGame) // for converted Units
	:
		_unitRule(unitRule),
		_geoscapeSoldier(nullptr),
		_id(id),
		_faction(faction),
		_originalFaction(faction),
		_killerFaction(FACTION_NONE),
		_murdererId(0),
		_armor(armor),
		_battleGame(battleGame),
		_rankInt(5), // aLien soldier, this includes Civies.
		_turretType(TRT_NONE),
//		_pos(Position()),
//		_posStart(Position()),
//		_posStop(Position()),
		_tile(nullptr),
		_dir(0),
		_dirTo(0),
		_dirTurret(0),
		_dirToTurret(0),
		_dirVertical(Pathfinding::DIR_VERT_NONE),
		_dirFace(-1),
		_status(STATUS_STANDING),
		_walkPhase(0),
		_walkPhaseHalf(0),
		_walkPhaseFull(0),
		_walkBackwards(false),
		_fallPhase(0),
		_spinPhase(-1),
		_aimPhase(0),
		_kneeled(false),
		_floating(false),
		_fire(0),
		_unitAIState(nullptr),
		_visible(false),
		_cacheInvalid(true),
		_expBravery(0),
		_expReactions(0),
		_expFiring(0),
		_expThrowing(0),
		_expPsiSkill(0),
		_expPsiStrength(0),
		_expMelee(0),
		_takedowns(0),
		_motionPoints(0),
		_coverReserve(0),
		_charging(nullptr),
		_hidingForTurn(false),
		_stopShot(false),
		_dashing(false),
		_takenExpl(false),
		_takenFire(false),
		_battleOrder(0),
		_hasCried(false),
		_hasBeenStunned(false),

		_morale(100),
		_stunLevel(0),
		_aboutToCollapse(false),
		_activeHand(AH_NONE),
		_diedByFire(false),
		_dirTurn(0),
		_mcStrength(0),
		_mcSkill(0),
		_drugDose(0),
//		_isZombie(false),
		_fist(nullptr),

		_statistics(nullptr), // Soldier Diary

		_type(unitRule->getType()),
		_race(unitRule->getRace()),
		_rank(unitRule->getRank()),
		_standHeight(unitRule->getStandHeight()),
		_kneelHeight(unitRule->getKneelHeight()),
		_floatHeight(unitRule->getFloatHeight()),
		_deathSound(unitRule->getDeathSound()),
		_aggroSound(unitRule->getAggroSound()),
		_moveSound(unitRule->getMoveSound()),
		_intelligence(unitRule->getIntelligence()),
		_aggression(unitRule->getAggression()),
		_spawnType(unitRule->getSpawnType()),
		_value(unitRule->getValue()),
		_psiBlock(unitRule->getPsiBlock()),
		_specab(unitRule->getSpecialAbility()),

		_loftSet(armor->getLoftSet()),
		_mType(armor->getMoveTypeArmor()),

		_stats(*unitRule->getStats()),

		_lastCover(-1,-1,-1)
{
	//Log(LOG_INFO) << "Create BattleUnit 2 : alien ID = " << getId();
	_stats += *_armor->getStats();

	_isZombie = (_race == "STR_ZOMBIE");

	switch (_faction)
	{
		case FACTION_PLAYER:
			_dontReselect = false;
			_turnsExposed = -1;
			break;

		case FACTION_HOSTILE:
			_dontReselect = true;
			_turnsExposed = 0;
			adjustStats(diff, month);
			break;

		case FACTION_NEUTRAL:
			_dontReselect = true;
			_turnsExposed = -1;
	}

	_tu		= _stats.tu;
	_energy	= _stats.stamina;
	_health	= _stats.health;

	if (unitRule->isFemale() == true)
		_gender = GENDER_FEMALE;
	else
		_gender = GENDER_MALE;

	_armorHp[SIDE_FRONT]	= _armor->getFrontArmor();
	_armorHp[SIDE_LEFT]		=
	_armorHp[SIDE_RIGHT]	= _armor->getSideArmor();
	_armorHp[SIDE_REAR]		= _armor->getRearArmor();
	_armorHp[SIDE_UNDER]	= _armor->getUnderArmor();

	for (size_t
			i = 0u;
			i != PARTS_ARMOR;
			++i)
	{
		_cache[i] = nullptr;
	}

	for (size_t
			i = 0u;
			i != PARTS_BODY;
			++i)
	{
		_fatalWounds[i] = 0;
	}

//	for (int i = 0; i < SPEC_WEAPON_MAX; ++i)
//		_specWeapon[i] = 0;

	//Log(LOG_INFO) << "Create BattleUnit 2, DONE";
}
/*	int rankInt = 0;
	if (faction == FACTION_HOSTILE)
	{
		const size_t ranks = 7;
		const char* rankList[ranks] =
		{
			"STR_LIVE_SOLDIER",
			"STR_LIVE_ENGINEER",
			"STR_LIVE_MEDIC",
			"STR_LIVE_NAVIGATOR",
			"STR_LIVE_LEADER",
			"STR_LIVE_COMMANDER",
			"STR_LIVE_TERRORIST",
		};

		for (size_t i = 0; i != ranks; ++i)
		{
			if (_rank.compare(rankList[i]) == 0)
			{
				rankInt = static_cast<int>(i);
				break;
			}
		}
	}
	else if (faction == FACTION_NEUTRAL)
		rankInt = std::rand() % 8;

	setRecolor(std::rand() % 8, std::rand() % 8, rankInt); */
//	setRecolor(0,0,0);	// kL, just make the vector so something naughty doesn't happen ....
						// On 2nd thought don't even do that.

/**
 * dTor.
 */
BattleUnit::~BattleUnit()
{
	//Log(LOG_INFO) << "Delete BattleUnit";
	for (size_t
			i = 0;
			i != PARTS_ARMOR;
			++i)
	{
//		if (_cache[i] != nullptr)
		delete _cache[i];
	}

/* Soldier Diary, not needed for nonSoldiers. Or soldiers for that matter ....
	if (getGeoscapeSoldier() == nullptr)
	{
		for (std::vector<BattleUnitKill*>::const_iterator
				i = _statistics->kills.begin();
				i != _statistics->kills.end();
				++i)
		{
			delete *i;
		}
	} */
//	if (_geoscapeSoldier != nullptr) // ... delete it anyway:
	delete _statistics;

	delete _unitAIState;
}

/**
 * Loads this BattleUnit from a YAML file.
 * @param node - reference a YAML node
 */
void BattleUnit::load(const YAML::Node& node)
{
	_status			= static_cast<UnitStatus>(node["status"]		.as<int>(_status));
	_killerFaction	= static_cast<UnitFaction>(node["killerFaction"].as<int>(_killerFaction));
	_faction		= static_cast<UnitFaction>(node["faction"]		.as<int>(_faction));
	if (node["originalFaction"])
		_originalFaction = static_cast<UnitFaction>(node["originalFaction"].as<int>());
	else
		_originalFaction = _faction;

	_id					= node["id"]					.as<int>(_id);
	_pos				= node["position"]				.as<Position>(_pos);

	_dir				=
	_dirTo				= node["direction"]				.as<int>(_dir);
	_dirTurret			=
	_dirToTurret		= node["directionTurret"]		.as<int>(_dirTurret);

	_tu					= node["tu"]					.as<int>(_tu);
	_health				= node["health"]				.as<int>(_health);
	_stunLevel			= node["stunLevel"]				.as<int>(_stunLevel);
	_energy				= node["energy"]				.as<int>(_energy);
	_morale				= node["morale"]				.as<int>(_morale);
	_floating			= node["floating"]				.as<bool>(_floating);
	_fire				= node["fire"]					.as<int>(_fire);
	_visible			= node["visible"]				.as<bool>(_visible);
	_turnsExposed		= node["turnsExposed"]			.as<int>(_turnsExposed);
	_rankInt			= node["rankInt"]				.as<int>(_rankInt);
	_takedowns			= node["takedowns"]				.as<int>(_takedowns);
	_dontReselect		= node["dontReselect"]			.as<bool>(_dontReselect);
	_motionPoints		= node["motionPoints"]			.as<int>(_motionPoints);
	_spawnType			= node["spawnType"]				.as<std::string>(_spawnType);
	_mcStrength			= node["mcStrength"]			.as<int>(_mcStrength);
	_mcSkill			= node["mcSkill"]				.as<int>(_mcSkill);
	_drugDose			= node["drugDose"]				.as<int>(_drugDose);
	_murdererId			= node["murdererId"]			.as<int>(_murdererId);
	_hasBeenStunned		= node["beenStunned"]			.as<bool>(_hasBeenStunned);
	_charging			= nullptr;

	_turretType = static_cast<TurretType>(node["turretType"].as<int>(_turretType));
	_activeHand = static_cast<ActiveHand>(node["activeHand"].as<int>(_activeHand));

	for (size_t
			i = 0u;
			i != PARTS_ARMOR;
			++i)
	{
		_armorHp[i] = node["armor"][i].as<int>(_armorHp[i]);
	}

	if (_geoscapeSoldier != nullptr)
	{
		if (node["fatalWounds"])
		{
			for (size_t
					i = 0u;
					i != PARTS_BODY;
					++i)
			{
				_fatalWounds[i] = node["fatalWounds"][i].as<int>(_fatalWounds[i]);
			}
		}

		if (node["diaryStatistics"])
			_statistics->load(node["diaryStatistics"]);

		_battleOrder	= node["battleOrder"]	.as<size_t>(_battleOrder);
		_kneeled		= node["kneeled"]		.as<bool>(_kneeled);

		_expBravery		= node["expBravery"]	.as<int>(_expBravery);
		_expReactions	= node["expReactions"]	.as<int>(_expReactions);
		_expFiring		= node["expFiring"]		.as<int>(_expFiring);
		_expThrowing	= node["expThrowing"]	.as<int>(_expThrowing);
		_expPsiSkill	= node["expPsiSkill"]	.as<int>(_expPsiSkill);
		_expPsiStrength	= node["expPsiStrength"].as<int>(_expPsiStrength);
		_expMelee		= node["expMelee"]		.as<int>(_expMelee);
	}

	if (node["spottedUnits"])
	{
		const std::vector<int> spotted (node["spottedUnits"].as<std::vector<int>>());
		for (size_t
				i = 0u;
				i != spotted.size();
				++i)
		{
			_spotted.push_back(spotted.at(i));
		}
	}
	// Convert those (int)id's into pointers to BattleUnits during
	// SavedBattleGame loading *after* all BattleUnits have loaded.

//	if (const YAML::Node& p = node["recolor"])
//	{
//		_recolor.clear();
//		for (size_t i = 0; i != p.size(); ++i)
//			_recolor.push_back(std::make_pair(p[i][0].as<uint8_t>(), p[i][1].as<uint8_t>()));
//	}
}

/**
 * Loads the vector of units-spotted-this-turn during SavedBattleGame load.
 * @param battleSave - pointer to the SavedBattleGame
 */
void BattleUnit::loadSpotted(SavedBattleGame* const battleSave)
{
	for (size_t
			i = 0u;
			i != _spotted.size();
			++i)
	{
		for (std::vector<BattleUnit*>::const_iterator
				j = battleSave->getUnits()->begin();
				j != battleSave->getUnits()->end();
				++j)
		{
			if ((*j)->getId() == _spotted.at(i))
			{
				_hostileUnitsThisTurn.push_back(*j);
				break;
			}
		}
	}
	_spotted.clear();
}

/**
 * Saves this BattleUnit to a YAML file.
 * @return, YAML node
 */
YAML::Node BattleUnit::save() const
{
	YAML::Node node;

	node["id"] = _id;

	if (getName().empty() == false)
		node["name"] = Language::wstrToUtf8(getName());

	node["genUnitType"]		= _type;
	node["genUnitArmor"]	= _armor->getType();

	node["faction"] = static_cast<int>(_faction);
	if (_originalFaction != _faction)
	{
		node["originalFaction"]	= static_cast<int>(_originalFaction);

		if (_originalFaction != FACTION_HOSTILE)
		{
			node["mcStrength"]	= _mcStrength;
			node["mcSkill"]		= _mcSkill;
		}
	}

	node["status"]			= static_cast<int>(_status);
	node["position"]		= _pos;
	node["direction"]		= _dir;
	node["directionTurret"]	= _dirTurret;
	node["tu"]				= _tu;
	node["health"]			= _health;
	node["stunLevel"]		= _stunLevel;
	node["energy"]			= _energy;
	node["turnsExposed"]	= _turnsExposed;
	node["rankInt"]			= _rankInt;

	if (_morale != 100)					node["morale"]			= _morale;
	if (_floating == true)				node["floating"]		= _floating;
	if (_fire != 0)						node["fire"]			= _fire;
	if (_turretType != TRT_NONE)		node["turretType"]		= static_cast<int>(_turretType); // TODO: use unitRule to get turretType.
	if (_visible == true)				node["visible"]			= _visible;
	if (_killerFaction != FACTION_NONE)	node["killerFaction"]	= static_cast<int>(_killerFaction);
	if (_motionPoints != 0)				node["motionPoints"]	= _motionPoints;
	if (_takedowns != 0)				node["takedowns"]		= _takedowns;
	if (_drugDose != 0)					node["drugDose"]		= _drugDose;
	if (_murdererId != 0)				node["murdererId"]		= _murdererId;
	if (_hasBeenStunned == true)		node["beenStunned"]		= _hasBeenStunned;

	node["activeHand"] = static_cast<int>(_activeHand);

	// could put (if not tank) here:

	if (_unitAIState != nullptr)
		node["AI"] = _unitAIState->save();

	if (_faction == FACTION_PLAYER
		&& (_dontReselect == true
			|| (_originalFaction != FACTION_PLAYER && _dontReselect == false)))
	{
		node["dontReselect"] = _dontReselect;
	}

	if (_spawnType.empty() == false)
		node["spawnType"] = _spawnType;

	for (size_t
			i = 0u;
			i != PARTS_ARMOR;
			++i)
	{
		node["armor"].push_back(_armorHp[i]);
	}

	if (_geoscapeSoldier != nullptr)
	{
		for (size_t
				i = 0u;
				i != PARTS_BODY;
				++i)
		{
			node["fatalWounds"].push_back(_fatalWounds[i]);
		}

		if (_statistics->statsDefault() == false)
			node["diaryStatistics"] = _statistics->save();

		node["battleOrder"] = _battleOrder;

		if (_kneeled == true) node["kneeled"] = _kneeled;

		if (_expBravery		!= 0) node["expBravery"]		= _expBravery;
		if (_expReactions	!= 0) node["expReactions"]		= _expReactions;
		if (_expFiring		!= 0) node["expFiring"]			= _expFiring;
		if (_expThrowing	!= 0) node["expThrowing"]		= _expThrowing;
		if (_expPsiSkill	!= 0) node["expPsiSkill"]		= _expPsiSkill;
		if (_expPsiStrength	!= 0) node["expPsiStrength"]	= _expPsiStrength;
		if (_expMelee		!= 0) node["expMelee"]			= _expMelee;
	}

//	for (size_t i = 0; i != _recolor.size(); ++i)
//	{
//		YAML::Node p;
//		p.push_back(static_cast<int>(_recolor[i].first));
//		p.push_back(static_cast<int>(_recolor[i].second));
//		node["recolor"].push_back(p);
//	}

	if (_faction == FACTION_PLAYER && _originalFaction == FACTION_PLAYER
		&& _status == STATUS_STANDING)
	{
		for (size_t
				i = 0u;
				i != _hostileUnitsThisTurn.size();
				++i)
		{
			node["spottedUnits"].push_back(_hostileUnitsThisTurn.at(i)->getId());
		}
	}


	return node;
		// kL_note: This doesn't save/load such things as
		// _hostileUnits (no need),
		// _hostileUnitsThisTurn (done),
		// _visibleTiles (removed).
		// AI is saved, but loaded someplace else -> SavedBattleGame ->
		// so are _hostileUnitsThisTurn
}

/**
 * Prepare vector values for recolor.
 * @param basicLook	- select index for hair and face color
 * @param utileLook	- select index for utile color
 * @param rankLook	- select index for rank color
 */
void BattleUnit::setRecolor(
		int basicLook,
		int utileLook,
		int rankLook)
{
	static const size_t GROUPS (4u);
	std::pair<int,int> colors[GROUPS] =
	{
		std::make_pair(
					_armor->getColorGroupFace(),
					_armor->getColorFace(basicLook)),
		std::make_pair(
					_armor->getColorGroupHair(),
					_armor->getColorHair(basicLook)),
		std::make_pair(
					_armor->getColorGroupUtile(),
					_armor->getColorUtile(utileLook)),
		std::make_pair(
					_armor->getColorGroupRank(),
					_armor->getColorRank(rankLook)),
	};

	for (size_t
			i = 0u;
			i != GROUPS;
			++i)
	{
		if (colors[i].first > 0 && colors[i].second > 0)
			_recolor.push_back(std::make_pair(
										colors[i].first << 4u,
										colors[i].second));
	}
}

/**
 * Gets this BattleUnit's unique-ID.
 * @return, the unique-ID
 */
int BattleUnit::getId() const
{
	return _id;
}

/**
 * Gets this BattleUnit's type-ID.
 * @return, unit type
 */
std::string BattleUnit::getType() const
{
	return _type;
}

/**
 * Gets this BattleUnit's rank-string.
 * @return, rank
 */
std::string BattleUnit::getRankString() const
{
	return _rank;
}

/**
 * Gets this BattleUnit's race-string.
 * @return, race
 */
std::string BattleUnit::getRaceString() const
{
	return _race;
}

/**
 * Gets this BattleUnit's geoscape-soldier if any.
 * @return, pointer to Soldier
 */
Soldier* BattleUnit::getGeoscapeSoldier() const
{
	return _geoscapeSoldier;
}

/**
 * Sets this BattleUnit's Position.
 * @param pos			- reference to a position
 */
void BattleUnit::setPosition(const Position& pos)
{
	_pos = pos;
}

/**
 * Gets this BattleUnit's position.
 * @return, reference to the position
 */
const Position& BattleUnit::getPosition() const
{
	return _pos;
}

/**
 * Gets the Position at which this BattleUnit started walking Tile-to-Tile.
 * @note This is one step only - UnitWalkBState updates it after *every* tile.
 * @return, reference to the origin
 */
const Position& BattleUnit::getStartPosition() const
{
	return _posStart;
}

/**
 * Gets the Position at which this BattleUnit will stop walking Tile-to-Tile.
 * @note This is one step only - UnitWalkBState updates it after *every* tile.
 * @return, reference to the destination
 */
const Position& BattleUnit::getStopPosition() const
{
	return _posStop;
}

/**
 * Sets this BattleUnit's horizontal direction.
 * @param dir		- new horizontal direction
 * @param turret	- true to set the turret-direction also (default true)
 */
void BattleUnit::setUnitDirection(
		int dir,
		bool turret)
{
	_dir =
	_dirTo = dir;

	if (turret == true) // || _turretType == TRT_NONE
		_dirTurret = dir;
}

/**
 * Gets this BattleUnit's horizontal direction.
 * @return, horizontal direction
 */
int BattleUnit::getUnitDirection() const
{
	return _dir;
}

/**
 * Looks at a specified Position.
 * @param pos		- reference to the position to look toward
 * @param turret	- true to turn the turret, false to turn the whole unit (default false)
 */
void BattleUnit::setDirectionTo(
		const Position& pos,
		bool turret)
{
	setDirectionTo(
				TileEngine::getDirectionTo(_pos, pos),
				turret);
}

/**
 * Look a direction.
 * @param dir		- direction to look
 * @param turret	- true to turn the turret, false to turn the whole unit (default false)
 */
void BattleUnit::setDirectionTo(
		int dir,
		bool turret)
{
	if (dir > -1 && dir < 8)
	{
		if (turret == true)
		{
			if (dir != _dirTurret)
			{
				_dirToTurret = dir;
				_status = STATUS_TURNING;
			}
		}
		else if (dir != _dir)
		{
			_dirTo = dir;
			_status = STATUS_TURNING;
		}
	}
}

/**
 * Sets this BattleUnit's horizontal direction (facing).
 * @note Only used for strafing moves.
 * @param dir - new horizontal direction (facing)
 */
void BattleUnit::setFaceDirection(int dir)
{
	_dirFace = dir;
}

/**
 * Gets this BattleUnit's horizontal direction (facing).
 * @note Used only during strafing moves.
 * @return, horizontal direction (facing)
 */
int BattleUnit::getFaceDirection() const
{
	return _dirFace;
}

/**
 * Sets this BattleUnit's turret direction.
 * @param dir - turret direction
 */
void BattleUnit::setTurretDirection(int dir)
{
	_dirTurret = dir;
}

/**
 * Gets this BattleUnit's turret direction.
 * @return, turret direction
 */
int BattleUnit::getTurretDirection() const
{
	return _dirTurret;
}

/**
 * Gets this BattleUnit's turret To direction.
 * @return, toDirectionTurret
 *
int BattleUnit::getTurretToDirection() const
{
	return _dirToTurret;
} */

/**
 * Gets this BattleUnit's vertical direction.
 * @note This is when going up or down, doh!
 * @return, vertical direction (0=none 8=up 9=down)
 */
int BattleUnit::getVerticalDirection() const
{
	return _dirVertical;
}

/**
 * Advances the turning towards the target direction.
 * @param turret - true to turn the turret (default false to turn the whole unit)
 */
void BattleUnit::turn(bool turret)
{
	int delta;
	if (turret == true)
	{
		if (_dirTurret == _dirToTurret)
		{
			_status = STATUS_STANDING;
			return;
		}
		delta = _dirToTurret - _dirTurret;
	}
	else
	{
		if (_dir == _dirTo)
		{
			_status = STATUS_STANDING;
			return;
		}
		delta = _dirTo - _dir;
	}

	if (delta != 0) // duh
	{
		if (delta > 0)
		{
			if (delta < 5 && _dirTurn != -1)
			{
				if (turret == false)
				{
					++_dir;
					if (_turretType != TRT_NONE)
						++_dirTurret;
				}
				else
					++_dirTurret;
			}
			else // > 4
			{
				if (turret == false)
				{
					--_dir;
					if (_turretType != TRT_NONE)
						--_dirTurret;
				}
				else
					--_dirTurret;
			}
		}
		else
		{
			if (delta > -5 && _dirTurn != 1)
			{
				if (turret == false)
				{
					--_dir;
					if (_turretType != TRT_NONE)
						--_dirTurret;
				}
				else
					--_dirTurret;
			}
			else // < -4
			{
				if (turret == false)
				{
					++_dir;
					if (_turretType != TRT_NONE)
						++_dirTurret;
				}
				else
					++_dirTurret;
			}
		}

		if		(_dir < 0) _dir = 7;
		else if	(_dir > 7) _dir = 0;

		if		(_dirTurret < 0) _dirTurret = 7;
		else if	(_dirTurret > 7) _dirTurret = 0;

		if (_visible == true)
			_cacheInvalid = true;
	}

	if (   (turret == false && _dirTo == _dir)
		|| (turret == true  && _dirToTurret == _dirTurret))
	{
		_status = STATUS_STANDING;
	}
}

/**
 * Gets the walk-phase for calculating Map offset.
 * @return, phase (full range)
 */
int BattleUnit::getWalkPhaseTrue() const
{
	return _walkPhase;
}

/**
 * Gets the walk-phase for sprite determination and various FX triggers.
 * @return, phase (0..7)
 */
int BattleUnit::getWalkPhase() const
{
	return _walkPhase % 8;
}

/**
 * Initializes variables to start walking.
 * @param dir		- the direction to walk
 * @param posStop	- reference to the Position the unit should end up at
 * @param tileBelow	- pointer to the Tile below destination position
 */
void BattleUnit::startWalking(
		int dir,
		const Position& posStop,
		const Tile* const tileBelow)
{
	_walkPhase = 0;

	_posStart = _pos;
	_posStop = posStop;

	_kneeled = false;

	switch (dir)
	{
		case Pathfinding::DIR_UP:
		case Pathfinding::DIR_DOWN:
			_status = STATUS_FLYING; // controls walking sound in UnitWalkBState, what else
			_dirVertical = dir;
			_floating = _tile->getMapData(O_FLOOR) == nullptr
					 || _tile->getMapData(O_FLOOR)->isGravLift() == false;
			break;

		default:
			_dir = dir;
			if (_tile->solidFloor(tileBelow) == false) // NOTE: The tile is the Tile of only the primary quadrant for large units.
			{
				_status = STATUS_FLYING;
				_floating = true;
			}
			else
			{
				_status = STATUS_WALKING;
				_floating = false;
			}
	}
	cacheWalkPhases();
}

/**
 * This will increment '_walkPhase'.
 * @param tileBelow	- pointer to tile currently below this unit
 * @param recache	- true to update the unit cache / redraw this unit's sprite
 */
void BattleUnit::keepWalking(
		const Tile* const tileBelow,
		bool recache)
{
	_cacheInvalid = recache;

	if (++_walkPhase == _walkPhaseHalf)	// assume unit reached the destination tile
		_pos = _posStop;				// This is actually a drawing hack so soldiers are not overlapped by floortiles fwiw.

	if (_walkPhase == _walkPhaseFull) // officially reached the destination tile
	{
		_walkPhase = 0;
		_status = STATUS_STANDING;
		_dirVertical = Pathfinding::DIR_VERT_NONE;

		if (_floating == true && _tile->solidFloor(tileBelow) == true)
			_floating = false;

		if (_dirFace != -1) // finish strafing move facing the correct way.
		{
			_dir = _dirFace;
			_dirFace = -1;
			_walkBackwards = false;
		}

		switch (_armor->getSize())
		{
			case 2: // motion points calculation for motion-scanner blips
				_motionPoints += 30;
				break;

			case 1:
				if (_standHeight > 16)	// sectoids actually have less motion points but instead of creating
					_motionPoints += 4;	// yet another variable use the height of the unit instead
				else
					_motionPoints += 3;
		}
	}
}

/**
 * Calculates and stores the half- and full-phase cutoffs for this BattleUnit's
 * sprite-drawing.
 */
void BattleUnit::cacheWalkPhases() // private.
{
	switch (_dirVertical)
	{
		case Pathfinding::DIR_VERT_NONE:
			_walkPhaseFull = 8 + (8 * (_dir & 1)); // diagonal walking takes double the steps
			switch (_armor->getSize())
			{
				case 1:
					_walkPhaseHalf = (_walkPhaseFull >> 1u);
					break;

				case 2:
					switch (_dir)
					{
						case 0:
						case 6:
						case 7:
							_walkPhaseHalf = _walkPhaseFull;
							break;

						case 1:
							_walkPhaseHalf = 5;
							break;

						case 2:
						case 3:
						case 4:
							_walkPhaseHalf = 1;
							break;

						case 5:
							_walkPhaseHalf = 12;
					}
			}
			break;

		case Pathfinding::DIR_UP:
		case Pathfinding::DIR_DOWN:
			_walkPhaseHalf = 4;
			_walkPhaseFull = 8;
	}
}

/**
 * Flags this BattleUnit as doing a backwards-ish strafe move.
 */
void BattleUnit::flagStrafeBackwards()
{
	_walkBackwards = true;
}

/**
 * Checks if this BattleUnit is strafing in a backwards-ish direction.
 * @return, true if backwards
 */
bool BattleUnit::isStrafeBackwards() const
{
	return _walkBackwards;
}

/**
 * Gets this BattleUnit's current walking-halfphase.
 * return, current half-phase
 */
int BattleUnit::getWalkPhaseHalf() const
{
	return _walkPhaseHalf;
}

/**
 * Gets this BattleUnit's current walking-fullphase.
 * return, current full-phase
 */
int BattleUnit::getWalkPhaseFull() const
{
	return _walkPhaseFull;
}

/**
 * Sets this BattleUnit's status.
 * @param status - UnitStatus (BattleUnit.h)
 */
void BattleUnit::setUnitStatus(const UnitStatus status)
{
	_status = status;	// TODO: Make a call to instaKill() or putDown() here.
}						// - adjust according to dead or unconscious and remove
						// all the extraneous stuff that's littered throughout the rest of the code.
/**
 * Gets this BattleUnit's status.
 * @return, UnitStatus (BattleUnit.h)
 */
UnitStatus BattleUnit::getUnitStatus() const
{
	return _status;
}

/**
 * Gets this BattleUnit's gender.
 * @return, SoldierGender enum
 */
SoldierGender BattleUnit::getGender() const
{
	return _gender;
}

/**
 * Gets this BattleUnit's faction.
 * @return, UnitFaction enum (player, hostile or neutral)
 */
UnitFaction BattleUnit::getFaction() const
{
	return _faction;
}

/**
 * Converts this BattleUnit to another faction.
 * @note Its original faction is still stored as such.
 * @param faction - UnitFaction
 */
void BattleUnit::setFaction(UnitFaction faction)
{
	_faction = faction;
}

/**
 * Gets this BattleUnit's original Faction.
 * @return, original UnitFaction (BattleUnit.h)
 */
UnitFaction BattleUnit::getOriginalFaction() const
{
	return _originalFaction;
}

/**
 * Gets this BattleUnit's sprite-cache.
 * @note When the unit animates it needs to be re-cached.
 * @param quadrant - quadrant to check (default 0)
 * @return, pointer to the Surface
 */
Surface* BattleUnit::getCache(int quadrant) const
{
	return _cache[static_cast<size_t>(quadrant)];
}

/**
 * Sets this BattleUnit's sprite-cached flag.
 * @note Set to true when the unit needs to be redrawn.
 * @param cache		- pointer to a Surface
 * @param quadrant	- unit quadrant to update (default 0)
 */
void BattleUnit::setCache(
		Surface* const cache,
		int quadrant)
{
	_cache[static_cast<size_t>(quadrant)] = cache;
	_cacheInvalid = false;
}

/**
 * Clears this BattleUnit's sprite-cached flag.
 */
void BattleUnit::setCacheInvalid()
{
	_cacheInvalid = true;
}

/**
 * Gets if this BattleUnit's sprite-cache is invalid.
 * @return, true if invalid
 */
bool BattleUnit::getCacheInvalid() const
{
	return _cacheInvalid;
}

/**
 * Gets values used for re-coloring sprites.
 * @return, pairs of values where first is the colorGroup to replace and the
 *			second is the new colorGroup with shade
 */
const std::vector<std::pair<Uint8, Uint8>>& BattleUnit::getRecolor() const
{
	return _recolor;
}

/**
 * Kneels or stands this BattleUnit.
 * @param kneeled - true to kneel, false to stand up
 */
void BattleUnit::kneelUnit(bool kneel)
{
	if (_kneeled != kneel)
	{
		_kneeled = kneel;
		_cacheInvalid = true;
	}
}

/**
 * Gets if this BattleUnit is kneeling.
 * @return, true if kneeled
 */
bool BattleUnit::isKneeled() const
{
	return _kneeled;
}

/**
 * Sets the BattleUnit floating.
 * param isAirborne - true if floating (default true)
 */
void BattleUnit::setFloating(bool isAirborne)
{
	_floating = isAirborne;
}

/**
 * Gets if this BattleUnit is floating.
 * @note A unit is floating if there is no ground underneath.
 * @return, true if floating
 */
bool BattleUnit::isFloating() const
{
	return _floating;
}

/**
 * Shows this BattleUnit's sprite with its arm(s) and weapon up & shooting.
 * @param shoot - true to shoot, false to stand there like an idiot (default true)
 *
void BattleUnit::setShoot(bool shoot)
{
	if (shoot == true)	_status = STATUS_AIMING;
	else				_status = STATUS_STANDING;
	_cacheInvalid = true;
} */
void BattleUnit::toggleShoot()
{
	switch (_status)
	{
		case STATUS_STANDING:
			_status = STATUS_AIMING;
			_cacheInvalid = true;
			_battleGame->getMap()->cacheUnitSprite(this);
			break;

		case STATUS_AIMING:
			_status = STATUS_STANDING;
			_cacheInvalid = true;
			_battleGame->getMap()->cacheUnitSprite(this);
	}
}

/**
 * Gets this BattleUnit's current TU.
 * @return, current turn-units
 */
int BattleUnit::getTu() const
{
	return _tu;
}

/**
 * Gets this BattleUnit's current energy.
 * @return, current stamina
 */
int BattleUnit::getEnergy() const
{
	return _energy;
}

/**
 * Gets this BattleUnit's current health.
 * @return, current health
 */
int BattleUnit::getHealth() const
{
	return _health;
}

/**
 * Gets this BattleUnit's current morale.
 * @return, current morale
 */
int BattleUnit::getMorale() const
{
	return _morale;
}

/**
 * Gets this BattleUnit's current effective strength.
 * @return, current strength
 */
int BattleUnit::getStrength() const
{
	return static_cast<int>(Round(static_cast<double>(_stats.strength) * (getAccuracyModifier() / 2. + 0.5)));
}

/**
 * Does an quantity of damage to this BattleUnit.
 * @note The return-value is used only to determine who gets credit for a kill
 * in the case of antecedentWoundage, in TileEngine::hit().
 * @param relVoxel		- reference to a Position in voxel-space that defines
 *						  which part of armor and/or body gets hit
 * @param power			- the quantity of pain to inflict
 * @param dType			- the DamageType being inflicted (RuleItem.h)
 * @param ignoreArmor	- true for stun & smoke & inc damage; no armor reduction
 *						  although vulnerability is still factored in
 */
int BattleUnit::takeDamage(
		const Position& relVoxel,
		int power,
		DamageType dType,
		const bool ignoreArmor)
{
	//Log(LOG_INFO) << "bu:takeDamage() id-" << _id << " power[0]= " << power;
	power = static_cast<int>(Round(
			static_cast<float>(power) * _armor->getDamageModifier(dType)));
	//Log(LOG_INFO) << ". dType = " << (int)dType << " power[1]= " << power;

//	if (power < 1) // kL_note: this early-out messes with got-hit sFx below_
//		return 0;

	if (dType == DT_SMOKE) // smoke is really stun damage.
		dType = DT_STUN;

	if (dType == DT_STUN && power < 1)
		return 0;

	UnitBodyPart bodyPart (BODYPART_TORSO);
	const bool woundable (isWoundable());

	if (power > 0 && ignoreArmor == false)
	{
		UnitSide side;

		if (relVoxel == Position(0,0,0))
			side = SIDE_UNDER;
		else
		{
			side = SIDE_FRONT;

			int dirRel;
			const int
				abs_x (std::abs(relVoxel.x)),
				abs_y (std::abs(relVoxel.y));

			if (abs_y > abs_x * 2)
				dirRel = 8 + 4 * static_cast<int>(relVoxel.y > 0);	// hit from South (y-pos) or North (y-neg)
			else if (abs_x > abs_y * 2)
				dirRel = 10 + 4 * static_cast<int>(relVoxel.x < 0);	// hit from East (x-pos) or West (x-neg)
			else
			{
				if (relVoxel.x < 0)	// hit from West (x-neg)
				{
					if (relVoxel.y > 0)
						dirRel = 13;	// hit from SouthWest (y-pos)
					else
						dirRel = 15;	// hit from NorthWest (y-neg)
				}
				else				// hit from East (x-pos)
				{
					if (relVoxel.y > 0)
						dirRel = 11;	// hit from SouthEast (y-pos)
					else
						dirRel = 9;		// hit from NorthEast (y-neg)
				}
			}

			switch ((dirRel - _dir) % 8)
			{
				case 0:	side = SIDE_FRONT;						break;
				case 1:	side = RNG::percent(50)	? SIDE_FRONT
												: SIDE_RIGHT;	break;
				case 2:	side = SIDE_RIGHT;						break;
				case 3:	side = RNG::percent(50)	? SIDE_REAR
												: SIDE_RIGHT;	break;
				case 4:	side = SIDE_REAR;						break;
				case 5:	side = RNG::percent(50)	? SIDE_REAR
												: SIDE_LEFT; 	break;
				case 6:	side = SIDE_LEFT;						break;
				case 7:	side = RNG::percent(50)	? SIDE_FRONT
												: SIDE_LEFT;
			}
			//Log(LOG_INFO) << ". . side= " << (int)side;

			if (woundable == true)
			{
				if (relVoxel.z > getHeight() - 4)
					bodyPart = BODYPART_HEAD;
				else if (relVoxel.z > 5)
				{
					switch (side)
					{
						case SIDE_LEFT:		bodyPart = BODYPART_LEFTARM;	break;
						case SIDE_RIGHT:	bodyPart = BODYPART_RIGHTARM;	break;
						default:			bodyPart = BODYPART_TORSO;
					}
				}
				else
				{
					switch (side)
					{
						case SIDE_LEFT: 	bodyPart = BODYPART_LEFTLEG; 	break;
						case SIDE_RIGHT:	bodyPart = BODYPART_RIGHTLEG; 	break;
						default:
							bodyPart = static_cast<UnitBodyPart>(RNG::generate(
																			BODYPART_RIGHTLEG,
																			BODYPART_LEFTLEG));
					}
				}
			}
			//Log(LOG_INFO) << ". . bodyPart = " << (int)bodyPart;
		}

		const int armor (getArmor(side)); // armor damage
		setArmor(
				std::max(0,
						 armor - (power + 9) / 10), // round up.
				side);

		power -= armor; // subtract armor-before-damage from power.
		//Log(LOG_INFO) << ". power[2]= " << power;
	}

	if (power > 0)
	{
		const bool selfAware (_geoscapeSoldier != nullptr
						  || (_unitRule->isMechanical() == false
								&& _isZombie == false));
		int wounds (0);

		switch (dType)
		{
			case DT_STUN:
				if (selfAware == true) _stunLevel += power;
				break;

			default: // health damage
				if ((_health -= power) < 1)
				{
					_health = 0;

					if (dType == DT_IN)
					{
						_diedByFire = true;
						_spawnType.clear();

						if (_isZombie == true)
							_specab = SPECAB_EXPLODE;
						else
							_specab = SPECAB_NONE;
					}
				}
				else
				{
					if (selfAware == true)
						_stunLevel += RNG::generate((power + 9) / 10, (power + 2) / 3); // round up.

					wounds = RNG::generate(1,3);

					if (ignoreArmor == false // Only wearers of armors-that-are-resistant-to-damage-type can take fatal wounds.
						&& woundable == true // fatal wounds
						&& RNG::generate(0,10) < power) // kL: refactor this.
					{
						_fatalWounds[bodyPart] += wounds;
					}

					if (dType == DT_IN) wounds = power; // for Morale loss by fire.
				}
		}

		if (_health != 0 && selfAware == true)
		{
			moraleChange(-wounds * 3);

			int morale ((110 - _stats.bravery) / 10);
			if (morale > 0)
			{
				int leadership (100);		// <- for civilians & pre-battle PS explosion.
				if (_battleGame != nullptr)	// ie. don't CTD on preBattle power-source explosion.
				{
					switch (_originalFaction)
					{
						case FACTION_PLAYER:
							leadership = _battleGame->getBattlescapeState()->getSavedBattleGame()->getMoraleModifier();
							break;

						case FACTION_HOSTILE:
							leadership = _battleGame->getBattlescapeState()->getSavedBattleGame()->getMoraleModifier(nullptr, false);
					}
				}

				morale = morale * power * 10 / leadership;
				moraleChange(-morale);
			}
		}

		if (_status != STATUS_UNCONSCIOUS // if not already collapsed but about to be.
			&& (_health == 0 || isStunned() == true))
		{
			_aboutToCollapse = true;
		}
	}

	// TODO: give a short "ugh" if hit causes no damage or perhaps stuns ( power must be > 0 though );
	// a longer "uuuhghghgh" if hit causes damage ... and let DieBState handle deathscreams.
	if (_aboutToCollapse == false //&& _visible == true && _health > 0 && _health > _stunLevel
		&& _hasCried == false
		&& _status != STATUS_UNCONSCIOUS
		&& dType != DT_STUN
		&& (_geoscapeSoldier != nullptr
			|| _unitRule->isMechanical() == false))
	{
		playDeathSound(true);
	}

	if (power < 0) power = 0;
	//Log(LOG_INFO) << ". ret power[3]= " << power;

	return power;
}

/**
 * Plays this BattleUnit's death-scream or hit-grunt.
 * @param fleshWound - true if only hit but not death; used by Soldiers only
 */
void BattleUnit::playDeathSound(bool fleshWound) const
{
	if (_battleGame != nullptr) // check if hit by pre-battle hidden/power-source explosion.
	{
		int soundId;
		if (_geoscapeSoldier != nullptr)
		{
			switch (_gender)
			{
				default:
				case GENDER_MALE:
					if (fleshWound == true)
						soundId = RNG::seedless(141,151);
					else
						soundId = RNG::seedless(111,116);
					break;

				case GENDER_FEMALE:
					if (fleshWound == true)
						soundId = RNG::seedless(121,135);
					else
						soundId = RNG::seedless(101,103);
			}
		}
		else if (_unitRule->getRace() == "STR_CIVILIAN")
		{
			switch (_gender)
			{
				default:
				case GENDER_MALE:
					soundId = static_cast<int>(ResourcePack::MALE_SCREAM[static_cast<size_t>(RNG::seedless(0,2))]);
					break;

				case GENDER_FEMALE:
					soundId = static_cast<int>(ResourcePack::FEMALE_SCREAM[static_cast<size_t>(RNG::seedless(0,2))]);
			}
		}
		else
			soundId = _deathSound;

		if (soundId != -1)
			_battleGame->getResourcePack()->getSound("BATTLE.CAT", static_cast<unsigned>(soundId))
											->play(-1, _battleGame->getMap()->getSoundAngle(_pos));
	}
}

/**
 * Sets this BattleUnit as having cried out from a shotgun blast to the face.
 * @note So that it doesn't scream for each pellet.
 * @param cried - true if hit
 */
void BattleUnit::hasCried(bool cried)
{
	_hasCried = cried;
}

/**
 * Gets if this BattleUnit has cried already.
 * @return, true if cried
 */
bool BattleUnit::hasCried() const
{
	return _hasCried;
}

/**
 * Sets this BattleUnit's health-level.
 * @param health - the health to set
 */
void BattleUnit::setHealth(int health)
{
	_health = health;
}

/**
 * Reduces this BattleUnit's stun-level.
 * @param stun - stun to recover
 * @return, true if unit revives
 */
bool BattleUnit::reduceStun(int stun)
{
	if ((_stunLevel -= stun) < 0)
		_stunLevel = 0;

	if (_status == STATUS_UNCONSCIOUS && _stunLevel < _health)
		return true;

	return false;
}

/**
 * Gets this BattleUnit's stun-level.
 * @return, quantity of stun
 */
int BattleUnit::getStun() const
{
	return _stunLevel;
}

/**
 * Sets this BattleUnit's stun-level.
 * @param stun - quantity of stun
 */
void BattleUnit::setStun(int stun)
{
	_stunLevel = stun;
}

/**
 * Checks if this BattleUnit is currently stunned.
 * return, true if stunned
 */
bool BattleUnit::isStunned()
{
	return _stunLevel >= _health;
}

/**
 * Raises this BattleUnit's stun-level sufficiently so that it's ready to go
 * to Status_Unconscious.
 * @note Units convert to their spawn-unit.
 */
void BattleUnit::knockOut()
{
	if (_spawnType.empty() == false)
	{
		BattleUnit* const unit (_battleGame->convertUnit(this));
		unit->knockOut();
	}
	else if (_unitRule != nullptr
		&& (_unitRule->isMechanical() == true || _isZombie == true))
	{
		_health = 0;
	}
	else if (_stunLevel < _health)
		_stunLevel = _health;
}

/**
 * Initializes the collapsing sequence.
 * @note This is only for dead or stunned units.
 */
void BattleUnit::startCollapsing()
{
	_status = STATUS_COLLAPSING;
	_fallPhase = 0;
	_cacheInvalid = true;
}

/**
 * Advances the phase of the collapsing sequence.
 */
void BattleUnit::keepCollapsing()
{
	if (_diedByFire == true)
		_fallPhase = _armor->getDeathFrames();
	else
		++_fallPhase;

	if (_fallPhase == _armor->getDeathFrames())
	{
		--_fallPhase;

		switch (_health)
		{
			case 0:
				_status = STATUS_DEAD;
				break;

			default:
				_status = STATUS_UNCONSCIOUS;
		}
	}
	_cacheInvalid = true;
}

/**
 * Gets the phase of the collapsing sequence.
 * @return, phase
 */
int BattleUnit::getCollapsingPhase() const
{
	return _fallPhase;
}

/**
 * Intializes the aiming sequence.
 */
void BattleUnit::startAiming()
{
	if (_armor->getShootFrames() == 0)
		return;

	_status = STATUS_AIMING;
	_aimPhase = 0;

	if (_visible == true)
		_cacheInvalid = true;
}

/**
 * Advances the phase of the aiming sequence.
 * @note This is not called in 1-to-1 sync with Map drawing; animation speed
 * changes will cause the phase-sprites to either get skipped or double up.
 * So I'm going to try doing this right in UnitSprite::drawRoutine9() - done.
 */
void BattleUnit::keepAiming()
{
	if (_aimPhase == _armor->getShootFrames() + 1)
		_status = STATUS_STANDING;

	if (_visible == true)
		_cacheInvalid = true;
}

/**
 * Gets the phase of the aiming sequence.
 * @return, aiming phase
 */
int BattleUnit::getAimingPhase() const
{
	return _aimPhase;
}

/**
 * Sets the phase of the aiming sequence.
 * @return, aiming phase
 */
void BattleUnit::setAimingPhase(int phase)
{
	_aimPhase = phase;
}

/**
 * Checks whether this BattleUnit is out of combat - eg, dead or unconscious.
 * @note A unit that is 'out' cannot perform any actions and cannot be selected
 * but it's still a valid unit.
 * @param test - what to check for (default OUT_ALL) (BattleUnit.h)
 *				 OUT_ALL		- everything
 *				 OUT_STAT		- status dead or unconscious or limbo'd
 *				 OUT_HEALTH		- health
 *				 OUT_STUNNED	- stun only
 *				 OUT_HLTH_STUN	- health or stun
 * @return, true if unit is incapacitated
 */
bool BattleUnit::isOut_t(OutCheck test) const
{
	switch (test)
	{
		default:
		case OUT_ALL:
			if (_health == 0 || _health <= _stunLevel)
				return true;
			// no break;
		case OUT_STAT:
			switch (_status)
			{
				case STATUS_DEAD:
				case STATUS_UNCONSCIOUS:
				case STATUS_LATENT:
				case STATUS_LATENT_START:
					return true;
			}
			break;

		case OUT_HEALTH:
			if (_health == 0)
				return true;
			break;

		case OUT_STUNNED:
			if (_health != 0 && _health <= _stunLevel)
				return true;
			break;

		case OUT_HLTH_STUN:
			if (_health == 0 || _health <= _stunLevel)
				return true;
	}

	return false;
}

/**
 * Gets the number of turn-units a certain action takes for this BattleUnit.
 * @param bat	- BattleActionType (BattlescapeGame.h)
 * @param item	- pointer to BattleItem for TU-cost
 * @return, TUs to perform action
 */
int BattleUnit::getActionTu( // TODO: Refactor these ....
		const BattleActionType bat,
		const BattleItem* const item) const
{
	if (bat != BA_NONE && item != nullptr)
		return getActionTu(bat, item->getRules());

	return 0;
}

/**
 * Gets the number of turn-units a certain action takes for this BattleUnit.
 * @param bat		- BattleActionType (BattlescapeGame.h)
 * @param itRule	- pointer to RuleItem for TU-costs (default nullptr -- see BattlescapeGame::checkReservedTu())
 * @return, TUs to perform action
 */
int BattleUnit::getActionTu(
		const BattleActionType bat,
		const RuleItem* const itRule) const
{
	if (bat == BA_NONE) return 0;

	int cost (0);
	switch (bat)
	{
		// TODO: Put "tuThrow" yaml-entry in rules under various grenade-types, etc.
		case BA_DROP:
		{
			const RuleInventory
				* const handRule (_battleGame->getRuleset()->getInventoryRule(ST_RIGHTHAND)), // might be leftHand Lol ...
				* const grdRule (_battleGame->getRuleset()->getInventoryRule(ST_GROUND));
			return handRule->getCost(grdRule); // flat rate.
		}

		case BA_DEFUSE:
			if (itRule != nullptr) cost = itRule->getDefuseTu();
			break;

		case BA_PRIME:
			if (itRule != nullptr) cost = itRule->getPrimeTu();
			break;

		case BA_THROW: // NOTE: use wt.
			cost = 23; // force fractional-rate below.
			break;

		case BA_LAUNCH:
			if (itRule != nullptr) cost = itRule->getLaunchTu();
			break;

		case BA_AIMEDSHOT:
			if (itRule != nullptr) cost = itRule->getAimedTu();
			break;

		case BA_AUTOSHOT:
			if (itRule != nullptr) cost = itRule->getAutoTu();
			break;

		case BA_SNAPSHOT:
			if (itRule != nullptr) cost = itRule->getSnapTu();
			break;

		case BA_MELEE:
			if (itRule != nullptr) cost = itRule->getMeleeTu();
			break;

		case BA_LIQUIDATE:
			return 19; // flat rate.

		case BA_USE:
		case BA_PSIPANIC:
		case BA_PSICONTROL:
		case BA_PSICONFUSE:
		case BA_PSICOURAGE:
			if (itRule != nullptr) cost = itRule->getUseTu();
			break;

		default:
			return 0;	// problem: cost=0 can lead to infinite loop in reaction fire
						// Presently, cost=0 means 'cannot do' but conceivably an action
						// could be free, or cost=0; so really cost=-1 ought be
						// implemented, to mean 'cannot do' ......
						// (ofc this default is rather meaningless, but there is a point)
	}

	if (cost != 0
		&& ((itRule != nullptr && itRule->isFlatRate() == false) // it's a percentage, apply to TUs
			|| bat == BA_THROW))
	{
		return std::max(1,
						static_cast<int>(std::floor(
						static_cast<float>(_stats.tu * cost) / 100.f)));
	}
	return cost;
}

/**
 * Expends turn-units if it can, returns false if it can't.
 * @param tu - the TU to check & spend
 * @return, true if this unit could expend the turn-units
 */
bool BattleUnit::expendTu(int tu)
{
	if (tu <= _tu)
	{
		_tu -= tu;
		return true;
	}
	return false;
}

/**
 * Expends energy if it can, returns false if it can't.
 * @param energy - the stamina to check & expend
 * @return, true if this unit could expend the stamina
 */
bool BattleUnit::expendEnergy(int energy)
{
	if (energy <= _energy)
	{
		_energy -= energy;
		return true;
	}
	return false;
}

/**
 * Expends TU and Energy.
 * @note Called by UnitWalkBState::statusStand() after checks are done.
 * @param tu		- tu
 * @param energy	- stamina
 */
void BattleUnit::expendTuEnergy(
		int tu,
		int energy)
{
	_tu -= tu;
	_energy -= energy;
}

/**
 * Sets a specified quantity of TUs.
 * @note This must be allowed to go above the unit's Tu-cap to allow for
 * accurate action-cancellations in or about BattlescapeGame::popState().
 * @param tu - the TU to set for this unit (default 0)
 */
void BattleUnit::setTu(int tu)
{
	if ((_tu = tu) < 0) _tu = 0;
}

/**
 * Sets a specified quantity of energy.
 * @param energy - the stamina to set for this unit (default 0)
 */
void BattleUnit::setEnergy(int energy)
{
	if ((_energy = energy) < 0)
		_energy = 0;
	else if (_energy > _stats.stamina)
		_energy = _stats.stamina;
}

/**
 * Sets whether this BattleUnit is visible to the player, should it be drawn.
 * @param flag - true if visible (default true)
 */
void BattleUnit::setUnitVisible(bool flag)
{
	_visible = flag;
}

/**
 * Gets whether this BattleUnit is visible.
 * @return, true if visible
 */
bool BattleUnit::getUnitVisible() const
{
	return _visible;
}

/**
 * Adds a unit to this BattleUnit's vectors of currently spotted and/or recently
 * spotted hostile units.
 * @note For aliens these are xCom and civies; for xCom these are aliens only.
 * '_hostileUnits' are currently spotted; '_hostileUnitsThisTurn' are just that
 * - don't confuse either of these with the '_visible' to Player flag.
 * @note Called from TileEngine::calcFov().
 * @param unit - pointer to a BattleUnit
 * @return, true if a hostile unit is freshly spotted on the current turn
 */
bool BattleUnit::addToHostileUnits(BattleUnit* const unit)
{
	if (std::find(
			_hostileUnits.begin(),
			_hostileUnits.end(),
			unit) == _hostileUnits.end())
	{
		_hostileUnits.push_back(unit);
	}

	bool spot (false);
	if (std::find(
			_hostileUnitsThisTurn.begin(),
			_hostileUnitsThisTurn.end(),
			unit) == _hostileUnitsThisTurn.end())
	{
		_hostileUnitsThisTurn.push_back(unit);	// <- don't think I even use this anymore ....
		spot = true;							// Maybe for AI .... doggie barks. unseen blocking units ...
	}
	return spot;
}

/**
 * Gets this BattleUnit's vector of currently spotted hostile units.
 * @return, pointer to a vector of pointers to BattleUnits
 */
std::vector<BattleUnit*>& BattleUnit::getHostileUnits()
{
	return _hostileUnits;
}

/**
 * Clears currently spotted hostile units.
 */
void BattleUnit::clearHostileUnits()
{
	_hostileUnits.clear();
}

/**
 * Gets this BattleUnit's vector of hostile units that have been spotted during
 * the current turn.
 * @return, reference to a vector of pointers to BattleUnits
 */
std::vector<BattleUnit*>& BattleUnit::getHostileUnitsThisTurn()
{
	return _hostileUnitsThisTurn;
}

/**
 * Clears recently spotted hostile units.
 *
void BattleUnit::clearHostileUnitsThisTurn()
{
	_hostileUnitsThisTurn.clear();
} */
/**
 * Adds a tile to the list of visible tiles.
 * @param tile - pointer to a tile to add
 * @return, true or CTD
 *
bool BattleUnit::addToVisibleTiles(Tile* const tile)
{
	_visibleTiles.push_back(tile);
	return true;
} */
/**
 * Gets the pointer to the vector of visible tiles.
 * @return, pointer to a vector of pointers to visible tiles
 *
std::vector<Tile*>* BattleUnit::getVisibleTiles()
{
	return &_visibleTiles;
} */
/**
 * Clears visible tiles.
 *
void BattleUnit::clearVisibleTiles()
{
	for (std::vector<Tile*>::const_iterator
			j = _visibleTiles.begin();
			j != _visibleTiles.end();
			++j)
	{
		(*j)->setTileVisible(false);
	}
	_visibleTiles.clear();
} */

/**
 * Calculates firing and/or throwing accuracy.
 * @param action	- reference to the current BattleAction (BattlescapeGame.h)
 * @param bat		- BattleActionType (default BA_NONE) (BattlescapeGame.h)
 * @return, accuracy
 */
double BattleUnit::getAccuracy(
		const BattleAction& action,
		const BattleActionType bat) const
{
	static const double PCT = 0.01;
	double ret;

	const RuleItem* const itRule (action.weapon->getRules());

	BattleActionType baType;
	switch (bat)
	{
		case BA_NONE:	baType = action.type; break;
		default:		baType = bat;
	}

	switch (baType)
	{
		case BA_LAUNCH:
			return 1.;

		case BA_MELEE:
			ret = static_cast<double>(itRule->getAccuracyMelee()) * PCT;

			if (itRule->isSkillApplied() == true)
				ret *= static_cast<double>(_stats.melee) * PCT;
			if (_kneeled == true)
				ret *= 0.83;
			break;

		case BA_THROW:
			ret = static_cast<double>(_stats.throwing) * PCT;
			if (_kneeled == true)
				ret *= 0.86;
			break;

		default:
			switch (baType)
			{
				case BA_AIMEDSHOT:
					ret = static_cast<double>(itRule->getAccuracyAimed()) * PCT;
					break;
				case BA_AUTOSHOT:
					ret = static_cast<double>(itRule->getAccuracyAuto()) * PCT;
					break;
				default:
				case BA_SNAPSHOT:
					ret = static_cast<double>(itRule->getAccuracySnap()) * PCT;
			}

			ret *= static_cast<double>(_stats.firing) * PCT;
			if (_kneeled == true)
				ret *= 1.16;
	}

	ret *= getAccuracyModifier(action.weapon);

	if (action.weapon == getItem(ST_RIGHTHAND))
	{
		const BattleItem* const itLeft (getItem(ST_LEFTHAND));
		if (itLeft != nullptr)
		{
			if (action.weapon->getRules()->isTwoHanded() == true)
			{
				if (itLeft->getRules()->isTwoHanded() == true)
					ret *= 0.51;	// big penalty for dual-wielding 2x2h weapons
				else
					ret *= 0.79;	// penalty for dual-wielding 2h w/ 1h offhand
			}
			else if (itLeft->getRules()->isTwoHanded() == true)
				ret *= 0.93;		// modest penalty for dual-wielding 1h w/ 2h offhand
		}
	}
	else if (action.weapon == getItem(ST_LEFTHAND))
	{
		const BattleItem* const itRight (getItem(ST_RIGHTHAND));
		if (itRight != nullptr)
		{
			if (action.weapon->getRules()->isTwoHanded() == true)
			{
				if (itRight->getRules()->isTwoHanded() == true)
					ret *= 0.51;	// big penalty for dual-wielding 2x2h weapons
				else
					ret *= 0.79;	// penalty for dual-wielding 2h w/ 1h offhand
			}
			else if (itRight->getRules()->isTwoHanded() == true)
				ret *= 0.93;		// modest penalty for dual-wielding 1h w/ 2h offhand
		}
	} // no penalty for dual-wielding 2x1h weapons

	if (_battleGame->playerPanicHandled() == false) // berserk xCom agents get lowered accuracy.
		ret *= 0.68;

	//Log(LOG_INFO) << "BattleUnit::getAccuracy() ret = " << ret;
	return ret;
}

/**
 * Calculates accuracy decrease as a result of wounds.
 * @note Takes health and fatal wounds into account.
 * Formula = accuracyStat * woundsPenalty(% health) * critWoundsPenalty (-10%/wound)
 * @param item - pointer to a BattleItem (default nullptr)
 * @return, modifier
 */
double BattleUnit::getAccuracyModifier(const BattleItem* const item) const
{
	double ret (static_cast<double>(_health) / static_cast<double>(_stats.health));

	int wounds (_fatalWounds[BODYPART_HEAD] * 2);
	if (item != nullptr)
	{
		if (item->getRules()->isTwoHanded() == true)
			wounds += _fatalWounds[BODYPART_RIGHTARM] + _fatalWounds[BODYPART_LEFTARM];
		else
		{
			if (item == getItem(ST_RIGHTHAND))
				wounds += _fatalWounds[BODYPART_RIGHTARM];
			else
				wounds += _fatalWounds[BODYPART_LEFTARM];
		}
	}

	ret *= std::max(0., 1. - 0.1 * static_cast<double>(wounds));
	ret = std::max(0.1, ret);
	//Log(LOG_INFO) << "BattleUnit::getAccuracyModifier() ret = " << ret;
	return ret;
}

/**
 * Sets the armor value of a certain armor side.
 * @param armor	- amount of armor
 * @param side	- the side of the armor
 */
void BattleUnit::setArmor(
		int armor,
		UnitSide side)
{
	if ((_armorHp[side] = armor) < 0) _armorHp[side] = 0;
}

/**
 * Gets the armor value of a certain armor side.
 * @param side - the side of the armor
 * @return, amount of armor
 */
int BattleUnit::getArmor(UnitSide side) const
{
	return _armorHp[side];
}

/**
 * Little formula that calculates initiative/reaction score.
 * @note Reactions Stat * Current Time Units / Max TUs
 * @param tuSpent - (default 0)
 * @return, reaction score aka INITIATIVE
 */
int BattleUnit::getInitiative(const int tuSpent) const
{
	double ret (static_cast<double>(
				_stats.reactions * (getTu() - tuSpent))
				/ static_cast<double>(_stats.tu));

	ret *= getAccuracyModifier();

	return static_cast<int>(std::ceil(ret));
}

/**
 * Prepares this BattleUnit for its turn unless it was Mind-controlled.
 * @note Mind-controlled units call this at the beginning of the opposing
 * faction's next turn.
 * @param preBattle - true to skip the full process (default false)
 */
void BattleUnit::prepareUnit(bool preBattle)
{
	//Log(LOG_INFO) << "BattleUnit::prepareUnit() id-" << _id;
//	bool debug = _id == 257;

	_dontReselect = false;

	_hostileUnitsThisTurn.clear();
	_motionPoints = 0;

	bool reverted;
	if (_faction != _originalFaction) // reverting from Mind Control at start of MC-ing faction's next turn
	{
		reverted = true;
		if ((_faction = _originalFaction) == FACTION_PLAYER)
			setAIState();
	}
	else
		reverted = false;
	//Log(LOG_INFO) << ". reverted= " << (int)reverted;

	bool isPanicked (false);
	if (preBattle == false) // Don't do damage or panic at start of tactical.
	{
		if (_fire != 0) --_fire;

		if ((_health -= getFatalsTotal()) < 1) // suffer from fatal wounds
		{
			_health = 0;
			setAIState(); // if unit is dead AI state disappears
			return;
		}

		if (_stunLevel != 0
			&& (_geoscapeSoldier != nullptr
				|| _unitRule->isMechanical() == false))
		{
			reduceStun(RNG::generate(1,3)); // recover stun
		}

		if (_status != STATUS_UNCONSCIOUS)
		{
			const int panicPct (100 - (getMorale() << 1u));
			//Log(LOG_INFO) << ". panicPct= " << panicPct;
			if (RNG::percent(panicPct) == true)
			{
				//Log(LOG_INFO) << ". . has Panicked";
				isPanicked = true;
				if (reverted == false) // stay STATUS_STANDING if just coming out of Mc. But init tu/stamina as if panicked.
				{
					if (RNG::percent(30) == true)
					{
						//Log(LOG_INFO) << ". . . set Status_Berserk";
						_status = STATUS_BERSERK;	// shoot stuff.
					}
					else
					{
						//Log(LOG_INFO) << ". . . set Status_Panicking";
						_status = STATUS_PANICKING;	// panic is either flee or freeze - determined later
					}
				}
				//else Log(LOG_INFO) << ". . . but has reverted so won't panic";
			}
			else if (panicPct > 0 && _geoscapeSoldier != nullptr) // successfully avoided Panic
				++_expBravery;
		}
	}

	if (_status != STATUS_UNCONSCIOUS)
		prepTuEnergy(preBattle, isPanicked, reverted);
}

/**
 * Calculates and resets this BattleUnit's turn-units and energy.
 * @param preBattle		- true for pre-battle initialization (default false)
 * @param isPanicked	- true if unit has just panicked (default false)
 * @param reverted		- true if unit has just reverted from MC (default false)
 */
void BattleUnit::prepTuEnergy(
		bool preBattle,
		bool isPanicked,
		bool reverted)
{
	_tu = _stats.tu;

	const int overBurden (getCarriedWeight() - getStrength());
	if (overBurden > 0)
		_tu -= overBurden;

	if (_geoscapeSoldier != nullptr) // Each fatal wound to the left or right leg reduces a Soldier's TUs by 10%.
		_tu -= _tu * (getFatals(BODYPART_LEFTLEG) + getFatals(BODYPART_RIGHTLEG)) / 10;

	if (isPanicked == true)
	{
		if (reverted == false)
			_tu = _tu * RNG::generate(0,100) / 100; // this is how many TU the unit gets to run around/shoot with.
		else
			_tu = 0;	// if unit fails its panic-roll when reverting from MC (at
	}					// the beginning of opponent's turn) it simply loses its TU.

	if (isPanicked == false || reverted == false)
		_tu = std::max(_tu,
					   _battleGame->getBattleSave()->getDropTu());


	if (preBattle == false)				// no energy recovery needed at battle start
	{									// and none wanted for next stage battles:
		int energy (_stats.stamina);	// advanced Energy recovery ->
		if (_geoscapeSoldier != nullptr)
		{
			if (_kneeled == true)
				energy >>= 1u;
			else
				energy /= 3;
		}
		else // aLiens & Tanks. and Dogs ...
			energy = energy * _unitRule->getEnergyRecovery() / 100;

		energy = static_cast<int>(Round(static_cast<double>(energy) * getAccuracyModifier()));

		// Each fatal wound to the body reduces a Soldier's
		// energy recovery by 10% of his/her current energy.
		// note: only xCom Soldiers get fatal wounds, atm
		if (_geoscapeSoldier != nullptr)
			energy -= _energy * getFatals(BODYPART_TORSO) / 10;

		setEnergy(std::max(12, _energy + energy));
	}
}

/**
 * Changes morale with bounds check.
 * @param change - can be positive or negative
 */
void BattleUnit::moraleChange(int change)
{
	if (isMoralable() == true)
	{
		_morale += change;
		if		(_morale > 100)	_morale = 100;
		else if	(_morale <   0)	_morale =   0;
	}
}

/**
 * Sets this BattleUnit's dontReselect flag.
 * @param reselect - true to allow reselect (default true)
 */
void BattleUnit::setReselect(bool reselect)
{
	_dontReselect = (reselect == false);
}

/**
 * Gets this BattleUnit's dontReselect flag.
 * @return, true if reselect allowed
 */
bool BattleUnit::getReselect() const
{
	return (_dontReselect == false);
}

/**
 * Checks if this BattleUnit can be selected.
 * @#note Only conscious units belonging to the specified faction can be selected.
 * @param faction			- the faction to compare
 * @param checkReselect		- true to check the unit's reselectable flag (default false)
 * @param checkInventory	- true to check if the unit has no inventory (default false)
 * @return, true if the unit can be selected, false otherwise
 */
bool BattleUnit::isSelectable(
		UnitFaction faction,
		bool checkReselect,
		bool checkInventory) const
{
	return _faction == faction
		&& (_status == STATUS_STANDING || _status == STATUS_PANICKING || _status == STATUS_BERSERK)
		&& (checkReselect == false || _dontReselect == false)
		&& (checkInventory == false || canInventory() == true);
}

/**
 * Sets the amount of turns this BattleUnit is on fire.
 * @param fire - amount of turns this unit will be on fire (no fire 0)
 */
void BattleUnit::setUnitFire(int fire)
{
	if (_specab != SPECAB_BURN)
		_fire = fire;
}

/**
 * Gets the amount of turns this BattleUnit is on fire.
 * @return, amount of turns this unit will be on fire (0 - no fire)
 */
int BattleUnit::getUnitFire() const
{
	return _fire;
}

/**
 * Gives this BattleUnit damage from personal fire.
 */
void BattleUnit::hitUnitFire()
{
	if (_fire != 0)
	{
		float vulnr;

		if ((vulnr = _armor->getDamageModifier(DT_SMOKE)) > 0.f)
			takeDamage(
					Position(0,0,0),
					static_cast<int>(3.f * vulnr),
					DT_SMOKE, // -> DT_STUN
					true);

		if ((vulnr = _armor->getDamageModifier(DT_IN)) > 0.f)
			takeDamage(
					Position(0,0,0),
					static_cast<int>(RNG::generate(2.f,6.f) * vulnr),
					DT_IN,
					true);
	}
}

/**
 * Gets the pointer to the vector of inventory items.
 * @return, pointer to a vector of pointers to this BattleUnit's battle items
 */
std::vector<BattleItem*>* BattleUnit::getInventory()
{
	return &_inventory;
}

/**
 * Lets the AI do its thing.
 * @note Called by BattlescapeGame::handleUnitAI().
 * @param action - current AI action
 */
void BattleUnit::thinkAi(BattleAction* const action)
{
	//bool debug = Options::traceAI;
	//if (debug) {
	//	Log(LOG_INFO) << "";
	//	Log(LOG_INFO) << "BattleUnit::think() id-" << _id;
	//	Log(LOG_INFO) << ". checkReload()";
	//}

	if (checkReload() == true)
		_cacheInvalid = true; // <- reloading a weapon could switch a unit's preferred weapon-hand.

	//if (debug) Log(LOG_INFO) << ". _unitAIState->thinkOnce()";
	_unitAIState->thinkOnce(action);

	//if (debug) {
	//	Log(LOG_INFO) << "BattleUnit::think() EXIT";
	//	Log(LOG_INFO) << "";
	//}
}

/**
 * Sets this BattleUnit's current AI-state.
 * @param aiState - pointer to AI-state (default nullptr)
 */
void BattleUnit::setAIState(BattleAIState* const aiState)
{
	if (_unitAIState != nullptr)
	{
//		_unitAIState->exit();
		delete _unitAIState;
	}
	_unitAIState = aiState;
//	_unitAIState->enter();
}

/**
 * Gets this BattleUnit's current AI-state.
 * @return, pointer to AI-state
 */
BattleAIState* BattleUnit::getAIState() const
{
	return _unitAIState;
}

/**
 * Sets the Tile that this BattleUnit occupies.
 * @param tile		- pointer to a tile (default nullptr)
 * @param tileBelow	- pointer to any tile-below (default nullptr)
 */
void BattleUnit::setUnitTile(
		Tile* const tile,
		const Tile* const tileBelow)
{
	if ((_tile = tile) != nullptr)
	{
		switch (_status) // if moving or revived ->
		{
			case STATUS_WALKING:
				if (_mType == MT_FLY
					&& _tile->solidFloor(tileBelow) == false)
				{
					_status = STATUS_FLYING;
					_floating = true;
				}
				break;

			case STATUS_FLYING:
				if (_dirVertical == Pathfinding::DIR_VERT_NONE // <- wait. What if unit went down onto solid floor.
					&& _tile->solidFloor(tileBelow) == true)
				{
					_status = STATUS_WALKING;
					_floating = false;
				}
				break;

			case STATUS_UNCONSCIOUS: // revived.
				_floating = _mType == MT_FLY
						 && _tile->solidFloor(tileBelow) == false;
		}
	}
	else
		_floating = false;
}

/**
 * Gets this BattleUnit's current Tile.
 * @note A unit that is Dead or Unconscious (or Latent) returns 'nullptr'.
 * @return, pointer to the tile or nullptr
 */
Tile* BattleUnit::getUnitTile() const
{
	return _tile;
}

/**
 * Checks if there's an item in the specified inventory position.
 * @param inRule	- pointer to RuleInventory
 * @param x			- x-position in section (default 0)
 * @param y			- y-position in section (default 0)
 * @return, pointer to BattleItem or nullptr if none
 */
BattleItem* BattleUnit::getItem(
		const RuleInventory* const inRule,
		int x,
		int y) const
{
	if (inRule->getCategory() != IC_GROUND) // Soldier items
	{
		for (std::vector<BattleItem*>::const_iterator
				i = _inventory.begin();
				i != _inventory.end();
				++i)
		{
			if ((*i)->getInventorySection() == inRule
				&& (*i)->occupiesSlot(x,y) == true)
			{
				return *i;
			}
		}
	}
	else if (_tile != nullptr) // Ground items
	{
		for (std::vector<BattleItem*>::const_iterator
				i = _tile->getInventory()->begin();
				i != _tile->getInventory()->end();
				++i)
		{
			if ((*i)->occupiesSlot(x,y) == true)
				return *i;
		}
	}
	return nullptr;
}

/**
 * Checks if there's an item in the specified inventory position.
 * @note Used only in BattlescapeGenerator::placeLayout()
 * @param type	- reference to an inventory-type
 * @param x		- x-position in section (default 0)
 * @param y		- y-position in section (default 0)
 * @return, pointer to BattleItem or nullptr if none
 */
BattleItem* BattleUnit::getItem(
		const std::string& type,
		int x,
		int y) const
{
	if (type != "STR_GROUND") // Soldier items
	{
		for (std::vector<BattleItem*>::const_iterator
				i = _inventory.begin();
				i != _inventory.end();
				++i)
		{
			if ((*i)->getInventorySection() != nullptr // not load.
				&& (*i)->getInventorySection()->getInventoryType() == type
				&& (*i)->occupiesSlot(x,y) == true)
			{
				return *i;
			}
		}
	}
	else if (_tile != nullptr) // Ground items
	{
		for (std::vector<BattleItem*>::const_iterator
				i = _tile->getInventory()->begin();
				i != _tile->getInventory()->end();
				++i)
		{
			if ((*i)->occupiesSlot(x,y) == true) //&& (*i)->getInventorySection() != nullptr
				return *i;
		}
	}
	return nullptr;
}

/**
 * Checks if there's an item in the specified inventory position.
 * @param section	- an InventorySection (RuleInventory.h)
 * @param x			- x-position in section (default 0)
 * @param y			- y-position in section (default 0)
 * @return, pointer to BattleItem or nullptr if none
 */
BattleItem* BattleUnit::getItem(
		InventorySection section,
		int x,
		int y) const
{
	if (section != ST_GROUND)
	{
		for (std::vector<BattleItem*>::const_iterator
				i = _inventory.begin();
				i != _inventory.end();
				++i)
		{
			if ((*i)->getInventorySection() != nullptr // not load.
				&& (*i)->getInventorySection()->getSectionType() == section
				&& (*i)->occupiesSlot(x,y) == true)
			{
				return *i;
			}
		}
	}
	else if (_tile != nullptr) // Ground items
	{
		for (std::vector<BattleItem*>::const_iterator
				i = _tile->getInventory()->begin();
				i != _tile->getInventory()->end();
				++i)
		{
			if ((*i)->occupiesSlot(x,y) == true) //&& (*i)->getInventorySection() != nullptr
				return *i;
		}
	}
	return nullptr;
}

/**
 * Sets this BattleUnit's active-hand.
 * @ note This is used for (a) sprite drawing when dual-wielding, and (b)
 * choosing a weapon during faction-player RF, TileEngine::reactionShot().
 * @param hand - the ActiveHand (BattleUnit.h)
 */
void BattleUnit::setActiveHand(ActiveHand hand)
{
/*	bool debug;
	if (_id == 1000013) debug = true;
	else debug = false;
	if (debug)
		Log(LOG_INFO) << "setActiveHand current= " << (int)_activeHand; */

	if (_activeHand != hand)
	{
		_activeHand = hand;
		_cacheInvalid = true;
		//if (debug) Log(LOG_INFO) << ". switch= " << (int)_activeHand;
	}
}

/**
 * Gets this BattleUnit's active-hand.
 * @note Must have an item in that hand else switch to other hand or use
 * righthand by default. Ergo this effectively sets Active-hand.
 * @return, the ActiveHand (BattleUnit.h)
 */
ActiveHand BattleUnit::getActiveHand()
{
/*	bool debug;
	if (_id == 1000013) debug = true;
	else debug = false;
	if (debug)
		Log(LOG_INFO) << "getActiveHand current= " << (int)_activeHand; */

	switch (_activeHand)
	{
		case AH_RIGHT:
			if (getItem(ST_RIGHTHAND) != nullptr)
			{
				//if (debug) Log(LOG_INFO) << ". ret= " << (int)_activeHand;
				return AH_RIGHT;
			}
			break;

		case AH_LEFT:
			if (getItem(ST_LEFTHAND) != nullptr)
			{
				//if (debug) Log(LOG_INFO) << ". ret= " << (int)_activeHand;
				return AH_LEFT;
			}
	}

	if (getItem(ST_RIGHTHAND) != nullptr)
	{
		//if (debug) Log(LOG_INFO) << ". set= " << (int)AH_RIGHT;
		_cacheInvalid = true;
		return (_activeHand = AH_RIGHT);
	}

	if (getItem(ST_LEFTHAND) != nullptr)
	{
		//if (debug) Log(LOG_INFO) << ". set= " << (int)AH_LEFT;
		_cacheInvalid = true;
		return (_activeHand = AH_LEFT);
	}

	return AH_NONE;
}

/**
 * Gets the main-hand-weapon of this BattleUnit.
 * @note Also sets an active-hand.
 * @param quickest	- true to choose the quickest weapon (default false)
 * @param inclMelee	- true to include check for melee-weapon (default true)
 * @param checkFist	- true to include a check for the universal-fist (default false)
 *					  false to bypass Fist or to engage handleUnitAI() -> pickupItem().
 * @return, pointer to a BattleItem or nullptr
 */
BattleItem* BattleUnit::getMainHandWeapon(
		bool quickest,
		bool inclMelee,
		bool checkFist)
{
//	if (_id == 1000013)
//		Log(LOG_INFO) << "getMainHandWeapon";

	//Log(LOG_INFO) << "BattleUnit::getMainHandWeapon()";
	BattleItem
		* const rtWeapon (getItem(ST_RIGHTHAND)),	// TODO: Prioritize blasters (the AI currently relies
		* const ltWeapon (getItem(ST_LEFTHAND));	// on any blaster being in the aLien RH-slot, iirc).
													// ooth, it relies on any blaster having more TU than another held weapon;
													// co-relatively that's how the AI switches to a (faster) pistol for reaction-fire.
	//if (rtWeapon != nullptr) Log(LOG_INFO) << "right weapon " << rtWeapon->getRules()->getType();
	//if (ltWeapon != nullptr) Log(LOG_INFO) << "left weapon " << ltWeapon->getRules()->getType();

	const bool
		hasRT (rtWeapon != nullptr
				&& ((inclMelee == true
						&& rtWeapon->getRules()->getBattleType() == BT_MELEE)
					|| (rtWeapon->getRules()->getBattleType() == BT_FIREARM
						&& rtWeapon->getAmmoItem() != nullptr))),
		hasLT (ltWeapon != nullptr
				&& ((inclMelee == true
						&& ltWeapon->getRules()->getBattleType() == BT_MELEE)
					|| (ltWeapon->getRules()->getBattleType() == BT_FIREARM
						&& ltWeapon->getAmmoItem() != nullptr)));
	//Log(LOG_INFO) << ". hasRT = " << hasRT;
	//Log(LOG_INFO) << ". hasLT = " << hasLT;

	if (!hasRT && !hasLT)
	{
		setActiveHand(AH_NONE);
		if (checkFist == true) return _fist;

		return nullptr;
	}

	if (hasRT && !hasLT)
	{
		setActiveHand(AH_RIGHT);
		return rtWeapon;
	}

	if (!hasRT && hasLT)
	{
		setActiveHand(AH_LEFT);
		return ltWeapon;
	}

	//Log(LOG_INFO) << ". . hasRT & hasLT VALID";

	const RuleItem* itRule (rtWeapon->getRules());
	int rtTU = itRule->getSnapTu();
	if (rtTU == 0)
		if ((rtTU = itRule->getAutoTu()) == 0)
			if ((rtTU = itRule->getAimedTu()) == 0)
				if ((rtTU = itRule->getLaunchTu()) == 0)
					rtTU = itRule->getMeleeTu();

	itRule = ltWeapon->getRules();
	int ltTU = itRule->getSnapTu();
	if (ltTU == 0)
		if ((ltTU = itRule->getAutoTu()) == 0)
			if ((ltTU = itRule->getAimedTu()) == 0)
				if ((ltTU = itRule->getLaunchTu()) == 0)
					ltTU = itRule->getMeleeTu();
	// note: Should probly account for 'noReaction' weapons ... before reaction-algorhithm fizzles.

	//Log(LOG_INFO) << ". . rtTU = " << rtTU;
	//Log(LOG_INFO) << ". . ltTU = " << ltTU;

	if (quickest == true) // rtTU && ltTU
	{
		if (rtTU <= ltTU)
		{
			setActiveHand(AH_RIGHT);
			return rtWeapon;
		}

		setActiveHand(AH_LEFT);
		return ltWeapon;
	}

	if (rtTU >= ltTU)
	{
		setActiveHand(AH_RIGHT);
		return rtWeapon;
	}

	setActiveHand(AH_LEFT);
	return ltWeapon;
}

/**
 * Gets a grenade.
 * @note Called by AI/player-panic.
 * @return, pointer to a grenade (nullptr if none)
 */
BattleItem* BattleUnit::getGrenade() const
{
	BattleItem* grenade;
	if ((grenade = getItem(ST_RIGHTHAND)) != nullptr
		&& grenade->getRules()->getBattleType() == BT_GRENADE
		&& isGrenadeSuitable(grenade) == true)
	{
		return grenade;
	}

	if ((grenade = getItem(ST_LEFTHAND)) != nullptr
		&& grenade->getRules()->getBattleType() == BT_GRENADE
		&& isGrenadeSuitable(grenade) == true)
	{
		return grenade;
	}

	std::vector<BattleItem*> grenades;
	for (std::vector<BattleItem*>::const_iterator
			i = _inventory.begin();
			i != _inventory.end();
			++i)
	{
		if ((*i)->getRules()->getBattleType() == BT_GRENADE
			&& isGrenadeSuitable(*i) == true)
		{
			grenades.push_back(*i);
		}
	}

	if (grenades.empty() == false)
		return grenades[RNG::pick(grenades.size())];

	return nullptr;
}

/**
 * Gets if a grenade is suitable for AI/panic usage.
 * @return, true if so
 */
bool BattleUnit::isGrenadeSuitable(const BattleItem* const grenade) const // private.
{
	if (grenade->getRules()->getDamageType() != DT_SMOKE
		|| _battleGame->playerPanicHandled() == false)
	{
		return true; // -> is player-panic allow smoke grenades.
	}

	return false; // -> is AI only !smoke grenades.
}

/**
 * Gets this BattleUnit's melee weapon if any.
 * @return, pointer to melee weapon (nullptr if none)
 */
BattleItem* BattleUnit::getMeleeWeapon() const
{
	BattleItem* melee (getItem("STR_RIGHT_HAND"));
	if (melee != nullptr && melee->getRules()->getBattleType() == BT_MELEE)
		return melee;

	melee = getItem("STR_LEFT_HAND");
	if (melee != nullptr && melee->getRules()->getBattleType() == BT_MELEE)
		return melee;

	return _fist;

//	melee = getSpecialWeapon(BT_MELEE);
//	if (melee) return melee;
//	return nullptr;
}

/**
 * Gets this BattleUnit's ranged weapon if any.
 * @param quickest - true if reaction fire
 * @return, pointer to weapon (nullptr if none)
 */
BattleItem* BattleUnit::getRangedWeapon(bool quickest) const
{
	BattleItem
		* const rtWeapon (getItem(ST_RIGHTHAND)),
		* const ltWeapon (getItem(ST_LEFTHAND));
	//if (rtWeapon != nullptr) Log(LOG_INFO) << "right weapon " << rtWeapon->getRules()->getType();
	//if (ltWeapon != nullptr) Log(LOG_INFO) << "left weapon " << ltWeapon->getRules()->getType();

	const bool
		hasRT (rtWeapon != nullptr
				&& rtWeapon->getRules()->getBattleType() == BT_FIREARM
				&& rtWeapon->getAmmoItem() != nullptr),
		hasLT (ltWeapon != nullptr
				&& ltWeapon->getRules()->getBattleType() == BT_FIREARM
				&& ltWeapon->getAmmoItem() != nullptr);

	if (!hasRT && !hasLT)
	{
//		setActiveHand(AH_NONE);
		return nullptr;
	}

	if (hasRT && !hasLT)
	{
//		setActiveHand(AH_RIGHT);
		return rtWeapon;
	}

	if (!hasRT && hasLT)
	{
//		setActiveHand(AH_LEFT);
		return ltWeapon;
	}

	const RuleItem* itRule (rtWeapon->getRules());
	int rtTU = itRule->getSnapTu();
	if (rtTU == 0)
		if ((rtTU = itRule->getAutoTu()) == 0)
			if ((rtTU = itRule->getAimedTu()) == 0)
				if ((rtTU = itRule->getLaunchTu()) == 0)
					rtTU = itRule->getMeleeTu();

	itRule = ltWeapon->getRules();
	int ltTU = itRule->getSnapTu();
	if (ltTU == 0)
		if ((ltTU = itRule->getAutoTu()) == 0)
			if ((ltTU = itRule->getAimedTu()) == 0)
				if ((ltTU = itRule->getLaunchTu()) == 0)
					ltTU = itRule->getMeleeTu();
	// note: Should probly account for 'noReaction' weapons ... before reaction-algorithm fizzles.

	if (quickest == true) // rtTU && ltTU
	{
		if (rtTU <= ltTU)
		{
//			setActiveHand(AH_RIGHT);
			return rtWeapon;
		}

//		setActiveHand(AH_LEFT);
		return ltWeapon;
	}

	if (rtTU >= ltTU)
	{
//		setActiveHand(AH_RIGHT);
		return rtWeapon;
	}

//	setActiveHand(AH_LEFT);
	return ltWeapon;
}

/**
 * Checks if this BattleUnit has ammo and if so reloads its weapon.
 * @note Used by the AI only - Player has much stricter reloading requirements
 * that are handled by the Inventory.
 * @return, true if unit loads its weapon
 */
bool BattleUnit::checkReload()
{
	BattleItem* weapon;
	if ((weapon = getItem(ST_RIGHTHAND)) == nullptr
		|| weapon->getRules()->getBattleType() != BT_FIREARM
		|| weapon->getAmmoItem() != nullptr)
	{
		if ((weapon = getItem(ST_LEFTHAND)) == nullptr
			|| weapon->getRules()->getBattleType() != BT_FIREARM
			|| weapon->getAmmoItem() != nullptr)
		{
			return false;
		}
	}

	const int tuReload (weapon->getRules()->getReloadTu());
	if (_tu >= tuReload)
	{
		for (std::vector<BattleItem*>::const_iterator
				i = getInventory()->begin();
				i != getInventory()->end();
				++i)
		{
			for (std::vector<std::string>::const_iterator
					j = weapon->getRules()->getCompatibleAmmo()->begin();
					j != weapon->getRules()->getCompatibleAmmo()->end();
					++j)
			{
				if (*j == (*i)->getRules()->getType())
				{
					weapon->setAmmoItem(*i);
					_tu -= tuReload;
					return true;
				}
			}
		}
	}
	return false;
}

/**
 * Checks if this BattleUnit is standing on a specified tile-type.
 * @param tileType - type of tile to check for (RuleItem.h)
 * @return, true if unit is currently conscious on @a tileType
 */
bool BattleUnit::isOnTiletype(TileType tileType) const
{
	return _tile != nullptr
		&& _tile->getMapData(O_FLOOR) != nullptr
		&& _tile->getMapData(O_FLOOR)->getTileType() == tileType;
}

/**
 * Gets this BattleUnit's height whether standing or kneeling.
 * @param floating - true to include floating value (default false)
 * @return, unit's height
 */
int BattleUnit::getHeight(bool floating) const
{
	if (_kneeled == true)
		return _kneelHeight;

	if (floating == true)
		return _standHeight + _floatHeight;

	return _standHeight;
}

/**
 * Gets this BattleUnit's Firing experience.
 * @return, firing xp
 */
int BattleUnit::getExpFiring() const
{
	return _expFiring;
}

/**
 * Gets a soldier's Throwing experience.
 * @return, throwing xp
 */
int BattleUnit::getExpThrowing() const
{
	return _expThrowing;
}

/**
 * Gets a soldier's Melee experience.
 * @return, melee xp
 */
int BattleUnit::getExpMelee() const
{
	return _expMelee;
}

/**
 * Gets a soldier's Reactions experience.
 * @return, reactions xp
 */
int BattleUnit::getExpReactions() const
{
	return _expReactions;
}

/**
 * Gets a soldier's Bravery experience.
 * @return, bravery xp
 */
int BattleUnit::getExpBravery() const
{
	return _expBravery;
}

/**
 * Gets a soldier's PsiSkill experience.
 * @return, psiskill xp
 */
int BattleUnit::getExpPsiSkill() const
{
	return _expPsiSkill;
}

/**
 * Gets a soldier's PsiStrength experience.
 * @return, psistrength xp
 */
int BattleUnit::getExpPsiStrength() const
{
	return _expPsiStrength;
}

/**
 * Adds one to the firing exp counter.
 */
void BattleUnit::addFiringExp()
{
//	if (_battleGame->getBattleSave()->getPacified() == false)
	++_expFiring;
}

/**
 * Adds one to the throwing exp counter.
 */
void BattleUnit::addThrowingExp()
{
//	if (_battleGame->getBattleSave()->getPacified() == false)
	++_expThrowing;
}

/**
 * Adds qty to the melee exp counter.
 * @param qty - amount to add (default 1)
 */
void BattleUnit::addMeleeExp(int qty)
{
//	if (_battleGame->getBattleSave()->getPacified() == false)
	_expMelee += qty;
}

/**
 * Adds one to the reaction exp counter.
 */
void BattleUnit::addReactionExp()
{
//	if (_battleGame->getBattleSave()->getPacified() == false)
	++_expReactions;
}

/**
 * Adds qty to the psiSkill exp counter.
 * @param qty - amount to add (default 1)
 */
void BattleUnit::addPsiSkillExp(int qty)
{
//	if (_battleGame->getBattleSave()->getPacified() == false)
	_expPsiSkill += qty;
}

/**
 * Adds qty to the psiStrength exp counter.
 * @param qty - amount to add (default 1)
 */
void BattleUnit::addPsiStrengthExp(int qty)
{
//	if (_battleGame->getBattleSave()->getPacified() == false)
	_expPsiStrength += qty;
}

/**
 * Calculates experience increases and days to spend in sickbay if wounded.
 * @param dead - true if dead or missing (default false)
 * @return, a vector of increases as ints
 */
std::vector<int> BattleUnit::postMissionProcedures(const bool dead)
{
	_geoscapeSoldier->postTactical(_takedowns);

	UnitStats* const stats (_geoscapeSoldier->getCurrentStats());
	if (dead == false && stats->health > _health)
	{
		const int recovery (stats->health - _health);
		_geoscapeSoldier->setRecovery(RNG::generate(
												(recovery + 1) >> 1u,
												 recovery));
	}

	static const size_t STATS (11u);
	std::vector<int> statIncs (STATS, 0);

	const UnitStats caps (_geoscapeSoldier->getRules()->getStatCaps());

	if (_expBravery != 0
		&& stats->bravery < caps.bravery)
	{
		if (_expBravery > RNG::generate(0,8))
		{
			stats->bravery += 10;
			statIncs[0u] = 10;
		}
	}

	int inc;
	if (_expFiring != 0
		&& stats->firing < caps.firing)
	{
		inc = improveStat(_expFiring);
		stats->firing += inc;
		statIncs[1u] = inc;

		// add a touch of reactions if good firing .....
		if (_expFiring - 2 > 0
			&& stats->reactions < caps.reactions)
		{
			_expReactions += _expFiring / 3;
		}
	}

	if (_expReactions != 0
		&& stats->reactions < caps.reactions)
	{
		inc = improveStat(_expReactions);
		stats->reactions += inc;
		statIncs[2u] = inc;
	}

	if (_expMelee != 0
		&& stats->melee < caps.melee)
	{
		inc = improveStat(_expMelee);
		stats->melee += inc;
		statIncs[3u] = inc;
	}

	if (_expPsiSkill != 0
		&& stats->psiSkill < caps.psiSkill)
	{
		inc = improveStat(_expPsiSkill);
		stats->psiSkill += inc;
		statIncs[4u] = inc;
	}

	if ((_expPsiStrength /= 3) != 0
		&& stats->psiStrength < caps.psiStrength)
	{
		inc = improveStat(_expPsiStrength);
		stats->psiStrength += inc;
		statIncs[5u] = inc;
	}

	if (_expThrowing != 0
		&& stats->throwing < caps.throwing)
	{
		inc = improveStat(_expThrowing);
		stats->throwing += inc;
		statIncs[6u] = inc;
	}


	const bool expPri (_expBravery != 0
					|| _expReactions != 0
					|| _expFiring != 0
					|| _expMelee != 0);

	if (expPri == true
		|| _expPsiSkill != 0
		|| _expPsiStrength != 0)
	{
		if (hasFirstTakedown() == true)
			_geoscapeSoldier->promoteRank();

		if (expPri == true)
		{
			int delta (caps.tu - stats->tu);
			if (delta > 0)
			{
				inc = RNG::generate(0, (delta / 10) + 2) - 1;
				stats->tu += inc;
				statIncs[7u] = inc;
			}

			delta = caps.health - stats->health;
			if (delta > 0)
			{
				inc = RNG::generate(0, (delta / 10) + 2) - 1;
				stats->health += inc;
				statIncs[8u] = inc;
			}

			delta = caps.strength - stats->strength;
			if (delta > 0)
			{
				inc = RNG::generate(0, (delta / 10) + 2) - 1;
				stats->strength += inc;
				statIncs[9u] = inc;
			}

			delta = caps.stamina - stats->stamina;
			if (delta > 0)
			{
				inc = RNG::generate(0, (delta / 10) + 2) - 1;
				stats->stamina += inc;
				statIncs[10u] = inc;
			}
		}
	}

	return statIncs;
}

/**
 * Converts an amount of experience to a stat increase.
 * @param exp - experience count from battle mission
 * @return, stat increase
 */
int BattleUnit::improveStat(int xp) const // private.
{
	int tier;

	if		(xp > 10) tier = 4;
	else if (xp >  5) tier = 3;
	else if (xp >  2) tier = 2;
	else			  tier = 1;

	return ((tier >> 1u) + RNG::generate(0, tier));
}

/**
 * Get this BattleUnit's minimap sprite index.
 * @note Used to display the unit on the MiniMap.
 * @return, the unit's ScanG-index
 */
int BattleUnit::getMiniMapSpriteIndex() const
{
	// ScanG sprite index:
	// 0-2   - Xcom soldier
	// 3-5   - Alien
	// 6-8   - Civilian
	// 9-11  - Item
	// 12-23 - Xcom HWP
	// 24-35 - Alien big terror unit (cyberdisk, etc)
	if (_status != STATUS_STANDING)
		return 9;

	switch (_faction)
	{
		case FACTION_HOSTILE:
			if (_armor->getSize() == 1)
				return 3;
			else
				return 24;

		case FACTION_NEUTRAL:
			return 6;

		default:
		case FACTION_PLAYER:
			if (_armor->getSize() == 1)
				return 0;
			else
				return 12;
	}
}

/**
 * Sets this BattleUnit's turret-type.
 * @param turretType - the TurretType (RuleItem.h)
 */
void BattleUnit::setTurretType(TurretType turretType)
{
	_turretType = turretType;
}

/**
 * Gets this BattleUnit's turret-type.
 * @return, the turret-type (RuleItem.h)
 */
TurretType BattleUnit::getTurretType() const
{
	return _turretType;
}

/**
 * Gets the total quantity of fatal wounds this BattleUnit has.
 * @return, quantity of fatals
 */
int BattleUnit::getFatalsTotal() const
{
	int ret (0);
	for (size_t
			i = 0;
			i != PARTS_BODY;
			++i)
	{
		ret += _fatalWounds[i];
	}
	return ret;
}

/**
 * Gets the quantity of fatal wounds for a specified body-part.
 * @param part - the body part in the range 0-5 (BattleUnit.h)
 * @return, fatal wounds @a part has
 */
int BattleUnit::getFatals(UnitBodyPart part) const
{
	return _fatalWounds[part];
}

/**
 * Heals a fatal wound of this BattleUnit.
 * @param part		- the body-part to heal (BattleUnit.h)
 * @param wounds	- the quantity of fatal wounds to heal
 * @param health	- the quantity of health to add
 */
void BattleUnit::heal(
		UnitBodyPart part,
		int wounds,
		int health)
{
	if (getFatals(part) != 0)
	{
		if ((_fatalWounds[part] -= wounds) < 0)
			_fatalWounds[part] = 0;

		if ((_health += health) > _stats.health)
			_health = _stats.health;

		moraleChange(health);
	}
}

/**
 * Restores morale of this BattleUnit. And kills w/ overdose.
 */
void BattleUnit::morphine()
{
	if (++_drugDose >= DOSE_LETHAL)
		_health = 0;
	else
	{
		_stunLevel += 7 + RNG::generate(0,6);
		const float healthPct (static_cast<float>(_health) / static_cast<float>(_stats.health));
		_morale = std::min(100,
						   _morale + 50 - static_cast<int>(30.f * healthPct));
	}

	if (_health == 0												// just died. Use death animations
		|| (isStunned() == true && _status != STATUS_UNCONSCIOUS))	// unless already unconscious.
	{
		_battleGame->checkCasualties(
								_battleGame->getTacticalAction()->weapon->getRules(),
								_battleGame->getTacticalAction()->actor);
	}
}

/**
 * Restores this BattleUnit's energy and reduces its stun-level.
 * @param energy	- the quantity of energy to add
 * @param stun		- the quantity of stun-level to recover
 * @return, true if unit regains consciousness
 */
bool BattleUnit::amphetamine(
		int energy,
		int stun)
{
	_energy += energy;
	if (_energy > _stats.stamina)
		_energy = _stats.stamina;

	return reduceStun(stun);
}

/**
 * Gets if this BattleUnit has overdosed on morphine.
 * @return, true if overdosed
 */
bool BattleUnit::getOverDose() const
{
	return (_drugDose >= DOSE_LETHAL);
}

/**
 * Gets this BattleUnit's motion-points for the motion-scanner.
 * @note More points is a larger blip on the scanner.
 * @return, motion points
 */
int BattleUnit::getMotionPoints() const
{
	return _motionPoints;
}

/**
 * Calculates arbitrary pre-battle TU and motion-points.
 * @sa keepWalking()
 */
void BattleUnit::preBattleMotion()
{
	_tu = static_cast<int>(ceil(
		  static_cast<double>(_stats.tu) * RNG::generate(0.,1.)));

	switch (_armor->getSize())
	{
		case 2:
			_motionPoints = ((_stats.tu - _tu) * 30) >> 2u;
			break;

		case 1:
			if (_standHeight > 16)
				_motionPoints = (_stats.tu - _tu);
			else
				_motionPoints = ((_stats.tu - _tu) * 3) >> 2u;
	}
}

/**
 * Gets this BattleUnit's armor.
 * @return, pointer to Armor
 */
const RuleArmor* BattleUnit::getArmor() const
{
	return _armor;
}

/**
 * Checks if this BattleUnit is wearing a PowerSuit.
 * @return, true if this unit is wearing a PowerSuit of some sort
 *
bool BattleUnit::hasPowerSuit() const
{
	std::string armorType = _armor->getType();

	if (   armorType == "STR_POWER_SUIT_UC"
		|| armorType == "STR_BLACK_ARMOR_UC"
		|| armorType == "STR_BLUE_ARMOR_UC"
		|| armorType == "STR_GREEN_ARMOR_UC"
		|| armorType == "STR_ORANGE_ARMOR_UC"
		|| armorType == "STR_PINK_ARMOR_UC"
		|| armorType == "STR_PURPLE_ARMOR_UC"
		|| armorType == "STR_RED_ARMOR_UC")
	{
		return true;
	}

	return false;
} */

/**
 * Checks if this BattleUnit is wearing a FlightSuit.
 * @return, true if this unit is wearing a FlightSuit of some sort
 *
bool BattleUnit::hasFlightSuit() const
{
	std::string armorType = _armor->getType();

	if (   armorType == "STR_FLYING_SUIT_UC"
		|| armorType == "STR_BLACKSUIT_ARMOR_UC"
		|| armorType == "STR_BLUESUIT_ARMOR_UC"
		|| armorType == "STR_GREENSUIT_ARMOR_UC"
		|| armorType == "STR_ORANGESUIT_ARMOR_UC"
		|| armorType == "STR_PINKSUIT_ARMOR_UC"
		|| armorType == "STR_PURPLESUIT_ARMOR_UC"
		|| armorType == "STR_REDSUIT_ARMOR_UC")
	{
		return true;
	}

	return false;
} */

/**
 * Gets a unit's name.
 * @note An aLien's name is the translation of its race and rank; hence the
 * language pointer needed.
 * @param lang		- pointer to Language (default nullptr)
 * @param debugId	- append unit-ID for debug purposes (default false)
 * @return, name of this BattleUnit
 */
std::wstring BattleUnit::getName(
		const Language* const lang,
		bool debugId) const
{
	if (_geoscapeSoldier == nullptr && lang != nullptr)
	{
		std::wstring ret;
		if (_type.find("STR_") != std::string::npos)
			ret = lang->getString(_type);
		else
			ret = lang->getString(_race);

		if (debugId == true)
		{
			std::wostringstream woststr;
			woststr << ret << L" " << _id;
			ret = woststr.str();
		}
		return ret;
	}

	return _name;
}

/**
 * Gets a pointer to this BattleUnit's stats.
 * @note xCom Soldiers return their unique statistics - aLiens & Units return
 * rule statistics.
 * @return, pointer to UnitStats
 */
const UnitStats* BattleUnit::getBattleStats() const
{
	return &_stats;
}

/**
 * Gets this BattleUnit's stand-height.
 * @return, this unit's height in voxels when standing
 */
int BattleUnit::getStandHeight() const
{
	return _standHeight;
}

/**
 * Gets this BattleUnit's kneel-height.
 * @return, this unit's height in voxels when kneeling
 */
int BattleUnit::getKneelHeight() const
{
	return _kneelHeight;
}

/**
 * Gets this BattleUnit's float-elevation.
 * @return, this unit's elevation over the ground in voxels when floating or flying
 */
int BattleUnit::getFloatHeight() const
{
	return _floatHeight;
}

/**
 * Gets this BattleUnit's LOFT-id.
 * @note This is one slice only as it is repeated over the entire height of the
 * unit - each tile has only one LOFT. Also, in practice each layer is doubled
 * so that each layer is 2 voxels in height.
 * @param layer - an entry in this BattleUnit's LOFT set (default 0)
 * @return, this unit's Line of Fire Template id
 */
size_t BattleUnit::getLoft(size_t layer) const
{
	return _loftSet.at(layer);
}

/**
 * Gets this BattleUnit's value.
 * @note Used for score at debriefing.
 * @return, value score
 */
int BattleUnit::getValue() const
{
	return _value;
}

/**
 * Gets this BattleUnit's death-sound.
 * @return, death sound ID
 */
int BattleUnit::getDeathSound() const
{
	return _deathSound;
}

/**
 * Gets this BattleUnit's move-sound.
 * @return, move sound ID
 */
int BattleUnit::getMoveSound() const
{
	return _moveSound;
}

/**
 * Gets whether this BattleUnit can be affected by fatal wounds.
 * @note Normally only soldiers are affected by fatal wounds.
 * @return, true if unit can be affected by fatal wounds
 */
bool BattleUnit::isWoundable() const
{
	return _status != STATUS_LATENT
		&& _status != STATUS_LATENT_START
		&& _status != STATUS_DEAD
		&& (_geoscapeSoldier != nullptr
			|| (Options::battleAlienBleeding == true
				&& _unitRule->isMechanical() == false
				&& _isZombie == false));
}

/**
 * Gets whether this BattleUnit can be affected by morale loss.
 * @return, true if unit can be affected by morale changes
 */
bool BattleUnit::isMoralable() const
{
	return _status != STATUS_LATENT
		&& _status != STATUS_LATENT_START
		&& _status != STATUS_DEAD
		&& _status != STATUS_UNCONSCIOUS
		&& (_geoscapeSoldier != nullptr
			|| (_unitRule->isMechanical() == false
				&& _isZombie == false));
}

/**
 * Gets whether this BattleUnit can be accessed with the Medikit.
 * @return, true if unit can be treated
 */
bool BattleUnit::isHealable() const
{
	return _status != STATUS_LATENT
		&& _status != STATUS_LATENT_START
		&& _status != STATUS_DEAD
		&& (_geoscapeSoldier != nullptr
			|| (_unitRule->isMechanical() == false
				&& _isZombie == false));
}

/**
 * Gets whether this BattleUnit can be revived.
 * @return, true if unit can be revived
 */
bool BattleUnit::isRevivable() const
{
	return _status == STATUS_UNCONSCIOUS
		&& (_geoscapeSoldier != nullptr
			|| (_unitRule->isMechanical() == false
				&& _armor->getSize() == 1
				&& _isZombie == false));
}

/**
 * Gets the number of turns an AI unit will remember a player-unit's position.
 * @return, intelligence
 */
int BattleUnit::getIntelligence() const
{
	return _intelligence;
}

/**
 * Gets this BattleUnit's aggression-rating for use by the AI.
 * @return, aggression
 */
int BattleUnit::getAggression() const
{
	return _aggression;
}

/**
 * Gets this BattleUnit's special-ability.
 * @return, SpecialAbility (RuleUnit.h)
 */
SpecialAbility BattleUnit::getSpecialAbility() const
{
	return _specab;
}

/**
 * Sets this BattleUnit's special-ability.
 * @param specab - SpecialAbility (RuleUnit.h)
 */
void BattleUnit::setSpecialAbility(const SpecialAbility specab)
{
	_specab = specab;
}

/**
 * Gets unit-type that is spawned when this one dies.
 * @return, special spawn unit type (ie. ZOMBIES!!!)
 */
std::string BattleUnit::getSpawnType() const
{
	return _spawnType;
}

/**
 * Sets a unit-type that is spawned when this one dies.
 * @param spawnType - reference to the special unit type
 */
void BattleUnit::setSpawnUnit(const std::string& spawnType)
{
	_spawnType = spawnType;
}

/**
 * Adds a takedown to the counter.
 */
void BattleUnit::addTakedown()
{
	++_takedowns;
}

/**
 * Gets the quantity of kills/stuns the BattleUnit currently has.
 * @return, quantity of takedowns
 */
int BattleUnit::getTakedowns() const
{
	return _takedowns;
}

/**
 * Gets if this is a Rookie and has made his/her first takedown.
 * @return, true if rookie has at least one kill or stun vs. Hostile
 */
bool BattleUnit::hasFirstTakedown() const
{
	return _rankInt == 0 && _takedowns != 0;
}

/**
 * Gets if this BattleUnit is in the awkward phase between getting killed or
 * stunned and the end of its collapse-sequence.
 * @return, true if about to die
 */
bool BattleUnit::getAboutToCollapse() const
{
	return _aboutToCollapse;
}

/**
 * Sets health to 0 and status dead - calls putDown() just to be sure.
 * @note Used when getting zombified, etc.
 */
void BattleUnit::instaKill()
{
	_health = 0;
	_status = STATUS_DEAD;

	putDown();
}

/**
 * Sets this BattleUnit's parameters as down - collapsed/ unconscious/ dead.
 */
void BattleUnit::putDown()
{
	if (_unitAIState != nullptr)
	{
		switch (_status)
		{
			case STATUS_DEAD: setAIState(); break;
			case STATUS_UNCONSCIOUS: _unitAIState->resetAI();
		}
	}

	_faction = _originalFaction;
	_turnsExposed = -1;	// don't risk aggro per the AI

	if (_spawnType.empty() == true) // else convertUnit() will take care of it.
		_visible = false;

	_dontReselect =
	_hasBeenStunned = true;

	_tu =
	_energy = 0;

	_kneeled = // don't get hunkerdown bonus against HE detonations
	_dashing =
	_aboutToCollapse =
	_hasCried = false;

	_hostileUnits.clear();
	_hostileUnitsThisTurn.clear();

	// clear this unit from all other BattleUnit's '_hostileUnits' & '_hostileUnitsThisTurn' vectors
	if (_battleGame != nullptr) // check if death by prebattle hidden/power-source explosion.
	{
		for (std::vector<BattleUnit*>::const_iterator
				i = _battleGame->getBattleSave()->getUnits()->begin();
				i != _battleGame->getBattleSave()->getUnits()->end();
				++i)
		{
			for (std::vector<BattleUnit*>::const_iterator
					j = (*i)->getHostileUnits().begin();
					j != (*i)->getHostileUnits().end();
					++j)
			{
				if (*j == this)
				{
					(*i)->getHostileUnits().erase(j);
					break;
				}
			}

			for (std::vector<BattleUnit*>::const_iterator
					j = (*i)->getHostileUnitsThisTurn().begin();
					j != (*i)->getHostileUnitsThisTurn().end();
					++j)
			{
				if (*j == this)
				{
					(*i)->getHostileUnitsThisTurn().erase(j);
					break;
				}
			}
		}
	}


	// These don't seem to affect anything:
	// ... but they could matter if a unit is later revived.
//	_floating = false;
//	_stopShot = false;
//	_takenExpl = false;
//	_takenFire = false;
//	_diedByFire = false;
	// etc.
}

/**
 * Gets sound to play when unit aggros.
 * @return, aggro sound
 */
int BattleUnit::getAggroSound() const
{
	return _aggroSound;
}

/**
 * Gets the faction this BattleUnit was killed by.
 * @return, UnitFaction (BattleUnit.h)
 */
UnitFaction BattleUnit::killerFaction() const
{
	return _killerFaction;
}

/**
 * Sets the faction this BattleUnit was killed by.
 * @param faction - UnitFaction (BattleUnit.h)
 */
void BattleUnit::killerFaction(UnitFaction faction)
{
	_killerFaction = faction;
}

/**
 * Sets a BattleUnit to charge towards.
 * @param chargeTarget - pointer to a BattleUnit (default nullptr)
 */
void BattleUnit::setChargeTarget(BattleUnit* const chargeTarget)
{
	_charging = chargeTarget;
}

/**
 * Gets the unit that this BattleUnit is charging towards.
 * @return, pointer to a BattleUnit
 */
BattleUnit* BattleUnit::getChargeTarget() const
{
	return _charging;
}

/**
 * Gets this BattleUnit's carried weight in strength-units.
 * @param dragItem - item to ignore
 * @return, weight
 */
int BattleUnit::getCarriedWeight(const BattleItem* const dragItem) const
{
	int weight (_armor->getWeight());
	for (std::vector<BattleItem*>::const_iterator
			i = _inventory.begin();
			i != _inventory.end();
			++i)
	{
		if (*i != dragItem)
		{
			weight += (*i)->getRules()->getWeight();
			if ((*i)->getAmmoItem() != nullptr && (*i)->getAmmoItem() != *i)
				weight += (*i)->getAmmoItem()->getRules()->getWeight();
		}
	}
	return std::max(0, weight);
}

/**
 * Sets how long since this BattleUnit was last exposed to a Hostile unit.
 * @note Use -1 for NOT exposed. Aliens are always exposed.
 * @param turns - turns this unit has been exposed (default 0)
 */
void BattleUnit::setExposed(int turns)
{
	_turnsExposed = turns;
	//Log(LOG_INFO) << "bu:setExposed() id " << _id << " -> " << _turnsExposed;
}

/**
 * Gets how long since this BattleUnit was exposed.
 * @return, turns this unit has been exposed
 */
int BattleUnit::getExposed() const
{
	return _turnsExposed;
}

/**
 * Invalidates this BattleUnit's sprite-cache.
 * @note Call after copying object :(
 */
void BattleUnit::invalidateCache()
{
	for (size_t
			i = 0u;
			i != PARTS_ARMOR;
			++i)
	{
		_cache[i] = nullptr;
	}
	_cacheInvalid = true;
}

/**
 * Sets the numeric version of this BattleUnit's rank.
 * @param rankInt - unit rank (0 = xCom lowest/aLien highest) TODO: straighten that out ...
 */
void BattleUnit::setRankInt(int rankInt)
{
	_rankInt = rankInt;
}

/**
 * Gets the numeric version of this BattleUnit's rank.
 * @return, unit rank (0 = xCom lowest/aLien highest) TODO: straighten that out ...
 */
int BattleUnit::getRankInt() const
{
	return _rankInt;
}

/**
 * Derives the numeric unit-rank from this BattleUnit's string-rank.
 * @note This is for xCom-soldier units only - alien-ranks are inverted.
 */
void BattleUnit::deriveRank()
{
	switch (_geoscapeSoldier->getRank())
	{
		default:
		case RANK_ROOKIE:		_rankInt = 0; break;
		case RANK_SQUADDIE:		_rankInt = 1; break;
		case RANK_SERGEANT:		_rankInt = 2; break;
		case RANK_CAPTAIN:		_rankInt = 3; break;
		case RANK_COLONEL:		_rankInt = 4; break;
		case RANK_COMMANDER:	_rankInt = 5;
	}
}

/**
 * Checks if a specified Position is inside this BattleUnit's facing-quadrant.
 * @note Using maths!
 * @param pos - reference to the position to check against
 * @return, whatever the maths decide
 */
bool BattleUnit::checkViewSector(const Position& pos) const
{
	int dir;
	switch (_turretType)
	{
		case TRT_NONE: dir = _dir; break;
		default:
//			if (Options::battleStrafe == true)
			dir = _dirTurret;
//			else dir = _dir;
	}

	const int
		offset_x (pos.x - _pos.x),
		offset_y (pos.y - _pos.y);
	int
		dx,dy;
	const int unitSize (_armor->getSize());
	for (int // Check view-cone from each of the unit's quadrants.
			x = 0;
			x != unitSize;
			++x)
	{
		dx = offset_x - x;
		for (int
				y = 0;
				y != unitSize;
				++y)
		{
			dy = offset_y - y;

			switch (dir)
			{
				case 1: if (dx > -1 && dy <  1) return true; break;
				case 3: if (dx > -1 && dy > -1) return true; break;
				case 5: if (dx <  1 && dy > -1) return true; break;
				case 7: if (dx <  1 && dy <  1) return true; break;

				case 0: if (dx + dy <  1 && dy - dx <  1) return true; break;
				case 2: if (dx + dy > -1 && dy - dx <  1) return true; break;
				case 4: if (dx + dy > -1 && dy - dx > -1) return true; break;
				case 6: if (dx + dy <  1 && dy - dx > -1) return true;
			}
		}
	}
	return false;
}

/**
 * Adjusts this BattleUnit's stats according to the current difficulty-level
 * setting (used by aLiens only).
 * @param diff	- the difficulty-level
 * @param month	- the quantity of months that have been played
 */
void BattleUnit::adjustStats(
		const DifficultyLevel diff,
		const int month)
{
	_stats.tu			+= 4 * diff * _stats.tu				/ 100;
	_stats.stamina		+= 4 * diff * _stats.stamina		/ 100;
	_stats.reactions	+= 6 * diff * _stats.reactions		/ 100;
	_stats.firing		+= 6 * diff * _stats.firing			/ 100;
	_stats.throwing		+= 4 * diff * _stats.throwing		/ 100;
	_stats.melee		+= 4 * diff * _stats.melee			/ 100;
	_stats.strength		+= 2 * diff * _stats.strength		/ 100;
	_stats.psiStrength	+= 4 * diff * _stats.psiStrength	/ 100;
	_stats.psiSkill		+= 4 * diff * _stats.psiSkill		/ 100;

	if (diff == DIFF_BEGINNER)
	{
		_stats.firing >>= 1u;

		for (size_t
				i = 0u;
				i != PARTS_ARMOR;
				++i)
		{
			_armorHp[i] >>= 1u;
		}
	}

	if (month > 0) // aLiens get tuffer as game progresses:
	{
		if (_stats.reactions > 0)	_stats.reactions	+= month;
		if (_stats.firing > 0)		_stats.firing		+= month;
		if (_stats.throwing > 0)	_stats.throwing		+= month;
		if (_stats.melee > 0)		_stats.melee		+= month;
		if (_stats.psiStrength > 0)	_stats.psiStrength	+= (month << 1u);
		if (_stats.psiSkill > 0)	_stats.psiSkill		+= (month >> 1u);

		_stats.health += (month >> 1u);

//		_stats.tu += month;
//		_stats.stamina += month;
//		_stats.strength += month;
//		if (_stats.psiSkill > 0)
//			_stats.psiSkill += month;
	}

	//Log(LOG_INFO) << "BattleUnit::adjustStats(), unitID = " << getId();
	//Log(LOG_INFO) << "BattleUnit::adjustStats(), _stats.tu = " << _stats.tu;
	//Log(LOG_INFO) << "BattleUnit::adjustStats(), _stats.stamina = " << _stats.stamina;
	//Log(LOG_INFO) << "BattleUnit::adjustStats(), _stats.reactions = " << _stats.reactions;
	//Log(LOG_INFO) << "BattleUnit::adjustStats(), _stats.firing = " << _stats.firing;
	//Log(LOG_INFO) << "BattleUnit::adjustStats(), _stats.strength = " << _stats.strength;
	//Log(LOG_INFO) << "BattleUnit::adjustStats(), _stats.melee = " << _stats.melee;
	//Log(LOG_INFO) << "BattleUnit::adjustStats(), _stats.psiSkill = " << _stats.psiSkill;
	//Log(LOG_INFO) << "BattleUnit::adjustStats(), _stats.psiStrength = " << _stats.psiStrength;
}

/**
 * Sets the quantity of TUs reserved for finding cover.
 * @param tuReserved - reserved turn-units
 */
void BattleUnit::setCoverReserve(int tuReserved)
{
	_coverReserve = tuReserved;
}

/**
 * Gets the quantity of TUs reserved for finding cover.
 * @return, reserved turn-units
 */
int BattleUnit::getCoverReserve() const
{
	return _coverReserve;
}

/**
 * Initializes a death-spin.
 */
void BattleUnit::initDeathSpin()
{
	_spinPhase = 0;
	_status = STATUS_TURNING;
	_cacheInvalid = true;
}

/**
 * Continues a death-spin.
 * _spinPhases:
 *				-1 = no spin
 *				 0 = start spin
 *				 1 = CW spin, 1st rotation
 *				 2 = CCW spin, 1st rotation
 *				 3 = CW spin, 2nd rotation
 *				 4 = CCW spin, 2nd rotation
 */
void BattleUnit::contDeathSpin()
{
	if (_dir == 3)	// when facing player, 1 rotation left
	{				// unless start-dir faces player, in which case 2 rotations left
		switch (_spinPhase)
		{
			case 0: //_spinPhase = 2; break; // CCW 2 spins.		- remove this clause to use only 1 rotation when start-dir faces player.
			case 1: //_spinPhase = 3; break; // CW rotation 2nd		- CW rotation
			case 2: //_spinPhase = 4; break; // CCW rotation 2nd	- CCW rotation
				_spinPhase += 2;
				break;
			case 3:
			case 4:
				_spinPhase = -1;
				_status = STATUS_STANDING; // end.
				return;
		}
	}

	if (_spinPhase == 0) // Start here! unless start was facing player above^
	{
		switch (_dir >> 2u)
		{
			case 0:
				switch (_dir)
				{
					case 3:  _spinPhase = 3; break;	// only 1 CW rotation to go ...
					default: _spinPhase = 1;		// 1st CW rotation of 2
				}
				break;

			case 1:
				switch (_dir)
				{
					case 3:  _spinPhase = 4; break;	// only 1 CCW rotation to go ...
					default: _spinPhase = 2;		// 1st CCW rotation of 2
				}
		}
	}

	int dir (_dir);
	switch (_spinPhase & 1)
	{
		case 0:
			if (--dir == -1) dir = 7;
			break;
		case 1:
			if (++dir == 8) dir = 0;
	}
	setUnitDirection(dir);
	_cacheInvalid = true;
}

/**
 * Regulates init, direction & duration of the death spin-cycle.
 * @return, deathspin rotation phase
 */
int BattleUnit::getSpinPhase() const
{
	return _spinPhase;
}

/**
 * Sets the spinphase of this BattleUnit.
 * @param spinphase - the spinphase to set
 */
void BattleUnit::setSpinPhase(int spinphase)
{
	_spinPhase = spinphase;
}

/**
 * Sets this BattleUnit to stop shooting/throwing if it spots a new opponent
 * while auto-turning.
 * @param stop - true to stop everything and refund TU (default true)
 */
void BattleUnit::setStopShot(bool stop)
{
	_stopShot = stop;
}

/**
 * Gets if this BattleUnit spotted a new opponent while auto-turning before
 * shooting/thowing.
 * @return, true if a new hostile was spotted
 */
bool BattleUnit::getStopShot() const
{
	return _stopShot;
}

/**
 * Sets this BattleUnit as dashing.
 * @note Reduces chance of getting hit during reaction-fire.
 * @param dash - true to dash (default true)
 */
void BattleUnit::setDashing(bool dash)
{
	_dashing = dash;
}

/**
 * Gets if this BattleUnit is dashing.
 * @return, true if dashing
 */
bool BattleUnit::getDashing() const
{
	return _dashing;
}

/**
 * Sets this BattleUnit as having been damaged in a single explosion.
 * @param beenhit - true to not deliver any more damage from a single explosion (default true)
 */
void BattleUnit::setTakenExpl(bool beenhit)
{
	_takenExpl = beenhit;
}

/**
 * Gets if this BattleUnit was aleady damaged in a single explosion.
 * @return, true if this unit has already taken damage
 */
bool BattleUnit::getTakenExpl() const
{
	return _takenExpl;
}

/**
 * Sets this BattleUnit as having been damaged in a single fire.
 * @param beenhit - true to not deliver any more damage from a single fire (default true)
 */
void BattleUnit::setTakenFire(bool beenhit)
{
	_takenFire = beenhit;
}

/**
 * Gets if this BattleUnit was aleady damaged in a single fire.
 * @return, true if this unit has already taken fire
 */
bool BattleUnit::getTakenFire() const
{
	return _takenFire;
}

/**
 * Checks if this BattleUnit has an inventory.
 * @note Large units and/or terror units shouldn't show inventories generally.
 * @return, true if an inventory is available
 */
bool BattleUnit::canInventory() const
{
	return _armor->canInventory() == true
		&& (_geoscapeSoldier != nullptr
			|| (_unitRule->isMechanical() == false
				&& _rank != "STR_LIVE_TERRORIST"));
}

/**
 * Gets this BattleUnit's movement-type.
 * @note Use this instead of checking the rules of the armor.
 * @return, MoveType (MapData.h)
 */
MoveType BattleUnit::getMoveTypeUnit() const
{
	return _mType;
}

/**
 * Helper function used by BattleUnit::setSpecialWeapon().
 * @param save -
 * @param unit -
 * @param rule -
 * @return, pointer to BattleItem
 *
// ps. this doesn't have to be and therefore shouldn't be static.
// pps. inline functions don't have to be keyed as such if LTCG is enabled
// and if it isn't they should be defined in the header.
static inline BattleItem* createItem(SavedBattleGame *save, BattleUnit *unit, RuleItem *rule)
{
	BattleItem* item = new BattleItem(rule, save->getCanonicalBattleId());
	item->setOwner(unit);
	save->removeItem(item); //item outside inventory, deleted when SavedBattleGame dTors.
	return item;
} */
/**
 * Sets special weapon that is handled outside inventory.
 * @param save -
 * @param rule -
 *
void BattleUnit::setSpecialWeapon(SavedBattleGame* save, const Ruleset* rule)
{
	RuleItem* item = nullptr;
	int i = 0;
	if (getUnitRules())
	{
		item = rule->getItem(getUnitRules()->getMeleeWeapon());
		if (item)
			_specWeapon[i++] = createItem(save, this, item);
	}
	item = rule->getItem(getArmor()->getSpecialWeapon());
	if (item)
		_specWeapon[i++] = createItem(save, this, item);
	if (_stats.psiSkill > 0 && getOriginalFaction() == FACTION_HOSTILE)
	{
		item = rule->getItem("ALIEN_PSI_WEAPON");
		if (item)
			_specWeapon[i++] = createItem(save, this, item);
	}
} */
/**
 * Gets special weapon.
 * @param type -
 * @return, pointer to BattleItem
 *
BattleItem* BattleUnit::getSpecialWeapon(BattleType type) const
{
	for (int i = 0; i < SPEC_WEAPON_MAX; ++i)
		if (_specWeapon[i] && _specWeapon[i]->getRules()->getBattleType() == type)
			return _specWeapon[i];
	return 0;
} */

/**
 * Get this BattleUnit's battle-statistics.
 * @return, pointer to BattleUnitStatistics
 */
BattleUnitStatistics* BattleUnit::getStatistics() const
{
	return _statistics;
}

/**
 * Sets this BattleUnit's murderer's ID.
 * @param id - murderer ID
 */
void BattleUnit::setMurdererId(int id)
{
	_murdererId = id;
}

/**
 * Gets this BattleUnit's murderer's ID.
 * @return, murderer ID
 */
int BattleUnit::getMurdererId() const
{
	return _murdererId;
}

/**
 * Sets this BattleUnit's order of battle.
 * @param order - position on the craft or at the base
 */
void BattleUnit::setBattleOrder(size_t order)
{
	_battleOrder = order;
}

/**
 * Gets this BattleUnit's order of battle.
 * @return, position on the craft or at the base
 */
size_t BattleUnit::getBattleOrder() const
{
	return _battleOrder;
}

/**
 * Sets the BattleGame for this BattleUnit.
 * @param battleGame - pointer to BattleGame
 */
void BattleUnit::setBattleForUnit(BattlescapeGame* const battleGame)
{
	_battleGame = battleGame;

	if (_unitRule != nullptr
		&& _unitRule->getMeleeWeapon() == "STR_FIST")
	{
		_fist = _battleGame->getFist();
	}
}

/**
 * Sets this BattleUnit's turn direction when spinning 180 degrees.
 * @param dir - 1 counterclockwise; -1 clockwise
 */
void BattleUnit::setTurnDirection(int dir)
{
	_dirTurn = dir;
}

/**
 * Clears turn direction.
 */
void BattleUnit::clearTurnDirection()
{
	_dirTurn = 0;
}

/**
 * Gets all units in the battlescape that are valid RF-spotters of this
 * BattleUnit.
 * @return, pointer to a list of pointers to BattleUnits that are spotting
 */
std::list<BattleUnit*>* BattleUnit::getRfSpotters()
{
	return &_rfSpotters;
}

/**
 * Sets or Gets the Psi strength and skill of an aLien who successfully
 * mind-controls this BattleUnit.
 * @note These values are used if Player tries to re-control a hostile xCom (or
 * civilian) unit. (But the AI doesn't do civies.)
 * @param strength	- psi strength
 * @param skill		- psi skill
 */
void BattleUnit::hostileMcValues(
		int& strength,
		int& skill)
{
	switch (skill)
	{
		case 0: // get params
			strength = _mcStrength;
			skill = _mcSkill;
			break;

		default: // set params
			_mcStrength	= strength - ((_stats.psiStrength + 2) / 3);
			_mcSkill	= skill    - ((_stats.psiSkill    + 2) / 3);

			if (_mcStrength < 0) _mcStrength = 0;
			if (_mcSkill    < 0) _mcSkill    = 0;
	}
}

/**
 * Gets if the BattleUnit is mind-controlled.
 * @return, true if Mc'd
 */
bool BattleUnit::isMindControlled() const
{
	return (_faction != _originalFaction);
}

/**
 * Gets if this BattleUnit is a Zombie.
 * @return, true if zombie
 */
bool BattleUnit::isZombie() const
{
	return _isZombie;
}

/**
 * Gets if this BattleUnit avoids fire-tiles.
 * @return, true if unit avoids fire
 */
bool BattleUnit::avoidsFire() const
{
	if (_faction != FACTION_PLAYER // used by the AI only.
		&& (_armor->getDamageModifier(DT_IN) > 0.f
			|| _isZombie == false))
	{
		return true;
	}
	return false;
}

/**
 * Gets if this Unit is immune to psionic attacks.
 * @return, true if unit is immune to Psi
 */
bool BattleUnit::psiBlock() const
{
	return _psiBlock;
}

/**
 * Gets if this BattleUnit has been stunned before.
 * @return, true if was stunned
 */
bool BattleUnit::beenStunned() const
{
	return _hasBeenStunned;
}

/**
 * Gets the BattleUnit's last-cover Position.
 * @note Scratch value for AI's lefthand to tell its righthand what's up ...
 * don't zone out and start patrolling again.
 */
Position BattleUnit::getLastCover() const
{
	return _lastCover;
}

/**
 * Tries to burn a Tile if this BattleUnit is capable of doing so.
 * @note A check for SPECAB_BURN ought be done before call here.
 * @param tile - pointer to a tile to start on fire
 */
void BattleUnit::burnTile(Tile* const tile)
{
	if (_unitRule != nullptr) // safety.
	{
		const int power (_unitRule->getSpecabPower());
		tile->igniteTile(power / 10);

		if (_battleGame != nullptr) // safety.
		{
			const Position targetVoxel (Position::toVoxelSpaceCentered(
																	tile->getPosition(),
																	-tile->getTerrainLevel()));
			_battleGame->getTileEngine()->hit(
											targetVoxel,
											power,
											DT_IN,
											this);
		}
	}
}

/**
 * Converts UnitStatus to a string for the Logfile.
 * @param status - status (BattleUnit.h)
 * @return, status as a string
 */
std::string BattleUnit::debugStatus(UnitStatus status) // static
{
	switch (status)
	{
		case STATUS_STANDING:		return "standing";
		case STATUS_WALKING:		return "walking";
		case STATUS_FLYING:			return "flying";
		case STATUS_TURNING:		return "turning";
		case STATUS_AIMING:			return "aiming";
		case STATUS_COLLAPSING:		return "collapsing";
		case STATUS_DEAD:			return "dead";
		case STATUS_UNCONSCIOUS:	return "unconscious";
		case STATUS_PANICKING:		return "panicking";
		case STATUS_BERSERK:		return "berserk";
		case STATUS_LATENT:			return "latent";
		case STATUS_LATENT_START:	return "latent_start";

		default:
			return "error: no status";
	}
}

}
