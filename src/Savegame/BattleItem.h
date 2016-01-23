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

#ifndef OPENXCOM_BATTLEITEM_H
#define OPENXCOM_BATTLEITEM_H

#include <yaml-cpp/yaml.h>


namespace OpenXcom
{

class BattleUnit;
class RuleInventory;
class RuleItem;
class SavedBattleGame;
class Tile;


/**
 * Represents a single item in the battlescape.
 */
class BattleItem
{

private:
	bool _isLoad;
	int
		_ammoQty,
		_fuse,
		_id,
		_inventoryX,
		_inventoryY,

		_heal,
		_painKiller,
		_stimulant;

	BattleItem* _ammoItem;
	BattleUnit
		* _owner,
		* _ownerPre,
		* _unit;
	const RuleInventory* _section;
	RuleItem* _itRule;
	Tile* _tile;


	public:
		/// Creates a item of the specified type.
		BattleItem(
				RuleItem* const itRule,
				int* pId,
				int id = -1);
		/// Cleans up the item.
		~BattleItem();

		/// Loads the item from YAML.
		void load(const YAML::Node& node);
		/// Saves the item to YAML.
		YAML::Node save() const;

		/// Gets the item's ruleset.
		RuleItem* getRules() const;

		/// Gets the item's ammo quantity
		int getAmmoQuantity() const;
		/// Sets the item's ammo quantity.
		void setAmmoQuantity(int qty);
		/// Gets if the item is a clip in a weapon.
		bool isLoad() const;

		/// Gets the item's ammo item.
		BattleItem* getAmmoItem() const;
		/// Sets the item's ammo item.
		int setAmmoItem(
				BattleItem* const item = nullptr,
				bool loadSave = false);
		/// Checks if this item uses ammo OR is self-powered.
		bool selfPowered() const;
		/// Spends a bullet from this BattleItem.
		void spendBullet(
				SavedBattleGame& battleSave,
				BattleItem& weapon);

		/// Gets turns until it explodes.
		int getFuse() const;
		/// Sets turns until it explodes.
		void setFuse(int turn);

		/// Gets the item's owner.
		BattleUnit* getOwner() const;
		/// Sets the owner.
		void setOwner(BattleUnit* const owner = nullptr);
		/// Gets the item's previous owner.
		BattleUnit* getPriorOwner() const;
		/// Sets the item's previous owner.
		void setPriorOwner(BattleUnit* const ownerPre);
		/// Removes the item from previous owner and moves to new owner.
		void changeOwner(BattleUnit* const owner = nullptr);

		/// Gets the item's inventory section.
		const RuleInventory* getInventorySection() const;
		/// Sets the item's inventory section.
		void setInventorySection(const RuleInventory* const inRule = nullptr);
		/// Gets the item's inventory X position.
		int getSlotX() const;
		/// Sets the item's inventory X position.
		void setSlotX(int x);
		/// Gets the item's inventory Y position.
		int getSlotY() const;
		/// Sets the item's inventory Y position.
		void setSlotY(int y);
		/// Checks if the item is occupying a slot.
		bool occupiesSlot(
				int x,
				int y,
				const BattleItem* const item = nullptr) const;

		/// Gets the item's tile.
		Tile* getTile() const;
		/// Sets the tile.
		void setTile(Tile* const tile);

		/// Gets it's unique id.
		int getId() const;

		/// Gets the corpse's unit.
		BattleUnit* getUnit() const;
		/// Sets the corpse's unit.
		void setUnit(BattleUnit* unit);

		/// Sets medikit Heal quantity
		void setHealQuantity(int heal);
		/// Gets medikit heal quantity
		int getHealQuantity() const;
		/// Sets medikit pain killers quantity
		void setPainKillerQuantity(int pk);
		/// Gets medikit pain killers quantity
		int getPainKillerQuantity() const;
		/// Sets medikit stimulant quantity
		void setStimulantQuantity(int stimulant);
		/// Gets medikit stimulant quantity
		int getStimulantQuantity() const;

		/// Sets the item's ruleset.
		void convertToCorpse(RuleItem* const itRule);
};

}

#endif
