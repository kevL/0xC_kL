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
 * Represents a research-rule / research-project.
 *
 * @note '_prerequisite' is the list of RuleResearch's that unlock the rule once
 * all are discovered.
 *
 * @note '_forced' is the list of RuleResearch's that are immediately unlocked
 * even if not all subsequent prerequisites of the latter have been discovered.
 *
 * @note kL: Therefore a rule that forces a RuleResearch does not have to be
 * listed as a prerequisite of the latter rule. Because the latter will be
 * immediately forced to unlock. Actually, yes it is needed or else a rule could
 * be unlocked by its prerequisites *without the rule that supposedly forced it*
 * open. So the point is that if all prerequisites of a rule also force that
 * rule unlocked then they are not needed as prerequisites also; alternately,
 * any prerequisites that are not also a forcer could safely be specified as
 * required-research instead.
 *
 * @note Ergo, '_prerequisite' is utterly redundant and ought be removed. Except
 * it can't be removed at present since the basic check to see if a rule has
 * been unlocked is a search through its prerequisites -- that should be changed
 * to search through its required-research instead, both at the start of play
 * and when a discovered-research forces it.
 *
 * @note '_reqResearch' is the list of RuleResearch's that *absolutely must
 * already be discovered* (prior to prerequisite or forcing) for its rule to
 * ever be unlocked.
 *
 * @note So care must be taken so that a rule that forces another rule, or that
 * is a prerequisite of another rule, remains available for research if/when
 * the latter rule does not get unlocked due to its required-research.
 *
 * @note Fake ResearchProjects: A RuleResearch is fake one if its cost is 0.
 * They are used to to create check points in the dependency tree. For example
 * if there is a Research E which needs either A & B or C & D two fake research-
 * rules can be created:
 *		- F which needs A & B
 *		- G which needs C & D
 *		- then either F or G can be used to unlock E.
 *
 * @note RuleResearch's that do not have any prerequisites or any required-
 * research are flagged unlocked at the start of play.
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
		_requisite,
		_required,
		_requested;


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

		/// Gets if the RuleResearch should be flagged as seen by default.
		bool getMarkSeen() const;

		/// Gets the list-priority for the RuleResearch.
		int getListOrder() const;
};

}

#endif
