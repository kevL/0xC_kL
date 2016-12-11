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

#ifndef OPENXCOM_ERRORMESSAGESTATE_H
#define OPENXCOM_ERRORMESSAGESTATE_H

//#include <string>

#include "../Engine/State.h"


namespace OpenXcom
{

class Text;
class TextButton;
class Window;


/**
 * Generic window used to display error messages.
 */
class ErrorMessageState
	:
		public State
{

private:
	bool _quit;

	TextButton* _btnOk;
	Window* _window;
	Text* _txtMessage;

	void create(
			const std::string& st,
			const std::wstring& wst,
			SDL_Color* palette,
			Uint8 color,
			const std::string& bg,
			int bgColor);

	public:
		/// Creates an ErrorMessage state w/ string.
		ErrorMessageState(
				const std::string& st,
				SDL_Color* palette,
				int color,
				const std::string& bg,
				int bgColor,
				bool quit = false);
		/// Creates an ErrorMessage state w/ wide-string.
		ErrorMessageState(
				const std::wstring& wst,
				SDL_Color* palette,
				int color,
				const std::string& bg,
				int bgColor,
				bool quit = false);
		/// Cleans up the ErrorMessage state.
		~ErrorMessageState();

		/// Handler for clicking the Ok button.
		void btnOkClick(Action* action);
};

}

#endif
