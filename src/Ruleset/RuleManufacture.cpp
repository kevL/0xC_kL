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
	_producedItems[_type] = 1; // produce 1 item of 'type' (at least)
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
	const bool inOut (_producedItems.size() == 1
				   && _type == _producedItems.begin()->first);

	_type = node["type"].as<std::string>(_type);

	if (inOut == true)
	{
		const int qtyOut (_producedItems.begin()->second);
		_producedItems.clear();
		_producedItems[_type] = qtyOut;
	} // End_why. Perhaps to overwrite a previous entry with a subsequently loaded ID-string, perhaps.

	_required			= node["required"]			.as<std::vector<std::string>>(_required);
	_space				= node["space"]				.as<int>(_space);
	_time				= node["time"]				.as<int>(_time);
	_cost				= node["cost"]				.as<int>(_cost);
	_requiredFacilities	= node["requiredFacilities"].as<std::map<std::string, int>>(_requiredFacilities);
	_requiredItems		= node["requiredItems"]		.as<std::map<std::string, int>>(_requiredItems);
	_producedItems		= node["producedItems"]		.as<std::map<std::string, int>>(_producedItems);
	_category			= node["category"]			.as<std::string>(_category);
	_listOrder			= node["listOrder"]			.as<int>(_listOrder);

	if (_listOrder == 0)
		_listOrder = listOrder;

//	if (_category == "STR_CRAFT")
//		_isCraft = true;

	_isCraft = false;
	int qty (0);

//	if (rules->getItemRule(_type) == nullptr // NOTE: '_type' was added to '_producedItems' in cTor.
//		&& rules->getCraft(_type) != nullptr)
//	{
//		_isCraft = true;
//		++qty;
//	}

	for (std::map<std::string, int>::const_iterator
			i = _producedItems.begin();
			i != _producedItems.end();
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
		Log(LOG_WARNING) << "RuleManufacture::load() The rule for " << _type << " produces two+ Craft."
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
 * Gets the category shown in the production-list.
 * @return, reference to the category
 */
const std::string& RuleManufacture::getCategory() const
{
	return _category;
}

/**
 * Gets if this RuleManufacture produces a Craft.
 * @return, true if a Craft is to be produced
 */
bool RuleManufacture::isCraft() const
{
	return _isCraft;
}

/**
 * Gets the list of research-requirements to produce this object.
 * @return, reference to a vector of research-types
 */
const std::vector<std::string>& RuleManufacture::getResearchRequirements() const
{
	return _required;
}

/**
 * Gets the workspace-requirements to start production.
 * @return, the required workspace
 */
int RuleManufacture::getSpaceRequired() const
{
	return _space;
}

/**
 * Gets the time needed to produce one object.
 * @return, the time needed in man-hours
 */
int RuleManufacture::getManufactureTime() const
{
	return _time;
}

/**
 * Gets the cost of producing one object.
 * @return, the cost
 */
int RuleManufacture::getManufactureCost() const
{
	return _cost;
}

/**
 * Gets the list of BaseFacilities required to produce the products.
 * @return, reference to the list of required base-facilities
 */
const std::map<std::string, int>& RuleManufacture::getRequiredFacilities() const
{
	return _requiredFacilities;
}

/**
 * Gets the list of items required to produce one object.
 * @return, reference to the list of required item-types
 */
const std::map<std::string, int>& RuleManufacture::getRequiredItems() const
{
	return _requiredItems;
}

/**
 * Gets the list of items produced by completing one object.
 * @note By default it contains only the item's ID-string with a value of 1.
 * @return, reference to the list of items produced
 */
const std::map<std::string, int>& RuleManufacture::getProducedItems() const
{
	return _producedItems;
}

/**
 * Gets the list-weight for this RuleManufacture.
 * @return, the list weight
 */
int RuleManufacture::getListOrder() const
{
	return _listOrder;
}

}
