/*
 * Copyright 2010-2018 OpenXcom Developers.
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

#include "RuleRegion.h"

//#include <assert.h>

#include "RuleCity.h"

#include "../Engine/RNG.h"


namespace OpenXcom
{

/**
 * Creates a RuleRegion of a specified type.
 * @param type - reference to the type
 */
RuleRegion::RuleRegion(const std::string& type)
	:
		_type(type),
		_buildCost(0),
		_weight(0u)
{}

/**
 * Deletes this RuleRegion from memory.
 */
RuleRegion::~RuleRegion()
{
	for (std::vector<RuleCity*>::const_iterator
			i = _cities.begin();
			i != _cities.end();
			++i)
		delete *i;
}

/**
 * Loads this RuleRegion from a YAML file.
 * @param node - reference a YAML node
 */
void RuleRegion::load(const YAML::Node& node)
{
	_type		= node["type"]		.as<std::string>(_type);
	_buildCost	= node["buildCost"]	.as<int>(_buildCost);

	std::vector<std::vector<double>> areas (node["areas"].as<std::vector<std::vector<double>>>());
	for (size_t
			i = 0u;
			i != areas.size();
			++i)
	{
		_lonMin.push_back(areas[i][0u] * M_PI / 180.); // converts degrees to radians ->
		_lonMax.push_back(areas[i][1u] * M_PI / 180.);
		_latMin.push_back(areas[i][2u] * M_PI / 180.);
		_latMax.push_back(areas[i][3u] * M_PI / 180.);

		// safeties ->
//		if (_lonMin.back() > _lonMax.back()) std::swap(_lonMin.back(), _lonMax.back());
//		if (_latMin.back() > _latMax.back()) std::swap(_latMin.back(), _latMax.back());
	}

	// TODO: if ["delete"] delete previous mission zones.
	// NOTE: the next line replaces previous zones:
	_zones = node["zones"].as<std::vector<MissionZone>>(_zones);
	// NOTE: while the following lines add to zones:
//	std::vector<MissionZone> misZones = node["zones"].as<std::vector<MissionZone>>(misZones);
//	_zones.insert(
//					_zones.end(),
//					misZones.begin(),
//					misZones.end());
	// revert that until it gets worked out. FalcoOXC says,
	/* So if two mods add zones how do the trajectory
	references and negative texture entries remain in sync? */

	// kL_begin:
	MissionArea area (*_zones.at(MZ_CITY).areas.begin());

	// Delete a possible placeholder in the Geography ruleset, by removing
	// its pointlike MissionArea at MissionZone[3] MZ_CITY; ie [0,0,0,0].
	// Note that safeties have been removed on all below_ ...
	if (area.isPoint() == true)
		_zones.at(MZ_CITY).areas.erase(_zones.at(MZ_CITY).areas.begin());

	if (const YAML::Node& cities = node["cities"])
	{
		RuleCity* city;
		for (YAML::const_iterator // load all Cities that are in YAML-ruleset
				i = cities.begin();
				i != cities.end();
				++i)
		{
			city = new RuleCity();
			city->load(*i);
			_cities.push_back(city);

			area.lonMin =
			area.lonMax = city->getLongitude();
			area.latMin =
			area.latMax = city->getLatitude();

			area.site = city->getLabel();
			area.texture = city->getTextureId();

			_zones.at(MZ_CITY).areas.push_back(area);
		}
	} // end_kL.

	if (const YAML::Node& weights = node["weightsMission"])
		_weightsMission.load(weights);

	_weight			= node["weight"]		.as<size_t>(_weight);
	_missionRegion	= node["missionRegion"]	.as<std::string>(_missionRegion);
}

/**
 * Gets the string that types this RuleRegion.
 * @note Each region-type has a unique label.
 * @return, a reference to the region-type
 */
const std::string& RuleRegion::getType() const
{
	return _type;
}

/**
 * Gets the cost of building a base inside this RuleRegion.
 * @return, the construction cost
 */
int RuleRegion::getBaseCost() const
{
	return _buildCost;
}

/**
 * Checks if a point is inside this RuleRegion.
 * @param lon - longitude in radians
 * @param lat - latitude in radians
 * @return, true if point is inside the region
 */
bool RuleRegion::insideRegion(
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
			inLon = (lon >= _lonMin[i]
				  && lon <  _lonMax[i]);
		else
			inLon = ((lon >= _lonMin[i] && lon <  M_PI * 2.)
				  || (lon <  _lonMax[i] && lon >= 0.));

		inLat = (lat >= _latMin[i]
			  && lat <  _latMax[i]);

		if (inLon == true && inLat == true)
			return true;
	}
	return false;
}

/**
 * Gets the list of cities contained in this RuleRegion.
 * @note Build & cache a vector of all MissionAreas that are Cities.
 * @return, reference to a vector of pointers to Cities
 */
const std::vector<RuleCity*>& RuleRegion::getCities()
{
//	if (_cities.empty() == true) // kL_note: unused for now. Just return the cities, thanks anyway.
//		for (std::vector<MissionZone>::const_iterator
//				i = _zones.begin();
//				i != _zones.end();
//				++i)
//			for (std::vector<MissionArea>::const_iterator
//					j = i->areas.begin();
//					j != i->areas.end();
//					++j)
//				if (j->isPoint() == true && j->site.empty() == false)
//					_cities.push_back(new RuleCity(
//												j->site,
//												j->lonMin,
//												j->latMin));
	return _cities;
}

/**
 * Gets the weight of this RuleRegion for AlienMission selection.
 * @note This is only used when creating a new game since these weights change
 * in the course of the game.
 * @return, the initial weight
 */
size_t RuleRegion::getWeight() const
{
	return _weight;
}

/**
 * Gets a list of all MissionZones in this RuleRegion.
 * @return, reference to a vector of MissionZones
 */
const std::vector<MissionZone>& RuleRegion::getMissionZones() const
{
	return _zones;
}

/**
 * Gets a random point that is guaranteed to be inside a specified MissionZone.
 * @param zoneId - the zone-ID
 * @return, a pair of longitude and latitude
 */
std::pair<double, double> RuleRegion::getZonePoint(size_t zoneId) const
{
	if (zoneId < _zones.size()) // safety.
	{
		const MissionZone& zone (_zones[zoneId]);
		return getAreaPoint(zone.areas[RNG::pick(zone.areas.size())]);
	}
	return std::make_pair(0.,0.);
}

/**
 * Gets a random point that is guaranteed to be inside a specified MissionArea.
 * @param area - reference to a MissionArea
 * @return, a pair of longitude and latitude
 */
std::pair<double, double> RuleRegion::getAreaPoint(const MissionArea& area) const
{
	const double
		lon (RNG::generate(
						area.lonMin,
						area.lonMax)),
		lat (RNG::generate(
						area.latMin,
						area.latMax));

	return std::make_pair(lon,lat);
}

/**
 * Gets the MissionArea (a City) for a terror-point in a specified zone-ID and
 * at the specified Target coordinates.
 * @param zoneId - the zone-ID
 * @param target - pointer to a Target for coordinate match
 * @return, a MissionArea
 */
MissionArea RuleRegion::getTerrorArea(
		size_t zoneId,
		const Target* const target) const
{
	if (zoneId < _zones.size()) // safety.
	{
		for (std::vector<MissionArea>::const_iterator
				i = _zones[zoneId].areas.begin();
				i != _zones[zoneId].areas.end();
				++i)
		{
			if (i->isPoint() == true
				&& AreSameTwo(
							target->getLongitude(), i->lonMin,
							target->getLatitude(),  i->latMin) == true)
			{
				return *i;
			}
		}
	}
	return MissionArea();
}

}

/**
 * Gets the area-data for the random mission-point in this RuleRegion.
 * @param zoneId - the zone-ID
 * @return, a MissionArea from which to extract coordinates, textures, or any other pertinent information
 *
MissionArea RuleRegion::getRandomMissionPoint(size_t zoneId) const
{
	if (zoneId < _zones.size())
	{
		std::vector<MissionArea> randArea = _zones[zoneId].areas;
		for (std::vector<MissionArea>::const_iterator
				i = randArea.begin();
				i != randArea.end();
				)
		{
			if (i->isPoint() == false)
				i = randArea.erase(i);
			else
				++i;
		}

		if (randArea.empty() == false)
			return randArea.at(static_cast<size_t>(RNG::generate(0,
							   static_cast<int>(randArea.size()) - 1)));

	}

//	assert(0 && "Invalid zoneId");
	return MissionArea();
} */
