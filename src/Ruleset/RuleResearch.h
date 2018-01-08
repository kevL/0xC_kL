/*
 * Copyright 2010-2018 OpenXcom Developers.
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
 * Represents a research-rule / research-project and is the basis for a
 * ResearchGeneral-object.
 *
 * @note '_requisite' is the list of RuleResearch's that unlock the rule once
 * all are discovered.
 *
 * @note '_requested' is the list of RuleResearch's that are immediately
 * unlocked even if not all requisites of the latter have been discovered.
 *
 * @note '_required' prevents requisites and/or requests from succeeding if any
 * are not discovered. That is, '_required' is the list of RuleResearch's that
 * *absolutely must already be discovered* (prior to requisite or requesting)
 * for its rule to ever be unlocked. Hint: required-research shall never be used
 * to actively unlock a rule.
 *
 * @note A rule that requests a RuleResearch to unlock does not have to be
 * listed as a requisite of the latter rule. The request is alone enough to
 * unlock the latter rule as long as all the latter's required-research has
 * already been satisified. The preferred method is to place a request rather
 * than to specify the rule as a requisite.
 *
 * @note Therefore a list of requisites can unlock a Rule or a request can
 * unlock that rule.
 *
 * @note Care must be taken so that a rule that requests another rule, or that
 * is a requisite of another rule, remains available for research if/when the
 * latter rule does not get unlocked due to its required-research.
 *
 * @note Fake ResearchProjects: A RuleResearch is fake one if its cost is 0.
 * They are used to to create check points in the dependency tree. For example
 * if there is a Research E which needs either A & B or C & D two fake research-
 * rules can be created:
 *		- F which needs A & B
 *		- G which needs C & D
 *		- then either F or G can be used to unlock E.
 *
 * @note To unlock a RuleResearch at the start of play put "STR_UNLOCKED" as a
 * requisite of the rule.
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
		_getOneFree,
		_requested,
		_required,
		_requisite;


	public:
		/// cTor.
		explicit RuleResearch(const std::string& type);
		/// dTor.
		~RuleResearch();

		/// Loads the RuleResearch from YAML.
		void load(
				const YAML::Node& node,
				int listOrder);

		/// Gets the RuleResearch type.
		const std::string& getType() const;

		/// Gets time needed to discover the RuleResearch.
		int getCost() const;

		/// Gets the points earned for discovering the RuleResearch.
		int getPoints() const;

		/// Gets the requisite-research for the RuleResearch.
		const std::vector<std::string>& getRequisiteResearch() const;

		/// Gets the required-research for the RuleResearch.
		const std::vector<std::string>& getRequiredResearch() const;

		/// Gets the list of research-types requested when the RuleResearch is discovered.
		const std::vector<std::string>& getRequestedResearch() const;

		/// Gets a list of research-types granted randomly for free by the RuleResearch.
		const std::vector<std::string>& getGetOneFree() const;

		/// Gets an alternate look-up string for the Ufopaedia.
		const std::string& getUfopaediaEntry() const;

		/// Checks if the RuleResearch needs a corresponding item for research.
		bool needsItem() const;
		/// Checks if the RuleResearch consumes a corresponding item when research completes.
		bool destroyItem() const;

		/// Checks if the RuleResearch should be flagged as seen by default.
		bool isMarkSeen() const;

		/// Gets the list-priority for the RuleResearch.
		int getListOrder() const;
};

}

#endif
