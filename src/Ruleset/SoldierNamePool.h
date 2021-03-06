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

#ifndef OPENXCOM_SOLDIERNAMEPOOL_H
#define OPENXCOM_SOLDIERNAMEPOOL_H

//#include <string>
//#include <vector>

#include "../Savegame/Soldier.h"


namespace OpenXcom
{

/**
 * Pool of soldier names to generate random names.
 * @note Each pool contains a set of first names (male or female) and last names.
 * The first names define the soldier's gender, and are randomly associated
 * with a last name.
 */
class SoldierNamePool
{

private:
	int _totalWeight;
//		_femaleFrequency;
	std::vector<int> _lookWeights;
	std::vector<std::wstring>
		_maleFirst,
		_maleLast,
		_femaleFirst,
		_femaleLast;


	public:
		/// Creates a blank pool.
		SoldierNamePool();
		/// Cleans up the pool.
		~SoldierNamePool();

		/// Loads the pool from YAML.
		void load(const std::string& file);

		/// Generates a new name from the pool.
		std::wstring genName(SoldierGender* const gender) const;
//				int femaleFrequency) const;
		/// Generates an integer representing the index of the soldier's look when passed the maximum index value.
		size_t genLook(const size_t qtyLooks);
};

}

#endif
