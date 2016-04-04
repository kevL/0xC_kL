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

#ifndef OPENXCOM_SOLDIERDIARY_H
#define OPENXCOM_SOLDIERDIARY_H

//#include <map>
//#include <string>
//#include <vector>

//#include <yaml-cpp/yaml.h>

#include "BattleUnit.h"


namespace OpenXcom
{

struct BattleUnitKill;


/**
 * Each entry is its own Award.
 */
class SoldierAward
{

private:
	bool _recent;
	size_t _level;
	std::string
		_type,
		_qual;


	public:
		/// Creates a SoldierAward of the specified type.
		SoldierAward(
				const std::string& type,
				const std::string& qualifier = "noQual",
				bool recent = true);
		/// Creates a SoldierAward and loads its contents from YAML.
		explicit SoldierAward(const YAML::Node& node);
		/// Cleans up the award.
		~SoldierAward();

		/// Loads the SoldierAward information from YAML.
		void load(const YAML::Node& node);
		/// Saves the SoldierAward information to YAML.
		YAML::Node save() const;

		/// Gets the SoldierAward's type.
		const std::string getType() const;
		/// Gets the SoldierAward's noun/qualifier.
		const std::string getQualifier() const;
		/// Gets the SoldierAward decoration-level type.
		const std::string getClassType(int skip) const;
		/// Gets the SoldierAward's decoration-level-ID.
		size_t getClassLevel() const;
		/// Gets the SoldierAward's decoration-description.
		const std::string getClassDescription() const;
		/// Gets the SoldierAward's decoration-class.
		const std::string getClassDegree() const;

		/// Gets whether the SoldierAward was recently awarded.
		bool isAwardRecent() const;
		/// Clears the SoldierAward's recent-flag.
		void clearRecent();

		/// Increments the decoration-level-ID and flags '_recent'.
		void addClassLevel();
};


class SoldierDiary
{

private:
	static const size_t
		BATS = 13,
		DATS = 11;

	int
		_scoreTotal,
		_pointTotal,
		_killTotal,
		_missionTotal,
		_winTotal,
		_stunTotal,
		_daysWoundedTotal,
		_baseDefenseMissionTotal,
		_totalShotByFriendlyCounter,
		_totalShotFriendlyCounter,
		_loneSurvivorTotal,
		_terrorMissionTotal,
		_nightMissionTotal,
		_nightTerrorMissionTotal,
		_monthsService,
		_unconsciousTotal,
		_shotAtCounterTotal,
		_hitCounterTotal,
		_ironManTotal,
		_importantMissionTotal,
		_longDistanceHitCounterTotal,
		_lowAccuracyHitCounterTotal,
		_shotsFiredCounterTotal,
		_shotsLandedCounterTotal,
		_shotAtCounter10in1Mission,
		_hitCounter5in1Mission,
		_reactionFireTotal,
		_timesWoundedTotal,
		_valiantCruxTotal,
		_KIA,
		_trapKillTotal,
		_alienBaseAssaultTotal,
		_allAliensKilledTotal,
		_mediApplicationsTotal,
		_revivedUnitTotal,
		_MIA;

	std::vector<int> _missionIdList;
	std::vector<SoldierAward*> _solAwards;
	std::vector<BattleUnitKill*> _killList;

	std::map<std::string, int>
		_regionTotal,
		_countryTotal,
		_typeTotal,
		_ufoTotal;

	///
/*	void manageModularCommendations(
			std::map<std::string, int>& nextCommendationLevel,
			std::map<std::string, int>& modularCommendations,
			std::pair<std::string, int> statTotal,
			int criteria); */
	///
/*	void awardCommendation(
			const std::string& type,
			const std::string& noun = "noQual"); */


	public:
		/// Creates a SoldierDiary and loads its contents from YAML.
		explicit SoldierDiary(const YAML::Node& node);
		/// Creates a SoldierDiary.
		SoldierDiary();
		/// Constructs a copy of the SoldierDiary.
		SoldierDiary(const SoldierDiary& copyThis);
		/// Deconstructs the SoldierDiary.
		~SoldierDiary();

		/// Overloads the SoldierDiary's assignment-operator.
		SoldierDiary& operator= (const SoldierDiary& assignThis);

		/// Loads the SoldierDiary from YAML.
		void load(const YAML::Node& node);
		/// Saves the SoldierDiary to YAML.
		YAML::Node save() const;

		/// Updates the SoldierDiary's statistics.
		void updateDiary(
				const BattleUnitStatistics* const unitStatistics,
				MissionStatistics* const missionStatistics,
				const Ruleset* const rules);

		/// Gets the list of kills, mapped by rank.
		std::map<std::string, int> getAlienRankTotal() const;
		/// Gets the list of kills, mapped by race.
		std::map<std::string, int> getAlienRaceTotal() const;
		/// Gets the list of kills, mapped by weapon used.
		std::map<std::string, int> getWeaponTotal() const;
		/// Gets the list of kills, mapped by weapon ammo used.
		std::map<std::string, int> getWeaponAmmoTotal() const;
		/// Gets the list of missions, mapped by region.
		std::map<std::string, int>& getRegionTotal();
		/// Gets the list of missions, mapped by country.
		std::map<std::string, int>& getCountryTotal();
		/// Gets the list of missions, mapped by type.
		std::map<std::string, int>& getTypeTotal();
		/// Gets the list of missions, mapped by UFO.
		std::map<std::string, int>& getUfoTotal();

		/// Gets the total score.
		int getScoreTotal() const;
		/// Gets the total point-value of aLiens killed or stunned.
		int getScorePoints() const;
		/// Gets the total number of kills.
		int getKillTotal() const;
		/// Gets the total number of stuns.
		int getStunTotal() const;
		/// Gets the total number of missions.
		size_t getMissionTotal() const;
		/// Gets the total number of wins.
		int getWinTotal() const;
		/// Gets the total number of days wounded.
		int getDaysWoundedTotal() const;
		/// Gets whether soldier died or went missing.
		std::string getKiaOrMia() const;

		/// Get the soldier's firing-proficiency.
		int getProficiency() const;

		/// Gets the SoldierAwards currently in the SoldierDiary.
		std::vector<SoldierAward*>* getSoldierAwards();

		/// Manages SoldierAwards and returns true if a medal is awarded.
		bool manageAwards(const Ruleset* const rules);

		/// Increments the Soldier's service-time.
		void addMonthlyService();
		/// Gets the total months in service.
		int getMonthsService() const;

		/// Awards a special medal to the original 8 Soldiers.
		void awardOriginalEight();
		/// Awards an honorary medal upon joining team-xCom.
		void awardHonoraryMedal();

		/// Gets the mission-ID list.
		std::vector<int>& getMissionIdList();
		/// Gets the kills-list.
		std::vector<BattleUnitKill*>& getKills();
};

}

#endif
