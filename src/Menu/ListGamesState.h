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

#ifndef OPENXCOM_LISTGAMESSTATE_H
#define OPENXCOM_LISTGAMESSTATE_H

//#include <string>
//#include <vector>

#include "OptionsBaseState.h"

#include "../Engine/Options.h"
#include "../Engine/State.h"

#include "../Savegame/SavedGame.h"


namespace OpenXcom
{

class ArrowButton;
class Text;
class TextButton;
class TextList;
class Window;


/**
 * A base-class for saved-game screens which provides a common layout and
 * listings.
 */
class ListGamesState
	:
		public State
{

private:
	/// Updates the listorder arrows.
	void updateArrows();

	/// Sorts the List.
	void sortList(SaveSort order);

	/// Handler for clicking the arrow that orders the List by name.
	void sortNameClick(Action* action);
	/// Handler for clicking the arrow that orders the List by date.
	void sortDateClick(Action* action);


	protected:
		bool
			_autoquick,
			_editMode,
			_jogRodent,
			_sortable;
		size_t _firstValid;

		OptionsOrigin _origin;

		ArrowButton
			* _sortName,
			* _sortDate;
		Text
			* _txtTitle,
			* _txtName,
			* _txtDate,
			* _txtDelete,
			* _txtDetails;
		TextButton* _btnCancel;
		TextList* _lstSaves;
		Window* _window;

		std::vector<SaveInfo> _info;

		/// Enables/disables sorting the list.
		void setSortable(bool sortable = true);


		public:
			/// Creates a ListGames state.
			ListGamesState(
					OptionsOrigin origin,
					size_t firstValid,
					bool autoquick);
			/// Cleans up the ListGames state.
			virtual ~ListGamesState();

			/// Sets up the List.
			void init() override;
			/// Checks when popup is done.
			void think() override;

			/// Updates the List.
			virtual void updateList();

			/// Handler for clicking the Cancel button.
			void btnCancelClick(Action* action);
			/// Handler for pressing the Escape key.
			void btnCancelKeypress(Action* action);

			/// Handler for moving the mouse over a List item.
			void lstMouseOver(Action* action);
			/// Handler for moving the mouse outside the List borders.
			void lstMouseOut(Action* action);
			/// Handler for clicking the List.
			virtual void lstPress(Action* action);

			/// Hides/shows the list.
			virtual void hideList(bool hide = true);
};

}

#endif
