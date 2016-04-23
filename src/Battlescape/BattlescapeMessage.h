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

#ifndef OPENXCOM_BATTLESCAPEMESSAGE_H
#define OPENXCOM_BATTLESCAPEMESSAGE_H

//#include <string>

#include "../Engine/Surface.h"


namespace OpenXcom
{

class Font;
class Text;
class Window;


/**
 * Generic window used to display messages over the Battlescape map.
 */
class BattlescapeMessage final
	:
		public Surface
{

private:
	Window* _window;
	Text* _text;


	public:
		/// Creates a BattlescapeMessage with the specified size and position.
		BattlescapeMessage(
				int width,
				int height,
				int x = 0,
				int y = 0);
		/// Cleans up the BattlescapeMessage.
		~BattlescapeMessage();

		/// Sets the x-position of the surface.
		void setX(int x) override;
		/// Sets the y-position of the surface.
		void setY(int y) override;

		/// Sets the BattlescapeMessage's background.
		void setBackground(Surface* const bg);
		/// Sets the BattlescapeMessage's text.
		void setText(const std::wstring& message);
		/// Initializes the BattlescapeMessage's resources.
		void initText(
				Font* const big,
				Font* const small,
				const Language* const lang) override;
		/// Sets the BattlescapeMessage's palette.
		void setPalette(
				SDL_Color* const colors,
				int firstcolor = 0,
				int ncolors = 256) override;
		/// Blits the BattlescapeMessage.
		void blit(const Surface* const srf) override;

		/// Special handling for setting the height of the BattlescapeMessage.
		void setHeight(int height) override;

		/// Sets the text-color of the BattlescapeMessage.
		void setTextColor(Uint8 color);
};

}

#endif
