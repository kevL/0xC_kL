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

#include "RuleAlienMission.h"

#include "../Savegame/WeightedOptions.h"


namespace YAML
{

template<>
struct convert<OpenXcom::MissionWave>
{
	///
	static Node encode(const OpenXcom::MissionWave& rhs)
	{
		Node node;

		node["ufo"]			= rhs.ufoType;
		node["count"]		= rhs.ufoTotal;
		node["trajectory"]	= rhs.trajectory;
		node["timer"]		= rhs.spawnTimer;
		node["isObjective"]	= rhs.isObjective;

		return node;
	}

	///
	static bool decode(
			const Node& node,
			OpenXcom::MissionWave& rhs)
	{
		if (node.IsMap() == false)
			return false;

		rhs.ufoType		= node["ufo"]			.as<std::string>();
		rhs.ufoTotal	= node["count"]			.as<int>();
		rhs.trajectory	= node["trajectory"]	.as<std::string>();
		rhs.spawnTimer	= node["timer"]			.as<int>();
		rhs.isObjective	= node["isObjective"]	.as<bool>(false);

		return true;
	}
};

}


namespace OpenXcom
{

/**
 * Creates an AlienMission rule.
 * @param type - reference the mission type
 */
RuleAlienMission::RuleAlienMission(const std::string& type)
	:
		_type(type),
		_points(0),
		_objectiveType(alm_SCORE),
		_specialZone(std::numeric_limits<size_t>::max()),
		_retalCoef(-1)
{}

/**
 * Ensures that allocated memory is released.
 */
RuleAlienMission::~RuleAlienMission()
{
	for (std::vector<std::pair<size_t, WeightedOptions*>>::const_iterator
			i = _raceDistribution.begin();
			i != _raceDistribution.end();
			++i)
		delete i->second;
}

/**
 * Loads the mission-data from a YAML node.
 * @param node - YAML node
 */
void RuleAlienMission::load(const YAML::Node& node)
{
	_type			= node["type"]			.as<std::string>(_type);
	_points			= node["points"]		.as<int>(_points);
	_waves			= node["waves"]			.as<std::vector<MissionWave>>(_waves);
	_specialUfo		= node["specialUfo"]	.as<std::string>(_specialUfo);
	_specialZone	= node["specialZone"]	.as<size_t>(_specialZone);
	_weights		= node["missionWeights"].as<std::map<size_t, int>>(_weights);
	_retalCoef		= node["retalCoef"]		.as<int>(_retalCoef);
	_siteType		= node["siteType"]		.as<std::string>(_siteType);

	_objectiveType = static_cast<MissionObjective>(node["objectiveType"].as<int>(_objectiveType));


	if (const YAML::Node& weights = node["raceWeights"]) // allow only full replacement of mission racial distribution.
	{
		typedef std::map<size_t, WeightedOptions*> Associative;
		typedef std::vector<std::pair<size_t, WeightedOptions*>> Linear;

		Associative assoc;

		for (Linear::const_iterator // place in the Associative container so indices can be entered and sorted by month.
				i = _raceDistribution.begin();
				i != _raceDistribution.end();
				++i)
		{
			assoc.insert(*i);
		}

		for (YAML::const_iterator // go through node contents and merge with existing data.
				i = weights.begin();
				i != weights.end();
				++i)
		{
			const size_t monthsPassed (i->first.as<size_t>());

			Associative::const_iterator existing (assoc.find(monthsPassed));
			if (assoc.end() == existing) // new entry, load and add it.
			{
				std::auto_ptr<WeightedOptions> weight (new WeightedOptions);
				weight->load(i->second);

				assoc.insert(std::make_pair(
										monthsPassed,
										weight.release()));
			}
			else
				existing->second->load(i->second); // existing entry, update it.
		}

		_raceDistribution.clear(); // replace values in the actual member variable!
		_raceDistribution.reserve(assoc.size());

		for (Associative::const_iterator
				i = assoc.begin();
				i != assoc.end();
				++i)
		{
			if (i->second->hasNoWeight() == true) // don't keep empty entries.
				delete i->second;
			else
				_raceDistribution.push_back(*i); // place it
		}
	}
}

/**
 * Chooses one of the available races for this mission.
 * @note The racial distribution may vary based on the current game date.
 * @param monthsPassed - the number of months that have passed
 * @return, the string ID of the race
 */
std::string RuleAlienMission::generateRace(size_t monthsPassed) const
{
	std::vector<std::pair<size_t, WeightedOptions*>>::const_reverse_iterator rit (_raceDistribution.rbegin());
	while (monthsPassed < rit->first)
		++rit;

	return rit->second->getOptionResult();
}

/**
 * Chooses the most likely race for this mission.
 * @note The racial distribution may vary based on the current game date.
 * @param monthsPassed - the number of months that have passed in the game world
 * @return, the string-ID of the race
 *
std::string RuleAlienMission::getTopRace(size_t monthsPassed) const
{
	std::vector<std::pair<size_t, WeightedOptions*>>::const_reverse_iterator race = _raceDistribution.rbegin();
	while (monthsPassed < race->first)
		++race;

	return race->second->topChoice();
//	std::vector<std::pair<size_t, WeightedOptions*>>::const_iterator race = _raceDistribution.begin();
//	return race->second->topChoice();
} */

/**
 * Gets the alien-score for this mission.
 * @return, amount of points
 */
int RuleAlienMission::getPoints() const
{
	return _points;
}

/**
 * Gets the chance of this mission being generated based on the game-date.
 * @param monthsPassed - the number of months that have passed
 * @return, the weight
 */
int RuleAlienMission::getWeight(size_t monthsPassed) const
{
	if (_weights.empty() == false)
	{
		int weight (0);
		for (std::map<size_t, int>::const_iterator
				i = _weights.begin();
				i != _weights.end();
				++i)
		{
			if (i->first > monthsPassed)
				break;

			weight = i->second;
		}
		return weight;
	}
	return 1;
}

/**
 * Gets the modified-chance that shooting down a UFO on this type of mission
 * causes a retaliation-mission to generate.
 * @return, retaliation coefficient
 */
int RuleAlienMission::getRetaliation() const
{
	return _retalCoef;
}

}
