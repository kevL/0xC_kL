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

#ifndef OPENXCOM_BASEFACILITY_H
#define OPENXCOM_BASEFACILITY_H

#include <yaml-cpp/yaml.h>


namespace OpenXcom
{

class Base;
class Craft;
class RuleBaseFacility;
class Ruleset;


/**
 * Represents a base facility placed in a base.
 * @note Contains variable info about a facility like position and build time.
 * @sa RuleBaseFacility
 */
class BaseFacility
{

private:
	int
		_buildTime,
		_x,
		_y;

	Base* _base;
	const Craft* _craft;
	const RuleBaseFacility* _facRule;


	public:
		/// Creates a BaseFacility of the specified type.
		BaseFacility(
				const RuleBaseFacility* const facRule,
				Base* const base);
		/// Cleans up the BaseFacility.
		~BaseFacility();

		/// Loads the BaseFacility from YAML.
		void load(const YAML::Node& node);
		/// Saves the BaseFacility to YAML.
		YAML::Node save() const;

		/// Gets the BaseFacility's ruleset.
		const RuleBaseFacility* getRules() const;

		/// Gets the the BaseFacility's x-position.
		int getX() const;
		/// Sets the the BaseFacility's x-position.
		void setX(int x);
		/// Gets the the BaseFacility's y-position.
		int getY() const;
		/// Sets the BaseFacility's y-position.
		void setY(int y);

		/// Gets the BaseFacility's construction time.
		int getBuildTime() const;
		/// Sets the BaseFacility's construction time.
		void setBuildTime(int buildTime);

		/// Builds up the BaseFacility.
		bool buildFacility();
		/// Gets if the BaseFacility has finished building.
		bool buildFinished() const;

		/// Checks if the BaseFacility is currently in use.
		bool inUse() const;

		/// Gets the Craft used for drawing the BaseFacility if any.
		const Craft* getCraft() const;
		/// Sets a Craft used for drawing the BaseFacility.
		void setCraft(const Craft* const craft = nullptr);
};

}

#endif
