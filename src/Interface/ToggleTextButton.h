/*
 * Copyright 2010-2018 OpenXcom Developers.
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

#ifndef OPENXCOM_TOGGLETEXTBUTTON_H
#define OPENXCOM_TOGGLETEXTBUTTON_H

#include "TextButton.h"


namespace OpenXcom
{

/**
 * A child-class of TextButton that stays depressed.
 */
class ToggleTextButton final
	:
		public TextButton
{

private:
	bool _isPressed;
	Uint8
		_color,
		_colorInvert;

	TextButton* _fakeGroup;


	public:
		/// Constructs a ToggleTextButton.
		ToggleTextButton(
				int width,
				int height,
				int x,
				int y);
		/// Destructs the ToggleTextButton.
		~ToggleTextButton(void);

		/// Draws the ToggleTextButton.
		void draw() override;

		/// Sets the color of the ToggleTextButton.
		void setColor(Uint8 color) override;
		/// Sets the depressed color of the ToggleTextButton.
		void setColorInvert(Uint8 color);

		/// Handles mouse-presses.
		void mousePress(Action* action, State* state) override;

		/// Sets the ToggleTextButton as depressed.
		void setPressed(bool press);
		/// Checks if the ToggleTextButton is depressed.
		bool getPressed() const
		{ return _isPressed; }
};

}

#endif
