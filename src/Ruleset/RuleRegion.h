/*
 * Copyright 2010-2015 OpenXcom Developers.
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

#ifndef _USE_MATH_DEFINES
	#define _USE_MATH_DEFINES
#endif

#include <cmath>
#include <string>
#include <vector>

#include <yaml-cpp/yaml.h>

#include "../fmath.h"

#include "../Savegame/WeightedOptions.h"


namespace OpenXcom
{

/**
 * Defines a rectangle in polar coordinates used to define areas for a MissionZone.
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
	bool operator== (const MissionArea& area) const
	{
		return AreSame(lonMax, area.lonMax)
			&& AreSame(lonMin, area.lonMin)
			&& AreSame(latMax, area.latMax)
			&& AreSame(latMin, area.latMin);
	}

	///
	bool isPoint() const
	{
		return AreSame(lonMin, lonMax)
			&& AreSame(latMin, latMax);
	}
};


/**
 * A zone (set of MissionAreas) on the globe.
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

	WeightedOptions _missionWeights;		// Weighted list of the different mission types for this region.
	size_t _regionWeight;					// Weight of this region when selecting regions for alien missions.
	std::vector<MissionZone> _missionZones;	// All the mission zones in this region.
	std::string _missionRegion;				// Do missions in the region defined by this string instead.


	public:
		static const int MZ_CITY = 3;


		/// Creates a blank Region ruleset.
		explicit RuleRegion(const std::string& type);
		/// Cleans up the Region ruleset.
		~RuleRegion();

		/// Loads the Region from YAML.
		void load(const YAML::Node& node);

		/// Gets the Region's type.
		const std::string& getType() const;

		/// Gets the Region's cost to construct a Base.
		int getBaseCost() const;

		/// Checks if a point is inside the Region.
		bool insideRegion(
				double lon,
				double lat) const;

		/// Gets the Cities in the Region.
		std::vector<RuleCity*>* getCities();

		/// Gets the weight of the Region for mission selection.
		size_t getWeight() const;
		/// Gets the weighted list of missions for the Region.
		const WeightedOptions& getAvailableMissions() const
		{ return _missionWeights; }

		/// Gets the substitute Mission Region.
		const std::string& getMissionRegion() const
		{ return _missionRegion; }

		/// Gets the list of MissionZones in the Region.
		const std::vector<MissionZone>& getMissionZones() const;

		/// Gets a random point inside a MissionZone.
		std::pair<double, double> getRandomPoint(size_t zone) const;
		/// Gets the MissionArea for a corresponding zone and target.
		MissionArea getMissionPoint(
				size_t zone,
				const Target* const target) const;
		/// Gets a random MissionArea in the Region.
//		MissionArea getRandomMissionPoint(size_t zone) const;

		/// Gets the Region's borders.
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
		if (node.IsSequence() == false
			|| node.size() < 4)
		{
			return false;
		}

		rhs.lonMin = node[0].as<double>() * M_PI / 180.;
		rhs.lonMax = node[1].as<double>() * M_PI / 180.;
		rhs.latMin = node[2].as<double>() * M_PI / 180.;
		rhs.latMax = node[3].as<double>() * M_PI / 180.;

		// safeties ->
//		if (rhs.lonMin > rhs.lonMax)
//			std::swap(rhs.lonMin, rhs.lonMax);
//		if (rhs.latMin > rhs.latMax)
//			std::swap(rhs.latMin, rhs.latMax);

		if (node.size() > 4)
			rhs.texture	= node[4].as<int>();
		if (node.size() > 5)
			rhs.site	= node[5].as<std::string>();

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

		rhs.areas = node.as<std::vector<OpenXcom::MissionArea> >(rhs.areas);

		return true;
	}
};

}

#endif
