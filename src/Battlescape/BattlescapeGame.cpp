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

#include "BattlescapeGame.h"

//#define _USE_MATH_DEFINES
//#include <cmath>
#include <sstream>

#include "AlienBAIState.h"
#include "BattlescapeState.h"
#include "BattleState.h"
#include "Camera.h"
#include "CivilianBAIState.h"
#include "ExplosionBState.h"
#include "Explosion.h"
#include "InfoboxOKState.h"
#include "InfoboxState.h"
#include "Inventory.h"
#include "Map.h"
#include "NextTurnState.h"
#include "Pathfinding.h"
#include "ProjectileFlyBState.h"
#include "TileEngine.h"
#include "UnitDieBState.h"
#include "UnitFallBState.h"
#include "UnitInfoState.h"
#include "UnitPanicBState.h"
#include "UnitTurnBState.h"
#include "UnitWalkBState.h"

#include "../Engine/Game.h"
#include "../Engine/Language.h"
//#include "../Engine/Logger.h"
#include "../Engine/Options.h"
#include "../Engine/RNG.h"
#include "../Engine/Sound.h"

#include "../Interface/Cursor.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/AlienDeployment.h"
#include "../Ruleset/RuleArmor.h"
#include "../Ruleset/RuleInventory.h"
#include "../Ruleset/RuleItem.h"
#include "../Ruleset/Ruleset.h"

#include "../Savegame/BattleItem.h"
//#include "../Savegame/BattleUnit.h"
#include "../Savegame/BattleUnitStatistics.h"
#include "../Savegame/SavedBattleGame.h"
//#include "../Savegame/SavedGame.h"
#include "../Savegame/Tile.h"


namespace OpenXcom
{

bool BattlescapeGame::_debugPlay = false;


/**
 * Initializes all the elements in the Battlescape screen.
 * @param battleSave	- pointer to the SavedBattleGame
 * @param parentState	- pointer to the parent BattlescapeState
 */
BattlescapeGame::BattlescapeGame(
		SavedBattleGame* const battleSave,
		BattlescapeState* const parentState)
	:
		_battleSave(battleSave),
		_parentState(parentState),
		_playerPanicHandled(true),
		_AIActionCounter(0),
		_AISecondMove(false),
		_playedAggroSound(false),
		_endTurnRequested(false),
		_firstInit(true),
		_executeProgress(false),
		_shotgunProgress(false)
//		_endTurnProcessed(false)
{
	//Log(LOG_INFO) << "Create BattlescapeGame";
	_debugPlay = false;

	cancelCurrentAction();
	checkForCasualties(
					nullptr,
					nullptr,
					true);

//	_currentAction.actor = nullptr;
//	_currentAction.type = BA_NONE;
//	_currentAction.targeting = false;
	_currentAction.clearAction();

	_universalFist = new BattleItem(
								getRuleset()->getItem("STR_FIST"),
								battleSave->getNextItemId());
	_alienPsi = new BattleItem(
							getRuleset()->getItem("ALIEN_PSI_WEAPON"),
							battleSave->getNextItemId());

	for (std::vector<BattleUnit*>::const_iterator
			i = _battleSave->getUnits()->begin();
			i != _battleSave->getUnits()->end();
			++i)
	{
		(*i)->setBattleForUnit(this);
	}
	// sequence of instantiations:
	// - SavedBattleGame
	// - BattlescapeGenerator
	// - BattlescapeState
	// - this.
}

/**
 * Delete BattlescapeGame.
 */
BattlescapeGame::~BattlescapeGame()
{
	for (std::list<BattleState*>::const_iterator
			i = _battleStates.begin();
			i != _battleStates.end();
			++i)
	{
		delete *i;
	}

	delete _universalFist;
	delete _alienPsi;
}

/**
 * Initializes the Battlescape game.
 */
void BattlescapeGame::init()
{
	//Log(LOG_INFO) << "bg: init()";
	if (_firstInit == true)
	{
		_firstInit = false;
		_battleSave->getTileEngine()->recalculateFOV();
	}
}

/**
 * Checks for units panicking or falling and so on.
 */
void BattlescapeGame::think()
{
	//Log(LOG_INFO) << "BattlescapeGame::think()";
	if (_battleStates.empty() == true) // nothing is happening - see if they need some alien AI or units panicking or what have you
	{
		//Log(LOG_INFO) << "BattlescapeGame::think() - _battleStates is Empty. Clear rfShotList";
		_battleSave->getTileEngine()->getReactionPositions()->clear(); // TODO: move that to end of popState()

		if (_battleSave->getSide() != FACTION_PLAYER) // it's a non-player turn (ALIENS or CIVILIANS)
		{
			if (_debugPlay == false)
			{
				if (_battleSave->getSelectedUnit() != nullptr)
				{
					if (handlePanickingUnit(_battleSave->getSelectedUnit()) == false)
					{
						//Log(LOG_INFO) << "BattlescapeGame::think() call handleUnitAI() " << _battleSave->getSelectedUnit()->getId();
						handleUnitAI(_battleSave->getSelectedUnit());
					}
				}
				else
				{
					if (_battleSave->selectNextFactionUnit(true, _AISecondMove) == nullptr)
					{
						if (_battleSave->getDebugMode() == false)
						{
							_endTurnRequested = true;
							statePushBack(nullptr); // end AI turn
						}
						else
						{
							_battleSave->selectNextFactionUnit();
							_debugPlay = true;
						}
					}
				}
			}
		}
		else // it's a player turn
		{
			if (_playerPanicHandled == false) // not all panicking units have been handled
			{
				//Log(LOG_INFO) << "bg:think() . panic Handled is FALSE";
				_playerPanicHandled = handlePanickingPlayer();
				//Log(LOG_INFO) << "bg:think() . panic Handled = " << _playerPanicHandled;

				if (_playerPanicHandled == true)
				{
					_battleSave->getTileEngine()->recalculateFOV();
					_battleSave->getBattleState()->updateSoldierInfo();
				}
			}
			else
			{
				//Log(LOG_INFO) << "bg:think() . panic Handled is TRUE";
				_parentState->updateExperienceInfo();
			}
		}

		if (_battleSave->getUnitsFalling() == true)
		{
			//Log(LOG_INFO) << "BattlescapeGame::think(), get/setUnitsFalling() ID " << _battleSave->getSelectedUnit()->getId();
			statePushFront(new UnitFallBState(this));
			_battleSave->setUnitsFalling(false);
		}
	}
	//Log(LOG_INFO) << "BattlescapeGame::think() EXIT";
}

/**
 * Gives a time slice to the front state.
 * @note The period is controlled by '_tacticalTimer' in BattlescapeState.
 */
void BattlescapeGame::handleState()
{
	if (_battleStates.empty() == false)
	{
		if (_battleStates.front() == nullptr) // possible End Turn request
		{
			_battleStates.pop_front();
			endTurn();
		}
		else
		{
			_battleStates.front()->think();
			getMap()->draw();		// old code!! Less clunky when scrolling the battlemap.
//			getMap()->invalidate();	// redraw map
		}
	}
}

/**
 * Pushes a state to the front of the queue and starts it.
 * @param battleState - pointer to BattleState
 */
void BattlescapeGame::statePushFront(BattleState* const battleState)
{
	_battleStates.push_front(battleState);
	battleState->init();
}

/**
 * Pushes a state as the next state after the current one.
 * @param battleState - pointer to BattleState
 */
void BattlescapeGame::statePushNext(BattleState* const battleState)
{
	if (_battleStates.empty() == true)
	{
		_battleStates.push_front(battleState);
		battleState->init();
	}
	else
		_battleStates.insert(
					++_battleStates.begin(),
					battleState);
}

/**
 * Pushes a state to the back.
 * @param battleState - pointer to BattleState
 */
void BattlescapeGame::statePushBack(BattleState* const battleState)
{
	if (_battleStates.empty() == true)
	{
		_battleStates.push_front(battleState);

		if (_battleStates.front() == nullptr) // possible End Turn request
		{
			_battleStates.pop_front();
			endTurn();
		}
		else
			battleState->init();
	}
	else
		_battleStates.push_back(battleState);
}

/**
 * Removes the current state.
 * @note This is a very important function. It is called by a BattleState
 * (walking, projectile is flying, explosions, etc.) at the moment that state
 * has finished the current BattleAction. Check the result of that BattleAction
 * here and do all the aftermath. The state is then popped off the list.
 */
void BattlescapeGame::popState()
{
	//Log(LOG_INFO) << "BattlescapeGame::popState() qtyStates = " << (int)_battleStates.size();
//	if (Options::traceAI)
//	{
//		Log(LOG_INFO) << "BattlescapeGame::popState() #" << _AIActionCounter << " with "
//		<< (_battleSave->getSelectedUnit()? _battleSave->getSelectedUnit()->getTimeUnits(): -9999) << " TU";
//	}

	if (getMap()->getExplosions()->empty() == true) // explosions need to run fast after popping ProjectileFlyBState etc etc.
	{
		//Log(LOG_INFO) << "bg: popState() set interval = " << BattlescapeState::STATE_INTERVAL_STANDARD;
		setStateInterval(BattlescapeState::STATE_INTERVAL_STANDARD);
	}

	if (_battleStates.empty() == false)
	{
		//Log(LOG_INFO) << ". states NOT Empty";
		const BattleAction action = _battleStates.front()->getAction();
		bool actionFailed = false;

		if ((_battleSave->getSide() == FACTION_PLAYER || _debugPlay == true)
			&& action.actor != nullptr
			&& action.actor->getFaction() == FACTION_PLAYER
			&& action.result.empty() == false // This queries the warning string message.
			&& _playerPanicHandled == true)
		{
			//Log(LOG_INFO) << ". actionFailed";
			actionFailed = true;
			_parentState->warning(action.result);

			// remove action.Cursor if error.Message (eg, not enough tu's)
			if (action.result.compare("STR_NOT_ENOUGH_TIME_UNITS") == 0
				|| action.result.compare("STR_NO_AMMUNITION_LOADED") == 0
				|| action.result.compare("STR_NO_ROUNDS_LEFT") == 0)
			{
				switch (action.type)
				{
					case BA_LAUNCH:
						_currentAction.waypoints.clear();
					case BA_THROW:
					case BA_SNAPSHOT:
					case BA_AIMEDSHOT:
					case BA_AUTOSHOT:
					case BA_PSIPANIC:
					case BA_PSICONFUSE:
					case BA_PSICOURAGE:
					case BA_PSICONTROL:
						cancelCurrentAction(true);
					break;

					case BA_USE:
						if (action.weapon->getRules()->getBattleType() == BT_MINDPROBE)
							cancelCurrentAction(true);
					break;
				}
			}
		}

		//Log(LOG_INFO) << ". move Front-state to _deleted.";
		_deleted.push_back(_battleStates.front());
		//Log(LOG_INFO) << ". states.Popfront";
		_battleStates.pop_front();


		if (action.actor != nullptr // handle the end of this unit's actions
			&& noActionsPending(action.actor) == true)
		{
			//Log(LOG_INFO) << ". noActionsPending for state actor";
			if (action.actor->getFaction() == FACTION_PLAYER)
			{
				//Log(LOG_INFO) << ". actor -> Faction_Player";
				// spend TUs of "target triggered actions" (shooting, throwing) only;
				// the other actions' TUs (healing, scanning, etc) are already take care of.
				// kL_note: But let's do this **before** checkReactionFire(), so aLiens
				// get a chance .....! Odd thing is, though, that throwing triggers
				// RF properly while shooting doesn't .... So, I'm going to move this
				// to primaryAction() anyways. see what happens and what don't
				//
				// Note: psi-attack, mind-probe uses this also, I think.
				// BUT, in primaryAction() below, mind-probe expends TU there, while
				// psi-attack merely checks for TU there, and shooting/throwing
				// didn't even Care about TU there until I put it in there, and took
				// it out here in order to get Reactions vs. shooting to work correctly
				// (throwing already did work to trigger reactions, somehow).
				if (action.targeting == true
					&& _battleSave->getSelectedUnit() != nullptr
					&& actionFailed == false)
				{
					//Log(LOG_INFO) << ". . ID " << action.actor->getId() << " currentTU = " << action.actor->getTimeUnits();
					action.actor->spendTimeUnits(action.TU);
					// kL_query: Does this happen **before** ReactionFire/getReactor()?
					// no. not for shooting, but for throwing it does; actually no it doesn't.
					//
					// wtf, now RF works fine. NO IT DOES NOT.
					//Log(LOG_INFO) << ". . ID " << action.actor->getId() << " currentTU = " << action.actor->getTimeUnits() << " spent TU = " << action.TU;
				}

				if (_battleSave->getSide() == FACTION_PLAYER)
				{
					//Log(LOG_INFO) << ". side -> Faction_Player";

					// after throwing the cursor returns to default cursor;
					// after shooting it stays in targeting mode and the player
					// can shoot again in the same mode (autoshot/snap/aimed)
					// unless he/she is out of tu's and/or ammo
					if (actionFailed == false)
					{
						const int tuActor = action.actor->getTimeUnits();

						switch (action.type)
						{
							case BA_LAUNCH:
								_currentAction.waypoints.clear();
							case BA_THROW:
								cancelCurrentAction(true);
							break;

							case BA_SNAPSHOT:
								//Log(LOG_INFO) << ". SnapShot, TU percent = " << (float)action.weapon->getRules()->getSnapTu();
								if (tuActor < action.actor->getActionTu(
																	BA_SNAPSHOT,
																	action.weapon)
									|| action.weapon->getAmmoItem() == nullptr)
//									|| (action.weapon->selfPowered() == false
//										&& (action.weapon->getAmmoItem() == nullptr
//											|| action.weapon->getAmmoItem()->getAmmoQuantity() == 0)))
								{
									cancelCurrentAction(true);
								}
							break;

							case BA_AUTOSHOT:
								//Log(LOG_INFO) << ". AutoShot, TU percent = " << (float)action.weapon->getRules()->getAutoTu();
								if (tuActor < action.actor->getActionTu(
																	BA_AUTOSHOT,
																	action.weapon)
									|| action.weapon->getAmmoItem() == nullptr)
//									|| (action.weapon->selfPowered() == false
//										&& (action.weapon->getAmmoItem() == nullptr
//											|| action.weapon->getAmmoItem()->getAmmoQuantity() == 0)))
								{
									cancelCurrentAction(true);
								}
							break;

							case BA_AIMEDSHOT:
								//Log(LOG_INFO) << ". AimedShot, TU percent = " << (float)action.weapon->getRules()->getAimedTu();
								if (tuActor < action.actor->getActionTu(
																	BA_AIMEDSHOT,
																	action.weapon)
									|| action.weapon->getAmmoItem() == nullptr)
//									|| (action.weapon->selfPowered() == false
//										&& (action.weapon->getAmmoItem() == nullptr
//											|| action.weapon->getAmmoItem()->getAmmoQuantity() == 0)))
								{
									cancelCurrentAction(true);
								}
							break;

/*							case BA_USE: // uh USE->UnitInfoState is not a true battle state.
								if (action.weapon->getRules()->getBattleType() == BT_MINDPROBE
									&& tuActor < action.actor->getActionTu(BA_USE, action.weapon))
								{
									cancelCurrentAction(true);
								} */

/*							case BA_PSIPANIC:
								if (tuActor < action.actor->getActionTu(BA_PSIPANIC, action.weapon))
									cancelCurrentAction(true);
							break;
							case BA_PSICONTROL:
								if (tuActor < action.actor->getActionTu(BA_PSICONTROL, action.weapon))
									cancelCurrentAction(true);
							break; */
						}
					}

					// -> I moved this to the end of the function to prevent cursor showing during RF.
//					setupCursor();
//					_parentState->getGame()->getCursor()->setVisible(); // might not be needed here anymore. But safety.
//					_parentState->getGame()->getCursor()->setHidden(false);
					//Log(LOG_INFO) << ". end NOT actionFailed";
				}
			}
			else // action.actor is not FACTION_PLAYER
			{
				//Log(LOG_INFO) << ". action -> NOT Faction_Player";
				action.actor->spendTimeUnits(action.TU);

				if (_battleSave->getSide() != FACTION_PLAYER
					&& _debugPlay == false)
				{
					BattleUnit* selUnit = _battleSave->getSelectedUnit();
					 // AI does three things per unit, before switching to the next, or it got killed before doing the second thing
					if (_AIActionCounter > 2
						|| selUnit == nullptr
						|| selUnit->isOut_t() == true)
					{
						_AIActionCounter = 0;

						if (selUnit != nullptr)
						{
							selUnit->clearCache();
							getMap()->cacheUnit(selUnit);
						}

						if (_battleStates.empty() == true
							&& _battleSave->selectNextFactionUnit(true) == nullptr)
						{
							if (_battleSave->getDebugMode() == false)
							{
								_endTurnRequested = true;
								statePushBack(nullptr); // end AI turn
							}
							else
							{
								_battleSave->selectNextFactionUnit();
								_debugPlay = true;
							}
						}

						selUnit = _battleSave->getSelectedUnit();
						if (selUnit != nullptr)
							getMap()->getCamera()->centerOnPosition(selUnit->getPosition());
					}
				}
				else if (_debugPlay == true)
				{
					setupCursor();
					_parentState->getGame()->getCursor()->setHidden(false);	// don't know if this be needed here.
//					_parentState->getGame()->getCursor()->setVisible();		// I seldom use debugPlay
				}
			}
		}


		//Log(LOG_INFO) << ". uh yeah";
		if (_battleStates.empty() == false)
		{
			//Log(LOG_INFO) << ". states NOT Empty [1]";
			if (_battleStates.front() == nullptr) // end turn request
			{
				//Log(LOG_INFO) << ". states.front() == nullptr";
				while (_battleStates.empty() == false)
				{
					//Log(LOG_INFO) << ". cycle through nullptr-states Front";
					if (_battleStates.front() == nullptr)
					{
						//Log(LOG_INFO) << ". pop Front";
						_battleStates.pop_front();
					}
					else
						break;
				}

				if (_battleStates.empty() == true)
				{
					//Log(LOG_INFO) << ". states Empty -> endTurn()";
					endTurn();
					//Log(LOG_INFO) << ". endTurn() DONE return";
					return;
				}
				else
				{
					//Log(LOG_INFO) << ". states NOT Empty -> prep back state w/ nullptr";
					_battleStates.push_back(nullptr);
				}
			}

			//Log(LOG_INFO) << ". states.front()->init()";
			_battleStates.front()->init(); // init the next state in queue
		}

		// The selected unit died or became unconscious or disappeared inexplicably.
		if (_battleSave->getSelectedUnit() == nullptr
			|| _battleSave->getSelectedUnit()->isOut_t() == true)
//			|| _battleSave->getSelectedUnit()->isOut(true, true) == true)
		{
			//Log(LOG_INFO) << ". unit incapacitated: cancelAction & deSelect)";
			cancelCurrentAction(); // note that this *will* setupCursor() under certain circumstances - ie, if current action was targetting.

			_battleSave->setSelectedUnit(nullptr);
			//if (_battleSave->getSelectedUnit() != nullptr) Log(LOG_INFO) << "selectUnit " << _battleSave->getSelectedUnit()->getId();
			//else Log(LOG_INFO) << "NO UNIT SELECTED";

/*			if (_battleSave->getSide() == FACTION_PLAYER) //|| _debugPlay == true) // kL <- let end of function set cursor.
			{
				//Log(LOG_INFO) << ". No selUnit, enable cursor";
//				getMap()->setCursorType(CT_NORMAL);
				setupCursor();
				_parentState->getGame()->getCursor()->setHidden(false);
//				_parentState->getGame()->getCursor()->setVisible();
			} */
		}

		if (_battleSave->getSide() == FACTION_PLAYER) //|| _debugPlay == true) // kL
		{
			//Log(LOG_INFO) << ". updateSoldierInfo()";
			_parentState->updateSoldierInfo(); // calcFoV ought have been done by now ...
		}
	}

	if (_battleStates.empty() == true) // note: endTurn() above^ might develop problems w/ cursor visibility ...
	{
		if (_battleSave->getRfTriggerPosition().z != -1) // this refocuses the Camera back onto RF trigger unit after a brief delay.
		{
			SDL_Delay(336);
			//Log(LOG_INFO) << "popState - STATES EMPTY - set Camera to triggerPos & clear triggerPos";
			getMap()->getCamera()->setMapOffset(_battleSave->getRfTriggerPosition());
			_battleSave->storeRfTriggerPosition(Position(0,0,-1));
		}

		if (_battleSave->getSide() == FACTION_PLAYER) //|| _debugPlay == true))
		{
			//Log(LOG_INFO) << ". states Empty, reable cursor";
			setupCursor();
			_parentState->getGame()->getCursor()->setHidden(false);
//			_parentState->getGame()->getCursor()->setVisible(); // might not be needed here anymore. But safety.

			_parentState->refreshMousePosition(); // update tile data on the HUD.
		}
	}
	//Log(LOG_INFO) << "BattlescapeGame::popState() EXIT";
}

/**
 * Determines whether there are any actions pending for a given unit.
 * @param unit - pointer to a BattleUnit
 * @return, true if there are no actions pending
 */
bool BattlescapeGame::noActionsPending(const BattleUnit* const unit) const // private
{
	if (_battleStates.empty() == false)
	{
		for (std::list<BattleState*>::const_iterator
				i = _battleStates.begin();
				i != _battleStates.end();
				++i)
		{
			if (*i != nullptr && (*i)->getAction().actor == unit)
				return false;
		}
	}

	return true;
}

/**
 * Sets the timer interval for think() calls of the state.
 * @param interval - interval in ms
 */
void BattlescapeGame::setStateInterval(Uint32 interval)
{
	_parentState->setStateInterval(interval);
}

/**
 * Centers the battlefield camera on a BattleUnit.
 * @param unit - pointer to a BattleUnit
 * @param draw - true to redraw the battlefield (default false)
 */
void BattlescapeGame::centerOnUnit( // private.
		const BattleUnit* const unit,
		bool draw) const
{
	if (unit != _battleSave->getWalkUnit())
	{
		if (unit->getUnitVisible() == true)
			_battleSave->setWalkUnit(unit);
		else
			_battleSave->setWalkUnit(nullptr);

		getMap()->getCamera()->centerOnPosition(unit->getPosition(), draw);
	}
}

/**
 * Handles the processing of the AI states of a unit.
 * @param unit - pointer to a BattleUnit
 */
void BattlescapeGame::handleUnitAI(BattleUnit* const unit)
{
	//Log(LOG_INFO) << "";
	//Log(LOG_INFO) << "BattlescapeGame::handleUnitAI() " << unit->getId();
	centerOnUnit(unit); // if you're going to reveal the map at least show the first aLien.

	//Log(LOG_INFO) << ". TU = " << unit->getTimeUnits();
	if (unit->getTimeUnits() == 0)
	{
		//Log(LOG_INFO) << ". set NOT reselect";
		unit->dontReselect();
	}

	if (_AIActionCounter > 1 // unit-AI done.
		|| unit->reselectAllowed() == false)
	{
		//Log(LOG_INFO) << ". find next unit";
		_AIActionCounter = 0;

		//if (_battleSave->getSelectedUnit() != nullptr) Log(LOG_INFO) << "selUnit[1] id-" << _battleSave->getSelectedUnit()->getId();
		//else Log(LOG_INFO) << "selUnit[1] NULL";

		if (_battleSave->selectNextFactionUnit(true, _AISecondMove) == nullptr)
		{
			//Log(LOG_INFO) << ". . no unit found, request endTurn";
			//if (_battleSave->getSelectedUnit() != nullptr) Log(LOG_INFO) << "selUnit[2] id-" << _battleSave->getSelectedUnit()->getId();
			//else Log(LOG_INFO) << "selUnit[2] NULL";

			if (_battleSave->getDebugMode() == false)
			{
				//Log(LOG_INFO) << "BattlescapeGame::handleUnitAI() statePushBack(end AI turn)";
				_endTurnRequested = true;
				statePushBack(nullptr); // end AI turn
			}
			else
			{
				//Log(LOG_INFO) << "BattlescapeGame::handleUnitAI() debugPlay";
				_battleSave->selectNextFactionUnit();
				_debugPlay = true;
			}
		}

		//if (_battleSave->getSelectedUnit() != nullptr) Log(LOG_INFO) << "selUnit[3] id-" << _battleSave->getSelectedUnit()->getId();
		//else Log(LOG_INFO) << "selUnit[3] NULL";

		if (_battleSave->getSelectedUnit() != nullptr)
		{
			//Log(LOG_INFO) << ". . unit found id-" << _battleSave->getSelectedUnit()->getId();
			centerOnUnit(_battleSave->getSelectedUnit());
			_parentState->updateSoldierInfo(false); // This is useful for player when AI-turn ends. CalcFoV is done below_

			if (_AISecondMove == false)
			{
				if (std::find(
						_battleSave->getShuffleUnits()->begin(),
						_battleSave->getShuffleUnits()->end(),
						_battleSave->getSelectedUnit())			// next unit up
							 -
					std::find(
						_battleSave->getShuffleUnits()->begin(),
						_battleSave->getShuffleUnits()->end(),
						unit) < 1)								// previous unit
				{
					//Log(LOG_INFO) << ". . . Second move";
					_AISecondMove = true;
				}
			}
			//else Log(LOG_INFO) << ". . . First move";

			//Log(LOG_INFO) << "[1]bsg secondMove = " << (int)_AISecondMove;
		}
		//else Log(LOG_INFO) << ". . selUnit[4] NULL";

		//Log(LOG_INFO) << "BattlescapeGame::handleUnitAI() early-EXIT id-" << unit->getId();
		return;
	}

	//Log(LOG_INFO) << "CONTINUE ...";

	unit->setUnitVisible(false);

	// might need this: populate _hostileUnit for a newly-created alien
	//Log(LOG_INFO) << "BattlescapeGame::handleUnitAI(), calculateFOV() call";
	_battleSave->getTileEngine()->calculateFOV(unit->getPosition());
	// it might also help chryssalids realize they've zombified someone and need to move on;
	// it should also hide units when they've killed the guy spotting them;
	// it's also for good luck and prosperity.

//	BattleAIState* ai = unit->getAIState();
//	const BattleAIState* const ai = unit->getAIState();
	if (unit->getAIState() == nullptr)
	{
		// for some reason the unit had no AI routine assigned..
		//Log(LOG_INFO) << "BattlescapeGame::handleUnitAI() !ai, assign AI";
		if (unit->getFaction() == FACTION_HOSTILE)
			unit->setAIState(new AlienBAIState(_battleSave, unit, nullptr));
		else
			unit->setAIState(new CivilianBAIState(_battleSave, unit, nullptr));
	}
//	_battleSave->getPathfinding()->setPathingUnit(unit);	// decided to do this in AI states;
															// things might be changing the pathing
															// unit or Pathfinding relevance .....

	if (++_AIActionCounter == 1)
	{
		_playedAggroSound = false;
		unit->setHiding(false);
		//if (Options::traceAI) Log(LOG_INFO) << "#" << unit->getId() << "--" << unit->getType();
	}
	//Log(LOG_INFO) << ". _AIActionCounter DONE";

	// this cast only works when ai was already AlienBAIState at heart
//	AlienBAIState* aggro = dynamic_cast<AlienBAIState*>(ai);

	//Log(LOG_INFO) << ". Declare action - define .actor & .AIcount";
	BattleAction action;
	action.actor = unit;
	action.AIcount = _AIActionCounter;
	//Log(LOG_INFO) << ". unit->think(&action)";
	unit->think(&action);

/*	Log(LOG_INFO) << ". _unit->think() DONE Mode = " << unit->getAIState()->getAIMode();
	std::string st;
	switch (action.type)
	{
		case  0: st = "none";			break;	// BA_NONE
		case  1: st = "turn";			break;	// BA_TURN
		case  2: st = "walk";			break;	// BA_MOVE
		case  3: st = "prime";			break;	// BA_PRIME
		case  4: st = "throw";			break;	// BA_THROW
		case  5: st = "autoshot";		break;	// BA_AUTOSHOT
		case  6: st = "snapshot";		break;	// BA_SNAPSHOT
		case  7: st = "aimedshot";		break;	// BA_AIMEDSHOT
		case  8: st = "hit";			break;	// BA_MELEE
		case  9: st = "use";			break;	// BA_USE
		case 10: st = "launch";			break;	// BA_LAUNCH
		case 11: st = "mindcontrol";	break;	// BA_PSICONTROL
		case 12: st = "panic";			break;	// BA_PSIPANIC
		case 13: st = "rethink";		break;	// BA_RETHINK
		case 14: st = "defuse";			break;	// BA_DEFUSE
		case 15: st = "drop";			break;	// BA_DROP
		case 16: st = "confuse";		break;	// BA_PSICONFUSE
		case 17: st = "courage";				// BA_PSICOURAGE
	}
	Log(LOG_INFO) << ". _unit->think() DONE Action = " << st; */

	if (action.type == BA_RETHINK)
	{
//		_parentState->debug(L"Rethink");
		unit->think(&action);
	}
	//Log(LOG_INFO) << ". BA_RETHINK DONE";

	_AIActionCounter = action.AIcount;

	//Log(LOG_INFO) << ". pre hunt for weapon";
//	if (unit->getOriginalFaction() == FACTION_HOSTILE
	if (unit->getFaction() == FACTION_HOSTILE
		&& unit->getMainHandWeapon() == nullptr)
//		&& unit->getHostileUnits().size() == 0)
	// TODO: and, if either no innate meleeWeapon, or a visible hostile is not within say 5 tiles.
	{
		//Log(LOG_INFO) << ". . no mainhand weapon or no ammo";
		//Log(LOG_INFO) << ". . . call pickupItem()";
		pickupItem(&action);
	}
	//Log(LOG_INFO) << ". findItem DONE";

	if (_playedAggroSound == false
		&& unit->getChargeTarget() != nullptr)
	{
		_playedAggroSound = true;

		const int soundId = unit->getAggroSound();
		if (soundId != -1)
			getResourcePack()->getSound("BATTLE.CAT", soundId)
								->play(-1, getMap()->getSoundAngle(unit->getPosition()));
	}
	//Log(LOG_INFO) << ". getChargeTarget DONE";


//	std::wostringstream ss; // debug.

	switch (action.type)
	{
		case BA_MOVE:
		{
//			ss << L"Walking to " << action.target;
//			_parentState->debug(ss.str());

			Pathfinding* const pf = _battleSave->getPathfinding();
			pf->setPathingUnit(action.actor);

			if (_battleSave->getTile(action.target) != nullptr)
				pf->calculate(action.actor, action.target);

			if (pf->getStartDirection() != -1)
				statePushBack(new UnitWalkBState(this, action));

			//Log(LOG_INFO) << ". BA_MOVE DONE";
			break;
		}

		case BA_SNAPSHOT:
		case BA_AUTOSHOT:
		case BA_AIMEDSHOT:
		case BA_THROW:
		case BA_MELEE:
		case BA_PSICONTROL:
		case BA_PSIPANIC:
		case BA_LAUNCH:
		{
//			ss.clear();
//			ss << L"Attack type = " << action.type
//					<< ", target = " << action.target
//					<< ", weapon = " << Language::utf8ToWstr(action.weapon->getRules()->getName());
//			_parentState->debug(ss.str());

			//Log(LOG_INFO) << ". . in action.type";
			switch (action.type)
			{
				case BA_PSICONTROL:
				case BA_PSIPANIC:
				{
//					statePushBack(new PsiAttackBState(this, action)); // post-cosmetic
					//Log(LOG_INFO) << ". . do Psi";
					action.weapon = _alienPsi;
					action.TU = unit->getActionTu(action.type, action.weapon);
					break;
				}

				default:
				{
					statePushBack(new UnitTurnBState(this, action));

					switch (action.type)
					{
						case BA_MELEE:
						{
							const std::string meleeWeapon = unit->getMeleeWeapon();
//							statePushBack(new MeleeAttackBState(this, action));
							bool instaWeapon = false;

							if (action.weapon != _universalFist
								&& meleeWeapon.empty() == false)
							{
								bool found = false;
								for (std::vector<BattleItem*>::const_iterator
										i = unit->getInventory()->begin();
										i != unit->getInventory()->end();
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
																getRuleset()->getItem(meleeWeapon),
																_battleSave->getNextItemId());
									action.weapon->setOwner(unit);
								}
							}
							else if (action.weapon != nullptr
								&& action.weapon->getRules()->getBattleType() != BT_MELEE
								&& action.weapon->getRules()->getBattleType() != BT_FIREARM)
							{
								action.weapon = nullptr;
							}

							if (action.weapon != nullptr) // also checked in getActionTu() & ProjectileFlyBState::init()
							{
								action.TU = unit->getActionTu(action.type, action.weapon);

								statePushBack(new ProjectileFlyBState(this, action));

								if (instaWeapon == true)
									_battleSave->removeItem(action.weapon);
							}

							return;
						}
					}
				}
			}

			//Log(LOG_INFO) << ". attack action.Type = " << action.type
			//				<< ", action.Target = " << action.target
			//				<< " action.Weapon = " << action.weapon->getRules()->getName().c_str();


			//Log(LOG_INFO) << ". . call ProjectileFlyBState()";
			statePushBack(new ProjectileFlyBState(this, action));
			//Log(LOG_INFO) << ". . ProjectileFlyBState DONE";

			switch (action.type)
			{
				case BA_PSIPANIC:
				case BA_PSICONTROL:
				{
					//Log(LOG_INFO) << ". . . in action.type Psi";
					//const bool success = _battleSave->getTileEngine()->psiAttack(&action);
					//Log(LOG_INFO) << ". . . success = " << success;
					if (_battleSave->getTileEngine()->psiAttack(&action) == true)
					{
						const BattleUnit* const psiVictim = _battleSave->getTile(action.target)->getTileUnit();
						Language* const lang = _parentState->getGame()->getLanguage();
						std::wstring wst;
						if (action.type == BA_PSICONTROL)
							wst = lang->getString("STR_IS_UNDER_ALIEN_CONTROL", psiVictim->getGender())
													.arg(psiVictim->getName(lang))
													.arg(action.value);
						else // Panic Atk
							wst = lang->getString("STR_PSI_PANIC_SUCCESS")
													.arg(action.value);

						_parentState->getGame()->pushState(new InfoboxState(wst));
					}
					//Log(LOG_INFO) << ". . . done Psi.";
				}
			}
		}
		//Log(LOG_INFO) << ". . action.type DONE";
		break;

		case BA_NONE:
		{
			//Log(LOG_INFO) << ". . in action.type None";
//			_parentState->debug(L"Idle");
			_AIActionCounter = 0;

			if (_battleSave->selectNextFactionUnit(true, _AISecondMove) == nullptr) // AI-faction turn done ->
			{
				if (_battleSave->getDebugMode() == false)
				{
					_endTurnRequested = true;
					//Log(LOG_INFO) << "BattlescapeGame::handleUnitAI() statePushBack(end AI turn) 2";
					statePushBack(nullptr); // end AI turn
				}
				else
				{
					_battleSave->selectNextFactionUnit();
					_debugPlay = true;
				}
			}

			if (_battleSave->getSelectedUnit() != nullptr)
			{
				_parentState->updateSoldierInfo();
				centerOnUnit(_battleSave->getSelectedUnit());

				if (_AISecondMove == false)
				{
					if (std::find(
							_battleSave->getShuffleUnits()->begin(),
							_battleSave->getShuffleUnits()->end(),
							_battleSave->getSelectedUnit())
								 -
						std::find(
							_battleSave->getShuffleUnits()->begin(),
							_battleSave->getShuffleUnits()->end(),
							unit) < 1)
					{
						_AISecondMove = true;
					}
				}
				//Log(LOG_INFO) << "[2]bsg secondMove = " << (int)_AISecondMove;
			}
		}
	}
	//Log(LOG_INFO) << "BattlescapeGame::handleUnitAI() EXIT";
}

/**
 * Handles the result of non target actions like priming a grenade or performing
 * a melee attack or using a medikit.
 * @note The action is set up in ActionMenuState.
 */
void BattlescapeGame::handleNonTargetAction()
{
	if (_currentAction.targeting == false)
	{
		_currentAction.cameraPosition = Position(0,0,-1);

		int showWarning = 0;

		// NOTE: These actions are done partly in ActionMenuState::btnActionMenuClick() and
		// this subsequently handles a greater or lesser proportion of the resultant niceties.
		//
		switch (_currentAction.type)
		{
			case BA_PRIME:
			case BA_DEFUSE:
				if (_currentAction.actor->spendTimeUnits(_currentAction.TU) == false)
				{
					_currentAction.result = "STR_NOT_ENOUGH_TIME_UNITS";
					showWarning = 1;
				}
				else
				{
					_currentAction.weapon->setFuse(_currentAction.value);

					if (_currentAction.value == -1)
					{
						_currentAction.result = "STR_GRENADE_IS_DEACTIVATED";
						showWarning = 1;
					}
					else if (_currentAction.value == 0)
					{
						_currentAction.result = "STR_GRENADE_IS_ACTIVATED";
						showWarning = 1;
					}
					else
					{
						_currentAction.result = "STR_GRENADE_IS_ACTIVATED_";
						showWarning = 2;
					}
				}
			break;

			case BA_USE:
				if (_currentAction.result.empty() == false)
					showWarning = 1;
				else if (_currentAction.targetUnit != nullptr)
				{
					_battleSave->reviveUnit(_currentAction.targetUnit);
					_currentAction.targetUnit = nullptr;
				}
			break;

			case BA_LAUNCH:
				if (_currentAction.result.empty() == false)
					showWarning = 1;
			break;

			case BA_MELEE:
				if (_currentAction.result.empty() == false)
					showWarning = 1;
				else if (_currentAction.actor->spendTimeUnits(_currentAction.TU) == false)
				{
					_currentAction.result = "STR_NOT_ENOUGH_TIME_UNITS";
					showWarning = 1;
				}
				else
				{
					statePushBack(new ProjectileFlyBState(this, _currentAction));
					return;
				}
			break;

			case BA_DROP:
				if (_currentAction.result.empty() == false)
					showWarning = 1;
				else
				{
					_battleSave->getTileEngine()->applyGravity(_currentAction.actor->getTile());
					getResourcePack()->getSound("BATTLE.CAT", ResourcePack::ITEM_DROP)
										->play(-1, getMap()->getSoundAngle(_currentAction.actor->getPosition()));
				}
			break;

			case BA_EXECUTE:
				if (_currentAction.result.empty() == false)
					showWarning = 1;
				else if (_currentAction.targetUnit != nullptr)
					executeUnit();
		}

		if (showWarning != 0)
		{
			if (showWarning == 1)
				_parentState->warning(_currentAction.result);
			else if (showWarning == 2)
				_parentState->warning(
									_currentAction.result,
									true,
									_currentAction.value);

			_currentAction.result.clear();
		}

		_currentAction.type = BA_NONE;
		_parentState->updateSoldierInfo();
	}

	setupCursor();
}

/**
 * Summary execution.
 */
void BattlescapeGame::executeUnit() // private.
{
	_currentAction.actor->aim();
	_currentAction.actor->clearCache();
	getMap()->cacheUnit(_currentAction.actor);

	const RuleItem* const itRule = _currentAction.weapon->getRules();
	BattleItem* const ammo = _currentAction.weapon->getAmmoItem();
	int
		soundId = -1,
		aniStart = 0,	// avoid vc++ linker warning.
		isMelee = 0;	// avoid vc++ linker warning.

	switch (itRule->getBattleType()) // find hit-sound & ani.
	{
		case BT_MELEE:
			isMelee = 1;
			aniStart = itRule->getMeleeAnimation();

			if ((soundId = ammo->getRules()->getMeleeHitSound()) == -1)
				if ((soundId = itRule->getMeleeHitSound()) == -1)
					soundId = ResourcePack::ITEM_DROP;
			break;
		case BT_FIREARM:
			aniStart = ammo->getRules()->getHitAnimation();

			if ((soundId = ammo->getRules()->getFireHitSound()) == -1)
				soundId = itRule->getFireHitSound();
	}

	if (soundId != -1)
		getResourcePack()->getSound("BATTLE.CAT", soundId)
							->play(-1, getMap()->getSoundAngle(_currentAction.actor->getPosition()));

	ammo->spendBullet(
				*_battleSave,
				*_currentAction.weapon);

	Position explVoxel = Position::toVoxelSpaceCentered(_currentAction.target, 2);
	Explosion* const explosion = new Explosion(
											explVoxel,
											aniStart,
											0,
											false,
											isMelee);
	getMap()->getExplosions()->push_back(explosion);
	_executeProgress = true;

	_currentAction.targetUnit->playDeathSound(); // scream little piggie

	_currentAction.actor->spendTimeUnits(_currentAction.TU);

	_currentAction.targetUnit->setHealth(0);
	_currentAction.targetUnit = nullptr;

	checkForCasualties(
				_currentAction.weapon,
				_currentAction.actor,
				false, false, true);
}

/**
 * Sets the cursor according to the selected action.
 */
void BattlescapeGame::setupCursor()
{
	getMap()->refreshSelectorPosition();

	CursorType cType;
	int quadrants = 1;

	if (_currentAction.targeting == true)
	{
		switch (_currentAction.type)
		{
			case BA_THROW:
				cType = CT_THROW;
			break;

			case BA_PSICONTROL:
			case BA_PSIPANIC:
			case BA_PSICONFUSE:
			case BA_PSICOURAGE:
			case BA_USE:
				cType = CT_PSI;
			break;

			case BA_LAUNCH:
				cType = CT_WAYPOINT;
			break;

			default:
				cType = CT_AIM;
		}
	}
	else
	{
		cType = CT_NORMAL;

		_currentAction.actor = _battleSave->getSelectedUnit();
		if (_currentAction.actor != nullptr)
			quadrants = _currentAction.actor->getArmor()->getSize();
	}

	getMap()->setCursorType(cType, quadrants);
}

/**
 * Determines whether a playable unit is selected.
 * @note Normally only player side units can be selected but in debug mode the
 * aLiens can play too :)
 * @note Is used to see if stats can be displayed and action buttons will work.
 * @return, true if a playable unit is selected
 */
bool BattlescapeGame::playableUnitSelected()
{
	return _battleSave->getSelectedUnit() != nullptr
		&& (_battleSave->getSide() == FACTION_PLAYER
			|| _battleSave->getDebugMode() == true);
}

/**
 * Toggles the kneel/stand status of a unit.
 * TODO: allow Civilian units to kneel when controlled by medikit or by AI.
 * @param unit - pointer to a BattleUnit
 * @return, true if the action succeeded
 */
bool BattlescapeGame::kneel(BattleUnit* const unit)
{
	//Log(LOG_INFO) << "BattlescapeGame::kneel()";
	if (unit->getGeoscapeSoldier() != nullptr)
	{
		if (unit->isMindControlled() == false)
		{
			if (unit->isFloating() == false) // This prevents flying soldiers from 'kneeling' .....
			{
				int tu;
				if (unit->isKneeled() == true)
					tu = 10;
				else
					tu = 3;

//				if (checkReservedTu(unit, tu) == true)
//					|| (tu == 3 && _battleSave->getKneelReserved() == true))
//				{
				if (unit->getTimeUnits() >= tu)
				{
					if (tu == 3
						|| (tu == 10
							&& unit->spendEnergy(std::max(0,
													5 - unit->getArmor()->getAgility())) == true))
					{
						unit->spendTimeUnits(tu);
						unit->kneel(unit->isKneeled() == false);
						// kneeling or standing up can reveal new terrain or units. I guess. -> sure can!
						// But updateSoldierInfo() also does does calculateFOV(), so ...
//						getTileEngine()->calculateFOV(unit);

						getMap()->cacheUnits();

//						_parentState->updateSoldierInfo(false); // <- also does calculateFOV() !
						// wait... shouldn't one of those calcFoV's actually trigger!! ? !
						// Hopefully it's done after returning, in another updateSoldierInfo... or newVis check.
						// So.. I put this in BattlescapeState::btnKneelClick() instead; updates will
						// otherwise be handled by walking or what have you. Doing it this way conforms
						// updates/FoV checks with my newVis routines.

//						getTileEngine()->checkReactionFire(unit);
						// ditto..

						_parentState->toggleKneelButton(unit);
						return true;
					}
					else
						_parentState->warning("STR_NOT_ENOUGH_ENERGY");
				}
				else
					_parentState->warning("STR_NOT_ENOUGH_TIME_UNITS");
//				}
//				else // note that checkReservedTu() sends its own warnings ....
//					_parentState->warning("STR_TIME_UNITS_RESERVED");
			}
			else
				_parentState->warning("STR_ACTION_NOT_ALLOWED_FLOAT");
		}
		else //if (unit->getGeoscapeSoldier() != nullptr) // MC'd xCom agent, trying to stand & walk by AI.
		{
			const int energyCost = std::max(0,
										5 - unit->getArmor()->getAgility());

			if (unit->getTimeUnits() > 9
				&& unit->getEnergy() >= energyCost)
			{
				unit->spendTimeUnits(10);
				unit->spendEnergy(energyCost);

				unit->kneel(false);
				getMap()->cacheUnits();

				return true;
			}
		}
	}
	else //if (unit->getOriginalFaction() == FACTION_HOSTILE
//		&& unit->isMindControlled() == true) //getFaction() == FACTION_PLAYER)
//		&& unit->getUnitRules()->isMechanical() == false) // MOB has Unit-rules
	{
		_parentState->warning("STR_ACTION_NOT_ALLOWED_ALIEN"); // TODO: change to "not a Soldier, can't kneel".
	}

	return false;
}

/**
 * Ends the turn.
 * @note This starts the switchover from one faction to the next.
 */
void BattlescapeGame::endTurn() // private.
{
	//Log(LOG_INFO) << "bg::endTurn()";
	_debugPlay = false;
	_AISecondMove = false;
	_parentState->showLaunchButton(false);

	_currentAction.targeting = false;
	_currentAction.type = BA_NONE;
	_currentAction.waypoints.clear();
	getMap()->getWaypoints()->clear();

	Tile* tile;
	Position pos;

//	if (_endTurnProcessed == false)
//	{
	for (size_t // check for hot grenades on the ground
			i = 0;
			i != _battleSave->getMapSizeXYZ();
			++i)
	{
		tile = _battleSave->getTiles()[i];
		for (std::vector<BattleItem*>::const_iterator
				j = tile->getInventory()->begin();
				j != tile->getInventory()->end();
				)
		{
			if ((*j)->getRules()->getBattleType() == BT_GRENADE
				&& (*j)->getFuse() != -1
				&& (*j)->getFuse() < 2) // it's a grenade to explode now
			{
				pos = Position::toVoxelSpaceCentered(
												tile->getPosition(),
												2 - tile->getTerrainLevel());
				statePushNext(new ExplosionBState(
												this, pos, *j,
												(*j)->getPriorOwner()));
				_battleSave->removeItem(*j);

				statePushBack(nullptr);
				return;
			}

			++j;
		}
	}

	if (_battleSave->getTileEngine()->closeUfoDoors() == true) // close doors between grenade & terrain explosions
		getResourcePack()->getSound("BATTLE.CAT", ResourcePack::SLIDING_DOOR_CLOSE)->play();
//	}

	tile = _battleSave->getTileEngine()->checkForTerrainExplosions();
	if (tile != nullptr)
	{
		pos = Position::toVoxelSpaceCentered(tile->getPosition(), 10);
		// kL_note: This seems to be screwing up.
		// Further info: what happens is that an explosive part of a tile gets destroyed by fire
		// during an end-turn sequence, has its setExplosive() set, then is somehow triggered
		// by the next projectile hit against whatever.
		statePushNext(new ExplosionBState(
										this,
										pos,
										nullptr,
										nullptr,
										tile));

//		tile = _battleSave->getTileEngine()->checkForTerrainExplosions();

		statePushBack(nullptr);	// this will repeatedly call another endTurn() so there's
		return;					// no need to continue this one till all explosions are done.
								// The problem arises because _battleSave->endFactionTurn() below
								// causes *more* destruction of explosive objects, that won't explode
								// until some later instantiation of ExplosionBState .....
								//
								// As to why this doesn't simply loop like other calls to
								// do terrainExplosions, i don't know.
	}

//	if (_endTurnProcessed == false)
//	{
	if (_battleSave->getSide() != FACTION_NEUTRAL) // tick down grenade timers
	{
		for (std::vector<BattleItem*>::const_iterator
				i = _battleSave->getItems()->begin();
				i != _battleSave->getItems()->end();
				++i)
		{
			if ((*i)->getOwner() == nullptr
				&& (*i)->getRules()->getBattleType() == BT_GRENADE
				&& (*i)->getFuse() > 1)
			{
				(*i)->setFuse((*i)->getFuse() - 1);
			}
		}
	}

	// THE NEXT 3 BLOCKS could get Quirky. (ie: tiles vs. units, tallyUnits, tiles vs. ground-items)
	for (std::vector<BattleUnit*>::const_iterator
			i = _battleSave->getUnits()->begin();
			i != _battleSave->getUnits()->end();
			++i)
	{
		if ((*i)->getOriginalFaction() == _battleSave->getSide()
			&& (*i)->isOut_t(OUT_STAT) == false)
		{
			tile = (*i)->getTile();
			if (tile != nullptr
				&& (tile->getSmoke() != 0 || tile->getFire() != 0))
			{
				tile->hitTileInventory(); // Damage tile's unit w/ Smoke & Fire at end of unit's faction's Turn-phase.
			}
		}
	}


	if (_battleSave->endFactionTurn() == true) // <- This rolls over Faction-turns. TRUE means FullTurn has ended.
	{
		for (size_t
				i = 0;
				i != _battleSave->getMapSizeXYZ();
				++i)
		{
			tile = _battleSave->getTiles()[i];
			if (tile->getInventory()->empty() == false
				&& (tile->getSmoke() != 0 || tile->getFire() != 0))
			{
				tile->hitTileInventory(_battleSave); // Damage tile's items w/ Fire at beginning of each full-turn.
			}
		}

		for (std::vector<BattleUnit*>::const_iterator // reset the takenExpl(smoke) and takenFire flags on every unit.
				i = _battleSave->getUnits()->begin();
				i != _battleSave->getUnits()->end();
				++i)
		{
			(*i)->setTakenExpl(false); // even if Status_Dead, just do it.
			(*i)->setTakenFire(false);
		}
	}
	// best just to do another call to checkForTerrainExplosions()/ ExplosionBState in there ....
	// -> SavedBattleGame::tileVolatiles()
	// Or here
	// ... done it in NextTurnState.

		// check AGAIN for terrain explosions
/*		tile = _battleSave->getTileEngine()->checkForTerrainExplosions();
		if (tile != nullptr)
		{
			pos = Position(
						tile->getPosition().x * 16 + 8,
						tile->getPosition().y * 16 + 8,
						tile->getPosition().z * 24 + 10);
			statePushNext(new ExplosionBState(
											this,
											pos,
											nullptr,
											nullptr,
											tile));
			statePushBack(nullptr);
			_endTurnProcessed = true;
			return;
		} */
		// kL_note: you know what, I'm just going to let my quirky solution run
		// for a while. BYPASS _endTurnProcessed
//	}
//	_endTurnProcessed = false;

	if (_battleSave->getDebugMode() == false)
	{
		if (_battleSave->getSide() == FACTION_PLAYER)
		{
			setupCursor();
			_battleSave->getBattleState()->toggleIcons(true);
		}
		else
		{
			getMap()->setCursorType(CT_NONE);
			_battleSave->getBattleState()->toggleIcons(false);
		}
	}


	checkForCasualties();

	int // if all units from either faction are killed - the mission is over.
		liveAliens,
		liveSoldiers;
	const bool hostilesPacified = tallyUnits(
										liveAliens,
										liveSoldiers);

	if (_battleSave->getObjectiveType() == MUST_DESTROY // brain death, end Final Mission.
		&& _battleSave->allObjectivesDestroyed() == true)
	{
		_parentState->finishBattle(
								false,
								liveSoldiers);
		return;
	}

	const bool battleComplete = liveAliens == 0
							 || liveSoldiers == 0;

	if (battleComplete == false)
	{
		showInfoBoxQueue();
		_parentState->updateSoldierInfo();

		if (playableUnitSelected() == true) // <- only Faction_Player or Debug-mode
		{
			centerOnUnit(_battleSave->getSelectedUnit());
			setupCursor();
		}

		if (hostilesPacified == true)
			_battleSave->setPacified();
	}

	if (_endTurnRequested == true)
	{
		_endTurnRequested = false;

		if (battleComplete == true
			|| _battleSave->getSide() != FACTION_NEUTRAL)
		{
			_parentState->getGame()->delayBlit();
			_parentState->getGame()->pushState(new NextTurnState(
															_battleSave,
															_parentState,
															hostilesPacified));
		}
	}
}

/**
 * Checks for casualties and adjusts morale accordingly.
 * @note Also checks if Alien Base Control was destroyed in a BaseAssault tactical.
 * @param weapon		- pointer to the weapon responsible (default nullptr)
 * @param attacker		- pointer to credit the kill (default nullptr)
 * @param hiddenExpl	- true for UFO Power Source explosions at the start of battlescape (default false)
 * @param terrainExpl	- true for terrain explosions (default false)
 * @param execution		- true if called by an execution (default false)
 */
void BattlescapeGame::checkForCasualties(
		const BattleItem* const weapon,
		BattleUnit* attacker,
		bool hiddenExpl,
		bool terrainExpl,
		bool execution)
{
	//Log(LOG_INFO) << "BattlescapeGame::checkForCasualties()"; if (attacker != nullptr) Log(LOG_INFO) << ". id-" << attacker->getId();

	// If the victim was killed by the attacker's death explosion,
	// fetch who killed the attacker and make THAT the attacker!
	if (attacker != nullptr && execution == false)
	{
		if (attacker->getUnitStatus() == STATUS_DEAD
			&& attacker->getMurdererId() != 0
			&& attacker->getSpecialAbility() == SPECAB_EXPLODE)
		{
			for (std::vector<BattleUnit*>::const_iterator
					i = _battleSave->getUnits()->begin();
					i != _battleSave->getUnits()->end();
					++i)
			{
				if ((*i)->getId() == attacker->getMurdererId())
				{
					attacker = *i;
					break;
				}
			}
		}

		// attacker gets Exposed if a spotter is still conscious
		// NOTE: Useful only after Melee attacks. Firearms & explosives handle
		// things differently ... see note in TileEngine::checkReactionFire().
		checkExposedByMelee(attacker);
	}
	// kL_note: what about tile explosions


	std::string
		killStatRace		= "STR_UNKNOWN",
		killStatRank		= "STR_UNKNOWN",
		killStatWeapon		= "STR_WEAPON_UNKNOWN",
		killStatWeaponAmmo	= "STR_WEAPON_UNKNOWN";
	int
		killStatMission	= 0,
		killStatTurn	= 0,
		killStatPoints	= 0;


	if (attacker != nullptr
		&& attacker->getGeoscapeSoldier() != nullptr)
	{
		killStatMission = _battleSave->getGeoscapeSave()->getMissionStatistics()->size();
		killStatTurn = _battleSave->getTurn() * 3 + static_cast<int>(_battleSave->getSide());

		if (weapon != nullptr)
		{
			killStatWeapon =
			killStatWeaponAmmo = weapon->getRules()->getName();
		}

		const RuleItem* itRule;
		const BattleItem* item = attacker->getItem(ST_RIGHTHAND);
		if (item != nullptr)
		{
			itRule = item->getRules();
			for (std::vector<std::string>::const_iterator
					i = itRule->getCompatibleAmmo()->begin();
					i != itRule->getCompatibleAmmo()->end();
					++i)
			{
				if (*i == killStatWeaponAmmo)
					killStatWeapon = itRule->getName();
			}
		}

		item = attacker->getItem(ST_LEFTHAND);
		if (item != nullptr)
		{
			itRule = item->getRules();
			for (std::vector<std::string>::const_iterator
					i = itRule->getCompatibleAmmo()->begin();
					i != itRule->getCompatibleAmmo()->end();
					++i)
			{
				if (*i == killStatWeaponAmmo)
					killStatWeapon = itRule->getName();
			}
		}
	}


	bool
		dead,
		stunned,
		converted,
		bypass;

	for (std::vector<BattleUnit*>::const_iterator
			i = _battleSave->getUnits()->begin();
			i != _battleSave->getUnits()->end();
			++i)
	{
		if ((*i)->getUnitStatus() != STATUS_LIMBO) // kL_tentative.
		{
			dead = (*i)->isOut_t(OUT_HLTH);
			stunned = (*i)->isOut_t(OUT_STUN);

			converted =
			bypass = false;

			if (dead == false) // for converting infected units that aren't dead.
			{
				if ((*i)->getSpawnUnit() == "STR_ZOMBIE") // human->zombie (nobody cares about zombie->chryssalid)
				{
					converted = true;
					convertUnit(*i);
				}
				else if (stunned == false)
					bypass = true;
			}

			if (bypass == false)
			{
				BattleUnit* const defender = *i; // kL

				// Awards: decide victim race and rank
				// TODO: if a unit was stunned but gets up and is re-stunned or killed,
				// erase it from the previous attacker's BattleUnitKill vector and add
				// it to the subsequent attacker.
				if (attacker != nullptr
					&& attacker->getGeoscapeSoldier() != nullptr)
				{
					killStatPoints = defender->getValue();

					if (defender->getOriginalFaction() == FACTION_PLAYER)	// <- xCom DIED
					{
						killStatPoints = -killStatPoints;

						if (defender->getGeoscapeSoldier() != nullptr)		// Soldier
						{
							killStatRace = "STR_SOLDIER";
							killStatRank = defender->getGeoscapeSoldier()->getRankString();
						}
						else												// Support unit
						{
							killStatRace = defender->getUnitRules()->getRace();
							killStatRank = "STR_SUPPORT";
						}
					}
					else if (defender->getOriginalFaction() == FACTION_HOSTILE)	// <- aLien DIED
					{
						killStatRace = defender->getUnitRules()->getRace();
						killStatRank = defender->getUnitRules()->getRank();
					}
					else if (defender->getOriginalFaction() == FACTION_NEUTRAL)	// <- Civilian DIED
					{
						killStatPoints = -killStatPoints * 2;
						killStatRace = "STR_HUMAN";
						killStatRank = "STR_CIVILIAN";
					}
				}


				if ((dead == true
						&& defender->getUnitStatus() != STATUS_DEAD
						&& defender->getUnitStatus() != STATUS_COLLAPSING	// kL_note: is this really needed ....
						&& defender->getUnitStatus() != STATUS_TURNING		// kL: may be set by UnitDieBState cTor
						&& defender->getUnitStatus() != STATUS_DISABLED)	// kL
					|| converted == true)
				{
					if (execution == true)
						defender->setUnitStatus(STATUS_DEAD);
					else if (dead == true)
						defender->setUnitStatus(STATUS_DISABLED);

					// attacker's Morale Bonus & diary ->
					if (attacker != nullptr)
					{
						defender->killedBy(attacker->getFaction()); // used in DebriefingState.
						//Log(LOG_INFO) << "BSG::checkForCasualties() " << defender->getId() << " killedBy = " << (int)attacker->getFaction();

						if (attacker->getGeoscapeSoldier() != nullptr)
						{
							defender->setMurdererId(attacker->getId());
							attacker->getStatistics()->kills.push_back(new BattleUnitKill(
																						killStatRank,
																						killStatRace,
																						killStatWeapon,
																						killStatWeaponAmmo,
																						defender->getFaction(),
																						STATUS_DEAD,
																						killStatMission,
																						killStatTurn,
																						killStatPoints));
						}

						if (attacker->isFearable() == true)
						{
							int bonus;
							if (attacker->getOriginalFaction() == FACTION_PLAYER)
							{
								bonus = _battleSave->getMoraleModifier();

								if (attacker->getFaction() == FACTION_PLAYER		// not MC'd
									&& attacker->getGeoscapeSoldier() != nullptr)	// is Soldier
								{
									attacker->addKillCount();
								}
							}
							else if (attacker->getOriginalFaction() == FACTION_HOSTILE)
								bonus = _battleSave->getMoraleModifier(nullptr, false);
							else
								bonus = 0;

							// attacker's Valor
							if ((attacker->getOriginalFaction() == FACTION_HOSTILE
									&& defender->getOriginalFaction() == FACTION_PLAYER)
								|| (attacker->getOriginalFaction() == FACTION_PLAYER
									&& defender->getOriginalFaction() == FACTION_HOSTILE))
							{
								const int courage = 10 * bonus / 100;
								attacker->moraleChange(courage); // double what rest of squad gets below
							}
							// attacker (mc'd or not) will get a penalty with friendly fire (mc'd or not)
							// ... except aLiens, who don't care.
							else if (attacker->getOriginalFaction() == FACTION_PLAYER
								&& defender->getOriginalFaction() == FACTION_PLAYER)
							{
								int chagrin = 5000 / bonus; // huge chagrin!
								if (defender->getUnitRules() != nullptr
									&& defender->getUnitRules()->isMechanical() == true)
								{
									chagrin /= 2;
								}
								attacker->moraleChange(-chagrin);
							}
							else if (defender->getOriginalFaction() == FACTION_NEUTRAL) // civilian kills
							{
								if (attacker->getOriginalFaction() == FACTION_PLAYER)
								{
									const int dishonor = 2000 / bonus;
									attacker->moraleChange(-dishonor);
								}
								else if (attacker->getOriginalFaction() == FACTION_HOSTILE)
									attacker->moraleChange(20); // no leadership bonus for aLiens executing civies: it's their job.
							}
						}
					}

					// cycle through units and do all faction
//					if (defender->getFaction() != FACTION_NEUTRAL) // civie deaths now affect other Factions.
//					{
					// penalty for the death of a unit; civilians & MC'd aLien units return 100.
					const int loss = _battleSave->getMoraleModifier(defender);
					// These two are factions (aTeam & bTeam leaderships mitigate losses).
					int
						aTeam, // winners
						bTeam; // losers

					if (defender->getOriginalFaction() == FACTION_HOSTILE)
					{
						aTeam = _battleSave->getMoraleModifier();
						bTeam = _battleSave->getMoraleModifier(nullptr, false);
					}
					else // victim is xCom or civilian
					{
						aTeam = _battleSave->getMoraleModifier(nullptr, false);
						bTeam = _battleSave->getMoraleModifier();
					}

					for (std::vector<BattleUnit*>::const_iterator // do bystander FACTION changes:
							j = _battleSave->getUnits()->begin();
							j != _battleSave->getUnits()->end();
							++j)
					{
//						if ((*j)->isOut(true, true) == false
						if ((*j)->isOut_t() == false
							&& (*j)->isFearable() == true) // not mechanical. Or a ZOMBIE!!
						{
							if ((*j)->getOriginalFaction() == defender->getOriginalFaction()
								|| (defender->getOriginalFaction() == FACTION_NEUTRAL			// for civie-death,
									&& (*j)->getFaction() == FACTION_PLAYER						// non-Mc'd xCom takes hit
									&& (*j)->getOriginalFaction() != FACTION_HOSTILE)			// but not Mc'd aLiens
								|| (defender->getOriginalFaction() == FACTION_PLAYER			// for death of xCom unit,
									&& (*j)->getOriginalFaction() == FACTION_NEUTRAL))			// civies take hit.
							{
								// losing team(s) all get a morale loss
								// based on their individual Bravery & rank of unit that was killed
								int moraleLoss = (110 - (*j)->getBattleStats()->bravery) / 10;
								if (moraleLoss > 0) // pure safety, ain't gonna happen really.
								{
									moraleLoss = moraleLoss * loss * 2 / bTeam;
									if (converted == true)
										moraleLoss = (moraleLoss * 5 + 3) / 4; // extra loss if xCom or civie turns into a Zombie.
									else if (defender->getUnitRules() != nullptr
										&& defender->getUnitRules()->isMechanical() == true)
									{
										moraleLoss /= 2;
									}

									(*j)->moraleChange(-moraleLoss);
								}
/*								if (attacker
									&& attacker->getFaction() == FACTION_PLAYER
									&& defender->getFaction() == FACTION_HOSTILE)
								{
									attacker->setExposed(); // interesting
									//Log(LOG_INFO) << ". . . . attacker Exposed";
								} */
							}
							else if ((((*j)->getOriginalFaction() == FACTION_PLAYER
										|| (*j)->getOriginalFaction() == FACTION_NEUTRAL)
									&& defender->getOriginalFaction() == FACTION_HOSTILE)
								|| ((*j)->getOriginalFaction() == FACTION_HOSTILE
									&& (defender->getOriginalFaction() == FACTION_PLAYER
										|| defender->getOriginalFaction() == FACTION_NEUTRAL)))
							{
								// winning faction(s) all get a morale boost unaffected by rank of the dead unit
								(*j)->moraleChange(aTeam / 10);
							}
						}
					}
//					}

					if (execution == false && converted == false)
					{
						DamageType dType;
						if (weapon != nullptr)
							dType = weapon->getRules()->getDamageType();
						else // hidden or terrain explosion or death by fatal wounds
						{
							if (hiddenExpl == true) // this is instant death from UFO powersources without screaming sounds
								dType = DT_HE;
							else
							{
								if (terrainExpl == true)
									dType = DT_HE;
								else // no attacker and no terrain explosion - must be fatal wounds
									dType = DT_NONE; // -> STR_HAS_DIED_FROM_A_FATAL_WOUND
							}
						}

						statePushNext(new UnitDieBState( // This is where units get sent to DEATH!
													this,
													*i,
													dType,
													hiddenExpl));
					}
				}
				else if (stunned == true
					&& defender->getUnitStatus() != STATUS_DEAD
					&& defender->getUnitStatus() != STATUS_UNCONSCIOUS
					&& defender->getUnitStatus() != STATUS_COLLAPSING	// kL_note: is this really needed ....
					&& defender->getUnitStatus() != STATUS_TURNING		// kL_note: may be set by UnitDieBState cTor
					&& defender->getUnitStatus() != STATUS_DISABLED)	// kL
				{
					(*i)->setUnitStatus(STATUS_DISABLED); // kL

					if (attacker != nullptr
						&& attacker->getGeoscapeSoldier() != nullptr)
					{
						attacker->getStatistics()->kills.push_back(new BattleUnitKill(
																					killStatRank,
																					killStatRace,
																					killStatWeapon,
																					killStatWeaponAmmo,
																					defender->getFaction(),
																					STATUS_UNCONSCIOUS,
																					killStatMission,
																					killStatTurn,
																					killStatPoints));
					}

					if (defender != nullptr
						&& defender->getGeoscapeSoldier() != nullptr)
					{
						defender->getStatistics()->wasUnconscious = true;
					}

					statePushNext(new UnitDieBState( // This is where units get sent to STUNNED.
												this,
												*i,
												DT_STUN,
												true));
				}
			}
		}
	}


	_parentState->hotWoundsRefresh();

	if (hiddenExpl == false)
	{
		if (_battleSave->getSide() == FACTION_PLAYER)
		{
			const BattleUnit* const unit = _battleSave->getSelectedUnit();
			_parentState->showPsiButton(unit != nullptr
									 && unit->getOriginalFaction() == FACTION_HOSTILE
									 && unit->getBattleStats()->psiSkill != 0
									 && unit->isOut_t() == false);
		}

/*		if (_battleSave->getTacType() == TCT_BASEASSAULT // do this in SavedBattleGame::addDestroyedObjective()
			&& _battleSave->getControlDestroyed() == false)
		{
			bool controlDestroyed = true;
			for (size_t
					i = 0;
					i != _battleSave->getMapSizeXYZ();
					++i)
			{
				if (_battleSave->getTiles()[i]->getMapData(O_OBJECT) != nullptr
					&& _battleSave->getTiles()[i]->getMapData(O_OBJECT)->getSpecialType() == UFO_NAVIGATION)
				{
					controlDestroyed = false;
					break;
				}
			}
			if (controlDestroyed == true)
			{
				_battleSave->setControlDestroyed();
				Game* const game = _parentState->getGame();
				game->pushState(new InfoboxOKState(game->getLanguage()->getString("STR_ALIEN_BASE_CONTROL_DESTROYED")));
			}
		} */
	}
}

/**
 * Checks if a BattleUnit gets exposed after making a melee-attack.
 * @param unit - the unit to check
 */
void BattlescapeGame::checkExposedByMelee(BattleUnit* const unit) const
{
	//Log(LOG_INFO) << ". Casualties: check for spotters Qty = " << (int)unit->getRfSpotters()->size();
	if (unit->getRfSpotters()->empty() == false)
	{
		for (std::list<BattleUnit*>::const_iterator // -> not sure what happens if RF-trigger kills Cyberdisc that kills aLien .....
				i = unit->getRfSpotters()->begin();
				i != unit->getRfSpotters()->end();
				++i)
		{
			if ((*i)->isOut_t(OUT_HLTH_STUN) == false)
			{
				//Log(LOG_INFO) << ". . melee attacker spotted id-" << unit->getId();
				unit->setExposed(); // defender has been spotted on Player turn.
				break;
			}
		}

		unit->getRfSpotters()->clear();
	}
}

/**
 * Shows the infoboxes in the queue if any.
 */
void BattlescapeGame::showInfoBoxQueue() // private.
{
	for (std::vector<InfoboxOKState*>::const_iterator
			i = _infoboxQueue.begin();
			i != _infoboxQueue.end();
			++i)
	{
		_parentState->getGame()->pushState(*i);
	}

	_infoboxQueue.clear();
}

/**
 * Checks against reserved time units.
 * @param unit	- pointer to a unit
 * @param tu	- TU to check against
 * @return, true if unit has enough time units - go!
 */
bool BattlescapeGame::checkReservedTu(
		BattleUnit* const unit,
		int tu) const
{
	if (unit->getFaction() != _battleSave->getSide()	// is RF.
		|| _battleSave->getSide() == FACTION_NEUTRAL)	// or Civies
	{
		return tu <= unit->getTimeUnits();
	}


	BattleActionType batReserved; // avoid changing _batReserved here.

	if (_battleSave->getSide() == FACTION_HOSTILE
		&& _debugPlay == false)
	{
		const AlienBAIState* const ai (dynamic_cast<AlienBAIState*>(unit->getAIState()));
		if (ai != nullptr)
			batReserved = ai->getReservedAIAction();
		else
			batReserved = BA_NONE;	// something went ... wrong. Should always be an AI for non-player units (although i
									// guess it could-maybe-but-unlikely be a CivilianBAIState here in checkReservedTu()).
		const int extraReserve = RNG::generate(0,13); // added in below ->

		// This could use some tweaking, for the poor aLiens:
		switch (batReserved) // aLiens reserve TUs as a percentage rather than just enough for a single action.
		{
			case BA_SNAPSHOT:
			return (tu + extraReserve + (unit->getBattleStats()->tu / 3) <= unit->getTimeUnits());		// 33%

			case BA_AUTOSHOT:
			return (tu + extraReserve + (unit->getBattleStats()->tu * 2 / 5) <= unit->getTimeUnits());	// 40%

			case BA_AIMEDSHOT:
			return (tu + extraReserve + (unit->getBattleStats()->tu / 2) <= unit->getTimeUnits());		// 50%

			default:
			return (tu <= unit->getTimeUnits()); // + extraReserve
		}
	}

	// ** Below here is for xCom soldiers exclusively ***
	// (which i don't care about - except that this is also used for pathPreviews in Pathfinding object)
//	batReserved = _battleSave->getBatReserved();
	batReserved = BA_NONE;	// <- default for player's units
							// <- for use when called by Pathfinding::previewPath() only.
	const BattleItem* weapon;
	switch (unit->getActiveHand())
	{
		case AH_RIGHT:
			weapon = unit->getItem(ST_RIGHTHAND);
			break;
		case AH_LEFT:
			weapon = unit->getItem(ST_LEFTHAND);
			break;

		default:
		case AH_NONE:
			weapon = unit->getMainHandWeapon(false);
	}

	if (weapon != nullptr)
	{
		if (weapon->getRules()->getBattleType() == BT_MELEE)
			batReserved = BA_MELEE;
		else if (weapon->getRules()->getBattleType() == BT_FIREARM)
		{
			if (unit->getActionTu(batReserved = BA_SNAPSHOT, weapon) == 0)
				if (unit->getActionTu((batReserved = BA_AUTOSHOT), weapon) == 0)
					batReserved = BA_AIMEDSHOT;
		}
	}

	if (tu + unit->getActionTu(batReserved, weapon) > unit->getTimeUnits()) // safeties in place @ getActionTu()
		return false;

	return true;
}
// * @param test - true to suppress error messages (default false)
/*		// if reserved for Aimed shot drop to Auto shot
		if (batReserved == BA_AIMEDSHOT
			&& unit->getActionTu(BA_AIMEDSHOT, weapon) == 0)
		{
			batReserved = BA_AUTOSHOT;
		}
		// if reserved for Auto shot drop to Snap shot
		if (batReserved == BA_AUTOSHOT
			&& unit->getActionTu(BA_AUTOSHOT, weapon) == 0)
		{
			batReserved = BA_SNAPSHOT;
		}
		// if reserved for Snap shot try Auto shot
		if (//batReserved == BA_SNAPSHOT &&
			unit->getActionTu(BA_SNAPSHOT, weapon) == 0)
		{
			batReserved = BA_AUTOSHOT;
		}
		// if reserved for Auto shot try Aimed shot
		if (batReserved == BA_AUTOSHOT
			&& unit->getActionTu(BA_AUTOSHOT, weapon) == 0)
		{
			batReserved = BA_AIMEDSHOT;
		} */
/*	int tuKneel;
	if (_battleSave->getKneelReserved() == true
		&& unit->getGeoscapeSoldier() != nullptr
		&& unit->isKneeled() == false)
	{
		tuKneel = 3;
	}
	else tuKneel = 0;

	// if no Aimed shot is available revert to bat_NONE
	if (batReserved == BA_AIMEDSHOT
		&& unit->getActionTu(BA_AIMEDSHOT, weapon) == 0)
	{
		if (tuKneel != 0) batReserved = BA_NONE;
		else return true;
	}

	if (batReserved != BA_NONE //|| _battleSave->getKneelReserved() == true)
		&& tu + tuKneel + unit->getActionTu(batReserved, weapon) > unit->getTimeUnits()
		&& (test == true || tuKneel + unit->getActionTu(batReserved, weapon) <= unit->getTimeUnits()))
	{
		if (test == false)
		{
			if (tuKneel != 0)
			{
				switch (batReserved)
				{
					case BA_NONE: _parentState->warning("STR_TIME_UNITS_RESERVED_FOR_KNEELING");
					break;
					default: _parentState->warning("STR_TIME_UNITS_RESERVED_FOR_KNEELING_AND_FIRING");
				}
			}
			else
			{
				switch (_battleSave->getBatReserved())
				{
					case BA_SNAPSHOT: _parentState->warning("STR_TIME_UNITS_RESERVED_FOR_SNAP_SHOT");
					break;
					case BA_AUTOSHOT: _parentState->warning("STR_TIME_UNITS_RESERVED_FOR_AUTO_SHOT");
					break;
					case BA_AIMEDSHOT: _parentState->warning("STR_TIME_UNITS_RESERVED_FOR_AIMED_SHOT");
				}
			}
		}
		return false;
	} */

/**
 * Picks the first soldier that is panicking.
 * @note Called from BattlescapeGame::think() at start of player's turn.
 * @return, true when all panicking is over
 */
bool BattlescapeGame::handlePanickingPlayer() // private.
{
	//Log(LOG_INFO) << "bg::handlePanickingPlayer()";
	for (std::vector<BattleUnit*>::const_iterator
			i = _battleSave->getUnits()->begin();
			i != _battleSave->getUnits()->end();
			++i)
	{
		if ((*i)->getFaction() == FACTION_PLAYER
			&& (*i)->isMindControlled() == false
			&& handlePanickingUnit(*i) == true)
		{
			//Log(LOG_INFO) << ". ret FALSE " << (*i)->getId();
			return false;
		}
	}

	//Log(LOG_INFO) << ". ret TRUE";
	return true;
}

/**
 * Handles panicking units.
 * @note Called from BattlescapeGame::think() at start of non-player turn.
 * @return, true if unit is panicking
 */
bool BattlescapeGame::handlePanickingUnit(BattleUnit* const unit) // private.
{
	//Log(LOG_INFO) << "bg::handlePanickingUnit() - " << unit->getId();
	const UnitStatus status = unit->getUnitStatus();

	if (status == STATUS_PANICKING
		|| status == STATUS_BERSERK)
	{
		_parentState->getMap()->setCursorType(CT_NONE);
		_battleSave->setSelectedUnit(unit);

		if (Options::battleAlienPanicMessages == true
			|| unit->getUnitVisible() == true)
		{
			//Log(LOG_INFO) << "bg: panic id-" << unit->getId();
			centerOnUnit(unit, true);

			Game* const game = _parentState->getGame();
			std::string st;
			if (status == STATUS_PANICKING)
				st = "STR_HAS_PANICKED";
			else
				st = "STR_HAS_GONE_BERSERK";

			game->pushState(new InfoboxState(
										game->getLanguage()->getString(st, unit->getGender())
																.arg(unit->getName(game->getLanguage()))));
		}

		if (unit->getAIState() != nullptr)
		{
			if (unit->getOriginalFaction() == FACTION_PLAYER)
				unit->setAIState();
			else
				unit->getAIState()->resetAI();
		}

		unit->setUnitStatus(STATUS_STANDING);
		BattleAction action;
		action.actor = unit;
		int tu = unit->getTimeUnits();

		switch (status)
		{
			case STATUS_PANICKING:
			{
				//Log(LOG_INFO) << ". PANIC";
				BattleItem* item;
				if (RNG::percent(75) == true)
				{
					item = unit->getItem(ST_RIGHTHAND);
					if (item != nullptr)
						dropItem(
								unit->getPosition(),
								item,
								false,
								true);
				}

				if (RNG::percent(75) == true)
				{
					item = unit->getItem(ST_LEFTHAND);
					if (item != nullptr)
						dropItem(
								unit->getPosition(),
								item,
								false,
								true);
				}

				unit->clearCache();

				Pathfinding* const pf = _battleSave->getPathfinding();
				pf->setPathingUnit(unit);

				const std::vector<int> reachable (pf->findReachable(unit, tu));
				const size_t tileId (static_cast<size_t>(reachable[RNG::pick(reachable.size())])); // <-- WARNING: no Safety on size !

				_battleSave->tileCoords(
									tileId,
									&action.target.x,
									&action.target.y,
									&action.target.z);
				pf->calculate(
							action.actor,
							action.target,
							nullptr,
							tu);

				if (pf->getStartDirection() != -1)
				{
					action.actor->setDashing();
					action.dash = true;
					action.type = BA_MOVE;
					statePushBack(new UnitWalkBState(this, action));
				}
			}
			break;

			case STATUS_BERSERK:
			{
				//Log(LOG_INFO) << ". BERSERK";
				action.type = BA_TURN;
				const int pivotQty (RNG::generate(2,5));
				for (int
						i = 0;
						i != pivotQty;
						++i)
				{
					action.target = Position(
									unit->getPosition().x + RNG::generate(-5,5),
									unit->getPosition().y + RNG::generate(-5,5),
									unit->getPosition().z);
					statePushBack(new UnitTurnBState(this, action, false));
				}

				action.weapon = unit->getMainHandWeapon();
				if (action.weapon == nullptr)
					action.weapon = unit->getGrenade();

				// TODO: run up to another unit and slug it with the Universal Fist.
				// Or w/ an already-equipped melee weapon

				if (action.weapon != nullptr)
				{
					if (action.weapon->getRules()->getBattleType() == BT_FIREARM)
					{
						Tile* const targetTile (_battleSave->getTiles()[RNG::pick(_battleSave->getMapSizeXYZ())]);
						action.target = targetTile->getPosition();
						if (_battleSave->getTile(action.target) != nullptr)
						{
							statePushBack(new UnitTurnBState(this, action, false));

							action.type = BA_SNAPSHOT;
							if (action.weapon->getAmmoItem() == nullptr
								|| action.weapon->getAmmoItem()->getRules()->getShotgunPellets() == 0)
							{
								action.cameraPosition = _battleSave->getBattleState()->getMap()->getCamera()->getMapOffset();
							}
							else
								action.cameraPosition = Position(0,0,-1);

							const int actionTu (action.actor->getActionTu(action.type, action.weapon));
							int shots; // tabulate how many shots can be fired before unit runs out of TUs
							if (actionTu != 0)
								shots = tu / actionTu;
							else
								shots = 0;

							for (int
									i = 0;
									i != shots;
									++i)
							{
								statePushBack(new ProjectileFlyBState(this, action));
							}
						}
					}
					else if (action.weapon->getRules()->getBattleType() == BT_GRENADE)
					{
						if (action.weapon->getFuse() == -1)
							action.weapon->setFuse(0); // yeh set timer even if throw is invalid.

						for (int // try a few times to get a tile to throw to.
								i = 0;
								i != 50;
								++i)
						{
							action.target = Position(
											unit->getPosition().x + RNG::generate(-20,20),
											unit->getPosition().y + RNG::generate(-20,20),
											unit->getPosition().z);

							if (_battleSave->getTile(action.target) != nullptr)
							{
								statePushBack(new UnitTurnBState(this, action, false));

								const Position
									originVoxel (_battleSave->getTileEngine()->getOriginVoxel(action)),
									targetVoxel (Position::toVoxelSpaceCentered(
																			action.target,
																			2 - _battleSave->getTile(action.target)->getTerrainLevel())); // LoFT of floor is typically 2 voxels thick.

								if (_battleSave->getTileEngine()->validateThrow(
																			action,
																			originVoxel,
																			targetVoxel) == true)
								{
									action.type = BA_THROW;
									action.cameraPosition = _battleSave->getBattleState()->getMap()->getCamera()->getMapOffset();
									statePushBack(new ProjectileFlyBState(this, action));
									break;
								}
							}
						}
					}
				}

				action.type = BA_NONE;
			}
		}

		statePushBack(new UnitPanicBState(this, unit));
		return true;
	}

	return false;
}

/**
  * Cancels the current action the Player has selected (firing, throwing, etc).
  * @note The return is used only by BattlescapeState::mapClick() to check if
  * pathPreview was cancelled or walking was aborted.
  * @param force - force the action to be cancelled (default false)
  * @return, true if anything was cancelled
  */
bool BattlescapeGame::cancelCurrentAction(bool force)
{
	if (_battleSave->getPathfinding()->removePreview() == false
		|| Options::battlePreviewPath == PATH_NONE)
	{
		if (_battleStates.empty() == true || force == true)
		{
			if (_currentAction.targeting == true)
			{
				if (_currentAction.type == BA_LAUNCH
					&& _currentAction.waypoints.empty() == false)
				{
					_currentAction.waypoints.pop_back();

					if (getMap()->getWaypoints()->empty() == false)
						getMap()->getWaypoints()->pop_back();

					if (_currentAction.waypoints.empty() == true)
						_parentState->showLaunchButton(false);
				}
				else
				{
					_currentAction.targeting = false;
					_currentAction.type = BA_NONE;

					if (force == false
						&& _battleSave->getSide() == FACTION_PLAYER) //|| _debugPlay == true
					{
						setupCursor();
						_parentState->getGame()->getCursor()->setHidden(false);
//						_parentState->getGame()->getCursor()->setVisible();
					}
				}
			}
			else
				return false;
		}
		else if (_battleStates.empty() == false && _battleStates.front() != nullptr)
			_battleStates.front()->cancel();
		else
			return false;
	}

	return true;
}

/**
 * Gets a pointer to access BattleAction struct members directly.
 * @note This appears to be for Player's units only.
 * @return, pointer to BattleAction
 */
BattleAction* BattlescapeGame::getCurrentAction()
{
	return &_currentAction;
}

/**
 * Determines whether an action is currently going on.
 * @return, true or false
 */
bool BattlescapeGame::isBusy() const
{
	return (_battleStates.empty() == false);
}

/**
 * Left click activates a primary action.
 * @param pos - reference a Position on the map
 */
void BattlescapeGame::primaryAction(const Position& pos)
{
	//Log(LOG_INFO) << "BattlescapeGame::primaryAction()";
	//if (_battleSave->getSelectedUnit()) Log(LOG_INFO) << ". ID " << _battleSave->getSelectedUnit()->getId();
	_currentAction.actor = _battleSave->getSelectedUnit();
	BattleUnit* const targetUnit = _battleSave->selectUnit(pos);

	if (_currentAction.actor != nullptr
		&& _currentAction.targeting == true)
	{
		//Log(LOG_INFO) << ". . _currentAction.targeting";
		_currentAction.strafe = false;

		if (_currentAction.type == BA_LAUNCH) // click to set BL waypoints.
		{
			if (static_cast<int>(_currentAction.waypoints.size()) < _currentAction.weapon->getRules()->isWaypoints())
			{
				//Log(LOG_INFO) << ". . . . BA_LAUNCH";
				_parentState->showLaunchButton();
				_currentAction.waypoints.push_back(pos);
				getMap()->getWaypoints()->push_back(pos);
			}
		}
		else if (_currentAction.type == BA_USE
			&& _currentAction.weapon->getRules()->getBattleType() == BT_MINDPROBE)
		{
			//Log(LOG_INFO) << ". . . . BA_USE -> BT_MINDPROBE";
			if (targetUnit != nullptr
				&& targetUnit->getFaction() != _currentAction.actor->getFaction()
				&& targetUnit->getUnitVisible() == true)
			{
				if (_currentAction.weapon->getRules()->isLosRequired() == false
					|| std::find(
							_currentAction.actor->getHostileUnits().begin(),
							_currentAction.actor->getHostileUnits().end(),
							targetUnit) != _currentAction.actor->getHostileUnits().end())
				{
					if (TileEngine::distance( // in Range
										_currentAction.actor->getPosition(),
										_currentAction.target) <= _currentAction.weapon->getRules()->getMaxRange())
					{
						if (_currentAction.actor->spendTimeUnits(_currentAction.TU) == true)
						{
							const int soundId = _currentAction.weapon->getRules()->getFireHitSound();
							if (soundId != -1)
								getResourcePack()->getSound("BATTLE.CAT", soundId)
													->play(-1, getMap()->getSoundAngle(pos));

							_parentState->getGame()->pushState(new UnitInfoState(
																			targetUnit,
																			_parentState,
																			false,true));
						}
						else
						{
							cancelCurrentAction();
							_parentState->warning("STR_NOT_ENOUGH_TIME_UNITS");
						}
					}
					else
						_parentState->warning("STR_OUT_OF_RANGE");
				}
				else
					_parentState->warning("STR_NO_LINE_OF_FIRE");
			}
		}
		else if (_currentAction.type == BA_PSIPANIC
			|| _currentAction.type == BA_PSICONTROL
			|| _currentAction.type == BA_PSICONFUSE
			|| _currentAction.type == BA_PSICOURAGE)
		{
			//Log(LOG_INFO) << ". . . . BA_PSIPANIC or BA_PSICONTROL or BA_PSICONFUSE or BA_PSICOURAGE";
			if (targetUnit != nullptr
				&& targetUnit->getUnitVisible() == true
				&& ((_currentAction.type != BA_PSICOURAGE
						&& targetUnit->getFaction() != FACTION_PLAYER)
					|| (_currentAction.type == BA_PSICOURAGE
						&& targetUnit->getFaction() != FACTION_HOSTILE)))
			{
				bool aLienPsi = (_currentAction.weapon == nullptr);
				if (aLienPsi == true)
					_currentAction.weapon = _alienPsi;

				_currentAction.target = pos;
				_currentAction.TU = _currentAction.actor->getActionTu(
																	_currentAction.type,
																	_currentAction.weapon);

				if (_currentAction.weapon->getRules()->isLosRequired() == false
					|| std::find(
							_currentAction.actor->getHostileUnits().begin(),
							_currentAction.actor->getHostileUnits().end(),
							targetUnit) != _currentAction.actor->getHostileUnits().end())
				{
					if (TileEngine::distance( // in Range
										_currentAction.actor->getPosition(),
										_currentAction.target) <= _currentAction.weapon->getRules()->getMaxRange())
					{
						// get the sound/animation started
//						getMap()->setCursorType(CT_NONE);
//						_parentState->getGame()->getCursor()->setVisible(false);
//						_currentAction.cameraPosition = getMap()->getCamera()->getMapOffset();
						_currentAction.cameraPosition = Position(0,0,-1); // don't adjust the camera when coming out of ProjectileFlyB/ExplosionB states


//						statePushBack(new PsiAttackBState(this, _currentAction));
						//Log(LOG_INFO) << ". . . . . . new ProjectileFlyBState";
						statePushBack(new ProjectileFlyBState(this, _currentAction));

						if (_currentAction.actor->getTimeUnits() >= _currentAction.TU) // WAIT, check this *before* all the stuff above!!!
						{
							if (getTileEngine()->psiAttack(&_currentAction) == true)
							{
								//Log(LOG_INFO) << ". . . . . . Psi successful";
								Game* const game = _parentState->getGame(); // show an infobox if successful

								std::wstring wst;
								if (_currentAction.type == BA_PSIPANIC)
									wst = game->getLanguage()->getString("STR_PSI_PANIC_SUCCESS")
																	.arg(_currentAction.value);
								else if (_currentAction.type == BA_PSICONTROL)
									wst = game->getLanguage()->getString("STR_PSI_CONTROL_SUCCESS")
																	.arg(_currentAction.value);
								else if (_currentAction.type == BA_PSICONFUSE)
									wst = game->getLanguage()->getString("STR_PSI_CONFUSE_SUCCESS")
																	.arg(_currentAction.value);
								else if (_currentAction.type == BA_PSICOURAGE)
									wst = game->getLanguage()->getString("STR_PSI_COURAGE_SUCCESS")
																	.arg(_currentAction.value);

								game->pushState(new InfoboxState(wst));


								//Log(LOG_INFO) << ". . . . . . updateSoldierInfo()";
								_parentState->updateSoldierInfo(false);
								//Log(LOG_INFO) << ". . . . . . updateSoldierInfo() DONE";


								// kL_begin: BattlescapeGame::primaryAction(), not sure where this bit came from.....
								// it doesn't seem to be in the official oXc but it might
								// stop some (psi-related) crashes i'm getting; (no, it was something else..),
								// but then it probably never runs because I doubt that selectedUnit can be other than xCom.
								// (yes, selectedUnit is currently operating unit of *any* faction)
								// BUT -> primaryAction() here is never called by the AI; only by Faction_Player ...
								// BUT <- it has to be, because this is how aLiens do their 'builtInPsi'
								//
								// ... it's for player using aLien to do Psi ...
								//
								// I could do a test Lol

//								if (_battleSave->getSelectedUnit()->getFaction() != FACTION_PLAYER)
//								{
//								_currentAction.targeting = false;
//								_currentAction.type = BA_NONE;
//								}
//								setupCursor();

//								getMap()->setCursorType(CT_NONE);							// kL
//								_parentState->getGame()->getCursor()->setVisible(false);	// kL
//								_parentState->getMap()->refreshSelectorPosition();			// kL
								// kL_end.

								//Log(LOG_INFO) << ". . . . . . inVisible cursor, DONE";
							}
						}
						else
						{
							cancelCurrentAction();
							_parentState->warning("STR_NOT_ENOUGH_TIME_UNITS");
						}
					}
					else
						_parentState->warning("STR_OUT_OF_RANGE");
				}
				else
					_parentState->warning("STR_NO_LINE_OF_FIRE");


				if (aLienPsi == true)
					_currentAction.weapon = nullptr;
			}
		}
		else
		{
			//Log(LOG_INFO) << ". . . . FIRING or THROWING";
			getMap()->setCursorType(CT_NONE);
			_parentState->getGame()->getCursor()->setHidden();

			_currentAction.target = pos;
			if (_currentAction.type == BA_THROW
				|| _currentAction.weapon->getAmmoItem() == nullptr
				|| _currentAction.weapon->getAmmoItem()->getRules()->getShotgunPellets() == 0)
			{
				_currentAction.cameraPosition = getMap()->getCamera()->getMapOffset();
			}
			else
				_currentAction.cameraPosition = Position(0,0,-1);

			_battleStates.push_back(new ProjectileFlyBState(this, _currentAction));	// TODO: should check for valid LoF/LoT *before* invoking this
																				// instead of the (flakey) checks in that state. Then conform w/ AI ...

			statePushFront(new UnitTurnBState(this, _currentAction));
			//Log(LOG_INFO) << ". . . . FIRING or THROWING done";
		}
	}
	else // select unit, or spin/ MOVE .......
	{
		//Log(LOG_INFO) << ". . NOT _currentAction.targeting";
		bool allowPreview = (Options::battlePreviewPath != PATH_NONE);

		if (targetUnit != nullptr // select unit
			&& targetUnit != _currentAction.actor
			&& (targetUnit->getUnitVisible() == true || _debugPlay == true))
		{
			if (targetUnit->getFaction() == _battleSave->getSide())
			{
				_battleSave->setSelectedUnit(targetUnit);
				_parentState->updateSoldierInfo();

				cancelCurrentAction();
				setupCursor();

				_currentAction.actor = targetUnit;
			}
		}
		else if (playableUnitSelected() == true) // spin or Move.
		{
			Pathfinding* const pf = _battleSave->getPathfinding();
			pf->setPathingUnit(_currentAction.actor);

			const bool
				ctrl = (SDL_GetModState() & KMOD_CTRL) != 0,
				alt = (SDL_GetModState() & KMOD_ALT) != 0;

			bool zPath;
			const Uint8* const keystate = SDL_GetKeyState(nullptr);
			if (keystate[SDLK_z] != 0)
				zPath = true;
			else
				zPath = false;

			if (targetUnit != nullptr // spin 180 degrees
				&& targetUnit == _currentAction.actor
				&& _currentAction.actor->getArmor()->getSize() == 1) // reasons: let click on large unit fallthrough to Move below_
			{
				if (ctrl == true
					&& (_currentAction.actor->getGeoscapeSoldier() != nullptr
						|| _currentAction.actor->getUnitRules()->isMechanical() == false))
				{
					if (allowPreview == true)
						pf->removePreview();

					Position screenPixel;
					getMap()->getCamera()->convertMapToScreen(pos, &screenPixel);
					screenPixel += getMap()->getCamera()->getMapOffset();

					Position mousePixel;
					getMap()->findMousePointer(mousePixel);

					if (mousePixel.x > screenPixel.x + 16)
						_currentAction.actor->setTurnDirection(-1);
					else
						_currentAction.actor->setTurnDirection(1);

					Pathfinding::directionToVector(
											(_currentAction.actor->getUnitDirection() + 4) % 8,
											&_currentAction.target);
					_currentAction.target += pos;

					statePushBack(new UnitTurnBState(this, _currentAction));
				}
			}
			else // handle pathPreview and MOVE ->
			{
				if (allowPreview == true
					&& (_currentAction.target != pos
						|| pf->isModCtrl() != ctrl
						|| pf->isModAlt() != alt
						|| pf->isZPath() != zPath))
				{
					pf->removePreview();
				}

				_currentAction.target = pos;
				pf->calculate(
						_currentAction.actor,
						_currentAction.target);

				if (pf->getStartDirection() != -1)
				{
					if (allowPreview == true
						&& pf->previewPath() == false)
					{
						pf->removePreview();
						allowPreview = false;
					}

					if (allowPreview == false)
					{
						getMap()->setCursorType(CT_NONE);
						_parentState->getGame()->getCursor()->setHidden();

						statePushBack(new UnitWalkBState(this, _currentAction));
					}
				}
			}
		}
	}
	//Log(LOG_INFO) << "BattlescapeGame::primaryAction() EXIT";
}

/**
 * Right click activates a secondary action.
 * @param pos - reference a Position on the map
 */
void BattlescapeGame::secondaryAction(const Position& pos)
{
	_currentAction.actor = _battleSave->getSelectedUnit();
	if (_currentAction.actor->getPosition() != pos)
	{
		_currentAction.target = pos;
		_currentAction.strafe = _currentAction.actor->getTurretType() != -1
							 && (SDL_GetModState() & KMOD_CTRL) != 0
							 && Options::battleStrafe == true;

		statePushBack(new UnitTurnBState(this, _currentAction)); // open door or rotate turret.
	}
	else
		_parentState->btnKneelClick(nullptr); // could put just about anything here Orelly.
}

/**
 * Handler for the blaster launcher button.
 */
void BattlescapeGame::launchAction()
{
	_parentState->showLaunchButton(false);

	getMap()->getWaypoints()->clear();
	_currentAction.target = _currentAction.waypoints.front();

	getMap()->setCursorType(CT_NONE);
	_parentState->getGame()->getCursor()->setHidden();

//	_currentAction.cameraPosition = getMap()->getCamera()->getMapOffset();

	_battleStates.push_back(new ProjectileFlyBState(this, _currentAction));
	statePushFront(new UnitTurnBState(this, _currentAction));
}

/**
 * Handler for the psi button.
 * @note Additional data gets assigned in primaryAction().
 */
void BattlescapeGame::psiButtonAction()
{
	_currentAction.weapon = nullptr;
	_currentAction.targeting = true;
	_currentAction.type = BA_PSIPANIC;

	setupCursor();
}

/**
 * Moves a unit up or down.
 * @note Used only by tactical icon buttons.
 * @param unit	- a unit
 * @param dir	- direction DIR_UP or DIR_DOWN
 */
void BattlescapeGame::moveUpDown(
		const BattleUnit* const unit,
		int dir)
{
	_currentAction.target = unit->getPosition();

	if (dir == Pathfinding::DIR_UP)
		++_currentAction.target.z;
	else
		--_currentAction.target.z;

	getMap()->setCursorType(CT_NONE);
	_parentState->getGame()->getCursor()->setHidden();

	Pathfinding* const pf = _battleSave->getPathfinding();
	pf->calculate(
				_currentAction.actor,
				_currentAction.target);

	statePushBack(new UnitWalkBState(this, _currentAction));
}

/**
 * Requests the end of the turn.
 * @note Waits for explosions etc to really end the turn.
 */
void BattlescapeGame::requestEndTurn()
{
	cancelCurrentAction();

	if (_endTurnRequested == false)
	{
		_endTurnRequested = true;
		statePushBack(nullptr);
	}
}

/**
 * Gets if an end-turn-request is waiting.
 *
bool BattlescapeGame::getEndTurnRequested() const
{
	return _endTurnRequested;
} */

/**
 * Drops an item to the floor and affects it with gravity then recalculates FoV
 * if it's a light-source.
 * @param pos		- reference position to place the item
 * @param item		- pointer to the item
 * @param create	- true if this is a new item (default false)
 * @param disown	- true to remove the item from the owner (default false)
 */
void BattlescapeGame::dropItem(
		const Position& pos,
		BattleItem* const item,
		bool created,
		bool disown)
{
	if (_battleSave->getTile(pos) != nullptr		// don't spawn anything outside of bounds
		&& item->getRules()->isFixed() == false)	// don't ever drop fixed items
	{
		_battleSave->getTile(pos)->addItem(
										item,
										getRuleset()->getInventoryRule(ST_GROUND));

		if (item->getUnit() != nullptr)
			item->getUnit()->setPosition(pos);

		if (created == true)
			_battleSave->getItems()->push_back(item);

		if (disown == true)
			item->changeOwner();
		else if (item->getRules()->isGrenade() == false)
			item->setOwner();

		getTileEngine()->applyGravity(_battleSave->getTile(pos));

		if (item->getRules()->getBattleType() == BT_FLARE)
		{
			getTileEngine()->calculateTerrainLighting();
			getTileEngine()->recalculateFOV(true);
		}
	}
}

/**
 * Converts a unit into a unit of another type.
 * @param unit - pointer to a unit to convert
 * @return, pointer to the new unit
 */
BattleUnit* BattlescapeGame::convertUnit(BattleUnit* const unit)
{
	//Log(LOG_INFO) << "BattlescapeGame::convertUnit() " << conType;
	const bool wasVisible = unit->getUnitVisible();

	_battleSave->getBattleState()->showPsiButton(false);
	_battleSave->removeCorpse(unit); // in case the unit was unconscious

	unit->instaKill();
	unit->setSpecialAbility(SPECAB_NONE);

	for (std::vector<BattleItem*>::const_iterator
			i = unit->getInventory()->begin();
			i != unit->getInventory()->end();
			++i)
	{
		dropItem(unit->getPosition(), *i);
		(*i)->setOwner();
	}

	unit->getInventory()->clear();

	unit->setTile(nullptr);
	_battleSave->getTile(unit->getPosition())->setUnit(nullptr);


	std::string st = unit->getSpawnUnit();
	RuleUnit* const unitRule = getRuleset()->getUnitRule(st);
	st = unitRule->getArmor();

	BattleUnit* const conUnit = new BattleUnit(
											unitRule,
											FACTION_HOSTILE,
											_battleSave->getUnits()->back()->getId() + 1,
											getRuleset()->getArmor(st),
											_parentState->getGame()->getSavedGame()->getDifficulty(),
											_parentState->getGame()->getSavedGame()->getMonthsPassed(),
											this);

	const Position posUnit = unit->getPosition();
	_battleSave->getTile(posUnit)->setUnit(
										conUnit,
										_battleSave->getTile(posUnit + Position(0,0,-1)));
	conUnit->setPosition(posUnit);
	conUnit->setTimeUnits(0);

	int dir;
	if (conUnit->isZombie() == true)
		dir = RNG::generate(0,7); // or, (unit->getUnitDirection())
	else
		dir = 3;
	conUnit->setUnitDirection(dir);

	_battleSave->getUnits()->push_back(conUnit);

	conUnit->setAIState(new AlienBAIState(_battleSave, conUnit));

	st = unitRule->getRace().substr(4) + "_WEAPON";
	BattleItem* const item = new BattleItem(
										getRuleset()->getItem(st),
										_battleSave->getNextItemId());
	item->changeOwner(conUnit);
	item->setInventorySection(getRuleset()->getInventoryRule(ST_RIGHTHAND));
	_battleSave->getItems()->push_back(item);

	getMap()->cacheUnit(conUnit);

	conUnit->setUnitVisible(wasVisible);

	getTileEngine()->applyGravity(conUnit->getTile());
//	getTileEngine()->calculateUnitLighting(); // <- done in UnitDieBState. But does pre-Spawned unit always go through UnitDieBState, and if not does it matter ...
	getTileEngine()->calculateFOV(conUnit->getPosition(), true);

	return conUnit;
}

/**
 * Gets the map.
 * @return, pointer to Map
 */
Map* BattlescapeGame::getMap() const
{
	return _parentState->getMap();
}

/**
 * Gets the battle game save data object.
 * @return, pointer to SavedBattleGame
 */
SavedBattleGame* BattlescapeGame::getBattleSave() const
{
	return _battleSave;
}

/**
 * Gets the tilengine.
 * @return, pointer to TileEngine
 */
TileEngine* BattlescapeGame::getTileEngine() const
{
	return _battleSave->getTileEngine();
}

/**
 * Gets the pathfinding.
 * @return, pointer to Pathfinding
 */
Pathfinding* BattlescapeGame::getPathfinding() const
{
	return _battleSave->getPathfinding();
}

/**
 * Gets the resourcepack.
 * @return, pointer to ResourcePack
 */
ResourcePack* BattlescapeGame::getResourcePack() const
{
	return _parentState->getGame()->getResourcePack();
}

/**
 * Gets the ruleset.
 * @return, pointer to Ruleset
 */
const Ruleset* BattlescapeGame::getRuleset() const
{
	return _parentState->getGame()->getRuleset();
}

/**
 * Tries to find an item and pick it up if possible.
 * @param action - pointer to the current BattleAction struct
 */
void BattlescapeGame::pickupItem(BattleAction* const action) const
{
	//Log(LOG_INFO) << "BattlescapeGame::findItem()";
	if (action->actor->getRankString() != "STR_LIVE_TERRORIST")
	{
		BattleItem* const targetItem = surveyItems(action->actor);
		if (targetItem != nullptr)
		{
			if (targetItem->getTile()->getPosition() == action->actor->getPosition())
			{
				if (takeItemFromGround(
									targetItem,
									action->actor) == 0
					&& targetItem->getAmmoItem() == nullptr)
				{
					action->actor->checkAmmo();
				}
			}
			else
			{
				action->target = targetItem->getTile()->getPosition();
				action->type = BA_MOVE;
			}
		}
	}
}

/**
 * Searches through items on the map that were dropped on an alien turn and
 * picks the most attractive one.
 * @param unit - pointer to the BattleUnit looking for an item
 * @return, the BattleItem to go for
 */
BattleItem* BattlescapeGame::surveyItems(BattleUnit* const unit) const
{
	//Log(LOG_INFO) << "BattlescapeGame::surveyItems()";
	const Tile* tile;
	std::vector<BattleItem*> groundItems;
	for (std::vector<BattleItem*>::const_iterator
			i = _battleSave->getItems()->begin();
			i != _battleSave->getItems()->end();
			++i)
	{
		if ((*i)->getInventorySection() != nullptr
			&& (*i)->getInventorySection()->getSectionType() == ST_GROUND
			&& (*i)->getRules()->getAttraction() != 0)
		{
			tile = (*i)->getTile();
			if (tile != nullptr
				&& (tile->getTileUnit() == nullptr
					|| tile->getTileUnit() == unit)
				&& tile->getTuCostTile(O_FLOOR, MT_WALK) != 255 // TODO:: pathfind.
				&& tile->getTuCostTile(O_OBJECT, MT_WALK) != 255
				&& worthTaking(*i, unit) == true)
			{
				groundItems.push_back(*i);
			}
		}
	}

	BattleItem* ret = nullptr;

	if (groundItems.empty() == false) // Select the most suitable candidate depending on attraction and distance.
	{
		const Position posUnit = unit->getPosition();
		int
			worth = 0,
			worthTest,
			dist;

		std::vector<BattleItem*> choiceItems;
		for (std::vector<BattleItem*>::const_iterator
				i = groundItems.begin();
				i != groundItems.end();
				++i)
		{
			dist = TileEngine::distance(
									posUnit,
									(*i)->getTile()->getPosition());
			worthTest = (*i)->getRules()->getAttraction() / (dist + 1);
			if (worthTest >= worth)
			{
				if (worthTest > worth)
				{
					worth = worthTest;
					choiceItems.clear();
				}

				choiceItems.push_back(*i);
			}
		}

		if (choiceItems.empty() == false)
			ret = choiceItems.at(RNG::pick(choiceItems.size()));
	}

	return ret;
}

/**
 * Assesses whether an item is worth trying to pick up.
 * @param item - pointer to a BattleItem to go for
 * @param unit - pointer to the BattleUnit looking for an item
 * @return, true if the item is worth going for
 */
bool BattlescapeGame::worthTaking( // TODO: rewrite & rework into rest of pickup code !
		BattleItem* const item,
		BattleUnit* const unit) const
{
	//Log(LOG_INFO) << "BattlescapeGame::worthTaking()";
	const int spaceReqd = item->getRules()->getInventoryHeight() * item->getRules()->getInventoryWidth();
	int spaceFree = 25; // note that my compiler throws a hissy-fit every time it sees this chunk of code ...
	for (std::vector<BattleItem*>::const_iterator
			i = unit->getInventory()->begin();
			i != unit->getInventory()->end();
			++i)
	{
		spaceFree -= (*i)->getRules()->getInventoryHeight() * (*i)->getRules()->getInventoryWidth();
		if (spaceFree < spaceReqd)
			return false;
	}


	bool ret = false;

	if (item->getRules()->getBattleType() == BT_AMMO)
	{
		for (std::vector<BattleItem*>::const_iterator
				i = unit->getInventory()->begin();
				i != unit->getInventory()->end()
					&& ret == false;
				++i)
		{
			if ((*i)->getRules()->getBattleType() == BT_FIREARM)
			{
				for (std::vector<std::string>::const_iterator
						j = (*i)->getRules()->getCompatibleAmmo()->begin();
						j != (*i)->getRules()->getCompatibleAmmo()->end()
							&& ret == false;
						++j)
				{
					if (*j == (*i)->getRules()->getName())
						ret = true;
				}
			}
		}
	}
	else if (item->getAmmoItem() == nullptr) // not loaded
	{
		for (std::vector<BattleItem*>::const_iterator
				i = unit->getInventory()->begin();
				i != unit->getInventory()->end()
					&& ret == false;
				++i)
		{
			if ((*i)->getRules()->getBattleType() == BT_AMMO)
			{
				for (std::vector<std::string>::const_iterator
						j = item->getRules()->getCompatibleAmmo()->begin();
						j != item->getRules()->getCompatibleAmmo()->end()
							&& ret == false;
						++j)
				{
					if (*j == (*i)->getRules()->getName())
						ret = true;
				}
			}
		}
	}

	// The problem here, in addition to the quirky space-calculation, is that
	// only weapons and ammo are considered worthwhile.
	return ret;
}

/**
 * Picks the item up from the ground.
 * @note At this point the unit has decided it's worthwhile to grab the item
 * so try to do just that. First check to make sure actor has time units then
 * that there is space (using horrifying logic!) attempt to actually recover
 * the item.
 * @param item - pointer to a BattleItem to go for
 * @param unit - pointer to the BattleUnit looking for an item
 * @return,	 0 - successful
 *			 1 - no-TUs
 *			 2 - not-enough-room
 *			 3 - won't-fit
 *			-1 - something-went-horribly-wrong
 */
int BattlescapeGame::takeItemFromGround(
		BattleItem* const item,
		BattleUnit* const unit) const
{
	//Log(LOG_INFO) << "BattlescapeGame::takeItemFromGround()";
	const int
		TAKE_SUCCESS	=  0,
		TAKE_NO_TU		=  1,
		TAKE_NO_SPACE	=  2,
		TAKE_NO_FIT		=  3;

	if (unit->getTimeUnits() < 6) // should replace that w/ Ground-to-Hand TU rule.
		return TAKE_NO_TU;
	else
	{
		const int spaceReqd = item->getRules()->getInventoryHeight() * item->getRules()->getInventoryWidth();
		int spaceFree = 25; // note that my compiler throws a hissy-fit every time it sees this chunk of code ...
		for (std::vector<BattleItem*>::const_iterator
				i = unit->getInventory()->begin();
				i != unit->getInventory()->end();
				++i)
		{
			spaceFree -= (*i)->getRules()->getInventoryHeight() * (*i)->getRules()->getInventoryWidth();
			if (spaceFree < spaceReqd)
				return TAKE_NO_SPACE;
		}

		if (takeItem(
					item,
					unit) == true)
		{
			unit->spendTimeUnits(6);
			item->getTile()->removeItem(item);

			return TAKE_SUCCESS;
		}

		return TAKE_NO_FIT;
	}
}

/**
 * Tries to fit an item into the unit's inventory.
 * @param item - pointer to a BattleItem to go for
 * @param unit - pointer to the BattleUnit looking for an item
 * @return, true if @a item was successfully retrieved
 */
bool BattlescapeGame::takeItem( // TODO: rewrite & rework into rest of pickup code !
		BattleItem* const item,
		BattleUnit* const unit) const
{
	//Log(LOG_INFO) << "BattlescapeGame::takeItem()";
	const RuleInventory
		* const rhRule = getRuleset()->getInventoryRule(ST_RIGHTHAND),
		* const lhRule = getRuleset()->getInventoryRule(ST_LEFTHAND);
	BattleItem
		* const rhWeapon = unit->getItem(ST_RIGHTHAND),
		* const lhWeapon = unit->getItem(ST_LEFTHAND);

	RuleItem* const itRule = item->getRules();

	int placed = 0;

	switch (itRule->getBattleType())
	{
		case BT_FIREARM:
		case BT_MELEE:
			if (rhWeapon == nullptr)
			{
				item->setInventorySection(rhRule);
				placed = 1;
				break;
			}

			if (lhWeapon == nullptr)
			{
				item->setInventorySection(lhRule);
				placed = 1;
				break;
			} // no break.
		case BT_AMMO:
			if (rhWeapon != nullptr
				&& rhWeapon->getAmmoItem() == nullptr
				&& rhWeapon->setAmmoItem(item) == 0)
			{
//				item->setInventorySection(rhRule);
				placed = 2;
				break;
			}

			if (lhWeapon != nullptr
				&& lhWeapon->getAmmoItem() == nullptr
				&& lhWeapon->setAmmoItem(item) == 0)
			{
//				item->setInventorySection(lhRule);
				placed = 2;
				break;
			} // no break.

		default:
		{
			std::vector<const RuleInventory*> inTypes;
			inTypes.push_back(getRuleset()->getInventoryRule(ST_BELT));
			inTypes.push_back(getRuleset()->getInventoryRule(ST_BACKPACK));

			for (std::vector<const RuleInventory*>::const_iterator
					i = inTypes.begin();
					i != inTypes.end() && placed == 0;
					++i)
			{
				for (std::vector<RuleSlot>::const_iterator
						j = (*i)->getSlots()->begin();
						j != (*i)->getSlots()->end() && placed == 0;
						++j)
				{
					if (Inventory::isOverlap(
										unit, item, *i,
										j->x, j->y) == false
						&& (*i)->fitItemInSlot(itRule, j->x, j->y) == true)
					{
						item->setInventorySection(*i);
						item->setSlotX(j->x);
						item->setSlotY(j->y);
						placed = 1;
					}
				}
			}
		}
	}

	switch (placed)
	{
		case 1:
			item->changeOwner(unit); // no break.
		case 2:
			return true;
	}

	return false;
}

/**
 * Tallies the living units in the game.
 * @param liveAliens	- reference in which to store the live alien tally
 * @param liveSoldiers	- reference in which to store the live XCom tally
 * @return, true if all aliens are dead or pacified independent of battleAllowPsionicCapture option
 */
bool BattlescapeGame::tallyUnits(
		int& liveAliens,
		int& liveSoldiers) const
{
	bool ret = true;

	liveSoldiers =
	liveAliens = 0;

	for (std::vector<BattleUnit*>::const_iterator
			j = _battleSave->getUnits()->begin();
			j != _battleSave->getUnits()->end();
			++j)
	{
//		if ((*j)->isOut() == false)
		if ((*j)->isOut_t(OUT_STAT) == false)
		{
			if ((*j)->getOriginalFaction() == FACTION_HOSTILE)
			{
				if ((*j)->getFaction() != FACTION_PLAYER
					|| Options::battleAllowPsionicCapture == false)
				{
					++liveAliens;
				}

				if ((*j)->getFaction() == FACTION_HOSTILE)
					ret = false;
			}
			else if ((*j)->getOriginalFaction() == FACTION_PLAYER)
			{
				if ((*j)->getFaction() == FACTION_PLAYER)
					++liveSoldiers;
				else
					++liveAliens;
			}
		}
	}

	//Log(LOG_INFO) << "bg:tallyUnits() ret = " << ret << "; Sol = " << liveSoldiers << "; aLi = " << liveAliens;
	return ret;
}

/*
 * Sets the TU reserved type as a BattleAction.
 * @param bat - a battleactiontype (BattlescapeGame.h)
 *
void BattlescapeGame::setReservedAction(BattleActionType bat)
{
	_battleSave->setBatReserved(bat);
} */

/*
 * Returns the action type that is reserved.
 * @return, the BattleActionType that is reserved
 *
BattleActionType BattlescapeGame::getReservedAction() const
{
	return _battleSave->getBatReserved();
} */

/*
 * Sets the kneel reservation setting.
 * @param reserved - true to reserve an extra 4 TUs to kneel
 *
void BattlescapeGame::setKneelReserved(bool reserved) const
{
	_battleSave->setKneelReserved(reserved);
} */

/*
 * Gets the kneel reservation setting.
 * @return, kneel reservation setting
 *
bool BattlescapeGame::getKneelReserved() const
{
	return _battleSave->getKneelReserved();
} */

/**
 * Checks if a unit has moved next to a proximity grenade.
 * @note Checks one tile around the unit in every direction. For a large unit
 * check around every tile the unit occupies.
 * @param unit - pointer to a BattleUnit
 * @return, true if a proximity grenade is triggered
 */
bool BattlescapeGame::checkProxyGrenades(BattleUnit* const unit)
{
	int unitSize = unit->getArmor()->getSize() - 1;
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
			for (int
					tx = -1;
					tx != 2;
					++tx)
			{
				for (int
						ty = -1;
						ty != 2;
						++ty)
				{
					Tile* const tile = _battleSave->getTile(unit->getPosition()
																   + Position( x, y, 0)
																   + Position(tx,ty, 0));
					if (tile != nullptr)
					{
						for (std::vector<BattleItem*>::const_iterator
								i = tile->getInventory()->begin();
								i != tile->getInventory()->end();
								++i)
						{
							if ((*i)->getRules()->getBattleType() == BT_PROXYGRENADE
								&& (*i)->getFuse() != -1)
							{
								int dir; // cred: animal310 - http://openxcom.org/bugs/openxcom/issues/765
								_battleSave->getPathfinding()->vectorToDirection(
																			Position(tx,ty,0),
																			dir);
								//Log(LOG_INFO) << "dir = " << dir;
								if (_battleSave->getPathfinding()->isBlockedPath(
																			_battleSave->getTile(unit->getPosition() + Position(x,y,0)),
																			dir,
																			unit) == false)	// kL try passing in OBJECT_SELF as a missile target to kludge for closed doors.
								{															// there *might* be a problem if the Proxy is on a non-walkable tile ....
									const Position pos = Position::toVoxelSpaceCentered(
																					tile->getPosition(),
																					-(tile->getTerrainLevel()));
									statePushNext(new ExplosionBState(
																	this, pos, *i,
																	(*i)->getPriorOwner()));
									_battleSave->removeItem(*i); // does/should this even be done (also done at end of ExplosionBState) -> causes a double-explosion if remarked here.

									unit->clearCache();
									getMap()->cacheUnit(unit);

									return true;
								}
							}
						}
					}
				}
			}
		}
	}

	return false;
}

/**
 * Cleans up all the deleted states.
 */
void BattlescapeGame::cleanupDeleted()
{
	for (std::list<BattleState*>::const_iterator
			i = _deleted.begin();
			i != _deleted.end();
			++i)
	{
		delete *i;
	}
	_deleted.clear();
}

/**
 * Gets the BattlescapeState.
 * @note For turning on/off the visUnits indicators from UnitWalk/TurnBStates.
 * @return, pointer to BattlescapeState
 */
BattlescapeState* BattlescapeGame::getBattlescapeState() const
{
	return _parentState;
}

/**
 * Gets the universal fist.
 * @return, the universal fist!!
 */
BattleItem* BattlescapeGame::getFist() const
{
	return _universalFist;
}

/**
 * Gets the universal alienPsi weapon.
 * @return, the alienPsi BattleItem
 */
BattleItem* BattlescapeGame::getAlienPsi() const
{
	return _alienPsi;
}

/**
 * Sets up a mission complete notification.
 */
void BattlescapeGame::objectiveDone()
{
	const Game* const game = _parentState->getGame();
	const AlienDeployment* const deployRule = game->getRuleset()->getDeployment(_battleSave->getTacticalType());
	if (deployRule != nullptr)
	{
		const std::string messagePop = deployRule->getObjectivePopup();
		if (messagePop.empty() == false)
			_infoboxQueue.push_back(new InfoboxOKState(game->getLanguage()->getString(messagePop)));
	}
}

/**
 * Gets if an execution is underway and needs animation.
 * @return, true if execute
 */
bool BattlescapeGame::getExecution() const
{
	return _executeProgress;
}

/**
 * Finishes an execution.
 */
void BattlescapeGame::endExecution()
{
	_executeProgress = false;
}

/**
 * Sets if a shotgun blast is underway and needs animation.
 * @param shotgun - true to shotgun (default true)
 */
void BattlescapeGame::setShotgun(bool shotgun)
{
	_shotgunProgress = shotgun;
}

/**
 * Gets if a shotgun blast is underway and needs animation.
 * @return, true if shotgun
 */
bool BattlescapeGame::getShotgun() const
{
	return _shotgunProgress;
}

}
