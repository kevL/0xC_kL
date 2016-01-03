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

#ifndef _USE_MATH_DEFINES
#	define _USE_MATH_DEFINES
#endif

#include "MovingTarget.h"

//#include <cmath>

#include "../fmath.h"

#include "SerializationHelper.h"

#include "../Geoscape/GeoscapeState.h"


namespace OpenXcom
{

/**
 * Initializes a moving target with blank coordinates.
 */
MovingTarget::MovingTarget()
	:
		Target(),
		_dest(nullptr),
		_speedLon(0.),
		_speedLat(0.),
		_speedRadian(0.),
		_speed(0),
		_meetPointLon(0.),
		_meetPointLat(0.)
{}

/**
 * Make sure to cleanup the target's destination followers.
 */
MovingTarget::~MovingTarget() // virtual.
{
	if (_dest != nullptr)
	{
		for (std::vector<Target*>::const_iterator
				i = _dest->getFollowers()->begin();
				i != _dest->getFollowers()->end();
				++i)
		{
			if (*i == this)
			{
				_dest->getFollowers()->erase(i);
				break;
			}
		}
	}
}

/**
 * Loads the moving target from a YAML file.
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
 * Saves the moving target to a YAML file.
 * @return, YAML node
 */
YAML::Node MovingTarget::save() const // virtual.
{
	YAML::Node node = Target::save();

	if (_dest != nullptr)
		node["dest"]	= _dest->saveId();

	node["speedLon"]	= serializeDouble(_speedLon);
	node["speedLat"]	= serializeDouble(_speedLat);
	node["speedRadian"]	= serializeDouble(_speedRadian);
	node["speed"]		= _speed;

	return node;
}

/**
 * Returns the destination the moving target is heading to.
 * @return, pointer to Target destination
 */
Target* MovingTarget::getDestination() const
{
	return _dest;
}

/**
 * Changes the destination the moving target is heading to.
 * @param dest - pointer to Target destination
 */
void MovingTarget::setDestination(Target* const dest) // virtual.
{
	if (_dest != nullptr)
	{
		for (std::vector<Target*>::const_iterator
				i = _dest->getFollowers()->begin();
				i != _dest->getFollowers()->end();
				++i)
		{
			if (*i == this)
			{
				_dest->getFollowers()->erase(i);
				break;
			}
		}
	}

	_dest = dest;

	if (_dest != nullptr)
		_dest->getFollowers()->push_back(this);

	calculateSpeed();
}

/**
 * Returns the speed of the moving target.
 * @return, speed in knots
 */
int MovingTarget::getSpeed() const
{
	return _speed;
}

/**
 * Returns the radial speed of the moving target.
 * @return, speed in 1/5 sec
 *
double MovingTarget::getSpeedRadian() const
{
	return _speedRadian;
} */

/**
 * Changes the speed of the moving target and converts it from standard knots
 * (nautical miles per hour) into radians per 5 IG seconds.
 * @param speed - speed in knots
 */
void MovingTarget::setSpeed(const int speed)
{
	_speed = speed;

	// each nautical mile is 1/60th of a degree; each hour contains 720 5-seconds
	_speedRadian = static_cast<double>(_speed) * unitToRads / 720.;

	calculateSpeed();
}

/**
 * Calculates the speed vector based on the great circle distance to destination
 * and current raw speed.
 */
void MovingTarget::calculateSpeed()
{
	if (_dest != nullptr)
	{
		calculateMeetPoint();
//		else // stock.
//		{
//			_meetPointLon = _dest->getLongitude();
//			_meetPointLat = _dest->getLatitude();
//		}

		const double
			dLon = std::sin(_meetPointLon - _lon)
				 * std::cos(_meetPointLat),
			dLat = std::cos(_lat)
				 * std::sin(_meetPointLat) - std::sin(_lat)
				 * std::cos(_meetPointLat)
				 * std::cos(_meetPointLon - _lon),
			dist = std::sqrt((dLon * dLon) + (dLat * dLat));

		_speedLat = dLat / dist * _speedRadian;
		_speedLon = dLon / dist * _speedRadian / std::cos(_lat + _speedLat);

		// Check for invalid speeds when a division by zero occurs due to near-lightspeed values
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
 * Checks if the moving target has reached its destination.
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
 * Executes a movement cycle for the moving target.
 */
void MovingTarget::moveTarget()
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
 * Calculates meeting point with the target.
 */
void MovingTarget::calculateMeetPoint()
{
	_meetPointLat = _dest->getLatitude();
	_meetPointLon = _dest->getLongitude();

	MovingTarget* ufo = dynamic_cast<MovingTarget*>(_dest);
	if (ufo != nullptr)
	{
		const double speedRatio = _speedRadian / ufo->_speedRadian; //ufo->getSpeedRadian();
		if (speedRatio > 1.)
		{
			double
				nx = cos(ufo->getLatitude()) * sin(ufo->getLongitude()) * sin(ufo->getDestination()->getLatitude())
				   - sin(ufo->getLatitude()) * cos(ufo->getDestination()->getLatitude()) * sin(ufo->getDestination()->getLongitude()),
				ny = sin(ufo->getLatitude()) * cos(ufo->getDestination()->getLatitude()) * cos(ufo->getDestination()->getLongitude())
				   - cos(ufo->getLatitude()) * cos(ufo->getLongitude()) * sin(ufo->getDestination()->getLatitude()),
				nz = cos(ufo->getLatitude()) * cos(ufo->getDestination()->getLatitude()) * sin(ufo->getDestination()->getLongitude()
				   - ufo->getLongitude());

			double nk = _speedRadian / sqrt(nx * nx + ny * ny + nz * nz);
			nx *= nk;
			ny *= nk;
			nz *= nk;

			double
				path = 0.,
				distance;

			do
			{
				_meetPointLat += nx * sin(_meetPointLon) - ny * cos(_meetPointLon);

				if (std::fabs(_meetPointLat) < M_PI / 2)
					_meetPointLon += nz - (nx * cos(_meetPointLon) + ny * sin(_meetPointLon)) * tan(_meetPointLat);
				else
					_meetPointLon += M_PI;

				path += _speedRadian;

				distance = acos(cos(_lat) * cos(_meetPointLat) * cos(_meetPointLon - _lon) + sin(_lat) * sin(_meetPointLat));
			} while (path < M_PI && distance - path * speedRatio > 0.);

			while (std::fabs(_meetPointLon) > M_PI)
				_meetPointLon -= copysign(M_PI * 2, _meetPointLon);

			while (std::fabs(_meetPointLat) > M_PI)
				_meetPointLat -= copysign(M_PI * 2, _meetPointLat);

			if (std::fabs(_meetPointLat) > M_PI / 2)
			{
				_meetPointLat = copysign(M_PI * 2 - std::fabs(_meetPointLat), _meetPointLat);
				_meetPointLon -= copysign(M_PI, _meetPointLon);
			}
		}
	}
}

/**
 * Returns the latitude of the meeting point.
 * @note Used in GeoscapeState::time5Seconds().
 * @return, angle in radians
 */
double MovingTarget::getMeetLatitude() const
{
	return _meetPointLat;
}

/**
 * Returns the longitude of the meeting point.
 * @note Used in GeoscapeState::time5Seconds().
 * @return, angle in radians
 */
double MovingTarget::getMeetLongitude() const
{
	return _meetPointLon;
}

}
