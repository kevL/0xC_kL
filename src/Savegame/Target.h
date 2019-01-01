/*
 * Copyright 2010-2019 OpenXcom Developers.
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
 * Parent class for Targets on the globe with a set of radian coordinates.
 */
class Target
{

protected:
	int _id;	// NOTE: This is required by AlienBase, TerrorSite, Craft, Ufo, Waypoint;
				// Base always uses 0. RuleCity needs none.
	double
		_lat,
		_lon;

	std::vector<Target*> _targeters;

	/// Creates a Target.
	Target();


	public:
		static const char* const stTarget[8u];

		/// Cleans up the Target.
		virtual ~Target();

		/// Loads the Target from YAML.
		virtual void load(const YAML::Node& node);
		/// Saves the Target to YAML.
		virtual YAML::Node save() const;
		/// Saves the Target's identificator to YAML.
		virtual YAML::Node saveIdentificator() const = 0;

		/// Sets the Target's ID.
		void setId(int id);
		/// Gets the Target's ID.
		int getId() const;

		/// Gets the Target's longitude.
		double getLongitude() const;
		/// Sets the Target's longitude.
		void setLongitude(double lon);
		/// Gets the Target's latitude.
		double getLatitude() const;
		/// Sets the Target's latitude.
		void setLatitude(double lat);

		/// Gets the Target's label.
		virtual std::wstring getLabel(
				const Language* const lang,
				bool id = true) const = 0;

		/// Gets the Target's globe-marker.
		virtual int getMarker() const = 0;

		/// Gets the Target's targeters.
		std::vector<Target*>* getTargeters();

		/// Gets a radian-angle to another Target.
		double getDistance(const Target* const target) const;
};

}

#endif
