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

#include "../Engine/Game.h"
//#include "../Engine/Logger.h"
//#include "../Engine/Options.h"
#include "../Engine/RNG.h"
#include "../Engine/Screen.h"
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
 * Sets up the ProjectileFlyBState.
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
		_init(true),
		_targetFloor(false),
		_prjStart(0),
		_prj(nullptr)
{
	if (_posOrigin.z == -1)
		_posOrigin = _action.actor->getPosition();
}

/**
 * Deletes this ProjectileFlyBState.
 */
ProjectileFlyBState::~ProjectileFlyBState()
{}

/**
 * Gets the label of this BattleState.
 * @return, label of the substate
 */
std::string ProjectileFlyBState::getBattleStateLabel() const
{
	std::ostringstream oststr;
	oststr << "ProjectileFlyBState";
	if (_action.actor != nullptr) oststr << " id-" << _action.actor->getId();
	else oststr << " - Actor INVALID";

	return oststr.str();
}

/**
 * Initializes the sequence:
 * - checks if the shot is valid
 * - determines the target voxel
 */
void ProjectileFlyBState::init()
{
	//Log(LOG_INFO) << "";
	//Log(LOG_INFO) << "ProjectileFlyBState::init()";
	if (_init == true)
	{
		//Log(LOG_INFO) << "projFlyB init() targetPosTile = " << _action.posTarget;
		_init = false;

		_unit = _action.actor;
		_parent->getTacticalAction()->takenXp = false;

		bool popThis (false);
		if (_unit->isOut_t() == true
			|| _action.weapon == nullptr
			|| _battleSave->getTile(_action.posTarget) == nullptr)
		{
			popThis = true;
		}
		else if (_unit->getTu() >= _action.TU // go ->
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
				_unit->setTu(_unit->getTu() + _action.TU);
				popThis = true;
			}
		}
		else
		{
			_action.result = BattlescapeGame::PLAYER_ERROR[0u]; // no TU
			popThis = true;
		}

		if (popThis == true)
		{
			_unit->setStopShot(false);
			_parent->popBattleState();
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
					_action.result = BattlescapeGame::PLAYER_ERROR[2u]; // no ammo loaded
					popThis = true;
				}
//				else if (_load->getAmmoQuantity() == 0) // TODO: Move this down to inform player of an autoshot that runs out of bullets.
//				{
//					//Log(LOG_INFO) << ". . . no ammo Quantity, EXIT";
//					_action.result = "STR_NO_ROUNDS_LEFT";
//					popThis = true;
//				}
				else if (TileEngine::distance(
										_unit->getPosition(),
										_action.posTarget) > _action.weapon->getRules()->getMaxRange())
				{
					//Log(LOG_INFO) << ". . . out of range, EXIT";
					_action.result = BattlescapeGame::PLAYER_ERROR[5u]; // out of range
					popThis = true;
				}
				break;

			case BA_THROW:
			{
				//Log(LOG_INFO) << ". . BA_THROW " << _action.posTarget << " panic= " << (int)(_parent->playerPanicHandled() == false);
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
					_action.result = BattlescapeGame::PLAYER_ERROR[5u]; // out of range
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
				_parent->stateBPushFront(new ExplosionBState(
														_parent,
														Position::toVoxelSpaceCentered(_action.posTarget, 10),
														_action.weapon->getRules(),
														_unit));
				return;

			default:
				//Log(LOG_INFO) << ". . default, EXIT";
				popThis = true;
		}

		if (popThis == true)
		{
			_parent->popBattleState();
			return;
		}


		// ** Assign TARGET voxel ** ->
		const Tile* const tileTarget ( _battleSave->getTile(_action.posTarget));
		_targetVoxel = Position::toVoxelSpace(_action.posTarget);
		//Log(LOG_INFO) << "FlyB init targetVoxel " << _targetVoxel;

		if (_action.type == BA_THROW || _action.type == BA_LAUNCH)
		{
			//Log(LOG_INFO) << "projFlyB init() B-Launch OR Throw";
			_targetVoxel.x += 8;
			_targetVoxel.y += 8;

			switch (_action.type)
			{
				case BA_THROW:
					_targetVoxel.z += 2 - tileTarget->getTerrainLevel(); // LoFT of floor is typically 2 voxels thick.
					break;
				case BA_LAUNCH:
					if (_targetFloor == false) _targetVoxel.z += 16;
			}
		}
		else if ((_unit->getFaction() == FACTION_PLAYER		// force fire at center of Tile by pressing [CTRL] but *not* SHIFT
				&& (SDL_GetModState() & KMOD_CTRL)  != 0	// force fire at Floor w/ [CTRL+ALT]
				&& (SDL_GetModState() & KMOD_SHIFT) == 0)
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
			// NOTE: non-Player units cannot target tileParts ... but they might someday.
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
					|| (   (SDL_GetModState() & KMOD_SHIFT) == 0
						&& (SDL_GetModState() & KMOD_CTRL)  == 0
						&& tileTarget->getTileUnit()->getUnitVisible() == true)))
			{
				//Log(LOG_INFO) << ". tileTarget has unit";
				if (tileTarget->getTileUnit() == _unit
					|| _action.posTarget == _posOrigin)
				{
					//Log(LOG_INFO) << "projFlyB targetPos[2] = " << _action.target;
					_targetVoxel.x += 8; // don't shoot yourself but shoot at the floor
					_targetVoxel.y += 8;
//					_targetVoxel.z += 2; // borkity bork.
					//Log(LOG_INFO) << "projFlyB targetVoxel[2] = " << _targetVoxel;
				}
//				else if (!_parent->getTileEngine()->doTargetUnit(&originVoxel, targetTile, &_targetVoxel, _unit)) // <- their code.
//				{
//					_targetVoxel = Position(-16,-16,-24); // out of bounds, even after voxel to tile calculation. // lo..
//				}
				else if (_parent->getTileEngine()->doTargetUnit( // <- this is a normal shot by xCom or aLiens.
															&originVoxel,
															tileTarget,
															&_targetVoxel,
															_unit,
															nullptr,
															&_forced) == false) // <- karadoc fix -> NOT SURE I WANT THIS !!! <---
				{
//					_targetVoxel = Position::toVoxelSpace(_action.posTarget);
//					_targetVoxel.x += 8;
//					_targetVoxel.y += 8;
//					_targetVoxel.z += 10;
					// question: where does no-LoF popup -- oh yeah in a convoluted crap.

					// karadoc: if this action requires direct line-of-sight, should abort.
					// iff it's a line-shot (not arcing).
					// kL_note: You're playing around with the AI here, dude -- and I don't think you've considered that AT ALL.
					// Apart from that, I'm not so sure this is needed with the changes I've made to
					// - doTargetUnit()
					// - plotLine()
					// - plotParabola()
					// - etc etc etc.
					// - validateThrow()
					// - validateTarget()
					// - verifyTarget()
					// - doTargetTilepart()
					// - &tc.
					// On the bright side, the AI may well have already done a doTargetUnit() call, and so this would
					// always be true for the AI if and whenever it gets to here.
					//
					// ... but disable it anyway.
/*					switch (_action.type)
					{
						case BA_SNAPSHOT:
						case BA_AUTOSHOT:
						case BA_AIMEDSHOT:
							if (_action.weapon->getRules()->isArcingShot() == false)
							{
								_action.result = BattlescapeGame::PLAYER_ERROR[6u]; // no LoF
//								_action.TU = 0;
//								_unit->setUnitStatus(STATUS_STANDING);
								_parent->popState();
								return;
								//Log(LOG_INFO) << ". doTargetUnit() targetVoxel " << _targetVoxel << " targetTile " << Position::toTileSpace(_targetVoxel);
							}
					} */
				}
			}
			else if (tileTarget->getMapData(O_OBJECT) != nullptr	// force vs. Object by using CTRL above^
				&& (_unit->getFaction() != FACTION_PLAYER			// bypass Object by pressing SHIFT
					|| (SDL_GetModState() & KMOD_SHIFT) == 0))
			{
				//Log(LOG_INFO) << ". tileTarget has content-object";
				if (tileTarget->isRevealed() == false
					|| _parent->getTileEngine()->doTargetTilepart(
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
					|| (SDL_GetModState() & KMOD_CTRL) == 0))
			{
				//Log(LOG_INFO) << ". tileTarget has northwall";
				if (tileTarget->isRevealed(ST_NORTH) == false
					|| _parent->getTileEngine()->doTargetTilepart(
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
					|| _parent->getTileEngine()->doTargetTilepart(
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
				if (tileTarget->isRevealed() == false
					|| _parent->getTileEngine()->doTargetTilepart(
															&originVoxel,
															tileTarget,
															O_FLOOR,
															&_targetVoxel,
															_unit) == false)
				{
					_targetVoxel = Position::toVoxelSpace(_action.posTarget);
					_targetVoxel.x += 8;
					_targetVoxel.y += 8;
//					_targetVoxel.z += 2; // borkity bork.
				}
			}
			else // target nothing, targets middle of the tile
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
			_parent->getMap()->setSelectorType(CT_NONE);		// might be already done in primaryAction(). Nope:
			_parent->getMap()->getCamera()->stopMouseScroll();	// the cursor is hidden there, the selector is hidden here.
		}
	}
	//Log(LOG_INFO) << "ProjectileFlyBState::init() EXIT";
}

/**
 * Tries to create a projectile and adds its sprite to the battlefield.
 * @note Also calculate its trajectory.
 * @return, true if the projectile was successfully created
 */
bool ProjectileFlyBState::createProjectile() // private.
{
	//Log(LOG_INFO) << "ProjectileFlyBState::createProjectile()";
	//Log(LOG_INFO) << ". _action_type = " << _action.type;

	//Log(LOG_INFO) << "projFlyB create() originTile = " << _posOrigin;
	//Log(LOG_INFO) << "projFlyB create() targetVoxel = " << _targetVoxel;
	//Log(LOG_INFO) << "projFlyB create() targetVoxel.x = " << static_cast<float>(_targetVoxel.x) / 16.f;
	//Log(LOG_INFO) << "projFlyB create() targetVoxel.y = " << static_cast<float>(_targetVoxel.y) / 16.f;
	//Log(LOG_INFO) << "projFlyB create() targetVoxel.z = " << static_cast<float>(_targetVoxel.z) / 24.f;

	++_action.autoShotCount;

	_prj = new Projectile(
						_parent->getResourcePack(),
						_battleSave,
						_action,
						_posOrigin,
						_targetVoxel);


	//Log(LOG_INFO) << "projFlyB: createProjectile() set interval = " << BattlescapeState::STATE_INTERVAL_FAST;
	_parent->setStateInterval(BattlescapeState::STATE_INTERVAL_FAST); // set the speed of the state think cycle to 16 ms (roughly one think-cycle per frame)
	int soundId (-1);

	_prjImpact = VOXEL_EMPTY; // let it calculate a trajectory

	if (_action.type == BA_THROW)
	{
		//Log(LOG_INFO) << ". call Projectile::calculateThrow() to " << _targetVoxel;
		//Log(LOG_INFO) << "";
		_prjImpact = _prj->calculateThrow(_unit->getAccuracy(_action)); // this should probly be TE:validateThrow() - cf. else(error) below_
		//Log(LOG_INFO) << ". BA_THROW dest " << _targetVoxel << " part= " << MapData::debugVoxelType(_prjImpact);

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
				_unit->setCacheInvalid();
				_parent->getMap()->cacheUnitSprite(_unit);

				soundId = static_cast<int>(ResourcePack::ITEM_THROW);

				if (_unit->getGeoscapeSoldier() != nullptr
					&& _unit->isMindControlled() == false
					&& _parent->playerPanicHandled() == true)
				{
					_unit->addThrowingExp();
				}
				break;

			default:
				// Note that BattleUnit accuracy^ should *not* be considered before this.
				// Unless this is some sort of failsafe/exploit for the AI ... no it's
				// just the fucko-spaghetti-like code that's used throughout. /shrug
				//Log(LOG_INFO) << ". . NOT Valid";
				//Log(LOG_INFO) << ". . no throw, Voxel_Empty or _Wall or _OutofBounds";
				delete _prj;

				_action.result = BattlescapeGame::PLAYER_ERROR[15u]; // unable to throw here
				_action.TU = 0;
				_unit->setUnitStatus(STATUS_STANDING);

				_parent->popBattleState();
				return false;
		}
	}
	else if (_action.weapon->getRules()->isArcingShot() == true) // special code for the "spit" trajectory
	{
		_prjImpact = _prj->calculateThrow(_unit->getAccuracy(_action)); // this should probly be TE:validateThrow() - cf. else(error) below_
		//Log(LOG_INFO) << ". acid spit, part = " << (int)_prjImpact;

		if (_prjImpact != VOXEL_OUTOFBOUNDS
			&& (_prjImpact != VOXEL_EMPTY
				|| _load->getRules()->getExplosionRadius() != -1)) // <- midair explosion
		{
			//Log(LOG_INFO) << ". . spit/arcing shot";
			if (_prjImpact == VOXEL_OBJECT
				&& _load->getRules()->getExplosionRadius() != -1)
			{
				const Tile* const tile (_battleSave->getTile(_prj->getFinalPosition()));
//				if (tile != nullptr && tile->getMapData(O_OBJECT) != nullptr) // safety. Should be unnecessary because _prjImpact=VOXEL_OBJECT ....
				{
					switch (tile->getMapData(O_OBJECT)->getBigwall())
					{
						case BIGWALL_NESW:
						case BIGWALL_NWSE:
//							_prj->storeProjectileDirection();		// Used to handle direct-explosive-hits against diagonal bigWalls.
							_prjVector = _prj->getStrikeVector();	// ^supercedes above^ storeProjectileDirection()
					}
				}
			}

			// lift-off
			soundId = _load->getRules()->getFireSound();
			if (soundId == -1)
				soundId = _action.weapon->getRules()->getFireSound();

			_unit->startAiming();
			_parent->getMap()->cacheUnitSprite(_unit);
		}
		else // no line of fire; Note that BattleUnit accuracy^ should *not* be considered before this. Unless this is some sort of failsafe/exploit for the AI ...
		{
			//Log(LOG_INFO) << ". . no spit, no LoF, Voxel_Empty";
			delete _prj;

			_action.result = BattlescapeGame::PLAYER_ERROR[6u]; // no LoF
			_action.TU = 0;
			_unit->setUnitStatus(STATUS_STANDING);

			_parent->popBattleState();
			return false;
		}
	}
	else // shoot weapon
	{
		//Log(LOG_INFO) << "FlyB: . shoot weapon";
		if (_originVoxel.z != -1) // origin is a BL waypoint
		{
			_prjImpact = _prj->calculateShot( // this should probly be TE:plotLine() - cf. else(error) below_
										_unit->getAccuracy(_action),
										_originVoxel,
										false); // <- don't consider excludeUnit if a BL-waypoint
			//Log(LOG_INFO) << ". shoot weapon[0], voxelType = " << (int)_prjImpact;
		}
		else // non-BL weapon
		{
			//Log(LOG_INFO) << "FlyB: . . not BL";
			if (_forced == true) _prj->setForced();
			_prjImpact = _prj->calculateShot(_unit->getAccuracy(_action)); // this should probly be TE:plotLine() - cf. else(error) below_
			//Log(LOG_INFO) << ". shoot weapon[1], voxelType = " << (int)_prjImpact;
			//Log(LOG_INFO) << "prjFlyB accuracy = " << _unit->getAccuracy(_action);

			if (TileEngine::distance(_posOrigin, _prj->getFinalPosition())
					> _action.weapon->getRules()->getMaxRange())
			{
				_prjImpact = VOXEL_EMPTY; // TODO: Change warning from no-LoF to beyond-max-range.
			}
		}
		//Log(LOG_INFO) << ". shoot weapon, voxelType = " << (int)_prjImpact;
		//Log(LOG_INFO) << ". finalTarget = " << _prj->getFinalPosition();

		if (_prjImpact != VOXEL_EMPTY || _action.type == BA_LAUNCH) //&& _targetVoxel != Position(-16,-16,-24) // <- their code.
		{
			//Log(LOG_INFO) << ". . _prjImpact AIM";
			if (_prjImpact == VOXEL_OBJECT
				&& _load->getRules()->getExplosionRadius() > 0)
			{
				const Tile* const tile (_battleSave->getTile(_prj->getFinalPosition()));
//				if (tile != nullptr && tile->getMapData(O_OBJECT) != nullptr) // safety. Should be unnecessary because _prjImpact=VOXEL_OBJECT ....
				switch (tile->getMapData(O_OBJECT)->getBigwall())
				{
					case BIGWALL_NESW:
					case BIGWALL_NWSE:
//						_prj->storeProjectileDirection();		// Used to handle direct-explosive-hits against diagonal bigWalls.
						_prjVector = _prj->getStrikeVector();	// ^supercedes storeProjectileDirection() above^
				}
			}

			switch (_action.type)
			{
				case BA_AUTOSHOT:
				case BA_LAUNCH:
					_parent->getMap()->setReveal();	// Reveal the Map until action completes.
			}

			// lift-off
			if ((soundId = _load->getRules()->getFireSound()) == -1)
				soundId = _action.weapon->getRules()->getFireSound();

			if (_originVoxel.z == -1) // not a BL-waypoint
				_unit->toggleShoot();

			//Log(LOG_INFO) << "FlyB: . okay to shoot";
		}
		else // VOXEL_EMPTY, no line of fire; Note that BattleUnit accuracy^ should *not* be considered before this. Unless this is some sort of failsafe/exploit for the AI ...
		{
			//Log(LOG_INFO) << ". no shot, no LoF, Voxel_Empty";
			delete _prj;

			_action.result = BattlescapeGame::PLAYER_ERROR[6u]; // no LoF
			_action.TU = 0;
			_unit->setUnitStatus(STATUS_STANDING);

			_parent->popBattleState();
			//Log(LOG_INFO) << "FlyB: . no LoS ret FALSE";
			return false;
		}
	}

	if (soundId != -1)
		_parent->getResourcePack()->getSound("BATTLE.CAT", static_cast<unsigned>(soundId))
					->play(-1, _parent->getMap()->getSoundAngle(_unit->getPosition()));

	if (_unit->getArmor()->getShootFrames() != 0) // postpone showing the Celatid spit-blob till later
		_parent->getMap()->showProjectile(false);

	if (_action.autoShotCount == 1 && _unit->getGeoscapeSoldier() != nullptr)
	{
		switch (_action.type)
		{
			case BA_SNAPSHOT:
			case BA_AUTOSHOT:
			case BA_AIMEDSHOT:
				++_unit->getStatistics()->shotsFiredCounter;
		}
	}

	//Log(LOG_INFO) << ". createProjectile() ret TRUE";
	_parent->getMap()->setProjectile(_prj); // add projectile to Map.

	return true;
}

/**
 * Animates the projectile as it moves to the next point in its trajectory.
 * @note If the animation is finished the projectile sprite is removed from the
 * battlefield and this state is finished.
 */
void ProjectileFlyBState::think()
{
	//Log(LOG_INFO) << "";
	//Log(LOG_INFO) << "ProjectileFlyBState::think() id-" << _unit->getId();
	//Log(LOG_INFO) << ". mapOffset " << _parent->getMap()->getCamera()->getMapOffset();
	if (_unit->getUnitStatus() == STATUS_AIMING
		&& _unit->getArmor()->getShootFrames() != 0)
	{
		if (_prjStart == 0) _prjStart = 1;
		_unit->keepAiming();

		if (_unit->getAimingPhase() < _unit->getArmor()->getFirePhase())
			return;
	}

	if (_prjStart == 1)
	{
		_prjStart = 2;
		_parent->getMap()->showProjectile();
	}

	_battleSave->getBattleState()->clearDragScroll();

	if (_prj == nullptr)
	{
		Position pos;
		if (_load != nullptr
			&& _action.type == BA_AUTOSHOT
			&& _action.autoShotCount < _action.weapon->getRules()->getAutoShots()
			&& _unit->isOut_t() == false
			&& (_unit->getMoveTypeUnit() == MT_FLY
				|| _battleSave->getTile(pos = _unit->getPosition())
						->isFloored(_battleSave->getTile(pos + Position(0,0,-1))) == true))
//			&& ((_battleSave->getTile(_unit->getPosition()) != nullptr // wtf. ->
//					&& _battleSave->getTile(_unit->getPosition())
//						->isFloored(_battleSave->getTile(_unit->getPosition() + Position(0,0,-1))) == true)
//				|| _unit->getMoveTypeUnit() == MT_FLY))
		{
			createProjectile(); // autoshot.
		}
		else // think() FINISH.
		{
			//const bool debug (_action.actor->getId() == 381);
			//if (debug) Log(LOG_INFO) << "";
			//if (debug) Log(LOG_INFO) << "ProjectileFlyBState::think() -> finish actorId-" << _action.actor->getId();
			switch (_action.type) // possible Camera re/positioning to pre-shot or post-RF position
			{
				case BA_THROW:
				case BA_AUTOSHOT:
				case BA_SNAPSHOT:
				case BA_AIMEDSHOT:
					//if (debug) Log(LOG_INFO) << "FlyB: . stored posCamera " << _action.posCamera;
					if (_action.posCamera.z != -1
						|| _parent->getTileEngine()->isReaction() == true)
					{
						//if (debug) Log(LOG_INFO) << "FlyB: . . setting Camera to shooter pos";
						Camera* const shotCamera (_parent->getMap()->getCamera());
						if (shotCamera->getPauseAfterShot() == true)	// TODO: Move 'pauseAfterShot' to the BattleAction struct. done -> but it didn't work; i'm a numby.
//						if (_action.pauseAfterShot == true)				// NOTE: That trying to store the camera position in the BattleAction didn't work either ... double numby.
						{
							shotCamera->setPauseAfterShot(false);

							switch (_prjImpact)
							{
//								case VOXEL_EMPTY: // -1
								case VOXEL_FLOOR:
								case VOXEL_WESTWALL:
								case VOXEL_NORTHWALL:
								case VOXEL_OBJECT:
								case VOXEL_UNIT:
									//if (debug) Log(LOG_INFO) << "FlyB: . . . . delay";
									SDL_Delay(Screen::SCREEN_PAUSE); // screen-pause when shot hits target before reverting camera to shooter.
//									break;
//
//								case VOXEL_OUTOFBOUNDS:
//									Log(LOG_INFO) << "FlyB: . . . . final vox OutofBounds - do NOT pause";
							}
						}

						if (_parent->getTileEngine()->isReaction() == true)
						{
							//if (debug) Log(LOG_INFO) << ". . is Reaction - set Camera to center on reactor";
							shotCamera->focusPosition(_unit->getPosition());
						}
						else
						{
							//if (debug) Log(LOG_INFO) << ". . is NOT Reaction - set Camera to cached position";
							shotCamera->setMapOffset(_action.posCamera);
							_parent->getMap()->draw(); // NOTE: Might not be needed. Ie, the camera-offset seems to take hold okay without.
						}
//						_action.posCamera = Position(0,0,-1);
//						_parent->getMap()->invalidate();

						//if (debug) Log(LOG_INFO) << "FlyB: . . . Screw you, State Machine.";
						_parent->getBattlescapeState()->blit();
						_parent->getBattlescapeState()->getGame()->getScreen()->flip();
						SDL_Delay(Screen::SCREEN_PAUSE);
					}
			}

			switch (_action.type)
			{
				case BA_PSIPANIC:
				case BA_PSICONFUSE:
				case BA_PSICOURAGE:
				case BA_PSICONTROL:
					break;

				case BA_LAUNCH:
					if (_prjImpact == VOXEL_EMPTY) break;
					// no break;
				case BA_SNAPSHOT:
				case BA_AUTOSHOT:
				case BA_AIMEDSHOT:
					_load->spendBullet(
									*_battleSave,
									*_action.weapon);
					// no break;
				default: // NOTE: Probably only BA_THROW left here.
					if (_unit->getFaction() == _battleSave->getSide()
						&& _battleSave->unitsFalling() == false)
					{
						//Log(LOG_INFO) << "ProjectileFlyBState::think() CALL te::checkReactionFire()"
						//			    << " id-" << _unit->getId()
						//			    << " action.type= " << BattleAction::debugBat(_action.type)
						//			    << " action.TU= " << _action.TU;
						_parent->getTileEngine()->checkReactionFire(		// note: I don't believe that smoke obscuration gets accounted
																_unit,		// for by this call if the current projectile caused cloud.
																_action.TU,	// But that's kinda ok.
																_action.type != BA_MELEE);
					}
			}

			if (_unit->isOut_t() == false && _action.type != BA_MELEE)
				_unit->setUnitStatus(STATUS_STANDING);

			_parent->popBattleState();
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
			_prj->skipTrajectory(); // skip trajectory of 1st pellet; the rest are not even added to Map.
		}

		if (_prj->traceProjectile() == false) // cycle projectile pathing -> Finished
		{
			if (_action.type == BA_THROW)
			{
//				_parent->getMap()->resetCameraSmoothing();
				Position
					throwVoxel (_prj->getPosition(-1)), // <- beware of 'offset -1'
					pos (Position::toTileSpace(throwVoxel));

				if (pos.x > _battleSave->getMapSizeX()) // note: Bounds-checking is also done better in Projectile::applyAccuracy()
					--pos.x;

				if (pos.y > _battleSave->getMapSizeY())
					--pos.y;

				BattleItem* const throwItem (_prj->getThrowItem());
				if (throwItem->getRules()->getBattleType() == BT_GRENADE
					&& throwItem->getFuse() == 0) //&& Options::battleInstantGrenade == true // -> moved to PrimeGrenadeState (0 cannot be set w/out InstantGrenades)
				{
					_parent->stateBPushFront(new ExplosionBState( // it's a hot potato set to explode on contact
															_parent,
															throwVoxel,
															throwItem->getRules(),
															_unit));
					_battleSave->toDeleteItem(throwItem);
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
				blasterFlyB->_originVoxel = _prj->getPosition(); // was (offset= -1) -> tada, fixed.
				if (_action.posTarget == _posOrigin) blasterFlyB->_targetFloor = true;

				_parent->getMap()->getCamera()->centerPosition(_posOrigin, false); // this follows BL as it hits through waypoints
				_parent->stateBPushNext(blasterFlyB);
			}
			else // shoot -> impact.
			{
//				_parent->getMap()->resetCameraSmoothing();
				switch (_action.type)
				{
					case BA_LAUNCH:
						_prjImpact = VOXEL_OBJECT; // Launch explodes at final waypoint auto.
						_battleSave->getBattleGame()->getTacticalAction()->waypoints.clear();	// NOTE: Do this here instead of BattlescapeGame::popBattleState()
						break;																	// because if the unit dies in its own blast the selector doesn't reset.

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

					Position explVoxel (_prj->getPosition(trjOffset));
					const Position pos (Position::toTileSpace(explVoxel));

					if (_prjVector.z != -1) // <- strikeVector by radial explosion vs. diagBigWall
					{
						Tile* const tileTrue (_parent->getBattlescapeState()->getSavedBattleGame()->getTile(pos));
						_parent->getTileEngine()->setTrueTile(tileTrue);

						explVoxel.x -= _prjVector.x << 4u; // note there is no safety on these for OoB.
						explVoxel.y -= _prjVector.y << 4u;
					}
					else
						_parent->getTileEngine()->setTrueTile();

					_parent->stateBPushFront(new ExplosionBState(
															_parent,
															explVoxel,
															_load->getRules(),
															_unit,
															nullptr,
															_action.type != BA_AUTOSHOT // final projectile -> stop Aiming.
																|| _action.autoShotCount == _action.weapon->getRules()->getAutoShots()
																|| _action.weapon->getAmmoItem() == nullptr,
															false, false,
															_action.type == BA_LAUNCH));

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
						BattleUnit* victim = _battleSave->getTile(Position::toTileSpace(_prj->getPosition(trjOffset))->getTileUnit();
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

				// Special Shotgun Behaviour: determine *extra* projectile-paths and add bullet-hits at their termination points.
				if (_load != nullptr)
				{
					Position shotVoxel;

					int pelletsLeft (_load->getRules()->getShotgunPellets() - 1); // shotgun pellets after 1st^
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
														  static_cast<int>(BattlescapeState::STATE_INTERVAL_EXPLOSION)
																- _load->getRules()->getExplosionSpeed())));
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


				if (_load == nullptr
					|| _action.type != BA_AUTOSHOT
					|| _action.autoShotCount == _action.weapon->getRules()->getAutoShots())
				{
					_parent->getMap()->setReveal(false);

					if (_prjImpact == VOXEL_OUTOFBOUNDS) // else ExplosionBState will lower weapon above^
						_unit->toggleShoot();
				}


				std::vector<BattleUnit*> contactsHandled; // This section is only for SoldierDiary.
				contactsHandled.resize(posContacts.size());

				for (std::vector<Position>::const_iterator
						i = posContacts.begin();
						i != posContacts.end();
						++i)
				{
					BattleUnit* const victim (_battleSave->getTile(*i)->getTileUnit());
					if (victim != nullptr	// position of impact has a victim; actually every entry in posContacts ought have a victim per above^.
						&& std::find(		// this needs to ID the unit itself because large units occupy more than one position.
								contactsHandled.begin(),
								contactsHandled.end(),
								victim) == contactsHandled.end())
					{
						contactsHandled.push_back(victim); // each victim counts only once.

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

			delete _prj;
			_parent->getMap()->setProjectile(_prj = nullptr);
		}
	}
	//Log(LOG_INFO) << "ProjectileFlyBState::think() EXIT";
}

/**
 * Flying projectiles cannot be cancelled but they can be skipped.
 */
void ProjectileFlyBState::cancel()
{
	if (_prj != nullptr)
	{
		_prj->skipTrajectory();
		_parent->getMap()->getCamera()->focusPosition(Position::toTileSpace(_prj->getPosition())); // NOTE: Wouldn't surprise me if this is entirely unnecessary.
	}
}

/**
 * Peforms a melee-attack.
 */
void ProjectileFlyBState::performMeleeAttack() // private.
{
	//Log(LOG_INFO) << "";
	//Log(LOG_INFO) << "fB:performMeleeAttack() id-" << _unit->getId();
	_unit->toggleShoot();

	_action.posTarget = _battleSave->getTileEngine()->getMeleePosition(_unit);
	//Log(LOG_INFO) << ". target " << _action.target;

	// moved here from ExplosionBState to play a proper hit/miss sFx
	bool success;
	if (RNG::percent(static_cast<int>(Round(_unit->getAccuracy(_action) * 100.))) == true)
		success = true;
	else
		success = false;
	//Log(LOG_INFO) << ". success= " << success;

	if (success == false || _action.weapon->getRules()->getMeleeHitSound() == -1)
	{
		const int soundId (_action.weapon->getRules()->getMeleeSound());
		if (soundId != -1)
			_parent->getResourcePack()->getSound("BATTLE.CAT", static_cast<unsigned>(soundId))
										->play(-1, _parent->getMap()->getSoundAngle(_action.posTarget));
	}

	if (_unit->getSpecialAbility() == SPECAB_BURN)
	{
		// Put burnedBySilacoid() here! etc
		_unit->burnTile(_battleSave->getTile(_action.posTarget));
	}

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

	_parent->stateBPushNext(new ExplosionBState(
											_parent,
											hitVoxel,
											_action.weapon->getRules(),
											_unit,
											nullptr,
											true,
											success));
}

}
