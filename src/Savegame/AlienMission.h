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

#ifndef OPENXCOM_ALIEN_MISSION_H
#define OPENXCOM_ALIEN_MISSION_H

//#include <string>

#include <yaml-cpp/yaml.h>


namespace OpenXcom
{

class AlienBase;
class AlienDeployment;
class Game;
class Globe;
class TerrorSite;
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
	bool _success;	// Prevents infiltration missions taking multiple success points
					// and creating multiple bases on the final 2xBattleship wave.
	int
		_id,
		_liveUfos,
		_spawnTime,
		_ufoCount;
	size_t
		_waveCount,
		_siteZone;

	std::string
		_race,
		_region;

	const AlienBase* _aBase;
	const RuleAlienMission& _missionRule;
	SavedGame& _gameSave;

	/// Calculates time remaining until the next wave generates.
	void calcGeneration(size_t waveId);

	/// Spawns a UFO based on the mission rules.
	Ufo* createUfo(
			const Ruleset& rules,
			const Globe& globe,
			const MissionWave& wave,
			const UfoTrajectory& trajectory);
	/// Spawns a TerrorSite at a specific location.
	TerrorSite* createTerror(
			const AlienDeployment* const deployment,
			const MissionArea& area);
	/// Spawns an AlienBase
	void createAlienBase(
			const Globe& globe,
			const Ruleset& rules,
			const size_t zone);

	/// Generates destination-coordinates for a Waypoint.
	std::pair<double, double> coordsWaypoint(
			const UfoTrajectory& trajectory,
			const size_t wpId,
			const Globe& globe,
			const RuleRegion& region);
	/// Generates destination-coordinates inside a specified Region and zone.
	std::pair<double, double> coordsLand(
			const Globe& globe,
			const RuleRegion& region,
			const size_t zone);

	/// Handles Points for mission success.
	void addScore(
			const double lon,
			const double lat) const;


	public:
		/// Creates an AlienMission of the specified type.
		AlienMission(
				const RuleAlienMission& missionRule,
				SavedGame& gameSave);
		/// Cleans up the AlienMission.
		~AlienMission();

		/// Loads the AlienMission from YAML.
		void load(const YAML::Node& node);
		/// Saves the AlienMission to YAML.
		YAML::Node save() const;

		/// Gets the AlienMission's ruleset.
		const RuleAlienMission& getRules() const
		{ return _missionRule; }

		/// Sets the unique-ID for the AlienMission.
		void setId(int id);
		/// Gets the unique-ID for the AlienMission.
		int getId() const;

		/// Gets the AlienMission's Region.
		const std::string& getRegion() const
		{ return _region; }
		/// Sets the AlienMission's Region.
		void setRegion(
				const std::string& region,
				const Ruleset& rules);

		/// Gets the AlienMission's AlienRace type.
		const std::string& getRace() const
		{ return _race; }
		/// Sets the AlienMission's AlienRace type.
		void setRace(const std::string& race)
		{ _race = race; }

		/// Gets the minutes until next wave spawns.
//		size_t getWaveCountdown() const
//		{ return _spawnTime; }
		/// Sets the minutes until next wave spawns.
		void setWaveCountdown(int minutes);

		/// Gets the wave of a UFO in the agenda of the AlienMission.
		size_t getWaveCount()
		{ return _waveCount; }

		/// Increases the quantity of live UFOs.
		void increaseLiveUfos()
		{ ++_liveUfos; }
		/// Decreases the quantity of live UFOs.
		void decreaseLiveUfos()
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
		void setAlienBase(const AlienBase* const base);

		/// Handles a UFO lifting from the ground.
		void ufoLifting(
				Ufo& ufo,
				Ruleset& rules,
				const Globe& globe);
		/// Handles a UFO that was shot down.
		void ufoShotDown(const Ufo& ufo);

		/// Flags tracking of the target-site.
		void setTerrorSiteZone(size_t zone);
};

}

#endif
