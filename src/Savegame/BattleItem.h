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
 * Represents a BattleItem in tactical.
 */
class BattleItem
{

private:
	bool
		_isLoad,
		_xcomProperty;
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
	const RuleItem* _itRule;
	Tile* _tile;


	public:
		/// Creates a BattleItem with specified rules.
		BattleItem(
				const RuleItem* const itRule,
				int* pId,
				int id = -1);
		/// Cleans up the BattleItem.
		~BattleItem();

		/// Loads the BattleItem from YAML.
		void load(const YAML::Node& node);
		/// Loads a deleted BattleItem from YAML.
		void loadDeleted();
		/// Saves the BattleItem to YAML.
		YAML::Node save() const;
		/// Saves a deleted BattleItem to YAML.
		YAML::Node saveDeleted() const;

		/// Gets the BattleItem's rules.
		const RuleItem* getRules() const;

		/// Gets the BattleItem's ammo quantity
		int getAmmoQuantity() const;
		/// Sets the BattleItem's ammo quantity.
		void setAmmoQuantity(int qty);
		/// Gets if the BattleItem is a clip in a weapon.
		bool isLoad() const;

		/// Gets the BattleItem's ammo BattleItem.
		BattleItem* getAmmoItem() const;
		/// Sets the BattleItem's ammo BattleItem.
		bool setAmmoItem(
				BattleItem* const load = nullptr,
				bool init = false);
		/// Checks if the BattleItem has unlimited shots.
		bool selfPowered() const;
		/// Checks if the BattleItem expends itself after its last shot.
		bool selfExpended() const;
		/// Spends a bullet from this BattleItem.
		void spendBullet(
				SavedBattleGame& battleSave,
				BattleItem& weapon);

		/// Gets turns until the BattleItem explodes.
		int getFuse() const;
		/// Sets turns until the BattleItem explodes.
		void setFuse(int turn);

		/// Sets the BattleItem's owner.
		void setOwner(BattleUnit* const owner = nullptr);
		/// Gets the BattleItem's owner.
		BattleUnit* getOwner() const;
		/// Sets the BattleItem's previous owner.
		void setPriorOwner(BattleUnit* const ownerPre);
		/// Gets the BattleItem's previous owner.
		BattleUnit* getPriorOwner() const;
		/// Clears the BattleItem from its previous owner and gives it to a different BattleUnit.
		void changeOwner(BattleUnit* const unit = nullptr);

		/// Gets the BattleItem's current Inventory section.
		const RuleInventory* getInventorySection() const;
		/// Sets the BattleItem's Inventory section.
		void setInventorySection(const RuleInventory* const inRule = nullptr);
		/// Gets the BattleItem's Inventory x-position.
		int getSlotX() const;
		/// Sets the BattleItem's Inventory x-position.
		void setSlotX(int x);
		/// Gets the BattleItem's Inventory y-position.
		int getSlotY() const;
		/// Sets the BattleItem's Inventory y-position.
		void setSlotY(int y);
		/// Checks if the BattleItem is occupying a certain slot.
		bool occupiesSlot(
				int x,
				int y,
				const BattleItem* const item = nullptr) const;

		/// Gets the BattleItem's tile.
		Tile* getItemTile() const;
		/// Sets the BattleItem's tile.
		void setItemTile(Tile* const tile = nullptr);

		/// Gets the BattleItem's unique id.
		int getId() const;

		/// Gets the BattleItem's corpse-unit if any.
		BattleUnit* getBodyUnit() const;
		/// Sets the BattleItem's corpse-unit.
		void setItemUnit(BattleUnit* unit);

		/// Sets the BattleItem's medikit-heal quantity.
		void setHealQuantity(int heal);
		/// Gets the BattleItem's medikit-heal quantity.
		int getHealQuantity() const;
		/// Sets the BattleItem's medikit-morphine quantity.
		void setPainKillerQuantity(int pk);
		/// Gets the BattleItem's medikit-morphine quantity.
		int getPainKillerQuantity() const;
		/// Sets the BattleItem's medikit-amphetamine quantity.
		void setStimulantQuantity(int stimulant);
		/// Gets the BattleItem's medikit-amphetamine quantity.
		int getStimulantQuantity() const;

		/// Sets a RuleItem for the BattleItem.
//		void changeRule(const RuleItem* const itRule);

		/// Sets the BattleItem as belonging to xCom.
		void setProperty();
		/// Gets if the BattleItem belongs to xCom.
		bool isProperty() const;
};

}

#endif
