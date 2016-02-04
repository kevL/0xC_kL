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
#include "../Engine/Logger.h"
#include "../Engine/Options.h"
#include "../Engine/RNG.h"
#include "../Engine/Sound.h"

#include "../Interface/Cursor.h"
#include "../Interface/Text.h"

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

bool BattlescapeGame::_debugPlay; // static.


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

	cancelTacticalAction();
	checkForCasualties(nullptr, nullptr, true);

//	_tacAction.actor = nullptr;
//	_tacAction.type = BA_NONE;
//	_tacAction.targeting = false;
	_tacAction.clearAction();

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
		if (_battleSave->getSide() != FACTION_PLAYER) // it's a non-player turn - ALIENS or CIVILIANS
		{
			if (_debugPlay == false)
			{
				BattleUnit* const selUnit (_battleSave->getSelectedUnit());
				if (selUnit != nullptr)
				{
					_parentState->debugPrint(L"ai " + Text::intWide(selUnit->getId()));
					if (handlePanickingUnit(selUnit) == false)
					{
						//Log(LOG_INFO) << "BattlescapeGame::think() call handleUnitAI() " << selUnit->getId();
						handleUnitAI(selUnit);
					}
				}
				else if (_battleSave->selectNextFactionUnit(true, _AISecondMove) == nullptr) // find 1st AI-unit else endTurn
					endAiTurn();
			}
		}
		else // it's a player turn
		{
			if (_playerPanicHandled == false) // not all panicking units have been handled
			{
				//Log(LOG_INFO) << "bg:think() . panic Handled is FALSE";
				if ((_playerPanicHandled = handlePanickingPlayer()) == true)
				{
					//Log(LOG_INFO) << "bg:think() . panic Handled TRUE";
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
 * (WalkB, ProjectileFlyB, ExplosionB, etc.) at the moment that state
 * has finished the current BattleAction. Check the result of that BattleAction
 * here and do all the aftermath. The state is then popped off the list.
 */
void BattlescapeGame::popState()
{
	//Log(LOG_INFO) << "BattlescapeGame::popState() qtyStates = " << (int)_battleStates.size();
	//if (Options::traceAI) Log(LOG_INFO) << "BattlescapeGame::popState() #" << _AIActionCounter
	//<< " with " << (_battleSave->getSelectedUnit()? _battleSave->getSelectedUnit()->getTimeUnits(): -9999) << " TU";

	if (getMap()->getExplosions()->empty() == true) // explosions need to run fast after popping ProjectileFlyBState etc etc.
	{
		//Log(LOG_INFO) << "bg: popState() set interval = " << BattlescapeState::STATE_INTERVAL_STANDARD;
		setStateInterval(BattlescapeState::STATE_INTERVAL_STANDARD);
	}

	if (_battleStates.empty() == false)
	{
		//Log(LOG_INFO) << ". states NOT Empty";
		const BattleAction action (_battleStates.front()->getAction());

		bool actionFail;
		if (action.result.empty() == false // query the warning string message
			&& (_battleSave->getSide() == FACTION_PLAYER || _debugPlay == true)
			&& action.actor != nullptr
			&& action.actor->getFaction() == FACTION_PLAYER
			&& _playerPanicHandled == true)
		{
			//Log(LOG_INFO) << ". actionFail";
			actionFail = true;
			_parentState->warning(action.result);

			// remove action.Cursor if error.Message (eg, not enough TUs)
			if (action.result.compare("STR_NOT_ENOUGH_TIME_UNITS") == 0
				|| action.result.compare("STR_NO_AMMUNITION_LOADED") == 0
				|| action.result.compare("STR_NO_ROUNDS_LEFT") == 0)
			{
				switch (action.type)
				{
					case BA_LAUNCH:
						_tacAction.waypoints.clear(); // no break;
					case BA_THROW:
					case BA_SNAPSHOT:
					case BA_AIMEDSHOT:
					case BA_AUTOSHOT:
					case BA_PSIPANIC:
					case BA_PSICONFUSE:
					case BA_PSICOURAGE:
					case BA_PSICONTROL:
						cancelTacticalAction(true);
						break;

					case BA_USE:
						if (action.weapon->getRules()->getBattleType() == BT_MINDPROBE)
							cancelTacticalAction(true);
						break;
				}
			}
		}
		else
			actionFail = false;

		//Log(LOG_INFO) << ". move Front-state to _deleted.";
		_deleted.push_back(_battleStates.front());
		//Log(LOG_INFO) << ". states.Popfront";
		_battleStates.pop_front();


		if (action.actor != nullptr // handle the end of the acting unit's actions
			&& noActionsPending(action.actor) == true)
		{
			//Log(LOG_INFO) << ". noActionsPending for state actor";
			switch (action.actor->getFaction())
			{
				case FACTION_PLAYER:
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
					if (actionFail == false)
					{
						if (action.targeting == true
							&& _battleSave->getSelectedUnit() != nullptr)
						{
							//Log(LOG_INFO) << ". . ID " << action.actor->getId() << " currentTU = " << action.actor->getTimeUnits();
							action.actor->spendTimeUnits(action.TU);
							// kL_query: Does this happen **before** ReactionFire/getReactor()?
							// no. not for shooting, but for throwing it does; actually no it doesn't.
							//
							// wtf, now RF works fine. NO IT DOES NOT.
							//Log(LOG_INFO) << ". . ID " << action.actor->getId() << " currentTU = " << action.actor->getTimeUnits() << " spent TU = " << action.TU;
						}

						if (_battleSave->getSide() == FACTION_PLAYER) // is NOT reaction-fire
						{
							//Log(LOG_INFO) << ". side -> Faction_Player";
							// After throwing the cursor returns to default cursor;
							// after shooting it stays in targeting mode and the player
							// can shoot again in the same mode (autoshot/snap/aimed)
							// unless he/she/it is out of ammo and/or TUs.
							const int tuActor (action.actor->getTimeUnits());
							switch (action.type)
							{
								case BA_LAUNCH:
									_tacAction.waypoints.clear(); // no break;
								case BA_THROW:
									cancelTacticalAction(true);
									break;

								case BA_SNAPSHOT:
									//Log(LOG_INFO) << ". SnapShot, TU percent = " << (float)action.weapon->getRules()->getSnapTu();
									if (action.weapon->getAmmoItem() == nullptr
										|| tuActor < action.actor->getActionTu(
																		BA_SNAPSHOT,
																		action.weapon))
									{
										cancelTacticalAction(true);
									}
									break;

								case BA_AUTOSHOT:
									//Log(LOG_INFO) << ". AutoShot, TU percent = " << (float)action.weapon->getRules()->getAutoTu();
									if (action.weapon->getAmmoItem() == nullptr
										|| tuActor < action.actor->getActionTu(
																		BA_AUTOSHOT,
																		action.weapon))
									{
										cancelTacticalAction(true);
									}
									break;

								case BA_AIMEDSHOT:
									//Log(LOG_INFO) << ". AimedShot, TU percent = " << (float)action.weapon->getRules()->getAimedTu();
									if (action.weapon->getAmmoItem() == nullptr
										|| tuActor < action.actor->getActionTu(
																		BA_AIMEDSHOT,
																		action.weapon))
									{
										cancelTacticalAction(true);
									}
							}
						}
					}
					break;

				default: // action.actor is NOT Faction_Player
				case FACTION_HOSTILE:
				case FACTION_NEUTRAL:
					//Log(LOG_INFO) << ". action -> NOT Faction_Player";
					action.actor->spendTimeUnits(action.TU);

					if (_battleSave->getSide() != FACTION_PLAYER && _debugPlay == false)
					{
						BattleUnit* selUnit = _battleSave->getSelectedUnit();
						if (_AIActionCounter > 2	// AI does three things per unit before switching to the
							|| selUnit == nullptr	// next or it got killed before doing the second thing
							|| selUnit->isOut_t() == true)
						{
							_AIActionCounter = 0;

							if (selUnit != nullptr)
							{
								selUnit->clearCache();
								getMap()->cacheUnit(selUnit);
							}

							if (_battleStates.empty() == true // nothing left for Actor to do
								&& _battleSave->selectNextFactionUnit(true) == nullptr)
							{
								endAiTurn();
							}

							if ((selUnit = _battleSave->getSelectedUnit()) != nullptr)
							{
								getMap()->getCamera()->centerOnPosition(selUnit->getPosition());
								if (_battleSave->getDebugTac() == true)
								{
									_parentState->refreshMousePosition();
									setupSelector();
								}
							}
						}
					}
					else if (_debugPlay == true)
					{
						setupSelector();
						_parentState->getGame()->getCursor()->setHidden(false);
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


		if (_battleSave->getSelectedUnit() == nullptr // selected unit died or became unconscious or disappeared inexplicably
			|| _battleSave->getSelectedUnit()->isOut_t() == true)
		{
			//Log(LOG_INFO) << ". unit incapacitated: cancelAction & deSelect)";
			cancelTacticalAction();	// note that this *will* setupSelector() under certain
			setupSelector();		// circumstances - eg, if current action was targetting.
			_battleSave->setSelectedUnit(nullptr);
		}

		if (_battleSave->getSide() == FACTION_PLAYER || _debugPlay == true)
		{
			//Log(LOG_INFO) << ". updateSoldierInfo()";
			_parentState->updateSoldierInfo(); // although calcFoV ought have been done by now ...
		}
	}

	if (_battleStates.empty() == true)
	{
		_battleSave->getTileEngine()->getRfShooterPositions()->clear();

		if (_battleSave->getRfTriggerPosition().z != -1) // refocus the Camera back onto RF trigger-unit after a brief delay
		{
			SDL_Delay(336);
			//Log(LOG_INFO) << "popState - STATES EMPTY - set Camera to triggerPos & clear triggerPos";
			getMap()->getCamera()->setMapOffset(_battleSave->getRfTriggerPosition());
			_battleSave->cacheRfTriggerPosition(Position(0,0,-1));
		}

		if (_battleSave->getSide() == FACTION_PLAYER || _debugPlay == true)
		{
			//Log(LOG_INFO) << ". states Empty, re-enable cursor";
			setupSelector();
			_parentState->getGame()->getCursor()->setHidden(false);
			_parentState->refreshMousePosition(); // update tile data on the HUD
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
	//Log(LOG_INFO) << "BattlescapeGame::centerOnUnit() id-" << unit->getId();
	if (unit->getUnitVisible() == true)
	{
		//Log(LOG_INFO) << ". . curUnit VISIBLE - walkUnit SET curUnit";
		_battleSave->setWalkUnit(unit);
	}
	else
	{
		//Log(LOG_INFO) << ". . curUnit NOT Visible - walkUnit Set NULL";
		_battleSave->setWalkUnit(nullptr);
	}

	//Log(LOG_INFO) << ". Center on id-" << unit->getId();
	getMap()->getCamera()->centerOnPosition(unit->getPosition(), draw);
}

/**
 * Handles the processing of the AI states of a unit.
 * @param unit - pointer to a BattleUnit
 */
void BattlescapeGame::handleUnitAI(BattleUnit* const unit)
{
	if (unit != _battleSave->getWalkUnit())
		centerOnUnit(unit); // if you're going to reveal the map at least center the first aLien.

	if (unit->getTimeUnits() == 0)
		unit->dontReselect();

	if (_AIActionCounter > 1
		|| unit->reselectAllowed() == false)
	{
		selectNextAiUnit(unit);
		return;
	}


	unit->setUnitVisible(false);

	_battleSave->getTileEngine()->calculateFOV(unit->getPosition());

	if (unit->getAIState() == nullptr)
	{
		if (unit->getFaction() == FACTION_HOSTILE)
			unit->setAIState(new AlienBAIState(_battleSave, unit, nullptr));
		else
			unit->setAIState(new CivilianBAIState(_battleSave, unit, nullptr));
	}

	if (++_AIActionCounter == 1)
	{
		_playedAggroSound = false;
		unit->setHiding(false);
	}

	BattleAction action;
	action.actor = unit;
	action.AIcount = _AIActionCounter;
	Log(LOG_INFO) << "";
	Log(LOG_INFO) << "BATTLESCAPE::handleUnitAI id-" << unit->getId();
	unit->think(&action);
	Log(LOG_INFO) << "BATTLESCAPE: id-" << unit->getId() << " bat [1] " << action.debugActionType(action.type);

	if (action.type == BA_RETHINK)
	{
		Log(LOG_INFO) << "";
		Log(LOG_INFO) << "BATTLESCAPE: Re-Think id-" << unit->getId();
		unit->think(&action);
		Log(LOG_INFO) << "BATTLESCAPE: id-" << unit->getId() << " bat [2] " << action.debugActionType(action.type);
	}

	_AIActionCounter = action.AIcount;

	if (unit->getFaction() == FACTION_HOSTILE
		&& unit->getMainHandWeapon(false) == nullptr
		&& unit->getRankString() != "STR_LIVE_TERRORIST"
		&& pickupItem(&action) == true
		&& _battleSave->getDebugTac() == true) // <- order matters.
	{
		_parentState->updateSoldierInfo();
	}

	if (_playedAggroSound == false
		&& unit->getChargeTarget() != nullptr)
	{
		_playedAggroSound = true;

		const int soundId (unit->getAggroSound());
		if (soundId != -1)
			getResourcePack()->getSound("BATTLE.CAT", soundId)
								->play(-1, getMap()->getSoundAngle(unit->getPosition()));
	}

	switch (action.type)
	{
		case BA_MOVE:
		{
			Pathfinding* const pf (_battleSave->getPathfinding());
			pf->setPathingUnit(action.actor);

			if (_battleSave->getTile(action.target) != nullptr)
				pf->calculate(action.actor, action.target);

			if (pf->getStartDirection() != -1)
				statePushBack(new UnitWalkBState(this, action));

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
			switch (action.type)
			{
				case BA_PSIPANIC:
				case BA_PSICONTROL:
					action.weapon = _alienPsi;
					action.TU = unit->getActionTu(action.type, action.weapon);
					break;

				default:
					statePushBack(new UnitTurnBState(this, action));
					switch (action.type)
					{
						case BA_MELEE:
						{
							const std::string meleeWeapon (unit->getMeleeWeapon());
							bool instaWeapon (false);

							if (action.weapon != _universalFist
								&& meleeWeapon.empty() == false)
							{
								bool found (false);
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

			//Log(LOG_INFO) << ". attack action.Type = " << action.type
			//				<< ", action.Target = " << action.target
			//				<< " action.Weapon = " << action.weapon->getRules()->getName().c_str();
			statePushBack(new ProjectileFlyBState(this, action));

			switch (action.type)
			{
				case BA_PSIPANIC:
				case BA_PSICONTROL:
					if (_battleSave->getTileEngine()->psiAttack(&action) == true)
					{
						const BattleUnit* const psiVictim (_battleSave->getTile(action.target)->getTileUnit());
						Language* const lang (_parentState->getGame()->getLanguage());
						std::wstring wst;
						switch (action.type)
						{
							default:
							case BA_PSIPANIC:
								wst = lang->getString("STR_PSI_PANIC_SUCCESS")
														.arg(action.value);
								break;
							case BA_PSICONTROL:
								wst = lang->getString("STR_IS_UNDER_ALIEN_CONTROL", psiVictim->getGender())
														.arg(psiVictim->getName(lang))
														.arg(action.value);
						}
						_parentState->getGame()->pushState(new InfoboxState(wst));
					}
			}
			break;

		default:
		case BA_NONE:
			selectNextAiUnit(unit);
	}
}

/**
 * Selects the next AI unit.
 * @note The current AI-turn ends when no unit is eligible to select.
 * @param unit - the current unit
 */
void BattlescapeGame::selectNextAiUnit(const BattleUnit* const unit) // private.
{
	_AIActionCounter = 0;

	if (_battleSave->selectNextFactionUnit(true, _AISecondMove) == nullptr)
		endAiTurn();

	const BattleUnit* const nextUnit (_battleSave->getSelectedUnit());
	if (nextUnit != nullptr)
	{
		centerOnUnit(nextUnit);
		_parentState->updateSoldierInfo();

		if (_battleSave->getDebugTac() == true)
		{
			_parentState->refreshMousePosition();
			setupSelector();
		}

		if (_AISecondMove == false)
		{
			if (std::find(
					_battleSave->getShuffleUnits()->begin(),
					_battleSave->getShuffleUnits()->end(),
					nextUnit)
						 -
				std::find(
					_battleSave->getShuffleUnits()->begin(),
					_battleSave->getShuffleUnits()->end(),
					unit) < 1)
			{
				_AISecondMove = true;
			}
		}
	}
}

/**
 * Ends the AI turn.
 */
void BattlescapeGame::endAiTurn()
{
	if (_battleSave->getDebugTac() == false)
	{
		_endTurnRequested = true;
		statePushBack(nullptr);
	}
	else
	{
		_battleSave->selectNextFactionUnit();
		_debugPlay = true;
	}
}

/**
 * Handles the result of non target actions like priming a grenade or performing
 * a melee attack or using a medikit.
 * @note The action is set up in ActionMenuState.
 */
void BattlescapeGame::handleNonTargetAction()
{
	if (_tacAction.targeting == false)
	{
		_tacAction.cameraPosition = Position(0,0,-1);

		int showWarning = 0;

		// NOTE: These actions are done partly in ActionMenuState::btnActionMenuClick() and
		// this subsequently handles a greater or lesser proportion of the resultant niceties.
		//
		switch (_tacAction.type)
		{
			case BA_PRIME:
			case BA_DEFUSE:
				if (_tacAction.actor->spendTimeUnits(_tacAction.TU) == false)
				{
					_tacAction.result = "STR_NOT_ENOUGH_TIME_UNITS";
					showWarning = 1;
				}
				else
				{
					_tacAction.weapon->setFuse(_tacAction.value);

					if (_tacAction.value == -1)
					{
						_tacAction.result = "STR_GRENADE_IS_DEACTIVATED";
						showWarning = 1;
					}
					else if (_tacAction.value == 0)
					{
						_tacAction.result = "STR_GRENADE_IS_ACTIVATED";
						showWarning = 1;
					}
					else
					{
						_tacAction.result = "STR_GRENADE_IS_ACTIVATED_";
						showWarning = 2;
					}
				}
			break;

			case BA_USE:
				if (_tacAction.result.empty() == false)
					showWarning = 1;
				else if (_tacAction.targetUnit != nullptr)
				{
					_battleSave->reviveUnit(_tacAction.targetUnit);
					_tacAction.targetUnit = nullptr;
				}
			break;

			case BA_LAUNCH:
				if (_tacAction.result.empty() == false)
					showWarning = 1;
			break;

			case BA_MELEE:
				if (_tacAction.result.empty() == false)
					showWarning = 1;
				else if (_tacAction.actor->spendTimeUnits(_tacAction.TU) == false)
				{
					_tacAction.result = "STR_NOT_ENOUGH_TIME_UNITS";
					showWarning = 1;
				}
				else
				{
					statePushBack(new ProjectileFlyBState(this, _tacAction));
					return;
				}
			break;

			case BA_DROP:
				if (_tacAction.result.empty() == false)
					showWarning = 1;
				else
				{
					_battleSave->getTileEngine()->applyGravity(_tacAction.actor->getTile());
					getResourcePack()->getSound("BATTLE.CAT", ResourcePack::ITEM_DROP)
										->play(-1, getMap()->getSoundAngle(_tacAction.actor->getPosition()));
				}
			break;

			case BA_EXECUTE:
				if (_tacAction.result.empty() == false)
					showWarning = 1;
				else if (_tacAction.targetUnit != nullptr)
					executeUnit();
		}

		if (showWarning != 0)
		{
			if (showWarning == 1)
				_parentState->warning(_tacAction.result);
			else if (showWarning == 2)
				_parentState->warning(
									_tacAction.result,
									true,
									_tacAction.value);

			_tacAction.result.clear();
		}

		_tacAction.type = BA_NONE;
		_parentState->updateSoldierInfo();
	}

	setupSelector();
}

/**
 * Summary execution.
 */
void BattlescapeGame::executeUnit() // private.
{
	_tacAction.actor->aim();
	_tacAction.actor->clearCache();
	getMap()->cacheUnit(_tacAction.actor);

	const RuleItem* const itRule = _tacAction.weapon->getRules();
	BattleItem* const ammo = _tacAction.weapon->getAmmoItem();
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
							->play(-1, getMap()->getSoundAngle(_tacAction.actor->getPosition()));

	ammo->spendBullet(
				*_battleSave,
				*_tacAction.weapon);

	Position explVoxel = Position::toVoxelSpaceCentered(_tacAction.target, 2);
	Explosion* const explosion = new Explosion(
											explVoxel,
											aniStart,
											0,
											false,
											isMelee);
	getMap()->getExplosions()->push_back(explosion);
	_executeProgress = true;

	_tacAction.targetUnit->playDeathSound(); // scream little piggie

	_tacAction.actor->spendTimeUnits(_tacAction.TU);

	_tacAction.targetUnit->setHealth(0);
	_tacAction.targetUnit = nullptr;

	checkForCasualties(
				_tacAction.weapon,
				_tacAction.actor,
				false, false, true);
}

/**
 * Sets the selector according to the current action.
 */
void BattlescapeGame::setupSelector()
{
	getMap()->refreshSelectorPosition();

	SelectorType type;
	int quads = 1;

	if (_tacAction.targeting == true)
	{
		switch (_tacAction.type)
		{
			case BA_THROW:
				type = CT_THROW;
				break;

			case BA_PSICONTROL:
			case BA_PSIPANIC:
			case BA_PSICONFUSE:
			case BA_PSICOURAGE:
			case BA_USE:
				type = CT_PSI;
				break;

			case BA_LAUNCH:
				type = CT_WAYPOINT;
				break;

			default:
				type = CT_AIM;
		}
	}
	else
	{
		type = CT_NORMAL;

		if ((_tacAction.actor = _battleSave->getSelectedUnit()) != nullptr)
			quads = _tacAction.actor->getArmor()->getSize();
	}

	getMap()->setSelectorType(type, quads);
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
			|| _battleSave->getDebugTac() == true); // should be (_debugPlay=TRUE)
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
	else
		_parentState->warning("STR_ACTION_NOT_ALLOWED_ALIEN"); // TODO: change to "not a Soldier, can't kneel".

	return false;
}

/**
 * Ends the turn.
 * @note This starts the switchover from one faction to the next.
 */
void BattlescapeGame::endTurn() // private.
{
	//Log(LOG_INFO) << "bg::endTurn()";
	_debugPlay =
	_AISecondMove = false;
	_parentState->showLaunchButton(false);

	_tacAction.targeting = false;
	_tacAction.type = BA_NONE;
	_tacAction.waypoints.clear();
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
			if ((tile = (*i)->getTile()) != nullptr
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


	checkForCasualties();

	int // if all units from either faction are killed - the mission is over.
		liveHostile,
		livePlayer;
	const bool hostilesPacified = tallyUnits(
										liveHostile,
										livePlayer);

	if (_battleSave->getObjectiveType() == MUST_DESTROY // brain death, end Final Mission.
		&& _battleSave->allObjectivesDestroyed() == true)
	{
		_parentState->finishBattle(false, livePlayer);
		return;
	}

	if (_battleSave->getTurnLimit() != 0
		&& _battleSave->getTurnLimit() < _battleSave->getTurn())
	{
		switch (_battleSave->getChronoResult())
		{
			case FORCE_WIN:
				_parentState->finishBattle(false, livePlayer);
				break;

			default:
			case FORCE_LOSE:
				_battleSave->setAborted();
				_parentState->finishBattle(true, 0);
				break;

			case FORCE_ABORT:
				_battleSave->setAborted();
				_parentState->finishBattle(true, tallyPlayerExit());
		}
		return;
	}

	const bool battleComplete = liveHostile == 0
							 || livePlayer == 0;

	if (battleComplete == false)
	{
		showInfoBoxQueue();
		_parentState->updateSoldierInfo();

		if (_battleSave->getSide() == FACTION_HOSTILE
			&& _battleSave->getDebugTac() == false)
		{
			getMap()->setSelectorType(CT_NONE);
			_battleSave->getBattleState()->toggleIcons(false);
		}
		else
		{
			setupSelector();
			if (playableUnitSelected() == true)
				centerOnUnit(_battleSave->getSelectedUnit());

			if (_battleSave->getDebugTac() == false)
				_battleSave->getBattleState()->toggleIcons(true);
		}

		if (hostilesPacified == true)
			_battleSave->setPacified();
	}

	if (_endTurnRequested == true)
	{
		_endTurnRequested = false;

		if (_battleSave->getSide() != FACTION_NEUTRAL
			|| battleComplete == true)
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
			killStatWeaponAmmo = weapon->getRules()->getType();
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
					killStatWeapon = itRule->getType();
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
					killStatWeapon = itRule->getType();
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
						&& attacker->getGeoscapeSoldier() != nullptr
						&& defender->beenStunned() == false) // credit first stunner only.
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

	if (_battleSave->getSide() == FACTION_HOSTILE && _debugPlay == false)
	{
		const AlienBAIState* const ai (dynamic_cast<AlienBAIState*>(unit->getAIState()));
		if (ai != nullptr)
			batReserved = ai->getReservedAiAction();
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
	const UnitStatus status (unit->getUnitStatus());

	if (status == STATUS_PANICKING || status == STATUS_BERSERK)
	{
		_parentState->getMap()->setSelectorType(CT_NONE);
		_battleSave->setSelectedUnit(unit);

		if (Options::battleAlienPanicMessages == true
			|| unit->getUnitVisible() == true)
		{
			//Log(LOG_INFO) << "bg: panic id-" << unit->getId();
			centerOnUnit(unit, true);

			Game* const game (_parentState->getGame());
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

				Pathfinding* const pf (_battleSave->getPathfinding());
				pf->setPathingUnit(unit);

				const std::vector<size_t> reachable (pf->findReachable(unit, tu));
				const size_t tileId (reachable[RNG::pick(reachable.size())]); // <-- WARNING: no Safety on size !

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

				action.weapon = unit->getMainHandWeapon(true);
				if (action.weapon == nullptr)
					action.weapon = unit->getGrenade();

				// TODO: run up to another unit and slug it with the Universal Fist.
				// Or w/ an already-equipped melee weapon

				if (action.weapon != nullptr)
				{
					switch (action.weapon->getRules()->getBattleType())
					{
						case BT_FIREARM:
							action.target = _battleSave->getTiles()[RNG::pick(_battleSave->getMapSizeXYZ())]->getPosition();
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
							break;

						case BT_GRENADE:
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
bool BattlescapeGame::cancelTacticalAction(bool force)
{
	if (_battleSave->getPathfinding()->clearPreview() == false
		|| Options::battlePreviewPath == PATH_NONE)
	{
		if (_battleStates.empty() == true || force == true)
		{
			if (_tacAction.targeting == true)
			{
				if (_tacAction.type == BA_LAUNCH
					&& _tacAction.waypoints.empty() == false)
				{
					_tacAction.waypoints.pop_back();

					if (getMap()->getWaypoints()->empty() == false)
						getMap()->getWaypoints()->pop_back();

					if (_tacAction.waypoints.empty() == true)
						_parentState->showLaunchButton(false);
				}
				else
				{
					_tacAction.targeting = false;
					_tacAction.type = BA_NONE;

					if (force == false
						&& (_battleSave->getSide() == FACTION_PLAYER || _debugPlay == true))
					{
						setupSelector();
						_parentState->getGame()->getCursor()->setHidden(false);
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
	return &_tacAction;
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
	_tacAction.actor = _battleSave->getSelectedUnit();
	BattleUnit* const targetUnit (_battleSave->selectUnit(pos));

	if (_tacAction.actor != nullptr
		&& _tacAction.targeting == true)
	{
		_tacAction.strafe = false;

		switch (_tacAction.type)
		{
			case BA_LAUNCH:
				if (static_cast<int>(_tacAction.waypoints.size()) < _tacAction.weapon->getRules()->isWaypoints())
				{
					_parentState->showLaunchButton();
					_tacAction.waypoints.push_back(pos);
					getMap()->getWaypoints()->push_back(pos);
				}
				break;

			case BA_USE:
				if (_tacAction.weapon->getRules()->getBattleType() == BT_MINDPROBE)
				{
					if (targetUnit != nullptr
						&& targetUnit->getFaction() != _tacAction.actor->getFaction()
						&& targetUnit->getUnitVisible() == true)
					{
						if (_tacAction.weapon->getRules()->isLosRequired() == false
							|| std::find(
									_tacAction.actor->getHostileUnits().begin(),
									_tacAction.actor->getHostileUnits().end(),
									targetUnit) != _tacAction.actor->getHostileUnits().end())
						{
							if (TileEngine::distance(
												_tacAction.actor->getPosition(),
												_tacAction.target) <= _tacAction.weapon->getRules()->getMaxRange())
							{
								if (_tacAction.actor->spendTimeUnits(_tacAction.TU) == true)
								{
									const int soundId = _tacAction.weapon->getRules()->getFireHitSound();
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
									cancelTacticalAction();
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
				break;

			case BA_PSIPANIC:
			case BA_PSICONTROL:
			case BA_PSICONFUSE:
			case BA_PSICOURAGE:
				if (targetUnit != nullptr
					&& targetUnit->getUnitVisible() == true
					&& ((_tacAction.type != BA_PSICOURAGE
							&& targetUnit->getFaction() != FACTION_PLAYER)
						|| (_tacAction.type == BA_PSICOURAGE
							&& targetUnit->getFaction() != FACTION_HOSTILE)))
				{
					bool aLienPsi = (_tacAction.weapon == nullptr);
					if (aLienPsi == true)
						_tacAction.weapon = _alienPsi;

					_tacAction.target = pos;
					_tacAction.TU = _tacAction.actor->getActionTu(
																		_tacAction.type,
																		_tacAction.weapon);

					if (_tacAction.weapon->getRules()->isLosRequired() == false
						|| std::find(
								_tacAction.actor->getHostileUnits().begin(),
								_tacAction.actor->getHostileUnits().end(),
								targetUnit) != _tacAction.actor->getHostileUnits().end())
					{
						if (TileEngine::distance(
											_tacAction.actor->getPosition(),
											_tacAction.target) <= _tacAction.weapon->getRules()->getMaxRange())
						{
							_tacAction.cameraPosition = Position(0,0,-1);

							statePushBack(new ProjectileFlyBState(this, _tacAction));

							if (_tacAction.actor->getTimeUnits() >= _tacAction.TU) // WAIT, check this *before* all the stuff above!!!
							{
								if (getTileEngine()->psiAttack(&_tacAction) == true)
								{
									Game* const game (_parentState->getGame());
									std::wstring wst;
									switch (_tacAction.type)
									{
										default:
										case BA_PSIPANIC:
											wst = game->getLanguage()->getString("STR_PSI_PANIC_SUCCESS")
																			.arg(_tacAction.value);
											break;
										case BA_PSICONTROL:
											wst = game->getLanguage()->getString("STR_PSI_CONTROL_SUCCESS")
																			.arg(_tacAction.value);
											break;
										case BA_PSICONFUSE:
											wst = game->getLanguage()->getString("STR_PSI_CONFUSE_SUCCESS")
																			.arg(_tacAction.value);
											break;
										case BA_PSICOURAGE:
											wst = game->getLanguage()->getString("STR_PSI_COURAGE_SUCCESS")
																			.arg(_tacAction.value);
									}
									game->pushState(new InfoboxState(wst));

									_parentState->updateSoldierInfo(false);
								}
							}
							else
							{
								cancelTacticalAction();
								_parentState->warning("STR_NOT_ENOUGH_TIME_UNITS");
							}
						}
						else
							_parentState->warning("STR_OUT_OF_RANGE");
					}
					else
						_parentState->warning("STR_NO_LINE_OF_FIRE");


					if (aLienPsi == true)
						_tacAction.weapon = nullptr;
				}
				break;

			default:
				getMap()->setSelectorType(CT_NONE);
				_parentState->getGame()->getCursor()->setHidden();

				_tacAction.target = pos;
				if (_tacAction.type == BA_THROW
					|| _tacAction.weapon->getAmmoItem() == nullptr
					|| _tacAction.weapon->getAmmoItem()->getRules()->getShotgunPellets() == 0)
				{
					_tacAction.cameraPosition = getMap()->getCamera()->getMapOffset();
				}
				else
					_tacAction.cameraPosition = Position(0,0,-1);

				_battleStates.push_back(new ProjectileFlyBState(this, _tacAction));	// TODO: should check for valid LoF/LoT *before* invoking this
																						// instead of the (flakey) checks in that state. Then conform w/ AI ...
				statePushFront(new UnitTurnBState(this, _tacAction));
		}
	}
	else
	{
		bool allowPreview = (Options::battlePreviewPath != PATH_NONE);

		if (targetUnit != nullptr
			&& targetUnit != _tacAction.actor
			&& (targetUnit->getUnitVisible() == true || _debugPlay == true))
		{
			if (targetUnit->getFaction() == _battleSave->getSide())
			{
				_battleSave->setSelectedUnit(targetUnit);
				_parentState->updateSoldierInfo();

				cancelTacticalAction();
				setupSelector();

				_tacAction.actor = targetUnit;
			}
		}
		else if (playableUnitSelected() == true)
		{
			Pathfinding* const pf (_battleSave->getPathfinding());
			pf->setPathingUnit(_tacAction.actor);

			const bool
				ctrl ((SDL_GetModState() & KMOD_CTRL) != 0),
				alt  ((SDL_GetModState() & KMOD_ALT)  != 0);

			bool zPath;
			const Uint8* const keystate (SDL_GetKeyState(nullptr));
			if (keystate[SDLK_z] != 0)
				zPath = true;
			else
				zPath = false;

			if (targetUnit != nullptr
				&& targetUnit == _tacAction.actor
				&& _tacAction.actor->getArmor()->getSize() == 1)
			{
				if (ctrl == true
					&& (_tacAction.actor->getGeoscapeSoldier() != nullptr
						|| _tacAction.actor->getUnitRules()->isMechanical() == false))
				{
					if (allowPreview == true)
						pf->clearPreview();

					Position screenPixel;
					getMap()->getCamera()->convertMapToScreen(pos, &screenPixel);
					screenPixel += getMap()->getCamera()->getMapOffset();

					Position mousePixel;
					getMap()->findMousePointer(mousePixel);

					if (mousePixel.x > screenPixel.x + 16)
						_tacAction.actor->setTurnDirection(-1);
					else
						_tacAction.actor->setTurnDirection(1);

					Pathfinding::directionToVector(
											(_tacAction.actor->getUnitDirection() + 4) % 8,
											&_tacAction.target);
					_tacAction.target += pos;

					statePushBack(new UnitTurnBState(this, _tacAction));
				}
			}
			else
			{
				if (allowPreview == true
					&& (_tacAction.target != pos
						|| pf->isModCtrl() != ctrl
						|| pf->isModAlt() != alt
						|| pf->isZPath() != zPath))
				{
					pf->clearPreview();
				}

				_tacAction.target = pos;
				pf->calculate(
						_tacAction.actor,
						_tacAction.target);

				if (pf->getStartDirection() != -1)
				{
					if (allowPreview == true
						&& pf->previewPath() == false)
					{
						pf->clearPreview();
						allowPreview = false;
					}

					if (allowPreview == false)
					{
						getMap()->setSelectorType(CT_NONE);
						_parentState->getGame()->getCursor()->setHidden();

						statePushBack(new UnitWalkBState(this, _tacAction));
					}
				}
			}
		}
	}
}

/**
 * Right click activates a secondary action.
 * @param pos - reference a Position on the map
 */
void BattlescapeGame::secondaryAction(const Position& pos)
{
	_tacAction.actor = _battleSave->getSelectedUnit();
	if (_tacAction.actor->getPosition() != pos)
	{
		_tacAction.target = pos;
		_tacAction.strafe = _tacAction.actor->getTurretType() != -1
							 && (SDL_GetModState() & KMOD_CTRL) != 0
							 && Options::battleStrafe == true;

		statePushBack(new UnitTurnBState(this, _tacAction)); // open door or rotate turret.
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
	_tacAction.target = _tacAction.waypoints.front();

	getMap()->setSelectorType(CT_NONE);
	_parentState->getGame()->getCursor()->setHidden();

//	_tacAction.cameraPosition = getMap()->getCamera()->getMapOffset();

	_battleStates.push_back(new ProjectileFlyBState(this, _tacAction));
	statePushFront(new UnitTurnBState(this, _tacAction));
}

/**
 * Handler for the psi button.
 * @note Additional data gets assigned in primaryAction().
 */
void BattlescapeGame::psiButtonAction()
{
	_tacAction.weapon = nullptr;
	_tacAction.targeting = true;
	_tacAction.type = BA_PSIPANIC;

	setupSelector();
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
	_tacAction.target = unit->getPosition();

	if (dir == Pathfinding::DIR_UP)
		++_tacAction.target.z;
	else
		--_tacAction.target.z;

	getMap()->setSelectorType(CT_NONE);
	_parentState->getGame()->getCursor()->setHidden();

	Pathfinding* const pf (_battleSave->getPathfinding());
	pf->calculate(
				_tacAction.actor,
				_tacAction.target);

	statePushBack(new UnitWalkBState(this, _tacAction));
}

/**
 * Requests the end of the turn.
 * @note Waits for explosions etc to really end the turn.
 */
void BattlescapeGame::requestEndTurn()
{
	cancelTacticalAction();

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
 * @return, true if an item was actually picked up
 */
bool BattlescapeGame::pickupItem(BattleAction* const action) const
{
	//Log(LOG_INFO) << "BattlescapeGame::findItem()";
	BattleItem* const targetItem (surveyItems(action->actor));
	if (targetItem != nullptr)
	{
		//Log(LOG_INFO) << ". found " << targetItem->getRules()->getType();
//		if (targetItem->getTile()->getPosition() == action->actor->getPosition())
		if (targetItem->getTile() == action->actor->getTile())
		{
			//Log(LOG_INFO) << ". . pickup on spot";
			if (takeItemFromGround(targetItem, action->actor) == true
				&& targetItem->getAmmoItem() == nullptr)
			{
				//Log(LOG_INFO) << ". . . check Ammo.";
				action->actor->checkAmmo();
			}
			return true;
		}
		else
		{
			//Log(LOG_INFO) << ". . move to spot";
			action->target = targetItem->getTile()->getPosition();
			action->type = BA_MOVE;
		}
	}
	return false;
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
	std::vector<BattleItem*> grdItems;
	for (std::vector<BattleItem*>::const_iterator
			i = _battleSave->getItems()->begin();
			i != _battleSave->getItems()->end();
			++i)
	{
		if ((*i)->getInventorySection() != nullptr
			&& (*i)->getInventorySection()->getSectionType() == ST_GROUND
			&& (*i)->getRules()->getAttraction() != 0)
		{
			//Log(LOG_INFO) << ". " << (*i)->getRules()->getType();
			//Log(LOG_INFO) << ". . grd attr = " << (*i)->getRules()->getAttraction();
			tile = (*i)->getTile();
			if (tile != nullptr
				&& (tile->getTileUnit() == nullptr
					|| tile->getTileUnit() == unit)
				&& tile->getTuCostTile(O_FLOOR, MT_WALK) != 255 // TODO:: pathfind.
				&& tile->getTuCostTile(O_OBJECT, MT_WALK) != 255
				&& worthTaking(*i, unit) == true)
			{
				//Log(LOG_INFO) << ". . . eligible";
				grdItems.push_back(*i);
			}
		}
	}

	if (grdItems.empty() == false) // Select the most suitable candidate depending on attraction and distance.
	{
		const Position posUnit (unit->getPosition());
		int
			worth = 0,
			testWorth;

		std::vector<BattleItem*> choiceItems;
		for (std::vector<BattleItem*>::const_iterator
				i = grdItems.begin();
				i != grdItems.end();
				++i)
		{
			//Log(LOG_INFO) << ". . . grd " << (*i)->getRules()->getType();
			testWorth = (*i)->getRules()->getAttraction()
					  / (TileEngine::distance(
											posUnit,
											(*i)->getTile()->getPosition()) + 1);
			if (testWorth >= worth)
			{
				if (testWorth > worth)
				{
					worth = testWorth;
					choiceItems.clear();
				}
				choiceItems.push_back(*i);
			}
		}

		if (choiceItems.empty() == false)
		{
			//BattleItem* ret = choiceItems.at(RNG::pick(choiceItems.size()));
			//Log(LOG_INFO) << ". . ret = " << ret->getRules()->getType();
			//return ret;
			return choiceItems.at(RNG::pick(choiceItems.size()));
		}
	}
	//else Log(LOG_INFO) << ". no elible items";

	return nullptr;
}

/**
 * Assesses whether an item is worth trying to pick up.
 * @param item - pointer to a BattleItem to go for
 * @param unit - pointer to the BattleUnit looking for an item
 * @return, true if the item is worth going for
 */
bool BattlescapeGame::worthTaking(
		BattleItem* const item,
		BattleUnit* const unit) const
{
	//Log(LOG_INFO) << "BattlescapeGame::worthTaking()";
	std::vector<const RuleInventory*> inTypes;
	inTypes.push_back(getRuleset()->getInventoryRule(ST_RIGHTHAND));
	inTypes.push_back(getRuleset()->getInventoryRule(ST_LEFTHAND));
	inTypes.push_back(getRuleset()->getInventoryRule(ST_BELT));
	inTypes.push_back(getRuleset()->getInventoryRule(ST_BACKPACK));

	bool fit = false;
	for (std::vector<const RuleInventory*>::const_iterator
			i = inTypes.begin();
			i != inTypes.end() && fit == false;
			++i)
	{
		for (std::vector<RuleSlot>::const_iterator
				j = (*i)->getSlots()->begin();
				j != (*i)->getSlots()->end() && fit == false;
				++j)
		{
			if (Inventory::isOverlap(
								unit, item, *i,
								j->x, j->y) == false
				&& (*i)->fitItemInSlot(
								item->getRules(),
								j->x, j->y) == true)
			{
				fit = true;
			}
		}
	}

	if (fit == true)
	{
		switch (item->getRules()->getBattleType())
		{
			default:
			case BT_FIREARM:
				//Log(LOG_INFO) << ". Firearm ret TRUE";
				return true;
/*				if (item->getAmmoItem() == nullptr) // not loaded
				{
					for (std::vector<BattleItem*>::const_iterator
							i = unit->getInventory()->begin();
							i != unit->getInventory()->end();
							++i)
					{
						if ((*i)->getRules()->getBattleType() == BT_AMMO)
						{
							for (std::vector<std::string>::const_iterator
									j = item->getRules()->getCompatibleAmmo()->begin();
									j != item->getRules()->getCompatibleAmmo()->end();
									++j)
							{
								if (*j == (*i)->getRules()->getName())
								{
									//Log(LOG_INFO) << ". ret [2] TRUE";
									return true;
								}
							}
						}
					}
				} */

			case BT_AMMO:
				//Log(LOG_INFO) << ". ammo";
				for (std::vector<BattleItem*>::const_iterator
						i = unit->getInventory()->begin();
						i != unit->getInventory()->end();
						++i)
				{
					if ((*i)->getRules()->getBattleType() == BT_FIREARM)
					{
						//Log(LOG_INFO) << ". . has Firearm for it";
						for (std::vector<std::string>::const_iterator
								j = (*i)->getRules()->getCompatibleAmmo()->begin();
								j != (*i)->getRules()->getCompatibleAmmo()->end();
								++j)
						{
							//Log(LOG_INFO) << ". . . try matching " << (*j);
							if (*j == (*i)->getRules()->getType())
							{
								//Log(LOG_INFO) << ". ret [1] TRUE";
								return true;
							}
						}
					}
				}
		}
	}

	// The problem here in addition to the quirky space-calculation is that only
	// weapons and ammo are considered worthwhile.
	//Log(LOG_INFO) << ". ret FALSE";
	return false;
}

/**
 * Picks the item up from the ground.
 * @param item - pointer to a BattleItem to go for
 * @param unit - pointer to the BattleUnit looking for an item
 * @return, true if enough TU
 */
bool BattlescapeGame::takeItemFromGround(
		BattleItem* const item,
		BattleUnit* const unit) const
{
	//Log(LOG_INFO) << "BattlescapeGame::takeItemFromGround()";
	const RuleInventory
		* const rhRule (getRuleset()->getInventoryRule(ST_RIGHTHAND)),
		* const grdRule (getRuleset()->getInventoryRule(ST_GROUND));
	const int cost = grdRule->getCost(rhRule);

	if (unit->getTimeUnits() >= cost
		&& takeItem(item, unit) == true)
	{
		unit->spendTimeUnits(cost);
		item->getTile()->removeItem(item);
		//Log(LOG_INFO) << ". ret TRUE";
		return true;
	}

	//Log(LOG_INFO) << ". ret FALSE";
	return false;
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
		* const rhRule (getRuleset()->getInventoryRule(ST_RIGHTHAND)),
		* const lhRule (getRuleset()->getInventoryRule(ST_LEFTHAND));
	BattleItem
		* const rhWeapon (unit->getItem(ST_RIGHTHAND)),
		* const lhWeapon (unit->getItem(ST_LEFTHAND));

	RuleItem* const itRule (item->getRules());
	int placed (0);

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
			} // no break;
		case BT_AMMO:
			if (rhWeapon != nullptr
				&& rhWeapon->getAmmoItem() == nullptr
				&& rhWeapon->setAmmoItem(item) == 0)
			{
				placed = 2;
				break;
			}

			if (lhWeapon != nullptr
				&& lhWeapon->getAmmoItem() == nullptr
				&& lhWeapon->setAmmoItem(item) == 0)
			{
				placed = 2;
				break;
			} // no break;

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
			item->changeOwner(unit); // no break;
		case 2:
			//Log(LOG_INFO) << ". ret TRUE";
			return true;
	}

	//Log(LOG_INFO) << ". ret FALSE";
	return false;
}

/**
 * Tallies the conscious units Player and Hostile.
 * @param liveHostile	- reference in which to store the live aLien tally
 * @param livePlayer	- reference in which to store the live xCom tally
 * @return, true if all aliens are dead or pacified independent of battleAllowPsionicCapture option
 */
bool BattlescapeGame::tallyUnits(
		int& liveHostile,
		int& livePlayer) const
{
	bool ret (true);

	liveHostile =
	livePlayer = 0;

	for (std::vector<BattleUnit*>::const_iterator
			j = _battleSave->getUnits()->begin();
			j != _battleSave->getUnits()->end();
			++j)
	{
		if ((*j)->isOut_t(OUT_STAT) == false)
		{
			switch ((*j)->getOriginalFaction())
			{
				case FACTION_HOSTILE:
					if (Options::battleAllowPsionicCapture == false
						|| (*j)->isMindControlled() == false)
					{
						++liveHostile;
					}

					if ((*j)->isMindControlled() == false)
						ret = false;
					break;

				case FACTION_PLAYER:
					if ((*j)->isMindControlled() == false)
						++livePlayer;
					else
						++liveHostile;
			}
		}
	}

	//Log(LOG_INFO) << "bg:tallyUnits() ret = " << ret << "; Sol = " << livePlayer << "; aLi = " << liveHostile;
	return ret;
}

/**
 * Tallies the conscious player-units at an Exit-area.
 * @return, quantity of units at exit
 */
int BattlescapeGame::tallyPlayerExit() const
{
	int ret (0);
	for (std::vector<BattleUnit*>::const_iterator
			j = _battleSave->getUnits()->begin();
			j != _battleSave->getUnits()->end();
			++j)
	{
		if ((*j)->isInExitArea(END_POINT) == true
			&& (*j)->isOut_t(OUT_STAT) == false
			&& (*j)->getOriginalFaction() == FACTION_PLAYER)
//			&& (*j)->isMindControlled() == false) // allow.
		{
			++ret;
		}
	}
	return ret;
}

/**
 * Tallies conscious hostile-units.
 * @return, quantity of hostiles
 */
int BattlescapeGame::tallyHostiles() const
{
	int ret (0);
	for (std::vector<BattleUnit*>::const_iterator
			j = _battleSave->getUnits()->begin();
			j != _battleSave->getUnits()->end();
			++j)
	{
		if ((*j)->isOut_t(OUT_STAT) == false
			&& (*j)->getOriginalFaction() == FACTION_HOSTILE
			&& (Options::battleAllowPsionicCapture == false
				|| (*j)->isMindControlled() == false))
		{
			++ret;
		}
	}
	return ret;
}

/**
 * Sets the TU reserved type as a BattleAction.
 * @param bat - a battleactiontype (BattlescapeGame.h)
 *
void BattlescapeGame::setReservedAction(BattleActionType bat)
{
	_battleSave->setBatReserved(bat);
} */

/**
 * Returns the action type that is reserved.
 * @return, the BattleActionType that is reserved
 *
BattleActionType BattlescapeGame::getReservedAction() const
{
	return _battleSave->getBatReserved();
} */

/**
 * Sets the kneel reservation setting.
 * @param reserved - true to reserve an extra 4 TUs to kneel
 *
void BattlescapeGame::setKneelReserved(bool reserved) const
{
	_battleSave->setKneelReserved(reserved);
} */

/**
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
