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

#ifndef OPENXCOM_SOLDIERINFOSTATE_H
#define OPENXCOM_SOLDIERINFOSTATE_H

//#include <vector>

#include "../Engine/State.h"


namespace OpenXcom
{

class Bar;
class Base;
class NumberText;
class Soldier;
class Surface;
class Text;
class TextButton;
class TextEdit;


/**
 * SoldierInfo screen that shows all the info of a specific Soldier.
 */
class SoldierInfoState final
	:
		public State
{

private:
	static const Uint8
		PURPLE_CRAFTOUT	=  73u,
		BROWN			= 160u,
		PURPLE			= 246u;

	bool
		_allowExit,
		_isQuickBattle;
	size_t _solId;

	std::vector<Soldier*>* _listBase;

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
	Base* _base;
	InteractiveSurface* _bg;
	NumberText* _battleOrder;
	Soldier* _sol;
	Surface
		* _gender,
		* _rank;
	Text
		* _txtArmor,
		* _txtCraft,
		* _txtKills,
		* _txtMissions,
		* _txtPsionic,
		* _txtRank,
		* _txtRecovery,
		* _txtDay,

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
		* _btnArmor,
		* _btnNext,
		* _btnOk,
		* _btnPrev,
		* _btnAutoStat,
		* _btnSack,
		* _btnDiary;
	TextEdit* _edtSoldier;

	/// Handles autoStat click.
	void btnAutoStat(Action* action);
	/// Handles autoStatAll click.
	void btnAutoStatAll(Action* action);

	/// Handles RMB on lower part of screen.
	void exitClick(Action* action);


	public:
		/// Creates a SoldierInfo state.
		SoldierInfoState(
				Base* const base,
				size_t solId);
		/// Cleans up the SoldierInfo state.
		~SoldierInfoState();

		/// Updates the Soldier's info.
		void init() override;

		/// Sets the soldier-ID.
		void setSoldierId(size_t solId);
		/// Handler for changing text on the Name edit.
		void edtSoldierChange(Action* action);

		/// Handler for clicking the Ok button.
		void btnOkClick(Action* action);
		/// Handler for clicking the Previous button.
		void btnPrevClick(Action* action);
		/// Handler for clicking the Next button.
		void btnNextClick(Action* action);

		/// Handler for clicking the Armor button.
		void btnArmorClick(Action* action);
		/// Handler for clicking the Sack button.
		void btnSackClick(Action* action);
		/// Handler for clicking the Diary button.
		void btnDiaryClick(Action* action);
};

}

#endif
