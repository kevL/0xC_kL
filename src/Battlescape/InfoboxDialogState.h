/*
 * Copyright 2010-2019 OpenXcom Developers.
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

#ifndef OPENXCOM_INFOBOXDIALOGSTATE_H
#define OPENXCOM_INFOBOXDIALOGSTATE_H

//#include <string>

#include "../Engine/State.h"


namespace OpenXcom
{

class Frame;
class Text;
class TextButton;


/**
 * Notifies the player about things like soldiers going unconscious or dying
 * from fatal wounds and must be closed by the Player.
 */
class InfoboxDialogState
	:
		public State
{

private:
	bool
		_cursorVisible,
		_cursorHidden;

	Frame* _frame;
	Text* _txtTitle;
	TextButton* _btnOk;


	public:
		/// Creates the InfoboxDialogState.
		explicit InfoboxDialogState(const std::wstring& wst);
		/// Cleans up the InfoboxDialogState.
		~InfoboxDialogState();

		/// Handler for clicking the Ok button.
		void btnOkClick(Action* action);
};

}

#endif
