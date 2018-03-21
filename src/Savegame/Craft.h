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
	static const int TAKEOFF_DELAY = 75;

	bool
		_tactical,
		_inDogfight,
		_interceptLanded,
		_lowFuel,
		_showReady,
		_tacticalReturn,
		_warned,
		
		_w1Disabled,
		_w2Disabled;
	int
		_fuel,
		_hull,
		_loadCap,
		_takeOffDelay,

		_kills;

	std::wstring _label;

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
				SavedGame* const playSave,
				bool hasId = false);
		/// Cleans up the Craft.
		~Craft() final;

		/// Loads the Craft from YAML.
		void loadCraft(
				const YAML::Node& node,
				const Ruleset* const rules);
		/// Saves the Craft to YAML.
		YAML::Node save() const override;
		/// Saves the Craft's identificator to YAML.
		YAML::Node saveIdentificator() const override;
		/// Loads the Craft's identificator from YAML.
		static CraftId loadIdentificator(const YAML::Node& node);

		/// Gets the Craft's ID.
		CraftId getIdentificator() const;

		/// Gets the Craft's ruleset.
		RuleCraft* getRules() const;
		/// Sets the Craft's ruleset.
		void changeRules(RuleCraft* const crRule);

		/// Gets the Craft's label.
		std::wstring getLabel(
				const Language* const lang,
				bool id = true) const override;
		/// Sets the Craft's label.
		void setLabel(const std::wstring& label);

		/// Gets the Craft's globe-marker.
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
		/// Gets the Craft's altitude as an integer.
		unsigned getAltitudeInt() const;
		/// Gets the Craft's heading as an integer.
		unsigned getHeadingInt() const;

		/// Sets the Craft's destination-target.
		void setTarget(Target* const target = nullptr) override;

		/// Gets the Craft's amount of weapons.
		size_t getQtyWeapons() const;
		/// Gets the Craft's amount of soldiers.
		int getQtySoldiers() const;
		/// Gets the Craft's amount of equipment.
		int getQtyEquipment() const;
		/// Gets the Craft's amount of vehicles.
		int getQtyVehicles(bool tiles = false) const;

		/// Gets the Craft's weapons.
		std::vector<CraftWeapon*>* getCraftWeapons();

		/// Gets the Craft's items.
		ItemContainer* getCraftItems() const;
		/// Gets the Craft's vehicles.
		std::vector<Vehicle*>* getVehicles();

		/// Sets this Craft to full hull.
		void setCraftHullFull();
		/// Sets the Craft's hull after inflicted hurt.
		void setCraftHull(int inflict);
		/// Gets the Craft's hull.
		int getCraftHull() const;
		/// Gets the Craft's hull-percentage.
		int getCraftHullPct() const;

		/// Checks if the Craft is destroyed during dogfights.
		bool isDestroyed() const;

		/// Sets the Craft's quantity of fuel.
		void setFuel(int fuel);
		/// Gets the Craft's quantity of fuel.
		int getFuel() const;
		/// Gets the Craft's percentage of fuel.
		int getFuelPct() const;
		/// Uses the Craft's fuel.
		bool useFuel();
		/// Gets the distance that the Craft needs to reserve fuel for to return to its Base.
		double getDistanceReserved(const Target* const target) const;
		/// Gets the distance that the Craft can travel with its current fuel.
		double getDistanceLeft() const;
//		double getDistanceLeft(bool select = false) const;
		/// Checks if the Craft is running out of fuel.
		bool isLowFuel() const;

		/// Sends the Craft to its Base.
		void returnToBase();
		/// Sets that the Craft has just finished a tactical.
		void setTacticalReturn();
		/// Checks if the Craft has just finished a tactical.
		bool isTacticalReturn() const;

		/// Handles Craft logic.
		void think();

		/// Gets if the Craft has left the ground.
		bool hasLeftGround() const;

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

		/// Gets the Craft's capacity load.
		int getLoadCapacity() const;
		/// Gets the Craft's current load.
		int calcLoadCurrent();

		/// Gets the Craft's current CraftWarning status.
		CraftWarning getWarning() const;
		/// Sets the Craft's CraftWarning status.
		void setWarning(const CraftWarning warning);
		/// Gets whether a warning has been issued for this Craft.
		bool getWarned() const;
		/// Sets whether a warning has been issued for this Craft.
		void setWarned(bool warned = true);

		/// Gets the amount of time this Craft will be repairing/rearming/refueling.
		int getDowntime(bool& isDelayed);

		/// Adds a dogfight kill.
		void addKill();
		/// Gets this Craft's dogfight kills.
		int getKills() const;

		/// Transfers soldiers, tanks, items, and weapons to its Base.
		void unloadCraft(
				const Ruleset* const rules,
				bool updateCraft = true);

		/// Sets the Craft as intercepting a land-site.
		void interceptLanded(bool intercept);
		/// Gets if the Craft is intercepting a land-site.
		bool interceptLanded() const;

		/// Gets the Craft's cost for tactical.
		int getOperationalExpense() const;

		/// Sets a craft-weapon disabled or enabled on the Craft.
		void setWeaponDisabled(int hardpoint, bool disabled);
		/// Gets if a craft-weapon is disabled or enabled on the Craft.
		bool getWeaponDisabled(int hardpoint) const;
};

}

#endif
