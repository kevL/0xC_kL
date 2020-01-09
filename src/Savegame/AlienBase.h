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

#ifndef OPENXCOM_ALIENBASE_H
#define OPENXCOM_ALIENBASE_H

//#include <string>
//#include <yaml-cpp/yaml.h>

#include "Target.h"


namespace OpenXcom
{

class RuleAlienDeployment;


/**
 * Represents an AlienBase on the Globe.
 */
class AlienBase final
	:
		public Target
{

private:
	bool
		_detected,
		_tactical;

	std::string
		_edit,
		_race;

	const RuleAlienDeployment* _ruleDeploy;


	public:
		/// Creates an AlienBase.
		explicit AlienBase(const RuleAlienDeployment* const ruleDeploy);
		/// Cleans up the AlienBase.
		~AlienBase();

		/// Loads the AlienBase from YAML.
		void load(const YAML::Node& node) override;
		/// Saves the AlienBase to YAML.
		YAML::Node save() const override;
		/// Saves the AlienBase's identificator to YAML.
		YAML::Node saveIdentificator() const override;

		/// Gets the AlienBase's label.
		std::wstring getLabel(
				const Language* const lang,
				bool id = true) const override;
		/// Gets the AlienBase's globe-marker.
		int getMarker() const override;

		/// Gets the AlienBase's alien race.
		std::string getAlienRace() const;
		/// Sets the AlienBase's alien race.
		void setAlienRace(const std::string& race);

		/// Returns textedit that the player has entered.
		std::string getUserLabel() const;
		/// Changes textedit that the player has entered.
		void setUserLabel(const std::string& edit);

		/// Sets the AlienBase's battlescape status.
		void setTactical(bool tactical = true);
		/// Gets the AlienBase's battlescape status.
		bool getTactical() const;

		/// Gets the AlienBase's detected status.
		bool isDetected() const;
		/// Sets the AlienBase's detected status.
		void setDetected(bool detected = true);

		/// Gets the AlienBase's deployment-rule.
		const RuleAlienDeployment* getAlienBaseDeployed() const;
};

}

#endif
