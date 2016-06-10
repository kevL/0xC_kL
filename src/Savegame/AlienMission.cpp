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

#include "AlienMission.h"

//#include <algorithm>
//#include <assert.h>

#include "AlienBase.h"
#include "Base.h"
#include "Country.h"
#include "Craft.h"
#include "Region.h"
#include "SavedGame.h"
#include "TerrorSite.h"
#include "Ufo.h"
#include "Waypoint.h"

#include "../Engine/Exception.h"
#include "../Engine/Game.h"
#include "../Engine/Logger.h"
#include "../Engine/RNG.h"

#include "../Geoscape/Globe.h"

#include "../Ruleset/RuleAlienDeployment.h"
#include "../Ruleset/RuleAlienMission.h"
#include "../Ruleset/RuleCity.h"
#include "../Ruleset/RuleCountry.h"
#include "../Ruleset/RuleGlobe.h"
#include "../Ruleset/RuleRegion.h"
#include "../Ruleset/Ruleset.h"
#include "../Ruleset/RuleTexture.h"
#include "../Ruleset/RuleUfo.h"
#include "../Ruleset/UfoTrajectory.h"


namespace OpenXcom
{

/**
 * Creates the AlienMission.
 * @param missionRule	- reference to RuleAlienMission
 * @param gameSave		- reference to the SavedGame
 */
AlienMission::AlienMission(
		const RuleAlienMission& missionRule,
		SavedGame& gameSave)
	:
		_missionRule(missionRule),
		_gameSave(gameSave),
		_waveCount(0u),
		_ufoCount(0),
		_spawnTime(0),
		_liveUfos(0),
		_id(0),
		_aBase(nullptr),
		_success(false),
		_terrorZone(std::numeric_limits<size_t>::max())
{}

/**
 * dTor.
 */
AlienMission::~AlienMission()
{}


/**
 ** FUNCTOR ***
 */
class MatchById
	:
		public std::unary_function<const AlienBase*, bool>
{
private:
	int _id;

	public:
		/// Cache the ID.
		explicit MatchById(int id)
			:
				_id(id)
		{}

		/// Match with cached ID.
		bool operator() (const AlienBase* const ab) const
		{
			return ab->getId() == _id;
		}
};

/**
 * Loads this AlienMission from a YAML file.
 * @param node - reference to a YAML node
 */
void AlienMission::load(const YAML::Node& node)
{
	_id			= node["id"]		.as<int>(_id);
	_region		= node["region"]	.as<std::string>(_region);
	_race		= node["race"]		.as<std::string>(_race);
	_waveCount	= node["waveCount"]	.as<size_t>(_waveCount);
	_ufoCount	= node["ufoCount"]	.as<int>(_ufoCount);
	_spawnTime	= node["spawnTime"]	.as<int>(_spawnTime);
	_liveUfos	= node["liveUfos"]	.as<int>(_liveUfos);
	_terrorZone	= node["terrorZone"].as<size_t>(_terrorZone);
	_success	= node["success"]	.as<bool>(_success);

	if (const YAML::Node& baseId = node["alienBase"])
	{
		const int id (baseId.as<int>());
		const std::vector<AlienBase*>::const_iterator aBase (std::find_if(
																	_gameSave.getAlienBases()->begin(),
																	_gameSave.getAlienBases()->end(),
																	MatchById(id)));
		if (aBase == _gameSave.getAlienBases()->end())
		{
			throw Exception("Corrupted save: Invalid aLien Base for AlienMission.");
		}
		_aBase = *aBase;
	}
}

/**
 * Saves this AlienMission to a YAML file.
 * @return, YAML node
 */
YAML::Node AlienMission::save() const
{
	YAML::Node node;

	node["type"]		= _missionRule.getType();
	node["id"]			= _id;
	node["region"]		= _region;
	node["race"]		= _race;
	node["waveCount"]	= _waveCount;
	node["ufoCount"]	= _ufoCount;
	node["spawnTime"]	= _spawnTime;
	node["liveUfos"]	= _liveUfos;

	if (_terrorZone != std::numeric_limits<size_t>::max())
		node["terrorZone"] = _terrorZone;

	if (_success == true)
		node["success"] = _success;

	if (_aBase != nullptr)
		node["alienBase"] = _aBase->getId();

	return node;
}

/**
 * Assigns a unique-ID to this AlienMission.
 * @note It is an error to assign two IDs to the same mission.
 * @param id - the ID to assign
 */
void AlienMission::setId(int id)
{
//	assert(_id == 0 && "Reassigning ID!");
	_id = id;
}

/**
 * Gets the unique-ID of this AlienMission.
 * @return, the unique-ID assigned to this mission
 */
int AlienMission::getId() const
{
//	assert(_id != 0 && "Uninitialized mission!");
	return _id;
}

/**
 * Sets this AlienMission's Region.
 * @note If the region is incompatible with actually carrying out an attack use
 * the fallback region as defined in the ruleset. This is a slight difference
 * from the original which just defaulted to zone[0] North America.
 * @param region	- reference to the region to try to set the mission to
 * @param rules		- reference to the ruleset in case an alt-region needs to be swapped in
 */
void AlienMission::setRegion(
		const std::string& region,
		const Ruleset& rules)
{
	if (rules.getRegion(region)->getMissionRegion().empty() == false)
		_region = rules.getRegion(region)->getMissionRegion();
	else
		_region = region;
}

/**
 *
 * @note The new time must be a multiple of 30 minutes and more than 0. Calling
 * this on a finished mission has no effect.
 * @param minutes - the minutes until the next UFO wave will spawn
 */
void AlienMission::setWaveCountdown(int minutes)
{
//	assert(minutes != 0 && minutes % 30 == 0);
	if (isOver() == false)
		_spawnTime = minutes;
}

/**
 * Checks if this AlienMission is over and can be safely removed.
 * @note The mission is over if it will spawn no more UFOs and it has no UFOs
 * still in the game.
 * @return, true if this AlienMission can be deleted
 */
bool AlienMission::isOver() const
{
	if (_liveUfos == 0
		&& _waveCount == _missionRule.getWaveTotal()
		&& _missionRule.getObjectiveType() != alm_INFILT) // Infiltrations continue for ever. Almost.
// ->		|| RNG::percent(static_cast<int>(_gameSave.getDifficulty()) * 20) == false))
	{
		return true;
	}
	return false;
}

/**
 * Starts this AlienMission.
 * @param countdown - countdown till next UFO (default 0)
 */
void AlienMission::start(int countdown)
{
	_waveCount = 0u;
	_ufoCount =
	_liveUfos = 0;

	switch (countdown)
	{
		case 0:
			calcCountdown(0u);
			break;
		default:
			_spawnTime = countdown;
	}
}

/**
 * Advances this AlienMission.
 * @param game	- reference to the Game
 * @param globe	- reference to the Globe
 */
void AlienMission::think(
		const Game& game,
		const Globe& globe)
{
	if (_waveCount < _missionRule.getWaveTotal())
	{
		if (_spawnTime > 30)
			_spawnTime -= 30;
		else
		{
			const Ruleset& rules (*game.getRuleset());
			const MissionWave& wave (_missionRule.getWave(_waveCount));
			const UfoTrajectory& trajectory (*rules.getUfoTrajectory(wave.trajectory));

			Ufo* const ufo (createUfo(
									rules,
									globe,
									wave,
									trajectory));

			if (ufo != nullptr)										// a UFO hath spawned!
				_gameSave.getUfos()->push_back(ufo);
			else if ((rules.getUfo(wave.ufoType) == nullptr			// a terror-site to spawn directly
					&& rules.getDeployment(wave.ufoType) != nullptr
					&& rules.getDeployment(wave.ufoType)->getMarkerType().empty() == false)
				|| (_missionRule.getObjectiveType() == alm_TERROR	// or spawn a site according to the terrain
					&& wave.isObjective == true))
			{
				size_t id;
				if (_missionRule.getObjectiveZone() == std::numeric_limits<size_t>::max())
					id = trajectory.getZone(0u);
				else
					id = _missionRule.getObjectiveZone();
				const std::vector<MissionArea> areas (rules.getRegion(_region)->getMissionZones().at(id).areas);

				if (_terrorZone == std::numeric_limits<size_t>::max())
					id = RNG::pick(areas.size());
				else
					id = _terrorZone;
				const MissionArea area (areas.at(id));

				const RuleTexture* const texture (rules.getGlobe()->getTextureRule(area.texture));

				const RuleAlienDeployment* ruleDeploy;
				if (rules.getDeployment(wave.ufoType) != nullptr)
					ruleDeploy = rules.getDeployment(wave.ufoType);
				else
					ruleDeploy = rules.getDeployment(texture->getTextureDeployment());

				createTerror(ruleDeploy, area);
			}

			if (++_ufoCount == wave.ufoTotal)
			{
				_ufoCount = 0;
				++_waveCount;
			}

			switch (_missionRule.getObjectiveType())
			{
				case alm_INFILT: // Infiltrations loop for ever.
					_waveCount = 0u;
					break;

				case alm_SCORE:
				case alm_BASE:
				case alm_TERROR:
				case alm_RETAL:
				case alm_SUPPLY:
					if (_waveCount != _missionRule.getWaveTotal())
						calcCountdown(_waveCount);
			}
		}
	}
}

/**
 * Calculates time remaining until the next wave of this AlienMission spawns.
 * @note These come in increments of 30sec (or min?) apiece.
 * @param waveId - the wave to check
 */
void AlienMission::calcCountdown(size_t waveId) // private.
{
	_spawnTime = _missionRule.getWave(waveId).spawnTimer / 30;
	_spawnTime = (RNG::generate(0, _spawnTime) + (_spawnTime >> 1u)) * 30;
}

/**
 * Spawns a UFO according this AlienMission's rules.
 * @note Some code is duplicated between cases but that's ok for now. It's on
 * different code paths and the function is MUCH easier to read written this way.
 * @param rules			- reference to the ruleset
 * @param globe			- reference to the globe for land checks
 * @param wave			- reference to the wave for the desired UFO type
 * @param trajectory	- reference to the rule for the desired UFO trajectory
 * @return, pointer to the spawned UFO; if the mission does not spawn a UFO return nullptr
 */
Ufo* AlienMission::createUfo( // private.
		const Ruleset& rules,
		const Globe& globe,
		const MissionWave& wave,
		const UfoTrajectory& trajectory)
{
	const RuleUfo* ufoRule (rules.getUfo(wave.ufoType));

	std::pair<double, double> coord;
	Waypoint* wp;
	Ufo* ufo;

	switch (_missionRule.getObjectiveType())
	{
		case alm_RETAL:
		{
			std::vector<Base*> baseTargets;
			for (std::vector<Base*>::const_iterator
					i = _gameSave.getBases()->begin();
					i != _gameSave.getBases()->end();
					++i)
			{
				if ((*i)->getBaseExposed() == true
					&& rules.getRegion(_region)->insideRegion(
														(*i)->getLongitude(),
														(*i)->getLatitude()) == true)
				{
					baseTargets.push_back(*i);
				}
			}

			if (baseTargets.empty() == false) // Spawn a battleship straight for an exposed XCOM Base.
			{
				const RuleUfo& battleshipRule (*rules.getUfo(_missionRule.getObjectiveUfo()));
				const UfoTrajectory& trjBattleship (*rules.getUfoTrajectory(UfoTrajectory::RETALIATION_ASSAULT_RUN));
				const RuleRegion& regionRule (*rules.getRegion(_region));

				ufo = new Ufo(
							&battleshipRule,
							&_gameSave);
				ufo->setUfoMissionInfo(
									this,
									&trjBattleship);

				if (trajectory.getAltitude(0u) == MovingTarget::stAltitude[0u])
					coord = coordsLand(
									globe,
									regionRule,
									trajectory.getZone(0u));
				else
					coord = regionRule.getZonePoint(trajectory.getZone(0u));

				ufo->setLongitude(coord.first);
				ufo->setLatitude(coord.second);

				ufo->setAltitude(trjBattleship.getAltitude(0u));
				ufo->setSpeed(static_cast<int>(std::ceil(
							  trjBattleship.getSpeedPct(0u) * static_cast<float>(battleshipRule.getMaxSpeed()))));

				wp = new Waypoint();
				const size_t pick (RNG::pick(baseTargets.size()));
				wp->setLongitude(baseTargets[pick]->getLongitude());
				wp->setLatitude(baseTargets[pick]->getLatitude());

				ufo->setDestination(wp);
				return ufo;
			}
			break; // if no XCOM Base is exposed create a regular UFO below_
		}

		case alm_SUPPLY: // check for an AlienBase to supply.
			if (ufoRule != nullptr
				&& (_aBase != nullptr || wave.isObjective == false))
			{
				ufo = new Ufo(
							ufoRule,
							&_gameSave);
				ufo->setUfoMissionInfo( // destination is always an alien base.
									this,
									&trajectory);
				const RuleRegion& regionRule (*rules.getRegion(_region));

				if (trajectory.getAltitude(0u) == MovingTarget::stAltitude[0u])
					coord = coordsLand(
									globe,
									regionRule,
									trajectory.getZone(0u));
				else
					coord = regionRule.getZonePoint(trajectory.getZone(0u));

				ufo->setLongitude(coord.first);
				ufo->setLatitude(coord.second);

				ufo->setAltitude(trajectory.getAltitude(0u));
				ufo->setSpeed(static_cast<int>(std::ceil(
							  trajectory.getSpeedPct(0u) * static_cast<float>(ufoRule->getMaxSpeed()))));

				if (trajectory.getAltitude(1u) == MovingTarget::stAltitude[0u])
				{
					if (wave.isObjective == true) // Supply ships on supply missions land on bases, ignore trajectory zone.
					{
						coord.first = _aBase->getLongitude();
						coord.second = _aBase->getLatitude();
					}
					else
						coord = coordsLand( // Other ships can land where they want.
										globe,
										regionRule,
										trajectory.getZone(1u));
				}
				else
					coord = regionRule.getZonePoint(trajectory.getZone(1u));

				wp = new Waypoint();
				wp->setLongitude(coord.first);
				wp->setLatitude(coord.second);

				ufo->setDestination(wp);
				return ufo;
			}
			return nullptr; // No base to supply!
	}

	if (ufoRule != nullptr) // else create a UFO according to sequence
	{
		ufo = new Ufo(
					ufoRule,
					&_gameSave);
		ufo->setUfoMissionInfo(
							this,
							&trajectory);
		const RuleRegion& regionRule (*rules.getRegion(_region));
		coord = coordsWaypoint(
						trajectory,
						0u,
						globe,
						regionRule);
		ufo->setLongitude(coord.first);
		ufo->setLatitude(coord.second);

		ufo->setAltitude(trajectory.getAltitude(0));
		if (trajectory.getAltitude(0u) == MovingTarget::stAltitude[0u])
			ufo->setSecondsLeft(trajectory.groundTimer() * 5);

		float speedPct;
		if (trajectory.getWaypointTotal() > 1u)
			speedPct = RNG::generate(
								trajectory.getSpeedPct(0u),
								trajectory.getSpeedPct(1u));
		else
			speedPct = trajectory.getSpeedPct(0u);

		ufo->setSpeed(static_cast<int>(std::ceil(
					  speedPct * static_cast<float>(ufoRule->getMaxSpeed()))));

		coord = coordsWaypoint(
							trajectory,
							1u,
							globe,
							regionRule);
		wp = new Waypoint();
		wp->setLongitude(coord.first);
		wp->setLatitude(coord.second);

		ufo->setDestination(wp);
		return ufo;
	}
	return nullptr;
}


/**
 ** FUNCTOR ***
 * @brief Match a base from its coordinates.
 * @note This function object uses coordinates to match a base. Helper for
 * ufoReachedWaypoint().
 */
class MatchBaseCoordinates
	:
		public std::unary_function<const Base*, bool>
{
private:
	double
		_lon,
		_lat;

	public:
		///
		MatchBaseCoordinates(
				double lon,
				double lat)
			:
				_lon(lon),
				_lat(lat)
		{}

		///
		bool operator() (const Base* const base) const
		{
			return AreSame(base->getLongitude(), _lon)
				&& AreSame(base->getLatitude(),  _lat);
		}
};


/**
 * One of this AlienMission's UFOs arrived at its current destination.
 * @note It takes care of sending the UFO to the next waypoint, landing
 * UFOs and marking them for removal as required. It must set the data in a way
 * that the rest of the code understands what to do. Doh!!
 * @param ufo	- reference to the Ufo that reached its waypoint
 * @param rules	- reference to the Ruleset
 * @param globe	- reference to the earth globe required to get access to land checks
 */
void AlienMission::ufoReachedWaypoint(
		Ufo& ufo,
		const Ruleset& rules,
		const Globe& globe)
{
	const UfoTrajectory& trajectory (ufo.getTrajectory());
	const size_t
		wpId (ufo.getTrajectoryPoint()),
		wpId_next (wpId + 1u);

	if (wpId_next < trajectory.getWaypointTotal())
	{
		ufo.setAltitude(trajectory.getAltitude(wpId_next));
		ufo.setTrajectoryPoint(wpId_next);

		const RuleRegion& regionRule (*rules.getRegion(_region));

		Waypoint* const wp (new Waypoint());
		const std::pair<double, double> coord (coordsWaypoint(
														trajectory,
														wpId_next,
														globe,
														regionRule));
		wp->setLongitude(coord.first);
		wp->setLatitude(coord.second);
		ufo.setDestination(wp);

		if (ufo.getAltitude() == MovingTarget::stAltitude[0u]) // UFO landed.
		{
			size_t wave;
			switch (_waveCount)
			{
				case 0u: // restart AlienMission -> if (RNG::percent(static_cast<int>(_gameSave.getDifficulty()) * 20) == false)
					wave = _missionRule.getWaveTotal() - 1u;
					break;
				default:
					wave = _waveCount - 1u;
			}
			// NOTE: 'wave' has to be reduced by one because think() has already advanced it past current, I suppose.

			if (_missionRule.getWave(wave).isObjective == true					// destroy UFO & replace with TerrorSite.
				&& trajectory.getZone(wpId) == _missionRule.getObjectiveZone())	// NOTE: Supply-missions bypasses this although it has (objective=true)
			{																	// because it does not have an 'objectiveZone' set in its rule.
				addScore( // alm_TERROR
					ufo.getLongitude(),
					ufo.getLatitude());

				ufo.setUfoStatus(Ufo::DESTROYED);

				// note: Looks like they're having probls with getting a mission wpId:
//				MissionArea area (regionRule.getMissionZones().at(trajectory.getZone(wpId)).areas.at(_terrorZone));
				const MissionArea area (regionRule.getTerrorArea(
															trajectory.getZone(wpId),
															dynamic_cast<Target*>(&ufo)));

				const RuleAlienDeployment* ruleDeploy (rules.getDeployment(_missionRule.getTerrorType()));
				if (ruleDeploy == nullptr)
				{
					const RuleTexture* const texture (rules.getGlobe()->getTextureRule(area.texture));
					ruleDeploy = rules.getDeployment(texture->getTextureDeployment());
				}

				TerrorSite* const site (createTerror(ruleDeploy, area));
				if (site != nullptr)
				{
					_gameSave.getTerrorSites()->push_back(site);

					for (std::vector<Target*>::const_iterator
							i = ufo.getTargeters()->begin();
							i != ufo.getTargeters()->end();
							)
					{
						Craft* const craft (dynamic_cast<Craft*>(*i));
						if (craft != nullptr)
						{
							craft->setDestination(site);
							i = ufo.getTargeters()->begin();
						}
						else
							++i;
					}
				}
			}
			else if (trajectory.getId() == UfoTrajectory::RETALIATION_ASSAULT_RUN)	// remove UFO, replace with Base defense.
			{																		// Ignore what the trajectory might say, this is a base defense.
				ufo.setDetected(false);
				const std::vector<Base*>::const_iterator i (std::find_if(			// Only spawn mission if the base is still there.
																	_gameSave.getBases()->begin(),
																	_gameSave.getBases()->end(),
																	MatchBaseCoordinates(
																					ufo.getLongitude(),
																					ufo.getLatitude())));
				if (i != _gameSave.getBases()->end())
					ufo.setDestination(*i);
				else
					ufo.setUfoStatus(Ufo::DESTROYED);
			}
			else // Set timer for UFO on the ground.
			{
				if (globe.insideLand(
								ufo.getLongitude(),
								ufo.getLatitude()) == true)
				{
					ufo.setSecondsLeft(trajectory.groundTimer() * 5);

					if (ufo.getDetected() == true && ufo.getLandId() == 0)
						ufo.setLandId(_gameSave.getCanonicalId(Target::stTarget[5u]));
				}
				else // there's nothing to land on
					ufo.setSecondsLeft(5);
			}
		}
		else // UFO is Flying.
		{
			ufo.setLandId(0);

			float speedPct (RNG::generate(
									trajectory.getSpeedPct(wpId),
									trajectory.getSpeedPct(wpId_next)));
			ufo.setSpeed(static_cast<int>(std::ceil(
						 speedPct * static_cast<float>(ufo.getRules()->getMaxSpeed()))));
		}
	}
	else // UFO left earth's atmosphere.
	{
		ufo.setDetected(false);
		ufo.setUfoStatus(Ufo::DESTROYED);
	}
}

/**
 * Attempts to spawn a Terror Site at a given location.
 * @param ruleDeploy	- pointer to the RuleAlienDeployment
 * @param area			- reference an area of the globe
 * @return, pointer to the site
 */
TerrorSite* AlienMission::createTerror( // private.
		const RuleAlienDeployment* const ruleDeploy,
		const MissionArea& area)
{
	if (ruleDeploy != nullptr)
	{
		TerrorSite* const site (new TerrorSite(
											&_missionRule,
											ruleDeploy));
		site->setLongitude(RNG::generate(area.lonMin, area.lonMax));
		site->setLatitude(RNG::generate(area.latMin, area.latMax));
		site->setId(_gameSave.getCanonicalId(ruleDeploy->getMarkerType()));
		site->setSecondsLeft(RNG::generate(
										ruleDeploy->getDurationMin(),
										ruleDeploy->getDurationMax()) * 3600);
		site->setAlienRace(_race);
		site->setSiteTextureId(area.texture);
		site->setCity(area.site);

		return site;
	}
	return nullptr;
}

/**
 * Spawns an AlienBase.
 * @param globe	- reference to the Globe, required to get access to land checks
 * @param rules	- reference to the Ruleset
 */
void AlienMission::createAlienBase( // private.
		const Globe& globe,
		const Ruleset& rules)
{
	if (_gameSave.getAlienBases()->size() <= 8u + (static_cast<size_t>(_gameSave.getDifficulty()) << 1u))
	{
		const size_t zoneId (_missionRule.getObjectiveZone());
		std::vector<MissionArea> areas (rules.getRegion(_region)->getMissionZones().at(zoneId).areas);
		MissionArea area (areas.at(RNG::pick(areas.size())));

		const RuleAlienDeployment* ruleDeploy;
		if (rules.getDeployment(_missionRule.getTerrorType()) != nullptr)
		{
			ruleDeploy = rules.getDeployment(_missionRule.getTerrorType());
		}
		else if (rules.getGlobe()->getTextureRule(area.texture) != nullptr
			&& rules.getGlobe()->getTextureRule(area.texture)->getTextureDeployments().empty() == false)
		{
			ruleDeploy = rules.getDeployment(rules.getGlobe()->getTextureRule(area.texture)->getTextureDeployment());
		}
		else
		{
			ruleDeploy = rules.getDeployment("STR_ALIEN_BASE_ASSAULT");
		}

		if (ruleDeploy == nullptr)
		{
			std::string st ("No RuleAlienDeployment defined for aLien Base.");
			st += " A deployment-rule must be defined in one of the mission-zone's texture,";
			st += " the mission's siteType, or by defining a deployment called";
			st += " \"STR_ALIEN_BASE_ASSAULT\" as a default or fallback.";
			throw Exception(st);
		}

		const RuleRegion& regionRule (*rules.getRegion(_region));
		const std::pair<double, double> pos (coordsLand(
													globe,
													regionRule,
													area));

		AlienBase* const aBase (new AlienBase(ruleDeploy));
		aBase->setAlienRace(_race);
//		aBase->setId(game.getId(deployment->getMarkerName())); // done in AlienBaseDetectedState.
		aBase->setLongitude(pos.first);
		aBase->setLatitude(pos.second);
		_gameSave.getAlienBases()->push_back(aBase);

		addScore( // alm_BASE, alm_INFILT
			pos.first,
			pos.second);
	}
}

/**
 * Sets the AlienBase associated with this AlienMission.
 * @note Only aLien supply-missions care about this.
 * @param base - pointer to an AlienBase
 */
void AlienMission::setAlienBase(const AlienBase* const base)
{
	_aBase = base;
}

/**
 * Gets the AlienBase associated with this AlienMission.
 * @note Only aLien supply-missions ever have a valid pointer.
 * @return, pointer to the AlienBase for this mission (possibly nullptr)
 */
const AlienBase* AlienMission::getAlienBase() const
{
	return _aBase;
}

/**
 * One of this AlienMission's UFOs has finished its time on the ground.
 * @note It takes care of sending the UFO to the next waypoint and marking it
 * for removal as required. It must set the game data in a way that the rest of
 * the code understands what to do.
 * @param ufo	- reference to the Ufo that is lifting off
 * @param rules	- reference to the Ruleset
 * @param globe	- reference to the Globe
 */
void AlienMission::ufoLifting(
		Ufo& ufo,
		Ruleset& rules,
		const Globe& globe)
{
	//Log(LOG_INFO) << "AlienMission::ufoLifting()";
	switch (ufo.getUfoStatus())
	{
		case Ufo::FLYING:
			ufo.setUfoTerrainType(); // safety i guess.
//			assert(0 && "Ufo is already on the air!");
			break;

		case Ufo::LANDED:
		{
			//Log(LOG_INFO) << ". mission complete, addScore + liftOff";
			if (ufo.getRules()->getType() == "STR_BATTLESHIP") // or could add and test for (objective=true) on the Battleship-wave rule.
			{
				switch (_missionRule.getObjectiveType())
				{
					case alm_INFILT:
						if (_success == false)
						{
							_success = true; // only the first Battleship needs to lift successfully - note this means that only one of the Battleships needs to be successful.

							createAlienBase( // adds alienPts.
										globe,
										rules);

							std::vector<Country*> suspectCountries;
							for (std::vector<Country*>::const_iterator
									i = _gameSave.getCountries()->begin();
									i != _gameSave.getCountries()->end();
									++i)
							{
								if ((*i)->getPact() == false
									&& (*i)->getRecentPact() == false
									&& rules.getRegion(_region)->insideRegion(
																		(*i)->getRules()->getLabelLongitude(),			// WARNING: The *label* of a Country must be inside the
																		(*i)->getRules()->getLabelLatitude()) == true)	// AlienMission's Region for aLiens to infiltrate!
								{
									suspectCountries.push_back(*i);
								}
							}

							if (suspectCountries.empty() == false)
							{
								Country* const infiltrated (suspectCountries.at(RNG::pick(suspectCountries.size())));
								//Log(LOG_INFO) << "AlienMission::ufoLifting(), GAAH! new Pact & aLien base: " << infiltrated->getType();
								if (infiltrated->getType() != "STR_RUSSIA")	// heh. ironically, this likely allows multiple alien
									infiltrated->setRecentPact();			// bases in Russia due solely to infiltrations ...!!
							}
						}
						else
							addScore(
									ufo.getLongitude(),
									ufo.getLatitude());
						break;

					case alm_BASE:
						createAlienBase( // adds alienPts.
									globe,
									rules);
						break;

					case alm_SCORE:
//					case alm_TERROR:	// doesn't do ufoLifting() at all
//					case alm_RETAL:		// has 0 pts.
					case alm_SUPPLY:
						addScore(
								ufo.getLongitude(),
								ufo.getLatitude());
				}
			}
			else
			{
				switch (_missionRule.getObjectiveType())
				{
					case alm_SCORE:
					case alm_INFILT:	// handled above. But let's let the littler UFOs rack up pts. also
					case alm_BASE:		// handled above. But let's let the littler UFOs rack up pts. also
//					case alm_TERROR:	// doesn't do ufoLifting() at all
//					case alm_RETAL:		// has 0 pts.
					case alm_SUPPLY:
						addScore(
								ufo.getLongitude(),
								ufo.getLatitude());
				}
			}

			ufo.setUfoTerrainType();
			ufo.setAltitude(MovingTarget::stAltitude[1u]);

			float speedPct;
			const size_t wpId (ufo.getTrajectoryPoint());
			const UfoTrajectory& trj (ufo.getTrajectory());
			if (wpId + 1u < trj.getWaypointTotal())
				speedPct = RNG::generate(
									trj.getSpeedPct(wpId),
									trj.getSpeedPct(wpId + 1u));
			else
				speedPct = trj.getSpeedPct(wpId);

			ufo.setSpeed(static_cast<int>(std::ceil(
						  speedPct * static_cast<float>(ufo.getRules()->getMaxSpeed()))));
			break;
		}

		case Ufo::CRASHED: // Mission expired
			ufo.setDetected(false);
			ufo.setUfoStatus(Ufo::DESTROYED);
			break;

//		case Ufo::DESTROYED:
//			assert(0 && "UFO can't fly!");
	}
}

/**
 * One of this AlienMission's UFOs is shot down - crashed or destroyed.
 * @note Currently the only thing that happens is delaying the next UFO in the
 * mission sequence.
 * @param ufo - reference to the Ufo that was shot down
 */
void AlienMission::ufoShotDown(const Ufo& ufo)
{
	switch (ufo.getUfoStatus())
	{
//		case Ufo::FLYING:
//		case Ufo::LANDED:
//			assert(0 && "Ufo seems ok!");
//			break;

		case Ufo::CRASHED:
		case Ufo::DESTROYED:
			if (_waveCount != _missionRule.getWaveTotal())
				_spawnTime += (RNG::generate(0,48) + 400) * 30; // delay next wave
	}
}

/**
 * Generates destination-coordinates based on a specified trajectory and a
 * specified waypoint-ID.
 * @param trajectory	- reference to the trajectory in question
 * @param wpId			- the next logical waypoint in sequence (0 for newly spawned UFOs)
 * @param globe			- reference to the Globe
 * @param region		- reference to the ruleset for the region of this mission
 * @return, pair of lon and lat coordinates based on the criteria of the trajectory
 */
std::pair<double, double> AlienMission::coordsWaypoint( // private.
		const UfoTrajectory& trajectory,
		const size_t wpId,
		const Globe& globe,
		const RuleRegion& region) const
{
	if (_terrorZone != std::numeric_limits<size_t>::max()
		&& trajectory.getZone(wpId) == _missionRule.getObjectiveZone())
	{
		size_t wave;
		switch (_waveCount)
		{
			case 0u: // restart AlienMission -> if (RNG::percent(static_cast<int>(_gameSave.getDifficulty()) * 20) == false)
				wave = _missionRule.getWaveTotal() - 1u;
				break;
			default:
				wave = _waveCount - 1u;
		}
		// NOTE: 'wave' has to be reduced by one because think() has already advanced it past current, I suppose.

		if (_missionRule.getWave(wave).isObjective == true)
		{
			const MissionArea* const area (&region.getMissionZones()
													.at(_missionRule.getObjectiveType()).areas
													.at(_terrorZone));
			return std::make_pair(
								area->lonMin,
								area->latMin);
		}
	}

	if (trajectory.getWaypointTotal() > wpId + 1u
		&& trajectory.getAltitude(wpId + 1u) == MovingTarget::stAltitude[0u])
 	{
 		return coordsLand(
						globe,
						region,
						trajectory.getZone(wpId));
 	}

	return region.getZonePoint(trajectory.getZone(wpId));
}

/**
 * Generates destination-coordinates inside a specified Region and MissionZone.
 * @note The point will be used to land a UFO so it *has to be on land*.
 * @param globe		- reference to the Globe
 * @param region	- reference to RuleRegion
 * @param zoneId	- ID of a MissionZone in the Region
 * @return, a pair of doubles (lon & lat)
 */
std::pair<double, double> AlienMission::coordsLand( // private.
		const Globe& globe,
		const RuleRegion& region,
		const size_t zoneId) const
{
	std::pair<double, double> coord;

	int t (0);
	do
	{
		++t;
		coord = region.getZonePoint(zoneId);
	}
	while (t < 1000
		&& (globe.insideLand(
						coord.first,
						coord.second) == false
			|| region.insideRegion(
						coord.first,
						coord.second) == false));

	if (t == 1000)
		Log(LOG_INFO) << "Region: " << region.getType()
					  << " lon " << coord.first
					  << " lat " << coord.second
					  << " invalid point in zoneId: " << zoneId << " - ufo was forced to land on water.";
	return coord;
}

/**
 * Generates destination-coordinates inside a specified Region and MissionArea.
 * @note The point will be used to land a UFO so it *has to be on land*.
 * @param globe		- reference to the Globe
 * @param region	- reference to RuleRegion
 * @param area		- a MissionArea in a MissionZone in the Region
 * @return, a pair of doubles (lon & lat)
 */
std::pair<double, double> AlienMission::coordsLand( // private.
		const Globe& globe,
		const RuleRegion& region,
		const MissionArea& area) const
{
	std::pair<double, double> coord;

	int t (0);
	do
	{
		++t;
		coord = region.getAreaPoint(area);
	}
	while (t < 1000
		&& (globe.insideLand(
						coord.first,
						coord.second) == false
			|| region.insideRegion(
						coord.first,
						coord.second) == false));

	if (t == 1000)
		Log(LOG_INFO) << "Region: " << region.getType()
					  << " lon " << coord.first
					  << " lat " << coord.second
					  << " invalid point in area - ufo was forced to land on water.";
	return coord;
}

/**
 * Adds aLien-points to the Country and Region at specified coordinates.
 * @param lon - longitudinal coordinate to check
 * @param lat - latitudinal coordinate to check
 */
void AlienMission::addScore( // private.
		const double lon,
		const double lat) const
{
	int aLienPts (_missionRule.getPoints());
	if (aLienPts != 0)
	{
		switch (_missionRule.getObjectiveType())
		{
//			case alm_TERROR: break;	// use default pt-value
//			case alm_RETAL: break;	// use default pt-value <- has 0 pts.

			case alm_INFILT:
			case alm_BASE:
				aLienPts += static_cast<int>(_gameSave.getDifficulty()) * 20	// TODO: Instead of '20' use a UFO-size modifier. 'Cause this
						 + (_gameSave.getMonthsPassed() << 1u);					// is gonna rack up *huge pts* in ufoLifting() as it is now.
				break;

			case alm_SUPPLY:
				aLienPts += static_cast<int>(_gameSave.getDifficulty()) * 10
						 + (_gameSave.getMonthsPassed() >> 1u);
				break;

			case alm_SCORE:
				aLienPts += (static_cast<int>(_gameSave.getDifficulty()) << 1u)
						 + _gameSave.getMonthsPassed();
		}
		_gameSave.scorePoints(lon,lat, aLienPts, true);
	}
}

/**
 * Tells this AlienMission which entry in the MissionZone vector is targeted.
 * @param zoneId - zone-ID to target, always a City-type zoneId #3 (probably)
 */
void AlienMission::setTerrorZone(size_t zoneId)
{
	_terrorZone = zoneId;
}

}
