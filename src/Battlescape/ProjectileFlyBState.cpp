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

#include "ProjectileFlyBState.h"

//#include <algorithm>

#include "../fmath.h"

#include "AlienBAIState.h"
#include "BattlescapeState.h"
#include "Camera.h"
#include "Explosion.h"
#include "ExplosionBState.h"
#include "Map.h"
#include "Pathfinding.h"
#include "Projectile.h"
#include "TileEngine.h"

//#include "../Engine/Logger.h"
//#include "../Engine/Options.h"
#include "../Engine/RNG.h"
#include "../Engine/Sound.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/RuleArmor.h"
#include "../Ruleset/RuleItem.h"

#include "../Savegame/BattleItem.h"
//#include "../Savegame/BattleUnit.h"
#include "../Savegame/BattleUnitStatistics.h"
#include "../Savegame/SavedBattleGame.h"
#include "../Savegame/Tile.h"


namespace OpenXcom
{

/**
 * Sets up a ProjectileFlyBState.
 * @param parent	- pointer to the BattlescapeGame
 * @param action	- the current BattleAction (BattlescapeGame.h)
 * @param posOrigin	- origin in tile-space (default Position(0,0,-1))
 */
ProjectileFlyBState::ProjectileFlyBState(
		BattlescapeGame* const parent,
		BattleAction action,
		Position posOrigin)
	:
		BattleState(parent, action),
		_posOrigin(posOrigin),
		_battleSave(parent->getBattleSave()),
		_originVoxel( 0, 0,-1),
		_targetVoxel(-1,-1,-1),
		_forced(false),
		_unit(nullptr),
		_load(nullptr),
		_prjItem(nullptr),
		_prjImpact(VOXEL_FLOOR),
		_prjVector(0,0,-1),
		_initialized(false),
		_targetFloor(false),
		_initUnitAni(0)
{
	if (_posOrigin.z == -1)
		_posOrigin = action.actor->getPosition();
}

/**
 * Deletes the ProjectileFlyBState.
 */
ProjectileFlyBState::~ProjectileFlyBState()
{}

/**
 * Initializes the sequence:
 * - checks if the shot is valid
 * - determines the target voxel
 */
void ProjectileFlyBState::init()
{
	//Log(LOG_INFO) << "ProjectileFlyBState::init()";
	if (_initialized == true) return;

	//Log(LOG_INFO) << "projFlyB init() targetPosTile = " << _action.target;
	_initialized = true;
	_parent->getTacticalAction()->takenXp = false;

	_unit = _action.actor;

	bool popThis (false);

	if (_unit->isOut_t() == true
		|| _action.weapon == nullptr
		|| _battleSave->getTile(_action.posTarget) == nullptr)
	{
		popThis = true;
	}
	else if (_unit->getTimeUnits() >= _action.TU // go ->
		|| _action.type == BA_MELEE
		|| _parent->playerPanicHandled() == false
		|| _unit->getFaction() != FACTION_PLAYER)
	{
		_load = _action.weapon->getAmmoItem();

		bool fireValid;
		if (_unit->getFaction() != _battleSave->getSide()) // reaction fire
		{
			const BattleUnit* const targetUnit (_battleSave->getTile(_action.posTarget)->getTileUnit());
			fireValid = targetUnit != nullptr
					 && targetUnit->isOut_t() == false
					 && targetUnit == _battleSave->getSelectedUnit()
					 && _load != nullptr;
		}
		else
			fireValid = true;

		if (fireValid == false || _unit->getStopShot() == true)
		{
			_unit->setTimeUnits(_unit->getTimeUnits() + _action.TU);
			popThis = true;
		}
	}
	else // no TU.
	{
		_action.result = "STR_NOT_ENOUGH_TIME_UNITS";
		popThis = true;
	}

	if (popThis == true)
	{
		_unit->setStopShot(false);
		_parent->popState();
		return;
	}


	// autoshot will default back to snapshot if it's not possible
	// This shouldn't happen w/ chooseFireMethod() properly in place.
	if (_action.type == BA_AUTOSHOT
		&& _action.weapon->getRules()->getAccuracyAuto() == 0)
	{
		_action.type = BA_SNAPSHOT;
	}
	// Except that Berserk tries to use SnapShot .... needs looking at.


	// snapshot defaults to "hit" if it's a melee weapon (in case of reaction
	// with a melee weapon) for Silacoid attack etc.
	if (_action.weapon->getRules()->getBattleType() == BT_MELEE)
	{
		//Log(LOG_INFO) << ". convert shotType to BA_MELEE";
		switch (_action.type)
		{
			case BA_SNAPSHOT:
			case BA_AUTOSHOT:
			case BA_AIMEDSHOT:
				_action.type = BA_MELEE;
		}
	}

	switch (_action.type)
	{
		case BA_SNAPSHOT:
		case BA_AUTOSHOT:
		case BA_AIMEDSHOT:
		case BA_LAUNCH:
			//Log(LOG_INFO) << ". . BA_SNAPSHOT, AIMEDSHOT, AUTOSHOT, or LAUNCH";
			if (_load == nullptr)
			{
				//Log(LOG_INFO) << ". . . no ammo, EXIT";
				_action.result = "STR_NO_AMMUNITION_LOADED";
				popThis = true;
			}
//			else if (_load->getAmmoQuantity() == 0)
//			{
//				//Log(LOG_INFO) << ". . . no ammo Quantity, EXIT";
//				_action.result = "STR_NO_ROUNDS_LEFT";
//				popThis = true;
//			}
			else if (TileEngine::distance(
									_unit->getPosition(),
									_action.posTarget) > _action.weapon->getRules()->getMaxRange())
			{
				//Log(LOG_INFO) << ". . . out of range, EXIT";
				_action.result = "STR_OUT_OF_RANGE";
				popThis = true;
			}
			break;

		case BA_THROW:
		{
			//Log(LOG_INFO) << ". . BA_THROW panic = " << (int)(_parent->playerPanicHandled() == false);
			const Tile* const tileTarget (_battleSave->getTile(_action.posTarget)); // always Valid.
			if (TileEngine::validThrowRange(
										&_action,
										_parent->getTileEngine()->getOriginVoxel(_action),
										tileTarget) == true)
			{
				_prjItem = _action.weapon;
				if (tileTarget->getTerrainLevel() == -24
					&& tileTarget->getPosition().z < _battleSave->getMapSizeZ() - 1)
				{
					++_action.posTarget.z;
				}
			}
			else
			{
				//Log(LOG_INFO) << ". . . not valid throw range, EXIT";
				_action.result = "STR_OUT_OF_RANGE";
				popThis = true;
			}
			break;
		}

		case BA_MELEE:
			performMeleeAttack();
			//Log(LOG_INFO) << ". . BA_MELEE performMeleeAttack() DONE - EXIT flyBState::init()";
			return;

		case BA_PSIPANIC:
		case BA_PSICONTROL:
		case BA_PSICONFUSE:
		case BA_PSICOURAGE:
			//Log(LOG_INFO) << ". . BA_PSIPANIC/CONTROL/CONFUSE/COURAGE, new ExplosionBState - EXIT flyBState::init()";
			_parent->statePushFront(new ExplosionBState(
													_parent,
													Position::toVoxelSpaceCentered(_action.posTarget, 10),
													_action.weapon,
													_unit));
			return;

		default:
			//Log(LOG_INFO) << ". . default, EXIT";
			popThis = true;
	}

	if (popThis == true)
	{
		_parent->popState();
		return;
	}


	// ** find TARGET voxel ** ->
	const Tile* const tileTarget ( _battleSave->getTile(_action.posTarget));
	_targetVoxel = Position::toVoxelSpace(_action.posTarget);
	//Log(LOG_INFO) << "FlyB init targetVoxel " << _targetVoxel;

	if (_action.type == BA_THROW || _action.type == BA_LAUNCH)
	{
		//Log(LOG_INFO) << "projFlyB init() B-Launch OR Throw";
		_targetVoxel.x += 8;
		_targetVoxel.y += 8;

		if (_action.type == BA_THROW)
			_targetVoxel.z += 2 - tileTarget->getTerrainLevel(); // LoFT of floor is typically 2 voxels thick.
		else if (_targetFloor == false)
			_targetVoxel.z += 16;
	}
	else if ((_unit->getFaction() == FACTION_PLAYER	// force fire at center of Tile by pressing [CTRL] but *not* SHIFT
			&& (SDL_GetModState() & KMOD_CTRL) != 0	// force fire at Floor w/ [CTRL+ALT]
			&& (SDL_GetModState() & KMOD_SHIFT) == 0
			&& Options::battleForceFire == true)
		|| _parent->playerPanicHandled() == false) // note that nonPlayer berserk bypasses this and targets according to targetUnit OR tileParts below_
	{
		//Log(LOG_INFO) << "projFlyB init() Player panic OR Ctrl [!Shift]";
		_targetVoxel.x += 8; // force fire at floor w/ Alt
		_targetVoxel.y += 8;

		if ((SDL_GetModState() & KMOD_ALT) == 0
			|| _parent->playerPanicHandled() == false)
		{
			_targetVoxel.z += 10;
		}
	}
	else
	{
		// determine the target voxel.
		// aim at (in this priority)
		//		- the center of the targetUnit, or the floor if target=origin
		//		- the object
		//		- the northwall
		//		- the westwall
		//		- the floor
		// if there is no LoF to the center canTarget*() tries moving the target-voxel outward.
		// Store that voxel.
		//
		// Force Fire keyboard modifiers:
		// note non-Player units cannot target tileParts ... but they might someday.
		// none			- See above^
		// CTRL			- center
		// CTRL+ALT		- floor
		// SHIFT		- northwall
		// SHIFT+CTRL	- westwall
		const Position originVoxel (_parent->getTileEngine()->getOriginVoxel(
																		_action,
																		_battleSave->getTile(_posOrigin)));
		if (tileTarget->getTileUnit() != nullptr
			&& (_unit->getFaction() != FACTION_PLAYER
				|| (((SDL_GetModState() & KMOD_SHIFT) == 0
						&& (SDL_GetModState() & KMOD_CTRL) == 0)
					|| Options::battleForceFire == false)))
		{
			//Log(LOG_INFO) << ". tileTarget has unit";
			if (_action.posTarget == _posOrigin
				|| tileTarget->getTileUnit() == _unit)
			{
				//Log(LOG_INFO) << "projFlyB targetPos[2] = " << _action.target;
				_targetVoxel.x += 8; // don't shoot yourself but shoot at the floor
				_targetVoxel.y += 8;
				_targetVoxel.z += 2;
				//Log(LOG_INFO) << "projFlyB targetVoxel[2] = " << _targetVoxel;
			}
			else
			{
				_parent->getTileEngine()->canTargetUnit( // <- this is a normal shot by xCom or aLiens.
													&originVoxel,
													tileTarget,
													&_targetVoxel,
													_unit,
													nullptr,
													&_forced);
				//Log(LOG_INFO) << ". canTargetUnit() targetVoxel " << _targetVoxel << " targetTile " << Position::toTileSpace(_targetVoxel);
			}
		}
		else if (tileTarget->getMapData(O_OBJECT) != nullptr	// force vs. Object by using CTRL above^
			&& (_unit->getFaction() != FACTION_PLAYER			// bypass Object by pressing SHIFT
				|| (SDL_GetModState() & KMOD_SHIFT) == 0
				|| Options::battleForceFire == false))
		{
			//Log(LOG_INFO) << ". tileTarget has content-object";
			if (tileTarget->isRevealed(ST_CONTENT) == false
				|| _parent->getTileEngine()->canTargetTilepart(
														&originVoxel,
														tileTarget,
														O_OBJECT,
														&_targetVoxel,
														_unit) == false)
			{
				_targetVoxel = Position::toVoxelSpace(_action.posTarget);
				_targetVoxel.x += 8;
				_targetVoxel.y += 8;
				_targetVoxel.z += 10;
			}
		}
		else if (tileTarget->getMapData(O_NORTHWALL) != nullptr // force Northwall by pressing [SHIFT] but not CTRL
			&& (_unit->getFaction() != FACTION_PLAYER
				|| (SDL_GetModState() & KMOD_CTRL) == 0
				|| Options::battleForceFire == false))
		{
			//Log(LOG_INFO) << ". tileTarget has northwall";
			if (tileTarget->isRevealed(ST_NORTH) == false
				|| _parent->getTileEngine()->canTargetTilepart(
														&originVoxel,
														tileTarget,
														O_NORTHWALL,
														&_targetVoxel,
														_unit) == false)
			{
				_targetVoxel = Position::toVoxelSpace(_action.posTarget);
				_targetVoxel.x += 8;
				_targetVoxel.y += 2;
				_targetVoxel.z += 10;
			}
		}
		else if (tileTarget->getMapData(O_WESTWALL) != nullptr) // force Westwall by pressing [CTRL+SHIFT]
		{
			//Log(LOG_INFO) << ". tileTarget has westwall";
			if (tileTarget->isRevealed(ST_WEST) == false
				|| _parent->getTileEngine()->canTargetTilepart(
														&originVoxel,
														tileTarget,
														O_WESTWALL,
														&_targetVoxel,
														_unit) == false)
			{
				_targetVoxel = Position::toVoxelSpace(_action.posTarget);
				_targetVoxel.x += 2;
				_targetVoxel.y += 8;
				_targetVoxel.z += 10;
			}
		}
		else if (tileTarget->getMapData(O_FLOOR) != nullptr) // forced-shot at Floor is handled above^ [CTRL+ALT]
		{
			//Log(LOG_INFO) << ". tileTarget has floor";
			if (tileTarget->isRevealed(ST_CONTENT) == false
				|| _parent->getTileEngine()->canTargetTilepart(
														&originVoxel,
														tileTarget,
														O_FLOOR,
														&_targetVoxel,
														_unit) == false)
			{
				_targetVoxel = Position::toVoxelSpace(_action.posTarget);
				_targetVoxel.x += 8;
				_targetVoxel.y += 8;
				_targetVoxel.z += 2;
			}
		}
		else // target nothing, targets the middle of the tile
		{
			//Log(LOG_INFO) << ". tileTarget is void";
			_targetVoxel.x += 8;
			_targetVoxel.y += 8;
			_targetVoxel.z += 10;
		}
	}

	//Log(LOG_INFO) << "FlyB final targetVoxel " << _targetVoxel;
	//Log(LOG_INFO) << "projFlyB init() targetPosVoxel.x = " << static_cast<float>(_targetVoxel.x) / 16.f;
	//Log(LOG_INFO) << "projFlyB init() targetPosVoxel.y = " << static_cast<float>(_targetVoxel.y) / 16.f;
	//Log(LOG_INFO) << "projFlyB init() targetPosVoxel.z = " << static_cast<float>(_targetVoxel.z) / 24.f;

	if (createProjectile() == true)
	{
		_parent->getMap()->setSelectorType(CT_NONE); // might be already done in primaryAction()
		_parent->getMap()->getCamera()->stopMouseScrolling();
	}
	//Log(LOG_INFO) << "ProjectileFlyBState::init() EXIT";
}

/**
 * Tries to create a projectile sprite and add it to the map.
 * Calculate its trajectory.
 * @return, true if the projectile was successfully created
 */
bool ProjectileFlyBState::createProjectile() // private.
{
	//Log(LOG_INFO) << "ProjectileFlyBState::createProjectile()";
	//Log(LOG_INFO) << ". _action_type = " << _action.type;
	if (++_action.autoShotCount == 1
		&& _unit->getGeoscapeSoldier() != nullptr)
	{
		switch (_action.type)
		{
			case BA_SNAPSHOT:
			case BA_AUTOSHOT:
			case BA_AIMEDSHOT:
				++_unit->getStatistics()->shotsFiredCounter;
		}
	}

	//Log(LOG_INFO) << "projFlyB create() originTile = " << _posOrigin;
	//Log(LOG_INFO) << "projFlyB create() targetVoxel = " << _targetVoxel;
	//Log(LOG_INFO) << "projFlyB create() targetVoxel.x = " << static_cast<float>(_targetVoxel.x) / 16.f;
	//Log(LOG_INFO) << "projFlyB create() targetVoxel.y = " << static_cast<float>(_targetVoxel.y) / 16.f;
	//Log(LOG_INFO) << "projFlyB create() targetVoxel.z = " << static_cast<float>(_targetVoxel.z) / 24.f;

	Projectile* const prj (new Projectile(
									_parent->getResourcePack(),
									_battleSave,
									_action,
									_posOrigin,
									_targetVoxel));
	_parent->getMap()->setProjectile(prj); // add projectile to Map.


	//Log(LOG_INFO) << "projFlyB: createProjectile() set interval = " << BattlescapeState::STATE_INTERVAL_FAST;
	_parent->setStateInterval(BattlescapeState::STATE_INTERVAL_FAST); // set the speed of the state think cycle to 16 ms (roughly one think-cycle per frame)
	int soundId (-1);

	_prjImpact = VOXEL_EMPTY; // let it calculate a trajectory

	if (_action.type == BA_THROW)
	{
		//Log(LOG_INFO) << ". call Projectile::calculateThrow()";
		//Log(LOG_INFO) << "";
		_prjImpact = prj->calculateThrow(_unit->getAccuracy(_action)); // this should probly be TE:validateThrow() - cf. else(error) below_
		//Log(LOG_INFO) << ". BA_THROW, part = " << MapData::debugVoxelType(_prjImpact);

		switch (_prjImpact)
		{
			case VOXEL_FLOOR:
			case VOXEL_OBJECT:
			case VOXEL_UNIT:
				//Log(LOG_INFO) << ". . VALID";
				if (_unit->getFaction() != FACTION_PLAYER
					&& _prjItem->getRules()->isGrenade() == true)
				{
					//Log(LOG_INFO) << ". . auto-prime for AI, unitID " << _unit->getId();
					_prjItem->setFuse(0);
				}

				_prjItem->changeOwner();
				_unit->flagCache();
				_parent->getMap()->cacheUnit(_unit);

				soundId = ResourcePack::ITEM_THROW;

				if (_unit->getGeoscapeSoldier() != nullptr
					&& _unit->isMindControlled() == false
					&& _parent->playerPanicHandled() == true)
				{
					_unit->addThrowingExp();
				}
				break;

			default: // unable to throw here
				// Note that BattleUnit accuracy^ should *not* be considered before this.
				// Unless this is some sort of failsafe/exploit for the AI ... no it's
				// just the fucko-spaghetti-like code that's used throughout. /shrug
				//Log(LOG_INFO) << ". . NOT Valid";
				//Log(LOG_INFO) << ". . no throw, Voxel_Empty or _Wall or _OutofBounds";
				delete prj;
				_parent->getMap()->setProjectile();

				_action.result = "STR_UNABLE_TO_THROW_HERE";
				_action.TU = 0;
				_unit->setUnitStatus(STATUS_STANDING);

				_parent->popState();
				return false;
		}
	}
	else if (_action.weapon->getRules()->isArcingShot() == true) // special code for the "spit" trajectory
	{
		_prjImpact = prj->calculateThrow(_unit->getAccuracy(_action)); // this should probly be TE:validateThrow() - cf. else(error) below_
		//Log(LOG_INFO) << ". acid spit, part = " << (int)_prjImpact;

		if (_prjImpact != VOXEL_OUTOFBOUNDS
			&& (_prjImpact != VOXEL_EMPTY
				|| _load->getRules()->getExplosionRadius() != -1)) // <- midair explosion
		{
			//Log(LOG_INFO) << ". . spit/arcing shot";
			if (_prjImpact == VOXEL_OBJECT
				&& _load->getRules()->getExplosionRadius() != -1)
			{
				const Tile* const tile (_battleSave->getTile(_parent->getMap()->getProjectile()->getFinalPosition()));
//				if (tile != nullptr && tile->getMapData(O_OBJECT) != nullptr) // safety. Should be unnecessary because _prjImpact=VOXEL_OBJECT ....
				{
					switch (tile->getMapData(O_OBJECT)->getBigwall())
					{
						case BIGWALL_NESW:
						case BIGWALL_NWSE:
//							prj->storeProjectileDirection();		// Used to handle direct-explosive-hits against diagonal bigWalls.
							_prjVector = prj->getStrikeVector();	// ^supercedes above^ storeProjectileDirection()
					}
				}
			}

			_load->spendBullet(
							*_battleSave,
							*_action.weapon);

			_unit->startAiming();
			_unit->flagCache();
			_parent->getMap()->cacheUnit(_unit);

			// lift-off
			soundId = _load->getRules()->getFireSound();
			if (soundId == -1)
				soundId = _action.weapon->getRules()->getFireSound();
		}
		else // no line of fire; Note that BattleUnit accuracy^ should *not* be considered before this. Unless this is some sort of failsafe/exploit for the AI ...
		{
			//Log(LOG_INFO) << ". . no spit, no LoF, Voxel_Empty";
			delete prj;
			_parent->getMap()->setProjectile();

			_action.result = "STR_NO_LINE_OF_FIRE";
			_action.TU = 0;
			_unit->setUnitStatus(STATUS_STANDING);

			_parent->popState();
			return false;
		}
	}
	else // shoot weapon
	{
		if (_originVoxel.z != -1) // origin is a BL waypoint
		{
			_prjImpact = prj->calculateShot( // this should probly be TE:plotLine() - cf. else(error) below_
										_unit->getAccuracy(_action),
										_originVoxel,
										false); // <- don't consider excludeUnit if a BL-waypoint
			//Log(LOG_INFO) << ". shoot weapon[0], voxelType = " << (int)_prjImpact;
		}
		else // non-BL weapon
		{
			if (_forced == true) prj->setForced();
			_prjImpact = prj->calculateShot(_unit->getAccuracy(_action)); // this should probly be TE:plotLine() - cf. else(error) below_
			//Log(LOG_INFO) << ". shoot weapon[1], voxelType = " << (int)_prjImpact;
			//Log(LOG_INFO) << "prjFlyB accuracy = " << _unit->getAccuracy(_action);
		}
		//Log(LOG_INFO) << ". shoot weapon, voxelType = " << (int)_prjImpact;
		//Log(LOG_INFO) << ". finalTarget = " << prj->getFinalPosition();

		if (_prjImpact != VOXEL_EMPTY || _action.type == BA_LAUNCH)
		{
			//Log(LOG_INFO) << ". . _prjImpact AIM";
			if (_prjImpact == VOXEL_OBJECT
				&& _load->getRules()->getExplosionRadius() > 0)
			{
				const Tile* const tile (_battleSave->getTile(_parent->getMap()->getProjectile()->getFinalPosition()));
//				if (tile != nullptr && tile->getMapData(O_OBJECT) != nullptr) // safety. Should be unnecessary because _prjImpact=VOXEL_OBJECT ....
				{
					switch (tile->getMapData(O_OBJECT)->getBigwall())
					{
						case BIGWALL_NESW:
						case BIGWALL_NWSE:
//							prj->storeProjectileDirection();		// Used to handle direct-explosive-hits against diagonal bigWalls.
							_prjVector = prj->getStrikeVector();	// ^supercedes storeProjectileDirection() above^
					}
				}
			}

			switch (_action.type)
			{
				case BA_LAUNCH:
					_parent->getMap()->setReveal();	// Reveal the Map until action completes.
					break;							// Launch spends bullet ... elsewhere.

				case BA_AUTOSHOT:
					_parent->getMap()->setReveal(); // no break;
				default:
					_load->spendBullet(
									*_battleSave,
									*_action.weapon);
			}

			if (_originVoxel.z == -1) // not a BL-waypoint
			{
				_unit->aim();
				_parent->getMap()->cacheUnit(_unit);
			}

			// lift-off
			soundId = _load->getRules()->getFireSound();
			if (soundId == -1)
				soundId = _action.weapon->getRules()->getFireSound();
		}
		else // VOXEL_EMPTY, no line of fire; Note that BattleUnit accuracy^ should *not* be considered before this. Unless this is some sort of failsafe/exploit for the AI ...
		{
			//Log(LOG_INFO) << ". no shot, no LoF, Voxel_Empty";
			delete prj;
			_parent->getMap()->setProjectile();

			_action.result = "STR_NO_LINE_OF_FIRE";
			_action.TU = 0;
			_unit->setUnitStatus(STATUS_STANDING);

			_parent->popState();
			return false;
		}
	}

	if (soundId != -1)
		_parent->getResourcePack()->getSound("BATTLE.CAT", soundId)
			->play(-1, _parent->getMap()->getSoundAngle(_unit->getPosition()));

	if (_unit->getArmor()->getShootFrames() != 0)
		_parent->getMap()->showProjectile(false); // postpone showing the Celatid spit-blob till later

	//Log(LOG_INFO) << ". createProjectile() ret TRUE";
	return true;
}

/**
 * Animates the projectile as it moves to the next point in its trajectory.
 * @note If the animation is finished the projectile sprite is removed from the
 * battlefield and this state is finished.
 */
void ProjectileFlyBState::think()
{
	//Log(LOG_INFO) << "ProjectileFlyBState::think() " << _unit->getId();
	if (_unit->getUnitStatus() == STATUS_AIMING
		&& _unit->getArmor()->getShootFrames() != 0)
	{
		if (_initUnitAni == 0)
			_initUnitAni = 1;

		_unit->keepAiming();

		if (_unit->getAimingPhase() < _unit->getArmor()->getFirePhase())
			return;
	}

	if (_initUnitAni == 1)
	{
		_initUnitAni = 2;
		_parent->getMap()->showProjectile();
	}

	_battleSave->getBattleState()->clearMouseScrollingState();

	// TODO: Store the projectile in this state instead of getting it from the map each time.
	if (_parent->getMap()->getProjectile() == nullptr)
	{
		if (_unit->isOut_t() == false
			&& _action.type == BA_AUTOSHOT
			&& _action.autoShotCount < _action.weapon->getRules()->getAutoShots()
			&& _action.weapon->getAmmoItem() != nullptr
			&& ((_battleSave->getTile(_unit->getPosition()) != nullptr
					&& _battleSave->getTile(_unit->getPosition())
						->hasNoFloor(_battleSave->getTile(_unit->getPosition() + Position(0,0,-1))) == false)
				|| _unit->getMoveTypeUnit() == MT_FLY))
		{
			createProjectile();

/*			if (_action.posCamera.z != -1) // this ought already be in Map draw, as camera follows projectile from barrel of weapon
			{
				camera->setMapOffset(_action.posCamera);
//				_parent->getMap()->invalidate();
//				_parent->getMap()->draw();
			} */
		}
		else // think() FINISH.
		{
			//Log(LOG_INFO) << "ProjectileFlyBState::think() -> finish " << _action.actor->getId();
			if (_action.actor->getFaction() != _battleSave->getSide()	// rf -> note that actionActor may not be the actual shooter,
				&& Options::battleSmoothCamera == true)					// but he/she will be on the same Side, doing a reaction shot.
			{
				//Log(LOG_INFO) << "reset Camera for " << _action.actor->getId();
				const std::map<int, Position>* const rfShotPos (_battleSave->getTileEngine()->getRfShooterPositions());
				std::map<int, Position>::const_iterator i (rfShotPos->find(_action.actor->getId()));

				//for (std::map<int, Position>::const_iterator j = rfShotPos->begin(); j != rfShotPos->end(); ++j)
				//{ Log(LOG_INFO) << ". . shotList"; Log(LOG_INFO) << ". . " << j->first << " " << j->second; }

				if (i != rfShotPos->end()) // note The shotList vector will be cleared in BattlescapeGame::think() after all BattleStates have popped.
				{
					_action.posCamera = i->second;
					//Log(LOG_INFO) << ". to " << _action.posCamera;
				}
				else
				{
					_action.posCamera.z = -1;
					//Log(LOG_INFO) << ". no reset";
				}
			}

			//Log(LOG_INFO) << ". stored posCamera " << _action.posCamera;
			//Log(LOG_INFO) << ". pauseAfterShot " << (int)camera->getPauseAfterShot();
			if (_action.posCamera.z != -1) //&& _action.waypoints.size() < 2)
			{
				//Log(LOG_INFO) << "ProjectileFlyBState::think() FINISH: posCamera was Set";
				switch (_action.type) // jump screen back to pre-shot position
				{
					case BA_THROW:
					case BA_AUTOSHOT:
					case BA_SNAPSHOT:
					case BA_AIMEDSHOT:
					{
						//Log(LOG_INFO) << "ProjectileFlyBState::think() FINISH: resetting Camera to original pos";
						Camera* const shotCam (_parent->getMap()->getCamera());
						if (shotCam->getPauseAfterShot() == true)	// TODO: move 'pauseAfterShot' to the BattleAction struct. done -> but it didn't work; i'm a numby.
//						if (_action.pauseAfterShot == true)			// note that trying to store the camera position in the BattleAction didn't work either ... double numby.
						{
							shotCam->setPauseAfterShot(false);
							if (_prjImpact != VOXEL_OUTOFBOUNDS)
							{
								//Log(LOG_INFO) << ". . delay - inBounds";
								SDL_Delay(331u); // screen-pause when shot hits target before reverting camera to shooter.
							}
							//else Log(LOG_INFO) << ". . final vox OutofBounds - do NOT pause";
						}

						//Log(LOG_INFO) << ". . reset Camera Position " << _action.actor->getId();
						shotCam->setMapOffset(_action.posCamera);
//						_action.posCamera = Position(0,0,-1); // reset.

//						_parent->getMap()->draw();
//						_parent->getMap()->invalidate();
					}
				}
			}

			switch (_action.type)
			{
				case BA_PSIPANIC:
				case BA_PSICONFUSE:
				case BA_PSICOURAGE:
				case BA_PSICONTROL:
					break;

				default:
					if (_unit->getFaction() == _battleSave->getSide()
						&& _battleSave->getUnitsFalling() == false)
					{
						//Log(LOG_INFO) << "ProjectileFlyBState::think() CALL te::checkReactionFire()";
						//	<< " id-" << _unit->getId()
						//	<< " action.type = " << _action.type
						//	<< " action.TU = " << _action.TU;
						_parent->getTileEngine()->checkReactionFire(		// note: I don't believe that smoke obscuration gets accounted
																_unit,		// for by this call if the current projectile caused cloud.
																_action.TU,	// But that's kinda ok.
																_action.type != BA_MELEE);
					}
			}

			if (_unit->isOut_t() == false && _action.type != BA_MELEE)
				_unit->setUnitStatus(STATUS_STANDING);

			_parent->popState();
		}
	}
	else // projectile VALID in motion -> ! impact !
	{
		//Log(LOG_INFO) << "ProjectileFlyBState::think() -> move Projectile";
		if (_action.type != BA_THROW
			&& _load != nullptr
			&& _load->getRules()->getShotgunPellets() != 0)
		{
			// shotgun pellets move to their terminal location instantly as fast as possible
			_parent->getMap()->getProjectile()->skipTrajectory(); // skip trajectory of 1st pellet; the rest are not even added to Map.
		}

		if (_parent->getMap()->getProjectile()->traceProjectile() == false) // cycle projectile pathing -> Finished
		{
			if (_action.type == BA_THROW)
			{
//				_parent->getMap()->resetCameraSmoothing();
				Position
					throwVoxel (_parent->getMap()->getProjectile()->getPosition(-1)), // <- beware of 'offset -1'
					pos (Position::toTileSpace(throwVoxel));

				if (pos.x > _battleSave->getMapSizeX()) // note: Bounds-checking is also done better in Projectile::applyAccuracy()
					--pos.x;

				if (pos.y > _battleSave->getMapSizeY())
					--pos.y;

				BattleItem* const throwItem (_parent->getMap()->getProjectile()->getThrowItem());
				if (throwItem->getRules()->getBattleType() == BT_GRENADE
					&& throwItem->getFuse() == 0) //&& Options::battleInstantGrenade == true // -> moved to PrimeGrenadeState (0 cannot be set w/out InstantGrenades)
				{
					_parent->statePushFront(new ExplosionBState( // it's a hot potato set to explode on contact
															_parent,
															throwVoxel,
															throwItem,
															_unit));
				}
				else
				{
					_parent->dropItem(throwItem, pos);

					if (_unit->getFaction() == FACTION_HOSTILE
						&& _prjItem->getRules()->getBattleType() == BT_GRENADE)
					{
						_parent->getTileEngine()->setDangerZone(
															pos,
															throwItem->getRules()->getExplosionRadius(),
															_unit);
					}
				}

				_parent->getResourcePack()->getSound("BATTLE.CAT", ResourcePack::ITEM_DROP)
											->play(-1, _parent->getMap()->getSoundAngle(pos));
			}
			else if (_action.type == BA_LAUNCH
				&& _prjImpact == VOXEL_EMPTY
				&& _action.waypoints.size() > 1u)
			{
				_posOrigin = _action.waypoints.front();
				_action.waypoints.pop_front();
				_action.posTarget = _action.waypoints.front();

				ProjectileFlyBState* const blasterFlyB (new ProjectileFlyBState( // launch the next projectile in the waypoint cascade
																			_parent,
																			_action,
																			_posOrigin)); // -> tilePos for BL.
				blasterFlyB->_originVoxel = _parent->getMap()->getProjectile()->getPosition(); // was (offset= -1) -> tada, fixed.
				if (_action.posTarget == _posOrigin) blasterFlyB->_targetFloor = true;

				_parent->getMap()->getCamera()->centerOnPosition(_posOrigin); // this follows BL as it hits through waypoints
				_parent->statePushNext(blasterFlyB);
			}
			else // shoot -> impact.
			{
//				_parent->getMap()->resetCameraSmoothing();
				switch (_action.type)
				{
					case BA_LAUNCH:
						_prjImpact = VOXEL_OBJECT;				// Launch explodes at final waypoint.
						_parent->getMap()->setReveal(false);	// Reveal-action is complete.
						_load->spendBullet(						// ... why not spendBullet() in createProjectile() like regular weapons ...
										*_battleSave,
										*_action.weapon);
						break;

					case BA_SNAPSHOT:
					case BA_AUTOSHOT:
					case BA_AIMEDSHOT:
					{
						BattleUnit* const shotAt (_battleSave->getTile(_action.posTarget)->getTileUnit());
						if (shotAt != nullptr
							&& shotAt->getGeoscapeSoldier() != nullptr)
						{
							++shotAt->getStatistics()->shotAtCounter; // only counts for guns, not throws or launches
						}
					}
				}

//				if (_action.type == BA_LAUNCH) //&& _load != nullptr) //&& _battleSave->getDebugTac() == false
//					_load->spendBullet(*_battleSave, *_action.weapon);

				std::vector<Position> posContacts; // stores tile-positions of all Voxel_Unit hits.

				if (_prjImpact != VOXEL_OUTOFBOUNDS) // *not* out of Map; caching will be taken care of in ExplosionBState
				{
					//Log(LOG_INFO) << "FlyB: *not* OoB";
					int trjOffset; // explosions impact not inside the voxel but two steps back;
					if (_load != nullptr
						&& _load->getRules()->getExplosionRadius() != -1
						&& _prjImpact != VOXEL_UNIT)
					{
						trjOffset = -2; // step back a bit so tileExpl isn't behind a wall.
					}
					else
						trjOffset = 0;

					Position explVoxel (_parent->getMap()->getProjectile()->getPosition(trjOffset));
					const Position pos (Position::toTileSpace(explVoxel));

					if (_prjVector.z != -1) // <- strikeVector by radial explosion vs. diagBigWall
					{
						Tile* const tileTrue (_parent->getBattlescapeState()->getSavedBattleGame()->getTile(pos));
						_parent->getTileEngine()->setTrueTile(tileTrue);

						explVoxel.x -= _prjVector.x << 4; // note there is no safety on these for OoB.
						explVoxel.y -= _prjVector.y << 4;
					}
					else
						_parent->getTileEngine()->setTrueTile();

					_parent->statePushFront(new ExplosionBState(
															_parent,
															explVoxel,
															_load,
															_unit,
															nullptr,
															_action.type != BA_AUTOSHOT // final projectile -> stop Aiming.
																|| _action.autoShotCount == _action.weapon->getRules()->getAutoShots()
																|| _action.weapon->getAmmoItem() == nullptr));

					if (_prjImpact == VOXEL_UNIT)	// note that Diary Statistics require direct hit by an explosive
					{								// projectile for it to be considered as a 'been hit' shot.
						switch (_action.type)
						{
							case BA_SNAPSHOT:
							case BA_AUTOSHOT:
							case BA_AIMEDSHOT:
								posContacts.push_back(pos);
						}
					}

					// ... Let's try something
/*					if (_prjImpact == VOXEL_UNIT)
					{
						BattleUnit* victim = _battleSave->getTile(Position::toTileSpace(_parent->getMap()->getProjectile()->getPosition(trjOffset))->getTileUnit();
						if (victim
							&& !victim->isOut(true, true)
							&& victim->getOriginalFaction() == FACTION_PLAYER
							&& _unit->getFaction() == FACTION_HOSTILE)
						{
							_unit->setExposed();
						} */ // But this is entirely unnecessary since aLien has already seen and logged the soldier.
/*						if (victim
							&& !victim->isOut(true, true)
							&& victim->getFaction() == FACTION_HOSTILE)
						{
							AlienBAIState* aggro = dynamic_cast<AlienBAIState*>(victim->getAIState());
							if (aggro != 0)
							{
								aggro->setWasHitBy(_unit);	// is used only for spotting on RA.
								_unit->setExposed();		// might want to remark this! Ok.
								// technically, in the original as I remember it, only
								// a BlasterLaunch (by xCom) would set an xCom soldier Exposed here!
								// Those aLiens had a way of tracing a BL back to its origin ....
								// ... but that's madness.
							}
						}
					} */
				}

				if (_action.type != BA_AUTOSHOT
					|| _action.autoShotCount == _action.weapon->getRules()->getAutoShots()
					|| _action.weapon->getAmmoItem() == nullptr)
				{
					_parent->getMap()->setReveal(false);

					if (_prjImpact == VOXEL_OUTOFBOUNDS) // else ExplosionBState will lower weapon above^
					{
						_unit->aim(false);
//						_unit->flagCache();
//						_parent->getMap()->cacheUnits(); // NOTE: Is that needed for like the unit(s) that got hit or something. no, Out_of_Bounds here.
						_parent->getMap()->cacheUnit(_unit);
					}
				}


				// Special Shotgun Behaviour: determine *extra* projectile paths and add bullet hits at their termination points.
				if (_load != nullptr)
				{
					Position shotVoxel;

					int pelletsLeft (_load->getRules()->getShotgunPellets() - 1); // shotgun pellets after 1st
					while (pelletsLeft > 0)
					{
						Projectile* const prj (new Projectile(
															_parent->getResourcePack(),
															_battleSave,
															_action,
															_posOrigin,
															_targetVoxel));
						const double
							spread (static_cast<double>(pelletsLeft * _load->getRules()->getShotgunPattern()) * 0.005), // pellet spread.
							accuracy (std::max(0.,
											   _unit->getAccuracy(_action) - spread));

						_prjImpact = prj->calculateShot(accuracy);
						if (_prjImpact != VOXEL_EMPTY && _prjImpact != VOXEL_OUTOFBOUNDS) // insert an explosion and hit
						{
							prj->skipTrajectory();			// skip the pellet to the end of its path
							shotVoxel = prj->getPosition();	// <- beware of 'offset 1'

							if (_prjImpact == VOXEL_UNIT)
							{
								switch (_action.type)
								{
									case BA_SNAPSHOT:
									case BA_AUTOSHOT:
									case BA_AIMEDSHOT:
										posContacts.push_back(Position::toTileSpace(shotVoxel));
								}
							}

							const int aniStart (_load->getRules()->getFireHitAnimation());
							if (aniStart != -1)
							{
								Explosion* const explosion (new Explosion(
																		ET_BULLET,
																		shotVoxel,
																		aniStart));
								_parent->getMap()->getExplosions()->push_back(explosion);
								_parent->setShotgun();

								Uint32 interval (static_cast<Uint32>(
												 std::max(1,
														  static_cast<int>(BattlescapeState::STATE_INTERVAL_EXPLOSION) - _load->getRules()->getExplosionSpeed())));
								_parent->setStateInterval(interval);
							}
							_battleSave->getTileEngine()->hit(
															shotVoxel,
															_load->getRules()->getPower(),
															_load->getRules()->getDamageType(),
															_unit,
															false, true);
						}

						delete prj;
						--pelletsLeft;
					}
				}


				std::vector<BattleUnit*> doneUnits; // This section is only for SoldierDiary mod.
				for (std::vector<Position>::const_iterator
						i = posContacts.begin();
						i != posContacts.end();
						++i)
				{
					BattleUnit* const victim (_battleSave->getTile(*i)->getTileUnit());
					if (victim != nullptr	// position of impact has a victim; actually every entry in posContacts ought have a victim per above^.
						&& std::find(		// this needs to ID the unit itself because large units occupy more than one position.
								doneUnits.begin(),
								doneUnits.end(),
								victim) == doneUnits.end())
					{
						doneUnits.push_back(victim); // each victim counts only once.

						if (_unit->getGeoscapeSoldier() != nullptr
							&& _unit->isMindControlled() == false)
						{
							BattleUnitStatistics* const statsActor (_unit->getStatistics());

							if (victim->getOriginalFaction() == FACTION_PLAYER)
								++statsActor->shotFriendlyCounter;

							const BattleUnit* const target (_battleSave->getTile(_action.posTarget)->getTileUnit()); // target (not necessarily who was hit)
							if (target == victim) // hit intended target
							{
								++statsActor->shotsLandedCounter;

								if (TileEngine::distance(_unit->getPosition(), *i) > _action.weapon->getRules()->getAimRange())
									++statsActor->longDistanceHitCounter;
							}
							else if (victim->getOriginalFaction() == FACTION_HOSTILE) // no Lucky Shots on civies or agents MC'd or not
								++statsActor->lowAccuracyHitCounter;
						}

						if (victim->getGeoscapeSoldier() != nullptr) // count these even if MC'd
						{
							BattleUnitStatistics* const statsVictim (victim->getStatistics());
							++statsVictim->hitCounter;

							if (_unit->getFaction() == FACTION_PLAYER)
								++statsVictim->shotByFriendlyCounter;
						}
					}
				}
			}

			delete _parent->getMap()->getProjectile();
			_parent->getMap()->setProjectile();
		}
	}
	//Log(LOG_INFO) << "ProjectileFlyBState::think() EXIT";
}

/**
 * Flying projectiles cannot be cancelled but they can be skipped.
 */
void ProjectileFlyBState::cancel()
{
	Projectile* const prj (_parent->getMap()->getProjectile());
	if (prj != nullptr)
	{
		prj->skipTrajectory();

		const Position pos (Position::toTileSpace(prj->getPosition()));
		if (_parent->getMap()->getCamera()->isOnScreen(pos) == false)
			_parent->getMap()->getCamera()->centerOnPosition(pos);
	}
}

/**
 * Peforms a melee-attack.
 */
void ProjectileFlyBState::performMeleeAttack() // private.
{
	//Log(LOG_INFO) << "flyB:performMeleeAttack() " << _unit->getId();
	_unit->aim();
//	_unit->flagCache();
	_parent->getMap()->cacheUnit(_unit);

	_action.posTarget = _battleSave->getTileEngine()->getMeleePosition(_unit);
	//Log(LOG_INFO) << ". target " << _action.target;

	// moved here from ExplosionBState to play a proper hit/miss sFx
	bool success;
	if (RNG::percent(static_cast<int>(Round(_unit->getAccuracy(_action) * 100.))) == true)
		success = true;
	else
		success = false;

	int soundId;
	if (success == false || _action.weapon->getRules()->getMeleeHitSound() == -1)
		soundId = _action.weapon->getRules()->getMeleeSound();
	else
		soundId = -1;

	if (soundId != -1)
		_parent->getResourcePack()->getSound("BATTLE.CAT", soundId)
									->play(-1, _parent->getMap()->getSoundAngle(_action.posTarget));

	if (_unit->getSpecialAbility() == SPECAB_BURN)
		_battleSave->getTile(_action.posTarget)->ignite(_unit->getUnitRules()->getSpecabPower() / 10);

	_parent->getMap()->setSelectorType(CT_NONE); // might be already done in primaryAction()


	const BattleUnit* const targetUnit (_battleSave->getTile(_action.posTarget)->getTileUnit());
	const int height ((targetUnit->getHeight() >> 1u)
					 + targetUnit->getFloatHeight()
					 - _battleSave->getTile(_action.posTarget)->getTerrainLevel());

	Position hitVoxel;
	Pathfinding::directionToVector(
								_unit->getUnitDirection(),
								&hitVoxel);
	hitVoxel = Position::toVoxelSpaceCentered(_action.posTarget, height) - (hitVoxel * 2);

	_parent->statePushNext(new ExplosionBState(
											_parent,
											hitVoxel,
											_action.weapon,
											_unit,
											nullptr,
											true,
											success));
}

}
