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
		_unit(nullptr),
		_tile(nullptr),
		_section(nullptr),
		_x(0),
		_y(0),
		_load(nullptr),
		_rounds(itRule->getFullClip()),
		_fuse(-1),
		_morphine(0),
		_heal(0),
		_stimulant(0),
		_property(false)
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
			_heal      = _itRule->getHealQuantity();
			_morphine  = _itRule->getPainKillerQuantity();
			_stimulant = _itRule->getStimulantQuantity();
			break;

		case BT_FIREARM: // Firearms w/out defined ammo ARE the ammo.
			if (_itRule->getClipTypes()->empty() == false)
				break;
			// no break;
		case BT_MELEE: // Melee weapons do NOT require ammo.
			_load = this;
	}
	// NOTE: lasers, melee, etc. have "clipsize -1" [ie, SetAmmoQuantity(-1)]
	// - needed for melee-item reaction hits, etc. Can be set in Ruleset but do
	// it here. Except that it creates problems w/ TANKS returning to Base. So
	// do it in Ruleset: melee-items need "clipSize -1" to do reactionFire.
	// Unless i changed it .... Actually they need "_ammoItem = this" as above.
	// ... not sure what's up with tanks-returning-to-Base anymore. Anyway,
	// somebody really jacked off all over ammo handling ......
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
	_x = node["x"].as<int>(_x);
	_y = node["y"].as<int>(_y);

	_rounds    = node["rounds"]   .as<int>(_rounds);
	_morphine  = node["morphine"] .as<int>(_morphine);
	_heal      = node["heal"]     .as<int>(_heal);
	_stimulant = node["stimulant"].as<int>(_stimulant);
	_fuse      = node["fuse"]     .as<int>(_fuse);
	_property  = node["property"] .as<bool>(_property);
}

/**
 * Loads a deleted item from a YAML file.
 */
void BattleItem::loadDeleted()
{
	_property = true;
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

	if (_x != 0) node["x"] = _x;
	if (_y != 0) node["y"] = _y;

	if (_rounds > 0)         node["rounds"]    = _rounds;
	if (_morphine != 0)      node["morphine"]  = _morphine;
	if (_heal != 0)          node["heal"]      = _heal;
	if (_stimulant != 0)     node["stimulant"] = _stimulant;
	if (_fuse != -1)         node["fuse"]      = _fuse;
	if (_property == true)   node["property"]  = _property;
	if (_owner != nullptr)   node["owner"]     = _owner->getId();
	if (_unit != nullptr)    node["unit"]      = _unit->getId();
	if (_section != nullptr) node["section"]   = _section->getInventoryType(); // NOTE: 'section' should always be valid. Unless it's a loaded Ammo-item.
	if (_tile != nullptr)    node["position"]  = _tile->getPosition();
	if (_load != nullptr)    node["load"]      = _load->getId();

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
 * Gets the rounds of this BattleItem.
 * @return, ammo quantity
 *			0       - item is not ammo
 *			<#>     - ammo qty.
 *			INT_MAX - item is its own ammo
 */
int BattleItem::getClipRounds() const
{
	if (_rounds == -1)
		return std::numeric_limits<int>::max();

	return _rounds;
}

/**
 * Sets the rounds of this BattleItem.
 * @param qty - ammo quantity
 */
void BattleItem::setClipRounds(int qty)
{
	_rounds = qty;
}

/**
 * Gets this BattleItem's currently loaded ammo-item.
 * @return, pointer to BattleItem or nullptr
 *			- nullptr if this BattleItem has no ammo loaded OR is a clip itself
 *			- the loaded ammo-item OR the weapon itself if weapon is its own ammo
 */
BattleItem* BattleItem::getClip() const
{
	return _load;
}

/**
 * Sets an ammo-item for this BattleItem.
 * @param load - the ammo-item (default nullptr)
 * @param init - true if called from SavedBattleGame::load() (default false)
 * @return, true if 'item' is valid and gets loaded into the weapon
 */
bool BattleItem::setClip(
		BattleItem* const load,
		bool init)
{
	if (_load != this) // ie. if weapon requires a load ...
	{
		if (load == nullptr) // unload weapon ->
			_load = nullptr;
		else if (_load == nullptr)
		{
			for (std::vector<std::string>::const_iterator
					i  = _itRule->getClipTypes()->begin();
					i != _itRule->getClipTypes()->end();
					++i)
			{
				if (*i == load->getRules()->getType()) // load weapon ->
				{
					_load = load;
					_load->_x =
					_load->_y = 0;
					if (init == false)
					{
						_load->changeOwner();
						_load->setInventorySection();
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
 * its own ammo - see assignment of '_ammoItem' in cTor.
 * @return, true if self-powered
 */
bool BattleItem::selfPowered() const
{
	return (_load == this);
}

/**
 * Checks if this BattleItem expends itself after its last shot.
 * @note A rocket-propelled-grenade for example.
 * @return, true if self-expended
 */
bool BattleItem::selfExpended() const
{
	return _itRule->getBattleType() == BT_FIREARM
		&& _itRule->getClipTypes()->empty() == true
		&& _itRule->getFullClip() > 0;
}

/**
 * Expends rounds from this BattleItem.
 * @param battleSave	- reference to the SavedBattleGame
 * @param weapon		- reference to the weapon containing this ammo
 * @param rounds		- quantity of rounds to expend (default 1)
 */
void BattleItem::expendRounds(
		SavedBattleGame& battleSave,
		BattleItem& weapon,
		int rounds)
{
	if (_rounds != -1 && (_rounds -= rounds) == 0) // -1== infinite
	{
		weapon.setClip();
		battleSave.sendItemToDelete(this);
	}
}

/**
 * Either (a) puts this BattleItem in a specified unit's inventory or (b)
 * removes it from its current owner's inventory.
 * IMPORTANT: The owner of an item is either carrying the item OR is its
 * last possessor; that is ownership is not cleared when the item is thrown or
 * dropped (to track responsibility for grenade-explosions). Ownership changes
 * only when an item is acquired. Check the '_tile' pointer for NULL to
 * determine if an item is actually carried (or cycle through unit-inventory
 * searching for the item).
 * NOTE: Loads do not have an owner or a tile.
 * @param unit - pointer to a BattleUnit (default nullptr)
 */
void BattleItem::changeOwner(BattleUnit* const unit)
{
	if (unit != nullptr)
	{
		_owner = unit;
		_owner->getInventory()->push_back(this);
	}
	else if (_owner != nullptr)
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
	}
}

/**
 * Sets this BattleItem's owner.
 * @param unit - pointer to BattleUnit (default nullptr)
 */
void BattleItem::setOwner(BattleUnit* const unit)
{
	_owner = unit;
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
	return _x;
}

/**
 * Sets the item's inventory x-position.
 * @param x - x-position
 */
void BattleItem::setSlotX(int x)
{
	_x = x;
}

/**
 * Gets the item's inventory y-position.
 * @return, y-position
 */
int BattleItem::getSlotY() const
{
	return _y;
}

/**
 * Sets the item's inventory y-position.
 * @param y - y-position
 */
void BattleItem::setSlotY(int y)
{
	_y = y;
}

/**
 * Checks if an item occupies x/y inventory slot(s).
 * @param x		- slot x-position
 * @param y 	- slot y-position
 * @param it	- pointer to an item to check to place (default nullptr)
 * @return, true if item occupies x/y slot
 */
bool BattleItem::occupiesSlot(
		int x,
		int y,
		const BattleItem* const it) const
{
	if (it == this)
		return false;

	if (_section->getCategory() == IC_HAND)
		return true;

	if (it == nullptr)
		return (   x >= _x
				&& x <  _x + _itRule->getInventoryWidth()
				&& y >= _y
				&& y <  _y + _itRule->getInventoryHeight());

	return (	x + it->getRules()->getInventoryWidth() > _x
			 && x < _x + _itRule->getInventoryWidth()
			 && y + it->getRules()->getInventoryHeight() > _y
			 && y < _y + _itRule->getInventoryHeight());
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
Tile* BattleItem::getTile() const
{
	return _tile;
}

/**
 * Sets this BattleItem's Tile.
 * @param tile - pointer to a tile (default nullptr)
 */
void BattleItem::setTile(Tile* const tile)
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
void BattleItem::setBodyUnit(BattleUnit* unit)
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
	_morphine = pk;
}

/**
 * Gets the pain killer quantity of the item.
 * @return, the new pain killer quantity
 */
int BattleItem::getPainKillerQuantity() const
{
	return _morphine;
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
	_property = true;
}

/**
 * Gets if this BattleItem belongs to xCom.
 * @return, true if xcom property
 */
bool BattleItem::isProperty() const
{
	return _property;
}

}
