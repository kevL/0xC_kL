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

#ifndef OPENXCOM_EXTRASPRITES_H
#define OPENXCOM_EXTRASPRITES_H

#include <yaml-cpp/yaml.h>


namespace OpenXcom
{

/**
 * For adding a set of extra-sprite data.
 */
class ExtraSprites final
{

private:
	bool _singleImage;	// ie. is loaded as just a single Surface.
						// is *not* a SurfaceSet or a frame thereof.
	int
		_width,
		_height,
		_modIndex,
		_subX,
		_subY;

	std::map<int, std::string> _sprites;


	public:
		/// Creates a blank external sprite-set.
		ExtraSprites();
		/// Cleans up the external sprite-set.
		virtual ~ExtraSprites();

		/// Loads the data from YAML.
		void load(
				const YAML::Node& node,
				int modIndex);

		/// Gets the list of sprites defined in rulesets.
		std::map<int, std::string>* getSprites();

		/// Gets the width of the surfaces (used for single images and new sprite-sets).
		int getWidth() const;
		/// Gets the height of the surfaces (used for single images and new sprite-sets).
		int getHeight() const;

		/// Checks if this is a single Surface or part of a SurfaceSet..
		bool isSingleImage() const;

		/// Gets the mod-index for this external sprite-set.
		int getModIndex() const;

		/// Gets the x-subdivision.
		int getSubX() const;
		/// Gets the y-subdivision.
		int getSubY() const;
};

}

#endif
