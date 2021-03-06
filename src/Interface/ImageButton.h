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

#ifndef OPENXCOM_IMAGEBUTTON_H
#define OPENXCOM_IMAGEBUTTON_H

#include "../Engine/InteractiveSurface.h"


namespace OpenXcom
{

/**
 * Regular image that works like a button.
 * @note Unlike the TextButton this button doesn't draw anything on its own. It
 * takes an existing graphic and treats it as a button, inverting colors when
 * necessary. This is used for special buttons like the Geoscape's
 * time-compression and list-scroll arrows.
 */
class ImageButton
	:
		public InteractiveSurface
{

protected:
	bool _inverted;
	Uint8 _color;

	ImageButton** _group;


	public:
		/// Creates an ImageButton with the specified size and position.
		ImageButton(
				int width,
				int height,
				int x,
				int y);
		/// Cleans up the ImageButton.
		virtual ~ImageButton();

		/// Sets the ImageButton's color.
		void setColor(Uint8 color) override;
		/// Gets the ImageButton's color.
		Uint8 getColor() const;

		/// Sets the ImageButton's group.
		void setGroup(ImageButton** group);

		/// Special handling for mouse-presses.
		void mousePress(Action* action, State* state) override;
		/// Special handling for mouse-releases.
		void mouseRelease(Action* action, State* state) override;

		/// Invert a button explicitly either ON or OFF.
		void toggleIb(bool invert);
		/// Forces a group of buttons to automatically switch to this ImageButton.
		void releaseButtonGroup();
};

}

#endif
