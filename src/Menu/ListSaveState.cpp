/*
 * Copyright 2010-2018 OpenXcom Developers.
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
 * @param origin - game section that originated this state (OptionsBaseState.h)
 */
ListSaveState::ListSaveState(OptionsOrigin origin)
	:
		ListGamesState(origin, 1u, false),
		_sel(-1),
		_sel0(-1)
{
	_edtSave     = new TextEdit(this, 168, 9);
	_btnSaveGame = new TextButton(134, 16, 170, 177);

	add(_edtSave);
	add(_btnSaveGame, "button", "saveMenus");

	_txtTitle->setText(tr("STR_SELECT_SAVE_POSITION"));

	if (_game->getSavedGame()->isIronman() == true)
		_btnCancel->setVisible(false);

	// NOTE: Tactical selected SaveSlot for Battlescape is grayscaled. Except TextEdit.
	_edtSave->setColor(COLOR_EDIT);
	_edtSave->setHighContrast();
	_edtSave->setVisible(false);
	_edtSave->onKeyboardPress(static_cast<ActionHandler>(&ListSaveState::keySavePress));
	// NOTE: BasescapeState, eg, uses onChange handler.

	_btnSaveGame->setText(tr("STR_OK"));
	_btnSaveGame->onMouseClick(static_cast<ActionHandler>(&ListSaveState::btnSaveClick));
	_btnSaveGame->setVisible(false);

	centerSurfaces();
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
void ListSaveState::lstPress(Action* action)
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
			{
				_editMode = true;
				_btnSaveGame->setVisible();
				_lstSaves->setScrollable(false);
				_lstSaves->setSelectable(false);

				_sel0 = _sel;
				const size_t sel (_lstSaves->getSelectedRow());
				_sel = static_cast<int>(sel);

				switch (_sel0)
				{
					case -1: // first click on the list
						break;

					case 0:
						_lstSaves->setCellText(static_cast<size_t>(_sel0), 0u, tr("STR_NEW_SAVED_GAME_SLOT"));
						break;

					default:
						_lstSaves->setCellText(static_cast<size_t>(_sel0), 0u, _label);
				}

				_label = _lstSaves->getCellText(sel, 0u);
				_lstSaves->setCellText(sel, 0u, L"");

				_edtSave->setStoredText(_label);

				if (sel == 0u)
					_label = L"";

				_edtSave->setText(_label);

				_edtSave->setX(_lstSaves->getColumnX(0u));
				_edtSave->setY(_lstSaves->getRowY(static_cast<size_t>(_sel)));
				_edtSave->setVisible();
				_edtSave->setFocusEdit(); // NOTE: modal=false allows keypress Enter to save.

				setSortable(false);
				break;
			}

			case SDL_BUTTON_RIGHT:
				ListGamesState::lstPress(action); // -> delete file
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
		if (   action->getDetails()->key.keysym.sym == Options::keyOk
			|| action->getDetails()->key.keysym.sym == Options::keyOkKeypad)
		{
			saveGame();
		}
		else if (action->getDetails()->key.keysym.sym == Options::keyCancel)
		{
			setSortable();

			_jogRodent = true;

			_btnSaveGame->setVisible(false);
			_lstSaves->setSelectable();
			_lstSaves->setScrollable();

			_edtSave->setVisible(false);
			_edtSave->setFocus(false); // NOTE: Not using setFocusEdit().

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
	if (_editMode == true && _sel != -1)
		saveGame();
}

/**
 * Saves the focused entry.
 * @note Helper for btnSaveClick() and keySavePress().
 */
void ListSaveState::saveGame() // private.
{
//	_editMode = false;					// safeties. Should not need these three <- ie.
//	_btnSaveGame->setVisible(false);	// SaveGameState() below_ pops stacked State(s) all the way back to play.
//	_lstSaves->setSelectable();
//	_lstSaves->setScrollable();			// don't need this either ....

	hideList();

	std::wstring label (_edtSave->getText());

	if (label.empty() == true && _game->getSavedGame()->isIronman() == true)
		label = SavedGame::SAVELABEL_Ironballs;

	_game->getSavedGame()->setLabel(label);

	std::string file (CrossPlatform::sanitizeFilename(Language::wstrToFs(label)));
	const std::string  fe (file + SavedGame::SAVE_ExtDot);
	const std::string pfe (Options::getUserFolder() + fe);

	if (_sel > 0)
	{
		const std::string fileinfo (_info[static_cast<size_t>(_sel - 1)].file);
		if (fileinfo != fe)
		{
			while (CrossPlatform::fileExists(pfe) == true)
				file += "_";

			CrossPlatform::moveFile(
								Options::getUserFolder() + fileinfo,
								pfe);
		}
	}
	else
	{
		while (CrossPlatform::fileExists(pfe) == true)
			file += "_";
	}

	_game->pushState(new SaveGameState(
									_origin,
									file + SavedGame::SAVE_ExtDot,
									_palette));
}

/**
 * Hides/shows the list.
 * @param hide - true to hide all elements of the state (default true)
 */
void ListSaveState::hideList(bool hide) // override
{
	ListGamesState::hideList(hide);

	_edtSave	->setVisible(!hide);
	_btnSaveGame->setVisible(!hide);
}

}
