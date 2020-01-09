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
 * along with OpenXcom. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef OPENXCOM_BATTLESTATE_H
#define OPENXCOM_BATTLESTATE_H

#include "BattlescapeGame.h"


namespace OpenXcom
{

/**
 * This is the base-class for all BattleAction states.
 * @note These states can be triggered by the player or the AI:
 * - ExplosionBState
 * - ProjectileFlyBState
 * - UnitBonkBState
 * - UnitDieBState
 * - UnitPanicBState
 * - UnitTurnBState
 * - UnitWalkBState
 */
class BattleState
{

protected:
	BattleAction _action;
	BattlescapeGame* _battle;


	public:
		/// Creates a BattleState linked to the BattlescapeGame.
		BattleState(
				BattlescapeGame* const battle,
				BattleAction action = {});
		/// Cleans up the BattleState.
		virtual ~BattleState();

		/// Gets the label of the BattleState.
		virtual std::string getBattleStateLabel() const;

		/// Initializes the BattleState.
		virtual void init();
		/// Handles a cancel request.
		virtual void cancel();
		/// Runs state functionality every cycle.
		virtual void think();

		/// Gets a copy of the BattleAction.
		BattleAction getAction() const;
};

}

#endif
