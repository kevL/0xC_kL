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

#ifndef OPENXCOM_RULERESEARCH_H
#define OPENXCOM_RULERESEARCH_H

//#include <string>
//#include <vector>

#include <yaml-cpp/yaml.h>


namespace OpenXcom
{

/**
 * Represents one research project.
 * @note Dependency is the list of RuleResearch's which must be discovered before
 * a RuleResearch becomes available.
 * @note Unlocks are used to immediately unlock a RuleResearch even if not all
 * its prerequisites have been researched.
 * @note Fake ResearchProjects: A RuleResearch is fake one if its cost is 0.
 * They are used to to create check points in the dependency tree. For example
 * if there is a Research E which needs either A & B or C & D two fake research
 * projects can be created:
 *		- F which needs A & B
 *		- G which needs C & D
 *		- both F and G can unlock E.
 */
class RuleResearch
{

private:
	std::string
		_uPed,
		_type;
	bool
		_destroyItem,
		_markSeen,
		_needsItem;
	int
		_cost,
		_listOrder,
		_points;

	std::vector<std::string>
		_forced,
		_getOneFree,
		_prerequisite,
		_reqResearch;


	public:
		/// cTor.
		explicit RuleResearch(const std::string& type);
		/// dTor.
		~RuleResearch();

		/// Loads the research from YAML.
		void load(
				const YAML::Node& node,
				int listOrder);

		/// Gets the research type.
		const std::string& getType() const;

		/// Gets time needed to discover this RuleResearch.
		int getCost() const;

		/// Gets the points earned for discovering this RuleResearch.
		int getPoints() const;

		/// Gets the prerequisites for this RuleResearch.
		const std::vector<std::string>& getPrerequisites() const;

		/// Gets the absolute requirements for this RuleResearch.
		const std::vector<std::string>& getRequiredResearch() const;

		/// Gets a list of research-types unlocked by this RuleResearch.
		const std::vector<std::string>& getForcedResearch() const;

		/// Gets a list of research-types granted randomly for free by this RuleResearch.
		const std::vector<std::string>& getGetOneFree() const;

		/// Gets an alternate look-up string for the Ufopaedia.
		const std::string& getUfopaediaEntry() const;

		/// Checks if this RuleResearch needs a corresponding item for research.
		bool needsItem() const;
		/// Checks if this RuleResearch consumes its corresponding item when research completes.
		bool destroyItem() const;

		/// Gets if this RuleResearch should be flagged as seen by default.
		bool getMarkSeen() const;

		/// Gets the list-priority for this RuleResearch.
		int getListOrder() const;
};

}

#endif
