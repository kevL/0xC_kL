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

#include "ResearchGeneral.h"

#include "../Ruleset/RuleResearch.h"


namespace OpenXcom
{

/**
 * Constructs a ResearchGeneral project.
 * @param resRule		- pointer to RuleResearch
 * @param isQuickBattle	- true to consider research discovered (default false)
 */
ResearchGeneral::ResearchGeneral(
		const RuleResearch* const resRule,
		bool isQuickBattle)
	:
		_resRule(resRule),
		_type(resRule->getType()),
		_beenSeen(resRule->getMarkSeen())
{
	if (isQuickBattle == true)
		_status = RG_DISCOVERED;
	else if (std::find(
					_resRule->getRequisiteResearch().begin(),
					_resRule->getRequisiteResearch().end(),
					"STR_UNLOCKED") != _resRule->getRequisiteResearch().end())
	{
		_status = RG_UNLOCKED;
	}
	else
		_status = RG_LOCKED;
}

/**
 * dTor.
 */
ResearchGeneral::~ResearchGeneral()
{}

/**
 * Gets this ResearchGeneral's type.
 * @return, type-string
 */
const std::string& ResearchGeneral::getType() const
{
	return _type;
}

/**
 * Gets this ResearchGeneral's research-rule.
 * @return, pointer to RuleResearch
 */
const RuleResearch* ResearchGeneral::getRules() const
{
	return _resRule;
}

/**
 * Gets the status.
 * @return, the research status
 */
ResearchStatus ResearchGeneral::getStatus() const
{
	return _status;
}

/**
 * Sets the status.
 * @param status - the research status
 */
void ResearchGeneral::setStatus(ResearchStatus status)
{
	_status = status;
}

/**
 * Sets the associated Ufopaedia entry as seen.
 * @param seen - true if seen (default false)
 */
void ResearchGeneral::setBeenSeen(const bool seen)
{
	_beenSeen = seen;
}

/**
 * Checks if the associated Ufopaedia entry has been seen.
 * @return, true if seen
 */
bool ResearchGeneral::hasBeenSeen() const
{
	return _beenSeen;
}

/**
 * Loads this ResearchGeneral from a YAML file.
 * @param node - reference a YAML node
 */
void ResearchGeneral::load(const YAML::Node& node)
{
	_type		= node["type"]		.as<std::string>();
	_beenSeen	= node["beenSeen"]	.as<bool>(_beenSeen);

	_status = static_cast<ResearchStatus>(node["status"].as<int>(_status));
}

/**
 * Saves this ResearchGeneral to a YAML file.
 * @return, YAML node
 */
YAML::Node ResearchGeneral::save() const
{
	YAML::Node node;

	node["type"]		= _type;
	node["beenSeen"]	= _beenSeen;

	node["status"] = static_cast<int>(_status);

	return node;
}

}
