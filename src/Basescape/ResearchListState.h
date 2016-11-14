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

#ifndef OPENXCOM_RESEARCHLISTSTATE_H
#define OPENXCOM_RESEARCHLISTSTATE_H

//#include <vector>

#include "../Engine/State.h"


namespace OpenXcom
{

class Base;
class ResearchProject;
class RuleResearch;
class Text;
class TextButton;
class TextList;
class Window;


/**
 * Window which displays possible research projects.
 */
class ResearchListState
	:
		public State
{

private:
	static const Uint8 GRAY = 5u;

	int _cutoff;
	size_t _scroll;

	Base* _base;
	Text* _txtTitle;
	TextButton* _btnCancel;
	TextList* _lstResearch;
	Window* _window;

	std::vector<ResearchProject*> _offlineProjects;
	std::vector<const RuleResearch*> _unlocked;

	/// Opens a window to assign scientists.
	void lstStartClick(Action* action);
	/// Fills the ResearchProject list with possible ResearchProjects.
	void fillProjectList();


	public:
		/// Creates the NewResearchList state.
		explicit ResearchListState(Base* const base);
		/// dTor.
		~ResearchListState();

		/// Initializes the state.
		void init() override;

		/// Handler for clicking the OK button.
		void btnCancelClick(Action* action);
};

}

#endif
