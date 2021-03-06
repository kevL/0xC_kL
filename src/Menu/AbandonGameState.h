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

#ifndef OPENXCOM_ABANDONGAMESTATE_H
#define OPENXCOM_ABANDONGAMESTATE_H

#include "OptionsBaseState.h"

#include "../Engine/State.h"


namespace OpenXcom
{

class Text;
class TextButton;
class Window;


/**
 * Abandon Game window shown before quitting the game from Geoscape.
 */
class AbandonGameState
	:
		public State
{

private:
	OptionsOrigin _origin;

	Text* _txtTitle;
	TextButton
		* _btnYes,
		* _btnNo;
	Window* _window;


	public:
		/// Creates the Abandon Game state.
		explicit AbandonGameState(OptionsOrigin origin);
		/// Cleans up the Abandon Game state.
		~AbandonGameState();

		/// Handler for clicking the Yes button.
		void btnYesClick(Action* action);
		/// Handler for clicking the No button.
		void btnNoClick(Action* action);
};

}

#endif
