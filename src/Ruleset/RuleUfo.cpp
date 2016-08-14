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

#include "RuleUfo.h"

#include "RuleTerrain.h"


namespace OpenXcom
{

const char* RuleUfo::stSize[5u] // static.
{
	"STR_VERY_SMALL",	// 0
	"STR_SMALL",		// 1
	"STR_MEDIUM_UC",	// 2
	"STR_LARGE",		// 3
	"STR_VERY_LARGE"	// 4
};


/**
 * Creates a blank ruleset for a specified type of UFO.
 * @param type - reference to the type
 */
RuleUfo::RuleUfo(const std::string& type)
	:
		_type(type),
		_size(stSize[0u]),
		_sizeType(UFO_VERYSMALL),
		_sprite(-1),
		_marker(-1),
		_damageMax(0),
		_speedMax(0),
		_accel(0),
		_power(0),
		_range(0),
		_score(0),
		_scoreAct(0),
		_reload(0),
		_escape(0),
		_rangeRecon(600),
		_terrainRule(nullptr)
{}

/**
 * dTor.
 */
RuleUfo::~RuleUfo()
{
	delete _terrainRule;
}

/**
 * Loads the UFO from a YAML file.
 * @param node	- reference a YAML node
 * @param rules	- pointer to Ruleset
 */
void RuleUfo::load(
		const YAML::Node& node,
		Ruleset* const rules)
{
	_type		= node["type"]		.as<std::string>(_type);
	_size		= node["size"]		.as<std::string>(_size);
	_sprite		= node["sprite"]	.as<int>(_sprite);
	_marker		= node["marker"]	.as<int>(_marker);
	_damageMax	= node["damageMax"]	.as<int>(_damageMax);
	_speedMax	= node["speedMax"]	.as<int>(_speedMax);
	_accel		= node["accel"]		.as<int>(_accel);
	_power		= node["power"]		.as<int>(_power);
	_range		= node["range"]		.as<int>(_range);
	_score		= node["score"]		.as<int>(_score);
	_scoreAct	= node["scoreAct"]	.as<int>(_scoreAct);
	_reload		= node["reload"]	.as<int>(_reload);
	_escape		= node["escape"]	.as<int>(_escape);
	_rangeRecon	= node["rangeRecon"].as<int>(_rangeRecon);
	_spriteAlt	= node["spriteAlt"]	.as<std::string>(_spriteAlt);

	if		(_size == stSize[0u]) _sizeType = UFO_VERYSMALL;
	else if	(_size == stSize[1u]) _sizeType = UFO_SMALL;
	else if	(_size == stSize[2u]) _sizeType = UFO_MEDIUM;
	else if	(_size == stSize[3u]) _sizeType = UFO_LARGE;
	else if	(_size == stSize[4u]) _sizeType = UFO_VERYLARGE;

	if (const YAML::Node& terrain = node["tacticalTerrain"])
	{
		delete _terrainRule;

		RuleTerrain* const terrainRule (new RuleTerrain(terrain["rule"].as<std::string>()));
		terrainRule->load(terrain, rules);
		_terrainRule = terrainRule;
	}
}

/**
 * Gets the language string that names this UFO.
 * @note Each UFO type has a unique type.
 * @return, the type
 */
const std::string& RuleUfo::getType() const
{
	return _type;
}

/**
 * Gets the size of this type of UFO.
 * @return, the size as string
 */
const std::string& RuleUfo::getSize() const
{
	return _size;
}

/**
 * Gets the size-type of this type of UFO.
 * @return, the size-type (RuleUfo.h)
 */
UfoSizeType RuleUfo::getSizeType() const
{
	return _sizeType;
}

/**
 * Gets the radius of this type of UFO on the dogfighting window.
 * @return, the radius in pixels
 */
size_t RuleUfo::getRadius() const
{
	return static_cast<size_t>(_sizeType);
//	switch (_sizeType)
//	{
//		case UFO_VERYSMALL:	return 0u;
//		case UFO_SMALL:		return 1u;
//		case UFO_MEDIUM:	return 2u;
//		case UFO_LARGE:		return 3u;
//		case UFO_VERYLARGE:	return 4u;
//	}
}

/**
 * Gets the ID of the sprite used to draw the UFO in the Dogfight window.
 * @return, the sprite ID
 */
int RuleUfo::getSprite() const
{
	return _sprite;
}

/**
 * Gets the globe-marker for the UFO type.
 * @return, marker sprite -1 if none defined in YAML
 */
int RuleUfo::getMarker() const
{
	return _marker;
}

/**
 * Gets the maximum damage (damage the UFO can take) of the UFO.
 * @return, the maximum damage
 */
int RuleUfo::getMaxDamage() const
{
	return _damageMax;
}

/**
 * Gets the maximum speed of the UFO flying around the Geoscape.
 * @return, the maximum speed
 */
int RuleUfo::getMaxSpeed() const
{
	return _speedMax;
}

/**
 * Gets the acceleration of the UFO for taking off or stopping.
 * @return, the acceleration
 */
int RuleUfo::getAcceleration() const
{
	return _accel;
}

/**
 * Gets the maximum damage done by the UFO's weapons per shot.
 * @return, the weapon power
 */
int RuleUfo::getWeaponPower() const
{
	return _power;
}

/**
 * Gets the maximum range for the UFO's weapons.
 * @return, the weapon range
 */
int RuleUfo::getWeaponRange() const
{
	return _range << 3u; // convert from "kilometers" ... to Dogfight distance.
}

/**
 * Gets the amount of points the player gets for shooting down the UFO.
 * @return, the score
 */
int RuleUfo::getScore() const
{
	return _score;
}

/**
 * Gets this type of Ufo's activity score.
 * @return, activity score
 */
int RuleUfo::getActivityScore() const
{
	return _scoreAct;
}

/**
 * Gets the RuleTerrain to draw this type of UFO on the battlefield.
 * @return, pointer to RuleTerrain
 */
RuleTerrain* RuleUfo::getTacticalTerrainData() const
{
	return _terrainRule;
}

/**
 * Gets the UFO's weapon reload period.
 * @return, ticks
 */
int RuleUfo::getWeaponReload() const
{
	return _reload;
}

/**
 * Gets the UFO's break off period.
 * @return, ticks
 */
int RuleUfo::getEscape() const
{
	return _escape;
}

/**
 * For user-defined UFOs use an alternate Surface for the dogfight preview.
 * @return, the string-ID for the surface
 */
std::string RuleUfo::getSpriteString() const
{
	return _spriteAlt;
}

/**
 * Gets the UFO's radar-range for detecting bases.
 * @return, the range in nautical miles
 */
int RuleUfo::getRangeRecon() const
{
	return _rangeRecon;
}

}
