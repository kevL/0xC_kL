/*
 * Copyright 2010-2017 OpenXcom Developers.
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

#ifndef OPENXCOM_BASELABELSTATE_H
#define OPENXCOM_BASELABELSTATE_H

#include "../Engine/State.h"


namespace OpenXcom
{

class Base;
class Globe;
class Text;
class TextButton;
class TextEdit;
class Window;


/**
 * Window used to input a label for a Base.
 * @note Player's first Base uses this screen; additional bases use
 * ConfirmBuildBaseState.
 */
class BaseLabelState
	:
		public State
{

private:
	bool _isFirstBase;

	Base* _base;
	Globe* _globe;
	Text* _txtTitle;
	TextButton* _btnOk;
	TextEdit* _edtLabel;
	Window* _window;


	public:
		/// Creates a BaseLabel state.
		BaseLabelState(
				Base* const base,
				Globe* const globe,
				bool isFirstBase = false);
		/// Cleans up the BaseLabel state.
		~BaseLabelState();

		/// Handler for clicking the Ok button.
		void btnOkClick(Action* action);
		/// Handler for changing text on the label-edit.
		void edtLabelChange(Action* action);
};

}

#endif
