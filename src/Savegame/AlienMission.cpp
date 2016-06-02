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
#include "MissionSite.h"
#include "Region.h"
#include "SavedGame.h"
#include "Ufo.h"
#include "Waypoint.h"

#include "../Engine/Exception.h"
#include "../Engine/Game.h"
#include "../Engine/Logger.h"
#include "../Engine/RNG.h"

#include "../Geoscape/Globe.h"

#include "../Ruleset/AlienDeployment.h"
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
 * @param gameSave		- reference the SavedGame
 */
AlienMission::AlienMission(
		const RuleAlienMission& missionRule,
		SavedGame& gameSave)
	:
		_missionRule(missionRule),
		_gameSave(gameSave),
		_waveCount(0u),
		_ufoCount(0u),
		_spawnTime(0u),
		_liveUfos(0u),
		_id(0),
		_aBase(nullptr),
		_success(false),
		_siteZone(std::numeric_limits<size_t>::max())
{}

/**
 * dTor.
 */
AlienMission::~AlienMission()
{}


/**
 ** FUNCTOR ***
 */
class matchById
	:
		public std::unary_function<const AlienBase*, bool>
{

private:
	int _id;

	public:
		/// Remember ID.
		explicit matchById(int id)
			:
				_id(id)
		{}

		/// Match with stored ID.
		bool operator() (const AlienBase* ab) const
		{
			return ab->getId() == _id;
		}
};


/**
 * Loads this AlienMission from a YAML file.
 * @param node - reference the YAML node containing data
 */
void AlienMission::load(const YAML::Node& node)
{
	_id			= node["id"]		.as<int>(_id);
	_region		= node["region"]	.as<std::string>(_region);
	_race		= node["race"]		.as<std::string>(_race);
	_waveCount	= node["waveCount"]	.as<size_t>(_waveCount);
	_ufoCount	= node["ufoCount"]	.as<size_t>(_ufoCount);
	_spawnTime	= node["spawnTime"]	.as<size_t>(_spawnTime);
	_liveUfos	= node["liveUfos"]	.as<size_t>(_liveUfos);
	_siteZone	= node["siteZone"]	.as<size_t>(_siteZone);
	_success	= node["success"]	.as<bool>(_success);

	if (const YAML::Node& baseId = node["alienBase"])
	{
		const int id (baseId.as<int>());
		const std::vector<AlienBase*>::const_iterator alienBase (std::find_if(
																		_gameSave.getAlienBases()->begin(),
																		_gameSave.getAlienBases()->end(),
																		matchById(id)));
		if (alienBase == _gameSave.getAlienBases()->end())
		{
			throw Exception("Corrupted save: Invalid base for mission.");
		}
		_aBase = *alienBase;
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

	if (_siteZone != std::numeric_limits<size_t>::max())
		node["siteZone"] = _siteZone;

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
	assert(_id == 0 && "Reassigning ID!");
	_id = id;
}

/**
 * Gets the unique-ID of this AlienMission.
 * @return, the unique ID assigned to this mission
 */
int AlienMission::getId() const
{
	assert(_id != 0 && "Uninitialized mission!");
	return _id;
}

/**
 * Sets this AlienMission's Region.
 * @note If the region is incompatible with actually carrying out an attack use
 * the fallback region as defined in the ruleset. This is a slight difference
 * from the original which just defaulted to zone[0] North America.
 * @param region	- reference the region to try to set the mission to
 * @param rules		- reference the ruleset in case the region needs to be swapped out
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
void AlienMission::setWaveCountdown(size_t minutes)
{
	assert(minutes != 0 && minutes % 30 == 0);

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
		&& _missionRule.getObjective() != alm_INFILT) // Infiltrations continue for ever. Almost.
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
void AlienMission::start(size_t countdown)
{
	_waveCount =
	_ufoCount =
	_liveUfos = 0;

	if (countdown == 0)
		calcSpawnTime(0);
	else
		_spawnTime = countdown;
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

			Ufo* const ufo (spawnUfo(
									rules,
									globe,
									wave,
									trajectory));

			if (ufo != nullptr) // a UFO hath spawned!
				_gameSave.getUfos()->push_back(ufo);
			else if ((rules.getUfo(wave.ufoType) == nullptr // a mission site to spawn directly
					&& rules.getDeployment(wave.ufoType) != nullptr
					&& rules.getDeployment(wave.ufoType)->getMarkerType().empty() == false)
				|| (_missionRule.getObjective() == alm_SITE // or spawn one at random according to the terrain
					&& wave.objective == true))
			{
				size_t zone;
				if (_missionRule.getSpawnZone() == std::numeric_limits<size_t>::max())
					zone = trajectory.getZone(0);
				else
					zone = _missionRule.getSpawnZone();
				const std::vector<MissionArea> areas (rules.getRegion(_region)->getMissionZones().at(zone).areas);

				if (_siteZone == std::numeric_limits<size_t>::max())
					zone = RNG::pick(areas.size());
//					zone = static_cast<size_t>(RNG::generate(0, static_cast<int>(areas.size()) - 1));
				else
					zone = _siteZone;
				const MissionArea area (areas.at(zone));

				const RuleTexture* const texture (rules.getGlobe()->getTextureRule(area.texture));

				const AlienDeployment* ruleDeploy;
				if (rules.getDeployment(wave.ufoType) != nullptr)
					ruleDeploy = rules.getDeployment(wave.ufoType);
				else
					ruleDeploy = rules.getDeployment(texture->getTextureDeployment());

				spawnMissionSite(ruleDeploy, area);
			}

			++_ufoCount;
			if (_ufoCount == wave.ufoTotal)
			{
				_ufoCount = 0;
				++_waveCount;
			}

			if (_waveCount != _missionRule.getWaveTotal())
				calcSpawnTime(_waveCount);
			else if (_missionRule.getObjective() == alm_INFILT)	// Infiltrations loop for ever.
				_waveCount = 0;
		}
	}
}
/*	// don't do these onSpawn; do them on ufoLifting() below_
	if (_waveCount == _missionRule.getWaveTotal())
	{
		const MissionObjective object = _missionRule.getObjective();

		if (object == alm_BASE
			|| object == alm_INFILT)
		{
			spawnAlienBase( // adds alienPts.
						globe,
						rules,
						_missionRule.getSpawnZone());

			if (object == alm_INFILT)
			{
				std::vector<Country*> countryList;

				for (std::vector<Country*>::const_iterator
						i = _gameSave.getCountries()->begin();
						i != _gameSave.getCountries()->end();
						++i)
				{
					if ((*i)->getPact() == false
						&& (*i)->getRecentPact() == false
						&& rules.getRegion(_region)->insideRegion( // WARNING: The *label* of a Country must be inside its Region for aLiens to infiltrate!
															(*i)->getRules()->getLabelLongitude(),
															(*i)->getRules()->getLabelLatitude()) == true)
					{
						countryList.push_back(*i);
					}
				}

				if (countryList.empty() == false)
				{
					//Log(LOG_INFO) << "AlienMission::think(), GAAH! new Pact & aLien base";
					const size_t pick = RNG::generate(
												0,
												countryList.size() - 1);
					Country* const infiltrated = countryList.at(pick);
					// kL_note: Ironically, this likely allows multiple alien
					// bases in Russia solely because of infiltrations ...!!
					if (infiltrated->getType() != "STR_RUSSIA") // heh.
						infiltrated->setRecentPact();
				}

				_waveCount = 0; // Infiltrations loop for ever.
			}
		}
	} // moved to ufoLifting() */

/**
 * Calculates time remaining until the next wave of this AlienMission spawns.
 * @note These come in increments of 30sec (or min?) apiece.
 * @param nextWave - the wave to check
 */
void AlienMission::calcSpawnTime(size_t nextWave) // private.
{
	int countdown (static_cast<int>(_missionRule.getWave(nextWave).spawnTimer));
	countdown /= 30;
	countdown = (countdown / 2) + RNG::generate(0, countdown);
	countdown *= 30;
	_spawnTime = static_cast<size_t>(countdown);
}


/**
 ** FUNCTOR ***
 * Finds an XCOM base in this region that is marked for retaliation.
 * @note Helper for spawnUfo().
 */
/* class FindExposedXCOMBase
	:
		public std::unary_function<const Base*, bool>
{

private:
	const RuleRegion& _region;

	public:
		///
		explicit FindExposedXCOMBase(const RuleRegion& region)
			:
				_region(region)
		{}

		///
		bool operator() (const Base* base) const
		{
			return base->getBaseExposed() == true
				&& _region.insideRegion(
									base->getLongitude(),
									base->getLatitude()) == true;
		}
}; */


/**
 * Spawns a UFO according this AlienMission's rules.
 * @note Some code is duplicated between cases but that's ok for now. It's on
 * different code paths and the function is MUCH easier to read written this way.
 * @param rules			- reference the ruleset
 * @param globe			- reference the globe for land checks
 * @param wave			- reference the wave for the desired UFO
 * @param trajectory	- reference the rule for the desired trajectory
 * @return, pointer to the spawned UFO; if the mission does not spawn a UFO return nullptr
 */
Ufo* AlienMission::spawnUfo( // private.
		const Ruleset& rules,
		const Globe& globe,
		const MissionWave& wave,
		const UfoTrajectory& trajectory)
{
	const RuleUfo* ufoRule (rules.getUfo(wave.ufoType));

	std::pair<double, double> coord;
	Waypoint* wp;
	Ufo* ufo;

	switch (_missionRule.getObjective())
	{
		case alm_RETAL:
			{
//				const RuleRegion& regionRule (*rules.getRegion(_region));
//				const std::vector<Base*>::const_iterator i (std::find_if(
//																	_gameSave.getBases()->begin(),
//																	_gameSave.getBases()->end(),
//																	FindMarkedXCOMBase(regionRule)));
//				if (i != _gameSave.getBases()->end())

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

				if (baseTargets.empty() == false)
				{
					// Spawn a battleship straight for the XCOM Base.
					const RuleUfo& battleshipRule (*rules.getUfo(_missionRule.getSpawnUfo()));
					const UfoTrajectory& trjBattleship (*rules.getUfoTrajectory(UfoTrajectory::RETALIATION_ASSAULT_RUN));
					const RuleRegion& regionRule (*rules.getRegion(_region));

					ufo = new Ufo(&battleshipRule);
					ufo->setUfoMissionInfo(
										this,
										&trjBattleship);

					if (trajectory.getAltitude(0) == "STR_GROUND")
						coord = getLandPoint(
										globe,
										regionRule,
										trajectory.getZone(0));
					else
						coord = regionRule.getRandomPoint(trajectory.getZone(0));

					ufo->setAltitude(trjBattleship.getAltitude(0));
					ufo->setSpeed(static_cast<int>(std::ceil(
								  static_cast<double>(trjBattleship.getSpeedPct(0))
								* static_cast<double>(battleshipRule.getMaxSpeed()))));
					ufo->setLongitude(coord.first);
					ufo->setLatitude(coord.second);

					wp = new Waypoint();
					const size_t pick (RNG::pick(baseTargets.size()));
					wp->setLongitude(baseTargets[pick]->getLongitude());
					wp->setLatitude(baseTargets[pick]->getLatitude());

					ufo->setDestination(wp);
					return ufo;
				}
			}
			break;

		case alm_SUPPLY: // check for base to supply.
			if (ufoRule != nullptr
				&& (_aBase != nullptr || wave.objective == false))
			{
				ufo = new Ufo(ufoRule);
				ufo->setUfoMissionInfo( // destination is always an alien base.
									this,
									&trajectory);
				const RuleRegion& regionRule (*rules.getRegion(_region));

				if (trajectory.getAltitude(0) == "STR_GROUND")
					coord = getLandPoint(
									globe,
									regionRule,
									trajectory.getZone(0));
				else
					coord = regionRule.getRandomPoint(trajectory.getZone(0));

				ufo->setAltitude(trajectory.getAltitude(0));
				ufo->setSpeed(static_cast<int>(std::ceil(
							  static_cast<double>(trajectory.getSpeedPct(0))
							* static_cast<double>(ufoRule->getMaxSpeed()))));
				ufo->setLongitude(coord.first);
				ufo->setLatitude(coord.second);

				if (trajectory.getAltitude(1) == "STR_GROUND")
				{
					if (wave.objective == true) // Supply ships on supply missions land on bases, ignore trajectory zone.
					{
						coord.first = _aBase->getLongitude();
						coord.second = _aBase->getLatitude();
					}
					else
						coord = getLandPoint( // Other ships can land where they want.
										globe,
										regionRule,
										trajectory.getZone(1));
				}
				else
					coord = regionRule.getRandomPoint(trajectory.getZone(1));

				wp = new Waypoint();
				wp->setLongitude(coord.first);
				wp->setLatitude(coord.second);

				ufo->setDestination(wp);
				return ufo;
			}
			return nullptr; // No base to supply!
	}

	if (ufoRule != nullptr)
	{
		ufo = new Ufo(ufoRule); // else Spawn according to sequence
		ufo->setUfoMissionInfo(this, &trajectory);
		const RuleRegion& regionRule (*rules.getRegion(_region));
		coord = getWaypoint(
						trajectory,
						0,
						globe,
						regionRule);

		ufo->setAltitude(trajectory.getAltitude(0));
		if (trajectory.getAltitude(0) == "STR_GROUND")
			ufo->setSecondsLeft(trajectory.groundTimer() * 5);

		ufo->setSpeed(static_cast<int>(std::ceil(
					  static_cast<double>(trajectory.getSpeedPct(0)) // TODO: Add a speed-modifier via RNG.
					* static_cast<double>(ufoRule->getMaxSpeed()))));
		ufo->setLongitude(coord.first);
		ufo->setLatitude(coord.second);

		coord = getWaypoint(
						trajectory,
						1,
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
				&& AreSame(base->getLatitude(), _lat);
		}
};


/**
 * One of this AlienMission's UFOs arrived at its current destination.
 * @note It takes care of sending the UFO to the next waypoint, landing
 * UFOs and marking them for removal as required. It must set the game data in a
 * way that the rest of the code understands what to do.
 * @param ufo	- reference the Ufo that reached its waypoint
 * @param rules	- reference the Ruleset
 * @param globe	- reference the earth globe required to get access to land checks
 */
void AlienMission::ufoReachedWaypoint(
		Ufo& ufo,
		const Ruleset& rules,
		const Globe& globe)
{
	const UfoTrajectory& trajectory (ufo.getTrajectory());
	const size_t
		pt (ufo.getTrajectoryPoint()),
		ptNext (pt + 1);

	if (ptNext < trajectory.getWaypointTotal())
	{
		ufo.setAltitude(trajectory.getAltitude(ptNext));
		ufo.setTrajectoryPoint(ptNext);

		const RuleRegion& regionRule (*rules.getRegion(_region));
		const std::pair<double, double> coord (getWaypoint(
														trajectory,
														ptNext,
														globe,
														regionRule));

		Waypoint* const wayPoint (new Waypoint());
		wayPoint->setLongitude(coord.first);
		wayPoint->setLatitude(coord.second);
		ufo.setDestination(wayPoint);

		if (ufo.getAltitude() != "STR_GROUND")
		{
			ufo.setLandId(0);
			ufo.setSpeed(static_cast<int>(std::ceil(
						 static_cast<double>(trajectory.getSpeedPct(ptNext)) // TODO: Add a speed-modifier via RNG.
					   * static_cast<double>(ufo.getRules()->getMaxSpeed()))));
		}
		else // UFO landed.
		{
			size_t wave;
			if (_waveCount != 0)
				wave = _waveCount - 1;
			else // -> if (RNG::percent(static_cast<int>(_gameSave.getDifficulty()) * 20) == false)
				wave = _missionRule.getWaveTotal() - 1;
			// note, 'wave' has to be reduced by one because think() has already advanced it past current, I suppose.

			if (_missionRule.getWave(wave).objective == true // destroy UFO & replace with MissionSite.
				&& trajectory.getZone(pt) == _missionRule.getSpawnZone()) // note Supply bypasses this although it has (objective==true) because it does not have a 'specialZone'
			{
				addScore( // alm_SITE
					ufo.getLongitude(),
					ufo.getLatitude());

				ufo.setUfoStatus(Ufo::DESTROYED);

				// note: Looks like they're having probls with getting a mission pt:
//				MissionArea area (regionRule.getMissionZones().at(trajectory.getZone(pt)).areas.at(_siteZone));
				const MissionArea area (regionRule.getMissionPoint(
																trajectory.getZone(pt),
																dynamic_cast<Target*>(&ufo)));

				const AlienDeployment* ruleDeploy (rules.getDeployment(_missionRule.getSiteType()));
				if (ruleDeploy == nullptr)
				{
					const RuleTexture* const texture (rules.getGlobe()->getTextureRule(area.texture));
					ruleDeploy = rules.getDeployment(texture->getTextureDeployment());
				}

				MissionSite* const site (spawnMissionSite(ruleDeploy, area));
				if (site != nullptr)
				{
					_gameSave.getMissionSites()->push_back(site);

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
				if (i == _gameSave.getBases()->end())
				{
					ufo.setUfoStatus(Ufo::DESTROYED);
					return;
				}
				ufo.setDestination(*i);
			}
			else // Set timer for UFO on the ground.
			{
				if (globe.insideLand(
								ufo.getLongitude(),
								ufo.getLatitude()) == true)
				{
					ufo.setSecondsLeft(trajectory.groundTimer() * 5);

					if (ufo.getDetected() == true && ufo.getLandId() == 0)
						ufo.setLandId(_gameSave.getCanonicalId("STR_LANDING_SITE"));
				}
				else // there's nothing to land on
					ufo.setSecondsLeft(5);
			}
		}
	}
	else // UFO leaving atmosphere.
	{
		ufo.setDetected(false);
		ufo.setUfoStatus(Ufo::DESTROYED);
	}
}

/**
 * Attempts to spawn a Mission Site at a given location.
 * @param ruleDeploy	- pointer to the AlienDeployment
 * @param area			- reference an area of the globe
 * @return, pointer to the site
 */
MissionSite* AlienMission::spawnMissionSite( // private.
		const AlienDeployment* const ruleDeploy,
		const MissionArea& area)
{
	if (ruleDeploy != nullptr)
	{
		MissionSite* const site (new MissionSite(
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
 * @param globe	- reference the Globe, required to get access to land checks
 * @param rules	- reference the Ruleset
 * @param zone	- the mission zone required for determining the base coordinates
 */
void AlienMission::spawnAlienBase( // private.
		const Globe& globe,
		const Ruleset& rules,
		const size_t zone)
{
	if (_gameSave.getAlienBases()->size() > 8 + static_cast<size_t>(_gameSave.getDifficulty()) * 2)
		return;

	// Once the last UFO is spawned the aliens build their base. TODO: <- change that!
	const RuleRegion& regionRule (*rules.getRegion(_region));
	const std::pair<double, double> pos (getLandPoint(
													globe,
													regionRule,
													zone));

	AlienBase* const aBase (new AlienBase());
	aBase->setAlienRace(_race);
	aBase->setLongitude(pos.first);
	aBase->setLatitude(pos.second);
	_gameSave.getAlienBases()->push_back(aBase);

	addScore( // alm_BASE, alm_INFILT
		pos.first,
		pos.second);
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
 * @param ufo	- reference the Ufo that is lifting off
 * @param rules	- reference the Ruleset
 * @param globe	- reference the Globe
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
			assert(0 && "Ufo is already on the air!");
			break;

		case Ufo::LANDED:
			//Log(LOG_INFO) << ". mission complete, addScore + liftOff";
			if (ufo.getRules()->getType() == "STR_BATTLESHIP") // or could add and test for (objective=true) on the Battleship-wave rule.
			{
				switch (_missionRule.getObjective())
				{
					case alm_INFILT:
						if (_success == false)
						{
							_success = true; // only the first Battleship needs to lift successfully - note this means that only one of the Battleships needs to be successful.
							spawnAlienBase( // adds alienPts.
										globe,
										rules,
										_missionRule.getSpawnZone());

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
						spawnAlienBase( // adds alienPts.
									globe,
									rules,
									_missionRule.getSpawnZone());
						break;

					case alm_SCORE:
//					case alm_SITE:	// doesn't do ufoLifting() at all
//					case alm_RETAL:	// has 0 pts.
					case alm_SUPPLY:
						addScore(
								ufo.getLongitude(),
								ufo.getLatitude());
				}
			}
			else
			{
				switch (_missionRule.getObjective())
				{
					case alm_SCORE:
					case alm_INFILT:	// handled above. But let's let the littler UFOs rack up pts. also
					case alm_BASE:		// handled above. But let's let the littler UFOs rack up pts. also
//					case alm_SITE:		// doesn't do ufoLifting() at all
//					case alm_RETAL:		// has 0 pts.
					case alm_SUPPLY:
						addScore(
								ufo.getLongitude(),
								ufo.getLatitude());
				}
			}

			ufo.setUfoTerrainType();
			ufo.setAltitude("STR_VERY_LOW");
			ufo.setSpeed(static_cast<int>(std::ceil(
						 static_cast<double>(ufo.getTrajectory().getSpeedPct(ufo.getTrajectoryPoint()))
					   * static_cast<double>(ufo.getRules()->getMaxSpeed()))));
			break;

		case Ufo::CRASHED: // Mission expired
			ufo.setDetected(false);
			ufo.setUfoStatus(Ufo::DESTROYED);
			break;

		case Ufo::DESTROYED:
			assert(0 && "UFO can't fly!");
	}
}

/**
 * One of this AlienMission's UFOs is shot down - crashed or destroyed.
 * @note Currently the only thing that happens is delaying the next UFO in the
 * mission sequence.
 * @param ufo - reference the Ufo that was shot down
 */
void AlienMission::ufoShotDown(const Ufo& ufo)
{
	switch (ufo.getUfoStatus())
	{
		case Ufo::FLYING:
		case Ufo::LANDED:
			assert(0 && "Ufo seems ok!");
			break;

		case Ufo::CRASHED:
		case Ufo::DESTROYED:
			if (_waveCount != _missionRule.getWaveTotal())
				_spawnTime += (static_cast<size_t>(RNG::generate(0,48)) + 400) * 30; // delay next wave
	}
}

/**
 * Selects a destination based on the criteria of the trajectory and desired
 * waypoint.
 * @param trajectory	- reference the trajectory in question
 * @param nextWaypoint	- the next logical waypoint in sequence (0 for newly spawned UFOs)
 * @param globe			- reference the Globe
 * @param region		- reference the ruleset for the region of this mission
 * @return, pair of lon and lat coordinates based on the criteria of the trajectory
 */
std::pair<double, double> AlienMission::getWaypoint(
		const UfoTrajectory& trajectory,
		const size_t nextWaypoint,
		const Globe& globe,
		const RuleRegion& region)
{
	size_t wave (_waveCount - 1);
	if (wave == std::numeric_limits<size_t>::max())
		wave = _missionRule.getWaveTotal() - 1;

	if (_siteZone != std::numeric_limits<size_t>::max()
		&& _missionRule.getWave(wave).objective == true
		&& trajectory.getZone(nextWaypoint) == _missionRule.getSpawnZone())
	{
		const MissionArea* const area (&region.getMissionZones().at(_missionRule.getObjective()).areas.at(_siteZone));
		return std::make_pair(
							area->lonMin,
							area->latMin);
	}

	if (trajectory.getWaypointTotal() > nextWaypoint + 1
		&& trajectory.getAltitude(nextWaypoint + 1) == "STR_GROUND")
 	{
 		return getLandPoint(
						globe, region,
						trajectory.getZone(nextWaypoint));
 	}

	return region.getRandomPoint(trajectory.getZone(nextWaypoint));
}

/**
 * Gets a random point inside a specified Region zone.
 * @note The point will be used to land a UFO so it *has to be on land*.
 * @param globe		- reference the Globe
 * @param region	- reference RuleRegion
 * @param zone		- zone number in the region
 * @return, a pair of doubles (lon & lat)
 */
std::pair<double, double> AlienMission::getLandPoint(
		const Globe& globe,
		const RuleRegion& region,
		const size_t zone)
{
	int tries (0);
	std::pair<double, double> coord;
	do
	{
		++tries;
		coord = region.getRandomPoint(zone);
	}
	while (tries < 100
		&& (globe.insideLand(
						coord.first,
						coord.second) == false
			|| region.insideRegion(
						coord.first,
						coord.second) == false));

	if (tries == 100)
		Log(LOG_INFO) << "Region: " << region.getType()
					  << " lon " << coord.first
					  << " lat " << coord.second
					  << " invalid zone: " << zone << " - ufo was forced to land on water.";
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
		switch (_missionRule.getObjective())
		{
//			case alm_SITE: break;	// use default pt-value
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
 * Tells this AlienMission which entry in the zone-array is targeted.
 * @param zone - entry of the zone to target; always a City-type zone (probably)
 */
void AlienMission::setMissionSiteZone(size_t zone)
{
	_siteZone = zone;
}

}
