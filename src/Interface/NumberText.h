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

#ifndef OPENXCOM_NUMBERTEXT_H
#define OPENXCOM_NUMBERTEXT_H

#include "../Engine/Surface.h"


namespace OpenXcom
{

/**
 * Numeric digits displayed on the screen.
 * @note Takes a number and displays it using a simple hard-coded font.
 */
class NumberText final
	:
		public Surface
{

private:
	static bool init;
	static const size_t DIGITS = 10;
	static const Uint8
		FG =  1,
		BG = 14;
	static const int
		WIDTH    = 3,
		HEIGHT   = 5,
		WIDTH_B  = 4,
		HEIGHT_B = 6;

	static Surface
		* _chars[DIGITS + 1u],
		* _charsBorder[DIGITS];

	bool
		_append,
		_bordered;
	unsigned _value;
	Uint8 _color;

	/// Creates digits as Surfaces.
	static void createStaticSurfaces();


	public:
		/// Creates a new NumberText with the specified size and position.
		NumberText(
				int width,
				int height,
				int x = 0,
				int y = 0,
				bool append = false);
		/// Cleans up the NumberText.
		~NumberText();

		/// Deletes all static Surfaces.
		static void deleteStaticSurfaces();

		/// Sets the NumberText's value.
		void setValue(unsigned value = 0);
		/// Gets the NumberText's value.
		unsigned getValue() const;

		/// Sets the number to have a border or not.
		void setBordered(bool bordered = true);

		/// Sets the NumberText's color.
		void setColor(Uint8 color) override;
		/// Gets the NumberText's color.
		Uint8 getColor() const;

		/// Sets the NumberText's palette.
		void setPalette(
				SDL_Color* colors,
				int firstcolor = 0,
				int ncolors = 256) override;

		/// Draws the NumberText.
		void draw() override;
};

}

#endif
