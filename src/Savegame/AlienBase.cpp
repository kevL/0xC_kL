/*
 * Copyright 2010-2020 OpenXcom Developers.
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

#include "AlienBase.h"

#include "../Engine/Language.h"

#include "../Geoscape/Globe.h" // Globe::GLM_ALIENBASE

#include "../Ruleset/RuleAlienDeployment.h"


namespace OpenXcom
{

/**
 * Initializes the AlienBase.
 */
AlienBase::AlienBase(const RuleAlienDeployment* const ruleDeploy)
	:
		Target(),
		_tactical(false),
		_detected(false),
		_ruleDeploy(ruleDeploy)
{}

/**
 * dTor.
 */
AlienBase::~AlienBase()
{}

/**
 * Loads this AlienBase from a YAML file.
 * @param node - reference a YAML node
 */
void AlienBase::load(const YAML::Node& node)
{
	Target::load(node);

	_id			= node["id"]		.as<int>(_id);
	_race		= node["race"]		.as<std::string>(_race);
	_edit		= node["edit"]		.as<std::string>(_edit);
	_tactical	= node["tactical"]	.as<bool>(_tactical);
	_detected	= node["detected"]	.as<bool>(_detected);
}

/**
 * Saves this AlienBase to a YAML file.
 * @return, YAML node
 */
YAML::Node AlienBase::save() const
{
	YAML::Node node (Target::save());

	node["deployment"] = _ruleDeploy->getType();

	node["id"]		= _id;
	node["race"]	= _race;
	node["edit"]	= _edit;

	if (_tactical == true) node["tactical"] = _tactical;
	if (_detected == true) node["detected"] = _detected;

	return node;
}

/**
 * Saves this AlienBase's identificator to a YAML file.
 * @return, YAML node
 */
YAML::Node AlienBase::saveIdentificator() const
{
	YAML::Node node (Target::save());

	node["type"] = _ruleDeploy->getMarkerType(); // Target::stTarget[2u];
	node["id"]   = _id;

	return node;
}

/**
 * Returns this AlienBase's uniquely identifying label.
 * @param lang	- pointer to Language to get strings from
 * @param id	- true to show the Id (default true)
 * @return, label
 */
std::wstring AlienBase::getLabel(
		const Language* const lang,
		bool id) const
{
	if (id == true)
		return lang->getString(_ruleDeploy->getMarkerType() + "_").arg(_id); //"STR_ALIEN_BASE_"

	return lang->getString(_ruleDeploy->getMarkerType());
}

/**
 * Gets the globe-marker for this AlienBase.
 * @return, marker-ID (-1 if not detected)
 */
int AlienBase::getMarker() const
{
	if (_detected == true)
	{
		const int id (_ruleDeploy->getMarkerIcon());
		if (id != -1) return id;

		return Globe::GLM_ALIENBASE;
	}
	return -1;
}

/**
 * Gets the alien race currently residing in this AlienBase.
 * @return, alien race string
 */
std::string AlienBase::getAlienRace() const
{
	return _race;
}

/**
 * Changes the alien race currently residing in this AlienBase.
 * @param race - reference to alien race string
 */
void AlienBase::setAlienRace(const std::string& race)
{
	_race = race;
}

/**
 * Gets any user-text that the player has entered.
 * @return, user-text
 */
std::string AlienBase::getUserLabel() const
{
	return _edit;
}

/**
 * Sets the user-text field.
 * @param edit - reference to user-text
 */
void AlienBase::setUserLabel(const std::string& edit)
{
	_edit = edit;
}

/**
 * Gets this AlienBase's battlescape status.
 * @return, true if this base is in the battlescape
 */
bool AlienBase::getTactical() const
{
	return _tactical;
}

/**
 * Sets this AlienBase's battlescape status.
 * @param tactical - true if this base is in battle (default true)
 */
void AlienBase::setTactical(bool tactical)
{
	_tactical = tactical;
}

/**
 * Gets this AlienBase's detected status.
 * @return, true if this base has been detected
 */
bool AlienBase::isDetected() const
{
	return _detected;
}

/**
 * Sets this AlienBase's detected status.
 * @param detected - true if this base has been detected (default true)
 */
void AlienBase::setDetected(bool detected)
{
	_detected = detected;
}

/**
 * Gets the rule for this AlienBase's deployment.
 * @return, pointer to RuleAlienDeployment
 */
const RuleAlienDeployment* AlienBase::getAlienBaseDeployed() const
{
	return _ruleDeploy;
}

}
