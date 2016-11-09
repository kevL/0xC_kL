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

#ifndef OPENXCOM_RULEMANUFACTURE_H
#define OPENXCOM_RULEMANUFACTURE_H

//#include <map>
//#include <string>

#include <yaml-cpp/yaml.h>


namespace OpenXcom
{

class Ruleset;


/**
 * Represents the information needed to manufacture an object.
 */
class RuleManufacture
{

private:
	bool _isCraft;
	int
		_cost,
		_listOrder,
		_space,
		_hours;

	std::string
		_category,
		_type;

	std::vector<std::string> _reqResearch;

	std::map<std::string, int>
		_partsProduced,
		_partsRequired,
		_reqFacs;


	public:
		/// Creates a new RuleManufacture.
		explicit RuleManufacture(const std::string& type);

		/// Loads the RuleManufacture from YAML.
		void load(
				const YAML::Node& node,
				int listOrder,
				const Ruleset* const rules);

		/// Gets the RuleManufacture's type.
		const std::string& getType() const;
		/// Gets the RuleManufacture's category.
		const std::string& getCategory() const;

		/// Checks if the RuleManufacture produces a Craft.
		bool isCraft() const;

		/// Gets required workshop-space.
		int getSpaceRequired() const;
		/// Gets time required to produce one object.
		int getManufactureTime() const;
		/// Gets cost to produce one object.
		int getManufactureCost() const;

		/// Gets the RuleManufacture's required-research.
		const std::vector<std::string>& getRequiredResearch() const;
		/// Gets the list of BaseFacilities required to produce the products.
		const std::map<std::string, int>& getRequiredFacilities() const;

		/// Gets the list of parts required for one iteration of the project.
		const std::map<std::string, int>& getPartsRequired() const;
		/// Gets the list of parts manufactured by one iteration of the project.
		const std::map<std::string, int>& getPartsProduced() const;

		/// Gets the list-weight for the RuleManufacture.
		int getListOrder() const;
};

}

#endif
