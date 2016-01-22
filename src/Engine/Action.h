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

#ifndef OPENXCOM_ACTION_H
#define OPENXCOM_ACTION_H

#include <SDL.h>


namespace OpenXcom
{

class InteractiveSurface;


/**
 * Container for all the information associated with a given user Action like
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
		_topBlackBand,
		_leftBlackBand;
	double
		_scaleX,
		_scaleY;

	InteractiveSurface* _sender;
	SDL_Event* _event;


	public:
		/// Creates an Action with given event data.
		Action(
				SDL_Event* const event,
				double scaleX,
				double scaleY,
				int topBlackBand,
				int leftBlackBand);
		/// Cleans up the Action.
		~Action();

		/// Gets the screen's X scale.
		double getXScale() const;
		/// Gets the screen's Y scale.
		double getYScale() const;

		/// Sets the Action as a mouse action.
		void setMouseAction(
				int mouseX,
				int mouseY,
				int surfaceX,
				int surfaceY);
		/// Gets if the Action is a mouse action.
		bool isMouseAction() const;

		/// Gets the top black band height.
		int getTopBlackBand() const;
		/// Gets the left black band width.
		int getLeftBlackBand() const;

		/// Gets the mouse's X position.
		int getXMouse() const;
		/// Gets the mouse's Y position.
		int getYMouse() const;
		/// Gets the mouse's absolute X position.
		double getAbsoluteXMouse() const;
		/// Gets the mouse's absolute Y position.
		double getAbsoluteYMouse() const;
		/// Gets the mouse's relative X position.
		double getRelativeXMouse() const;
		/// Gets the mouse's relative Y position.
		double getRelativeYMouse() const;

		/// Gets the sender of the Action.
		InteractiveSurface* getSender() const;
		/// Sets the sender of the Action.
		void setSender(InteractiveSurface* sender);

		/// Gets the details of the Action.
		SDL_Event* getDetails() const;
};

}

#endif
