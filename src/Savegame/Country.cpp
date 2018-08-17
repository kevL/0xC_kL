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

#include "Country.h"

#include "../Engine/RNG.h"

#include "../Ruleset/RuleCountry.h"


namespace OpenXcom
{

/**
 * Instantiates the Country from a specified rule.
 * @param countryRule	- pointer to RuleCountry
 * @param genFunds		- true to generate funding at start (default false)
 */
Country::Country(
		const RuleCountry* const countryRule,
		bool genFunds)
	:
		_countryRule(countryRule),
		_funds(0),
		_pact(PACT_NONE),
		_satisfaction(SAT_NEUTRAL),
		_actAhrs(-1),
		_actXhrs(-1)
{
	int funds;
	if (genFunds == true)
		funds = _countryRule->generateFunding(); // 50% .. 200% or 0.
	else
		funds = 0;

	_funds.push_back(funds);

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
	_funds   = node["funds"]    .as<std::vector<int>>(_funds);
	_actA    = node["actA"]     .as<std::vector<int>>(_actA);
	_actX    = node["actX"]     .as<std::vector<int>>(_actX);
	_actAhrs = node["actAhours"].as<int>(_actAhrs);
	_actXhrs = node["actXhours"].as<int>(_actXhrs);

	_pact = static_cast<PactStatus>(node["pactStatus"].as<int>(static_cast<int>(_pact)));
}

/**
 * Saves this Country to a YAML file.
 * @return, YAML node
 */
YAML::Node Country::save() const
{
	YAML::Node node;

	node["type"]      = _countryRule->getType();
	node["funds"]     = _funds;
	node["actA"]      = _actA;
	node["actX"]      = _actX;
	node["actAhours"] = _actAhrs;
	node["actXhours"] = _actXhrs;

	if (_pact != PACT_NONE) node["pactStatus"] = static_cast<int>(_pact);

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
 * @return, reference to a vector of monthly funds ($thousands)
 */
std::vector<int>& Country::getCountryFunds()
{
	return _funds;
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
	_actAhrs = 0;
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
	_actXhrs = 0;
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
 * @param actXtotal	- the council's xCom score
 * @param actAtotal	- the council's aLien score
 * @param diff		- game difficulty
 */
void Country::newMonth(
		const int actXtotal,
		const int actAtotal,
		const int diff)
{
	Log(LOG_INFO) << "Country::newMonth() " << _countryRule->getType();
	int funds;
	if (_pact == PACT_NONE)
	{
		const int funds0 (_funds.back());
		Log(LOG_INFO) << ". funds0= " << funds0;
		if (funds0 != 0)
		{
			const int
				player (actXtotal / 10 + _actX.back()), // ie. 10% of global XCOM activity  + 100% of this country's XCOM activity
				aLiens (actAtotal / 20 + _actA.back()); // ie.  5% of global aLien activity + 100% of this country's aLien activity

			Log(LOG_INFO) << ". . player= " << player;
			Log(LOG_INFO) << ". . aLiens= " << aLiens;

			funds = static_cast<int>(static_cast<float>(funds0) * RNG::generate(0.05f,0.2f)); // 5% .. 20% increase OR decrease
			Log(LOG_INFO) << ". . funds= " << funds;

			if (player > aLiens + ((diff + 1) * 20))
			{
				funds = std::min(funds,
								_countryRule->getFundingCap() - funds0);
				Log(LOG_INFO) << ". . . case1 funds= " << funds;
				switch (funds)
				{
					case 0: // Country's funding is already capped.
						_satisfaction = SAT_NEUTRAL;
						break;
					default:
						_satisfaction = SAT_SATISFIED;
				}
			}
			else if (player - (diff * 20) > aLiens)
			{
				if (RNG::generate(0, player) > aLiens)
				{
					funds = std::min(funds,
									_countryRule->getFundingCap() - funds0);
					Log(LOG_INFO) << ". . . case2a funds= " << funds;
					switch (funds)
					{
						case 0: // Country's funding is already capped.
							_satisfaction = SAT_NEUTRAL;
							break;
						default:
							_satisfaction = SAT_SATISFIED;
					}
				}
				else if (RNG::generate(0, aLiens) > player)
				{
					Log(LOG_INFO) << ". . . case2b funds= " << funds;
					switch (funds)
					{
						case 0: // Country's funding is already zero'd.
							_satisfaction = SAT_NEUTRAL;
							break;
						default:
							_satisfaction = SAT_MIFFED;
					}
				}
				else
				{
					funds = 0;
					Log(LOG_INFO) << ". . . case2c funds= " << funds;
					_satisfaction = SAT_NEUTRAL;
				}
			}
			else
			{
				Log(LOG_INFO) << ". . . case3 funds= " << funds;
				switch (funds)
				{
					case 0: // Country's funding is already zero'd.
						_satisfaction = SAT_NEUTRAL;
						break;
					default:
						_satisfaction = SAT_MIFFED;
				}
			}

			switch (_satisfaction) // NOTE: changes 'funds' (delta) to 'funds' (granted)
			{
				case SAT_SATISFIED:
					funds += funds0;
					break;

				case SAT_NEUTRAL:
					funds = funds0;
					break;

				case SAT_MIFFED:
					funds = funds0 - funds;
			}
		}
		else // grant a chance for zero-funding unpacted Countries to join the Project.
		{
			switch (funds = _countryRule->generateFunding())
			{
				case 0:
					_satisfaction = SAT_NEUTRAL;
					break;
				default:
					_satisfaction = SAT_JOINED;
			}
		}
	}
	else // pacted or about to pact.
	{
		funds = 0;
		_satisfaction = SAT_NEUTRAL;
	}

	Log(LOG_INFO) << ". funds= " << funds;
	Log(LOG_INFO) << ". _satisfaction= " << _satisfaction;
	Log(LOG_INFO) << " ";

	_funds.push_back(funds);

	_actA.push_back(0);
	_actX.push_back(0);

	if (_funds.size() > 12u)
	{
		_funds.erase(_funds.begin());

		_actA.erase(_actA.begin());
		_actX.erase(_actX.begin());
	}
}

/**
 * Sets this Country's pact-status.
 * @param pact - the status
 */
void Country::setPactStatus(const PactStatus pact)
{
	_pact = pact;
}

/**
 * Gets this Country's pact-status.
 * @return, the status
 */
PactStatus Country::getPactStatus() const
{
	return _pact;
}

/**
 * Advances recent activity in this Country.
 */
void Country::stepActivity()
{
	if (_actAhrs != -1 && ++_actAhrs == 24) // aLien bases tally activity every 24 hrs so don't go lower than 24 hrs.
		_actAhrs = -1;

	if (_actXhrs != -1 && ++_actXhrs == 24)
		_actXhrs = -1;
}

/**
 * Checks if there has been activity in this Country recently.
 * @param aLien - true to check aLien activity, false for xCom activity
 * @return, true if recent activity
 */
bool Country::checkActivity(bool aLien)
{
	if (aLien == true)
		return (_actAhrs != -1);

	return (_actXhrs != -1);
}

/**
 * Resets activity.
 */
void Country::clearActivity()
{
	_actAhrs =
	_actXhrs = -1;
}

}
