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

#include "SoundDefinition.h"


namespace OpenXcom
{

/**
 * cTor.
 * @param type - reference to the type
 */
SoundDefinition::SoundDefinition(const std::string& type)
	:
		_type(type)
{}

/**
 * dTor.
 */
SoundDefinition::~SoundDefinition()
{}

/**
 *
 * @param node - reference a YAML node
 */
void SoundDefinition::load(const YAML::Node& node)
{
	for (YAML::const_iterator
			i = node["soundRanges"].begin();
			i != node["soundRanges"].end();
			++i)
	{
//		std::pair<int, int> range = (*i).as<std::pair<int, int>>(std::make_pair<int, int>(0,0)); // <- that should crap-out on c++11
		std::pair<int, int> range = (*i).as<std::pair<int, int>>(std::make_pair(0,0));
		for (int
				j = range.first;
				j <= range.second;
				++j)
		{
			_soundList.push_back(j);
		}
	}

	for (YAML::const_iterator
			i = node["sounds"].begin();
			i != node["sounds"].end();
			++i)
	{
		_soundList.push_back((*i).as<int>(-1));
	}

	_catFile = node["file"].as<std::string>(_catFile);
}

/**
 *
 * @return, reference to a vector of ints
 */
const std::vector<int>& SoundDefinition::getSoundList() const
{
	return _soundList;
}

/**
 *
 * @return, string
 */
std::string SoundDefinition::getCatFile() const
{
	return _catFile;
}

}
