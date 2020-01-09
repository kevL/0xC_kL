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

#include "Craft.h"

//#include <cmath>

#include "AlienBase.h"
#include "Base.h"
#include "CraftWeapon.h"
#include "ItemContainer.h"
#include "Soldier.h"
#include "TerrorSite.h"
#include "Ufo.h"
#include "Vehicle.h"
#include "Waypoint.h"

#include "../Engine/Game.h"
#include "../Engine/Language.h"
#include "../Engine/Logger.h"

#include "../Geoscape/GeoscapeState.h"
#include "../Geoscape/Globe.h" // Globe::GLM_CRAFT

#include "../Ruleset/RuleAlienDeployment.h"
#include "../Ruleset/RuleArmor.h"
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
 * @param playSave	- pointer to the SavedGame
 * @param hasId		- true if craft already has an ID assigned (default false)
 */
Craft::Craft(
		RuleCraft* const crRule,
		Base* const base,
		SavedGame* const playSave,
		bool hasId)
	:
		MovingTarget(playSave),
		_crRule(crRule),
		_base(base),
		_fuel(0),
		_hull(crRule->getCraftHullCap()),
		_takeOffDelay(0),
		_status(CS_READY),
		_lowFuel(false),
		_tacticalReturn(false),
		_tactical(false),
		_inDogfight(false),
		_warning(CW_NONE),
		_warned(false), // do not save-to-file; ie, re-warn player if reloading
		_kills(0),
		_showReady(false),
		_targetGround(false),
		_w1Disabled(false), // TODO: save weapon-disabled states to file.
		_w2Disabled(false)
{
	if (hasId == true)
		_id = 0; // will load from save
	else
		_id = _playSave->getCanonicalId(_crRule->getType());

	_items = new ItemContainer();

	for (size_t
			i = 0u;
			i != _crRule->getWeaponCapacity();
			++i)
		_weapons.push_back(nullptr);

	if (_base != nullptr)
		setBase(_base);

	_loadCap = _crRule->getItemCapacity() + _crRule->getSoldierCapacity() * 10;
}

/**
 * Deletes this Craft.
 */
Craft::~Craft()
{
	delete _items;

	for (std::vector<CraftWeapon*>::const_iterator
			i  = _weapons.begin();
			i != _weapons.end();
			++i)
		delete *i;

	for (std::vector<Vehicle*>::const_iterator
			i  = _vehicles.begin();
			i != _vehicles.end();
			++i)
		delete *i;
}

/**
 * Loads this Craft from a YAML file.
 * @param node	- reference a YAML node
 * @param rules	- pointer to the Ruleset
 */
void Craft::loadCraft(
		const YAML::Node& node,
		const Ruleset* const rules)
{
	MovingTarget::load(node);

	_id   = node["id"]  .as<int>(_id);
	_fuel = node["fuel"].as<int>(_fuel);
	_hull = node["hull"].as<int>(_hull);

	_warning = static_cast<CraftWarning>(node["warning"].as<int>(_warning));

	std::string type;

	size_t j (0u);
	for (YAML::const_iterator
			i  = node["weapons"].begin();
			i != node["weapons"].end();
			++i)
	{
		if (_crRule->getWeaponCapacity() > j)
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

	if (_crRule->getItemCapacity() != 0)
	{
		_items->load(node["items"]);
		for (std::map<std::string, int>::const_iterator
				i  = _items->getContents()->begin();
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
	}

	for (YAML::const_iterator
			i  = node["vehicles"].begin();
			i != node["vehicles"].end();
			++i)
	{
		type = (*i)["type"].as<std::string>();
		if (rules->getItemRule(type) != nullptr)
		{
			const int unitSize (rules->getArmor(rules->getUnitRule(type)->getArmorType())->getSize());
			Vehicle* const vehicle (new Vehicle(
											rules->getItemRule(type),
											0,
											unitSize * unitSize));
			vehicle->load(*i);
			_vehicles.push_back(vehicle);
		}
		else Log(LOG_ERROR) << "Failed to load item " << type;
	}

	_status = static_cast<CraftStatus>(node["status"].as<int>(_status));

	_lowFuel        = node["lowFuel"]       .as<bool>(_lowFuel);
	_tacticalReturn = node["tacticalReturn"].as<bool>(_tacticalReturn);
	_kills          = node["kills"]         .as<int>(_kills);

	if (const YAML::Node& label = node["label"])
		_label = Language::utf8ToWstr(label.as<std::string>());

	if (const YAML::Node& target = node["target"])
	{
		int id (target["id"].as<int>());

//		"STR_UFO",			// 0
//		"STR_BASE",			// 1
//		"STR_ALIEN_BASE",	// 2
//		"STR_TERROR_SITE",	// 3
//		"STR_WAYPOINT",		// 4
//		"STR_LANDING_SITE",	// 5
//		"STR_CRASH_SITE"	// 6

		if ((type = target["type"].as<std::string>()) == Target::stTarget[1u]) // "STR_BASE"
			setTarget(_base);
		else if (type == Target::stTarget[0u]) // "STR_UFO" // is this *always* "STR_UFO" .........
		{
			const std::vector<Ufo*>* const ufoList (_playSave->getUfos());
			for (std::vector<Ufo*>::const_iterator
					i  = ufoList->begin();
					i != ufoList->end();
					++i)
			{
				if ((*i)->getId() == id)
				{
					setTarget(*i);
					break;
				}
			}
		}
		else if (type == Target::stTarget[4u]) // "STR_WAYPOINT"
		{
			const std::vector<Waypoint*>* const wpList (_playSave->getWaypoints());
			for (std::vector<Waypoint*>::const_iterator
					i  = wpList->begin();
					i != wpList->end();
					++i)
			{
				if ((*i)->getId() == id)
				{
					setTarget(*i);
					break;
				}
			}
		}
/*		else if (type == Target::stTarget[2u]) // "STR_ALIEN_BASE"
		{
			const std::vector<AlienBase*>* const abList (_playSave->getAlienBases());
			for (std::vector<AlienBase*>::const_iterator
					i  = abList->begin();
					i != abList->end();
					++i)
			{
				if ((*i)->getId() == id)
				{
					setTarget(*i);
					break;
				}
			}
		}
		else if (type == Target::stTarget[3u]) // "STR_TERROR_SITE"
		{
			const std::vector<TerrorSite*>* const terrorList (_playSave->getTerrorSites());
			for (std::vector<TerrorSite*>::const_iterator
					i  = terrorList->begin();
					i != terrorList->end();
					++i)
			{
				if ((*i)->getId() == id)
				{
					setTarget(*i);
					break;
				}
			}
		} */
		else // there could be either an AlienBase-type or a TerrorSite-type here ...
		{
			bool found (false);

			const std::vector<AlienBase*>* const abList (_playSave->getAlienBases());
			for (std::vector<AlienBase*>::const_iterator
					i  = abList->begin();
					i != abList->end();
					++i)
			{
				if ((*i)->getId() == id
					&& (*i)->getAlienBaseDeployed()->getMarkerType() == type) // is this necessary. not for stock UFO.
				{
					found = true;
					setTarget(*i);
					break;
				}
			}

			if (found == false)
			{
				const std::vector<TerrorSite*>* const tsList (_playSave->getTerrorSites());
				for (std::vector<TerrorSite*>::const_iterator //(type == Target::stTarget[3u])
						i  = tsList->begin();
						i != tsList->end();
						++i)
				{
					if ((*i)->getId() == id
						&& (*i)->getTerrorDeployed()->getMarkerType() == type) // is this necessary. not for stock UFO.
					{
						setTarget(*i);
						break;
					}
				}
			}
		}
	}

	_takeOffDelay = node["takeOff"] .as<int>(_takeOffDelay);
	_tactical     = node["tactical"].as<bool>(_tactical);

	if (_tactical == true) setSpeed();
}

/**
 * Saves this Craft to a YAML file.
 * @return, YAML node
 */
YAML::Node Craft::save() const
{
	YAML::Node node (MovingTarget::save());

	node["type"] = _crRule->getType();
	node["id"]   = _id;

	for (std::vector<CraftWeapon*>::const_iterator
			i  = _weapons.begin();
			i != _weapons.end();
			++i)
	{
		YAML::Node weapons;
		if (*i != nullptr)
			weapons = (*i)->save();
		else
			weapons["type"] = "0";
		node["weapons"].push_back(weapons);
	}

	if (_crRule->getItemCapacity() != 0)
		node["items"] = _items->save();

	for (std::vector<Vehicle*>::const_iterator
			i  = _vehicles.begin();
			i != _vehicles.end();
			++i)
	{
		node["vehicles"].push_back((*i)->save());
	}

	node["status"] = static_cast<int>(_status);
	node["hull"]   = _hull;

	if (_fuel != 0)              node["fuel"]           = _fuel;
	if (_lowFuel == true)        node["lowFuel"]        = _lowFuel;
	if (_tacticalReturn == true) node["tacticalReturn"] = _tacticalReturn;
	if (_tactical == true)       node["tactical"]       = _tactical;
	if (_kills != 0)             node["kills"]          = _kills;
	if (_takeOffDelay != 0)      node["takeOff"]        = _takeOffDelay;
	if (_label.empty() == false) node["label"]          = Language::wstrToUtf8(_label);
	if (_warning != CW_NONE)     node["warning"]        = static_cast<int>(_warning);

	return node;
}

/**
 * Loads this Craft's identificator from a YAML file.
 * @param node - reference a YAML node
 * @return, unique craft id
 */
CraftId Craft::loadIdentificator(const YAML::Node& node) // static.
{
	return std::make_pair(
						node["type"].as<std::string>(),
						node["id"]	.as<int>());
}

/**
 * Saves this Craft's identificator to a YAML file.
 * @return, YAML node
 */
YAML::Node Craft::saveIdentificator() const
{
	YAML::Node node;

	node["type"] = _crRule->getType();
	node["id"]   = _id;

	return node;
}

/**
 * Gets this Craft's identificator.
 * @return, tuple of the craft-type and per-type-id
 */
CraftId Craft::getIdentificator() const
{
	return std::make_pair(
					_crRule->getType(),
					_id);
}

/**
 * Gets the rule for this Craft's type.
 * @return, pointer to RuleCraft
 */
RuleCraft* Craft::getRules() const
{
	return _crRule;
}

/**
 * Sets the rules for this Craft's type.
 * @warning FOR QUICK-BATTLE USE ONLY!
 * @param crRule - pointer to a different RuleCraft
 */
void Craft::changeRules(RuleCraft* const crRule)
{
	_weapons.clear();

	_crRule = crRule;
	for (size_t
			i  = 0u;
			i != _crRule->getWeaponCapacity();
			++i)
		_weapons.push_back(nullptr);
}

/**
 * Gets this Craft's uniquely identifying label.
 * @note If there's not a custom-label the language default is used.
 * @param lang	- pointer to Language to get strings from
 * @param id	- true to show the Id (default true)
 * @return, label of craft
 */
std::wstring Craft::getLabel(
		const Language* const lang,
		bool id) const
{
	if (_label.empty() == true)
	{
		if (id == true)
			return lang->getString("STR_CRAFTLABEL").arg(lang->getString(_crRule->getType())).arg(_id);

		return lang->getString(_crRule->getType());
	}
	return _label;
}

/**
 * Sets this Craft's custom-label.
 * @param label - reference to a new custom-label; if blank the language default will be used
 */
void Craft::setLabel(const std::wstring& label)
{
	_label = label;
}

/**
 * Gets the globe-marker for this Craft.
 * @return, marker-ID (-1 if not out)
 */
int Craft::getMarker() const
{
	if (_status == CS_OUT)
	{
		const int id (_crRule->getMarker());
		if (id != -1) return id;

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
 * Gets this Craft's status as a string.
 * @return, status-string
 */
std::string Craft::getCraftStatusString() const
{
	switch (_status)
	{
		default:
		case CS_READY:      return "STR_READY";
		case CS_REFUELLING: return "STR_REFUELLING";
		case CS_REARMING:   return "STR_REARMING";
		case CS_REPAIRS:    return "STR_REPAIRS";
		case CS_OUT:        return "STR_OUT";
	}
}

/**
 * Gets the altitude of this Craft.
 * @sa getAltitudeInt().
 * @return, altitude as a string
 */
std::string Craft::getAltitude() const
{
	if (_takeOffDelay > 50) return MovingTarget::stAltitude[0u];
	if (_takeOffDelay > 25) return MovingTarget::stAltitude[1u];
	if (_takeOffDelay != 0) return MovingTarget::stAltitude[2u];

	if (_target == nullptr)
		return MovingTarget::stAltitude[2u];

	const Ufo* const ufo (dynamic_cast<Ufo*>(_target));
	if (ufo != nullptr)
	{
		if (ufo->getAltitude() == MovingTarget::stAltitude[0u])
		{
			if (getDistance(ufo) * radius_earth < 150.)
				return MovingTarget::stAltitude[1u];

			if (getDistance(ufo) * radius_earth < 750.)
				return MovingTarget::stAltitude[2u];

			return MovingTarget::stAltitude[3u];
		}
		return ufo->getAltitude();
	}
	return MovingTarget::stAltitude[3u];
}

/**
 * Gets the altitude of this Craft as an integer.
 * @sa getAltitude(), Ufo::getAltitudeInt().
 * @return, alititude as an integer
 */
unsigned Craft::getAltitudeInt() const
{
	if (_takeOffDelay > 50) return 0u;
	if (_takeOffDelay > 25) return 1u;
	if (_takeOffDelay != 0) return 2u;

	if (_target == nullptr)
		return 2u;

	const Ufo* const ufo (dynamic_cast<Ufo*>(_target));
	if (ufo != nullptr)
	{
		if (ufo->getAltitude() == MovingTarget::stAltitude[0u])
		{
			if (getDistance(ufo) * radius_earth < 150.)
				return 1u;

			if (getDistance(ufo) * radius_earth < 750.)
				return 2u;

			return 3u;
		}
		return ufo->getAltitudeInt();
	}
	return 3u;
}

/**
 * Gets the heading of this Craft as an integer.
 * @sa Ufo::calculateSpeed().
 * @return, heading as an integer
 */
unsigned Craft::getHeadingInt() const
{
	const double
		x ( _speedLon),
		y (-_speedLat);

	if (AreSame(x, 0.) == true || AreSame(y, 0.) == true) // This section guards vs. divide-by-zero.
	{
		if (AreSameTwo(x, 0., y, 0.) == true)
			return 0u;

		if (AreSame(x, 0.) == true)
		{
			if (y > 0.) return 8u;
			return 4u;
		}

		if (x > 0.) return 2u;
		return 6u;
	}

	double theta (std::atan2(y,x)); // theta is radians.
	// Convert radians to degrees so i don't go bonkers;
	// ie. KILL IT WITH FIRE!!1@!
	// NOTE: This is between +/- 180 deg.
	theta *= 180. / M_PI;

	if (theta > 157.5 || theta < -157.5)
		return 6u;

	if (theta >  112.5) return 7u;
	if (theta >   67.5) return 8u;
	if (theta >   22.5) return 1u;
	if (theta < -112.5) return 5u;
	if (theta <  -67.5) return 4u;
	if (theta <  -22.5) return 3u;

	return 2u;
}

/**
 * Sets the destination-target of this Craft.
 * @param target - pointer to Target destination (default nullptr)
 */
void Craft::setTarget(Target* const target)
{
	_targetGround = false;

	MovingTarget::setTarget(target);

	if (_status != CS_OUT)
		_takeOffDelay = TAKEOFF_DELAY;
	else if (target == nullptr)
		setSpeed(_crRule->getTopSpeed() >> 1u);
	else
		setSpeed(_crRule->getTopSpeed());
}

/**
 * Gets the quantity of weapons equipped on this Craft.
 * @return, quantity of weapons
 */
size_t Craft::getQtyWeapons() const
{
	if (_crRule->getWeaponCapacity() != 0u)
	{
		size_t qty (0u);
		for (std::vector<CraftWeapon*>::const_iterator
				i  = _weapons.begin();
				i != _weapons.end();
				++i)
		{
			if (*i != nullptr)
				++qty;
		}
		return qty;
	}
	return 0u;
}

/**
 * Gets the quantity of Soldiers that are currently loaded onto this Craft.
 * @return, quantity of Soldiers
 */
int Craft::getQtySoldiers() const
{
	if (_crRule->getSoldierCapacity() != 0)
	{
		int qty (0);
		for (std::vector<Soldier*>::const_iterator
				i  = _base->getSoldiers()->begin();
				i != _base->getSoldiers()->end();
				++i)
		{
			if ((*i)->getCraft() == this)
				++qty;
		}
		return qty;
	}
	return 0;
}

/**
 * Gets the quantity of equipment-items that are currently loaded onto this Craft.
 * @return, quantity of items
 */
int Craft::getQtyEquipment() const
{
	if (_crRule->getItemCapacity() != 0)
		return _items->getTotalQuantity();

	return 0;
}

/**
 * Gets the quantity of Vehicles that are currently loaded onto this Craft.
 * @param tiles - true to return tile-area in a transport (default false)
 * @return, either quantity of Vehicles or tile-space used
 */
int Craft::getQtyVehicles(bool tiles) const
{
	if (_crRule->getVehicleCapacity() != 0)
	{
		if (tiles == true)
		{
			int quads (0);
			for (std::vector<Vehicle*>::const_iterator
					i  = _vehicles.begin();
					i != _vehicles.end();
					++i)
			{
				quads += (*i)->getQuads();
			}
			return quads;
		}
		return static_cast<int>(_vehicles.size());
	}
	return 0;
}

/**
 * Gets the CraftWeapons currently equipped on this Craft.
 * @return, pointer to a vector of pointers to CraftWeapon
 */
std::vector<CraftWeapon*>* Craft::getCraftWeapons()
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
 * Sets this Craft to full hull.
 */
void Craft::setCraftHullFull()
{
	_hull = _crRule->getCraftHullCap();
}

/**
 * Sets this Craft's hull after inflicted hurt.
 * @param inflict - inflicted hurt
 */
void Craft::setCraftHull(int inflict)
{
	if ((_hull -= inflict) < 0) _hull = 0;
}

/**
 * Gets this Craft's hull.
 * @return, hull
 */
int Craft::getCraftHull() const
{
	return _hull;
}

/**
 * Gets this Craft's hull-percentage.
 * @return, hull pct
 */
int Craft::getCraftHullPct() const
{
	if (_hull == 0) return 0;
	if (_hull == _crRule->getCraftHullCap()) return 100;

	return Vicegrip(static_cast<int>(Round(
		   static_cast<float>(_hull) / static_cast<float>(_crRule->getCraftHullCap()) * 100.f)), 1, 99);
}

/**
 * Checks if this Craft has been destroyed.
 * @note If the amount of damage a Craft takes is more than its health it will
 * be destroyed.
 * @return, true if destroyed
 */
bool Craft::isDestroyed() const
{
	return _hull == 0;
}

/**
 * Sets the quantity of fuel currently contained in this Craft.
 * @param fuel - quantity of fuel
 */
void Craft::setFuel(int fuel)
{
	if ( fuel > _crRule->getFuelCapacity())
		_fuel = _crRule->getFuelCapacity();
	else if (fuel < 0)
		_fuel = 0;
	else
		_fuel = fuel;
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
 * Gets the ratio between the quantity of fuel currently contained in this Craft
 * and the total that it can carry.
 * @return, fuel as a percentage
 */
int Craft::getFuelPct() const
{
	if (_fuel == 0) return 0;
	if (_fuel == _crRule->getFuelCapacity()) return 100;

	return Vicegrip(static_cast<int>(Round(
		   static_cast<float>(_fuel) / static_cast<float>(_crRule->getFuelCapacity()) * 100.f)), 1, 99);
}

/**
 * Uses this Craft's fuel every 10 minutes while airborne.
 * @return, true if low on fuel
 */
bool Craft::useFuel()
{
	//Log(LOG_INFO) << "";
	//Log(LOG_INFO) << "Craft::useFuel()";

	int fuelUsage;
	if (_crRule->getRefuelItem().empty() == false)	// Firestorm, Lightning, Avenger, etc.
		fuelUsage = 1;
	else											// Skyranger, Interceptor, etc.
		fuelUsage = _speed;

	setFuel(_fuel - fuelUsage);

	//Log(LOG_INFO) << ". getDistanceLeft= " << getDistanceLeft();
	//Log(LOG_INFO) << ". getDistanceReserved= " << getDistanceReserved(_target);

	if (_lowFuel == false
		&& getDistanceLeft() < getDistance(_base))
//		&& getDistanceLeft() < getDistanceReserved(_target))
	{
		//Log(LOG_INFO) << ". . set Low Fuel";
		_lowFuel = true;
		setTarget(_base);
		return true;
	}
	return false;
}

/**
 * Gets the distance that this Craft needs to reserve fuel for to return to its
 * Base.
 * @param target - a Target other than the craft's Base
 * @return, the required distance in radians
 */
double Craft::getDistanceReserved(const Target* const target) const
{
//	if (target == nullptr)
//		return getDistance(_base);

	return (getDistance(target) + _base->getDistance(target));
}

/**
 * Gets the distance that this Craft can travel with its current fuel.
 * @return, the distance in radians the craft can still travel
 */
double Craft::getDistanceLeft() const
{
	int range (_fuel);

	if (_crRule->getRefuelItem().empty() == false)
		range *= _crRule->getTopSpeed();

	return (static_cast<double>(range) * arcToRads / 6.); // 6 doses per hour
}
/* @note The craft's total range effectively gets an extra dose to account for
 * the discrepancy between fuel-usage per 10 minutes and each step's check for
 * low fuel per 5 seconds. This bonus however is not in effect if player is
 * selecting a destination since it would allow selecting a destination that
 * would almost instantly trigger the craft's low-fuel flag.
 * UPDATE: This is no longer strictly needed since the condition for low fuel
 * no longer checks full distance but only the distance left to get back to Base.
 * @select - true if player is selecting a destination (default false)
double Craft::getDistanceLeft(bool select) const
{
	int range (_fuel);

	if (_crRule->getRefuelItem().empty() == false)
	{
		if (select == false)	// give Craft an extra dose as leeway if player is not currently selecting a destination.
			range += 1;

		range *= _crRule->getTopSpeed();
	}
	else if (select == false)	// give Craft an extra dose as leeway if player is not currently selecting a destination.
		range += _crRule->getTopSpeed();

	return (static_cast<double>(range) * arcToRads / 6.); // 6 doses per hour
} */

/**
 * Checks if this Craft is currently low on fuel and has been forced to return
 * to its Base.
 * @return, true if fuel is low
 */
bool Craft::isLowFuel() const
{
	return _lowFuel;
}

/**
 * Sends this Craft back to its Base.
 */
void Craft::returnToBase()
{
	setTarget(_base);
}

/**
 * Sets that this Craft has just done a tactical battle and is forced to return
 * to its Base.
 */
void Craft::setTacticalReturn()
{
	setTarget(_base);
	_tacticalReturn = true;
	_tactical = false;
}

/**
 * Checks if this Craft has just done a tactical battle and is forced to return
 * to its Base.
 * @return, true if this Craft needs to return to base
 */
bool Craft::isTacticalReturn() const
{
	return _tacticalReturn;
}

/**
 * Moves this Craft to its destination.
 */
void Craft::think()
{
	if (_status == CS_OUT)
	{
		switch (_takeOffDelay)
		{
			case 0:
				stepTarget();

				if (reachedDestination() == true
					&& _target == dynamic_cast<Target*>(_base))
				{
					setTarget();
					setSpeed();

					_lowFuel =
					_tacticalReturn = false;
					_warning = CW_NONE;

					checkup();
				}
				break;

			default:
				if (_target == dynamic_cast<Target*>(_base)) // craft is allowed to rebase if contact is lost vs. UFO before take-off completes - handle it.
				{
					setFuel(_fuel - 1);	// top up and ...
					_takeOffDelay = 0;	// -> recurse.
					think();
				}
				else if (--_takeOffDelay == 0)
					setSpeed(_crRule->getTopSpeed());
				else
				{
					const int speed (std::ceil(static_cast<double>(_crRule->getTopSpeed())
											* (static_cast<double>(TAKEOFF_DELAY - _takeOffDelay) / static_cast<double>(TAKEOFF_DELAY))));
					setSpeed(speed);
				}
		}
	}
}

/**
 * Gets if this Craft has left the ground.
 * @return, true if airborne
 */
bool Craft::hasLeftGround() const
{
	return (_takeOffDelay == 0);
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
		cw    (0),
		armok (0);

	if (_crRule->getWeaponCapacity() != 0u)
	{
		for (std::vector<CraftWeapon*>::const_iterator
				i  = _weapons.begin();
				i != _weapons.end();
				++i)
		{
			if (*i != nullptr)
			{
				++cw;
				if ((*i)->getCwLoad() < (*i)->getRules()->getLoadCapacity())
					(*i)->setRearming();
				else
					++armok;
			}
		}
	}

	if (_hull < _crRule->getCraftHullCap())
		_status = CS_REPAIRS;		// 1st stage
	else if (cw > armok)
		_status = CS_REARMING;		// 2nd stage
	else if (_fuel < _crRule->getFuelCapacity())
		_status = CS_REFUELLING;	// 3rd stage
	else
	{
		_status = CS_READY;			// 4th Ready.

		if (showReady == true) _showReady = true;
	}
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
 * Repairs this Craft's damage every half-hour while it's docked at its Base.
 */
void Craft::repair()
{
	_warning = CW_NONE;
	_warned = false;

	const int hullCap (_crRule->getCraftHullCap());
	if ((_hull += _crRule->getRepairRate()) >= hullCap)
	{
		_hull = hullCap;
		checkup();
	}
	// TODO: prepare to set CW_CANTREPAIR ...
}

/**
 * Rearms this Craft's weapons by adding ammo every half-hour while it's docked
 * at its Base.
 * @param rules - pointer to the Ruleset
 * @return, blank string if ArmOk else an ammo-type-needed string for cantLoad
 */
std::string Craft::rearm(const Ruleset* const rules)
{
	std::string
		ret,
		warn;

	_warning = CW_NONE;

	for (std::vector<CraftWeapon*>::const_iterator
			i  = _weapons.begin();
			i != _weapons.end();
			++i)
	{
		if (*i != nullptr && (*i)->getRearming() == true)
		{
			warn.clear();

			const std::string clip ((*i)->getRules()->getClipType());
			const int baseQty (_base->getStorageItems()->getItemQuantity(clip));

			if (clip.empty() == true)
				(*i)->rearm();
			else if (baseQty > 0)
			{
				int usedQty ((*i)->rearm(
										baseQty,
										rules->getItemRule(clip)->getFullClip()));
				if (usedQty != 0)
				{
					if (usedQty < 0) // trick. See CraftWeapon::rearm() - not enough clips at Base
					{
						usedQty = -usedQty;
						warn = clip;

						_warning = CW_CANTREARM;
					}

					_base->getStorageItems()->removeItem(clip, usedQty);
				}
			}
			else // no ammo at base
			{
				warn = clip;
				_warning = CW_CANTREARM;
				(*i)->setCantLoad();
			}

			if (warn.empty() == false) // warning
			{
				if (ret.empty() == false) // double warning
					ret = "STR_ORDNANCE_LC";
				else // check next weapon
					ret = warn;
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

	if (_fuel == _crRule->getFuelCapacity())
		checkup();
}

/**
 * Gets if a UFO is detected by this Craft's radar.
 * @param target - pointer to a Target
 * @return, true if detected
 */
bool Craft::detect(const Target* const target) const
{
	const int range (_crRule->getRangeRadar());
	return range != 0
		&& getDistance(target) * radius_earth <= static_cast<double>(range);
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
		setSpeed();
}

/**
 * Gets the quantity of space available for Soldiers and Vehicles.
 * @return, space available
 */
int Craft::getSpaceAvailable() const
{
	return _crRule->getSoldierCapacity() - getSpaceUsed();
}

/**
 * Gets the quantity of space in use by Soldiers and Vehicles.
 * @return, space used
 */
int Craft::getSpaceUsed() const
{
	int total (0); // <- could use getQtyVehicles(true)
	for (std::vector<Vehicle*>::const_iterator
			i  = _vehicles.begin();
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
			i  = _vehicles.begin();
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
 * Gets capacity load.
 * @return, capacity load
 */
int Craft::getLoadCapacity() const
{
	return _loadCap;
}

/**
 * Gets current load.
 * @return, current load
 */
int Craft::calcLoadCurrent()
{
	return getQtyEquipment() + getSpaceUsed() * 10;
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
void Craft::setWarned(bool warned)
{
	if ((_warned = warned) == false)
		_warning = CW_NONE;
}

/**
 * Gets the quantity of time that this Craft will be repairing/rearming/refueling.
 * @note These are checked & attempted every half hour.
 * @param isDelayed - reference to set true if this Craft's Base will run out of materials
 * @param rules     - pointer to the Ruleset
 * @return, hours before Craft can fly
 */
int Craft::getDowntime(bool& isDelayed, const Ruleset* const rules)
{
	isDelayed = false;

	int hours (0);
	if (_hull < _crRule->getCraftHullCap())
	{
		hours += static_cast<int>(std::ceil(
				 static_cast<float>(_crRule->getCraftHullCap() - _hull) / static_cast<float>(_crRule->getRepairRate())
				 / 2.f));
	}

	for (std::vector<CraftWeapon*>::const_iterator
			i  = _weapons.begin();
			i != _weapons.end();
			++i)
	{
		if (*i != nullptr && (*i)->getRearming() == true)
		{
			const int reqQty ((*i)->getRules()->getLoadCapacity() - (*i)->getCwLoad());

			hours += static_cast<int>(std::ceil(
					 static_cast<float>(reqQty) / static_cast<float>((*i)->getRules()->getRearmRate())
					 / 2.f));

			if (isDelayed == false)
			{
				const std::string& clip ((*i)->getRules()->getClipType());
				if (clip.empty() == false)
				{
					int baseQty (_base->getStorageItems()->getItemQuantity(clip) * rules->getItemRule(clip)->getFullClip());
					if (baseQty < reqQty)
					{
						for (std::vector<Transfer*>::const_iterator // check Transfers
								j  = _base->getTransfers()->begin();
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

					if (baseQty < reqQty) isDelayed = true;
				}
			}
		}
	}

	if (_fuel < _crRule->getFuelCapacity())
	{
		const int reqQty (_crRule->getFuelCapacity() - _fuel);

		hours += static_cast<int>(std::ceil(
				 static_cast<float>(reqQty) / static_cast<float>(_crRule->getRefuelRate())
				 / 2.f));

		if (isDelayed == false)
		{
			const std::string& fuel (_crRule->getRefuelItem());
			if (fuel.empty() == false)
			{
				int baseQty (_base->getStorageItems()->getItemQuantity(fuel));
				if (baseQty < reqQty)
				{
					for (std::vector<Transfer*>::const_iterator // check Transfers
							i  = _base->getTransfers()->begin();
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

				if (baseQty < reqQty) isDelayed = true;
			}
		}
	}

	return hours;
}

/**
 * Adds a Dogfight kill.
 */
void Craft::ace()
{
	++_kills; // <- cap this or do a log or an inverted exponential increase or ...
}

/**
 * Gets this Craft's Dogfight kills.
 * @return, kills
 */
int Craft::getAces() const
{
	return _kills;
}

/**
 * Transfers all soldiers, tanks, items, and weapons to its Base.
 * @note Do weapons & rounds use space at the Base ......
 * @param rules			- pointer to the Ruleset
 * @param updateCraft	- true to keep the Craft and update its contents and
 *						  keep weapon hard-points intact (default true)
 */
void Craft::unloadCraft(
		const Ruleset* const rules,
		bool updateCraft)
{
	if (_crRule->getSoldierCapacity() != 0)
	{
		for (std::vector<Soldier*>::const_iterator
				i  = _base->getSoldiers()->begin();
				i != _base->getSoldiers()->end();
				++i)
		{
			if ((*i)->getCraft() == this)
				(*i)->setCraft();
		}
	}

	if (_crRule->getVehicleCapacity() != 0)
	{
		const RuleItem* itRule;
		for (std::vector<Vehicle*>::const_iterator
				i  = _vehicles.begin();
				i != _vehicles.end();
				)
		{
			itRule = (*i)->getRules();
			_base->getStorageItems()->addItem(itRule->getType());

			if (itRule->getFullClip() > 0)
				_base->getStorageItems()->addItem(
											itRule->getClipTypes()->front(),
											(*i)->getLoad());
			if (updateCraft == true)
			{
				delete *i;
				i = _vehicles.erase(i);
			}
			else
				++i;
		}
	}

	if (_crRule->getItemCapacity() != 0)
	{
		for (std::map<std::string, int>::const_iterator
				i  = _items->getContents()->begin();
				i != _items->getContents()->end();
				++i)
		{
			_base->getStorageItems()->addItem(i->first, i->second);

			if (updateCraft == true)
				_items->removeItem(i->first, i->second);
		}
	}

	if (updateCraft == false && _crRule->getWeaponCapacity() != 0u)
	{
		for (std::vector<CraftWeapon*>::const_iterator
				i  = _weapons.begin();
				i != _weapons.end();
				++i)
		{
			if (*i != nullptr)
			{
				_base->getStorageItems()->addItem((*i)->getRules()->getLauncherType());
				_base->getStorageItems()->addItem(
											(*i)->getRules()->getClipType(),
											(*i)->getClipsLoaded(rules));
			}
		}
	}


	for (std::vector<BaseFacility*>::const_iterator
			i  = _base->getFacilities()->begin();
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

/**
 * Sets this Craft as intercepting a ground-target.
 * @note This is used only by non-transport craft so that player can at least
 * see the data of a ConfirmLanding screen but without being allowed to start
 * tactical.
 * @param intercept - true to intercept
 */
void Craft::interceptGroundTarget(bool intercept)
{
	_targetGround = intercept;
}

/**
 * Gets if this Craft is intercepting a ground-target.
 * @note This is used only by non-transport craft so that player can at least
 * see the data of a ConfirmLanding screen but without being allowed to start
 * tactical.
 * @return, true if intercept
 */
bool Craft::interceptGroundTarget() const
{
	return _targetGround;
}

/**
 * Gets this Craft's cost for tactical.
 * @note Is used to predict the operational cost of sending this craft on a
 * tactical mission.
 * @return, cost
 */
int Craft::getOperationalCost() const
{
	int cost (_crRule->getSoldierCapacity() * 1000); // the craft's tactical cost

	for (std::vector<Soldier*>::const_iterator
			i = _base->getSoldiers()->begin(), j = _base->getSoldiers()->end();
			i != j;
			++i)
	{
		if ((*i)->getCraft() == this)
		{
			cost += (*i)->getRankCost(false); // onboard soldiers' cost
		}
	}

	return (cost + getQtyVehicles(true) * 750); // onboard supports' cost
}

/**
 * Sets a craft-weapon disabled or enabled on this Craft.
 * @param hardpoint	- w1 or w2
 * @param disabled	- true to disable
 */
void Craft::setWeaponDisabled(int hardpoint, bool disabled)
{
	switch (hardpoint)
	{
		default:
		case 1: _w1Disabled = disabled; break;
		case 2: _w2Disabled = disabled; break;
	}
}

/**
 * Gets if a craft-weapon is disabled or enabled on the Craft.
 * @param hardpoint - w1 or w2
 * @return, true if disabled
 */
bool Craft::getWeaponDisabled(int hardpoint) const
{
	switch (hardpoint)
	{
		default:
		case 1: return _w1Disabled;
		case 2: return _w2Disabled;
	}
}

}
