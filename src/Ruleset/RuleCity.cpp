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

#include "RuleCity.h"

#include <cmath>

#include "../Engine/Language.h"

#include "../Geoscape/Globe.h" // Globe::GLM_CITY


namespace OpenXcom
{

/**
 * Instantiates a rule for a City.
 * @note A City is a 1-pixel MissionArea within a MissionZone as defined in
 * RuleRegion.
 */
RuleCity::RuleCity()
	:
		Target(),
		_texture(-1),
		_zoomLevel(4u),
		_labelTop(false)
{}

/**
 * dTor.
 */
RuleCity::~RuleCity()
{}

/**
 * Loads the rule from a YAML file.
 * @param node - reference a YAML node
 */
void RuleCity::load(const YAML::Node& node)
{
	_lon		= node["lon"]		.as<double>(_lon) * M_PI / 180.; // radians
	_lat		= node["lat"]		.as<double>(_lat) * M_PI / 180.;
	_texture	= node["texture"]	.as<int>(_texture);
	_name		= node["name"]		.as<std::string>(_name);
	_zoomLevel	= node["zoomLevel"]	.as<size_t>(_zoomLevel);
	_labelTop	= node["labelTop"]	.as<bool>(_labelTop);

	// iDea: _hidden, marker -1 etc.
	// add _zoneType (to specify the missionZone category 0..5+ that City is part of)
}

/**
 * Returns this RuleCity's name as seen on the Globe.
 * @param lang - pointer to Language to get strings from
 * @return, a city's IG name
 */
std::wstring RuleCity::getName(const Language* const lang) const
{
	return lang->getString(_name);
}

/**
 * Returns this RuleCity's name as a raw string.
 * @return, a city's ID string
 */
const std::string& RuleCity::getName() const
{
	return _name;
}

/**
 * Gets the globe-marker for this RuleCity.
 * @return, marker-sprite #8
 */
int RuleCity::getMarker() const
{
	return Globe::GLM_CITY;
}

/**
 * Gets the minimal zoom-level that is required to show name of this RuleCity.
 * @return, minimum zoom-level
 */
size_t RuleCity::getZoomLevel() const
{
	return _zoomLevel;
}

/**
 * Gets if this RuleCity's label is to be positioned above or below its marker.
 * @return, true if label goes on top
 */
bool RuleCity::getLabelTop() const
{
	return _labelTop;
}

/**
 * Gets the texture of this RuleCity for tactical.
 * @return, texture-ID
 */
int RuleCity::getTextureId() const
{
	return _texture;
}

}
