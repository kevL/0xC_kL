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

#include "Waypoint.h"

#include "../Engine/Language.h"

#include "../Geoscape/Globe.h" // Globe::GLM_WAYPOINT


namespace OpenXcom
{

/**
 * Initializes the Waypoint.
 */
Waypoint::Waypoint()
	:
		Target()
{}

/**
 * dTor.
 */
Waypoint::~Waypoint()
{}

/**
 * Loads this Waypoint from a YAML file.
 * @param node - reference a YAML node
 */
void Waypoint::load(const YAML::Node& node)
{
	Target::load(node);

	_id = node["id"].as<int>(_id);
}

/**
 * Saves this Waypoint to a YAML file.
 * @return, YAML node
 */
YAML::Node Waypoint::save() const
{
	YAML::Node node	(Target::save());

	node["id"] = _id;

	return node;
}

/**
 * Saves this Waypoint's unique-ID to a YAML file.
 * @return, YAML node
 */
YAML::Node Waypoint::saveIdentificator() const
{
	YAML::Node node (Target::save());

	node["type"] = Target::stTarget[4u];
	node["id"]   = _id;

	return node;
}

/**
 * Returns this Waypoint's unique identifying label.
 * @param lang	- pointer to Language to get strings from
 * @param id	- true to show the Id (default true)
 * @return, label
 */
std::wstring Waypoint::getLabel(
		const Language* const lang,
		bool id) const
{
	if (id == true)
		return lang->getString("STR_WAY_POINT_").arg(_id);

	return lang->getString("STR_WAY_POINT");
}

/**
 * Gets the globe-marker for this Waypoint.
 * @return, marker-ID
 */
int Waypoint::getMarker() const
{
	return Globe::GLM_WAYPOINT;
}

}
