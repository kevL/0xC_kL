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

#ifndef OPENXCOM_RULEARMOR_H
#define OPENXCOM_RULEARMOR_H

//#include <string>
//#include <vector>

//#include <yaml-cpp/yaml.h>

#include "MapData.h"
#include "RuleUnit.h"


namespace OpenXcom
{

enum ForcedTorso
{
	TORSO_USE_GENDER,	// 0
	TORSO_ALWAYS_MALE,	// 1
	TORSO_ALWAYS_FEMALE	// 2
};


/**
 * Represents a specific type of Armor.
 * @note Not only soldier armor but also alien armor - aliens can be assigned
 * different types of armor depending on their rank.
 */
class RuleArmor
{
	public:
		static const size_t DAMAGE_TYPES = 10;

private:
	std::string
		_corpseGeo,
//		_specWeapon,
		_spriteSheet,
		_spriteInv,
		_storeItem,
		_type;
	bool
		_canHoldWeapon,
		_constantAnimation,
		_hasInventory,
		_isBasic,
		_isSpacesuit;
	int
		_agility,

		_firePhase,
		_deathFrames,
		_shootFrames,

		_frontArmor,
		_rearArmor,
		_sideArmor,
		_underArmor,

		_drawRoutine,

		_size,
		_weight,

		_colorGroupFace,
		_colorGroupHair,
		_colorGroupRank,
		_colorGroupUtile;

	float _damageModifier[DAMAGE_TYPES];

	ForcedTorso _forcedTorso;
	MovementType _moveType;
	UnitStats _stats;

	std::vector<int>
		_colorFace,
		_colorHair,
		_colorRank,
		_colorUtile;
	std::vector<size_t> _loftSet;
	std::vector<std::string>
		_corpseBattle,
		_units;

	public:
		/// Creates a blank Armor rule.
		explicit RuleArmor(const std::string& type);
		/// Cleans up an Armor's ruleset.
		~RuleArmor();

		/// Loads an Armor's data from YAML.
		void load(const YAML::Node& node);

		/// Gets an Armor's type.
		std::string getType() const;

		/// Gets an Armor's sprite sheet.
		std::string getSpriteSheet() const;
		/// Gets an Armor's inventory sprite.
		std::string getSpriteInventory() const;

		/// Gets the front armor level.
		int getFrontArmor() const;
		/// Gets the side armor level.
		int getSideArmor() const;
		/// Gets the rear armor level.
		int getRearArmor() const;
		/// Gets the under armor level.
		int getUnderArmor() const;

		/// Gets the Geoscape corpse item.
		std::string getCorpseGeoscape() const;
		/// Gets the Battlescape corpse item.
		const std::vector<std::string>& getCorpseBattlescape() const;

		/// Gets the storage item-ID.
		std::string getStoreItem() const;

		/// Gets the special weapon type.
//		std::string getSpecialWeapon() const;

		/// Gets the battlescape drawing-routine for an Armor.
		int getDrawRoutine() const;

		/// Gets whether an Armor allows flight.
		/// DO NOT USE THIS FUNCTION OUTSIDE THE BATTLEUNIT CONSTRUCTOR OR I WILL HUNT YOU DOWN and kiss you. On the lips.
		MovementType getMoveTypeArmor() const;

		/// Gets whether an Armor is for a small or large BattleUnit.
		int getSize() const;

		/// Gets damage modifier.
		float getDamageModifier(DamageType dType) const;

		/// Gets loftSet.
		const std::vector<size_t>& getLoftSet() const;

		/// Gets an Armor's stats.
		const UnitStats* getStats() const;

		/// Gets an Armor's weight.
		int getWeight() const;

		/// Gets number of death frames.
		int getDeathFrames() const;
		/// Gets number of shoot frames.
		int getShootFrames() const;
		/// Gets the frame of an Armor's aiming-animation that first shows a projectile.
		int getFirePhase() const;

		/// Gets if an Armor uses constant animation.
		bool getConstantAnimation() const;

		/// Gets if an Armor can hold a weapon.
		bool getCanHoldWeapon() const;

		/// Checks if an Armor ignores gender (power suit/flying suit).
		ForcedTorso getForcedTorso() const;

		/// Gets face base color.
		int getColorGroupFace() const;
		/// Gets hair base color.
		int getColorGroupHair() const;
		/// Gets utile base color.
		int getColorGroupUtile() const;
		/// Gets rank base color.
		int getColorGroupRank() const;
		/// Gets face color.
		int getColorFace(int id) const;
		/// Gets hair color.
		int getColorHair(int id) const;
		/// Gets utile color.
		int getColorUtile(int id) const;
		/// Gets rank color.
		int getColorRank(int id) const;

		/// Checks if an Armor's inventory can be accessed.
		bool hasInventory() const;

		/// Gets an Armor's applicable BattleUnits.
		const std::vector<std::string>& getUnits() const;

		/// Gets if an Armor is basic (lowest rank, standard issue wear).
		bool isBasic() const;
		/// Gets if an Armor is powered and suitable for Mars.
		bool isSpacesuit() const;

		/// Gets an Armor's agility used to determine stamina expenditure.
		int getAgility() const;
};

}

#endif
