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

#include "ListSaveState.h"

#include "SaveGameState.h"

#include "../Engine/Action.h"
#include "../Engine/CrossPlatform.h"
#include "../Engine/Game.h"
#include "../Engine/Language.h"
//#include "../Engine/Options.h"
#include "../Engine/Palette.h"

#include "../Interface/ArrowButton.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextEdit.h"
#include "../Interface/TextList.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the ListSave screen.
 * @param origin - game section that originated this state
 */
ListSaveState::ListSaveState(OptionsOrigin origin)
	:
		ListGamesState(origin, 1u, false),
		_selected(-1),
		_selectedPre(-1)
{
	_edtSave		= new TextEdit(this, 168, 9);
	_btnSaveGame	= new TextButton(134, 16, 170, 177);
//	_btnSaveGame	= new TextButton(_game->getSavedGame()->isIronman() ? 200 : 80, 16, 60, 172);

	add(_edtSave);
	add(_btnSaveGame, "button", "saveMenus");

	_txtTitle->setText(tr("STR_SELECT_SAVE_POSITION"));

//	if (_game->getSavedGame()->isIronman())
//		_btnCancel->setVisible(false);
//	else
//		_btnCancel->setX(180);

	// Tactical: selected SaveSlot for Battlescape is grayscaled.
	_edtSave->setColor(Palette::blockOffset(10u)); // geo:SLATE
	_edtSave->setHighContrast();
	_edtSave->setVisible(false);
	_edtSave->onKeyboardPress((ActionHandler)& ListSaveState::keySavePress);
	// note: BasescapeState, eg, uses onChange handler.

	_btnSaveGame->setText(tr("STR_OK"));
	_btnSaveGame->onMouseClick((ActionHandler)& ListSaveState::btnSaveClick);
	_btnSaveGame->setVisible(false);

	centerAllSurfaces();
}

/**
 * dTor.
 */
ListSaveState::~ListSaveState()
{}

/**
 * Updates the saved-game list with the current list available.
 */
void ListSaveState::updateList()
{
	_lstSaves->addRow(1, tr("STR_NEW_SAVED_GAME_SLOT").c_str());

	if (_origin != OPT_BATTLESCAPE)
		_lstSaves->setRowColor(0u, _lstSaves->getSecondaryColor());

	ListGamesState::updateList();
}

/**
 * Edits the selected entry.
 * @param action - pointer to an Action
 */
void ListSaveState::lstSavesPress(Action* action)
{
//	if (action->getDetails()->button.button == SDL_BUTTON_RIGHT && _edtSave->isFocused())
//	{
//		_edtSave->setText(L"");
//		_edtSave->setVisible(false);
//		_edtSave->setFocus(false, false);
//		_lstSaves->setScrolling(true);
//	}

	if (_editMode == false)
	{
		switch (action->getDetails()->button.button)
		{
			case SDL_BUTTON_LEFT:
				_editMode = true;
				_btnSaveGame->setVisible();
				_lstSaves->setSelectable(false);
				_lstSaves->setScrollable(false);

				_selectedPre = _selected;
				_selected = static_cast<int>(_lstSaves->getSelectedRow());

				switch (_selectedPre)
				{
					case -1: // first click on the savegame list
						break;

					case 0:
						_lstSaves->setCellText(static_cast<size_t>(_selectedPre), 0u, tr("STR_NEW_SAVED_GAME_SLOT"));
						break;

					default:
						_lstSaves->setCellText(static_cast<size_t>(_selectedPre), 0u, _label);
				}

				_label = _lstSaves->getCellText(_lstSaves->getSelectedRow(), 0u);
				_lstSaves->setCellText(_lstSaves->getSelectedRow(), 0u, L"");

				_edtSave->storeText(_label);

				if (_lstSaves->getSelectedRow() == 0u)
					_label = L"";

				_edtSave->setText(_label);

				_edtSave->setX(_lstSaves->getColumnX(0));
				_edtSave->setY(_lstSaves->getRowY(_selected));
				_edtSave->setVisible();
				_edtSave->setFocus(true, false); // note: modal=false allows keypress Enter to save.

				ListGamesState::disableSort();
				break;

			case SDL_BUTTON_RIGHT:
				ListGamesState::lstSavesPress(action); // -> delete file
		}
	}
}

/**
 * Saves the focused entry or cancels edit.
 * @param action - pointer to an Action
 */
void ListSaveState::keySavePress(Action* action)
{
	if (_editMode == true)
	{
		if (action->getDetails()->key.keysym.sym == Options::keyOk
			|| action->getDetails()->key.keysym.sym == Options::keyOkKeypad)
		{
			saveGame();
		}
		else if (action->getDetails()->key.keysym.sym == Options::keyCancel)
		{
			_btnSaveGame->setVisible(false);
			_lstSaves->setSelectable();
			_lstSaves->setScrollable();

			_edtSave->setVisible(false);
			_edtSave->setFocus(false);

			_lstSaves->setCellText(
								_lstSaves->getSelectedRow(),
								0u,
								_edtSave->getStoredText());
		}
	}
}

/**
 * Saves the focused entry.
 * @param action - pointer to an Action
 */
void ListSaveState::btnSaveClick(Action*)
{
	if (_editMode == true && _selected != -1)
		saveGame();
}

/**
 * Saves the focused entry.
 * @note Helper for btnSaveClick() and keySavePress().
 */
void ListSaveState::saveGame() // private.
{
//	_editMode = false;					// safeties. Should not need these three <- ie.
//	_btnSaveGame->setVisible(false);	// SaveGameState() below_ pops current state(s) all the way back to play.
//	_lstSaves->setSelectable();
//	_lstSaves->setScrollable();			// don't need this either ....

	hideElements();
	_game->getSavedGame()->setName(_edtSave->getText());

	std::string
		file (CrossPlatform::sanitizeFilename(Language::wstrToFs(_edtSave->getText()))),
		fileOld;

	if (_selected > 0)
	{
		if ((fileOld = _saves[static_cast<size_t>(_selected - 1)].file) != file + SavedGame::SAVE_EXT)
		{
			while (CrossPlatform::fileExists(Options::getUserFolder() + file + SavedGame::SAVE_EXT) == true)
				file += "_";

			CrossPlatform::moveFile(
								Options::getUserFolder() + fileOld,
								Options::getUserFolder() + file + SavedGame::SAVE_EXT);
		}
	}
	else
	{
		while (CrossPlatform::fileExists(Options::getUserFolder() + file + SavedGame::SAVE_EXT) == true)
			file += "_";
	}

	file += SavedGame::SAVE_EXT;
	_game->pushState(new SaveGameState(_origin, file, _palette));
}

/**
 * Hides textual elements of this state.
 */
void ListSaveState::hideElements() // private.
{
	_txtTitle->setVisible(false);
	_txtDelete->setVisible(false);
	_txtName->setVisible(false);
	_txtDate->setVisible(false);
	_sortName->setVisible(false);
	_sortDate->setVisible(false);
	_lstSaves->setVisible(false);
	_edtSave->setVisible(false);
	_txtDetails->setVisible(false);
	_btnCancel->setVisible(false);
	_btnSaveGame->setVisible(false);
}

}
