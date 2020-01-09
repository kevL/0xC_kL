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

#include "Polygon.h"

#include "../fmath.h"


namespace OpenXcom
{

/**
 * Initializes the Polygon with arrays that store each point's coordinates.
 * @param points - quantity of points
 */
Polygon::Polygon(size_t points)
	:
		_points(points),
		_texture(0u)
{
	_lat = new double[_points];
	_lon = new double[_points];
	_x = new Sint16[_points];
	_y = new Sint16[_points];

	for (size_t
			i = 0u;
			i != _points;
			++i)
	{
		_lat[i] =
		_lon[i] = 0.;
		_x[i] =
		_y[i] = 0;
	}
}

/**
 * Performs a deep copy of an existing Polygon.
 * @param other - reference to another Polygon to copy
 */
Polygon::Polygon(const Polygon& other)
{
	_points	= other._points;
	_lat = new double[_points];
	_lon = new double[_points];
	_x = new Sint16[_points];
	_y = new Sint16[_points];

	for (size_t
			i = 0u;
			i != _points;
			++i)
	{
		_lat[i] = other._lat[i];
		_lon[i] = other._lon[i];
		_x[i] = other._x[i];
		_y[i] = other._y[i];
	}

	_texture = other._texture;
}

/**
 * Deletes this Polygon's arrays from memory.
 */
Polygon::~Polygon()
{
	delete[] _lat;
	delete[] _lon;
	delete[] _x;
	delete[] _y;
}

/**
 * Loads this Polygon from a YAML file.
 * @param node - reference a YAML node
 */
void Polygon::load(const YAML::Node& node)
{
	delete[] _lat;
	delete[] _lon;
	delete[] _x;
	delete[] _y;

	const std::vector<double> coords (node.as<std::vector<double>>());
	_points = (coords.size() - 1u) >> 1u;

	_lat = new double[_points];
	_lon = new double[_points];

	_x = new Sint16[_points];
	_y = new Sint16[_points];

	_texture = static_cast<size_t>(coords[0u]);

	for (size_t
			i = 1u, j;
			i < coords.size();
			i += 2u)
	{
		j = (i - 1u) >> 1u;

		_lon[j] = coords[i] * M_PI / 180.;
		_lat[j] = coords[i + 1u] * M_PI / 180.;

		_x[j] =
		_y[j] = 0;
	}
}

/**
 * Gets the latitude of a given point.
 * @param i - point (0 to max)
 * @return, the point's latitude
 */
double Polygon::getLatitude(size_t i) const
{
	return _lat[i];
}

/**
 * Sets the latitude of a given point.
 * @param i		- point (0 to max)
 * @param lat	- the point's latitude
 */
void Polygon::setLatitude(
		size_t i,
		double lat)
{
	_lat[i] = lat;
}

/**
 * Gets the longitude of a given point.
 * @param i - point (0 to max)
 * @return, the point's longitude
 */
double Polygon::getLongitude(size_t i) const
{
	return _lon[i];
}

/**
 * Changes the latitude of a given point.
 * @param i		- point (0 to max)
 * @param lon	- the point's longitude
 */
void Polygon::setLongitude(
		size_t i,
		double lon)
{
	_lon[i] = lon;
}

/**
 * Gets the x-coordinate of a given point.
 * @param i - point (0 to max)
 * @return, the point's x-coordinate
 */
Sint16 Polygon::getX(size_t i) const
{
	return _x[i];
}

/**
 * Sets the x-coordinate of a given point.
 * @param i - point (0 to max)
 * @param x - the point's x-coordinate
 */
void Polygon::setX(
		size_t i,
		Sint16 x)
{
	_x[i] = x;
}

/**
 * Gets the y-coordinate of a given point.
 * @param i - point (0 to max)
 * @return, the point's y-coordinate
 */
Sint16 Polygon::getY(size_t i) const
{
	return _y[i];
}

/**
 * Sets the y-coordinate of a given point.
 * @param i - point (0 to max)
 * @param y - the point's y-coordinate
 */
void Polygon::setY(
		size_t i,
		Sint16 y)
{
	_y[i] = y;
}

/**
 * Gets the texture used to draw this Polygon.
 * @note Textures are stored in a set.
 * @return, texture sprite-ID
 */
size_t Polygon::getPolyTexture() const
{
	return _texture;
}

/**
 * Sets the texture used to draw this Polygon.
 * @param tex - texture sprite-ID
 */
void Polygon::setPolyTexture(size_t tex)
{
	_texture = tex;
}

/**
 * Gets the quantity of points (vertices) that make up this Polygon.
 * @return, quantity of points
 */
size_t Polygon::getPoints() const
{
	return _points;
}

}
