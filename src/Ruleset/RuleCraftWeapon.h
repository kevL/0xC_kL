/*
 * Copyright 2010-2018 OpenXcom Developers.
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

#ifndef OPENXCOM_RULECRAFTWEAPON_H
#define OPENXCOM_RULECRAFTWEAPON_H

//#include <string>

#include <yaml-cpp/yaml.h>

#include "../Savegame/CraftWeaponProjectile.h"


namespace OpenXcom
{

/**
 * Represents a specific type of CraftWeapon.
 * @note Contains constant info about a CraftWeapon like damage, range,
 * accuracy, items used, etc.
 * @sa CraftWeapon
 */
class RuleCraftWeapon
{

private:
	std::string
		_clip,
		_launcher,
		_type;
	int
		_accuracy,
		_loadCap,
		_power,
		_prjSpeed,
		_range,
		_rearmRate,
		_reloadAggressive,
		_reloadCautious,
		_reloadStandard,
		_sound,
		_sprite;

	CwpType _prjType;


	public:
		/// Creates a blank CraftWeapon ruleset.
		explicit RuleCraftWeapon(const std::string& type);
		/// Cleans up a CraftWeapon ruleset.
		~RuleCraftWeapon();

		/// Loads CraftWeapon data from YAML.
		void load(
				const YAML::Node& node,
				int modIndex);

		/// Gets a CraftWeapon's type.
		std::string getType() const;

		/// Gets a CraftWeapon's sprite.
		int getSprite() const;
		/// Gets a CraftWeapon's sound.
		int getMissileSound() const;
		/// Gets a CraftWeapon's damage.
		int getPower() const;
		/// Gets a CraftWeapon's range.
		int getRange() const;
		/// Gets a CraftWeapon's accuracy.
		int getAccuracy() const;

		/// Gets a CraftWeapon's cautious reload time.
		int getCautiousReload() const;
		/// Gets a CraftWeapon's standard reload time.
		int getStandardReload() const;
		/// Gets a CraftWeapon's aggressive reload time.
		int getAggressiveReload() const;

		/// Gets a CraftWeapon's maximum ammo.
		int getLoadCapacity() const;
		/// Gets a CraftWeapon's rearm rate.
		int getRearmRate() const;

		/// Gets a CraftWeapon's launcher item-type.
		std::string getLauncherType() const;
		/// Gets a CraftWeapon's ammunition item-type.
		std::string getClipType() const;

		/// Gets a CraftWeapon's projectile's type.
		CwpType getProjectileType() const;
		/// Gets a CraftWeapon's projectile speed.
		int getProjectileSpeed() const;
};

}

#endif
