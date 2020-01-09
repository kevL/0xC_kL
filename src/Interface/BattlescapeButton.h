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

#ifndef OPENXCOM_BATTLESCAPEBUTTON_H
#define OPENXCOM_BATTLESCAPEBUTTON_H

#include "../Engine/InteractiveSurface.h"


namespace OpenXcom
{

enum InversionType
{
	INVERT_NONE,	// 0
	INVERT_CLICK,	// 1
	INVERT_TOGGLE	// 2
};

/**
 * Regular image that works like a button.
 * @note Unlike the TextButton this button doesn't draw anything on its own. It
 * takes an existing graphic and treats it as a button and inverts colors when
 * necessary. This is necessary for special buttons like in the Geoscape. Hence
 * the name, "BattlescapeButton".
 */
class BattlescapeButton final
	:
		public InteractiveSurface
{

protected:
	bool _inverted;
	Uint8 _color;

	InversionType _toggleMode;

	BattlescapeButton** _group;
	Surface* _altSurface;


	public:
		/// Creates a BattlescapeButton with the specified size and position.
		BattlescapeButton(
				int width,
				int height,
				int x,
				int y);
		/// Cleans up the BattlescapeButton.
		virtual ~BattlescapeButton();

		/// Sets the BattlescapeButton's color.
		void setColor(Uint8 color) override;
		/// Gets the BattlescapeButton's color.
		Uint8 getColor() const;

		/// Sets up the "pressed" surface.
		void altSurface();
		/// Blits this Surface onto another one.
		void blit(const Surface* const srf) override;

		/// Alters both versions of the button's x-pos.
		void setX(int x) override;
		/// Alters both versions of the button's y-pos.
		void setY(int y) override;

		/// Sets the BattlescapeButton's group.
//		void setGroup(BattlescapeButton** group);
		/// Special handling for mouse-presses.
//		void mousePress(Action* action, State* state) override;
		/// Special handling for mouse-releases.
//		void mouseRelease(Action* action, State* state) override;

		/// Invert a button explicitly either ON or OFF.
//		void toggleBb(bool invert);
		/// Allows this BattlescapeButton to be toggled on/off with a click.
//		void allowToggleInversion();
		/// Allows this BattlescapeButton to be toggled on when clicked and off when released.
//		void allowClickInversion();
};

}

#endif
