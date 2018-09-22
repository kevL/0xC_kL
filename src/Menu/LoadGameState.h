/*
 * Copyright 2010-2018 OpenXcom Developers.
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

#ifndef OPENXCOM__LOADGAMESTATE
#define OPENXCOM__LOADGAMESTATE

#include "../Engine/State.h"

//#include <string>

//#include <SDL/SDL.h>

#include "OptionsBaseState.h"

#include "../Savegame/SavedGame.h"


namespace OpenXcom
{

class ListLoadState;
class Text;


/**
 * Loads a saved-game with an optional message.
 */
class LoadGameState
	:
		public State
{

private:
	static const int WAIT_TICKS = 5;

	int _wait;
	std::string _file;

	Text* _txtStatus;

	ListLoadState* _parent;

	OptionsOrigin _origin;

	/// Creates the interface.
	void build(SDL_Color* const palette);


	public:
		/// Creates a LoadGameState.
		LoadGameState(
				OptionsOrigin origin,
				const std::string& file,
				SDL_Color* const palette,
				ListLoadState* const parent);
		LoadGameState(
				OptionsOrigin origin,
				SDL_Color* const palette);
		/// Cleans up the LoadGameState.
		~LoadGameState();

		/// Ignores quick-loads without a save available.
		void init() override;
		/// Loads a clicked entry.
		void think() override;
};

}

#endif
