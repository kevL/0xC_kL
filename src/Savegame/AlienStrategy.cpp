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

#include "AlienStrategy.h"

//#include <cmath>
#include <cassert>

#include "WeightedOptions.h"

#include "../Ruleset/RuleRegion.h"
#include "../Ruleset/Ruleset.h"


namespace OpenXcom
{

typedef std::map<std::string, WeightedOptions*> MissionsByRegion;


/**
 * Creates an AlienStrategy with no values.
 * @note Running a game like that will most likely crash.
 */
AlienStrategy::AlienStrategy()
{}

/**
 * Frees all resources used by this AlienStrategy.
 */
AlienStrategy::~AlienStrategy()
{
	for (MissionsByRegion::const_iterator
			i = _regionMissions.begin();
			i != _regionMissions.end();
			++i)
		delete i->second;
}

/**
 * Gets starting values from the rules.
 * @param rules - pointer to the game Ruleset
 */
void AlienStrategy::init(const Ruleset* const rules)
{
	const std::vector<std::string> regions (rules->getRegionsList());
	for (std::vector<std::string>::const_iterator
			i = regions.begin();
			i != regions.end();
			++i)
	{
		const RuleRegion* const region (rules->getRegion(*i));
		_regionChances.setWeight(
							*i,
							region->getWeight());

		WeightedOptions* const weightedMissions (new WeightedOptions(region->getAvailableMissions()));
		_regionMissions.insert(std::make_pair(*i, weightedMissions));
	}
}

/**
 * Loads the data from a YAML file.
 * @param node - reference a YAML node
 */
void AlienStrategy::load(const YAML::Node& node)
{
	for (MissionsByRegion::const_iterator
			i = _regionMissions.begin();
			i != _regionMissions.end();
			++i)
		delete i->second;

	_regionMissions.clear();
	_regionChances.clearWeights();
	_regionChances.load(node["regions"]);

	const YAML::Node& strat (node["possibleMissions"]);

	for (YAML::const_iterator
			i = strat.begin();
			i != strat.end();
			++i)
	{
		const std::string region	((*i)["region"].as<std::string>());
		const YAML::Node& missions	((*i)["missions"]);

		WeightedOptions* options(new WeightedOptions());
		options->load(missions);
		_regionMissions.insert(std::make_pair(
											region,
											options));
	}

	_missionLocations	= node["missionLocations"]	.as<std::map<std::string, std::vector<std::pair<std::string, size_t>>>>(_missionLocations);
	_missionRuns		= node["missionsRun"]		.as<std::map<std::string, int>>(_missionRuns);
}

/**
 * Saves the aLien Strategy to a YAML file.
 * @return, YAML node
 */
YAML::Node AlienStrategy::save() const
{
	YAML::Node node;

	node["regions"] = _regionChances.save();

	for (MissionsByRegion::const_iterator
			i = _regionMissions.begin();
			i != _regionMissions.end();
			++i)
	{
		YAML::Node subnode;

		subnode["region"]	= i->first;
		subnode["missions"]	= i->second->save();

		node["possibleMissions"].push_back(subnode);
	}

	node["missionLocations"]	= _missionLocations;
	node["missionsRun"]			= _missionRuns;

	return node;
}

/**
 * Chooses a region that's available for a mission.
 * @param rules - pointer to the Ruleset
 * @return, the region-type
 */
std::string AlienStrategy::chooseRegion(const Ruleset* const rules)
{
	std::string ret (_regionChances.getOptionResult());
	if (ret.empty() == true)
	{
		for (MissionsByRegion::const_iterator
				i = _regionMissions.begin();
				i != _regionMissions.end();
				++i)
			delete i->second;

		_regionMissions.clear();

 		init(rules);
 		ret = _regionChances.getOptionResult();
	}
	assert(ret != "");

	return ret;
}

/**
 * Chooses a mission that's available for a region.
 * @param regionType - reference to a region-type
 * @return, the mission-type
 */
std::string AlienStrategy::chooseMission(const std::string& regionType) const
{
	MissionsByRegion::const_iterator weightedRegions (_regionMissions.find(regionType));
	assert(weightedRegions != _regionMissions.end());

	return weightedRegions->second->getOptionResult();
}

/**
 * Removes @a missionType from the list of possible missions for @a regionType.
 * @param regionType	- reference to the region-type
 * @param missionType	- reference to the mission-type
 * @return, true if there are no more regions with missions available
 */
bool AlienStrategy::clearRegion(
		const std::string& regionType,
		const std::string& missionType)
{
	MissionsByRegion::const_iterator weightedRegions (_regionMissions.find(regionType));
	if (weightedRegions != _regionMissions.end())
	{
		weightedRegions->second->setWeight(missionType, 0u);
		if (weightedRegions->second->hasNoWeight() == true)
		{
			_regionMissions.erase(weightedRegions);
			_regionChances.setWeight(regionType, 0u);
		}
	}

	return (_regionMissions.empty() == true);
}

/**
 * Gets the quantity of a specified mission-type that have run.
 * @param missionType - reference to a mission-type
 * @return, the quantity of missions already run
 */
int AlienStrategy::getMissionsRun(const std::string& missionType)
{
	if (_missionRuns.find(missionType) != _missionRuns.end())
		return _missionRuns[missionType];

	return 0;
}

/**
 * Increments the quantity of a specified mission-type that have run.
 * @param missionType - reference to a mission-type
 */
void AlienStrategy::addMissionRun(const std::string& missionType)
{
//	if (missionType.empty() == false)
//		++_missionRuns[missionType]; // i don't trust that. Perhaps post-fix increment works ....
	if (missionType.empty() == false)
	{
		if (_missionRuns.find(missionType) != _missionRuns.end())
			++_missionRuns[missionType];
		else
			_missionRuns[missionType] = 1;
	}
}

/**
 * Adds a mission-location to the cache.
 * @param missionType	- reference to a mission-type
 * @param regionType	- reference to a region-type
 * @param zone			- the zone-ID within the region
 * @param track			- quantity of entries to maintain in the list
 */
void AlienStrategy::addMissionLocation(
		const std::string& missionType,
		const std::string& regionType,
		size_t zone,
		size_t track)
{
	if (track != 0u)
	{
		_missionLocations[missionType].push_back(std::make_pair(regionType, zone));
		if (_missionLocations[missionType].size() > track)
			_missionLocations.erase(_missionLocations.begin());
	}
}

/**
 * Checks if a given mission-location and zone-ID are stored together in the
 * list of previously attacked locations.
 * @param missionType	- reference to a mission-type
 * @param regionType	- reference to a region-type
 * @param zone			- the zone-ID within the region
 * @return, true if the region is not in the table
 */
bool AlienStrategy::validateMissionLocation(
		const std::string& missionType,
		const std::string& regionType,
		size_t zone)
{
	if (_missionLocations.find(missionType) != _missionLocations.end())
	{
		for (std::vector<std::pair<std::string, size_t>>::const_iterator
				i = _missionLocations[missionType].begin();
				i != _missionLocations[missionType].end();
				++i)
		{
			if ((*i).first == regionType && (*i).second == zone)
				return false;
		}
	}

	return true;
}

/**
 * Checks if a given region is already in the strategy table.
 * @param regionType - reference to a region-type
 * @return, true if the region is in the table
 */
bool AlienStrategy::validateMissionRegion(const std::string& regionType) const
{
	return (_regionMissions.find(regionType) != _regionMissions.end());
}

}
