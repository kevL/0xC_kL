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

#ifndef OPENXCOM_RULECITY_H
#define OPENXCOM_RULECITY_H

//#include <string>

#include "../Savegame/Target.h"


namespace OpenXcom
{

class Language;


/**
 * Represents a city of the world.
 * @note Aliens target cities for terror-missions.
 */
class RuleCity final
	:
		public Target
{

private:
	bool _labelTop;
	int _texture;
	size_t _zoomLevel;

	std::string _name;


	public:
		/// Creates a rule for a City.
		RuleCity();
		/// Cleans up the rule.
		~RuleCity();

		/// Loads the rule from YAML.
		void load(const YAML::Node& node) override;

		/// Gets a City's name.
		std::wstring getName(const Language* const lang) const override;
		/// Gets a City's name as a raw string.
		const std::string& getName() const;

		/// Gets a City's globe-marker.
		int getMarker() const override;

		/// Gets the level of zoom that shows a City's name.
		size_t getZoomLevel() const;

		/// Gets if a City's label is above or below its marker.
		bool getLabelTop() const;

		/// Gets the texture of a City for tactical.
		int getTextureId() const;

		/// Blanked function to stop C++ complaints.
		YAML::Node saveId() const override
		{
			YAML::Node node;
			return node;
		}
};

}

#endif
