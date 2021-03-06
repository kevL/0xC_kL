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

#ifndef OPENXCOM_UFOPAEDIASTARTSTATE_H
#define OPENXCOM_UFOPAEDIASTARTSTATE_H

//#include <string>

#include "../Engine/State.h"


namespace OpenXcom
{

class Action;
class Text;
class TextButton;
class Window;


/**
 * UfopaediaStartState is the screen that opens when clicking Ufopaedia button.
 * @note Presents buttons to all sections of Ufopaedia and opens a
 * UfopaediaSelectState on click.
 */
class UfopaediaStartState
	:
		public State
{

protected:
	static const size_t ped_SECTIONS = 10u;
	static const std::string ped_TITLES[ped_SECTIONS];

	bool _tactical;

	Text* _txtTitle;
	TextButton
		* _btnOk,
		* _btnSection[ped_SECTIONS];
	Window* _window;

	// navigation callbacks
	///
	void btnSectionClick(Action* action);
	///
	void btnOkClick(Action* action);


	public:
		/// cTor.
		explicit UfopaediaStartState(bool tactical = false);
		/// dTor.
		virtual ~UfopaediaStartState();
};

}

#endif
