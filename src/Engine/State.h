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

#ifndef OPENXCOM_STATE_H
#define OPENXCOM_STATE_H

#include <string>
#include <vector>

#include <SDL/SDL.h>

#include "Palette.h"


namespace OpenXcom
{

class Action;	// wtf why doesn't a guy just define all this crap globally
class Game;		// and be done with it.
class InteractiveSurface;
class LocalizedText;
class RuleInterface;
class Surface;


/**
 * A game-state that receives user input and reacts accordingly.
 * @note Game-states typically represent a whole window or screen that the user
 * interacts with, making the game ... well, interactive. They automatically
 * handle child elements used to transmit information from/to the user and are
 * linked to the core game-engine which manages them.
 */
class State
{
	friend class Timer;


protected:
	static Game* _game;

	bool _fullScreen;
	Uint8 _cursorColor;

	InteractiveSurface* _isfModal;
	RuleInterface
		* _uiRule,
		* _uiRuleParent;

	SDL_Color _palette[256u];

	std::vector<Surface*> _surfaces;

	/// Adjusts the quantity by which to increase/decrease a TextList value.
	static int stepDelta();


	public:
		/// Creates a State linked to the Game.
		State();
		/// Cleans up the State.
		virtual ~State();

		/// Sets the Game-object pointer.
		static void setGamePtr(Game* const ptrG);
		/// Gets the Game-object pointer.
		static Game* getGamePtr();

		/// Gets the label of the State.
//		virtual std::string getStateLabel() const;

		/// Sets the Interface rule.
		void setInterface(
				const std::string& category,
				bool altBackpal = false,
				bool tactical = false);

		/// Adds a child-element to the State.
		void add(Surface* const srf);
		void add(
				Surface* const srf,
				const std::string& id,
				const std::string& category,
				Surface* const parent = nullptr);
		/// Gets whether the State is full-screen.
		bool isFullScreen() const;
		/// Toggles whether the State is full-screen.
		void toggleScreen();

		/// Initializes the State.
		virtual void init();
		/// Runs State functionality every cycle.
		virtual void think();
		/// Blits the State to the screen.
		virtual void blit();

		/// Handles any events.
		virtual void handle(Action* action);

		/// Hides all the State's Surfaces.
		void hideAll();
		/// Shows all the State's Surfaces.
		void showAll();

		/// Resets all the State's Surfaces.
		void resetSurfaces();

		/// Gets a LocalizedText.
		const LocalizedText& tr(const std::string& id) const;
		LocalizedText tr(
				const std::string& id,
				unsigned qty) const;

		/// Redraws all the text-type Surfaces.
		void redrawText();

		/// Centers all Surfaces relative to the Screen's height and width.
		void centerSurfaces();
		/// Lowers all Surfaces by half the Screen's height.
		void lowerSurfaces();

		/// Switches colors to use the Battlescape Palette.
		void applyBattlescapeColors();

		/// Sets a specified InteractiveSurface as modal for the State.
		void setModal(InteractiveSurface* const isf = nullptr);

		/// Changes a set of colors on the State's 8-bpp Palette.
		void setPalette(
				SDL_Color* const colors = nullptr,
				int firstcolor = 0,
				int ncolors = 256,
				bool apply = true);
		/// Changes the State's 8-bpp Palette with specified resources.
		void setPalette(
				const PaletteType palType,
				BackPals backpal = BACKPAL_NONE);
		/// Gets the State's 8-bpp Palette.
		SDL_Color* getPalette();

		/// Lets the State know the window has been resized.
		virtual void resize(
				int& dX,
				int& dY);
		/// Re-orients all Surfaces in the State.
		virtual void recenter(
				int dX,
				int dY);

		/// Forces a transparent SDL mouse-motion event.
		void refreshMousePosition() const;
};

}

#endif
