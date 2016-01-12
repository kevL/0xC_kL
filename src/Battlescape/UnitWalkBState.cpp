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

#include "UnitWalkBState.h"

#include "BattlescapeState.h"
#include "Camera.h"
#include "Map.h"
#include "Pathfinding.h"
#include "ProjectileFlyBState.h"
#include "UnitFallBState.h"
#include "TileEngine.h"

//#include "../Engine/Logger.h"
//#include "../Engine/Options.h"
#include "../Engine/RNG.h"
#include "../Engine/Sound.h"
#include "../Engine/Surface.h" // for turning on/off visUnit indicators.

#include "../Interface/Bar.h"
#include "../Interface/NumberText.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/RuleArmor.h"
#include "../Ruleset/Ruleset.h"

#include "../Savegame/BattleItem.h"
#include "../Savegame/BattleUnit.h"
#include "../Savegame/SavedBattleGame.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/Tile.h"


namespace OpenXcom
{

/**
 * Sets up a UnitWalkBState.
 * @param parent - pointer to the BattlescapeGame
 * @param action - the BattleAction struct (BattlescapeGame.h)
 */
UnitWalkBState::UnitWalkBState(
		BattlescapeGame* const parent,
		BattleAction action) // these BattleActions had better be assignments/copies ... not references.
	:
		BattleState(parent, action),
		_unit(action.actor),
		_pf(parent->getPathfinding()),
		_te(parent->getTileEngine()),
		_battleSave(parent->getBattleSave()),
		_falling(false),
		_preStepTurn(false),
		_unitsSpotted(0),
		_preStepCost(0),
		_tileSwitchDone(false),
		_isVisible(false),
		_walkCam(parent->getMap()->getCamera()),
		_dirStart(-1),
		_kneelCheck(true),
		_playFly(false)
{
	//Log(LOG_INFO) << "walkB: cTor id-" << _unit->getId();
}

/**
 * Deletes the UnitWalkBState.
 */
UnitWalkBState::~UnitWalkBState()
{
	//Log(LOG_INFO) << "walkB: dTor" << _unit->getId();
}

/**
 * Initializes the state.
 */
void UnitWalkBState::init()
{
	//Log(LOG_INFO) << "UnitWalkBState::init() unitID = " << _unit->getId();
	//Log(LOG_INFO) << ". walking from " << _unit->getPosition() << " to " << _action.target;
	if (_unit->getFaction() != FACTION_PLAYER
		&& _unit != _battleSave->getWalkUnit()) // See.
	{
		//Log(LOG_INFO) << "walkB: init() center on unit id-" << _unit->getId();
		_walkCam->centerOnPosition(_unit->getPosition());
	}

	// This is used only for aLiens:
	_unitsSpotted = _unit->getHostileUnitsThisTurn().size();

	_pf->setPathingUnit(_unit);
	_dirStart = _pf->getStartDirection();
	//Log(LOG_INFO) << ". strafe = " << (int)_action.strafe;
	//Log(LOG_INFO) << ". StartDirection(init) = " << _dirStart;
	//Log(LOG_INFO) << ". getUnitDirection(init) = " << _unit->getUnitDirection();
	if (_action.strafe == false						// not strafing
		&& _dirStart > -1 && _dirStart < 8			// moving but not up or down
		&& _dirStart != _unit->getUnitDirection())	// not facing in direction of movement
	{
		// if unit is not facing in the direction that it's about to walk toward ...
		// This makes the unit expend tu's if it spots a new alien when turning,
		// but stops before actually walking.
		// Also expends tu if cancel() is called before first step.
		_preStepTurn = true;
	}

	doFallCheck();
	//Log(LOG_INFO) << "UnitWalkBState::init() EXIT";
}

/**
 * Runs state functionality every cycle.
 */
void UnitWalkBState::think()
{
	//Log(LOG_INFO) << "\n***** UnitWalkBState::think() : " << _unit->getId();
	if (_unit->isOut_t() == true)
	{
		//Log(LOG_INFO) << ". . isOut() abort.";
		_pf->abortPath();
		_parent->popState();
		return;
	}

	_isVisible = _unit->getUnitVisible();
	//Log(LOG_INFO) << ". _isVisible = " << _isVisible;


/* _oO **** STATUS WALKING **** Oo_ */// #2

	if (_unit->getUnitStatus() == STATUS_WALKING
		|| _unit->getUnitStatus() == STATUS_FLYING)
	{
		//Log(LOG_INFO) << "STATUS_WALKING or FLYING : " << _unit->getId();
/*		if (_isVisible == true)
		{
			const int stopZ = _unit->getStopPosition().z;
			if (_walkCam->isOnScreen(_unit->getPosition()) == true
				&& _walkCam->getViewLevel() < stopZ)
			{
				_walkCam->setViewLevel(stopZ);
			}
		} */

		if (doStatusWalk() == false)
		{
			//Log(LOG_INFO) << ". . doStatusWalk() FALSE return";
			return;
		}


/* _oO **** STATUS STANDING end **** Oo_ */// #3

		// walkPhase reset as the unit completes its transition to the next tile
		if (_unit->getUnitStatus() == STATUS_STANDING)
		{
			//Log(LOG_INFO) << "STATUS_STANDING_end in UnitWalkBState _WALKING or _FLYING !!!" ;
			clearTilesLink(true);

/*			if (_isVisible == true)
			{
				const Position pos = _unit->getPosition();

				if (_unit->getFaction() != FACTION_PLAYER
					&& _walkCam->isOnScreen(pos) == false)
				{
					_walkCam->centerOnPosition(pos);
					_walkCam->setViewLevel(_unit->getStopPosition().z);
				}
				else if (_walkCam->isOnScreen(pos) == true)
				{
					const int stopZ = _unit->getStopPosition().z;
					if (_walkCam->getViewLevel() > stopZ
						&& (_pf->getPath().size() == 0 || _pf->getPath().back() != _pf->DIR_UP))
					{
						_walkCam->setViewLevel(stopZ);
					}
				}
			} */

			if (doStatusStand_end() == false)
			{
				//Log(LOG_INFO) << ". . doStatusStand_end() FALSE return";
				return;
			}

			if (_unit->getFaction() == FACTION_PLAYER
				&& _parent->getPanicHandled() == true)
			{
				BattlescapeState* const battleState (_parent->getBattlescapeState());
				if (battleState->playableUnitSelected() == true)
				{
					battleState->hotSqrsClear();
					battleState->hotSqrsUpdate();
				}
			}
		}
		else if (_isVisible == true) // keep walking ... make sure the unit sprites are up to date
		{
			//Log(LOG_INFO) << ". _isVisible : still walking ...";
//			if (_pf->getStrafeMove() == true) // NOTE: This could be trimmed, because I had to make tanks use getFaceDirection() in UnitSprite::drawRoutine2() anyway ...
			if (_action.strafe == true)
			{
				//Log(LOG_INFO) << ". WALKING strafe, unitDir = " << _unit->getUnitDirection();
				//Log(LOG_INFO) << ". WALKING strafe, faceDir = " << _unit->getFaceDirection();
				const int dirStrafe = _unit->getUnitDirection(); // direction of travel
				_unit->setUnitDirection(
									_unit->getFaceDirection(),
									false);

//				_unit->clearCache(); // might play around with Strafe anim's ......
				_parent->getMap()->cacheUnit(_unit);
				_unit->setUnitDirection(dirStrafe, false);
			}
			else
			{
				//Log(LOG_INFO) << ". WALKING no strafe, cacheUnit()";
				_unit->clearCache(); // might play around with non-Strafe anim's ......
				_parent->getMap()->cacheUnit(_unit);
			}
		}
	}


/* _oO **** STATUS STANDING **** Oo_ */// #1 & #4

	if (_unit->getUnitStatus() == STATUS_STANDING
		|| _unit->getUnitStatus() == STATUS_PANICKING)
	{
		//Log(LOG_INFO) << "STATUS_STANDING or PANICKING : " << _unit->getId();
		if (doStatusStand() == false)
		{
			//Log(LOG_INFO) << ". . doStatusStand() FALSE return";
			return;
		}

		// Destination is not valid until *after* doStatusStand() runs.
/*		if (_isVisible == true)
		{
			//Log(LOG_INFO) << ". onScreen";
			const Position pos = _unit->getPosition();

			if (_unit->getFaction() != FACTION_PLAYER
				&& _walkCam->isOnScreen(pos) == false)
			{
				_walkCam->centerOnPosition(pos);
				_walkCam->setViewLevel(pos.z);
			}
			else if (_walkCam->isOnScreen(pos) == true) // is Faction_Player
			{
				const int stopZ = _unit->getStopPosition().z;
				if (pos.z == stopZ || (pos.z < stopZ && _walkCam->getViewLevel() < stopZ))
				{
					_walkCam->setViewLevel(pos.z);
				}
			}
		} */
	}


/* _oO **** STATUS TURNING **** Oo_ */

	if (_unit->getUnitStatus() == STATUS_TURNING) // turning during walking costs no tu
	{
		//Log(LOG_INFO) << "STATUS_TURNING : " << _unit->getId();
		doStatusTurn();
	}
	//Log(LOG_INFO) << "think() : " << _unit->getId() << " EXIT ";
}

/**
 * Aborts unit walking.
 */
void UnitWalkBState::cancel()
{
	if (_battleSave->getSide() == FACTION_PLAYER
		&& _parent->getPanicHandled() == true)
	{
		if (_preStepTurn == true)
		{
			_unit->spendTimeUnits(_preStepCost);

			_preStepCost = 0;
			_preStepTurn = false;
		}
		_pf->abortPath();
	}
}

/**
 * This begins unit movement. And may end unit movement.
 * Called from think()...
 * @return, true to continue moving, false to exit think()
 */
bool UnitWalkBState::doStatusStand() // private.
{
	//Log(LOG_INFO) << "";
	//Log(LOG_INFO) << "***** UnitWalkBState::doStatusStand() : " << _unit->getId();
	int dir = _pf->getStartDirection();
	//Log(LOG_INFO) << ". StartDirection = " << dir;

	const Position pos = _unit->getPosition();

	if (_unit->getFaction() != FACTION_PLAYER // && _isVisible == true
		&& _walkCam->isOnScreen(_unit->getPosition()) == false)
	{
		//Log(LOG_INFO) << "walkB: statusStand() center on unit id-" << _unit->getId();
		_walkCam->centerOnPosition(pos);
//		_walkCam->setViewLevel(pos.z);
	}

	const Tile* const tile = _battleSave->getTile(pos);
	const bool gravLift = dir >= _pf->DIR_UP // Assumes tops & bottoms of gravLifts always have floors/ceilings.
					   && tile->getMapData(O_FLOOR) != nullptr
					   && tile->getMapData(O_FLOOR)->isGravLift();
	setWalkSpeed(gravLift);

	if (dir != -1
		&& _kneelCheck == true				// check if unit is kneeled
		&& _unit->isKneeled() == true		// unit is kneeled
		&& gravLift == false				// not on a gravLift
		&& _pf->getPath().empty() == false)	// not the final tile of path; that is, the unit is actually going to move.
	{
		//Log(LOG_INFO) << ". kneeled, and path Valid";
		_kneelCheck = false;

		if (_parent->kneel(_unit) == true)
		{
			//Log(LOG_INFO) << ". . Stand up";
			_unit->clearCache();
			_parent->getMap()->cacheUnit(_unit);

			if (_te->checkReactionFire(_unit) == true) // unit got fired upon - stop.
			{
				//Log(LOG_INFO) << ". . . RF triggered";
				_pf->abortPath();
				_parent->popState();
				return false;
			}
		}
		else
		{
			//Log(LOG_INFO) << ". . don't stand: not enough TU";
			_action.result = "STR_NOT_ENOUGH_TIME_UNITS"; // note: redundant w/ kneel() error messages ...

			_pf->abortPath();
			_parent->popState();
			return false;
		}
	}

	if (visForUnits() == true)
	{
		//Log(LOG_INFO) << "Uh-oh! STATUS_STANDING or PANICKING Company!";
		//if (_unit->getFaction() == FACTION_PLAYER) Log(LOG_INFO) << ". . _newVis = TRUE, postPathProcedures";
		//else if (_unit->getFaction() != FACTION_PLAYER) Log(LOG_INFO) << ". . _newUnitSpotted = TRUE, postPathProcedures";

		if (_unit->getFaction() != FACTION_PLAYER)
			_unit->setHiding(false);

		_unit->clearCache();					// Calls to cacheUnit() are bogus without setCache(nullptr) first ...!
		_parent->getMap()->cacheUnit(_unit);	// although _cacheInvalid might be set elsewhere but i doubt it.

		postPathProcedures();
		return false;
	}

	_tileSwitchDone = false;

	//Log(LOG_INFO) << ". getStartDirection() dir = " << dir;
	if (_falling == true)
	{
		dir = _pf->DIR_DOWN;
		//Log(LOG_INFO) << ". . _falling, dir = " << dir;
	}

	if (dir == -1)
	{
		//Log(LOG_INFO) << ". dir = " << _unit->getUnitDirection();
		//Log(LOG_INFO) << ". . CALL postPathProcedures()";
		postPathProcedures();
		return false;
	}

	//Log(LOG_INFO) << "enter (dir!= -1) : " << _unit->getId();
	if (_action.strafe == true
		&& _pf->getPath().empty() == false) // <- don't bother with this if it's the end of movement/ State.
	{
		if (_unit->getGeoscapeSoldier() != nullptr
			|| _unit->getUnitRules()->isMechanical() == false)
		{
			//Log(LOG_INFO) << ". STANDING strafeMove, setFaceDirection() -> " << _unit->getUnitDirection();
			_unit->setFaceDirection(_unit->getUnitDirection());
		}
		else
		{
			const int dirStrafe = (_dirStart + 4) % 8;
			_unit->setFaceDirection(dirStrafe);
			//Log(LOG_INFO) << ". STANDING strafeTank, setFaceDirection() -> " << dirStrafe;

			if (_unit->getTurretType() != -1)
			{
				const int turretOffset = _unit->getTurretDirection() - _unit->getUnitDirection();
				_unit->setTurretDirection((turretOffset + dirStrafe) % 8);
				//Log(LOG_INFO) << ". STANDING strafeTank, setTurretDirection() -> " << (turretOffset + dirStrafe);
			}
		}
	}
	//else Log(LOG_INFO) << ". STANDING no strafe.";

	//Log(LOG_INFO) << ". getTuCostPf() & posStop";
	Position posStop;
	int
		tuCost = _pf->getTuCostPf(pos, dir, &posStop), // gets tu cost but also sets the destination position.
		tuTest,
		staCost;
	//Log(LOG_INFO) << ". tuCost = " << tuCost;

	Tile* const destTile = _battleSave->getTile(posStop);

	if (destTile != nullptr // would hate to see what happens if destTile=nullptr, nuclear war no doubt.
		&& destTile->getFire() > 0
		&& _unit->avoidsFire() == true)
	{
		//Log(LOG_INFO) << ". . subtract tu inflation for a fireTile";
		// The TU cost was artificially inflated by 32 points in getTuCostPf
		// so it has to be deflated again here under the same conditions.
		// See: Pathfinding::getTuCostPf(), where TU cost was inflated.
		tuCost -= 32;
	}

	if (_falling == true)
	{
		//Log(LOG_INFO) << ". . falling, set tuCost 0";
		tuCost =
		tuTest =
		staCost = 0;
	}
	else
	{
		tuTest =
		staCost = tuCost;

		if (gravLift == false)
		{
			if (_action.dash == true // allow dash when moving vertically 1 tile (or more).
				|| (_action.strafe == true
					&& dir >= _pf->DIR_UP))
			{
				tuCost -= _pf->getOpenDoor();
				tuCost = (tuCost * 3 / 4) + _pf->getOpenDoor();

				staCost -= _pf->getOpenDoor();
				staCost = staCost * 3 / 2;
			}

			staCost -= _unit->getArmor()->getAgility();
			if (staCost < 0) staCost = 0;
		}
		else // gravLift
		{
			//Log(LOG_INFO) << ". . using GravLift";
			staCost = 0;
		}
	}

	//Log(LOG_INFO) << ". check tuCost + stamina, etc. TU = " << tuCost;
	//Log(LOG_INFO) << ". unit->TU = " << _unit->getTimeUnits();
	if (tuCost - _pf->getOpenDoor() > _unit->getTimeUnits())
	{
		//Log(LOG_INFO) << ". . tuCost > _unit->TU()";
		if (_unit->getFaction() == FACTION_PLAYER
			&& _parent->getPanicHandled() == true
			&& tuTest < 255)
		{
			//Log(LOG_INFO) << ". send warning: not enough TU";
			_action.result = "STR_NOT_ENOUGH_TIME_UNITS";
		}

//		_unit->clearCache();
//		_parent->getMap()->cacheUnit(_unit);
		_pf->abortPath();
		_parent->popState();
		return false;
	}

	if (staCost > _unit->getEnergy())
	{
		//Log(LOG_INFO) << ". . staCost > _unit->getEnergy()";
		if (_unit->getFaction() == FACTION_PLAYER
			&& _parent->getPanicHandled() == true)
		{
			_action.result = "STR_NOT_ENOUGH_ENERGY";
		}

//		_unit->clearCache();
//		_parent->getMap()->cacheUnit(_unit);
		_pf->abortPath();
		_parent->popState();
		return false;
	}

	if (_parent->getPanicHandled() == true						// note this operates differently for player-units and non-player units;
		&& _unit->getFaction() != FACTION_PLAYER				// <- no Reserve tolerance.
		&& _parent->checkReservedTu(_unit, tuCost) == false)	// Only player's units will *bypass* abortPath() due to panicking ....
																// Tbh, other code should have rendered the getPanicHandled() redundant.
																// That is to say this should kick in *only* when player has actively
	{															// clicked to move but tries to go further than TUs allow; because
		//Log(LOG_INFO) << ". . checkReservedTu(_unit, tuCost) == false";	// either the AI or the panic-code should not try to
//		_unit->clearCache();												// move a unit farther than its [reserved] TUs would allow
//		_parent->getMap()->cacheUnit(_unit);
		_pf->abortPath();
		return false;
	}

	if (dir != _unit->getUnitDirection()	// unit is looking in the wrong way so turn first - unless strafe.
		&& dir < _pf->DIR_UP				// Do not use TurnBState because turning during walking doesn't cost TU.
		&& _action.strafe == false)
	{
		//Log(LOG_INFO) << ". . dir != _unit->getUnitDirection() -> turn";
		_unit->setDirectionTo(dir);
//		_unit->clearCache();
//		_parent->getMap()->cacheUnit(_unit);
		return false;
	}

	if (dir < _pf->DIR_UP) // now open doors if any
	{
		bool wait = false;

		int soundId;
		switch (_te->unitOpensDoor(_unit, false, dir))
		{
			case 0: // wooden door
				soundId = ResourcePack::DOOR_OPEN;
				break;
			case 1: // ufo door open
				wait = true;
				soundId = ResourcePack::SLIDING_DOOR_OPEN;
				break;
			case 2:	// ufo door still opening ...
				wait = true; // no break.
			default:
				soundId = -1;
		}

		if (soundId != -1)
			_parent->getResourcePack()->getSound("BATTLE.CAT", soundId)
										->play(-1, _parent->getMap()->getSoundAngle(pos));

		if (wait == true) return false; // wait for the ufo door to open
	}

	// proxy blows up in face after door opens - copied doStatusStand_end()
	if (_parent->checkProxyGrenades(_unit) == true) // kL_add: Put checkForSilacoid() here!
	{
		_parent->popState();
//		postPathProcedures(); // .. one or the other i suppose.
		return false;
	}

	//Log(LOG_INFO) << ". check size for obstacles";
	const int armorSize = _unit->getArmor()->getSize() - 1;
	for (int
			x = armorSize;
			x != -1;
			--x)
	{
		for (int
				y = armorSize;
				y != -1;
				--y)
		{
			//Log(LOG_INFO) << ". . check obstacle(unit)";
			const BattleUnit
				* const unitInMyWay = _battleSave->getTile(posStop + Position(x,y,0))->getTileUnit(),
				* unitBelowMyWay = nullptr;

			const Tile* const belowDest = _battleSave->getTile(posStop + Position(x,y,-1));
			if (belowDest != nullptr)
				unitBelowMyWay = belowDest->getTileUnit();

			// can't walk into units in this tile, or on top of other units sticking their head into this tile
			if (_falling == false
				&& ((unitInMyWay != nullptr
						&& unitInMyWay != _unit)
					|| (belowDest != nullptr
						&& unitBelowMyWay != nullptr
						&& unitBelowMyWay != _unit
						&& unitBelowMyWay->getHeight(true) - belowDest->getTerrainLevel() > 27)))
						// 4+ voxels poking into the tile above, we don't kick people in the head here at XCom.
						// kL_note: this appears to be only +2 in Pathfinding....
			{
				//Log(LOG_INFO) << ". . . obstacle(unit) -> abortPath()";
//				_action.TU = 0;
//				_unit->clearCache();
//				_parent->getMap()->cacheUnit(_unit);
				_pf->abortPath();
				_parent->popState();
				return false;
			}
		}
	}

	dir = _pf->dequeuePath();
	//Log(LOG_INFO) << ". dequeuePath() dir[0] = " << dir;

	if (_falling == true)
	{
		//Log(LOG_INFO) << ". . falling, _pf->DIR_DOWN";
		dir = _pf->DIR_DOWN;
	}
	//Log(LOG_INFO) << ". dequeuePath() dir[1] = " << dir;

	if (_unit->spendTimeUnits(tuCost) == true
		&& _unit->spendEnergy(staCost) == true)
	{
		//Log(LOG_INFO) << ". . WalkBState: spend TU & Energy -> establish tile-links";
		_preStepTurn = false;
		_playFly = false;

		//Log(LOG_INFO) << ". . WalkBState: startWalking()";
		_unit->startWalking(
						dir, posStop,
						_battleSave->getTile(pos + Position(0,0,-1)));

		//Log(LOG_INFO) << ". . WalkBState: establishTilesLink()";
		establishTilesLink();
	}
	//Log(LOG_INFO) << ". EXIT (dir!=-1) : " << _unit->getId();

	if (dir == Pathfinding::DIR_DOWN)
		_walkCam->setViewLevel(pos.z - 1);
	else
		_walkCam->setViewLevel(pos.z);

	return true;
}

/**
 * This continues unit movement.
 * Called from think()...
 * @return, true to continue moving, false to exit think()
 */
bool UnitWalkBState::doStatusWalk() // private.
{
	//Log(LOG_INFO) << "***** UnitWalkBState::doStatusWalk() : " << _unit->getId();
	if (_battleSave->getTile(_unit->getStopPosition())->getTileUnit() == nullptr // next tile must be not occupied
		// And, if not flying, the position directly below the tile must not be
		// occupied ... had that happen with a sectoid left standing in the air
		// because a cyberdisc was 2 levels below it.
		// btw, these have probably been already checked...
		|| _battleSave->getTile(_unit->getStopPosition())->getTileUnit() == _unit)
	{
		//Log(LOG_INFO) << ". WalkBState, keepWalking()";
		playMoveSound();
		_unit->keepWalking( // advances _walkPhase
						_battleSave->getTile(_unit->getPosition() + Position(0,0,-1)),
						_isVisible);
	}
	else if (_falling == false) // walked into an unseen unit
	{
		//Log(LOG_INFO) << ". WalkBState, !falling Abort path; another unit is blocking path";
		clearTilesLink(false);

		_unit->setDirectionTo( // turn to blocking unit
						_unit->getStopPosition(),
						_unit->getTurretType() != -1);

		_pf->abortPath();
		_unit->setUnitStatus(STATUS_STANDING);
	}

	//Log(LOG_INFO) << ". . unitPos " << _unit->getPosition();
	// unit moved from one tile to the other, update the tiles & investigate new flooring
	if (_tileSwitchDone == false
		&& _unit->getPosition() != _unit->getStartPosition())
	{
		//Log(LOG_INFO) << ". tile switch from _lastpos to _destination";
		_tileSwitchDone = true;

		Tile* tile;
		const Tile* tileBelow;
		const int armorSize = _unit->getArmor()->getSize() - 1;

		for (int
				x = armorSize;
				x != -1;
				--x)
		{
			for (int
					y = armorSize;
					y != -1;
					--y)
			{
				//Log(LOG_INFO) << ". . remove unit from previous tile";
				tile = _battleSave->getTile(_unit->getStartPosition() + Position(x,y,0));
				tile->setUnit(nullptr);
				tile->setTransitUnit(_unit); // IMPORTANT: lastTile transiently holds onto this unit (all quads) for Map drawing.
			}
		}

		bool doFallCheck = true;
		for (int
				x = armorSize;
				x != -1;
				--x)
		{
			for (int
					y = armorSize;
					y != -1;
					--y)
			{
				tile = _battleSave->getTile(_unit->getPosition() + Position(x,y,0));
				tileBelow = _battleSave->getTile(_unit->getPosition() + Position(x,y,-1));
				if (tile->hasNoFloor(tileBelow) == false)
				{
					//Log(LOG_INFO) << ". . . hasFloor ( doFallCheck set FALSE )";
					doFallCheck = false;
				}

				//Log(LOG_INFO) << ". . set unit on new tile";
				tile->setUnit(_unit, tileBelow);
				//Log(LOG_INFO) << ". . . NEW unitPos " << _unit->getPosition();
			}
		}

		_falling = doFallCheck == true
				&& _pf->getMoveTypePf() != MT_FLY
				&& _unit->getPosition().z != 0;

		if (_falling == true)
		{
			//Log(LOG_INFO) << ". falling";
			for (int
					x = armorSize;
					x != -1;
					--x)
			{
				for (int
						y = armorSize;
						y != -1;
						--y)
				{
					tileBelow = _battleSave->getTile(_unit->getPosition() + Position(x,y,-1));
					//if (tileBelow) Log(LOG_INFO) << ". . otherTileBelow exists";
					if (tileBelow != nullptr
						&& tileBelow->getTileUnit() != nullptr)
					{
						//Log(LOG_INFO) << ". . . another unit already occupies lower tile";
						clearTilesLink(true);

						_falling = false;

						_pf->dequeuePath();
						_battleSave->addFallingUnit(_unit);

						//Log(LOG_INFO) << "UnitWalkBState::think(), addFallingUnit() ID " << _unit->getId();
						_parent->statePushFront(new UnitFallBState(_parent));
						return false;
					}
					//else Log(LOG_INFO) << ". . otherTileBelow Does NOT contain other unit";
				}
			}
		}
	}

	return true;
}

/**
 * This function ends unit movement.
 * Called from think()...
 * @return, true to continue moving, false to exit think()
 */
bool UnitWalkBState::doStatusStand_end() // private.
{
	//Log(LOG_INFO) << "***** UnitWalkBState::doStatusStand_end() : " << _unit->getId();
	if (_unit->getFaction() != FACTION_PLAYER)
		_unit->setUnitVisible(false);
	else
	{
		const BattlescapeState* const battleState = _battleSave->getBattleState();

		double stat = static_cast<double>(_unit->getBattleStats()->tu);
		const int tu = _unit->getTimeUnits();
		battleState->getTuField()->setValue(static_cast<unsigned>(tu));
		battleState->getTuBar()->setValue(std::ceil(
											static_cast<double>(tu) / stat * 100.));

		stat = static_cast<double>(_unit->getBattleStats()->stamina);
		const int energy = _unit->getEnergy();
		battleState->getEnergyField()->setValue(static_cast<unsigned>(energy));
		battleState->getEnergyBar()->setValue(std::ceil(
											static_cast<double>(energy) / stat * 100.));
	}

	const Position pos = _unit->getPosition();

	if (_falling == false
		&& _unit->getSpecialAbility() == SPECAB_BURN) // if the unit burns floortiles, burn floortiles
	{
		// Put burnedBySilacoid() here! etc
		const int power (_unit->getUnitRules()->getSpecabPower());
		_unit->getTile()->ignite(power / 10);
		const Position targetVoxel = Position::toVoxelSpaceCentered(
																pos,
																-_unit->getTile()->getTerrainLevel());
		_parent->getTileEngine()->hit(
									targetVoxel,
									power,
									DT_IN,
									_unit);

		if (_unit->getUnitStatus() != STATUS_STANDING)	// ie: burned a hole in the floor and fell through it
		{												// Trace TileEngine::hit() through applyGravity() etc. to determine unit-status.
//			_action.TU = 0;
			_pf->abortPath();
//			_unit->setCache(0);
//			_parent->getMap()->cacheUnit(_unit);
//			_parent->popState();
			return false;
		}
	}

	_te->calculateUnitLighting();

	if (_unit->getTimeUnits() < 4)
		_walkCam->centerOnPosition(_unit->getPosition()); // KLUDGE!
		// Okay, better write something about this. When the last aLien unit to
		// do its AI (or perhaps a Civie) is marked unselectable in handleUnitAI()
		// or thereabouts, but it moves into Player-unit's view when *entering
		// its last tile* the Hidden Movement is revealed ... but it won't be
		// centered because it didn't start its walk-step in Player view. This,
		// simply by checking if the aLien is low on TU, allows a forced
		// "center on Position" to be done here. (A forced center otherwise
		// would cause the camera to jolt along with each tile-step.)
		//
		// That works in conjunction with the extended-reveal granted by
		// Game::delayBlit(), btw.
	else
		_walkCam->setViewLevel(pos.z);

	// This needs to be done *before* the calculateFOV(pos) or else any newVis will
	// be marked Visible before visForUnits() catches the new unit that is !Visible.
	bool newVis = visForUnits();

/*	// debug -->
	BattleUnit* hostile;
	for (std::vector<BattleUnit*>::const_iterator
			i = _battleSave->getUnits()->begin();
			i != _battleSave->getUnits()->end();
			++i)
	{
		if ((*i)->getId() == 1000007)
		{
			Log(LOG_INFO) << ". dist = " << TileEngine::distance(pos, (*i)->getPosition());
			break;
		}
	} // debug_end. */

	// This calculates or 'refreshes' the Field of View of all units within
	// maximum distance (20 tiles) of current unit.
	_te->calculateFOV(pos, true);

	if (_parent->checkProxyGrenades(_unit) == true) // Put checkForSilacoid() here!
	{
		_parent->popState();
		return false;
	}

	if (newVis == true)
	{
		//if (_unit->getFaction() == FACTION_PLAYER) Log(LOG_INFO) << ". . _newVis TRUE, Abort path";
		//else if (_unit->getFaction() != FACTION_PLAYER) Log(LOG_INFO) << ". . _newUnitSpotted TRUE, Abort path";
		_unit->clearCache();
		_parent->getMap()->cacheUnit(_unit);

		_pf->abortPath();
		_parent->popState();
		return false;
	}

	if (_falling == false) // check for reaction fire
	{
		//Log(LOG_INFO) << ". . WalkBState: NOT falling, checkReactionFire()";
		if (_te->checkReactionFire(_unit) == true) // unit got fired upon - stop walking
		{
			//Log(LOG_INFO) << ". . . RF triggered - cacheUnit/pop state";
			_unit->clearCache();
			_parent->getMap()->cacheUnit(_unit);

			_pf->abortPath();
			_parent->popState();
			return false;
		}
		//else Log(LOG_INFO) << ". . WalkBState: checkReactionFire() FALSE - no caching";
	}

	return true;
}

/**
 * This function turns unit during movement.
 * Called from think()
 */
void UnitWalkBState::doStatusTurn() // private.
{
	//Log(LOG_INFO) << "***** UnitWalkBState::doStatusTurn() : " << _unit->getId();
	if (_preStepTurn == true)	// turning during walking costs no tu
		++_preStepCost;			// except before the first step.

	_unit->turn();

	_unit->clearCache();
	_parent->getMap()->cacheUnit(_unit);

	// calculateFOV() is unreliable for setting the _newUnitSpotted bool as it
	// can be called from various other places in the code, ie: doors opening
	// (& explosions/terrain destruction) and that messes up the result.
	// But let's do it anyway! ps. Fixed
	if (visForUnits() == true)
	{
		if (_preStepTurn == true)
		{
			_unit->spendTimeUnits(_preStepCost);

			_preStepCost = 0;
			_preStepTurn = false;
		}

		//if (_unit->getFaction() == FACTION_PLAYER) Log(LOG_INFO) << ". . _newVis = TRUE, Abort path, popState";
		//else if (_unit->getFaction() != FACTION_PLAYER) Log(LOG_INFO) << ". . _newUnitSpotted = TRUE, Abort path, popState";

		if (_unit->getFaction() != FACTION_PLAYER)
			_unit->setHiding(false);

//		_unit->clearCache();
//		_parent->getMap()->cacheUnit(_unit);
		_pf->abortPath();
		_unit->setUnitStatus(STATUS_STANDING);
		_parent->popState();
	}
	else if (_unit->getFaction() == FACTION_PLAYER
		&& _parent->getPanicHandled() == true)
	{
		BattlescapeState* const battleState (_parent->getBattlescapeState());
		if (battleState->playableUnitSelected() == true)
		{
			battleState->hotSqrsClear();
			battleState->hotSqrsUpdate();
		}
	}
}

/**
 * Handles some calculations when the path is finished.
 */
void UnitWalkBState::postPathProcedures() // private.
{
	//Log(LOG_INFO) << "UnitWalkBState::postPathProcedures(), unit = " << _unit->getId();
	_action.TU = 0;

	if (_unit->getFaction() != FACTION_PLAYER)
	{
		int dir = _action.finalFacing;

		if (_action.finalAction == true)
			_unit->dontReselect();

		if (_unit->getChargeTarget() != nullptr)
		{
			//Log(LOG_INFO) << ". . charging = TRUE";
			const Position posTarget = _unit->getChargeTarget()->getPosition();
			dir = TileEngine::getDirectionTo(_unit->getPosition(), posTarget);
			// kL_notes (pre-above):
			// put an appropriate facing direction here
			// don't stare at a wall. Get if aggro, face closest xCom op <- might be done somewhere already.
			// Cheat: face closest xCom op based on a percentage (perhaps alien 'value' or rank)
			// cf. void AggroBAIState::setAggroTarget(BattleUnit *unit)
			// and bool TileEngine::calculateFOV(BattleUnit *unit)

			if (_parent->getTileEngine()->validMeleeRange(
													_unit, dir,
													_action.actor->getChargeTarget()) == true)
			{
				BattleAction action;
				action.actor = _unit;
				action.target = posTarget;
				action.targeting = true;
				action.type = BA_HIT;
				action.weapon = _unit->getMainHandWeapon();
//				action.weapon = _unit->getMeleeWeapon();

// if (action.weapon == nullptr)
				const std::string meleeWeapon = _unit->getMeleeWeapon();
				bool instaWeapon = false;

				if (meleeWeapon == "STR_FIST")
					action.weapon = _parent->getFist();
				else if (meleeWeapon.empty() == false)
				{
					bool found = false;

					for (std::vector<BattleItem*>::const_iterator
							i = _unit->getInventory()->begin();
							i != _unit->getInventory()->end();
							++i)
					{
						if ((*i)->getRules()->getType() == meleeWeapon)
						{
							// note this ought be conformed w/ bgen.addAlien equipped items to
							// ensure radical (or standard) BT_MELEE weapons get equipped in hand;
							// but for now just grab the meleeItem wherever it was equipped ...
							found = true;
							action.weapon = *i;
							break;
						}
					}

					if (found == false)
					{
						instaWeapon = true;
						action.weapon = new BattleItem(
													_parent->getRuleset()->getItem(meleeWeapon),
													_battleSave->getNextItemId());
						action.weapon->setOwner(_unit);
					}
				}
				else if (action.weapon != nullptr
					&& action.weapon->getRules()->getBattleType() != BT_MELEE
					&& action.weapon->getRules()->getBattleType() != BT_FIREARM) // probly shouldn't be here <-
				{
					action.weapon = nullptr;
				}


				_unit->setChargeTarget(nullptr);

				if (action.weapon != nullptr) // also checked in getActionTu() & ProjectileFlyBState::init()
				{
					action.TU = _unit->getActionTu(action.type, action.weapon);
					_parent->statePushBack(new ProjectileFlyBState(_parent, action));

					if (instaWeapon == true)
						_battleSave->removeItem(action.weapon);
				}
			}
		}
		else if (_unit->isHiding() == true)
		{
//			dir = _unit->getUnitDirection() + 4; // just remove this so I don't have to look at Sectopod arses.
			_unit->setHiding(false);
			_unit->dontReselect();
		}

		if (dir == -1)
			dir = getFinalDirection();

		if (dir != -1)
		{
			_unit->setDirectionTo(dir % 8);
			while (_unit->getUnitStatus() == STATUS_TURNING)
			{
				_unit->turn();
				_parent->getTileEngine()->calculateFOV(_unit);
				// might need newVis/newUnitSpotted -> abort
			}
		}
	}
	else if (_parent->getPanicHandled() == false) // is Faction_Player
		_unit->setTimeUnits(0);


	_te->calculateUnitLighting();
	_te->calculateFOV(_unit->getPosition(), true); // in case unit opened a door and stopped without doing Status_WALKING

	_unit->clearCache();
	_parent->getMap()->cacheUnit(_unit);

	if (_falling == false)
		_parent->popState();
}

/**
 * Gets a suitable final facing direction for aLiens.
 * @note This does not require any TU cost.
 * @return, direction to face
 */
int UnitWalkBState::getFinalDirection() const // private.
{
	const int diff = static_cast<int>(_parent->getBattlescapeState()->getSavedGame()->getDifficulty());
	if (RNG::percent((diff + 1) * 20 - _unit->getRankInt() * 5) == true)
	{
		const BattleUnit* facedUnit = nullptr;
		int
			distSqr = 100000,
			distTest;

		for (std::vector<BattleUnit*>::const_iterator
				i = _battleSave->getUnits()->begin();
				i != _battleSave->getUnits()->end();
				++i)
		{
			if ((*i)->getFaction() == FACTION_PLAYER
				&& (*i)->isOut_t(OUT_STAT) == false
				&& (*i)->getExposed() != -1
				&& (*i)->getExposed() <= _unit->getIntelligence())
			{
				distTest = TileEngine::distanceSqr(
												_unit->getPosition(),
												(*i)->getPosition());
				if (distTest < distSqr)
				{
					distSqr = distTest;
					facedUnit = *i;
				}
			}
		}

		if (facedUnit != nullptr)
			return TileEngine::getDirectionTo(_unit->getPosition(), facedUnit->getPosition());
	}

	return -1;
}

/**
 * Checks visibility for new opponents.
 * @return, true if a new enemy is spotted
 */
bool UnitWalkBState::visForUnits() const // private.
{
	if (_falling == true
		|| _parent->getPanicHandled() == false)	// note: _playerPanicHandled can be false only on Player's turn
	{											// so if expression== TRUE then it's a player's turn.
		return false;
	}

	bool ret = _te->calculateFOV(_unit);

	if (_unit->getFaction() != FACTION_PLAYER)
	{
		ret = ret
		   && _unit->getHostileUnitsThisTurn().size() > _unitsSpotted
		   && _action.desperate == false
		   && _unit->getChargeTarget() == nullptr;
	}

	return ret;
}

/**
 * Sets the animation speed of units.
 * @param gravLift - true if moving up/down a gravLift
 */
void UnitWalkBState::setWalkSpeed(bool gravLift) const // private.
{
	Uint32 interval;
	if (_unit->getFaction() == FACTION_PLAYER)
	{
		if (_action.dash == true
			|| (_unit->getUnitRules() != nullptr
				&& _unit->getUnitRules()->isDog() == true))
		{
			interval = _parent->getBattlescapeState()->STATE_INTERVAL_XCOMDASH;
		}
		else
			interval = _parent->getBattlescapeState()->STATE_INTERVAL_XCOM;
	}
	else
		interval = _parent->getBattlescapeState()->STATE_INTERVAL_ALIEN;

	if (gravLift == true)
		interval *= 2;

	//Log(LOG_INFO) << "unitWalkB: setWalkSpeed() set interval = " << interval;
	_parent->setStateInterval(interval);
}

/**
 * Handles walking/ flying/ other movement sounds.
 */
void UnitWalkBState::playMoveSound() // private.
{
	const int phase = _unit->getWalkPhase();

	if (_unit->getUnitVisible() == true
		|| _battleSave->getDebugMode() == true)
	{
		int soundId = -1;

		if (_unit->getMoveSound() != -1)
		{
			if (phase == 0)
			{
				if (_unit->getUnitStatus() == STATUS_FLYING
					&& _unit->isFloating() == false
					&& _falling == false)
				{
					soundId = ResourcePack::GRAVLIFT_SOUND; // GravLift note: isFloating() might be redundant w/ (_falling=false). See below_
				}
				else
					soundId = _unit->getMoveSound();
			}
		}
		else
		{
			if (_unit->getUnitStatus() == STATUS_WALKING)
			{
				_playFly = true;
				if (phase == 3 || phase == 7)
				{
					const Tile
						* const tile = _unit->getTile(),
						* const tileBelow = _battleSave->getTile(tile->getPosition() + Position(0,0,-1));

					const int stepSound = tile->getFootstepSound(tileBelow);
					if (stepSound > -1)
					{
						if (phase == 3)
							soundId = stepSound * 2 + ResourcePack::WALK_OFFSET + 1;
						else // phase == 7
							soundId = stepSound * 2 + ResourcePack::WALK_OFFSET;
					}
				}
			}
			else if (_unit->getUnitStatus() == STATUS_FLYING)
			{
				if (phase == 0 || _playFly == true)
				{
					_playFly = false;
					if (_falling == false)
					{
						if (_unit->isFloating() == false) // GravLift note: isFloating() might be redundant w/ (_falling=false). See above^
							soundId = ResourcePack::GRAVLIFT_SOUND;
						else
						{
							if (_unit->getUnitRules() != nullptr
								&& _unit->getUnitRules()->isMechanical() == true)
							{
								soundId = ResourcePack::FLYING_SOUND;		// hoverSound flutter
							}
							else
								soundId = ResourcePack::FLYING_SOUND_HQ;	// HQ hoverSound
						}
					}
				}
				else if (phase == 7
					&& groundCheck() == true
					&& (_falling == true
						|| (_unit->isFloating() == true
							&& _pf->getMoveTypePf() == MT_WALK)))
				{
					soundId = ResourcePack::ITEM_DROP; // *thunk*
				}
			}
		}

		if (soundId != -1)
			_parent->getResourcePack()->getSound("BATTLE.CAT", soundId)
										->play(-1, _parent->getMap()->getSoundAngle(_unit->getPosition()));
	}
}

/**
 * For determining if a flying unit turns flight off at start of movement.
 * @note '_falling' should always be false when this is called in init().
 * @note And unit must be capable of flight for this to be relevant.
 * @note This could get problematic if/when falling onto nonFloors like water
 * and/or if there is another unit on tileBelow.
 */
void UnitWalkBState::doFallCheck() // private.
{
	if (_pf->getMoveTypePf() != MT_FLY
		&& _unit->getPosition().z != 0
		&& groundCheck() == false)
	{
		_falling = true;
	}
}

/**
 * Checks if there is ground below when unit is falling.
 * @note Pathfinding already has a function canFallDown() that could be used.
 @return, true if unit hits a Floor
 */
bool UnitWalkBState::groundCheck() const // private.
{
	const Tile* tileBelow;
	Position pos;

	const int unitSize = _unit->getArmor()->getSize() - 1;
	for (int
			x = unitSize;
			x != -1;
			--x)
	{
		for (int
				y = unitSize;
				y != -1;
				--y)
		{
			pos = _unit->getPosition() + Position(x,y,0);
			tileBelow = _battleSave->getTile(pos + Position(0,0,-1));
			if (_battleSave->getTile(pos + Position(0,0,0))->hasNoFloor(tileBelow) == false)
				return true;
		}
	}

	return false;
}

/**
 * Establishes unit's transient link(s) to its destination Tile(s).
 */
void UnitWalkBState::establishTilesLink() const // private.
{
	//Log(LOG_INFO) << "UnitWalkBState::establishTilesLink()";
	const int armorSize = _unit->getArmor()->getSize() - 1;
	for (int
			x = armorSize;
			x != -1;
			--x)
	{
		for (int
				y = armorSize;
				y != -1;
				--y)
		{
			_battleSave->getTile(_unit->getStopPosition() + Position(x,y,0))->setTransitUnit(_unit);
		}
	}
}

/**
 * Clears unit's transient link(s) to other Tile(s).
 * @param origin - true if previous tile is cleared; false for destination tile
 */
void UnitWalkBState::clearTilesLink(bool origin) const // private.
{
	//Log(LOG_INFO) << "UnitWalkBState::clearTilesLink()";
	std::vector<Position> posCurrent;
	Position
		pos = _unit->getPosition(),
		posTest;

	const int armorSize = _unit->getArmor()->getSize() - 1;
	for (int
			x = armorSize;
			x != -1;
			--x)
	{
		for (int
				y = armorSize;
				y != -1;
				--y)
		{
			posCurrent.push_back(pos + Position(x,y,0));
		}
	}

	if (origin == true)
		pos = _unit->getStartPosition();
	else
		pos = _unit->getStopPosition();

	for (int
			x = armorSize;
			x != -1;
			--x)
	{
		for (int
				y = armorSize;
				y != -1;
				--y)
		{
			posTest = pos + Position(x,y,0);
			if (std::find(
						posCurrent.begin(),
						posCurrent.end(),
						posTest) == posCurrent.end())
			{
				_battleSave->getTile(posTest)->setTransitUnit(nullptr);
			}
		}
	}
}

}
