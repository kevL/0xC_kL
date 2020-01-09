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

#include "Ufo.h"

//#include <algorithm>
//#include <assert.h>

#include "../fmath.h"

#include "AlienMission.h"
#include "Craft.h"
#include "SavedGame.h"
#include "Waypoint.h"

#include "../Engine/Game.h"
#include "../Engine/Exception.h"
#include "../Engine/Language.h"
//#include "../Engine/Logger.h" // DEBUG

#include "../Geoscape/Globe.h" // Globe::GLM_UFO_*

#include "../Ruleset/RuleAlienMission.h"
#include "../Ruleset/Ruleset.h"
#include "../Ruleset/RuleUfo.h"
#include "../Ruleset/UfoTrajectory.h"


namespace OpenXcom
{

/**
 * Creates the Ufo from a specified RuleUfo.
 * @param ufoRule	- pointer to RuleUfo
 * @param playSave	- pointer to the SavedGame
 */
Ufo::Ufo(
		const RuleUfo* const ufoRule,
		SavedGame* const playSave)
	:
		MovingTarget(playSave),
		_ufoRule(ufoRule),
		_idCrashed(0),
		_idLanded(0),
		_hull(ufoRule->getUfoHullCap()),
		_heading("STR_NORTH"),
		_dir(8u),
		_altitude(MovingTarget::stAltitude[3u]),
		_status(FLYING),
		_secondsLeft(0),
		_tactical(false),
		_isQuickBattle(false), // TODO: Might be able to use id=-1 for this.
		_mission(nullptr),
		_trajectory(nullptr),
		_missionPoint(0u),
		_detected(false),
		_hyperdecoded(false),
		_shootingAt(0u),
		_hitStep(0),
		_processedIntercept(false),
		_fireCountdown(0),
		_escapeCountdown(0)
{}

/**
 * Clears any Craft that were targeting this UFO and decrements the quantity of
 * UFOs that are involved in this UFO's AlienMission.
 * @note The dTor for MovingTarget handles all the Waypoint-jiggery stuff.
 */
Ufo::~Ufo()
{
	Craft* craft;
	for (size_t
			i  = 0u;
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
		_mission->decrLiveUfos();
}


/**
 ** FUNCTOR ***
 * Match AlienMission based on ID value.
 */
class MatchMissionId
	:
		public std::unary_function<const AlienMission*, bool>
{
private:
	int _id;

	public:
		/// Store ID for later comparisons.
		explicit MatchMissionId(int id)
			:
				_id(id)
		{}

		/// Match with stored ID.
		bool operator ()(const AlienMission* const mission) const
		{
			return mission->getId() == _id;
		}
};


/**
 * Loads this UFO from a YAML file.
 * @param node	- reference a YAML node
 * @param rules	- reference to the Ruleset
 */
void Ufo::loadUfo(
		const YAML::Node& node,
		const Ruleset& rules)
{
	MovingTarget::load(node);

	_id            = node["id"]           .as<int>(_id);
	_idCrashed     = node["idCrashed"]    .as<int>(_idCrashed);
	_idLanded      = node["idLanded"]     .as<int>(_idLanded);
	_hull          = node["hull"]         .as<int>(_hull);
	_altitude      = node["altitude"]     .as<std::string>(_altitude);
	_heading       = node["heading"]      .as<std::string>(_heading);
	_dir           = node["dir"]          .as<unsigned>(_dir);
	_detected      = node["detected"]     .as<bool>(_detected);
	_hyperdecoded  = node["hyperdecoded"] .as<bool>(_hyperdecoded);
	_secondsLeft   = node["secondsLeft"]  .as<int>(_secondsLeft);
	_tactical      = node["tactical"]     .as<bool>(_tactical);
	_terrain       = node["terrain"]      .as<std::string>(_terrain);
	_isQuickBattle = node["isQuickBattle"].as<bool>(_isQuickBattle);

	double
		lonTarget,
		latTarget;
	if (const YAML::Node& target = node["target"])
	{
		lonTarget = target["lon"].as<double>();
		latTarget = target["lat"].as<double>();
	}
	else
	{
		lonTarget = _lon;
		latTarget = _lat;
	}
	_target = new Waypoint();
	_target->setLongitude(lonTarget);
	_target->setLatitude(latTarget);

	if (const YAML::Node& status = node["status"])
		_status = static_cast<UfoStatus>(status.as<int>());
	else if (_hull == 0)									// I believe these cases are pointless ->
		_status = DESTROYED;								// ie. 'status' is always saved.
	else if (_hull <= (_ufoRule->getUfoHullCap() >> 1u))
		_status = CRASHED;
	else if (_altitude == MovingTarget::stAltitude[0u])
		_status = LANDED;
	else
		_status = FLYING;									// <- already done in cTor init.

	if (_isQuickBattle == false)
	{
		const int missionId (node["mission"].as<int>());
		std::vector<AlienMission*>::const_iterator mission (std::find_if(
																	_playSave->getAlienMissions().begin(),
																	_playSave->getAlienMissions().end(),
																	MatchMissionId(missionId)));
		if (mission == _playSave->getAlienMissions().end())
		{
			throw Exception("Unknown mission, save file is corrupt.");
		}
		_mission = *mission;

		_trajectory = rules.getUfoTrajectory(node["trajectory"].as<std::string>());
		_missionPoint = node["trjPoint"].as<size_t>(_missionPoint);
	}

	_fireCountdown   = node["fireCountdown"]  .as<int>(_fireCountdown);
	_escapeCountdown = node["escapeCountdown"].as<int>(_escapeCountdown);

	if (_tactical == true) setSpeed();
}

/**
 * Saves this UFO to a YAML file.
 * @return, YAML node
 */
YAML::Node Ufo::save() const
{
	YAML::Node node (MovingTarget::save());

	node["type"] = _ufoRule->getType();
	node["id"]   = _id;

	if      (_idCrashed != 0) node["idCrashed"] = _idCrashed;
	else if (_idLanded  != 0) node["idLanded"]  = _idLanded;

	if (_terrain.empty() == false) node["terrain"] = _terrain;

	node["altitude"] = _altitude;
	node["heading"]  = _heading;
	node["dir"]      = _dir;
	node["status"]   = static_cast<int>(_status);
	node["hull"]     = _hull;

	if (_detected != false)     node["detected"]     = _detected;
	if (_hyperdecoded != false) node["hyperdecoded"] = _hyperdecoded;
	if (_secondsLeft != 0)      node["secondsLeft"]  = _secondsLeft;
	if (_tactical != false)     node["tactical"]     = _tactical;

	if (_isQuickBattle == false) // TODO: Do not save trajectory-info if UFO was shot down.
	{
		node["mission"]    = _mission->getId();
		node["trajectory"] = _trajectory->getType();
		node["trjPoint"]   = _missionPoint;
	}
	else
		node["isQuickBattle"] = _isQuickBattle;

	switch (_status)
	{
		case FLYING:
		case LANDED:
			if (_fireCountdown != 0)   node["fireCountdown"]   = _fireCountdown;
			if (_escapeCountdown != 0) node["escapeCountdown"] = _escapeCountdown;
	}

	return node;
}

/**
 * Saves this UFO's identificator to a YAML file.
 * @return, YAML node
 */
YAML::Node Ufo::saveIdentificator() const
{
	YAML::Node node (Target::save());

	node["type"] = Target::stTarget[0u];
	node["id"]   = _id;

	return node;
}

/**
 * Gets the rule for this UFO's type.
 * @return, pointer to RuleUfo
 */
const RuleUfo* Ufo::getRules() const
{
	return _ufoRule;
}

/**
 * Changes the rule for this UFO's type.
 * @warning FOR QUICK-BATTLE USE ONLY!
 * @param ufoRule - pointer to RuleUfo
 */
void Ufo::changeRules(const RuleUfo* const ufoRule)
{
	_ufoRule = ufoRule;
}

/**
 * Gets this UFO's unique identifier.
 * @param lang	- pointer to Language to get strings from
 * @param id	- true to show the Id (default true)
 * @return, label
 */
std::wstring Ufo::getLabel(
		const Language* const lang,
		bool id) const
{
	switch (_status)
	{
		case FLYING:
		case DESTROYED: // Destroyed also means leaving Earth.
			if (id == true)
				return lang->getString("STR_UFO_").arg(_id);

			return lang->getString("STR_UFO");

		case LANDED:
			if (id == true)
				return lang->getString("STR_LANDING_SITE_").arg(_idLanded);

			return lang->getString("STR_LANDING_SITE");

		case CRASHED:
			if (id == true)
				return lang->getString("STR_CRASH_SITE_").arg(_idCrashed);

			return lang->getString("STR_CRASH_SITE");
	}
	return L"";
}

/**
 * Gets the globe-marker for this UFO.
 * @return, marker-ID (-1 if not detected)
 */
int Ufo::getMarker() const
{
	if (_detected == true || _playSave->getDebugGeo() == true)
	{
		const int id (_ufoRule->getMarker());
		if (id != -1) return id; // NOTE: This does not differentiate between Flying/Crashed/Landed.

		switch (_status)
		{
			case Ufo::FLYING:  return Globe::GLM_UFO_FLYING;
			case Ufo::LANDED:  return Globe::GLM_UFO_LANDED;
			case Ufo::CRASHED: return Globe::GLM_UFO_CRASHED;
		}
	}
	return -1;
}

/**
 * Sets this Ufo's hull after inflicted hurt.
 * @param inflict - inflicted hurt
 */
void Ufo::setUfoHull(int inflict)
{
	if ((_hull -= inflict) <= 0)
	{
		_hull = 0;
		_status = DESTROYED;
	}
	else if (isCrashed() == true)
		_status = CRASHED;
}

/**
 * Gets this Ufo's hull-percentage.
 * @return, hull pct
 */
int Ufo::getUfoHullPct() const
{
	if (_hull == 0) return 0;

	return static_cast<int>(std::ceil(
		   static_cast<float>(_hull) / static_cast<float>(_ufoRule->getUfoHullCap()) * 100.f));
}

/**
 * Checks if this Ufo took enough hurt to crash it.
 * @return, true if crashed
 */
bool Ufo::isCrashed() const
{
	return _hull <= (_ufoRule->getUfoHullCap() >> 1u);
}

/**
 * Checks if this Ufo took enough hurt to destroy it.
 * @return, true if destroyed
 */
bool Ufo::isDestroyed() const
{
	return _hull == 0;
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
void Ufo::setHyperdecoded(bool hyperdecoded)
{
	_hyperdecoded = hyperdecoded;
}

/**
 * Gets whether this Ufo has been detected by hyper-wave.
 * @return, true if hyperwave-detected
 */
bool Ufo::getHyperdecoded() const
{
	return _hyperdecoded;
}

/**
 * Sets the amount of remaining seconds this UFO has left on the ground.
 * @note After this many seconds this Ufo will take off if landed or disappear
 * if crashed.
 * @param sec - time in seconds
 */
void Ufo::setSecondsLeft(int sec)
{
	_secondsLeft = std::max(0, sec);
}

/**
 * Gets the amount of remaining seconds this UFO has left on the ground.
 * @note After this many seconds this Ufo will take off if landed or disappear
 * if crashed.
 * @return, amount of seconds
 */
int Ufo::getSecondsLeft() const
{
	return _secondsLeft;
}

/**
 * Reduces this UFO's seconds left by 5.
 * @return, true if 0 seconds left
 */
bool Ufo::reduceSecondsLeft()
{
	if ((_secondsLeft -= 5) <= 0)
	{
		_secondsLeft = 0;
		return true;
	}
	return false;
}

/**
 * Sets the current altitude and status of this UFO.
 * @param altitude - altitude
 */
void Ufo::setAltitude(const std::string& altitude)
{
	if ((_altitude = altitude) != MovingTarget::stAltitude[0u])
		_status = FLYING;
	else if (isCrashed() == true)
		_status = CRASHED;
	else
		_status = LANDED;
}

/**
 * Gets the current altitude of this UFO.
 * @return, altitude
 */
std::string Ufo::getAltitude() const
{
	return _altitude;
}

/**
 * Gets the current altitude of this UFO as an integer.
 * @return, alititude as an integer
 */
unsigned Ufo::getAltitudeInt() const
{
	if (_altitude == MovingTarget::stAltitude[0u]) return 0u;
	if (_altitude == MovingTarget::stAltitude[1u]) return 1u;
	if (_altitude == MovingTarget::stAltitude[2u]) return 2u;
	if (_altitude == MovingTarget::stAltitude[3u]) return 3u;

	return 4u;
}

/**
 * Gets the current heading of this UFO.
 * @return, heading
 */
std::string Ufo::getHeading() const
{
	return _heading;
}

/**
 * Gets the current heading of this UFO as an integer.
 * @return, heading as an integer
 */
unsigned Ufo::getHeadingInt() const
{
	return _dir;
}

/**
 * Calculates the heading/direction for this Ufo based on the current raw speed
 * and destination.
 * @sa Craft::getHeadingInt().
 */
void Ufo::calculateSpeed() // private.
{
	MovingTarget::calculateSpeed();

	const double
		x ( _speedLon),
		y (-_speedLat);

	if (AreSame(x, 0.) == true || AreSame(y, 0.) == true) // This section guards vs. divide-by-zero.
	{
		if (AreSameTwo(x, 0., y, 0.) == true)
		{
			_heading = "STR_NONE_UC";
			_dir = 0u;
		}
		else if (AreSame(x, 0.) == true)
		{
			if (y > 0.)
			{
				_heading = "STR_NORTH";
				_dir = 8u;
			}
			else
			{
				_heading = "STR_SOUTH";
				_dir = 4u;
			}
		}
		else
		{
			if (x > 0.)
			{
				_heading = "STR_EAST";
				_dir = 2u;
			}
			else
			{
				_heading = "STR_WEST";
				_dir = 6u;
			}
		}
		return;
	}

	double theta (std::atan2(y,x)); // theta is radians.
	// Convert radians to degrees so i don't go bonkers;
	// ie. KILL IT WITH FIRE!!1@!
	// NOTE: This is between +/- 180 deg.
	theta *= 180. / M_PI;

	if (theta > 157.5 || theta < -157.5)
	{
		_heading = "STR_WEST";
		_dir = 6u;
	}
	else if (theta > 112.5)
	{
		_heading = "STR_NORTH_WEST";
		_dir = 7u;
	}
	else if (theta > 67.5)
	{
		_heading = "STR_NORTH";
		_dir = 8u;
	}
	else if (theta > 22.5)
	{
		_heading = "STR_NORTH_EAST";
		_dir = 1u;
	}
	else if (theta < -112.5)
	{
		_heading = "STR_SOUTH_WEST";
		_dir = 5u;
	}
	else if (theta < -67.5)
	{
		_heading = "STR_SOUTH";
		_dir = 4u;
	}
	else if (theta < -22.5)
	{
		_heading = "STR_SOUTH_EAST";
		_dir = 3u;
	}
	else
	{
		_heading = "STR_EAST";
		_dir = 2u;
	}
}

/**
 * Sets this Ufo's battlescape status.
 * @param tactical - true if in battlescape (default true)
 */
void Ufo::setTactical(bool tactical)
{
	if ((_tactical = tactical) == true)
		setSpeed();
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
 * Sets the UFO as the UFO in a quick-battle.
 */
void Ufo::setQuickBattle()
{
	_isQuickBattle = true;
}

/**
 * Gets the alien race currently residing in this UFO.
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
 * Gets the scare-factor of this Ufo for activity on the Graphs.
 * @return, activity points
 */
int Ufo::getActivityPoints() const
{
	int ret (_ufoRule->getActivityScore());
	switch (_status)
	{
		default:
		case Ufo::FLYING:    ret += 1; break; // per half-hr.	// 2
		case Ufo::LANDED:    ret += 3; break; // per half-hr.	// 5
		case Ufo::CRASHED:   ret += 2; break; // per hr.		// 3
		case Ufo::DESTROYED: return 0;
	}

	switch (_ufoRule->getSizeType())
	{
		case UFO_VERYSMALL: ret += 0; break;
		case UFO_SMALL:     ret += 1; break;
		case UFO_MEDIUM:    ret += 2; break;
		case UFO_LARGE:     ret += 3; break;
		case UFO_VERYLARGE: ret += 6;
	}

	if      (_altitude == MovingTarget::stAltitude[0u]) ret += 0; // Status _LANDED or _CRASHED included above.
	else if (_altitude == MovingTarget::stAltitude[1u]) ret += 3;
	else if (_altitude == MovingTarget::stAltitude[2u]) ret += 2;
	else if (_altitude == MovingTarget::stAltitude[3u]) ret += 1;
	else if (_altitude == MovingTarget::stAltitude[4u]) ret += 0;

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
		case UFO_VERYSMALL: ret -= 30; break;
		case UFO_SMALL:     ret -= 15; break;
		case UFO_MEDIUM:    ret -=  0; break;
		case UFO_LARGE:     ret += 15; break;
		case UFO_VERYLARGE: ret += 30;
	}

	if      (_altitude == MovingTarget::stAltitude[0u]) ret -= 50;
	else if (_altitude == MovingTarget::stAltitude[1u]) ret -= 20;
	else if (_altitude == MovingTarget::stAltitude[2u]) ret -= 10;
	else if (_altitude == MovingTarget::stAltitude[3u]) ret -=  0;
	else if (_altitude == MovingTarget::stAltitude[4u]) ret -= 10;

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
		case UFO_VERYSMALL: ret -= 12; break;
		case UFO_SMALL:     ret -=  8; break;
		case UFO_MEDIUM:    ret -=  5; break;
		case UFO_LARGE:     ret -=  2; break;
		case UFO_VERYLARGE: ret -=  0;
	}

	if      (_altitude == MovingTarget::stAltitude[0u]) ret -= 32;
	else if (_altitude == MovingTarget::stAltitude[1u]) ret += 18;
	else if (_altitude == MovingTarget::stAltitude[2u]) ret +=  6;
	else if (_altitude == MovingTarget::stAltitude[3u]) ret -=  9;
	else if (_altitude == MovingTarget::stAltitude[4u]) ret -= 19;

	return ret;
}

/**
 * Sets the mission-information of this Ufo.
 * @note The UFO will start at the first point of its trajectory. The actual
 * information is not changed here; this only sets the information on behalf of
 * the mission.
 * @param mission		- pointer to the actual mission-object
 * @param trajectory	- pointer to the actual mission-trajectory
 */
void Ufo::setUfoMissionInfo(
		AlienMission* const mission,
		const UfoTrajectory* const trajectory)
{
//	assert(!_mission && mission && trajectory);

	_mission = mission;
	_mission->incrLiveUfos();

	_trajectory = trajectory;
	_missionPoint = 0u;
}

/**
 * Gets the mission-type of this Ufo.
 * @note Used only for hyperwave decoder info and has been superceded elsewhere
 * by getAlienMission()->getRules().getObjectiveType().
 * @return, reference to the type
 */
const std::string& Ufo::getUfoMissionType() const
{
	return _mission->getRules().getType();
}

/**
 * Sets the intercept-slot this Ufo is shooting at during a Dogfight.
 * @param slot - intercept-slot
 */
void Ufo::setShootingAt(size_t slot)
{
	_shootingAt = slot;
}

/**
 * Gets the intercept-slot this Ufo is shooting at during a Dogfight.
 * @return, intercept-slot
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
 * Sets this Ufo's hit-step.
 * @param step - hit-step
 */
void Ufo::setHitStep(int step)
{
	_hitStep = step;
}

/**
 * Gets this Ufo's hit-step.
 * @return, hit-step
 */
int Ufo::getHitStep() const
{
	return _hitStep;
}

/**
 * Sets the remaining countdown-ticks before this Ufo attempts to escape from
 * its Dogfights.
 * @param ticks - ticks remaining
 */
void Ufo::setEscapeTicks(int ticks)
{
	_escapeCountdown = ticks;
}

/**
 * Gets the remaining countdown-ticks before this Ufo attempts to escape from
 * its Dogfights.
 * @return, ticks remaining
 */
int Ufo::getEscapeTicks() const
{
	return _escapeCountdown;
}

/**
 * Sets the remaining countdown-ticks before this Ufo fires its weapon.
 * @param ticks - ticks remaining
 */
void Ufo::setFireTicks(int ticks)
{
	_fireCountdown = ticks;
}

/**
 * Gets the remaining countdown-ticks before this Ufo fires its weapon.
 * @return, ticks remaining
 */
int Ufo::getFireTicks() const
{
	return _fireCountdown;
}

/**
 * Sets a flag denoting that this Ufo has had its fire- and escape-counters
 * decremented.
 * @note Prevents multiple interceptions from decrementing or resetting an
 * already running counter. This flag is reset in advance every time Geoscape
 * processes Dogfights.
 * @param tick - true if the counters have been processed (default true)
 */
void Ufo::setTicked(bool tick)
{
	_processedIntercept = tick;
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
