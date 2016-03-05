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

#include "AlienBase.h"

#include "../Engine/Language.h"

#include "../Geoscape/Globe.h" // Globe::GLM_ALIENBASE


namespace OpenXcom
{

/**
 * Initializes an AlienBase.
 */
AlienBase::AlienBase()
	:
		Target(),
		_id(0),
		_tactical(false),
		_detected(false)
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
	YAML::Node node = Target::save();

	node["id"]		= _id;
	node["race"]	= _race;
	node["edit"]	= _edit;

	if (_tactical == true) node["tactical"] = _tactical;
	if (_detected == true) node["detected"] = _detected;

	return node;
}

/**
 * Saves this AlienBase's unique identifiers to a YAML file.
 * @return, YAML node
 */
YAML::Node AlienBase::saveId() const
{
	YAML::Node node (Target::saveId());

	node["type"] = "STR_ALIEN_BASE";
	node["id"]   = _id;

	return node;
}

/**
 * Returns this AlienBase's unique ID.
 * @return, unique ID
 */
int AlienBase::getId() const
{
	return _id;
}

/**
 * Changes this AlienBase's unique ID.
 * @param id - unique ID
 */
void AlienBase::setId(int id)
{
	_id = id;
}

/**
 * Returns this AlienBase's uniquely identifying name.
 * @param lang - pointer to Language to get strings from
 * @return, full name
 */
std::wstring AlienBase::getName(const Language* const lang) const
{
	return lang->getString("STR_ALIEN_BASE_").arg(_id);
}

/**
 * Returns the globe marker for this AlienBase.
 * @return, marker sprite #7 (-1 if none)
 */
int AlienBase::getMarker() const
{
	if (_detected == true)
		return Globe::GLM_ALIENBASE;

	return -1;
}

/**
 * Returns the alien race currently residing in this AlienBase.
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

}
