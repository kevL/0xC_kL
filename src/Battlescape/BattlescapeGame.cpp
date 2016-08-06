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

//#include <cmath>
#include <sstream>

#include "AlienBAIState.h"
#include "BattlescapeState.h"
#include "BattleState.h"
#include "Camera.h"
#include "CivilianBAIState.h"
#include "ExplosionBState.h"
#include "Explosion.h"
#include "InfoboxDialogState.h"
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
#include "../Engine/Screen.h"
#include "../Engine/Sound.h"

#include "../Interface/Cursor.h"
#include "../Interface/Text.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/RuleAlienDeployment.h"
#include "../Ruleset/RuleArmor.h"
#include "../Ruleset/RuleInventory.h"
#include "../Ruleset/RuleItem.h"
#include "../Ruleset/Ruleset.h"

#include "../Savegame/BattleItem.h"
#include "../Savegame/BattleUnit.h"
#include "../Savegame/BattleUnitStatistics.h"
#include "../Savegame/SavedBattleGame.h"
#include "../Savegame/Tile.h"


namespace OpenXcom
{

/// Outputs the BattleAction as a string.
std::string BattleAction::debugBAction(const BattleAction& action) // static.
{
	std::ostringstream oststr;
	oststr
		<< "\n"
		<< "\ttype= "			<< debugBat(action.type) << "\n"
		<< "\tactor= "			<< (action.actor ? std::to_string(action.actor->getId()) : "NONE") << "\n"
		<< "\ttargetUnit= "		<< (action.targetUnit ? std::to_string(action.targetUnit->getId()) : "NONE") << "\n"
		<< "\tposTarget= "		<< action.posTarget << "\n"
		<< "\tweapon= "			<< (action.weapon ? action.weapon->getRules()->getType() : "NONE") << "\n"
		<< "\tTU= "				<< action.TU << "\n"
//		<< "\tfirstTU= "		<< action.firstTU << "\n"
		<< "\ttargeting= "		<< action.targeting << "\n"
		<< "\tvalue= "			<< action.value << "\n"
		<< "\tresult= "			<< action.result << "\n"
		<< "\tstrafe= "			<< action.strafe << "\n"
		<< "\tdash= "			<< action.dash << "\n"
		<< "\tdiff= "			<< action.diff << "\n"
		<< "\tautoShotCount= "	<< action.autoShotCount << "\n"
		<< "\tposCamera= "		<< action.posCamera << "\n"
		<< "\tdesperate= "		<< action.desperate << "\n"
		<< "\tfinalFacing= "	<< action.finalFacing << "\n"
		<< "\tfinalAction= "	<< action.finalAction << "\n"
		<< "\ttakenXp= "		<< action.takenXp << "\n"
		<< "\twaypoints= "		<< action.waypoints.size();
	return oststr.str();
}


//bool BattlescapeGame::_debug = false;		// static.
bool BattlescapeGame::_debugPlay = false;	// static.

const char* const BattlescapeGame::PLAYER_ERROR[16u] // static.
{
	"STR_NOT_ENOUGH_TIME_UNITS",			//  0
	"STR_NOT_ENOUGH_ENERGY",				//  1
	"STR_NO_AMMUNITION_LOADED",				//  2
	"STR_ACTION_NOT_ALLOWED_FLOAT",			//  3
	"STR_ACTION_NOT_ALLOWED_ALIEN",			//  4
	"STR_OUT_OF_RANGE",						//  5
	"STR_NO_LINE_OF_FIRE",					//  6
	"STR_THERE_IS_NO_ONE_THERE",			//  7
	"STR_WEAPON_IS_ALREADY_LOADED",			//  8
	"STR_WRONG_AMMUNITION_FOR_THIS_WEAPON",	//  9
	"STR_BOTH_HANDS_MUST_BE_EMPTY",			// 10
	"STR_ACTION_NOT_ALLOWED_PSIONIC",		// 11
	"STR_ACTION_NOT_ALLOWED_NOFLY",			// 12
	"STR_ACTION_NOT_ALLOWED_ROOF",			// 13
	"STR_ACTION_NOT_ALLOWED_FLOOR",			// 14
	"STR_UNABLE_TO_THROW_HERE"				// 15
//	"STR_TUS_RESERVED"
//	"STR_NO_ROUNDS_LEFT"
};


/**
 * Initializes the core elements of tactical combat.
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
		_executeProgress(false),
		_shotgunProgress(false),
		_killStatMission(0),
		_killStatTurn(0),
		_killStatPoints(0)
//		_endTurnProcessed(false)
{
	//Log(LOG_INFO) << "Create BattlescapeGame";
	_debugPlay = false;

	cancelTacticalAction();
	checkCasualties(nullptr, nullptr, true); // ... aLien power-source explosions.

//	_playerAction.actor = nullptr;
//	_playerAction.type = BA_NONE;
//	_playerAction.targeting = false;
	_playerAction.clearAction();

	_universalFist = new BattleItem(
								getRuleset()->getItemRule("STR_FIST"),
								battleSave->getCanonicalBattleId());
	_alienPsi = new BattleItem(
							getRuleset()->getItemRule("ALIEN_PSI_WEAPON"),
							battleSave->getCanonicalBattleId());

	for (std::vector<BattleUnit*>::const_iterator	// NOTE: This stuff needs to be done *after*
			i = _battleSave->getUnits()->begin();	// all the tactical-related class-objects
			i != _battleSave->getUnits()->end();	// have finished instantiating ->
			++i)
	{
		(*i)->setBattleForUnit(this);

		if ((*i)->getAIState() != nullptr) // check for faction/ unconscious etc. units
			(*i)->getAIState()->init();
	}

	_parentState->getMap()->setBattleGame(this);

	// sequence of instantiations:
	// - SavedBattleGame		- dTor: DebriefingState::btnOkClick()
	// - BattlescapeGenerator
	// - BattlescapeState
	// - this.
}

/**
 * Deletes BattlescapeGame.
 */
BattlescapeGame::~BattlescapeGame()
{
	for (std::list<BattleState*>::const_iterator
			i = _battleStates.begin();
			i != _battleStates.end();
			++i)
		delete *i;

	delete _universalFist;
	delete _alienPsi;
}

/**
 * Checks for units panicking or falling and so on.
 * @note Called by BattlescapeState::think().
 */
void BattlescapeGame::think()
{
	//Log(LOG_INFO) << "bg:think()";
	if (_battleStates.empty() == true) // nothing is happening -> see if they need some aLien AI or units panicking or whatever
	{
		switch (_battleSave->getSide())
		{
			case FACTION_PLAYER:
				if (_playerPanicHandled == false) // not all panicking units have been handled
				{
					//Log(LOG_INFO) << ". panic Handled is FALSE";
					if ((_playerPanicHandled = handlePanickingPlayer()) == true)
					{
						//Log(LOG_INFO) << ". panic Handled TRUE";
						getTileEngine()->calcFovTiles_all();
						getTileEngine()->calcFovUnits_all();
						_battleSave->getBattleState()->updateSoldierInfo(false); // start Player turn.
					}
				}
				else
				{
					//Log(LOG_INFO) << ". panic Handled is TRUE";
					_parentState->updateExperienceInfo();
				}
				break;

			case FACTION_HOSTILE:
			case FACTION_NEUTRAL:
				if (_debugPlay == false)
				{
					BattleUnit* selUnit (_battleSave->getSelectedUnit());
					if (selUnit != nullptr)
					{
						//Log(LOG_INFO) << "bg:think() selUnit VALID id-" << selUnit->getId();
						_parentState->printDebug(Text::intWide(selUnit->getId()));
						if (handlePanickingUnit(selUnit) == false)
						{
							//Log(LOG_INFO) << "bg:think() . handleUnitAI()";
							handleUnitAI(selUnit); // will select next AI-unit when the current unit is done.
						}
					}
					else if ((selUnit = _battleSave->firstFactionUnit(_battleSave->getSide())) != nullptr)
					{
						//Log(LOG_INFO) << "bg:think() first VALID id-" << selUnit->getId();
						_AIActionCounter = 0;

						_battleSave->setSelectedUnit(selUnit);
						getMap()->getCamera()->centerPosition(selUnit->getPosition(), false);
					}
					else
					{
						//Log(LOG_INFO) << "bg:think() endAiTurn (think)";
						endAiTurn();
					}
				}
		}

		if (_battleSave->unitsFalling() == true)
		{
			//Log(LOG_INFO) << ". Units are Falling() selUnit id-" << _battleSave->getSelectedUnit()->getId();
			_battleSave->unitsFalling() = false;
			stateBPushFront(new UnitFallBState(this));
		}
	}
	//Log(LOG_INFO) << "BattlescapeGame::think() EXIT";
}

/**
 * Gives a slice to the front BattleState and redraws the battlefield.
 * @note The period is controlled by '_tacticalTimer' in BattlescapeState.
 */
void BattlescapeGame::handleBattleState()
{
	if (_battleStates.empty() == false)
	{
		if (_battleStates.front() != nullptr)
		{
			_battleStates.front()->think();
			getMap()->draw();		// old code!! Less clunky when scrolling the battlefield.
//			getMap()->invalidate();	// redraw map
		}
		else
		{
			_battleStates.pop_front();
			endTurn();
		}
	}
}

/**
 * Pushes a BattleState to the front of the queue and starts it.
 * @param battleState - pointer to BattleState
 */
void BattlescapeGame::stateBPushFront(BattleState* const battleState)
{
	_battleStates.push_front(battleState);
	battleState->init();
}

/**
 * Pushes a BattleState as the next state after the current one.
 * @param battleState - pointer to BattleState
 */
void BattlescapeGame::stateBPushNext(BattleState* const battleState)
{
	if (_battleStates.empty() == false)
		_battleStates.insert(
						++_battleStates.begin(),
						battleState);
	else
	{
		_battleStates.push_front(battleState);
		battleState->init();
	}
}

/**
 * Pushes a BattleState to the back of the states-vector.
 * @note Passing in NULL causes an end-turn request.
 * @param battleState - pointer to BattleState (default nullptr)
 */
void BattlescapeGame::stateBPushBack(BattleState* const battleState)
{
	if (_battleStates.empty() == false)
		_battleStates.push_back(battleState);
	else if (battleState != nullptr)
	{
		_battleStates.push_front(battleState);
		battleState->init();
	}
	else
		endTurn();
}

/**
 * Post-procedures for the most recent BattleState.
 * @note This is a very important function. It is called by a BattleState
 * (WalkB, ProjectileFlyB, ExplosionB, etc.) at the moment that state has
 * finished its BattleAction. Check the result of that BattleAction here and do
 * all the aftermath. The state is then popped off the '_battleStates' list.
 */
void BattlescapeGame::popBattleState()
{
	//Log(LOG_INFO) << "";
	//Log(LOG_INFO) << "BattlescapeGame::popBattleState() qtyStates= " << _battleStates.size();

	//for (std::list<BattleState*>::const_iterator
	//		i = _battleStates.begin();
	//		i != _battleStates.end();
	//		++i)
	//	Log(LOG_INFO) << ". . " << (*i)->getBattleStateLabel();

//	if (Options::traceAI)
//		Log(LOG_INFO) << "BattlescapeGame::popBattleState() "
//					  << "id-" << (_playerAction.actor ? std::to_string(_playerAction.actor->getId()) : " Actor NONE")
//					  << " AIActionCounter = " << _AIActionCounter
//					  << " tuSelected = " << (_battleSave->getSelectedUnit() ? std::to_string(_battleSave->getSelectedUnit()->getTu()) : "selUnit NONE");

//	std::set<Tile*>& tilesToDetonate (_battleSave->detonationTiles()); // detonate tiles affected with HE
//	Log(LOG_INFO) << ". explode Tiles qty= " << tilesToDetonate.size();
//	for (std::set<Tile*>::const_iterator
//			i = tilesToDetonate.begin();
//			i != tilesToDetonate.end();
//			++i)
//	{
//		getTileEngine()->detonateTile(*i);
//
//		getTileEngine()->applyGravity(*i);
//		Tile* const tileAbove (_battleSave->getTile((*i)->getPosition() + Position(0,0,1)));
//		if (tileAbove != nullptr)
//			getTileEngine()->applyGravity(tileAbove);
//	}
//	tilesToDetonate.clear();
//	Log(LOG_INFO) << ". explode Tiles DONE";

	if (_battleStates.empty() == false)
	{
		//Log(LOG_INFO) << ". states NOT Empty";
		if (getMap()->getExplosions()->empty() == true)						// Explosions need to continue to run fast after
		{																	// popping ProjectileFlyBState etc etc.
			setStateInterval(BattlescapeState::STATE_INTERVAL_STANDARD);	// So don't set core-speed to Standard until they're done.
			//Log(LOG_INFO) << "bg:popBattleState() set interval= " << BattlescapeState::STATE_INTERVAL_STANDARD;
		}

		const BattleAction action (_battleStates.front()->getAction());

		bool actionFail;
		if (action.result.empty() == false // query the warning string message
			&& (_battleSave->getSide() == FACTION_PLAYER || _debugPlay == true)
			&& action.actor != nullptr
			&& action.actor->getFaction() == FACTION_PLAYER
			&& _playerPanicHandled == true)
		{
			//Log(LOG_INFO) << ". . actionFail";
			actionFail = true;
			_parentState->warning(action.result);

			// remove action.Cursor if error.Message (eg, not enough TUs)
			if (   action.result.compare(BattlescapeGame::PLAYER_ERROR[0u]) == 0	// no TU
				|| action.result.compare(BattlescapeGame::PLAYER_ERROR[2u]) == 0)	// no Load
//				|| action.result.compare("STR_NO_ROUNDS_LEFT") == 0) // <- removed from ProjectileFlyBState, clips are deleted at 0-rounds.
			{
				switch (action.type)
				{
					case BA_LAUNCH:
						_playerAction.waypoints.clear();
						// no break;
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

		//Log(LOG_INFO) << ". move Front-state to _deletedStates.";
		_deletedStates.push_back(_battleStates.front());
		//Log(LOG_INFO) << ". states.Popfront";
		_battleStates.pop_front();


		if (action.actor != nullptr // handle the end of the acting BattleUnit's actions
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
							//Log(LOG_INFO) << ". . id-" << action.actor->getId() << " tu= " << action.actor->getTu();
							action.actor->expendTu(action.TU);
							// kL_query: Does this happen **before** ReactionFire/getReactor()?
							// no. not for shooting, but for throwing it does; actually no it doesn't.
							//
							// wtf, now RF works fine. NO IT DOES NOT.
							//Log(LOG_INFO) << ". . ID " << action.actor->getId() << " currentTU = " << action.actor->getTu() << " spent TU = " << action.TU;

							if (_battleSave->getSide() == FACTION_PLAYER) // is NOT reaction-fire
							{
								//Log(LOG_INFO) << ". side -> Faction_Player";
								// After throwing the cursor returns to default cursor;
								// after shooting it stays in targeting mode and the player
								// can shoot again in the same mode (autoshot/snap/aimed)
								// unless he/she/it is out of ammo and/or TUs.
								switch (action.type)
								{
									case BA_USE: // NOTE: Only MindProbe is a targeting-action w/ type BA_USE.
									case BA_PSICONTROL:
									case BA_PSIPANIC:
									case BA_PSICONFUSE:
									case BA_PSICOURAGE:
										if (action.actor->getTu() < action.actor->getActionTu(
																							action.type,
																							action.weapon))
										{
											cancelTacticalAction(true); // NOTE: Not sure if these needs to be 'forced' ->
										}
										break;

									case BA_LAUNCH:
										_playerAction.waypoints.clear();
										// no break;
									case BA_THROW:
										cancelTacticalAction(true);
										break;

									case BA_SNAPSHOT:
									case BA_AUTOSHOT:
									case BA_AIMEDSHOT:
										if (action.weapon->getAmmoItem() == nullptr
											|| action.actor->getTu() < action.actor->getActionTu(
																								action.type,
																								action.weapon))
										{
											cancelTacticalAction(true);
										}
								}
							}
						}
					}
					break;

				default: // action.actor is NOT Faction_Player ->
				case FACTION_HOSTILE:
				case FACTION_NEUTRAL:
					//Log(LOG_INFO) << ". action -> NOT Faction_Player";
					action.actor->expendTu(action.TU);

					if (_battleSave->getSide() != FACTION_PLAYER && _debugPlay == false)
					{
						BattleUnit* selUnit (_battleSave->getSelectedUnit());
						if (_AIActionCounter > 2	// AI does two think-cycles per unit before switching to the next
							|| selUnit == nullptr	// unit -- or unit got killed before doing its second think.
							|| selUnit->getHealth() == 0
							|| selUnit->getStun() >= selUnit->getHealth())
//							|| selUnit->isOut_t() == true)
						{
							if (selUnit != nullptr) // NOTE: This is getting silly.
							{
								selUnit->flagCache();
								getMap()->cacheUnitSprite(selUnit);
							}

							if (_battleStates.empty() == true) // nothing left for current Actor to do
							{
								if (_battleSave->selectNextUnit(false, true) != nullptr)
								{
									//Log(LOG_INFO) << "bg:popBattleState() selUnit funky - next id-" << _battleSave->getSelectedUnit()->getId();
									_AIActionCounter = 0;
								}
								else
								{
									//Log(LOG_INFO) << "bg:popBattleState() -> endAiTurn()";
									endAiTurn(); // NOTE: This is probly handled just as well in think().
								}
							}

							if ((selUnit = _battleSave->getSelectedUnit()) != nullptr)
							{
								getMap()->getCamera()->centerPosition(selUnit->getPosition());
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


		if (_battleStates.empty() == false)
		{
			if (_battleStates.front() == nullptr) // end turn request
			{
				while (_battleStates.empty() == false && _battleStates.front() == nullptr)
					_battleStates.pop_front();

				if (_battleStates.empty() == false)
					_battleStates.push_back(nullptr);
				else
				{
					//Log(LOG_INFO) << "bg:popBattleState() endTurn.";
					endTurn();
					return;
				}
			}
			_battleStates.front()->init(); // (re)init the next state
		}


		if (_battleSave->getSelectedUnit() == nullptr
			|| _battleSave->getSelectedUnit()->isOut_t() == true)
		{
			//Log(LOG_INFO) << ". selUnit invalid OR incapacitated: cancelAction";
			cancelTacticalAction();

			switch (_battleSave->getSide())
			{
				case FACTION_PLAYER:
					_battleSave->setSelectedUnit();
					break;

				case FACTION_HOSTILE:
				case FACTION_NEUTRAL:
					_battleSave->selectNextUnit(true, true);
			}
		}

		if (_battleSave->getSide() == FACTION_PLAYER || _debugPlay == true)
		{
			//Log(LOG_INFO) << ". updateSoldierInfo()";
			_parentState->updateSoldierInfo(false);	// try no calcFoV()
		}
	}

	if (_battleStates.empty() == true) // NOTE: This could probly go upstairs but for now it's working okay.
	{
		//Log(LOG_INFO) << "";
		//Log(LOG_INFO) << "popState: STATES are Empty";
		if (getTileEngine()->isReaction() == true)
		{
			//Log(LOG_INFO) << "PopState: rfShooter VALID - clear!";
			getTileEngine()->isReaction() = false;
			SDL_Delay(Screen::SCREEN_PAUSE);
		}

		if (_battleSave->rfTriggerOffset().z != -1)	// NOTE: Since non-vis reactors don't set isReaction() TRUE
		{											// the camera-to-original-position must be handled independently.
			//Log(LOG_INFO) << ". set Camera to triggerPos " << _battleSave->rfTriggerOffset();
			getMap()->getCamera()->setMapOffset(_battleSave->rfTriggerOffset());
			_battleSave->rfTriggerOffset(Position(0,0,-1));
		}

		if (_battleSave->getSide() == FACTION_PLAYER || _debugPlay == true)
		{
			_parentState->getGame()->getCursor()->setHidden(false);
			_parentState->refreshMousePosition(); // update tile-data on the HUD
			setupSelector();
		}
	}
	//Log(LOG_INFO) << "BattlescapeGame::popBattleState() EXIT";
}

/**
 * Determines whether there are any actions pending for a specified BattleUnit.
 * @param unit - pointer to a BattleUnit
 * @return, true if there are no actions pending
 */
bool BattlescapeGame::noActionsPending(const BattleUnit* const unit) const
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
 * Clears the trace-AI markers on all battlefield Tiles.
 */
void BattlescapeGame::resetTraceTiles() // private.
{
	Tile* tile;
	for (size_t
			i = 0u;
			i != _battleSave->getMapSizeXYZ();
			++i)
	{
		tile = _battleSave->getTiles()[i];
		tile->setPreviewColor(0u);
		tile->setPreviewDir(-1);
		tile->setPreviewTu(-1);
	}
}

/**
 * Handles the processing of the AI-state of a non-player BattleUnit.
 * @note Called by BattlescapeGame::think().
 * @param unit - pointer to a BattleUnit
 */
void BattlescapeGame::handleUnitAI(BattleUnit* const unit) // private.
{
	bool _debug (false);
	//if (unit->getId() == 1000006)	_debug = true;
	//else							_debug = false;

	if (_debug) {
		Log(LOG_INFO) << "";
		Log(LOG_INFO) << "bg::handleUnitAI() id-" << unit->getId()
					  << " vis= " << unit->getUnitVisible()
					  << " " << unit->getPosition()
					  << " status= " << BattleUnit::debugStatus(unit->getUnitStatus())
					  << " tu= " << unit->getTu();
	}

	const int debug (Options::traceAI);
	switch (debug)
	{
		case 2:
			resetTraceTiles();
			// no break;
//		case 1:
			//Log(LOG_INFO) << "BattlescapeGame::handleUnitAI() id-" << unit->getId();
	}

//	unit->setUnitVisible(false);
//	getTileEngine()->calcFovUnits_pos(unit->getPosition());
	getTileEngine()->calcFovUnits(unit); // refresh unit's "targets"

	if (unit->getAIState() == nullptr)
	{
		BattleAIState* aiState;
		switch (unit->getFaction())
		{
			default:
			case FACTION_HOSTILE: aiState = new AlienBAIState(_battleSave, unit); break;
			case FACTION_NEUTRAL: aiState = new CivilianBAIState(_battleSave, unit);
		}
		aiState->init();
		unit->setAIState(aiState);
	}


	Pathfinding* const pf (_battleSave->getPathfinding());
	pf->setPathingUnit(unit);

	unit->getAIState()->reachableTiles() = pf->findReachable(unit, unit->getTu());
	if (unit->getAIState()->reachableTiles().size() < 2)	// not enough TU to really do anything ...
	{														// NOTE: That won't work well w/ no-energy stationary emplacement guns.
		unit->setReselect(false);							// It could also obviate niceties like finalFacing.
		if (debug) Log(LOG_INFO) << ". no reachableTiles/low TU - flag dontReselect";
	}

	if (unit->getReselect() == false || ++_AIActionCounter > 1)
	{
		if (debug) {
			Log(LOG_INFO) << ". select next[1]";
			Log(LOG_INFO) << ". curr id-" << (_battleSave->getSelectedUnit() ? _battleSave->getSelectedUnit()->getId() : 0);
		}

		selectNextAiUnit(unit);
		if (debug) Log(LOG_INFO) << ". next id-" << (_battleSave->getSelectedUnit() ? _battleSave->getSelectedUnit()->getId() : 0);
		return;
	}
	else if (_AIActionCounter == 1)
	{
		_playedAggroSound = false;
		unit->setHiding(false);
	}

	BattleAction aiAction;
	aiAction.actor = unit;

	bool doThink;
	if (unit->getFaction() == FACTION_HOSTILE // pickup Item ->
		&& unit->getMainHandWeapon() == nullptr
		&& unit->getRankString() != "STR_LIVE_TERRORIST") // TODO: new funct. hasFixedWeapon().
	{
		if (pickupItem(&aiAction) == true)
		{
			if (debug) Log(LOG_INFO) << ". pickup Weapon ... do think";
			doThink = true;
			if (_battleSave->getDebugTac() == true) // <- order matters.
				_parentState->updateSoldierInfo(false);
		}
		else
		{
			if (debug) Log(LOG_INFO) << ". MOVE to pickup Weapon ... no think req'd";
			doThink = false;
		}
	}
	else
		doThink = true;

	if (doThink == true)
	{
		aiAction.type = BA_THINK;
		if (debug) {
			Log(LOG_INFO) << "";
			Log(LOG_INFO) << "";
			Log(LOG_INFO) << "";
			Log(LOG_INFO) << "BATTLESCAPE::handleUnitAI id-" << unit->getId();
			Log(LOG_INFO) << "BATTLESCAPE: AIActionCount [in] = " << _AIActionCounter;
		}
		unit->thinkAi(&aiAction);
		if (debug) {
			Log(LOG_INFO) << "BATTLESCAPE: id-" << unit->getId() << " bat = " << BattleAction::debugBat(aiAction.type);
			Log(LOG_INFO) << "BATTLESCAPE: AIActionCount [out] = " << _AIActionCounter;
		}

		if (aiAction.type == BA_THINK)
		{
			if (debug) {
				Log(LOG_INFO) << "";
				Log(LOG_INFO) << ". BATTLESCAPE: Re-Think id-" << unit->getId();
				Log(LOG_INFO) << ". BATTLESCAPE: AIActionCount [in] = " << _AIActionCounter;
			}

			unit->thinkAi(&aiAction);

			if (debug) {
				Log(LOG_INFO) << ". BATTLESCAPE: id-" << unit->getId() << " bat = " << BattleAction::debugBat(aiAction.type);
				Log(LOG_INFO) << ". BATTLESCAPE: AIActionCount [out] = " << _AIActionCounter;
			}
		}

		if (_playedAggroSound == false && unit->getChargeTarget() != nullptr)
		{
			_playedAggroSound = true;

			const int soundId (unit->getAggroSound());
			if (soundId != -1)
				getResourcePack()->getSound("BATTLE.CAT", static_cast<unsigned>(soundId))
									->play(-1, getMap()->getSoundAngle(unit->getPosition()));
		}
	}

	std::wstring wst (Language::fsToWstr(BattleAIState::debugAiMode(unit->getAIState()->getAIMode())));
	_parentState->printDebug(wst + L"> " + Text::intWide(unit->getId()));
	if (debug) Log(LOG_INFO) << BattleAction::debugBAction(aiAction);

	switch (aiAction.type)
	{
		case BA_MOVE:
		{
			pf->setPathingUnit(unit);
			if (_debug) {
				Log(LOG_INFO) << "handleUnitAI() id-" << unit->getId() << " status= " << BattleUnit::debugStatus(unit->getUnitStatus());
				Log(LOG_INFO) << ". " << unit->getPosition();
				Log(LOG_INFO) << ". " << aiAction.posTarget;
			}

			if (_battleSave->getTile(aiAction.posTarget) != nullptr)	// TODO: Check that .posTarget is not unit's current Tile.
			{															// Or ensure that AIState does not return BA_MOVE if so.
				pf->calculatePath(unit, aiAction.posTarget);			// TODO: AI will choose a position to move to that unit cannot move
				if (_debug) {
					Log(LOG_INFO) << ". . posTarget VALID";
					Log(LOG_INFO) << ". . start dir= " << pf->getStartDirection();
					Log(LOG_INFO) << ". . tu= " << unit->getTu();
//					Log(LOG_INFO) << ". . firstTU= " << aiAction.firstTU;
				}
				if (pf->getStartDirection() != -1)						// to ... stop that (unless blocked by unseen player- or neutral-unit).
//					&& unit->getTu() >= aiAction.firstTU)
				{
					stateBPushBack(new UnitWalkBState(this, aiAction));	// TODO: If aiAction.desperate use 'dash' interval-speed.
				}
				// else select next AI unit.
			}
			break;
		}

		case BA_SNAPSHOT:
		case BA_AUTOSHOT:
		case BA_AIMEDSHOT:
		case BA_LAUNCH:
		case BA_MELEE:
		case BA_THROW:
		case BA_PSICONTROL:
		case BA_PSIPANIC:
			switch (aiAction.type)
			{
				case BA_PSIPANIC:
				case BA_PSICONTROL:
					aiAction.weapon = _alienPsi;
					aiAction.TU = unit->getActionTu(aiAction.type, aiAction.weapon);
					break;

				case BA_MELEE:
					aiAction.TU = unit->getActionTu(aiAction.type, aiAction.weapon);
					// no break;

				default:
					aiAction.value = -1;
					stateBPushBack(new UnitTurnBState(this, aiAction));
					// NOTE: See below_ for (aiAction.type == BA_MELEE).
			}

			if (debug) {
				Log(LOG_INFO) << "BATTLESCAPE: Attack:";
				Log(LOG_INFO) << ". aiAction.type = " << BattleAction::debugBat(aiAction.type);
				Log(LOG_INFO) << ". aiAction.posTarget = " << aiAction.posTarget;
				Log(LOG_INFO) << ". aiAction.weapon = " << aiAction.weapon->getRules()->getName().c_str();
			}
			stateBPushBack(new ProjectileFlyBState(this, aiAction));

			switch (aiAction.type)
			{
				case BA_PSIPANIC:
				case BA_PSICONTROL:
					if (getTileEngine()->psiAttack(&aiAction) == true)
					{
						const BattleUnit* const psiVictim (_battleSave->getTile(aiAction.posTarget)->getTileUnit());
						Language* const lang (_parentState->getGame()->getLanguage());
						switch (aiAction.type)
						{
							default:
							case BA_PSIPANIC:
								wst = lang->getString("STR_PSI_PANIC_SUCCESS")
														.arg(aiAction.value);
								break;
							case BA_PSICONTROL:
								wst = lang->getString("STR_IS_UNDER_ALIEN_CONTROL", psiVictim->getGender())
														.arg(psiVictim->getName(lang))
														.arg(aiAction.value);
						}
						_parentState->getGame()->pushState(new InfoboxState(wst));
					}
			}
			break;

		default:
		case BA_NONE:
//		case BA_THINK: // <- should never happen.
			if (debug) Log(LOG_INFO) << ". select next[2]";
			selectNextAiUnit(unit);
	}
}

/**
 * Decreases the AI-counter by one pip.
 */
void BattlescapeGame::decAiActionCount()
{
	--_AIActionCounter;
}

/**
 * Selects the next AI-unit.
 * @note The current AI-turn ends when no unit is eligible to select.
 * @param unit - the current non-player unit
 */
void BattlescapeGame::selectNextAiUnit(const BattleUnit* const unit) // private.
{
	//Log(LOG_INFO) << "bg:selectNextAiUnit()";
	BattleUnit* const nextUnit (_battleSave->selectNextUnit(
														_AISecondMove == true,
														true));
	if (nextUnit != nullptr)
	{
		//Log(LOG_INFO) << ". id-" << nextUnit->getId();
		_AIActionCounter = 0;

		getMap()->getCamera()->centerPosition(nextUnit->getPosition(), false);
		_parentState->updateSoldierInfo(false); // try no calcFov() ... calcFoV(pos) is going to happen in handleUnitAI() before AI-battlestate-think.

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
				//Log(LOG_INFO) << "BATTLESCAPE::selectNextAiUnit() --- second Move ---";
				_AISecondMove = true;
			}
		}
	}
	else
	{
		//Log(LOG_INFO) << ". endAiTurn (selectNextAiUnit)";
		endAiTurn();
	}
}

/**
 * Ends the AI-turn.
 */
void BattlescapeGame::endAiTurn()
{
	//Log(LOG_INFO) << "bg:endAiTurn()";
	if (_battleSave->getDebugTac() == false)
	{
		_endTurnRequested = true;
		stateBPushBack();
	}
	else // do not show NextTurn screen at the end of hostile/neutral turns if debugPlay.
	{
		_battleSave->selectNextUnit();
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
	if (_playerAction.targeting == false)
	{
		_playerAction.posCamera = Position(0,0,-1);

		static const int
			WARN_NONE	(0),
			WARN		(1),
			WARN_ARG	(2);

		int showWarning (WARN_NONE);

		// NOTE: These actions are done partly in ActionMenuState::btnActionMenuClick() and
		// this subsequently handles a greater or lesser proportion of the resultant niceties.
		//
		switch (_playerAction.type)
		{
			case BA_PRIME:
			case BA_DEFUSE:
				if (_playerAction.actor->expendTu(_playerAction.TU) == false)
				{
					_playerAction.result = BattlescapeGame::PLAYER_ERROR[0u]; // no TU
					showWarning = WARN;
				}
				else
				{
					_playerAction.weapon->setFuse(_playerAction.value);

					switch (_playerAction.value)
					{
						case -1:
							_playerAction.result = "STR_GRENADE_DEACTIVATED";
							showWarning = WARN;
							break;

						case 0:
							_playerAction.result = "STR_GRENADE_ACTIVATED";
							showWarning = WARN;
							break;

						default:
							_playerAction.result = "STR_GRENADE_ACTIVATED_";
							showWarning = WARN_ARG;
					}
				}
				break;

			case BA_USE:
				if (_playerAction.result.empty() == false)
					showWarning = WARN;
				else if (_playerAction.targetUnit != nullptr)
				{
					_battleSave->checkUnitRevival(_playerAction.targetUnit);
					_playerAction.targetUnit = nullptr;
				}
				break;

			case BA_LAUNCH:
				if (_playerAction.result.empty() == false)
					showWarning = WARN;
				break;

			case BA_MELEE:
				if (_playerAction.result.empty() == false)
					showWarning = WARN;
				else if (_playerAction.actor->expendTu(_playerAction.TU) == false)
				{
					_playerAction.result = BattlescapeGame::PLAYER_ERROR[0u]; // no TU
					showWarning = WARN;
				}
				else
				{
					stateBPushBack(new ProjectileFlyBState(this, _playerAction));
					return;
				}
				break;

			case BA_DROP:
				if (_playerAction.result.empty() == false)
					showWarning = WARN;
				else
				{
					_playerAction.actor->flagCache();

					const Position pos (_playerAction.actor->getPosition());
					dropItem(_playerAction.weapon, pos, DROP_FROMINVENTORY);
					getResourcePack()->getSound("BATTLE.CAT", ResourcePack::ITEM_DROP)
										->play(-1, getMap()->getSoundAngle(pos));
				}
				break;

			case BA_LIQUIDATE:
				if (_playerAction.result.empty() == false)
					showWarning = WARN;
				else if (_playerAction.targetUnit != nullptr)
					liquidateUnit();
				break;

			case BA_PSICOURAGE: // one more way to cook a goose.
			case BA_PSICONFUSE:
			case BA_PSIPANIC:
			case BA_PSICONTROL:
				if (_playerAction.result.empty() == false)
					showWarning = WARN;
		}

		switch (showWarning)
		{
			case WARN:
				_parentState->warning(_playerAction.result);
				_playerAction.result.clear();
				break;

			case WARN_ARG:
				_parentState->warning(
									_playerAction.result,
									_playerAction.value);
				_playerAction.result.clear();
		}

		_playerAction.type = BA_NONE;
		_parentState->updateSoldierInfo(false); // try no calcFov()
	}
	setupSelector();
}

/**
 * Coup de grace - a non-target action.
 */
void BattlescapeGame::liquidateUnit() // private.
{
	_playerAction.actor->toggleShoot();
//	_playerAction.actor->setShoot();
//	getMap()->cacheUnitSprite(_playerAction.actor);

	const RuleItem* const itRule (_playerAction.weapon->getRules());
	BattleItem* const load (_playerAction.weapon->getAmmoItem());
	ExplosionType explType;
	int
		soundId,
		aniStart;

	switch (itRule->getBattleType())
	{
		default:
		case BT_FIREARM:
			if (itRule->getType() == "STR_FUSION_TORCH_POWER_CELL")
				explType = ET_TORCH;
			else
				explType = ET_BULLET;

			aniStart = load->getRules()->getFireHitAnimation();

			if ((soundId = load->getRules()->getFireHitSound()) == -1)
				soundId = itRule->getFireHitSound();
			break;

		case BT_MELEE:
			explType = ET_MELEE_ATT;
			aniStart = itRule->getMeleeAnimation();
			soundId = itRule->getMeleeHitSound();
	}

	if (soundId != -1)
		getResourcePack()->getSound("BATTLE.CAT", static_cast<unsigned>(soundId))
							->play(-1, getMap()->getSoundAngle(_playerAction.actor->getPosition()));

	load->spendBullet(
				*_battleSave,
				*_playerAction.weapon);

	if (aniStart != -1)
	{
		Explosion* const explosion (new Explosion(
												explType,
												Position::toVoxelSpaceCentered(_playerAction.posTarget, FLOOR_TLEVEL),
												aniStart));
		getMap()->getExplosions()->push_back(explosion);
		_executeProgress = true;

		Uint32 interval (static_cast<Uint32>(
						 std::max(1,
								  static_cast<int>(BattlescapeState::STATE_INTERVAL_EXPLOSION) - itRule->getExplosionSpeed())));
		setStateInterval(interval);
	}

	_playerAction.targetUnit->playDeathSound(); // scream little piggie

	_playerAction.actor->expendTu(_playerAction.TU);

	_playerAction.targetUnit->setHealth(0);
	_playerAction.targetUnit = nullptr;

	checkCasualties(
				_playerAction.weapon->getRules(),
				_playerAction.actor);
//				false, false, true);
}

/**
 * Sets the selector according to the current action.
 */
void BattlescapeGame::setupSelector() // NOTE: This might not be needed when called right after cancelTacticalAction(false).
{
	getMap()->refreshSelectorPosition();

	SelectorType type;
	int sideSize (1);

	if (_playerAction.targeting == true)
	{
		switch (_playerAction.type)
		{
			case BA_THROW:
				type = CT_TOSS;
				break;

			case BA_PSICONTROL:
			case BA_PSIPANIC:
			case BA_PSICONFUSE:
			case BA_PSICOURAGE:
			case BA_USE:
				type = CT_PSI;
				break;

			case BA_LAUNCH:
				type = CT_LAUNCH;
				break;

			default:
				type = CT_TARGET;
		}
	}
	else
	{
		type = CT_CUBOID;

		if ((_playerAction.actor = _battleSave->getSelectedUnit()) != nullptr)
			sideSize = _playerAction.actor->getArmor()->getSize();
	}

	getMap()->setSelectorType(type, sideSize);
}

/**
 * Determines whether a playable unit is selected.
 * @note Normally only player side units can be selected but in debug mode the
 * aLiens can play too :)
 * @note Is used to see if stats can be displayed and action buttons will work.
 * @return, true if a playable unit is selected
 */
bool BattlescapeGame::playableUnitSelected() const
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
bool BattlescapeGame::kneelToggle(BattleUnit* const unit)
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
					tu = Pathfinding::TU_STAND;
				else
					tu = Pathfinding::TU_KNEEL;

//				if (checkReservedTu(unit, tu) == true)
//					|| (tu == Pathfinding::TU_KNEEL && _battleSave->getKneelReserved() == true))
//				{
				if (unit->getTu() >= tu)
				{
					if (tu == Pathfinding::TU_KNEEL
						|| (tu == Pathfinding::TU_STAND
							&& unit->expendEnergy(std::max(0,
														   Pathfinding::EN_STAND - unit->getArmor()->getAgility())) == true))
					{
						unit->expendTu(tu);
						unit->kneelUnit(unit->isKneeled() == false);
						// kneeling or standing up can reveal new terrain or units. I guess. -> sure can!
						// But updateSoldierInfo() also does does calcFov(), so ...
//						getTileEngine()->calcFov(unit);

						getMap()->cacheUnitSprite(unit);

//						_parentState->updateSoldierInfo(false); // <- also does calcFov() !
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
						_parentState->warning(BattlescapeGame::PLAYER_ERROR[1u]); // no Energy
				}
				else
					_parentState->warning(BattlescapeGame::PLAYER_ERROR[0u]); // no TU
//				}
//				else // note that checkReservedTu() sends its own warnings ....
//					_parentState->warning("STR_TIME_UNITS_RESERVED");
			}
			else
				_parentState->warning(BattlescapeGame::PLAYER_ERROR[3u]); // not allowed: Float
		}
		else //if (unit->getGeoscapeSoldier() != nullptr) // MC'd xCom agent, trying to stand & walk by AI.
		{
			const int energyCost (std::max(0,
										   Pathfinding::EN_STAND - unit->getArmor()->getAgility()));

			if (unit->getTu() >= Pathfinding::TU_STAND
				&& unit->getEnergy() >= energyCost)
			{
				unit->expendTu(Pathfinding::TU_STAND);
				unit->expendEnergy(energyCost);

				unit->kneelUnit(false);
				getMap()->cacheUnitSprite(unit);

				// NOTE: Might want to do a calcFoV for the AI, not sure.
				// Although UnitWalkBState ought handle it.

				return true;
			}
		}
	}
	else
		_parentState->warning(BattlescapeGame::PLAYER_ERROR[4u]); // not allowed: Alien -- TODO: change to "not a Soldier, can't kneel".

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

	_playerAction.targeting = false;
	_playerAction.type = BA_NONE;
	_playerAction.waypoints.clear();
	getMap()->getWaypoints()->clear();

	Tile* tile;
	Position pos;

//	if (_endTurnProcessed == false)
//	{
	for (size_t // check for hot potatos on the ground
			i = 0u;
			i != _battleSave->getMapSizeXYZ();
			++i)
	{
		tile = _battleSave->getTiles()[i];
		for (std::vector<BattleItem*>::const_iterator
				j = tile->getInventory()->begin();
				j != tile->getInventory()->end();
				++j)
		{
			if ((*j)->getRules()->getBattleType() == BT_GRENADE
				&& (*j)->getFuse() != -1 && (*j)->getFuse() < 2) // it's a grenade to explode now
			{
				pos = Position::toVoxelSpaceCentered(
												tile->getPosition(),
												FLOOR_TLEVEL - tile->getTerrainLevel());
				stateBPushNext(new ExplosionBState(
												this,
												pos,
												(*j)->getRules(),
												(*j)->getPriorOwner()));
				_battleSave->toDeleteItem(*j);

				stateBPushBack(); // recycle endTurn() call.
				return;
			}
		}
	}

	if (getTileEngine()->closeSlideDoors() == true) // close doors between grenade & terrain explosions
		getResourcePack()->getSound("BATTLE.CAT", ResourcePack::SLIDING_DOOR_CLOSE)->play();
//	}

	if ((tile = getTileEngine()->checkForTerrainExplosives()) != nullptr)
	{
		pos = Position::toVoxelSpaceCentered(tile->getPosition(), 10);
		// kL_note: This seems to be screwing up.
		// Further info: what happens is that an explosive part of a tile gets destroyed by fire
		// during an end-turn sequence, has its setExplosive() set, then is somehow triggered
		// by the next projectile hit against whatever.
		stateBPushNext(new ExplosionBState(
										this,
										pos,
										nullptr,
										nullptr,
										tile));

//		tile = getTileEngine()->checkForTerrainExplosives();

		stateBPushBack();	// this will repeatedly call another endTurn() so there's
		return;				// no need to continue this one till all explosions are done.
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
			if ((tile = (*i)->getUnitTile()) != nullptr
				&& (tile->getSmoke() != 0 || tile->getFire() != 0))
			{
				tile->hitTileContent(); // hit the tile's unit w/ Smoke & Fire at end of unit's faction's Turn-phase.
			}
		}
	}


	if (_battleSave->endFactionTurn() == true) // <- This rolls over Faction-turns. TRUE means FullTurn has ended.
	{
		for (size_t
				i = 0u;
				i != _battleSave->getMapSizeXYZ();
				++i)
		{
			tile = _battleSave->getTiles()[i];
			if (tile->getInventory()->empty() == false
				&& (tile->getSmoke() != 0 || tile->getFire() != 0))
			{
				tile->hitTileContent(_battleSave); // hit the tile's inventory w/ Fire at beginning of each full-turn.
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
	// best just to do another call to checkForTerrainExplosives()/ ExplosionBState in there ....
	// -> SavedBattleGame::tileVolatiles()
	// Or here
	// ... done it in NextTurnState.

		// check AGAIN for terrain explosions
/*		tile = getTileEngine()->checkForTerrainExplosives();
		if (tile != nullptr)
		{
			pos = Position(
						tile->getPosition().x * 16 + 8,
						tile->getPosition().y * 16 + 8,
						tile->getPosition().z * 24 + 10);
			stateBPushNext(new ExplosionBState(
											this,
											pos,
											nullptr,
											nullptr,
											tile));
			stateBPushBack();
			_endTurnProcessed = true;
			return;
		} */
		// kL_note: you know what, I'm just going to let my quirky solution run
		// for a while. BYPASS _endTurnProcessed
//	}
//	_endTurnProcessed = false;


	checkCasualties();


	int // if all units from either faction are killed - the mission is over.
		liveHostile,
		livePlayer;
	const bool pacified (tallyUnits(
								liveHostile,
								livePlayer));

	if (_battleSave->getObjectiveTileType() == MUST_DESTROY // brain death, end Final Mission.
		&& _battleSave->allObjectivesDestroyed() == true)
	{
		_parentState->finishBattle(false, livePlayer);
	}
	else
	{
		if (_battleSave->getTurnLimit() != 0
			&& _battleSave->getTurnLimit() < _battleSave->getTurn())
		{
			switch (_battleSave->getChronoResult())
			{
				case FORCE_WIN:
					_parentState->finishBattle(false, livePlayer);
					break;

				case FORCE_LOSE:
					_battleSave->isAborted(true);
					_parentState->finishBattle(true, 0);
					break;

				case FORCE_ABORT:
					_battleSave->isAborted(true);
					_parentState->finishBattle(true, tallyPlayerExit());
			}
			return;
		}

		const bool battleComplete ((liveHostile == 0 && _battleSave->getObjectiveTileType() != MUST_DESTROY)
								 || livePlayer  <  1);

		if (battleComplete == false)
		{
			showInfoBoxQueue();
			_parentState->updateSoldierInfo(false); // try no calcFov()

			if (_battleSave->getSide() == FACTION_PLAYER
				|| _battleSave->getDebugTac() == true)
			{
				setupSelector();
				if (playableUnitSelected() == true) // ... there's better be a unit selected here!
					getMap()->getCamera()->centerPosition(
													_battleSave->getSelectedUnit()->getPosition(),
													false);

				if (_battleSave->getDebugTac() == false)
					_battleSave->getBattleState()->toggleIcons(true);
			}
			else
			{
				getMap()->setSelectorType(CT_NONE);
				_battleSave->getBattleState()->toggleIcons(false);
			}

			_battleSave->setPacified(pacified);
		}

		if (_endTurnRequested == true)
		{
			_endTurnRequested = false;

			switch (_battleSave->getSide())
			{
				case FACTION_NEUTRAL:
					if (battleComplete == false) break;
					// no break;
				case FACTION_HOSTILE:
				case FACTION_PLAYER:
//					_parentState->getGame()->delayBlit(); // TODO: Move this to NextTurnState::ctor and out of the core-engine cycle.
					_parentState->getGame()->pushState(new NextTurnState(
																	_battleSave,
																	_parentState,
																	pacified));
			}
		}
	}
}

/**
 * Checks for casualties and adjusts morale accordingly.
 * @note Etc.
// * @note Also checks if Alien Base Control was destroyed in a BaseAssault tactical.
 * @param itRule	- pointer to the weapon's rule (default nullptr)
 * @param attacker	- pointer to credit the kill (default nullptr)
 * @param hidden	- true for UFO Power Source explosions at the start of
 *					  battlescape (default false)
 * @param terrain	- true for terrain explosions (default false)
 */
void BattlescapeGame::checkCasualties(
		const RuleItem* const itRule,
		BattleUnit* attacker,
		bool hidden,
		bool terrain)
{
	//Log(LOG_INFO) << "BattlescapeGame::checkCasualties()"; if (attacker != nullptr) Log(LOG_INFO) << ". id-" << attacker->getId();

	// If the victim was killed by the attacker's death explosion fetch what
	// killed the attacker and make THAT the attacker!
	if (attacker != nullptr)
	{
		if (attacker->getGeoscapeSoldier() == nullptr
			&& attacker->getUnitStatus() == STATUS_DEAD
			&& attacker->getSpecialAbility() == SPECAB_EXPLODE	// TODO: Factor in tile explosions. Eg. hit a barrel -
			&& attacker->getMurdererId() != 0)					// it explodes and transfers any kills to a shooter.
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

		if (attacker->getGeoscapeSoldier() != nullptr)
			diaryAttacker(attacker, itRule);
	}

	bool
		dead,
		converted;
	std::vector<BattleUnit*> convertedUnits;
	BattleUnit* defender;

	for (std::vector<BattleUnit*>::const_iterator
			i = _battleSave->getUnits()->begin();
			i != _battleSave->getUnits()->end();
			++i)
	{
		defender = *i; // <- a helpful label for ptrIterator.

		switch (defender->getUnitStatus())
		{
			case STATUS_DEAD:
			case STATUS_LATENT:
			case STATUS_LATENT_START:
				break;

			case STATUS_UNCONSCIOUS:
				if (defender->getHealth() != 0) break; // bypass unconscious units that aren't died yet.
				// no break;

			default:
				dead = defender->getHealth() == 0;
				converted = false;

				if (dead == false) // for converting infected units that aren't dead. Dead-conversions are handled in UnitDieBState.
				{
					if (defender->getSpawnType() == "STR_ZOMBIE") // human->zombie (nobody cares about zombie->chryssalid)
					{
						converted = true; // do morale-changes and SoldierDiary but not collapsing animations.
						convertedUnits.push_back(defender);
					}
					else if (defender->getHealth() > defender->getStun())
						break;
				}

				if (dead == true || converted == true)
				{
					if (attacker != nullptr) // attacker's Morale Bonus & diary ->
					{
						defender->killerFaction(attacker->getFaction()); // used in DebriefingState.

						if (attacker->getGeoscapeSoldier() != nullptr)
						{
							defender->setMurdererId(attacker->getId());

							diaryDefender(defender);
							attacker->getStatistics()->kills.push_back(new BattleUnitKill(
																					_killStatRank,
																					_killStatRace,
																					_killStatWeapon,
																					_killStatWeaponAmmo,
																					defender->getOriginalFaction(),
																					STATUS_DEAD,
																					_killStatMission,
																					_killStatTurn,
																					_killStatPoints));
						}

						if (attacker->isMoralable() == true)
							attackerMorale(attacker, defender);
					}

//					if (defender->getFaction() != FACTION_NEUTRAL)	// civie deaths now affect other Factions.
					factionMorale(defender, converted);				// cycle through units and do all faction

					if (defender->getUnitStatus() == STATUS_UNCONSCIOUS || converted == true)
						defender->instaKill(); // never send an unconscious/converted unit through UnitDieBState.
					else
					{
						DamageType dType;
						if (itRule != nullptr)
							dType = itRule->getDamageType();
						else if (hidden == true || terrain == true)
							dType = DT_HE;
						else
							dType = DT_NONE; // -> STR_HAS_DIED_FROM_A_FATAL_WOUND

						stateBPushNext(new UnitDieBState( // This is where units get sent to DEATH!
													this,
													defender,
													dType,
													hidden,
													hidden));
					}
				}
				else // recently stunned.
				{
					if (attacker != nullptr
						&& defender->beenStunned() == false) // credit first stunner only.
					{
						if (attacker->getGeoscapeSoldier() != nullptr)
						{
							diaryDefender(defender);
							attacker->getStatistics()->kills.push_back(new BattleUnitKill(
																					_killStatRank,
																					_killStatRace,
																					_killStatWeapon,
																					_killStatWeaponAmmo,
																					defender->getOriginalFaction(),
																					STATUS_UNCONSCIOUS,
																					_killStatMission,
																					_killStatTurn,
																					_killStatPoints >> 1u)); // half pts. for stun
						}

						if (attacker->isMoralable() == true)
							attackerMorale(attacker, defender, true);
					}

					factionMorale(defender, false, true); // cycle through units and do all faction

					if (defender->getGeoscapeSoldier() != nullptr)
						defender->getStatistics()->wasUnconscious = true;

					stateBPushNext(new UnitDieBState( // This is where units get sent to STUNNED.
												this,
												defender,
												DT_STUN,
												true));
				}
		} // end defender-Status switch.
	} // end BattleUnits loop.

	for (std::vector<BattleUnit*>::const_iterator
			i = convertedUnits.begin();
			i != convertedUnits.end();
			++i)
	{
		convertUnit(*i);
	}


	_parentState->hotWoundsRefresh();

	if (hidden == false)
	{
		if (_battleSave->getSide() == FACTION_PLAYER)
		{
			const BattleUnit* const unit (_battleSave->getSelectedUnit());
			_parentState->showPsiButton(unit != nullptr
									 && unit->getOriginalFaction() == FACTION_HOSTILE
									 && unit->getBattleStats()->psiSkill != 0
									 && unit->isOut_t() == false);
		}

//		if (_battleSave->getTacType() == TCT_BASEASSAULT // do this in SavedBattleGame::addDestroyedObjective()
//			&& _battleSave->getControlDestroyed() == false)
//		{
//			bool controlDestroyed = true;
//			for (size_t
//					i = 0;
//					i != _battleSave->getMapSizeXYZ();
//					++i)
//			{
//				if (_battleSave->getTiles()[i]->getMapData(O_OBJECT) != nullptr
//					&& _battleSave->getTiles()[i]->getMapData(O_OBJECT)->getTileType() == UFO_NAVIGATION)
//				{
//					controlDestroyed = false;
//					break;
//				}
//			}
//			if (controlDestroyed == true)
//			{
//				_battleSave->setControlDestroyed();
//				Game* const game = _parentState->getGame();
//				game->pushState(new InfoboxDialogState(game->getLanguage()->getString("STR_ALIEN_BASE_CONTROL_DESTROYED")));
//			}
//		}
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
 * Collects data about attacker for SoldierDiary.
 * @note Helper for checkCasualties().
 * @param attacker	- pointer to the attacker
 * @param itRule	- pointer to the weapon's rule
 */
void BattlescapeGame::diaryAttacker( // private.
		const BattleUnit* const attacker,
		const RuleItem* itRule)
{
	_killStatMission = static_cast<int>(_battleSave->getSavedGame()->getMissionStatistics()->size());
	_killStatTurn = _battleSave->getTurn() * 3 + static_cast<int>(_battleSave->getSide());

	if (itRule != nullptr)
	{
		_killStatWeapon =
		_killStatWeaponAmmo = itRule->getType();
	}
	else
	{
		_killStatWeapon =
		_killStatWeaponAmmo = "STR_WEAPON_UNKNOWN";
	}

	const BattleItem* item (attacker->getItem(ST_RIGHTHAND));
	if (item != nullptr)
	{
		itRule = item->getRules();
		for (std::vector<std::string>::const_iterator
				i = itRule->getCompatibleAmmo()->begin();
				i != itRule->getCompatibleAmmo()->end();
				++i)
		{
			if (*i == _killStatWeaponAmmo)
				_killStatWeapon = itRule->getType();
		}
	}

	if ((item = attacker->getItem(ST_LEFTHAND)) != nullptr)
	{
		itRule = item->getRules();
		for (std::vector<std::string>::const_iterator
				i = itRule->getCompatibleAmmo()->begin();
				i != itRule->getCompatibleAmmo()->end();
				++i)
		{
			if (*i == _killStatWeaponAmmo)
				_killStatWeapon = itRule->getType();
		}
	}
}

/**
 * Collects data about defender for SoldierDiary.
 * @note Helper for checkCasualties().
 * @param attacker - pointer to the defender
 */
void BattlescapeGame::diaryDefender(const BattleUnit* const defender) // private.
{
	_killStatPoints = defender->getValue();
	switch (defender->getOriginalFaction())
	{
		case FACTION_PLAYER:
			_killStatPoints = -_killStatPoints;
			if (defender->getGeoscapeSoldier() != nullptr)
			{
				_killStatRace = "STR_SOLDIER";
				_killStatRank = defender->getGeoscapeSoldier()->getRankString();
			}
			else
			{
				_killStatRace = defender->getUnitRules()->getRace();
				_killStatRank = "STR_SUPPORT";
			}
			break;

		case FACTION_HOSTILE:
			_killStatRace = defender->getUnitRules()->getRace();
			_killStatRank = defender->getUnitRules()->getRank();
			break;

		case FACTION_NEUTRAL:
			_killStatPoints = -(_killStatPoints << 1u);
			_killStatRace = "STR_HUMAN";
			_killStatRank = "STR_CIVILIAN";
	}
}

/**
 * Adjusts a BattleUnit's morale for making a kill.
 * @note Helper for checkCasualties().
 * @param attacker	- pointer to an attacker BattleUnit
 * @param defender	- pointer to a defender BattleUnit
 * @param half		- true for half-value if defender unconscious (default false)
 */
void BattlescapeGame::attackerMorale( // private.
		BattleUnit* const attacker,
		const BattleUnit* const defender,
		bool half) const
{
	int bonus;
	switch (attacker->getOriginalFaction())
	{
		case FACTION_PLAYER:
			bonus = _battleSave->getMoraleModifier();

			if (attacker->getGeoscapeSoldier() != nullptr
				&& attacker->isMindControlled() == false
				&& defender->getFaction() == FACTION_HOSTILE) // include neutralizing MC'd units
			{
				attacker->addTakedown();
			}
			break;

		case FACTION_HOSTILE:
			bonus = _battleSave->getMoraleModifier(nullptr, false);
			break;

		default:
		case FACTION_NEUTRAL:
			bonus = 0; // beware div-by-zero (not possible)
	}

	if ((defender->getOriginalFaction() == FACTION_PLAYER
			&& attacker->getOriginalFaction() == FACTION_HOSTILE)
		|| (defender->getOriginalFaction() == FACTION_HOSTILE
			&& attacker->getOriginalFaction() == FACTION_PLAYER))
	{
		if (half == false)
			attacker->moraleChange(bonus / 10);
		else
			attacker->moraleChange(bonus / 20);
	}
	else if (defender->getOriginalFaction() == FACTION_PLAYER
		&& attacker->getOriginalFaction() == FACTION_PLAYER)
	{
		int morale;
		if (defender->getGeoscapeSoldier() != nullptr)
			morale = 5000 / bonus;
		else
			morale = 2500 / bonus;

		if (half == true) morale >>= 1u;

		attacker->moraleChange(-morale);
	}
	else if (defender->getOriginalFaction() == FACTION_NEUTRAL)
	{
		switch (attacker->getOriginalFaction())
		{
			case FACTION_PLAYER:
				if (half == false)
					attacker->moraleChange(-2000 / bonus);
				else
					attacker->moraleChange(-1000 / bonus);
				break;

			case FACTION_HOSTILE:
				if (half == false)
					attacker->moraleChange(20);
				else
					attacker->moraleChange(10);
		}
	}
}

/**
 * Adjusts morale of units by faction when a BattleUnit dies.
 * @note Helper for checkCasualties().
 * @param defender	- pointer to a BattleUnit that died
 * @param converted	- true if unit was converted to another lifeform
 * @param half		- true for half-value if defender unconscious (default false)
 */
void BattlescapeGame::factionMorale( // private.
		const BattleUnit* const defender,
		bool converted,
		bool half) const
{
	// penalty for the death of a unit; civilians & MC'd aLien units return 100.
	const int loss (_battleSave->getMoraleModifier(defender));
	// These two are factions (aTeam & bTeam leaderships mitigate losses).
	int
		aTeam, // winners
		bTeam; // losers

	switch (defender->getOriginalFaction())
	{
		case FACTION_HOSTILE:
			aTeam = _battleSave->getMoraleModifier();
			bTeam = _battleSave->getMoraleModifier(nullptr, false);
			break;

		default:
		case FACTION_PLAYER:
		case FACTION_NEUTRAL:
			aTeam = _battleSave->getMoraleModifier(nullptr, false);
			bTeam = _battleSave->getMoraleModifier();
	}

	for (std::vector<BattleUnit*>::const_iterator // do bystander FACTION changes:
			j = _battleSave->getUnits()->begin();
			j != _battleSave->getUnits()->end();
			++j)
	{
		if ((*j)->isMoralable() == true)
		{
			if ((*j)->getOriginalFaction() == defender->getOriginalFaction()
				|| (defender->getOriginalFaction() == FACTION_PLAYER			// for death of xCom unit,
					&& (*j)->getOriginalFaction() == FACTION_NEUTRAL)			// civies take hit.
				|| (defender->getOriginalFaction() == FACTION_NEUTRAL			// for civie-death,
					&& (*j)->getFaction() == FACTION_PLAYER						// non-Mc'd xCom takes hit
					&& (*j)->getOriginalFaction() != FACTION_HOSTILE))			// but not Mc'd aLiens
			{
				// losing team(s) all get a morale loss
				// based on their individual Bravery & rank of unit that was killed
				int morale ((110 - (*j)->getBattleStats()->bravery) / 10);
				if (morale > 0) // pure safety, ain't gonna happen really.
				{
					morale = ((morale * loss) << 1u) / bTeam;
					if (converted == true)
						morale = (morale * 5 + 3) >> 2u; // extra loss if xCom or civie turns into a Zombie.
					else if (defender->getUnitRules() != nullptr)
					{
						if (defender->getUnitRules()->isMechanical() == true)
							morale >>= 2u;
						else
							morale >>= 1u;
					}

					if (half == false)
						(*j)->moraleChange(-morale);
					else
						(*j)->moraleChange(-(morale >> 1u));
				}
//				if (attacker
//					&& attacker->getFaction() == FACTION_PLAYER
//					&& defender->getFaction() == FACTION_HOSTILE)
//				{
//					attacker->setExposed(); // interesting
//					//Log(LOG_INFO) << ". . . . attacker Exposed";
//				}
			}
			else if ((((*j)->getOriginalFaction() == FACTION_PLAYER
						|| (*j)->getOriginalFaction() == FACTION_NEUTRAL)
					&& defender->getOriginalFaction() == FACTION_HOSTILE)
				|| ((*j)->getOriginalFaction() == FACTION_HOSTILE
					&& (defender->getOriginalFaction() == FACTION_PLAYER
						|| defender->getOriginalFaction() == FACTION_NEUTRAL)))
			{
				// winning faction(s) all get a morale boost unaffected by rank of the dead unit
				if (half == false)
					(*j)->moraleChange(aTeam / 10);
				else
					(*j)->moraleChange(aTeam / 20);
			}
		}
	}
}

/**
 * Shows the infoboxes in the queue if any.
 */
void BattlescapeGame::showInfoBoxQueue() // private.
{
	for (std::vector<InfoboxDialogState*>::const_iterator
			i = _infoboxQueue.begin();
			i != _infoboxQueue.end();
			++i)
	{
		_parentState->getGame()->pushState(*i);
	}
	_infoboxQueue.clear();
}

/**
 * Checks against reserved-TU.
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
		return tu <= unit->getTu();
	}


	BattleActionType batReserved; // avoid changing _batReserved here.

	if (_battleSave->getSide() == FACTION_HOSTILE && _debugPlay == false)
	{
		const AlienBAIState* const aiState (dynamic_cast<AlienBAIState*>(unit->getAIState()));
		if (aiState != nullptr)
			batReserved = aiState->getReservedAiAction();
		else
			batReserved = BA_NONE;	// something went ... wrong. Should always be an AI for non-player units (although i
									// guess it could-maybe-but-unlikely be a CivilianBAIState here in checkReservedTu()).

		// This could use some tweaking, for the poor aLiens:
//		const int extraReserve (RNG::generate(0,13)); // unfortunately the state-machine may cause an unpredictable quantity of calls to this ... via UnitWalkBState::think().
		int tuReserved;

		switch (batReserved) // aLiens reserve TUs as a percentage rather than just enough for a single action.
		{
			case BA_SNAPSHOT:
				tuReserved = unit->getBattleStats()->tu / 3;// + extraReserve;			// 33%
				break;

			case BA_AUTOSHOT:
				tuReserved = (unit->getBattleStats()->tu << 1u) / 5;// + extraReserve;	// 40%
				break;

			case BA_AIMEDSHOT:
				tuReserved = (unit->getBattleStats()->tu >> 1u);// + extraReserve;		// 50%
				break;

			default:
				tuReserved = 0;
		}
		return (tu + tuReserved <= unit->getTu());
	}

	// ** Below here is for xCom units exclusively ***
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
			weapon = unit->getMainHandWeapon();
	}

	if (weapon != nullptr)
	{
		const RuleItem* const itRule (weapon->getRules());
		switch (itRule->getBattleType())
		{
			case BT_MELEE:
				batReserved = BA_MELEE;
				break;

			case BT_FIREARM:
				if (itRule->getSnapTu() != 0)
					batReserved = BA_SNAPSHOT;
				else if (itRule->getAutoTu() != 0)
					batReserved = BA_AUTOSHOT;
				else if (itRule->getAimedTu() != 0)
					batReserved = BA_AIMEDSHOT;
		}
	}

	if (tu + unit->getActionTu(batReserved, weapon) <= unit->getTu()) // safeties in place @ getActionTu()
		return true;

	return false;
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
		&& tu + tuKneel + unit->getActionTu(batReserved, weapon) > unit->getTu()
		&& (test == true || tuKneel + unit->getActionTu(batReserved, weapon) <= unit->getTu()))
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
 * Handles a panicking BattleUnit.
 * @note Called from BattlescapeGame::think() at start of non-player turn.
 * @return, true if unit is panicking
 */
bool BattlescapeGame::handlePanickingUnit(BattleUnit* const unit) // private.
{
	//Log(LOG_INFO) << "bg::handlePanickingUnit() id-" << unit->getId();
	const UnitStatus status (unit->getUnitStatus());
	switch (status)
	{
		case STATUS_PANICKING:
		case STATUS_BERSERK:
			_parentState->getMap()->setSelectorType(CT_NONE);
			_battleSave->setSelectedUnit(unit);

			if (Options::battleAlienPanicMessages == true
				|| unit->getUnitVisible() == true)
			{
				//Log(LOG_INFO) << "bg: panic id-" << unit->getId();
				getMap()->getCamera()->centerPosition(
												unit->getPosition(),
												(unit->getUnitVisible() == true)); // NOTE: Should not have to redraw Map.

				Game* const game (_parentState->getGame());
				std::string st;
				switch (status)
				{
					default:
					case STATUS_PANICKING: st = "STR_HAS_PANICKED"; break;
					case STATUS_BERSERK:   st = "STR_HAS_GONE_BERSERK";
				}
				game->pushState(new InfoboxState(
											game->getLanguage()->getString(st, unit->getGender())
																.arg(unit->getName(game->getLanguage()))));
			}

			if (unit->getAIState() != nullptr)
			{
				switch (unit->getOriginalFaction())
				{
					case FACTION_PLAYER:  unit->setAIState(); break;
					case FACTION_HOSTILE:
					case FACTION_NEUTRAL: unit->getAIState()->resetAI();
				}
			}

			unit->setUnitStatus(STATUS_STANDING);

			BattleAction action;
			action.actor = unit;
			int tu (unit->getTu());

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
							dropItem(item, unit->getPosition(), DROP_FROMINVENTORY);
					}

					if (RNG::percent(75) == true)
					{
						item = unit->getItem(ST_LEFTHAND);
						if (item != nullptr)
							dropItem(item, unit->getPosition(), DROP_FROMINVENTORY);
					}

					unit->flagCache();

					Pathfinding* const pf (_battleSave->getPathfinding());
					pf->setPathingUnit(unit);

					const std::vector<size_t> reachable (pf->findReachable(unit, tu));
					const size_t tileId (reachable[RNG::pick(reachable.size())]); // <-- WARNING: no Safety on size !

					_battleSave->tileCoords(
										tileId,
										&action.posTarget.x,
										&action.posTarget.y,
										&action.posTarget.z);
					pf->calculatePath(
								action.actor,
								action.posTarget,
								tu);
					if (pf->getStartDirection() != -1)
					{
						action.actor->setDashing();
						action.dash = true;
						action.type = BA_MOVE;
						stateBPushBack(new UnitWalkBState(this, action));
					}
					break;
				}

				case STATUS_BERSERK:
				{
					//Log(LOG_INFO) << ". BERSERK";
					action.type = BA_TURN;
					const int pivots (RNG::generate(2,5));
					for (int
							i = 0;
							i != pivots;
							++i)
					{
						action.value = -1;
						action.posTarget = Position(
												unit->getPosition().x + RNG::generate(-5,5),
												unit->getPosition().y + RNG::generate(-5,5),
												unit->getPosition().z);
						stateBPushBack(new UnitTurnBState(this, action, false));
					}

					action.weapon = unit->getRangedWeapon(true);	// unit->getMainHandWeapon(true); unit->getMeleeWeapon();
					if (action.weapon == nullptr)					// TODO: implement Charge + Melee against nearest unit.
						action.weapon = unit->getGrenade();

					if (action.weapon != nullptr)
					{
						switch (action.weapon->getRules()->getBattleType())
						{
							case BT_FIREARM:
								action.posTarget = _battleSave->getTiles()[RNG::pick(_battleSave->getMapSizeXYZ())]->getPosition();
								if (_battleSave->getTile(action.posTarget) != nullptr)
								{
									action.value = -1;
									stateBPushBack(new UnitTurnBState(this, action, false));

									action.type = BA_SNAPSHOT;
									if (action.weapon->getAmmoItem() == nullptr
										|| action.weapon->getAmmoItem()->getRules()->getShotgunPellets() == 0)
									{
										action.posCamera = _battleSave->getBattleState()->getMap()->getCamera()->getMapOffset();
									}
									else
										action.posCamera = Position(0,0,-1);

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
										stateBPushBack(new ProjectileFlyBState(this, action));
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
									action.posTarget = Position(
															unit->getPosition().x + RNG::generate(-20,20),
															unit->getPosition().y + RNG::generate(-20,20),
															unit->getPosition().z);

									if (_battleSave->getTile(action.posTarget) != nullptr)
									{
										action.value = -1;
										stateBPushBack(new UnitTurnBState(this, action, false));

										const Position
											originVoxel (getTileEngine()->getOriginVoxel(action)),
											targetVoxel (Position::toVoxelSpaceCentered(
																					action.posTarget,
																					FLOOR_TLEVEL - _battleSave->getTile(action.posTarget)->getTerrainLevel()));

										if (getTileEngine()->validateThrow(
																		action,
																		originVoxel,
																		targetVoxel) == true)
										{
											action.type = BA_THROW;
											action.posCamera = _battleSave->getBattleState()->getMap()->getCamera()->getMapOffset();
											stateBPushBack(new ProjectileFlyBState(this, action));
											break;
										}
									}
								}
						}
					}

					action.type = BA_NONE;
				}
			}

			stateBPushBack(new UnitPanicBState(this, unit)); // <- ends Panic/Berserk.
			return true;
	}
	return false;
}

/**
 * Cancels the current action the Player has selected (firing, throwing, etc).
 * @note The return is used by BattlescapeState::mapClick() to check if
 * pathPreview was cancelled or walking was aborted and by
 * BattlescapeState::btnBattleOptionsClick() to let the Esc-key remove
 * pathPreview or cancel walking/targeting.
 * @param force - force the action to be cancelled (default false)
 * @return, true if anything was cancelled
 */
bool BattlescapeGame::cancelTacticalAction(bool force)
{
	if (Options::battlePreviewPath == PATH_NONE
		|| _battleSave->getPathfinding()->clearPreview() == false)
	{
		if (_battleStates.empty() == true || force == true)
		{
			if (_playerAction.targeting == true)
			{
				if (_playerAction.type == BA_LAUNCH
					&& _playerAction.waypoints.empty() == false)
				{
					_playerAction.waypoints.pop_back();

					if (getMap()->getWaypoints()->empty() == false)
						getMap()->getWaypoints()->pop_back();

					if (_playerAction.waypoints.empty() == true)
						_parentState->showLaunchButton(false);
				}
				else
				{
					_playerAction.targeting = false;
					_playerAction.type = BA_NONE;

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
 * @note This is for Player's units only.
 * @return, pointer to BattleAction
 */
BattleAction* BattlescapeGame::getTacticalAction()
{
	return &_playerAction;
}

/**
 * Checks whether a BattleState is currently in progress.
 * @return, true or false
 */
bool BattlescapeGame::isBusy() const
{
	return (_battleStates.empty() == false);
}

/**
 * Left-click activates a primary action.
 * @param pos - reference to a Position on the Map
 */
void BattlescapeGame::primaryAction(const Position& pos)
{
	BattleUnit* const targetUnit (_battleSave->getTile(pos)->getTileUnit());

	_playerAction.actor = _battleSave->getSelectedUnit();
	if (_playerAction.actor != nullptr && _playerAction.targeting == true)
	{
		_playerAction.strafe = false;

		switch (_playerAction.type)
		{
			case BA_LAUNCH:
				if (static_cast<int>(_playerAction.waypoints.size()) < _playerAction.weapon->getRules()->isWaypoints())
				{
					_parentState->showLaunchButton();
					_playerAction.waypoints.push_back(pos);
					getMap()->getWaypoints()->push_back(pos);
				}
				break;

			case BA_USE:
				if (_playerAction.weapon->getRules()->getBattleType() == BT_MINDPROBE)
				{
					if (targetUnit != nullptr
						&& targetUnit->getFaction() != _playerAction.actor->getFaction()
						&& targetUnit->getUnitVisible() == true)
					{
						if (_playerAction.actor->expendTu(_playerAction.TU) == true)
						{
							if (_playerAction.weapon->getRules()->isLosRequired() == false
								|| std::find(
										_playerAction.actor->getHostileUnits().begin(),
										_playerAction.actor->getHostileUnits().end(),
										targetUnit) != _playerAction.actor->getHostileUnits().end())
							{
								if (TileEngine::distance(
													_playerAction.actor->getPosition(),
													_playerAction.posTarget) <= _playerAction.weapon->getRules()->getMaxRange())
								{
										const int soundId (_playerAction.weapon->getRules()->getFireHitSound());
										if (soundId != -1)
											getResourcePack()->getSound("BATTLE.CAT", static_cast<unsigned>(soundId))
																->play(-1, getMap()->getSoundAngle(pos));

										_parentState->getGame()->pushState(new UnitInfoState(
																						targetUnit,
																						_parentState,
																						false, true));
										_parentState->getGame()->getScreen()->fadeScreen();
								}
								else
									_parentState->warning(BattlescapeGame::PLAYER_ERROR[5u]); // out of range
							}
							else
								_parentState->warning(BattlescapeGame::PLAYER_ERROR[6u]); // no LoF
						}
						else
						{
							cancelTacticalAction();
							_parentState->warning(BattlescapeGame::PLAYER_ERROR[0u]); // no TU
						}
					}
				}
				break;

			case BA_PSIPANIC:
			case BA_PSICONTROL:
			case BA_PSICONFUSE:
			case BA_PSICOURAGE:
				if (targetUnit != nullptr
					&& targetUnit->getUnitVisible() == true
					&& (   (_playerAction.type != BA_PSICOURAGE && targetUnit->getFaction() != FACTION_PLAYER)
						|| (_playerAction.type == BA_PSICOURAGE && targetUnit->getFaction() != FACTION_HOSTILE))) // NOTE: This allows a unit to encourage itself.
				{
					bool aLienPsi (_playerAction.weapon == nullptr);
					if (aLienPsi == true)
						_playerAction.weapon = _alienPsi; // ie. Player is using an MC'd aLien to do a panick-atk.

					_playerAction.TU = _playerAction.actor->getActionTu(
																	_playerAction.type,
																	_playerAction.weapon);

					if (_playerAction.actor->getTu() >= _playerAction.TU)
					{
						if (_playerAction.weapon->getRules()->isLosRequired() == false
							|| std::find(
									_playerAction.actor->getHostileUnits().begin(),
									_playerAction.actor->getHostileUnits().end(),
									targetUnit) != _playerAction.actor->getHostileUnits().end())
						{
							_playerAction.posTarget = pos;

							if (TileEngine::distance(
												_playerAction.actor->getPosition(),
												_playerAction.posTarget) <= _playerAction.weapon->getRules()->getMaxRange())
							{
									_playerAction.posCamera = Position(0,0,-1);

									stateBPushBack(new ProjectileFlyBState(this, _playerAction)); // TODO: Clear out the redundancy that occurs in ProjFlyB::init().

									if (getTileEngine()->psiAttack(&_playerAction) == true)
									{
										std::string st;
										switch (_playerAction.type)
										{
											default:
											case BA_PSIPANIC:	st = "STR_PSI_PANIC_SUCCESS";	break;
											case BA_PSICONTROL:	st = "STR_PSI_CONTROL_SUCCESS";	break;
											case BA_PSICONFUSE:	st = "STR_PSI_CONFUSE_SUCCESS";	break;
											case BA_PSICOURAGE:	st = "STR_PSI_COURAGE_SUCCESS";
										}
										Game* const game (_parentState->getGame());
										game->pushState(new InfoboxState(game->getLanguage()->getString(st).arg(_playerAction.value)));

										_parentState->updateSoldierInfo(false);
									}
							}
							else
								_parentState->warning(BattlescapeGame::PLAYER_ERROR[5u]); // out of range
						}
						else
							_parentState->warning(BattlescapeGame::PLAYER_ERROR[6u]); // no LoF
					}
					else
					{
						cancelTacticalAction();
						_parentState->warning(BattlescapeGame::PLAYER_ERROR[0u]); // no TU
					}


					if (aLienPsi == true)
						_playerAction.weapon = nullptr;
				}
				break;

			case BA_SNAPSHOT:
			case BA_AUTOSHOT:
			case BA_AIMEDSHOT:
			case BA_THROW:
				getMap()->setSelectorType(CT_NONE);
				_parentState->getGame()->getCursor()->setHidden();

				_playerAction.posTarget = pos;
				if (_playerAction.type == BA_THROW
					|| _playerAction.weapon->getAmmoItem() == nullptr
					|| _playerAction.weapon->getAmmoItem()->getRules()->getShotgunPellets() == 0)
				{
					_playerAction.posCamera = getMap()->getCamera()->getMapOffset();
				}
				else
					_playerAction.posCamera = Position(0,0,-1);

				_battleStates.push_back(new ProjectileFlyBState(this, _playerAction));	// TODO: should check for valid LoF/LoT *before* invoking this
																						// instead of the (flakey) checks in that state. Then conform w/ AI ...
				_playerAction.value = -1;
				stateBPushFront(new UnitTurnBState(this, _playerAction));
		}
	}
	else if (targetUnit != nullptr && targetUnit != _playerAction.actor
		&& (targetUnit->getUnitVisible() == true || _debugPlay == true))
	{
		if (targetUnit->getFaction() == _battleSave->getSide())
		{
			_battleSave->setSelectedUnit(targetUnit);
			_parentState->updateSoldierInfo(false); // try no calcFov()

			cancelTacticalAction();
			_playerAction.actor = targetUnit;

			setupSelector();
		}
	}
	else if (playableUnitSelected() == true)
	{
		bool allowPreview (Options::battlePreviewPath != PATH_NONE);

		Pathfinding* const pf (_battleSave->getPathfinding());
		pf->setPathingUnit(_playerAction.actor);

		const bool ctrl ((SDL_GetModState() & KMOD_CTRL) != 0);

		if (ctrl == true
			&& targetUnit != nullptr
			&& targetUnit == _playerAction.actor
			&& _playerAction.actor->getArmor()->getSize() == 1)
		{
			if (allowPreview == true)
				pf->clearPreview();

			Position
				pxScreen,
				pxPointer;

			getMap()->getCamera()->convertMapToScreen(pos, &pxScreen);
			pxScreen += getMap()->getCamera()->getMapOffset();

			getMap()->findMousePointer(pxPointer);

			if (pxPointer.x > pxScreen.x + 16)
				_playerAction.actor->setTurnDirection(-1);
			else
				_playerAction.actor->setTurnDirection(+1);

			_playerAction.value = (_playerAction.actor->getUnitDirection() + 4) % 8;

			stateBPushBack(new UnitTurnBState(this, _playerAction));
		}
		else
		{
			const bool alt ((SDL_GetModState() & KMOD_ALT)  != 0);

//			bool zPath;
//			const Uint8* const keystate (SDL_GetKeyState(nullptr));
//			if (keystate[SDLK_z] != 0)
//				zPath = true;
//			else
//				zPath = false;

			if (allowPreview == true
				&& (_playerAction.posTarget != pos
					|| pf->isModCtrl() != ctrl
					|| pf->isModAlt() != alt))
//					|| pf->isZPath() != zPath))
			{
				pf->clearPreview();
			}

			_playerAction.posTarget = pos;
			pf->calculatePath(
						_playerAction.actor,
						_playerAction.posTarget);

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

					stateBPushBack(new UnitWalkBState(this, _playerAction));
				}
			}
		}
	}
}

/**
 * Right-click activates a secondary action.
 * @param pos - reference a Position on the Map
 */
void BattlescapeGame::secondaryAction(const Position& pos)
{
	_playerAction.actor = _battleSave->getSelectedUnit();
	if (_playerAction.actor->getPosition() != pos)
	{
		_playerAction.value = -1;
		_playerAction.posTarget = pos;
		_playerAction.strafe = _playerAction.actor->getTurretType() != TRT_NONE
							&& (SDL_GetModState() & KMOD_CTRL) != 0
							&& Options::battleStrafe == true;

		stateBPushBack(new UnitTurnBState(this, _playerAction)); // turn, rotate turret, or open door
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
	_playerAction.posTarget = _playerAction.waypoints.front();

	getMap()->setSelectorType(CT_NONE);
	_parentState->getGame()->getCursor()->setHidden();

//	_playerAction.posCamera = getMap()->getCamera()->getMapOffset();
	_playerAction.value = -1;

	_battleStates.push_back(new ProjectileFlyBState(this, _playerAction));
	stateBPushFront(new UnitTurnBState(this, _playerAction));
}

/**
 * Handler for the psi button.
 * @note Additional data gets assigned in primaryAction().
 */
void BattlescapeGame::psiButtonAction()
{
	_playerAction.weapon = nullptr;
	_playerAction.targeting = true;
	_playerAction.type = BA_PSIPANIC;

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
	_playerAction.posTarget = unit->getPosition();
	switch (dir)
	{
		case Pathfinding::DIR_UP:
			++_playerAction.posTarget.z;
			break;
		case Pathfinding::DIR_DOWN:
			--_playerAction.posTarget.z;
	}

	getMap()->setSelectorType(CT_NONE);
	_parentState->getGame()->getCursor()->setHidden();

	Pathfinding* const pf (_battleSave->getPathfinding());
	pf->calculatePath(
				_playerAction.actor,
				_playerAction.posTarget);

	stateBPushBack(new UnitWalkBState(this, _playerAction));
}

/**
 * The Player requests to end the turn.
 * @note Waits for explosions etc to really end the turn.
 */
void BattlescapeGame::requestEndTurn()
{
	//Log(LOG_INFO) << "bg:requestEndTurn()";
	cancelTacticalAction();

	if (_endTurnRequested == false)	// unnecessary.
	{
		_endTurnRequested = true;
		stateBPushBack();			// think() will pop as many NULL states as are stacked.
	}
}

/**
 * Drops an item to the floor and affects it with gravity then recalculates FoV
 * if it's a light-source.
 * @param item		- pointer to a BattleItem
 * @param pos		- reference to a Position to place the item
 * @param dropType	- how to handle this drop (BattlescapeGame.h) (default DROP_STANDARD)
 *					  0 - do nothing special here
 *					  1 - clear the owner (not used currently)
 *					  2 - clear the owner & remove the item from dropper's inventory
 *					  3 - as a newly created item on the battlefield that needs
 *						  to be added to the battleSave's item-list
 */
void BattlescapeGame::dropItem(
		BattleItem* const item,
		const Position& pos,
		DropType dropType)
{
	if (_battleSave->getTile(pos) != nullptr
		&& item->getRules()->isFixed() == false)
	{
		item->setInventorySection(getRuleset()->getInventoryRule(ST_GROUND));
		_battleSave->getTile(pos)->addItem(item);

		if (item->getItemUnit() != nullptr)
			item->getItemUnit()->setPosition(pos);

		switch (dropType)
		{
//			case DROP_STANDARD:
//				break;
			case DROP_CLEAROWNER: item->setOwner();
				break;
			case DROP_FROMINVENTORY: item->changeOwner();
				break;
			case DROP_CREATE: _battleSave->getItems()->push_back(item);
		}

		if (pos.z != 0)
			getTileEngine()->applyGravity(_battleSave->getTile(pos));

		if (item->getRules()->getBattleType() == BT_FLARE
			&& item->getFuse() != -1)
		{
			getTileEngine()->calculateTerrainLighting();
			getTileEngine()->calcFovUnits_all(true);
		}
	}
}

/**
 * Drops all items in a specified BattleUnit's inventory to the ground.
 * @param unit - pointer to a BattleUnit
 */
void BattlescapeGame::dropUnitInventory(BattleUnit* const unit)
{
	const Position pos (unit->getPosition());
	if (_battleSave->getTile(pos) != nullptr)
	{
		bool calcFoV (false);
		for (std::vector<BattleItem*>::const_iterator
				i = unit->getInventory()->begin();
				i != unit->getInventory()->end();
				++i)
		{
			if ((*i)->getRules()->isFixed() == false)
			{
				(*i)->setOwner();
				(*i)->setInventorySection(getRuleset()->getInventoryRule(ST_GROUND));
				_battleSave->getTile(pos)->addItem(*i);

				if ((*i)->getItemUnit() != nullptr)
					(*i)->getItemUnit()->setPosition(pos);

				if ((*i)->getRules()->getBattleType() == BT_FLARE
					&& (*i)->getFuse() != -1)
				{
					calcFoV = true;
				}
			}
		}
		unit->getInventory()->clear();

		if (pos.z != 0)
			getTileEngine()->applyGravity(_battleSave->getTile(pos));

		if (calcFoV == true)
		{
			getTileEngine()->calculateTerrainLighting();
			getTileEngine()->calcFovUnits_all(true);
		}
	}
}

/**
 * Converts a BattleUnit into a different type of BattleUnit.
 * @param potato - pointer to a BattleUnit to convert
 * @return, pointer to the converted BattleUnit
 */
BattleUnit* BattlescapeGame::convertUnit(BattleUnit* potato)
{
	//Log(LOG_INFO) << "BattlescapeGame::convertUnit() " << conType;
	const bool antecedentVisibility (potato->getUnitVisible());

	_battleSave->getBattleState()->showPsiButton(false);

	switch (potato->getUnitStatus())
	{
		case STATUS_UNCONSCIOUS:
			potato->setUnitStatus(STATUS_DEAD);
			potato->setHealth(0);
			// no break;
		case STATUS_DEAD:
			_battleSave->deleteBody(potato);
			break;

		default:
			potato->instaKill();
	}

	potato->setSpecialAbility(SPECAB_NONE);

	dropUnitInventory(potato);

	potato->setUnitTile();										// TODO: Run potato through putDown().
	_battleSave->getTile(potato->getPosition())->setTileUnit();	// NOTE: This could, theoretically, be a large potato.


	RuleUnit* const unitRule (getRuleset()->getUnitRule(potato->getSpawnType()));
	const Position& pos (potato->getPosition());
	potato = new BattleUnit(
						unitRule,
						FACTION_HOSTILE,
						_battleSave->getUnits()->back()->getId() + 1,
						getRuleset()->getArmor(unitRule->getArmorType()),
						_parentState->getGame()->getSavedGame()->getDifficulty(),
						_parentState->getGame()->getSavedGame()->getMonthsPassed(),
						this);

	_battleSave->getTile(pos)->setTileUnit(
										potato,
										_battleSave->getTile(pos + Position(0,0,-1)));
	potato->setPosition(pos);

	int dir;
	if (potato->isZombie() == true)
		dir = RNG::generate(0,7); // or, (potato->getUnitDirection())
	else
		dir = 3;
	potato->setUnitDirection(dir);
	potato->setTu();

	_battleSave->getUnits()->push_back(potato);

	AlienBAIState* const aiState (new AlienBAIState(_battleSave, potato));
	aiState->init();
	potato->setAIState(aiState);

	const RuleItem* const itRule (getRuleset()->getItemRule(unitRule->getRace().substr(4u) + "_WEAPON"));
	if (itRule != nullptr)
	{
		BattleItem* const weapon (new BattleItem(
											itRule,
											_battleSave->getCanonicalBattleId()));
		weapon->changeOwner(potato);
		weapon->setInventorySection(getRuleset()->getInventoryRule(ST_RIGHTHAND));

		_battleSave->getItems()->push_back(weapon);
	}

	getMap()->cacheUnitSprite(potato);
	potato->setUnitVisible(antecedentVisibility);

	getTileEngine()->applyGravity(potato->getUnitTile());
	getTileEngine()->calculateUnitLighting();
	getTileEngine()->calcFovUnits_pos(potato->getPosition(), true);

	return potato;
}

/**
 * Gets the battlefield Map.
 * @return, pointer to Map
 */
Map* BattlescapeGame::getMap() const
{
	return _parentState->getMap();
}

/**
 * Gets the SavedBattleGame data object.
 * @return, pointer to SavedBattleGame
 */
SavedBattleGame* BattlescapeGame::getBattleSave() const
{
	return _battleSave;
}

/**
 * Gets the TileEngine.
 * @return, pointer to TileEngine
 */
TileEngine* BattlescapeGame::getTileEngine() const
{
	return _battleSave->getTileEngine();
}

/**
 * Gets Pathfinding.
 * @return, pointer to Pathfinding
 */
Pathfinding* BattlescapeGame::getPathfinding() const
{
	return _battleSave->getPathfinding();
}

/**
 * Gets the ResourcePack.
 * @return, pointer to ResourcePack
 */
ResourcePack* BattlescapeGame::getResourcePack() const
{
	return _parentState->getGame()->getResourcePack();
}

/**
 * Gets the Ruleset.
 * @return, pointer to Ruleset
 */
const Ruleset* BattlescapeGame::getRuleset() const
{
	return _parentState->getGame()->getRuleset();
}

/**
 * Tries to find a BattleItem and pick it up if possible.
 * @note Called by handleUnitAI().
 * @param aiAction - pointer to an AI-BattleAction struct
 * @return, true if an item was actually picked up
 */
bool BattlescapeGame::pickupItem(BattleAction* const aiAction) const
{
	//Log(LOG_INFO) << "BattlescapeGame::findItem()";
	BattleItem* const targetItem (surveyItems(aiAction->actor));
	if (targetItem != nullptr)
	{
		//Log(LOG_INFO) << ". found " << targetItem->getRules()->getType();
//		if (targetItem->getTile()->getPosition() == aiAction->actor->getPosition())
		if (targetItem->getItemTile() == aiAction->actor->getUnitTile())
		{
			//Log(LOG_INFO) << ". . pickup on spot";
			if (takeItemFromGround(targetItem, aiAction->actor) == true
				&& targetItem->getRules()->getBattleType() == BT_FIREARM
				&& targetItem->getAmmoItem() == nullptr)
			{
				//Log(LOG_INFO) << ". . . check Ammo.";
				aiAction->actor->checkReload();
			}
			return true;
		}
		else
		{
			//Log(LOG_INFO) << ". . move to spot";
			aiAction->type = BA_MOVE;
			aiAction->posTarget = targetItem->getItemTile()->getPosition();
		}
	}
	return false;
}

/**
 * Searches through BattleItems on the Map [that were dropped on an alien turn]
 * and picks the most attractive one.
 * @param unit - pointer to the BattleUnit looking for an item
 * @return, the item to go for
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
			if ((tile = (*i)->getItemTile()) != nullptr
				&& (tile->getTileUnit() == nullptr || tile->getTileUnit() == unit)
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
			worth (0),
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
											(*i)->getItemTile()->getPosition()) + 1);
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
 * Assesses whether a BattleItem is worth trying to pick up.
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

	bool fit (false);
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
//				if (item->getAmmoItem() == nullptr) // not loaded
//				{
//					for (std::vector<BattleItem*>::const_iterator
//							i = unit->getInventory()->begin();
//							i != unit->getInventory()->end();
//							++i)
//					{
//						if ((*i)->getRules()->getBattleType() == BT_AMMO)
//						{
//							for (std::vector<std::string>::const_iterator
//									j = item->getRules()->getCompatibleAmmo()->begin();
//									j != item->getRules()->getCompatibleAmmo()->end();
//									++j)
//							{
//								if (*j == (*i)->getRules()->getName())
//								{
//									//Log(LOG_INFO) << ". ret [2] TRUE";
//									return true;
//								}
//							}
//						}
//					}
//				}

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
	const int cost (grdRule->getCost(rhRule));

	if (unit->getTu() >= cost && takeItem(item, unit) == true)
	{
		unit->expendTu(cost);
		item->getItemTile()->removeItem(item);
		//Log(LOG_INFO) << ". ret TRUE";
		return true;
	}

	//Log(LOG_INFO) << ". ret FALSE";
	return false;
}

/**
 * Tries to fit an item into a BattleUnit's inventory.
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

	const RuleItem* const itRule (item->getRules());
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
			item->changeOwner(unit);
			// no break;
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
 *						  including MC'd aLiens and MC'd xCom units
 * @param livePlayer	- reference in which to store the live xCom tally
 *						  excluding MC'd xCom units and MC'd aLiens
 * @return, true if all the aLiens are dead or MC'd (aka. pacified)
 */
bool BattlescapeGame::tallyUnits(
		int& liveHostile,
		int& livePlayer) const
{
	bool ret (true);

	liveHostile =
	livePlayer = 0;

	for (std::vector<BattleUnit*>::const_iterator
			i = _battleSave->getUnits()->begin();
			i != _battleSave->getUnits()->end();
			++i)
	{
		if ((*i)->getUnitStatus() == STATUS_STANDING)
		{
			switch ((*i)->getOriginalFaction())
			{
				case FACTION_PLAYER:
					if ((*i)->isMindControlled() == false)
						++livePlayer;
					else
						++liveHostile;
					break;

				case FACTION_HOSTILE:
					++liveHostile;
					if ((*i)->isMindControlled() == false)
						ret = false;
			}
		}
	}

	if (livePlayer == 0 && liveHostile == 0) // adjudicate: Player Victory.
		livePlayer = -1;

	//Log(LOG_INFO) << "bg:tallyUnits() ret= " << ret << " soldiers= " << livePlayer << " aLiens= " << liveHostile;
	return ret;
}

/**
 * Tallies the conscious player-units at an Exit-area unless MC'd.
 * @return, quantity of units at exit
 */
int BattlescapeGame::tallyPlayerExit() const
{
	int ret (0);
	for (std::vector<BattleUnit*>::const_iterator
			i = _battleSave->getUnits()->begin();
			i != _battleSave->getUnits()->end();
			++i)
	{
		if ((*i)->getOriginalFaction() == FACTION_PLAYER
			&& (*i)->isMindControlled() == false
			&& (*i)->isOnTiletype(END_POINT) == true) // NOTE: Unit will be Status_Standing to be on tile.
		{
			++ret;
		}
	}
	return ret;
}

/**
 * Tallies conscious hostile-units including MC'd aLiens.
 * @return, quantity of hostiles
 */
int BattlescapeGame::tallyHostiles() const
{
	int ret (0);
	for (std::vector<BattleUnit*>::const_iterator
			i = _battleSave->getUnits()->begin();
			i != _battleSave->getUnits()->end();
			++i)
	{
		if ((*i)->getOriginalFaction() == FACTION_HOSTILE
			&& (*i)->getUnitStatus() == STATUS_STANDING)
		{
			++ret;
		}
	}
	return ret;
}

/**
 * Checks if a unit has moved next to a proximity grenade.
 * @note Checks one tile around the unit in every direction. For a large unit
 * check around every tile the unit occupies.
 * @param unit - pointer to a BattleUnit
 * @return, true if a proximity grenade is triggered
 */
bool BattlescapeGame::checkProxyGrenades(BattleUnit* const unit)
{
	Tile* tile;
	Position pos;

	const int unitSize (unit->getArmor()->getSize() - 1);
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
					dx = -1;
					dx != 2;
					++dx)
			{
				for (int
						dy = -1;
						dy != 2;
						++dy)
				{
					tile = _battleSave->getTile(unit->getPosition()
													   + Position( x, y, 0)
													   + Position(dx,dy, 0));
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
								Pathfinding::vectorToDirection(
															Position(dx,dy,0),
															dir);
								if (_battleSave->getPathfinding()->isBlockedPath(
																			_battleSave->getTile(unit->getPosition() + Position(x,y,0)),
																			dir,
																			unit) == false)	// try passing in OBJECT_SELF as a missile target to kludge for closed doors.
								{															// there *might* be a problem if the Proxy is on a non-walkable tile ....
									pos = Position::toVoxelSpaceCentered(
																	tile->getPosition(),
																	FLOOR_TLEVEL - (tile->getTerrainLevel()));
									stateBPushNext(new ExplosionBState(
																	this,
																	pos,
																	(*i)->getRules(),
																	(*i)->getPriorOwner()));
									_battleSave->toDeleteItem(*i);

									unit->flagCache();
									getMap()->cacheUnitSprite(unit);
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
 * Deletes all BattleStates that are waiting for deletion.
 */
void BattlescapeGame::cleanBattleStates()
{
	for (std::list<BattleState*>::const_iterator
			i = _deletedStates.begin();
			i != _deletedStates.end();
			++i)
	{
		delete *i;
	}
	_deletedStates.clear();
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
	const Game* const game (_parentState->getGame());
	const RuleAlienDeployment* const ruleDeploy (game->getRuleset()->getDeployment(_battleSave->getTacticalType()));
	if (ruleDeploy != nullptr)
	{
		const std::string messagePop (ruleDeploy->getObjectivePopup());
		if (messagePop.empty() == false)
			_infoboxQueue.push_back(new InfoboxDialogState(game->getLanguage()->getString(messagePop)));
	}
}

/**
 * Gets if a coup-de-grace action is underway and needs to be animated.
 * @return, true if execute
 */
bool BattlescapeGame::getLiquidate() const
{
	return _executeProgress;
}

/**
 * Finishes a coup-de-grace action.
 */
void BattlescapeGame::endLiquidate()
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
 * Gets if a shotgun blast is underway and needs to be animated.
 * @return, true if shotgun
 */
bool BattlescapeGame::getShotgun() const
{
	return _shotgunProgress;
}

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
 * Gets the action type that is reserved.
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
