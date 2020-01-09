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

#ifndef OPENXCOM_RULEINVENTORY_H
#define OPENXCOM_RULEINVENTORY_H

//#include <map>
//#include <string>
//#include <vector>

#include <yaml-cpp/yaml.h>


namespace OpenXcom
{

/**
 * A struct that holds x-y slot-positions of Inventory sections.
 */
struct InSlot
{
	int
		x,y;
};


enum InventorySection
{
 	ST_NONE,			//  0
	ST_GROUND,			//  1
	ST_RIGHTHAND,		//  2
	ST_LEFTHAND,		//  3
	ST_BELT,			//  4
	ST_RIGHTLEG,		//  5
	ST_LEFTLEG,			//  6
	ST_RIGHTSHOULDER,	//  7
	ST_LEFTSHOULDER,	//  8
	ST_BACKPACK,		//  9
	ST_QUICKDRAW		// 10
};

enum InventoryCategory
{
	IC_SLOT,	// 0
	IC_HAND,	// 1
	IC_GROUND	// 2
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
	InventorySection _section;

	std::map<InventorySection, int> _costs;
	std::vector<InSlot> _slots;

	/// Sets the rule's InventorySection based on the typeId.
	static InventorySection assignSectionType(const std::string& type);
	/// Sets the rule's costs based on typeIds.
	static std::map<InventorySection, int> assignCosts(std::map<std::string, int>& costs);


	public:
		static const int
			SLOT_W		= 16,
			SLOT_H		= 16,
			SLOT_W_2	=  8,
			SLOT_H_2	=  8,
			HAND_W		=  2,
			HAND_H		=  3,
			GROUND_W	= 20,
			GROUND_H	=  3;

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
		/// Gets the Inventory's section.
		InventorySection getSectionType() const;

		/// Gets the x-position of the Inventory.
		int getX() const;
		/// Gets the y-position of the Inventory.
		int getY() const;

		/// Gets the Inventory category.
		InventoryCategory getCategory() const;

		/// Gets all the Slots in the Inventory.
		const std::vector<InSlot>* getSlots() const;

		/// Checks for a Slot in a mouse position.
		bool detSlotAtCursor(
				int* x,
				int* y) const;
		/// Checks if an item fits into a Slot-position.
		bool fitItemInSlot(
				const RuleItem* const item,
				int x,
				int y) const;

		/// Gets the TU-cost to move an item in the Inventory.
		int getCost(const RuleInventory* const inRule) const;

		/// Gets the list order.
		int getListOrder() const;
};

}

#endif
