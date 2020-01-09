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

#ifndef OPENXCOM_RULEREGION_H
#define OPENXCOM_RULEREGION_H

#include <string>
#include <vector>

#include "../fmath.h"

#include <yaml-cpp/yaml.h>

#include "../Savegame/WeightedOptions.h"


namespace OpenXcom
{

/**
 * A rectangle in polar coordinates that defines a distinct subset of a larger
 * MissionZone.
 * @note MissionZones are abstract; their MissionAreas can be located anywhere
 * on the Globe and they usually don't even intersect/overlap.
 */
struct MissionArea
{
	int texture;
	double
		lonMin,
		lonMax,
		latMin,
		latMax;
	std::string site;

	///
	bool operator ==(const MissionArea& area) const
	{
		return AreSameTwo(
						lonMax, area.lonMax,
						lonMin, area.lonMin) == true
			&& AreSameTwo(
						latMax, area.latMax,
						latMin, area.latMin) == true;
	}

	///
	bool isPoint() const
	{
		return AreSameTwo(
						lonMin, lonMax,
						latMin, latMax) == true;
	}
};


/**
 * A zone (a superset of MissionAreas) on the Globe.
 */
struct MissionZone
{
	std::vector<MissionArea> areas;

	///
	void swap(MissionZone& other)
	{
		areas.swap(other.areas);
	}
};


class RuleCity;
class Target;

/**
 * Represents a specific region of the world.
 * @note Contains constant info about a region like area covered and base
 * construction costs.
 */
class RuleRegion
{

private:
	int _buildCost;
	std::string _type;

	std::vector<double>
		_lonMin,
		_lonMax,
		_latMin,
		_latMax;

	std::vector<RuleCity*> _cities;

	size_t _weight;						// weight of this Region when selecting regions for AlienMissions.
	WeightedOptions _weightsMission;	// weighted list of the different mission-types for this Region.
	std::vector<MissionZone> _zones;	// all the mission-zones in this Region.
	std::string _missionRegion;			// do missions in the Region defined by this string instead.


	public:
		static const size_t MZ_CITY = 3u;


		/// Creates a blank RuleRegion.
		explicit RuleRegion(const std::string& type);
		/// Cleans up the RuleRegion.
		~RuleRegion();

		/// Loads the RuleRegion from YAML.
		void load(const YAML::Node& node);

		/// Gets the RuleRegion's type.
		const std::string& getType() const;

		/// Gets the RuleRegion's cost to construct a Base.
		int getBaseCost() const;

		/// Checks if a point is inside the RuleRegion.
		bool insideRegion(
				double lon,
				double lat) const;

		/// Gets the Cities in the RuleRegion.
		const std::vector<RuleCity*>& getCities();

		/// Gets the weight of the RuleRegion for alien-mission-selection.
		size_t getWeight() const;
		/// Gets the weighted list of missions for the RuleRegion.
		const WeightedOptions& getAvailableMissions() const
		{ return _weightsMission; }

		/// Gets the substitute region-type for an alien-mission.
		const std::string& getMissionRegion() const
		{ return _missionRegion; }

		/// Gets the list of MissionZones in the RuleRegion.
		const std::vector<MissionZone>& getMissionZones() const;

		/// Gets a random point inside a MissionZone.
		std::pair<double, double> getZonePoint(size_t zoneId) const;
		/// Gets a random point inside a MissionArea.
		std::pair<double, double> getAreaPoint(const MissionArea& area) const;

		/// Gets the MissionArea for a corresponding MissionZone and Target.
		MissionArea getTerrorArea(
				size_t zoneId,
				const Target* const target) const;
		/// Gets a random MissionArea in the RuleRegion.
//		MissionArea getRandomMissionPoint(size_t zone) const;

		/// Gets the RuleRegion's borders.
		const std::vector<double>& getLonMax() const {return _lonMax;}
		const std::vector<double>& getLonMin() const {return _lonMin;}
		const std::vector<double>& getLatMax() const {return _latMax;}
		const std::vector<double>& getLatMin() const {return _latMin;}
};

}


namespace YAML
{

/**
 *
 */
template<>
struct convert<OpenXcom::MissionArea>
{
	///
	static Node encode(const OpenXcom::MissionArea& rhs)
	{
		Node node;
		node.push_back(rhs.lonMin / M_PI * 180.);
		node.push_back(rhs.lonMax / M_PI * 180.);
		node.push_back(rhs.latMin / M_PI * 180.);
		node.push_back(rhs.latMax / M_PI * 180.);

		return node;
	}

	///
	static bool decode(
			const Node& node,
			OpenXcom::MissionArea& rhs)
	{
		if (node.IsSequence() == false || node.size() < 4)
			return false;

		rhs.lonMin = node[0u].as<double>() * M_PI / 180.;
		rhs.lonMax = node[1u].as<double>() * M_PI / 180.;
		rhs.latMin = node[2u].as<double>() * M_PI / 180.;
		rhs.latMax = node[3u].as<double>() * M_PI / 180.;

		// safeties ->
//		if (rhs.lonMin > rhs.lonMax) std::swap(rhs.lonMin, rhs.lonMax);
//		if (rhs.latMin > rhs.latMax) std::swap(rhs.latMin, rhs.latMax);

		if (node.size() > 4u) rhs.texture = node[4u].as<int>();
		if (node.size() > 5u) rhs.site    = node[5u].as<std::string>();

		// TODO: needs entries #7, #8; labelTop & showNameAtZoomLevel
		return true;
	}
};

/**
 *
 */
template<>
struct convert<OpenXcom::MissionZone>
{
	///
	static Node encode(const OpenXcom::MissionZone& rhs)
	{
		Node node;
		node = rhs.areas;

		return node;
	}

	///
	static bool decode(
			const Node& node,
			OpenXcom::MissionZone& rhs)
	{
		if (node.IsSequence() == false)
			return false;

		rhs.areas = node.as<std::vector<OpenXcom::MissionArea>>(rhs.areas);

		return true;
	}
};

}

#endif
