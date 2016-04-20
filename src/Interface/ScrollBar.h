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

#ifndef OPENXCOM_SCROLLBAR_H
#define OPENXCOM_SCROLLBAR_H

#include "../Engine/InteractiveSurface.h"


namespace OpenXcom
{

enum ScrollDirection
{
	MSCROLL_NONE,	// 0
	MSCROLL_UP,		// 1
	MSCROLL_DOWN	// 2
};


class TextList;
class Timer;


/**
 * A vertical scrollbar-control for TextLists.
 */
class ScrollBar final
	:
		public InteractiveSurface
{

private:
	bool
		_contrast,
		_useScalePad;
	int _offset;
	Uint8 _color;

	SDL_Rect _rect;

	Surface
		* _bg,
		* _track,
		* _btn;
	TextList* _list;
	Timer
		* _timerScrollKey,
		* _timerScrollMouse;

	ScrollDirection _scrollDir;

	/// Scrolls the TextList with the keyboard.
	void keyScroll();
	/// Scrolls the TextList with the mouse.
	void mouseScroll();

	/// Draws the ScrollBar track.
	void drawTrack();
	/// Draws the ScrollBar button.
	void drawButton();


	public:
		/// Creates a ScrollBar with the specified size and position.
		ScrollBar(
				int width,
				int height,
				int x = 0,
				int y = 0);
		/// Cleans up the ScrollBar.
		~ScrollBar();

		/// Sets the x-position of the Surface.
		void setX(int x) override;
		/// Sets the y-position of the Surface.
		void setY(int y) override;
		/// Sets the height of the Surface.
		void setHeight(int height) override;

		/// Sets the ScrollBar's color.
		void setColor(Uint8 color) override;
		/// Gets the ScrollBar's color.
		Uint8 getColor() const;
		/// Sets the ScrollBar's high-contrast color-setting.
		void setHighContrast(bool contrast = true) override;

		/// Sets the ScrollBar's TextList.
		void setTextList(TextList* const textList);

		/// Sets the background for the track.
		void setBackground(Surface* const bg);

		/// Sets the ScrollBar's Palette.
		void setPalette(
				SDL_Color* const colors,
				int firstcolor = 0,
				int ncolors = 256) override;

		/// Blits the ScrollBar onto another Surface.
		void blit(Surface* surface) override;

		/// Moves the TextList on mouse-drag.
		void handle(Action* action, State* state) override;
		/// Special handling for mouse-presses.
		void mousePress(Action* action, State* state) override;
		/// Special handling for mouse-releases.
		void mouseRelease(Action* action, State* state) override;
		/// Handles keyboard-presses.
		void keyboardPress(Action* action, State* state) override;
		/// Handles keyboard-releases.
		void keyboardRelease(Action* action, State* state) override;

		/// Gives ticks to mouse- and keyboard-events.
		void think() override;

		/// Draws the ScrollBar.
		void draw() override;
};

}

#endif
