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

#ifndef OPENXCOM_CRAFTWEAPON_H
#define OPENXCOM_CRAFTWEAPON_H

#include <yaml-cpp/yaml.h>


namespace OpenXcom
{

class CraftWeaponProjectile;
class RuleCraftWeapon;
class Ruleset;


/**
 * Represents a CraftWeapon equipped by a Craft.
 * @note Contains variable info about a CraftWeapon like ammo.
 * @sa RuleCraftWeapon
 */
class CraftWeapon
{

private:
	bool
		_cantLoad,
		_rearming;
	int _load;

	const RuleCraftWeapon* _cwRule;


	public:
		/// Creates a CraftWeapon of the specified type.
		CraftWeapon(
				const RuleCraftWeapon* const cwRule,
				int load = 0);
		/// Cleans up the CraftWeapon.
		~CraftWeapon();

		/// Loads the CraftWeapon from YAML.
		void load(const YAML::Node& node);
		/// Saves the CraftWeapon to YAML.
		YAML::Node save() const;

		/// Gets the CraftWeapon's ruleset.
		const RuleCraftWeapon* getRules() const;

		/// Gets the CraftWeapon's ammo.
		int getCwLoad() const;
		/// Sets the CraftWeapon's ammo.
		bool setCwLoad(int load);
		/// Gets the CraftWeapon's rearming status.
		bool getRearming() const;
		/// Sets the CraftWeapon's rearming status
		void setRearming(bool rearming = true);
		/// Rearms the CraftWeapon.
		int rearm(
				int baseQty = 0,
				int clipSize = 0);
		/// Gets the CraftWeapon's cantLoad status - no stock in Base Stores.
		bool getCantLoad() const;
		/// Sets the CraftWeapon's cantLoad status - no stock in Base Stores.
		void setCantLoad(bool cantLoad = true);

		/// Fires the CraftWeapon - used during dogfights.
		CraftWeaponProjectile* fire() const;

		/// Gets how many clips are loaded into the CraftWeapon.
		int getClipsLoaded(const Ruleset* const rules) const;
};

}

#endif
