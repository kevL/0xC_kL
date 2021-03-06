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
	bool _property;
	int
		_fuse,
		_id,
		_rounds,
		_x,
		_y,

		_heal,
		_morphine,
		_stimulant;

	BattleItem* _clip;
	BattleUnit
		* _owner,
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

		/// Gets the BattleItem's ammo-quantity.
		int getClipRounds() const;
		/// Sets the BattleItem's ammo-quantity.
		void setClipRounds(int qty);

		/// Gets the BattleItem's ammo BattleItem.
		BattleItem* getClip() const;
		/// Sets the BattleItem's ammo BattleItem.
		bool setClip(
				BattleItem* const it = nullptr,
				bool init = false);
		/// Checks if the BattleItem has unlimited shots.
		bool selfPowered() const;
		/// Checks if the BattleItem expends itself after its last shot.
		bool selfExpended() const;
		/// Spends a bullet from this BattleItem.
		void expendRounds(
				SavedBattleGame& battleSave,
				BattleItem& weapon,
				int rounds = 1);

		/// Gets turns until the BattleItem explodes.
		int getFuse() const;
		/// Sets turns until the BattleItem explodes.
		void setFuse(int turn);

		/// Adds or clears the BattleItem from a unit's inventory.
		void changeOwner(
				BattleUnit* const unit = nullptr,
				bool clear = false);
		/// Sets the BattleItem's owner.
		void setOwner(BattleUnit* const unit = nullptr);
		/// Gets the BattleItem's owner.
		BattleUnit* getOwner() const;

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
				const BattleItem* const it = nullptr) const;

		/// Gets the BattleItem's tile.
		Tile* getTile() const;
		/// Sets the BattleItem's tile.
		void setTile(Tile* const tile = nullptr);

		/// Gets the BattleItem's unique id.
		int getId() const;

		/// Gets the BattleItem's corpse-unit if any.
		BattleUnit* getBodyUnit() const;
		/// Sets the BattleItem's corpse-unit.
		void setBodyUnit(BattleUnit* unit);

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
