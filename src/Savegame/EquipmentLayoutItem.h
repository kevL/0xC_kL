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

#ifndef OPENXCOM_EQUIPMENTLAYOUTITEM_H
#define OPENXCOM_EQUIPMENTLAYOUTITEM_H

//#include <string>

#include <yaml-cpp/yaml.h>


namespace OpenXcom
{

/**
 * Represents a Soldier's EquipmentLayoutItem which is used to equip a Soldier
 * for tactical battles.
 */
class EquipmentLayoutItem
{

private:
	int
		_fuse,
		_slotX,
		_slotY;

	std::string
		_ammoItem,
		_itemType,
		_section;


	public:
		/// Creates a Soldier's EquipmentLayoutItem.
		EquipmentLayoutItem(
				const std::string& itemType,
				const std::string& section,
				int slotX,
				int slotY,
				const std::string& ammoItem,
				int fuse);
		/// Creates a Soldier's EquipmentLayoutItem and loads its contents from YAML.
		explicit EquipmentLayoutItem(const YAML::Node& node);
		/// Cleans up the Soldier's EquipmentLayoutItem.
		~EquipmentLayoutItem();

		/// Loads the Soldier's EquipmentLayoutItem from YAML.
		void load(const YAML::Node& node);
		/// Saves the Soldier's EquipmentLayoutItem to YAML.
		YAML::Node save() const;

		/// Gets the item's type which has to be in an Inventory section.
		std::string getItemType() const;
		/// Gets the Inventory section to be occupied.
		std::string getLayoutSection() const;
		/// Gets the slotX to be occupied.
		int getSlotX() const;
		/// Gets the slotY to be occupied.
		int getSlotY() const;

		/// Gets the item's ammo-item.
		std::string getAmmoItem() const;

		/// Gets the turn until explosion.
		int getFuse() const;
};

}

#endif
