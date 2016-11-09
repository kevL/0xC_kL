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

#include "RuleManufacture.h"

#include "Ruleset.h"

#include "../Engine/Logger.h"


namespace OpenXcom
{

/**
 * Creates a new Manufacture rule.
 * @param type - reference a unique manufacture-type
 */
RuleManufacture::RuleManufacture(const std::string& type)
	:
		_type(type),
		_space(0),
		_hours(0),
		_cost(0),
		_isCraft(false),
		_listOrder(0)
{
	_partsProduced[_type] = 1; // produce 1 item of 'type' (at least)
}

/**
 * Loads the RuleManufacture from a YAML file.
 * @param node		- reference a YAML node
 * @param listOrder	- the list weight for this manufacture
 */
void RuleManufacture::load(
		const YAML::Node& node,
		int listOrder,
		const Ruleset* const rules)
{
	// why ->
	const bool isDefault (_partsProduced.size() == 1u
					   && _partsProduced.begin()->first == _type);

	_type = node["type"].as<std::string>(_type);

	if (isDefault == true)
	{
		const int qtyProduced (_partsProduced.begin()->second);
		_partsProduced.clear();
		_partsProduced[_type] = qtyProduced;
	} // End_why. Perhaps to overwrite a previous entry with a subsequently loaded string-ID, perhaps.

	_reqResearch	= node["reqResearch"]	.as<std::vector<std::string>>(_reqResearch);
	_space			= node["space"]			.as<int>(_space);
	_hours			= node["hours"]			.as<int>(_hours);
	_cost			= node["cost"]			.as<int>(_cost);
	_reqFacs		= node["reqFacs"]		.as<std::map<std::string, int>>(_reqFacs);
	_partsRequired	= node["partsRequired"]	.as<std::map<std::string, int>>(_partsRequired);
	_partsProduced	= node["partsProduced"]	.as<std::map<std::string, int>>(_partsProduced); // question: Does this add to the vector or overwrite it.
	_category		= node["category"]		.as<std::string>(_category);
	_listOrder		= node["listOrder"]		.as<int>(_listOrder);

	if (_listOrder == 0)
		_listOrder = listOrder;

//	if (_category == "STR_CRAFT")
//		_isCraft = true;

	_isCraft = false;
	int qty (0);

	for (std::map<std::string, int>::const_iterator
			i = _partsProduced.begin();
			i != _partsProduced.end();
			++i)
	{
		if (rules->getItemRule(i->first) == nullptr
			&& rules->getCraft(i->first) != nullptr)
		{
			_isCraft = true;
			qty += i->second;
		}
	}

	if (qty > 1)
	{
		Log(LOG_WARNING) << "RuleManufacture::load() The rule for " << _type << " produces " << qty << " Craft."
						 << " The manufacturing subsystem allows production of only one Craft.";
		// TODO: Delete 'this'.
	}
}

/**
 * Gets the unique type of this RuleManufacture.
 * @return, reference to the type
 */
const std::string& RuleManufacture::getType() const
{
	return _type;
}

/**
 * Gets the category for the manufacture-list.
 * @return, reference to the category
 */
const std::string& RuleManufacture::getCategory() const
{
	return _category;
}

/**
 * Checks if this RuleManufacture produces a Craft.
 * @return, true if a Craft will be produced
 */
bool RuleManufacture::isCraft() const
{
	return _isCraft;
}

/**
 * Gets the workspace-requirements to start the project.
 * @return, the required workspace
 */
int RuleManufacture::getSpaceRequired() const
{
	return _space;
}

/**
 * Gets the time needed to complete one iteration of the project.
 * @return, the time needed in man-hours
 */
int RuleManufacture::getManufactureTime() const
{
	return _hours;
}

/**
 * Gets the cost for one iteration of the project.
 * @return, the cost
 */
int RuleManufacture::getManufactureCost() const
{
	return _cost;
}

/**
 * Gets the list of required-research for the project.
 * @return, reference to a vector of research-types
 */
const std::vector<std::string>& RuleManufacture::getRequiredResearch() const
{
	return _reqResearch;
}

/**
 * Gets the list of BaseFacilities required for the project.
 * @return, reference to a list of required base-facilities
 */
const std::map<std::string, int>& RuleManufacture::getRequiredFacilities() const
{
	return _reqFacs;
}

/**
 * Gets the list of parts required for one iteration of the project.
 * @return, reference to a list of required parts
 */
const std::map<std::string, int>& RuleManufacture::getPartsRequired() const
{
	return _partsRequired;
}

/**
 * Gets the list of parts manufactured by one iteration of the project.
 * @note By default it contains only the part's string-ID with a value of 1 (rf.
 * constructor).
 * @return, reference to a list of parts produced
 */
const std::map<std::string, int>& RuleManufacture::getPartsProduced() const
{
	return _partsProduced;
}

/**
 * Gets the list-weight for this RuleManufacture.
 * @return, the list-weight
 */
int RuleManufacture::getListOrder() const
{
	return _listOrder;
}

}
