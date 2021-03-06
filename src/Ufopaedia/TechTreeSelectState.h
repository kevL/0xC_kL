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

#ifndef OPENXCOM_TECHTREESELECTSTATE
#define OPENXCOM_TECHTREESELECTSTATE

#include "../Engine/State.h"


namespace OpenXcom
{

class TechTreeViewerState;

class Text;
class TextButton;
class TextEdit;
class TextList;
class Window;


/**
 * Window that allows selecting a topic for the TechTreeViewer.
 */
class TechTreeSelectState
	:
		public State
{

private:
	size_t _firstManufactureId;

	std::vector<std::string> _topics;

	TechTreeViewerState* _viewer;

	Surface* _srfSearchField;
	Text* _txtTitle;
	TextButton* _btnOk;
	TextEdit* _edtQuickSearch;
	TextList* _lstTopics;
	Window* _window;

	/// Populates the topics.
	void fillTechTreeLists();
	/// Selects a topic.
	void lstTopicClick(Action* action);


	public:
		/// Creates a TechTreeSelect state.
		explicit TechTreeSelectState(TechTreeViewerState* const viewer);
		/// Cleans up the TechTreeSelect state.
		~TechTreeSelectState();

		/// Initializes the State.
		void init() override;

		/// Handler for clicking the Ok button.
		void btnOkClick(Action* action);

		/// Handler for QuickSearch toggle.
		void keyQuickSearchToggle(Action* action);
		/// Handler for QuickSearch apply.
		void keyQuickSearchApply(Action* action);
};

}

#endif
