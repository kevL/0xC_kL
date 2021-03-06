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

#ifndef OPENXCOM_SOLDIERDIEDSTATE_H
#define OPENXCOM_SOLDIERDIEDSTATE_H

//#include <string>

#include "../Engine/State.h"


namespace OpenXcom
{

class Text;
class TextButton;
class Window;


/**
 * Notifies the player when a soldier dies due to critical injuries in Sickbay.
 */
class SoldierDiedState
	:
		public State
{

private:
	static const Uint8 CYAN = 133u;

	Text
		* _txtBase,
		* _txtTitle;
	TextButton* _btnOk;
	Window* _window;


	public:
		/// Creates a SoldierDied state.
		SoldierDiedState(
				const std::wstring& soldier,
				const std::wstring& base);
		/// Cleans up the SoldierDied state.
		~SoldierDiedState();

		/// Handler for clicking the Ok button.
		void btnOkClick(Action* action);
};

}

#endif

