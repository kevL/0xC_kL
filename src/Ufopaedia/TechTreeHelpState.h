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

#ifndef OPENXCOM_TECHTREEHELPSTATE
#define OPENXCOM_TECHTREEHELPSTATE

#include "../Engine/State.h"


namespace OpenXcom
{

class Text;
class TextButton;
class TextList;
class Window;


/**
 * Window that shows Research Help for a specified topic in the TechTree.
 */
class TechTreeHelpState
	:
		public State
{

private:
	Text
		* _txtTitle,
		* _txtTopic;
	TextButton* _btnOk;
	TextList* _lstAliens;
	Window* _window;


	public:
		/// Creates a TechTreeHelp state.
		explicit TechTreeHelpState(const std::string& selTopic);
		/// Cleans up the TechTreeHelp state.
		~TechTreeHelpState();

		/// Initializes the State.
//		void init() override;

		/// Handler for clicking the Ok button.
		void btnOkClick(Action* action);
};

}

#endif
