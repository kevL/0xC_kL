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

#ifndef OPENXCOM_UFOPAEDIASELECTSTATE_H
#define OPENXCOM_UFOPAEDIASELECTSTATE_H

//#include <string>

#include "Ufopaedia.h"

#include "../Engine/State.h"


namespace OpenXcom
{

class Action;
class Text;
class TextButton;
class TextList;
class Window;


/**
 * UfopaediaSelectState is the screen that lists articles of a specified
 * section-type.
 */
class UfopaediaSelectState
	:
		public State
{

protected:
	std::string _section;

	Text* _txtTitle;
	TextButton* _btnOk;
	TextList* _lstSelection;
	Window* _window;

	ArticleDefinitionList _article_list;


	/// Handler for clicking the Ok button
	void btnOkClick(Action* action);
	/// Handler for clicking the selection list.
	void lstSelectionClick(Action* action);

	/// Loads available articles into the selection-list.
	void loadSelectionList();


	public:
		/// Creates a UfopaediaSelect state.
		UfopaediaSelectState(
				const std::string& section,
				bool tactical);
		/// Destroys the state.
		virtual ~UfopaediaSelectState();

		/// Initializes the state.
		void init() override;
};

}

#endif
