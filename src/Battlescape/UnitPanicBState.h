/*
 * Copyright 2010-2018 OpenXcom Developers.
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
#ifndef OPENXCOM_UNITPANICBSTATE_H
#define OPENXCOM_UNITPANICBSTATE_H

#include "BattleState.h"


namespace OpenXcom
{

class BattleUnit;


/**
 * State for panicking units.
 */
class UnitPanicBState
	:
		public BattleState
{

private:
	BattleUnit* _unit;


	public:
		/// Creates a UnitPanicBState BattleState.
		UnitPanicBState(
				BattlescapeGame* const battle,
				BattleUnit* const unit);
		/// Cleans up the UnitPanicBState.
		~UnitPanicBState();

		/// Gets the label of the BattleState.
		std::string getBattleStateLabel() const override;

		/// Initializes the BattleState.
//		void init();
		/// Runs BattleState functionality every cycle.
		void think() override;
		/// Handles a cancel request.
//		void cancel();
};

}

#endif
