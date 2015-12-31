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

#ifndef OPENXCOM_STARTSTATE_H
#define OPENXCOM_STARTSTATE_H

#include <sstream>

#include "../Engine/State.h"


namespace OpenXcom
{

class Font;
class Language;
class Text;
class Timer;


enum LoadingPhase
{
	LOADING_STARTED,	// 0
	LOADING_FAILED,		// 1
	LOADING_SUCCESSFUL,	// 2
	LOADING_DONE		// 3
};


/**
 * Initializes the game and loads all required content.
 */
class StartState
	:
		public State
{

private:
	size_t _anim;

	Font* _font;
	Language* _lang;
	Text
		* _cursor,
		* _text;
	Timer* _timer;

	SDL_Thread* _thread;

	std::wostringstream _output;
	std::wstring _dosart;


	public:
		static LoadingPhase loading;
		static std::string error;
		static bool kL_ready;

		/// Creates the Start state.
		StartState();
		/// Cleans up the Start state.
		~StartState();

		/// Reset everything.
		void init() override;
		/// Displays messages.
		void think() override;

		/// Handles key clicks.
		void handle(Action* action) override;

		/// Animates the terminal.
		void animate();
		/// Adds a line of text.
		void addLine(const std::wstring& line);
		///
		void addLine_kL();
		///
		void addChar_kL(const size_t nextChar);
		///
		void addCursor_kL();

		/// Loads the game resources.
		static int load(void* ptrGame);
};

}

#endif
