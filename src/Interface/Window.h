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

#ifndef OPENXCOM_WINDOW_H
#define OPENXCOM_WINDOW_H

#include "../Engine/Surface.h"


namespace OpenXcom
{

class Sound;
class State;
class Timer;


/**
 * The direction-type of animation when a window pops up.
 */
enum PopupType
{
	POPUP_NONE,			// 0
	POPUP_HORIZONTAL,	// 1
	POPUP_VERTICAL,		// 2
	POPUP_BOTH			// 3
};


/**
 * Box with a colored border and specified background.
 * @note Pretty much used as the background in most of the interface. In fact
 * it's also used in screens so it's not really much of a window - it's just a
 * ... a box.
 * But box sounds lame. Think window.
 */
class Window final
	:
		public Surface
{

private:
	static const float POPUP_SPEED;
	static const int
		POP_START	= 0,
		POP_GO		= 1,
		POP_HALT	= 2;

	bool
		_contrast,
		_fullScreen,
		_thinBorder,
		_toggle;
	int
		_dX,_dY,
		_bgX,_bgY,
		_popProgress;
	float _popStep;

	Uint8
		_color,
		_colorFill;

	State* _state;
	Surface* _bg;
	Timer* _timer;

	PopupType _popType;

	/// Popups the Window.
	void popup();


	public:
		static Sound* soundPopup[3u];

		/// Creates a Window with the specified size and position.
		Window(
				State* const state,
				int width = 320,
				int height = 200,
				int x = 0,
				int y = 0,
				PopupType popType = POPUP_NONE,
				bool toggle = true);
		/// Cleans up the Window.
		~Window();

		/// Handles the Timer.
		void think() override;
		/// Gets if the Window has finished popping up.
		bool isPopupDone() const;

		/// Draws the Window.
		void draw() override;

		/// Sets the Window's border-color.
		void setColor(Uint8 color) override;
		/// Gets the Window's border-color.
		Uint8 getColor() const;

		/// Sets the Window's high-contrast color setting.
		void setHighContrast(bool contrast = true) override;

		/// Sets the Window's background Surface.
		void setBackground(
				Surface* const bg,
				int dX = 0,
				int dY = 0);
		/// Sets the Window's background to a solid color instead of transparent.
		void setBackgroundFill(Uint8 color);

		/// Sets the Window's x-delta.
		void setDX(int dX);
		/// Sets the Window's y-delta.
		void setDY(int dY);

		/// Give the Window a thin border.
		void setThinBorder();
};

}

#endif
