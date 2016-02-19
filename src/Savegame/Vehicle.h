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

#ifndef OPENXCOM_VEHICLE_H
#define OPENXCOM_VEHICLE_H

#include <yaml-cpp/yaml.h>


namespace OpenXcom
{

class RuleItem;

/**
 * Represents a Vehicle (tanks, dogs, etc.) kept onboard a Craft.
 * @note Contains variable info about a Vehicle like ammo.
 * @sa RuleItem
 */
class Vehicle
{

private:

	int
		_ammo,
		_quadrants;

	const RuleItem* const _itRule;


	public:

		/// Creates a Vehicle of the specified type.
		Vehicle(
				const RuleItem* const itRule,
				int ammo,
				int quadrants);
		/// Cleans up the Vehicle.
		~Vehicle();

		/// Loads the Vehicle from YAML.
		void load(const YAML::Node& node);
		/// Saves the Vehicle to YAML.
		YAML::Node save() const;

		/// Gets the Vehicle's rules.
		const RuleItem* getRules() const;

		/// Gets the Vehicle's ammo.
		int getAmmo() const;
		/// Sets the Vehicle's ammo.
		void setAmmo(int ammo);

		/// Gets the Vehicle's size.
		int getSize() const;
};

}

#endif
