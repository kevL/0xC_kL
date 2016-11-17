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

#ifndef OPENXCOM_RULEAWARD_H
#define OPENXCOM_RULEAWARD_H

#include <yaml-cpp/yaml.h>


namespace OpenXcom
{

/**
 * Represents a specific type of Award.
 * @note Contains constant info about the Award.
 */
class RuleAward
{

private:
	int _sprite;
	std::string
		_description,
		_descGeneric;

	std::map<std::string, std::vector<int>> _criteria;
	std::vector<std::vector<std::pair<int, std::vector<std::string>>>> _killCriteria;


	public:
		/// Creates a RuleAward.
		RuleAward();
		/// Cleans up the RuleAward.
		~RuleAward();

		/// Loads the RuleAward data from YAML.
		void load(const YAML::Node& node);

		/// Gets the RuleAward's description.
		const std::string& getDescription() const;
		/// Gets the RuleAward's generic description.
		const std::string& getDescriptionGeneric() const;

		/// Gets the RuleAward's sprite.
		int getSprite() const;

		/// Gets the RuleAward's criteria.
		const std::map<std::string, std::vector<int>>* getCriteria() const;
		/// Gets the RuleAward's kill-criteria.
		const std::vector<std::vector<std::pair<int, std::vector<std::string>>>>* getKillCriteria() const;
};

}

#endif
