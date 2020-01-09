/*
 * Copyright 2010-2020 OpenXcom Developers.
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

#include "RuleAward.h"


namespace OpenXcom
{

/**
 * Creates the RuleAward.
 */
RuleAward::RuleAward()
	:
		_sprite(-1)
{}

/**
 * Cleans up this RuleAward.
 */
RuleAward::~RuleAward()
{}

/**
 * Loads this RuleAward from YAML.
 * @param node - reference a YAML node
 */
void RuleAward::load(const YAML::Node& node)
{
	_description	= node["description"]	.as<std::string>(_description);
	_descGeneric	= node["descGeneric"]	.as<std::string>(_descGeneric);
	_sprite			= node["sprite"]		.as<int>(_sprite);
	_criteria		= node["criteria"]		.as<std::map<std::string, std::vector<int>>>(_criteria);
	_killCriteria	= node["killCriteria"]	.as<std::vector<std::vector<std::pair<int, std::vector<std::string>>>>>(_killCriteria);
}

/**
 * Gets this RuleAward's description.
 * @return, award description
 */
const std::string& RuleAward::getDescription() const
{
	return _description;
}

/**
 * Gets this RuleAward's non-specific description.
 * @return, generic description
 */
const std::string& RuleAward::getDescriptionGeneric() const
{
	return _descGeneric;
}

/**
 * Gets this RuleAward's sprite.
 * @return, sprite number
 */
int RuleAward::getSprite() const
{
	return _sprite;
}

/**
 * Gets this RuleAward's criteria.
 * @return, pointer to a map of (strings + vectors of ints) that denote criteria
 */
const std::map<std::string, std::vector<int>>* RuleAward::getCriteria() const
{
	return &_criteria;
}

/**
 * Gets this RuleAward's kill-criteria.
 * @return, pointer to a vector of vectors of pairs of (ints + vectors of strings) that denote kill-criteria
 * @note Think of how hard that would be to do without c++ I feel like singing ...
 */
const std::vector<std::vector<std::pair<int, std::vector<std::string>>>>* RuleAward::getKillCriteria() const
{
	return &_killCriteria;
}

}
