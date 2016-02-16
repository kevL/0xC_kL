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

#ifndef OPENXCOM_UNITDIEBSTATE_H
#define OPENXCOM_UNITDIEBSTATE_H

#include "BattleState.h"

#include "../Ruleset/RuleItem.h"


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
		_init,
		_doneScream,
		_noScream;
	int _extraTicks;

	BattleUnit* _unit;
	SavedBattleGame* _battleSave;

	DamageType _dType;

	/// Converts a BattleUnit to a body-item.
	void convertToBodyItem();
	/// Centers Camera on a collapsing BattleUnit.
	void centerOnUnitDeath();


	public:
		/// Creates a UnitDieBState object.
		UnitDieBState(
				BattlescapeGame* const parent,
				BattleUnit* const unit,
				const DamageType dType,
				const bool noScream = false);
		/// Cleans up the UnitDieBState.
		~UnitDieBState();

		/// Initializes the state.
//		void init();
		/// Runs state functionality every cycle.
		void think() override;
		/// Handles a cancel request.
//		void cancel();
};

}

#endif
