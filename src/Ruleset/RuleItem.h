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

#ifndef OPENXCOM_RULEITEM_H
#define OPENXCOM_RULEITEM_H

//#include <string>
//#include <vector>
//#include <yaml-cpp/yaml.h>

#include "../Battlescape/BattlescapeGame.h" // BattleActionType



namespace OpenXcom
{

const int BIGSPRITE_NONE = -99;	// NOTE: This is set low since several terror-unit weapons
								// use values just below 0 for their bigsprites.
enum DamageType
{
	DT_NONE,	// 0
	DT_AP,		// 1
	DT_IN,		// 2
	DT_HE,		// 3
	DT_LASER,	// 4
	DT_PLASMA,	// 5
	DT_STUN,	// 6
	DT_MELEE,	// 7
	DT_ACID,	// 8
	DT_SMOKE	// 9
};

enum BattleType
{
	BT_NONE,			// 0
	BT_FIREARM,			// 1
	BT_AMMO,			// 2
	BT_MELEE,			// 3
	BT_GRENADE,			// 4 -> includes SmokeGrenade, HE-Satchel, and AlienGrenade (see Ruleset)
	BT_PROXYGRENADE,	// 5
	BT_MEDIKIT,			// 6
	BT_SCANNER,			// 7
	BT_MINDPROBE,		// 8
	BT_PSIAMP,			// 9
	BT_FLARE,			// 10
	BT_CORPSE,			// 11
	BT_FUEL				// 12
};

enum TurretType
{
	TRT_NONE = -1,	// -1
	TRT_CANNON,		//  0 // NOTE: values > -1 are used only for UnitSprite drawing.
	TRT_ROCKET,		//  1
	TRT_LASER,		//  2
	TRT_PLASMA,		//  3
	TRT_BLASTER		//  4
};

enum TilepartSpecial // NOTE: These are set in MCD files (and they pertain to parts not full Tiles).
{
	TILE,					//  0
	START_TILE,				//  1
	UFO_POWER_SOURCE,		//  2
	UFO_NAVIGATION,			//  3
	UFO_CONSTRUCTION,		//  4
	ALIEN_FOOD,				//  5
	ALIEN_REPRODUCTION,		//  6
	ALIEN_ENTERTAINMENT,	//  7
	ALIEN_SURGERY,			//  8
	ALIEN_EXAMINATION,		//  9
	ALIEN_ALLOYS,			// 10
	ALIEN_HABITAT,			// 11
	RUINED_ALLOYS,			// 12
	EXIT_TILE,				// 13
	OBJECTIVE_TILE			// 14
};


class Surface;
class SurfaceSet;


/**
 * Represents a specific type of item.
 * @note Contains constant info about an item like storage size, sell price, etc.
 * @sa Item
 */
class RuleItem
{

private:
	bool
		_arcingShot,
		_canExecute,
		_defusePulse,
		_fixedWeapon,
		_flatRate,
		_liveAlien,
		_LOSRequired,
		_noReaction,
		_noResearch,
		_recover,
		_skillApplied,
		_strengthApplied,
		_twoHanded;
	int
		_armor,
		_attraction,
		_costBuy,
		_costSell,
		_invWidth,
		_invHeight,
		_listOrder,
		_score,
		_transferTime,
		_weight,

		_heal,
		_painKiller,
		_stimulant,
		_energyRecovery,
		_healthRecovery,
		_stunRecovery,
		_woundRecovery,

		_bulletSpeed,
		_explosionSpeed,

		_fireSound,
		_fireSoundHit,
		_fireHitAni,
		_firePower,
		_meleeSound,
		_meleeSoundHit,
		_meleeAni,
		_meleeAniHit,
		_meleePower,
		_shotgunPellets,
		_shotgunPattern,
		_autoKick,
		_autoShots,

		_blastRadius,
		_fullClip,
		_waypoint,

		_accuracyAimed,
		_accuracyAuto,
		_accuracyMelee,
		_accuracySnap,
		_tuAimed,
		_tuAuto,
		_tuDefuse,
		_tuLaunch,
		_tuMelee,
		_tuPrime,
		_tuReload,
		_tuSnap,
		_tuUnload,
		_tuUse,

		_maxRange,
		_aimRange,
		_snapRange,
		_autoRange,
		_minRange,
		_dropoff,

		_bigSprite,
		_floorSprite,
		_handSprite,
		_bulletSprite;

	double
		_size,
		_acuCrouch;


	std::string
		_label, // two types of objects can have the same label
		_type, // but the types are always unique
		_zombieUnit;

	BattleType _bType;
	DamageType _dType;
	TilepartSpecial _specialType;
	TurretType _turretType;

	std::vector<std::string>
		_clipTypes,
		_reqResearch;


	public:
		/// Creates a blank item ruleset.
		explicit RuleItem(const std::string& type);
		/// Cleans up the item ruleset.
		~RuleItem();

		/// Loads item data from YAML.
		void load(
				const YAML::Node& node,
				int modIndex,
				int listIndex);

		/// Gets the item's type.
		const std::string& getType() const;
		/// Gets the item's label.
		const std::string& getLabel() const;

		/// Gets the item's research requirements.
		const std::vector<std::string>& getRequiredResearch() const;

		/// Gets the item's size.
		double getStoreSize() const;

		/// Gets the item's purchase cost.
		int getBuyCost() const;
		/// Gets the item's sale cost.
		int getSellCost() const;
		/// Gets the item's transfer time.
		int getTransferTime() const;

		/// Gets the item's weight.
		int getWeight() const;

		/// Gets the item's reference in BIGOBS.PCK for use in inventory.
		int getBigSprite() const;
		/// Gets the item's reference in FLOOROB.PCK for use in inventory.
		int getFloorSprite() const;
		/// Gets the item's reference in HANDOB.PCK for use in inventory.
		int getHandSprite() const;

		/// Gets if the item is two-handed.
		bool isTwoHanded() const;
		/// Gets if the item is a launcher and if so how many waypoints it can set.
		int isWaypoints() const;
		/// Gets if the item is fixed.
		bool isFixed() const;

		/// Gets the item's bullet sprite reference.
		int getBulletSprite() const;
		/// Gets the item's fire sound.
		int getFireSound() const;
		/// Gets the item's hit sound.
		int getFireHitSound() const;
		/// Gets the item's hit animation.
		int getFireHitAnimation() const;

		/// Gets the item's power.
		int getPower() const;

		/// Gets the item's snapshot accuracy.
		int getAccuracySnap() const;
		/// Gets the item's autoshot accuracy.
		int getAccuracyAuto() const;
		/// Gets the item's aimed shot accuracy.
		int getAccuracyAimed() const;
		/// Gets the item's melee accuracy.
		int getAccuracyMelee() const;

		/// Gets the item's snapshot TU-cost.
		int getSnapTu() const;
		/// Gets the item's autoshot TU-cost.
		int getAutoTu() const;
		/// Gets the item's aimed shot TU-cost.
		int getAimedTu() const;
		/// Gets the item's launch shot TU-cost.
		int getLaunchTu() const;
		/// Gets the item's melee TU-cost.
		int getMeleeTu() const;
		/// Gets the item's use TU.
		int getUseTu() const;
		/// Gets the item's reload TU.
		int getReloadTu() const;
		/// Gets the item's unload TU.
		int getUnloadTu() const;
		/// Gets the item's prime TU.
		int getPrimeTu() const;
		/// Gets the item's defuse TU.
		int getDefuseTu() const;

		/// Gets a list of loadable ammunition that the Item accepts.
		const std::vector<std::string>* getClipTypes() const;

		/// Gets the item's damage-type.
		DamageType getDamageType() const;
		/// Gets the item's battle-type.
		BattleType getBattleType() const;

		/// Gets the item's inventory width.
		int getInventoryWidth() const;
		/// Gets the item's inventory height.
		int getInventoryHeight() const;

		/// Gets the ammo amount.
		int getFullClip() const;

		/// Draws the item's hand sprite onto a Surface.
		void drawHandSprite(
				SurfaceSet* const srt,
				Surface* const srf) const;

		/// Gets the medikit heal quantity.
		int getHealQuantity() const;
		/// Gets the medikit pain killer quantity.
		int getPainKillerQuantity() const;
		/// Gets the medikit stimulant quantity.
		int getStimulantQuantity() const;
		/// Gets the medikit wounds healed per shot.
		int getWoundRecovery() const;
		/// Gets the medikit health recovered per shot.
		int getHealthRecovery() const;
		/// Gets the medikit energy recovered per shot.
		int getEnergyRecovery() const;
		/// Gets the medikit stun recovered per shot.
		int getStunRecovery() const;

		/// Gets the max explosion radius.
		int getExplosionRadius() const;

		/// Gets the recovery-points score.
		int getRecoveryScore() const;

		/// Gets the item's armor.
		int getArmorPoints() const;

		/// Gets the item's recoverability.
		bool isRecoverable() const;

		/// Gets the item's turret-type.
		TurretType getTurretType() const;

		/// Checks if this a live alien.
		bool isLiveAlien() const;

		/// Gets if the rated TU-cost is a flat rate.
		bool isFlatRate() const;

		/// Gets if the weapon fires an arcing shot-trajectory.
		bool isArcingShot() const;

		/// How much do aliens want this thing?
		int getAttraction() const;

		/// Gets the list weight for this item.
		int getListOrder() const;

		/// Gets how fast a projectile fired from the weapon travels.
		int getBulletSpeed() const;
		/// Gets how fast the weapon's explosion-animation plays.
		int getExplosionSpeed() const;

		/// Gets how many auto-shots the weapon fires.
		int getAutoShots() const;
		/// Gets how much kick the weapon gives on auto-shot.
		int getAutoKick() const;

		/// Checks if this item is a two-handed weapon.
		bool isRifle() const;
		/// Checks if this item is a one-handed weapon.
		bool isPistol() const;
		/// Checks if this item is a grenade.
		bool isGrenade() const;

		/// Gets the max range of this weapon.
		int getMaxRange() const;
		/// Gets the max range of aimed shots with this weapon.
		int getAimRange() const;
		/// Gets the max range of snap shots with this weapon.
		int getSnapRange() const;
		/// Gets the max range of auto shots with this weapon.
		int getAutoRange() const;
		/// Gets the minimum effective range of this weapon.
		int getMinRange() const;
		/// Gets the accuracy dropoff of this weapon.
		int getDropoff() const;

		/// Gets the number of shotgun projectiles to trace.
		int getShotgunPellets() const;
		/// Gets the breadth of cone for shotgun projectiles.
		int getShotgunPattern() const;

		/// Gets a weapon's zombie unit, if any.
		const std::string& getZombieUnit() const;

		/// Gets if strength should be applied to the damage of this melee weapon.
		bool isStrengthApplied() const;
		/// Gets if skill is applied to the accuracy of this melee weapon.
		bool isSkillApplied() const;

		/// Used to determine if a weapon is capable of Reaction Fire.
		bool canReactionFire() const;

		/// Gets the sound a weapon makes when you swing it at someone.
		int getMeleeSound() const;
		/// Gets the sound a weapon makes when you punch someone in the face with it.
		int getMeleeHitSound() const;
		/// Gets a weapon's melee damage.
		int getMeleePower() const;
		/// Gets a weapon's melee-swing animation.
		int getMeleeAnimation() const;
		/// Gets a weapon's melee-hit animation.
		int getMeleeHitAnimation() const;

		/// Checks if LOS is required to use this item (only applies to psionic type items)
		bool isLosRequired() const;

		/// Gets the associated special type of this item.
		TilepartSpecial getSpecialType() const;

		/// Gets the item's default BattleAction.
		BattleActionType getDefaultAction(bool isPrimed = false) const;

		/// Checks if an item is exempt from research.
		bool isResearchExempt() const;

		/// Checks if this item is capable of executing a BattleUnit.
		bool canExecute() const;

		/// Gets if an explosion of the item causes an electro-magnetic pulse.
		bool defusePulse() const;

		/// Gets the Item's accuracy-modifier for crouched units.
		double getCrouch() const;
};

}

#endif
