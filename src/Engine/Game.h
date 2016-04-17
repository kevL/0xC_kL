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

#ifndef OPENXCOM_GAME_H
#define OPENXCOM_GAME_H

#include <list>
#include <string>

#include <SDL.h>


namespace OpenXcom
{

class Cursor;
class FpsCounter;
class Language;
class ResourcePack;
class Ruleset;
class SavedGame;
class Screen;
class State;


/**
 * The core of the game engine.
 * @note Manages the game's entire contents and structure.
 * @note Takes care of encapsulating all the core SDL commands, provides access
 * to all the game's resources and contains a stack state machine to handle all
 * the initializations, events and blits of each state, as well as transitions.
 */
class Game
{

private:
	static const double VOLUME_GRADIENT;

	bool
		_init,
		_inputActive,
		_blitDelay,
		_quit;
	int
		_debugCycle,	// for debugging country-zones.
		_debugCycle_b,	// for debugging country-zones.
		_ticksTillNextSlice;
	Uint32 _tickOfLastSlice;

	Cursor* _cursor;
	FpsCounter* _fpsCounter;
	Language* _lang;
	ResourcePack* _res;
	Ruleset* _rules;
	SavedGame* _gameSave;
	Screen* _screen;

	SDL_Event _event;

	std::list<State*>
		_deleted,
		_states;


	public:
		/// Creates a Game and initializes SDL.
		explicit Game(const std::string& title);
		/// Cleans up all the Game's resources and shuts down SDL.
		~Game();

		/// Starts the Game's state-machine.
		void run();

		/// Quits the Game.
		void quit(bool force = false);
		/// Returns whether the Game is shutting down.
		bool isQuitting() const;

		/// Sets whether the mouse-cursor is activated/responsive.
		void setInputActive(bool active);

		/// Causes the engine to delay blitting the top state.
		void delayBlit();

		/// Resets a state-stack to a new state.
		void setState(State* const state);
		/// Pushes a state into the state-stack.
		void pushState(State* const state);
		/// Pops the last state from the state-stack.
		void popState();
		/// kL. Gets the current (top) state.
//		State* getState() const;
		/// Gets the quantity of currently running states.
		int getQtyStates() const;
		/// Returns whether a state is the current state.
		bool isState(const State* const state) const;

		/// Sets up the audio.
		void initAudio();
		/// Sets the Game's audio-amplitudes.
		void setVolume(
				int music,
				int sound,
				int ui = -1);
		/// Adjusts a linear volume-level to an exponential one.
		static double volExp(int vol);

		/// Sets up a default Language.
		void defaultLanguage();
		/// Loads a Language for the Game.
		void loadLanguage(const std::string& file);
		/// Gets the Game's currently loaded Language.
		Language* getLanguage() const;

		/// Gets the Game's currently loaded ResourcePack.
		ResourcePack* getResourcePack() const;
		/// Sets a ResourcePack for the Game.
		void setResourcePack(ResourcePack* const res);

		/// Loads a Ruleset for the Game.
		void loadRuleset();
		/// Gets the currently loaded Ruleset.
		Ruleset* getRuleset() const;

		/// Sets the SavedGame.
		void setSavedGame(SavedGame* const gameSave = nullptr);
		/// Gets the currently loaded SavedGame.
		SavedGame* getSavedGame() const;

		/// Gets the Game's display-screen.
		Screen* getScreen() const;
		/// Gets the Game's Cursor.
		Cursor* getCursor() const;
		/// Gets the Game's FpsCounter.
		FpsCounter* getFpsCounter() const;

		/// Gets the country-cycle for debugging country-zones.
		int getDebugCycle() const;
		/// Sets the country-cycle for debugging country-zones.
		void setDebugCycle(const int cycle);
};

}

#endif
