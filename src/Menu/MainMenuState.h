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

#ifndef OPENXCOM_MAINMENUSTATE_H
#define OPENXCOM_MAINMENUSTATE_H

#include "../Engine/State.h"


namespace OpenXcom
{

class Text;
class TextButton;
class Window;


/**
 * MainMenu window displayed when starting the Game or loading a save etc.
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
		/// Creates a MainMenu state.
		MainMenuState();
		/// Cleans up the MainMenu state.
		~MainMenuState();

		/// Initializes the state.
//		void init() override;

		/// Handler for clicking the StartPlay button.
		void btnStartPlayClick(Action* action);
		/// Handler for clicking the QuickBattle button.
		void btnQuickBattleClick(Action* action);
		/// Handler for clicking the LoadSaved button.
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
