/*
 * Copyright 2010-2015 OpenXcom Developers.
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

#ifndef OPENXCOM_RESEARCHPROJECT_H
#define OPENXCOM_RESEARCHPROJECT_H

#include <yaml-cpp/yaml.h>


namespace OpenXcom
{

class RuleResearch;


/**
 * Represents a ResearchProject.
 * @note Contains information about assigned scientists, time already spent and
 * cost of the project.
 */
class ResearchProject
{

private:
	bool _offline;
	int
		_assigned,
		_cost,
		_spent;

	const RuleResearch* _resRule;


	public:
		/// Constructor.
		ResearchProject(
				const RuleResearch* const resRule,
				int cost = 0);
		/// dTor.
		~ResearchProject();

		/// Called every new day to compute time spent.
		bool stepProject();

		/// Gets the ResearchProject Ruleset.
		const RuleResearch* getRules() const;

		/// Sets the number of scientists assigned to this ResearchProject.
		void setAssignedScientists(const int qty);
		/// Gets the number of scientist assigned to this ResearchProject.
		int getAssignedScientists() const;

		/// Sets time already spent on this ResearchProject.
		void setSpent(const int spent);
		/// Gets time already spent on this ResearchProject.
		int getSpent() const;

		/// Sets time cost of this ResearchProject.
		void setCost(const int cost);
		/// Gets time cost of this ResearchProject.
		int getCost() const;

		/// Sets the project offline.
		void setOffline(const bool offline = true);
		/// Gets whether the project is offline or not.
		bool getOffline() const;

		/// Loads the ResearchProject from YAML.
		void load(const YAML::Node& node);
		/// Saves the ResearchProject to YAML.
		YAML::Node save() const;

		/// Gets a string describing current progress.
		std::string getResearchProgress() const;
};

}

#endif
