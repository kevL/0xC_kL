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

#ifndef OPENXCOM_SOLDIERDEAD_H
#define OPENXCOM_SOLDIERDEAD_H

//#include <string>

//#include <yaml-cpp/yaml.h>

#include "../Savegame/Soldier.h"


namespace OpenXcom
{

class SoldierDeath;


/**
 * Represents a dead Soldier.
 * @note SoldierDeads have a wide variety of stats that affect our memory of
 * their heroic and/or not so heroic battles.
 */
class SoldierDead
{

private:
	int
		_id,
		_kills,
		_missions;

	std::wstring _label;

	SoldierDeath* _death;
	SoldierDiary* _diary;

	SoldierGender _gender;
	SoldierLook _look;
	SoldierRank _rank;
	UnitStats
		_initialStats,
		_currentStats;


	public:
		/// Creates a SoldierDead. Used for Soldiers dying IG.
		SoldierDead(
				const std::wstring& label,
				const int id,
				const SoldierRank rank,
				const SoldierGender gender,
				const SoldierLook look,
				const int missions,
				const int kills,
				SoldierDeath* const death,
				const UnitStats& initialStats,
				const UnitStats& currentStats,
				SoldierDiary diary); // + Base if I want to...
		/// Creates a SoldierDead without a diary. Used for loading a SaveGame.
		SoldierDead();
		/// Cleans up the SoldierDead.
		~SoldierDead();

		/// Loads the SoldierDead from YAML.
		void load(const YAML::Node& node);
		/// Saves the SoldierDead to YAML.
		YAML::Node save() const;

		/// Gets the SoldierDead's label.
		std::wstring getLabel() const;

		/// Gets a string version of the SoldierDead's rank.
		std::string getRankString() const;
		/// Gets a sprite version of the SoldierDead's rank.
		int getRankSprite() const;
		/// Gets the SoldierDead's rank.
		SoldierRank getRank() const;

		/// Gets the SoldierDead's missions.
		int getMissions() const;
		/// Gets the SoldierDead's kills.
		int getKills() const;

		/// Gets the SoldierDead's gender.
		SoldierGender getGender() const;
		/// Gets the SoldierDead's look.
		SoldierLook getLook() const;

		/// Gets the SoldierDead's ID.
		int getId() const;

		/// Gets the SoldierDead's initial stats.
		UnitStats* getInitStats();
		/// Gets the SoldierDead current stats.
		UnitStats* getCurrentStats();

		/// Gets the SoldierDead's time of death.
		SoldierDeath* getDeath() const;

		/// Gets the SoldierDead's SoldierDiary.
		SoldierDiary* getDiary() const;
};

}

#endif
