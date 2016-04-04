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

#ifndef OPENXCOM_SOLDIERDIARYMISSIONSTATE_H
#define OPENXCOM_SOLDIERDIARYMISSIONSTATE_H

#include "../Engine/State.h"


namespace OpenXcom
{

class Base;
class SoldierDiary;
class Surface;
class Text;
class TextButton;
class TextList;
class Window;


/**
 * A small window that shows mission-details for a Soldier.
 */
class SoldierDiaryMissionState
	:
		public State
{

private:
	size_t
		_soldierId,
		_rowEntry;
	Uint8 _color;

	Base* _base;
	SoldierDiary* _diary;

	Surface
		* _srfLine,
		* _srfLineShade;
	Text
		* _txtTitle,
//		* _txtMissionId,
		* _txtScore,
		* _txtKills,
		* _txtMissionType,
		* _txtUFO,
		* _txtRace,
		* _txtDaylight,
		* _txtDaysWounded,
		* _txtPoints;
	TextButton
		* _btnPrev,
		* _btnNext,
		* _btnOk;
	TextList* _lstKills;
	Window* _window;


	public:
		/// Creates a SoldierDiaryMission state.
		SoldierDiaryMissionState(
				Base* const base,
				size_t soldierId,
				size_t rowEntry);
		/// Cleans up the SoldierDiaryMission state.
		~SoldierDiaryMissionState();

		/// Updates the mission-info.
		void init() override;

		/// Handler for clicking the Previous button.
		void btnPrevClick(Action* action);
		/// Handler for clicking the Next button.
		void btnNextClick(Action* action);
		/// Handler for clicking the Cancel button.
		void btnOkClick(Action* action);
};

}

#endif
