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

#ifndef OPENXCOM_ALIENSCRASHSTATE_H
#define OPENXCOM_ALIENSCRASHSTATE_H

#include "../Engine/State.h"


namespace OpenXcom
{

class Text;
class TextButton;
class Window;


/**
 * Screen shown when all aLiens died at a crash-site.
 */
class AliensCrashState
	:
		public State
{

private:
	Text* _txtTitle;
	TextButton* _btnOk;
	Window* _window;


	public:
		/// Creates an AliensCrash state.
		AliensCrashState();
		/// Cleans up the AliensCrash state.
		~AliensCrashState();

		/// Handler for clicking the Ok button.
		void btnOkClick(Action* action);
};

}

#endif
