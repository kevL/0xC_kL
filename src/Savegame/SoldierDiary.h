/*
 * Copyright 2010-2017 OpenXcom Developers.
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

//#include <map>	// std::map
//#include <string>	// std::string
//#include <vector>	// std::vector

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
		_qualifier;


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

		/// Loads the SoldierAward from YAML.
		void load(const YAML::Node& node);
		/// Saves the SoldierAward to YAML.
		YAML::Node save() const;

		/// Gets the SoldierAward's type.
		const std::string& getType() const;
		/// Gets the SoldierAward's noun/qualifier.
		const std::string& getQualifier() const;

		/// Gets the SoldierAward's level as an integer.
		size_t getAwardLevel() const;
		/// Adds a level to this SoldierAward.
		void addAwardLevel();

		/// Gets the SoldierAward's level.
		const std::string GetLevelString(size_t skip) const;
		/// Gets the SoldierAward's grade.
		const std::string getGradeString() const;
		/// Gets the SoldierAward's class.
		const std::string getClassString() const;

		/// Gets if the SoldierAward was recently awarded.
		bool isAwardRecent() const;
		/// Clears the SoldierAward's recent-flag.
		void clearRecent();
};


class SoldierDiary
{

private:
	static const size_t
		BATS = 14u,
		DATS = 11u;

	int
		_daysWoundedTotal,
		_totalShotByFriendlyCounter,
		_totalShotFriendlyCounter,
		_loneSurvivorTotal,
		_monthsService,
		_unconsciousTotal,
		_shotAtCounterTotal,
		_hitCounterTotal,
		_ironManTotal,
		_longDistanceHitCounterTotal,
		_lowAccuracyHitCounterTotal,
		_shotsFiredCounterTotal,
		_shotsLandedCounterTotal,
		_shotAtCounter10in1Mission,
		_hitCounter5in1Mission,
		_timesWoundedTotal,
		_allAliensKilledTotal,
		_mediApplicationsTotal,
		_revivedUnitTotal,
		_kia,
		_mia;

	std::vector<int> _tacIdList;
	std::vector<SoldierAward*> _solAwards;
	std::vector<BattleUnitKill*> _killList;


	public:
		/// Creates a SoldierDiary.
		SoldierDiary();
		/// Creates a SoldierDiary and loads its contents from YAML.
//		explicit SoldierDiary(const YAML::Node& node);
		/// Constructs a copy of a SoldierDiary.
		SoldierDiary(const SoldierDiary& copyThat);
		/// Deconstructs the SoldierDiary.
		~SoldierDiary();

		/// Overloads the SoldierDiary's assignment-operator.
		SoldierDiary& operator =(const SoldierDiary& assignThat);

		/// Loads the SoldierDiary from YAML.
		void load(const YAML::Node& node);
		/// Saves the SoldierDiary to YAML.
		YAML::Node save() const;

		/// Updates the SoldierDiary's statistics.
		void postTactical(
				BattleUnitStatistics* const tacstats,
				const TacticalStatistics* const tactical);

		/// Accesses the SoldierAwards currently in the SoldierDiary.
		std::vector<SoldierAward*>& getSoldierAwards();

		/// Manages SoldierAwards and returns true if a medal is awarded.
		bool updateAwards(
				const Ruleset* const rules,
				const std::vector<TacticalStatistics*>& tacticals);

		/// Gets the list of kills, mapped by rank.
		std::map<std::string, int> getAlienRankTotal() const;
		/// Gets the list of kills, mapped by race.
		std::map<std::string, int> getAlienRaceTotal() const;
		/// Gets the list of kills, mapped by weapon used.
		std::map<std::string, int> getWeaponTotal() const;
		/// Gets the list of kills, mapped by weapon load used.
//		std::map<std::string, int> getLoadTotal() const;

		/// Gets the quantities of missions mapped by Region.
		std::map<std::string, int> getRegionTotal(const std::vector<TacticalStatistics*>& tacticals) const;
		/// Gets the quantities of missions mapped by Country.
		std::map<std::string, int> getCountryTotal(const std::vector<TacticalStatistics*>& tacticals) const;
		/// Gets the quantities of missions mapped by tactical-type.
		std::map<std::string, int> getTypeTotal(const std::vector<TacticalStatistics*>& tacticals) const;
		/// Gets the quantities of missions mapped by UFO-type.
		std::map<std::string, int> getUfoTotal(const std::vector<TacticalStatistics*>& tacticals) const;

		/// Gets the total score.
		int getScoreTotal(const std::vector<TacticalStatistics*>& tacticals) const;
		/// Gets the total point-value of aLiens killed or stunned.
		int getPointsTotal() const;
		/// Gets the total quantity of kills.
		int getKillTotal() const;
		/// Gets the total quantity of stuns.
		int getStunTotal() const;
		/// Gets the total quantity of missions.
		size_t getMissionTotal() const;
		/// Gets the total quantity of wins.
		int getWinTotal(const std::vector<TacticalStatistics*>& tacticals) const;
		/// Gets the total quantity of days wounded.
		int getDaysWoundedTotal() const;
		/// Gets whether soldier died or went missing.
		std::string getKiaOrMia() const;
		/// Gets the total quantity of shots fired.
		int getShotsFiredTotal() const;
		/// Gets the total quantity of shots landed on target.
		int getShotsLandedTotal() const;
		/// Gets the soldier's firing-proficiency.
		int getProficiency() const;
		/// Gets trap kills total.
		int getTrapKillTotal(const Ruleset* const rules) const;
		/// Gets reaction kill total.
		int getReactionFireKillTotal(const Ruleset* const rules) const;
		/// Gets the total of terror missions.
		int getTerrorMissionTotal(const std::vector<TacticalStatistics*>& tacticals) const;
		/// Gets the total of night missions.
		int getNightMissionTotal(const std::vector<TacticalStatistics*>& tacticals) const;
		/// Gets the total of night terror missions.
		int getNightTerrorMissionTotal(const std::vector<TacticalStatistics*>& tacticals) const;
		/// Gets the total of base defense missions.
		int getBaseDefenseMissionTotal(const std::vector<TacticalStatistics*>& tacticals) const;
		/// Gets the total of alien base assaults.
		int getAlienBaseAssaultTotal(const std::vector<TacticalStatistics*>& tacticals) const;
		/// Gets the total of important missions.
		int getImportantMissionTotal(const std::vector<TacticalStatistics*>& tacticals) const;
		/// Gets the Valient Crux total.
		int getValiantCruxTotal(const std::vector<TacticalStatistics*>& tacticals) const;

		/// Increments the Soldier's service-time.
		void addMonthlyService();
		/// Gets the total months in service.
		int getMonthsService() const;

		/// Awards a special medal to the original 8 Soldiers.
		void awardOriginalEight();
		/// Awards an honorary medal upon joining team-xCom.
		void awardHonorMedal();

		/// Gets the mission-ID list.
		const std::vector<int>& getTacticalIdList() const;
		/// Gets the kills-list.
		const std::vector<BattleUnitKill*>& getKills() const;
};

}

#endif
