/*
 * Copyright 2010-2019 OpenXcom Developers.
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

#ifndef OPENXCOM_MONTHNEARENDSTATE_H
#define OPENXCOM_MONTHNEARENDSTATE_H

#include "../Engine/State.h"


namespace OpenXcom
{

class Text;
class TextButton;
class Window;


/**
 * Window displayed to warn player that the end of the month is approaching.
 */
class MonthNearEndState
	:
		public State
{

private:
	Text
		* _txtTitle,
		* _txtMessage;
	TextButton* _btnOk;
	Window* _window;


	public:
		/// Constructs a MonthNearEnd state.
		MonthNearEndState();
		/// Destructs the MonthNearEnd state.
		~MonthNearEndState();

		/// Handler for clicking the Ok button.
		void btnOkClick(Action* action);
};

}

#endif
