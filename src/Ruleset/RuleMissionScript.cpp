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

#include "RuleMissionScript.h"

#include "../Engine/Exception.h"


namespace OpenXcom
{

/**
 * RuleMissionScript cTor - the rules for Alien Mission propagation.
 * @note The geoscape will probe the scripts each month to determine what's
 * going to happen.
 * @param type - reference the type ID
 */
RuleMissionScript::RuleMissionScript(const std::string& type)
	:
		_type(type),
		_firstMonth(0),
		_lastMonth(-1),
		_label(0),
		_executionOdds(100),
		_targetBaseOdds(0),
		_minDifficulty(DIFF_BEGINNER),
		_maxRuns(-1),
		_avoidRepeats(0),
		_delay(0),
		_useTable(true),
		_siteType(false)
{}

/**
 * Destructor.
 * @note Cleans up the mess Warboy left in RAM.
 */
RuleMissionScript::~RuleMissionScript()
{
	for (std::vector<std::pair<size_t, WeightedOptions*>>::const_iterator
			i = _missionWeights.begin();
			i != _missionWeights.end();
			++i)
		delete i->second;

	for (std::vector<std::pair<size_t, WeightedOptions*>>::const_iterator
			i = _raceWeights.begin();
			i != _raceWeights.end();
			++i)
		delete i->second;

	for (std::vector<std::pair<size_t, WeightedOptions*>>::const_iterator
			i = _regionWeights.begin();
			i != _regionWeights.end();
			++i)
		delete i->second;
}

/**
 * Loads a missionScript from a YAML file.
 * @param node - reference a YAML node
 */
void RuleMissionScript::load(const YAML::Node& node)
{
	_varType		= node["varType"]		.as<std::string>(_varType);
	_firstMonth		= node["firstMonth"]	.as<int>(_firstMonth);
	_lastMonth		= node["lastMonth"]		.as<int>(_lastMonth);
	_label			= node["label"]			.as<unsigned int>(_label);
	_executionOdds	= node["executionOdds"]	.as<int>(_executionOdds);
	_targetBaseOdds	= node["targetBaseOdds"].as<int>(_targetBaseOdds);
	_maxRuns		= node["maxRuns"]		.as<int>(_maxRuns);
	_avoidRepeats	= node["avoidRepeats"]	.as<int>(_avoidRepeats);
	_delay			= node["startDelay"]	.as<int>(_delay);
	_conditions		= node["conditions"]	.as<std::vector<int>>(_conditions);

	_minDifficulty = static_cast<DifficultyLevel>(node["minDifficulty"].as<int>(_minDifficulty));

	WeightedOptions* weightOpt;

	if (const YAML::Node& weights = node["missionWeights"])
	{
		for (YAML::const_iterator
				i = weights.begin();
				i != weights.end();
				++i)
		{
			weightOpt = new WeightedOptions();
			weightOpt->load(i->second);
			_missionWeights.push_back(std::make_pair(
												i->first.as<size_t>(0),
												weightOpt));
		}
	}

	if (const YAML::Node& weights = node["raceWeights"])
	{
		for (YAML::const_iterator
				i = weights.begin();
				i != weights.end();
				++i)
		{
			weightOpt = new WeightedOptions();
			weightOpt->load(i->second);
			_raceWeights.push_back(std::make_pair(
											i->first.as<size_t>(0),
											weightOpt));
		}
	}

	if (const YAML::Node& weights = node["regionWeights"])
	{
		for (YAML::const_iterator
				i = weights.begin();
				i != weights.end();
				++i)
		{
			weightOpt = new WeightedOptions();
			weightOpt->load(i->second);
			_regionWeights.push_back(std::make_pair(
												i->first.as<size_t>(0),
												weightOpt));
		}
	}

	_researchTriggers	= node["researchTriggers"]	.as<std::map<std::string, bool>>(_researchTriggers);
	_useTable			= node["useTable"]			.as<bool>(_useTable);

	if (_varType.empty() == true
		&& (_maxRuns > 0 || _avoidRepeats > 0))
	{
		throw Exception("ERROR in MissionScript: " + _type
			+ ": no varType provided for a script with maxRuns or repeatAvoidance.");
	}
}

/**
 * Gets the type of this MissionScript.
 * @return, the type as string
 */
std::string RuleMissionScript::getType() const
{
	return _type;
}

/**
 * Gets the variable used to track stuff in the SavedGame.
 * @return, the varType as string
 */
std::string RuleMissionScript::getVarType() const
{
	return _varType;
}

/**
 * Gets the first month this MissionScript is allowed to execute.
 * @return, the first month
 */
int RuleMissionScript::getFirstMonth() const
{
	return _firstMonth;
}

/**
 * Gets the last month this MissionScript is allowed to execute.
 * @return, the last month
 */
int RuleMissionScript::getLastMonth() const
{
	return _lastMonth;
}

/**
 * Gets the label of this MissionScript for its conditions.
 * @return, the label
 */
int RuleMissionScript::getLabel() const
{
	return _label;
}

/**
 * Gets the odds of this MissionScript executing.
 * @return, the odds of execution
 */
int RuleMissionScript::getExecutionOdds() const
{
	return _executionOdds;
}

/**
 * Gets the odds of this MissionScript targeting an xCom Base.
 * @return, the odds of targeting a base
 */
int RuleMissionScript::getTargetBaseOdds() const
{
	return _targetBaseOdds;
}

/**
 * Gets the minimum game-difficulty this MissionScript is allowed to execute at.
 * @return, minimum difficulty (SavedGame.h)
 */
DifficultyLevel RuleMissionScript::getMinDifficulty() const
{
	return _minDifficulty;
}

/**
 * Gets the maximum number of times a directive with this MissionScript's varType
 * is allowed to execute.
 * @return, maximum runs
 */
int RuleMissionScript::getMaxRuns() const
{
	return _maxRuns;
}

/**
 * Gets the quantity of previous mission-sites to track.
 * @return, the quantity of repeats allowed
 */
int RuleMissionScript::getRepeatAvoidance() const
{
	return _avoidRepeats;
}

/**
 * Gets the quantity of minutes to delay spawning a first wave.
 * @note Overrides the spawn-delay defined in the mission-waves.
 * @return, delay
 */
int RuleMissionScript::getDelay() const
{
	return _delay;
}

/**
 * Gets a list of conditions this MissionScript requires before it is allowed to execute.
 * @return, reference to a vector of ints
 */
const std::vector<int>& RuleMissionScript::getConditions() const
{
	return _conditions;
}

/**
 * Gets if this MissionScript uses a weighted distribution to pick a race.
 * @return, true if race-weights
 */
bool RuleMissionScript::hasRaceWeights() const
{
	return (_raceWeights.empty() == false);
}

/**
 * Gets if this MissionScript uses a weighted distribution to pick a mission.
 * @return, true if mission-weights
 */
bool RuleMissionScript::hasMissionWeights() const
{
	return (_missionWeights.empty() == false);
}

/**
 * Gets if this MissionScript uses a weighted distribution to pick a region.
 * @return, true if region-weights
 */
bool RuleMissionScript::hasRegionWeights() const
{
	return (_regionWeights.empty() == false);
}

/**
 * Gets any research triggers that apply to this MissionScript.
 * @return, map of strings to bools
 */
const std::map<std::string, bool>& RuleMissionScript::getResearchTriggers() const
{
	return _researchTriggers;
}

/**
 * Gets if this command should remove the mission it generates from the strategy
 * table.
 * @note Stops it coming up again in random selection but NOT if a missionScript
 * calls it by name.
 * @return, true to use table
 */
bool RuleMissionScript::usesTable() const
{
	return _useTable;
}

/**
 * Gets a list of all the mission types this MissionScript can generate.
 * @return, set of strings
 */
const std::set<std::string> RuleMissionScript::getAllMissionTypes() const
{
	std::set<std::string> ret;
	for (std::vector<std::pair<size_t, WeightedOptions*>>::const_iterator
			i = _missionWeights.begin();
			i != _missionWeights.end();
			++i)
	{
		std::vector<std::string> missionTypes ((*i).second->getTypes());
		for (std::vector<std::string>::const_iterator
				j = missionTypes.begin();
				j != missionTypes.end();
				++j)
		{
			ret.insert(*j);
		}
	}
	return ret;
}

/**
 * Gets a list of the possible missions for the given month.
 * @param month - the month for info
 * @return, vector of strings
 */
const std::vector<std::string> RuleMissionScript::getMissionTypes(const size_t month) const
{
	std::vector<std::string> ret;
	std::vector<std::pair<size_t, WeightedOptions*>>::const_reverse_iterator rit (_missionWeights.rbegin());
	while (month < rit->first)
	{
		++rit;
		if (rit == _missionWeights.rend())
		{
			--rit;
			break;
		}
	}

	std::vector<std::string> missionTypes (rit->second->getTypes());
	for (std::vector<std::string>::const_iterator
			i = missionTypes.begin();
			i != missionTypes.end();
			++i)
	{
		ret.push_back(*i);
	}
	return ret;
}

/**
 * Gets the list of regions to pick from this month.
 * @param month - the month for info
 * @return, vector of strings
 */
const std::vector<std::string> RuleMissionScript::getRegions(const size_t month) const
{
	std::vector<std::string> ret;
	std::vector<std::pair<size_t, WeightedOptions*>>::const_reverse_iterator rit (_regionWeights.rbegin());
	while (month < rit->first)
	{
		++rit;
		if (rit == _regionWeights.rend())
		{
			--rit;
			break;
		}
	}

	std::vector<std::string> regionTypes (rit->second->getTypes());
	for (std::vector<std::string>::const_iterator
			i = regionTypes.begin();
			i != regionTypes.end();
			++i)
	{
		ret.push_back(*i);
	}
	return ret;
}

/**
 * Chooses one of the available races, regions, or missions for this command.
 * @param monthsPassed	- number of months that have passed in the game world
 * @param type			- type of thing to generate; region, mission, or race
 * @return, string-id of the type generated
 */
std::string RuleMissionScript::genDataType(
		const size_t monthsPassed,
		const GenerationType type) const
{
	std::vector<std::pair<size_t, WeightedOptions*>>::const_reverse_iterator rit;

	switch (type)
	{
		case GT_RACE:
			rit = _raceWeights.rbegin();
			break;

		case GT_REGION:
			rit = _regionWeights.rbegin();
			break;

		default:
		case GT_MISSION:
			rit = _missionWeights.rbegin();
	}

	while (monthsPassed < rit->first)
		++rit;

	return rit->second->getOptionResult();
}

/**
 * Sets this MissionScript to a site-mission type directive or not.
 * @param siteType - true if site-type
 */
void RuleMissionScript::setSiteType(const bool siteType)
{
	_siteType = siteType;
}

/**
 * Gets if this MissionScript is a site-mission type or not.
 * @return, true if site-type
 */
bool RuleMissionScript::getSiteType() const
{
	return _siteType;
}

}
