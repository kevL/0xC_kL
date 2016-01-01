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

#ifndef OPENXCOM_MAINMENUSTATE_H
#define OPENXCOM_MAINMENUSTATE_H

#include "../Engine/State.h"


namespace OpenXcom
{

class Text;
class TextButton;
class Window;


/**
 * Main Menu window displayed when first starting the game.
 */
class MainMenuState
	:
		public State
{

private:
	Text
		* _txtBuild,
		* _txtTitle;
	TextButton
		* _btnStart,
		* _btnTactical,
		* _btnLoad,
//		* _btnOptions,
		* _btnIntro,
		* _btnQuit;
	Window
		* _window;


	public:
		/// Creates the Main Menu state.
		MainMenuState();
		/// Cleans up the Main Menu state.
		~MainMenuState();

		/// Handler for clicking the New Game button.
		void btnNewGameClick(Action* action);
		/// Handler for clicking the New Battle button.
		void btnNewBattleClick(Action* action);
		/// Handler for clicking the Load Saved Game button.
		void btnLoadClick(Action* action);
		/// Handler for clicking the Options button.
//		void btnOptionsClick(Action* action);
		/// Plays the intro video.
		void btnPlayIntroClick(Action*);
		/// Handler for clicking the Quit button.
		void btnQuitClick(Action* action);

		/// Update the resolution settings - just resized the window.
//		void resize(int& dX, int& dY);
};

}

#endif
