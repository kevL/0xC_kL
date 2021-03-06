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

#include "RuleInterface.h"

//#include <climits>


namespace OpenXcom
{

/**
 * Creates a blank ruleset for a certain type of interface containing an index
 * of elements that make it up.
 * @param type - reference to the string defining the type
 */
RuleInterface::RuleInterface(const std::string& type)
	:
		_type(type),
		_palType(PAL_NONE)
{}

/**
 * dTor.
 */
RuleInterface::~RuleInterface()
{}

/**
 * Loads the elements from a YAML file.
 * @param node - reference a YAML node
 */
void RuleInterface::load(const YAML::Node& node)
{
	_parent		= node["parent"]	.as<std::string>(_parent);
	_palette	= node["palette"]	.as<std::string>(_palette);
	_palType	= convertToPaletteType(_palette);

	for (YAML::const_iterator
			i = node["elements"].begin();
			i != node["elements"].end();
			++i)
	{
		Element element;

		if ((*i)["pos"])
		{
			const std::pair<int,int> pos ((*i)["pos"].as<std::pair<int,int>>());
			element.x = pos.first;
			element.y = pos.second;
		}
		else
			element.x =
			element.y = std::numeric_limits<int>::max();

		if ((*i)["size"])
		{
			const std::pair<int,int> pos ((*i)["size"].as<std::pair<int,int>>());
			element.w = pos.first;
			element.h = pos.second;
		}
		else
			element.w =
			element.h = -1;

		element.color  = (*i)["color"] .as<int>(-1);
		element.color2 = (*i)["color2"].as<int>(-1);
		element.border = (*i)["border"].as<int>(-1);

		const std::string id ((*i)["id"].as<std::string>());
		_elements[id] = element;
	}
}

/**
 * Converts the palette from a string to PaletteType.
 * @param palette - the palette string to convert
 * @return, the corresponding PaletteType (Palette.h)
 */
PaletteType RuleInterface::convertToPaletteType(const std::string& palette) // private/static.
{
	if (palette == "BACKPALS.DAT")		return PAL_BACKPALS;
	if (palette == "PAL_BASESCAPE")		return PAL_BASESCAPE;
	if (palette == "PAL_BATTLEPEDIA")	return PAL_BATTLEPEDIA;
	if (palette == "PAL_BATTLESCAPE")	return PAL_BATTLESCAPE;
	if (palette == "PAL_GEOSCAPE")		return PAL_GEOSCAPE;
	if (palette == "PAL_GRAPHS")		return PAL_GRAPHS;
	if (palette == "PAL_UFOPAEDIA")		return PAL_UFOPAEDIA;

	return PAL_NONE;
}

/**
 * Retrieves info on an element.
 * @param id - reference a string defining the element
 * @return, pointer to Element
 */
const Element* RuleInterface::getElement(const std::string& id) const // <- why i hate const. There is likely NO optimization done despite this.
{
	const std::map<std::string, Element>::const_iterator i (_elements.find(id));
	if (i != _elements.end())
		return &i->second;

	return nullptr;
}

/**
 * Gets this Interface's PaletteType.
 * @return, the PaletteType (Palette.h)
 */
PaletteType RuleInterface::getPalette() const
{
	return _palType;
}

/**
 * Gets this Interface's parent state.
 * @return, reference to parent state
 */
const std::string& RuleInterface::getParent() const
{
	return _parent;
}

}
