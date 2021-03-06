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

#include "ExtraSprites.h"


namespace OpenXcom
{

/**
 * Creates a blank set of extra-sprite data.
 */
ExtraSprites::ExtraSprites()
	:
		_width(320),
		_height(200),
		_subX(0),
		_subY(0),
		_singleImage(false),
		_modIndex(0)
{}

/**
 * Cleans up the extra sprite-set.
 */
ExtraSprites::~ExtraSprites() // virtual. why.'cause.
{}

/**
 * Loads the extra sprite-set from YAML.
 * @param node		- reference a YAML node
 * @param modIndex	- the internal index of the associated mod
 */
void ExtraSprites::load(
		const YAML::Node& node,
		int modIndex)
{
	_modIndex = modIndex;

	_sprites		= node["files"]			.as<std::map<int, std::string>>(_sprites);
	_width			= node["width"]			.as<int>(_width);
	_height			= node["height"]		.as<int>(_height);
	_subX			= node["subX"]			.as<int>(_subX);
	_subY			= node["subY"]			.as<int>(_subY);
	_singleImage	= node["singleImage"]	.as<bool>(_singleImage);
}

/**
 * Gets the list of sprites defined by this rule.
 * @return, pointer to the list of sprites
 */
std::map<int, std::string>* ExtraSprites::getSprites()
{
	return &_sprites;
}

/**
 * Gets the width of the surfaces (used for single images and new sprite-sets).
 * @return, the width of the surfaces
 */
int ExtraSprites::getWidth() const
{
	return _width;
}

/**
 * Gets the height of the surfaces (used for single images and new sprite-sets).
 * @return, the height of the surfaces
 */
int ExtraSprites::getHeight() const
{
	return _height;
}

/**
 * Checks if this is a single Surface or part of a SurfaceSet.
 * @return, true if this is a single surface
 */
bool ExtraSprites::isSingleImage() const
{
	return _singleImage;
}

/**
 * Gets the mod-index for this external sprite-set.
 * @return, the mod-index
 */
int ExtraSprites::getModIndex() const
{
	return _modIndex;
}

/**
 * Gets the x-subdivision.
 * @return, the x-subdivision
 */
int ExtraSprites::getSubX() const
{
	return _subX;
}

/**
 * Gets the y-subdivision.
 * @return, the y-subdivision
 */
int ExtraSprites::getSubY() const
{
	return _subY;
}

}
