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

#ifndef OPENXCOM_TERRORSITE_H
#define OPENXCOM_TERRORSITE_H

//#include <string>
//#include <yaml-cpp/yaml.h>

#include "Target.h"


namespace OpenXcom
{

class RuleAlienDeployment;
class RuleAlienMission;


/**
 * Represents an alien TerrorSite in the world.
 */
class TerrorSite final
	:
		public Target
{

private:
	bool
		_detected,
		_tactical;
	int
		_id,
		_secondsLeft,
		_texture;
	std::string
		_city,
		_race,
		_terrain;

	const RuleAlienDeployment* _ruleDeploy;
	const RuleAlienMission* _missionRule;


	public:
		/// Creates a TerrorSite.
		TerrorSite(
				const RuleAlienMission* const missionRule,
				const RuleAlienDeployment* const ruleDeploy);
		/// Cleans up the TerrorSite.
		~TerrorSite();

		/// Loads the TerrorSite from YAML.
		void load(const YAML::Node& node) override;
		/// Saves the TerrorSite to YAML.
		YAML::Node save() const override;
		/// Saves the TerrorSite's unique-ID to YAML.
		YAML::Node saveId() const override;

		/// Gets the TerrorSite's ruleset.
		const RuleAlienMission* getRules() const;
		/// Gets the TerrorSite's deployment.
		const RuleAlienDeployment* getSiteDeployment() const;

		/// Gets the TerrorSite's ID.
		int getId() const;
		/// Sets the TerrorSite's ID.
		void setId(const int id);

		/// Gets the TerrorSite's name.
		std::wstring getName(const Language* const lang) const override;

		/// Gets the TerrorSite site's marker.
		int getMarker() const override;

		/// Gets the seconds until the TerrorSite expires.
		int getSecondsLeft() const;
		/// Sets the seconds until the TerrorSite expires.
		void setSecondsLeft(int sec);

		/// Sets the TerrorSite's battlescape status.
		void setTactical(bool tactical = true);
		/// Gets if the TerrorSite is in battlescape.
		bool getTactical() const;

		/// Gets the TerrorSite's alien race.
		std::string getAlienRace() const;
		/// Sets the TerrorSite's alien race.
		void setAlienRace(const std::string& race);

		/// Gets the TerrorSite's terrainType.
		std::string getSiteTerrainType() const;
		/// Sets the TerrorSite's terrainType.
		void setSiteTerrainType(const std::string& terrain);

		/// Gets the TerrorSite's texture.
		int getSiteTextureId() const;
		/// Sets the TerrorSite's texture.
		void setSiteTextureId(int texture);

		/// Gets the TerrorSite's city.
		std::string getCity() const;
		/// Sets the TerrorSite's city.
		void setCity(const std::string& city);

		/// Gets the TerrorSite's detected state.
		bool getDetected() const;
		/// Sets the TerrorSite's detected state.
		void setDetected(bool detected = true);
};

}

#endif
