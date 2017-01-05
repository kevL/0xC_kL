/*
 * Copyright 2010-2017 OpenXcom Developers.
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

#include <list>		// std::list
#include <string>	// std::string

#include <SDL/SDL.h>


namespace OpenXcom
{

class Action;
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
	static SDL_Event
		eventD,
		eventU;

	static Action
		syntheticD,
		syntheticU;


	static const double VOLUME_GRADIENT;

	bool
		_init,
		_inputActive,
		_quit;
	int
		_debugCycle,	// for debugging country-zones.
		_debugCycle_b,	// for debugging country-zones.
		_ticksTillNextSlice;
	Uint32
		_rodentState,
		_tickOfLastSlice;

	Cursor*			_cursor;
	FpsCounter*		_fpsCounter;
	Language*		_lang;
	ResourcePack*	_res;
	Ruleset*		_rules;
	SavedGame*		_playSave;
	Screen*			_screen;

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

		/// Quits.
		void quit(bool force = false);
		/// Checks if the Game is shutting down.
		bool isQuitting() const;

		/// Sets whether the mouse-cursor is activated/responsive.
		void setInputActive(bool active);

		/// Resets a state-stack to a new state.
		void setState(State* const state);
		/// Pushes a state into the state-stack.
		void pushState(State* const state);
		/// Pops the last state from the state-stack.
		void popState();
		/// Gets the current (top) state.
//		State* getState() const;
		/// Gets the quantity of currently running states.
		size_t getQtyStates() const;
		/// Checks whether a state is the current state.
		bool isState(const State* const state) const;

		/// Sets up the audio.
		void initAudio();
		/// Sets the audio-amplitudes.
		void setVolume(
				int music,
				int fx,
				int ui = -1,
				bool force = false);
		/// Adjusts a linear volume-level to an exponential one.
		static double volExp(int vol);

		/// Sets up a default Language.
		void defaultLanguage();
		/// Loads a Language.
		void loadLanguage(const std::string& file);
		/// Gets the currently loaded Language.
		Language* getLanguage() const;

		/// Gets the currently loaded ResourcePack.
		ResourcePack* getResourcePack() const;
		/// Sets a ResourcePack for the Game.
		void setResourcePack(ResourcePack* const res = nullptr);

		/// Loads a Ruleset.
		void loadRulesets();
		/// Gets the currently loaded Ruleset.
		Ruleset* getRuleset() const;

		/// Sets the SavedGame.
		void setSavedGame(SavedGame* const playSave = nullptr);
		/// Gets the currently loaded SavedGame.
		SavedGame* getSavedGame() const;

		/// Gets the Screen.
		Screen* getScreen() const;
		/// Gets the Cursor.
		Cursor* getCursor() const;
		/// Gets the FpsCounter.
		FpsCounter* getFpsCounter() const;

		/// Gets the country-cycle for debugging country-zones.
		int getDebugCycle() const;
		/// Sets the country-cycle for debugging country-zones.
		void setDebugCycle(int cycle);

		/// Gets a synthetic mouse-down Action.
		static Action* getFakeMouseActionD();
		/// Gets a synthetic mouse-up Action.
		static Action* getFakeMouseActionU();
};

}

#endif
