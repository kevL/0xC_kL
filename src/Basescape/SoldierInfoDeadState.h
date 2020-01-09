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

#ifndef OPENXCOM_SOLDIERDEADINFOSTATE_H
#define OPENXCOM_SOLDIERDEADINFOSTATE_H

//#include <vector>

#include "../Engine/State.h"


namespace OpenXcom
{

class Bar;
class SoldierDead;
class Surface;
class Text;
class TextButton;


/**
 * SoldierInfoDead screen that shows all the info of a specific dead Soldier.
 */
class SoldierInfoDeadState final
	:
		public State
{

private:
	size_t _solId;

	std::vector<SoldierDead*>* _listDead;

	SoldierDead* _sol;

	Bar
		* _barTimeUnits,
		* _barStamina,
		* _barHealth,
		* _barBravery,
		* _barReactions,
		* _barFiring,
		* _barThrowing,
		* _barMelee,
		* _barStrength,
		* _barPsiStrength,
		* _barPsiSkill;
	Surface
		* _bg,
		* _gender,
		* _rank;
	Text
		* _txtDate,
		* _txtDeath,
		* _txtKills,
		* _txtMissions,
		* _txtRank,
		* _txtSoldier,

		* _txtTimeUnits,
		* _txtStamina,
		* _txtHealth,
		* _txtBravery,
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
		* _txtBravery_i,
		* _txtReactions_i,
		* _txtFiring_i,
		* _txtThrowing_i,
		* _txtMelee_i,
		* _txtStrength_i,
		* _txtPsiStrength_i,
		* _txtPsiSkill_i;
	TextButton
		* _btnNext,
		* _btnOk,
		* _btnPrev,
		* _btnDiary;


	public:
		/// Creates a SoldierInfoDead state.
		explicit SoldierInfoDeadState(size_t solId);
		/// Cleans up the SoldierInfoDead state.
		~SoldierInfoDeadState();

		/// Updates the dead Soldier info.
		void init() override;

		/// Sets the soldier-ID.
		void setSoldierId(size_t solId);

		/// Handler for clicking the Ok button.
		void btnOkClick(Action* action);
		/// Handler for clicking the Previous button.
		void btnPrevClick(Action* action);
		/// Handler for clicking the Next button.
		void btnNextClick(Action* action);

		/// Handler for clicking the Diary button.
		void btnDiaryClick(Action* action);
};

}

#endif
