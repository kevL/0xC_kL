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

#ifndef OPENXCOM__SAVEGAMESTATE
#define OPENXCOM__SAVEGAMESTATE

//#include <string>

//#include <SDL/SDL.h>

#include "OptionsBaseState.h"

#include "../Engine/State.h"

#include "../Savegame/SavedGame.h"


namespace OpenXcom
{

class Text;


/**
 * Saves the current game with an optional message.
 */
class SaveGameState
	:
		public State
{

private:
	static const int WAIT_TICKS = 3;

	int _wait;
	std::string _file;

	OptionsOrigin _origin;
	SaveType _type;

	Text* _txtStatus;

	/// Creates the interface.
	void build(SDL_Color* const palette);


	public:
		/// Creates a SaveGame state.
		SaveGameState(
				OptionsOrigin origin,
				const std::string& file,
				SDL_Color* const palette);
		/// Creates a SaveGame state.
		SaveGameState(
				OptionsOrigin origin,
				SaveType type,
				SDL_Color* const palette);
		/// Cleans up the SaveGame state.
		~SaveGameState();

		/// Saves the game.
		void think() override;
};

}

#endif
