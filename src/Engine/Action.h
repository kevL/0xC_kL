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

#ifndef OPENXCOM_ACTION_H
#define OPENXCOM_ACTION_H

#include <SDL/SDL.h>


namespace OpenXcom
{

class InteractiveSurface;


/**
 * Container for all the information associated with a specified user-event like
 * mouse-clicks, key-presses, etc.
 * @note Called Action because Event is reserved. And don't confuse this with a
 * BattleAction either.
 */
class Action
{

private:
	int
		_mouseX,
		_mouseY,
		_surfaceX,
		_surfaceY,
		_borderTop,
		_borderLeft;
//	Uint32 _rodentState;
	double
		_scaleX,
		_scaleY;

	InteractiveSurface* _sender;
	SDL_Event* _event;


	public:
		/// Creates an Action with specified event-data.
		explicit Action(
				SDL_Event* const event,
				double scaleX = 0.,
				double scaleY = 0.,
				int topBlackBand  = 0,
				int leftBlackBand = 0);
//				Uint32 rodentState);
		/// Cleans up the Action.
		~Action();

		/// Gets the Screen's x-scale.
		double getScaleX() const;
		/// Gets the Screen's y-scale.
		double getScaleY() const;

		/// Sets the Action as a mouse-action.
		void setMouseAction(
				int mX,
				int mY,
				int surfaceX,
				int surfaceY);
		/// Checks if the Action is a mouse-action.
		bool isMouseAction() const;

		/// Checks a specified mouse-button's state.
		bool isMouseButton(int btnId) const;
		/// Gets the overall mouse-buttons' state.
//		Uint32 getMouseState() const;

		/// Gets the top-black-band's height.
		int getBorderTop() const;
		/// Gets the left-black-band's width.
		int getBorderLeft() const;

		/// Gets the mouse's x-position.
		int getMouseX() const;
		/// Gets the mouse's y-position.
		int getMouseY() const;
		/// Gets the mouse's absolute x-position.
		double getAbsoluteMouseX() const;
		/// Gets the mouse's absolute y-position.
		double getAbsoluteMouseY() const;
		/// Gets the mouse's relative x-position.
		double getRelativeMouseX() const;
		/// Gets the mouse's relative y-position.
		double getRelativeMouseY() const;

		/// Sets the sender of the Action.
		void setSender(InteractiveSurface* const sender);
		/// Gets the sender of the Action.
		InteractiveSurface* getSender() const;

		/// Gets the SDL-details of the Action.
		SDL_Event* getDetails() const;
};

}

#endif
