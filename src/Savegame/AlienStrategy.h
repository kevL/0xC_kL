/*
 * Copyright 2010-2018 OpenXcom Developers.
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

#ifndef OPENXCOM_ALIENSTRATEGY_H
#define OPENXCOM_ALIENSTRATEGY_H

//#include <yaml-cpp/yaml.h>

#include "WeightedOptions.h"


namespace OpenXcom
{

class Ruleset;


/**
 * Stores the information about aLien Strategy.
 */
class AlienStrategy
{

private:
	WeightedOptions _regionChances;								// The chances of each region-type to be targeted for a mission.
	std::map<std::string, WeightedOptions*> _regionMissions;	// The chances of each mission-type for each region.

	std::map<std::string, int> _missionRuns;
	std::map<std::string, std::vector<std::pair<std::string, size_t>>> _missionLocations;

	/// Disable copy and assignments.
	AlienStrategy(const AlienStrategy&);
	///
	AlienStrategy& operator= (const AlienStrategy&);


	public:
		/// Creates an AlienStrategy with no data.
		AlienStrategy();
		/// Frees resources used by the AlienStrategy.
		~AlienStrategy();

		/// Initializes values according to the rules.
		void init(const Ruleset* const rules);

		/// Loads the data from YAML.
		void load(const YAML::Node& node);
		/// Saves the requisite data to YAML.
		YAML::Node save() const;

		/// Chooses a random region for a regular mission.
		std::string chooseRegion(const Ruleset* const rules);
		/// Chooses a random mission for a region.
		std::string chooseMission(const std::string& regionType) const;

		/// Removes a region and mission from the list of possibilities.
		bool clearRegion(
				const std::string& regionType,
				const std::string& missionType);

		/// Gets the quantity of a specified mission-type that have run.
		int getMissionsRun(const std::string& missionType);
		/// Increments the quantity of a specified mission-type that have run.
		void addMissionRun(const std::string& missionType);
		/// Adds a mission's location to the cache.
		void addMissionLocation(
				const std::string& missionType,
				const std::string& regionType,
				size_t zone,
				size_t track);
		/// Checks if a specified mission-location has been attacked already.
		bool validateMissionLocation(
				const std::string& missionType,
				const std::string& regionType,
				size_t zone);
		/// Checks if a specified region appears in the strategy table.
		bool validateMissionRegion(const std::string& regionType) const;
};

}

#endif
