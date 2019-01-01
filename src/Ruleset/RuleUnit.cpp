/*
 * Copyright 2010-2019 OpenXcom Developers.
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

#include "RuleUnit.h"

#include "../Engine/Exception.h"


namespace OpenXcom
{

/**
 * Creates the rule for a specified type of unit.
 * @param type - reference to the unit-type
 */
RuleUnit::RuleUnit(const std::string& type)
	:
		_type(type),
		_standHeight(0),
		_kneelHeight(0),
		_floatHeight(0),
		_score(0),
		_deathSound(-1),
		_aggroSound(-1),
		_moveSound(-1),
		_intelligence(0),
		_aggression(0),
		_energyRecovery(30),
		_specab(SPECAB_NONE),
		_capturable(true),
		_specabPower(0),
		_livingWeapon(false),
		_female(false),
		_dog(false),
		_mechanical(false), // kL: these two should perhaps go to Armor class.
		_psiBlock(false),
		_hasHands(true)
{}

/**
 * dTor.
 */
RuleUnit::~RuleUnit()
{}

/**
 * Loads this unit-type from a YAML file.
 * @param node		- YAML node
 * @param modIndex	- a value that offsets the sounds and sprite values to avoid conflicts
 */
void RuleUnit::load(
		const YAML::Node& node,
		int modIndex)
{
	_type			= node["type"]				.as<std::string>(_type);
	_race			= node["race"]				.as<std::string>(_race);
	_rank			= node["rank"]				.as<std::string>(_rank);
	_stats			.mergeStats(node["stats"]	.as<UnitStats>(_stats));
	_armor			= node["armor"]				.as<std::string>(_armor);
	_standHeight	= node["standHeight"]		.as<int>(_standHeight);
	_kneelHeight	= node["kneelHeight"]		.as<int>(_kneelHeight);
	_floatHeight	= node["floatHeight"]		.as<int>(_floatHeight);

//	if (_floatHeight + _standHeight > 24)
	if (_standHeight > 24) // reasons. See TileEngine::doTargetUnit()
	{
//		throw Exception("Error with unit " + _type + ": Unit height + float height may not exceed 24");
		throw Exception("Error with Unit " + _type + ": Unit height shall not exceed 24");
	}

	_score			= node["score"]			.as<int>(_score);
	_intelligence	= node["intelligence"]	.as<int>(_intelligence);
	_aggression		= node["aggression"]	.as<int>(_aggression);
	_energyRecovery	= node["energyRecovery"].as<int>(_energyRecovery);
	_livingWeapon	= node["livingWeapon"]	.as<bool>(_livingWeapon);
	_meleeWeapon	= node["meleeWeapon"]	.as<std::string>(_meleeWeapon);
//	_builtInWeapons	= node["builtInWeapons"].as<std::vector<std::string>>(_builtInWeapons);
	_female			= node["female"]		.as<bool>(_female);
	_dog			= node["dog"]			.as<bool>(_dog);
	_mechanical		= node["mechanical"]	.as<bool>(_mechanical);
	_psiBlock		= node["psiBlock"]		.as<bool>(_psiBlock);
	_hasHands		= node["hasHands"]		.as<bool>(_hasHands);
	_spawnType		= node["spawnType"]		.as<std::string>(_spawnType);
	_specabPower	= node["specabPower"]	.as<int>(_specabPower);
	_capturable		= node["capturable"]	.as<bool>(_capturable);

	_specab			= static_cast<SpecialAbility>(node["specab"]    .as<int>(_specab));

	if (node["deathSound"])
	{
		_deathSound = node["deathSound"].as<int>(_deathSound);
		if (_deathSound > 54) // BATTLE.CAT: 55 entries
			_deathSound += modIndex;
	}

	if (node["aggroSound"])
	{
		_aggroSound = node["aggroSound"].as<int>(_aggroSound);
		if (_aggroSound > 54) // BATTLE.CAT: 55 entries
			_aggroSound += modIndex;
	}

	if (node["moveSound"])
	{
		_moveSound = node["moveSound"].as<int>(_moveSound);
		if (_moveSound > 54) // BATTLE.CAT: 55 entries
			_moveSound += modIndex;
	}
}

/**
 * Gets this unit-type's identifier.
 * @note Each unit is identified by race and if alien rank.
 * @return, the unit's name
 */
std::string RuleUnit::getType() const
{
	return _type;
}

/**
 * Gets this unit-type's stats.
 * @return, pointer to the stats
 */
UnitStats* RuleUnit::getStats()
{
	return &_stats;
}

/**
 * Gets this unit-type's height when standing.
 * @return, the standing height
 */
int RuleUnit::getStandHeight() const
{
	return _standHeight;
}

/**
 * Gets this unit-type's height when kneeling.
 * @return, the kneeling height
 */
int RuleUnit::getKneelHeight() const
{
	return _kneelHeight;
}

/**
 * Gets this unit-type's floating elevation.
 * @return, the floating elevation
 */
int RuleUnit::getFloatHeight() const
{
	return _floatHeight;
}

/**
 * Gets this unit-type's armor-type.
 * @return, the armor-type
 */
std::string RuleUnit::getArmorType() const
{
	return _armor;
}

/**
 * Gets this unit-type's race-type.
 * @return, the race
 */
std::string RuleUnit::getRace() const
{
	return _race;
}

/**
 * Gets this unit-type's rank.
 * @return, the rank
 */
std::string RuleUnit::getRank() const
{
	return _rank;
}

/**
 * Gets this unit-type's score.
 * @return, the score
 */
int RuleUnit::getValue() const
{
	return _score;
}

/**
 * Gets this unit-type's death-sound.
 * @return, the death-sound ID
 */
int RuleUnit::getDeathSound() const
{
	return _deathSound;
}

/**
 * Gets this unit-type's move-sound.
 * @return, the move-sound ID
 */
int RuleUnit::getMoveSound() const
{
	return _moveSound;
}

/**
 * Gets the this unit-type's warcry.
 * @return, the aggro-sound ID
 */
int RuleUnit::getAggroSound() const
{
	return _aggroSound;
}

/**
 * Gets this unit-type's intelligence.
 * @note This is the number of turns the AI remembers player's unit-positions.
 * @return, the intelligence
 */
int RuleUnit::getIntelligence() const
{
	return _intelligence;
}

/**
 * Gets this unit-type's aggression.
 * @note The AI uses this value to determines the chance of taking revenge or
 * cover.
 * @return, the aggression
 */
int RuleUnit::getAggression() const
{
	return _aggression;
}

/**
 * Gets this unit-type's special ability.
 * @return, the specab
 */
SpecialAbility RuleUnit::getSpecialAbility() const
{
	return _specab;
}

/**
 * Gets this unit-type's special ability power (fire or explode).
 * @return, specab power
 */
int RuleUnit::getSpecabPower() const
{
	return _specabPower;
}

/**
 * Gets the unit-type that spawns when this one dies.
 * @return, the unit's spawn-type
 */
const std::string& RuleUnit::getSpawnType() const
{
	return _spawnType;
}

/**
 * Gets this unit-type's stamina recovery per turn as a percentage.
 * @return, rate of stamina recovery
 */
int RuleUnit::getEnergyRecovery() const
{
	return _energyRecovery;
}

/**
 * Checks if this unit-type is a living weapon - eg: chryssalid.
 * @note A living weapon ignores any loadout that may be available to its rank
 * and uses the one associated with its race. This is applied only to aLien
 * terroristic units in BattlescapeGenerator::deployAliens() where the string
 * "_WEAPON" is added to their type to get their weapon.
 * @return, true if this unit is a living weapon
 */
bool RuleUnit::isLivingWeapon() const
{
	return _livingWeapon;
}

/**
 * Gets this unit-type's built-in melee weapon if any.
 * @return, the type of the weapon
 */
const std::string& RuleUnit::getMeleeWeapon() const
{
	return _meleeWeapon;
}

/**
 * Gets what weapons this unit-type has built in.
 * @note This is a vector of strings representing any weapons that are inherent
 * to this creature. Unlike '_livingWeapon' this is used in ADDITION to any
 * loadout or living weapon item that may be defined.
 * @return, list of weapons that are (further) integral to this unit.
 *
const std::vector<std::string>& RuleUnit::getBuiltInWeapons() const
{
	return _builtInWeapons;
} */

/**
 * Checks if this unit-type is female.
 * @return, true if female
 */
bool RuleUnit::isFemale() const
{
	return _female;
}

/**
 * Checks if this unit-type is dog.
 * @return, true if dog
 */
bool RuleUnit::isDog() const
{
	return _dog;
}

/**
 * Checks if this unit-type is a mechanical apparatus.
 * @note This var subsumes several more detailed ideas:
 * - isTrackedVehicle
 * - isPsiAttackable / isSentient <- DONE
 * - canRevive (from status_Unconscious)
 * - canChangeMorale (see isMoralable())
 * - isInfectable (can have a spawnType string set on it)
 * - isMetal (cannot catch fire)
 * - canOpenDoors etc.
 * @return, true if this is a non-organic purely mechanical unit
 */
bool RuleUnit::isMechanical() const
{
	return _mechanical;
}

/**
 * Checks if this unit-type is immune to psionic attacks.
 * @return, true if immune to psi
 */
bool RuleUnit::getPsiBlock() const
{
	return _psiBlock;
}

/**
 * Checks if this unit-type can open a door w/ RMB click or prime a grenade during
 * battle.
 * @note Units can always open doors by moving through them if space permits as
 * well as prime grenades if in preBattle inventory.
 * @return, true if this unit can open doors etc
 */
bool RuleUnit::hasHands() const
{
	return _hasHands;
}

/**
 * Gets whether this unit-type can be captured alive.
 * @return, a value determining whether the alien can be captured alive.
 */
bool RuleUnit::isCapturable() const
{
	return _capturable;
}

}
