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

#ifndef OPENXCOM_SOLDIER_H
#define OPENXCOM_SOLDIER_H

//#include <string>
//#include <yaml-cpp/yaml.h>

#include <SDL/SDL.h>

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


class Base;
class Craft;
class Language;
class RuleArmor;
class Ruleset;
class RuleSoldier;
class SavedGame;
class SoldierDiary;
class SoldierLayout;
//class SoldierNamePool;


/**
 * Represents a Soldier hired by the player.
 * @note Soldiers have a wide variety of stats that affect their performance
 * during battles.
 */
class Soldier
{

private:
	bool
		_isQuickBattle,
		_psiTraining,
		_recentlyPromoted;
	int
		_id,
		_kills,
		_missions,
		_recovery;

	std::wstring _label;
//		_statString;

	Craft* _craft;
	const RuleArmor* _arRule;
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
		/// Creates a Soldier from scratch.
		Soldier(
				const RuleSoldier* const solRule,
				const RuleArmor* const arRule,
//				const std::vector<SoldierNamePool*>* const names = nullptr,
				int id);
		/// Creates a Soldier to be filled w/ YAML data.
		Soldier(const RuleSoldier* const solRule);
		/// Cleans up the Soldier.
		~Soldier();

		/// Loads the Soldier from YAML.
		void load(
				const YAML::Node& node,
				const Ruleset* const rules);
		/// Saves the Soldier to YAML.
		YAML::Node save() const;

		/// Gets Soldier rules.
		const RuleSoldier* getRules() const;

		/// Gets a pointer to initial stats.
		const UnitStats* getInitStats();
		/// Gets a pointer to current stats.
		UnitStats* getCurrentStats();

		/// Gets the Soldier's ID.
		int getId() const;

		/// Sets the Soldier's label.
		void setLabel(const std::wstring& label);
		/// Gets the Soldier's label.
		std::wstring getLabel() const;
//		std::wstring getLabel(bool statstring = false, size_t maxLength = 20) const;

		/// Gets the Soldier's Craft.
		Craft* getCraft() const;
		/// Sets the Soldier's Craft.
		void setCraft(
				Craft* const craft = nullptr,
				Base* const base = nullptr,
				bool isQuickBattle = false);
		/// Gets the Soldier's craft-string.
		std::wstring getCraftLabel(const Language* const lang) const;

		/// Gets a string version of the Soldier's rank.
		std::string getRankString() const;
		/// Gets a sprite version of the Soldier's rank.
		int getRankSprite() const;
		/// Gets the Soldier's rank.
		SoldierRank getRank() const;
		/// Increases the Soldier's military rank.
		void promoteRank();
		/// Decreases the Soldier's military rank.
		void demoteRank();

		/// Adds kills/stuns and a mission-count to this Soldier's stats.
		void postTactical(int takedowns);
		/// Gets the Soldier's missions.
		int getMissions() const;
		/// Gets the Soldier's kills.
		int getKills() const;

		/// Gets the Soldier's gender.
		SoldierGender getGender() const;
		/// Gets the Soldier's look.
		SoldierLook getLook() const;

		/// Get whether the unit was recently promoted.
		bool isPromoted();

		/// Gets the Soldier armor.
		const RuleArmor* getArmor() const;
		/// Sets the Soldier armor.
		void setArmor(const RuleArmor* const armor);

		/// Gets the Soldier's wound-recovery time.
		int getSickbay() const;
		/// Sets the Soldier's wound-recovery time.
		void setSickbay(int recovery);
		/// Gets the color for the Soldier's wound-recovery time.
		Uint8 getSickbayColor();
		/// Gets a Soldier's wounds as a percent.
		int getPctWounds() const;
		/// Heals wound recoveries.
		void heal();

		/// Gets the Soldier's equipment-layout.
		std::vector<SoldierLayout*>* getLayout();

		/// Trains the Soldier's psionic abilities.
		bool trainPsiDay();
		/// Gets whether the Soldier is in psi-training or not.
		bool inPsiTraining() const;
		/// Toggles the psi-training status of the Soldier.
		void togglePsiTraining();

		/// Sets the Soldier as a quick-battle soldier.
		void setQuickBattle();

		/// Kills the Soldier and sends him/her to the dead-soldiers' bin.
		void die(SavedGame* const playSave);

		/// Gets the Soldier's diary.
		SoldierDiary* getDiary() const;

		/// Calculates a statString.
//		void calcStatString(const std::vector<StatString*>& statStrings, bool psiStrengthEval);
		/// Automatically renames the Soldier according to his/her current statistics.
		void autoStat();

		/// Gets this Soldier's wage for battles or salary.
		int getSoldierExpense(bool tactical = true) const;
};

}

#endif
