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

#include "TerrorSite.h"

#include "../Engine/Language.h"

#include "../Geoscape/Globe.h" // Globe::GLM_TERRORSITE

#include "../Ruleset/RuleAlienDeployment.h"
#include "../Ruleset/RuleAlienMission.h"


namespace OpenXcom
{

/**
 * Creates the TerrorSite.
 */
TerrorSite::TerrorSite(
		const RuleAlienMission* const missionRule,
		const RuleAlienDeployment* const ruleDeploy)
	:
		Target(),
		_missionRule(missionRule),
		_ruleDeploy(ruleDeploy),
		_texture(-1),
		_secondsLeft(0),
		_tactical(false),
		_detected(false)
{}

/**
 * dTor.
 */
TerrorSite::~TerrorSite()
{}

/**
 * Loads this TerrorSite from a YAML file.
 * @param node - reference a YAML node
 */
void TerrorSite::load(const YAML::Node& node)
{
	// NOTE: "type" & "deployment" loaded by SavedGame and passed into cTor.
	Target::load(node);

	_id				= node["id"]			.as<int>(_id);
	_texture		= node["texture"]		.as<int>(_texture);
	_race			= node["race"]			.as<std::string>(_race);
	_secondsLeft	= node["secondsLeft"]	.as<int>(_secondsLeft);
	_detected		= node["detected"]		.as<bool>(_detected);
	_tactical		= node["tactical"]		.as<bool>(_tactical);

}

/**
 * Saves this TerrorSite to a YAML file.
 * @return, YAML node
 */
YAML::Node TerrorSite::save() const
{
	YAML::Node node = Target::save();

	node["type"]		= _missionRule->getType();
	node["deployment"]	= _ruleDeploy->getType();

	node["id"]			= _id;
	node["texture"]		= _texture;
	node["race"]		= _race;

	if (_secondsLeft != 0)	node["secondsLeft"]	= _secondsLeft;
	if (_detected == true)	node["detected"]	= _detected;
	if (_tactical == true)	node["tactical"]	= _tactical;

	return node;
}

/**
 * Saves this TerrorSite's identificator to a YAML file.
 * @return, YAML node
 */
YAML::Node TerrorSite::saveIdentificator() const
{
	YAML::Node node (Target::save());

	node["type"] = _ruleDeploy->getMarkerType(); // Target::stTarget[3u]
	node["id"]   = _id;

	return node;
}

/**
 * Gets the rule for this TerrorSite's type.
 * @return, pointer to RuleAlienMission
 */
const RuleAlienMission* TerrorSite::getRules() const
{
	return _missionRule;
}

/**
 * Gets the rule for this TerrorSite's deployment.
 * @return, pointer to RuleAlienDeployment rule
 */
const RuleAlienDeployment* TerrorSite::getTerrorDeployed() const
{
	return _ruleDeploy;
}

/**
 * Gets this TerrorSite's uniquely identifying label.
 * @param lang	- pointer to Language to get strings from
 * @param id	- true to show the Id (default true)
 * @return, label
 */
std::wstring TerrorSite::getLabel(
		const Language* const lang,
		bool id) const
{
	if (id == true)
		return lang->getString(_ruleDeploy->getMarkerType()).arg(_id);

	return lang->getString(_ruleDeploy->getMarkerType());
}

/**
 * Gets the globe-marker for this TerrorSite.
 * @return, marker-ID (-1 if not detected)
 */
int TerrorSite::getMarker() const
{
	if (_detected == true)
	{
		const int id (_ruleDeploy->getMarkerIcon());
		if (id != -1) return id;

		return Globe::GLM_TERRORSITE;
	}
	return -1;
}

/**
 * Gets the number of seconds remaining before this TerrorSite expires.
 * @return, seconds remaining
 */
int TerrorSite::getSecondsLeft() const
{
	return _secondsLeft;
}

/**
 * Sets the number of seconds before this TerrorSite expires.
 * @param sec - time in seconds
 */
void TerrorSite::setSecondsLeft(int sec)
{
	_secondsLeft = std::max(0, sec);
}

/**
 * Gets this TerrorSite's battlescape status.
 * @return, true if in the battlescape
 */
bool TerrorSite::getTactical() const
{
	return _tactical;
}

/**
 * Sets this TerrorSite's battlescape status.
 * @param tactical - true if in the battlescape (default true)
 */
void TerrorSite::setTactical(bool tactical)
{
	_tactical = tactical;
}

/**
 * Gets the alien-race currently residing in this TerrorSite.
 * @return, alien-race
 */
std::string TerrorSite::getAlienRace() const
{
	return _race;
}

/**
 * Sets the alien-race currently residing in this TerrorSite.
 * @param race - reference to the alien-race-string
 */
void TerrorSite::setAlienRace(const std::string& race)
{
	_race = race;
}

/**
 * Gets this TerrorSite's terrain-type.
 * @return, terrain-type-string
 */
std::string TerrorSite::getSiteTerrainType() const
{
	return _terrain;
}

/**
 * Sets this TerrorSite's terrain-type.
 * @param terrain - reference to the terrain-type-string
 */
void TerrorSite::setSiteTerrainType(const std::string& terrain)
{
	_terrain = terrain;
}

/**
 * Gets this TerrorSite's associated texture.
 * @return, the texture-ID
 */
int TerrorSite::getSiteTextureId() const
{
	return _texture;
}

/**
 * Sets this TerrorSite's associated texture.
 * @param texture - the texture-ID
 */
void TerrorSite::setSiteTextureId(int texture)
{
	_texture = texture;
}

/**
 * Gets this TerrorSite's associated City if any.
 * @return, string-ID for the city, "" if none
 */
std::string TerrorSite::getCity() const
{
	return _city;
}

/**
 * Sets this TerrorSite's associated City if any.
 * @param city - reference to the string-ID for a city, "" if none
 */
void TerrorSite::setCity(const std::string& city)
{
	_city = city;
}

/**
 * Gets the detection state for this TerrorSite.
 * @note Used for popups of sites spawned directly rather than by UFOs.
 * @return, true if this site has been detected
 */
bool TerrorSite::getDetected() const
{
	return _detected;
}

/**
 * Sets the detection state for this TerrorSite.
 * @param detected - true to show on the geoscape (default true)
 */
void TerrorSite::setDetected(bool detected)
{
	_detected = detected;
}

}
