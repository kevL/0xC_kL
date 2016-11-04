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
	static LoadingPhase _loadPhase;
	static bool _ready;
	static std::string _error;

	int _charWidth;
	size_t _dosStep;

	Font* _font;
	Language* _lang;
	Text
		* _caret,
		* _text;
	Timer* _timer;

	SDL_Thread* _thread;

	std::wostringstream _output;
	std::wstring _dosart;

	/// Loads game resources.
	static int load(void* ptrG);

	/// Animates the dos-terminal.
	void doDosart();

	/// Adds a line of dos-text.
	void addLine(const std::wstring& line);
	/// Adds a newline to dos-text
	void addNewline();
	/// Adds a character to dos-text.
	void addChar(size_t pos);
	/// Adds a caret to dos-text.
	void addCaret();
	/// Adds a wait to dos-text.
	void addWait();


	public:
		/// Creates a Start state.
		StartState();
		/// Cleans up the Start state.
		~StartState();

		/// Reset everything.
		void init() override;
		/// Displays fake ASCII print.
		void think() override;

		/// Handles key clicks.
		void handle(Action* action) override;

};

}

#endif
