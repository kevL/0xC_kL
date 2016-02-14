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

#ifndef OPENXCOM_ALIENBASE_H
#define OPENXCOM_ALIENBASE_H

//#include <string>
//#include <yaml-cpp/yaml.h>

#include "Target.h"


namespace OpenXcom
{

/**
 * Represents an AlienBase on the globe.
 */
class AlienBase final
	:
		public Target
{

private:
	bool
		_detected,
		_tactical;
	int _id;

	std::string
		_edit,
		_race;


	public:
		/// Creates an AlienBase.
		AlienBase();
		/// Cleans up the AlienBase.
		~AlienBase();

		/// Loads the AlienBase from YAML.
		void load(const YAML::Node& node) override;
		/// Saves the AlienBase to YAML.
		YAML::Node save() const override;
		/// Saves the AlienBase's ID to YAML.
		YAML::Node saveId() const override;

		/// Gets the AlienBase's ID.
		int getId() const;
		/// Sets the AlienBase's ID.
		void setId(int id);

		/// Gets the AlienBase's name.
		std::wstring getName(const Language* const lang) const override;
		/// Gets the AlienBase's marker.
		int getMarker() const override;

		/// Gets the AlienBase's alien race.
		std::string getAlienRace() const;
		/// Sets the AlienBase's alien race.
		void setAlienRace(const std::string& race);

		/// Returns textedit that the player has entered.
		std::string getLabel() const;
		/// Changes textedit that the player has entered.
		void setLabel(const std::string& edit);

		/// Sets the AlienBase's battlescape status.
		void setTactical(bool tactical = true);
		/// Gets the AlienBase's battlescape status.
		bool getTactical() const;

		/// Gets the AlienBase's detected status.
		bool isDetected() const;
		/// Sets the AlienBase's detected status.
		void setDetected(bool detected = true);
};

}

#endif
