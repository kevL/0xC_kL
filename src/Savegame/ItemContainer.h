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

#ifndef OPENXCOM_ITEMCONTAINER_H
#define OPENXCOM_ITEMCONTAINER_H

//#include <map>
//#include <string>

#include <yaml-cpp/yaml.h>


namespace OpenXcom
{

class Ruleset;


/**
 * Represents the items contained by a certain entity like base stores, craft
 * equipment, etc.
 * @note Handles all necessary item management tasks.
 */
class ItemContainer
{

private:
	std::map<std::string, int> _contents;


	public:
		/// Creates an empty item container.
		ItemContainer();
		/// Cleans up the item container.
		~ItemContainer();

		/// Loads the item container from YAML.
		void load(const YAML::Node& node);
		/// Saves the item container to YAML.
		YAML::Node save() const;

		/// Modifies an entry in the Container.
/*		void modifyContents(
				const std::string& type,
				int qty = 1); */
		/// Adds an item to the container.
		void addItem(
				const std::string& type,
				int qty = 1);
		/// Removes an item from the container.
//		std::map<std::string, int>::const_iterator removeItem(
		void removeItem(
				const std::string& type,
				int qty = 1);

		/// Gets an item in the container.
		int getItemQuantity(const std::string& type) const;
		/// Gets the total quantity of items in the container.
		int getTotalQuantity() const;

		/// Checks if the container is empty.
		bool isEmpty();

		/// Gets the total size of items in the container.
		double getTotalSize(const Ruleset* const rule) const;

		/// Gets all the items in the container.
		std::map<std::string, int>* getContents();
};

}

#endif
