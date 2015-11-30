/*
 * Copyright 2010-2015 OpenXcom Developers.
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
class Surface;
class Text;
class TextButton;
class TextList;
class Window;


/**
 * A small window that shows mission details for a soldier.
 */
class SoldierDiaryMissionState
	:
		public State
{

private:
	static const Uint8
		WHITE	= 208,
		YELLOW	= 213,
		BLUE	= 218;

	Surface* _srfLine;
	Text
		* _txtTitle,
		* _txtScore,
		* _txtKills,
		* _txtMissionType,
		* _txtUFO,
		* _txtRace,
		* _txtDaylight,
		* _txtDaysWounded,
		* _txtPoints;
	TextButton* _btnOk;
	TextList* _lstKills;
	Window* _window;


	public:
		/// Creates the Soldier Diary Mission state.
		SoldierDiaryMissionState(
				Base* const base,
				size_t soldierId,
				size_t entry);
		/// Cleans up the Soldier Diary Mission state.
		~SoldierDiaryMissionState();

		/// Handler for clicking the Cancel button.
		void btnOkClick(Action* action);
};

}

#endif
