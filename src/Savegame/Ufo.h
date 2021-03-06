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
class UfoTrajectory;


/**
 * Represents an aLien UFO on the Globe.
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
		_hyperdecoded,
		_isQuickBattle,
		_processedIntercept,
		_tactical;
	int
		_escapeCountdown,
		_fireCountdown,
		_hitStep,
		_hull,
		_idCrashed,
		_idLanded,
		_secondsLeft;
	size_t
		_shootingAt,
		_missionPoint;
	unsigned _dir;

	AlienMission* _mission;
	const RuleUfo* _ufoRule;
	const UfoTrajectory* _trajectory;

	std::string
		_altitude,
		_heading,
		_terrain;

	CraftId _shotDownByCraftId;

	UfoStatus _status;

	/// Calculates a speed vector to the destination.
	void calculateSpeed() override;


	public:
		/// Creates a Ufo of the specified type.
		Ufo(
				const RuleUfo* const ufoRule,
				SavedGame* const playSave);
		/// Cleans up the Ufo.
		~Ufo();

		/// Loads the Ufo from YAML.
		void loadUfo(
				const YAML::Node& node,
				const Ruleset& rules);
		/// Saves the Ufo to YAML.
		YAML::Node save() const override;
		/// Saves the Ufo's identificator to YAML.
		YAML::Node saveIdentificator() const override;

		/// Gets the Ufo's ruleset.
		const RuleUfo* getRules() const;
		/// Sets the Ufo's ruleset.
		void changeRules(const RuleUfo* const ufoRule);

		/// Gets the Ufo's label.
		std::wstring getLabel(
				const Language* const lang,
				bool id = true) const override;

		/// Gets the Ufo's globe-marker.
		int getMarker() const override;

		/// Sets the Ufo's hull after inflicted hurt.
		void setUfoHull(int inflict);
		/// Gets the Ufo's hull-percentage.
		int getUfoHullPct() const;

		/// Checks if the Ufo has crashed.
		bool isCrashed() const;
		/// Checks if the Ufo has been destroyed.
		bool isDestroyed() const;

		/// Sets the Ufo's detection status.
		void setDetected(bool detected = true);
		/// Gets the Ufo's detection status.
		bool getDetected() const;
		/// Sets the Ufo's hyper detection status.
		void setHyperdecoded(bool hyperdecoded = true);
		/// Gets the Ufo's hyper detection status.
		bool getHyperdecoded() const;

		/// Sets the Ufo's seconds left on the ground.
		void setSecondsLeft(int sec);
		/// Gets the Ufo's seconds left on the ground.
		int getSecondsLeft() const;
		/// Reduces the UFO's seconds left by 5.
		bool reduceSecondsLeft();

		/// Sets the Ufo's altitude and status.
		void setAltitude(const std::string& altitude);
		/// Gets the Ufo's altitude.
		std::string getAltitude() const;
		/// Gets the Ufo's altitude as an integer.
		unsigned getAltitudeInt() const;
		/// Gets the Ufo's heading.
		std::string getHeading() const;
		/// Gets the Ufo's heading as an integer.
		unsigned getHeadingInt() const;

		/// Gets the Ufo status.
		UfoStatus getUfoStatus() const
		{ return _status; }
		/// Sets the Ufo's status.
		void setUfoStatus(UfoStatus status)
		{	if ((_status = status) == Ufo::DESTROYED)
				_detected = false; }

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
		int getActivityPoints() const;

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

		/// Sets the Ufo's progress on its Mission.
		size_t setMissionPoint()
		{ return ++_missionPoint; }
		/// Gets the Ufo's progress on its Mission.
		size_t getMissionPoint() const
		{ return _missionPoint; }
		/// Gets the Ufo's trajectory-rule.
		const UfoTrajectory& getTrajectory() const
		{ return *_trajectory; }

		/// Sets the interceptor engaged by the Ufo.
		void setShootingAt(size_t slot);
		/// Gets the interceptor engaged by the Ufo.
		size_t getShootingAt() const;

		/// Sets the Ufo's landing site ID.
		void setLandId(int id);
		/// Gets the Ufo's landing site ID.
		int getLandId() const;
		/// Sets the Ufo's crash site ID.
		void setCrashId(int id);
		/// Gets the Ufo's crash site ID.
		int getCrashId() const;

		/// Sets the Ufo's hit-step.
		void setHitStep(int step);
		/// Gets the Ufo's hit-step.
		int getHitStep() const;

		/// Sets the time left before the Ufo can fire in a Dogfight.
		void setFireTicks(int ticks);
		/// Gets the time left before the Ufo can fire in a Dogfight.
		int getFireTicks() const;
		/// Sets the time left before the Ufo attempts to escape a Dogfight.
		void setEscapeTicks(int ticks);
		/// Gets the time left before the Ufo attempts to escape a Dogfight.
		int getEscapeTicks() const;
		/// Sets whether or not the Ufo has had Dogfight info processed.
		void setTicked(bool tick = true);
		/// Gets whether or not the Ufo has had Dogfight info processed.
		bool getTicked() const;

		/// Sets a crashed or landed Ufo's terrainType.
		void setUfoTerrainType(const std::string& terrainType = "");
		/// Gets a crashed or landed Ufo's terrainType.
		std::string getUfoTerrainType() const;
};

}

#endif
