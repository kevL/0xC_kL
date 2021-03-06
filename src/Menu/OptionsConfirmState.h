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

#ifndef OPENXCOM_OPTIONSCONFIRMSTATE_H
#define OPENXCOM_OPTIONSCONFIRMSTATE_H

#include "OptionsBaseState.h"

#include "../Engine/State.h"


namespace OpenXcom
{

class Text;
class TextButton;
class Timer;
class Window;


/**
 * Confirmation window when Display Options are changed.
 */
class OptionsConfirmState
	:
		public State
{

private:
	int _countdown;
	OptionsOrigin _origin;

	Text
		* _txtTitle,
		* _txtTimer;
	TextButton
		* _btnYes,
		* _btnNo;
	Timer* _timer;
	Window* _window;


	public:
		/// Creates the Confirm Display Options state.
		explicit OptionsConfirmState(OptionsOrigin origin);
		/// Cleans up the Confirm Display Options state.
		~OptionsConfirmState();

		/// Handle timers.
		void think() override;

		/// Countdown for reverting options.
		void countdown();

		/// Handler for clicking the Yes button.
		void btnYesClick(Action* action);
		/// Handler for clicking the No button.
		void btnNoClick(Action* action);
};

}

#endif
