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

#ifndef OPENXCOM_SCREEN_H
#define OPENXCOM_SCREEN_H

//#include <string>

//#include <SDL/SDL.h>

#include "OpenGL.h"


namespace OpenXcom
{

class Action;
class Surface;


/**
 * A display Screen - handles rendering onto the game-window.
 * @note In SDL a Screen is treated like a Surface so this is just a specialized
 * version of a Surface with functionality more relevant for display screens.
 * Contains a Surface buffer where all the contents are kept so any filters or
 * conversions can be applied before rendering the Screen.
 */
class Screen
{

private:
	bool _pushPalette;
	int
		_baseHeight,
		_baseWidth,
		_bpp,
		_firstColor,
		_qtyColors,
		_borderTop,
		_borderBot,
		_borderLeft,
		_borderRight,
		_borderTopCursor,
		_borderLeftCursor;
	double
		_scaleX,
		_scaleY;
	Uint32 _flags;

	std::string _shader;

	SDL_Rect _clear;

	SDL_Surface* _screen;
	Surface* _surface;

	OpenGL _glOutput;
	SDL_Color _deferredPalette[256u];

	/// Sets the '_flags' and '_bpp' and base-resolution variables etc.
	void setVideoFlags();


	public:
		static const int
			ORIGINAL_WIDTH	= 320,
			ORIGINAL_HEIGHT	= 200;

		static const Uint32 SCREEN_PAUSE = 335u;

		/// Creates a display-screen.
		Screen();
		/// Cleans up the display-screen.
		~Screen();

		/// Gets the internal buffer.
		Surface* getSurface();

		/// Handles keyboard-events.
		void handle(Action* action);

		/// Renders the Screen to the video-card's output.
		void flip();
		/// Clears the Screen.
		void clear();

		/// Resets the Screen's display.
		void resetDisplay(bool resetVideo = true);
		/// Resets the OpenGL shader.
		void resetShader();

		/// Sets the Screen's 8-bpp palette.
		void setPalette(
				SDL_Color* const colors,
				int firstcolor = 0,
				int ncolors = 256,
				bool immediately = false);
		/// Gets the Screen's 8-bpp palette.
		SDL_Color* getPalette() const;

		/// Gets the Screen's width.
		int getWidth() const;
		/// Gets the Screen's height.
		int getHeight() const;

		/// Gets the Screen's x-scale.
		double getScaleX() const;
		/// Gets the Screen's y-scale.
		double getScaleY() const;

		/// Gets the horizontal-offset.
		int getDX() const;
		/// Gets the vertical-offset.
		int getDY() const;

		/// Gets the Screen's top black forbidden-to-cursor band's height.
		int getBorderTop() const;
		/// Gets the Screen's left black forbidden-to-cursor band's width.
		int getBorderLeft() const;

		/// Takes a screenshot.
		void screenshot(const std::string& file) const;

		/// Checks whether a 32-bit scaler is requested and works for the selected resolution.
		static bool use32bitScaler();
		/// Checks whether OpenGL output is requested.
		static bool useOpenGL();

		/// Updates the video-scale as required.
		static void updateScale(
				int& type,
				int selection,
				int& x,
				int& y,
				bool change);

		/// Fades the Screen.
		void fadeScreen(
				Uint8 steps = 10u,
				Uint32 delay = 10u);
};

}

#endif
