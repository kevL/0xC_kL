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

#ifndef OPENXCOM_RESEARCHGENERAL_H
#define OPENXCOM_RESEARCHGENERAL_H

#include <yaml-cpp/yaml.h>


namespace OpenXcom
{

enum ResearchStatus
{
	RS_HIDDEN,		// 0
	RS_AVAILABLE,	// 1
	RS_COMPLETED	// 3
//	RS_PROJECT,		// 4
//	RS_OFFLINE,		// 5
};

class RuleResearch;


/**
 * A ResearchGeneral co-ordinates data of a research-type.
 */
class ResearchGeneral
{

private:
	bool _beenSeen;

	std::string _type;

	const RuleResearch* _resRule;

	ResearchStatus _status;


	public:
		/// Constructor.
		ResearchGeneral(
				const RuleResearch* const resRule,
				bool done = false);
//		void init();
		/// dTor.
		~ResearchGeneral();

		/// Gets the ResearchGeneral type.
		const std::string& getType() const;

		/// Gets the ResearchGeneral research rule.
		const RuleResearch* getRules() const;

		/// Gets the status.
		ResearchStatus getStatus() const;
		/// Sets the status.
		void setStatus(ResearchStatus status);

		/// Sets the Ufopaedia entry as seen.
		void setBeenSeen(const bool seen = false);
		/// Gets whether the Ufopaedia entry has been accessed.
		bool getBeenSeen() const;

		/// Loads the ResearchGeneral from a YAML file.
		void load(const YAML::Node& node);
		/// Saves the ResearchGeneral to a YAML file.
		YAML::Node save() const;
};

}

#endif
