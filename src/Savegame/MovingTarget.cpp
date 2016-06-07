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

#include "MovingTarget.h"

#include "../fmath.h"

#include "SavedGame.h"
#include "SerializationHelper.h"
#include "Waypoint.h"

#include "../Geoscape/GeoscapeState.h"


namespace OpenXcom
{

const char* MovingTarget::stAltitude[5u] // static.
{
	"STR_GROUND",	// 0
	"STR_VERY_LOW",	// 1
	"STR_LOW_UC",	// 2
	"STR_HIGH_UC",	// 3
	"STR_VERY_HIGH"	// 4
};


/**
 * Initializes the MovingTarget with blank coordinates.
 * @param gameSave - pointer to the SavedGame
 */
MovingTarget::MovingTarget(SavedGame* const gameSave)
	:
		Target(),
		_dest(nullptr),
		_speedLon(0.),
		_speedLat(0.),
		_speedRadian(0.),
		_speed(0),
		_meetPointLon(0.),
		_meetPointLat(0.),
		_gameSave(gameSave)
{}

/**
 * dTor.
 * @note Clears this MovingTarget from its destination's targeters.
 */
MovingTarget::~MovingTarget() // virtual.
{
	checkOtherTargeters();
}

/**
 * Saves this MovingTarget to a YAML file.
 * @return, YAML node
 */
YAML::Node MovingTarget::save() const // virtual.
{
	YAML::Node node (Target::save());

	if (_dest != nullptr) node["dest"] = _dest->saveId();

	if (_speed != 0)
	{
		node["speed"]		= _speed;
		node["speedRadian"]	= serializeDouble(_speedRadian);
		node["speedLon"]	= serializeDouble(_speedLon);
		node["speedLat"]	= serializeDouble(_speedLat);
	}

	return node;
}

/**
 * Loads this MovingTarget from a YAML file.
 * @param node - reference a YAML node
 */
void MovingTarget::load(const YAML::Node& node) // virtual.
{
	Target::load(node);

	_speedLon		= node["speedLon"]		.as<double>(_speedLon);
	_speedLat		= node["speedLat"]		.as<double>(_speedLat);
	_speedRadian	= node["speedRadian"]	.as<double>(_speedRadian);
	_speed			= node["speed"]			.as<int>(_speed);
}

/**
 * Sets the destination-target of this MovingTarget.
 * @param dest - pointer to Target destination (default nullptr)
 */
void MovingTarget::setDestination(Target* const dest) // virtual.
{
	checkOtherTargeters();

	if ((_dest = dest) != nullptr)
		_dest->getTargeters()->push_back(this);

	calculateSpeed();
}

/**
 * Gets the destination-target of this MovingTarget.
 * @return, pointer to Target destination
 */
Target* MovingTarget::getDestination() const
{
	return _dest;
}

/**
 * Checks if this MovingTarget has reached its destination.
 * @return, true if it has
 */
bool MovingTarget::reachedDestination() const
{
	if (_dest != nullptr)
		return AreSame(_lon, _dest->getLongitude())
			&& AreSame(_lat, _dest->getLatitude());

	return false;
}

/**
 * Checks this MovingTarget's current destination for other targeters and if
 * none are found deletes its Waypoint if applicable.
 */
void MovingTarget::checkOtherTargeters() // protected.
{
	if (_dest != nullptr)
	{
		bool destTargeter (false);
		for (std::vector<Target*>::const_iterator
				i = _dest->getTargeters()->begin();
				i != _dest->getTargeters()->end();
				)
		{
			if (*i == this)
				i = _dest->getTargeters()->erase(i);
			else
			{
				destTargeter = true;
				++i;
			}
		}

		if (destTargeter == false)
		{
			const Waypoint* const wpPre (dynamic_cast<Waypoint*>(_dest));
			if (wpPre != nullptr)
			{
				delete wpPre;

				for (std::vector<Waypoint*>::const_iterator
						i = _gameSave->getWaypoints()->begin();
						i != _gameSave->getWaypoints()->end();
						++i)
				{
					if (*i == wpPre)
					{
						_gameSave->getWaypoints()->erase(i);
						break;
					}
				}
			}
		}
	}
}

/**
 * Sets the speed of this MovingTarget and converts it from standard knots
 * (nautical miles-per-hour) into radians-per-5-IG-seconds.
 * @param speed - speed in knots
 */
void MovingTarget::setSpeed(const int speed)
{
	_speed = speed;

	// Each nautical mile is 1/60th of a degree; each hour contains 720 5-second periods.
	_speedRadian = static_cast<double>(_speed) * unitToRads / 720.;

	calculateSpeed();
}

/**
 * Gets the speed of this MovingTarget.
 * @return, speed in knots
 */
int MovingTarget::getSpeed() const
{
	return _speed;
}

/**
 * Advances a flight-step for this MovingTarget.
 */
void MovingTarget::stepTarget()
{
	calculateSpeed();

	if (_dest != nullptr)
	{
		if (getDistance(_dest) > _speedRadian)
		{
			setLongitude(_lon + _speedLon);
			setLatitude(_lat + _speedLat);
		}
		else
		{
			setLongitude(_dest->getLongitude());
			setLatitude(_dest->getLatitude());
		}
	}
}

/**
 * Calculates the speed-vector based on the great-circle-distance to destination
 * and current speed.
 */
void MovingTarget::calculateSpeed() // protected/virtual.
{
	if (_dest != nullptr)
	{
		calculateMeetPoint();

		const double
			dLon (std::sin(_meetPointLon - _lon)
				* std::cos(_meetPointLat)),
			dLat (std::cos(_lat)
				* std::sin(_meetPointLat) - std::sin(_lat)
				* std::cos(_meetPointLat)
				* std::cos(_meetPointLon - _lon)),
			dist (std::sqrt((dLon * dLon) + (dLat * dLat)));

		_speedLat = dLat / dist * _speedRadian;
		_speedLon = dLon / dist * _speedRadian / std::cos(_lat + _speedLat);

		// Check for invalid speeds when a division-by-zero occurs due to near-lightspeed values.
		if (isNaNorInf(_speedLon, _speedLat) == true)
		{
			_speedLon =
			_speedLat = 0.;
		}
	}
	else
	{
		_speedLon =
		_speedLat = 0.;
	}
}

/**
 * Calculates the meet-point with a destination-target.
 */
void MovingTarget::calculateMeetPoint() // protected.
{
	_meetPointLat = _dest->getLatitude();
	_meetPointLon = _dest->getLongitude();

	MovingTarget* const ufo (dynamic_cast<MovingTarget*>(_dest));
	if (ufo != nullptr
		&& ufo->getDestination() != nullptr
		&& AreSame(ufo->_speedRadian, 0.) == false)
	{
		const double speedRatio (_speedRadian / ufo->_speedRadian);
		double
			nx (std::cos(ufo->getLatitude()) * std::sin(ufo->getLongitude()) * std::sin(ufo->getDestination()->getLatitude())
			  - std::sin(ufo->getLatitude()) * std::cos(ufo->getDestination()->getLatitude()) * std::sin(ufo->getDestination()->getLongitude())),
			ny (std::sin(ufo->getLatitude()) * std::cos(ufo->getDestination()->getLatitude()) * std::cos(ufo->getDestination()->getLongitude())
			  - std::cos(ufo->getLatitude()) * std::cos(ufo->getLongitude()) * std::sin(ufo->getDestination()->getLatitude())),
			nz (std::cos(ufo->getLatitude()) * std::cos(ufo->getDestination()->getLatitude()) * std::sin(ufo->getDestination()->getLongitude()
			  - ufo->getLongitude()));

		const double nk (_speedRadian / std::sqrt(nx * nx + ny * ny + nz * nz));
		nx *= nk;
		ny *= nk;
		nz *= nk;

		double
			path (0.),
			dist;
		double
			old_pdist,
			new_pdist (std::acos(
								std::cos(_lat)
							  * std::cos(_meetPointLat)
							  * std::cos(_meetPointLon - _lon)
							  + std::sin(_lat)
							  * std::sin(_meetPointLat)));
		do
		{
			old_pdist = new_pdist;
			_meetPointLat += nx * std::sin(_meetPointLon)
						   - ny * std::cos(_meetPointLon);

			if (std::fabs(_meetPointLat) < M_PI_2)
				_meetPointLon += nz
							  - (nx * std::cos(_meetPointLon)
							  +  ny * std::sin(_meetPointLon))
							  * std::tan(_meetPointLat);
			else
				_meetPointLon += M_PI;

			path += _speedRadian;
			dist = std::acos(
							std::cos(_lat)
						  * std::cos(_meetPointLat)
						  * std::cos(_meetPointLon - _lon)
						  + std::sin(_lat)
						  * std::sin(_meetPointLat));
			new_pdist = dist - path * speedRatio;
		}
		while (path < M_PI && new_pdist > 0. && old_pdist > new_pdist);

		while (std::fabs(_meetPointLon) > M_PI)
			_meetPointLon -= std::copysign(M_PI * 2, _meetPointLon);

		while (std::fabs(_meetPointLat) > M_PI)
			_meetPointLat -= std::copysign(M_PI * 2, _meetPointLat);

		if (std::fabs(_meetPointLat) > M_PI_2)
		{
			_meetPointLat  = std::copysign(M_PI * 2 - std::fabs(_meetPointLat), _meetPointLat);
			_meetPointLon -= std::copysign(M_PI, _meetPointLon);
		}
	}
}

/**
 * Gets the latitude of the meet-point.
 * @note Used in GeoscapeState::time5Seconds().
 * @return, angle in radians
 */
double MovingTarget::getMeetLatitude() const
{
	return _meetPointLat;
}

/**
 * Gets the longitude of the meet-point.
 * @note Used in GeoscapeState::time5Seconds().
 * @return, angle in radians
 */
double MovingTarget::getMeetLongitude() const
{
	return _meetPointLon;
}

}
