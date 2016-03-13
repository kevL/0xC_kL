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
	_producedItems[type] = 1;
}

/**
 * Loads the RuleManufacture from a YAML file.
 * @param node		- reference a YAML node
 * @param listOrder	- the list weight for this manufacture
 */
void RuleManufacture::load(
		const YAML::Node& node,
		int listOrder)
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

	_required		= node["required"]		.as<std::vector<std::string>>(_required);
	_space			= node["space"]			.as<int>(_space);
	_time			= node["time"]			.as<int>(_time);
	_cost			= node["cost"]			.as<int>(_cost);
	_requiredItems	= node["requiredItems"]	.as<std::map<std::string, int>>(_requiredItems);
	_producedItems	= node["producedItems"]	.as<std::map<std::string, int>>(_producedItems);
	_category		= node["category"]		.as<std::string>(_category);
	_listOrder		= node["listOrder"]		.as<int>(_listOrder);

	if (_listOrder == 0)
		_listOrder = listOrder;

	if (_category == "STR_CRAFT")
		_isCraft = true;
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
 * @return, reference to a vector of research IDs
 */
const std::vector<std::string>& RuleManufacture::getRequirements() const
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
