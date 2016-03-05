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

#ifndef OPENXCOM_WAYPOINT_H
#define OPENXCOM_WAYPOINT_H

#include "Target.h"

//#include <string>
//#include <yaml-cpp/yaml.h>


namespace OpenXcom
{

/**
 * Represents a fixed Waypoint on the world.
 */
class Waypoint final
	:
		public Target
{

private:
	int _id;


	public:
		/// Creates a Waypoint.
		Waypoint();
		/// Cleans up the Waypoint.
		~Waypoint();

		/// Loads the Waypoint from YAML.
		void load(const YAML::Node& node) override;
		/// Saves the Waypoint to YAML.
		YAML::Node save() const override;
		/// Saves the Waypoint's unique-ID to YAML.
		YAML::Node saveId() const override;

		/// Gets the Waypoint's ID.
		int getId() const;
		/// Sets the Waypoint's ID.
		void setId(int id);

		/// Gets the Waypoint's name.
		std::wstring getName(const Language* const lang) const override;

		/// Gets the Waypoint's marker.
		int getMarker() const override;
};

}

#endif
