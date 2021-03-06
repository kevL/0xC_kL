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

#ifndef OPENXCOM_RULEALIENRACE_H
#define OPENXCOM_RULEALIENRACE_H

//#include <string>
//#include <vector>

#include <yaml-cpp/yaml.h>


enum AlienRank
{
	AR_HUMAN = -1,	// -1
	AR_COMMANDER,	//  0
	AR_LEADER,		//  1
	AR_ENGINEER,	//  2
	AR_MEDIC,		//  3
	AR_NAVIGATOR,	//  4
	AR_SOLDIER,		//  5
	AR_TERRORIST,	//  6
	AR_TERRORIST2	//  7
};


namespace OpenXcom
{

/**
 * Represents a specific race "family" or a "main race" if you wish.
 * @note Here is defined which ranks it contains and also which accompanying
 * terror units.
 */
class RuleAlienRace
{

private:
	std::string _type;
	std::vector<std::string> _members;


	public:
		/// Creates rules for an RuleAlienRace.
		explicit RuleAlienRace(const std::string& type);
		/// Cleans up the rules for an RuleAlienRace.
		~RuleAlienRace();

		/// Loads the RuleAlienRace rules-data from YAML.
		void load(const YAML::Node& node);

		/// Gets the RuleAlienRace's type.
//		std::string getAlienType() const;
		/// Gets a certain member of the RuleAlienRace family.
		std::string getMember(int rankId) const;
};

}

#endif
