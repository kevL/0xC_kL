/*
 * Copyright 2010-2019 OpenXcom Developers.
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

#ifndef OPENXCOM_REGION_H
#define OPENXCOM_REGION_H

//#include <vector>

#include <yaml-cpp/yaml.h>


namespace OpenXcom
{

class RuleRegion;

/**
 * Represents a region of the world.
 * @note Contains variable info about a region like X-Com and alien activity.
 */
class Region
{

private:
	int
		_actAhrs, // aLiens
		_actXhrs; // xCom

	const RuleRegion* _regionRule;

	std::vector<int>
		_actA, // aLiens
		_actX; // xCom


	public:
		/// Creates a Region with the specified rule.
		explicit Region(const RuleRegion* const regionRule);
		/// Cleans up the Region.
		~Region();

		/// Loads the Region from YAML.
		void load(const YAML::Node& node);
		/// Saves the Region to YAML.
		YAML::Node save() const;

		/// Gets the Region's rules.
		const RuleRegion* getRules() const;
		/// Get the Region's type.
		std::string getType() const;

		/// Adds aLien activity in the Region.
		void addActivityAlien(int activity);
		/// Gets aLien activity for the Region.
		std::vector<int>& getActivityAlien();
		/// Adds xCom activity in the Region.
		void addActivityXCom(int activity);
		/// Gets xCom activity for the Region.
		std::vector<int>& getActivityXCom();

		/// Stores last month's counters and starts new counters.
		void newMonth();

		/// Advances recent activity in the Region for GraphsState blink.
		void stepActivity();
		/// Checks recent activity in the Region for GraphsState blink.
		bool checkActivity(bool aLien);
		/// Resets both aLien and xCom activity via GraphsState button.
		void clearActivity();
};

}

#endif
