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

#include "ExtraSounds.h"


namespace OpenXcom
{

/**
 * Creates a blank set of extra sound data.
 */
ExtraSounds::ExtraSounds()
	:
		_modIndex(0)
{}

/**
 * Cleans up the extra sound set.
 */
ExtraSounds::~ExtraSounds() // virtual. why
{}

/**
 * Loads the extra sound set from YAML.
 * @param node		- reference a YAML node
 * @param modIndex	- the internal index of the associated mod
 */
void ExtraSounds::load(
		const YAML::Node& node,
		int modIndex)
{
	_modIndex = modIndex;
	_sounds = node["files"].as<std::map<int, std::string>>(_sounds);
}

/**
 * Gets the list of sounds defined by this mod.
 * @return, the list of sounds defined by this mod
 */
std::map<int, std::string>* ExtraSounds::getSounds()
{
	return &_sounds;
}

/**
 * Gets the mod index for this external sounds set.
 * @return, the mod index for this external sounds set
 */
int ExtraSounds::getModIndex() const
{
	return _modIndex;
}

}
