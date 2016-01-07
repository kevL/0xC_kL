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

#include "EquipmentLayoutItem.h"


namespace OpenXcom
{

/**
 * Initializes a Soldier's EquipmentLayoutItem.
 * @param itemType	- reference to the item's type
 * @param slot		- reference to the occupied section type
 * @param slotX		- position-X in the occupied slot
 * @param slotY		- position-Y in the occupied slot
 * @param ammoItem	- reference to the ammo that has to be loaded into the item (its type)
 * @param fuse		- the turn until explosion of the item (if it's an activated grenade-type)
 */
EquipmentLayoutItem::EquipmentLayoutItem(
		const std::string& itemType,
		const std::string& section,
		int slotX,
		int slotY,
		const std::string& ammoItem,
		int fuse)
	:
		_itemType(itemType),
		_section(section),
		_slotX(slotX),
		_slotY(slotY),
		_ammoItem(ammoItem),
		_fuse(fuse)
{}

/**
 * Initializes a Soldier's EquipmentLayoutItem from YAML.
 * @param node, YAML node.
 */
EquipmentLayoutItem::EquipmentLayoutItem(const YAML::Node& node)
{
	load(node);
}

/**
 * dTor.
 */
EquipmentLayoutItem::~EquipmentLayoutItem()
{}

/**
 * Loads the Soldier's EquipmentLayoutItem from a YAML file.
 * @param node - reference a YAML node
 */
void EquipmentLayoutItem::load(const YAML::Node& node)
{
	_itemType	= node["itemType"]	.as<std::string>(_itemType);
	_section	= node["section"]	.as<std::string>(_section);
	_slotX		= node["slotX"]		.as<int>(0);
	_slotY		= node["slotY"]		.as<int>(0);
	_ammoItem	= node["ammoItem"]	.as<std::string>("NONE");
	_fuse		= node["fuse"]		.as<int>(-1);
}

/**
 * Saves the Soldier's EquipmentLayoutItem to a YAML file.
 * @return, YAML node
 */
YAML::Node EquipmentLayoutItem::save() const
{
	YAML::Node node;

	node["itemType"]	= _itemType;
	node["section"]		= _section;

	if (_slotX != 0)			node["slotX"]		= _slotX;
	if (_slotY != 0)			node["slotY"]		= _slotY;
	if (_ammoItem != "NONE")	node["ammoItem"]	= _ammoItem;
	if (_fuse > -1)				node["fuse"]		= _fuse;

	return node;
}

/**
 * Returns the item's type which has to be in an Inventory section.
 * @return, item type
 */
std::string EquipmentLayoutItem::getItemType() const
{
	return _itemType;
}

/**
 * Returns the Inventory section to be occupied.
 * @return, section type
 */
std::string EquipmentLayoutItem::getLayoutSection() const
{
	return _section;
}

/**
 * Returns the position-X in the Inventory section to be occupied.
 * @return, slot-X
 */
int EquipmentLayoutItem::getSlotX() const
{
	return _slotX;
}

/**
 * Returns the position-Y in the Inventory section to be occupied.
 * @return, slot-Y
 */
int EquipmentLayoutItem::getSlotY() const
{
	return _slotY;
}

/**
 * Returns the ammo-type that has to be loaded into the item.
 * @return, the ammo-type
 */
std::string EquipmentLayoutItem::getAmmoType() const
{
	return _ammoItem;
}

/**
 * Returns the turn until explosion of the item (if it's an activated grenade-type).
 * @return, turn count
 */
int EquipmentLayoutItem::getFuse() const
{
	return _fuse;
}

}
