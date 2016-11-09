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
		_time(0),
		_cost(0),
		_isCraft(false),
		_listOrder(0)
{
	_manufacturedItems[_type] = 1; // produce 1 item of 'type' (at least)
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
	const bool inOut (_manufacturedItems.size() == 1
				   && _type == _manufacturedItems.begin()->first);

	_type = node["type"].as<std::string>(_type);

	if (inOut == true)
	{
		const int qtyOut (_manufacturedItems.begin()->second);
		_manufacturedItems.clear();
		_manufacturedItems[_type] = qtyOut;
	} // End_why. Perhaps to overwrite a previous entry with a subsequently loaded ID-string, perhaps.

	_required			= node["required"]			.as<std::vector<std::string>>(_required);
	_space				= node["space"]				.as<int>(_space);
	_time				= node["time"]				.as<int>(_time);
	_cost				= node["cost"]				.as<int>(_cost);
	_requiredFacilities	= node["requiredFacilities"].as<std::map<std::string, int>>(_requiredFacilities);
	_requiredItems		= node["requiredItems"]		.as<std::map<std::string, int>>(_requiredItems);
	_manufacturedItems	= node["manufacturedItems"]	.as<std::map<std::string, int>>(_manufacturedItems);
	_category			= node["category"]			.as<std::string>(_category);
	_listOrder			= node["listOrder"]			.as<int>(_listOrder);

	if (_listOrder == 0)
		_listOrder = listOrder;

//	if (_category == "STR_CRAFT")
//		_isCraft = true;

	_isCraft = false;
	int qty (0);

//	if (rules->getItemRule(_type) == nullptr // NOTE: '_type' was added to '_manufacturedItems' in cTor.
//		&& rules->getCraft(_type) != nullptr)
//	{
//		_isCraft = true;
//		++qty;
//	}

	for (std::map<std::string, int>::const_iterator
			i = _manufacturedItems.begin();
			i != _manufacturedItems.end();
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
 * Gets if this RuleManufacture produces a Craft.
 * @return, true if a Craft will be produced
 */
bool RuleManufacture::isCraft() const
{
	return _isCraft;
}

/**
 * Gets a list of required-research for the project.
 * @return, reference to a vector of research-types
 */
const std::vector<std::string>& RuleManufacture::getResearchRequirements() const
{
	return _required;
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
	return _time;
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
 * Gets the list of BaseFacilities required for the project.
 * @return, reference to the list of required base-facilities
 */
const std::map<std::string, int>& RuleManufacture::getRequiredFacilities() const
{
	return _requiredFacilities;
}

/**
 * Gets the list of items required for one iteration of the project.
 * @return, reference to the list of required item-types
 */
const std::map<std::string, int>& RuleManufacture::getRequiredItems() const
{
	return _requiredItems;
}

/**
 * Gets the list of items manufactured by one iteration of the project.
 * @note By default it contains only the item's ID-string with a value of 1 (rf.
 * constructor).
 * @return, reference to the list of items produced
 */
const std::map<std::string, int>& RuleManufacture::getManufacturedItems() const
{
	return _manufacturedItems;
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
