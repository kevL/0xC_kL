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

#ifndef OPENXCOM_POLYGON_H
#define OPENXCOM_POLYGON_H

#include <SDL/SDL_stdinc.h>

#include <yaml-cpp/yaml.h>


namespace OpenXcom
{

/**
 * Represents a Polygon on the Globe.
 * @note Polygons constitute the textured land portions of the Globe and
 * typically have 3 or 4 points.
 */
class Polygon
{

private:
	size_t
		_points,
		_texture;
	Sint16
		* _x,
		* _y;
	double
		* _lat,
		* _lon;


	public:
		/// Creates a Polygon with a quantity of points.
		explicit Polygon(size_t points);
		/// Creates a Polygon from an existing one (copy-constructor).
		Polygon(const Polygon& other);
		/// Cleans up the Polygon.
		~Polygon();

		/// Loads the Polygon from YAML.
		void load(const YAML::Node& node);

		/// Gets the latitude of a point.
		double getLatitude(size_t i) const;
		/// Sets the latitude of a point.
		void setLatitude(
				size_t i,
				double lat);
		/// Gets the longitude of a point.
		double getLongitude(size_t i) const;
		/// Sets the longitude of a point.
		void setLongitude(
				size_t i,
				double lon);

		/// Gets the x-coordinate of a point.
		Sint16 getX(size_t i) const;
		/// Sets the x-coordinate of a point.
		void setX(
				size_t i,
				Sint16 x);
		/// Gets the y-coordinate of a point.
		Sint16 getY(size_t i) const;
		/// Sets the y-coordinate of a point.
		void setY(
				size_t i,
				Sint16 y);

		/// Gets the texture-ID of the Polygon.
		size_t getPolyTexture() const;
		/// Sets the texture-ID of the Polygon.
		void setPolyTexture(size_t tex);

		/// Gets the quantity of points in the Polygon.
		size_t getPoints() const;
};

}

#endif
