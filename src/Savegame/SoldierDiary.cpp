/*
 * Copyright 2010-2015 OpenXcom Developers.
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

#include "SoldierDiary.h"

#include "BattleUnitStatistics.h"
#include "MissionStatistics.h"

#include "../Ruleset/RuleAward.h"
#include "../Ruleset/Ruleset.h"


namespace OpenXcom
{

/**
 * Initializes a new diary entry from YAML.
 * @param node - YAML node
 */
SoldierDiary::SoldierDiary(const YAML::Node& node)
{
	load(node);
}

/**
 * Initializes a new blank diary.
 */
SoldierDiary::SoldierDiary()
	:
		_scoreTotal(0),
		_pointTotal(0),
		_killTotal(0),
		_stunTotal(0),
		_missionTotal(0),
		_winTotal(0),
		_baseDefenseMissionTotal(0),
		_daysWoundedTotal(0),
		_terrorMissionTotal(0),
		_nightMissionTotal(0),
		_nightTerrorMissionTotal(0),
		_monthsService(0),
		_unconsciousTotal(0),
		_shotAtCounterTotal(0),
		_hitCounterTotal(0),
		_loneSurvivorTotal(0),
		_totalShotByFriendlyCounter(0),
		_totalShotFriendlyCounter(0),
		_ironManTotal(0),
		_importantMissionTotal(0),
		_longDistanceHitCounterTotal(0),
		_lowAccuracyHitCounterTotal(0),
		_shotsFiredCounterTotal(0),
		_shotsLandedCounterTotal(0),
		_shotAtCounter10in1Mission(0),
		_hitCounter5in1Mission(0),
		_reactionFireTotal(0),
		_timesWoundedTotal(0),
		_valiantCruxTotal(0),
		_KIA(0),
		_trapKillTotal(0),
		_alienBaseAssaultTotal(0),
		_allAliensKilledTotal(0),
		_mediApplicationsTotal(0),
		_revivedUnitTotal(0),
		_MIA(0)
{}

/**
 * Constructs a copy of a SoldierDiary.
 * @param copyThis - reference the diary to copy to this SoldierDiary
 */
SoldierDiary::SoldierDiary(const SoldierDiary& copyThis)
	:
		_scoreTotal(copyThis._scoreTotal),
		_pointTotal(copyThis._pointTotal),
		_killTotal(copyThis._killTotal),
		_stunTotal(copyThis._stunTotal),
		_missionTotal(copyThis._missionTotal),
		_winTotal(copyThis._winTotal),
		_daysWoundedTotal(copyThis._daysWoundedTotal),
		_baseDefenseMissionTotal(copyThis._baseDefenseMissionTotal),
		_totalShotByFriendlyCounter(copyThis._totalShotByFriendlyCounter),
		_totalShotFriendlyCounter(copyThis._totalShotFriendlyCounter),
		_loneSurvivorTotal(copyThis._loneSurvivorTotal),
		_terrorMissionTotal(copyThis._terrorMissionTotal),
		_nightMissionTotal(copyThis._nightMissionTotal),
		_nightTerrorMissionTotal(copyThis._nightTerrorMissionTotal),
		_monthsService(copyThis._monthsService),
		_unconsciousTotal(copyThis._unconsciousTotal),
		_shotAtCounterTotal(copyThis._shotAtCounterTotal),
		_hitCounterTotal(copyThis._hitCounterTotal),
		_ironManTotal(copyThis._ironManTotal),
		_importantMissionTotal(copyThis._importantMissionTotal),
		_longDistanceHitCounterTotal(copyThis._longDistanceHitCounterTotal),
		_lowAccuracyHitCounterTotal(copyThis._lowAccuracyHitCounterTotal),
		_shotsFiredCounterTotal(copyThis._shotsFiredCounterTotal),
		_shotsLandedCounterTotal(copyThis._shotsLandedCounterTotal),
		_shotAtCounter10in1Mission(copyThis._shotAtCounter10in1Mission),
		_hitCounter5in1Mission(copyThis._hitCounter5in1Mission),
		_reactionFireTotal(copyThis._reactionFireTotal),
		_timesWoundedTotal(copyThis._timesWoundedTotal),
		_valiantCruxTotal(copyThis._valiantCruxTotal),
		_KIA(copyThis._KIA),
		_trapKillTotal(copyThis._trapKillTotal),
		_alienBaseAssaultTotal(copyThis._alienBaseAssaultTotal),
		_allAliensKilledTotal(copyThis._allAliensKilledTotal),
		_mediApplicationsTotal(copyThis._mediApplicationsTotal),
		_revivedUnitTotal(copyThis._revivedUnitTotal),
		_MIA(copyThis._MIA)
{
	for (size_t
			i = 0;
			i != copyThis._missionIdList.size();
			++i)
	{
//		if (copyThis._missionIdList.at(i) != nullptr) // Bzzzt.
		_missionIdList.push_back(copyThis._missionIdList.at(i));
	}

	std::map<std::string, int>::const_iterator pCopy;
	for (
			pCopy = copyThis._regionTotal.begin();
			pCopy != copyThis._regionTotal.end();
			++pCopy)
	{
		_regionTotal[(*pCopy).first] = (*pCopy).second;
	}

	for (
			pCopy = copyThis._countryTotal.begin();
			pCopy != copyThis._countryTotal.end();
			++pCopy)
	{
		_countryTotal[(*pCopy).first] = (*pCopy).second;
	}

	for (
			pCopy = copyThis._typeTotal.begin();
			pCopy != copyThis._typeTotal.end();
			++pCopy)
	{
		_typeTotal[(*pCopy).first] = (*pCopy).second;
	}

	for (
			pCopy = copyThis._ufoTotal.begin();
			pCopy != copyThis._ufoTotal.end();
			++pCopy)
	{
		_ufoTotal[(*pCopy).first] = (*pCopy).second;
	}


	for (size_t
			i = 0;
			i != copyThis._solAwards.size();
			++i)
	{
		if (copyThis._solAwards.at(i) != nullptr)
		{
			std::string
				type = copyThis._solAwards.at(i)->getType(),
				noun = copyThis._solAwards.at(i)->getQualifier();

			_solAwards.push_back(new SoldierAward(type, noun));
		}
	}

	for (size_t
			i = 0;
			i != copyThis._killList.size();
			++i)
	{
		if (copyThis._killList.at(i) != nullptr)
		{
			std::string
				unitRank = copyThis._killList.at(i)->_rank,
				race = copyThis._killList.at(i)->_race,
				weapon = copyThis._killList.at(i)->_weapon,
				weaponAmmo = copyThis._killList.at(i)->_weaponAmmo;
			int
				mission = copyThis._killList.at(i)->_mission,
				turn = copyThis._killList.at(i)->_turn,
				points = copyThis._killList.at(i)->_points;

			UnitFaction faction = copyThis._killList.at(i)->_faction;
			UnitStatus status = copyThis._killList.at(i)->_status;

			_killList.push_back(new BattleUnitKill(
												unitRank,
												race,
												weapon,
												weaponAmmo,
												faction,
												status,
												mission,
												turn,
												points));
		}
	}
}

/**
 * dTor.
 */
SoldierDiary::~SoldierDiary()
{
	for (std::vector<SoldierAward*>::const_iterator
			i = _solAwards.begin();
			i != _solAwards.end();
			++i)
	{
		delete *i;
	}

	for (std::vector<BattleUnitKill*>::const_iterator
			i = _killList.begin();
			i != _killList.end();
			++i)
	{
		delete *i;
	}
}

/**
 * Overloads the assignment operator.
 * @param assignThis - reference the diary to assign to this SoldierDiary
 * @return, address of the new diary
 */
SoldierDiary& SoldierDiary::operator= (const SoldierDiary& assignThis)
{
	if (this != &assignThis)
	{
		_scoreTotal = assignThis._scoreTotal;
		_pointTotal = assignThis._pointTotal;
		_killTotal = assignThis._killTotal;
		_stunTotal = assignThis._stunTotal;
		_missionTotal = assignThis._missionTotal;
		_winTotal = assignThis._winTotal;
		_daysWoundedTotal = assignThis._daysWoundedTotal;
		_baseDefenseMissionTotal = assignThis._baseDefenseMissionTotal;
		_totalShotByFriendlyCounter = assignThis._totalShotByFriendlyCounter;
		_totalShotFriendlyCounter = assignThis._totalShotFriendlyCounter;
		_loneSurvivorTotal = assignThis._loneSurvivorTotal;
		_terrorMissionTotal = assignThis._terrorMissionTotal;
		_nightMissionTotal = assignThis._nightMissionTotal;
		_nightTerrorMissionTotal = assignThis._nightTerrorMissionTotal;
		_monthsService = assignThis._monthsService;
		_unconsciousTotal = assignThis._unconsciousTotal;
		_shotAtCounterTotal = assignThis._shotAtCounterTotal;
		_hitCounterTotal = assignThis._hitCounterTotal;
		_ironManTotal = assignThis._ironManTotal;
		_importantMissionTotal = assignThis._importantMissionTotal;
		_longDistanceHitCounterTotal = assignThis._longDistanceHitCounterTotal;
		_lowAccuracyHitCounterTotal = assignThis._lowAccuracyHitCounterTotal;
		_shotsFiredCounterTotal = assignThis._shotsFiredCounterTotal;
		_shotsLandedCounterTotal = assignThis._shotsLandedCounterTotal;
		_shotAtCounter10in1Mission = assignThis._shotAtCounter10in1Mission;
		_hitCounter5in1Mission = assignThis._hitCounter5in1Mission;
		_reactionFireTotal = assignThis._reactionFireTotal;
		_timesWoundedTotal = assignThis._timesWoundedTotal;
		_valiantCruxTotal = assignThis._valiantCruxTotal;
		_KIA = assignThis._KIA;
		_trapKillTotal = assignThis._trapKillTotal;
		_alienBaseAssaultTotal = assignThis._alienBaseAssaultTotal;
		_allAliensKilledTotal = assignThis._allAliensKilledTotal;
		_mediApplicationsTotal = assignThis._mediApplicationsTotal;
		_revivedUnitTotal = assignThis._revivedUnitTotal;
		_MIA = assignThis._MIA;

		_missionIdList.clear();
		for (std::vector<int>::const_iterator
				i = assignThis._missionIdList.begin();
				i != assignThis._missionIdList.end();
				++i)
		{
			_missionIdList.push_back(*i);
		}

		_regionTotal.clear();
		std::map<std::string, int>::const_iterator pCopy;
		for (
				pCopy = assignThis._regionTotal.begin();
				pCopy != assignThis._regionTotal.end();
				++pCopy)
		{
			_regionTotal[(*pCopy).first] = (*pCopy).second;
		}

		_countryTotal.clear();
		for (
				pCopy = assignThis._countryTotal.begin();
				pCopy != assignThis._countryTotal.end();
				++pCopy)
		{
			_countryTotal[(*pCopy).first] = (*pCopy).second;
		}

		_typeTotal.clear();
		for (
				pCopy = assignThis._typeTotal.begin();
				pCopy != assignThis._typeTotal.end();
				++pCopy)
		{
			_typeTotal[(*pCopy).first] = (*pCopy).second;
		}

		_ufoTotal.clear();
		for (
				pCopy = assignThis._ufoTotal.begin();
				pCopy != assignThis._ufoTotal.end();
				++pCopy)
		{
			_ufoTotal[(*pCopy).first] = (*pCopy).second;
		}


		for (std::vector<SoldierAward*>::const_iterator
				i = _solAwards.begin();
				i != _solAwards.end();
				++i)
		{
			delete *i;
		}

		for (std::vector<BattleUnitKill*>::const_iterator
				i = _killList.begin();
				i != _killList.end();
				++i)
		{
			delete *i;
		}

		for (size_t
				i = 0;
				i != assignThis._solAwards.size();
				++i)
		{
			if (assignThis._solAwards.at(i) != nullptr)
			{
				std::string
					type = assignThis._solAwards.at(i)->getType(),
					noun = assignThis._solAwards.at(i)->getQualifier();

				_solAwards.push_back(new SoldierAward(type, noun));
			}
		}

		for (size_t
				i = 0;
				i != assignThis._killList.size();
				++i)
		{
			if (assignThis._killList.at(i) != nullptr)
			{
				std::string
					unitRank = assignThis._killList.at(i)->_rank,
					race = assignThis._killList.at(i)->_race,
					weapon = assignThis._killList.at(i)->_weapon,
					weaponAmmo = assignThis._killList.at(i)->_weaponAmmo;
				int
					mission = assignThis._killList.at(i)->_mission,
					turn = assignThis._killList.at(i)->_turn,
					points = assignThis._killList.at(i)->_points;

				UnitFaction faction = assignThis._killList.at(i)->_faction;
				UnitStatus status = assignThis._killList.at(i)->_status;

				_killList.push_back(new BattleUnitKill(
													unitRank,
													race,
													weapon,
													weaponAmmo,
													faction,
													status,
													mission,
													turn,
													points));
			}
		}
	}

	return *this;
}

/**
 * Loads the diary from a YAML file.
 * @param node - reference a YAML node
 */
void SoldierDiary::load(const YAML::Node& node)
{
	if (const YAML::Node& awards = node["awards"])
	{
		for (YAML::const_iterator
				i = awards.begin();
				i != awards.end();
				++i)
		{
			_solAwards.push_back(new SoldierAward(*i));
		}
	}

	if (const YAML::Node& killList = node["killList"])
	{
		for (YAML::const_iterator
				i = killList.begin();
				i != killList.end();
				++i)
		{
			_killList.push_back(new BattleUnitKill(*i));
		}
	}

	_missionIdList					= node["missionIdList"]					.as<std::vector<int>>(_missionIdList);
	_regionTotal					= node["regionTotal"]					.as<std::map<std::string, int>>(_regionTotal);
	_countryTotal					= node["countryTotal"]					.as<std::map<std::string, int>>(_countryTotal);
	_typeTotal						= node["typeTotal"]						.as<std::map<std::string, int>>(_typeTotal);
	_ufoTotal						= node["ufoTotal"]						.as<std::map<std::string, int>>(_ufoTotal);
	_scoreTotal						= node["scoreTotal"]					.as<int>(_scoreTotal);
	_pointTotal						= node["pointTotal"]					.as<int>(_pointTotal);
	_killTotal						= node["killTotal"]						.as<int>(_killTotal);
	_stunTotal						= node["stunTotal"]						.as<int>(_stunTotal);
	_missionTotal					= node["missionTotal"]					.as<int>(_missionTotal);
	_winTotal						= node["winTotal"]						.as<int>(_winTotal);
	_daysWoundedTotal				= node["daysWoundedTotal"]				.as<int>(_daysWoundedTotal);
	_baseDefenseMissionTotal		= node["baseDefenseMissionTotal"]		.as<int>(_baseDefenseMissionTotal);
	_totalShotByFriendlyCounter		= node["totalShotByFriendlyCounter"]	.as<int>(_totalShotByFriendlyCounter);
	_totalShotFriendlyCounter		= node["totalShotFriendlyCounter"]		.as<int>(_totalShotFriendlyCounter);
	_loneSurvivorTotal				= node["loneSurvivorTotal"]				.as<int>(_loneSurvivorTotal);
	_terrorMissionTotal				= node["terrorMissionTotal"]			.as<int>(_terrorMissionTotal);
	_nightMissionTotal				= node["nightMissionTotal"]				.as<int>(_nightMissionTotal);
	_nightTerrorMissionTotal		= node["nightTerrorMissionTotal"]		.as<int>(_nightTerrorMissionTotal);
	_monthsService					= node["monthsService"]					.as<int>(_monthsService);
	_unconsciousTotal				= node["unconsciousTotal"]				.as<int>(_unconsciousTotal);
	_shotAtCounterTotal				= node["shotAtCounterTotal"]			.as<int>(_shotAtCounterTotal);
	_hitCounterTotal				= node["hitCounterTotal"]				.as<int>(_hitCounterTotal);
	_ironManTotal					= node["ironManTotal"]					.as<int>(_ironManTotal);
	_importantMissionTotal			= node["importantMissionTotal"]			.as<int>(_importantMissionTotal);
	_longDistanceHitCounterTotal	= node["longDistanceHitCounterTotal"]	.as<int>(_longDistanceHitCounterTotal);
	_lowAccuracyHitCounterTotal		= node["lowAccuracyHitCounterTotal"]	.as<int>(_lowAccuracyHitCounterTotal);
	_shotsFiredCounterTotal			= node["shotsFiredCounterTotal"]		.as<int>(_shotsFiredCounterTotal);
	_shotsLandedCounterTotal		= node["shotsLandedCounterTotal"]		.as<int>(_shotsLandedCounterTotal);
	_shotAtCounter10in1Mission		= node["shotAtCounter10in1Mission"]		.as<int>(_shotAtCounter10in1Mission);
	_hitCounter5in1Mission			= node["hitCounter5in1Mission"]			.as<int>(_hitCounter5in1Mission);
	_reactionFireTotal				= node["reactionFireTotal"]				.as<int>(_reactionFireTotal);
	_timesWoundedTotal				= node["timesWoundedTotal"]				.as<int>(_timesWoundedTotal);
	_valiantCruxTotal				= node["valiantCruxTotal"]				.as<int>(_valiantCruxTotal);
	_trapKillTotal					= node["trapKillTotal"]					.as<int>(_trapKillTotal);
	_alienBaseAssaultTotal			= node["alienBaseAssaultTotal"]			.as<int>(_alienBaseAssaultTotal);
	_allAliensKilledTotal			= node["allAliensKilledTotal"]			.as<int>(_allAliensKilledTotal);
	_mediApplicationsTotal			= node["mediApplicationsTotal"]			.as<int>(_mediApplicationsTotal);
	_revivedUnitTotal				= node["revivedUnitTotal"]				.as<int>(_revivedUnitTotal);

	_KIA =
	_MIA = 0;
}

/**
 * Saves the diary to a YAML file.
 * @return, YAML node
 */
YAML::Node SoldierDiary::save() const
{
	YAML::Node node;

	for (std::vector<SoldierAward*>::const_iterator
			i = _solAwards.begin();
			i != _solAwards.end();
			++i)
	{
		node["awards"].push_back((*i)->save());
	}

	for (std::vector<BattleUnitKill*>::const_iterator
			i = _killList.begin();
			i != _killList.end();
			++i)
	{
		node["killList"].push_back((*i)->save());
	}

	if (_missionIdList.empty() == false)	node["missionIdList"]				= _missionIdList;
	if (_regionTotal.empty() == false)		node["regionTotal"]					= _regionTotal;
	if (_countryTotal.empty() == false)		node["countryTotal"]				= _countryTotal;
	if (_typeTotal.empty() == false)		node["typeTotal"]					= _typeTotal;
	if (_ufoTotal.empty() == false)			node["ufoTotal"]					= _ufoTotal;
	if (_scoreTotal)						node["scoreTotal"]					= _scoreTotal;
	if (_pointTotal)						node["pointTotal"]					= _pointTotal;
	if (_killTotal)							node["killTotal"]					= _killTotal;
	if (_stunTotal)							node["stunTotal"]					= _stunTotal;
	if (_missionTotal)						node["missionTotal"]				= _missionTotal;
	if (_winTotal)							node["winTotal"]					= _winTotal;
	if (_daysWoundedTotal)					node["daysWoundedTotal"]			= _daysWoundedTotal;
	if (_baseDefenseMissionTotal)			node["baseDefenseMissionTotal"]		= _baseDefenseMissionTotal;
	if (_totalShotByFriendlyCounter)		node["totalShotByFriendlyCounter"]	= _totalShotByFriendlyCounter;
	if (_totalShotFriendlyCounter)			node["totalShotFriendlyCounter"]	= _totalShotFriendlyCounter;
	if (_loneSurvivorTotal)					node["loneSurvivorTotal"]			= _loneSurvivorTotal;
	if (_terrorMissionTotal)				node["terrorMissionTotal"]			= _terrorMissionTotal;
	if (_nightMissionTotal)					node["nightMissionTotal"]			= _nightMissionTotal;
	if (_nightTerrorMissionTotal)			node["nightTerrorMissionTotal"]		= _nightTerrorMissionTotal;
	if (_monthsService)						node["monthsService"]				= _monthsService;
	if (_unconsciousTotal)					node["unconsciousTotal"]			= _unconsciousTotal;
	if (_shotAtCounterTotal)				node["shotAtCounterTotal"]			= _shotAtCounterTotal;
	if (_hitCounterTotal)					node["hitCounterTotal"]				= _hitCounterTotal;
	if (_ironManTotal)						node["ironManTotal"]				= _ironManTotal;
	if (_importantMissionTotal)				node["importantMissionTotal"]		= _importantMissionTotal;
	if (_longDistanceHitCounterTotal)		node["longDistanceHitCounterTotal"]	= _longDistanceHitCounterTotal;
	if (_lowAccuracyHitCounterTotal)		node["lowAccuracyHitCounterTotal"]	= _lowAccuracyHitCounterTotal;
	if (_shotsFiredCounterTotal)			node["shotsFiredCounterTotal"]		= _shotsFiredCounterTotal;
	if (_shotsLandedCounterTotal)			node["shotsLandedCounterTotal"]		= _shotsLandedCounterTotal;
	if (_shotAtCounter10in1Mission)			node["shotAtCounter10in1Mission"]	= _shotAtCounter10in1Mission;
	if (_hitCounter5in1Mission)				node["hitCounter5in1Mission"]		= _hitCounter5in1Mission;
	if (_reactionFireTotal)					node["reactionFireTotal"]			= _reactionFireTotal;
	if (_timesWoundedTotal)					node["timesWoundedTotal"]			= _timesWoundedTotal;
	if (_valiantCruxTotal)					node["valiantCruxTotal"]			= _valiantCruxTotal;
	if (_trapKillTotal)						node["trapKillTotal"]				= _trapKillTotal;
	if (_alienBaseAssaultTotal)				node["alienBaseAssaultTotal"]		= _alienBaseAssaultTotal;
	if (_allAliensKilledTotal)				node["allAliensKilledTotal"]		= _allAliensKilledTotal;
	if (_mediApplicationsTotal)				node["mediApplicationsTotal"]		= _mediApplicationsTotal;
	if (_revivedUnitTotal)					node["revivedUnitTotal"]			= _revivedUnitTotal;

	return node;
}

/**
 * Updates this SoldierDiary's statistics.
 * @note BattleUnitKill is a substruct of BattleUnitStatistics.
 * @param unitStatistics	- pointer to BattleUnitStatistics to get stats from (BattleUnitStatistics.h)
 * @param missionStatistics	- pointer to MissionStatistics to get stats from (MissionStatistics.h)
 * @param rules				- pointer to Ruleset
 */
void SoldierDiary::updateDiary(
		const BattleUnitStatistics* const unitStatistics,
		MissionStatistics* const missionStatistics,
		const Ruleset* const rules)
{
	//Log(LOG_INFO) << "SoldierDiary::updateDiary()";
	const std::vector<BattleUnitKill*> unitKills = unitStatistics->kills;
	for (std::vector<BattleUnitKill*>::const_iterator
			i = unitKills.begin();
			i != unitKills.end();
			++i)
	{
		_killList.push_back(*i);

		(*i)->makeTurnUnique();

		_pointTotal += (*i)->_points; // kL - if hostile unit was MC'd this should be halved

		if ((*i)->_faction == FACTION_HOSTILE)
		{
			if ((*i)->_status == STATUS_DEAD)
				++_killTotal;
			else //if ((*i)->_status == STATUS_UNCONSCIOUS)
				++_stunTotal;

			if ((*i)->hostileTurn() == true)
			{
				const RuleItem* const itRule (rules->getItem((*i)->_weapon));
				if (itRule == nullptr || itRule->isGrenade() == true)
					++_trapKillTotal;
				else
					++_reactionFireTotal;
			}
		}
	}

	++_regionTotal[missionStatistics->region];
	++_countryTotal[missionStatistics->country];
	++_typeTotal[missionStatistics->type]; // 'type' was, getMissionTypeLowerCase()
	++_ufoTotal[missionStatistics->ufo];
	_scoreTotal += missionStatistics->score;

	if (missionStatistics->success == true)
	{
		++_winTotal;

		if (   missionStatistics->type != "STR_SMALL_SCOUT"
			&& missionStatistics->type != "STR_MEDIUM_SCOUT"
			&& missionStatistics->type != "STR_LARGE_SCOUT"
			&& missionStatistics->type != "STR_SUPPLY_SHIP")
		{
			++_importantMissionTotal;
		}

		if (missionStatistics->type == "STR_ALIEN_BASE_ASSAULT")
			++_alienBaseAssaultTotal;

		if (unitStatistics->loneSurvivor == true)
			++_loneSurvivorTotal;

		if (unitStatistics->ironMan == true)
			++_ironManTotal;
	}

	if (unitStatistics->daysWounded > 0)
	{
		_daysWoundedTotal += unitStatistics->daysWounded;
		++_timesWoundedTotal;
	}

	if (missionStatistics->type == "STR_BASE_DEFENSE")
		++_baseDefenseMissionTotal;
	else if (missionStatistics->type == "STR_TERROR_MISSION")
	{
		++_terrorMissionTotal;

		if (missionStatistics->shade > 8)
			++_nightTerrorMissionTotal;
	}

	if (missionStatistics->shade > 8)
		++_nightMissionTotal;

	if (unitStatistics->wasUnconscious == true)
		++_unconsciousTotal;

	_shotAtCounterTotal += unitStatistics->shotAtCounter;
	_shotAtCounter10in1Mission += (unitStatistics->shotAtCounter) / 10;
	_hitCounterTotal += unitStatistics->hitCounter;
	_hitCounter5in1Mission += (unitStatistics->hitCounter) / 5;
	_totalShotByFriendlyCounter += unitStatistics->shotByFriendlyCounter;
	_totalShotFriendlyCounter += unitStatistics->shotFriendlyCounter;
	_longDistanceHitCounterTotal += unitStatistics->longDistanceHitCounter;
	_lowAccuracyHitCounterTotal += unitStatistics->lowAccuracyHitCounter;
	_mediApplicationsTotal += unitStatistics->medikitApplications;
	_revivedUnitTotal += unitStatistics->revivedSoldier;

	if (missionStatistics->valiantCrux == true)
		++_valiantCruxTotal;

	if (unitStatistics->KIA == true)
		++_KIA;

	if (unitStatistics->MIA == true)
		++_MIA;

	if (unitStatistics->nikeCross == true)
		++_allAliensKilledTotal;

	_missionIdList.push_back(missionStatistics->id);

//	_missionTotal = _missionIdList.size(); // CAN GET RID OF MISSION TOTAL
	//Log(LOG_INFO) << "SoldierDiary::updateDiary() EXIT";
}

/**
 * Get soldier awards.
 * @return, pointer to a vector of pointers to SoldierAward - a list of soldier's awards
 */
std::vector<SoldierAward*>* SoldierDiary::getSoldierAwards()
{
	return &_solAwards;
}

/**
 * Manage the soldier's awards - award new ones if earned.
 * @param rules - pointer to the Ruleset
 * @return, true if an award is awarded
 */
bool SoldierDiary::manageAwards(const Ruleset* const rules)
{
	//Log(LOG_INFO) << "";
	//Log(LOG_INFO) << "Diary: manageAwards()";
	bool
		doCeremony (false),	// this value is returned TRUE if at least one award is given.
		grantAward;			// this value determines if an award will be given.

	std::vector<std::string> qualifiedAwards;	// <types>
	std::map<std::string, size_t> levelReq;		// <noun, qtyLevels required>

	const std::map<std::string, RuleAward*> awardsList = rules->getAwardsList(); // loop over all possible RuleAwards.
	for (std::map<std::string, RuleAward*>::const_iterator
			i = awardsList.begin();
			i != awardsList.end();
			)
	{
		//Log(LOG_INFO) << ". [1] iter awardList - " << (*i).first;
		qualifiedAwards.clear();
		levelReq.clear();
		levelReq["noQual"] = 0;

		// loop over all of soldier's current Awards; map the award's qualifier w/ next-required level.
		const std::string type = (*i).first;
		for (std::vector<SoldierAward*>::const_iterator
				j = _solAwards.begin();
				j != _solAwards.end();
				++j)
		{
			if ((*j)->getType() == type)
			{
				//Log(LOG_INFO) << ". . set Level[" << ((*j)->getClassLevel() + 1)  << "] req'd for Qualifier \"" << (*j)->getQualifier() << "\"";
				levelReq[(*j)->getQualifier()] = (*j)->getClassLevel() + 1;
			}
		}

		// go through each possible criteria. Assume the award is awarded, set to FALSE if not;
		// ie, as soon as an award criteria that *fails to be achieved* is found, then no award.
		grantAward = true;

		const std::map<std::string, std::vector<int> >* criteriaList = (*i).second->getCriteria();
		for (std::map<std::string, std::vector<int> >::const_iterator
				j = criteriaList->begin();
				j != criteriaList->end();
				++j)
		{
			//Log(LOG_INFO) << ". . [2] iter Criteria " << (*j).first;
			// skip a "noQual" award if its max award level has been reached
			// or if it has a qualifier skip it if it has 0 total levels (which ain't gonna happen);
			// you see, Rules can't be positively examined for nouns - only awards already given to soldiers can.
			if ((*j).second.size() <= levelReq["noQual"])
			{
				//Log(LOG_INFO) << ". . . max \"noQual\" Level reached (or, Criteria has no vector)";
				grantAward = false;
				break;
			}

			const std::string criteriaType = (*j).first; // vector of (ints) mapped to a (string). Eg, "totalByNoun" incl. "noQual".

			// these criteria have no nouns, so only the levelReq["noQual"] will ever be compared
			const int val = (*j).second.at(levelReq["noQual"]);
			if ( //levelReq.count("noQual") == 1 && // <- this is relevant only if entry "noQual" were removed from the map in the sections following this one.
				(criteriaType == "totalKills"						&& static_cast<int>(_killList.size()) < val)
					|| (criteriaType == "totalMissions"				&& static_cast<int>(_missionIdList.size()) < val)
					|| (criteriaType == "totalWins"					&& _winTotal < val)
					|| (criteriaType == "totalScore"				&& _scoreTotal < val)
					|| (criteriaType == "totalPoints"				&& _pointTotal < val)
					|| (criteriaType == "totalStuns"				&& _stunTotal < val)
					|| (criteriaType == "totalBaseDefenseMissions"	&& _baseDefenseMissionTotal < val)
					|| (criteriaType == "totalTerrorMissions"		&& _terrorMissionTotal < val)
					|| (criteriaType == "totalNightMissions"		&& _nightMissionTotal < val)
					|| (criteriaType == "totalNightTerrorMissions"	&& _nightTerrorMissionTotal < val)
					|| (criteriaType == "totalMonthlyService"		&& _monthsService < val)
					|| (criteriaType == "totalFellUnconscious"		&& _unconsciousTotal < val)
					|| (criteriaType == "totalShotAt10Times"		&& _shotAtCounter10in1Mission < val)
					|| (criteriaType == "totalHit5Times"			&& _hitCounter5in1Mission < val)
					|| (criteriaType == "totalFriendlyFired"		&& (_totalShotByFriendlyCounter < val || _KIA != 0 || _MIA != 0)) // didn't survive ......
					|| (criteriaType == "totalLoneSurvivor"			&& _loneSurvivorTotal < val)
					|| (criteriaType == "totalIronMan"				&& _ironManTotal < val)
					|| (criteriaType == "totalImportantMissions"	&& _importantMissionTotal < val)
					|| (criteriaType == "totalLongDistanceHits"		&& _longDistanceHitCounterTotal < val)
					|| (criteriaType == "totalLowAccuracyHits"		&& _lowAccuracyHitCounterTotal < val)
					|| (criteriaType == "totalReactionFire"			&& _reactionFireTotal < val)
					|| (criteriaType == "totalTimesWounded"			&& _timesWoundedTotal < val)
					|| (criteriaType == "totalDaysWounded"			&& _daysWoundedTotal < val)
					|| (criteriaType == "totalValientCrux"			&& _valiantCruxTotal < val)
					|| (criteriaType == "isDead"					&& _KIA < val)
					|| (criteriaType == "totalTrapKills"			&& _trapKillTotal < val)
					|| (criteriaType == "totalAlienBaseAssaults"	&& _alienBaseAssaultTotal < val)
					|| (criteriaType == "totalAllAliensKilled"		&& _allAliensKilledTotal < val)
					|| (criteriaType == "totalMediApplications"		&& _mediApplicationsTotal < val)
					|| (criteriaType == "totalRevives"				&& _revivedUnitTotal < val)
					|| (criteriaType == "isMIA"						&& _MIA < val))
			{
				//Log(LOG_INFO) << ". . . no Award w/ \"noQual\"";
				grantAward = false;
				break;
			}
			else if (criteriaType == "totalKillsWithAWeapon"	// awards with the following criteria are unique because they need a qualifier
				|| criteriaType == "totalMissionsInARegion"		// and they loop over a map<> (this allows for super-good-plus modability).
				|| criteriaType == "totalKillsByRace"
				|| criteriaType == "totalKillsByRank")
			{
				//Log(LOG_INFO) << ". . . try Award w/ weapon,region,race,rank";
				std::map<std::string, int> total;
				if (criteriaType == "totalKillsWithAWeapon")
					total = getWeaponTotal();
				else if (criteriaType == "totalMissionsInARegion")
					total = _regionTotal;
				else if (criteriaType == "totalKillsByRace")
					total = getAlienRaceTotal();
				else if (criteriaType == "totalKillsByRank")
					total = getAlienRankTotal();

				for (std::map<std::string, int>::const_iterator // loop over the 'total' map and match Qualifiers with Levels.
						k = total.begin();
						k != total.end();
						++k)
				{
					//Log(LOG_INFO) << ". . . . [3] " << (*k).first << " - " << (*k).second;
					int threshold = -1;
					const std::string qualifier = (*k).first;
					if (levelReq.count(qualifier) == 0)					// if there is no matching Qualifier get the first criteria
					{
						//Log(LOG_INFO) << ". . . . . no relevant qualifier yet, threshold = " << (*j).second.front();
						threshold = (*j).second.front();
					}
					else if (levelReq[qualifier] != (*j).second.size())	// otherwise get the criteria per the soldier's award Level.
					{
						//Log(LOG_INFO) << ". . . . . qualifier found, next level available, threshold = " << (*j).second.at(levelReq[qualifier]);
						threshold = (*j).second.at(levelReq[qualifier]);
					}

					if (threshold != -1 && threshold <= (*k).second)	// if a criteria was set AND the stat's count exceeds that criteria ...
					{
						//Log(LOG_INFO) << ". . . . . threshold good, add to qualifiedAwards vector";
						qualifiedAwards.push_back(qualifier);
					}
				}

				if (qualifiedAwards.empty() == true) // if 'qualifiedAwards' is still empty soldier did not get an award.
				{
					//Log(LOG_INFO) << ". . . . no Award w/ weapon,region,race,rank";
					grantAward = false;
					break;
				}
			}
			else if (criteriaType == "killsWithCriteriaCareer"
				|| criteriaType == "killsWithCriteriaMission"
				|| criteriaType == "killsWithCriteriaTurn")
			{
				//Log(LOG_INFO) << ". . . try Award w/ career,mission,turn";
				const std::vector<std::map<int, std::vector<std::string> > >* killCriteriaList = (*i).second->getKillCriteria(); // fetch the killCriteria list.
				for (std::vector<std::map<int, std::vector<std::string> > >::const_iterator // loop over the OR vectors.
						orCriteria = killCriteriaList->begin();
						orCriteria != killCriteriaList->end();
						++orCriteria)
				{
					//Log(LOG_INFO) << ". . . . [3] iter killCriteria OR list";// << (*orCriteria)->;
					for (std::map<int, std::vector<std::string> >::const_iterator // loop over the AND vectors.
							andCriteria = orCriteria->begin();
							andCriteria != orCriteria->end();
							++andCriteria)
					{
						//Log(LOG_INFO) << ". . . . . [4] iter killCriteria AND list";// << *andCriteria->second.begin();
						int qty = 0; // how many AND vectors (list of DETAILs) have been successful.
						if (criteriaType != "killsWithCriteriaCareer")
						{
							++qty; // "killsWith..." Turns or Missions start at 1 because of how thisIter and lastIter work.
						}
						//Log(LOG_INFO) << ". . . . . start Qty = " << qty;

						bool skip = false;
						int
							thisIter = -1, // being a turn or a mission
							lastIter = -1;
						//Log(LOG_INFO) << ". . . . . init skip= false, thisIter/lastIter= -1";

						for (std::vector<BattleUnitKill*>::const_iterator // loop over the KILLS vector.
								kill = _killList.begin();
								kill != _killList.end();
								++kill)
						{
							//Log(LOG_INFO) << ". . . . . . [5] iter KILLS";
							if (criteriaType == "killsWithCriteriaMission")
							{
								thisIter = (*kill)->_mission;
								if (kill != _killList.begin())
								{
									lastIter = (*(kill - 1))->_mission;
//									--kill;
//									lastIter = (*kill)->_mission;
//									++kill;
								}
							}
							else if (criteriaType == "killsWithCriteriaTurn")
							{
								thisIter = (*kill)->_turn;
								if (kill != _killList.begin())
								{
									lastIter = (*(kill - 1))->_turn;
//									--kill;
//									lastIter = (*kill)->_turn;
//									++kill;
								}
							}
							//Log(LOG_INFO) << ". . . . . . " << criteriaType;
							//Log(LOG_INFO) << ". . . . . . skip = " << skip;
							//Log(LOG_INFO) << ". . . . . . thisIter = " << thisIter;
							//Log(LOG_INFO) << ". . . . . . lastIter = " << lastIter;

							if (criteriaType != "killsWithCriteriaCareer"	// skip kill-groups that soldier already got an award
								&& thisIter == lastIter						// for and skip kills that are inbetween turns.
								&& skip == true)
							{
								//Log(LOG_INFO) << ". . . . . . . continue [1]";
								continue;
							}

							if (thisIter != lastIter)
							{
								qty = 1; // reset.
								skip = false;
								//Log(LOG_INFO) << ". . . . . . . continue [2]";
								continue;
							}

							bool found = true;

							for (std::vector<std::string>::const_iterator // loop over the DETAILs of the AND vector.
									detail = andCriteria->second.begin();
									detail != andCriteria->second.end();
									++detail)
							{
								//Log(LOG_INFO) << ". . . . . . . [6] iter DETAIL = " << (*detail);
								size_t
									bType = 0,
									dType = 0;

								static const std::string
									bType_array[BATS] =
									{
										"BT_NONE",		"BT_FIREARM",		"BT_AMMO",		"BT_MELEE",
										"BT_GRENADE",	"BT_PROXYGRENADE",	"BT_MEDIKIT",	"BT_SCANNER",
										"BT_MINDPROBE",	"BT_PSIAMP",		"BT_FLARE",		"BT_CORPSE",
										"BT_END"
									},
									dType_array[DATS] =
									{
										"DT_NONE",	"DT_AP",		"DT_IN",	"DT_HE",
										"DT_LASER",	"DT_PLASMA",	"DT_STUN",	"DT_MELEE",
										"DT_ACID",	"DT_SMOKE",		"DT_END"
									};

								for (
										;
										bType != BATS;
										++bType)
								{
									//Log(LOG_INFO) << ". . . . . . . . [7] iter bType";
									if (*detail == bType_array[bType])
									{
										//Log(LOG_INFO) << ". . . . . . . . . bType = " << (*detail);
										break;
									}
								}

								for (
										;
										dType != DATS;
										++dType)
								{
									//Log(LOG_INFO) << ". . . . . . . . [7] iter dType";
									if (*detail == dType_array[dType])
									{
										//Log(LOG_INFO) << ". . . . . . . . . dType = " << (*detail);
										break;
									}
								}

								if ((*kill)->_weapon == "STR_WEAPON_UNKNOWN" // if there are NO matches break and try the next Criteria.
									|| (*kill)->_weaponAmmo == "STR_WEAPON_UNKNOWN"
									|| ((*kill)->_rank != *detail
										&& (*kill)->_race != *detail
										&& (*kill)->_weapon != *detail
										&& (*kill)->_weaponAmmo != *detail
										&& (*kill)->getUnitStatusString() != *detail
										&& (*kill)->getUnitFactionString() != *detail
										&& rules->getItem((*kill)->_weapon)->getBattleType() != static_cast<BattleType>(bType)
										&& rules->getItem((*kill)->_weaponAmmo)->getDamageType() != static_cast<DamageType>(dType)))
								{
									//Log(LOG_INFO) << ". . . . . . . . no more Matching - break DETAIL";
									found = false;
									break;
								}
							}

							if (found == true)
							{
								++qty;
								//Log(LOG_INFO) << ". . . . . . . found Qty = " << qty;
								if (qty == (*andCriteria).first)
								{
									//Log(LOG_INFO) << ". . . . . . . . andCriteria qty is GOOD";
									skip = true; // criteria met so move to next mission/turn.
								}
							}
						}

						const int multiCriteria = (*andCriteria).first; // if one of the AND criteria fail stop looking.
						//Log(LOG_INFO) << ". . . . . qty = " << qty;
						//Log(LOG_INFO) << ". . . . . multiCriteria = " << multiCriteria;
						//Log(LOG_INFO) << ". . . . . \"noQual\" Levels required = " << (*j).second.at(levelReq["noQual"]);
						if (multiCriteria == 0 || qty / multiCriteria < (*j).second.at(levelReq["noQual"]))
						{
							//Log(LOG_INFO) << ". . . . . . no Award w/ career,mission,turn - BREAK andCriteria";
							grantAward = false;
							break;
						}
						else
						{
							//Log(LOG_INFO) << ". . . . . . grant Award w/ career,mission,turn";
							grantAward = true;
						}
					}

					if (grantAward == true) // stop looking because soldier is getting one regardless.
					{
						//Log(LOG_INFO) << ". . . . . grant Award w/ career,mission,turn - BREAK orCriteria";
						break;
					}
				}
			}
		}


		if (grantAward == true)
		{
			//Log(LOG_INFO) << ". do Award";
			doCeremony = true;

			if (qualifiedAwards.empty() == true)		// if there are NO qualified awards but the soldier *is*
			{											// being awarded an award its qualifier will be "noQual".
				//Log(LOG_INFO) << ". . add \"noQual\" type";
				qualifiedAwards.push_back("noQual");
			}

			for (std::vector<std::string>::const_iterator
					j = qualifiedAwards.begin();
					j != qualifiedAwards.end();
					++j)
			{
				//Log(LOG_INFO) << ". . . iter Qualifier = \"" << (*j) << "\"";
				bool firstOfType = true;
				for (std::vector<SoldierAward*>::const_iterator
						k = _solAwards.begin();
						k != _solAwards.end();
						++k)
				{
					if ((*k)->getType() == type && (*k)->getQualifier() == *j)
					{
						//Log(LOG_INFO) << ". . . . found = " << type;
						firstOfType = false;
						(*k)->addClassLevel();
						break;
					}
				}

				if (firstOfType == true)
					_solAwards.push_back(new SoldierAward(type, *j));
			}
		}
		else
		{
			//Log(LOG_INFO) << ". do NOT Award -> iterate to top";
			//Log(LOG_INFO) << "";
			++i;
		}
	}

	//Log(LOG_INFO) << "Diary: manageAwards() EXIT w/ Ceremony = " << doCeremony;
	return doCeremony;
}

/*
 * Manages modular awards. (private)
 * @param nextCommendationLevel	- refrence map<string, int>
 * @param modularCommendations	- reference map<string, int>
 * @param statTotal				- pair<string, int>
 * @param criteria				- int
 *
void SoldierDiary::manageModularCommendations(
		std::map<std::string, int>& nextCommendationLevel,
		std::map<std::string, int>& modularCommendations,
		std::pair<std::string, int> statTotal,
		int criteria)
{
	// If criteria is 0, we don't have this noun OR if we meet
	// the criteria, remember the noun for award purposes.
	if ((modularCommendations.count(statTotal.first) == 0 && statTotal.second >= criteria)
		|| (modularCommendations.count(statTotal.first) != 0 && nextCommendationLevel.at(statTotal.first) >= criteria))
	{
		modularCommendations[statTotal.first]++;
	}
} */

/*
 * Awards medals to the soldier.
 * @param type - reference the type
 * @param noun - reference the noun (default "noQual")
 *
void SoldierDiary::awardCommendation(
		const std::string& type,
		const std::string& noun)
{
	bool newAward = true;
	for (std::vector<SoldierAward*>::const_iterator i = _solAwards.begin(); i != _solAwards.end(); ++i)
	{
		if ((*i)->getType() == type && (*i)->getQualifier() == noun)
		{
			(*i)->addClassLevel();
			newAward = false;
			break;
		}
	}

	if (newAward == true)
		_solAwards.push_back(new SoldierAward(type, noun));
} */

/**
 * Gets a vector of mission ids.
 * @return, address of a vector of mission IDs
 */
std::vector<int>& SoldierDiary::getMissionIdList()
{
	return _missionIdList;
}

/**
 * Gets a vector of all kills in this SoldierDiary.
 * @return, address of a vector of pointers to BattleUnitKills
 */
std::vector<BattleUnitKill*>& SoldierDiary::getKills()
{
	return _killList;
}

/**
 * Gets list of kills by rank.
 * @return, map of alien ranks to qty killed
 */
std::map<std::string, int> SoldierDiary::getAlienRankTotal() const
{
	std::map<std::string, int> ret;

	for(std::vector<BattleUnitKill*>::const_iterator
			i = _killList.begin();
			i != _killList.end();
			++i)
	{
		++ret[(*i)->_rank];
	}

	return ret;
}

/**
 * Gets list of kills by race.
 * @return, map of alien races to qty killed
 */
std::map<std::string, int> SoldierDiary::getAlienRaceTotal() const
{
	std::map<std::string, int> ret;

	for(std::vector<BattleUnitKill*>::const_iterator
			i = _killList.begin();
			i != _killList.end();
			++i)
	{
		++ret[(*i)->_race];
	}

	return ret;
}
/**
 * Gets list of kills by weapon.
 * @return, map of weapons to qty killed with
 */
std::map<std::string, int> SoldierDiary::getWeaponTotal() const
{
	std::map<std::string, int> ret;

	for(std::vector<BattleUnitKill*>::const_iterator
			i = _killList.begin();
			i != _killList.end();
			++i)
	{
		++ret[(*i)->_weapon];
	}

	return ret;
}

/**
 * Gets list of kills by ammo.
 * @return, map of ammos to qty killed with
 */
std::map<std::string, int> SoldierDiary::getWeaponAmmoTotal() const
{
	std::map<std::string, int> ret;

	for(std::vector<BattleUnitKill*>::const_iterator
			i = _killList.begin();
			i != _killList.end();
			++i)
	{
		++ret[(*i)->_weaponAmmo];
	}

	return ret;
}

/**
 * Gets a list of quantity of missions done in a region.
 * @return, address of a map of regions to missions done there
 */
std::map<std::string, int>& SoldierDiary::getRegionTotal()
{
	return _regionTotal;
}

/**
 * Gets a list of quantity of missions done in a country.
 * @return, address of a map of countries to missions done there
 */
std::map<std::string, int>& SoldierDiary::getCountryTotal()
{
	return _countryTotal;
}

/**
 * Gets a list of quantity of missions done of a mission-type.
 * @return, address of a map of mission types to qty
 */
std::map<std::string, int>& SoldierDiary::getTypeTotal()
{
	return _typeTotal;
}

/**
 * Gets a list of quantity of missions done of a UFO-type
 * @return, address of a map of UFO types to qty
 */
std::map<std::string, int>& SoldierDiary::getUfoTotal()
{
	return _ufoTotal;
}

/**
 * Gets a total score for all missions.
 * @return, sum score of all missions engaged
 */
int SoldierDiary::getScoreTotal() const
{
	return _scoreTotal;
}

/**
 * Gets the total point-value of aLiens killed or stunned.
 * @return, sum points for all aliens killed or stunned
 */
int SoldierDiary::getScorePoints() const
{
	return _pointTotal;
}

/**
 * Gets the total quantity of kills.
 * @return, qty of kills
 */
int SoldierDiary::getKillTotal() const
{
	return _killTotal;
}

/**
 * Gets the total quantity of stuns.
 * @return, qty of stuns
 */
int SoldierDiary::getStunTotal() const
{
	return _stunTotal;
}

/**
 * Gets the total quantity of missions.
 * @return, qty of missions
 */
int SoldierDiary::getMissionTotal() const
{
	return static_cast<int>(_missionIdList.size());
}

/**
 * Gets the quantity of successful missions.
 * @return, qty of successful missions
 */
int SoldierDiary::getWinTotal() const
{
	return _winTotal;
}

/**
 * Gets the total quantity of days wounded.
 * @return, qty of days in sickbay
 */
int SoldierDiary::getDaysWoundedTotal() const
{
	return _daysWoundedTotal;
}

/**
 * Gets whether soldier died or went missing.
 * @return, kia or mia - or an empty string if neither
 */
std::string SoldierDiary::getKiaOrMia() const
{
	if (_KIA != 0)
		return "STR_KIA";

	if (_MIA != 0)
		return "STR_MIA";

	return "";
}

/**
 * Increments soldier's service time by one month.
 */
void SoldierDiary::addMonthlyService()
{
	++_monthsService;
}

/**
 * Awards special medal to each of the original 8 soldiers.
 */
void SoldierDiary::awardOriginalEight()
{
	_solAwards.push_back(new SoldierAward("STR_MEDAL_ORIGINAL8_NAME"));
}



//____________________________________//
/*___________________________________/
/
/         SOLDIER AWARD class
/ ___________________________________*/
/**
 * Initializes a SoldierAward.
 * @param type		- reference the type
 * @param qualifier	- reference the noun (default "noQual")
 */
SoldierAward::SoldierAward(
		const std::string& type,
		const std::string& qualifier)
	:
		_type(type),
		_qual(qualifier),
		_level(0),
		_new(true)
{}

/**
 * Initializes a new SoldierAward entry from YAML.
 * @param node - YAML node
 */
SoldierAward::SoldierAward(const YAML::Node& node)
{
	load(node);
}

/**
 * dTor.
 */
SoldierAward::~SoldierAward()
{}

/**
 * Loads the SoldierAward from a YAML file.
 * @param node - reference a YAML node
 */
void SoldierAward::load(const YAML::Node& node)
{
	_type	= node["type"]		.as<std::string>(_type);
	_qual	= node["qualifier"]	.as<std::string>("noQual");
	_level	= node["level"]		.as<size_t>(_level);

	_new = false;
}

/**
 * Saves this SoldierAward to a YAML file.
 * @return, YAML node
 */
YAML::Node SoldierAward::save() const
{
	YAML::Node node;

	node["type"] = _type;
	node["level"] = static_cast<int>(_level); // warning: Save this even if "0".

	if (_qual != "noQual")
		node["qualifier"] = _qual;

	return node;
}

/**
 * Gets this SoldierAward's type.
 * @return, award name
 */
const std::string SoldierAward::getType() const
{
	return _type;
}

/**
 * Get this SoldierAward's noun.
 * @return, award noun
 */
const std::string SoldierAward::getQualifier() const
{
	return _qual;
}

/**
 * Gets this SoldierAward's level's type.
 * @param skip -
 * @return, award level
 */
const std::string SoldierAward::getClassType(int skip) const
{
	std::ostringstream oststr;
	oststr << "STR_AWARD_" << _level - skip;
	return oststr.str();
}

/**
 * Gets this SoldierAward's level as an integer.
 * @return, award level int
 */
size_t SoldierAward::getClassLevel() const
{
	return _level;
}

/**
 * Gets this SoldierAward's level description.
 * @return, award level description
 */
const std::string SoldierAward::getClassDescription() const
{
	std::ostringstream oststr;
	oststr << "STR_AWARD_DECOR_" << _level;
	return oststr.str();
}

/**
 * Gets this SoldierAward's level class - qty stars.
 * @return, award level class
 */
const std::string SoldierAward::getClassDegree() const
{
	std::ostringstream oststr;
	oststr << "STR_AWARD_CLASS_" << _level;
	return oststr.str();
}

/**
 * Gets newness of this SoldierAward.
 * @return, true if the award is new
 */
bool SoldierAward::isNew() const
{
	return _new;
}

/**
 * Sets the newness of this SoldierAward to old.
 */
void SoldierAward::setOld()
{
	_new = false;
}

/**
 * Adds a level of decoration to this SoldierAward.
 */
void SoldierAward::addClassLevel()
{
	++_level;
	_new = true;
}

}
