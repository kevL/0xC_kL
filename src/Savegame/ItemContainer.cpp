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

#include "ItemContainer.h"

//#include "../Engine/Logger.h"

#include "../Ruleset/RuleItem.h"
#include "../Ruleset/Ruleset.h"


namespace OpenXcom
{

/**
 * Initializes an ItemContainer with no contents.
 */
ItemContainer::ItemContainer()
{}

/**
 * dTor.
 */
ItemContainer::~ItemContainer()
{}

/**
 * Loads the ItemContainer from a YAML file.
 * @param node - reference a YAML node
 */
void ItemContainer::load(const YAML::Node& node)
{
	_contents = node.as<std::map<std::string, int>>(_contents);
}

/**
 * Saves the item container to a YAML file.
 * @return, YAML node
 */
YAML::Node ItemContainer::save() const
{
	YAML::Node node;

	node = _contents;

	return node;
}

/**
 * Modifies an entry in the Container.
 * @note Usage is intended to supercede add/removeItem() functions.
 * @param type	- reference to an item-type
 * @param qty	- modification (default +1)
 *
void ItemContainer::modifyContents(
		const std::string& type,
		int qty)
{
	if (_contents.find(type) != _contents.end())
	{
		_contents[type] += qty;
		if (_contents[type] < 1)
			_contents.erase(type);
	}
	else if (qty > 0)
		_contents[type] = qty;
} */

/**
 * Adds an item amount to this Container.
 * @param type	- reference an item type
 * @param qty	- item quantity (default 1)
 */
void ItemContainer::addItem(
		const std::string& type,
		int qty)
{
	if (qty > 0 && type.empty() == false) // likely redundant in a well-wrought ruleset.
	{
		if (_contents.find(type) != _contents.end())
			_contents[type] += qty;
		else
			_contents[type] = qty;
	}
	//else Log(LOG_WARNING) << "ItemContainer::addItem() failed ID " << type;
}

/**
 * Removes an item amount from this Container.
 * @param type	- reference an item type
 * @param qty	- item quantity (default 1)
// * @return, an iterator to the next position or end() if not found
 */
void ItemContainer::removeItem(
		const std::string& type,
		int qty)
{
	if (qty > 0 && type.empty() == false // likely redundant in a well-wrought ruleset.
		&& _contents.find(type) != _contents.end())
	{
		if (qty < _contents[type])
			_contents[type] -= qty;
		else
			_contents.erase(type);
	}
	//else if (type.empty() == true) Log(LOG_WARNING) << "ItemContainer::removeItem() failed ID " << type;
	//else if (_contents.find(type) == _contents.end()) Log(LOG_WARNING) << "ItemContainer::removeItem() failed to find " << type;
}
/* std::map<std::string, int>::const_iterator ItemContainer::removeItem(const std::string& type, int qty)
{
	for (std::map<std::string, int>::const_iterator i = _contents.begin(); i != _contents.end(); ++i)
	{
		if (i->first == type)
		{
			if (qty < _contents[type])
			{
				_contents[type] -= qty;
				return ++i;
			}
			return _contents.erase(i);
		}
	}
	return _contents.end();
} */

/**
 * Gets the quantity of an item in the container.
 * @param type - reference an item type
 * @return, item quantity
 */
int ItemContainer::getItemQuantity(const std::string& type) const
{
	if (type.empty() == false) // likely redundant in a well-wrought ruleset.
	{
		std::map<std::string, int>::const_iterator i (_contents.find(type));
		if (i != _contents.end())
			return i->second;
	}
	return 0;
}

/**
 * Gets the total quantity of the items in the container.
 * @return, total item quantity
 */
int ItemContainer::getTotalQuantity() const
{
	int total (0);
	for (std::map<std::string, int>::const_iterator
			i = _contents.begin();
			i != _contents.end();
			++i)
	{
		total += i->second;
	}
	return total;
}

/**
 * Checks if the container is empty.
 * return, true if empty
 */
bool ItemContainer::isEmpty()
{
	for (std::map<std::string, int>::const_iterator
			i = _contents.begin();
			i != _contents.end();
			++i)
	{
		if (i->second != 0)
			return false;
	}
	return true;
}

/**
 * Gets the total size (in storage-units) of the items in the container.
 * @param rules - pointer to Ruleset
 * @return, total item size
 */
double ItemContainer::getTotalSize(const Ruleset* const rules) const
{
	double total (0.);
	for (std::map<std::string, int>::const_iterator
			i = _contents.begin();
			i != _contents.end();
			++i)
	{
		total += rules->getItemRule(i->first)->getStoreSize() * static_cast<double>(i->second);
	}
	return total;
}

/**
 * Returns all the items currently contained within.
 * @return, pointer to the map of contents
 */
std::map<std::string, int>* ItemContainer::getContents()
{
	return &_contents;
}

}
