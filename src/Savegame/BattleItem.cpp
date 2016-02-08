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

#include "BattleItem.h"

#include "BattleUnit.h"
#include "Tile.h"

//#include "../Engine/Logger.h"

#include "../Ruleset/RuleInventory.h"
#include "../Ruleset/RuleItem.h"

#include "../Savegame/SavedBattleGame.h"


namespace OpenXcom
{

/**
 * Initializes an item of the specified type.
 * @param itRule	- pointer to RuleItem
 * @param pId		- pointer to an integer ID for this item
 * @param id		- value for ID when loading a saved game (default -1)
 */
BattleItem::BattleItem(
		RuleItem* const itRule,
		int* pId,
		int id)
	:
		_itRule(itRule),
		_owner(nullptr),
		_ownerPre(nullptr),
		_unit(nullptr),
		_tile(nullptr),
		_section(nullptr),
		_inventoryX(0),
		_inventoryY(0),
		_ammoItem(nullptr),
		_ammoQty(itRule->getClipSize()),
		_fuse(-1),
		_painKiller(0),
		_heal(0),
		_stimulant(0),
		_isLoad(false)
{
	if (pId != nullptr)	// <- this is for SavedBattleGame only to keep
	{					// track of a brand new item on the battlefield
		_id = *pId;
		++(*pId);
	}
	else				// load item from saved game
		_id = id;


	if (_itRule->getBattleType() == BT_MEDIKIT)
	{
		_heal		= _itRule->getHealQuantity();
		_painKiller	= _itRule->getPainKillerQuantity();
		_stimulant	= _itRule->getStimulantQuantity();
	}
	else if (_itRule->getBattleType() == BT_MELEE				// Melee weapons do NOT take ammo atm.
		|| (_itRule->getBattleType() == BT_FIREARM				// These weapons do not need ammo -->
			&& _itRule->getCompatibleAmmo()->empty() == true))	// '_ammoItem' points to the weapon itself.
	{
		// lasers, melee, etc have "clipsize -1"
//		setAmmoQuantity(-1);	// needed for melee-item reaction hits, etc. (can be set in Ruleset but do it here)
								// Except that it creates problems w/ TANKS returning to Base. So do it in Ruleset:
								// melee items need "clipSize -1" to do reactionFire. Unless i changed it ....
		_ammoItem = this;
	}
}

/**
 * dTor.
 */
BattleItem::~BattleItem()
{}

/**
 * Loads the item from a YAML file.
 * @param node - YAML node
 */
void BattleItem::load(const YAML::Node& node)
{
	_inventoryX	= node["inventoryX"].as<int>(_inventoryX);
	_inventoryY	= node["inventoryY"].as<int>(_inventoryY);
	_ammoQty	= node["ammoQty"]	.as<int>(_ammoQty);
	_painKiller	= node["painKiller"].as<int>(_painKiller);
	_heal		= node["heal"]		.as<int>(_heal);
	_stimulant	= node["stimulant"]	.as<int>(_stimulant);
	_fuse		= node["fuse"]		.as<int>(_fuse);
}

/**
 * Saves the item to a YAML file.
 * @return, YAML node
 */
YAML::Node BattleItem::save() const
{
	YAML::Node node;

	node["id"]   = _id;
	node["type"] = _itRule->getType();

	if (_inventoryX != 0) node["inventoryX"]	= _inventoryX;
	if (_inventoryY != 0) node["inventoryY"]	= _inventoryY;

	if (_ammoQty > 0)		node["ammoQty"]		= _ammoQty;
	if (_painKiller != 0)	node["painKiller"]	= _painKiller;
	if (_heal != 0)			node["heal"]		= _heal;
	if (_stimulant != 0)	node["stimulant"]	= _stimulant;
	if (_fuse != -1)		node["fuse"]		= _fuse;


	if (_owner != nullptr)		node["owner"]		= _owner->getId();
//	else					node["owner"] = -1; // cf. SavedBattleGame::load()
	if (_ownerPre != nullptr
		&& _ownerPre != _owner)	node["ownerPre"]	= _ownerPre->getId();


	if (_unit != nullptr)		node["unit"]		= _unit->getId();
//	else						node["unit"] = -1; // cf. SavedBattleGame::load()
	if (_section != nullptr)	node["section"]		= _section->getInventoryType(); // note: 'section' should always be valid. Unless it's a loaded Ammo-item.
//	else						node["section"] = "NONE"; // cf. SavedBattleGame::load()
	if (_tile != nullptr)		node["position"]	= _tile->getPosition();
//	else						node["position"] = Position(-1,-1,-1); // cf. SavedBattleGame::load()
	if (_ammoItem != nullptr)	node["ammoItem"]	= _ammoItem->getId();
//	else						node["ammoItem"] = -1; // cf. SavedBattleGame::load()

	return node;
}

/**
 * Gets the ruleset for the item's type.
 * @return, pointer to ruleset
 */
RuleItem* BattleItem::getRules() const
{
	return _itRule;
}

/**
 * Gets the turns until detonation. -1 = unprimed grenade
 * @return, turns until detonation
 */
int BattleItem::getFuse() const
{
	return _fuse;
}

/**
 * Sets the turns until detonation.
 * @param turns - turns until detonation (player/alien turns not full turns)
 */
void BattleItem::setFuse(int turn)
{
	_fuse = turn;
}

/**
 * Gets the '_ammoQty' of this BattleItem.
 * @return, ammo quantity
 *			0		- item is not ammo
 *			<#>		- ammo qty.
 *			INT_MAX	- item is its own ammo
 */
int BattleItem::getAmmoQuantity() const
{
	if (_ammoQty == -1)
		return std::numeric_limits<int>::max();

	return _ammoQty;
}

/**
 * Sets the '_ammoQty' of this BattleItem.
 * @param qty - ammo quantity
 */
void BattleItem::setAmmoQuantity(int qty)
{
	_ammoQty = qty;
}

/**
 * Gets if the item is loaded in a weapon.
 * @return, true if loaded
 */
bool BattleItem::isLoad() const
{
	return _isLoad;
}

/**
 * Gets this BattleItem's currently loaded ammo-item.
 * @return, pointer to BattleItem or nullptr
 *			- nullptr if this BattleItem has no ammo loaded OR is a clip on its own
 *			- the loaded ammo-item OR the weapon itself if weapon is its own ammo
 */
BattleItem* BattleItem::getAmmoItem() const
{
	return _ammoItem;
}

/**
 * Sets an ammo item for this BattleItem.
 * @param item		- the ammo item (default nullptr)
 * @param loadSave	- true if called from SavedBattleGame::load() (default false)
 * @return,	 0 = successful load or unload or nothing to unload
 *			-1 = weapon already contains ammo
 *			-2 = item doesn't fit / weapon is self-powered
 */
int BattleItem::setAmmoItem(
		BattleItem* const item,
		bool loadSave)
{
	if (_ammoItem != this)
	{
		if (item == nullptr) // unload weapon
		{
			if (_ammoItem != nullptr)
			{
				_ammoItem->_isLoad = false;
				_ammoItem = nullptr;
			}
			return 0;
		}

		if (_ammoItem != nullptr)
			return -1;

		for (std::vector<std::string>::const_iterator
				i = _itRule->getCompatibleAmmo()->begin();
				i != _itRule->getCompatibleAmmo()->end();
				++i)
		{
			if (*i == item->getRules()->getType()) // load weapon
			{
				_ammoItem = item;
				_ammoItem->_isLoad = true;
				if (loadSave == false)
				{
					_ammoItem->setInventorySection();
					_ammoItem->changeOwner();
				}
				return 0;
			}
		}
	}

	return -2;
}

/**
 * Determines if this BattleItem uses ammo OR is self-powered.
 * @note No ammo is needed if the item has itself assigned as its '_ammoItem'.
 * @return, true if self-powered
 */
bool BattleItem::selfPowered() const
{
	return (_ammoItem == this);
}

/**
 * Spends a bullet from this BattleItem.
 * @param battleSave	- reference to the SavedBattleGame
 * @param weapon		- reference to the weapon containing this ammo
 */
void BattleItem::spendBullet(
		SavedBattleGame& battleSave,
		BattleItem& weapon)
{
	if (_ammoQty != -1 // <- infinite ammo
		&& --_ammoQty == 0)
	{
		weapon.setAmmoItem();
		battleSave.toDeleteItem(this);
	}
}

/**
 * Gets this BattleItem's owner.
 * @return, pointer to a BattleUnit
 */
BattleUnit* BattleItem::getOwner() const
{
	return _owner;
}

/**
 * Sets this BattleItem's owner.
 * @param owner - pointer to BattleUnit (default nullptr)
 */
void BattleItem::setOwner(BattleUnit* const owner)
{
	_ownerPre = _owner;
	_owner = owner;
}

/**
 * Gets this BattleItem's prior owner.
 * @return, pointer to a BattleUnit
 */
BattleUnit* BattleItem::getPriorOwner() const
{
	return _ownerPre;
}

/**
 * Sets this BattleItem's prior owner.
 * @param ownerPre - pointer to a BattleUnit
 */
void BattleItem::setPriorOwner(BattleUnit* const ownerPre)
{
	_ownerPre = ownerPre;
}

/**
 * Moves this BattleItem from a previous owner if applicable and moves it to
 * another owner if applicable.
 * @param owner - pointer to a BattleUnit (default nullptr)
 */
void BattleItem::changeOwner(BattleUnit* const owner)
{
	if (_owner != nullptr)
		_ownerPre = _owner;
	else
		_ownerPre = owner;

	_owner = owner;

	if (_ownerPre != nullptr)
	{
		for (std::vector<BattleItem*>::const_iterator
				i = _ownerPre->getInventory()->begin();
				i != _ownerPre->getInventory()->end();
				++i)
		{
			if (*i == this)
			{
				_ownerPre->getInventory()->erase(i);
				break;
			}
		}
	}

	if (_owner != nullptr)
		_owner->getInventory()->push_back(this);
}

/**
 * Gets the item's inventory section.
 * @return, the section rule
 */
const RuleInventory* BattleItem::getInventorySection() const
{
	return _section;
}

/**
 * Sets the item's inventory section.
 * @param slot - the section rule (default nullptr)
 */
void BattleItem::setInventorySection(const RuleInventory* const inRule)
{
	_section = inRule;
}

/**
 * Gets the item's inventory X position.
 * @return, X position
 */
int BattleItem::getSlotX() const
{
	return _inventoryX;
}

/**
 * Sets the item's inventory X position.
 * @param x - X position
 */
void BattleItem::setSlotX(int x)
{
	_inventoryX = x;
}

/**
 * Gets the item's inventory Y position.
 * @return, Y position
 */
int BattleItem::getSlotY() const
{
	return _inventoryY;
}

/**
 * Sets the item's inventory Y position.
 * @param y - Y position
 */
void BattleItem::setSlotY(int y)
{
	_inventoryY = y;
}

/**
 * Checks if an item occupies x-y inventory slot(s).
 * @param x		- slot X position
 * @param y 	- slot Y position
 * @param item	- pointer to an item to check to place (default nullptr)
 * @return, true if item occupies x-y slot
 */
bool BattleItem::occupiesSlot(
		int x,
		int y,
		const BattleItem* const item) const
{
	if (item == this)
		return false;

	if (_section->getCategory() == IC_HAND)
		return true;

	if (item == nullptr)
		return (   x >= _inventoryX
				&& x <  _inventoryX + _itRule->getInventoryWidth()
				&& y >= _inventoryY
				&& y <  _inventoryY + _itRule->getInventoryHeight());

	return (	x + item->getRules()->getInventoryWidth() > _inventoryX
			 && x < _inventoryX + _itRule->getInventoryWidth()
			 && y + item->getRules()->getInventoryHeight() > _inventoryY
			 && y < _inventoryY + _itRule->getInventoryHeight());
/*	return !(
				x >= _inventoryX + _itRule->getInventoryWidth()
			 || x + item->getRules()->getInventoryWidth() <= _inventoryX
			 || y >= _inventoryY + _itRule->getInventoryHeight()
			 || y + item->getRules()->getInventoryHeight() <= _inventoryY); */
}

/**
 * Gets the item's tile.
 * @return, Pointer to the Tile
 */
Tile* BattleItem::getTile() const
{
	return _tile;
}

/**
 * Sets the item's tile.
 * @param tile - pointer to a Tile
 */
void BattleItem::setTile(Tile* const tile)
{
	_tile = tile;
}

/**
 * Gets the item's id.
 * @return, the item's id
 */
int BattleItem::getId() const
{
	return _id;
}

/**
 * Gets a corpse's unit.
 * @return, pointer to BattleUnit
 */
BattleUnit* BattleItem::getUnit() const
{
	return _unit;
}

/**
 * Sets the corpse's unit.
 * @param unit - pointer to BattleUnit
 */
void BattleItem::setUnit(BattleUnit* unit)
{
	_unit = unit;
}

/**
 * Sets the heal quantity of the item.
 * @param heal - the new heal quantity
 */
void BattleItem::setHealQuantity(int heal)
{
	_heal = heal;
}

/**
 * Gets the heal quantity of the item.
 * @return, the new heal quantity
 */
int BattleItem::getHealQuantity() const
{
	return _heal;
}

/**
 * Sets the pain killer quantity of the item.
 * @param pk - the new pain killer quantity
 */
void BattleItem::setPainKillerQuantity(int pk)
{
	_painKiller = pk;
}

/**
 * Gets the pain killer quantity of the item.
 * @return, the new pain killer quantity
 */
int BattleItem::getPainKillerQuantity() const
{
	return _painKiller;
}

/**
 * Sets the stimulant quantity of the item.
 * @param stimulant - the new stimulant quantity
 */
void BattleItem::setStimulantQuantity(int stimulant)
{
	_stimulant = stimulant;
}

/**
 * Gets the stimulant quantity of the item.
 * @return, the new stimulant quantity
 */
int BattleItem::getStimulantQuantity() const
{
	return _stimulant;
}

/**
 * Converts a carried unconscious body into a battlefield corpse-item.
 * @param itRule - pointer to rules of the corpse item to convert this item into
 */
void BattleItem::convertToCorpse(RuleItem* const itRule)
{
	if (_unit != nullptr
		&& _itRule->getBattleType() == BT_CORPSE
		&& itRule->getBattleType() == BT_CORPSE)
	{
		_itRule = itRule;
	}
}

}
