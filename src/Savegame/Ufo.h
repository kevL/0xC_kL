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

#ifndef OPENXCOM_UFO_H
#define OPENXCOM_UFO_H

//#include <string>
//#include <yaml-cpp/yaml.h>

#include "MovingTarget.h"


namespace OpenXcom
{

class AlienMission;
class Ruleset;
class RuleUfo;
class SavedGame;
class UfoTrajectory;


/**
 * Represents an alien UFO on the map.
 * @note Contains variable info about a UFO.
 * @sa RuleUfo
 */
class Ufo final
	:
		public MovingTarget
{

	public:
		enum UfoStatus
		{
			FLYING,		// 0
			LANDED,		// 1
			CRASHED,	// 2
			DESTROYED	// 3
		};


private:
	bool
		_detected,
		_hyperDetected,
		_processedIntercept,
		_quickBattle,
		_tactical;
	int
		_damage,
		_escapeCountdown,
		_fireCountdown,
		_hitFrame,
		_id,
		_idCrashed,
		_idLanded,
		_secondsLeft,
		_shootingAt;
	size_t _trajectoryWp;

	AlienMission* _mission;
	const RuleUfo* _ufoRule;
	const UfoTrajectory* _trajectory;

	std::string
		_altitude,
		_direction,
		_terrain;

	CraftId _shotDownByCraftId;

	UfoStatus _status;

	/// Calculates a new speed vector to the destination.
	void calculateSpeed() override;


	public:
		/// Creates a Ufo of the specified type.
		Ufo(
				const RuleUfo* const ufoRule,
				SavedGame* const gameSave);
		/// Cleans up the Ufo.
		~Ufo();

		/// Loads the Ufo from YAML.
		using MovingTarget::load;
		void load(
				const YAML::Node& node,
				const Ruleset& rules,
				SavedGame& game);
		/// Saves the Ufo to YAML.
		YAML::Node save() const override;
		/// Saves the Ufo's unique-ID to YAML.
		YAML::Node saveId() const override;

		/// Gets the Ufo's ruleset.
		const RuleUfo* getRules() const;
		/// Sets the Ufo's ruleset.
		void changeRules(const RuleUfo* const ufoRule);

		/// Handles Ufo logic.
		void think();

		/// Sets the Ufo's ID.
		void setId(int id);
		/// Gets the Ufo's ID.
		int getId() const;
		/// Gets the Ufo's name.
		std::wstring getName(const Language* const lang) const override;

		/// Gets the Ufo's marker.
		int getMarker() const override;

		/// Sets the Ufo's amount of damage.
		void setUfoDamage(int damage);
		/// Gets the Ufo's amount of damage.
		int getUfoDamage() const;
		/// Gets the Ufo's percentage of damage.
		int getUfoDamagePct() const;

		/// Sets the Ufo's detection status.
		void setDetected(bool detected = true);
		/// Gets the Ufo's detection status.
		bool getDetected() const;
		/// Sets the Ufo's hyper detection status.
		void setHyperDetected(bool hyperdetected = true);
		/// Gets the Ufo's hyper detection status.
		bool getHyperDetected() const;

		/// Sets the Ufo's seconds left on the ground.
		void setSecondsLeft(int sec);
		/// Gets the Ufo's seconds left on the ground.
		int getSecondsLeft() const;

		/// Sets the Ufo's altitude and status.
		void setAltitude(const std::string& altitude);
		/// Gets the Ufo's altitude.
		std::string getAltitude() const;
		/// Gets the Ufo's direction.
		std::string getDirection() const;

		/// Gets the Ufo status.
		UfoStatus getUfoStatus() const
		{ return _status; }
		/// Set the Ufo's status.
		void setUfoStatus(UfoStatus status)
		{ _status = status; }

		/// Gets if the Ufo has crashed.
		bool isCrashed() const;
		/// Gets if the Ufo has been destroyed.
		bool isDestroyed() const;

		/// Sets the Ufo's battlescape status.
		void setTactical(bool tactical = true);
		/// Gets if the Ufo is in battlescape.
		bool getTactical() const;

		/// Sets the UFO as the UFO in a quick-battle.
		void setQuickBattle();

		/// Gets the Ufo's alien-race.
		const std::string& getAlienRace() const;

		/// Sets the ID of the Craft that shot down the Ufo.
		void setShotDownByCraftId(const CraftId& craftId);
		/// Gets the ID of the Craft that shot down the Ufo.
		CraftId getShotDownByCraftId() const;

		/// Gets the scare-factor of UFOs for activity on the Graphs.
		int getVictoryPoints() const;

		/// Gets the Ufo's visibility.
		int getVisibility() const;
		/// Gets a Ufo's detect-xCom-base ability.
		int getDetectors() const;

		/// Sets the Ufo's mission information.
		void setUfoMissionInfo(
				AlienMission* const mission,
				const UfoTrajectory* const trajectory);
		/// Gets the Ufo's Mission type.
		const std::string& getUfoMissionType() const;
		/// Gets the Ufo's mission object.
		AlienMission* getAlienMission() const
		{ return _mission; }

		/// Gets the Ufo's progress on its trajectory.
		size_t getTrajectoryPoint() const
		{ return _trajectoryWp; }
		/// Sets the Ufo's progress on its trajectory.
		void setTrajectoryPoint(size_t wpId)
		{ _trajectoryWp = wpId; }
		/// Gets the Ufo's trajectory.
		const UfoTrajectory& getTrajectory() const
		{ return *_trajectory; }

		/// Sets the interceptor engaging the Ufo.
		void setShootingAt(const size_t target);
		/// Gets the interceptor engaging the Ufo.
		size_t getShootingAt() const;

		/// Sets the Ufo's landing site ID.
		void setLandId(int id);
		/// Gets the Ufo's landing site ID.
		int getLandId() const;
		/// Sets the Ufo's crash site ID.
		void setCrashId(int id);
		/// Gets the Ufo's crash site ID.
		int getCrashId() const;

		/// Sets the Ufo's hit frame.
		void setHitFrame(int frame);
		/// Gets the Ufo's hit frame.
		int getHitFrame() const;

		/// Sets the time left before the Ufo can fire in a Dogfight.
		void setFireCountdown(int ticks);
		/// Gets the time left before the Ufo can fire in a Dogfight.
		int getFireCountdown() const;
		/// Sets the time left before the Ufo attempts to escape a Dogfight.
		void setEscapeCountdown(int ticks);
		/// Gets the time left before the Ufo attempts to escape a Dogfight.
		int getEscapeCountdown() const;
		/// Sets whether or not the Ufo has had Dogfight info processed.
		void setTicked(bool done = true);
		/// Gets whether or not the Ufo has had Dogfight info processed.
		bool getTicked() const;

		/// Sets a crashed or landed Ufo's terrainType.
		void setUfoTerrainType(const std::string& terrainType = "");
		/// Gets a crashed or landed Ufo's terrainType.
		std::string getUfoTerrainType() const;
};

}

#endif
