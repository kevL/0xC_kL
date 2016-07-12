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

#ifndef OPENXCOM_UFOTRAJECTORY_H
#define OPENXCOM_UFOTRAJECTORY_H

//#include <vector>
//#include <string>

#include <yaml-cpp/yaml.h>


namespace OpenXcom
{

/**
 * Information for points of a UFO trajectory.
 */
struct TrajectoryWaypoint
{
	size_t
		zone,		// The mission zone.
		altitude,	// The altitude to reach.
		speed;		// The speed percentage [0..100]
};


YAML::Emitter& operator<< (
		YAML::Emitter& emitter,
		const TrajectoryWaypoint& wp);

bool operator>> (
		const YAML::Node& node,
		TrajectoryWaypoint& wp);


/**
 * Holds information about a specific trajectory.
 * @note Ufo trajectories are a sequence of mission zones, altitudes, and speed
 * percentages.
 */
class UfoTrajectory
{

private:
	int _groundTimer;
	std::string _id;

	std::vector<TrajectoryWaypoint> _waypoints;


	public:
		static const std::string RETALIATION_ASSAULT_RUN;

		/// cTor.
		explicit UfoTrajectory(const std::string& id);

		/// Loads trajectory-data from YAML.
		void load(const YAML::Node &node);

		/**
		 * Gets the trajectory's ID.
		 * @return, the type-ID
		 */
		const std::string& getId() const
		{ return _id; }

		/**
		 * Gets the number of waypoints in the rule's trajectory.
		 * @return, the number of waypoints
		 */
		size_t getWaypointTotal() const
		{ return _waypoints.size(); }

		/**
		 * Gets the zone-ID at a waypoint.
		 * @param wpId - the waypoint-ID
		 * @return, the zone-ID
		 */
		size_t getZone(size_t wpId) const
		{ return _waypoints[wpId].zone; }

		/// Gets the altitude at a waypoint.
		const std::string getAltitude(size_t wpId) const; // does not like return &ref

		/**
		 * Gets the speed-percentage at a waypoint.
		 * @param wpId - the waypoint-ID
		 * @return, the speed percent
		 */
		float getSpeedPct(size_t wpId) const
		{ return static_cast<float>(_waypoints[wpId].speed) / 100.f; }

		/**
		 * Gets the number of seconds that a UFO should spend on the ground.
		 * @return, seconds
		 */
		int groundTimer() const
		{ return _groundTimer; }
};

}

#endif
