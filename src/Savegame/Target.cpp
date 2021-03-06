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

#include "Target.h"

#include "../fmath.h"

#include "Craft.h"
#include "SerializationHelper.h"


namespace OpenXcom
{

const char* const Target::stTarget[8u] // static.
{
	"STR_UFO",			// 0
	"STR_BASE",			// 1
	"STR_ALIEN_BASE",	// 2
	"STR_TERROR_SITE",	// 3
	"STR_WAYPOINT",		// 4
	"STR_LANDING_SITE",	// 5
	"STR_CRASH_SITE",	// 6
	"STR_ALIEN_MISSION"	// 7
};


/**
 * Initializes a Target with blank coordinates.
 */
Target::Target()
	:
		_id(0),
		_lon(0.),
		_lat(0.)
{}

/**
 * Make sure no Craft are left targeting this Target.
 */
Target::~Target() // virtual.
{
	Craft* craft;
	for (size_t
			i = 0u;
			i != _targeters.size();
			)
	{
		if ((craft = dynamic_cast<Craft*>(_targeters[i])) != nullptr)
		{
			craft->returnToBase();
			i = 0u;
		}
		else
			++i;
	}
}

/**
 * Loads this Target from a YAML file.
 * @param node - YAML node
 */
void Target::load(const YAML::Node& node) // virtual.
{
	_lon = node["lon"].as<double>(_lon);
	_lat = node["lat"].as<double>(_lat);
}

/**
 * Saves this Target to a YAML file.
 * @return, YAML node
 */
YAML::Node Target::save() const // virtual.
{
	YAML::Node node;

	node["lon"] = serializeDouble(_lon);
	node["lat"] = serializeDouble(_lat);

	return node;
}

/**
 * Sets this Target's ID.
 * @param id - id
 */
void Target::setId(int id)
{
	_id = id;
}

/**
 * Gets this Target's ID.
 * @return, id
 */
int Target::getId() const
{
	return _id;
}

/**
 * Gets the longitude coordinate of this Target.
 * @return, longitude in radians
 */
double Target::getLongitude() const
{
	return _lon;
}

/**
 * Sets the longitude coordinate of this Target.
 * @param lon - longitude in radians
 */
void Target::setLongitude(double lon)
{
	_lon = lon;

	while (_lon < 0.) // keep it between 0 and 2PI
		_lon += M_PI * 2.;

	while (_lon >= M_PI * 2.)
		_lon -= M_PI * 2.;
}

/**
 * Gets the latitude coordinate of this Target.
 * @return, latitude in radians
 */
double Target::getLatitude() const
{
	return _lat;
}

/**
 * Sets the latitude coordinate of this Target.
 * @param lat - latitude in radians
 */
void Target::setLatitude(double lat)
{
	_lat = lat;

	if (_lat < -M_PI_2) // keep it between -pi/2 and pi/2
	{
		_lat = -M_PI - _lat;
		setLongitude(_lon + M_PI);
	}
	else if (_lat > M_PI_2)
	{
		_lat = M_PI - _lat;
		setLongitude(_lon - M_PI);
	}
}

/**
 * Gets the list of MovingTargets currently targeting this Target.
 * @return, pointer to a vector of pointers to MovingTargets
 */
std::vector<Target*>* Target::getTargeters()
{
	return &_targeters;
}

/**
 * Gets the radian-angle to another Target on the Globe.
 * @param target - pointer to other target
 * @return, distance in radians
 */
double Target::getDistance(const Target* const target) const
{
	const double lonTarget (target->getLongitude());
	const double latTarget (target->getLatitude());

	if (AreSameTwo(
				_lon, lonTarget,
				_lat, latTarget))
	{
		return 0.;
	}
	// else the formula below returns NaN

	return std::acos(
				  std::cos(_lat)
				* std::cos(latTarget)
				* std::cos(lonTarget - _lon)
				+ std::sin(_lat)
				* std::sin(latTarget));
}

}
