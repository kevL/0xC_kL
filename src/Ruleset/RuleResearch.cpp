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

#include "RuleResearch.h"


namespace OpenXcom
{

/**
 * Creates a new RuleResearch.
 * @param type - reference to the type-string of this RuleResearch
 */
RuleResearch::RuleResearch(const std::string& type)
	:
		_type(type),
		_cost(0),
		_points(0),
		_needsItem(false),
		_destroyItem(false),
		_markSeen(false),
		_listOrder(0)
{}

/**
 * dTor.
 */
RuleResearch::~RuleResearch()
{}

/**
 * Loads the RuleResearch from a YAML file.
 * @param node		- reference a YAML node
 * @param listOrder	- list-weight
 */
void RuleResearch::load(
		const YAML::Node& node,
		int listOrder)
{
	_type			= node["type"]			.as<std::string>(_type);
	_uPed			= node["uPed"]			.as<std::string>(_uPed);
	_cost			= node["cost"]			.as<int>(_cost);
	_points			= node["points"]		.as<int>(_points);
	_required		= node["required"]		.as<std::vector<std::string>>(_required);
	_requisite		= node["requisite"]		.as<std::vector<std::string>>(_requisite);
	_requested		= node["requested"]		.as<std::vector<std::string>>(_requested);
	_getOneFree		= node["getOneFree"]	.as<std::vector<std::string>>(_getOneFree);
	_needsItem		= node["needsItem"]		.as<bool>(_needsItem);
	_destroyItem	= node["destroyItem"]	.as<bool>(_destroyItem);
	_markSeen		= node["markSeen"]		.as<bool>(_markSeen);
	_listOrder		= node["listOrder"]		.as<int>(_listOrder);

	if (_listOrder == 0)
		_listOrder = listOrder;
}

/**
 * Gets the type of this RuleResearch.
 * @return, reference to the type
 */
const std::string& RuleResearch::getType() const
{
	return _type;
}

/**
 * Gets the cost of this RuleResearch.
 * @return, cost in man/days
 */
int RuleResearch::getCost() const
{
	return _cost;
}

/**
 * Get the points earned for completing this RuleResearch.
 * @return, points granted
 */
int RuleResearch::getPoints() const
{
	return _points;
}

/**
 * Gets the requisite-research for this RuleResearch.
 * @return, reference to a vector of research-types
 */
const std::vector<std::string>& RuleResearch::getRequisiteResearch() const
{
	return _requisite;
}

/**
 * Gets the required-research for this RuleResearch.
 * @return, reference to a vector of research-types
 */
const std::vector<std::string>& RuleResearch::getRequiredResearch() const
{
	return _required;
}

/**
 * Gets the list of research-types requested when this RuleResearch is discovered.
 * @return, reference to a vector of research-types
 */
const std::vector<std::string>& RuleResearch::getRequestedResearch() const
{
	return _requested;
}

/**
 * Gets the list of research-types granted at random by this RuleResearch.
 * @return, reference to a vector of research-types
 */
const std::vector<std::string>& RuleResearch::getGetOneFree() const
{
	return _getOneFree;
}

/**
 * Gets what article to look up in the Ufopedia.
 * @return, reference to an article to look up
 */
const std::string& RuleResearch::getUfopaediaEntry() const
{
	if (_uPed.empty() == false)
		return _uPed;

	return _type;
}

/**
 * Checks if this RuleResearch needs a corresponding item to begin research.
 * @return, true if item is required
 */
bool RuleResearch::needsItem() const
{
	return _needsItem;
}

/**
 * Checks if this RuleResearch consumes its item when research completes.
 * @return, true if item is consumed
 */
bool RuleResearch::destroyItem() const
{
	return _destroyItem;
}

/**
 * Checks if this RuleResearch should be flagged as seen by default.
 * @return, true if flagged seen
 */
bool RuleResearch::isMarkSeen() const
{
	return _markSeen;
}

/**
 * Gets the list-priority for this RuleResearch.
 * @return, list-priority
 */
int RuleResearch::getListOrder() const
{
	return _listOrder;
}

}
