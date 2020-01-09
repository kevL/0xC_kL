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

	public:
		/// Creates a Waypoint.
		Waypoint();
		/// Cleans up the Waypoint.
		~Waypoint();

		/// Loads the Waypoint from YAML.
		void load(const YAML::Node& node) override;
		/// Saves the Waypoint to YAML.
		YAML::Node save() const override;
		/// Saves the Waypoint's identificator to YAML.
		YAML::Node saveIdentificator() const override;

		/// Gets the Waypoint's label.
		std::wstring getLabel(
				const Language* const lang,
				bool id = true) const override;

		/// Gets the Waypoint's globe-marker.
		int getMarker() const override;
};

}

#endif
