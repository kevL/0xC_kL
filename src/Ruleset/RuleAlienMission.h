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

#ifndef OPENXCOM_RULEALIENMISSION_H
#define OPENXCOM_RULEALIENMISSION_H

//#include <map>
//#include <string>
//#include <vector>

#include <yaml-cpp/yaml.h>


namespace OpenXcom
{

class WeightedOptions;


/**
 * @brief Information about a mission wave.
 * @note Mission waves control the UFOs that will be generated during an alien
 * mission.
 */
struct MissionWave
{
	std::string ufoType;	// The type of the spawned UFOs.
	size_t ufoTotal;		// The number of UFOs that will be generated.
							// The UFOs are generated sequentially, one every spawnTimer minutes.
	std::string trajectory;	// The trajectory ID for this wave's UFOs.
							// Trajectories control the way UFOs fly around the Geoscape.
	size_t spawnTimer;		// Number of minutes between UFOs in the wave. Or seconds.
							// The actual value used is (spawnTimer*1/4) or (spawnTimer*3/4.)
	bool objective;			// This wave performs the mission objective.
							// The UFO executes a special action based on its MissionObjective.
};


enum MissionObjective
{
	alm_SCORE,	// 0
	alm_INFILT,	// 1
	alm_BASE,	// 2
	alm_SITE,	// 3
	alm_RETAL,	// 4
	alm_SUPPLY	// 5
};


/**
 * Stores fixed information about an AlienMission.
 * @note This stores the mission-waves and the distribution of alien-races that
 * can undertake the mission based on game-date.
 */
class RuleAlienMission
{

private:
	int
		_points,			// The mission's points.
		_retalCoef;			// Modifier for chance of retaliation.
	size_t _specialZone;	// The mission zone to use for spawning.
	std::string
		_siteType,			// The type of mission-site to generate.
		_specialUfo,		// The UFO to use for spawning.
		_type;				// The mission's type-ID.

	std::vector<MissionWave> _waves;									// The mission's waves.
	std::vector<std::pair<size_t, WeightedOptions*>> _raceDistribution;	// The race distribution over game-time.
	std::map<size_t, int> _weights;										// The mission's weights.

	MissionObjective _objective; // The mission's objective type.


	public:
		/// Creates an AlienMission rule.
		explicit RuleAlienMission(const std::string& type);
		/// Releases all resources held by the mission.
		~RuleAlienMission();

		/// Loads AlienMission data from YAML.
		void load(const YAML::Node& node);

		/// Gets a race based on the game-time and the racial distribution.
		std::string generateRace(size_t monthsPassed) const;
		/// Gets the most likely race based on the game-time and the racial distribution.
//		std::string getTopRace(size_t monthsPassed) const;

		/// Gets the mission's type-ID.
		const std::string& getType() const
		{ return _type; }

		/// Gets the number of waves.
		size_t getWaveTotal() const
		{ return _waves.size(); }
		/// Gets the full wave information.
		const MissionWave& getWave(size_t index) const
		{ return _waves[index]; }

		/// Gets the score for this mission.
		int getPoints() const;

		/// Gets the objective for this mission.
		MissionObjective getObjective() const
		{ return _objective; }

		/// Gets the UFO type for special spawns.
		const std::string& getSpawnUfo() const
		{ return _specialUfo; }
		/// Gets the zone for spawning a site or base.
		size_t getSpawnZone() const
		{ return _specialZone; }

		/// Gets the chances of this mission based on the game-time.
		int getWeight(size_t monthsPassed) const;

		/// Gets the basic odds of this mission spawning a retaliation mission.
		int getRetaliation() const;

		/// The type of missionSite to spawn if any.
		const std::string& getSiteType() const
		{ return _siteType; }
};

}

#endif
