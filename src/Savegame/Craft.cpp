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

#include "Craft.h"

//#define _USE_MATH_DEFINES
//#include <cmath>

#include "AlienBase.h"
#include "Base.h"
#include "CraftWeapon.h"
#include "ItemContainer.h"
#include "MissionSite.h"
#include "Soldier.h"
#include "Ufo.h"
#include "Vehicle.h"
#include "Waypoint.h"

#include "../Engine/Language.h"
#include "../Engine/Logger.h"

#include "../Geoscape/GeoscapeState.h"
#include "../Geoscape/Globe.h" // Globe::GLM_CRAFT

#include "../Ruleset/RuleArmor.h"
#include "../Ruleset/AlienDeployment.h"
#include "../Ruleset/RuleCraftWeapon.h"
#include "../Ruleset/RuleItem.h"
#include "../Ruleset/Ruleset.h"
#include "../Ruleset/RuleUnit.h"

#include "../Savegame/BaseFacility.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/Transfer.h"


namespace OpenXcom
{

/**
 * Creates a Craft of the specified type and assigns it the latest craft-ID
 * available.
 * @param crRule	- pointer to RuleCraft
 * @param base		- pointer to the Base of origin
 * @param id		- ID to assign to the Craft; 0 for no ID (default 0)
 */
Craft::Craft(
		RuleCraft* const crRule,
		Base* const base,
		int id)
	:
		MovingTarget(),
		_crRule(crRule),
		_base(base),
		_id(id),
		_fuel(0),
		_damage(0),
		_takeOff(0),
		_status(CS_READY),
		_lowFuel(false),
		_tacticalDone(false),
		_tactical(false),
		_inDogfight(false),
		_loadCur(0),
		_warning(CW_NONE),
		_warned(false), // do not save-to-file; ie, re-warn player if reloading
		_kills(0),
		_showReady(false)
{
	_items = new ItemContainer();

	for (int
			i = 0;
			i != _crRule->getWeapons();
			++i)
		_weapons.push_back(nullptr);

	if (_base != nullptr)
		setBase(_base);

	_loadCap = _crRule->getItems() + _crRule->getSoldiers() * 10;
}

/**
 * Deletes this Craft.
 */
Craft::~Craft()
{
	delete _items;

	for (std::vector<CraftWeapon*>::const_iterator
			i = _weapons.begin();
			i != _weapons.end();
			++i)
		delete *i;

	for (std::vector<Vehicle*>::const_iterator
			i = _vehicles.begin();
			i != _vehicles.end();
			++i)
		delete *i;
}

/**
 * Loads this Craft from a YAML file.
 * @param node		- reference a YAML node
 * @param rules		- pointer to the Ruleset
 * @param gameSave	- pointer to the SavedGame
 */
void Craft::load(
		const YAML::Node& node,
		const Ruleset* const rules,
		SavedGame* const gameSave)
{
	MovingTarget::load(node);

	_id			= node["id"]	.as<int>(_id);
	_fuel		= node["fuel"]	.as<int>(_fuel);
	_damage		= node["damage"].as<int>(_damage);

	_warning = static_cast<CraftWarning>(node["warning"].as<int>(_warning));

	std::string type;

	size_t j = 0;
	for (YAML::const_iterator
			i = node["weapons"].begin();
			i != node["weapons"].end();
			++i)
	{
		if (_crRule->getWeapons() > static_cast<int>(j))
		{
			type = (*i)["type"].as<std::string>();
			if (type != "0"
				&& rules->getCraftWeapon(type) != nullptr)
			{
				CraftWeapon* const cw (new CraftWeapon(
													rules->getCraftWeapon(type),
													0));
				cw->load(*i);
				_weapons[j] = cw;
			}
			else
			{
				_weapons[j] = nullptr;
				if (type != "0") Log(LOG_ERROR) << "Failed to load craft weapon " << type;
			}
			++j;
		}
	}

	_items->load(node["items"]);
	for (std::map<std::string, int>::const_iterator
			i = _items->getContents()->begin();
			i != _items->getContents()->end();
			)
	{
		if (rules->getItemRule(i->first) == nullptr)
		{
			Log(LOG_ERROR) << "Failed to load item " << i->first;
			i = _items->getContents()->erase(i);
		}
		else
			++i;
	}

	for (YAML::const_iterator
			i = node["vehicles"].begin();
			i != node["vehicles"].end();
			++i)
	{
		type = (*i)["type"].as<std::string>();
		if (rules->getItemRule(type) != nullptr)
		{
			const int quadrants (rules->getArmor(rules->getUnitRule(type)->getArmor())->getSize());
			Vehicle* const vehicle (new Vehicle(
											rules->getItemRule(type),
											0,
											quadrants * quadrants));
			vehicle->load(*i);
			_vehicles.push_back(vehicle);
		}
		else Log(LOG_ERROR) << "Failed to load item " << type;
	}

	_status = static_cast<CraftStatus>(node["status"].as<int>(_status));

	_lowFuel		= node["lowFuel"]		.as<bool>(_lowFuel);
	_tacticalDone	= node["tacticalDone"]	.as<bool>(_tacticalDone);
	_kills			= node["kills"]			.as<int>(_kills);

	if (const YAML::Node& name = node["name"])
		_name = Language::utf8ToWstr(name.as<std::string>());

	if (const YAML::Node& dest = node["dest"])
	{
		int id (dest["id"].as<int>());

		type = dest["type"].as<std::string>();
		if (type == "STR_BASE")
			returnToBase();
		else if (type == "STR_UFO")
		{
			for (std::vector<Ufo*>::const_iterator
					i = gameSave->getUfos()->begin();
					i != gameSave->getUfos()->end();
					++i)
			{
				if ((*i)->getId() == id)
				{
					setDestination(*i);
					break;
				}
			}
		}
		else if (type == "STR_WAYPOINT")
		{
			for (std::vector<Waypoint*>::const_iterator
					i = gameSave->getWaypoints()->begin();
					i != gameSave->getWaypoints()->end();
					++i)
			{
				if ((*i)->getId() == id)
				{
					setDestination(*i);
					break;
				}
			}
		}
		else if (type == "STR_ALIEN_BASE")
		{
			for (std::vector<AlienBase*>::const_iterator
					i = gameSave->getAlienBases()->begin();
					i != gameSave->getAlienBases()->end();
					++i)
			{
				if ((*i)->getId() == id)
				{
					setDestination(*i);
					break;
				}
			}
		}
		else // type = "STR_TERROR_SITE" (was "STR_ALIEN_TERROR")
		{
			for (std::vector<MissionSite*>::iterator
					i = gameSave->getMissionSites()->begin();
					i != gameSave->getMissionSites()->end();
					++i)
			{
				if ((*i)->getId() == id
					&& (*i)->getSiteDeployment()->getMarkerType() == type) // is this necessary. not for UFO.
				{
					setDestination(*i);
					break;
				}
			}
		}
	}

	_takeOff	= node["takeOff"]	.as<int>(_takeOff);
	_tactical	= node["tactical"]	.as<bool>(_tactical);
	if (_tactical == true)
		setSpeed(0);

	_loadCur = getQtyEquipment() + (getQtySoldiers() + getQtyVehicles(true) * 10); // note: 10 is the 'load' that a single 'space' uses.
}

/**
 * Saves this Craft to a YAML file.
 * @return, YAML node
 */
YAML::Node Craft::save() const
{
	YAML::Node node = MovingTarget::save();

	node["type"] = _crRule->getType();
	node["id"]   = _id;

	for (std::vector<CraftWeapon*>::const_iterator
			i = _weapons.begin();
			i != _weapons.end();
			++i)
	{
		YAML::Node subnode;
		if (*i != nullptr)
			subnode = (*i)->save();
		else
			subnode["type"] = "0";
		node["weapons"].push_back(subnode);
	}

	node["items"] = _items->save();

	for (std::vector<Vehicle*>::const_iterator
			i = _vehicles.begin();
			i != _vehicles.end();
			++i)
	{
		node["vehicles"].push_back((*i)->save());
	}

	node["status"] = static_cast<int>(_status);

	if (_fuel != 0)				node["fuel"]			= _fuel;
	if (_damage != 0)			node["damage"]			= _damage;
	if (_lowFuel == true)		node["lowFuel"]			= _lowFuel;
	if (_tacticalDone == true)	node["tacticalDone"]	= _tacticalDone;
	if (_tactical == true)		node["tactical"]		= _tactical;
	if (_kills != 0)			node["kills"]			= _kills;
	if (_takeOff != 0)			node["takeOff"]			= _takeOff;
	if (_name.empty() == false)	node["name"]			= Language::wstrToUtf8(_name);
	if (_warning != CW_NONE)	node["warning"]			= static_cast<int>(_warning);

	return node;
}

/**
 * Loads this Craft's unique identifier from a YAML file.
 * @param node - reference a YAML node
 * @return, unique craft id
 */
CraftId Craft::loadId(const YAML::Node& node) // static.
{
	return std::make_pair(
						node["type"].as<std::string>(),
						node["id"]	.as<int>());
}

/**
 * Saves this Craft's unique identifiers to a YAML file.
 * @return, YAML node
 */
YAML::Node Craft::saveId() const
{
	YAML::Node node = MovingTarget::saveId();

	const CraftId uniqueId (getUniqueId());

	node["type"] = uniqueId.first;
	node["id"]   = uniqueId.second;

	return node;
}

/**
 * Gets the rules for this Craft's type.
 * @return, pointer to RuleCraft
 */
RuleCraft* Craft::getRules() const
{
	return _crRule;
}

/**
 * Sets the rules for this Craft's type.
 * @param crRule - pointer to a different RuleCraft
 * @warning ONLY FOR NEW BATTLE USE!
 */
void Craft::changeRules(RuleCraft* const crRule)
{
	_crRule = crRule;
	_weapons.clear();

	for (int
			i = 0;
			i != _crRule->getWeapons();
			++i)
		_weapons.push_back(nullptr);
}

/**
 * Gets this Craft's unique ID.
 * @note Each craft can be identified by its type and ID.
 * @return, unique ID
 */
int Craft::getId() const
{
	return _id;
}

/**
 * Gets this Craft's uniquely identifying name.
 * @note If there's not a custom-name the language default is used.
 * @param lang - pointer to a Language to get strings from (default nullptr)
 * @return, full name of craft
 */
std::wstring Craft::getName(const Language* const lang) const
{
	if (_name.empty() == true)
		return lang->getString("STR_CRAFTNAME").arg(lang->getString(_crRule->getType())).arg(_id);

	return _name;
}

/**
 * Sets this Craft's custom-name.
 * @param newName - reference a new custom-name; if blank the language default will be used
 */
void Craft::setName(const std::wstring& wst)
{
	_name = wst;
}

/**
 * Gets the globe-marker for this Craft.
 * @return, marker sprite (-1 if based)
 */
int Craft::getMarker() const
{
	if (_status == CS_OUT)
	{
		const int ret (_crRule->getMarker());
		if (ret != -1) return ret; // for a custom marker.

		return Globe::GLM_CRAFT;
	}
	return -1;
}

/**
 * Gets the Base this Craft belongs to.
 * @return, pointer to a Base
 */
Base* Craft::getBase() const
{
	return _base;
}

/**
 * Sets the Base this Craft belongs to.
 * @param base		- pointer to a Base
 * @param transfer	- true to move Craft to the Base's coordinates (default true)
 */
void Craft::setBase(
		Base* const base,
		bool transfer)
{
	_base = base;

	if (transfer == true)
	{
		_lon = base->getLongitude();
		_lat = base->getLatitude();
	}
}

/**
 * Sets the status of this Craft.
 * @param status - CraftStatus (RuleCraft.h)
 */
void Craft::setCraftStatus(const CraftStatus status)
{
	_status = status;
}

/**
 * Gets the status of this Craft.
 * @return, CraftStatus (RuleCraft.h)
 */
CraftStatus Craft::getCraftStatus() const
{
	return _status;
}

/**
 * Gets the Craft's status as a string.
 * @return, status-string
 */
std::string Craft::getCraftStatusString() const
{
	switch (_status)
	{
		default:
		case CS_READY:
			return "STR_READY";

		case CS_REFUELLING:
			return "STR_REFUELLING";

		case CS_REARMING:
			return "STR_REARMING";

		case CS_REPAIRS:
			return "STR_REPAIRS";

		case CS_OUT:
			return "STR_OUT";
	}
}

/**
 * Gets the altitude of this Craft.
 * @return, altitude-string
 */
std::string Craft::getAltitude() const
{
	if (_dest == nullptr)
		return "STR_LOW_UC";

	const Ufo* const ufo (dynamic_cast<Ufo*>(_dest));
	if (ufo != nullptr)
	{
		if (ufo->getAltitude() == "STR_GROUND")
			return "STR_VERY_LOW";

		return ufo->getAltitude();
	}

	return "STR_HIGH_UC";

/*	switch (RNG::generate(0,3))
	{
		default: // avoid vc++ linker warnings.
		case 0:
		case 1: return "STR_LOW_UC";
		case 2: return "STR_HIGH_UC";
		case 3: return "STR_VERY_HIGH";
	} */
}

/**
 * Sets the destination of this Craft.
 * @param dest - pointer to Target destination
 */
void Craft::setDestination(Target* const dest)
{
	if (_status != CS_OUT)
	{
		_takeOff = 75;
		setSpeed(_crRule->getMaxSpeed() / 10);
	}
	else if (dest == nullptr)
		setSpeed(_crRule->getMaxSpeed() / 2);
	else
		setSpeed(_crRule->getMaxSpeed());

	MovingTarget::setDestination(dest);
}

/**
 * Gets the quantity of weapons equipped on this Craft.
 * @return, quantity of weapons
 */
int Craft::getQtyWeapons() const
{
	if (_crRule->getWeapons() != 0)
	{
		int ret (0);
		for (std::vector<CraftWeapon*>::const_iterator
				i = _weapons.begin();
				i != _weapons.end();
				++i)
		{
			if (*i != nullptr)
				++ret;
		}
		return ret;
	}
	return 0;
}

/**
 * Gets the quantity of Soldiers that are currently loaded onto this Craft.
 * @return, quantity of Soldiers
 */
int Craft::getQtySoldiers() const
{
	if (_crRule->getSoldiers() != 0)
	{
		int ret (0);
		for (std::vector<Soldier*>::const_iterator
				i = _base->getSoldiers()->begin();
				i != _base->getSoldiers()->end();
				++i)
		{
			if ((*i)->getCraft() == this)
				++ret;
		}
		return ret;
	}
	return 0;
}

/**
 * Gets the quantity of equipment-items that are currently loaded onto this Craft.
 * @return, quantity of items
 */
int Craft::getQtyEquipment() const
{
	if (_crRule->getItems() != 0)
		return _items->getTotalQuantity();

	return 0;
}

/**
 * Gets the quantity of Vehicles that are currently loaded onto this Craft.
 * @param quadrants - true to return tile-spaces in a transport (default false)
 * @return, either quantity of Vehicles or tile-space used
 */
int Craft::getQtyVehicles(bool quadrants) const
{
	if (_crRule->getVehicles() != 0)
	{
		if (quadrants == true)
		{
			int ret (0);
			for (std::vector<Vehicle*>::const_iterator
					i = _vehicles.begin();
					i != _vehicles.end();
					++i)
			{
				ret += (*i)->getQuads();
			}
			return ret;
		}
		return static_cast<int>(_vehicles.size());
	}
	return 0;
}

/**
 * Gets the CraftWeapons currently equipped on this Craft.
 * @return, pointer to a vector of pointers to CraftWeapon
 */
std::vector<CraftWeapon*>* Craft::getWeapons()
{
	return &_weapons;
}

/**
 * Gets a list of the items that are currently loaded onto this Craft.
 * @return, pointer to ItemContainer
 */
ItemContainer* Craft::getCraftItems() const
{
	return _items;
}

/**
 * Gets a list of Vehicles that are currently loaded onto this Craft.
 * @return, pointer to a vector of pointers to Vehicle
 */
std::vector<Vehicle*>* Craft::getVehicles()
{
	return &_vehicles;
}

/**
 * Gets the quantity of damage that this Craft has taken.
 * @return, quantity of damage
 */
int Craft::getCraftDamage() const
{
	return _damage;
}

/**
 * Sets the quantity of damage that this Craft has taken.
 * @param damage - quantity of damage
 */
void Craft::setCraftDamage(const int damage)
{
	_damage = damage;

	if (_damage < 0) _damage = 0;
}

/**
 * Gets the ratio between the quantity of damage this Craft has taken and the
 * total it can take before being destroyed.
 * @return, damage as a percentage
 */
int Craft::getCraftDamagePct() const
{
	return static_cast<int>(std::ceil(
		   static_cast<double>(_damage) / static_cast<double>(_crRule->getMaxDamage()) * 100.));
}

/**
 * Gets the quantity of fuel currently contained in this Craft.
 * @return, quantity of fuel
 */
int Craft::getFuel() const
{
	return _fuel;
}

/**
 * Sets the quantity of fuel currently contained in this Craft.
 * @param fuel - quantity of fuel
 */
void Craft::setFuel(int fuel)
{
	if (fuel > _crRule->getMaxFuel())
		fuel = _crRule->getMaxFuel();
	else if (fuel < 0)
		fuel = 0;

	_fuel = fuel;
}

/**
 * Gets the ratio between the quantity of fuel currently contained in this Craft
 * and the total that it can carry.
 * @return, fuel as a percentage
 */
int Craft::getFuelPct() const
{
	return static_cast<int>(std::ceil(
		   static_cast<double>(_fuel) / static_cast<double>(_crRule->getMaxFuel()) * 100.));
}

/**
 * Gets whether this Craft is currently low on fuel - only has enough to get
 * back to base.
 * @return, true if fuel is low
 */
bool Craft::getLowFuel() const
{
	return _lowFuel;
}

/**
 * Sets whether this Craft is currently low on fuel - only has enough to get
 * back to its Base.
 * @param low - true if fuel is low (default true)
 */
void Craft::setLowFuel(bool low)
{
	_lowFuel = low;
}

/**
 * Consumes this Craft's fuel every 10 minutes while it's in the air.
 */
void Craft::consumeFuel()
{
	setFuel(_fuel - getFuelConsumption());
}

/**
 * Gets the quantity of fuel this Craft uses while it's airborne.
 * @return, fuel quantity
 */
int Craft::getFuelConsumption() const
{
	if (_crRule->getRefuelItem().empty() == false) // Firestorm, Lightning, Avenger, etc.
		return 1;

	return _speed; // Skyranger, Interceptor, etc.
}

/**
 * Gets the minimum required fuel for this Craft to get back to Base.
 * @note This now assumes that Craft cannot be transferred during mid-flight.
 * @return, fuel amount
 */
int Craft::getFuelLimit() const
{
//	return calcFuelLimit(_base);
	double dist;
	if (_dest == nullptr)
		dist = getDistance(_base);
	else
		dist = getDistance(_dest) + _base->getDistance(_dest);

	const double speed (static_cast<double>(_crRule->getMaxSpeed()) * unitToRads / 6.);

	return static_cast<int>(std::ceil(
		   static_cast<double>(getFuelConsumption()) * dist / speed));
}

/*
 * Calculates the minimum required fuel for this Craft to get back to Base.
 * @note Speed and distance are in radians.
 * @param base - pointer to a target Base
 * @return, fuel amount
 *
int Craft::calcFuelLimit(const Base* const base) const // private.
{
	double
		dist,
		patrol_factor;

	if (_dest != nullptr)
	{
		patrol_factor = 1.;
		dist = getDistance(_dest) + _base->getDistance(_dest);
	}
	else
	{
		dist = getDistance(base);
		if (_crRule->getRefuelItem().empty() == false)
			patrol_factor = 1.;	// Elerium-powered Craft do not get an increase for patrolling; they use 1 fuel per 10-min regardless of patrol speed.
		else
			patrol_factor = 2.;
	}

	const double speed = static_cast<double>(_crRule->getMaxSpeed()) * unitToRads / 6.;

	return static_cast<int>(std::ceil(
		   static_cast<double>(getFuelConsumption()) * dist * patrol_factor / speed));
} */

/**
 * Sends this Craft back to its Base.
 */
void Craft::returnToBase()
{
	setDestination(_base);
}

/**
 * Gets whether this Craft has just done a ground mission and is forced to
 * return to its Base.
 * @return, true if this Craft needs to return to base
 */
bool Craft::getTacticalReturn() const
{
	return _tacticalDone;
}

/**
 * Sets that this Craft has just done a ground mission and is forced to return
 * to its Base.
 */
void Craft::setTacticalReturn()
{
	_tacticalDone = true;
}

/**
 * Moves this Craft to its destination.
 */
void Craft::think()
{
	if (_takeOff != 0)
	{
		--_takeOff;

		if (_takeOff == 0)
			setSpeed(_crRule->getMaxSpeed());
	}
	else
	{
		moveTarget();

		if (reachedDestination() == true
			&& _dest == dynamic_cast<Target*>(_base))
		{
			setDestination(nullptr);
			setSpeed(0);

			_lowFuel =
			_tacticalDone = false;
			_warning = CW_NONE;
			_takeOff = 0;

			checkup();
		}
	}
}

/**
 * Checks the condition of all this Craft's systems to define its new status
 * when arriving at a Base by flight or transfer.
 * @note This is actually used here and all over there too - basically whenever
 * a status-phase completes a checkup is done.
 */
void Craft::checkup()
{
	bool showReady (_status != CS_READY);

	int
		cw (0),
		armok (0);

	for (std::vector<CraftWeapon*>::const_iterator
			i = _weapons.begin();
			i != _weapons.end();
			++i)
	{
		if (*i != nullptr)
		{
			++cw;
			if ((*i)->getAmmo() < (*i)->getRules()->getAmmoMax())
				(*i)->setRearming();
			else
				++armok;
		}
	}

	if (_damage > 0)
		_status = CS_REPAIRS;		// 1st stage
	else if (cw > armok)
		_status = CS_REARMING;		// 2nd stage
	else if (_fuel < _crRule->getMaxFuel())
		_status = CS_REFUELLING;	// 3rd stage
	else
		_status = CS_READY;			// 4th Ready.

	if (showReady == true && _status == CS_READY)
		_showReady = true;
}

/**
 * Gets whether to show a message to player that this Craft is ready.
 * @return, true to push a message
 */
bool Craft::showReady()
{
	if (_showReady == true)
	{
		_showReady = false;
		return true;
	}
	return false;
}

/**
 * Repairs this Craft's damage every half-hour while it's docked at a Base.
 */
void Craft::repair()
{
	_warning = CW_NONE;
	_warned = false;

	setCraftDamage(_damage - _crRule->getRepairRate());
	// TODO: prepare to set CW_CANTREPAIR ...

	if (_damage == 0)
		checkup();
}

/**
 * Rearms this Craft's weapons by adding ammo every half-hour while it's docked
 * at a Base.
 * @param rules - pointer to the Ruleset
 * @return, blank string if ArmOk else a string for cantLoad
 */
std::string Craft::rearm(const Ruleset* const rules)
{
	std::string
		ret,
		test;

	_warning = CW_NONE;

	for (std::vector<CraftWeapon*>::const_iterator
			i = _weapons.begin();
			i != _weapons.end();
			++i)
	{
		if (*i != nullptr
			&& (*i)->getRearming() == true)
		{
			test.clear();

			const std::string clip ((*i)->getRules()->getClipItem());
			const int baseClips (_base->getStorageItems()->getItemQuantity(clip));

			if (clip.empty() == true)
				(*i)->rearm();
			else if (baseClips > 0)
			{
				int clipsUsed ((*i)->rearm(
										baseClips,
										rules->getItemRule(clip)->getFullClip()));
				if (clipsUsed != 0)
				{
					if (clipsUsed < 0) // trick. See CraftWeapon::rearm() - not enough clips at Base
					{
						clipsUsed = -clipsUsed;
						test = clip;

						_warning = CW_CANTREARM;
					}

					_base->getStorageItems()->removeItem(clip, clipsUsed);
				}
			}
			else // no ammo at base
			{
				test = clip;
				_warning = CW_CANTREARM;
				(*i)->setCantLoad();
			}

			if (test.empty() == false) // warning
			{
				if (ret.empty() == false) // double warning
					ret = "STR_ORDNANCE_LC";
				else // check next weapon
					ret = test;
			}
			else // armok
				break;
		}
	} // NOTE: That handles only 2 craft weapons.

	if (ret.empty() == true)
		checkup();

	return ret;
}

/**
 * Refuels this Craft every half-hour while docked at a Base.
 */
void Craft::refuel()
{
	_warning = CW_NONE;
	_warned = false;

	setFuel(_fuel + _crRule->getRefuelRate());

	if (_fuel == _crRule->getMaxFuel())
		checkup();
}

/**
 * Gets if a UFO is detected by this Craft's radar.
 * @param target - pointer to a Target
 * @return, true if detected
 */
bool Craft::detect(const Target* const target) const
{
	const int radarRange (_crRule->getRadarRange());
	if (radarRange != 0)
	{
		const double
			range (static_cast<double>(radarRange) * greatCircleConversionFactor),
			dist (getDistance(target) * earthRadius);

		if (range >= dist)
			return true;
	}
	return false;
}

/**
 * Gets whether this Craft's is participating in a tactical mission.
 * @return, true if on or near the battlefield
 */
bool Craft::getTactical() const
{
	return _tactical;
}

/**
 * Sets whether this Craft is participating in a tactical mission.
 * @param tactical - true if on or near the battlefield (default true)
 */
void Craft::setTactical(bool tactical)
{
	if ((_tactical = tactical) == true)
		setSpeed(0);
}

/**
 * Gets whether this Craft has been destroyed.
 * @note If the amount of damage a Craft takes is more than its health it will
 * be destroyed.
 * @return, true if destroyed
 */
bool Craft::isDestroyed() const
{
	return (_damage >= _crRule->getMaxDamage());
}

/**
 * Gets the quantity of space available for Soldiers and Vehicles.
 * @return, space available
 */
int Craft::getSpaceAvailable() const
{
	return _crRule->getSoldiers() - getSpaceUsed();
}

/**
 * Gets the quantity of space in use by Soldiers and Vehicles.
 * @return, space used
 */
int Craft::getSpaceUsed() const
{
	int total (0); // <- could use getQtyVehicles(true)
	for (std::vector<Vehicle*>::const_iterator
			i = _vehicles.begin();
			i != _vehicles.end();
			++i)
	{
		total += (*i)->getQuads();
	}
	return total + getQtySoldiers();
}

/**
 * Gets the total quantity of Vehicles of a certain type loaded onto this Craft.
 * @param vehicle - reference a vehicle-type
 * @return, quantity of vehicles
 */
int Craft::getVehicleCount(const std::string& vehicle) const
{
	int total (0);
	for (std::vector<Vehicle*>::const_iterator
			i = _vehicles.begin();
			i != _vehicles.end();
			++i)
	{
		if ((*i)->getRules()->getType() == vehicle)
			++total;
	}
	return total;
}

/**
 * Gets whether this Craft is in a Dogfight.
 * @return, true if dogfighting
 */
bool Craft::inDogfight() const
{
	return _inDogfight;
}

/**
 * Sets whether this Craft is in a Dogfight.
 * @param inDogfight - true if dogfighting
 */
void Craft::inDogfight(bool dogfight)
{
	_inDogfight = dogfight;
}

/**
 * Gets this Craft's unique-ID.
 * @return, tuple of the craft-type and per-type-id
 */
CraftId Craft::getUniqueId() const
{
	return std::make_pair(
					_crRule->getType(),
					_id);
}

/**
 * Sets capacity load.
 * @param load - capacity load
 */
void Craft::setLoadCapacity(const int load)
{
	_loadCap = load;
}

/**
 * Gets capacity load.
 * @return, capacity load
 */
int Craft::getLoadCapacity() const
{
	return _loadCap;
}

/*
 * Sets current load.
 * @param load - current load
 *
void Craft::setLoadCurrent(const int load)
{
	_loadCur = load;
} */

/**
 * Gets current load.
 * @note Also recalculates '_loadCur' value.
 * @return, current load
 */
int Craft::calcLoadCurrent()
{
	return (_loadCur = (getQtyEquipment() + getSpaceUsed() * 10));
}

/**
 * Gets this Craft's CraftWarning.
 * @return, CraftWarning (Craft.h)
 */
CraftWarning Craft::getWarning() const
{
	return _warning;
}

/**
 * Sets this Craft's CraftWarning.
 * @param warning - CraftWarning (Craft.h)
 */
void Craft::setWarning(const CraftWarning warning)
{
	_warning = warning;
}

/**
 * Gets whether a CraftWarning has been issued for this Craft.
 * @return, true if player has been warned about low resources
 */
bool Craft::getWarned() const
{
	return _warned;
}

/**
 * Sets whether a CraftWarning has been issued for this Craft.
 * @param warned - true if player has been warned about low resources (default true)
 */
void Craft::setWarned(const bool warned)
{
	if ((_warned = warned) == false)
		_warning = CW_NONE;
}

/**
 * Gets the quantity of time that this Craft will be repairing/rearming/refueling.
 * @note These are checked & attempted every half hour.
 * @param delayed - reference to set true if this Craft's Base will run out of materiel
 * @return, hours before Craft can fly
 */
int Craft::getDowntime(bool& delayed)
{
	delayed = false;

	int hours (0);
	if (_damage > 0)
	{
		hours += static_cast<int>(std::ceil(
				 static_cast<double>(_damage) / static_cast<double>(_crRule->getRepairRate())
				 / 2.));
	}

	for (std::vector<CraftWeapon*>::const_iterator
			i = _weapons.begin();
			i != _weapons.end();
			++i)
	{
		if (*i != nullptr
			&& (*i)->getRearming() == true)
		{
			const int reqQty ((*i)->getRules()->getAmmoMax() - (*i)->getAmmo());

			hours += static_cast<int>(std::ceil(
					 static_cast<double>(reqQty)
						/ static_cast<double>((*i)->getRules()->getRearmRate())
					 / 2.));

			if (delayed == false)
			{
				const std::string clip ((*i)->getRules()->getClipItem());
				if (clip.empty() == false)
				{
					int baseQty (_base->getStorageItems()->getItemQuantity(clip));
					if (baseQty < reqQty)
					{
						for (std::vector<Transfer*>::const_iterator // check Transfers
								j = _base->getTransfers()->begin();
								j != _base->getTransfers()->end();
								++j)
						{
							if ((*j)->getTransferType() == PST_ITEM
								&& (*j)->getTransferItems() == clip)
							{
								baseQty += (*j)->getQuantity();

								if (baseQty >= reqQty)
									break;
							}
						}
					}

					if (baseQty < reqQty)
						delayed = true;
				}
			}
		}
	}

	if (_fuel < _crRule->getMaxFuel())
	{
		const int reqQty (_crRule->getMaxFuel() - _fuel);

		hours += static_cast<int>(std::ceil(
				 static_cast<double>(reqQty)
					/ static_cast<double>(_crRule->getRefuelRate())
				 / 2.));

		if (delayed == false)
		{
			const std::string fuel (_crRule->getRefuelItem());
			if (fuel.empty() == false)
			{
				int baseQty (_base->getStorageItems()->getItemQuantity(fuel));
				if (baseQty < reqQty)
				{
					for (std::vector<Transfer*>::const_iterator // check Transfers
							i = _base->getTransfers()->begin();
							i != _base->getTransfers()->end();
							++i)
					{
						if ((*i)->getTransferType() == PST_ITEM
							&& (*i)->getTransferItems() == fuel)
						{
							baseQty += (*i)->getQuantity();

							if (baseQty >= reqQty)
								break;
						}
					}
				}

				if (baseQty < reqQty)
					delayed = true;
			}
		}
	}

	return hours;
}

/**
 * Adds a Dogfight kill.
 */
void Craft::addKill() // <- cap this or do a log or an inverted exponential increase or ...
{
	++_kills;
}

/**
 * Gets this Craft's Dogfight kills.
 * @return, kills
 */
int Craft::getKills() const
{
	return _kills;
}

/**
 * Gets if this Craft has left the ground.
 * @return, true if airborne
 */
bool Craft::getTakeoff() const
{
	return (_takeOff == 0);
}

/**
 * Transfers all soldiers, tanks, items, and weapons to its Base.
 * NOTE: Do weapons & rounds use space at the Base ......
 * @param rules			- pointer to the Ruleset
 * @param updateCraft	- true to keep the Craft and update its contents and
 *						  keep weapon hard-points intact (default true)
 */
void Craft::unloadCraft(
		const Ruleset* const rules,
		bool updateCraft)
{
	if (_crRule->getSoldiers() != 0)
	{
		for (std::vector<Soldier*>::const_iterator
				i = _base->getSoldiers()->begin();
				i != _base->getSoldiers()->end();
				++i)
		{
			if ((*i)->getCraft() == this)
				(*i)->setCraft();
		}
	}

	if (_crRule->getVehicles() != 0)
	{
		for (std::vector<Vehicle*>::const_iterator
				i = _vehicles.begin();
				i != _vehicles.end();
				)
		{
			_base->getStorageItems()->addItem((*i)->getRules()->getType());

			if ((*i)->getRules()->getCompatibleAmmo()->empty() == false)
				_base->getStorageItems()->addItem(
											(*i)->getRules()->getCompatibleAmmo()->front(),
											(*i)->getAmmo());
			if (updateCraft == true)
			{
				delete *i;
				i = _vehicles.erase(i);
			}
			else
				++i;
		}
	}

	if (_crRule->getItems() != 0)
	{
		for (std::map<std::string, int>::const_iterator
				i = _items->getContents()->begin();
				i != _items->getContents()->end();
				++i)
		{
			_base->getStorageItems()->addItem(i->first, i->second);

			if (updateCraft == true)
				_items->removeItem(i->first, i->second);
		}
	}

	if (updateCraft == false && _crRule->getWeapons() != 0)
	{
		for (std::vector<CraftWeapon*>::const_iterator
				i = _weapons.begin();
				i != _weapons.end();
				++i)
		{
			if (*i != nullptr)
			{
				_base->getStorageItems()->addItem((*i)->getRules()->getLauncherItem());
				_base->getStorageItems()->addItem(
											(*i)->getRules()->getClipItem(),
											(*i)->getClipsLoaded(rules));
			}
		}
	}


	for (std::vector<BaseFacility*>::const_iterator
			i = _base->getFacilities()->begin();
			i != _base->getFacilities()->end();
			++i)
	{
		if ((*i)->getCraft() == this)
		{
			(*i)->setCraft();
			break;
		}
	}
}

}
