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

#include "Country.h"

#include "../Engine/RNG.h"

#include "../Ruleset/RuleCountry.h"


namespace OpenXcom
{

/**
 * Initializes the Country from a specified rule.
 * @param countryRule	- pointer to RuleCountry
 * @param genFunds		- true to generate new funding (default false)
 */
Country::Country(
		const RuleCountry* const countryRule,
		bool genFunds)
	:
		_countryRule(countryRule),
		_funding(0),
		_pactStatus(PACT_NONE),
		_satisfaction(SAT_NEUTRAL),
		_recentActA(-1),
		_recentActX(-1)
{
	if (genFunds == true)
		_funding.push_back(_countryRule->generateFunding()); // 50% - 200% or 0.

	_actA.push_back(0);
	_actX.push_back(0);
}

/**
 * dTor.
 */
Country::~Country()
{}

/**
 * Loads this Country from a YAML file.
 * @param node - reference a YAML node
 */
void Country::load(const YAML::Node& node)
{
	_funding	= node["funding"]	.as<std::vector<int>>(_funding);
	_actA		= node["actA"]		.as<std::vector<int>>(_actA);
	_actX		= node["actX"]		.as<std::vector<int>>(_actX);
	_recentActA	= node["recentActA"].as<int>(_recentActA);
	_recentActX	= node["recentActX"].as<int>(_recentActX);

	_pactStatus = static_cast<PactStatus>(node["pactStatus"].as<int>(static_cast<int>(_pactStatus)));
}

/**
 * Saves this Country to a YAML file.
 * @return, YAML node
 */
YAML::Node Country::save() const
{
	YAML::Node node;

	node["type"]		= _countryRule->getType();
	node["funding"]		= _funding;
	node["actA"]		= _actA;
	node["actX"]		= _actX;
	node["recentActA"]	= _recentActA;
	node["recentActX"]	= _recentActX;

	if (_pactStatus != PACT_NONE) node["pactStatus"] = static_cast<int>(_pactStatus);

	return node;
}

/**
 * Gets the rule for this Country's type.
 * @return, pointer to RuleCountry
 */
const RuleCountry* Country::getRules() const
{
	return _countryRule;
}

/**
 * Gets this Country's type.
 * @return, country-type
 */
const std::string& Country::getType() const
{
	return _countryRule->getType();
}

/**
 * Gets this Country's current monthly funding.
 * @return, reference to a vector of monthly funds
 */
std::vector<int>& Country::getFunding()
{
	return _funding;
}

/**
 * Keith Richards would be so proud.
 * @return, satisfaction level (Country.h)
 *			0 = unhappy
 *			1 = neutral OR has pact w/ aLiens
 *			2 = happy
 */
SatisfactionType Country::getSatisfaction() const
{
	return _satisfaction;
}

/**
 * Adds to this Country's alien-activity-level.
 * @param activity - how many points to add
 */
void Country::addActivityAlien(int activity)
{
	_actA.back() += activity;
}

/**
 * Gets this Country's alien-activity-level.
 * @return, reference to a vector of activity levels
 */
std::vector<int>& Country::getActivityAlien()
{
	return _actA;
}

/**
 * Adds to this Country's xcom-activity-level.
 * @param activity - how many points to add
 */
void Country::addActivityXCom(int activity)
{
	_actX.back() += activity;
}

/**
 * Gets this Country's xcom-activity-level.
 * @return, reference to a vector of activity levels
 */
std::vector<int>& Country::getActivityXCom()
{
	return _actX;
}

/**
 * Resets the funding- and activity-counters, calculates monthly funding, and
 * sets the cash-delta-value for the month.
 * @param totalX	- the council's xCom score
 * @param totalA	- the council's aLien score
 * @param diff		- game difficulty
 */
void Country::newMonth(
		const int totalX,
		const int totalA,
		const int diff)
{
	//Log(LOG_INFO) << "Country::newMonth()";
	int funds;
	if (_pactStatus == PACT_NONE)
	{
		const int fundsBefore (_funding.back());
		if (fundsBefore != 0)
		{
			const int
				scorePlayer (totalX / 10 + _actX.back()),
				scoreAlien  (totalA / 20 + _actA.back());

			funds = static_cast<int>(static_cast<float>(fundsBefore) * RNG::generate(0.05f,0.2f)); // 5% - 20% increase OR decrease

			if (scorePlayer > scoreAlien + ((diff + 1) * 20))
			{
				funds = std::min(funds,
								_countryRule->getFundingCap() - fundsBefore);
				switch (funds)
				{
					case 0: // Country's funding is already capped.
						_satisfaction = SAT_NEUTRAL;
						break;
					default:
						_satisfaction = SAT_HAPPY;
				}
			}
			else if (scorePlayer - (diff * 20) > scoreAlien)
			{
				if (RNG::generate(0, scorePlayer) > scoreAlien)
				{
					funds = std::min(funds,
									_countryRule->getFundingCap() - fundsBefore);
					switch (funds)
					{
						case 0: // Country's funding is already capped.
							_satisfaction = SAT_NEUTRAL;
							break;
						default:
							_satisfaction = SAT_HAPPY;
					}
				}
				else if (RNG::generate(0, scoreAlien) > scorePlayer)
				{
					switch (funds)
					{
						case 0: // Country's funding is already zero'd.
							_satisfaction = SAT_NEUTRAL;
							break;
						default:
							_satisfaction = SAT_SAD;
					}
				}
				else
				{
					funds = 0;
					_satisfaction = SAT_NEUTRAL;
				}
			}
			else
			{
				switch (funds)
				{
					case 0: // Country's funding is already zero'd.
						_satisfaction = SAT_NEUTRAL;
						break;
					default:
						_satisfaction = SAT_SAD;
				}
			}

			switch (_satisfaction)
			{
				case SAT_HAPPY:
					funds += fundsBefore;
					break;

				case SAT_SAD:
					funds = fundsBefore - funds;
			}
		}
		else
		{
			switch (funds = _countryRule->generateFunding()) // grant a chance for zero-funding unpacted Countries to join the Project.
			{
				case 0:
					_satisfaction = SAT_NEUTRAL;
					break;
				default:
					_satisfaction = SAT_PROJECT;
			}
		}
	}
	else // pacted or about to pact.
	{
		funds = 0;
		_satisfaction = SAT_NEUTRAL;
	}

	_funding.push_back(funds);

	_actA.push_back(0);
	_actX.push_back(0);

	if (_actA.size() > 12u)
	{
		_actA.erase(_actA.begin());
		_actX.erase(_actX.begin());
		_funding.erase(_funding.begin());
	}
}

/**
 * Sets this Country's pact-status.
 * @param pact - the status
 */
void Country::setPactStatus(const PactStatus pact)
{
	_pactStatus = pact;
}

/**
 * Gets this Country's pact-status.
 * @return, the status
 */
PactStatus Country::getPactStatus() const
{
	return _pactStatus;
}

/**
 * Handles recent aLien activity in this Country for GraphsState blink.
 * @param activity	- true to reset the startcounter (default true)
 * @param graphs	- not sure lol (default false)
 * @return, true if there is activity
 */
bool Country::recentActivityAlien(
		bool activity,
		bool graphs)
{
	if (activity == true)
		_recentActA = 0;
	else if (_recentActA != -1)
	{
		if (graphs == true)
			return true;


		++_recentActA;

		if (_recentActA == 24) // aLien bases show activity every 24 hrs.
			_recentActA = -1;
	}

	if (_recentActA == -1)
		return false;

	return true;
}

/**
 * Handles recent XCOM activity in this Country for GraphsState blink.
 * @param activity	- true to reset the startcounter (default true)
 * @param graphs	- not sure lol (default false)
 * @return, true if there is activity
 */
bool Country::recentActivityXCom(
		bool activity,
		bool graphs)
{
	if (activity == true)
		_recentActX = 0;
	else if (_recentActX != -1)
	{
		if (graphs == true)
			return true;


		++_recentActX;

		if (_recentActX == 24) // aLien bases show activity every 24 hrs.
			_recentActX = -1;
	}

	if (_recentActX == -1)
		return false;

	return true;
}

/**
 * Resets activity.
 */
void Country::resetActivity()
{
	_recentActA =
	_recentActX = -1;
}

}
