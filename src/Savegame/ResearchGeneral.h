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

#ifndef OPENXCOM_RESEARCHGENERAL_H
#define OPENXCOM_RESEARCHGENERAL_H

#include <yaml-cpp/yaml.h>


namespace OpenXcom
{

enum ResearchStatus
{
	RG_LOCKED,		// 0
	RG_UNLOCKED,	// 1
	RG_DISCOVERED	// 2
};

class RuleResearch;


/**
 * A ResearchGeneral co-ordinates data of a research-type.
 * @note This class is designed to hold information about Research independently
 * of what separate Bases are doing with their research-projects. And unlike a
 * RuleResearch this data is allowed to change.
 */
class ResearchGeneral
{

private:
	bool _beenSeen;

	std::string _type;

	const RuleResearch* _resRule;

	ResearchStatus _status;


	public:
		/// Constructs a ResearchGeneral.
		ResearchGeneral(
				const RuleResearch* const resRule,
				bool isQuickBattle = false);
		/// dTor.
		~ResearchGeneral();

		/// Gets the research-type.
		const std::string& getType() const;

		/// Gets the research-rule.
		const RuleResearch* getRules() const;

		/// Gets the current status.
		ResearchStatus getStatus() const;
		/// Sets the current status.
		void setStatus(ResearchStatus status);

		/// Sets the associated Ufopaedia entry as seen.
		void setBeenSeen(const bool seen = false);
		/// Checks if the associated Ufopaedia entry has been seen.
		bool hasBeenSeen() const;

		/// Loads the ResearchGeneral from a YAML file.
		void load(const YAML::Node& node);
		/// Saves the ResearchGeneral to a YAML file.
		YAML::Node save() const;
};

}

#endif
