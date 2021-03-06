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

#ifndef OPENXCOM_RULEMUSIC_H
#define OPENXCOM_RULEMUSIC_H

#include <yaml-cpp/yaml.h>


namespace OpenXcom
{

class RuleMusic final
{

private:
	int _midiIndex;

	std::string
		_type,
		_mode;

	std::vector<int> _indexes;
	std::vector<std::string>
		_files,
		_terrains;


	public:
		/// cTor
		RuleMusic();
		/// dTor
		virtual ~RuleMusic();

		/// Loads the data from yaml
		void load(const YAML::Node& node);

		///
		std::string getType() const;
		///
		std::string getMode() const;
		///
		int getMidiIndex() const;
		///
		std::vector<std::string> getMusicalTerrains() const;
		///
		std::vector<std::string> getFiles() const;
		///
		std::vector<int> getIndexes() const;
};

}

#endif
