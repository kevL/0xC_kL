/*
 * Copyright 2010-2017 OpenXcom Developers.
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

#ifndef OPENXCOM_RULEINTERFACE_H
#define OPENXCOM_RULEINTERFACE_H

//#include <string>
//#include <map>

#include <yaml-cpp/yaml.h>

#include "../Engine/Palette.h"


namespace OpenXcom
{

struct Element
{
	int
		x,y,
		w,h;
//	Uint8 // <- fucko-fuclo'd
	int
		color,
		color2,
		border;
};


class RuleInterface
{

private:
	std::string
		_palette,
		_parent,
		_type;
	std::map<std::string, Element> _elements;

	PaletteType _palType;

	/// Converts the palette from a string to PaletteType.
	static PaletteType convertToPaletteType(const std::string& palette);


	public:
		/// Consructor.
		explicit RuleInterface(const std::string& type);
		/// Destructor.
		~RuleInterface();

		/// Loads from YAML.
		void load(const YAML::Node& node);

		/// Gets an element.
		const Element* getElement(const std::string& id) const;

		/// Gets the PaletteType.
		PaletteType getPalette() const;

		/// Gets parent interface rule.
		const std::string& getParent() const;
};

}

#endif
