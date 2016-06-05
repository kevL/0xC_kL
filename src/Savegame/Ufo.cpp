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

#include "Ufo.h"

//#include <algorithm>
//#include <assert.h>

#include "../fmath.h"

#include "AlienMission.h"
#include "Craft.h"
#include "SavedGame.h"
#include "Waypoint.h"

#include "../Engine/Exception.h"
#include "../Engine/Language.h"
//#include "../Engine/Logger.h"

#include "../Geoscape/Globe.h" // Globe::GLM_UFO_*

#include "../Ruleset/RuleAlienMission.h"
#include "../Ruleset/Ruleset.h"
#include "../Ruleset/RuleUfo.h"
#include "../Ruleset/UfoTrajectory.h"


namespace OpenXcom
{

/**
 * Creates the Ufo from a specified ufo-rule.
 * @param ufoRule - pointer to RuleUfo
 */
Ufo::Ufo(const RuleUfo* const ufoRule)
	:
		MovingTarget(),
		_ufoRule(ufoRule),
		_id(0),
		_idCrashed(0),
		_idLanded(0),
		_damage(0),
		_direction("STR_NORTH"),
		_altitude("STR_HIGH_UC"),
		_status(FLYING),
		_secondsLeft(0),
		_tactical(false),
		_mission(nullptr),
		_trajectory(nullptr),
		_trajectoryPoint(0u),
		_detected(false),
		_hyperDetected(false),
		_shootingAt(0),
		_hitFrame(0),
		_processedIntercept(false),
		_fireCountdown(0),
		_escapeCountdown(0)
{}

/**
 * Make sure the AlienMission forgets this UFO and delete only owned-Targets, ie.
 * Waypoints.
 */
Ufo::~Ufo()
{
	Craft* craft;
	for (size_t
			i = 0u;
			i != _targeters.size();
			)
	{
		if ((craft = dynamic_cast<Craft*>(_targeters[i])) != nullptr)
		{
			craft->returnToBase();
			i = 0u;
		}
		else
			++i;
	}

	if (_mission != nullptr)
		_mission->decreaseLiveUfos();

	if (_dest != nullptr)
	{
		const Waypoint* const wp (dynamic_cast<Waypoint*>(_dest));
		if (wp != nullptr)
		{
			delete _dest;
			_dest = nullptr;
		}
	}
}


/**
 ** FUNCTOR ***
 * Match AlienMission based on the unique-ID.
 */
class matchMissionID
	:
		public std::unary_function<const AlienMission*, bool>
{
private:
	int _id;

	public:
		/// Store ID for later comparisons.
		explicit matchMissionID(int id)
			:
				_id(id)
		{}

		/// Match with stored ID.
		bool operator() (const AlienMission* am) const
		{
			return am->getId() == _id;
		}
};


/**
 * Loads the UFO from a YAML file.
 * @param node	- reference a YAML node
 * @param rules	- reference the Ruleset (used to access trajectory data)
 * @param game	- reference the SavedGame (used to get the UFO's mission)
 */
void Ufo::load(
		const YAML::Node& node,
		const Ruleset& rules,
		SavedGame& game)
{
	MovingTarget::load(node);

	_id				= node["id"]			.as<int>(_id);
	_idCrashed		= node["idCrashed"]		.as<int>(_idCrashed);
	_idLanded		= node["idLanded"]		.as<int>(_idLanded);
	_damage			= node["damage"]		.as<int>(_damage);
	_altitude		= node["altitude"]		.as<std::string>(_altitude);
	_direction		= node["direction"]		.as<std::string>(_direction);
	_detected		= node["detected"]		.as<bool>(_detected);
	_hyperDetected	= node["hyperDetected"]	.as<bool>(_hyperDetected);
	_secondsLeft	= node["secondsLeft"]	.as<int>(_secondsLeft);
	_tactical		= node["tactical"]		.as<bool>(_tactical);
	_terrain		= node["terrain"]		.as<std::string>(_terrain);

	double
		lon (_lon),
		lat (_lat);
	if (const YAML::Node& dest = node["dest"])
	{
		lon = dest["lon"].as<double>();
		lat = dest["lat"].as<double>();
	}

	_dest = new Waypoint();
	_dest->setLongitude(lon);
	_dest->setLatitude(lat);

	if (const YAML::Node& status = node["status"])
		_status = static_cast<UfoStatus>(status.as<int>());
	else
	{
		if (_damage >= _ufoRule->getMaxDamage())
			_status = DESTROYED;
		else if (_damage >= (_ufoRule->getMaxDamage() >> 1u))
			_status = CRASHED;
		else if (_altitude == "STR_GROUND")
			_status = LANDED;
		else
			_status = FLYING; // <- already done in cTor init.
	}

	switch (_status) // safety. Although this should never show up as Destroyed ....
	{
		case FLYING:
		case LANDED:
		case CRASHED:
		case DESTROYED:
			break;

		default:
			_status = FLYING;
	}

	if (game.getMonthsPassed() != -1)
	{
		const int missionId (node["mission"].as<int>());
		std::vector<AlienMission*>::const_iterator mission (std::find_if(
																	game.getAlienMissions().begin(),
																	game.getAlienMissions().end(),
																	matchMissionID(missionId)));
		if (mission == game.getAlienMissions().end())
		{
			throw Exception("Unknown mission, save file is corrupt.");
		}
		_mission = *mission;

		const std::string trjType (node["trajectory"].as<std::string>()); // TODO: Don't save trajectory-info if UFO has been shot down.
		_trajectory = rules.getUfoTrajectory(trjType);
		_trajectoryPoint = node["trajectoryPoint"].as<size_t>(_trajectoryPoint);
	}

	_fireCountdown		= node["fireCountdown"]		.as<int>(_fireCountdown);
	_escapeCountdown	= node["escapeCountdown"]	.as<int>(_escapeCountdown);

	if (_tactical == true)
		setSpeed(0);
}

/**
 * Saves the UFO to a YAML file.
 * @return, YAML node
 */
YAML::Node Ufo::save(bool skirmish) const
{
	YAML::Node node (MovingTarget::save());

	node["type"]	= _ufoRule->getType();
	node["id"]		= _id;

	if (_terrain.empty() == false) node["terrain"] = _terrain;

	if		(_idCrashed != 0) node["idCrashed"]	= _idCrashed;
	else if	(_idLanded  != 0) node["idLanded"]	= _idLanded;

	node["altitude"]	= _altitude;
	node["direction"]	= _direction;
	node["status"]		= static_cast<int>(_status);

	if (_damage != 0)			node["damage"]			= _damage;
	if (_detected == true)		node["detected"]		= _detected;
	if (_hyperDetected == true)	node["hyperDetected"]	= _hyperDetected;
	if (_secondsLeft != 0)		node["secondsLeft"]		= _secondsLeft;
	if (_tactical == true)		node["tactical"]		= _tactical;

	if (skirmish == false)
	{
		node["mission"]			= _mission->getId();
		node["trajectory"]		= _trajectory->getId();
		node["trajectoryPoint"]	= _trajectoryPoint;
	}

	if (_fireCountdown != 0)	node["fireCountdown"]	= _fireCountdown;
	if (_escapeCountdown != 0)	node["escapeCountdown"]	= _escapeCountdown;

	return node;
}

/**
 * Saves the UFO's unique identifiers to a YAML file.
 * @return, YAML node
 */
YAML::Node Ufo::saveId() const
{
	YAML::Node node (MovingTarget::saveId());

	node["type"] = "STR_UFO";
	node["id"]   = _id;

	return node;
}

/**
 * Gets the ruleset for the UFO's type.
 * @return, pointer to RuleUfo
 */
const RuleUfo* Ufo::getRules() const
{
	return _ufoRule;
}

/**
 * Changes the rule for the UFO's type.
 * @warning ONLY FOR NEW BATTLE USE!
 * @param ufoRule - pointer to RuleUfo
 */
void Ufo::changeRules(const RuleUfo* const ufoRule)
{
	_ufoRule = ufoRule;
}

/**
 * Sets the UFO's unique-ID.
 * @param id - unique-ID
 */
void Ufo::setId(int id)
{
	_id = id;
}

/**
 * Gets the UFO's unique-ID.
 * @note If it's 0 this UFO has never been detected.
 * @return, unique ID
 */
int Ufo::getId() const
{
	return _id;
}

/**
 * Gets the UFO's unique identifier.
 * @param lang - pointer to Language to get strings from
 * @return, label
 */
std::wstring Ufo::getName(const Language* const lang) const
{
	switch (_status)
	{
		case FLYING:
		case DESTROYED: // Destroyed also means leaving Earth.
			return lang->getString("STR_UFO_").arg(_id);

		case LANDED:
			return lang->getString("STR_LANDING_SITE_").arg(_idLanded);

		case CRASHED:
			return lang->getString("STR_CRASH_SITE_").arg(_idCrashed);
	}
	return L"";
}

/**
 * Gets the globe-marker for the UFO.
 * @return, marker sprite #2,3,4 (-1 if not detected)
 */
int Ufo::getMarker() const
{
	if (_detected == true)
	{
		const int ret (_ufoRule->getMarker());
		if (ret != -1) return ret; // for a custom marker.

		switch (_status)
		{
			case Ufo::FLYING:	return Globe::GLM_UFO_FLYING;
			case Ufo::LANDED:	return Globe::GLM_UFO_LANDED;
			case Ufo::CRASHED:	return Globe::GLM_UFO_CRASHED;
		}
	}
	return -1;
}

/**
 * Sets the quantity of damage this UFO has taken.
 * @param damage - amount of damage
 */
void Ufo::setUfoDamage(int damage)
{
	if ((_damage = damage) < 0) _damage = 0;

	if (isDestroyed() == true)
		_status = DESTROYED;
	else if (isCrashed() == true)
		_status = CRASHED;
}

/**
 * Gets the quantity of damage this UFO has taken.
 * @return, amount of damage
 */
int Ufo::getUfoDamage() const
{
	return _damage;
}

/**
 * Gets the ratio between the amount of damage this UFO
 * has taken and the total it can take before it's destroyed.
 * @return, damage percent
 */
int Ufo::getUfoDamagePct() const
{
	return static_cast<int>(std::floor(
		   static_cast<float>(_damage) / static_cast<float>(_ufoRule->getMaxDamage()) * 100.f));
}

/**
 * Sets whether this UFO has been detected by radars.
 * @param detected - true if detected (default true)
 */
void Ufo::setDetected(bool detected)
{
	_detected = detected;
}

/**
 * Gets whether this UFO has been detected by radars.
 * @return, true if detected
 */
bool Ufo::getDetected() const
{
	return _detected;
}

/**
 * Sets whether this Ufo has been detected by hyper-wave.
 * @param hyperdetected - true if hyperwave-detected (default true)
 */
void Ufo::setHyperDetected(bool hyperdetected)
{
	_hyperDetected = hyperdetected;
}

/**
 * Gets whether this Ufo has been detected by hyper-wave.
 * @return, true if hyperwave-detected
 */
bool Ufo::getHyperDetected() const
{
	return _hyperDetected;
}

/**
 * Sets the amount of remaining seconds the UFO has left on the ground.
 * @note After this many seconds this Ufo will take off if landed or disappear
 * if crashed.
 * @param sec - time in seconds
 */
void Ufo::setSecondsLeft(int sec)
{
	_secondsLeft = std::max(0, sec);
}

/**
 * Gets the amount of remaining seconds the UFO has left on the ground.
 * @note After this many seconds this Ufo will take off if landed or disappear
 * if crashed.
 * @return, amount of seconds
 */
int Ufo::getSecondsLeft() const
{
	return _secondsLeft;
}

/**
 * Sets the current altitude and status of the UFO.
 * @param altitude - altitude
 */
void Ufo::setAltitude(const std::string& altitude)
{
	if ((_altitude = altitude) != "STR_GROUND")
		_status = FLYING;
	else if (isCrashed() == true)
		_status = CRASHED;
	else
		_status = LANDED;
}

/**
 * Gets the current altitude of the UFO.
 * @return, altitude
 */
std::string Ufo::getAltitude() const
{
	return _altitude;
}

/**
 * Gets the current direction the UFO is headed.
 * @return, direction
 */
std::string Ufo::getDirection() const
{
	return _direction;
}

/**
 * Gets if this Ufo took enough damage to cause it to crash.
 * @return, true if crashed
 */
bool Ufo::isCrashed() const
{
	return _damage >= (_ufoRule->getMaxDamage() >> 1u);
}

/**
 * Gets if this Ufo took enough damage to destroy it.
 * @return, true if destroyed
 */
bool Ufo::isDestroyed() const
{
	return _damage >= _ufoRule->getMaxDamage();
}

/**
 * Calculates the direction for this Ufo based on the current raw speed and
 * destination.
 */
void Ufo::calculateSpeed() // private.
{
	MovingTarget::calculateSpeed();

	const double
		x ( _speedLon),
		y (-_speedLat);

	// This section guards vs. divide-by-zero.
	if (AreSame(x, 0.) || AreSame(y, 0.))
	{
		if (AreSame(x, 0.) && AreSame(y, 0.))
			_direction = "STR_NONE_UC";
		else if (AreSame(x, 0.))
		{
			if (y > 0.)
				_direction = "STR_NORTH";
			else //if (y < 0.)
				_direction = "STR_SOUTH";
		}
		else if (AreSame(y, 0.))
		{
			if (x > 0.)
				_direction = "STR_EAST";
			else //if (x < 0.)
				_direction = "STR_WEST";
		}
		return;
	}

	double theta (std::atan2(y,x)); // theta is radians.
	// Convert radians to degrees so i don't go bonkers;
	// ie. KILL IT WITH FIRE!!1@!
	// note that this is between +/- 180 deg.
	theta *= 180. / M_PI;

	if (theta > 157.5 || theta < -157.5)
		_direction = "STR_WEST";
	else if (theta > 112.5)
		_direction = "STR_NORTH_WEST";
	else if (theta > 67.5)
		_direction = "STR_NORTH";
	else if (theta > 22.5)
		_direction = "STR_NORTH_EAST";
	else if (theta < -112.5)
		_direction = "STR_SOUTH_WEST";
	else if (theta < -67.5)
		_direction = "STR_SOUTH";
	else if (theta < -22.5)
		_direction = "STR_SOUTH_EAST";
	else
		_direction = "STR_EAST";
}

/**
 * Moves this Ufo to its destination.
 */
void Ufo::think()
{
	switch (_status)
	{
		case FLYING:
			stepTarget();
			if (reachedDestination() == true)
				setSpeed(0);
			break;

		case LANDED:
//			assert(_secondsLeft >= 5 && "Wrong time management.");
			_secondsLeft -= 5;
			break;

		case CRASHED:
//			if (_detected == false)
			_detected = true;
	}
}

/**
 * Sets this Ufo's battlescape status.
 * @param tactical - true if in battlescape (default true)
 */
void Ufo::setTactical(bool tactical)
{
	if ((_tactical = tactical) == true)
		setSpeed(0);
}

/**
 * Gets this Ufo's battlescape status.
 * @return, true if in battlescape
 */
bool Ufo::getTactical() const
{
	return _tactical;
}

/**
 * Gets the alien race currently residing in the UFO.
 * @return, address of aLien race
 */
const std::string& Ufo::getAlienRace() const
{
	return _mission->getRace();
}

/**
 * Sets a pointer to a craft that shot down this Ufo.
 * @param craft - reference to CraftID (CraftId.h)
 */
void Ufo::setShotDownByCraftId(const CraftId& craft)
{
	_shotDownByCraftId = craft;
}

/**
 * Gets the pointer to the craft that shot down this Ufo.
 * @return, CraftId (CraftId.h)
 */
CraftId Ufo::getShotDownByCraftId() const
{
	return _shotDownByCraftId;
}

/**
 * Gets the scare-factor of UFOs for activity on the Graphs.
 * @return, activity points
 */
int Ufo::getVictoryPoints() const
{
	int ret (0);
	switch (_status)
	{
		default:
		case Ufo::FLYING:		ret = 2; break; // per half-hr.
		case Ufo::LANDED:		ret = 5; break; // per hr.
		case Ufo::CRASHED:		ret = 3; break; // per hr.
		case Ufo::DESTROYED:	ret = 0;
	}

	switch (_ufoRule->getSizeType())
	{
		default:
		case UFO_VERYSMALL:	ret += 0; break;
		case UFO_SMALL:		ret += 1; break;
		case UFO_MEDIUM:	ret += 2; break;
		case UFO_LARGE:		ret += 3; break;
		case UFO_VERYLARGE:	ret += 5;
	}

	if		(_altitude == "STR_GROUND")		ret += 0; // Status _LANDED or _CRASHED included above.
	else if	(_altitude == "STR_VERY_LOW")	ret += 3;
	else if	(_altitude == "STR_LOW_UC")		ret += 2;
	else if	(_altitude == "STR_HIGH_UC")	ret += 1;
	else if	(_altitude == "STR_VERY_HIGH")	ret += 0;

	return ret;
}

/**
 * Gets this Ufo's visibility to radar detection.
 * The UFO's size and altitude affect its chances of being detected on radar.
 * @return, detection modifier
 */
int Ufo::getVisibility() const
{
	int ret (0);
	switch (_ufoRule->getSizeType())
	{
		default:
		case UFO_VERYSMALL:	ret -= 30; break;
		case UFO_SMALL:		ret -= 15; break;
		case UFO_MEDIUM:	ret -=  0; break;
		case UFO_LARGE:		ret += 15; break;
		case UFO_VERYLARGE:	ret += 30;
	}

	if		(_altitude == "STR_GROUND")		ret -= 50;
	else if	(_altitude == "STR_VERY_LOW")	ret -= 20;
	else if	(_altitude == "STR_LOW_UC")		ret -= 10;
	else if	(_altitude == "STR_HIGH_UC")	ret -=  0;
	else if	(_altitude == "STR_VERY_HIGH")	ret -= 10;

	return ret;
}

/**
 * Gets this Ufo's detect-xCom-base ability.
 * @return, detect-a-base modifier
 */
int Ufo::getDetectors() const
{
	int ret (0);
	switch (_ufoRule->getSizeType())
	{
		default:
		case UFO_VERYSMALL:	ret -= 12; break;
		case UFO_SMALL:		ret -=  8; break;
		case UFO_MEDIUM:	ret -=  5; break;
		case UFO_LARGE:		ret -=  2; break;
		case UFO_VERYLARGE:	ret -=  0;
	}

	if		(_altitude == "STR_GROUND")		ret -= 32;
	else if	(_altitude == "STR_VERY_LOW")	ret += 18;
	else if	(_altitude == "STR_LOW_UC")		ret +=  6;
	else if	(_altitude == "STR_HIGH_UC")	ret -=  9;
	else if	(_altitude == "STR_VERY_HIGH")	ret -= 19;

	return ret;
}

/**
 * Sets the mission-information of this Ufo.
 * @note The UFO will start at the first point of the trajectory. The actual
 * information is not changed here; this only sets the information on
 * behalf of the mission.
 * @param mission		- pointer to the actual mission-object
 * @param trajectory	- pointer to the actual mission-trajectory
 */
void Ufo::setUfoMissionInfo(
		AlienMission* const mission,
		const UfoTrajectory* const trajectory)
{
	assert(!_mission && mission && trajectory);

	_mission = mission;
	_mission->increaseLiveUfos();

	_trajectoryPoint = 0;
	_trajectory = trajectory;
}

/**
 * Gets the mission-type of this Ufo.
 * @note Used only for hyperwave decoder info and has been superceded elsewhere
 * by getAlienMission()->getRules().getObjective().
 * @return, reference to the type
 */
const std::string& Ufo::getUfoMissionType() const
{
	return _mission->getRules().getType();
}

/**
 * Handles destination changes and deletes old Waypoint destinations.
 * @param dest - pointer to a new destination (default nullptr)
 */
void Ufo::setDestination(Target* const dest)
{
	const Waypoint* const old (dynamic_cast<Waypoint*>(_dest));
	MovingTarget::setDestination(dest);

	delete old;
}

/**
 * Sets the intercept-port this Ufo is shooting at during a Dogfight.
 * @param target - interception-port-ID
 */
void Ufo::setShootingAt(const size_t target)
{
	_shootingAt = target;
}

/**
 * Gets the intercept-port this Ufo is shooting at during a Dogfight.
 * @return, interception-port-ID
 */
size_t Ufo::getShootingAt() const
{
	return _shootingAt;
}

/**
 * Sets this Ufo's landing-site-ID.
 * @param id - landing-site-ID
 */
void Ufo::setLandId(int id)
{
	_idLanded = id;
}

/**
 * Gets this Ufo's landing-site-ID.
 * @return, landing-site-ID
 */
int Ufo::getLandId() const
{
	return _idLanded;
}

/**
 * Sets this Ufo's crash-site-ID.
 * @param id - crash-site-ID
 */
void Ufo::setCrashId(int id)
{
	_idCrashed = id;
}

/**
 * Gets this Ufo's crash-site-ID.
 * @return, crash-site-ID
 */
int Ufo::getCrashId() const
{
	return _idCrashed;
}

/**
 * Sets this Ufo's hit-frame.
 * @param frame - hit-frame
 */
void Ufo::setHitFrame(int frame)
{
	_hitFrame = frame;
}

/**
 * Gets this Ufo's hit-frame.
 * @return, hit-frame
 */
int Ufo::getHitFrame() const
{
	return _hitFrame;
}

/**
 * Sets the remaining countdown-ticks before this Ufo attempts to escape from
 * its Dogfights.
 * @param ticks - ticks remaining
 */
void Ufo::setEscapeCountdown(int ticks)
{
	_escapeCountdown = ticks;
}

/**
 * Gets the remaining countdown-ticks before this Ufo attempts to escape from
 * its Dogfights.
 * @return, ticks remaining
 */
int Ufo::getEscapeCountdown() const
{
	return _escapeCountdown;
}

/**
 * Sets the remaining countdown-ticks before this Ufo fires its weapon.
 * @param ticks - ticks remaining
 */
void Ufo::setFireCountdown(int ticks)
{
	_fireCountdown = ticks;
}

/**
 * Gets the remaining countdown-ticks before this Ufo fires its weapon.
 * @return, ticks remaining
 */
int Ufo::getFireCountdown() const
{
	return _fireCountdown;
}

/**
 * Sets a flag denoting that this Ufo has had its fire- and escape-counters
 * decremented.
 * @note Prevents multiple interceptions from decrementing or resetting an
 * already running counter. This flag is reset in advance every time Geoscape
 * processes Dogfights.
 * @param done - true if the counters have been processed (default true)
 */
void Ufo::setTicked(bool done)
{
	_processedIntercept = done;
}

/**
 * Gets if this Ufo has had its counters decremented on a cycle of Dogfight
 * updates.
 * @return, true if the counters have already been processed
 */
bool Ufo::getTicked() const
{
	return _processedIntercept;
}

/**
 * Sets this Ufo's terrain-type when it crashes or lands.
 * @param terrainType - the terrain type-ID (default "")
 */
void Ufo::setUfoTerrainType(const std::string& terrainType)
{
	_terrain = terrainType;
}

/**
 * Gets this Ufo's terrain type once it's crashed or landed.
 * @return, the terrain type
 */
std::string Ufo::getUfoTerrainType() const
{
	return _terrain;
}

}
