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

#include "CraftWeapon.h"

//#include <algorithm>

#include "CraftWeaponProjectile.h"

#include "../Ruleset/RuleCraftWeapon.h"
#include "../Ruleset/RuleItem.h"
#include "../Ruleset/Ruleset.h"


namespace OpenXcom
{

/**
 * Initializes the CraftWeapon with a specified rule.
 * @param cwRule	- pointer to RuleCraftWeapon
 * @param ammo		- initial ammo quantity (default 0)
 */
CraftWeapon::CraftWeapon(
		const RuleCraftWeapon* const cwRule,
		int ammo)
	:
		_cwRule(cwRule),
		_ammo(ammo),
		_rearming(false),
		_cantLoad(false)
{}

/**
 * dTor.
 */
CraftWeapon::~CraftWeapon()
{}

/**
 * Loads this CraftWeapon from a YAML file.
 * @param node - reference a YAML node
 */
void CraftWeapon::load(const YAML::Node& node)
{
	_ammo		= node["ammo"]		.as<int>(_ammo);
	_rearming	= node["rearming"]	.as<bool>(_rearming);
	_cantLoad	= node["cantLoad"]	.as<bool>(_cantLoad);
}

/**
 * Saves this CraftWeapon to a YAML file.
 * @return, YAML node
 */
YAML::Node CraftWeapon::save() const
{
	YAML::Node node;

	node["type"] = _cwRule->getType();
	node["ammo"] = _ammo;

	if (_rearming) node["rearming"] = _rearming;
	if (_cantLoad) node["cantLoad"] = _cantLoad;

	return node;
}

/**
 * Gets the ruleset for this CraftWeapon.
 * @return, pointer to RuleCraftWeapon
 */
const RuleCraftWeapon* CraftWeapon::getRules() const
{
	return _cwRule;
}

/**
 * Gets the quantity of ammo contained in this CraftWeapon.
 * @return, quantity of ammo
 */
int CraftWeapon::getAmmo() const
{
	return _ammo;
}

/**
 * Sets the quantity of ammo contained in this CraftWeapon.
 * @note Maintains min/max levels.
 * @param ammo - quantity of ammo
 * @return, true if there was enough ammo to fire a round off
 */
bool CraftWeapon::setAmmo(int ammo)
{
	if (_ammo = ammo < 0)
	{
		_ammo = 0;
		return false;
	}

	if (_ammo > _cwRule->getLoadCapacity())
		_ammo = _cwRule->getLoadCapacity();

	return true;
}

/**
 * Gets whether this CraftWeapon needs rearming.
 * @return, rearming status
 */
bool CraftWeapon::getRearming() const
{
	return _rearming;
}

/**
 * Sets whether this CraftWeapon needs rearming - in case there's no more ammo.
 * @param rearming - rearming status (default true)
 */
void CraftWeapon::setRearming(bool rearming)
{
	_rearming = rearming;
}

/**
 * Rearms this CraftWeapon.
 * @param baseClips	- the quantity of clips available at the Base (default 0)
 * @param clipSize	- the quantity of rounds in a clip (default 0)
 * @return, the quantity of clips used (negative if not enough clips at Base)
 */
int CraftWeapon::rearm(
		int baseClips,
		int clipSize)
{
	const int
		fullQty (_cwRule->getLoadCapacity()),
		rateQty (_cwRule->getRearmRate());

	int
		ret,
		loadQty,
		clipsRequested;

	if (clipSize > 0)
		clipsRequested = std::min(rateQty + clipSize - 1, // round up int ->
								  fullQty - _ammo + clipSize - 1)
								/ clipSize;
	else
		clipsRequested = 0;

	if (baseClips >= clipsRequested)
	{
		_cantLoad = false;

		if (clipSize == 0)
			loadQty = rateQty;
		else
			loadQty = clipsRequested * clipSize;
	}
	else // baseClips < clipsRequested
	{
		_cantLoad = true;
		loadQty = baseClips * clipSize;
	}

	setAmmo(_ammo + loadQty);
	_rearming = (_ammo < fullQty);

	if (clipSize == 0)
		return 0;

	ret = (loadQty + clipSize - 1) / clipSize;
	if (clipsRequested > baseClips)
		ret = -ret; // trick to tell Craft there isn't enough clips at Base.

	return ret;
}

/**
 * Gets this CraftWeapon's cantLoad status - no stock in Base Stores.
 * @return, true if weapon ammo is low in stock
 */
bool CraftWeapon::getCantLoad() const
{
	return _cantLoad;
}

/**
 * Sets this CraftWeapon's cantLoad status - no stock in Base Stores.
 * @param cantLoad - true if weapon ammo is low in stock (default true)
 */
void CraftWeapon::setCantLoad(bool cantLoad)
{
	_cantLoad = cantLoad;
}

/**
 * Fires a projectile from this CraftWeapon.
 * @return, pointer to the fired CraftWeaponProjectile
 */
CraftWeaponProjectile* CraftWeapon::fire() const
{
	CraftWeaponProjectile* const prj (new CraftWeaponProjectile());

	prj->setType(_cwRule->getProjectileType());
	prj->setSpeed(_cwRule->getProjectileSpeed());
	prj->setAccuracy(_cwRule->getAccuracy());
	prj->setPower(_cwRule->getPower());
	prj->setRange(_cwRule->getRange(), true);

	return prj;
}

/**
 * Gets how many clips are loaded in this CraftWeapon.
 * @param ruleset - pointer to the core Ruleset
 * @return, the number of clips loaded
 */
int CraftWeapon::getClipsLoaded(const Ruleset* const rules) const
{
	const RuleItem* const clip (rules->getItemRule(_cwRule->getClipType()));

	if (clip == nullptr || clip->getFullClip() < 1)
		return _ammo / _cwRule->getRearmRate(); // round down.

	return _ammo / clip->getFullClip(); // round down.
}

}
