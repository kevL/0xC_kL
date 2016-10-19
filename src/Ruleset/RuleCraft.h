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
 * @note Contains constant info about a craft like costs speed capacities
 * consumptions etc.
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
		_damageMax,
		_fuelMax,
		_speedMax,

		_items,
		_soldiers,
		_vehicles;
	size_t _weapons;

	std::string
		_refuelItem,
		_type;

	RuleTerrain* _terrainRule;

	std::vector<std::vector<int>> _unitLocations;
	std::vector<std::string> _required;


	public:
		/// Creates a Craft ruleset.
		explicit RuleCraft(const std::string& type);
		/// Cleans up the Craft ruleset.
		~RuleCraft();

		/// Loads Craft ruleset-data from YAML.
		void load(
				const YAML::Node& node,
				Ruleset* const rules,
				int modIndex,
				int nextCraftIndex);

		/// Gets a Craft's type.
		std::string getType() const;

		/// Gets a Craft's requirements.
		const std::vector<std::string>& getRequirements() const;

		/// Gets a Craft's sprite.
		int getSprite() const;
		/// Gets a Craft's globe-marker.
		int getMarker() const;

		/// Gets a Craft's maximum fuel.
		int getMaxFuel() const;
		/// Gets a Craft's maximum damage.
		int getMaxDamage() const;
		/// Gets a Craft's maximum speed.
		int getMaxSpeed() const;
		/// Gets a Craft's acceleration.
		int getAcceleration() const;

		/// Gets a Craft's weapon capacity.
		size_t getWeaponCapacity() const;
		/// Gets a Craft's soldier capacity.
		int getSoldierCapacity() const;
		/// Gets a Craft's vehicle capacity.
		int getVehicleCapacity() const;
		/// Gets a Craft's item capacity.
		int getItemCapacity() const;

		/// Gets a Craft's cost.
		int getBuyCost() const;
		/// Gets a Craft's rent for a month.
		int getRentCost() const;
		/// Gets a Craft's value.
		int getSellCost() const;

		/// Gets a Craft's refuel item.
		std::string getRefuelItem() const;

		/// Gets a Craft's repair rate.
		int getRepairRate() const;
		/// Gets a Craft's refuel rate.
		int getRefuelRate() const;
		/// Gets a Craft's radar range.
		int getRangeRadar() const;
		/// Gets a Craft's sight range.
		int getRangeRecon() const;
		/// Gets a Craft's transfer time.
		int getTransferTime() const;

		/// Gets a Craft's score.
		int getScore() const;

		/// Gets a Craft's terrain data.
		RuleTerrain* getTacticalTerrainData();

		/// Checks if a Craft is capable of travelling to Mars.
		bool getSpacecraft() const;

		/// Gets the list weight for a Craft.
		int getListOrder() const;

		/// Gets the unit-locations for a Craft.
		std::vector<std::vector<int>>& getUnitLocations();
};

}

#endif
