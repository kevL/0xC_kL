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

#ifndef OPENXCOM_MISSIONSITE_H
#define OPENXCOM_MISSIONSITE_H

//#include <string>
//#include <yaml-cpp/yaml.h>

#include "Target.h"


namespace OpenXcom
{

class AlienDeployment;
class RuleAlienMission;


/**
 * Represents an alien MissionSite in the world.
 */
class MissionSite final
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

	const AlienDeployment* _ruleDeploy;
	const RuleAlienMission* _missionRule;


	public:
		/// Creates a MissionSite.
		MissionSite(
				const RuleAlienMission* const missionRule,
				const AlienDeployment* const ruleDeploy);
		/// Cleans up the MissionSite.
		~MissionSite();

		/// Loads the MissionSite from YAML.
		void load(const YAML::Node& node) override;
		/// Saves the MissionSite to YAML.
		YAML::Node save() const override;
		/// Saves the MissionSite's unique-ID to YAML.
		YAML::Node saveId() const override;

		/// Gets the MissionSite's ruleset.
		const RuleAlienMission* getRules() const;
		/// Gets the MissionSite's deployment.
		const AlienDeployment* getSiteDeployment() const;

		/// Gets the MissionSite's ID.
		int getId() const;
		/// Sets the MissionSite's ID.
		void setId(const int id);

		/// Gets the MissionSite's name.
		std::wstring getName(const Language* const lang) const override;

		/// Gets the MissionSite site's marker.
		int getMarker() const override;

		/// Gets the seconds until the MissionSite expires.
		int getSecondsLeft() const;
		/// Sets the seconds until the MissionSite expires.
		void setSecondsLeft(int sec);

		/// Sets the MissionSite's battlescape status.
		void setTactical(bool tactical = true);
		/// Gets if the MissionSite is in battlescape.
		bool getTactical() const;

		/// Gets the MissionSite's alien race.
		std::string getAlienRace() const;
		/// Sets the MissionSite's alien race.
		void setAlienRace(const std::string& race);

		/// Gets the MissionSite's terrainType.
		std::string getSiteTerrainType() const;
		/// Sets the MissionSite's terrainType.
		void setSiteTerrainType(const std::string& terrain);

		/// Gets the MissionSite's texture.
		int getSiteTextureId() const;
		/// Sets the MissionSite's texture.
		void setSiteTextureId(int texture);

		/// Gets the MissionSite's city.
		std::string getCity() const;
		/// Sets the MissionSite's city.
		void setCity(const std::string& city);

		/// Gets the MissionSite's detected state.
		bool getDetected() const;
		/// Sets the MissionSite's detected state.
		void setDetected(bool detected = true);
};

}

#endif
