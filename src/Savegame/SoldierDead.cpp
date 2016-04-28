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

#include "SoldierDead.h"

#include "SoldierDeath.h"
#include "SoldierDiary.h"

#include "../Engine/Language.h"


namespace OpenXcom
{

/**
 * Creates the SoldierDead from a Soldier with a pre-existing SoldierDiary.
 * @note Used by Soldiers dying IG.
 * @param name			-
 * @param id			-
 * @param rank			-
 * @param gender		-
 * @param look			-
 * @param missions		-
 * @param kills			-
 * @param death			- pointer to SoldierDeath time
 * @param initialStats	-
 * @param currentStats	-
 * @param diary			- copy of SoldierDiary from the recently deceased Soldier
 */
SoldierDead::SoldierDead(
		const std::wstring& name,
		const int id,
		const SoldierRank rank,
		const SoldierGender gender,
		const SoldierLook look,
		const int missions,
		const int kills,
		SoldierDeath* const death,
		const UnitStats& initialStats,
		const UnitStats& currentStats,
		SoldierDiary diary) // + Base if I want to ...
	:
		_name(name),
		_id(id),
		_rank(rank),
		_gender(gender),
		_look(look),
		_missions(missions),
		_kills(kills),
		_death(death),
		_initialStats(initialStats),
		_currentStats(currentStats)
{
	_diary = new SoldierDiary();
	*_diary = diary; // copy diary from previous owner ....
}

/**
 * Creates the SoldierDead without a pre-existing SoldierDiary.
 * @note Used when loading a saved game.
 */
SoldierDead::SoldierDead(
		const std::wstring& name,
		const int id,
		const SoldierRank rank,
		const SoldierGender gender,
		const SoldierLook look,
		const int missions,
		const int kills,
		SoldierDeath* const death,
		const UnitStats& initialStats,
		const UnitStats& currentStats)
	:
		_name(name),
		_id(id),
		_rank(rank),
		_gender(gender),
		_look(look),
		_missions(missions),
		_kills(kills),
		_death(death),
		_initialStats(initialStats),
		_currentStats(currentStats)
{
	_diary = new SoldierDiary(); // empty diary. Should fill up from YAML save.
}

/**
 * dTor.
 */
SoldierDead::~SoldierDead()
{
	delete _death;
	delete _diary;
}

/**
 * Loads this SoldierDead from a YAML file.
 * @param node - reference a YAML node
 */
void SoldierDead::load(const YAML::Node& node)
{
	_rank	= static_cast<SoldierRank>(node["rank"]		.as<int>());
	_gender	= static_cast<SoldierGender>(node["gender"]	.as<int>());
	_look	= static_cast<SoldierLook>(node["look"]		.as<int>());

	_name = Language::utf8ToWstr(node["name"].as<std::string>());

	_id				= node["id"]						.as<int>(_id);
	_initialStats	= node["initialStats"]				.as<UnitStats>(_initialStats);
	_currentStats	= node["currentStats"]				.as<UnitStats>(_currentStats);
	_missions		= node["missions"]					.as<int>(_missions);
	_kills			= node["kills"]						.as<int>(_kills);

	_death = new SoldierDeath();
	_death->load(node["death"]);

	if (node["diary"] != nullptr)
		_diary->load(node["diary"]);
}

/**
 * Saves this SoldierDead to a YAML file.
 * @return, YAML node
 */
YAML::Node SoldierDead::save() const
{
	YAML::Node node;

	node["name"]			= Language::wstrToUtf8(_name);
	node["id"]				= _id;
	node["initialStats"]	= _initialStats;
	node["currentStats"]	= _currentStats;
	node["rank"]			= static_cast<int>(_rank);
	node["gender"]			= static_cast<int>(_gender);
	node["look"]			= static_cast<int>(_look);
	node["missions"]		= _missions;
	node["kills"]			= _kills;

	node["death"]			= _death->save();

	if (_diary->getMissionIdList().empty() == false
		|| _diary->getSoldierAwards()->empty() == false)
	{
		node["diary"] = _diary->save();
	}

	return node;
}

/**
 * Gets this SoldierDead's name.
 * @return, name-string
 */
std::wstring SoldierDead::getName() const
{
	return _name;
}

/**
 * Gets a localizable-string representation of this SoldierDead's rank.
 * @return, string-ID of rank
 */
std::string SoldierDead::getRankString() const
{
	switch (_rank)
	{
		case RANK_ROOKIE:		return "STR_ROOKIE";
		case RANK_SQUADDIE:		return "STR_SQUADDIE";
		case RANK_SERGEANT:		return "STR_SERGEANT";
		case RANK_CAPTAIN:		return "STR_CAPTAIN";
		case RANK_COLONEL:		return "STR_COLONEL";
		case RANK_COMMANDER:	return "STR_COMMANDER";
	}
	return "";
}

/**
 * Gets a graphic representation of this SoldierDead's rank.
 * @note THE MEANING OF LIFE
 * @return, sprite-ID for the rank
 */
int SoldierDead::getRankSprite() const
{
	return 42 + _rank;
}

/**
 * Gets this SoldierDead's rank.
 * @return, rank (Soldier.h)
 */
SoldierRank SoldierDead::getRank() const
{
	return _rank;
}

/**
 * Gets this SoldierDead's quantity of missions.
 * @return, quantity of missions
 */
int SoldierDead::getMissions() const
{
	return _missions;
}

/**
 * Gets this SoldierDead's quantity of kills.
 * @return, quantity of kills
 */
int SoldierDead::getKills() const
{
	return _kills;
}

/**
 * Gets this SoldierDead's gender.
 * @return, gender (Soldier.h)
 */
SoldierGender SoldierDead::getGender() const
{
	return _gender;
}

/**
 * Gets this SoldierDead's look.
 * @return, look (Soldier.h)
 */
SoldierLook SoldierDead::getLook() const
{
	return _look;
}

/**
 * Gets this SoldierDead's unique-ID.
 * @note Each dead soldier can be identified by its ID (not it's name).
 * @return, unique-ID
 */
int SoldierDead::getId() const
{
	return _id;
}

/**
 * Gets this SoldierDead's initial stats.
 * @return, pointer to UnitStats struct
 */
UnitStats* SoldierDead::getInitStats()
{
	return &_initialStats;
}

/**
 * Gets this SoldierDead's current stats.
 * @return, pointer to UnitStats struct
 */
UnitStats* SoldierDead::getCurrentStats()
{
	return &_currentStats;
}

/**
 * Gets this SoldierDead's time of death.
 * @return, pointer to SoldierDeath
 */
SoldierDeath* SoldierDead::getDeath() const
{
	return _death;
}

/**
 * Gets this SoldierDead's SoldierDiary.
 * @return, pointer to SoldierDiary
 */
SoldierDiary* SoldierDead::getDiary() const
{
	return _diary;
}

}
