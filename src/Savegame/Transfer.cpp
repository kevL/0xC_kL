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

#include "Transfer.h"

#include "Craft.h"
#include "ItemContainer.h"
#include "Soldier.h"

#include "../Engine/Language.h"
#include "../Engine/Logger.h"

#include "../Ruleset/Ruleset.h"


namespace OpenXcom
{

/**
 * Initializes a transfer.
 * @param hours - hours in-transit
 */
Transfer::Transfer(int hours)
	:
		_hours(hours),
		_soldier(nullptr),
		_craft(nullptr),
		_itemQty(0),
		_scientists(0),
		_engineers(0),
		_delivered(false)
{}

/**
 * Cleans up undelivered transfers.
 */
Transfer::~Transfer()
{
	if (_delivered == false)
	{
		delete _soldier;
		delete _craft;
	}
}

/**
 * Loads the transfer from a YAML file.
 * @param node	- reference a YAML node
 * @param base	- pointer to a destination Base
 * @param rules	- pointer to the game's Ruleset
 * @return, true if the transfer content is valid
 */
bool Transfer::load(
		const YAML::Node& node,
		Base* const base,
		const Ruleset* const rules)
{
	_hours = node["hours"].as<int>(_hours);

	std::string type;

	if (const YAML::Node& sol = node["soldier"])
	{
		type = sol["type"].as<std::string>(rules->getSoldiersList().front());
		if (rules->getSoldier(type) != nullptr)
		{
			_soldier = new Soldier(rules->getSoldier(type));
			_soldier->load(sol, rules);
		}
		else
		{
			Log(LOG_ERROR) << "Failed to load soldier " << type;
			delete this;
			return false;
		}
	}

	if (const YAML::Node& craft = node["craft"])
	{
		type = craft["type"].as<std::string>();
		if (rules->getCraft(type) != nullptr)
		{
			_craft = new Craft(
							rules->getCraft(craft["type"].as<std::string>()),
							base);
			_craft->load(craft, rules, nullptr);
		}
		else
		{
			Log(LOG_ERROR) << "Failed to load craft " << type;
			delete this;
			return false;
		}
	}

	if (const YAML::Node& item = node["itemId"])
	{
		_itemId = item.as<std::string>(_itemId);
		if (rules->getItem(_itemId) == nullptr)
		{
			Log(LOG_ERROR) << "Failed to load item " << _itemId;
			delete this;
			return false;
		}
	}

	_itemQty	= node["itemQty"]	.as<int>(_itemQty);
	_scientists	= node["scientists"].as<int>(_scientists);
	_engineers	= node["engineers"]	.as<int>(_engineers);
	_delivered	= node["delivered"]	.as<bool>(_delivered);

	return true;
}

/**
 * Saves the transfer to a YAML file.
 * @return, YAML node
 */
YAML::Node Transfer::save() const
{
	YAML::Node node;

	node["hours"] = _hours;

	if		(_soldier != nullptr)	node["soldier"]	= _soldier->save();
	else if (_craft != nullptr)		node["craft"]	= _craft->save();
	else if (_itemQty != 0)
	{
		node["itemId"]	= _itemId;
		node["itemQty"]	= _itemQty;
	}
	else if (_scientists != 0)	node["scientists"]	= _scientists;
	else if (_engineers != 0)	node["engineers"]	= _engineers;

	if (_delivered == true)		node["delivered"]	= _delivered;

	return node;
}

/**
 * Changes the soldier being transfered.
 * @param soldier - pointer to a Soldier
 */
void Transfer::setSoldier(Soldier* const soldier)
{
	_soldier = soldier;
}

/**
 * Gets the soldier being transfered.
 * @return, pointer to Soldier
 */
Soldier* Transfer::getSoldier() const
{
	return _soldier;
}

/**
 * Changes the scientists being transfered.
 * @param scientists - amount of scientists
 */
void Transfer::setScientists(int scientists)
{
	_scientists = scientists;
}

/**
 * Changes the engineers being transfered.
 * @param engineers - amount of engineers
 */
void Transfer::setEngineers(int engineers)
{
	_engineers = engineers;
}

/**
 * Changes the craft being transfered.
 * @param craft - pointer to a Craft
 */
void Transfer::setCraft(Craft* const craft)
{
	_craft = craft;
}

/**
 * Gets the craft being transfered.
 * @return, pointer to the Craft
 */
Craft* Transfer::getCraft() const
{
	return _craft;
}

/**
 * Returns the item-type being transfered.
 * @return, item ID
 */
std::string Transfer::getTransferItems() const
{
	return _itemId;
}

/**
 * Changes the item-type being transfered.
 * @param id	- reference the item ID
 * @param qty	- item quantity (default 1)
 */
void Transfer::setTransferItems(
		const std::string& id,
		int qty)
{
	_itemId = id;
	_itemQty = qty;
}

/**
 * Returns the name of the contents of the transfer.
 * @param lang - pointer to a Language to get strings from
 * @return, name string
 */
std::wstring Transfer::getName(Language* const lang) const
{
	if (_soldier != nullptr)
		return _soldier->getName();

	if (_craft != nullptr)
		return _craft->getName(lang);

	if (_scientists != 0)
		return lang->getString("STR_SCIENTISTS");

	if (_engineers != 0)
		return lang->getString("STR_ENGINEERS");

	return lang->getString(_itemId);
}

/**
 * Returns the time remaining until the transfer arrives at its destination.
 * @return, amount of hours
 */
int Transfer::getHours() const
{
	return _hours;
}

/**
 * Returns the quantity of items in the transfer.
 * @return, amount of items
 */
int Transfer::getQuantity() const
{
	if (_itemQty != 0)
		return _itemQty;

	if (_scientists != 0)
		return _scientists;

	if (_engineers != 0)
		return _engineers;

	return 1;
}

/**
 * Returns the type of the contents of the transfer.
 * @return, PurchaseSellTransferType (Base.h)
 */
PurchaseSellTransferType Transfer::getTransferType() const
{
	if (_soldier != nullptr)
		return PST_SOLDIER;

	if (_craft != nullptr)
		return PST_CRAFT;

	if (_scientists != 0)
		return PST_SCIENTIST;

	if (_engineers != 0)
		return PST_ENGINEER;

	return PST_ITEM;
}

/**
 * Advances the transfer and takes care of the delivery once it's arrived.
 * @param base - pointer to destination Base
 */
void Transfer::advance(Base* const base)
{
	if (--_hours == 0)
	{
		_delivered = true;

		if (_soldier != nullptr)
			base->getSoldiers()->insert(
									base->getSoldiers()->begin(),
									_soldier);
		else if (_craft != nullptr)
		{
			base->getCrafts()->push_back(_craft);
			_craft->setBase(base);
			_craft->checkup();
		}
		else if (_itemQty != 0)
			base->getStorageItems()->addItem(_itemId, _itemQty);
		else if (_scientists != 0)
			base->setScientists(base->getScientists() + _scientists);
		else if (_engineers != 0)
			base->setEngineers(base->getEngineers() + _engineers);
	}
}

}
