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

#include "RuleInventory.h"

//#include <cmath>

#include "RuleItem.h"

#include "../Engine/Options.h"


namespace YAML
{

template<>
struct convert<OpenXcom::RuleSlot>
{
	///
	static Node encode(const OpenXcom::RuleSlot& rhs)
	{
		Node node;

		node.push_back(rhs.x);
		node.push_back(rhs.y);

		return node;
	}

	///
	static bool decode(const Node& node, OpenXcom::RuleSlot& rhs)
	{
		if (node.IsSequence() == false
			|| node.size() != 2)
		{
			return false;
		}

		rhs.x = node[0].as<int>();
		rhs.y = node[1].as<int>();

		return true;
	}
};

}


namespace OpenXcom
{

/**
 * Creates a blank ruleset for a certain type of Inventory section.
 * @param id - string defining the type
 */
RuleInventory::RuleInventory(const std::string& type)
	:
		_type(type),
		_x(0),
		_y(0),
		_cat(IC_SLOT),
		_section(ST_NONE),
		_listOrder(0)
{}

/**
 * dTor.
 */
RuleInventory::~RuleInventory()
{}

/**
 * Loads this Inventory section's rules from a YAML file.
 * @param node - reference a YAML node
 * @param listOrder - the list weight for this inventory
 */
void RuleInventory::load(
		const YAML::Node &node,
		int listOrder)
{
	_type		= node["type"]		.as<std::string>(_type);
	_x			= node["x"]			.as<int>(_x);
	_y			= node["y"]			.as<int>(_y);
	_slots		= node["slots"]		.as<std::vector<RuleSlot>>(_slots);
	_listOrder	= node["listOrder"]	.as<int>(listOrder);

	_x += (Options::baseXResolution - 320) / 2;
	_y += (Options::baseYResolution - 200) / 2 - 20; // See Inventory if you want to.

	_cat = static_cast<InventoryCategory>(node["category"].as<int>(_cat));

	// convert crappy strings into speedy Enums:
	std::map<std::string, int> costs = node["costs"].as<std::map<std::string, int>>();
	_costs = assignCosts(costs);
	_section = assignSectionType(_type);
}

/**
 * Gets the string that identifies this RuleInventory section.
 * @return, section string
 */
std::string RuleInventory::getInventoryType() const
{
	return _type;
}

/**
 * Gets the InventorySection that identifies this RuleInventory section.
 * @return, InventorySection (RuleInventory.h)
 */
InventorySection RuleInventory::getSectionType() const
{
	return _section;
}

/**
 * Sets this rule's InventorySection based on the typeId.
 * @param type - type-string of this RuleInventory
 * @return, the corresponding InventorySection (RuleInventory.h)
 */
InventorySection RuleInventory::assignSectionType(const std::string& type) // private/static.
{
	if (type == "STR_GROUND")
		return ST_GROUND;

	if (type == "STR_RIGHT_HAND")
		return ST_RIGHTHAND;

	if (type == "STR_LEFT_HAND")
		return ST_LEFTHAND;

	if (type == "STR_BELT")
		return ST_BELT;

	if (type == "STR_RIGHT_LEG")
		return ST_RIGHTLEG;

	if (type == "STR_LEFT_LEG")
		return ST_LEFTLEG;

	if (type == "STR_RIGHT_SHOULDER")
		return ST_RIGHTSHOULDER;

	if (type == "STR_LEFT_SHOULDER")
		return ST_LEFTSHOULDER;

	if (type == "STR_BACK_PACK")
		return ST_BACKPACK;

	if (type == "STR_QUICK_DRAW")
		return ST_QUICKDRAW;

	return ST_NONE; // better not happen.
}

/**
 * Sets the rule's costs based on typeIds.
 * @param costs - reference to a map of strings & ints of the costs
 * @return, a map of InventorySections & ints (RuleInventory.h)
 */
std::map<InventorySection, int> RuleInventory::assignCosts(std::map<std::string, int>& costs) // private/static.
{
	std::map<InventorySection, int> ret;

	if (costs["STR_GROUND"] != 0)
		ret[ST_GROUND] = costs["STR_GROUND"];

	if (costs["STR_RIGHT_HAND"] != 0)
		ret[ST_RIGHTHAND] = costs["STR_RIGHT_HAND"];

	if (costs["STR_LEFT_HAND"] != 0)
		ret[ST_LEFTHAND] = costs["STR_LEFT_HAND"];

	if (costs["STR_BELT"] != 0)
		ret[ST_BELT] = costs["STR_BELT"];

	if (costs["STR_RIGHT_LEG"] != 0)
		ret[ST_RIGHTLEG] = costs["STR_RIGHT_LEG"];

	if (costs["STR_LEFT_LEG"] != 0)
		ret[ST_LEFTLEG] = costs["STR_LEFT_LEG"];

	if (costs["STR_RIGHT_SHOULDER"] != 0)
		ret[ST_RIGHTSHOULDER] = costs["STR_RIGHT_SHOULDER"];

	if (costs["STR_LEFT_SHOULDER"] != 0)
		ret[ST_LEFTSHOULDER] = costs["STR_LEFT_SHOULDER"];

	if (costs["STR_BACK_PACK"] != 0)
		ret[ST_BACKPACK] = costs["STR_BACK_PACK"];

	if (costs["STR_QUICK_DRAW"] != 0)
		ret[ST_QUICKDRAW] = costs["STR_QUICK_DRAW"];

	return ret;
}

/**
 * Gets the X position of this Inventory section on the screen.
 * @return, X position in pixels
 */
int RuleInventory::getX() const
{
	return _x;
}

/**
 * Gets the Y position of this Inventory section on the screen.
 * @return, Y position in pixels
 */
int RuleInventory::getY() const
{
	return _y;
}

/**
 * Gets the category of this Inventory section.
 * @note Slot-based contain a limited number of slots. Hands only contain one
 * slot but can hold any item. Ground can hold infinite items but it doesn't
 * attach to soldiers.
 * @return, InventoryCategory (RuleInventory.h)
 */
InventoryCategory RuleInventory::getCategory() const
{
	return _cat;
}

/**
 * Gets all the Slots in this Inventory section.
 * @return, pointer to a vector of RuleSlot-structs
 */
const std::vector<struct RuleSlot>* RuleInventory::getSlots()
{
	return &_slots;
}

/**
 * Checks if there's a Slot located in the specified mouse-position.
 * @param x - mouse X position; returns the slot's X position
 * @param y - mouse Y position; returns the slot's Y position
 * @return, true if there's a slot here
 */
bool RuleInventory::checkSlotAtPosition(
		int* x,
		int* y) const
{
	const int
		mouseX = *x,
		mouseY = *y;

	switch (_cat)
	{
		case IC_HAND:
			for (int
					x1 = 0;
					x1 != HAND_W;
					++x1)
			{
				for (int
						y1 = 0;
						y1 != HAND_H;
						++y1)
				{
					if (   mouseX >= _x + SLOT_W *  x1
						&& mouseX <  _x + SLOT_W * (x1 + 1)
						&& mouseY >= _y + SLOT_H *  y1
						&& mouseY <  _y + SLOT_H * (y1 + 1))
					{
						*x =
						*y = 0;
						return true;
					}
				}
			}
		break;

		case IC_GROUND:
			if (   mouseX >= _x
				&& mouseX <  _x + SLOT_W * GROUND_W
				&& mouseY >= _y
				&& mouseY <  _y + SLOT_H * GROUND_H)
			{
				*x = static_cast<int>(std::floor(
					 static_cast<double>(mouseX - _x) / static_cast<double>(SLOT_W)));
				*y = static_cast<int>(std::floor(
					 static_cast<double>(mouseY - _y) / static_cast<double>(SLOT_H)));

				return true;
			}
		break;

		default:
			for (std::vector<RuleSlot>::const_iterator
					i = _slots.begin();
					i != _slots.end();
					++i)
			{
				if (   mouseX >= _x + SLOT_W *  i->x
					&& mouseX <  _x + SLOT_W * (i->x + 1)
					&& mouseY >= _y + SLOT_H *  i->y
					&& mouseY <  _y + SLOT_H * (i->y + 1))
				{
					*x = i->x;
					*y = i->y;

					return true;
				}
			}
	}

	return false;
}

/**
 * Checks if an item completely fits into a certain Slot-position.
 * @param item	- pointer to RuleItem
 * @param x		- slot X position
 * @param y		- slot Y position
 * @return, true if there's room there
 */
bool RuleInventory::fitItemInSlot(
		const RuleItem* const item,
		int x,
		int y) const
{
	switch (_cat)
	{
		case IC_GROUND:
		{
			int xOffset = 0;
			while (x >= xOffset + GROUND_W)
				xOffset += GROUND_W;

			for (int
					find_x = x;
					find_x != x + item->getInventoryWidth();
					++find_x)
			{
				for (int
						find_y = y;
						find_y != y + item->getInventoryHeight();
						++find_y)
				{
					if (!
							(  find_x >= xOffset
							&& find_x <  xOffset + GROUND_W
							&& find_y > -1
							&& find_y < GROUND_H))
					{
						return false;
					}
				}
			}
		}
		case IC_HAND:
			return true;

		default:
		{
			const int slotsTotal = item->getInventoryWidth() * item->getInventoryHeight();
			int slotsFound = 0;

			for (std::vector<RuleSlot>::const_iterator
					i = _slots.begin();
					i != _slots.end() && slotsFound < slotsTotal;
					++i)
			{
				if (   i->x >= x
					&& i->x <  x + item->getInventoryWidth()
					&& i->y >= y
					&& i->y <  y + item->getInventoryHeight())
				{
					++slotsFound;
				}
			}

			return (slotsFound == slotsTotal);
		}
	}
}

/**
 * Gets the time unit cost to move an item from this Inventory section to another.
 * @param slot - pointer to the section to move the item to
 * @return, TU cost
 */
int RuleInventory::getCost(const RuleInventory* const inRule) const
{
	if (inRule != this)
		return _costs.at(inRule->getSectionType());

	return 0;
}

/**
 * Gets the list order.
 * @return, list order
 */
int RuleInventory::getListOrder() const
{
	return _listOrder;
}

}
