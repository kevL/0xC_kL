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

		node["ufoType"]		= rhs.ufoType;
		node["ufoTotal"]	= rhs.ufoTotal;
		node["trajectory"]	= rhs.trajectory;
		node["waveTimer"]	= rhs.waveTimer;
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

		rhs.ufoType		= node["ufoType"]		.as<std::string>();
		rhs.ufoTotal	= node["ufoTotal"]		.as<int>();
		rhs.trajectory	= node["trajectory"]	.as<std::string>();
		rhs.waveTimer	= node["waveTimer"]	.as<int>();
		rhs.isObjective	= node["isObjective"]	.as<bool>(false);

		return true;
	}
};

}


namespace OpenXcom
{

/**
 * Creates an AlienMission rule.
 * @param type - reference to the mission type
 */
RuleAlienMission::RuleAlienMission(const std::string& type)
	:
		_type(type),
		_score(0),
		_objectiveType(alm_SCORE),
		_objectiveZone(std::numeric_limits<size_t>::max()),
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
	_score			= node["score"]			.as<int>(_score);
	_waves			= node["waves"]			.as<std::vector<MissionWave>>(_waves);
	_objectiveUfo	= node["objectiveUfo"]	.as<std::string>(_objectiveUfo);
	_objectiveZone	= node["objectiveZone"]	.as<size_t>(_objectiveZone);
	_weights		= node["weightsMission"].as<std::map<size_t, int>>(_weights);
	_retalCoef		= node["retalCoef"]		.as<int>(_retalCoef);
	_terrorType		= node["terrorType"]	.as<std::string>(_terrorType);

	_objectiveType = static_cast<MissionObjective>(node["objectiveType"].as<int>(_objectiveType));


	if (const YAML::Node& weights = node["weightsRace"]) // allow only full replacement of mission racial distribution.
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
			const size_t elapsed (i->first.as<size_t>());

			Associative::const_iterator existing (assoc.find(elapsed));
			if (assoc.end() == existing) // new entry, load and add it.
			{
				WeightedOptions* weight (new WeightedOptions);
				weight->load(i->second);

				assoc.insert(std::make_pair(
										elapsed,
										weight));
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
 * Chooses one of the available races for this AlienMission rule.
 * @note The racial distribution may vary based on the current date.
 * @param elapsed - the number of months that have passed
 * @return, the string ID of the race
 */
std::string RuleAlienMission::generateRace(size_t elapsed) const
{
	std::vector<std::pair<size_t, WeightedOptions*>>::const_reverse_iterator rit (_raceDistribution.rbegin());
	while (elapsed < rit->first)
		++rit;

	return rit->second->getOptionResult();
}

/**
 * Gets the chance of this mission being generated based on the date.
 * @param elapsed - the months that have passed
 * @return, the weight
 */
int RuleAlienMission::getWeight(size_t elapsed) const
{
	if (_weights.empty() == false)
	{
		int weight (0);
		for (std::map<size_t, int>::const_iterator
				i = _weights.begin();
				i != _weights.end();
				++i)
		{
			if (i->first > elapsed)
				break;

			weight = i->second;
		}
		return weight;
	}
	return 1;
}

}
