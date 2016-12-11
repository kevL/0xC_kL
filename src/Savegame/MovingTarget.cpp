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
#include "Ufo.h"
#include "Waypoint.h"

//#include "../Engine/Logger.h" // DEBUG

#include "../Geoscape/GeoscapeState.h" // unitToRads


namespace OpenXcom
{

const char* const MovingTarget::stAltitude[5u] // static.
{
	"STR_GROUND",	// 0
	"STR_VERY_LOW",	// 1
	"STR_LOW_UC",	// 2
	"STR_HIGH_UC",	// 3
	"STR_VERY_HIGH"	// 4
};


/**
 * Initializes the MovingTarget with blank coordinates.
 * @param playSave - pointer to the SavedGame
 */
MovingTarget::MovingTarget(SavedGame* const playSave)
	:
		Target(),
		_playSave(playSave),
		_target(nullptr),
		_speedLon(0.),
		_speedLat(0.),
		_speedRads(0.),
		_speed(0),
		_lonPoint(0.),
		_latPoint(0.)
{}

/**
 * dTor.
 * @note Clears this MovingTarget from its destination's targeters.
 */
MovingTarget::~MovingTarget() // virtual.
{
	checkTargets();
}

/**
 * Saves this MovingTarget to a YAML file.
 * @return, YAML node
 */
YAML::Node MovingTarget::save() const // virtual.
{
	YAML::Node node (Target::save());

	if (_speed != 0)
	{
		node["speed"]		= _speed;
		node["speedRads"]	= serializeDouble(_speedRads);
		node["speedLon"]	= serializeDouble(_speedLon);
		node["speedLat"]	= serializeDouble(_speedLat);
	}

	if (_target != nullptr)
	{
		const Ufo* const ufo (dynamic_cast<const Ufo*>(this));
		if (ufo == nullptr || ufo->getUfoStatus() != Ufo::CRASHED)	// ie, only Craft or UFO-flying or -landed
			node["target"] = _target->saveIdentificator();			// can have a target.
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

	_speed		= node["speed"]		.as<int>(_speed);
	_speedRads	= node["speedRads"]	.as<double>(_speedRads);
	_speedLon	= node["speedLon"]	.as<double>(_speedLon);
	_speedLat	= node["speedLat"]	.as<double>(_speedLat);
}

/**
 * Sets the destination-target of this MovingTarget.
 * @param target - pointer to Target destination (default nullptr)
 */
void MovingTarget::setTarget(Target* const target) // virtual.
{
	//Log(LOG_INFO) << "";
	//Log(LOG_INFO) << "MovingTarget::setTarget()";

	checkTargets();

	if ((_target = target) != nullptr)
	{
		_target->getTargeters()->push_back(this);
		calculateSpeed();
	}
	else
		setSpeed();
}

/**
 * Gets the destination-target of this MovingTarget.
 * @return, pointer to Target destination
 */
Target* MovingTarget::getTarget() const
{
	return _target;
}

/**
 * Checks if this MovingTarget has reached its destination.
 * @return, true if it has
 */
bool MovingTarget::reachedDestination() const
{
	//Log(LOG_INFO) << "";
	//Log(LOG_INFO) << "MovingTarget::reachedDestination()";

	if (_target != nullptr)
		return AreSameTwo(
					_lon, _target->getLongitude(),
					_lat, _target->getLatitude()) == true;

	return false;
}

/**
 * Checks this MovingTarget for targeters and erases it from their
 * target-vectors, and also checks its current destination-target for other
 * targeters and if none are found deletes the Waypoint if applicable.
 */
void MovingTarget::checkTargets() // private.
{
	if (_target != nullptr)
	{
		bool destroy (true);
		for (std::vector<Target*>::const_iterator
				i = _target->getTargeters()->begin();
				i != _target->getTargeters()->end();
				)
		{
			if (*i == this)
				i = _target->getTargeters()->erase(i);
			else
			{
				destroy = false;
				++i;
			}
		}

		if (destroy == true)
		{
			const Waypoint* const wp (dynamic_cast<Waypoint*>(_target));
			if (wp != nullptr)
			{
				delete wp;
				for (std::vector<Waypoint*>::const_iterator
						i = _playSave->getWaypoints()->begin();
						i != _playSave->getWaypoints()->end();
						++i)
				{
					if (*i == wp)
					{
						_playSave->getWaypoints()->erase(i);
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
 * @param speed - speed in knots (default 0)
 */
void MovingTarget::setSpeed(int speed)
{
	//Log(LOG_INFO) << "";
	//Log(LOG_INFO) << "MovingTarget::setSpeed() speed= " << speed;

	if ((_speed = speed) == 0)
	{
		_speedRads =
		_speedLon =
		_speedLat = 0.;
	}
	else
	{
		_speedRads = static_cast<double>(_speed) * unitToRads / 720.;	// Each nautical mile is 1/60th of a degree;
		calculateSpeed();												// each hour contains 720 5-second periods.
	}
	//Log(LOG_INFO) << ". speedRads= " << _speedRads;
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
	//Log(LOG_INFO) << "";
	//Log(LOG_INFO) << "MovingTarget::stepTarget()";
	//Log(LOG_INFO) << ". speedRads= " << _speedRads;

	if (_target != nullptr)
	{
		//Log(LOG_INFO) << "";
		//Log(LOG_INFO) << ". stepTarget - target VALID step Lon/Lat";

		calculateSpeed();

		if (getDistance(_target) > _speedRads)
		{
			setLongitude(_lon + _speedLon);
			setLatitude( _lat + _speedLat);
		}
		else
		{
			setLongitude(_target->getLongitude());
			setLatitude( _target->getLatitude());
		}
	}
	else
		setSpeed();
}

/**
 * Calculates the speed-vector based on the great-circle-distance to destination
 * and current speed.
 */
void MovingTarget::calculateSpeed() // protected/virtual.
{
	//Log(LOG_INFO) << "";
	//Log(LOG_INFO) << "MovingTarget::calculateSpeed()";

	if (_target != nullptr)
	{
		//Log(LOG_INFO) << ". target VALID -> calculateMeetPoint";
		calculateMeetPoint();

		//Log(LOG_INFO) << ". . lon= "		<< _lon;
		//Log(LOG_INFO) << ". . lat= "		<< _lat;
		//Log(LOG_INFO) << ". . lonPoint= "	<< _lonPoint;
		//Log(LOG_INFO) << ". . latPoint= "	<< _latPoint;

		const double
			dLon (std::sin(_lonPoint - _lon)
				* std::cos(_latPoint)),
			dLat (std::cos(_lat)
				* std::sin(_latPoint) - std::sin(_lat)
				* std::cos(_latPoint)
				* std::cos(_lonPoint - _lon)),
			dist (std::sqrt((dLon * dLon) + (dLat * dLat)));

		//Log(LOG_INFO) << ". . dLon= " << dLon;
		//Log(LOG_INFO) << ". . dLat= " << dLat;
		//Log(LOG_INFO) << ". . dist= " << dist;

		_speedLat = dLat / dist * _speedRads;
		_speedLon = dLon / dist * _speedRads / std::cos(_lat + _speedLat);

		//Log(LOG_INFO) << ". . speedLon= " << _speedLon;
		//Log(LOG_INFO) << ". . speedLat= " << _speedLat;

		// Check for invalid speeds when a division-by-zero occurs due to near-lightspeed values.
		if (isNaNorInf(_speedLon, _speedLat) == true)
		{
			//Log(LOG_INFO) << ". . isNaNorInf 0,0";
			setSpeed();
		}
	}
	else
	{
		//Log(LOG_INFO) << ". target NOT Valid 0,0";
		setSpeed();
	}
}

/**
 * Calculates the meet-point with a destination-target.
 */
void MovingTarget::calculateMeetPoint() // private.
{
	//Log(LOG_INFO) << "";
	//Log(LOG_INFO) << "MovingTarget::calculateMeetPoint()";

	_lonPoint = _target->getLongitude();
	_latPoint = _target->getLatitude();

	//Log(LOG_INFO) << ". lonPoint= " << _lonPoint;
	//Log(LOG_INFO) << ". latPoint= " << _latPoint;

	MovingTarget* const ufo (dynamic_cast<MovingTarget*>(_target));
	if (ufo != nullptr && ufo->getTarget() != nullptr
		&& AreSame(ufo->_speedRads, 0.) == false)
	{
		const double
			lonUfo				(ufo->getLongitude()),
			sin_lonUfo			(std::sin(lonUfo)),
			cos_lonUfo			(std::cos(lonUfo)),

			latUfo				(ufo->getLatitude()),
			sin_latUfo			(std::sin(latUfo)),
			cos_latUfo			(std::cos(latUfo)),

			lonUfoTarget		(ufo->getTarget()->getLongitude()),
			sin_lonUfoTarget	(std::sin(lonUfoTarget)),
			cos_lonUfoTarget	(std::cos(lonUfoTarget)),

			latUfoTarget		(ufo->getTarget()->getLatitude()),
			sin_latUfoTarget	(std::sin(latUfoTarget)),
			cos_latUfoTarget	(std::cos(latUfoTarget)),

			sin_lon				(std::sin(lonUfoTarget - lonUfo));

		double
			nx (cos_latUfo * sin_lonUfo       * sin_latUfoTarget
			  - sin_latUfo * cos_latUfoTarget * sin_lonUfoTarget),

			ny (sin_latUfo * cos_latUfoTarget * cos_lonUfoTarget
			  - cos_latUfo * cos_lonUfo       * sin_latUfoTarget),

			nz (cos_latUfo * cos_latUfoTarget * sin_lon);

		const double nk (_speedRads / std::sqrt(nx * nx + ny * ny + nz * nz));
		nx *= nk;
		ny *= nk;
		nz *= nk;

		double
			path (0.),
			dist;
		double
			old_path,
			new_path (std::acos(
								std::cos(_lat)
							  * std::cos(_latPoint)
							  * std::cos(_lonPoint - _lon)
							  + std::sin(_lat)
							  * std::sin(_latPoint)));
		const double speedRatio (_speedRads / ufo->_speedRads);

		do
		{
			old_path = new_path;
			_latPoint += nx * std::sin(_lonPoint)
					   - ny * std::cos(_lonPoint);

			if (std::fabs(_latPoint) < M_PI_2)
				_lonPoint += nz
							  - (nx * std::cos(_lonPoint)
							  +  ny * std::sin(_lonPoint))
							  * std::tan(_latPoint);
			else
				_lonPoint += M_PI;

			path += _speedRads;
			dist = std::acos(
							std::cos(_lat)
						  * std::cos(_latPoint)
						  * std::cos(_lonPoint - _lon)
						  + std::sin(_lat)
						  * std::sin(_latPoint));
			new_path = dist - path * speedRatio;
		}
		while (path < M_PI && new_path > 0. && old_path > new_path);

		while (std::fabs(_lonPoint) > M_PI)
			_lonPoint -= std::copysign(M_PI * 2., _lonPoint);

		while (std::fabs(_latPoint) > M_PI)
			_latPoint -= std::copysign(M_PI * 2., _latPoint);

		if (std::fabs(_latPoint) > M_PI_2)
		{
			_lonPoint -= std::copysign(M_PI, _lonPoint);
			_latPoint  = std::copysign(M_PI * 2. - std::fabs(_latPoint), _latPoint);
		}
	}
}

/**
 * Gets the longitude of the meet-point.
 * @note Used in GeoscapeState::time5Seconds().
 * @return, globe-angle in radians
 */
double MovingTarget::getMeetLongitude() const
{
	return _lonPoint;
}

/**
 * Gets the latitude of the meet-point.
 * @note Used in GeoscapeState::time5Seconds().
 * @return, globe-angle in radians
 */
double MovingTarget::getMeetLatitude() const
{
	return _latPoint;
}

}
