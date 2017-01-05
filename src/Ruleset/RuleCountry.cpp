/*
 * Copyright 2010-2017 OpenXcom Developers.
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

#include "RuleCountry.h"

#include "../fmath.h"

#include "../Engine/RNG.h"


namespace OpenXcom
{

/**
 * Creates blank rules for a specific Country.
 * @param type - string defining the type
 */
RuleCountry::RuleCountry(const std::string& type)
	:
		_type(type),
		_fundingBase(0),
		_fundingCap(0),
		_labelLon(0.),
		_labelLat(0.),
		_pactScore(0)
{}

/**
 * dTor.
 */
RuleCountry::~RuleCountry()
{}

/**
 * Loads the RuleCountry from a YAML file.
 * @param node - reference a YAML node
 */
void RuleCountry::load(const YAML::Node& node)
{
	_type			= node["type"]			.as<std::string>(_type);
	_fundingBase	= node["fundingBase"]	.as<int>(_fundingBase);
	_fundingCap		= node["fundingCap"]	.as<int>(_fundingCap);
	_labelLon		= node["labelLon"]		.as<double>(_labelLon) * M_PI / 180.; // converts degrees to radians
	_labelLat		= node["labelLat"]		.as<double>(_labelLat) * M_PI / 180.; // converts degrees to radians
	_region			= node["region"]		.as<std::string>();
	_pactScore		= node["pactScore"]		.as<int>(_pactScore);

	std::vector<std::vector<double>> areas;
	areas = node["areas"].as<std::vector<std::vector<double>>>(areas);

	for (size_t
			i = 0;
			i != areas.size();
			++i)
	{
		_lonMin.push_back(areas[i][0] * M_PI / 180.);
		_lonMax.push_back(areas[i][1] * M_PI / 180.);
		_latMin.push_back(areas[i][2] * M_PI / 180.);
		_latMax.push_back(areas[i][3] * M_PI / 180.);

		// safeties ->
//		if (_lonMin.back() > _lonMax.back())
//			std::swap(_lonMin.back(), _lonMax.back());
//		if (_latMin.back() > _latMax.back())
//			std::swap(_latMin.back(), _latMax.back());
	}
}

/**
 * Gets the type-string of the RuleCountry.
 * @note Each country-type is unique.
 * @return, the country-type
 */
const std::string& RuleCountry::getType() const
{
	return _type;
}

/**
 * Generates the random starting funding for the represented Country.
 * @note Countries do not automatically fund the Xcom Project; each has only a
 * chance based on its rule's base-funding value.
 * @return, the monthly funding in $thousands or 0.
 */
int RuleCountry::generateFunding() const
{
	const int funds (RNG::generate( // 50% - 200%
								_fundingBase >> 1u,
								_fundingBase << 1u));
	const int pct (static_cast<int>(static_cast<float>(funds) / 10.f));
	if (RNG::percent(pct) == true)
		return funds;

	return 0;
}

/**
 * Gets the represented Country's funding-cap.
 * @note Country-funding can never exceed this amount.
 * @return, the funding cap in thousands
 */
int RuleCountry::getFundingCap() const
{
	return _fundingCap;
}

/**
 * Gets the longitude of the represented Country's label on the Globe.
 * @return, the longitude in radians
 */
double RuleCountry::getLabelLongitude() const
{
	return _labelLon;
}

/**
 * Gets the latitude of the represented Country's label on the Globe.
 * @return, the latitude in radians
 */
double RuleCountry::getLabelLatitude() const
{
	return _labelLat;
}

/**
 * Gets the Region in which the label of a Country is based.
 * @return, the region-type string
 */
const std::string& RuleCountry::getCountryRegion() const
{
	return _region;
}

/**
 * Checks if a point is inside the represented Country's borders.
 * @param lon - longitude in radians
 * @param lat - latitude in radians
 * @return, true if inside
 */
bool RuleCountry::insideCountry(
		double lon,
		double lat) const
{
	for (size_t
			i = 0u;
			i != _lonMin.size();
			++i)
	{
		bool
			inLon,
			inLat;

		if (_lonMin[i] <= _lonMax[i])
			inLon = (lon >= _lonMin[i] && lon < _lonMax[i]);
		else
			inLon = (lon >= _lonMin[i] && lon < M_PI * 2.)
					|| (lon >= 0. && lon < _lonMax[i]);

		inLat = (lat >= _latMin[i] && lat < _latMax[i]);

		if (inLon && inLat)
			return true;
	}

	return false;
}

/**
 * Gets the aLien-points for signing a pact.
 * @return, score
 */
int RuleCountry::getPactScore() const
{
	return _pactScore;
}

}
