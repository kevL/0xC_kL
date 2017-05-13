/*
 * Copyright 2010-2017 OpenXcom Developers.
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

#ifndef OPENXCOM_ALIEN_MISSION_H
#define OPENXCOM_ALIEN_MISSION_H

//#include <string>

#include <yaml-cpp/yaml.h>

#include "../Ruleset/RuleUfo.h"


namespace OpenXcom
{

class AlienBase;
class Game;
class Globe;
class TerrorSite;
class RuleAlienDeployment;
class RuleAlienMission;
class RuleRegion;
class Ruleset;
class SavedGame;
class Ufo;
class UfoTrajectory;

struct MissionArea;
struct MissionWave;


/**
 * Represents an ongoing AlienMission.
 * @note Contains variable info about the mission such as spawn-counter and
 * target-region and current-wave.
 * @sa RuleAlienMission
 */
class AlienMission
{

private:
	const int MAX_TRIES = 1000;

	bool _success;	// Prevents infiltration missions taking multiple success points
					// and creating multiple bases on the final 2xBattleship wave.
	int
		_countdown,
		_id,
		_liveUfos,
		_ufoCount;
	size_t
		_waveCount,
		_terrorZone;

	std::string
		_raceType,
		_regionType;

	const AlienBase* _alienBase;
	const RuleAlienMission& _missionRule;
	SavedGame& _playSave;

	/// Gets a standard wait.
	int getStandardDelay() const;
	/// Calculates time remaining until the next wave generates.
	void initiateCountdown(size_t waveId);

	/// Spawns a UFO based on the mission rules.
	Ufo* createUfo(
			const Ruleset& rules,
			const Globe& globe,
			const MissionWave& wave,
			const UfoTrajectory& trj);
	/// Spawns a TerrorSite at a specific location.
	TerrorSite* createTerror(
			const RuleAlienDeployment* const ruleDeploy,
			const MissionArea& area);
	/// Spawns an AlienBase
	void createAlienBase(
			const Globe& globe,
			const Ruleset& rules);

	/// Generates destination-coordinates for a Waypoint.
	std::pair<double, double> coordsWaypoint(
			const UfoTrajectory& trj,
			const size_t pointId,
			const Globe& globe,
			const RuleRegion& region) const;
	/// Generates destination-coordinates inside a specified Region and MissionZone.
	std::pair<double, double> coordsLandZone(
			const Globe& globe,
			const RuleRegion& region,
			const size_t zoneId) const;
	/// Generates destination-coordinates inside a specified Region and MissionArea.
	std::pair<double, double> coordsLandArea(
			const Globe& globe,
			const RuleRegion& region,
			const MissionArea& area) const;

	/// Handles points for mission success.
	void addScore(
			const double lon,
			const double lat,
			const UfoSizeType ufoSize = UFO_VERYLARGE) const;


	public:
		/// Creates an AlienMission of the specified type.
		AlienMission(
				const RuleAlienMission& missionRule,
				SavedGame& playSave);
		/// Cleans up the AlienMission.
		~AlienMission();

		/// Loads the AlienMission from YAML.
		void load(const YAML::Node& node);
		/// Saves the AlienMission to YAML.
		YAML::Node save() const;

		/// Gets the AlienMission's ruleset.
		const RuleAlienMission& getRules() const
		{ return _missionRule; }

		/// Sets an ID for the AlienMission.
		void setId(int id);
		/// Gets the ID of the AlienMission.
		int getId() const;

		/// Gets the AlienMission's Region.
		const std::string& getRegion() const
		{ return _regionType; }
		/// Sets the AlienMission's Region.
		void setRegion(
				const std::string& region,
				const Ruleset& rules);

		/// Gets the AlienMission's RuleAlienRace type.
		const std::string& getRace() const
		{ return _raceType; }
		/// Sets the AlienMission's RuleAlienRace type.
		void setRace(const std::string& race)
		{ _raceType = race; }

		/// Gets the minutes until the next wave spawns.
//		size_t getWaveCountdown() const
//		{ return _countdown; }
		/// Sets the minutes until the next wave spawns.
		void resetCountdown();

		/// Gets the wave of a UFO in the agenda of the AlienMission.
		size_t getWaveCount()
		{ return _waveCount; }

		/// Increases the quantity of live UFOs.
		void incrLiveUfos()
		{ ++_liveUfos; }
		/// Decreases the quantity of live UFOs.
		void decrLiveUfos()
		{ --_liveUfos; }

		/// Gets if the AlienMission over.
		bool isOver() const;
		/// Initializes the AlienMission with rule-data.
		void start(int countdown = 0);

		/// Handles UFO spawning for the AlienMission.
		void think(
				const Game& game,
				const Globe& globe);

		/// Handles a UFO reaching a waypoint.
		void ufoReachedWaypoint(
				Ufo& ufo,
				const Ruleset& rules,
				const Globe& globe);

		/// Gets the AlienBase for the AlienMission.
		const AlienBase* getAlienBase() const;
		/// Sets the AlienBase for the AlienMission.
		void setAlienBase(const AlienBase* const alienBase = nullptr);

		/// Handles a UFO lifting from the ground.
		void ufoLifting(
				Ufo& ufo,
				Ruleset& rules,
				const Globe& globe);
		/// Handles a UFO that was shot down.
		void ufoShotDown(const Ufo& ufo);

		/// Flags tracking of the target-site.
		void setTerrorZone(size_t zoneId);
};

}

#endif
