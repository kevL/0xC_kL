/*
 * Copyright 2010-2020 OpenXcom Developers.
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
 * along with OpenXcom. If not, see <http:///www.gnu.org/licenses/>.
 */

#ifndef OPENXCOM_BATTLESCAPEGAME_H
#define OPENXCOM_BATTLESCAPEGAME_H

#include <list>
//#include <string>
#include <vector>

#include <SDL/SDL.h>

#include "Position.h"


namespace OpenXcom
{

class BattleItem;
class BattlescapeState;
class BattleState;
class BattleUnit;
class InfoboxDialogState;
class Map;
class Pathfinding;
class ResourcePack;
class RuleItem;
class Ruleset;
class SavedBattleGame;
class TileEngine;


enum BattleActionType
{
	BA_NONE,		//  0
	BA_TURN,		//  1
	BA_MOVE,		//  2
	BA_PRIME,		//  3
	BA_THROW,		//  4
	BA_AUTOSHOT,	//  5
	BA_SNAPSHOT,	//  6
	BA_AIMEDSHOT,	//  7
	BA_MELEE,		//  8
	BA_USE,			//  9
	BA_LAUNCH,		// 10
	BA_PSICONTROL,	// 11
	BA_PSIPANIC,	// 12
	BA_THINK,		// 13
	BA_DEFUSE,		// 14 -> kL_add ->>
	BA_DROP,		// 15
	BA_PSICONFUSE,	// 16 - reduces a victim's TU
	BA_PSICOURAGE,	// 17 - increases morale of an ally
	BA_LIQUIDATE	// 18 - executes an unconscious unit
};

/**
 * Object that holds relevant battle action data in one container.
 */
struct BattleAction
{
	bool
		desperate, // ignore any newly-spotted units
		finalAction,
		dash,
//		pauseAfterShot,
		strafe,
		takenXp,
		targeting;
	int
		shotCount,
		diff,
		finalFacing,
//		firstTU, // is setup but not used.
		TU,
		value;

	std::string result; // an error message or else an empty string if everything went fine.

	BattleActionType type;
	BattleItem* weapon;
	BattleUnit
		* actor,
		* targetUnit;
	Position
		posCamera,
		posTarget;

	std::list<Position> waypoints;


	BattleAction()
		:
			type(BA_NONE),
			actor(nullptr),
			targetUnit(nullptr),
			weapon(nullptr),
			TU(0),
//			firstTU(-1),
			targeting(false),
			value(0),
			strafe(false),
			dash(false),
			diff(0),
			shotCount(0),
			posCamera(0,0,-1),
			desperate(false),
			finalFacing(-1),
			finalAction(false),
			takenXp(false)
//			pauseAfterShot(false)
	{}

	/// heh This could cause problems.
	/**
	 * Clears the BattleAction completely.
	 * @note This is not to be used unless instantiating a BattlescapeGame.
	 */
	void clearAction()
	{
		type = BA_NONE;
		actor = nullptr;
		targetUnit = nullptr;
		weapon = nullptr;
		TU = 0;
//		firstTU = -1;
		targeting = false;
		value = 0;
		result = "";
		strafe = false;
		dash = false;
		diff = 0;
		shotCount = 0;
		posCamera = Position::POS_BELOW;
		desperate = false;
		finalFacing = -1;
		finalAction = false;
		takenXp = false;
		waypoints.clear();
//		pauseAfterShot = false;
	}

	/**
	 * Translates type into a debug-string.
	 * @param type - the BattleActionType (BattlescapeGame.h)
	 * @return, the action-type as a string
	 */
	static std::string debugBat(const BattleActionType& type)
	{
		switch (type)
		{
			default:
			case BA_NONE:       return "none";
			case BA_TURN:       return "turn";
			case BA_MOVE:       return "move";
			case BA_PRIME:      return "prime";
			case BA_THROW:      return "throw";
			case BA_AUTOSHOT:   return "autoshot";
			case BA_SNAPSHOT:   return "snapshot";
			case BA_AIMEDSHOT:  return "aimedshot";
			case BA_MELEE:      return "melee";
			case BA_USE:        return "use";
			case BA_LAUNCH:     return "launch";
			case BA_PSICONTROL: return "psi-control";
			case BA_PSIPANIC:   return "psi-panic";
			case BA_THINK:      return "think";
			case BA_DEFUSE:     return "defuse";
			case BA_DROP:       return "drop";
			case BA_PSICONFUSE: return "psi-confuse";
			case BA_PSICOURAGE: return "psi-courage";
		}
	}

	/// Outputs the BattleAction as a string.
	static std::string debugBAction(const BattleAction& action);
};


/**
 * Battlescape Game - the core class of the tactical battlefield.
 */
class BattlescapeGame
{

private:
//	static bool _debug;

	bool
		_AISecondMove,
//		_endTurnProcessed,
		_endTurnRequested,
		_executeProgress,
		_playedAggroSound,
		_playerPanicHandled,
		_shotgunProgress;
	int _AIActionCounter; // first action of turn, second, etc.


	std::string // SoldierDiary killStat info ->
		_killStatRace,
		_killStatRank,
		_killStatWeapon,
		_killStatWeaponLoad;
	int
		_killStatTacId,
		_killStatTurn,
		_killStatPoints;


	BattleAction _playerAction;

	BattleItem
		* _alienPsi,
		* _universalFist;
	BattlescapeState* _battleState;
	SavedBattleGame* _battleSave;

	std::list<BattleState*>
		_deletedStates,
		_battleStates;
	std::vector<InfoboxDialogState*> _infoboxQueue;


	/// Changes the active-hand of a BattleUnit.
	void changeActiveHand(BattleUnit* const unit);

	/// Clears the trace-AI markers on all battlefield Tiles.
	void resetTraceTiles();

	/// Handles non-player BattleUnit AI.
	void handleUnitAI(BattleUnit* const unit);
	/// Selects the next AI unit.
	void selectNextAiUnit(const BattleUnit* const unit);
	/// Ends the AI turn.
	void endAiTurn();

	/// Coup de grace - a non-target action.
	void liquidateUnit();

	/// Ends the turn.
	void endTurn();

	/// Picks the first Soldier that is panicking.
	bool handlePanickingPlayer();
	/// Common function for handling panicking units.
	bool handlePanickingUnit(BattleUnit* const unit);

	/// Updates the Tu field and bar.
	void updateTuInfo(const BattleUnit* const unit);

	/// Collects data about attacker for SoldierDiary.
	void diaryAttacker(
			const BattleUnit* const attacker,
			const RuleItem* itRule);
	/// Collects data about defender for SoldierDiary.
	void diaryDefender(const BattleUnit* const defender);
	/// Adjusts a BattleUnit's morale for making a kill.
	void attackerMorale(
			BattleUnit* const attacker,
			const BattleUnit* const defender,
			bool half = false) const;
	/// Adjusts morale of units by faction when a BattleUnit dies.
	void factionMorale(
			const BattleUnit* const defender,
			bool isConvert,
			bool half = false) const;

	/// Shows any infoboxes in the queue.
	void showInfoBoxQueue();


	public:
		static bool _debugPlay;

		static const char* const PLAYER_ERROR[16u];


		/// Creates a BattlescapeGame.
		BattlescapeGame(
				SavedBattleGame* const battleSave,
				BattlescapeState* const battleState);
		/// Cleans up the BattlescapeGame.
		~BattlescapeGame();

		/// Checks for units panicking or falling and so on.
		void think();

		/// Handles BattleStates per the tactical-timer.
		void handleBattleState();
		/// Pushes a BattleState to the front of the list.
		void stateBPushFront(BattleState* const battleState);
		/// Pushes a BattleState to second on the list.
		void stateBPushNext(BattleState* const battleState);
		/// Pushes a BattleState to the back of the list.
		void stateBPushBack(BattleState* const battleState = nullptr);
		/// Pops the current BattleState and handles any after-effects.
		void popBattleState();
		/// Checks that there are no BattleStates pending for a specified actor.
		bool noActionsPending(const BattleUnit* const unit) const;
		/// Sets the BattleState->think() interval.
		void setStateInterval(Uint32 interval);

		/// Decreases the AI-counter by one pip.
		void decAiActionCount();

		/// Handles the result of non-target battle-actions like priming a grenade.
		void handleNonTargetAction();

		/// Sets the selector according to the current battle-action.
		void setupSelector();

		/// Determines whether a playable unit is selected.
		bool playableUnitSelected() const;

		/// Handles kneeling and/or standing.
		bool kneelToggle(BattleUnit* const unit);

		/// Checks for casualties.
		void checkCasualties(
				const RuleItem* const itRule = nullptr,
				BattleUnit* attacker = nullptr,
				bool isPreTactical = false,
				bool isTerrain = false);
		/// Checks if a BattleUnit gets exposed after making a melee-attack.
		void checkExposedByMelee(BattleUnit* const unit) const;

		/// Checks against reserved-TU.
		bool checkReservedTu(
				BattleUnit* const unit,
				int tu) const;

		/// Cancels all current battle-actions.
//		void cancelAllActions();
		/// Cancels the current battle-action.
		bool cancelTacticalAction(bool force = false);
		/// Gets a pointer for access to the battle-action-struct directly.
		BattleAction* getTacticalAction();

		/// Checks whether a BattleState is currently in progress.
		bool isBusy() const;

		/// Left click activates a primary action.
		void primaryAction(const Position& pos);
		/// Right click activates a secondary action.
		void secondaryAction(const Position& pos);
		/// Handler for the blaster-launcher button.
		void launchAction();
		/// Handler for the psi-button.
		void psiButtonAction();

		/// Moves a BattleUnit up or down.
		void moveUpDown(
				const BattleUnit* const unit,
				int dir);

		/// Requests the end of the turns.
		void requestEndTurn();

		/// Drops an item and affects it with gravity.
		void dropItem(
				BattleItem* const it,
				const Position& pos,
				bool create = false);
		/// Drops all items in a specific BattleUnit's inventory to the ground.
		void dropUnitInventory(BattleUnit* const unit);

		/// Converts a BattleUnit into a different type of BattleUnit.
		void convertUnit(BattleUnit* potato);

		/// Gets the battlefield Map.
		Map* getMap() const;
		/// Gets the SavedBattleGame data object.
		SavedBattleGame* getBattleSave() const;
		/// Gets the TileEngine.
		TileEngine* getTileEngine() const;
		/// Gets Pathfinding.
		Pathfinding* getPathfinding() const;
		/// Gets the ResourcePack.
		ResourcePack* getResourcePack() const;
		/// Gets the Ruleset.
		const Ruleset* getRuleset() const;

		/// Returns whether player-panic has been handled.
		bool playerPanicHandled() const
		{ return _playerPanicHandled; }
		/// Sets var to start handling panic for Player's units.
		void setPlayerPanic()
		{ _playerPanicHandled = false; }

		/// Tries to find an item and pick it up if possible.
		bool pickupItem(BattleAction* const aiAction) const;
		/// Checks through all items on the ground and picks one.
		BattleItem* surveyItems(BattleUnit* const unit) const;
		/// Evaluates if it's worthwhile to take an item.
		bool worthTaking(
				BattleItem* const item,
				BattleUnit* const unit) const;
		/// Picks an item up from the ground.
		bool takeItemFromGround(
				BattleItem* const item,
				BattleUnit* const unit) const;
		/// Assigns an item to a slot (stolen from battlescapeGenerator::addItem()).
		bool takeItem(
				BattleItem* const item,
				BattleUnit* const unit) const;

		/// Tallies conscious units.
		bool tallyUnits(
				int& liveHostile,
				int& livePlayer) const;
		/// Tallies conscious player-units at an Exit-area.
		int tallyPlayerExit() const;
		/// Tallies conscious hostile-units.
		int tallyHostiles() const;

		/// Checks for and triggers proximity grenades.
		bool checkProxyGrenades(BattleUnit* const unit);

		/// Cleans up all the deleted BattleStates.
		void clearBattleStates();

		/// Gets the BattlescapeState.
		BattlescapeState* getBattlescapeState() const;

		/// Gets the universal fist.
		BattleItem* getFist() const;
		/// Gets the alienPsi weapon.
		BattleItem* getAlienPsi() const;

		/// Sets up a mission complete notification.
		void objectiveSuccess();

		/// Gets if a coup-de-grace action is underway and needs to be animated.
		bool getLiquidate() const;
		/// Finishes a coup-de-grace action.
		void endLiquidate();

		/// Sets if a shotgun blast is underway.
		void setShotgun(bool shotgun = true);
		/// Gets if a shotgun blast is underway.
		bool getShotgun() const;

		/// Sets the TU reserved type.
//		void setReservedAction(BattleActionType bat);
		/// Gets the type of action that is reserved.
//		BattleActionType getReservedAction() const;
		/// Sets the kneel reservation setting.
//		void setKneelReserved(bool reserved) const;
		/// Checks the kneel reservation setting.
//		bool getKneelReserved() const;
};

}

#endif
