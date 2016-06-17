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

#ifndef OPENXCOM_WARNINGMESSAGE_H
#define OPENXCOM_WARNINGMESSAGE_H

#include "../Engine/Surface.h"


namespace OpenXcom
{

class Font;
class Text;
class Timer;


/**
 * Colored box with text inside that fades out after it is displayed.
 * @note Used to display warning/error messages on the Battlescape.
 */
class WarningMessage final
	:
		public Surface
{

private:
	Uint8
		_color,
		_fadeStep;

	Text* _text;
	Timer* _timer;


	public:
		/// Creates a WarningMessage with the specified size and position.
		WarningMessage(
				int width,
				int height,
				int x = 0,
				int y = 0);
		/// Cleans up the WarningMessage.
		~WarningMessage();

		/// Sets the color for the WarningMessage.
		void setColor(Uint8 color) override;
		/// Sets the text color for the WarningMessage.
		void setTextColor(Uint8 color);

		/// Initializes the WarningMessage's resources.
		void initText(
				Font* const big,
				Font* const small,
				const Language* const lang) override;
		/// Sets the WarningMessage's palette.
		void setPalette(
				SDL_Color* const colors,
				int firstcolor = 0,
				int ncolors = 256) override;

		/// Shows the WarningMessage.
		void showMessage(const std::wstring& wst);

		/// Handles the timers.
		void think() override;
		/// Fades the message.
		void fade();
		/// Draws the message.
		void draw() override;
};

}

#endif
