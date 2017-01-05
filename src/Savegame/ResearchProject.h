/*
 * Copyright 2010-2017 OpenXcom Developers.
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
		_scientists,
		_daysCost,
		_daysSpent;

	const RuleResearch* _resRule;


	public:
		/// Constructor.
		explicit ResearchProject(const RuleResearch* const resRule);
		/// dTor.
		~ResearchProject();

		/// Loads the ResearchProject from YAML.
		void load(const YAML::Node& node);
		/// Saves the ResearchProject to YAML.
		YAML::Node save() const;

		/// Gets the ResearchProject rules.
		const RuleResearch* getRules() const;

		/// Called daily.
		bool stepResearch();

		/// Sets the quantity of scientists assigned to the ResearchProject.
		void setAssignedScientists(int qty);
		/// Gets the quantity of scientists assigned to the ResearchProject.
		int getAssignedScientists() const;

		/// Sets time already spent on the ResearchProject.
		void setSpent(int spent);
		/// Gets time already spent on the ResearchProject.
		int getSpent() const;

		/// Sets time-cost of the ResearchProject.
		void setCost(int cost);
		/// Gets time-cost of the ResearchProject.
		int getCost() const;

		/// Sets the ResearchProject offline.
		void setOffline(bool offline = true);
		/// Gets whether the ResearchProject is offline or not.
		bool getOffline() const;

		/// Gets a string describing current progress.
		std::string getResearchProgress() const;
};

}

#endif
