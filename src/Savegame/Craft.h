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

#ifndef OPENXCOM_CRAFT_H
#define OPENXCOM_CRAFT_H

//#include <string>
//#include <vector>

#include "MovingTarget.h"

#include "../Ruleset/RuleCraft.h"


namespace OpenXcom
{

enum CraftWarning
{
	CW_NONE,		// 0
	CW_CANTREPAIR,	// 1
	CW_CANTREARM,	// 2
	CW_CANTREFUEL	// 3
};


class Base;
class CraftWeapon;
class ItemContainer;
class RuleCraft;
class Ruleset;
class SavedGame;
class Soldier;
class Vehicle;


/**
 * Represents a Craft stored in a Base.
 * @note Contains variable info about a Craft like position, fuel, damage, etc.
 * @sa RuleCraft
 */
class Craft final
	:
		public MovingTarget
{

private:
	bool
		_tactical,
		_inDogfight,
		_lowFuel,
		_showReady,
		_tacticalDone,
		_warned;
	int
		_loadCap,
		_loadCur,
		_damage,
		_fuel,
		_id,
		_takeOff,

		_kills;

	std::wstring _name;

	CraftStatus _status;
	CraftWarning _warning;

	Base* _base;
	ItemContainer* _items;
	RuleCraft* _crRule;

	std::vector<CraftWeapon*> _weapons;
	std::vector<Vehicle*> _vehicles;

	/// Calculates the Craft's minimum fuel required to go to a base.
//	int calcFuelLimit(const Base* const base) const;


	public:
		/// Creates a Craft of the specified type.
		Craft(
				RuleCraft* const crRule,
				Base* const base,
				int id = 0);
		/// Cleans up the Craft.
		~Craft() final;

		/// Loads the Craft from YAML.
		using MovingTarget::load;
		void load(
				const YAML::Node& node,
				const Ruleset* const rules,
				SavedGame* const gameSave);
		/// Saves the Craft to YAML.
		YAML::Node save() const override;
		/// Saves the Craft's ID to YAML.
		YAML::Node saveId() const override;
		/// Loads the Craft's ID from YAML.
		static CraftId loadId(const YAML::Node& node);
		/// Gets the Craft's unique-ID.
		CraftId getUniqueId() const;
		/// Gets the Craft's ID.
		int getId() const;

		/// Gets the Craft's ruleset.
		RuleCraft* getRules() const;
		/// Sets the Craft's ruleset.
		void changeRules(RuleCraft* const crRule);

		/// Gets the Craft's name.
		std::wstring getName(const Language* const lang) const override;
		/// Sets the Craft's name.
		void setName(const std::wstring& wst);

		/// Gets the Craft's marker.
		int getMarker() const override;

		/// Gets the Craft's base.
		Base* getBase() const;
		/// Sets the Craft's base.
		void setBase(
				Base* const base,
				bool transfer = true);

		/// Sets the Craft's status.
		void setCraftStatus(const CraftStatus status);
		/// Gets the Craft's status.
		CraftStatus getCraftStatus() const;
		/// Gets the Craft's status-string.
		std::string getCraftStatusString() const;

		/// Gets the Craft's altitude.
		std::string getAltitude() const;

		/// Sets the Craft's destination.
		void setDestination(Target* const dest) override;

		/// Gets the Craft's amount of weapons.
		int getQtyWeapons() const;
		/// Gets the Craft's amount of soldiers.
		int getQtySoldiers() const;
		/// Gets the Craft's amount of equipment.
		int getQtyEquipment() const;
		/// Gets the Craft's amount of vehicles.
		int getQtyVehicles(bool quadrants = false) const;

		/// Gets the Craft's weapons.
		std::vector<CraftWeapon*>* getWeapons();

		/// Gets the Craft's items.
		ItemContainer* getCraftItems() const;
		/// Gets the Craft's vehicles.
		std::vector<Vehicle*>* getVehicles();

		/// Gets the Craft's amount of damage.
		int getCraftDamage() const;
		/// Sets the Craft's amount of damage.
		void setCraftDamage(const int damage);
		/// Gets the Craft's percentage of damage.
		int getCraftDamagePct() const;

		/// Gets the Craft's amount of fuel.
		int getFuel() const;
		/// Sets the Craft's amount of fuel.
		void setFuel(int fuel);
		/// Gets the Craft's percentage of fuel.
		int getFuelPct() const;
		/// Gets whether the Craft is running out of fuel.
		bool getLowFuel() const;
		/// Sets whether the Craft is running out of fuel.
		void setLowFuel(bool low = true);
		/// Consumes the Craft's fuel.
		void consumeFuel();
		/// Gets the Craft's fuel consumption.
		int getFuelConsumption() const;
		/// Gets the Craft's minimum fuel limit.
		int getFuelLimit() const;

		/// Returns the Craft to its base.
		void returnToBase();
		/// Gets whether the Craft has just finished a mission.
		bool getTacticalReturn() const;
		/// Sets that the Craft has just finished a mission.
		void setTacticalReturn();

		/// Handles Craft logic.
		void think();

		/// Does a full Craft checkup.
		void checkup();
		/// Flag to notify player that the Craft is ready.
		bool showReady();

		/// Repairs the Craft.
		void repair();
		/// Rearms the Craft.
		std::string rearm(const Ruleset* const rules);
		/// Refuels the Craft.
		void refuel();

		/// Checks if a target is detected by the Craft's radar.
		bool detect(const Target* const target) const;

		/// Sets if the Craft is participating in a tactical mission.
		void setTactical(bool tactical = true);
		/// Gets if the Craft is participating in a tactical mission.
		bool getTactical() const;

		/// Gets if Craft is destroyed during dogfights.
		bool isDestroyed() const;

		/// Gets the amount of space available inside Craft.
		int getSpaceAvailable() const;
		/// Gets the amount of space used inside Craft.
		int getSpaceUsed() const;
		/// Gets the Craft's vehicles of a certain type.
		int getVehicleCount(const std::string& vehicle) const;

		/// Sets if the Craft is in a dogfight.
		void inDogfight(bool dogfight);
		/// Gets if the Craft is in a dogfight.
		bool inDogfight() const;

		/// Sets the Craft's capacity load.
		void setLoadCapacity(const int load);
		/// Gets the Craft's capacity load.
		int getLoadCapacity() const;
		/// Sets the Craft's current load.
//		void setLoadCurrent(const int load);
		/// Gets the Craft's current load.
		int calcLoadCurrent();

		/// Gets the Craft's current CraftWarning status.
		CraftWarning getWarning() const;
		/// Sets the Craft's CraftWarning status.
		void setWarning(const CraftWarning warning);
		/// Gets whether a warning has been issued for this Craft.
		bool getWarned() const;
		/// Sets whether a warning has been issued for this Craft.
		void setWarned(const bool warned = true);

		/// Gets the amount of time this Craft will be repairing/rearming/refueling.
		int getDowntime(bool& isDelayed);

		/// Adds a dogfight kill.
		void addKill();
		/// Gets this Craft's dogfight kills.
		int getKills() const;

		/// Gets if the Craft has left the ground.
		bool getTakeoff() const;

		/// Transfers soldiers, tanks, items, and weapons to its Base.
		void unloadCraft(
				const Ruleset* const rules,
				bool updateCraft = true);
};

}

#endif
