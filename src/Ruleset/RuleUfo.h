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

#ifndef OPENXCOM_RULEUFO_H
#define OPENXCOM_RULEUFO_H

//#include <string>

#include <yaml-cpp/yaml.h>


namespace OpenXcom
{

enum UfoSizeType
{
	UFO_VERYSMALL,	// 0
	UFO_SMALL,		// 1
	UFO_MEDIUM,		// 2
	UFO_LARGE,		// 3
	UFO_VERYLARGE	// 4
};

class Ruleset;
class RuleTerrain;


/**
 * Represents a specific type of UFO.
 * @note Contains constant info about a UFO like speed, weapons, scores, etc.
 * @sa Ufo
 */
class RuleUfo
{

private:
	int
		_accel,
		_damageMax,
		_escape,
		_marker,
		_power,
		_range,
		_rangeRecon,
		_reload,
		_score,
		_scoreAct,
		_speedMax,
		_sprite;
	std::string
		_spriteAlt,
		_size,
		_type;

	UfoSizeType _sizeType;

	RuleTerrain* _terrainRule;


	public:
		static const char* stSize[5u];

		/// Creates a blank UFO ruleset.
		explicit RuleUfo(const std::string& type);
		/// Cleans up the UFO ruleset.
		~RuleUfo();

		/// Loads UFO data from YAML.
		void load(
				const YAML::Node& node,
				Ruleset* const rules);

		/// Gets the UFO's type.
		const std::string& getType() const;
		/// Gets the UFO's size.
		const std::string& getSize() const;
		/// Gets the size-type of this type of UFO.
		UfoSizeType getSizeType() const;

		/// Gets the UFO's radius.
		size_t getRadius() const;
		/// Gets the UFO's sprite.
		int getSprite() const;
		/// Gets the UFO's globe-marker.
		int getMarker() const;

		/// Gets the UFO's maximum damage.
		int getMaxDamage() const;
		/// Gets the UFO's maximum speed.
		int getMaxSpeed() const;
		/// Gets the UFO's acceleration.
		int getAcceleration() const;

		/// Gets the UFO's weapon power.
		int getWeaponPower() const;
		/// Gets the UFO's weapon range.
		int getWeaponRange() const;

		/// Gets the UFO's score.
		int getScore() const;
		/// Gets the UFO's activity score.
		int getActivityScore() const;

		/// Gets the tactical-terrain-data-rule for the type of UFO.
		RuleTerrain* getTacticalTerrainData() const;

		/// Gets the reload time of the UFO's weapon.
		int getWeaponReload() const;
		/// Gets the UFO's escape time.
		int getEscape() const;

		/// Gets the string-ID of the Surface that represents this UFO.
		std::string getSpriteString() const;
		/// Gets the UFO's radar range.
		int getRangeRecon() const;
};

}

#endif
