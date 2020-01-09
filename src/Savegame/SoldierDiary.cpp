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

#include "SoldierDiary.h"

#include <algorithm>	// find()
//#include <sstream>	// std::ostringstream

#include "BattleUnitStatistics.h"
#include "TacticalStatistics.h"

//#include "../Engine/Logger.h"

#include "../Ruleset/RuleAward.h"
#include "../Ruleset/Ruleset.h"


namespace OpenXcom
{

/**
 * Creates the SoldierDiary.
 */
SoldierDiary::SoldierDiary()
	:
		_daysWoundedTotal			(0),
		_monthsService				(0),
		_unconsciousTotal			(0),
		_shotAtCounterTotal			(0),
		_hitCounterTotal			(0),
		_loneSurvivorTotal			(0),
		_totalShotByFriendlyCounter	(0),
		_totalShotFriendlyCounter	(0),
		_ironManTotal				(0),
		_longDistanceHitCounterTotal(0),
		_lowAccuracyHitCounterTotal	(0),
		_shotsFiredCounterTotal		(0),
		_shotsLandedCounterTotal	(0),
		_shotAtCounter10in1Mission	(0),
		_hitCounter5in1Mission		(0),
		_timesWoundedTotal			(0),
		_allAliensKilledTotal		(0),
		_mediApplicationsTotal		(0),
		_revivedUnitTotal			(0),
		_kia						(0),
		_mia						(0)
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
		_daysWoundedTotal				(copyThat._daysWoundedTotal),
		_totalShotByFriendlyCounter		(copyThat._totalShotByFriendlyCounter),
		_totalShotFriendlyCounter		(copyThat._totalShotFriendlyCounter),
		_loneSurvivorTotal				(copyThat._loneSurvivorTotal),
		_monthsService					(copyThat._monthsService),
		_unconsciousTotal				(copyThat._unconsciousTotal),
		_shotAtCounterTotal				(copyThat._shotAtCounterTotal),
		_hitCounterTotal				(copyThat._hitCounterTotal),
		_ironManTotal					(copyThat._ironManTotal),
		_longDistanceHitCounterTotal	(copyThat._longDistanceHitCounterTotal),
		_lowAccuracyHitCounterTotal		(copyThat._lowAccuracyHitCounterTotal),
		_shotsFiredCounterTotal			(copyThat._shotsFiredCounterTotal),
		_shotsLandedCounterTotal		(copyThat._shotsLandedCounterTotal),
		_shotAtCounter10in1Mission		(copyThat._shotAtCounter10in1Mission),
		_hitCounter5in1Mission			(copyThat._hitCounter5in1Mission),
		_timesWoundedTotal				(copyThat._timesWoundedTotal),
		_allAliensKilledTotal			(copyThat._allAliensKilledTotal),
		_mediApplicationsTotal			(copyThat._mediApplicationsTotal),
		_revivedUnitTotal				(copyThat._revivedUnitTotal),
		_kia							(copyThat._kia),
		_mia							(copyThat._mia)
{
	for (size_t
			i = 0u;
			i != copyThat._tacIdList.size();
			++i)
	{
//		if (copyThat._tacIdList.at(i) != nullptr) // Bzzzt.
		_tacIdList.push_back(copyThat._tacIdList.at(i));
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
				unitRank		(copyThat._killList.at(i)->_rank),
				race			(copyThat._killList.at(i)->_race),
				weapon			(copyThat._killList.at(i)->_weapon),
				load			(copyThat._killList.at(i)->_load);
			int
				mission			(copyThat._killList.at(i)->_tactical),
				turn			(copyThat._killList.at(i)->_turn),
				points			(copyThat._killList.at(i)->_points);

			UnitFaction faction	(copyThat._killList.at(i)->_faction);
			UnitStatus status	(copyThat._killList.at(i)->_status);

			_killList.push_back(new BattleUnitKill(
												unitRank,
												race,
												weapon,
												load,
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
SoldierDiary& SoldierDiary::operator =(const SoldierDiary& assignThat)
{
	if (this != &assignThat)
	{
		_daysWoundedTotal				= assignThat._daysWoundedTotal;
		_totalShotByFriendlyCounter		= assignThat._totalShotByFriendlyCounter;
		_totalShotFriendlyCounter		= assignThat._totalShotFriendlyCounter;
		_loneSurvivorTotal				= assignThat._loneSurvivorTotal;
		_monthsService					= assignThat._monthsService;
		_unconsciousTotal				= assignThat._unconsciousTotal;
		_shotAtCounterTotal				= assignThat._shotAtCounterTotal;
		_hitCounterTotal				= assignThat._hitCounterTotal;
		_ironManTotal					= assignThat._ironManTotal;
		_longDistanceHitCounterTotal	= assignThat._longDistanceHitCounterTotal;
		_lowAccuracyHitCounterTotal		= assignThat._lowAccuracyHitCounterTotal;
		_shotsFiredCounterTotal			= assignThat._shotsFiredCounterTotal;
		_shotsLandedCounterTotal		= assignThat._shotsLandedCounterTotal;
		_shotAtCounter10in1Mission		= assignThat._shotAtCounter10in1Mission;
		_hitCounter5in1Mission			= assignThat._hitCounter5in1Mission;
		_timesWoundedTotal				= assignThat._timesWoundedTotal;
		_allAliensKilledTotal			= assignThat._allAliensKilledTotal;
		_mediApplicationsTotal			= assignThat._mediApplicationsTotal;
		_revivedUnitTotal				= assignThat._revivedUnitTotal;
		_kia							= assignThat._kia;
		_mia							= assignThat._mia;

		_tacIdList.clear();
		for (std::vector<int>::const_iterator
				i = assignThat._tacIdList.begin();
				i != assignThat._tacIdList.end();
				++i)
		{
			_tacIdList.push_back(*i);
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
					unitRank		(assignThat._killList.at(i)->_rank),
					race			(assignThat._killList.at(i)->_race),
					weapon			(assignThat._killList.at(i)->_weapon),
					load			(assignThat._killList.at(i)->_load);
				int
					mission			(assignThat._killList.at(i)->_tactical),
					turn			(assignThat._killList.at(i)->_turn),
					points			(assignThat._killList.at(i)->_points);

				UnitFaction faction	(assignThat._killList.at(i)->_faction);
				UnitStatus status	(assignThat._killList.at(i)->_status);

				_killList.push_back(new BattleUnitKill(
													unitRank,
													race,
													weapon,
													load,
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

	_tacIdList						= node["tacIdList"]						.as<std::vector<int>>(_tacIdList);
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

	_kia = node["kia"].as<int>(_kia);
	_mia = node["mia"].as<int>(_mia);
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

	if (_tacIdList.empty() == false)		node["tacIdList"]					= _tacIdList;

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

	if (_kia) node["kia"] = _kia;
	if (_mia) node["mia"] = _mia;

	return node;
}

/**
 * Updates this SoldierDiary's statistics.
 * @note BattleUnitKill is a substruct of BattleUnitStatistics.
 * @param tacstats	- pointer to BattleUnitStatistics for info on a Soldier's
 *					  current tactical performance ala DebriefingState.
 * @param tacticals	- pointer to latest TacticalStatistics
 */
void SoldierDiary::postTactical(
		BattleUnitStatistics* const tacstats,
		const TacticalStatistics* const tactical)
{
	for (std::vector<BattleUnitKill*>::const_iterator
			i = tacstats->kills.begin();
			i != tacstats->kills.end();
			++i)
	{
		(*i)->setTurn();
		_killList.push_back(*i); // transfer ownership of BattleUnitKills.
	}
	tacstats->kills.clear();

	if (tactical->success == true)
	{
		if (tacstats->loneSurvivor == true)
			++_loneSurvivorTotal;

		if (tacstats->ironMan == true)
			++_ironManTotal;
	}

	if (tacstats->daysWounded != 0)
	{
		_daysWoundedTotal += tacstats->daysWounded;
		++_timesWoundedTotal;
	}

	if (tacstats->wasUnconscious == true)
		++_unconsciousTotal;

	_shotAtCounterTotal				+= tacstats->shotAtCounter;
	_shotAtCounter10in1Mission		+= tacstats->shotAtCounter / 5;		// kL_edit.
	_hitCounterTotal				+= tacstats->hitCounter;
	_hitCounter5in1Mission			+= tacstats->hitCounter / 2;		// kL_edit.
	_totalShotByFriendlyCounter		+= tacstats->shotByFriendlyCounter;
	_totalShotFriendlyCounter		+= tacstats->shotFriendlyCounter;
	_longDistanceHitCounterTotal	+= tacstats->longDistanceHitCounter;
	_lowAccuracyHitCounterTotal		+= tacstats->lowAccuracyHitCounter;
	_shotsFiredCounterTotal			+= tacstats->shotsFiredCounter;
	_shotsLandedCounterTotal		+= tacstats->shotsLandedCounter;
	_mediApplicationsTotal			+= tacstats->medikitApplications;
	_revivedUnitTotal				+= tacstats->revivedSoldier;

	if (tacstats->nikeCross == true)
		++_allAliensKilledTotal;

	if (tacstats->KIA == true)
		_kia = 1;
	else if (tacstats->MIA == true)
		_mia = 1;

	_tacIdList.push_back(tactical->id);
}

/**
 * Accesses the SoldierAwards in this SoldierDiary.
 * @return, reference to a vector of pointers to SoldierAward
 */
std::vector<SoldierAward*>& SoldierDiary::getSoldierAwards()
{
	return _solAwards;
}

/**
 * Updates the owner's SoldierAwards.
 * @note May God help anyone who has to address this further!!!!
 * @param rules		- pointer to the Ruleset
 * @param tacticals	- reference to a vector of pointers to TacticalStatistics
 * @return, true if an award is awarded
 */
bool SoldierDiary::updateAwards(
		const Ruleset* const rules,
		const std::vector<TacticalStatistics*>& tacticals)
{
	//Log(LOG_INFO) << "";
	//Log(LOG_INFO) << "SoldierDiary::updateAwards()";
	bool showAwardsPostTactical (false); // Return shall be TRUE if 1+ Award is granted/upgraded.

	std::vector<std::string> qualifiers;			// types of generic Awards
	std::map<std::string, size_t> levelRequired;	// qualifier + levelQty required

	std::string criteriaType;
	int val;

	std::map<std::string, int> solTotal;
	int threshold;

	const std::vector<std::vector<std::pair<int, std::vector<std::string>>>>* killCriteria;

	int
		iterCur,
		iterPre;
	bool
		found,
		firstOfType;
	size_t
		bType,
		dType;

	const RuleItem
		* weaponRule,
		* loadRule;

//	int iter (1); // debug.
	const std::map<std::string, const RuleAward*>& allAwards (rules->getAwardsList()); // loop over all RuleAwards ->
	for (std::map<std::string, const RuleAward*>::const_iterator
			i = allAwards.begin();
			i != allAwards.end();
			/*++iter*/) // debug.
	{
		const std::string& awardType (i->first);
		const RuleAward* const awardRule (i->second);


		//Log(LOG_INFO) << "";
		//Log(LOG_INFO) << "";
		//Log(LOG_INFO) << ". iter awardList - " << awardType << " [try." << iter << "]";

		qualifiers.clear();
		levelRequired.clear();
		//Log(LOG_INFO) << ". clear qualifier-strings";
		//Log(LOG_INFO) << ". clear levelRequired map";


		levelRequired["noQual"] = 0u;	// only if Soldier does not yet have the current Award.
										// it's also used as a generic place-holder for qualified Awards ...
		//Log(LOG_INFO) << ". (init) levelRequired[\"noQual\"] 0";

		for (std::vector<SoldierAward*>::const_iterator	// get the qualifiers for the current Award if it has already
				solAward = _solAwards.begin();			// been granted to the Soldier possibly in the last iteration
				solAward != _solAwards.end();
				++solAward)
		{
			if ((*solAward)->getType() == awardType)
			{
				levelRequired[(*solAward)->getQualifier()] = (*solAward)->getAwardLevel() + 1;
				//Log(LOG_INFO) << ". . levelRequired[\"" << (*solAward)->getQualifier() << "\"] " << levelRequired[(*solAward)->getQualifier()];
			}
		}


		const std::map<std::string, std::vector<int>>::const_iterator j (awardRule->getCriteria()->begin());
		criteriaType = j->first;
		const std::vector<int>& levels (j->second);

		//Log(LOG_INFO) << "";
		//Log(LOG_INFO) << ". . criteriaType [" << criteriaType << "]";

		// skip the Award if its max level has been reached
		if (levelRequired["noQual"] == levels.size())	// use the "noQual" entry assigned above just to find out what the highest level is.
		{												// ... in practice it's always "10"
			//Log(LOG_INFO) << ". . . max level reached - go to next Award";
//			iter = 0; // debug.
			++i;
			continue;
		}


		val = levels.at(levelRequired["noQual"]); // use the "noQual" entry assigned above just to find out what the highest value is.

		// the following criteria have no qualifiers so only "noQual" will ever be compared
		if (   (criteriaType == "totalKills"				&& getKillTotal() < val)
			|| (criteriaType == "totalMissions"				&& static_cast<int>(_tacIdList.size()) < val)
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
			|| (criteriaType == "totalFriendlyFired"		&& (_totalShotByFriendlyCounter < val || _kia == 1 || _mia == 1)) // didn't survive ...... NOTE: This screws up when refreshing Awards.
			|| (criteriaType == "totalLoneSurvivor"			&& _loneSurvivorTotal < val)
			|| (criteriaType == "totalIronMan"				&& _ironManTotal < val)
			|| (criteriaType == "totalImportantMissions"	&& getImportantMissionTotal(tacticals) < val)
			|| (criteriaType == "totalLongDistanceHits"		&& _longDistanceHitCounterTotal < val)
			|| (criteriaType == "totalLowAccuracyHits"		&& _lowAccuracyHitCounterTotal < val)
			|| (criteriaType == "totalReactionFire"			&& getReactionFireKillTotal(rules) < val)
			|| (criteriaType == "totalTimesWounded"			&& _timesWoundedTotal < val)
			|| (criteriaType == "totalDaysWounded"			&& _daysWoundedTotal < val)
			|| (criteriaType == "totalValientCrux"			&& getValiantCruxTotal(tacticals) < val)
			|| (criteriaType == "totalTrapKills"			&& getTrapKillTotal(rules) < val)
			|| (criteriaType == "totalAlienBaseAssaults"	&& getAlienBaseAssaultTotal(tacticals) < val)
			|| (criteriaType == "totalAllAliensKilled"		&& _allAliensKilledTotal < val)
			|| (criteriaType == "totalMediApplications"		&& _mediApplicationsTotal < val)
			|| (criteriaType == "totalRevives"				&& _revivedUnitTotal < val)
			|| (criteriaType == "totalIsDead"				&& _kia < val)
			|| (criteriaType == "totalIsMissing"			&& _mia < val))
		{
			//Log(LOG_INFO) << ". . . no Total Award - go to next Award";
//			iter = 0; // debug.
			++i;
			continue;
		}

		// as soon as an Award's conditions *fail to be achieved* then stop
		// recursing and iterate to the next Award.

		if (   criteriaType == "totalKillsWeapon" // this category of Award needs a qualifier
			|| criteriaType == "totalKillsRegion"
			|| criteriaType == "totalKillsByRace"
			|| criteriaType == "totalKillsByRank")
		{
			//Log(LOG_INFO) << ". . . try Award w/ weapon,region,race,rank";

			if		(criteriaType == "totalKillsWeapon") solTotal = getWeaponTotal();
			else if	(criteriaType == "totalKillsRegion") solTotal = getRegionTotal(tacticals);
			else if	(criteriaType == "totalKillsByRace") solTotal = getAlienRaceTotal();
			else if	(criteriaType == "totalKillsByRank") solTotal = getAlienRankTotal();

			for (std::map<std::string, int>::const_iterator			// loop over 'solTotal' and compare results with requiredLevels.
					k = solTotal.begin();
					k != solTotal.end();
					++k)
			{
				//Log(LOG_INFO) << ". . . . " << (k->first) << " - " << (k->second);

				if (levelRequired.count(k->first) == 0)				// no matching Qualifier so get the (first) level from 'criteria'
				{
					//Log(LOG_INFO) << ". . . . . no relevant qualifier, threshold = " << (levels.front());
					threshold = levels.front();
				}
				else if (levelRequired[k->first] != levels.size())	// otherwise get the level per the soldier's Award decoration.
				{
					//Log(LOG_INFO) << ". . . . . qualifier found, threshold= " << levels.at(levelRequired[k->first]);
					threshold = levels.at(levelRequired[k->first]);
				}
				else
					threshold = -1;

				if (threshold != -1 && k->second >= threshold)		// if a criteria was set AND the stat's count exceeds that criteria ...
				{
					//Log(LOG_INFO) << ". . . . . threshold good, add to qualifiers vector";
					qualifiers.push_back(k->first);
				}
			}

			if (qualifiers.empty() == true)							// if 'qualifiers' is still empty soldier did not get an award.
			{
				//Log(LOG_INFO) << ". . . . no Award w/ weapon,region,race,rank - go to next Award";
//				iter = 0; // debug.
				++i;
				continue;
			}
		}
		else if (criteriaType == "killsCriteriaCareer" // NOTE: The And-vector has been fixed.
			  || criteriaType == "killsCriteriaMission"
			  || criteriaType == "killsCriteriaTurn")
		{
			//Log(LOG_INFO) << ". . . try Award w/ career,mission,turn";

			killCriteria = awardRule->getKillCriteria();
			//Log(LOG_INFO) << ". . . killCriteria Or-vectors= " << killCriteria->size();

			int
				qtySuccess_or  (0),
				qtySuccess_and (0);
			//Log(LOG_INFO) << ". . . . (init) qtySuccess_or  0";
			//Log(LOG_INFO) << ". . . . (init) qtySuccess_and 0";


			for (std::vector<std::vector<std::pair<int, std::vector<std::string>>>>::const_iterator // loop over the orCriteria ->
					orCriteria = killCriteria->begin();
					orCriteria != killCriteria->end();
					++orCriteria)
			{
				//Log(LOG_INFO) << "";
				//Log(LOG_INFO) << ". . . . iter orCriteria - And-vectors= " << orCriteria->size();

				int qtySuccess_detail (0);
				//Log(LOG_INFO) << ". . . . (init) qtySuccess_detail 0";

				for (std::vector<std::pair<int, std::vector<std::string>>>::const_iterator // loop over the andCriteria ->
						andCriteria = orCriteria->begin();
						andCriteria != orCriteria->end();
						++andCriteria)
				{
					//Log(LOG_INFO) << "";
					//Log(LOG_INFO) << ". . . . . iter andCriteria - details= " << andCriteria->second.size() << " [req." << andCriteria->first << "]";

					int qty0;
					if (criteriaType == "killsCriteriaCareer")
						qty0 = 0; // counts the And-vectors that match their details against Soldier's killstats.
					else
						qty0 = 1;	// "killsWith..." Turns or Missions start at 1 because of how iterCur and iterPre work. thanks.
									// The *real* reason is that the tacId/turnId is initialized on the first killstat and so needs/gets a free increment here.

					//Log(LOG_INFO) << ". . . . . (init) qty0 0";

					bool
						resync   (false),
						iterInit (false);
					iterCur = // for a turn or a mission Award, not used for a career Award.
					iterPre = -1;
					//Log(LOG_INFO) << ". . . . . (init) resync FALSE";
					//Log(LOG_INFO) << ". . . . . (init) iterCur/iterPre -1";

					std::vector<int> ids;
					int
						id         (-1),
						recursions (-1);
					//Log(LOG_INFO) << ". . . . . (init) id -1";
					//Log(LOG_INFO) << ". . . . . (init) recursions -1";

					for (std::vector<BattleUnitKill*>::const_iterator // loop over the Soldier's killstats ->
							killstat = _killList.begin();
							killstat != _killList.end();
							++killstat)
					{
						//Log(LOG_INFO) << "";
						//Log(LOG_INFO) << ". . . . . . iter Soldier's killList";
						//Log(LOG_INFO) << ". . . . . . . race=\t\t\t "		<< (*killstat)->_race;
						//Log(LOG_INFO) << ". . . . . . . rank=\t\t\t "		<< (*killstat)->_rank;
						//Log(LOG_INFO) << ". . . . . . . faction=\t\t "		<< (*killstat)->getUnitFactionString();
						//Log(LOG_INFO) << ". . . . . . . status=\t\t "		<< (*killstat)->getUnitStatusString();
						//Log(LOG_INFO) << ". . . . . . . weapon=\t\t "		<< (*killstat)->_weapon;
						//Log(LOG_INFO) << ". . . . . . . load=\t\t\t "		<< (*killstat)->_load;
						//Log(LOG_INFO) << ". . . . . . . turnHostile=\t "	<< (*killstat)->hostileTurn();
						//Log(LOG_INFO) << ". . . . . . . tacId=\t\t\t "		<< (*killstat)->_tactical;
						//Log(LOG_INFO) << ". . . . . . . turnId=\t\t\t "		<< (*killstat)->_turn;

						if (criteriaType == "killsCriteriaMission")
						{
							id = (*killstat)->_tactical;
							if (std::find(
										ids.begin(),
										ids.end(),
										id) != ids.end()) // if the tacId has already been iterated skip it.
							{
								//Log(LOG_INFO) << "";
								//Log(LOG_INFO) << ". . . . . . . id found - (set) resync TRUE";
								resync = true;
							}

							//Log(LOG_INFO) << "";
							//Log(LOG_INFO) << ". . . . . . resync  = " << resync;

							iterCur = id;
							if (resync == false)
							{
								if (killstat != _killList.begin())
									iterPre = (*(killstat - 1))->_tactical;
							}
							else
							{
								resync = false;
								if (iterInit == true)
								{
									iterInit = false;
									if (std::find(
												ids.begin(),
												ids.end(),
												id) == ids.end()) // recursion to a previously searched tacId - bypass it.
									{
										//Log(LOG_INFO) << ". . . . . . . . (set) iterPre=iterCur";
										iterPre = iterCur;
									}
								}
							}
						}
						else if (criteriaType == "killsCriteriaTurn")
						{
							id = (*killstat)->_turn;
							if (std::find(
										ids.begin(),
										ids.end(),
										id) != ids.end()) // if the turnId has already been searched skip it.
							{
								//Log(LOG_INFO) << "";
								//Log(LOG_INFO) << ". . . . . . . id found - (set) resync TRUE";
								resync = true;
							}

							//Log(LOG_INFO) << "";
							//Log(LOG_INFO) << ". . . . . . resync  = " << resync;

							iterCur = id;
							if (resync == false)
							{
								if (killstat != _killList.begin())
									iterPre = (*(killstat - 1))->_turn;
							}
							else
							{
								resync = false;
								if (iterInit == true)
								{
									iterInit = false;
									if (std::find(
												ids.begin(),
												ids.end(),
												id) == ids.end()) // recursion to a previously searched turnId - bypass it.
									{
										//Log(LOG_INFO) << ". . . . . . . . (set) iterPre=iterCur";
										iterPre = iterCur;
									}
								}
							}
						}

						//Log(LOG_INFO) << ". . . . . . iterCur = " << iterCur;
						//Log(LOG_INFO) << ". . . . . . iterPre = " << iterPre;

						if (iterCur != iterPre) // always false for criteriaCareer
						{
							//Log(LOG_INFO) << "";
							//Log(LOG_INFO) << ". . . . . . . (iterCur!=iterPre)";

							if (iterPre != -1)
							{
								iterCur = iterPre;
								resync = true;
								//Log(LOG_INFO) << ". . . . . . . . (set) resync TRUE";
							}
							//Log(LOG_INFO) << ". . . . . . . . - CONTINUE to next killstat";
							continue;
						}

						found = true;
						for (std::vector<std::string>::const_iterator // loop over the DETAILs of the andCriteria vector ->
								detail = andCriteria->second.begin();
								detail != andCriteria->second.end();
								++detail)
						{
							//Log(LOG_INFO) << "";
							//Log(LOG_INFO) << ". . . . . . . iter DETAIL - " << (*detail);
							static const std::string
								bType_array[BATS] // these entries shall correspond to BattleType enum (RuleItem.h)
								{
									"BT_NONE",		"BT_FIREARM",		"BT_AMMO",		"BT_MELEE",
									"BT_GRENADE",	"BT_PROXYGRENADE",	"BT_MEDIKIT",	"BT_SCANNER",
									"BT_MINDPROBE",	"BT_PSIAMP",		"BT_FLARE",		"BT_CORPSE",
									"BT_FUEL",		"BT_END"
								},
								dType_array[DATS] // these entries shall correspond to DamageType enum (RuleItem.h)
								{
									"DT_NONE",		"DT_AP",			"DT_IN",		"DT_HE",
									"DT_LASER",		"DT_PLASMA",		"DT_STUN",		"DT_MELEE",
									"DT_ACID",		"DT_SMOKE",			"DT_END"
								};

							for (bType = 0u; bType != BATS - 1u; ++bType)
								if (bType_array[bType] == *detail)	//Log(LOG_INFO) << ". . . . . . . . iter bType";
									break;							//Log(LOG_INFO) << ". . . . . . . . bType= " << (*detail);

							for (dType = 0u; dType != DATS - 1u; ++dType)
								if (dType_array[dType] == *detail)	//Log(LOG_INFO) << ". . . . . . . . iter dType";
									break;							//Log(LOG_INFO) << ". . . . . . . . dType= " << (*detail);

							weaponRule	= rules->getItemRule((*killstat)->_weapon);
							loadRule	= rules->getItemRule((*killstat)->_load);

							if (   (*killstat)->_rank	!= *detail // if every killstat mis-matches the (single) Detail break and try the next andCriteria.
								&& (*killstat)->_race	!= *detail
								&& (*killstat)->_weapon	!= *detail
								&& (*killstat)->_load	!= *detail
								&& (*killstat)->getUnitStatusString()  != *detail
								&& (*killstat)->getUnitFactionString() != *detail
								&& ((weaponRule == nullptr
										&& (   bType_array[bType] != "BT_NONE"
											|| bType_array[bType] != "BT_END"))
									|| (   weaponRule != nullptr
										&& weaponRule->getBattleType() != static_cast<BattleType>(bType)))
								&& ((loadRule == nullptr
										&& (   dType_array[dType] != "DT_NONE"
											|| dType_array[dType] != "DT_END"))
									|| (   loadRule != nullptr
										&& loadRule->getDamageType() != static_cast<DamageType>(dType))))
							{
								//Log(LOG_INFO) << ". . . . . . . . all killstats mismatch rule-Detail - BREAK detail & go to next killstat";
								found = false;
								break;
							}
						} // detail ^

						if (found == true) // all details were found in the killstat -> so check current-qty vs the single andCriteria's required-qty ->
						{
							//Log(LOG_INFO) << "";
							//Log(LOG_INFO) << ". . . . . . . ++qty0= " << (qty0 + 1) << " required= " << andCriteria->first * levels.at(levelRequired["noQual"]);

							if (++qty0 % andCriteria->first == 0) // for each detail achieved.
							{
								++qtySuccess_detail;
								//Log(LOG_INFO) << ". . . . . . . . ++qtySuccess_detail= " << qtySuccess_detail;
							}

							if (qty0 == andCriteria->first * levels.at(levelRequired["noQual"])) // for each detail achieved @ levels.
							{
								//Log(LOG_INFO) << ". . . . . . . . detail-qty is MET - BREAK killstats & go to next andCriteria";
								break;
							}
						}

						if (criteriaType != "killsCriteriaCareer"						// for Missions/Turns: if the killList-vector gets to the end of
							&& killstat == _killList.end() - 1							// all the Soldier's killstats recurse the killList but start at the
							&& _killList.begin() + (++recursions) != _killList.end())	// 2nd, 3rd, 4th, etc. entry and try each different tac/turnIds
						{
							//Log(LOG_INFO) << "";
							//Log(LOG_INFO) << ". . . . . . . recursions= " << recursions;

							qty0 = (qtySuccess_detail * andCriteria->first) + 1; // +1 for 1st killstat after recursion.
							//Log(LOG_INFO) << ". . . . . . . (set) qty0= " << qty0;

							iterCur =		// these are probably redundant to a degree
							iterPre = -1;	// but I ain't in the mood to refactor it.
							//Log(LOG_INFO) << ". . . . . . . (reset) iterCur/iterPre -1";

							resync =
							iterInit = true;
							//Log(LOG_INFO) << ". . . . . . . (set) resync TRUE";
							//Log(LOG_INFO) << ". . . . . . . (set) iterInit TRUE";

							//Log(LOG_INFO) << ". . . . . . . push_back id= " << id;
							ids.push_back(id);

							//Log(LOG_INFO) << "";
							//Log(LOG_INFO) << ". . . . . . . ----> RECURSE killList RECURSE <----";
							killstat = _killList.begin() + (recursions);
						}
					} // killstat ^

					// NOTE: If killList runs to end() then the And-vector has
					// failed so stop looking and try the next orCriteria; ie.
					// the And-vector will break on a detail's success as soon
					// as the detail's required-qty is reached ...

				} // andCriteria ^

				//Log(LOG_INFO) << "";

				qtySuccess_and += qtySuccess_detail / static_cast<int>(orCriteria->size()); // tabulates +levels if the And-vector was successful.

				//Log(LOG_INFO) << ". . . . qtySuccess_and= " << qtySuccess_and << " required= " << levels.at(levelRequired["noQual"]);
				if (qtySuccess_and >= levels.at(levelRequired["noQual"]))
				{
					//Log(LOG_INFO) << ". . . . . . levels MET - BREAK orCriteria & grant Award w/ career,mission,turn";
					qtySuccess_or += qtySuccess_and;
					//Log(LOG_INFO) << ". . . . . . qtySuccess_or= " << qtySuccess_or;
					break;
				}
			} // orCriteria ^

			//Log(LOG_INFO) << "";
			//Log(LOG_INFO) << ". . . qtySuccess_or= " << qtySuccess_or << " required= " << levels.at(levelRequired["noQual"]);
			if (qtySuccess_or < levels.at(levelRequired["noQual"]))
			{
				//Log(LOG_INFO) << ". . . . orCriteria has NOT been met - go to next Award";
//				iter = 0; // debug.
				++i;
				continue;
			}
		} // criteriaType_end.


		//Log(LOG_INFO) << "";
		//Log(LOG_INFO) << ". do Award";
		showAwardsPostTactical = true;

		if (qualifiers.empty() == true)	// if the Soldier is getting an Award but there are no qualifiers
		{								// the qualifier will be "noQual"
			//Log(LOG_INFO) << ". . GRANT noQual " << awardType;
			qualifiers.push_back("noQual");
		}

		for (std::vector<std::string>::const_iterator
				j = qualifiers.begin();
				j != qualifiers.end();
				++j)
		{
			//Log(LOG_INFO) << ". . . iter Qualifiers= " << *j;
			firstOfType = true;
			for (std::vector<SoldierAward*>::const_iterator
					k = _solAwards.begin();
					k != _solAwards.end();
					++k)
			{
				if ((*k)->getType() == awardType && (*k)->getQualifier() == *j)
				{
					//Log(LOG_INFO) << ". . . . GRANT " << (*j) << " " << awardType;
					firstOfType = false;
					(*k)->addAwardLevel();
					break;
				}
			}

			if (firstOfType == true)
			{
				//Log(LOG_INFO) << ". . . is First of Type";
				_solAwards.push_back(new SoldierAward(awardType, *j));
			}
		}

		//Log(LOG_INFO) << ". recurse Award type -> check for higher level";
	} // award ^

	//Log(LOG_INFO) << "Diary: updateAwards() EXIT w/ post-tactical= " << showAwardsPostTactical;
	return showAwardsPostTactical;
}
							//Log(LOG_INFO) << ". . . . . . . . bType= " << bType;
							//Log(LOG_INFO) << ". . . . . . . . bType Detail= " << bType_array[bType];
							//Log(LOG_INFO) << ". . . . . . . . dType= " << dType;
							//Log(LOG_INFO) << ". . . . . . . . dType Detail= " << dType_array[dType];

							//Log(LOG_INFO) << ". . . . . . . . weapRule Valid= " << (weaponRule != nullptr);
							//Log(LOG_INFO) << ". . . . . . . . loadRule Valid= " << (loadRule   != nullptr);

							//Log(LOG_INFO) << ". . . . . . . . MATCHES:";
							//Log(LOG_INFO) << ". . . . . . . . . rank= " << ((*killstat)->_rank		== *detail);
							//Log(LOG_INFO) << ". . . . . . . . . race= " << ((*killstat)->_race		== *detail);
							//Log(LOG_INFO) << ". . . . . . . . . weap= " << ((*killstat)->_weapon	== *detail);
							//Log(LOG_INFO) << ". . . . . . . . . load= " << ((*killstat)->_load		== *detail);
							//Log(LOG_INFO) << ". . . . . . . . . stat= " << ((*killstat)->getUnitStatusString()	== *detail);
							//Log(LOG_INFO) << ". . . . . . . . . fact= " << ((*killstat)->getUnitFactionString()	== *detail);
							//Log(LOG_INFO) << ". . . . . . . . . bTyp= " << !((weaponRule == nullptr
							//														&& (   bType_array[bType] != "BT_NONE"
							//															|| bType_array[bType] != "BT_END"))
							//													|| (   weaponRule != nullptr
							//														&& weaponRule->getBattleType() != static_cast<BattleType>(bType)));
							//Log(LOG_INFO) << ". . . . . . . . . dTyp= " << !((loadRule == nullptr
							//														&& (   dType_array[dType] != "DT_NONE"
							//															|| dType_array[dType] != "DT_END"))
							//													|| (   loadRule != nullptr
							//														&& loadRule->getDamageType() != static_cast<DamageType>(dType)));

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
 * @return, map of weapons to qty killed
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
 * @return, map of ammos to qty killed
 *
std::map<std::string, int> SoldierDiary::getLoadTotal() const
{
	std::map<std::string, int> ret;
	for(std::vector<BattleUnitKill*>::const_iterator
			i = _killList.begin();
			i != _killList.end();
			++i)
	{
		++ret[(*i)->_load];
	}
	return ret;
} */

/**
 * Gets a list of quantities of tacticals done by Region-type.
 * @param tacticals - reference to a vector of pointers to TacticalStatistics
 * @return, map of regions to tacticals done there
 */
std::map<std::string, int> SoldierDiary::getRegionTotal(const std::vector<TacticalStatistics*>& tacticals) const
{
	std::map<std::string, int> ret;
	for (std::vector<TacticalStatistics*>::const_iterator
			i = tacticals.begin();
			i != tacticals.end();
			++i)
	{
		for (std::vector<int>::const_iterator
				j = _tacIdList.begin();
				j != _tacIdList.end();
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
 * @param tacticals - reference to a vector of pointers to TacticalStatistics
 * @return, map of countries to tacticals done there
 */
std::map<std::string, int> SoldierDiary::getCountryTotal(const std::vector<TacticalStatistics*>& tacticals) const
{
	std::map<std::string, int> ret;
	for (std::vector<TacticalStatistics*>::const_iterator
			i = tacticals.begin();
			i != tacticals.end();
			++i)
	{
		for (std::vector<int>::const_iterator
				j = _tacIdList.begin();
				j != _tacIdList.end();
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
 * @param tacticals - reference to a vector of pointers to TacticalStatistics
 * @return, map of mission-types to qty of
 */
std::map<std::string, int> SoldierDiary::getTypeTotal(const std::vector<TacticalStatistics*>& tacticals) const
{
	std::map<std::string, int> ret;
	for (std::vector<TacticalStatistics*>::const_iterator
			i = tacticals.begin();
			i != tacticals.end();
			++i)
	{
		for (std::vector<int>::const_iterator
				j = _tacIdList.begin();
				j != _tacIdList.end();
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
 * @param tacticals - reference to a vector of pointers to TacticalStatistics
 * @return, map of UFO-types to qty of
 */
std::map<std::string, int> SoldierDiary::getUfoTotal(const std::vector<TacticalStatistics*>& tacticals) const
{
	std::map<std::string, int> ret;
	for (std::vector<TacticalStatistics*>::const_iterator
			i = tacticals.begin();
			i != tacticals.end();
			++i)
	{
		for (std::vector<int>::const_iterator
				j = _tacIdList.begin();
				j != _tacIdList.end();
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
 * @param tacticals - reference to a vector of pointers to TacticalStatistics
 * @return, score of all tacticals engaged
 */
int SoldierDiary::getScoreTotal(const std::vector<TacticalStatistics*>& tacticals) const
{
	int ret (0);
	for (std::vector<TacticalStatistics*>::const_iterator
			i = tacticals.begin();
			i != tacticals.end();
			++i)
	{
		for (std::vector<int>::const_iterator
				j = _tacIdList.begin();
				j != _tacIdList.end();
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
	return _tacIdList.size();
}

/**
 * Gets the current total quantity of successful tacticals.
 * @param tacticals - reference to a vector of pointers to TacticalStatistics
 * @return, qty of successful tacticals
 */
int SoldierDiary::getWinTotal(const std::vector<TacticalStatistics*>& tacticals) const
{
	int ret (0);
	for (std::vector<TacticalStatistics*>::const_iterator
			i = tacticals.begin();
			i != tacticals.end();
			++i)
	{
		for (std::vector<int>::const_iterator
				j = _tacIdList.begin();
				j != _tacIdList.end();
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
 * @return, kia/mia or "" if neither
 */
std::string SoldierDiary::getKiaOrMia() const
{
	if (_kia != 0)
		return "STR_KIA";

	if (_mia != 0)
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

	const RuleItem* itRule;
	for (std::vector<BattleUnitKill*>::const_iterator
			i = _killList.begin();
			i != _killList.end();
			++i)
	{
		if ((*i)->hostileTurn() == true)
		{
			if ((itRule = rules->getItemRule((*i)->_weapon)) == nullptr)
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

	const RuleItem* itRule;
	for (std::vector<BattleUnitKill*>::const_iterator
			i = _killList.begin();
			i != _killList.end();
			++i)
	{
		if ((*i)->hostileTurn() == true)
		{
			if ((itRule = rules->getItemRule((*i)->_weapon)) != nullptr)
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
 * @param tacticals - reference to a vector of pointers to TacticalStatistics
 * @return, total terror missions
 */
int SoldierDiary::getTerrorMissionTotal(const std::vector<TacticalStatistics*>& tacticals) const
{
	int ret (0);
	for (std::vector<TacticalStatistics*>::const_iterator
			i = tacticals.begin();
			i != tacticals.end();
			++i)
	{
		for (std::vector<int>::const_iterator
				j = _tacIdList.begin();
				j != _tacIdList.end();
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
 * @param tacticals - reference to a vector of pointers to TacticalStatistics
 * @return, total night missions
 */
int SoldierDiary::getNightMissionTotal(const std::vector<TacticalStatistics*>& tacticals) const
{
	int ret (0);
	for (std::vector<TacticalStatistics*>::const_iterator
			i = tacticals.begin();
			i != tacticals.end();
			++i)
	{
		for (std::vector<int>::const_iterator
				j = _tacIdList.begin();
				j != _tacIdList.end();
				++j)
		{
			if (*j == (*i)->id
				&& (*i)->shade >= TacticalStatistics::NIGHT_SHADE
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
 * @param tacticals - reference to a vector of pointers to TacticalStatistics
 * @return, total night terror missions
 */
int SoldierDiary::getNightTerrorMissionTotal(const std::vector<TacticalStatistics*>& tacticals) const
{
	int ret (0);
	for (std::vector<TacticalStatistics*>::const_iterator
			i = tacticals.begin();
			i != tacticals.end();
			++i)
	{
		for (std::vector<int>::const_iterator
				j = _tacIdList.begin();
				j != _tacIdList.end();
				++j)
		{
			if (*j == (*i)->id
				&& (*i)->shade >= TacticalStatistics::NIGHT_SHADE
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
 * @param tacticals - reference to a vector of pointers to TacticalStatistics
 * @return, total base defense tacticals
 */
int SoldierDiary::getBaseDefenseMissionTotal(const std::vector<TacticalStatistics*>& tacticals) const
{
	int ret (0);
	for (std::vector<TacticalStatistics*>::const_iterator
			i = tacticals.begin();
			i != tacticals.end();
			++i)
	{
		for (std::vector<int>::const_iterator
				j = _tacIdList.begin();
				j != _tacIdList.end();
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
 * @param tacticals - reference to a vector of pointers to TacticalStatistics
 * @return, total alien base assaults
 */
int SoldierDiary::getAlienBaseAssaultTotal(const std::vector<TacticalStatistics*>& tacticals) const
{
	int ret (0);
	for (std::vector<TacticalStatistics*>::const_iterator
			i = tacticals.begin();
			i != tacticals.end();
			++i)
	{
		for (std::vector<int>::const_iterator
				j = _tacIdList.begin();
				j != _tacIdList.end();
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
 * @param tacticals - reference to a vector of pointers to TacticalStatistics
 * @return, total important missions
 */
int SoldierDiary::getImportantMissionTotal(const std::vector<TacticalStatistics*>& tacticals) const
{
	int ret (0);
	for (std::vector<TacticalStatistics*>::const_iterator
			i = tacticals.begin();
			i != tacticals.end();
			++i)
	{
		for (std::vector<int>::const_iterator
				j = _tacIdList.begin();
				j != _tacIdList.end();
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
 * @param tacticals - reference to a vector of pointers to TacticalStatistics
 * @return, total valiant crutches
 */
int SoldierDiary::getValiantCruxTotal(const std::vector<TacticalStatistics*>& tacticals) const
{
	int ret (0);
	for (std::vector<TacticalStatistics*>::const_iterator
			i = tacticals.begin();
			i != tacticals.end();
			++i)
	{
		for (std::vector<int>::const_iterator
				j = _tacIdList.begin();
				j != _tacIdList.end();
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
									"STR_MEDAL_ORIGINAL8",
									"noQual",
									false));
}

/**
 * Awards an honorary medal upon joining team-xCom.
 */
void SoldierDiary::awardHonorMedal()
{
	_solAwards.push_back(new SoldierAward(
									"STR_MEDAL_HONOR_CROSS",
									"noQual",
									false));
}

/**
 * Gets a vector of tactical-IDs.
 * @return, reference to a vector of tactical-IDs
 */
const std::vector<int>& SoldierDiary::getTacticalIdList() const
{
	return _tacIdList;
}

/**
 * Gets a vector of all kills in this SoldierDiary.
 * @return, reference to a vector of pointers to BattleUnitKill
 */
const std::vector<BattleUnitKill*>& SoldierDiary::getKills() const
{
	return _killList;
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
		_qualifier(qualifier),
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
	_type		= node["type"]		.as<std::string>(_type);
	_qualifier	= node["qualifier"]	.as<std::string>("noQual");
	_level		= node["level"]		.as<size_t>(_level);

	_recent = false;
}

/**
 * Saves this SoldierAward to a YAML file.
 * @return, YAML node
 */
YAML::Node SoldierAward::save() const
{
	YAML::Node node;

	node["type"]	= _type;
	node["level"]	= static_cast<int>(_level); // WARNING: Save this even if '0'. don't know why tho

	if (_qualifier != "noQual") node["qualifier"] = _qualifier;

	return node;
}

/**
 * Gets this SoldierAward's type.
 * @return, reference to the type
 */
const std::string& SoldierAward::getType() const
{
	return _type;
}

/**
 * Gets this SoldierAward's noun/qualifier.
 * @note "STR_SECTOPOD" or "STR_HEAVY_PLASMA" for examples.
 * @return, reference to the qualifier
 */
const std::string& SoldierAward::getQualifier() const
{
	return _qualifier;
}

/**
 * Gets this SoldierAward's level as an integer.
 * @return, decoration-level as int
 */
size_t SoldierAward::getAwardLevel() const
{
	return _level;
}

/**
 * Adds a level to this SoldierAward.
 */
void SoldierAward::addAwardLevel()
{
	++_level;
	_recent = true;
}

/**
 * Gets this SoldierAward's level.
 * @param skip - a quantity that accounts for repeated levels
 * @return, level-string
 */
const std::string SoldierAward::GetLevelString(size_t skip) const
{
	std::ostringstream oststr;
	oststr << "STR_AWARD_" << _level - skip;
	return oststr.str();
}

/**
 * Gets this SoldierAward's grade.
 * @note Represents the color: none,bronze,silver,gold.
 * @return, grade-string
 */
const std::string SoldierAward::getGradeString() const
{
	std::ostringstream oststr;
	oststr << "STR_AWARD_GRADE_" << _level;
	return oststr.str();
}

/**
 * Gets this SoldierAward's class.
 * @note Represents the quantity of stars [0..3].
 * @return, class-string
 */
const std::string SoldierAward::getClassString() const
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

}
