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
 * @note Contains variable info about a Vehicle like load.
 * @sa RuleItem
 */
class Vehicle
{

private:

	int
		_load,
		_quadrants;

	const RuleItem* const _itRule;


	public:

		/// Instantiates a Vehicle from the specified RuleItem.
		Vehicle(
				const RuleItem* const itRule,
				int load,
				int quadrants);
		/// Cleans up the Vehicle.
		~Vehicle();

		/// Loads the Vehicle from YAML.
		void load(const YAML::Node& node);
		/// Saves the Vehicle to YAML.
		YAML::Node save() const;

		/// Gets the RuleItem for the Vehicle.
		const RuleItem* getRules() const;

		/// Gets the Vehicle's load.
		int getLoad() const;
		/// Sets the Vehicle's load.
//		void setAmmo(int load);

		/// Gets the Vehicle's size in quadrants.
		int getQuads() const;
};

}

#endif
