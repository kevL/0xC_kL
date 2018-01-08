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

#ifndef OPENXCOM_UNITINFOSTATE_H
#define OPENXCOM_UNITINFOSTATE_H

#include "../Engine/State.h"


namespace OpenXcom
{

class Bar;
class BattlescapeState;
class BattleUnit;
class NumberText;
class SavedBattleGame;
class Surface;
class Text;
class TextButton;


/**
 * UnitInfo screen that shows the tactical-info of a specified BattleUnit.
 */
class UnitInfoState final
	:
		public State
{

private:
	static const Uint8
		WHITE	=  1u,
		BROWN_L	= 80u;

	bool
		_fromInventory,
		_mindProbe,
		_preBattle;

	Bar
		* _barTimeUnits,
		* _barEnergy,
		* _barHealth,
		* _barFatals,
		* _barBravery,
		* _barMorale,
		* _barReactions,
		* _barFiring,
		* _barThrowing,
		* _barMelee,
		* _barStrength,
		* _barPsiStrength,
		* _barPsiSkill,

		* _barFrontArmor,
		* _barLeftArmor,
		* _barRightArmor,
		* _barRearArmor,
		* _barUnderArmor;
	BattlescapeState* _battleState;
	const BattleUnit* _unit;
	InteractiveSurface* _exit;
	NumberText* _battleOrder;
	SavedBattleGame* _battleSave;
	Surface
		* _bgUnibord,
		* _gender;
	Text
		* _txtName,

		* _txtTimeUnits,
		* _txtEnergy,
		* _txtHealth,
		* _txtFatals,
		* _txtBravery,
		* _txtMorale,
		* _txtReactions,
		* _txtFiring,
		* _txtThrowing,
		* _txtMelee,
		* _txtStrength,
		* _txtPsiStrength,
		* _txtPsiSkill,

		* _txtTimeUnits_i,
		* _txtEnergy_i,
		* _txtHealth_i,
		* _txtStun_i,
		* _txtFatals_i,
		* _txtBravery_i,
		* _txtMorale_i,
		* _txtReactions_i,
		* _txtFiring_i,
		* _txtThrowing_i,
		* _txtMelee_i,
		* _txtStrength_i,
		* _txtPsiStrength_i,
		* _txtPsiSkill_i,

		* _txtFrontArmor,
		* _txtLeftArmor,
		* _txtRightArmor,
		* _txtRearArmor,
		* _txtUnderArmor,

		* _txtFrontArmor_i,
		* _txtLeftArmor_i,
		* _txtRightArmor_i,
		* _txtRearArmor_i,
		* _txtUnderArmor_i;
	TextButton
		* _btnNext,
		* _btnPrev;
//	Timer* _timer;

	/// Advances to the next/previous Unit when right/left key is depressed.
//	void keyRepeat(); // <- too twitchy.

	/// Handler for exiting the state.
	void exitClick(Action* action = nullptr);


	public:
		/// Creates a UnitInfo state.
		UnitInfoState(
				const BattleUnit* const unit,
				BattlescapeState* const battleState,
				const bool fromInventory = false,
				const bool mindProbe = false,
				const bool preBattle = false);
		/// Cleans up the UnitInfo state.
		~UnitInfoState();

		/// Updates unit-info.
		void init() override;
		/// Runs the Timer.
//		void think();

		/// Handler for clicking stuff.
		void handle(Action* action) override;

		/// Handler for clicking the Next button.
		void btnNextClick(Action* action);
		/// Handler for clicking the Previous button.
		void btnPrevClick(Action* action);
};

}

#endif
