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

#include "ExtraMusic.h"


namespace OpenXcom
{

/**
 * Creates a blank set of extra music data.
 */
ExtraMusic::ExtraMusic()
	:
		_modIndex(0)
{}

/**
 * Cleans up the extra music set.
 */
ExtraMusic::~ExtraMusic() // virtual. why
{}

/**
 * Loads the extra music set from YAML.
 * @param node, YAML node.
 * @param modIndex, The internal index of the associated mod.
 */
void ExtraMusic::load(
		const YAML::Node& node,
		int modIndex)
{
	_modIndex = modIndex;

	_media		= node["media"]		.as<std::string>(_media);
	_terrains	= node["terrain"]	.as<std::vector<std::string>>(_terrains);

	if (node["overrides"])
		_overrides = node["overrides"].as<std::string>(_overrides);

	if (node["extends"])
		_extends = node["extends"].as<std::string>(_extends);
}

/**
 * Gets the mod index for this external music set.
 * @return, The mod index for this external music set.
 */
int ExtraMusic::getModIndex() const
{
	return _modIndex;
}

/**
 *
 */
std::string ExtraMusic::getOverridden() const
{
	return _overrides;
}

/**
 *
 */
std::string ExtraMusic::getExtended() const
{
	return _extends;
}

/**
 *
 */
bool ExtraMusic::hasTerrainSpecification() const
{
	return (_terrains.empty() == false);
}

/**
 *
 */
std::vector<std::string> ExtraMusic::getTerrains() const
{
	return _terrains;
}

}
