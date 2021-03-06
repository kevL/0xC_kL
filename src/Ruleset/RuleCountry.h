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

#ifndef OPENXCOM_RULECOUNTRY_H
#define OPENXCOM_RULECOUNTRY_H

//#include <string>

#include <yaml-cpp/yaml.h>


namespace OpenXcom
{

/**
 * Represents a specific funding country.
 * @note Contains constant info like its location in the world and starting
 * funding range.
 */
class RuleCountry
{

private:
	int
		_fundsBasic,
		_fundsLimit,
		_pactScore;
	double
		_labelLon,
		_labelLat;

	std::string
		_region,
		_type;

	std::vector<double>
		_lonMin,
		_lonMax,
		_latMin,
		_latMax;


	public:
		/// Creates a blank country ruleset.
		explicit RuleCountry(const std::string& type);
		/// Cleans up the country ruleset.
		~RuleCountry();

		/// Loads the country from YAML.
		void load(const YAML::Node& node);

		/// Gets the country's type.
		const std::string& getType() const;

		/// Generates the country's starting funding.
		int generateFunding() const;
		/// Gets the country's funding cap.
		int getFundingCap() const;

		/// Gets the country's label x-position.
		double getLabelLongitude() const;
		/// Gets the country's label y-position.
		double getLabelLatitude() const;

		/// Gets the Region in which the label of a Country is based.
		const std::string& getCountryRegion() const;

		/// Checks if a point is inside the country.
		bool insideCountry(
				double lon,
				double lat) const;

		/// Gets this Country's borders.
		const std::vector<double>& getLonMin() const {return _lonMin;}
		const std::vector<double>& getLonMax() const {return _lonMax;}
		const std::vector<double>& getLatMin() const {return _latMin;}
		const std::vector<double>& getLatMax() const {return _latMax;}
		// kL: for reloading country lines in Ruleset::reloadCountryLines(). See also Globe::drawDetail().
		std::vector<double>& getLonMin() {return _lonMin;}
		std::vector<double>& getLonMax() {return _lonMax;}
		std::vector<double>& getLatMin() {return _latMin;}
		std::vector<double>& getLatMax() {return _latMax;}

		/// Gets the aLien-points for signing a pact.
		int getPactScore() const;
};

}

#endif
