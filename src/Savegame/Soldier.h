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

#ifndef OPENXCOM_SOLDIER_H
#define OPENXCOM_SOLDIER_H

//#include <string>
//#include <yaml-cpp/yaml.h>

//#include "../Ruleset/StatString.h"
#include "../Ruleset/RuleUnit.h"


namespace OpenXcom
{

enum SoldierRank
{
	RANK_ROOKIE,	// 0
	RANK_SQUADDIE,	// 1
	RANK_SERGEANT,	// 2
	RANK_CAPTAIN,	// 3
	RANK_COLONEL,	// 4
	RANK_COMMANDER	// 5
};

enum SoldierGender
{
	GENDER_MALE,	// 0
	GENDER_FEMALE	// 1
};

enum SoldierLook
{
	LOOK_BLONDE,	// 0
	LOOK_BROWNHAIR,	// 1
	LOOK_ORIENTAL,	// 2
	LOOK_AFRICAN	// 3
};


class Craft;
class Language;
class RuleArmor;
class Ruleset;
class RuleSoldier;
class SavedGame;
class SoldierDiary;
class SoldierLayout;
class SoldierNamePool;


/**
 * Represents a soldier hired by the player.
 * @note Soldiers have a wide variety of stats that affect their performance
 * during battles.
 */
class Soldier // no copy cTor.
{

private:
	bool
		_psiTraining,
		_recentlyPromoted;
	int
		_id,
		_kills,
		_missions,
		_recovery;

	std::wstring _name;
//		_statString;

	Craft* _craft;
	const RuleArmor* _armorRule;
	const RuleSoldier* _solRule;
	SoldierDiary* _diary;

	SoldierGender _gender;
	SoldierLook _look;
	SoldierRank _rank;

	UnitStats
		_initialStats,
		_currentStats;

	std::vector<SoldierLayout*> _layout;


	public:
		/// Creates a new soldier.
		Soldier(
				const RuleSoldier* const solRule,
				const RuleArmor* const armorRule = nullptr,
				const std::vector<SoldierNamePool*>* const names = nullptr,
				int id = 0);
		/// Cleans up the soldier.
		~Soldier();

		/// Loads the soldier from YAML.
		void load(
				const YAML::Node& node,
				const Ruleset* const rules);
		/// Saves the soldier to YAML.
		YAML::Node save() const;

		/// Gets soldier rules.
		const RuleSoldier* getRules() const;

		/// Gets a pointer to initial stats.
		const UnitStats* getInitStats();
		/// Gets a pointer to current stats.
		UnitStats* getCurrentStats();

		/// Gets the soldier's unique ID.
		int getId() const;

		/// Gets the soldier's name.
		std::wstring getName() const;
//		std::wstring getName(bool statstring = false, size_t maxLength = 20) const;
		/// Sets the soldier's name.
		void setName(const std::wstring& name);

		/// Gets the soldier's craft.
		Craft* getCraft() const;
		/// Sets the soldier's craft.
		void setCraft(Craft* const craft = nullptr);
		/// Gets the soldier's craft string.
		std::wstring getCraftString(const Language* const lang) const;

		/// Gets a string version of the soldier's rank.
		std::string getRankString() const;
		/// Gets a sprite version of the soldier's rank.
		int getRankSprite() const;
		/// Gets the soldier's rank.
		SoldierRank getRank() const;
		/// Increase the soldier's military rank.
		void promoteRank();

		/// Adds kills and a mission to this Soldier's stats.
		void postTactical(int kills);
		/// Gets the soldier's missions.
		int getMissions() const;
		/// Gets the soldier's kills.
		int getKills() const;

		/// Gets the soldier's gender.
		SoldierGender getGender() const;
		/// Gets the soldier's look.
		SoldierLook getLook() const;

		/// Get whether the unit was recently promoted.
		bool isPromoted();

		/// Gets the soldier armor.
		const RuleArmor* getArmor() const;
		/// Sets the soldier armor.
		void setArmor(RuleArmor* const armor);

		/// Gets the soldier's wound recovery time.
		int getSickbay() const;
		/// Sets the soldier's wound recovery time.
		void setRecovery(int recovery);
		/// Gets a soldier's wounds as a percent.
		int getRecoveryPct() const;
		/// Heals wound recoveries.
		void heal();

		/// Gets the soldier's equipment-layout.
		std::vector<SoldierLayout*>* getLayout();

		/// Trains a soldier's psionic abilities.
		bool trainPsiDay();
		/// Returns whether the unit is in psi training or not
		bool inPsiTraining() const;
		/// Sets the psi training status
		void togglePsiTraining();

		/// Kills the soldier and sends it to the dead soldiers' bin.
		void die(SavedGame* const gameSave);

		/// Gets the soldier's diary.
		SoldierDiary* getDiary() const;

		/// Calculates a statString.
//		void calcStatString(const std::vector<StatString*>& statStrings, bool psiStrengthEval);
		/// Automatically renames the soldier according to his/her current statistics.
		void autoStat();
};

}

#endif
