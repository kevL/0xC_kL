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

#include "RuleBaseFacility.h"


namespace OpenXcom
{

/**
 * Creates a blank ruleset for a certain type of base facility.
 * @param type - reference the type
 */
RuleBaseFacility::RuleBaseFacility(const std::string& type)
	:
		_type(type),
		_spriteShape(-1),
		_spriteFacility(-1),
		_lift(false),
		_hyper(false),
		_mind(false),
		_grav(false),
		_size(1),
		_buildCost(0),
		_buildTime(0),
		_monthlyCost(0),
		_storage(0),
		_personnel(0),
		_aliens(0),
		_crafts(0),
		_labs(0),
		_workshops(0),
		_psiLabs(0),
		_radarRange(0),
		_radarChance(0),
		_defense(0),
		_hitRatio(0),
		_fireSound(0),
		_hitSound(0),
		_listOrder(0)
{}

/**
 * dTor.
 */
RuleBaseFacility::~RuleBaseFacility()
{}

/**
 * Loads the base facility type from a YAML file.
 * @param node		- reference a YAML node
 * @param modIndex	- a value that offsets the sounds and sprite values to avoid conflicts
 * @param listOrder	- the list weight for this facility
 */
void RuleBaseFacility::load(
		const YAML::Node& node,
		int modIndex,
		int listOrder)
{
	_type		= node["type"]		.as<std::string>(_type);
	_required	= node["required"]	.as<std::vector<std::string> >(_required);

	if (node["spriteShape"])
	{
		_spriteShape = node["spriteShape"].as<int>(_spriteShape);
		if (_spriteShape > 33) // BASEBITS.PCK: 34 entries
			_spriteShape += modIndex;
	}

	if (node["spriteFacility"])
	{
		_spriteFacility = node["spriteFacility"].as<int>(_spriteFacility);
		if (_spriteFacility > 33) // BASEBITS.PCK: 34 entries
			_spriteFacility	+= modIndex;
	}

	_lift				= node["lift"]			.as<bool>(_lift);
	_hyper				= node["hyper"]			.as<bool>(_hyper);
	_mind				= node["mind"]			.as<bool>(_mind);
	_grav				= node["grav"]			.as<bool>(_grav);
	_size				= node["size"]			.as<int>(_size);
	_buildCost			= node["buildCost"]		.as<int>(_buildCost);
	_buildTime			= node["buildTime"]		.as<int>(_buildTime);
	_monthlyCost		= node["monthlyCost"]	.as<int>(_monthlyCost);
	_storage			= node["storage"]		.as<int>(_storage);
	_personnel			= node["personnel"]		.as<int>(_personnel);
	_aliens				= node["aliens"]		.as<int>(_aliens);
	_crafts				= node["crafts"]		.as<int>(_crafts);
	_labs				= node["labs"]			.as<int>(_labs);
	_workshops			= node["workshops"]		.as<int>(_workshops);
	_psiLabs			= node["psiLabs"]		.as<int>(_psiLabs);
	_radarRange			= node["radarRange"]	.as<int>(_radarRange);
	_radarChance		= node["radarChance"]	.as<int>(_radarChance);
	_defense			= node["defense"]		.as<int>(_defense);
	_hitRatio			= node["hitRatio"]		.as<int>(_hitRatio);

	if (node["fireSound"])
	{
		_fireSound = node["fireSound"].as<int>(_fireSound);
		if (_fireSound > 13) // GEO.CAT: 14 entries
			_fireSound += modIndex;
	}

	if (node["hitSound"])
	{
		_hitSound = node["hitSound"].as<int>(_hitSound);
		if (_hitSound > 13) // GEO.CAT: 14 entries
			_hitSound += modIndex;
	}

	_mapName		= node["mapName"]	.as<std::string>(_mapName);
	_listOrder		= node["listOrder"]	.as<int>(_listOrder);
	if (_listOrder == 0)
		_listOrder	= listOrder;
}

/**
 * Gets the language string that names this base facility.
 * @note Each base facility type has a unique name.
 * @return, the facility's type
 */
std::string RuleBaseFacility::getType() const
{
	return _type;
}

/**
 * Gets the list of research required to build this base facility.
 * @return, reference to a vector of research-type strings
 */
const std::vector<std::string>& RuleBaseFacility::getRequirements() const
{
	return _required;
}

/**
 * Gets the ID of the sprite used to draw the base structure of the facility
 * that defines its shape.
 * @return, sprite ID
 */
int RuleBaseFacility::getSpriteShape() const
{
	return _spriteShape;
}

/**
 * Gets the ID of the sprite used to draw the facility's contents inside the
 * base shape.
 * @return, sprite ID
 */
int RuleBaseFacility::getSpriteFacility() const
{
	return _spriteFacility;
}

/**
 * Gets the size of the facility on the base grid.
 * @return, length in grid squares
 */
size_t RuleBaseFacility::getSize() const
{
	return _size;
}

/**
 * Checks if this facility is the core access lift of a base.
 * @note Every base has an access lift and all facilities have to be connected
 * to it.
 * @return, true if a lift
 */
bool RuleBaseFacility::isLift() const
{
	return _lift;
}

/**
 * Checks if this facility has hyperwave detection capabilities.
 * @note This allows facility to get extra details about UFOs.
 * @return, true if has hyperwave detection
 */
bool RuleBaseFacility::isHyperwave() const
{
	return _hyper;
}

/**
 * Checks if this facility has a mind shield - which greatly helps cover the
 * base from alien detection.
 * @return, true if has a mind shield
 */
bool RuleBaseFacility::isMindShield() const
{
	return _mind;
}

/**
 * Checks if this facility has a grav shield - which doubles base defense's
 * fire ratio.
 * @return, true if has a grav shield
 */
bool RuleBaseFacility::isGravShield() const
{
	return _grav;
}

/**
 * Gets the amount of funds that this facility costs to build at a base.
 * @return, building cost
 */
int RuleBaseFacility::getBuildCost() const
{
	return _buildCost;
}

/**
 * Gets the amount of time that this facility takes to be constructed after
 * placement.
 * @return, time in days
 */
int RuleBaseFacility::getBuildTime() const
{
	return _buildTime;
}

/**
 * Gets the amount of funds this facility costs monthly to maintain after it's
 * fully built.
 * @return, monthly cost
 */
int RuleBaseFacility::getMonthlyCost() const
{
	return _monthlyCost;
}

/**
 * Gets the amount of storage space this facility provides for base equipment.
 * @return, storage space
 */
int RuleBaseFacility::getStorage() const
{
	return _storage;
}

/**
 * Gets the number of base personnel - soldiers scientists engineers - this
 * facility can contain.
 * @return, number of personnel
 */
int RuleBaseFacility::getPersonnel() const
{
	return _personnel;
}

/**
 * Gets the number of captured live aLiens this facility can contain.
 * @return, number of aliens
 */
int RuleBaseFacility::getAliens() const
{
	return _aliens;
}

/**
 * Gets the number of Craft this facility can contain.
 * @return, number of craft
 */
int RuleBaseFacility::getCrafts() const
{
	return _crafts;
}

/**
 * Gets the amount of laboratory space this facility provides for research
 * projects.
 * @return, laboratory space
 */
int RuleBaseFacility::getLaboratories() const
{
	return _labs;
}

/**
 * Gets the amount of workshop space this facility provides for manufacturing
 * projects.
 * @return, workshop space
 */
int RuleBaseFacility::getWorkshops() const
{
	return _workshops;
}

/**
 * Gets the number of soldiers this facility can contain for monthly
 * psi-training.
 * @return, number of soldiers
 */
int RuleBaseFacility::getPsiLaboratories() const
{
	return _psiLabs;
}

/**
 * Gets the radar range this facility provides for the detection of UFOs.
 * @return, range in nautical miles
 */
int RuleBaseFacility::getRadarRange() const
{
	return _radarRange;
}

/**
 * Gets the chance of detecting UFOs that come within this facility's radar
 * range.
 * @return, chance as a percentage
 */
int RuleBaseFacility::getRadarChance() const
{
	return _radarChance;
}

/**
 * Gets the defense value of this facility's weaponry against UFO invasions.
 * @return, defense value
 */
int RuleBaseFacility::getDefenseValue() const
{
	return _defense;
}

/**
 * Gets the hit ratio of this facility's weaponry against UFO invasions.
 * @return, hit ratio as a percentage
 */
int RuleBaseFacility::getHitRatio() const
{
	return _hitRatio;
}

/**
 * Gets the battlescape map-block name for this facility to construct the base
 * defense mission map.
 * @return, map name
 */
std::string RuleBaseFacility::getMapName() const
{
	return _mapName;
}

/**
 * Gets the hit sound of this facility's anti-UFO weaponry.
 * @return, sound index number
 */
int RuleBaseFacility::getHitSound() const
{
	return _hitSound;
}

/**
 * Gets the fire sound of this facility's anti-UFO weaponry.
 * @return, sound index number
 */
int RuleBaseFacility::getFireSound() const
{
	return _fireSound;
}

/**
 * Gets this facility's list weight.
 * @return, list weight
 */
int RuleBaseFacility::getListOrder() const
{
	return _listOrder;
}

}
