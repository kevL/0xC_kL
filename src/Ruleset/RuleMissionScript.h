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

#ifndef OPENXCOM_RULEMISSIONSCRIPT_H
#define OPENXCOM_RULEMISSIONSCRIPT_H

//#include <map>
//#include <string>
//#include <vector>

#include <yaml-cpp/yaml.h>

#include "../Savegame/SavedGame.h"
#include "../Savegame/WeightedOptions.h"


namespace OpenXcom
{

enum GenerationType
{
	GT_REGION,	// 0
	GT_MISSION,	// 1
	GT_RACE		// 2
};

class WeightedOptions;


/**
 * For so-called scripted missions.
 */
class RuleMissionScript
{

private:
	bool
		_siteType,
		_useTable;
	int
		_avoidRepeats,
		_delay,
		_executionOdds,
		_firstMonth,
		_label,
		_lastMonth,
		_maxRuns,
		_targetBaseOdds;

	std::string
		_type,
		_varType;

	std::vector<int> _conditions;

	std::vector<std::pair<size_t, WeightedOptions*>>
		_missionWeights,
		_raceWeights,
		_regionWeights;

	std::map<std::string, bool> _researchTriggers;

	DifficultyLevel _minDifficulty;


	public:
		/// Creates a MissionScript.
		explicit RuleMissionScript(const std::string& type);
		/// Deletes the MissionScript.
		~RuleMissionScript();

		/// Loads a MissionScript from YAML.
		void load(const YAML::Node& node);

		/// Gets the type of the MissionScript.
		std::string getType() const;
		/// Gets the variable used to track stuff in the SavedGame.
		std::string getVarType() const;

		/// Gets the first month the MissionScript is allowed to execute.
		int getFirstMonth() const;
		/// Gets the last month the MissionScript is allowed to execute.
		int getLastMonth() const;

		/// Gets the label of the MissionScript for its conditions.
		int getLabel() const;

		/// Gets the odds of the MissionScript executing.
		int getExecutionOdds() const;
		/// Gets the odds of the MissionScript targeting an xCom Base.
		int getTargetBaseOdds() const;

		/// Gets the minimum game-difficulty the MissionScript is allowed to execute at.
		DifficultyLevel getMinDifficulty() const;

		/// Gets the maximum number of times a directive with the MissionScript's varType is allowed to execute.
		int getMaxRuns() const;
		/// Gets the quantity of previous mission-sites to track.
		int getRepeatAvoidance() const;
		/// Gets the quantity of minutes to delay spawning a first wave.
		int getDelay() const;

		/// Gets a list of conditions the MissionScript requires before it is allowed to execute.
		const std::vector<int>& getConditions() const;

		/// Checks if the MissionScript has race-weights.
		bool hasRaceWeights() const;
		/// Checks if the MissionScript has mission-weights.
		bool hasMissionWeights() const;
		/// Checks if the MissionScript has region-weights.
		bool hasRegionWeights() const;

		/// Gets any research triggers that apply to the MissionScript.
		const std::map<std::string, bool>& getResearchTriggers() const;

		/// Checks if the MissionScript uses the "table".
		bool usesTable() const;

		/// Gets the list of all mission-types contained within the MissionScript.
		const std::set<std::string> getAllMissionTypes() const;
		/// Gets a list of mission-types in this MissionScript's mission-weights for a given month.
		const std::vector<std::string> getMissionTypes(const size_t month) const;
		/// Gets a list of the Regions in this MissionScript's region-weights for a given month.
		const std::vector<std::string> getRegions(const size_t month) const;

		/// Generates a region, a mission, or a race based on the current month.
		std::string genDataType(
				const size_t monthsPassed,
				const GenerationType type) const;

		/// Sets the MissionScript to a site-mission type directive or not.
		void setSiteType(const bool siteType);
		/// Gets if the MissionScript is a site-mission type or not.
		bool getSiteType() const;
};

}

#endif
