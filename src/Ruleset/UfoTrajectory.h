/*
 * Copyright 2010-2019 OpenXcom Developers.
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
 * Information for points of a UFO mission-trajectory.
 */
struct MissionPoint
{
	size_t
		zone,		// the zoneId
		altitude,	// the altitude
		speed;		// the speed percentage
};

YAML::Emitter& operator <<(
		YAML::Emitter& out,
		const MissionPoint& point);

bool operator >>(
		const YAML::Node& node,
		MissionPoint& point);


/**
 * Holds information about a specific trajectory.
 * @note Ufo trajectories are a sequence of zoneIds, altitudes, and speed
 * percentages.
 */
class UfoTrajectory
{

private:
	int _ground;
	std::string _type;

	std::vector<MissionPoint> _points;


	public:
		static const std::string XCOM_BASE_ASSAULT;

		/// cTor.
		explicit UfoTrajectory(const std::string& type);

		/// Loads UfoTrajectory data from YAML.
		void load(const YAML::Node& node);

		/**
		 * Gets the UfoTrajectory's type.
		 * @return, the trajectory's type
		 */
		const std::string& getType() const
		{ return _type; }

		/**
		 * Gets the quantity of mission-points in the UfoTrajectory.
		 * @return, the quantity of points
		 */
		size_t getMissionPointTotal() const
		{ return _points.size(); }

		/**
		 * Gets the zone-ID at a specified mission-point.
		 * @param pointId - the point-ID
		 * @return, the zone-ID
		 */
		size_t getZoneId(size_t pointId) const
		{ return _points[pointId].zone; }

		/// Gets the altitude at a specified mission-point.
		const std::string getAltitude(size_t pointId) const; // does not like return &ref

		/**
		 * Gets the speed-percentage at a specified mission-point.
		 * @param pointId - the point-ID
		 * @return, the speed percent
		 */
		double getSpeedPct(size_t pointId) const
		{ return static_cast<double>(_points[pointId].speed) / 100.; }

		/**
		 * Gets the seconds that a UFO should spend on the ground.
		 * @return, seconds
		 */
		int getGroundDuration() const
		{ return _ground * 5; } // INVESTIGATE: *5
};

}

#endif
