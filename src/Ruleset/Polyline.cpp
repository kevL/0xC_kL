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

#include "Polyline.h"

#include "../fmath.h"


namespace OpenXcom
{

/**
 * Initializes the Polyline with arrays that store each point's coordinates.
 * @param points - quantity of points
 */
Polyline::Polyline(size_t points)
	:
		_points(points)
{
	_lat = new double[_points];
	_lon = new double[_points];
}

/**
 * Deletes this Polyline's arrays from memory.
 */
Polyline::~Polyline()
{
	delete[] _lat;
	delete[] _lon;
}

/**
 * Loads this Polyline from a YAML file.
 * @param node - reference a YAML node
 */
void Polyline::load(const YAML::Node& node)
{
	delete[] _lat;
	delete[] _lon;

	const std::vector<double> coords (node.as<std::vector<double>>());
	_points = coords.size() >> 1u;

	_lat = new double[_points];
	_lon = new double[_points];

	for (size_t
			i = 0u;
			i < coords.size();
			i += 2u)
	{
		size_t j (i >> 1u);

		_lon[j] = coords[i] * M_PI / 180.;
		_lat[j] = coords[i + 1u] * M_PI / 180.;
	}
}

/**
 * Gets the latitude of a given point.
 * @param i - point (0 to max)
 * @return, the point's latitude
 */
double Polyline::getLatitude(size_t i) const
{
	return _lat[i];
}

/**
 * Sets the latitude of a given point.
 * @param i		- point (0 to max)
 * @param lat	- the point's latitude
 *
void Polyline::setLatitude(
		size_t i,
		double lat)
{
	_lat[i] = lat;
} */

/**
 * Gets the longitude of a given point.
 * @param i - point (0 to max)
 * @return, the point's longitude
 */
double Polyline::getLongitude(size_t i) const
{
	return _lon[i];
}

/**
 * Sets the latitude of a given point.
 * @param i		- point (0 to max)
 * @param lon	- the point's longitude
 *
void Polyline::setLongitude(
		size_t i,
		double lon)
{
	_lon[i] = lon;
} */

/**
 * Gets the quantity of points (vertexes) that make up this Polyline.
 * @return, quantity of points
 */
size_t Polyline::getPoints() const
{
	return _points;
}

}
