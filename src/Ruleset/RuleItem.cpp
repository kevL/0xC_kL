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

#include "RuleItem.h"

#include "RuleInventory.h"

//#include "../Engine/Logger.h"
#include "../Engine/Surface.h"
#include "../Engine/SurfaceSet.h"


namespace OpenXcom
{

/**
 * Creates a blank ruleset for a certain type of item.
 * @param type - string defining the type
 */
RuleItem::RuleItem(const std::string& type)
	:
		_type(type),
		_label(type),
		_size(0.),
		_costBuy(0),
		_costSell(0),
		_transferTime(24),
		_weight(3),
		_bigSprite(BIGSPRITE_NONE),
		_floorSprite(-1),
		_handSprite(120),
		_bulletSprite(-1),
		_firePower(0),
		_fireSound(-1),
		_fireSoundHit(-1),
		_fireHitAni(-1),
		_dType(DT_NONE),
		_maxRange(200), // could be -1 for infinite.
		_aimRange(20),
		_snapRange(10),
		_autoRange(5),
		_minRange(0), // should be -1 so unit can shoot its own tile (don't ask why .. just for logical consistency)
		_dropoff(1),
		_accuracyAimed(0),
		_accuracySnap(0),
		_accuracyAuto(0),
		_accuracyMelee(0),
		_tuLaunch(0), // these could all be -1 meaning not possible, but this would be problematic re. ReactionFire etc etc.
		_tuAimed(0),
		_tuSnap(0),
		_tuAuto(0),
		_tuMelee(0),
		_tuUse(0),
		_tuReload(15),
		_tuUnload(12),
		_tuPrime(0),
		_tuDefuse(15),
		_fullClip(0),
		_blastRadius(-1),
		_battleType(BT_NONE),
		_arcingShot(false),
		_twoHanded(false),
		_waypoint(0),
		_fixedWeapon(false),
		_flatRate(false),
		_invWidth(1),
		_invHeight(1),
		_painKiller(0),
		_heal(0),
		_stimulant(0),
		_woundRecovery(0),
		_healthRecovery(0),
		_stunRecovery(0),
		_energyRecovery(0),
		_score(0),
		_armor(20),
		_turretType(TRT_NONE),
		_recover(true),
		_liveAlien(false),
		_attraction(0),
		_listOrder(0),
		_bulletSpeed(0),
		_explosionSpeed(0),
		_autoShots(3),
		_autoKick(12),
		_shotgunPellets(0),
		_shotgunPattern(5),
		_strengthApplied(false),
		_skillApplied(true),
		_LOSRequired(false),
		_noReaction(false),
		_noResearch(false),
		_meleePower(0),
		_meleeAni(-1),
		_meleeAniHit(-1),
		_meleeSound(-1),
		_meleeSoundHit(-1),
		_specialType(TILE),
		_canExecute(false),
		_defusePulse(false),
		_acuCrouch(1.16)
{}

/**
 * dTor.
 */
RuleItem::~RuleItem()
{}

/**
 * Loads this RuleItem from a YAML file.
 * @param node		- reference to the YAML node
 * @param modIndex	- offsets the sounds and sprite values to avoid conflicts
 * @param listOrder	- the list weight of the item
 */
void RuleItem::load(
		const YAML::Node& node,
		int modIndex,
		int listOrder)
{
	_type			= node["type"]			.as<std::string>(_type);
	_label			= node["label"]			.as<std::string>(_label);
	_reqResearch	= node["reqResearch"]	.as<std::vector<std::string>>(_reqResearch);
	_size			= node["size"]			.as<double>(_size);
	_costBuy		= node["costBuy"]		.as<int>(_costBuy);
	_costSell		= node["costSell"]		.as<int>(_costSell);
	_transferTime	= node["transferTime"]	.as<int>(_transferTime);
	_weight			= node["weight"]		.as<int>(_weight);

	if (node["bigSprite"])
	{
		_bigSprite = node["bigSprite"].as<int>(_bigSprite);
		if (_bigSprite > 56) // BIGOBS.PCK: 57 entries
			_bigSprite += modIndex;
	}

	if (node["floorSprite"])
	{
		_floorSprite = node["floorSprite"].as<int>(_floorSprite);
		if (_floorSprite > 72) // FLOOROB.PCK: 73 entries
			_floorSprite += modIndex;
	}

	if (node["handSprite"])
	{
		_handSprite = node["handSprite"].as<int>(_handSprite);
		if (_handSprite > 127) // HANDOB.PCK: 128 entries
			_handSprite += modIndex;
	}

	if (node["bulletSprite"])
	{
		_bulletSprite = node["bulletSprite"].as<int>(_bulletSprite) * 35;
		if (_bulletSprite >= 385) // Projectiles: 385 entries ((105*33) / (3*3)) (35 sprites per projectile(0-34), 11 projectiles (0-10))
			_bulletSprite += modIndex;
	}

	if (node["fireSound"])
	{
		_fireSound = node["fireSound"].as<int>(_fireSound);
		if (_fireSound > 54) // BATTLE.CAT: 55 entries
			_fireSound += modIndex;
	}

	if (node["fireSoundHit"])
	{
		_fireSoundHit = node["fireSoundHit"].as<int>(_fireSoundHit);
		if (_fireSoundHit > 54) // BATTLE.CAT: 55 entries
			_fireSoundHit += modIndex;
	}

	if (node["meleeSound"])
	{
		_meleeSound = node["meleeSound"].as<int>(_meleeSound);
		if (_meleeSound > 54) // BATTLE.CAT: 55 entries
			_meleeSound += modIndex;
	}

	if (node["meleeSoundHit"])
	{
		_meleeSoundHit = node["meleeSoundHit"].as<int>(_meleeSoundHit);
		if (_meleeSoundHit > 54) // BATTLE.CAT: 55 entries
			_meleeSoundHit += modIndex;
	}

	if (node["fireHitAni"])
	{
		_fireHitAni = node["fireHitAni"].as<int>(_fireHitAni);
		if (_fireHitAni > 55) // SMOKE.PCK: 56 entries
			_fireHitAni += modIndex;
	}

	if (node["meleeAniHit"])
	{
		_meleeAniHit = node["meleeAniHit"].as<int>(_meleeAniHit);
		if (_meleeAniHit > 3) // HIT.PCK: 4 entries
			_meleeAniHit += modIndex;
	}

	_meleeAni = node["meleeAni"].as<int>(_meleeAni);

	_dType			= static_cast<DamageType>(node["damageType"].as<int>(_dType));
	_battleType		= static_cast<BattleType>(node["battleType"].as<int>(_battleType));
	_specialType	= static_cast<TileType>(node["specialType"]	.as<int>(_specialType));
	_turretType		= static_cast<TurretType>(node["turretType"].as<int>(_turretType));

	_firePower			= node["power"]				.as<int>(_firePower);
	_meleePower			= node["meleePower"]		.as<int>(_meleePower);
	_strengthApplied	= node["strengthApplied"]	.as<bool>(_strengthApplied);
	_skillApplied		= node["skillApplied"]		.as<bool>(_skillApplied);
	_fullClip			= node["fullClip"]			.as<int>(_fullClip);
	_acceptedLoads		= node["acceptedLoads"]		.as<std::vector<std::string>>(_acceptedLoads);
	_accuracyAuto		= node["accuracyAuto"]		.as<int>(_accuracyAuto);
	_accuracySnap		= node["accuracySnap"]		.as<int>(_accuracySnap);
	_accuracyAimed		= node["accuracyAimed"]		.as<int>(_accuracyAimed);
	_accuracyMelee		= node["accuracyMelee"]		.as<int>(_accuracyMelee);
	_tuAuto				= node["tuAuto"]			.as<int>(_tuAuto);
	_tuSnap				= node["tuSnap"]			.as<int>(_tuSnap);
	_tuAimed			= node["tuAimed"]			.as<int>(_tuAimed);
	_tuLaunch			= node["tuLaunch"]			.as<int>(_tuLaunch);
	_tuUse				= node["tuUse"]				.as<int>(_tuUse);
	_tuPrime			= node["tuPrime"]			.as<int>(_tuPrime);
	_tuDefuse			= node["tuDefuse"]			.as<int>(_tuDefuse);
	_tuMelee			= node["tuMelee"]			.as<int>(_tuMelee);
	_tuReload			= node["tuReload"]			.as<int>(_tuReload);
	_tuUnload			= node["tuUnload"]			.as<int>(_tuUnload);
	_twoHanded			= node["twoHanded"]			.as<bool>(_twoHanded);
	_waypoint			= node["waypoint"]			.as<int>(_waypoint);
	_fixedWeapon		= node["fixedWeapon"]		.as<bool>(_fixedWeapon);
	_invWidth			= node["invWidth"]			.as<int>(_invWidth);
	_invHeight			= node["invHeight"]			.as<int>(_invHeight);
	_painKiller			= node["painKiller"]		.as<int>(_painKiller);
	_heal				= node["heal"]				.as<int>(_heal);
	_stimulant			= node["stimulant"]			.as<int>(_stimulant);
	_woundRecovery		= node["woundRecovery"]		.as<int>(_woundRecovery);
	_healthRecovery		= node["healthRecovery"]	.as<int>(_healthRecovery);
	_stunRecovery		= node["stunRecovery"]		.as<int>(_stunRecovery);
	_energyRecovery		= node["energyRecovery"]	.as<int>(_energyRecovery);
	_score				= node["score"]				.as<int>(_score);
	_armor				= node["armor"]				.as<int>(_armor);
	_recover			= node["recover"]			.as<bool>(_recover);
	_liveAlien			= node["liveAlien"]			.as<bool>(_liveAlien);
	_blastRadius		= node["blastRadius"]		.as<int>(_blastRadius);
	_attraction			= node["attraction"]		.as<int>(_attraction);
	_flatRate			= node["flatRate"]			.as<bool>(_flatRate);
	_arcingShot			= node["arcingShot"]		.as<bool>(_arcingShot);
	_maxRange			= node["maxRange"]			.as<int>(_maxRange);
	_aimRange			= node["aimRange"]			.as<int>(_aimRange);
	_snapRange			= node["snapRange"]			.as<int>(_snapRange);
	_autoRange			= node["autoRange"]			.as<int>(_autoRange);
	_minRange			= node["minRange"]			.as<int>(_minRange);
	_dropoff			= node["dropoff"]			.as<int>(_dropoff);
	_bulletSpeed		= node["bulletSpeed"]		.as<int>(_bulletSpeed);
	_explosionSpeed		= node["explosionSpeed"]	.as<int>(_explosionSpeed);
	_autoShots			= node["autoShots"]			.as<int>(_autoShots);
	_autoKick			= node["autoKick"]			.as<int>(_autoKick);
	_shotgunPellets		= node["shotgunPellets"]	.as<int>(_shotgunPellets);
	_shotgunPattern		= node["shotgunPattern"]	.as<int>(_shotgunPattern);
	_zombieUnit			= node["zombieUnit"]		.as<std::string>(_zombieUnit);
	_LOSRequired		= node["LOSRequired"]		.as<bool>(_LOSRequired);
	_noReaction			= node["noReaction"]		.as<bool>(_noReaction);
	_noResearch			= node["noResearch"]		.as<bool>(_noResearch);
	_defusePulse		= node["defusePulse"]		.as<bool>(_defusePulse);
	_acuCrouch			= node["acuCrouch"]			.as<double>(_acuCrouch);

	switch (_dType)
	{
		case DT_AP:
		case DT_LASER:
		case DT_PLASMA:
		case DT_MELEE:
		case DT_ACID:
			_canExecute = true;
	}

	_listOrder = node["listOrder"].as<int>(_listOrder);
	if (_listOrder == 0)
		_listOrder = listOrder;
}

/**
 * Gets the Item's type.
 * @note Each Item has a unique type.
 * @return, the item-type
 */
const std::string& RuleItem::getType() const
{
	return _type;
}

/**
 * Gets the string that names the Item.
 * @note This is not necessarily unique. Currently used only to differentiate
 * corpses from their respective battlefield parts.
 * @return, the item's label
 */
const std::string& RuleItem::getLabel() const
{
	return _label;
}

/**
 * Gets the list of required-research to use the Item.
 * @return, the list of research IDs
 */
const std::vector<std::string>& RuleItem::getRequiredResearch() const
{
	return _reqResearch;
}

/**
 * Gets the amount of space the Item takes up in a storage facility.
 * @return, the storage size
 */
double RuleItem::getStoreSize() const
{
	return _size;
}

/**
 * Gets the amount of money the Item costs to purchase (0 if not purchasable).
 * @return, the buy cost
 */
int RuleItem::getBuyCost() const
{
	return _costBuy;
}

/**
 * Gets the amount of money the Item is worth to sell.
 * @return, the sell cost
 */
int RuleItem::getSellCost() const
{
	return _costSell;
}

/**
 * Gets the amount of time the Item takes to arrive at a base.
 * @return, the time in hours
 */
int RuleItem::getTransferTime() const
{
	return _transferTime;
}

/**
 * Gets the weight of the Item.
 * @return, the weight in strength units
 */
int RuleItem::getWeight() const
{
	return _weight;
}

/**
 * Gets the reference in BIGOBS.PCK for use in inventory.
 * @return, the sprite reference
 */
int RuleItem::getBigSprite() const
{
	return _bigSprite;
}

/**
 * Gets the reference in FLOOROB.PCK for use in inventory.
 * @return, the sprite reference
 */
int RuleItem::getFloorSprite() const
{
	return _floorSprite;
}

/**
 * Gets the reference in HANDOB.PCK for use in inventory.
 * @return, the sprite reference
 */
int RuleItem::getHandSprite() const
{
	return _handSprite;
}

/**
 * Checks if the Item is held with two hands.
 * @return, true if Item is two-handed
 */
bool RuleItem::isTwoHanded() const
{
	return _twoHanded;
}

/**
 * Checks if the Item is a launcher and if so how many waypoints it can set.
 * @return, maximum quantity of waypoints
 */
int RuleItem::isWaypoints() const
{
	return _waypoint;
}

/**
 * Checks if the Item is a fixed-weapon.
 * @note Fixed-weapons can't be moved/thrown/dropped - e.g. HWP-turrets.
 * @return, true if fixed-weapon
 */
bool RuleItem::isFixed() const
{
	return _fixedWeapon;
}

/**
 * Gets the Item's bullet sprite reference.
 * @return, the sprite reference
 */
int RuleItem::getBulletSprite() const
{
	return _bulletSprite;
}

/**
 * Gets the Item's fire sound.
 * @return, the fire sound ID
 */
int RuleItem::getFireSound() const
{
	return _fireSound;
}

/**
 * Gets the Item's hit sound.
 * @return, the hit sound ID
 */
int RuleItem::getFireHitSound() const
{
	return _fireSoundHit;
}

/**
 * Gets the Item's hit animation.
 * @return, the hit animation ID
 */
int RuleItem::getFireHitAnimation() const
{
	return _fireHitAni;
}

/**
 * Gets the Item's damage-power or light-power if its BattleType is BT_FLARE.
 * @return, the power
 */
int RuleItem::getPower() const
{
	return _firePower;
}

/**
 * Gets the Item's accuracy for snapshots.
 * @return, the snapshot accuracy
 */
int RuleItem::getAccuracySnap() const
{
	return _accuracySnap;
}

/**
 * Gets the Item's accuracy for autoshots.
 * @return, the autoshot accuracy
 */
int RuleItem::getAccuracyAuto() const
{
	return _accuracyAuto;
}

/**
 * Gets the Item's accuracy for aimed shots.
 * @return, the aimed accuracy
 */
int RuleItem::getAccuracyAimed() const
{
	return _accuracyAimed;
}

/**
 * Gets the Item's accuracy for melee attacks.
 * @return, the melee accuracy
 */
int RuleItem::getAccuracyMelee() const
{
	return _accuracyMelee;
}

/**
 * Gets the Item's TU cost for snapshots.
 * @return, the snapshot TU percentage
 */
int RuleItem::getSnapTu() const
{
	return _tuSnap;
}

/**
 * Gets the Item's TU cost for autoshots.
 * @return, the autoshot TU percentage
 */
int RuleItem::getAutoTu() const
{
	return _tuAuto;
}

/**
 * Gets the Item's TU cost for aimed shots.
 * @return, the aimed shot TU percentage
 */
int RuleItem::getAimedTu() const
{
	return _tuAimed;
}

/**
 * Gets the Item's TU cost for launched shots.
 * @return, the launch shot TU percentage
 */
int RuleItem::getLaunchTu() const
{
	return _tuLaunch;
}

/**
 * Gets the Item's TU cost for melee attacks.
 * @return, the melee TU percentage
 */
int RuleItem::getMeleeTu() const
{
	return _tuMelee;
}

/**
 * Gets the number of Time Units needed to use the Item.
 * @return, the TU
 */
int RuleItem::getUseTu() const
{
	return _tuUse;
}

/**
 * Gets the number of Time Units needed to reload the Item.
 * @return, the TU
 */
int RuleItem::getReloadTu() const
{
	return _tuReload;
}

/**
 * Gets the number of Time Units needed to unload the Item.
 * @return, the TU
 */
int RuleItem::getUnloadTu() const
{
	return _tuUnload;
}

/**
 * Gets the number of Time Units needed to prime the Item.
 * @return, the TU
 */
int RuleItem::getPrimeTu() const
{
	return _tuPrime;
}

/**
 * Gets the number of Time Units needed to defuse the Item.
 * @return, the TU
 */
int RuleItem::getDefuseTu() const
{
	return _tuDefuse;
}

/**
 * Gets a list of loadable ammunition that the Item accepts.
 * @return, pointer to a vector of loadable types
 */
const std::vector<std::string>* RuleItem::getAcceptedLoadTypes() const
{
	return &_acceptedLoads;
}

/**
 * Gets the Item's damage-type.
 * @return, the damage-type (RuleItem.h)
 */
DamageType RuleItem::getDamageType() const
{
	return _dType;
}

/**
 * Gets the Item's battle-type.
 * @return, the battle-type (RuleItem.h)
 */
BattleType RuleItem::getBattleType() const
{
	return _battleType;
}

/**
 * Gets the Item's width in an Inventory.
 * @return, the width
 */
int RuleItem::getInventoryWidth() const
{
	return _invWidth;
}

/**
 * Gets the Item's height in an Inventory.
 * @return, the height
 */
int RuleItem::getInventoryHeight() const
{
	return _invHeight;
}

/**
 * Gets the Item's clip-size.
 * @note Melee and other self-powered items have fullClip= -1.
 * @return, the clip-size
 */
int RuleItem::getFullClip() const
{
	return _fullClip;
}

/**
 * Draws and centers the hand-sprite on a Surface according to its dimensions.
 * @param srt - pointer to the SurfaceSet of a bigob-sprite
 * @param srf - pointer to the Surface on which to draw it
 */
void RuleItem::drawHandSprite(
		SurfaceSet* const srt,
		Surface* const srf) const
{
	Surface* const sprite (srt->getFrame(_bigSprite));
	if (sprite != nullptr) // safety.
	{
		sprite->setX(
				(RuleInventory::HAND_W - _invWidth)  * RuleInventory::SLOT_W_2);
		sprite->setY(
				(RuleInventory::HAND_H - _invHeight) * RuleInventory::SLOT_H_2);

		sprite->blit(srf);
	}
//	else Log(LOG_WARNING) << "RuleItem::drawHandSprite() bigob not found #" << _bigSprite; // also in Inventory.
}

/**
 * Gets the heal quantity of the Item.
 * @return, the new heal quantity
 */
int RuleItem::getHealQuantity() const
{
	return _heal;
}

/**
 * Gets the pain killer quantity of the Item.
 * @return, the new pain killer quantity
 */
int RuleItem::getPainKillerQuantity() const
{
	return _painKiller;
}

/**
 * Gets the stimulant quantity of the Item.
 * @return, the new stimulant quantity
 */
int RuleItem::getStimulantQuantity() const
{
	return _stimulant;
}

/**
 * Gets the amount of fatal wounds healed per usage.
 * @return, the amount of fatal wounds healed
 */
int RuleItem::getWoundRecovery() const
{
	return _woundRecovery;
}

/**
 * Gets the amount of health added to a wounded soldier's health.
 * @return, the amount of health to add
 */
int RuleItem::getHealthRecovery() const
{
	return _healthRecovery;
}

/**
 * Gets the amount of energy added to a soldier's energy.
 * @return, the amount of energy to add
 */
int RuleItem::getEnergyRecovery() const
{
	return _energyRecovery;
}

/**
 * Gets the amount of stun removed from a soldier's stun-level.
 * @return, the amount of stun removed
 */
int RuleItem::getStunRecovery() const
{
	return _stunRecovery;
}

/**
 * Gets the Item's explosion-radius.
 * @note Small explosions don't have a restriction. Larger explosions are
 * restricted using a formula with a maximum of radius 10 no matter how large
 * the explosion. kL_note: nah...
 * @return, the radius (-1 if not AoE)
 */
int RuleItem::getExplosionRadius() const
{
	switch (_dType)
	{
		case DT_HE:
		case DT_STUN:
		case DT_SMOKE:
		case DT_IN:
			if (_blastRadius == -1)
				return _firePower / 20;
	}
	return _blastRadius;
}

/**
 * Gets the Item's battlefield recovery-score.
 * @note This is used during the battlescape debriefing-score calculation.
 * @return, the recovery-points
 */
int RuleItem::getRecoveryScore() const
{
	return _score;
}

/**
 * Gets the Item's armor-points.
 * @note The item is destroyed when an explosive power higher than its armor-
 * points hits it.
 * @return, the armor-points
 */
int RuleItem::getArmorPoints() const
{
	return _armor;
}

/**
 * Checks if the Item is recoverable from the battlefield.
 * @return, true if recoverable
 */
bool RuleItem::isRecoverable() const
{
	return _recover;
}

/**
 * Gets the Item's turret-type.
 * @return, the TurretType (RuleItem.h)
 */
TurretType RuleItem::getTurretType() const
{
	return _turretType;
}

/**
 * Checks if this is a live-alien.
 * @return, true if live alien
 */
bool RuleItem::isLiveAlien() const
{
	return _liveAlien;
}

/**
 * Gets whether the Item charges a flat TU rate.
 * @return, true if flat TU rate
 */
bool RuleItem::isFlatRate() const
{
	return _flatRate;
}

/**
 * Checks if the Item uses arcing-shots.
 * @return, true if parabola-trajectory
 */
bool RuleItem::isArcingShot() const
{
	return _arcingShot;
}

/**
 * Gets the attraction-value for the Item (used by AI).
 * @return, the attraction-value
 */
int RuleItem::getAttraction() const
{
	return _attraction;
}

/**
 * Gets the list-weight for this research item
 * @return, the list-weight
 */
int RuleItem::getListOrder() const
{
	 return _listOrder;
}

/**
 * Gets the maximum range of the Item.
 * @return, the range in tile-space
 */
int RuleItem::getMaxRange() const
{
	return _maxRange;
}

/**
 * Gets the maximum effective range of the Item when using Aimed Shot.
 * @return, the range in tile-space
 */
int RuleItem::getAimRange() const
{
	return _aimRange;
}

/**
 * Gets the maximim effective range of the Item for Snap Shot.
 * @return, the range in tile-space
 */
int RuleItem::getSnapRange() const
{
	return _snapRange;
}

/**
 * Gets the maximim effective range of the Item for Auto Shot.
 * @return, the range in tile-space
 */
int RuleItem::getAutoRange() const
{
	return _autoRange;
}

/**
 * Gets the minimum effective range of the Item.
 * @return, the range in tile-space
 */
int RuleItem::getMinRange() const
{
	return _minRange;
}

/**
 * Gets the accuracy dropoff of the Item.
 * @return, the per-tile dropoff
 */
int RuleItem::getDropoff() const
{
	return _dropoff;
}

/**
 * Gets the speed at which the Item travels as a projectile.
 * @return, the speed
 */
int RuleItem::getBulletSpeed() const
{
	return _bulletSpeed;
}

/**
 * Gets the speed at which the Item explodes as a projectile.
 * @return, the speed
 */
int RuleItem::getExplosionSpeed() const
{
	return _explosionSpeed;
}

/**
 * Gets the quantity of auto-shots fired by the Item.
 * @return, the quantity of shots
 */
int RuleItem::getAutoShots() const
{
	return _autoShots;
}

/**
 * Gets the kick that the Item does on auto-shot.
 */
int RuleItem::getAutoKick() const
{
	return _autoKick;
}

/**
 * Gets if the Item is a rifle (two-handed weapon).
 * @return, true if rifle
 */
bool RuleItem::isRifle() const
{
	return _twoHanded == true
			&& (   _battleType == BT_FIREARM
				|| _battleType == BT_MELEE);
}

/**
 * Gets if the Item is a pistol (one-handed weapon).
 * @return, true if pistol
 */
bool RuleItem::isPistol() const
{
	return _twoHanded == false
			&& (   _battleType == BT_FIREARM
				|| _battleType == BT_MELEE);
}

/**
 * Gets if the Item is a grenade.
 * @return, true if grenade
 */
bool RuleItem::isGrenade() const
{
	return _battleType == BT_GRENADE
		|| _battleType == BT_PROXYGRENADE;
}

/**
 * Gets the number of projectiles this ammo shoots at once - a shotgun effect.
 * @return, the number of projectiles
 */
int RuleItem::getShotgunPellets() const
{
	return _shotgunPellets;
}

/**
 * Gets the breadth of cone for shotgun projectiles.
 * @return, the breadth of cone
 */
int RuleItem::getShotgunPattern() const
{
	return _shotgunPattern;
}

/**
 * Gets the unit that the victim is morphed into when attacked.
 * @return, the weapon's zombie unit
 */
const std::string& RuleItem::getZombieUnit() const
{
	return _zombieUnit;
}

/**
 * Used to determine if a unit's strength is added to melee-damage.
 * @return, true if added
 */
bool RuleItem::isStrengthApplied() const
{
	return _strengthApplied;
}

/**
 * Checks if skill is applied to the accuracy of the Item.
 * @note This applies only to melee-weapons.
 * @return, true if applied
 */
bool RuleItem::isSkillApplied() const
{
	return _skillApplied;
}

/**
 * Checks if the Item is capable of reaction-fire.
 * @return, true if a weapon can react during opponent's turn
 */
bool RuleItem::canReactionFire() const
{
	return (_noReaction == false);
}

/**
 * The sound that the Item makes when you swing it at someone.
 * @return, the melee-attack sound-ID
 */
int RuleItem::getMeleeSound() const
{
	return _meleeSound;
}

/**
 * The sound that the Item makes when you hit someone in the face with it.
 * @return, the melee-hit sound-ID
 */
int RuleItem::getMeleeHitSound() const
{
	return _meleeSoundHit;
}

/**
 * The damage that the Item does when you hit someone in the face with it.
 * @return, the melee-power
 */
int RuleItem::getMeleePower() const
{
	return _meleePower;
}

/**
 * Gets the sprite-offset in "ClawTooth" used to start the melee-swing animation.
 * @return, the offset to start at
 */
int RuleItem::getMeleeAnimation() const
{
	return _meleeAni;
}

/**
 * Gets the sprite-offset in "HIT.PCK" used to start the hit-success animation.
 * @return, the offset to start at
 */
int RuleItem::getMeleeHitAnimation() const
{
	return _meleeAniHit;
}

/**
 * Checks if line of sight is required for this psionic weapon to function.
 * @return, true if line of sight is required
 */
bool RuleItem::isLosRequired() const
{
	return _LOSRequired;
}

/**
 * Gets the associated special type of the Item.
 * @note Type 14 is the alien brain and types 0 and 1 are "regular tile" and
 * "starting point" so try not to use those ones.
 * @return, special type
 */
TileType RuleItem::getTileType() const
{
	return _specialType;
}

/**
 * Gets the Item's default BattleAction.
 * @note Used to show a TU cost in InventoryState. Lifted from ActionMenuState cTor.
 * @param isPrimed - true if checking a grenade and it's primed (default false)
 * @return, BattleActionType (BattlescapeGame.h)
 */
BattleActionType RuleItem::getDefaultAction(bool isPrimed) const
{
	if (_fixedWeapon == false)
	{
		switch (_battleType)
		{
//			case BT_AMMO:
//				return BA_RELOAD;

			case BT_MELEE:
				return BA_MELEE;

			case BT_FIREARM:
				if (_waypoint != 0)
					return BA_LAUNCH;

				if (_accuracySnap != 0)
					return BA_SNAPSHOT;

				if (_accuracyAuto != 0)
					return BA_AUTOSHOT;

				if (_accuracyAimed != 0)
					return BA_AIMEDSHOT;
				break;

			case BT_GRENADE:
			case BT_PROXYGRENADE:
			case BT_FLARE:
				if (isPrimed == false)
					return BA_PRIME;

				return BA_DEFUSE;

			case BT_MEDIKIT:
			case BT_SCANNER:
			case BT_MINDPROBE:
				return BA_USE;

			case BT_PSIAMP:
				return BA_PSIPANIC;
		}
	}
	return BA_NONE;
}

/**
 * Checks if the Item is exempt from research.
 * @note Currently this is used to exclude SHADICS ARMORS from getting marked
 * as unresearched in various lists, such as Stores & Transfers ...
 * This boolean should be set in the Rulesets under these ITEMS respectively.
 * and then the checks both here and in those lists ought be simplified.
 * @note Put in Ruleset done.
 * @return, true if the item shows in lists without being researched
 */
bool RuleItem::isResearchExempt() const
{
	return _noResearch;
//	if (   getType() == "STR_BLACKSUIT_ARMOR"
//		|| getType() == "STR_BLUESUIT_ARMOR"
//		|| getType() == "STR_GREENSUIT_ARMOR"
//		|| getType() == "STR_ORANGESUIT_ARMOR"
//		|| getType() == "STR_PINKSUIT_ARMOR"
//		|| getType() == "STR_PURPLESUIT_ARMOR"
//		|| getType() == "STR_REDSUIT_ARMOR"
//		|| getType() == "STR_BLACK_ARMOR"
//		|| getType() == "STR_BLUE_ARMOR"
//		|| getType() == "STR_GREEN_ARMOR"
//		|| getType() == "STR_ORANGE_ARMOR"
//		|| getType() == "STR_PINK_ARMOR"
//		|| getType() == "STR_PURPLE_ARMOR"
//		|| getType() == "STR_RED_ARMOR")
//	{
//		return true;
//	}
//	return false;
}

/**
 * Checks if the Item is capable of executing a BattleUnit.
 * @return, true if executable
 */
bool RuleItem::canExecute() const
{
	return _canExecute;
}

/**
 * Gets if an explosion of the Item causes an electro-magnetic pulse.
 * @return, true if EM pulse
 */
bool RuleItem::defusePulse() const
{
	return _defusePulse;
}

/**
 * Gets the Item's accuracy-modifier when fired by crouched units.
 * @note Is used only for flat- or arching-shots; not for throwing or melee.
 * @return, the crouched coefficient
 */
double RuleItem::getCrouch() const
{
	return _acuCrouch;
}

}
