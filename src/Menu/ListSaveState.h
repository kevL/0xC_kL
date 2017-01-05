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

#ifndef OPENXCOM_LISTSAVESTATE_H
#define OPENXCOM_LISTSAVESTATE_H

#include "ListGamesState.h"


namespace OpenXcom
{

class TextButton;
class TextEdit;


/**
 * Save Game screen for listing info on available saved games and saving them.
 */
class ListSaveState
	:
		public ListGamesState
{

private:
	static const Uint8 COLOR_EDIT = 160u; // slate in Geoscape/brown in Battlescape.

	int // TODO: These should be size_t's; indepth consideration required ....
		_selectedPre,
		_selected;

	std::wstring _label;

	TextButton* _btnSaveGame;
	TextEdit* _edtSave;

	/// Saves the current game.
	void saveGame();
	/// Hides textual elements of this state.
	void hideElements();


	public:
		/// Creates a ListSave state.
		explicit ListSaveState(OptionsOrigin origin);
		/// Cleans up the ListSave state.
		~ListSaveState();

		/// Updates the List.
		void updateList() override;

		/// Handler for pressing a key on the edit.
		void keySavePress(Action* action);
		/// Handler for clicking on the save-button.
		void btnSaveClick(Action *action);
		/// Handler for pressing the List.
		void lstSavesPress(Action* action) override;
};

}

#endif
