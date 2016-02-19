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

#include "Vehicle.h"

#include "../Ruleset/RuleItem.h"


namespace OpenXcom
{

/**
 * Initializes the Vehicle from the specified rule.
 * @note This describes a Vehicle that has been loaded onto a Craft only.
 * @param itRule	- pointer to RuleItem
 * @param ammo		- ammo quantity when loaded onto Craft
 * @param quadrants	- size in quadrants
 */
Vehicle::Vehicle(
		const RuleItem* const itRule,
		int ammo,
		int quadrants)
	:
		_itRule(itRule),
		_ammo(ammo),
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
	_ammo		= node["ammo"].as<int>(_ammo);
	_quadrants	= node["size"].as<int>(_quadrants);
}

/**
 * Saves this Vehicle to a YAML file.
 * @return, YAML node
 */
YAML::Node Vehicle::save() const
{
	YAML::Node node;

	node["type"] = _itRule->getType();
	node["ammo"] = _ammo;
	node["size"] = _quadrants;

	return node;
}

/**
 * Returns the rules for this Vehicle's type.
 * @return, pointer to RuleItem
 */
const RuleItem* Vehicle::getRules() const
{
	return _itRule;
}

/**
 * Gets the ammo contained in this Vehicle.
 * @return, quantity of weapon-ammo
 */
int Vehicle::getAmmo() const
{
	if (_ammo != -1)
		return _ammo;

	return 255;
}

/**
 * Sets the ammo contained in this Vehicle.
 * @param ammo - quantity of weapon-ammo
 */
void Vehicle::setAmmo(int ammo)
{
	if (_ammo != -1)
		_ammo = ammo;
}

/**
 * Returns the size occupied by this Vehicle in a transport Craft.
 * @return, size in tiles
 */
int Vehicle::getSize() const
{
	return _quadrants;
}

}
