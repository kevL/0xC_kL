/*
 * Copyright 2010-2017 OpenXcom Developers.
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

#ifndef OPENXCOM_RULECRAFT_H
#define OPENXCOM_RULECRAFT_H

//#include <string>
//#include <vector>

#include <yaml-cpp/yaml.h>


namespace OpenXcom
{

enum CraftStatus
{
	CS_READY,		// 0
	CS_REFUELLING,	// 1
	CS_REARMING,	// 2
	CS_REPAIRS,		// 3
	CS_OUT			// 4
};

class Ruleset;
class RuleTerrain;


/**
 * Represents a specific type of Craft.
 * @note Contains constant info about a craft-type like costs, speed, capacities,
 * consumptions, etc.
 * @sa Craft
 */
class RuleCraft
{

private:
	bool _spacecraft;
	int
		_costBuy,
		_costRent,
		_costSell,
		_listOrder,
		_marker,
		_rangeRadar,
		_rangeRecon,
		_refuelRate,
		_repairRate,
		_score,
		_sprite,
		_transferTime,

		_accel,
		_fuel,
		_hull,
		_speed,

		_items,
		_soldiers,
		_vehicles;
	size_t _weapons;

	std::string
		_refuelItem,
		_type;

	RuleTerrain* _terrainRule;

	std::vector<std::vector<int>> _unitLocations;
	std::vector<std::string> _reqResearch;


	public:
		/// Creates a rule for a specified craft-type.
		explicit RuleCraft(const std::string& type);
		/// Cleans up the rule.
		~RuleCraft();

		/// Loads the rule from YAML.
		void load(
				const YAML::Node& node,
				Ruleset* const rules,
				int modIndex,
				int nextCraftIndex);

		/// Gets the rule's craft-type.
		const std::string& getType() const;

		/// Gets the craft-type's required-research.
		const std::vector<std::string>& getRequiredResearch() const;

		/// Gets the craft-type's sprite.
		int getSprite() const;
		/// Gets the craft-type's globe-marker.
		int getMarker() const;

		/// Gets the craft-type's maximum fuel.
		int getFuelCapacity() const;
		/// Gets the craft-type's maximum damage.
		int getCraftHullCap() const;
		/// Gets the craft-type's maximum speed.
		int getTopSpeed() const;
		/// Gets the craft-type's acceleration.
		int getAcceleration() const;

		/// Gets the craft-type's weapon capacity.
		size_t getWeaponCapacity() const;
		/// Gets the craft-type's soldier capacity.
		int getSoldierCapacity() const;
		/// Gets the craft-type's vehicle capacity.
		int getVehicleCapacity() const;
		/// Gets the craft-type's item capacity.
		int getItemCapacity() const;

		/// Gets the craft-type's cost.
		int getBuyCost() const;
		/// Gets the craft-type's rent for a month.
		int getRentCost() const;
		/// Gets the craft-type's value.
		int getSellCost() const;

		/// Gets the craft-type's refuel item.
		std::string getRefuelItem() const;

		/// Gets the craft-type's repair rate.
		int getRepairRate() const;
		/// Gets the craft-type's refuel rate.
		int getRefuelRate() const;
		/// Gets the craft-type's radar range.
		int getRangeRadar() const;
		/// Gets the craft-type's sight range.
		int getRangeRecon() const;
		/// Gets the craft-type's transfer time.
		int getTransferTime() const;

		/// Gets the craft-type's score.
		int getScore() const;

		/// Gets the craft-type's terrain-data.
		RuleTerrain* getTacticalTerrain();

		/// Checks if the craft-type is capable of travelling to Mars.
		bool getSpacecraft() const;

		/// Gets the list-weight for the craft-type.
		int getListOrder() const;

		/// Gets the unit-locations for the craft-type.
		std::vector<std::vector<int>>& getUnitLocations();
};

}

#endif
