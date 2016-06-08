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

class SavedGame;


/**
 * Base class for moving targets on the globe with a certain speed and destination.
 */
class MovingTarget
	:
		public Target
{

private:
	SavedGame* _gameSave;

	/// Checks the MovingTarget's current destination for safe deletion.
	void checkOtherTargeters();

	/// Calculates a meet-point with a destination-target.
	void calculateMeetPoint();


	protected:
		int _speed;
		double
			_meetPointLon,
			_meetPointLat,
			_speedLat,
			_speedLon,
			_speedRadian;

		Target* _dest;

		/// Calculates a new speed-vector to a destination.
		virtual void calculateSpeed();

		/// Creates a MovingTarget.
		explicit MovingTarget(SavedGame* const gameSave);


		public:
			static const char* stAltitude[5u];

			/// Cleans up the MovingTarget.
			virtual ~MovingTarget();

			/// Saves the MovingTarget to YAML.
			virtual YAML::Node save() const override;
			/// Loads the MovingTarget from YAML.
			virtual void load(const YAML::Node& node) override;

			/// Sets the MovingTarget's destination.
			virtual void setDestination(Target* const dest = nullptr);
			/// Gets the MovingTarget's destination.
			Target* getDestination() const;

			/// Checks if the MovingTarget has reached its destination.
			bool reachedDestination() const;

			/// Sets the MovingTarget's speed.
			void setSpeed(const int speed);
			/// Gets the MovingTarget's speed.
			int getSpeed() const;

			/// Moves toward the destination.
			void stepTarget();

			/// Returns the longitude of a meet-point.
			double getMeetLongitude() const;
			/// Returns the latitude of a meet-point.
			double getMeetLatitude() const;
};

}

#endif
