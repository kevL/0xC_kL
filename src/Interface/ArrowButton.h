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

#ifndef OPENXCOM_ARROWBUTTON_H
#define OPENXCOM_ARROWBUTTON_H

#include "ImageButton.h"


namespace OpenXcom
{

enum ArrowShape
{
	ARROW_NONE,			// 0
	ARROW_BIG_UP,		// 1
	ARROW_BIG_DOWN,		// 2
	ARROW_SMALL_UP,		// 3
	ARROW_SMALL_DOWN,	// 4
	ARROW_SMALL_LEFT,	// 5
	ARROW_SMALL_RIGHT	// 6
};


class TextList;
class Timer;


/**
 * Button with an arrow on it.
 * @note Can be used for scrolling lists, spinners, etc. Contains various
 * arrow shapes.
 */
class ArrowButton final
	:
		public ImageButton
{

private:
	static const int WAIT_TICKS = 2;

	int _wait;

	ArrowShape _shape;
	TextList* _list;
	Timer* _timer;


protected:
	/// Checks if the specified mouse-button has been handled.
	bool isButtonHandled(Uint8 btn = 0u) override;


	public:
		/// Creates an ArrowButton with a specified size and position.
		ArrowButton(
				ArrowShape shape,
				int width,
				int height,
				int x = 0,
				int y = 0);
		/// Cleans up the ArrowButton.
		~ArrowButton();

		/// Sets the ArrowButton's color.
		void setColor(Uint8 color) override;
		/// Sets the ArrowButton's shape.
		void setShape(ArrowShape shape);

		/// Sets the ArrowButton's list.
		void setTextList(TextList* const textList);

		/// Handles the timers.
		void think() override;
		/// Scrolls the list.
		void scroll();
		/// Draws the ArrowButton.
		void draw() override;

		/// Special handling for mouse-presses.
		void mousePress(Action* action, State* state) override;
		/// Special handling for mouse-releases.
		void mouseRelease(Action* action, State* state) override;
		/// Special handling for mouse-clicks.
		void mouseClick(Action* action, State* state) override;
};

}

#endif
