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

#ifndef OPENXCOM_FONT_H
#define OPENXCOM_FONT_H

#include <map>
#include <string>

#include <SDL/SDL.h>

#include <yaml-cpp/yaml.h>


namespace OpenXcom
{

class Surface;


/**
 * Takes care of loading and storing each character in a sprite font.
 * @note Sprite fonts consist of a set of fixed-size characters all lined up in
 * one column in a surface.
 * @note The characters don't all need to be the same size - they can have blank
 * space and will be automatically lined up properly.
 */
class Font
{

private:
	static std::wstring _index;

	static SDL_Color
		_palette[6u],
		_terminal[2u];

	bool _monospace;
	int
		_width,
		_height,
		_spacing; // for some reason the X-Com small font is smooshed together by one pixel...

	Surface* _surface;

	std::map<wchar_t, SDL_Rect> _chars;


	public:
		/// Creates a Font.
		Font();
		/// Cleans up the Font.
		~Font();

		/// Checks if a character is a linebreak.
		static inline bool isLinebreak(wchar_t c)
		{ return (c == L'\n' || c == L'\x02'); }
		/// Checks if a character is a blank space (includes non-breaking spaces).
		static inline bool isSpace(wchar_t c)
		{ return (c == L' ' || c == L'\xA0'); }
		/// Checks if a character is a word separator.
		static inline bool isSeparator(wchar_t c)
		{ return (c == L'-' || c == '/'); }
		/// Checks if a character is a non-breaking space.
		static inline bool isNonBreakableSpace(wchar_t c)
		{ return (c == L'\xA0'); }

		/// Sets the index for every character.
		static void setIndex(const std::wstring& index);

		/// Loads the Font from YAML.
		void load(const YAML::Node& node);

		/// Generates the terminal Font.
		void loadTerminal();

		/// Determines the size and position of each character in the Font.
		void init();

		/// Gets a particular character from the Font with its real size.
		Surface* getChar(wchar_t fontChar);

		/// Gets the Font's character width.
		int getWidth() const;
		/// Gets the Font's character height.
		int getHeight() const;

		/// Gets the spacing between characters.
		int getSpacing() const;

		/// Gets the size of a particular character.
		SDL_Rect getCharSize(wchar_t fontChar);

		/// Gets the Font's Surface.
		Surface* getSurface() const;
};

}

#endif
