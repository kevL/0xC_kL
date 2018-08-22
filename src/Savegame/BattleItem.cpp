/*
 * Copyright 2010-2018 OpenXcom Developers.
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
 * @param pId		- pointer to an integer ID for this item (has precedence over 'id')
 * @param id		- value for ID when loading a saved game (default -1)
 */
BattleItem::BattleItem(
		const RuleItem* const itRule,
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
		_ammoQty(itRule->getFullClip()),
		_fuse(-1),
		_painKiller(0),
		_heal(0),
		_stimulant(0),
		_isLoad(false),
		_xcomProperty(false)
{
	if (pId != nullptr)	// <- this is for SavedBattleGame to keep track
	{					//    of a brand new item on the battlefield
		_id = *pId;
		++(*pId);
	}
	else				// <- load item from saved game
		_id = id;


	switch (_itRule->getBattleType())
	{
		case BT_MEDIKIT:
			_heal       = _itRule->getHealQuantity();
			_painKiller = _itRule->getPainKillerQuantity();
			_stimulant  = _itRule->getStimulantQuantity();
			break;

		case BT_FIREARM: // Firearms w/out defined ammo ARE the ammo.
			if (_itRule->getAcceptedLoadTypes()->empty() == false)
				break; // no break;
		case BT_MELEE: // Melee weapons do NOT require ammo.
			_ammoItem = this;
	}
	// NOTE: lasers, melee, etc. have "clipsize -1" [ie, SetAmmoQuantity(-1)]
	// - needed for melee-item reaction hits, etc. Can be set in Ruleset but do
	// it here. Except that it creates problems w/ TANKS returning to Base. So
	// do it in Ruleset: melee-items need "clipSize -1" to do reactionFire.
	// Unless i changed it .... Actually they need "_ammoItem = this" as above.
	// ... not sure what's up with tanks-returning-to-Base anymore.
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
	_inventoryX   = node["inventoryX"]  .as<int>(_inventoryX);
	_inventoryY   = node["inventoryY"]  .as<int>(_inventoryY);
	_ammoQty      = node["ammoQty"]     .as<int>(_ammoQty);
	_painKiller   = node["painKiller"]  .as<int>(_painKiller);
	_heal         = node["heal"]        .as<int>(_heal);
	_stimulant    = node["stimulant"]   .as<int>(_stimulant);
	_fuse         = node["fuse"]        .as<int>(_fuse);
	_xcomProperty = node["xcomProperty"].as<bool>(_xcomProperty);
}

/**
 * Loads a deleted item from a YAML file.
 */
void BattleItem::loadDeleted()
{
	_xcomProperty = true;
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

	if (_inventoryX != 0) node["inventoryX"] = _inventoryX;
	if (_inventoryY != 0) node["inventoryY"] = _inventoryY;

	if (_ammoQty > 0)          node["ammoQty"]      = _ammoQty;
	if (_painKiller != 0)      node["painKiller"]   = _painKiller;
	if (_heal != 0)            node["heal"]         = _heal;
	if (_stimulant != 0)       node["stimulant"]    = _stimulant;
	if (_fuse != -1)           node["fuse"]         = _fuse;
	if (_xcomProperty == true) node["xcomProperty"] = _xcomProperty;

	if (_owner != nullptr)      node["owner"]    = _owner->getId();
	if (_ownerPre != nullptr
		&& _ownerPre != _owner) node["ownerPre"] = _ownerPre->getId();

	if (_unit != nullptr)     node["unit"]     = _unit->getId();
	if (_section != nullptr)  node["section"]  = _section->getInventoryType(); // NOTE: 'section' should always be valid. Unless it's a loaded Ammo-item.
	if (_tile != nullptr)     node["position"] = _tile->getPosition();
	if (_ammoItem != nullptr) node["ammoItem"] = _ammoItem->getId();

	return node;
}

/**
 * Saves a deleted item to a YAML file.
 * @return, YAML node
 */
YAML::Node BattleItem::saveDeleted() const
{
	YAML::Node node;

	node["id"]   = _id;
	node["type"] = _itRule->getType();

	return node;
}

/**
 * Gets the ruleset for the item's type.
 * @return, pointer to ruleset
 */
const RuleItem* BattleItem::getRules() const
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
 *			0       - item is not ammo
 *			<#>     - ammo qty.
 *			INT_MAX - item is its own ammo
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
 * Sets an ammo-item for this BattleItem.
 * @param load - the ammo-item (default nullptr)
 * @param init - true if called from SavedBattleGame::load() (default false)
 * @return, true if 'item' is valid and gets loaded into the weapon
 */
bool BattleItem::setAmmoItem(
		BattleItem* const load,
		bool init)
{
	if (_ammoItem != this) // ie. if weapon requires a load ...
	{
		if (load == nullptr) // unload weapon ->
		{
			if (_ammoItem != nullptr)
			{
				_ammoItem->_isLoad = false;
				_ammoItem = nullptr;
			}
		}
		else if (_ammoItem == nullptr)
		{
			for (std::vector<std::string>::const_iterator
					i  = _itRule->getAcceptedLoadTypes()->begin();
					i != _itRule->getAcceptedLoadTypes()->end();
					++i)
			{
				if (*i == load->getRules()->getType()) // load weapon ->
				{
					_ammoItem = load;
					_ammoItem->_isLoad = true;
					_ammoItem->_inventoryX =
					_ammoItem->_inventoryY = 0;
					if (init == false)
					{
						_ammoItem->changeOwner();
						_ammoItem->setInventorySection();
					}
					return true;
				}
			}
		}
	}
	return false;
}

/**
 * Checks if this BattleItem has unlimited shots.
 * @note No ammo is needed if a firearm or melee-weapon has itself assigned as
 * its own ammo -- see assignment of '_ammoItem' in cTor.
 * @return, true if self-powered
 */
bool BattleItem::selfPowered() const
{
	return _ammoItem == this;
}

/**
 * Checks if this BattleItem expends itself after its last shot.
 * @note A rocket-propelled-grenade for example.
 * @return, true if self-expended
 */
bool BattleItem::selfExpended() const
{
	return _itRule->getBattleType() == BT_FIREARM
		&& _itRule->getAcceptedLoadTypes()->empty() == true
		&& _itRule->getFullClip() > 0;
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
	if (_ammoQty != -1 && --_ammoQty == 0) // -1== infinite
	{
		weapon.setAmmoItem();
		battleSave.sendItemToDelete(this);
	}
}

/**
 * Removes this BattleItem from a previous owner if applicable and moves it to
 * another owner if applicable.
 * @param unit - pointer to a BattleUnit (default nullptr)
 */
void BattleItem::changeOwner(BattleUnit* const unit)
{
	if (_owner != nullptr)
	{
		for (std::vector<BattleItem*>::const_iterator
				i  = _owner->getInventory()->begin();
				i != _owner->getInventory()->end();
				++i)
		{
			if (*i == this)
			{
				_owner->getInventory()->erase(i);
				break;
			}
		}

		_ownerPre = _owner;
	}
	else
		_ownerPre = unit;

	if ((_owner = unit) != nullptr)
		_owner->getInventory()->push_back(this);
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
 * Gets this BattleItem's owner.
 * @return, pointer to a BattleUnit
 */
BattleUnit* BattleItem::getOwner() const
{
	return _owner;
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
 * Gets this BattleItem's prior owner.
 * @return, pointer to a BattleUnit
 */
BattleUnit* BattleItem::getPriorOwner() const
{
	return _ownerPre;
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
 * Gets the item's inventory x-position.
 * @return, x-position
 */
int BattleItem::getSlotX() const
{
	return _inventoryX;
}

/**
 * Sets the item's inventory x-position.
 * @param x - x-position
 */
void BattleItem::setSlotX(int x)
{
	_inventoryX = x;
}

/**
 * Gets the item's inventory y-position.
 * @return, y-position
 */
int BattleItem::getSlotY() const
{
	return _inventoryY;
}

/**
 * Sets the item's inventory y-position.
 * @param y - y-position
 */
void BattleItem::setSlotY(int y)
{
	_inventoryY = y;
}

/**
 * Checks if an item occupies x-y inventory slot(s).
 * @param x		- slot x-position
 * @param y 	- slot y-position
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
 * Gets this BattleItem's Tile.
 * @return, pointer to the tile
 */
Tile* BattleItem::getItemTile() const
{
	return _tile;
}

/**
 * Sets this BattleItem's Tile.
 * @param tile - pointer to a tile (default nullptr)
 */
void BattleItem::setItemTile(Tile* const tile)
{
	_tile = tile;
}

/**
 * Gets this BattleItem's ID.
 * @return, the ID
 */
int BattleItem::getId() const
{
	return _id;
}

/**
 * Gets the BattleUnit that this BattleItem is a corpse of if any.
 * @return, pointer to BattleUnit or nullptr
 */
BattleUnit* BattleItem::getBodyUnit() const
{
	return _unit;
}

/**
 * Sets the BattleUnit that this BattleItem is a corpse of.
 * @param unit - pointer to BattleUnit
 */
void BattleItem::setItemUnit(BattleUnit* unit)
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
 * @param itRule - pointer to rules of the corpse-item to convert this item into
 *
void BattleItem::changeRule(const RuleItem* const itRule)
{
	if (_unit != nullptr
		&& _itRule->getBattleType() == BT_CORPSE
		&& itRule->getBattleType() == BT_CORPSE)
	{
		_itRule = itRule;
	}
} */

/**
 * Sets this BattleItem as belonging to xCom.
 */
void BattleItem::setProperty()
{
	_xcomProperty = true;
}

/**
 * Gets if this BattleItem belongs to xCom.
 * @return, true if xcom property
 */
bool BattleItem::isProperty() const
{
	return _xcomProperty;
}

}
