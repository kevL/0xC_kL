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

#ifndef OPENXCOM_BASEDEFENSESTATE_H
#define OPENXCOM_BASEDEFENSESTATE_H

#include "../Engine/State.h"


namespace OpenXcom
{

class Base;
class GeoscapeState;
class Text;
class TextButton;
class TextList;
class Timer;
class Ufo;
class Window;


enum BaseDefenseActionType
{
	BD_NONE,	// 0
	BD_FIRE,	// 1
	BD_RESOLVE,	// 2
	BD_DESTROY,	// 3
	BD_END		// 4
};


/**
 * BaseDefense screen for when UFOs attack.
 */
class BaseDefenseState
	:
		public State
{

private:
	static const Uint32
		TI_SLOW		= 973u, // Time Intervals
		TI_MEDIUM	= 269u,
		TI_FAST		=  76u;

	static const size_t DISPLAYED = 14u; // max rows that can be displayed before scrolling down.

	int
		_thinkCycles;
	size_t
		_attacks,
		_defenses,
		_explosionCount,
		_gravShields,
		_passes,
		_row,
		_stLen_destroyed,
		_stLen_initiate,
		_stLen_repulsed;

	std::wstring
		_destroyed,
		_initiate,
		_repulsed;

	BaseDefenseActionType _action;

	Base* _base;
	GeoscapeState* _geoState;
	Text
		* _txtDestroyed,
		* _txtInit,
		* _txtTitle;
	TextButton* _btnOk;
	TextList* _lstDefenses;
	Timer* _timer;
	Ufo* _ufo;
	Window* _window;

	/// Advances the defense-state.
	void next();


	public:
		/// Creates a BaseDefense state.
		BaseDefenseState(
				Base* const base,
				Ufo* const ufo,
				GeoscapeState* const geoState);
		/// Cleans up the BaseDefense state.
		~BaseDefenseState();

		/// Handles the Timer.
		void think() override;

		/// Handler for clicking the Ok button.
		void btnOkClick(Action* action);
};

}

#endif
