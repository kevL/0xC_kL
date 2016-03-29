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

#ifndef OPENXCOM_COUNTRY_H
#define OPENXCOM_COUNTRY_H

//#include <vector>

#include <yaml-cpp/yaml.h>


namespace OpenXcom
{

enum SatisfactionType
{
	SAT_SAD,		// 0
	SAT_NEUTRAL,	// 1
	SAT_HAPPY		// 2
};

class RuleCountry;

/**
 * Represents a Country that funds the player.
 * @note Contains variable info about a Country like monthly funding and various
 * activities.
 */
class Country
{

private:
	bool
		_newPact,
		_pact;
	int
		_recentActA,
		_recentActX;

	SatisfactionType _satisfaction;

	const RuleCountry* _countryRule;

	std::vector<int>
		_actA,
		_actX,
		_funding;


	public:
		/// Creates a Country of the specified type.
		Country(
				const RuleCountry* const countryRule,
				bool genFunds = false);
		/// Cleans up the Country.
		~Country();

		/// Loads the Country from YAML.
		void load(const YAML::Node& node);
		/// Saves the Country to YAML.
		YAML::Node save() const;

		/// Gets the Country's ruleset.
		const RuleCountry* getRules() const;
		/// Gets the Country's type.
		const std::string& getType() const;

		/// Gets the Country's funding.
		std::vector<int>& getFunding();
		/// Sets the Country's funding.
		void setFunding(int funding);

		/// Gets the Country's satisfaction-level.
		SatisfactionType getSatisfaction() const;

		/// Adds alien activity in the Country.
		void addActivityAlien(int activity);
		/// Gets alien activity for the Country.
		std::vector<int>& getActivityAlien();
		/// Adds xcom activity in the Country.
		void addActivityXCom(int activity);
		/// Gets xcom activity for the Country.
		std::vector<int>& getActivityXCom();

		/// Stores last month's counters, starts new counters, sets this month's change.
		void newMonth(
				const int totalX,
				const int totalA,
				const int diff);

		/// Signs a pact w/ aLiens at the end of the current month.
		void setRecentPact();
		/// Gets if they're signing a pact w/ aLiens.
		bool getRecentPact() const;
		/// Gets if they already signed a pact w/ aLiens.
		bool getPact() const;
		/// Signs a pact w/ aLiens immediately!1
//		void setPact();

		/// Handles recent alien activity in the Country for GraphsState blink.
		bool recentActivityAlien(
				bool activity = true,
				bool graphs = false);
		/// Handles recent XCOM activity in the Country for GraphsState blink.
		bool recentActivityXCom(
				bool activity = true,
				bool graphs = false);
		/// Resets activity.
		void resetActivity();
};

}

#endif
