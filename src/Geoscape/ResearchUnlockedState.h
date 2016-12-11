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

#ifndef OPENXCOM_RESEARCHUNLOCKEDSTATE
#define OPENXCOM_RESEARCHUNLOCKEDSTATE

#include "../Engine/State.h"


namespace OpenXcom
{

class Base;
class RuleResearch;

class Text;
class TextButton;
class TextList;
class Window;


/**
 * Window that informs the player of Research projects that are recently
 * available.
 * @note Also allows the player to go to ResearchState to allocate scientists.
 */
class ResearchUnlockedState
	:
		public State
{

private:
	Base* _base;
	Text* _txtTitle;
	TextList* _lstPossibilities;
	TextButton
		* _btnResearch,
		* _btnOk;
	Window* _window;


	public:
		/// Creates a ResearchUnlocked state.
		ResearchUnlockedState(
				Base* const base,
				const std::vector<const RuleResearch*>& projects,
				bool allocate);
		// Deconstructs the ResearchUnlocked state.
		~ResearchUnlockedState();

		/// Handler for clicking the Ok button.
		void btnOkClick(Action* action);
		/// Handler for clicking the Allocate Research button.
		void btnResearchClick(Action* action);
};

}

#endif
