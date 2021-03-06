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

#ifndef OPENXCOM_RESEARCHREQUIREDSTATE
#define OPENXCOM_RESEARCHREQUIREDSTATE

#include "../Engine/State.h"


namespace OpenXcom
{

class RuleItem;
class Text;
class TextButton;
class Window;


/**
 * Window shown when the player researches a weapon before the respective clip.
 */
class ResearchRequiredState
	:
		public State
{

private:
	Text* _txtTitle;
	TextButton* _btnOk;
	Window* _window;


	public:
		/// Creates a ResearchRequired state.
		ResearchRequiredState(
				const std::string& parentType,
				const std::string& dependentType);

		/// Handler for clicking the Ok button.
		void btnOkClick(Action* action);
};

}

#endif
