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

#ifndef OPENXCOM_RULEINVENTORY_H
#define OPENXCOM_RULEINVENTORY_H

//#include <map>
//#include <string>
//#include <vector>

#include <yaml-cpp/yaml.h>


namespace OpenXcom
{

struct RuleSlot
{
	int
		x,y;
};


enum InventoryCategory
{
	INV_SLOT,	// 0
	INV_HAND,	// 1
	INV_GROUND	// 2
};


class RuleItem;


/**
 * Represents a specific section of the Inventory.
 * @note This contains information like Slots and screen position.
 */
class RuleInventory
{

private:
	int
		_listOrder,
		_x,
		_y;
	std::string _type;

	InventoryCategory _cat;

	std::map<std::string, int> _costs;
	std::vector<RuleSlot> _slots;


	public:
		static const int
			SLOT_W = 16,
			SLOT_H = 16,
			HAND_W = 2,
			HAND_H = 3;

		/// Creates a blank Inventory ruleset.
		explicit RuleInventory(const std::string& type);
		/// Cleans up the Inventory ruleset.
		~RuleInventory();

		/// Loads the Inventory data from YAML.
		void load(
				const YAML::Node& node,
				int listOrder);

		/// Gets the Inventory's type.
		std::string getInventoryType() const;

		/// Gets the X position of the Inventory.
		int getX() const;
		/// Gets the Y position of the Inventory.
		int getY() const;

		/// Gets the Inventory category.
		InventoryCategory getCategory() const;

		/// Gets all the Slots in the Inventory.
		const std::vector<struct RuleSlot>* getSlots();
		/// Gets the number of Slots along the top row of this Inventory.
//		int getSlotsX() const;

		/// Checks for a Slot in a mouse position.
		bool checkSlotInPosition(
				int* x,
				int* y) const;
		/// Checks if an item fits into a Slot-position.
		bool fitItemInSlot(
				const RuleItem* const item,
				int x,
				int y) const;

		/// Gets the TU cost to move an item in the Inventory.
		int getCost(const RuleInventory* const section) const;

		/// Gets the list order.
		int getListOrder() const;
};

}

#endif
