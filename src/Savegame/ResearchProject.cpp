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

#include "ResearchProject.h"

#include "../Ruleset/RuleResearch.h"


namespace OpenXcom
{

/**
 * Constructs a ResearchProject at a Base.
 * @param resRule - pointer to RuleResearch
 */
ResearchProject::ResearchProject(const RuleResearch* const resRule)
	:
		_resRule(resRule),
		_scientists(0),
		_daysCost(0),
		_daysSpent(0),
		_offline(false)
{}

/**
 * dTor.
 */
ResearchProject::~ResearchProject()
{}

/**
 * Loads this ResearchProject from a YAML file.
 * @param node - reference a YAML node
 */
void ResearchProject::load(const YAML::Node& node)
{
	_scientists	= node["scientists"].as<int>(_scientists);
	_daysCost	= node["daysCost"]	.as<int>(_daysCost);
	_daysSpent	= node["daysSpent"]	.as<int>(_daysSpent);
	_offline	= node["offline"]	.as<bool>(_offline);
}

/**
 * Saves this ResearchProject to a YAML file.
 * @return, YAML node
 */
YAML::Node ResearchProject::save() const
{
	YAML::Node node;

	node["project"] = _resRule->getType();

	if (_scientists != 0)	node["scientists"]	= _scientists;
	if (_daysCost != 0)		node["daysCost"]	= _daysCost;
	if (_daysSpent != 0)	node["daysSpent"]	= _daysSpent;
	if (_offline != false)	node["offline"]		= _offline;

	return node;
}

/**
 * Gets this ResearchProject's rule.
 * @return, pointer to RuleResearch
 */
const RuleResearch* ResearchProject::getRules() const
{
	return _resRule;
}

/**
 * Called every day to compute days-spent on this ResearchProject.
 * @return, true if project finishes
 */
bool ResearchProject::stepResearch()
{
	if ((_daysSpent += _scientists) >= _daysCost)
		return true;

	return false;
}

/**
 * Changes the number of scientists to this ResearchProject.
 * @param qty - quantity of scientists assigned
 */
void ResearchProject::setAssignedScientists(int qty)
{
	_scientists = qty;
}

/**
 * Gets the number of scientists assigned to this ResearchProject.
 * @return, quantity of scientists assigned
 */
int ResearchProject::getAssignedScientists() const
{
	return _scientists;
}

/**
 * Changes the cost of this ResearchProject.
 * @param spent - new project cost in man-days
 */
void ResearchProject::setSpent(int spent)
{
	_daysSpent = spent;
}

/**
 * Gets the time already spent on this ResearchProject.
 * @return, the time already spent in man-days
 */
int ResearchProject::getSpent() const
{
	return _daysSpent;
}

/**
 * Changes the cost of this ResearchProject.
 * @param cost - new project cost in man-days
 */
void ResearchProject::setCost(int cost)
{
	_daysCost = cost;
}

/**
 * Gets the cost of this ResearchProject.
 * @return, the cost in man-days
 */
int ResearchProject::getCost() const
{
	return _daysCost;
}

/**
 * Sets this ResearchProject offline.
 * @note Used to remove the project from lists while preserving the cost & spent
 * values.
 * @param offline - true to set project offline (default true)
 */
void ResearchProject::setOffline(bool offline)
{
	_offline = offline;
}

/**
 * Gets whether this ResearchProject is offline or not.
 * @return, true if project is offline
 */
bool ResearchProject::getOffline() const
{
	return _offline;
}

/**
 * Returns a string describing lab-progress.
 * @return, progress-string
 */
std::string ResearchProject::getResearchProgress() const
{
	static const float
		LIMIT_UNKNOWN	= 0.09f, // 0.1
		LIMIT_POOR		= 0.23f, // 0.2
		LIMIT_AVERAGE	= 0.55f, // 0.5
		LIMIT_GOOD		= 0.86f; // 0.8

	if (_scientists == 0)
		return "STR_NA"; //"STR_NONE"

	if (static_cast<float>(_daysSpent) / static_cast<float>(_daysCost) < LIMIT_UNKNOWN)
		return "STR_UNKNOWN";

	const float progress (static_cast<float>(_scientists)
						/ static_cast<float>(_resRule->getCost()));

	if (progress < LIMIT_POOR)
		return "STR_POOR";

	if (progress < LIMIT_AVERAGE)
		return "STR_AVERAGE";

	if (progress < LIMIT_GOOD)
		return "STR_GOOD";

	return "STR_EXCELLENT";
}

}
