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

#include "../Savegame/BattleUnit.h"
#include "../Savegame/SavedBattleGame.h"
#include "../Savegame/Tile.h"


namespace OpenXcom
{

/**
 * Sets up an ExplosionBState.
 * @param parent		- pointer to the BattlescapeGame
 * @param centerVoxel	- center position in voxel-space
 * @param itRule		- pointer to the weapon's rule
 * @param unit			- pointer to unit involved in the explosion (eg unit throwing the grenade, cyberdisc, etc)
 * @param tile			- pointer to tile the explosion is on (default nullptr)
 * @param lowerWeapon	- true to tell the unit causing this explosion to lower their weapon (default false)
 * @param meleeSuccess	- true if the (melee) attack was succesful (default false)
 * @param forceCamera	- forces Camera to center on the explosion (default false)
 * @param isLaunched	- true if shot by a Launcher (default false)
 */
ExplosionBState::ExplosionBState(
		BattlescapeGame* const parent,
		const Position centerVoxel,
		const RuleItem* const itRule,
		BattleUnit* const unit,
		Tile* const tile,
		bool lowerWeapon,
		bool meleeSuccess,
		bool forceCamera,
		bool isLaunched)
	:
		BattleState(parent),
		_centerVoxel(centerVoxel),
		_itRule(itRule),
		_unit(unit),
		_tile(tile),
		_lowerWeapon(lowerWeapon),
		_meleeSuccess(meleeSuccess),
		_forceCamera(forceCamera),
		_isLaunched(isLaunched),
		_battleSave(parent->getBattleSave()),
		_power(0),
		_areaOfEffect(true),
		_buttHurt(false),
		_melee(false)
//		_extend(3) // extra think-cycles before this state is allowed to Pop.
{
	//Log(LOG_INFO) << "";
	//Log(LOG_INFO) << "cTor ExplBState id-" << (_unit ? _unit->getId() : 0);
	//Log(LOG_INFO) << ". item type " << (itRule ? itRule->getType() : "NONe");
}

/**
 * Deletes this ExplosionBState.
 */
ExplosionBState::~ExplosionBState()
{
	//Log(LOG_INFO) << "dTor ExplBState";
}

/**
 * Gets the name of this BattleState.
 * @return, label of the substate
 */
std::string ExplosionBState::getBattleStateLabel() const
{
	std::ostringstream oststr;
	oststr << "ExplosionBState";
	if (_action.actor != nullptr) oststr << " id-" << _action.actor->getId();
	else oststr << " - Actor INVALID";

	return oststr.str();
}

/**
 * Initializes this ExplosionBState.
 * @note The animation and sound starts here. If the animation is finished the
 * actual effect takes place.
 */
void ExplosionBState::init()
{
	//Log(LOG_INFO) << ". init()";
	if (_itRule != nullptr)
	{
		switch (_itRule->getBattleType())
		{
			case BT_PSIAMP: // pass by. Let cTor initialization handle it. Except '_areaOfEffect' value
				_areaOfEffect = false;
				break;

			default:
				// getTacticalAction() only works for player actions: aliens cannot melee attack with rifle butts.
				_buttHurt = _unit != nullptr
						 && _unit->getFaction() == FACTION_PLAYER
						 && _itRule->getBattleType() != BT_MELEE
						 && _parent->getTacticalAction()->type == BA_MELEE;

				if (_buttHurt == true)
					_power = _itRule->getMeleePower();
				else
					_power = _itRule->getPower();
				//Log(LOG_INFO) << ". . power[1]= " << _power;

				// since melee aliens don't use a conventional weapon type use their strength instead.
				if (_unit != nullptr
					&& _itRule->isStrengthApplied() == true
					&& (_itRule->getBattleType() == BT_MELEE || _buttHurt == true))
				{
					int extraPower (_unit->getStrength() >> 1u);
					//Log(LOG_INFO) << ". . . extraPower[1]= " << extraPower;

					if (_buttHurt == true)
						extraPower >>= 1u;		// pistolwhipping adds only 1/2 extraPower.

					if (_unit->isKneeled() == true)
						extraPower >>= 1u;		// kneeled units further half extraPower.

					//Log(LOG_INFO) << ". . . extraPower[2]= " << extraPower;
					_power += RNG::generate(	// add 10% to 100% of extPower
										(extraPower + 9) / 10,
										 extraPower);
					//Log(LOG_INFO) << ". . power[2]= " << _power;
				}

				// HE, incendiary, smoke or stun bombs create AOE explosions;
				// all the rest hits one point: AP, melee (stun or AP), laser, plasma, acid
				_areaOfEffect = _buttHurt == false
							 && _itRule->getBattleType() != BT_MELEE
							 && _itRule->getExplosionRadius() != -1;
		}
	}
	else if (_tile != nullptr)
		_power = _tile->getExplosive();
	else if (_unit != nullptr // cyberdiscs!!! And ... ZOMBIES.
		&& _unit->getSpecialAbility() == SPECAB_EXPLODE)
	{
		_power = _parent->getRuleset()->getItemRule(_unit->getArmor()->getCorpseGeoscape())->getPower();
		const int
			power1 ((_power << 1u) / 3),
			power2 ((_power * 3) >> 1u);
		_power = RNG::generate(power1, power2)
			   + RNG::generate(power1, power2);
		_power >>= 1u;
	}
	else // unhandled cyberdisc!!!
	{
		_power = RNG::generate(67, 137)
			   + RNG::generate(67, 137);
		_power >>= 1u;
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
			if (_itRule != nullptr)
			{
				if (_itRule->defusePulse() == true)
					_parent->getMap()->setBlastFlash(true);

				interval = static_cast<Uint32>(
						   std::max(1,
									static_cast<int>(BattlescapeState::STATE_INTERVAL_EXPLOSION) - _itRule->getExplosionSpeed()));

				aniStart = _itRule->getFireHitAnimation();
				radius = std::max(0, _itRule->getExplosionRadius());

				switch (_itRule->getDamageType())
				{
					case DT_SMOKE:
					case DT_STUN:
						qty = (qty << 1u) / 3; // smoke & stun bombs do fewer anims.
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
					_parent->getMap()->getExplosions()->push_back(new Explosion(
																			ET_AOE,
																			explVoxel - Position(16,16,0), // jog downward on the screen.
																			aniStart,
																			aniDelay));
					_parent->setStateInterval(interval);
				}
			}


			int soundId;
			if (_itRule != nullptr)
				soundId = _itRule->getFireHitSound();
			else if (_power < 73)
				soundId = static_cast<int>(ResourcePack::SMALL_EXPLOSION);
			else
				soundId = static_cast<int>(ResourcePack::LARGE_EXPLOSION);

			if (soundId != -1)
				_parent->getResourcePack()->getSound("BATTLE.CAT", static_cast<unsigned>(soundId))
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
			_parent->popBattleState();
	}
	else // create a bullet hit, or melee hit, or psi-hit, or acid spit hit
	{
		ExplosionType
			explType_att,
			explType_hit;
		int
			soundId,
			aniStart_att,
			aniStart_hit;

		_melee = _buttHurt
			  || _itRule->getBattleType() == BT_MELEE
			  || _itRule->getBattleType() == BT_PSIAMP;

		if (_melee == true)
		{
			switch (_itRule->getBattleType())
			{
				default:
					explType_att = ET_MELEE_ATT;
					explType_hit = ET_MELEE_HIT;

					if (_buttHurt == true)
					{
						aniStart_att = -1;
						if (_meleeSuccess == true)
						{
							soundId = _itRule->getMeleeHitSound();
							aniStart_hit = _itRule->getMeleeHitAnimation();
						}
						else
							soundId = aniStart_hit = -1;
					}
					else
					{
						soundId = _itRule->getMeleeHitSound();
						aniStart_att = _itRule->getMeleeAnimation();
						if (_meleeSuccess == true)
							aniStart_hit = _itRule->getMeleeHitAnimation();
						else
							aniStart_hit = -1;
					}
					break;

				case BT_PSIAMP:
					explType_hit = explType_att = ET_PSI;
					soundId = _itRule->getMeleeHitSound();
					aniStart_hit = _itRule->getMeleeHitAnimation();
					aniStart_att = -1;
			}
		}
		else
		{
			soundId = _itRule->getFireHitSound();
			aniStart_hit = _itRule->getFireHitAnimation();
			aniStart_att = -1;

			if (_itRule->getType() == "STR_FUSION_TORCH_POWER_CELL")
				explType_hit = explType_att = ET_TORCH;
			else
				explType_hit = explType_att = ET_BULLET;
		}

		if (soundId != -1)
			_parent->getResourcePack()->getSound("BATTLE.CAT", static_cast<unsigned>(soundId))
										->play(-1, _parent->getMap()->getSoundAngle(posTarget));

		if (aniStart_att != -1 || aniStart_hit != -1)
		{
			if (aniStart_att != -1) // TODO: Move create Explosion for start-swing to ProjectileFlyBState::performMeleeAttack().
				_parent->getMap()->getExplosions()->push_back(new Explosion(
																		explType_att,
																		_centerVoxel,
																		aniStart_att));

			if (aniStart_hit != -1)
				_parent->getMap()->getExplosions()->push_back(new Explosion(
																		explType_hit,
																		_centerVoxel,
																		aniStart_hit));

			Uint32 interval (static_cast<Uint32>(
							 std::max(1,
									  static_cast<int>(BattlescapeState::STATE_INTERVAL_EXPLOSION) - _itRule->getExplosionSpeed())));
			_parent->setStateInterval(interval);
		}

		Camera* const exploCam (_parent->getMap()->getCamera());
		if (_forceCamera == true
			|| (exploCam->isOnScreen(posTarget) == false
				&& (_battleSave->getSide() != FACTION_PLAYER
					|| _itRule->getBattleType() != BT_PSIAMP))
			|| (_battleSave->getSide() != FACTION_PLAYER
				&& _itRule->getBattleType() == BT_PSIAMP))
		{
			exploCam->centerOnPosition(posTarget, false);
		}
		else if (exploCam->getViewLevel() != posTarget.z)
			exploCam->setViewLevel(posTarget.z);
	}
}

/**
 * Animates this ExplosionBState's battlefield sprites.
 * @note The ExplosionBState has multiple Explosion-objects. If an animation is
 * finished remove it from the list. If the list is empty this state is finished
 * and the actual calculations take place in explode() and TileEngine.
// * kL_rewrite: Allow a few extra cycles for explosion animations to dissipate.
 */
void ExplosionBState::think()
{
//	for (std::list<Explosion*>::const_iterator
//			i = _parent->getMap()->getExplosions()->begin();
//			i != _parent->getMap()->getExplosions()->end();
//			)
//	{
//		if ((*i)->animate() == false)
//		{
//			delete *i;
//			i = _parent->getMap()->getExplosions()->erase(i);
//		}
//		else ++i;
//	}
//
//	if (_parent->getMap()->getExplosions()->empty() == true)
//		--_extend; // not working as intended; needs to go to Explosion class so that explosions-vector doesn't 'empty' so fast.
//
//	if (_extend < 1) explode();

	//Log(LOG_INFO) << "ExplosionBState::think()";
	if (_parent->getMap()->getBlastFlash() == false)
	{
		std::list<Explosion*>* const explList (_parent->getMap()->getExplosions());
		//Log(LOG_INFO) << ". expl qty= " << explList->size();

		for (std::list<Explosion*>::const_iterator
				i = explList->begin();
				i != explList->end();
				)
		{
			//Log(LOG_INFO) << ". . iterate";
			if ((*i)->animate() == false) // done gFx.
			{
				//Log(LOG_INFO) << ". . . Done";
				delete *i;
				i = explList->erase(i);
			}
			else
				++i;
		}

		if (explList->empty() == true)
		{
			//Log(LOG_INFO) << ". . final explode()";
			explode();
		}
	}
}

/**
 * Explosions cannot be cancelled.
 */
//void ExplosionBState::cancel(){}

/**
 * Calculates the effects of an attack by this ExplosionBState.
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
	if (_itRule != nullptr && _itRule->getBattleType() == BT_PSIAMP)
	{
		_parent->popBattleState();
		return;
	}


	TileEngine* const te (_battleSave->getTileEngine());
	bool isTerrain;

	if (_itRule != nullptr)
	{
		isTerrain = false;

		if (_areaOfEffect == true)
		{
			//Log(LOG_INFO) << "ExplosionBState::explode() AoE te::explode";
//			te->setProjectileDirection(-1);
			te->explode(
					_centerVoxel,
					_power,
					_itRule->getDamageType(),
					_itRule->getExplosionRadius(),
					_unit,
					_itRule->isGrenade() == true,
					_itRule->defusePulse() == true,
					_isLaunched);
		}
		else
		{
			//Log(LOG_INFO) << "ExplosionBState::explode() point te::hit";
			// NOTE: melee Hit success/failure, and hit/miss sound-FX, are determined in ProjectileFlyBState.
			if (_melee == true)
			{
				_parent->getTacticalAction()->type = BA_NONE;

				if (_unit != nullptr)
				{
					if (_unit->isOut_t() == false)
					{
						_unit->aim(false);
//						_unit->flagCache();
					}

					if (_unit->getGeoscapeSoldier() != nullptr
						&& _unit->isMindControlled() == false)
					{
						const BattleUnit* const targetUnit (_battleSave->getTile(Position::toTileSpace(_centerVoxel))->getTileUnit());
						if (targetUnit != nullptr) // safety.
						{
							switch (targetUnit->getFaction())
							{
								case FACTION_HOSTILE:
									if (_meleeSuccess == true)
										_unit->addMeleeExp(2);
									else
										_unit->addMeleeExp(1);
									break;

								case FACTION_PLAYER:
								case FACTION_NEUTRAL:
									if (_meleeSuccess == true)
										_unit->addMeleeExp(1);
							}
						}
					}
				}

				if (_meleeSuccess == false) // MISS.
				{
					_parent->checkExposedByMelee(_unit); // determine whether playerFaction-attacker gets exposed.
					_parent->getMap()->cacheUnits();
					_parent->popBattleState();
					return;
				}
			}

			DamageType dType;
			if (_buttHurt == true)
				dType = DT_STUN;
			else
				dType = _itRule->getDamageType();

			te->hit(
					_centerVoxel,
					_power,
					dType,
					_unit,
					_melee,
					_itRule->getShotgunPellets() != 0,
					_itRule->getZombieUnit());
		}
	}
	else if (_tile != nullptr)
	{
		isTerrain = true;
		const DamageType dType (_tile->getExplosiveType());
		if (dType != DT_HE)
			_tile->clearExplosive();

		te->explode(
				_centerVoxel,
				_power,
				dType,
				_power / 10);
	}
	else // explosion not caused by terrain or an item - must be a cyberdisc or burning zombie.
	{
		isTerrain = true;
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

	//Log(LOG_INFO) << "ExplosionBState::explode() CALL bg::checkCasualties()";
	_parent->checkCasualties(
						_itRule,
						_unit,
						false,
						isTerrain);

	if (_itRule != nullptr && _itRule->getShotgunPellets() != 0)
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

	if (_lowerWeapon == true // if this hit/explosion was caused by a unit put the weapon down
		&& _unit != nullptr
		&& _unit->isOut_t(OUT_STAT) == false)
	{
		_unit->aim(false);
//		_unit->flagCache();
	}

	_parent->getMap()->cacheUnits();
	_parent->popBattleState();
	//Log(LOG_INFO) << ". . pop";


	Tile* const tile (te->checkForTerrainExplosives()); // check for more exploding tiles
	if (tile != nullptr)
	{
		const Position explVoxel (Position::toVoxelSpaceCentered(tile->getPosition(), 10));
		_parent->stateBPushFront(new ExplosionBState(
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
