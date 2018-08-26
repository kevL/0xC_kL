/*
 * Copyright 2010-2018 OpenXcom Developers.
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

#include "Vehicle.h"

#include "../Ruleset/RuleItem.h"


namespace OpenXcom
{

/**
 * Instantiates the Vehicle from a specified RuleItem.
 * @note This describes a Vehicle that has been loaded onto a Craft only.
 * @param itRule	- pointer to RuleItem
 * @param load		- quantity of ammunition when loaded onto a Craft
 * @param quadrants	- size in quadrants
 */
Vehicle::Vehicle(
		const RuleItem* const itRule,
		int load,
		int quadrants)
	:
		_itRule(itRule),
		_load(load),
		_quadrants(quadrants)
{}

/**
 * dTor.
 */
Vehicle::~Vehicle()
{}

/**
 * Loads this Vehicle from a YAML file.
 * @param node - reference a YAML node
 */
void Vehicle::load(const YAML::Node& node)
{
	_load      = node["load"].as<int>(_load);
	_quadrants = node["size"].as<int>(_quadrants);
}

/**
 * Saves this Vehicle to a YAML file.
 * @return, YAML node
 */
YAML::Node Vehicle::save() const
{
	YAML::Node node;

	node["type"] = _itRule->getType();
	node["load"] = _load;
	node["size"] = _quadrants;

	return node;
}

/**
 * Gets the RuleItem for the Vehicle.
 * @return, pointer to RuleItem
 */
const RuleItem* Vehicle::getRules() const
{
	return _itRule;
}

/**
 * Gets the load contained in this Vehicle.
 * @return, quantity of weapon-load
 */
int Vehicle::getLoad() const
{
	return _load;
}

/**
 * Sets the load contained in this Vehicle.
 * @param load - quantity of weapon-load
 *
void Vehicle::setAmmo(int load)
{
	if (_load != -1) _load = load;
} */

/**
 * Gets the size in quadrants that this Vehicle occupies in a transport Craft.
 * @return, size in tiles/quadrants
 */
int Vehicle::getQuads() const
{
	return _quadrants;
}

}
