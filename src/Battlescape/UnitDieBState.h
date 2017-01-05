/*
 * Copyright 2010-2017 OpenXcom Developers.
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

#ifndef OPENXCOM_UNITDIEBSTATE_H
#define OPENXCOM_UNITDIEBSTATE_H

#include "BattleState.h"


namespace OpenXcom
{

class BattlescapeGame;
class BattleUnit;
class SavedBattleGame;


/**
 * State for dying units.
 */
class UnitDieBState
	:
		public BattleState
{

private:
	bool
		_isInfected,
		_isInjury,
		_isPreTactical,
		_isSilent;
	int _post;

	BattleUnit* _unit;
	SavedBattleGame* _battleSave;

	/// Converts the BattleUnit to a body-item.
	void drop();


	public:
		/// Creates a UnitDieBState object.
		UnitDieBState(
				BattlescapeGame* const battleGame,
				BattleUnit* const unit,
				const bool isPreTactical,
				const bool isSilent = true,
				const bool isInjury = false);
		/// Cleans up the UnitDieBState.
		~UnitDieBState();

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
