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

//bool UnitWalkBState::_debug = false; // static.

/**
 * Sets up the UnitWalkBState.
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
		_fall(false),
		_preStepTurn(false),
		_preStepCost(0),
		_tileSwitchDone(false),
		_isVisible(false),
		_walkCamera(parent->getMap()->getCamera()),
		_dirStart(-1),
		_kneelCheck(true),
		_playFly(false),
		_door(false),
		_tilesLinked(false)
{
//	Log(LOG_INFO) << "";
//	Log(LOG_INFO) << "walkB:cTor id-" << _unit->getId();
//	if (_unit->getId() == 1000028)	_debug = true;
//	else							_debug = false;
}

/**
 * Deletes this UnitWalkBState.
 */
UnitWalkBState::~UnitWalkBState()
{
	//Log(LOG_INFO) << "walkB: dTor" << _unit->getId();
}

/**
 * Gets the name of this BattleState.
 * @return, label of the substate
 */
std::string UnitWalkBState::getBattleStateLabel() const
{
	std::ostringstream oststr;
	oststr << "UnitWalkBState";
	if (_action.actor != nullptr) oststr << " id-" << _action.actor->getId();
	else oststr << " - Actor INVALID";

	return oststr.str();
}

/**
 * Initializes this BattleState.
 */
void UnitWalkBState::init()
{
//	if (_debug)
//	{
//		Log(LOG_INFO) << "";
//		Log(LOG_INFO) << "walkB:init() id-" << _unit->getId();
//		Log(LOG_INFO) << ". " << _unit->getPosition() << " to " << _action.posTarget;
//	}

	_pf->setPathingUnit(_unit);
	_dirStart = _pf->getStartDirection();

	if (_action.strafe == false						// not strafing
		&& _dirStart > -1 && _dirStart < 8			// moving but not up or down
		&& _dirStart != _unit->getUnitDirection())	// not facing in direction of movement
	{												// NOTE: If unit is not facing in the direction that it's about to walk
		_preStepTurn = true;						// toward ... this makes the unit expend TUs if it spots a new aLien
	}												// when turning but stops before actually walking. Also expends TU if
													// cancel() is called before first step.
	doFallCheck();
}

/**
 * Runs BattleState functionality every cycle.
 */
void UnitWalkBState::think()
{
//	if (_debug)
//	{
//		Log(LOG_INFO) << "";
//		Log(LOG_INFO) << "***** UnitWalkBState::think()\t\t\t\tid-" << _unit->getId()
//					  << " " << _unit->getPosition() << " to " << _unit->getStopPosition()
//					  << " vis= " << _unit->getUnitVisible();
//	}

	_isVisible = _unit->getUnitVisible() == true
			  || _battleSave->getDebugTac() == true;

	if (_unit->getHealth() == 0 || _unit->isStunned() == true)
	{
		//Log(LOG_INFO) << ". . isOut() abort.";
		abortState(false);
		return;
	}


// #2 _oO **** STATUS WALKING **** Oo_

	switch (_unit->getUnitStatus())
	{
		case STATUS_WALKING:
		case STATUS_FLYING:
			//Log(LOG_INFO) << "STATUS_WALKING or FLYING : " << _unit->getId();
//			if (_isVisible == true)
//			{
//				const int stopZ = _unit->getStopPosition().z;
//				if (_walkCamera->isOnScreen(_unit->getPosition()) == true
//					&& _walkCamera->getViewLevel() < stopZ)
//				{
//					_walkCamera->setViewLevel(stopZ);
//				}
//			}

			if (statusWalk() == false)
			{
				//Log(LOG_INFO) << ". . statusWalk() FALSE return";
				return;
			}


// #3 _oO **** STATUS STANDING end **** Oo_

			// walkPhase reset as the unit completes its transition to the next tile
			if (_unit->getUnitStatus() == STATUS_STANDING)
			{
				//Log(LOG_INFO) << "STATUS_STANDING_end in UnitWalkBState _WALKING or _FLYING !!!" ;
				clearTilesLink(true);

//				if (_isVisible == true)
//				{
//					const Position pos = _unit->getPosition();
//
//					if (_unit->getFaction() != FACTION_PLAYER
//						&& _walkCamera->isOnScreen(pos) == false)
//					{
//						_walkCamera->centerPosition(pos);
//						_walkCamera->setViewLevel(_unit->getStopPosition().z);
//					}
//					else if (_walkCamera->isOnScreen(pos) == true)
//					{
//						const int stopZ = _unit->getStopPosition().z;
//						if (_walkCamera->getViewLevel() > stopZ
//							&& (_pf->getPath().size() == 0 || _pf->getPath().back() != Pathfinding::DIR_UP))
//						{
//							_walkCamera->setViewLevel(stopZ);
//						}
//					}
//				}

				if (statusStand_end() == false)
				{
					//Log(LOG_INFO) << ". . statusStand_end() FALSE return";
					return;
				}

				if (_unit->getFaction() == FACTION_PLAYER)
				{
					_parent->getBattlescapeState()->clearHostileIcons();

					if (_parent->playerPanicHandled() == true)
						_parent->getBattlescapeState()->updateHostileIcons();
				}
			}
			else if (_isVisible == true) // keep walking ... make sure the unit sprites are up to date
			{
				//Log(LOG_INFO) << ". _isVisible : still walking ...";
//				if (_pf->getStrafeMove() == true) // NOTE: This could be trimmed, because I had to make tanks use getFaceDirection() in UnitSprite::drawRoutine2() anyway ...
				if (_action.strafe == true)
				{
					//Log(LOG_INFO) << ". WALKING strafe, unitDir = " << _unit->getUnitDirection();
					//Log(LOG_INFO) << ". WALKING strafe, faceDir = " << _unit->getFaceDirection();
					const int dirStrafe (_unit->getUnitDirection()); // direction of travel
					_unit->setUnitDirection(
										_unit->getFaceDirection(),
										false);

//					_unit->setCacheInvalid(); // might play around with Strafe anim's ......
					_parent->getMap()->cacheUnitSprite(_unit);
					_unit->setUnitDirection(dirStrafe, false);
				}
				else
				{
					//Log(LOG_INFO) << ". WALKING no strafe, cacheUnitSprite()";
					_unit->setCacheInvalid(); // might play around with non-Strafe anim's ......
					_parent->getMap()->cacheUnitSprite(_unit);
				}
			}
	}


// #1 & #4 _oO **** STATUS STANDING **** Oo_

	switch (_unit->getUnitStatus())
	{
		case STATUS_STANDING:
		case STATUS_PANICKING:
			//Log(LOG_INFO) << "STATUS_STANDING or PANICKING : " << _unit->getId();
			if (statusStand() == false)
			{
				//Log(LOG_INFO) << ". . statusStand() FALSE return";
				return;
			}

			// Destination is not valid until *after* statusStand() runs.
//			if (_isVisible == true)
//			{
//				//Log(LOG_INFO) << ". onScreen";
//				const Position pos = _unit->getPosition();
//
//				if (_unit->getFaction() != FACTION_PLAYER
//					&& _walkCamera->isOnScreen(pos) == false)
//				{
//					_walkCamera->centerPosition(pos);
//					_walkCamera->setViewLevel(pos.z);
//				}
//				else if (_walkCamera->isOnScreen(pos) == true) // is Faction_Player
//				{
//					const int stopZ = _unit->getStopPosition().z;
//					if (pos.z == stopZ || (pos.z < stopZ && _walkCamera->getViewLevel() < stopZ))
//					{
//						_walkCamera->setViewLevel(pos.z);
//					}
//				}
//			}
	}


// _oO **** STATUS TURNING **** Oo_

	if (_unit->getUnitStatus() == STATUS_TURNING) // turning during walking costs no TU
	{
		//Log(LOG_INFO) << "STATUS_TURNING : " << _unit->getId();
		statusTurn();
	}
	//Log(LOG_INFO) << "think() : " << _unit->getId() << " EXIT ";
}

/**
 * Aborts unit walking.
 */
void UnitWalkBState::cancel()
{
	if (_battleSave->getSide() == FACTION_PLAYER
		&& _parent->playerPanicHandled() == true)
	{
		if (_preStepTurn == true)
		{
			_unit->expendTu(_preStepCost);

			_preStepCost = 0;
			_preStepTurn = false;
		}
		_pf->abortPath();
	}
}

/**
 * Begins unit-walk and may also end unit-walk.
 * @note Called from think().
 * @return, true to continue moving, false to exit think()
 */
bool UnitWalkBState::statusStand() // private.
{
	const Position& posStart (_unit->getPosition());
//	if (_debug)
//	{
//		Log(LOG_INFO) << "***** UnitWalkBState::statusStand()\t\tid-" << _unit->getId()
//					  << " " << posStart
//					  << " vis= " << _unit->getUnitVisible();
//	}

	bool gravLift (false);

	int dir (_pf->getStartDirection());
	//Log(LOG_INFO) << ". StartDirection = " << dir;
	if (_fall == true)
	{
		dir = Pathfinding::DIR_DOWN;
		//Log(LOG_INFO) << ". . _fall, dir = " << dir;
	}
	else if (dir == -1) // end.
	{
		//Log(LOG_INFO) << ". dir = " << _unit->getUnitDirection();
		//Log(LOG_INFO) << ". . CALL postPathProcedures()";
		postPathProcedures();
		return false;
	}
	else // about to start.
	{
		if (visForUnits() == true)
		{
			//if (_unit->getFaction() == FACTION_PLAYER) Log(LOG_INFO) << ". . _newVis = TRUE, postPathProcedures";
			//else Log(LOG_INFO) << ". . _newUnitSpotted = TRUE, postPathProcedures";

//			if (_unit->getFaction() != FACTION_PLAYER)
			_unit->setHiding(false);

			_unit->setCacheInvalid();
			_parent->getMap()->cacheUnitSprite(_unit);

			postPathProcedures();	// NOTE: This is the only call for which _door==TRUE might be needed.
			return false;			// Update: '_door' is also used for calcFovTiles_pos() in statusStand_end().
		}

		const Tile* const tile (_battleSave->getTile(posStart));
		gravLift = dir >= Pathfinding::DIR_UP						// Assumes tops & bottoms of gravLifts always have floors/ceilings.
				&& tile->getMapData(O_FLOOR) != nullptr				// that is, gravLifts on roofs of UFOs can screw this up.
				&& tile->getMapData(O_FLOOR)->isGravLift() == true;	// Unless a further check is made for a gravLift-floor on the stopTile.

		if (_kneelCheck == true) // check if unit is kneeled
		{
			//Log(LOG_INFO) << ". do kneelCheck";
			_kneelCheck = false;

			if (_unit->isKneeled() == true			// unit is kneeled
				&& gravLift == false				// not on a gravLift
				&& _pf->getPath().empty() == false)	// not the final tile of path; that is, the unit is actually going to move. Might be unnecessary after refactor above^
			{
				//Log(LOG_INFO) << ". . kneeled and path Valid";
				if (_parent->kneelToggle(_unit) == true)
				{
					//Log(LOG_INFO) << ". . . Stand up";
					if (_te->checkReactionFire(_unit) == true) // got fired at -> stop.
					{
						//Log(LOG_INFO) << ". . . RF triggered";
						_battleSave->rfTriggerOffset(_walkCamera->getMapOffset());

						abortState(false);
						return false;
					}
				}
				else
				{
					//Log(LOG_INFO) << ". . don't stand: not enough TU";
					_action.result = BattlescapeGame::PLAYER_ERROR[0u];	// NOTE: redundant w/ bg:kneelToggle() error messages ...
																		// But '_action.result' might need to be set for bg:popBattleState() to deal with stuff.
					abortState(false);
					return false;
				}
			}
		}

		if (_action.strafe == true)
		{
			if (_unit->getGeoscapeSoldier() != nullptr
				|| _unit->getUnitRules()->isMechanical() == false)
			{
				_unit->setFaceDirection(_unit->getUnitDirection());
				const int delta (std::min(
										std::abs(8 + _dirStart - _unit->getUnitDirection()),
										std::min(
											std::abs(_unit->getUnitDirection() - _dirStart),
											std::abs(8 + _unit->getUnitDirection() - _dirStart))));
				if (delta > 2) _unit->flagStrafeBackwards();
			}
			else // turret-swivel.
			{
				const int dirStrafe ((_dirStart + 4) % 8);
				_unit->setFaceDirection(dirStrafe);

				if (_unit->getTurretType() != TRT_NONE)
				{
					const int dirTurret (_unit->getTurretDirection() - _unit->getUnitDirection());
					_unit->setTurretDirection((dirTurret + dirStrafe) % 8);
				}
			}
		}
	}

	_tileSwitchDone = false;

	//Log(LOG_INFO) << "enter (dir!= -1) : " << _unit->getId();

	//Log(LOG_INFO) << ". getTuCostPf() & posStop";
	Position posStop;
	int
		tuCost (_pf->getTuCostPf(posStart, dir, &posStop)), // gets tu cost but also sets the destination position.
		tuTest,
		enCost;
	//Log(LOG_INFO) << ". tuCost = " << tuCost;

	if (_fall == true)
	{
		//Log(LOG_INFO) << ". . falling, set tuCost 0";
		tuCost =
		tuTest =
		enCost = 0;
	}
	else
	{
		Tile* const destTile (_battleSave->getTile(posStop));

		if (destTile != nullptr // would hate to see what happens if destTile=nullptr, nuclear war no doubt.
			&& destTile->getFire() != 0
			&& _unit->avoidsFire() == true)
		{
			//Log(LOG_INFO) << ". . subtract tu inflation for a fireTile";
			// The TU-cost was artificially inflated by 32 points in _pf::getTuCostPf()
			// so it has to be deflated again here under the same conditions.
			// See: Pathfinding::getTuCostPf() where TU cost was inflated.
			tuCost -= Pathfinding::TU_FIRE_AVOID;
		}

		tuTest =
		enCost = tuCost;

		if (gravLift == false)
		{
			if (_action.dash == true // allow dash when moving vertically 1 tile (or more).
				|| (_action.strafe == true && dir >= Pathfinding::DIR_UP))
			{
				tuCost -= _pf->getDoorCost();
				tuCost = ((tuCost * 3) >> 2u) + _pf->getDoorCost();

				enCost -= _pf->getDoorCost();
				enCost = ((enCost * 3) >> 1u);
			}
			if ((enCost -= _unit->getArmor()->getAgility()) < 0) enCost = 0;
		}
		else // gravLift
		{
			//Log(LOG_INFO) << ". . using GravLift";
			enCost = 0;
		}

		//Log(LOG_INFO) << ". check tuCost + stamina, etc. TU = " << tuCost;
		//Log(LOG_INFO) << ". unit->TU = " << _unit->getTu();
		static const int FAIL (255);
		if (tuCost - _pf->getDoorCost() > _unit->getTu())
		{
			//Log(LOG_INFO) << ". . tuCost > _unit->TU()";
			if (_unit->getFaction() == FACTION_PLAYER
				&& _parent->playerPanicHandled() == true
				&& tuTest < FAIL)
			{
				//Log(LOG_INFO) << ". send warning: not enough TU";
				_action.result = BattlescapeGame::PLAYER_ERROR[0u];
			}
			abortState();
			return false;
		}

		if (enCost - _pf->getDoorCost() > _unit->getEnergy())
		{
			//Log(LOG_INFO) << ". . enCost > _unit->getEnergy()";
			if (_unit->getFaction() == FACTION_PLAYER
				&& _parent->playerPanicHandled() == true)
			{
				_action.result = BattlescapeGame::PLAYER_ERROR[1u];
			}
			abortState();
			return false;
		}
	}

	if (//_parent->playerPanicHandled() == true &&							// NOTE: this operates differently for player-units and non-player units;
		_unit->getFaction() != FACTION_PLAYER								// <- no Reserve tolerance.
		&& _parent->checkReservedTu(_unit, tuCost) == false)				// Only player's units will *bypass* abortPath() due to panicking ....
	{																		// Tbh, other code should have rendered the playerPanicHandled() redundant.
		//Log(LOG_INFO) << ". . checkReservedTu(_unit, tuCost) == false";	// That is to say this should kick in *only* when player has actively
		_unit->setCacheInvalid();											// clicked to move but tries to go further than TUs allow; because
		_parent->getMap()->cacheUnitSprite(_unit);							// either the AI or the panic-code should not try to
		_pf->abortPath();													// move a unit farther than its [reserved] TUs would allow
		return false;														// WOULD THAT RESULT IN AN ENDLESS LOOP.
	} // TODO: Conform all the ambiguous returns.

	if (dir < Pathfinding::DIR_UP)
	{
		if (_action.strafe == false && dir != _unit->getUnitDirection())	// unit is looking in the wrong way so turn first - unless strafe.
		{																	// Do not use TurnBState because turning during walking doesn't cost TU.
			//Log(LOG_INFO) << ". . dir != _unit->getUnitDirection() -> turn";
			_unit->setDirectionTo(dir);

			_unit->setCacheInvalid();
			_parent->getMap()->cacheUnitSprite(_unit);
			return false;
		}

		bool wait (false);
		int soundId;
		switch (_te->unitOpensDoor(_unit, false, dir)) // open doors if any
		{
			case DR_WOOD_OPEN:
				soundId = static_cast<int>(ResourcePack::DOOR_OPEN);
				_door = true;
				break;

			case DR_UFO_OPEN:
				soundId = static_cast<int>(ResourcePack::SLIDING_DOOR_OPEN);
				_door = true;
				wait = true;
				break;

			case DR_UFO_WAIT:
				wait = true; // no break;

			default:
				soundId = -1;
		}
		if (soundId != -1)
			_parent->getResourcePack()->getSound("BATTLE.CAT", static_cast<unsigned>(soundId))
										->play(-1, _parent->getMap()->getSoundAngle(posStart));

		if (wait == true) return false; // wait for the ufo door to open
	}

	// TODO: Put checkForSilacoid() around here!

	if (_parent->checkProxyGrenades(_unit) == true) // proxy blows up in face after door opens - copied statusStand_end()
	{
		abortState();
		return false;
	}

	if (_fall == false)
	{
		//Log(LOG_INFO) << ". check size for obstacles";
		const int unitSize (_unit->getArmor()->getSize() - 1);
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
				//Log(LOG_INFO) << ". . check obstacle(unit)";
				const BattleUnit
					* const blockUnit (_battleSave->getTile(posStop + Position(x,y,0))->getTileUnit()),
					* blockBelowUnit;

				const Tile* const tileBelowDest (_battleSave->getTile(posStop + Position(x,y,-1)));
				if (tileBelowDest != nullptr)
					blockBelowUnit = tileBelowDest->getTileUnit();
				else
					blockBelowUnit = nullptr;

				if (((blockUnit != nullptr && blockUnit != _unit) // can't walk into units in this tile or on top of other units sticking their head into this tile
					|| (blockBelowUnit != nullptr && blockBelowUnit != _unit
						&& blockBelowUnit->getHeight(true) - tileBelowDest->getTerrainLevel()
								> Pathfinding::UNIT_HEIGHT))) // cf. Pathfinding::getTuCostPf()
				{
					//Log(LOG_INFO) << ". . . obstacle(unit) -> abortPath()";
//					_action.TU = 0;
					abortState();
					return false;
				}
			}
		}

		//Log(LOG_INFO) << ". walkB:spend TU & Energy";
		_unit->expendTuEnergy(tuCost, enCost);
	}

	dir = _pf->dequeuePath();
	//Log(LOG_INFO) << ". dequeuePath() dir[0] = " << dir;
	if (_fall == true)
	{
		//Log(LOG_INFO) << ". . falling, _pf->DIR_DOWN";
		dir = Pathfinding::DIR_DOWN;
	}
	//Log(LOG_INFO) << ". dequeuePath() dir[1] = " << dir;

	setWalkSpeed(gravLift);

	//Log(LOG_INFO) << ". walkB:startWalking()";
	_unit->startWalking(
					dir, posStop,
					_battleSave->getTile(posStart + Position(0,0,-1)));

	if (_isVisible == true)
	{
		if (_unit->getFaction() != FACTION_PLAYER
			&& _walkCamera->isOnScreen(_unit->getPosition()) == false)
		{
			//if (_debug)
			//Log(LOG_INFO) << "walkB:statusStand() centerPos id-" << _unit->getId();
			_walkCamera->centerPosition(posStart);
		}
		else
		{
			//Log(LOG_INFO) << "walkB:statusStand() set viewLevel id-" << _unit->getId();
			switch (dir)
			{
				case Pathfinding::DIR_DOWN:
					_walkCamera->setViewLevel(posStart.z - 1);
					break;
				default:
					_walkCamera->setViewLevel(posStart.z);
			}
		}
	}

	_preStepTurn =
	_playFly = false;

	//Log(LOG_INFO) << ". EXIT (dir!=-1) id-" << _unit->getId();
	return true;
}

/**
 * Continues unit-walk.
 * @note Called from think().
 * @return, true to continue moving, false to exit think()
 */
bool UnitWalkBState::statusWalk() // private.
{
//	if (_debug)
//	{
//		Log(LOG_INFO) << "***** UnitWalkBState::statusWalk()\t\tid-" << _unit->getId()
//					  << " " << _unit->getPosition()
//					  << " vis= " << _unit->getUnitVisible();
//	}

	if (_battleSave->getTile(_unit->getStopPosition())->getTileUnit() == nullptr	// next tile must be not occupied
		// And, if not flying, the position directly below the tile must not be
		// occupied ... had that happen with a sectoid left standing in the air
		// because a cyberdisc was 2 levels below it.
		// btw, these have probably been already checked...
		|| _battleSave->getTile(_unit->getStopPosition())->getTileUnit() == _unit)	// why aren't all quadrants checked.
	{																				// Or was this all checked in Pathfinding already.
		//Log(LOG_INFO) << ". . walkB: keepWalking()";
		if (_isVisible == true) playMoveSound();
		_unit->keepWalking( // advances _walkPhase
						_battleSave->getTile(_unit->getPosition() + Position(0,0,-1)),
						_isVisible == true);
		//Log(LOG_INFO) << ". . walkB: establishTilesLink()";
		if (_tilesLinked == false) establishTilesLink();
	}
	else if (_fall == false) // walked into an unseen unit
	{
		//Log(LOG_INFO) << ". . walkB: !falling Abort path; another unit is blocking path";
		clearTilesLink(false);
		_unit->setDirectionTo( // turn to blocking unit. TODO: This likely needs sprite-caching ....
						_unit->getStopPosition(),
						_unit->getTurretType() != TRT_NONE);

		_pf->abortPath();
		_unit->setUnitStatus(STATUS_STANDING);
	}

	//Log(LOG_INFO) << ". pos " << _unit->getPosition();
	// unit moved from one tile to the other, update the tiles & investigate new flooring
	if (_tileSwitchDone == false
		&& _unit->getPosition() != _unit->getStartPosition())
	{
		//Log(LOG_INFO) << ". . tile switch from _posStart to _posStop";
		// IMPORTANT: startTile transiently holds onto this unit (all quads) for Map drawing.
		_tileSwitchDone = true;

		_unit->setUnitTile(
						_battleSave->getTile(_unit->getPosition()),
						_battleSave->getTile(_unit->getPosition() + Position(0,0,-1)));

		Tile* tile;
		const Tile* tileBelow;

		bool doFallCheck (true);
		const int unitSize (_unit->getArmor()->getSize() - 1);
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
				//Log(LOG_INFO) << ". . . set unit on stop tile " << _unit->getPosition();
				tile = _battleSave->getTile(_unit->getPosition() + Position(x,y,0));
				tile->setTileUnit(_unit);

				tileBelow = _battleSave->getTile(_unit->getPosition() + Position(x,y,-1));
				if (tile->solidFloor(tileBelow) == true) // NOTE: I have a suspicion this should be checked for the primary quadrant only.
				{
					//Log(LOG_INFO) << ". . . . hasFloor ( doFallCheck set FALSE )";
					doFallCheck = false;
				}
				//else Log(LOG_INFO) << ". . . . no Floor ( doFallCheck TRUE )";
			}
		}

		_fall = doFallCheck == true
			 && _pf->getMoveTypePf() != MT_FLY
			 && _unit->getPosition().z != 0;

		if (_fall == true)
		{
			//Log(LOG_INFO) << ". . . falling";
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
					tileBelow = _battleSave->getTile(_unit->getPosition() + Position(x,y,-1));
					//if (tileBelow) Log(LOG_INFO) << ". . . . otherTileBelow exists";
					if (tileBelow != nullptr && tileBelow->getTileUnit() != nullptr)
					{
						//Log(LOG_INFO) << ". . . . . another unit already occupies lower tile";
						clearTilesLink(true);

						_fall = false;

						_pf->dequeuePath();
						_battleSave->addFallingUnit(_unit);

						//Log(LOG_INFO) << "UnitWalkBState::think() addFallingUnit() id-" << _unit->getId();
						_parent->stateBPushFront(new UnitFallBState(_parent));
						return false;
					}
					//else Log(LOG_INFO) << ". . . . . otherTileBelow Does NOT contain other unit";
				}
			}
		}
	}
	return true;
}

/**
 * Ends unit-walk.
 * @note Called from think().
 * @return, true to continue moving, false to exit think()
 */
bool UnitWalkBState::statusStand_end() // private.
{
//	if (_debug)
//	{
//		Log(LOG_INFO) << "***** UnitWalkBState::statusStand_end()\tid-" << _unit->getId()
//					  << " " << _unit->getPosition()
//					  << " vis= " << _unit->getUnitVisible();
//	}

	if (_unit->getFaction() == FACTION_PLAYER
		|| _battleSave->getDebugTac() == true)
	{
		const BattlescapeState* const battleState (_battleSave->getBattleState());

		double stat (static_cast<double>(_unit->getBattleStats()->tu));
		const int tu (_unit->getTu());
		battleState->getTuField()->setValue(static_cast<unsigned>(tu));
		battleState->getTuBar()->setValue(std::ceil(
											static_cast<double>(tu) / stat * 100.));

		stat = static_cast<double>(_unit->getBattleStats()->stamina);
		const int energy (_unit->getEnergy());
		battleState->getEnergyField()->setValue(static_cast<unsigned>(energy));
		battleState->getEnergyBar()->setValue(std::ceil(
											static_cast<double>(energy) / stat * 100.));
	}

	if (_unit->getUnitFire() != 0) // TODO: Also add to falling and/or all quadrants of large units.
		_unit->getUnitTile()->addSmoke(1); //(_unit->getUnitFire() + 1) >> 1u);


	if (_fall == false
		&& _unit->getSpecialAbility() == SPECAB_BURN) // if the unit burns floortiles, burn floortiles
	{
		// Put burnedBySilacoid() here! etc
		_unit->burnTile(_unit->getUnitTile());

		if (_unit->getUnitStatus() != STATUS_STANDING)	// ie: burned a hole in the floor and fell through it
		{												// Trace TileEngine::hit() through applyGravity() etc. to determine unit-status.
//			_action.TU = 0;

			_pf->abortPath();
//			_unit->setCacheInvalid();
//			_parent->getMap()->cacheUnitSprite(_unit);
//			_parent->popState();
			return false; // NOTE: This should probably set unit non-vis and be allowed to do a recalc Fov by other units.
		}
	}


	_te->calculateUnitLighting();

	const Position pos (_unit->getPosition());

	if (_door == true)
		_te->calcFovTiles_pos(pos);
	else if (_unit->getFaction() == FACTION_PLAYER)
		_te->calcFovTiles(_unit);

	// This needs to be done *before* calcFovUnits_pos() below_ or else any
	// units spotted would be flagged-visible before a call to visForUnits()
	// has had a chance to catch a newly spotted unit (that was not-visible).
	const bool spot (visForUnits());

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

	if (_unit->getFaction() != FACTION_PLAYER)	// set aLien/Civie non-visible
		_unit->setUnitVisible(false);			// then re-calc

	UnitFaction faction;
	switch (_unit->getFaction())
	{
		default:
		case FACTION_PLAYER:
		case FACTION_NEUTRAL: faction = FACTION_HOSTILE; break;
		case FACTION_HOSTILE: faction = FACTION_PLAYER;
	}
	_te->calcFovUnits_pos(pos, false, faction);

	switch (_unit->getFaction())
	{
		case FACTION_PLAYER:
			if (_walkCamera->isOnScreen(_unit->getPosition()))
				_walkCamera->setViewLevel(_unit->getPosition().z);
			break;

		case FACTION_HOSTILE:
		case FACTION_NEUTRAL:
			if (_unit->getUnitVisible() == true)
			{
				if (_isVisible == false)
					_walkCamera->centerPosition(_unit->getPosition(), false);
				else
					_walkCamera->focusPosition(
											_unit->getPosition(),
											false, false);
			}
	}

	if (_parent->checkProxyGrenades(_unit) == true) // Put checkForSilacoid() here!
	{
		abortState();
		return false;
	}

	if (spot == true)
	{
		//if (_unit->getFaction() == FACTION_PLAYER) Log(LOG_INFO) << ". . _newVis TRUE, Abort path";
		//else if (_unit->getFaction() != FACTION_PLAYER) Log(LOG_INFO) << ". . _newUnitSpotted TRUE, Abort path";
		abortState();
		return false;
	}

	if (_fall == false) // check for reaction fire
	{
		//Log(LOG_INFO) << ". . walkB:NOT falling, checkReactionFire()";
		if (_te->checkReactionFire(_unit) == true) // unit got fired upon - stop walking
		{
			//Log(LOG_INFO) << ". . . RF triggered - cacheUnitSprite/pop state";
			_battleSave->rfTriggerOffset(_walkCamera->getMapOffset());

			abortState();
			return false;
		}
		//else Log(LOG_INFO) << ". . walkB:checkReactionFire() FALSE - no caching";
	}

	_door = false;
	return true;
}

/**
 * Swivels unit.
 * @note Called from think().
 */
void UnitWalkBState::statusTurn() // private.
{
//	if (_debug)
//	{
//		Log(LOG_INFO) << "***** UnitWalkBState::statusTurn()\t\tid-" << _unit->getId()
//					  << " " << _unit->getPosition()
//					  << " vis= " << _unit->getUnitVisible();
//	}

	if (_preStepTurn == true) // turning during walking costs no tu unless aborted.
		++_preStepCost;

	_unit->turn();

//	_unit->setCacheInvalid();
	_parent->getMap()->cacheUnitSprite(_unit);

	if (_unit->getFaction() == FACTION_PLAYER)
		_te->calcFovTiles(_unit);

	// calcFov() is unreliable for setting the _newUnitSpotted bool as it
	// can be called from various other places in the code, ie: doors opening
	// (& explosions/terrain destruction) and that messes up the result.
	// But let's do it anyway! ps. Fixed
	if (visForUnits() == true)
	{
		if (_preStepTurn == true)
		{
			_unit->expendTu(_preStepCost);

			_preStepCost = 0;
			_preStepTurn = false;
		}

		//if (_unit->getFaction() == FACTION_PLAYER) Log(LOG_INFO) << ". . _newVis = TRUE, Abort path, popState";
		//else if (_unit->getFaction() != FACTION_PLAYER) Log(LOG_INFO) << ". . _newUnitSpotted = TRUE, Abort path, popState";

		if (_unit->getFaction() != FACTION_PLAYER)
			_unit->setHiding(false);

		_unit->setUnitStatus(STATUS_STANDING);
		abortState(false);
	}
	else if (_unit->getFaction() == FACTION_PLAYER)
	{
		_parent->getBattlescapeState()->clearHostileIcons();

		if (_parent->playerPanicHandled() == true)
			_parent->getBattlescapeState()->updateHostileIcons();
	}
}

/**
 * Resets the unit-cache, aborts the path and this BattleState.
 * @param recache - true to re-cache the unit-sprite (default true)
 */
void UnitWalkBState::abortState(bool recache) // private.
{
	if (recache == true)
	{
		_unit->setCacheInvalid();
		_parent->getMap()->cacheUnitSprite(_unit);
	}
	_pf->abortPath();
	_parent->popBattleState();
}

/**
 * Handles some calculations when the path is finished.
 * @note Used mostly to finish an AI BA_MOVE but also to set player-unit's TU
 * to zero after panic. Also updates lighting, FoV, sprites, and pops this
 * BattleState.
 */
void UnitWalkBState::postPathProcedures() // private.
{
	//Log(LOG_INFO) << "";
	//Log(LOG_INFO) << "UnitWalkBState::postPathProcedures() id-" << _unit->getId();
	_action.TU = 0;

	if (_unit->getFaction() == FACTION_HOSTILE)
	{
		if (_action.finalAction == true) // set by AlienBAI Ambush/Escape.
			_unit->setReselect(false);

		int dir;

		const BattleUnit* const chargeUnit (_unit->getChargeTarget());
		if (chargeUnit != nullptr)
		{
			//Log(LOG_INFO) << ". . charging vs id-" << chargeUnit->getId();
			const Position posTarget (chargeUnit->getPosition());
			dir = TileEngine::getDirectionTo(_unit->getPosition(), posTarget);
			//Log(LOG_INFO) << ". . pos" << _unit->getPosition() << " target" << posTarget << " dir= " << dir;
			// kL_notes (pre-above):
			// put an appropriate facing direction here
			// don't stare at a wall. Get if aggro, face closest xCom op <- might be done somewhere already.
			// Cheat: face closest xCom op based on a percentage (perhaps alien 'value' or rank)
			// cf. void AggroBAIState::setAggroTarget(BattleUnit* unit)
			// and bool TileEngine::calcFov(BattleUnit* unit)

			if (_te->validMeleeRange(_unit, dir, chargeUnit) == true)
			{
				//Log(LOG_INFO) << ". . . range VALID";
				_unit->setChargeTarget();

				BattleAction action;
				action.weapon = _unit->getMeleeWeapon(); // will get Melee OR Fist
				if (action.weapon != nullptr) // also checked in getActionTu() & ProjectileFlyBState::init()
				{
					action.actor		= _unit;
					action.posTarget	= posTarget;
					action.targeting	= true;
					action.type			= BA_MELEE;
					action.TU			= _unit->getActionTu(action.type, action.weapon);

					_parent->stateBPushBack(new ProjectileFlyBState(_parent, action));
					//Log(LOG_INFO) << ". . . . FlyB melee vs " << posTarget;
				}
			}
		}
		else if (_unit->isHiding() == true) // set by AI_ESCAPE Mode.
		{
			//Log(LOG_INFO) << ". . hiding";
			_unit->setHiding(false);
			_unit->setReselect(false);
			dir = RNG::generate(0,7);
//			dir = (_unit->getUnitDirection() + 4) % 8u; // turn 180 deg.
		}
		else if ((dir = _action.finalFacing) == -1) // set by AlienBAIState::setupAmbush() & findFirePosition()
		{
			dir = getFinalDirection();
			//Log(LOG_INFO) << ". . final dir= " << dir;
		}

		if (dir != -1)
		{
			_unit->setDirectionTo(dir);
			while (_unit->getUnitStatus() == STATUS_TURNING)
			{
				_unit->turn();
				_te->calcFovUnits(_unit); // NOTE: Might need newVis/newUnitSpotted -> abort.
			}
		}
	}

	if (_door == true) // in case a door opened AND state was aborted.
	{
		_te->calculateUnitLighting();
		_te->calcFovUnits_pos(_unit->getPosition(), true);
	}

	_unit->setCacheInvalid();
	_parent->getMap()->cacheUnitSprite(_unit);

	if (_fall == false)
		_parent->popBattleState();
}

/**
 * Gets a suitable final facing direction for aLiens.
 * @note This does not require any TU cost.
 * @return, direction to face
 */
int UnitWalkBState::getFinalDirection() const // private.
{
	const BattleUnit* unitFaced (nullptr);
	int
		dist (100000),
		distTest;

	const Position pos (_unit->getPosition());

	std::vector<BattleUnit*> hostileList (_unit->getHostileUnits()); // firstly check currently spotted hostiles
	for (std::vector<BattleUnit*>::const_iterator
			i = hostileList.begin();
			i != hostileList.end();
			++i)
	{
		if ((distTest = TileEngine::distSqr(pos, (*i)->getPosition())) < dist)
		{
			dist = distTest;
			unitFaced = *i;
		}
	}

	if (unitFaced == nullptr)
	{
		dist = 100000;
		hostileList = _unit->getHostileUnitsThisTurn(); // secondly check recently spotted hostiles
		for (std::vector<BattleUnit*>::const_iterator
				i = hostileList.begin();
				i != hostileList.end();
				++i)
		{
			if ((distTest = TileEngine::distSqr(pos, (*i)->getPosition())) < dist)
			{
				dist = distTest;
				unitFaced = *i;
			}
		}
	}

	if (unitFaced == nullptr)
	{
		const int diff (static_cast<int>(_parent->getBattlescapeState()->getSavedGame()->getDifficulty()));
		if (RNG::percent((diff + 1) * 20 - _unit->getRankInt() * 5) == true)
		{
			dist = 100000;
			for (std::vector<BattleUnit*>::const_iterator // thirdly check for exposed player-units
					i = _battleSave->getUnits()->begin();
					i != _battleSave->getUnits()->end();
					++i)
			{
				if ((*i)->getFaction() == FACTION_PLAYER
					&& (*i)->isOut_t(OUT_STAT) == false
					&& (*i)->getExposed() != -1
					&& (*i)->getExposed() <= _unit->getIntelligence())
				{
					if ((distTest = TileEngine::distSqr(pos, (*i)->getPosition())) < dist)
					{
						dist = distTest;
						unitFaced = *i;
					}
				}
			}
		}
	}

	if (unitFaced != nullptr)
		return TileEngine::getDirectionTo(pos, unitFaced->getPosition());

	return -1;
}

/**
 * Checks visibility against new opponents.
 * @return, true if a new opponent is spotted
 */
bool UnitWalkBState::visForUnits() const // private.
{
	if (_fall == true) return false;

	bool spot;
	switch (_unit->getFaction())
	{
		case FACTION_PLAYER:
			spot = _parent->playerPanicHandled() == true // short-circuit of calcFovUnits() is intentional.
				&& _te->calcFovUnits(_unit);
			break;

		case FACTION_HOSTILE:
			spot = _te->calcFovUnits(_unit)
				&& _action.desperate == false
				&& _unit->getChargeTarget() == nullptr;
			break;

		case FACTION_NEUTRAL:
		default:
			spot = false;
			break;
	}
	return spot;
}

/**
 * Sets the animation speed of units.
 * @param gravLift - true if moving up/down a gravLift
 */
void UnitWalkBState::setWalkSpeed(bool gravLift) const // private.
{
	Uint32 interval;
	switch (_unit->getFaction())
	{
		case FACTION_PLAYER:
			if (_action.dash == true
				|| (_unit->getUnitRules() != nullptr
					&& _unit->getUnitRules()->isDog() == true))
			{
				interval = _parent->getBattlescapeState()->STATE_INTERVAL_XCOMDASH;
			}
			else
				interval = _parent->getBattlescapeState()->STATE_INTERVAL_XCOM;
			break;

		case FACTION_HOSTILE:
		case FACTION_NEUTRAL:
		default:
			interval = _parent->getBattlescapeState()->STATE_INTERVAL_ALIEN;
	}

	if (gravLift == true)
		interval <<= 1u;

	//Log(LOG_INFO) << "unitWalkB: setWalkSpeed() set interval = " << interval;
	_parent->setStateInterval(interval);
}

/**
 * Handles walking/ flying/ other movement sounds.
 */
void UnitWalkBState::playMoveSound() // private.
{
	const int walkPhase (_unit->getWalkPhase());
	int soundId (-1);

	if (_unit->getMoveSound() == -1)
	{
		switch (_unit->getUnitStatus())
		{
			case STATUS_WALKING:
				_playFly = true;
				switch (walkPhase)
				{
					case 3:
					case 7:
					{
						const Tile
							* const tile (_unit->getUnitTile()),
							* const tileBelow (_battleSave->getTile(tile->getPosition() + Position(0,0,-1)));
						const int stepSound (tile->getFootstepSound(tileBelow));
						if (stepSound != 0)
						{
							switch (walkPhase)
							{
								case 3:
									soundId = (stepSound << 1u) + static_cast<int>(ResourcePack::WALK_OFFSET) + 1;
									break;
								case 7:
									soundId = (stepSound << 1u) + static_cast<int>(ResourcePack::WALK_OFFSET);
							}
						}
					}
				}
				break;

			case STATUS_FLYING:
				if (walkPhase == 0 || _playFly == true)
				{
					_playFly = false;
					if (_fall == false)
					{
						if (_unit->isFloating() == false) // GravLift note: isFloating() might be redundant w/ (_fall=false). See above^
							soundId = static_cast<int>(ResourcePack::GRAVLIFT_SOUND);
						else
						{
							if (_unit->getUnitRules() != nullptr
								&& _unit->getUnitRules()->isMechanical() == true)
							{
								soundId = static_cast<int>(ResourcePack::FLYING_SOUND);		// hoverSound flutter
							}
							else
								soundId = static_cast<int>(ResourcePack::FLYING_SOUND_HQ);	// HQ hoverSound
						}
					}
				}
				else if (walkPhase == 7
					&& (_fall == true
						|| (_unit->isFloating() == true && _pf->getMoveTypePf() != MT_FLY))
					&& groundCheck() == true)
				{
					soundId = static_cast<int>(ResourcePack::ITEM_DROP); // *thunk*
				}
		}
	}
	else if (_unit->getWalkPhaseTrue() == 0) // hover-tanks play too much on diagonals ...
	{
		if (_unit->getUnitStatus() == STATUS_FLYING
			&& _unit->isFloating() == false
			&& _fall == false)
		{
			soundId = static_cast<int>(ResourcePack::GRAVLIFT_SOUND); // GravLift note: isFloating() might be redundant w/ (_fall=false). See above^
		}
		else
			soundId = _unit->getMoveSound();
	}

	//Log(LOG_INFO) << ". phase= " << walkPhase << " id= " << soundId;
	if (soundId != -1)
		_parent->getResourcePack()->getSound("BATTLE.CAT", static_cast<unsigned>(soundId))
									->play(-1, _parent->getMap()->getSoundAngle(_unit->getPosition()));
}

/**
 * Determines if a flying unit turns off flight at the start of movement.
 * @note '_fall' should always be false when this is called in init(). And unit
 * must be capable of flight for this to be relevant. This could get problematic
 * if/when falling onto nonFloors like water and/or if there is another unit on
 * tileBelow.
 */
void UnitWalkBState::doFallCheck() // private.
{
	if (_pf->getMoveTypePf() != MT_FLY
		&& _unit->getPosition().z != 0
		&& groundCheck() == false)
	{
		_fall = true;
	}
}

/**
 * Checks if there is ground below when unit is falling.
 * @note Pathfinding already has a function canFallDown() that could be used.
 * @return, true if unit is on solid floor
 */
bool UnitWalkBState::groundCheck() const // private.
{
	const Tile* tileBelow;
	Position pos;

	const int unitSize (_unit->getArmor()->getSize() - 1);
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
			if (_battleSave->getTile(pos)->solidFloor(tileBelow) == true)
				return true;
		}
	}
	return false;
}

/**
 * Establishes unit's transient link(s) to its destination Tile(s).
 */
void UnitWalkBState::establishTilesLink() // private.
{
	//Log(LOG_INFO) << "UnitWalkBState::establishTilesLink()";
	_tilesLinked = true;

	const int unitSize (_unit->getArmor()->getSize() - 1);
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
			_battleSave->getTile(_unit->getStopPosition() + Position(x,y,0))->setTileUnit(_unit);
		}
	}
}

/**
 * Clears unit's transient link(s) to other Tile(s).
 * @param origin - true if start-tile(s) should be cleared, false for stop-tile(s)
 */
void UnitWalkBState::clearTilesLink(bool origin) // private.
{
	//Log(LOG_INFO) << "UnitWalkBState::clearTilesLink()";
	_tilesLinked = false;

	Position pos;

	const int unitSize (_unit->getArmor()->getSize() - 1);
	switch (unitSize)
	{
		case 0:
			if (origin == true)
				pos = _unit->getStartPosition();
			else
				pos = _unit->getStopPosition();

			_battleSave->getTile(pos)->setTileUnit(); // clear Tile's unit.
			break;

		case 1:
			pos = _unit->getPosition();
			std::vector<Position> posCurrent;	// gather unit's current Positions
			for (int							// so they're not cleared.
					x = unitSize;
					x != -1;
					--x)
			{
				for (int
						y = unitSize;
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

			Position posTest;
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
					posTest = pos + Position(x,y,0);
					if (std::find(
								posCurrent.begin(),
								posCurrent.end(),
								posTest) == posCurrent.end())
					{
						_battleSave->getTile(posTest)->setTileUnit(); // clear Tiles' unit.
					}
				}
			}
	}
}

}
