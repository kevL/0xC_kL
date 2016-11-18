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

#ifndef OPENXCOM_SOLDIERDIARYPERFORMANCESTATE_H
#define OPENXCOM_SOLDIERDIARYPERFORMANCESTATE_H

#include "../Engine/State.h"

//#include <string>
//#include <vector>


namespace OpenXcom
{

enum SoldierDiaryDisplay
{
	DIARY_KILLS,	// 0
	DIARY_MISSIONS,	// 1
	DIARY_MEDALS	// 2
};


class Base;
class Soldier;
class SoldierDead;
class SoldierDiary;
class SoldierDiaryOverviewState;
class Surface;
class SurfaceSet;
class Text;
class TextButton;
class TextList;
class Window;


/**
 * Diary screens that list Kills/ Missions/ Awards totals.
 */
class SoldierDiaryPerformanceState final
	:
		public State
{

private:
	static const size_t SPRITE_ROWS = 12u;
	static const int SPRITES_y = 49;

	size_t
		_lastScroll,
		_rows,
		_solId;
	Uint8
		_colorBtnDown,
		_colorBtnUp,
		_color1stCol;

	SoldierDiaryDisplay _display;

	std::vector<std::wstring> _awardsList;

	const std::vector<Soldier*>* _listBase;
	const std::vector<SoldierDead*>* _listDead;
	std::vector<Surface*>
		_srfAward,
		_srfDecor;

	SoldierDiary* _diary;
	SoldierDiaryOverviewState* _overview;

	SurfaceSet
		* _srtAwards,
		* _srtLevels;
	Text
		* _txtTitle,
		* _txtBaseLabel,

		* _txtRank,
		* _txtRace,
		* _txtWeapon,

		* _txtLocation,
		* _txtType,
		* _txtUFO,

		* _txtMedal,
		* _txtMedalGrade,
		* _txtMedalClass,
		* _txtMedalInfo,

		* _txtProficiency;
	TextButton
		* _btnOk,
		* _btnPrev,
		* _btnNext,
		* _btnKills,
		* _btnMissions,
		* _btnAwards,

		* _displayGroup;
	TextList
		* _lstRank,
		* _lstRace,
		* _lstWeapon,
		* _lstKillTotals,

		* _lstLocation,
		* _lstType,
		* _lstUFO,
		* _lstMissionTotals,

		* _lstAwards;
	Window
		* _window;

	/// Draw medal sprites.
	void drawMedals();


	public:
		/// Creates a SoldierDiaryPerformance state.
		SoldierDiaryPerformanceState(
				Base* const base,
				const size_t solId,
				SoldierDiaryOverviewState* const overview,
				const SoldierDiaryDisplay display);
		/// Cleans up the SoldierDiaryPerformance state.
		~SoldierDiaryPerformanceState();

		/// Updates the info.
		void init() override;

		/// Handler for clicking the OK button.
		void btnOkClick(Action* action);

		/// Handler for clicking the Kills button.
		void btnKillsToggle(Action* action);
		/// Handler for clicking the Missions button.
		void btnMissionsToggle(Action* action);
		/// Handler for clicking the Awards button.
		void btnMedalsToggle(Action* action);

		/// Handler for moving the mouse over a medal.
		void lstMouseOver(Action* action);
		/// Handler for moving the mouse outside the medals list.
		void lstMouseOut(Action* action);

		/// Handler for clicking the Previous button.
		void btnPrevClick(Action* action);
		/// Handler for clicking the Next button.
		void btnNextClick(Action* action);

		/// Runs state functionality every cycle.
		void think() override;
};

}

#endif
