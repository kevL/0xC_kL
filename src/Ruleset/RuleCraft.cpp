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

#include "RuleCraft.h"

#include "RuleTerrain.h"


namespace OpenXcom
{

/**
 * Creates the blank ruleset for a specified type of Craft.
 * @param type - reference to a string that identifies the type
 */
RuleCraft::RuleCraft(const std::string& type)
	:
		_type(type),
		_sprite(-1),
		_marker(-1),
		_fuel(0),	// NOTE: Do not assign "0" else potential div-by-zero.
		_hull(0),	// NOTE: Do not assign "0" else potential div-by-zero.
		_speed(0),
		_accel(0),
		_weapons(0u),
		_soldiers(0),
		_vehicles(0),
		_items(0),
		_costBuy(0),
		_costRent(0),
		_costSell(0),
		_repairRate(1),
		_refuelRate(1),
		_rangeRadar(600),
		_rangeRecon(900),
		_transferTime(0),
		_score(0),
		_terrainRule(nullptr),
		_spacecraft(false),
		_listOrder(0)
{}

/**
 * dTor.
 */
RuleCraft::~RuleCraft()
{
	delete _terrainRule;
}

/**
 * Loads this craft-type from a YAML file.
 * @param node		- reference a YAML node
 * @param rules		- pointer to Ruleset
 * @param modIndex	- a value that offsets the sounds and sprite values to avoid conflicts
 * @param listOrder	- the list weight for this craft
 */
void RuleCraft::load(
		const YAML::Node& node,
		Ruleset* const rules,
		int modIndex,
		int listOrder)
{
	_type			= node["type"]			.as<std::string>(_type);
	_reqResearch	= node["reqResearch"]	.as<std::vector<std::string>>(_reqResearch);

	if (node["sprite"])
	{
		_sprite = node["sprite"].as<int>(_sprite);
		if (_sprite > 4) // this is an offset in BASEBITS.PCK, and two in INTICONS.PCK
			_sprite += modIndex;
	}

	_marker			= node["marker"]		.as<int>(_marker);
	_fuel			= node["fuel"]			.as<int>(_fuel);
	_hull			= node["hull"]			.as<int>(_hull);
	_speed			= node["speed"]			.as<int>(_speed);
	_accel			= node["accel"]			.as<int>(_accel);
	_weapons		= node["weapons"]		.as<size_t>(_weapons);
	_soldiers		= node["soldiers"]		.as<int>(_soldiers);
	_vehicles		= node["vehicles"]		.as<int>(_vehicles);
	_items			= node["items"]			.as<int>(_items);
	_costBuy		= node["costBuy"]		.as<int>(_costBuy);
	_costRent		= node["costRent"]		.as<int>(_costRent);
	_costSell		= node["costSell"]		.as<int>(_costSell);
	_refuelItem		= node["refuelItem"]	.as<std::string>(_refuelItem);
	_repairRate		= node["repairRate"]	.as<int>(_repairRate);
	_refuelRate		= node["refuelRate"]	.as<int>(_refuelRate);
	_rangeRadar		= node["rangeRadar"]	.as<int>(_rangeRadar);
	_rangeRecon		= node["rangeRecon"]	.as<int>(_rangeRecon);
	_transferTime	= node["transferTime"]	.as<int>(_transferTime);
	_score			= node["score"]			.as<int>(_score);
	_spacecraft		= node["spacecraft"]	.as<bool>(_spacecraft);
	_unitLocations	= node["unitLocations"]	.as<std::vector<std::vector<int>>>(_unitLocations);

	if (const YAML::Node& terrain = node["tacticalTerrain"])
	{
		RuleTerrain* const terrainRule (new RuleTerrain(terrain["rule"].as<std::string>()));
		terrainRule->load(terrain, rules);
		_terrainRule = terrainRule;
	}

	_listOrder = node["listOrder"].as<int>(_listOrder);
	if (_listOrder == 0)
		_listOrder = listOrder;
}

/**
 * Gets this craft-type.
 * @return, reference to the craft's type
 */
const std::string& RuleCraft::getType() const
{
	return _type;
}

/**
 * Gets the list of required-research to acquire this craft-type.
 * @return, reference to a vector of strings that lists the needed research IDs
 */
const std::vector<std::string>& RuleCraft::getRequiredResearch() const
{
	return _reqResearch;
}

/**
 * Gets the ID of the sprite used to draw this craft-type in CraftInfoState
 * (BASEBITS) and DogfightState (INTICON).
 * @return, the sprite ID
 */
int RuleCraft::getSprite() const
{
	return _sprite;
}

/**
 * Gets the globe-marker for this craft-type.
 * @return, marker-ID (-1 if not defined in ruleset)
 */
int RuleCraft::getMarker() const
{
	return _marker;
}

/**
 * Gets the maximum fuel this craft-type can contain.
 * @return, the fuel amount
 */
int RuleCraft::getFuelCapacity() const
{
	return _fuel;
}

/**
 * Gets the maximum damage this craft-type can take before it's destroyed.
 * @return, the maximum damage
 */
int RuleCraft::getCraftHullCap() const
{
	return _hull;
}

/**
 * Gets the maximum speed of this craft-type flying around the Geoscape.
 * @return, the top speed in knots
 */
int RuleCraft::getTopSpeed() const
{
	return _speed;
}

/**
 * Gets the acceleration of this craft-type for closing/disengaging UFOs.
 * @return, the acceleration
 */
int RuleCraft::getAcceleration() const
{
	return _accel;
}

/**
 * Gets the maximum number of weapons that can be equipped onto this craft-type.
 * @return, the weapon capacity
 */
size_t RuleCraft::getWeaponCapacity() const
{
	return _weapons;
}

/**
 * Gets the maximum number of soldiers that this craft-type can carry.
 * @return, the soldier capacity
 */
int RuleCraft::getSoldierCapacity() const
{
	return _soldiers;
}

/**
 * Gets the maximum number of vehicles that this craft-type can carry.
 * @return, the vehicle capacity
 */
int RuleCraft::getVehicleCapacity() const
{
	return _vehicles;
}

/**
 * Gets the maximum amount of items this craft-type can carry/store.
 * @return, quantity of items
 */
int RuleCraft::getItemCapacity() const
{
	return _items;
}

/**
 * Gets the cost of this craft-type for purchase/rent (0 if not purchasable).
 * @return, the cost
 */
int RuleCraft::getBuyCost() const
{
	return _costBuy;
}

/**
 * Gets the cost of renting this craft-type for a month.
 * @return, the cost
 */
int RuleCraft::getRentCost() const
{
	return _costRent;
}

/**
 * Gets the sell value of this craft-type -- rented craft should use 0.
 * @return, the sell value
 */
int RuleCraft::getSellCost() const
{
	return _costSell;
}

/**
 * Gets what item is required to refuel this craft-type.
 * @return, the item-type or "" if none
 */
std::string RuleCraft::getRefuelItem() const
{
	return _refuelItem;
}

/**
 * Gets how much damage is removed from this craft-type while repairing.
 * @return, the quantity of damage
 */
int RuleCraft::getRepairRate() const
{
	return _repairRate;
}

/**
 * Gets how much fuel is added to this craft-type while refuelling.
 * @return, the quantity of fuel
 */
int RuleCraft::getRefuelRate() const
{
	return _refuelRate;
}

/**
 * Gets this craft-type's radar range for detecting UFOs.
 * @return, range in nautical miles
 */
int RuleCraft::getRangeRadar() const
{
	return _rangeRadar;
}

/**
 * Gets this craft-type's sight range for detecting bases.
 * @return, range in nautical miles
 */
int RuleCraft::getRangeRecon() const
{
	return _rangeRecon;
}

/**
 * Gets the amount of time this craft-type takes to arrive at Base after
 * purchase.
 * @return, hours
 */
int RuleCraft::getTransferTime() const
{
	return _transferTime;
}

/**
 * Gets the score you lose when this craft-type is destroyed.
 * @return, score
 */
int RuleCraft::getScore() const
{
	return _score;
}

/**
 * Gets the terrain-data needed to draw this craft-type in the battlescape.
 * @return, pointer to RuleTerrain data
 */
RuleTerrain* RuleCraft::getTacticalTerrain()
{
	return _terrainRule;
}

/**
 * Checks if this craft-type is capable of going to Mars.
 * @return, true if ship is space-worthy
 */
bool RuleCraft::getSpacecraft() const
{
	return _spacecraft;
}

/**
 * Gets the list-weight for this craft-type in research-lists.
 * @return, the list-weight
 */
int RuleCraft::getListOrder() const
{
	 return _listOrder;
}

/**
 * Gets the unit-location spawn-points for this craft-type.
 * @return, reference to a vector of vectors of unit-locations
 */
std::vector<std::vector<int>>& RuleCraft::getUnitLocations()
{
	return _unitLocations;
}

}
