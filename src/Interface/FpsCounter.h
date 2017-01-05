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

#ifndef OPENXCOM_FPSCOUNTER_H
#define OPENXCOM_FPSCOUNTER_H

#include "../Engine/Surface.h"


namespace OpenXcom
{

class Action;
class NumberText;
class Timer;


/**
 * Counts the quantity of graphic-slices per second and displays them with a
 * NumberText Surface.
 */
class FpsCounter final
	:
		public Surface
{

private:
	Uint32 _frames;

	NumberText* _text;
	Timer* _timer;


	public:
		/// Creates an FpsCounter.
		FpsCounter(
				int width,
				int height,
				int x = 0,
				int y = 0);
		/// Cleans up the FpsCounter.
		~FpsCounter();

		/// Sets the FpsCounter's palette.
		void setPalette(
				SDL_Color* const colors,
				int firstcolor = 0,
				int ncolors = 256) override;
		/// Sets the FpsCounter's color.
		void setColor(Uint8 color) override;

		/// Handles keyboard-events.
		void handle(Action* action);

		/// Calls update on schedule.
		void think() override;
		/// Updates the FPS-value.
		void update();

		/// Draws the FpsCounter.
		void draw() override;

		/// Adds a tick.
		void addFrame();
};

}

#endif
