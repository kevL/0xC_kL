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
		_time;

	std::string
		_category,
		_type;

	std::vector<std::string> _required;

	std::map<std::string, int>
		_producedItems,
		_requiredItems;


	public:
		/// Creates a new manufacture.
		explicit RuleManufacture(const std::string& type);

		/// Loads the manufacture from YAML.
		void load(
				const YAML::Node& node,
				int listOrder);

		/// Gets the manufacture type.
		const std::string& getType() const;
		/// Gets the manufacture category.
		const std::string& getCategory() const;

		/// Gets if this is a Craft-type.
		bool isCraft() const;

		/// Gets the manufacture's requirements.
		const std::vector<std::string>& getRequirements() const;

		/// Gets the required workshop space.
		int getSpaceRequired() const;
		/// Gets the time required to manufacture one object.
		int getManufactureTime() const;
		/// Gets the cost of manufacturing one object.
		int getManufactureCost() const;

		/// Gets the list of items required to manufacture one object.
		const std::map<std::string, int>& getRequiredItems() const;
		/// Gets the list of items produced by completing one object of this project.
		const std::map<std::string, int>& getProducedItems() const;

		/// Gets the list weight for this manufacture item.
		int getListOrder() const;
};

}

#endif
