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

#include "Country.h"

#include "../Engine/RNG.h"

#include "../Ruleset/RuleCountry.h"


namespace OpenXcom
{

/**
 * Initializes the Country from a specified type.
 * @param countryRule	- pointer to RuleCountry
 * @param genFunds		- true to generate new funding (default false)
 */
Country::Country(
		const RuleCountry* const countryRule,
		bool genFunds)
	:
		_countryRule(countryRule),
		_pact(false),
		_newPact(false),
		_funding(0),
		_satisfaction(SAT_NEUTRAL),
		_recentActA(-1),
		_recentActX(-1)
{
	if (genFunds == true)
		_funding.push_back(_countryRule->generateFunding());

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
	_pact		= node["pact"]		.as<bool>(_pact);
	_newPact	= node["newPact"]	.as<bool>(_newPact);
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

	if (_pact == true)
		node["pact"]	= _pact;
	else if (_newPact == true)
		node["newPact"]	= _newPact;

	return node;
}

/**
 * Gets the ruleset for this Country's type.
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
 * Returns this Country's current monthly funding.
 * @return, reference to a vector of monthly funds
 */
std::vector<int>& Country::getFunding()
{
	return _funding;
}

/**
 * Changes this Country's current monthly funding.
 * @param funding - monthly funds
 */
void Country::setFunding(int funding)
{
	_funding.back() = funding;
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
	if (_pact == false)
		return _satisfaction;

	return SAT_NEUTRAL;
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
 * Resets all the counters, calculates monthly funding, and sets the delta value
 * for the month.
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
	_satisfaction = SAT_NEUTRAL;

	const int
		xComPts ((totalX / 10) + _actX.back()),
		aLienPts ((totalA / 20) + _actA.back()),
		funding (getFunding().back()),
		oldFunding (_funding.back() / 1000);
	int newFunding ((oldFunding * RNG::generate(5,20) / 100) * 1000);

	if (xComPts > aLienPts + ((diff + 1) * 20)) // country auto. increases funding
	{
		//Log(LOG_INFO) << ". auto funding increase";
		const int cap (getRules()->getFundingCap() * 1000);
		if (funding + newFunding > cap)
			newFunding = cap - funding;

		if (newFunding != 0)
			_satisfaction = SAT_HAPPY;
	}
	else if (xComPts - (diff * 20) > aLienPts) // 50-50 increase/decrease funding
	{
		//Log(LOG_INFO) << ". possible funding increase/decrease";
		if (RNG::generate(0, xComPts) > aLienPts)
		{
			//Log(LOG_INFO) << ". . funding increase";
			const int cap (getRules()->getFundingCap() * 1000);
			if (funding + newFunding > cap)
				newFunding = cap - funding;

			if (newFunding != 0)
				_satisfaction = SAT_HAPPY;
		}
		else if (RNG::generate(0, aLienPts) > xComPts
			&& newFunding != 0)
		{
			//Log(LOG_INFO) << ". . funding decrease";
			newFunding = -newFunding;
			_satisfaction = SAT_SAD;
		}
	}
	else if (newFunding != 0) // auto. funding decrease
	{
		//Log(LOG_INFO) << ". auto funding decrease";
		newFunding = -newFunding;
		_satisfaction = SAT_SAD;
	}

	if (_newPact == true && _pact == false) // about to be in cahoots
	{
		_newPact = false;
		_pact = true;
	}

	if (_pact == true) // set the new funding and reset the activity meters
		_funding.push_back(0);
	else if (_satisfaction != SAT_NEUTRAL)
		_funding.push_back(funding + newFunding);
	else
		_funding.push_back(funding);

	_actA.push_back(0);
	_actX.push_back(0);

	if (_actA.size() > 12)
	{
		_actA.erase(_actA.begin());
		_actX.erase(_actX.begin());
		_funding.erase(_funding.begin());
	}
}

/**
 * Signs a pact with aLiens at month's end.
 */
void Country::setRecentPact()
{
	 _newPact = true;
}

/**
 * Gets if this country has signed a recent pact with aLiens.
 * @return, true if so
 */
bool Country::getRecentPact() const
{
	return _newPact;
}

/**
 * Gets if this country already has a pact with aLiens.
 * @note There is no setter for this one since it gets set automatically at the
 * end of the month if @a _newPact is true.
 * @return, true if country has a pact with aLiens
 */
bool Country::getPact() const
{
	return _pact;
}

/**
 * Signs a new pact.
 *
void Country::setPact()
{
	 _pact = true;
} */

/**
 * Handles recent aLien activity in this country for GraphsState blink.
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
 * Handles recent XCOM activity in this country for GraphsState blink.
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
