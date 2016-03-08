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

#include "ExplosionBState.h"

#include "BattlescapeState.h"
#include "Camera.h"
#include "Explosion.h"
#include "Map.h"
#include "TileEngine.h"

//#include "../Engine/Logger.h"
#include "../Engine/RNG.h"
#include "../Engine/Sound.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/RuleArmor.h"
#include "../Ruleset/RuleItem.h"
#include "../Ruleset/Ruleset.h"

#include "../Savegame/BattleItem.h"
#include "../Savegame/BattleUnit.h"
#include "../Savegame/SavedBattleGame.h"
#include "../Savegame/Tile.h"


namespace OpenXcom
{

/**
 * Sets up an ExplosionBState.
 * @param parent		- pointer to the BattlescapeGame
 * @param centerVoxel	- center position in voxel-space
 * @param item			- pointer to item involved in the explosion (eg grenade)
 * @param unit			- pointer to unit involved in the explosion (eg unit throwing the grenade, cyberdisc, etc)
 * @param tile			- pointer to tile the explosion is on (default nullptr)
 * @param lowerWeapon	- true to tell the unit causing this explosion to lower their weapon (default false)
 * @param meleeSuccess	- true if the (melee) attack was succesful (default false)
 * @param forceCamera	- forces Camera to center on the explosion (default false)
 */
ExplosionBState::ExplosionBState(
		BattlescapeGame* const parent,
		const Position centerVoxel,
		BattleItem* const item,
		BattleUnit* const unit,
		Tile* const tile,
		bool lowerWeapon,
		bool meleeSuccess,
		bool forceCamera)
	:
		BattleState(parent),
		_centerVoxel(centerVoxel),
		_item(item),
		_unit(unit),
		_tile(tile),
		_lowerWeapon(lowerWeapon),
		_meleeSuccess(meleeSuccess),
		_forceCamera(forceCamera),
		_battleSave(parent->getBattleSave()),
		_power(0),
		_areaOfEffect(true),
		_buttHurt(false),
		_melee(false)
//		_extend(3) // extra think-cycles before this state is allowed to Pop.
{
	//Log(LOG_INFO) << "cTor ExplBState";
}

/**
 * Deletes the ExplosionBState.
 */
ExplosionBState::~ExplosionBState()
{
	//Log(LOG_INFO) << "dTor ExplBState";
}

/**
 * Initializes the explosion.
 * @note The animation and sound starts here. If the animation is finished the
 * actual effect takes place.
 */
void ExplosionBState::init()
{
	//Log(LOG_INFO) << ". init()";
	if (_item != nullptr)
	{
		const RuleItem* const itRule (_item->getRules());
		if (itRule->getBattleType() == BT_PSIAMP) // pass by. Let cTor initialization handle it. Except '_areaOfEffect' value
			_areaOfEffect = false;
		else
		{
			// getTacticalAction() only works for player actions: aliens cannot melee attack with rifle butts.
			_buttHurt = _unit != nullptr
					 && _unit->getFaction() == FACTION_PLAYER
					 && itRule->getBattleType() != BT_MELEE
					 && _parent->getTacticalAction()->type == BA_MELEE;

			if (_buttHurt == true)
				_power = itRule->getMeleePower();
			else
				_power = itRule->getPower();

			// since melee aliens don't use a conventional weapon type use their strength instead.
			if (_unit != nullptr
				&& itRule->isStrengthApplied() == true
				&& (itRule->getBattleType() == BT_MELEE
					|| _buttHurt == true))
			{
				int extraPower (_unit->getStrength() / 2);

				if (_buttHurt == true)
					extraPower /= 2; // pistolwhipping adds only 1/2 extraPower.

				if (_unit->isKneeled() == true)
					extraPower /= 2; // kneeled units further half extraPower.

				_power += RNG::generate( // add 10% to 100% of extPower
									(extraPower + 9) / 10,
									 extraPower);
			}

			// HE, incendiary, smoke or stun bombs create AOE explosions;
			// all the rest hits one point: AP, melee (stun or AP), laser, plasma, acid
			_areaOfEffect = _buttHurt == false
						 && itRule->getBattleType() != BT_MELEE
						 && itRule->getExplosionRadius() != -1;
		}
	}
	else if (_tile != nullptr)
		_power = _tile->getExplosive();
	else if (_unit != nullptr // cyberdiscs!!! And ... ZOMBIES.
		&& _unit->getSpecialAbility() == SPECAB_EXPLODE)
	{
		_power = _parent->getRuleset()->getItemRule(_unit->getArmor()->getCorpseGeoscape())->getPower();
		const int
			power1 (_power * 2 / 3),
			power2 (_power * 3 / 2);
		_power = RNG::generate(power1, power2)
			   + RNG::generate(power1, power2);
		_power /= 2;
	}
	else // unhandled cyberdisc!!!
	{
		_power = RNG::generate(67, 137)
			   + RNG::generate(67, 137);
		_power /= 2;
	}


	const Position posTarget (Position::toTileSpace(_centerVoxel));

	if (_areaOfEffect == true)
	{
		if (_power > 0)
		{
			int
				aniStart,
				aniDelay (0),
				qty (_power),
				radius,
				offset;

			Uint32 interval;
			if (_item != nullptr)
			{
				const RuleItem* const itRule (_item->getRules());
				if (itRule->defusePulse() == true)
					_parent->getMap()->setBlastFlash(true);

				interval = static_cast<Uint32>(
						   std::max(1,
									static_cast<int>(BattlescapeState::STATE_INTERVAL_EXPLOSION) - itRule->getExplosionSpeed()));

				aniStart = itRule->getFireHitAnimation();
				radius = std::max(0, itRule->getExplosionRadius());

				switch (itRule->getDamageType())
				{
					case DT_SMOKE:
					case DT_STUN:
						qty = qty * 2 / 3; // smoke & stun bombs do fewer anims.
				}
			}
			else
			{
				aniStart = ResourcePack::EXPLOSION_OFFSET;
				radius = _power / 9; // <- for cyberdiscs & terrain expl.
				interval = BattlescapeState::STATE_INTERVAL_EXPLOSION;
			}

			offset = radius * 6; // voxelspace
//			qty = static_cast<int>(sqrt(static_cast<double>(radius) * static_cast<double>(qty))) / 3;
			qty = radius * qty / 100;
			if (qty < 1 || offset == 0)
				qty = 1;

			Position explVoxel (_centerVoxel);
			for (int
					i = 0;
					i != qty;
					++i)
			{
				if (i != 0) // bypass 1st explosion: it's always centered w/out any delay.
				{
//					explVoxel.x += RNG::generate(-offset, offset); // these cause anims to sweep across the battlefield. Pretty cool.
//					explVoxel.y += RNG::generate(-offset, offset);
					explVoxel.x = _centerVoxel.x + RNG::generate(-offset, offset);
					explVoxel.y = _centerVoxel.y + RNG::generate(-offset, offset);

					if (RNG::percent(65) == true)
						++aniDelay;
				}

				if (aniStart != -1)
				{
					Explosion* const explosion (new Explosion(
															ET_AOE,
															explVoxel - Position(16,16,0), // jog downward on the screen.
															aniStart,
															aniDelay));
					_parent->getMap()->getExplosions()->push_back(explosion);
					_parent->setStateInterval(interval);
				}
			}


			int soundId;
			if (_item != nullptr)
				soundId = _item->getRules()->getFireHitSound();
			else if (_power < 73)
				soundId = ResourcePack::SMALL_EXPLOSION;
			else
				soundId = ResourcePack::LARGE_EXPLOSION;

			if (soundId != -1)
				_parent->getResourcePack()->getSound("BATTLE.CAT", soundId)
											->play(-1, _parent->getMap()->getSoundAngle(posTarget));

			Camera* const exploCam (_parent->getMap()->getCamera());
			if (_forceCamera == true
				|| exploCam->isOnScreen(posTarget) == false)
			{
				exploCam->centerOnPosition(posTarget, false);
			}
			else if (exploCam->getViewLevel() != posTarget.z)
				exploCam->setViewLevel(posTarget.z);
		}
		else
			_parent->popState();
	}
	else // create a bullet hit, or melee hit, or psi-hit, or acid spit hit
	{
		const RuleItem* const itRule (_item->getRules());
		ExplosionType
			explType_att,
			explType_hit;
		int
			soundId,
			aniStart_att,
			aniStart_hit;

		_melee = _buttHurt
			  || itRule->getBattleType() == BT_MELEE
			  || itRule->getBattleType() == BT_PSIAMP;

		if (_melee == true)
		{
			if (itRule->getBattleType() != BT_PSIAMP)
			{
				explType_att = ET_MELEE_ATT;
				explType_hit = ET_MELEE_HIT;

				if (_buttHurt == true)
				{
					aniStart_att = -1;
					if (_meleeSuccess == true)
					{
						soundId = itRule->getMeleeHitSound();
						aniStart_hit = itRule->getMeleeHitAnimation();
					}
					else
						soundId = aniStart_hit = -1;
				}
				else
				{
					soundId = itRule->getMeleeHitSound();
					aniStart_att = itRule->getMeleeAnimation();
					if (_meleeSuccess == true)
						aniStart_hit = itRule->getMeleeHitAnimation();
					else
						aniStart_hit = -1;
				}
			}
			else
			{
				explType_hit = explType_att = ET_PSI;
				soundId = itRule->getMeleeHitSound();
				aniStart_hit = itRule->getMeleeHitAnimation();
				aniStart_att = -1;
			}
		}
		else
		{
			soundId = itRule->getFireHitSound();
			aniStart_hit = itRule->getFireHitAnimation();
			aniStart_att = -1;

			if (itRule->getType() == "STR_FUSION_TORCH_POWER_CELL")
				explType_hit = explType_att = ET_TORCH;
			else
				explType_hit = explType_att = ET_BULLET;
		}

		if (soundId != -1)
			_parent->getResourcePack()->getSound("BATTLE.CAT", soundId)
										->play(-1, _parent->getMap()->getSoundAngle(posTarget));

		if (aniStart_att != -1 || aniStart_hit != -1)
		{
			Explosion* explosion;
			if (aniStart_att != -1) // TODO: Move create Explosion for start-swing to ProjectileFlyBState::performMeleeAttack().
			{
				explosion = new Explosion(
										explType_att,
										_centerVoxel,
										aniStart_att);
				_parent->getMap()->getExplosions()->push_back(explosion);
			}

			if (aniStart_hit != -1)
			{
				explosion = new Explosion(
										explType_hit,
										_centerVoxel,
										aniStart_hit);
				_parent->getMap()->getExplosions()->push_back(explosion);
			}

			Uint32 interval (static_cast<Uint32>(
							 std::max(1,
									  static_cast<int>(BattlescapeState::STATE_INTERVAL_EXPLOSION) - itRule->getExplosionSpeed())));
			_parent->setStateInterval(interval);
		}

		Camera* const exploCam (_parent->getMap()->getCamera());
		if (_forceCamera == true
			|| (exploCam->isOnScreen(posTarget) == false
				&& (_battleSave->getSide() != FACTION_PLAYER
					|| itRule->getBattleType() != BT_PSIAMP))
			|| (_battleSave->getSide() != FACTION_PLAYER
				&& itRule->getBattleType() == BT_PSIAMP))
		{
			exploCam->centerOnPosition(posTarget, false);
		}
		else if (exploCam->getViewLevel() != posTarget.z)
			exploCam->setViewLevel(posTarget.z);
	}
}

/**
 * Animates explosion sprites.
 * @note If their animation is finished remove them from the list. If the list
 * is empty this state is finished and the actual calculations take place.
 * kL_rewrite: Allow a few extra cycles for explosion animations to dissipate.
 */
void ExplosionBState::think()
{
/*	for (std::list<Explosion*>::const_iterator
			i = _parent->getMap()->getExplosions()->begin();
			i != _parent->getMap()->getExplosions()->end();
			)
	{
		if ((*i)->animate() == false)
		{
			delete *i;
			i = _parent->getMap()->getExplosions()->erase(i);
		}
		else ++i;
	}

	if (_parent->getMap()->getExplosions()->empty() == true)
		--_extend; // not working as intended; needs to go to Explosion class so that explosions-vector doesn't 'empty' so fast.

	if (_extend < 1)
		explode(); */

	//Log(LOG_INFO) << ". think()";
	if (_parent->getMap()->getBlastFlash() == false)
	{
		if (_parent->getMap()->getExplosions()->empty() == true)
		{
			//Log(LOG_INFO) << ". . empty";
			explode();
		}

		for (std::list<Explosion*>::const_iterator
				i = _parent->getMap()->getExplosions()->begin();
				i != _parent->getMap()->getExplosions()->end();
				)
		{
			//Log(LOG_INFO) << ". . iterate";
			if ((*i)->animate() == false) // done.
			{
				//Log(LOG_INFO) << ". . . done";
				delete *i;
				i = _parent->getMap()->getExplosions()->erase(i);

				if (_parent->getMap()->getExplosions()->empty() == true)
				{
					//Log(LOG_INFO) << ". . . . final empty";
					explode();
					return;
				}
			}
			else
				++i;
		}
	}
}

/**
 * Explosions cannot be cancelled.
 */
//void ExplosionBState::cancel(){}

/**
 * Calculates the effects of an attack.
 * @note After the animation is done the real explosion/hit takes place here!
 * @note This function passes to TileEngine::explode() or TileEngine::hit()
 * depending on if it came from a bullet/psi/melee/spit or an actual explosion;
 * that is "explode" here means "attack has happened". Typically called from
 * either ProjectileFlyBState::think() or
 * BattlescapeGame::endTurn()/checkProxyGrenades()
 */
void ExplosionBState::explode() // private.
{
	//Log(LOG_INFO) << ". explode()";
	const RuleItem* itRule;
	if (_item != nullptr)
	{
		itRule = _item->getRules();
		if (itRule->getBattleType() == BT_PSIAMP)
		{
			_parent->popState();
			return;
		}
	}
	else
		itRule = nullptr;

	// Note: melee Hit success/failure, and hit/miss sound-FX, are determined in ProjectileFlyBState.

	if (_melee == true)
	{
		_parent->getTacticalAction()->type = BA_NONE;

		if (_unit != nullptr)
		{
			if (_unit->isOut_t() == false)
			{
				_unit->aim(false);
				_unit->clearCache();
			}

			if (_unit->getGeoscapeSoldier() != nullptr
				&& _unit->isMindControlled() == false)
			{
				const BattleUnit* const targetUnit (_battleSave->getTile(Position::toTileSpace(_centerVoxel))->getTileUnit());
				if (targetUnit != nullptr && targetUnit->getFaction() != FACTION_PLAYER)
				{
					int xpMelee;
					if (_meleeSuccess == true)
						xpMelee = 2;
					else
						xpMelee = 1;

					_unit->addMeleeExp(xpMelee);
				}
			}
		}

		if (_meleeSuccess == false) // MISS.
		{
			_parent->checkExposedByMelee(_unit); // determine whether playerFaction-attacker gets exposed.
			_parent->getMap()->cacheUnits();
			_parent->popState();
			return;
		}
	}

	TileEngine* const te (_battleSave->getTileEngine());

	if (itRule != nullptr)
	{
		if (_unit == nullptr && _item->getPriorOwner() != nullptr)
			_unit = _item->getPriorOwner();

		if (_areaOfEffect == true)
		{
			//Log(LOG_INFO) << "ExplosionBState::explode() AoE te::explode";
//			te->setProjectileDirection(-1);
			te->explode(
					_centerVoxel,
					_power,
					itRule->getDamageType(),
					itRule->getExplosionRadius(),
					_unit,
					itRule->isGrenade() == true,
					itRule->defusePulse() == true);
		}
		else
		{
			//Log(LOG_INFO) << "ExplosionBState::explode() point te::hit";
			DamageType dType;
			if (_buttHurt == true)
				dType = DT_STUN;
			else
				dType = itRule->getDamageType();

			te->hit(
					_centerVoxel,
					_power,
					dType,
					_unit,
					_melee,
					itRule->getShotgunPellets() != 0,
					itRule->getZombieUnit());
		}
	}


	bool terrain;
	if (_tile != nullptr)
	{
		terrain = true;
		const DamageType dType (_tile->getExplosiveType());
		if (dType != DT_HE)
			_tile->setExplosive(0, DT_NONE, true);

		te->explode(
				_centerVoxel,
				_power,
				dType,
				_power / 10);
	}
	else if (itRule == nullptr) // explosion not caused by terrain or an item - must be a cyberdisc
	{
		terrain = true;
		int radius;
		if (_unit != nullptr && _unit->getSpecialAbility() == SPECAB_EXPLODE)
			radius = _parent->getRuleset()->getItemRule(_unit->getArmor()->getCorpseGeoscape())->getExplosionRadius();
		else
			radius = 6;

		te->explode(
				_centerVoxel,
				_power,
				DT_HE,
				radius);
	}
	else
		terrain = false;


	//Log(LOG_INFO) << "ExplosionBState::explode() CALL bg::checkCasualties()";
	_parent->checkCasualties(_item, _unit, false, terrain);

	if (itRule != nullptr && itRule->getShotgunPellets() != 0)
	{
		for (std::vector<BattleUnit*>::const_iterator
				i = _battleSave->getUnits()->begin();
				i != _battleSave->getUnits()->end();
				++i)
		{
			if ((*i)->isOut_t(OUT_HEALTH) == false)
				(*i)->hasCried(false);
		}
	}


	if (_unit != nullptr // if this hit/explosion was caused by a unit put the weapon down
		&& _unit->isOut_t(OUT_STAT) == false
		&& _lowerWeapon == true)
	{
		_unit->aim(false);
		_unit->clearCache();
	}

	_parent->getMap()->cacheUnits();
	_parent->popState();
	//Log(LOG_INFO) << ". . pop";


	if (itRule != nullptr && itRule->isGrenade() == true)
	{
		for (std::vector<BattleItem*>::const_iterator
				i = _battleSave->getItems()->begin();
				i != _battleSave->getItems()->end();
				++i)
		{
			if ((*i)->getId() == _item->getId())
			{
				_battleSave->toDeleteItem(_item);
				break;
			}
		}
	}


	Tile* const tile (te->checkForTerrainExplosions()); // check for more exploding tiles
	if (tile != nullptr)
	{
		const Position explVoxel (Position::toVoxelSpaceCentered(tile->getPosition(), 10));
		_parent->statePushFront(new ExplosionBState(
												_parent,
												explVoxel,
												nullptr,
												_unit,
												tile,
												false,
												false,
												_forceCamera));
	}
}

}
