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

#ifndef OPENXCOM_SOLDIERDIARYOVERVIEWSTATE_H
#define OPENXCOM_SOLDIERDIARYOVERVIEWSTATE_H

#include "../Engine/State.h"

//#include <vector>


namespace OpenXcom
{

class Base;
class Soldier;
class SoldierDead;
class SoldierInfoState;
class SoldierInfoDeadState;
class Text;
class TextButton;
class TextList;
class Window;


/**
 * Medals screen that lets the player see all the medals that a Soldier has.
 */
class SoldierDiaryOverviewState final
	:
		public State
{

private:
	size_t
		_recall,
		_rows,
		_solId;

	const std::vector<Soldier*>* _listBase;
	const std::vector<SoldierDead*>* _listDead;

	Base* _base;
	SoldierInfoState* _solInfo;
	SoldierInfoDeadState* _solInfoDead;

	Text
		* _txtTitle,
		* _txtBaseLabel,
		* _txtMissionId,
		* _txtLocation,
		* _txtStatus,
		* _txtDate;
	TextButton
		* _btnOk,
		* _btnPrev,
		* _btnNext,
		* _btnKills,
		* _btnMissions,
		* _btnAwards;
	TextList* _lstDiary;
	Window* _window;


	public:
		/// Creates a SoldierDiaryOverview state.
		SoldierDiaryOverviewState(
				Base* const base,
				size_t solId,
				SoldierInfoState* const solInfo,
				SoldierInfoDeadState* const solInfoDead);
		/// Cleans up the SoldierDiaryOverview state.
		~SoldierDiaryOverviewState();

		/// Updates the list info.
		void init() override;

		/// Sets the current Soldier by his/her ID.
		void setSoldierId(size_t solId);

		/// Handler for clicking the OK button.
		void btnOkClick(Action* action);
		/// Handler for clicking the Kills button.
		void btnKillsClick(Action* action);
		/// Handler for clicking the Missions button.
		void btnMissionsClick(Action* action);
		/// Handler for clicking the Awards button.
		void btnMedalsClick(Action* action);

		/// Handler for clicking the Previous button.
		void btnPrevClick(Action* action);
		/// Handler for clicking the Next button.
		void btnNextClick(Action* action);

		/// Handler for clicking on mission list.
		void lstMissionInfoClick(Action* action);
};

}

#endif
