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
//#include "../Engine/Language.h" // DEBUG
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
 * @param playSave		- reference to the SavedGame
 */
AlienMission::AlienMission(
		const RuleAlienMission& missionRule,
		SavedGame& playSave)
	:
		_missionRule(missionRule),
		_playSave(playSave),
		_waveCount(0u),
		_ufoCount(0),
		_countdown(0),
		_liveUfos(0),
		_id(0),
		_alienBase(nullptr),
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
class MatchBaseId
	:
		public std::unary_function<const AlienBase*, bool>
{
private:
	int _id;

	public:
		/// Cache the ID.
		explicit MatchBaseId(int id)
			:
				_id(id)
		{}

		/// Match with cached ID.
		bool operator ()(const AlienBase* const base) const
		{
			return base->getId() == _id;
		}
};

/**
 * Loads this AlienMission from a YAML file.
 * @param node - reference to a YAML node
 */
void AlienMission::load(const YAML::Node& node)
{
	_id			= node["id"]		.as<int>(_id);
	_regionType	= node["region"]	.as<std::string>(_regionType);
	_raceType	= node["race"]		.as<std::string>(_raceType);
	_waveCount	= node["waveCount"]	.as<size_t>(_waveCount);
	_ufoCount	= node["ufoCount"]	.as<int>(_ufoCount);
	_countdown	= node["countdown"]	.as<int>(_countdown);
	_liveUfos	= node["liveUfos"]	.as<int>(_liveUfos);
	_terrorZone	= node["terrorZone"].as<size_t>(_terrorZone);
	_success	= node["success"]	.as<bool>(_success);

	if (const YAML::Node& baseId = node["alienBase"])
	{
		const int id (baseId.as<int>());
		const std::vector<AlienBase*>::const_iterator i (std::find_if(
																	_playSave.getAlienBases()->begin(),
																	_playSave.getAlienBases()->end(),
																	MatchBaseId(id)));
		if (i == _playSave.getAlienBases()->end())
		{
			throw Exception("Corrupted save: Invalid aLien Base for AlienMission.");
		}
		_alienBase = *i;
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
	node["region"]		= _regionType;
	node["race"]		= _raceType;
	node["waveCount"]	= _waveCount;
	node["ufoCount"]	= _ufoCount;
	node["countdown"]	= _countdown;
	node["liveUfos"]	= _liveUfos;

	if (_terrorZone != std::numeric_limits<size_t>::max())
		node["terrorZone"] = _terrorZone;

	if (_success == true)
		node["success"] = _success;

	if (_alienBase != nullptr)
		node["alienBase"] = _alienBase->getId();

	return node;
}

/**
 * Sets an ID for this AlienMission.
 * @note It is an error to assign two IDs to the same mission.
 * @param id - the ID to assign
 */
void AlienMission::setId(int id)
{
//	assert(_id == 0 && "Reassigning ID!");
	_id = id;
}

/**
 * Gets the ID of this AlienMission.
 * @return, the ID assigned to this mission
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
		_regionType = rules.getRegion(region)->getMissionRegion();
	else
		_regionType = region;
}

/**
 * Sets the minutes until the next wave of UFOs will spawn.
 * @note The new time must be a multiple of 30 minutes and > 0. Calling this
 * on a finished mission has no effect.
 */
void AlienMission::resetCountdown()
{
//	assert(minutes != 0 && minutes % 30 == 0);
	if (isOver() == false)
		_countdown = getStandardDelay();
}

/**
 * Gets a standard wait.
 * @return, the delay in minutes
 */
int AlienMission::getStandardDelay() const // private.
{
	return (RNG::generate(0,400) + 48) * 30;
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
// ->		|| RNG::percent(_playSave.getDifficultyInt() * 20) == false
	{
		return true;
	}
	return false;
}

/**
 * Starts this AlienMission.
 * @param countdown - countdown in minutes till next wave starts (default 0)
 */
void AlienMission::start(int countdown)
{
	//Log(LOG_INFO) << "";
	//Log(LOG_INFO) << "AlienMission::start() countdown= " << countdown;
	_waveCount = 0u;
	_ufoCount =
	_liveUfos = 0;

	switch (countdown)
	{
		case 0:
			initiateCountdown(0u);
			break;
		default:
			_countdown = countdown;
	}
}

/**
 * Advances this AlienMission.
 * @note Called by GeoscapeState every 30 minutes.
 * @param game	- reference to the Game
 * @param globe	- reference to the Globe
 */
void AlienMission::think(
		const Game& game,
		const Globe& globe)
{
//	Log(LOG_INFO) << "";
//	Log(LOG_INFO) << "AlienMission::think()";
//
//	Log(LOG_INFO) << ". rule= "			<< _missionRule.getType();
//	Log(LOG_INFO) << ". waves= "		<< _missionRule.getWaveTotal();
//
//	Log(LOG_INFO) << ". race= "			<< _raceType;
//	Log(LOG_INFO) << ". region= "		<< _regionType;
//
//	Log(LOG_INFO) << ". id= "			<< _id;
//	Log(LOG_INFO) << ". liveUfos= "		<< _liveUfos;
//	Log(LOG_INFO) << ". countdown= "	<< _countdown;
//	Log(LOG_INFO) << ". ufoCount= "		<< _ufoCount;
//	Log(LOG_INFO) << ". waveCount= "	<< _waveCount;
//
//	if (_terrorZone != std::numeric_limits<size_t>::max())
//		Log(LOG_INFO) << ". terrorZone= " << _terrorZone;
//	else
//		Log(LOG_INFO) << ". terrorZone NOTSET";
//
//	if (_alienBase != nullptr)
//		Log(LOG_INFO) << ". alienBase= " << Language::wstrToFs(_alienBase->getLabel(game.getLanguage()));
//	else
//		Log(LOG_INFO) << ". alienBase NULL";
//
//	if (_missionRule.getWaves().empty() == false)
//	{
//		Log(LOG_INFO) << ". . waves size= "	<< _missionRule.getWaves().size();
//
//		Log(LOG_INFO) << ". . ufoType= "	<< _missionRule.getWaveData(_waveCount).ufoType;
//		Log(LOG_INFO) << ". . ufoTotal= "	<< _missionRule.getWaveData(_waveCount).ufoTotal;
//		Log(LOG_INFO) << ". . traj= "		<< _missionRule.getWaveData(_waveCount).trajectory;
//		Log(LOG_INFO) << ". . timer= "		<< _missionRule.getWaveData(_waveCount).waveTimer;
//	}
//	else Log(LOG_INFO) << ". . waves EMPTY";


	if (_waveCount < _missionRule.getWaveTotal())
	{
		if (_countdown  > 30)
			_countdown -= 30;
		else
		{
			const Ruleset& rules		(*game.getRuleset());
			const MissionWave& wavedata	(_missionRule.getWaveData(_waveCount));
			const UfoTrajectory& trj	(*rules.getUfoTrajectory(wavedata.trajectory));

			Ufo* const ufo (createUfo(
									rules,
									globe,
									wavedata,
									trj));

			if (ufo != nullptr)										// a UFO hath spawned!
				_playSave.getUfos()->push_back(ufo);
			else if ((rules.getUfo(wavedata.ufoType) == nullptr		// a terror-site to spawn directly
					&& rules.getDeployment(wavedata.ufoType) != nullptr
					&& rules.getDeployment(wavedata.ufoType)->getMarkerType().empty() == false)
				|| (_missionRule.getObjectiveType() == alm_TERROR	// or spawn a site according to the terrain
					&& wavedata.isObjective == true))
			{
				size_t id;
				if (_missionRule.getObjectiveZone() == std::numeric_limits<size_t>::max())
					id = trj.getZoneId(0u);
				else
					id = _missionRule.getObjectiveZone();
				const std::vector<MissionArea> areas (rules.getRegion(_regionType)->getMissionZones().at(id).areas);

				if (_terrorZone == std::numeric_limits<size_t>::max())
					id = RNG::pick(areas.size());
				else
					id = _terrorZone;
				const MissionArea area (areas.at(id));

				const RuleTexture* const texture (rules.getGlobe()->getTextureRule(area.texture));

				const RuleAlienDeployment* ruleDeploy;
				if (rules.getDeployment(wavedata.ufoType) != nullptr)
					ruleDeploy = rules.getDeployment(wavedata.ufoType);
				else
					ruleDeploy = rules.getDeployment(texture->getTextureDeployment());

				createTerror(ruleDeploy, area);
			}

			if (++_ufoCount == wavedata.ufoTotal)
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
						initiateCountdown(_waveCount);
			}
		}
	}
}

/**
 * Calculates time remaining until the next wave of this AlienMission spawns.
 * @note The '_countdown' must be maintained as an exact multiple of 30 minutes. <- maybe!.
 * @param waveId - the wave to check
 */
void AlienMission::initiateCountdown(size_t waveId) // private.
{
	//Log(LOG_INFO) << "";
	//Log(LOG_INFO) << "AlienMission::initiateCountdown()";
	_countdown = _missionRule.getWaveData(waveId).waveTimer / 30;
	_countdown = (RNG::generate(0, _countdown) + (_countdown >> 1u)) * 30;
	//Log(LOG_INFO) << ". _countdown= " << _countdown;
}

/**
 * Spawns a UFO according this AlienMission's rules.
 * @note Some code is duplicated between cases but that's ok for now. It's on
 * different code paths and the function is MUCH easier to read written this way.
 * @param rules	- reference to the Ruleset
 * @param globe	- reference to the Globe for land checks
 * @param wave	- reference to the MissionWave
 * @param trj	- reference to the UfoTrajectory
 * @return, pointer to the spawned UFO; if the mission does not spawn a UFO return nullptr
 */
Ufo* AlienMission::createUfo( // private.
		const Ruleset& rules,
		const Globe& globe,
		const MissionWave& wave,
		const UfoTrajectory& trj)
{
	//Log(LOG_INFO) << "";
	//Log(LOG_INFO) << "AlienMission::createUfo()";
	const RuleUfo* ufoRule (rules.getUfo(wave.ufoType));

	std::pair<double, double> coord;
	Waypoint* wp;
	Ufo* ufo;

	switch (_missionRule.getObjectiveType())
	{
		case alm_RETAL:
		{
			std::vector<Base*> baseTargets; // check for exposed XCOM Bases to send a battleship at
			for (std::vector<Base*>::const_iterator
					i = _playSave.getBases()->begin();
					i != _playSave.getBases()->end();
					++i)
			{
				if ((*i)->getBaseExposed() == true
					&& rules.getRegion(_regionType)->insideRegion(
															(*i)->getLongitude(),
															(*i)->getLatitude()) == true)
				{
					baseTargets.push_back(*i);
				}
			}

			if (baseTargets.empty() == false) // send a battleship straight for an exposed XCOM Base.
			{
				const RuleUfo& assaultRule		(*rules.getUfo(_missionRule.getObjectiveUfo()));
				const UfoTrajectory& trjAssault	(*rules.getUfoTrajectory(UfoTrajectory::XCOM_BASE_ASSAULT));
				const RuleRegion& regionRule	(*rules.getRegion(_regionType));

				ufo = new Ufo(
							&assaultRule,
							&_playSave);
				ufo->setUfoMissionInfo(
									this,
									&trjAssault);

				if (trjAssault.getAltitude(0u) == MovingTarget::stAltitude[0u]) // assault-ufo spawns instantly landed
					coord = coordsLandZone(
										globe,
										regionRule,
										trjAssault.getZoneId(0u));
				else
					coord = regionRule.getZonePoint(trjAssault.getZoneId(0u)); // assault-ufo spawns airborne

				ufo->setLongitude(coord.first);
				ufo->setLatitude(coord.second);

				ufo->setAltitude(trjAssault.getAltitude(0u));

				wp = new Waypoint();
				const size_t pick (RNG::pick(baseTargets.size()));
				wp->setLongitude(baseTargets[pick]->getLongitude());
				wp->setLatitude(baseTargets[pick]->getLatitude());

				ufo->setTarget(wp); // NOTE: Target must be set before speed.
				ufo->setSpeed(static_cast<int>(std::ceil(
							  trjAssault.getSpeedPct(0u) * static_cast<double>(assaultRule.getTopSpeed()))));
				return ufo;
			}
			break; // if no XCOM Base is exposed create a regular UFO below_
		}

		case alm_SUPPLY: // check for an AlienBase to supply.
			if (ufoRule != nullptr
				&& (_alienBase != nullptr || wave.isObjective == false))
			{
				ufo = new Ufo(
							ufoRule,
							&_playSave);
				ufo->setUfoMissionInfo( // destination is always an alien base.
									this,
									&trj);
				const RuleRegion& regionRule (*rules.getRegion(_regionType));

				if (trj.getAltitude(0u) == MovingTarget::stAltitude[0u]) // supply-ufo spawns instantly landed
					coord = coordsLandZone(
										globe,
										regionRule,
										trj.getZoneId(0u));
				else
					coord = regionRule.getZonePoint(trj.getZoneId(0u)); // supply-ufo spawns airborne

				ufo->setLongitude(coord.first);
				ufo->setLatitude(coord.second);

				ufo->setAltitude(trj.getAltitude(0u));
				if (trj.getAltitude(1u) == MovingTarget::stAltitude[0u])
				{
					if (wave.isObjective == true) // supply-ufo on a supply-mission lands at an AlienBase: ignore trajectory zone.
					{
						coord.first = _alienBase->getLongitude();
						coord.second = _alienBase->getLatitude();
					}
					else
						coord = coordsLandZone( // other UFOs can land where they want. huh, "other" this is Supply.
											globe,
											regionRule,
											trj.getZoneId(1u));
				}
				else
					coord = regionRule.getZonePoint(trj.getZoneId(1u));

				wp = new Waypoint();
				wp->setLongitude(coord.first);
				wp->setLatitude(coord.second);

				ufo->setTarget(wp); // NOTE: Target must be set before speed. Not anymore.
				ufo->setSpeed(static_cast<int>(std::ceil(
							  trj.getSpeedPct(0u) * static_cast<double>(ufoRule->getTopSpeed()))));
				return ufo;
			}
			return nullptr; // No base to supply!
	}

	if (ufoRule != nullptr) // else create a UFO according to sequence
	{
		ufo = new Ufo(
					ufoRule,
					&_playSave);
		ufo->setUfoMissionInfo(
							this,
							&trj);
		const RuleRegion& regionRule (*rules.getRegion(_regionType));
		coord = coordsWaypoint(
							trj,
							0u,
							globe,
							regionRule);
		ufo->setLongitude(coord.first);
		ufo->setLatitude(coord.second);

		ufo->setAltitude(trj.getAltitude(0));
		if (trj.getAltitude(0u) == MovingTarget::stAltitude[0u])
			ufo->setSecondsLeft(trj.getGroundDuration());

		coord = coordsWaypoint(
							trj,
							1u,
							globe,
							regionRule);
		wp = new Waypoint();
		wp->setLongitude(coord.first);
		wp->setLatitude(coord.second);

		ufo->setTarget(wp); // NOTE: Target must be set before speed.

		double speedPct;
		if (trj.getMissionPointTotal() > 1u)
			speedPct = RNG::generate(
								trj.getSpeedPct(0u),
								trj.getSpeedPct(1u));
		else
			speedPct = trj.getSpeedPct(0u);
		//Log(LOG_INFO) << ". speedPct= " << speedPct;
		//Log(LOG_INFO) << ". topSpeed= " << ufoRule->getTopSpeed();
		//Log(LOG_INFO) << ". speed= " << static_cast<int>(std::ceil(speedPct * static_cast<double>(ufoRule->getTopSpeed())));

		ufo->setSpeed(static_cast<int>(std::ceil(
					  speedPct * static_cast<float>(ufoRule->getTopSpeed()))));
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
		bool operator ()(const Base* const base) const
		{
			return AreSameTwo(
							base->getLongitude(), _lon,
							base->getLatitude(),  _lat) == true;
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
	//Log(LOG_INFO) << "";
	//Log(LOG_INFO) << "AlienMission::ufoReachedWaypoint()";
	const UfoTrajectory& trj (ufo.getTrajectory());

	const size_t
		pointId		(ufo.getMissionPoint()),
		pointId_pst	(ufo.setMissionPoint());

	//Log(LOG_INFO) << ". pointId= "		<< pointId;
	//Log(LOG_INFO) << ". pointId_pst= "	<< pointId_pst;
	//Log(LOG_INFO) << ". pointsTotal= "	<< trj.getMissionPointTotal();

	if (pointId_pst != trj.getMissionPointTotal())
	{
		//Log(LOG_INFO) << ". . set next Waypoint";
		ufo.setAltitude(trj.getAltitude(pointId_pst));

		const RuleRegion& regionRule (*rules.getRegion(_regionType));

		Waypoint* const wp (new Waypoint());
		const std::pair<double, double> coord (coordsWaypoint(
															trj,
															pointId_pst,
															globe,
															regionRule));
		wp->setLongitude(coord.first);
		wp->setLatitude(coord.second);
		ufo.setTarget(wp); // NOTE: Target must be set before speed.

		if (ufo.getAltitude() == MovingTarget::stAltitude[0u]) // UFO landed.
		{
			//Log(LOG_INFO) << ". . . ufo is Landed";
			size_t wave;
			switch (_waveCount)
			{
				case 0u: // restart AlienMission -> if (RNG::percent(_playSave.getDifficultyInt() * 20) == false)
					wave = _missionRule.getWaveTotal() - 1u;
					break;
				default:
					wave = _waveCount - 1u;
			}
			// NOTE: 'wave' has to be reduced by one because think() has already advanced it past current, I suppose.

			if (_missionRule.getWaveData(wave).isObjective == true				// destroy UFO & replace with TerrorSite.
				&& trj.getZoneId(pointId) == _missionRule.getObjectiveZone())	// NOTE: Supply-missions bypasses this although it has (objective=true)
			{																	// because it does not have an 'objectiveZone' set in its rule.
				addScore( // alm_TERROR
						ufo.getLongitude(),
						ufo.getLatitude());

				ufo.setUfoStatus(Ufo::DESTROYED);

				// note: Looks like they're having probls with getting a mission's pointId:
//				MissionArea area (regionRule.getMissionZones().at(trj.getZoneId(pointId)).areas.at(_terrorZone));
				const MissionArea area (regionRule.getTerrorArea(
															trj.getZoneId(pointId),
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
					_playSave.getTerrorSites()->push_back(site);

					for (std::vector<Target*>::const_iterator
							i = ufo.getTargeters()->begin();
							i != ufo.getTargeters()->end();
							)
					{
						Craft* const craft (dynamic_cast<Craft*>(*i));
						if (craft != nullptr)
						{
							craft->setTarget(site);
							i = ufo.getTargeters()->begin();
						}
						else
							++i;
					}
				}
			}
			else if (trj.getType() == UfoTrajectory::XCOM_BASE_ASSAULT)	// remove UFO, replace with Base defense.
			{															// Ignore what the trajectory might say, this is a base defense.
				ufo.setDetected(false);
				const std::vector<Base*>::const_iterator i (std::find_if( // Only spawn mission if the base is still there.
																	_playSave.getBases()->begin(),
																	_playSave.getBases()->end(),
																	MatchBaseCoordinates(
																					ufo.getLongitude(),
																					ufo.getLatitude())));
				if (i != _playSave.getBases()->end())
					ufo.setTarget(*i);
				else
					ufo.setUfoStatus(Ufo::DESTROYED);
			}
			else // Set timer for UFO on the ground.
			{
				//Log(LOG_INFO) << ". . Landed ufo= " << ufo.getRules()->getType();
				if (globe.insideLand(
								ufo.getLongitude(),
								ufo.getLatitude()) == true)
				{
					//Log(LOG_INFO) << ". . . seconds left= " << trj.getGroundDuration();
					ufo.setSecondsLeft(trj.getGroundDuration());

					if (ufo.getDetected() == true && ufo.getLandId() == 0)
						ufo.setLandId(_playSave.getCanonicalId(Target::stTarget[5u]));
				}
				else // there's nothing to land on
					ufo.setSecondsLeft(5);
			}
		}
		else // UFO is Flying.
		{
			//Log(LOG_INFO) << ". . . ufo is Flying";
			ufo.setLandId(0);

			double speedPct (RNG::generate(
										trj.getSpeedPct(pointId),
										trj.getSpeedPct(pointId_pst)));
			ufo.setSpeed(static_cast<int>(std::ceil(
						 speedPct * static_cast<double>(ufo.getRules()->getTopSpeed()))));
		}
	}
	else // UFO left earth's atmosphere.
		ufo.setUfoStatus(Ufo::DESTROYED);
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
		site->setId(_playSave.getCanonicalId(ruleDeploy->getMarkerType()));
		site->setSecondsLeft(RNG::generate(
										ruleDeploy->getDurationMin(),
										ruleDeploy->getDurationMax()) * 3600);
		site->setAlienRace(_raceType);
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
	if (_playSave.getAlienBases()->size() <= 8u + (static_cast<size_t>(_playSave.getDifficulty()) << 1u))
	{
		const size_t zoneId (_missionRule.getObjectiveZone());
		std::vector<MissionArea> areas (rules.getRegion(_regionType)->getMissionZones().at(zoneId).areas);
		MissionArea area (areas.at(RNG::pick(areas.size())));


		// Question: What does Ruleset getDeployment() return when passed an empty string.
		// What do any of the Ruleset getters return when passed an empty string.
		// Because the excessive "modability" is verging on fucko'd.
		//
		// TODO: REVERT. Or smooth in the practical usefulness of getTerrorType().
		// (atm I'm dealing with 15000 warnings that the pop-code generates.)
		const RuleAlienDeployment* ruleDeploy;
		if (rules.getDeployment(_missionRule.getTerrorType()) != nullptr)
			ruleDeploy = rules.getDeployment(_missionRule.getTerrorType());
		else if (rules.getGlobe()->getTextureRule(area.texture) != nullptr
			&&   rules.getGlobe()->getTextureRule(area.texture)->getTextureDeployments().empty() == false)
		{
			ruleDeploy = rules.getDeployment(rules.getGlobe()->getTextureRule(area.texture)->getTextureDeployment());
		}
		else
			ruleDeploy = rules.getDeployment("STR_ALIEN_BASE_ASSAULT");

		if (ruleDeploy == nullptr)
		{
			std::string st ("No RuleAlienDeployment defined for aLien Base.");
			st += " A deployment-rule must be defined in one of the mission-zone's texture,";
			st += " the mission's siteType, or by defining a deployment called";
			st += " \"STR_ALIEN_BASE_ASSAULT\" as a default or fallback.";
			throw Exception(st);
		}

		const RuleRegion& regionRule (*rules.getRegion(_regionType));
		const std::pair<double, double> pos (coordsLandArea(
														globe,
														regionRule,
														area));

		AlienBase* const alienBase (new AlienBase(ruleDeploy));
		alienBase->setAlienRace(_raceType);
//		alienBase->setId(game.getId(deployment->getMarkerName())); // done in AlienBaseDetectedState.
		alienBase->setLongitude(pos.first);
		alienBase->setLatitude(pos.second);
		_playSave.getAlienBases()->push_back(alienBase);

		addScore( // alm_BASE, alm_INFILT
				pos.first,
				pos.second);
	}
}

/**
 * Sets the AlienBase associated with this AlienMission.
 * @note Only aLien supply-missions care about this.
 * @param alienBase - pointer to an AlienBase (default nullptr)
 */
void AlienMission::setAlienBase(const AlienBase* const alienBase)
{
	_alienBase = alienBase;
}

/**
 * Gets the AlienBase associated with this AlienMission.
 * @note Only aLien supply-missions ever have a valid pointer.
 * @return, pointer to the AlienBase for this mission (possibly nullptr)
 */
const AlienBase* AlienMission::getAlienBase() const
{
	return _alienBase;
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
	//Log(LOG_INFO) << "";
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

							std::vector<Country*> suspects;
							for (std::vector<Country*>::const_iterator
									i = _playSave.getCountries()->begin();
									i != _playSave.getCountries()->end();
									++i)
							{
								if ((*i)->getPactStatus() == PACT_NONE
									&& (*i)->getRules()->getCountryRegion() == _regionType)
								{
									suspects.push_back(*i);
								}
							}

							if (suspects.empty() == false)
							{
								Country* const infiltrated (suspects.at(RNG::pick(suspects.size())));
								//Log(LOG_INFO) << "AlienMission::ufoLifting(), GAAH! new Pact & aLien base: " << infiltrated->getType();
								if (infiltrated->getType() != "STR_RUSSIA")		// heh. ironically, this likely allows multiple alien
									infiltrated->setPactStatus(PACT_RECENT);	// bases in Russia due solely to infiltrations ...!!
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

			double speedPct;
			const size_t pointId (ufo.getMissionPoint());
			const UfoTrajectory& trj (ufo.getTrajectory());
			if (pointId + 1u < trj.getMissionPointTotal())
				speedPct = RNG::generate(
									trj.getSpeedPct(pointId),
									trj.getSpeedPct(pointId + 1u));
			else
				speedPct = trj.getSpeedPct(pointId);

			ufo.setSpeed(static_cast<int>(std::ceil(
						 speedPct * static_cast<double>(ufo.getRules()->getTopSpeed()))));
			break;
		}

		case Ufo::CRASHED: // Mission expired
			ufo.setUfoStatus(Ufo::DESTROYED);

//			break;
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
				_countdown += getStandardDelay(); // delay the next wave
	}
}

/**
 * Generates destination-coordinates based on a specified UfoTrajectory and
 * mission-point ID.
 * @param trj		- reference to the trajectory in question
 * @param pointId	- the next logical waypoint in sequence (0 for newly spawned UFOs)
 * @param globe		- reference to the Globe
 * @param region	- reference to the ruleset for the region of this mission
 * @return, pair of lon and lat coordinates based on the criteria of the trajectory
 */
std::pair<double, double> AlienMission::coordsWaypoint( // private.
		const UfoTrajectory& trj,
		const size_t pointId,
		const Globe& globe,
		const RuleRegion& region) const
{
	const size_t zoneId (trj.getZoneId(pointId));

	if (_terrorZone != std::numeric_limits<size_t>::max()
		&& zoneId == _missionRule.getObjectiveZone())
	{
		size_t wave;
		switch (_waveCount)
		{
			case 0u: // restart AlienMission -> if (RNG::percent(_playSave.getDifficultyInt() * 20) == false)
				wave = _missionRule.getWaveTotal() - 1u;
				break;
			default:
				wave = _waveCount - 1u;
		}
		// NOTE: 'wave' has to be reduced by one because think() has already advanced it past current, I suppose.

		if (_missionRule.getWaveData(wave).isObjective == true)
		{
			const MissionArea* const area (&region.getMissionZones()
													.at(_missionRule.getObjectiveType()).areas
													.at(_terrorZone));
			return std::make_pair(
								area->lonMin,
								area->latMin);
		}
	}

	const size_t pointId_post (pointId + 1u);								// if the point after the current point is ground,
	if (   trj.getMissionPointTotal() > pointId_post						// return land-coords for the current point.
		&& trj.getAltitude(pointId_post) == MovingTarget::stAltitude[0u])	// ... huh.
 	{
 		return coordsLandZone(
							globe,
							region,
							zoneId);
 	}

	return region.getZonePoint(zoneId);
}

/**
 * Generates destination-coordinates inside a specified Region and MissionZone.
 * @note The point will be used to land a UFO so it *has to be on land*.
 * @param globe		- reference to the Globe
 * @param region	- reference to RuleRegion
 * @param zoneId	- ID of a MissionZone in the Region
 * @return, a pair of doubles (lon & lat)
 */
std::pair<double, double> AlienMission::coordsLandZone( // private.
		const Globe& globe,
		const RuleRegion& region,
		const size_t zoneId) const
{
	std::pair<double, double> coord;

	int t (0);
	do
	{
		coord = region.getZonePoint(zoneId);
	}
	while ((globe.insideLand(
						coord.first,
						coord.second) == false
			|| region.insideRegion(
						coord.first,
						coord.second) == false)
		&& ++t < MAX_TRIES);

	if (t == MAX_TRIES)
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
std::pair<double, double> AlienMission::coordsLandArea( // private.
		const Globe& globe,
		const RuleRegion& region,
		const MissionArea& area) const
{
	std::pair<double, double> coord;

	int t (0);
	do
	{
		coord = region.getAreaPoint(area);
	}
	while ((globe.insideLand(
						coord.first,
						coord.second) == false
			|| region.insideRegion(
						coord.first,
						coord.second) == false)
		&& ++t < MAX_TRIES);

	if (t == MAX_TRIES)
		Log(LOG_INFO) << "Region: " << region.getType()
					  << " lon " << coord.first
					  << " lat " << coord.second
					  << " invalid point in area - ufo was forced to land on water.";
	return coord;
}

/**
 * Adds aLien-points to the Country and Region at specified coordinates.
 * @param lon		- longitudinal coordinate to check
 * @param lat		- latitudinal coordinate to check
 * @param ufoSize	- the UfoSizeType for infiltration and aLien-Base (RuleUfo.h) (default UFO_VERYLARGE)
 */
void AlienMission::addScore( // private.
		const double lon,
		const double lat,
		const UfoSizeType ufoSize) const
{
	int aLienPts (_missionRule.getMissionScore());
	if (aLienPts != 0)
	{
		switch (_missionRule.getObjectiveType())
		{
//			case alm_TERROR: break; // use default pt-value
//			case alm_RETAL:  break; // use default pt-value <- has 0 pts.

			case alm_INFILT:
			case alm_BASE:
				aLienPts += ((_playSave.getDifficultyInt() * (static_cast<int>(ufoSize) + 1)) << 2u)
						 +   (_playSave.getMonthsElapsed() << 1u);
				break;

			case alm_SUPPLY:
				aLienPts += _playSave.getDifficultyInt() * 10
						 + (_playSave.getMonthsElapsed() >> 1u);
				break;

			case alm_SCORE:
				aLienPts += (_playSave.getDifficultyInt() << 1u)
						 +   _playSave.getMonthsElapsed();
		}
		_playSave.scorePoints(lon,lat, aLienPts, true);
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
