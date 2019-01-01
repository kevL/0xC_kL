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

#ifndef OPENXCOM_POLYLINE_H
#define OPENXCOM_POLYLINE_H

#include <yaml-cpp/yaml.h>


namespace OpenXcom
{

/**
 * Represents a polyline on the Globe.
 * @note Polylines constitute the detailed portions of the Globe and typically
 * represent borders and rivers.
 */
class Polyline
{

private:
	size_t _points;
	double
		* _lat,
		* _lon;


	public:
		/// Creates a Polyline with a specified quantity of points.
		explicit Polyline(size_t points);
		/// Cleans up the Polyline.
		~Polyline();

		/// Loads the Polyline from YAML.
		void load(const YAML::Node& node);

		/// Gets the latitude of a point.
		double getLatitude(size_t i) const;
		/// Sets the latitude of a point.
/*		void setLatitude(
				size_t i,
				double lat); */
		/// Gets the longitude of a point.
		double getLongitude(size_t i) const;
		/// Sets the longitude of a point.
/*		void setLongitude(
				size_t i,
				double lon); */

		/// Gets the quantity of points in the Polyline.
		size_t getPoints() const;
};

}

#endif
