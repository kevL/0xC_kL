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

#include "SoldierDiary.h"

//#include <sstream> // std::ostringstream

#include "BattleUnitStatistics.h"
#include "MissionStatistics.h"

#include "../Ruleset/RuleAward.h"
#include "../Ruleset/Ruleset.h"


namespace OpenXcom
{

/**
 * Creates the SoldierDiary.
 */
SoldierDiary::SoldierDiary()
	:
		_daysWoundedTotal(0),
		_monthsService(0),
		_unconsciousTotal(0),
		_shotAtCounterTotal(0),
		_hitCounterTotal(0),
		_loneSurvivorTotal(0),
		_totalShotByFriendlyCounter(0),
		_totalShotFriendlyCounter(0),
		_ironManTotal(0),
		_longDistanceHitCounterTotal(0),
		_lowAccuracyHitCounterTotal(0),
		_shotsFiredCounterTotal(0),
		_shotsLandedCounterTotal(0),
		_shotAtCounter10in1Mission(0),
		_hitCounter5in1Mission(0),
		_timesWoundedTotal(0),
		_allAliensKilledTotal(0),
		_mediApplicationsTotal(0),
		_revivedUnitTotal(0),
		_KIA(0),
		_MIA(0)
{}

/**
 * Creates the SoldierDiary and fills it w/ YAML data.
 * @param node - reference a YAML node
 *
SoldierDiary::SoldierDiary(const YAML::Node& node)
{
	load(node);
} */

/**
 * Constructs the SoldierDiary from a copy of another diary.
 * @note I think there's a C++11 operator that does this *way* easier: "&&".
 * @param copyThis - reference to a SoldierDiary to copy into this diary
 */
SoldierDiary::SoldierDiary(const SoldierDiary& copyThat)
	:
		_daysWoundedTotal(copyThat._daysWoundedTotal),
		_totalShotByFriendlyCounter(copyThat._totalShotByFriendlyCounter),
		_totalShotFriendlyCounter(copyThat._totalShotFriendlyCounter),
		_loneSurvivorTotal(copyThat._loneSurvivorTotal),
		_monthsService(copyThat._monthsService),
		_unconsciousTotal(copyThat._unconsciousTotal),
		_shotAtCounterTotal(copyThat._shotAtCounterTotal),
		_hitCounterTotal(copyThat._hitCounterTotal),
		_ironManTotal(copyThat._ironManTotal),
		_longDistanceHitCounterTotal(copyThat._longDistanceHitCounterTotal),
		_lowAccuracyHitCounterTotal(copyThat._lowAccuracyHitCounterTotal),
		_shotsFiredCounterTotal(copyThat._shotsFiredCounterTotal),
		_shotsLandedCounterTotal(copyThat._shotsLandedCounterTotal),
		_shotAtCounter10in1Mission(copyThat._shotAtCounter10in1Mission),
		_hitCounter5in1Mission(copyThat._hitCounter5in1Mission),
		_timesWoundedTotal(copyThat._timesWoundedTotal),
		_allAliensKilledTotal(copyThat._allAliensKilledTotal),
		_mediApplicationsTotal(copyThat._mediApplicationsTotal),
		_revivedUnitTotal(copyThat._revivedUnitTotal),
		_KIA(copyThat._KIA),
		_MIA(copyThat._MIA)
{
	for (size_t
			i = 0u;
			i != copyThat._missionIdList.size();
			++i)
	{
//		if (copyThat._missionIdList.at(i) != nullptr) // Bzzzt.
		_missionIdList.push_back(copyThat._missionIdList.at(i));
	}

	for (size_t
			i = 0u;
			i != copyThat._solAwards.size();
			++i)
	{
		if (copyThat._solAwards.at(i) != nullptr)
		{
			std::string
				type (copyThat._solAwards.at(i)->getType()),
				qual (copyThat._solAwards.at(i)->getQualifier());

			_solAwards.push_back(new SoldierAward(type, qual));
		}
	}

	for (size_t
			i = 0u;
			i != copyThat._killList.size();
			++i)
	{
		if (copyThat._killList.at(i) != nullptr)
		{
			std::string
				unitRank (copyThat._killList.at(i)->_rank),
				race (copyThat._killList.at(i)->_race),
				weapon (copyThat._killList.at(i)->_weapon),
				weaponAmmo (copyThat._killList.at(i)->_weaponAmmo);
			int
				mission (copyThat._killList.at(i)->_mission),
				turn (copyThat._killList.at(i)->_turn),
				points (copyThat._killList.at(i)->_points);

			UnitFaction faction (copyThat._killList.at(i)->_faction);
			UnitStatus status (copyThat._killList.at(i)->_status);

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
		delete *i;

	for (std::vector<BattleUnitKill*>::const_iterator
			i = _killList.begin();
			i != _killList.end();
			++i)
		delete *i;
}

/**
 * Overloads the assignment operator.
 * @param assignThat - reference to a SoldierDiary to assign to this diary
 * @return, reference to the fresh diary
 */
SoldierDiary& SoldierDiary::operator= (const SoldierDiary& assignThat)
{
	if (this != &assignThat)
	{
		_daysWoundedTotal = assignThat._daysWoundedTotal;
		_totalShotByFriendlyCounter = assignThat._totalShotByFriendlyCounter;
		_totalShotFriendlyCounter = assignThat._totalShotFriendlyCounter;
		_loneSurvivorTotal = assignThat._loneSurvivorTotal;
		_monthsService = assignThat._monthsService;
		_unconsciousTotal = assignThat._unconsciousTotal;
		_shotAtCounterTotal = assignThat._shotAtCounterTotal;
		_hitCounterTotal = assignThat._hitCounterTotal;
		_ironManTotal = assignThat._ironManTotal;
		_longDistanceHitCounterTotal = assignThat._longDistanceHitCounterTotal;
		_lowAccuracyHitCounterTotal = assignThat._lowAccuracyHitCounterTotal;
		_shotsFiredCounterTotal = assignThat._shotsFiredCounterTotal;
		_shotsLandedCounterTotal = assignThat._shotsLandedCounterTotal;
		_shotAtCounter10in1Mission = assignThat._shotAtCounter10in1Mission;
		_hitCounter5in1Mission = assignThat._hitCounter5in1Mission;
		_timesWoundedTotal = assignThat._timesWoundedTotal;
		_allAliensKilledTotal = assignThat._allAliensKilledTotal;
		_mediApplicationsTotal = assignThat._mediApplicationsTotal;
		_revivedUnitTotal = assignThat._revivedUnitTotal;
		_KIA = assignThat._KIA;
		_MIA = assignThat._MIA;

		_missionIdList.clear();
		for (std::vector<int>::const_iterator
				i = assignThat._missionIdList.begin();
				i != assignThat._missionIdList.end();
				++i)
		{
			_missionIdList.push_back(*i);
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
				i = 0u;
				i != assignThat._solAwards.size();
				++i)
		{
			if (assignThat._solAwards.at(i) != nullptr)
			{
				std::string
					type (assignThat._solAwards.at(i)->getType()),
					qual (assignThat._solAwards.at(i)->getQualifier());

				_solAwards.push_back(new SoldierAward(type, qual));
			}
		}

		for (size_t
				i = 0u;
				i != assignThat._killList.size();
				++i)
		{
			if (assignThat._killList.at(i) != nullptr)
			{
				std::string
					unitRank (assignThat._killList.at(i)->_rank),
					race (assignThat._killList.at(i)->_race),
					weapon (assignThat._killList.at(i)->_weapon),
					weaponAmmo (assignThat._killList.at(i)->_weaponAmmo);
				int
					mission (assignThat._killList.at(i)->_mission),
					turn (assignThat._killList.at(i)->_turn),
					points (assignThat._killList.at(i)->_points);

				UnitFaction faction (assignThat._killList.at(i)->_faction);
				UnitStatus status (assignThat._killList.at(i)->_status);

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
	_daysWoundedTotal				= node["daysWoundedTotal"]				.as<int>(_daysWoundedTotal);
	_totalShotByFriendlyCounter		= node["totalShotByFriendlyCounter"]	.as<int>(_totalShotByFriendlyCounter);
	_totalShotFriendlyCounter		= node["totalShotFriendlyCounter"]		.as<int>(_totalShotFriendlyCounter);
	_loneSurvivorTotal				= node["loneSurvivorTotal"]				.as<int>(_loneSurvivorTotal);
	_monthsService					= node["monthsService"]					.as<int>(_monthsService);
	_unconsciousTotal				= node["unconsciousTotal"]				.as<int>(_unconsciousTotal);
	_shotAtCounterTotal				= node["shotAtCounterTotal"]			.as<int>(_shotAtCounterTotal);
	_hitCounterTotal				= node["hitCounterTotal"]				.as<int>(_hitCounterTotal);
	_ironManTotal					= node["ironManTotal"]					.as<int>(_ironManTotal);
	_longDistanceHitCounterTotal	= node["longDistanceHitCounterTotal"]	.as<int>(_longDistanceHitCounterTotal);
	_lowAccuracyHitCounterTotal		= node["lowAccuracyHitCounterTotal"]	.as<int>(_lowAccuracyHitCounterTotal);
	_shotsFiredCounterTotal			= node["shotsFiredCounterTotal"]		.as<int>(_shotsFiredCounterTotal);
	_shotsLandedCounterTotal		= node["shotsLandedCounterTotal"]		.as<int>(_shotsLandedCounterTotal);
	_shotAtCounter10in1Mission		= node["shotAtCounter10in1Mission"]		.as<int>(_shotAtCounter10in1Mission);
	_hitCounter5in1Mission			= node["hitCounter5in1Mission"]			.as<int>(_hitCounter5in1Mission);
	_timesWoundedTotal				= node["timesWoundedTotal"]				.as<int>(_timesWoundedTotal);
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
	if (_daysWoundedTotal)					node["daysWoundedTotal"]			= _daysWoundedTotal;
	if (_totalShotByFriendlyCounter)		node["totalShotByFriendlyCounter"]	= _totalShotByFriendlyCounter;
	if (_totalShotFriendlyCounter)			node["totalShotFriendlyCounter"]	= _totalShotFriendlyCounter;
	if (_loneSurvivorTotal)					node["loneSurvivorTotal"]			= _loneSurvivorTotal;
	if (_monthsService)						node["monthsService"]				= _monthsService;
	if (_unconsciousTotal)					node["unconsciousTotal"]			= _unconsciousTotal;
	if (_shotAtCounterTotal)				node["shotAtCounterTotal"]			= _shotAtCounterTotal;
	if (_hitCounterTotal)					node["hitCounterTotal"]				= _hitCounterTotal;
	if (_ironManTotal)						node["ironManTotal"]				= _ironManTotal;
	if (_longDistanceHitCounterTotal)		node["longDistanceHitCounterTotal"]	= _longDistanceHitCounterTotal;
	if (_lowAccuracyHitCounterTotal)		node["lowAccuracyHitCounterTotal"]	= _lowAccuracyHitCounterTotal;
	if (_shotsFiredCounterTotal)			node["shotsFiredCounterTotal"]		= _shotsFiredCounterTotal;
	if (_shotsLandedCounterTotal)			node["shotsLandedCounterTotal"]		= _shotsLandedCounterTotal;
	if (_shotAtCounter10in1Mission)			node["shotAtCounter10in1Mission"]	= _shotAtCounter10in1Mission;
	if (_hitCounter5in1Mission)				node["hitCounter5in1Mission"]		= _hitCounter5in1Mission;
	if (_timesWoundedTotal)					node["timesWoundedTotal"]			= _timesWoundedTotal;
	if (_allAliensKilledTotal)				node["allAliensKilledTotal"]		= _allAliensKilledTotal;
	if (_mediApplicationsTotal)				node["mediApplicationsTotal"]		= _mediApplicationsTotal;
	if (_revivedUnitTotal)					node["revivedUnitTotal"]			= _revivedUnitTotal;

	return node;
}

/**
 * Updates this SoldierDiary's statistics.
 * @note BattleUnitKill is a substruct of BattleUnitStatistics.
 * @param diaryStats	- pointer to BattleUnitStatistics for info on a Soldier's
 *						  current tactical performance ala DebriefingState.
 * @param tacticals		- pointer to latest MissionStatistics
 * @param rules			- pointer to the Ruleset
 */
void SoldierDiary::updateDiary(
		const BattleUnitStatistics* const diaryStats,
		const MissionStatistics* const tactical,
		const Ruleset* const rules)
{
	//Log(LOG_INFO) << "SoldierDiary::updateDiary()";
	const std::vector<BattleUnitKill*> unitKills (diaryStats->kills);
	for (std::vector<BattleUnitKill*>::const_iterator
			i = unitKills.begin();
			i != unitKills.end();
			++i)
	{
		(*i)->makeTurnUnique();
		_killList.push_back(*i);
	}

	if (tactical->success == true)
	{
		if (diaryStats->loneSurvivor == true)
			++_loneSurvivorTotal;

		if (diaryStats->ironMan == true)
			++_ironManTotal;
	}

	if (diaryStats->daysWounded != 0)
	{
		_daysWoundedTotal += diaryStats->daysWounded;
		++_timesWoundedTotal;
	}

	if (diaryStats->wasUnconscious == true)
		++_unconsciousTotal;

	_shotAtCounterTotal				+= diaryStats->shotAtCounter;
	_shotAtCounter10in1Mission		+= diaryStats->shotAtCounter / 10;
	_hitCounterTotal				+= diaryStats->hitCounter;
	_hitCounter5in1Mission			+= diaryStats->hitCounter / 5;
	_totalShotByFriendlyCounter		+= diaryStats->shotByFriendlyCounter;
	_totalShotFriendlyCounter		+= diaryStats->shotFriendlyCounter;
	_longDistanceHitCounterTotal	+= diaryStats->longDistanceHitCounter;
	_lowAccuracyHitCounterTotal		+= diaryStats->lowAccuracyHitCounter;
	_shotsFiredCounterTotal			+= diaryStats->shotsFiredCounter;
	_shotsLandedCounterTotal		+= diaryStats->shotsLandedCounter;
	_mediApplicationsTotal			+= diaryStats->medikitApplications;
	_revivedUnitTotal				+= diaryStats->revivedSoldier;

	if (diaryStats->nikeCross == true)
		++_allAliensKilledTotal;

	if (diaryStats->KIA == true)
		_KIA = 1;
	else if (diaryStats->MIA == true)
		_MIA = 1;

	_missionIdList.push_back(tactical->id);
	//Log(LOG_INFO) << "SoldierDiary::updateDiary() EXIT";
}

/**
 * Gets the SoldierAwards in this SoldierDiary.
 * @return, pointer to a vector of pointers to SoldierAward - a list of awards
 */
std::vector<SoldierAward*>* SoldierDiary::getSoldierAwards()
{
	return &_solAwards;
}

/**
 * Manages the SoldierAwards - award new ones if earned.
 * @param rules		- pointer to the Ruleset
 * @param tacticals	- pointer to a vector of pointers to MissionStatistics
 * @return, true if an award is awarded
 */
bool SoldierDiary::manageAwards(
		const Ruleset* const rules,
		const std::vector<MissionStatistics*>* const tacticals)
{
	//Log(LOG_INFO) << "";
	//Log(LOG_INFO) << "Diary: manageAwards()";
	bool
		doCeremony (false),	// this value is returned TRUE if at least one award is given.
		grantAward;			// this value determines if an award will be given.

	std::vector<std::string> qualifiedAwards;	// <types>
	std::map<std::string, size_t> levelReq;		// <noun, qtyLevels required>

	const std::map<std::string, RuleAward*> allAwards (rules->getAwardsList()); // loop over all possible RuleAwards.
	for (std::map<std::string, RuleAward*>::const_iterator
			i = allAwards.begin();
			i != allAwards.end();
			)
	{
		//Log(LOG_INFO) << ". [1] iter awardList - " << (*i).first;
		qualifiedAwards.clear();
		levelReq.clear();
		levelReq["noQual"] = 0;

		// loop over all of soldier's current Awards; map the award's qualifier w/ next-required level.
		for (std::vector<SoldierAward*>::const_iterator
				j = _solAwards.begin();
				j != _solAwards.end();
				++j)
		{
			if ((*j)->getType() == i->first)
			{
				//Log(LOG_INFO) << ". . set Level[" << ((*j)->getClassLevel() + 1)  << "] req'd for Qualifier \"" << (*j)->getQualifier() << "\"";
				levelReq[(*j)->getQualifier()] = (*j)->getClassLevel() + 1;
			}
		}

		// go through each possible criteria. Assume the award is awarded, set to FALSE if not;
		// ie, as soon as an award criteria that *fails to be achieved* is found, then no award.
		grantAward = true;

		const std::map<std::string, std::vector<int>>* allCriteria (i->second->getCriteria());
		for (std::map<std::string, std::vector<int>>::const_iterator
				j = allCriteria->begin();
				j != allCriteria->end();
				++j)
		{
			//Log(LOG_INFO) << ". . [2] iter Criteria " << (*j).first;
			// skip a "noQual" award if its max award level has been reached
			// or if it has a qualifier skip it if it has 0 total levels (which ain't gonna happen);
			// you see, Rules can't be positively examined for nouns - only awards already given to soldiers can.
			if (j->second.size() <= levelReq["noQual"])
			{
				//Log(LOG_INFO) << ". . . max \"noQual\" Level reached (or, Criteria has no vector)";
				grantAward = false;
				break;
			}

			const std::string criteriaType (j->first);			// vector of (ints) mapped to a (string). Eg, "totalByNoun" incl. "noQual".
			const int val (j->second.at(levelReq["noQual"]));	// these criteria have no nouns, so only the levelReq["noQual"] will ever be compared

			if ( //levelReq.count("noQual") == 1 && // <- this is relevant only if entry "noQual" were removed from the map in the sections following this one.
				(       criteriaType == "totalKills"				&& getKillTotal() < val)
					|| (criteriaType == "totalMissions"				&& static_cast<int>(_missionIdList.size()) < val)
					|| (criteriaType == "totalWins"					&& getWinTotal(tacticals) < val)
					|| (criteriaType == "totalScore"				&& getScoreTotal(tacticals) < val)
					|| (criteriaType == "totalPoints"				&& getPointsTotal() < val)
					|| (criteriaType == "totalStuns"				&& getStunTotal() < val)
					|| (criteriaType == "totalBaseDefenseMissions"	&& getBaseDefenseMissionTotal(tacticals) < val)
					|| (criteriaType == "totalTerrorMissions"		&& getTerrorMissionTotal(tacticals) < val)
					|| (criteriaType == "totalNightMissions"		&& getNightMissionTotal(tacticals) < val)
					|| (criteriaType == "totalNightTerrorMissions"	&& getNightTerrorMissionTotal(tacticals) < val)
					|| (criteriaType == "totalMonthlyService"		&& _monthsService < val)
					|| (criteriaType == "totalFellUnconscious"		&& _unconsciousTotal < val)
					|| (criteriaType == "totalShotAt10Times"		&& _shotAtCounter10in1Mission < val)
					|| (criteriaType == "totalHit5Times"			&& _hitCounter5in1Mission < val)
					|| (criteriaType == "totalFriendlyFired"		&& (_totalShotByFriendlyCounter < val || _KIA == 1 || _MIA == 1)) // didn't survive ......
					|| (criteriaType == "totalLoneSurvivor"			&& _loneSurvivorTotal < val)
					|| (criteriaType == "totalIronMan"				&& _ironManTotal < val)
					|| (criteriaType == "totalImportantMissions"	&& getImportantMissionTotal(tacticals) < val)
					|| (criteriaType == "totalLongDistanceHits"		&& _longDistanceHitCounterTotal < val)
					|| (criteriaType == "totalLowAccuracyHits"		&& _lowAccuracyHitCounterTotal < val)
					|| (criteriaType == "totalReactionFire"			&& getReactionFireKillTotal(rules) < val)
					|| (criteriaType == "totalTimesWounded"			&& _timesWoundedTotal < val)
					|| (criteriaType == "totalDaysWounded"			&& _daysWoundedTotal < val)
					|| (criteriaType == "totalValientCrux"			&& getValiantCruxTotal(tacticals) < val)
					|| (criteriaType == "isDead"					&& _KIA < val)
					|| (criteriaType == "totalTrapKills"			&& getTrapKillTotal(rules) < val)
					|| (criteriaType == "totalAlienBaseAssaults"	&& getAlienBaseAssaultTotal(tacticals) < val)
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
				  || criteriaType == "totalMissionsInARegion"	// and they loop over a map<> (this allows for super-good-plus modability).
				  || criteriaType == "totalKillsByRace"
				  || criteriaType == "totalKillsByRank")
			{
				//Log(LOG_INFO) << ". . . try Award w/ weapon,region,race,rank";
				std::map<std::string, int> total;
				if		(criteriaType == "totalKillsWithAWeapon")	total = getWeaponTotal();
				else if	(criteriaType == "totalMissionsInARegion")	total = getRegionTotal(tacticals);
				else if	(criteriaType == "totalKillsByRace")		total = getAlienRaceTotal();
				else if	(criteriaType == "totalKillsByRank")		total = getAlienRankTotal();

				for (std::map<std::string, int>::const_iterator // loop over the 'total' map and match Qualifiers with Levels.
						k = total.begin();
						k != total.end();
						++k)
				{
					//Log(LOG_INFO) << ". . . . [3] " << (*k).first << " - " << (*k).second;
					int threshold (-1);
					if (levelReq.count(k->first) == 0)					// if there is no matching Qualifier get the first criteria
					{
						//Log(LOG_INFO) << ". . . . . no relevant qualifier yet, threshold = " << (*j).second.front();
						threshold = j->second.front();
					}
					else if (levelReq[k->first] != j->second.size())	// otherwise get the criteria per the soldier's award Level.
					{
						//Log(LOG_INFO) << ". . . . . qualifier found, next level available, threshold = " << j->second.at(levelReq[k->first]);
						threshold = j->second.at(levelReq[k->first]);
					}

					if (threshold != -1 && threshold <= k->second)		// if a criteria was set AND the stat's count exceeds that criteria ...
					{
						//Log(LOG_INFO) << ". . . . . threshold good, add to qualifiedAwards vector";
						qualifiedAwards.push_back(k->first);
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
				const std::vector<std::map<int, std::vector<std::string>>>* allKillCriteria (i->second->getKillCriteria()); // fetch the killCriteria list.
				for (std::vector<std::map<int, std::vector<std::string>>>::const_iterator // loop over the OR vectors.
						exclusiveCriteria = allKillCriteria->begin();
						exclusiveCriteria != allKillCriteria->end();
						++exclusiveCriteria)
				{
					//Log(LOG_INFO) << ". . . . [3] iter killCriteria OR list";// << (*exclusiveCriteria)->;
					for (std::map<int, std::vector<std::string>>::const_iterator // loop over the AND vectors.
							additiveCriteria = exclusiveCriteria->begin();
							additiveCriteria != exclusiveCriteria->end();
							++additiveCriteria)
					{
						//Log(LOG_INFO) << ". . . . . [4] iter killCriteria AND list";// << *additiveCriteria->second.begin();
						int qty (0); // how many AND vectors (list of DETAILs) have been successful.
						if (criteriaType != "killsWithCriteriaCareer")
							++qty; // "killsWith..." Turns or Missions start at 1 because of how thisIter and lastIter work.
						//Log(LOG_INFO) << ". . . . . start Qty = " << qty;

						bool skip (false);
						int
							thisIter (-1), // being a turn or a mission
							lastIter (-1);
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
									lastIter = (*(kill - 1))->_mission;
							}
							else if (criteriaType == "killsWithCriteriaTurn")
							{
								thisIter = (*kill)->_turn;
								if (kill != _killList.begin())
									lastIter = (*(kill - 1))->_turn;
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

							bool found (true);

							for (std::vector<std::string>::const_iterator // loop over the DETAILs of the AND vector.
									detail = additiveCriteria->second.begin();
									detail != additiveCriteria->second.end();
									++detail)
							{
								//Log(LOG_INFO) << ". . . . . . . [6] iter DETAIL = " << (*detail);
								size_t
									bType (0u),
									dType (0u);

								static const std::string
									bType_array[BATS]
									{
										"BT_NONE",		"BT_FIREARM",		"BT_AMMO",		"BT_MELEE",
										"BT_GRENADE",	"BT_PROXYGRENADE",	"BT_MEDIKIT",	"BT_SCANNER",
										"BT_MINDPROBE",	"BT_PSIAMP",		"BT_FLARE",		"BT_CORPSE",
										"BT_END"
									},
									dType_array[DATS]
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

								const RuleItem // if there are NO matches break and try the next Criteria.
									* const weapon (rules->getItemRule((*kill)->_weapon)),
									* const weaponAmmo (rules->getItemRule((*kill)->_weaponAmmo));

								if (   weapon == nullptr		//(*kill)->_weapon == "STR_WEAPON_UNKNOWN"
									|| weaponAmmo == nullptr	//(*kill)->_weaponAmmo == "STR_WEAPON_UNKNOWN"
									|| (   (*kill)->_rank != *detail
										&& (*kill)->_race != *detail
										&& (*kill)->_weapon != *detail
										&& (*kill)->_weaponAmmo != *detail
										&& (*kill)->getUnitStatusString() != *detail
										&& (*kill)->getUnitFactionString() != *detail
										&& weapon->getBattleType() != static_cast<BattleType>(bType)
										&& weaponAmmo->getDamageType() != static_cast<DamageType>(dType)))
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
								if (qty == additiveCriteria->first)
								{
									//Log(LOG_INFO) << ". . . . . . . . additiveCriteria qty is GOOD";
									skip = true; // criteria met so move to next mission/turn.
								}
							}
						}

						// if one of the AND criteria fail stop looking.
						//Log(LOG_INFO) << ". . . . . qty = " << qty;
						//Log(LOG_INFO) << ". . . . . multiCriteria = " << additiveCriteria->first;
						//Log(LOG_INFO) << ". . . . . \"noQual\" Levels required = " << j->second.at(levelReq["noQual"]);
						if (additiveCriteria->first == 0
							|| qty / additiveCriteria->first < j->second.at(levelReq["noQual"]))
						{
							//Log(LOG_INFO) << ". . . . . . no Award w/ career,mission,turn - BREAK additiveCriteria";
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

			if (qualifiedAwards.empty() == true)	// if there are NO qualified awards but the soldier *is*
			{										// being awarded an award its qualifier will be "noQual".
				//Log(LOG_INFO) << ". . add \"noQual\" type";
				qualifiedAwards.push_back("noQual");
			}

			for (std::vector<std::string>::const_iterator
					j = qualifiedAwards.begin();
					j != qualifiedAwards.end();
					++j)
			{
				//Log(LOG_INFO) << ". . . iter Qualifier = \"" << (*j) << "\"";
				bool firstOfType (true);
				for (std::vector<SoldierAward*>::const_iterator
						k = _solAwards.begin();
						k != _solAwards.end();
						++k)
				{
					if ((*k)->getType() == i->first && (*k)->getQualifier() == *j)
					{
						//Log(LOG_INFO) << ". . . . found = " << i->first;
						firstOfType = false;
						(*k)->addClassLevel();
						break;
					}
				}

				if (firstOfType == true)
					_solAwards.push_back(new SoldierAward(i->first, *j));
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

/**
 * Gets a vector of mission-IDs.
 * @return, reference to a vector of mission-IDs
 */
std::vector<int>& SoldierDiary::getMissionIdList()
{
	return _missionIdList;
}

/**
 * Gets a vector of all kills in this SoldierDiary.
 * @return, reference to a vector of pointers to BattleUnitKill
 */
std::vector<BattleUnitKill*>& SoldierDiary::getKills()
{
	return _killList;
}

/**
 * Gets list of kills by rank.
 * @return, map of alien-ranks to qty killed
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
 * @return, map of alien-races to qty killed
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
 * Gets a list of quantities of tacticals done by Region-type.
 * @param tacticals - pointer to a vector of pointers to MissionStatistics
 * @return, map of regions to tacticals done there
 */
std::map<std::string, int> SoldierDiary::getRegionTotal(const std::vector<MissionStatistics*>* const tacticals) const
{
	std::map<std::string, int> ret;
	for (std::vector<MissionStatistics*>::const_iterator
			i = tacticals->begin();
			i != tacticals->end();
			++i)
	{
		for (std::vector<int>::const_iterator
				j = _missionIdList.begin();
				j != _missionIdList.end();
				++j)
		{
			if (*j == (*i)->id)
				++ret[(*i)->region];
		}
	}
	return ret;
}

/**
 * Gets a list of quantities of tacticals done by Country-type.
 * @param tacticals - pointer to a vector of pointers to MissionStatistics
 * @return, map of countries to tacticals done there
 */
std::map<std::string, int> SoldierDiary::getCountryTotal(const std::vector<MissionStatistics*>* const tacticals) const
{
	std::map<std::string, int> ret;
	for (std::vector<MissionStatistics*>::const_iterator
			i = tacticals->begin();
			i != tacticals->end();
			++i)
	{
		for (std::vector<int>::const_iterator
				j = _missionIdList.begin();
				j != _missionIdList.end();
				++j)
		{
			if (*j == (*i)->id)
				++ret[(*i)->country];
		}
	}
	return ret;
}

/**
 * Gets a list of quantities of tacticals done by Tactical-type.
 * @param tacticals - pointer to a vector of pointers to MissionStatistics
 * @return, map of mission-types to qty of
 */
std::map<std::string, int> SoldierDiary::getTypeTotal(const std::vector<MissionStatistics*>* const tacticals) const
{
	std::map<std::string, int> ret;
	for (std::vector<MissionStatistics*>::const_iterator
			i = tacticals->begin();
			i != tacticals->end();
			++i)
	{
		for (std::vector<int>::const_iterator
				j = _missionIdList.begin();
				j != _missionIdList.end();
				++j)
		{
			if (*j == (*i)->id)
				++ret[(*i)->type];
		}
	}
	return ret;
}

/**
 * Gets a list of quantities of tacticals done by UFO-type.
 * @param tacticals - pointer to a vector of pointers to MissionStatistics
 * @return, map of UFO-types to qty of
 */
std::map<std::string, int> SoldierDiary::getUfoTotal(const std::vector<MissionStatistics*>* const tacticals) const
{
	std::map<std::string, int> ret;
	for (std::vector<MissionStatistics*>::const_iterator
			i = tacticals->begin();
			i != tacticals->end();
			++i)
	{
		for (std::vector<int>::const_iterator
				j = _missionIdList.begin();
				j != _missionIdList.end();
				++j)
		{
			if (*j == (*i)->id)
				++ret[(*i)->ufo];
		}
	}
	return ret;
}

/**
 * Gets the current total-score.
 * @param tacticals - pointer to a vector of pointers to MissionStatistics
 * @return, score of all tacticals engaged
 */
int SoldierDiary::getScoreTotal(const std::vector<MissionStatistics*>* const tacticals) const
{
	int ret (0);
	for (std::vector<MissionStatistics*>::const_iterator
			i = tacticals->begin();
			i != tacticals->end();
			++i)
	{
		for (std::vector<int>::const_iterator
				j = _missionIdList.begin();
				j != _missionIdList.end();
				++j)
		{
			if (*j == (*i)->id)
				ret += (*i)->score;
		}
	}
	return ret;
}

/**
 * Gets the current total points-value of units killed or stunned.
 * @return, points for all aliens killed or stunned
 */
int SoldierDiary::getPointsTotal() const
{
	int ret (0);
	for (std::vector<BattleUnitKill*>::const_iterator
			i = _killList.begin();
			i != _killList.end();
			++i)
	{
		ret += (*i)->_points;
	}
	return ret;
}

/**
 * Gets the current total quantity of kills.
 * @return, qty of kills
 */
int SoldierDiary::getKillTotal() const
{
	int ret (0);
	for (std::vector<BattleUnitKill*>::const_iterator
			i = _killList.begin();
			i != _killList.end();
			++i)
	{
		if ((*i)->_status == STATUS_DEAD && (*i)->_faction == FACTION_HOSTILE)
			++ret;
	}
	return ret;
}

/**
 * Gets the current total quantity of stuns.
 * @return, qty of stuns
 */
int SoldierDiary::getStunTotal() const
{
	int ret (0);
	for (std::vector<BattleUnitKill*>::const_iterator
			i = _killList.begin();
			i != _killList.end();
			++i)
	{
		if ((*i)->_status == STATUS_UNCONSCIOUS && (*i)->_faction == FACTION_HOSTILE)
			++ret;
	}
	return ret;
}

/**
 * Gets the current total quantity of tacticals.
 * @return, qty of tacticals
 */
size_t SoldierDiary::getMissionTotal() const
{
	return _missionIdList.size();
}

/**
 * Gets the current total quantity of successful tacticals.
 * @param tacticals - pointer to a vector of pointers to MissionStatistics
 * @return, qty of successful tacticals
 */
int SoldierDiary::getWinTotal(const std::vector<MissionStatistics*>* const tacticals) const
{
	int ret (0);
	for (std::vector<MissionStatistics*>::const_iterator
			i = tacticals->begin();
			i != tacticals->end();
			++i)
	{
		for (std::vector<int>::const_iterator
				j = _missionIdList.begin();
				j != _missionIdList.end();
				++j)
		{
			if (*j == (*i)->id && (*i)->success)
				++ret;
		}
	}
	return ret;
}

/**
 * Gets the current total quantity of days wounded.
 * @return, qty of days in sickbay
 */
int SoldierDiary::getDaysWoundedTotal() const
{
	return _daysWoundedTotal;
}

/**
 * Gets if the Soldier died or went missing.
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
 * Gets the total quantity of shots fired.
 * @return, shots fired
 */
int SoldierDiary::getShotsFiredTotal() const
{
	return _shotsFiredCounterTotal;
}

/**
 * Gets the total quantity of shots landed on target.
 * @return, shots landed
 */
int SoldierDiary::getShotsLandedTotal() const
{
	return _shotsLandedCounterTotal;
}

/**
 * Gets the Soldier's firing-proficiency.
 * @return, firing-proficiency as percent (-1 if no shots fired yet)
 */
int SoldierDiary::getProficiency() const
{
	if (_shotsFiredCounterTotal != 0)
		return 100 * _shotsLandedCounterTotal / _shotsFiredCounterTotal;

	return -1;
}

/**
 * Gets trap kills total.
 * @param rules - pointer to the Ruleset
 * @return, total trap kills
 */
int SoldierDiary::getTrapKillTotal(const Ruleset* const rules) const
{
	int ret (0);
	for (std::vector<BattleUnitKill*>::const_iterator
			i = _killList.begin();
			i != _killList.end();
			++i)
	{
		if ((*i)->hostileTurn() == true)
		{
			const RuleItem* const itRule (rules->getItemRule((*i)->_weapon));
			if (itRule == nullptr)
				++ret;
			else
			{
				switch (itRule->getBattleType())
				{
					case BT_GRENADE:
					case BT_PROXYGRENADE:
						++ret;
				}
			}
		}
	}
	return ret;
}

/**
 * Gets reaction kill total.
 * @param rules - pointer to the Ruleset
 * @return, total reaction-fire kills
 */
 int SoldierDiary::getReactionFireKillTotal(const Ruleset* const rules) const
 {
	int ret (0);
	for (std::vector<BattleUnitKill*>::const_iterator
			i = _killList.begin();
			i != _killList.end();
			++i)
	{
		if ((*i)->hostileTurn() == true)
		{
			const RuleItem* const itRule (rules->getItemRule((*i)->_weapon));
			if (itRule != nullptr)
			{
				switch (itRule->getBattleType())
				{
					case BT_GRENADE:
					case BT_PROXYGRENADE:
						break;

					default:
						++ret;
				}
			}
		}
	}
	return ret;
 }

/**
 * Gets the total of terror tacticals.
 * @param tacticals - pointer to a vector of pointers to MissionStatistics
 * @return, total terror missions
 */
int SoldierDiary::getTerrorMissionTotal(const std::vector<MissionStatistics*>* const tacticals) const
{
	int ret (0);
	for (std::vector<MissionStatistics*>::const_iterator
			i = tacticals->begin();
			i != tacticals->end();
			++i)
	{
		for (std::vector<int>::const_iterator
				j = _missionIdList.begin();
				j != _missionIdList.end();
				++j)
		{
			if (*j == (*i)->id
				&& (*i)->isBaseDefense() == false
				&& (*i)->isAlienBase() == false
				&& (*i)->isUfoMission() == false)
			{
				++ret;
			}
		}
	}
	return ret;
}

/**
 * Gets the total of night tacticals.
 * @param tacticals - pointer to a vector of pointers to MissionStatistics
 * @return, total night missions
 */
int SoldierDiary::getNightMissionTotal(const std::vector<MissionStatistics*>* const tacticals) const
{
	int ret (0);
	for (std::vector<MissionStatistics*>::const_iterator
			i = tacticals->begin();
			i != tacticals->end();
			++i)
	{
		for (std::vector<int>::const_iterator
				j = _missionIdList.begin();
				j != _missionIdList.end();
				++j)
		{
			if (*j == (*i)->id
				&& (*i)->shade >= MissionStatistics::NIGHT_SHADE
				&& (*i)->isBaseDefense() == false
				&& (*i)->isAlienBase() == false)
			{
				++ret;
			}
		}
	}
	return ret;
}

/**
 * Gets the total of night terror tacticals.
 * @param tacticals - pointer to a vector of pointers to MissionStatistics
 * @return, total night terror missions
 */
int SoldierDiary::getNightTerrorMissionTotal(const std::vector<MissionStatistics*>* const tacticals) const
{
	int ret (0);
	for (std::vector<MissionStatistics*>::const_iterator
			i = tacticals->begin();
			i != tacticals->end();
			++i)
	{
		for (std::vector<int>::const_iterator
				j = _missionIdList.begin();
				j != _missionIdList.end();
				++j)
		{
			if (*j == (*i)->id
				&& (*i)->shade >= MissionStatistics::NIGHT_SHADE
				&& (*i)->isBaseDefense() == false
				&& (*i)->isAlienBase() == false
				&& (*i)->isUfoMission() == false)
			{
				++ret;
			}
		}
	}
	return ret;
}

/**
 * Gets the total of base defense tacticals.
 * @param tacticals - pointer to a vector of pointers to MissionStatistics
 * @return, total base defense tacticals
 */
int SoldierDiary::getBaseDefenseMissionTotal(const std::vector<MissionStatistics*>* const tacticals) const
{
	int ret (0);
	for (std::vector<MissionStatistics*>::const_iterator
			i = tacticals->begin();
			i != tacticals->end();
			++i)
	{
		for (std::vector<int>::const_iterator
				j = _missionIdList.begin();
				j != _missionIdList.end();
				++j)
		{
			if (*j == (*i)->id
				&& (*i)->isBaseDefense() == true)
			{
				++ret;
			}
		}
	}
	return ret;
}

/**
 * Gets the total of alien base assaults.
 * @param tacticals - pointer to a vector of pointers to MissionStatistics
 * @return, total alien base assaults
 */
int SoldierDiary::getAlienBaseAssaultTotal(const std::vector<MissionStatistics*>* const tacticals) const
{
	int ret (0);
	for (std::vector<MissionStatistics*>::const_iterator
			i = tacticals->begin();
			i != tacticals->end();
			++i)
	{
		for (std::vector<int>::const_iterator
				j = _missionIdList.begin();
				j != _missionIdList.end();
				++j)
		{
			if (*j == (*i)->id
				&& (*i)->isAlienBase() == true)
			{
				++ret;
			}
		}
	}
	return ret;
}

/**
 * Gets the total of important tacticals.
 * @param tacticals - pointer to a vector of pointers to MissionStatistics
 * @return, total important missions
 */
int SoldierDiary::getImportantMissionTotal(const std::vector<MissionStatistics*>* const tacticals) const
{
	int ret (0);
	for (std::vector<MissionStatistics*>::const_iterator
			i = tacticals->begin();
			i != tacticals->end();
			++i)
	{
		for (std::vector<int>::const_iterator
				j = _missionIdList.begin();
				j != _missionIdList.end();
				++j)
		{
			if (*j == (*i)->id
				&& (*i)->isUfoMission() == false)
			{
				++ret;
			}
		}
	}
	return ret;
}

/**
 * Gets the Valient Crux total.
 * @param tacticals - pointer to a vector of pointers to MissionStatistics
 * @return, total valiant crutches
 */
int SoldierDiary::getValiantCruxTotal(const std::vector<MissionStatistics*>* const tacticals) const
{
	int ret (0);
	for (std::vector<MissionStatistics*>::const_iterator
			i = tacticals->begin();
			i != tacticals->end();
			++i)
	{
		for (std::vector<int>::const_iterator
				j = _missionIdList.begin();
				j != _missionIdList.end();
				++j)
		{
			if (*j == (*i)->id && (*i)->valiantCrux == true)
				++ret;
		}
	}
	return ret;
}

/**
 * Increments the Soldier's service-time by one month.
 */
void SoldierDiary::addMonthlyService()
{
	++_monthsService;
}

/**
 * Gets the total months the Soldier has been in service.
 * @return, quantity of months
 */
int SoldierDiary::getMonthsService() const
{
	return _monthsService;
}

/**
 * Awards a special medal to each of the original 8 Soldiers.
 */
void SoldierDiary::awardOriginalEight()
{
	_solAwards.push_back(new SoldierAward(
									"STR_MEDAL_ORIGINAL8_NAME",
									"noQual",
									false));
}

/**
 * Awards an honorary medal upon joining team-xCom.
 */
void SoldierDiary::awardHonoraryMedal()
{
	_solAwards.push_back(new SoldierAward(
									"STR_MEDAL_HONOR_CROSS_NAME",
									"noQual",
									false));
}


//____________________________________//
/*___________________________________/
/
/         SOLDIER AWARD class
/ ___________________________________*/
/**
 * Initializes a SoldierAward.
 * @param type		- reference to the type
 * @param qualifier	- reference to the noun/qualifier (default "noQual")
 * @param recent	- true to inform player of award after tactical (default true)
 */
SoldierAward::SoldierAward(
		const std::string& type,
		const std::string& qualifier,
		bool recent)
	:
		_type(type),
		_qual(qualifier),
		_recent(recent),
		_level(0u)
{}

/**
 * Initializes a SoldierAward entry from YAML.
 * @param node - reference a YAML node
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

	_recent = false;
}

/**
 * Saves this SoldierAward to a YAML file.
 * @return, YAML node
 */
YAML::Node SoldierAward::save() const
{
	YAML::Node node;

	node["type"] = _type;
	node["level"] = static_cast<int>(_level); // WARNING: Save this even if '0'. don't know why tho

	if (_qual != "noQual") node["qualifier"] = _qual;

	return node;
}

/**
 * Gets this SoldierAward's type.
 * @return, the type
 */
const std::string SoldierAward::getType() const
{
	return _type;
}

/**
 * Gets this SoldierAward's noun/qualifier.
 * @return, the qualifier
 */
const std::string SoldierAward::getQualifier() const
{
	return _qual;
}

/**
 * Gets this SoldierAward's level-type.
 * @param skip -
 * @return, decoration-level as string
 */
const std::string SoldierAward::getClassType(int skip) const
{
	std::ostringstream oststr;
	oststr << "STR_AWARD_" << _level - skip;
	return oststr.str();
}

/**
 * Gets this SoldierAward's level as an integer.
 * @return, decoration-level as int
 */
size_t SoldierAward::getClassLevel() const
{
	return _level;
}

/**
 * Gets this SoldierAward's level-description.
 * @return, decoration-level description
 */
const std::string SoldierAward::getClassDescription() const
{
	std::ostringstream oststr;
	oststr << "STR_AWARD_DECOR_" << _level;
	return oststr.str();
}

/**
 * Gets this SoldierAward's level-class - represents quantity of stars.
 * @return, decoration-level class
 */
const std::string SoldierAward::getClassDegree() const
{
	std::ostringstream oststr;
	oststr << "STR_AWARD_CLASS_" << _level;
	return oststr.str();
}

/**
 * Gets if this SoldierAward has been recently awarded.
 * @return, true if recent
 */
bool SoldierAward::isAwardRecent() const
{
	return _recent;
}

/**
 * Clears the recently-awarded flag from this SoldierAward.
 */
void SoldierAward::clearRecent()
{
	_recent = false;
}

/**
 * Adds a level of decoration to this SoldierAward.
 */
void SoldierAward::addClassLevel()
{
	++_level;
	_recent = true;
}

}
