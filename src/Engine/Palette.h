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

#ifndef OPENXCOM_PALETTE_H
#define OPENXCOM_PALETTE_H

#include <string>

#include <SDL.h>


namespace OpenXcom
{

enum PaletteType
{
	PAL_NONE = -1,		// -1
	PAL_BACKPALS,		//  0
	PAL_BASESCAPE,		//  1
	PAL_BATTLEPEDIA,	//  2
	PAL_BATTLESCAPE,	//  3
	PAL_GEOSCAPE,		//  4
	PAL_GRAPHS,			//  5
	PAL_UFOPAEDIA		//  6
};

enum BackPals
{
	BACKPAL_NONE = -1,	// -1
	BACKPAL_OLIVE,		//  0
	BACKPAL_RUSSET,		//  1
	BACKPAL_RED_L,		//  2
	BACKPAL_RUST,		//  3
	BACKPAL_SEAGREEN,	//  4
	BACKPAL_RED_D,		//  5
	BACKPAL_ORANGE,		//  6
	BACKPAL_BLUE		//  7
};


/**
 * Container for palettes (sets of 8-bpp colors).
 * Works as an encapsulation for SDL's SDL_Color struct and
 * provides shortcuts for common tasks to make code more readable.
 */
class Palette
{

private:
	size_t _count;
	SDL_Color* _colors;


	public:
		/// Position of the background colors block in an X-Com palette (used for background images in screens).
		static const int PAL_bgID = 224;

		/// Creates a blank palette.
		Palette();
		/// Cleans up the palette.
		~Palette();

		/// Loads the colors from an X-Com palette.
		void loadDat(
				const std::string& file,
				int qColors,
				int offset = 0);

		/// Gets a certain color from the palette.
		SDL_Color* getColors(int offset = 0) const;

		/// Converts a given color into a RGBA color value.
		static Uint32 getRGBA(
				const SDL_Color* const pal,
				Uint8 color);

		/// Gets the position of a given palette.
		/**
		 * Returns the position of a palette inside an X-Com palette file (each is a 768-byte chunks).
		 * Handy for loading the palettes from the game files.
		 * @param palette - requested palette
		 * @return, palette position in bytes
		 */
		static inline int palOffset(int palette)
		{ return palette * (768 + 6); }

		/// Gets the position of a certain color block in a palette.
		/**
		 * Returns the position of a certain color block in an X-Com palette (they're usually split in 16-color gradients).
		 * Makes setting element colors a lot easier than determining the exact color position.
		 * @param block - requested block
		 * @return, color position
		 */
		static inline Uint8 blockOffset(Uint8 block)
		{ return (static_cast<Uint8>(block << 4)); }

		///
//		void setColors(SDL_Color* pal, int qColors);
		///
//		void savePal(const std::string& file) const;
};

}

#endif
