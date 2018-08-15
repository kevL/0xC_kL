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

#include "Region.h"

//#include <cmath>

#include "../Ruleset/RuleRegion.h"


namespace OpenXcom
{

/**
 * Instantiates the Region with a specified rule.
 * @param regionRule - pointer to RuleRegion
 */
Region::Region(const RuleRegion* const regionRule)
	:
		_regionRule(regionRule),
		_actAhrs(-1),
		_actXhrs(-1)
{
	_actA.push_back(0);
	_actX.push_back(0);
}

/**
 * dTor.
 */
Region::~Region()
{}

/**
 * Loads this Region from a YAML file.
 * @param node - reference a YAML node
 */
void Region::load(const YAML::Node& node)
{
	_actA		= node["actA"]   .as<std::vector<int>>(_actA);
	_actX		= node["actX"]   .as<std::vector<int>>(_actX);
	_actAhrs	= node["actAhrs"].as<int>(_actAhrs);
	_actXhrs	= node["actXhrs"].as<int>(_actXhrs);
}

/**
 * Saves this Region to a YAML file.
 * @return, YAML node
 */
YAML::Node Region::save() const
{
	YAML::Node node;

	node["type"]    = _regionRule->getType();
	node["actA"]    = _actA;
	node["actX"]    = _actX;
	node["actAhrs"] = _actAhrs;
	node["actXhrs"] = _actXhrs;

	return node;
}

/**
 * Gets the ruleset for this Region's type.
 * @return, pointer to RuleRegion
 */
const RuleRegion* Region::getRules() const
{
	return _regionRule;
}

/**
 * Gets this Region's type.
 * @return, region type
 */
std::string Region::getType() const
{
	return _regionRule->getType();
}

/**
 * Adds to this Region's aLien activity level.
 * @param activity - amount to add
 */
void Region::addActivityAlien(int activity)
{
	_actAhrs = 0;
	_actA.back() += activity;
}

/**
 * Gets this Region's aLien activity level.
 * @return, activity level
 */
std::vector<int>& Region::getActivityAlien()
{
	return _actA;
}

/**
 * Adds to this Region's xCom activity level.
 * @param activity - amount to add
 */
void Region::addActivityXCom(int activity)
{
	_actXhrs = 0;
	_actX.back() += activity;
}

/**
 * Gets this Region's xCom activity level.
 * @return, activity level
 */
std::vector<int>& Region::getActivityXCom()
{
	return _actX;
}

/**
 * Stores last month's counters, starts new counters.
 */
void Region::newMonth()
{
	_actA.push_back(0);
	_actX.push_back(0);

	if (_actA.size() > 12)
	{
		_actA.erase(_actA.begin());
		_actX.erase(_actX.begin());
	}
}

/**
 * Advances recent activity in this Region.
 */
void Region::stepActivity()
{
	if (_actAhrs != -1 && ++_actAhrs == 24) // aLien bases tally activity every 24 hrs so don't go lower than 24 hrs.
		_actAhrs = -1;

	if (_actXhrs != -1 && ++_actXhrs == 24)
		_actXhrs = -1;
}

/**
 * Checks if there has been activity in this Region recently.
 * @param aLien - true to check aLien activity, false for xCom activity
 * @return, true if recent activity
 */
bool Region::checkActivity(bool aLien)
{
	if (aLien == true)
		return (_actAhrs != -1);

	return (_actXhrs != -1);
}

/**
 * Resets activity in this Region.
 */
void Region::clearActivity()
{
	_actAhrs =
	_actXhrs = -1;
}

}
