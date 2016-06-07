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

#ifndef OPENXCOM_TARGET_H
#define OPENXCOM_TARGET_H

#include <string>
#include <vector>

#include <yaml-cpp/yaml.h>


namespace OpenXcom
{

class Language;


/**
 * Parent class for targets on the globe with a set of radian coordinates.
 */
class Target
{

protected:
	double
		_lat,
		_lon;

	std::vector<Target*> _targeters;

	/// Creates a Target.
	Target();


	public:
		/// Cleans up the Target.
		virtual ~Target();

		/// Loads the Target from YAML.
		virtual void load(const YAML::Node& node);
		/// Saves the Target to YAML.
		virtual YAML::Node save() const;
		/// Saves the Target's unique-ID to YAML.
		virtual YAML::Node saveId() const = 0;

		/// Gets the Target's longitude.
		double getLongitude() const;
		/// Sets the Target's longitude.
		void setLongitude(double lon);
		/// Gets the Target's latitude.
		double getLatitude() const;
		/// Sets the Target's latitude.
		void setLatitude(double lat);

		/// Gets the Target's name.
		virtual std::wstring getName(const Language* const lang) const = 0;

		/// Gets the Target's marker.
		virtual int getMarker() const = 0;

		/// Gets the Target's targeters.
		std::vector<Target*>* getTargeters();

		/// Gets the distance to another Target.
		double getDistance(const Target* const target) const;
};

}

#endif
