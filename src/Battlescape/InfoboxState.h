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
 * along with OpenXcom. If not, see <http:///www.gnu.org/licenses/>.
 */

#ifndef OPENXCOM_INFOBOXSTATE_H
#define OPENXCOM_INFOBOXSTATE_H

#include "../Engine/State.h"


namespace OpenXcom
{

class Frame;
class Text;
class Timer;


/**
 * Frame that briefly shows some info like : Yasuaki Okamoto Has Panicked.
 * @note It disappears after 2-1/2 seconds.
 */
class InfoboxState
	:
		public State
{

private:
	Text* _text;
	Frame* _frame;
	Timer* _timer;


	public:
		static const int INFOBOX_DURATION = 2225;

		/// Creates an Infobox state.
		explicit InfoboxState(const std::wstring& wst);
		/// Cleans up the Infobox state.
		~InfoboxState();

		/// Handler for hardware interrupts.
		void handle(Action* action) override;

		/// Handles the Timer.
		void think() override;

		/// Closes the window.
		void exit();
};

}

#endif
