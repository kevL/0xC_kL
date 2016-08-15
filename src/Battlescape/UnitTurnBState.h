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

#ifndef OPENXCOM_UNITTURNBSTATE_H
#define OPENXCOM_UNITTURNBSTATE_H

#include "BattleState.h"


namespace OpenXcom
{

class BattleUnit;


/**
 * State for turning units.
 */
class UnitTurnBState
	:
		public BattleState
{

private:
	bool
		_chargeTu,
		_turret;

	int _tu;

	BattleUnit* _unit;


	public:
		/// Creates a UnitTurnBState.
		UnitTurnBState(
				BattlescapeGame* const parent,
				BattleAction action,
				bool chargeTu = true);
		/// Cleans up the UnitTurnBState.
		~UnitTurnBState();

		/// Gets the label of the BattleState.
		std::string getBattleStateLabel() const override;

		/// Initializes the BattleState.
		void init() override;
		/// Runs BattleState functionality every cycle.
		void think() override;
		/// Handles a cancel request.
//		void cancel();
};

}

#endif
