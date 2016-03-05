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

#ifndef OPENXCOM_MOVINGTARGET_H
#define OPENXCOM_MOVINGTARGET_H

#include "Target.h"

//#include <utility>


namespace OpenXcom
{

typedef std::pair<std::string, int> CraftId;


/**
 * Base class for moving targets on the globe with a certain speed and destination.
 */
class MovingTarget
	:
		public Target
{

protected:
//	static const double GLOBE_RADIUS;

	int _speed;
	double
		_meetPointLon,
		_meetPointLat,
		_speedLat,
		_speedLon,
		_speedRadian;

	Target* _dest;

	/// Calculates a new speed vector to the destination.
	virtual void calculateSpeed();

	/// Creates a MovingTarget.
	MovingTarget();


	public:
		/// Cleans up the MovingTarget.
		virtual ~MovingTarget();

		/// Loads the MovingTarget from YAML.
		virtual void load(const YAML::Node& node) override;
		/// Saves the MovingTarget to YAML.
		virtual YAML::Node save() const override;

		/// Gets the MovingTarget's destination.
		Target* getDestination() const;
		/// Sets the MovingTarget's destination.
		virtual void setDestination(Target* const dest);

		/// Gets the MovingTarget's speed.
		int getSpeed() const;
		/// Sets the MovingTarget's speed.
		void setSpeed(const int speed);

		/// Checks if the MovingTarget has reached its destination.
		bool reachedDestination() const;
		/// Move towards the destination.
		void moveTarget();

		/// Calculate meeting point with the target.
		void calculateMeetPoint();
		/// Returns the latitude of the meeting point.
		double getMeetLatitude() const;
		/// Returns the longitude of the meeting point.
		double getMeetLongitude() const;
};

}

#endif
