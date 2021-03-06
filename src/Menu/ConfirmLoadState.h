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

#ifndef OPENXCOM_CONFIRMLOADSTATE_H
#define OPENXCOM_CONFIRMLOADSTATE_H

//#include <string>

#include "../Engine/State.h"

#include "OptionsBaseState.h"


namespace OpenXcom
{

class ListLoadState;

class Text;
class TextButton;
class Window;


/**
 * Confirms loading a save with missing content.
 */
class ConfirmLoadState
	:
		public State
{

private:
	OptionsOrigin _origin;

	std::string _file;

	ListLoadState* _parent;
	Text* _txtText;
	TextButton
		* _btnYes,
		* _btnNo;
	Window* _window;


	public:
		/// Creates a ConfirmLoad state.
		ConfirmLoadState(
				OptionsOrigin origin,
				const std::string& file,
				ListLoadState* const parent);
		/// Cleans up the ConfirmLoad state.
		virtual ~ConfirmLoadState();

		/// Handler for clicking the Yes button.
		void btnYesClick(Action* action);
		/// Handler for clicking the No button.
		void btnNoClick(Action* action);
};

}

#endif
