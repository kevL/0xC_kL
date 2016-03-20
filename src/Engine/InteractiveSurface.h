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

#ifndef OPENXCOM_INTERACTIVE_SURFACE_H
#define OPENXCOM_INTERACTIVE_SURFACE_H

#include <map>

#include <SDL.h>

#include "State.h"
#include "Surface.h"


namespace OpenXcom
{

typedef void (State::*ActionHandler)(Action*);


/**
 * Surface that the user can interact with.
 * @note Specialized version of the standard Surface that processes all the
 * various SDL events and turns them into useful interactions with the Surface
 * so specialized subclasses don't need to worry about it.
 */
class InteractiveSurface
	:
		public Surface
{

private:
	static const Uint8 NUM_BUTTONS = 7;
	static const SDLKey SDLK_ANY;

	Uint8 _buttonsPressed;


	protected:
		bool
			_isFocused,
			_isHovered,
			_listButton;

		ActionHandler
			_in,
			_out,
			_over;

		std::map<SDLKey, ActionHandler>
			_keyPress,
			_keyRelease;
		std::map<Uint8, ActionHandler>
			_click,
			_press,
			_release;

		/// Checks if mouse button is pressed.
		bool isButtonPressed(Uint8 btn = 0);
		/// Checks if mouse button event handled.
		virtual bool isButtonHandled(Uint8 btn = 0);
		/// Set a mouse button's internal state.
		void setButtonPressed(
				Uint8 btn,
				bool pressed);


		public:
			/// Creates an InteractiveSurface with the specified size and position.
			InteractiveSurface(
					int width,
					int height,
					int x = 0,
					int y = 0);
			/// Cleans up the Interactivesurface.
			virtual ~InteractiveSurface();

			/// Sets the Surface's visibility.
			void setVisible(bool visible = true) override;

			/// Processes any pending events.
			virtual void handle(
					Action* action,
					State* state);

			/// Sets the focus of the Surface.
			virtual void setFocus(bool focus = true);
			/// Gets the focus of the Surface.
			bool isFocused() const;

			/// Unpresses the Surface.
			virtual void unpress(State* state);

			/// Processes a mouse button press event.
			virtual void mousePress(Action* action, State* state);
			/// Processes a mouse button release event.
			virtual void mouseRelease(Action* action, State* state);
			/// Processes a mouse click event.
			virtual void mouseClick(Action* action, State* state);
			/// Processes a mouse hover in event.
			virtual void mouseIn(Action* action, State* state);
			/// Processes a mouse hover event.
			virtual void mouseOver(Action* action, State* state);
			/// Processes a mouse hover out event.
			virtual void mouseOut(Action* action, State* state);

			/// Processes a keyboard key press event.
			virtual void keyboardPress(Action* action, State* state);
			/// Processes a keyboard key release event.
			virtual void keyboardRelease(Action* action, State* state);

			/// Hooks an ActionHandler to a mouse click on the Surface.
			void onMouseClick(
					ActionHandler handler,
					Uint8 btn = SDL_BUTTON_LEFT);
			/// Hooks an ActionHandler to a mouse press over the Surface.
			void onMousePress(
					ActionHandler handler,
					Uint8 btn = 0);
			/// Hooks an ActionHandler to a mouse release over the Surface.
			void onMouseRelease(
					ActionHandler handler,
					Uint8 btn = 0);
			/// Hooks an ActionHandler to moving the mouse into the Surface.
			void onMouseIn(ActionHandler handler);
			/// Hooks an ActionHandler to moving the mouse over the Surface.
			void onMouseOver(ActionHandler handler);
			/// Hooks an ActionHandler to moving the mouse out of the Surface.
			void onMouseOut(ActionHandler handler);

			/// Hooks an ActionHandler to pressing a key when the Surface is focused.
			void onKeyboardPress(
					ActionHandler handler,
					SDLKey key = SDLK_ANY);
			/// Hooks an ActionHandler to releasing a key when the Surface is focused.
			void onKeyboardRelease(
					ActionHandler handler,
					SDLKey key = SDLK_ANY);

			/// Sets the InteractiveSurface as a TextList button.
			void setListButton();
};

}

#endif
