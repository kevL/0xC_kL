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
		_doneScream,
		_init,
		_hidden,
		_noScream;
	int _extraTicks;

	BattleUnit* _unit;
	SavedBattleGame* _battleSave;

	DamageType _dType;

	/// Converts the BattleUnit to a body-item.
	void convertToBody();
	/// Centers the Camera on the collapsing unit.
	void centerOnDeath();


	public:
		/// Creates a UnitDieBState object.
		UnitDieBState(
				BattlescapeGame* const parent,
				BattleUnit* const unit,
				const DamageType dType,
				const bool noScream = false,
				const bool hidden = false);
		/// Cleans up the UnitDieBState.
		~UnitDieBState();

		/// Gets the name of the BattleState.
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
