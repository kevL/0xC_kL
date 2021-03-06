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

#include "ListGamesState.h"

//#include <algorithm>

#include "DeleteSaveState.h"

#include "../Engine/Action.h"
#include "../Engine/CrossPlatform.h"
#include "../Engine/Exception.h"
#include "../Engine/Game.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Logger.h"

#include "../Interface/ArrowButton.h"
#include "../Interface/Cursor.h"
#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextList.h"
#include "../Interface/Window.h"

#include "../Resource/ResourcePack.h"

#include "../Savegame/SavedGame.h"


namespace OpenXcom
{

/**
 * Compares entries in the List by name.
 */
struct compareSaveName
	:
		public std::binary_function<SaveInfo&, SaveInfo&, bool>
{
	bool _reverse;

	///
	explicit compareSaveName(bool rev)
		:
			_reverse(rev)
	{}

	///
	bool operator () (
			const SaveInfo& a,
			const SaveInfo& b) const
	{
		if (a.reserved == b.reserved)
			return CrossPlatform::naturalCompare(
											a.label,
											b.label);

		return _reverse ? b.reserved : a.reserved;
	}
};

/**
 * Compares entries in the List by date.
 */
struct compareSaveTimestamp
	:
		public std::binary_function<SaveInfo&, SaveInfo&, bool>
{
	bool _reverse;

	///
	explicit compareSaveTimestamp(bool rev)
		:
			_reverse(rev)
	{}

	///
	bool operator () (
			const SaveInfo& a,
			const SaveInfo& b) const
	{
		if (a.reserved == b.reserved)
			return a.timestamp < b.timestamp;

		return _reverse ? b.reserved : a.reserved;
	}
};


/**
 * Initializes all the elements in the ListGames screen.
 * @param origin		- game section that originated this state (OptionsBaseState.h)
 * @param firstValid	- first row containing saves
 * @param autoquick		- true to show auto/quick saved games
 */
ListGamesState::ListGamesState(
		OptionsOrigin origin,
		size_t firstValid,
		bool autoquick)
	:
		_origin(origin),
		_firstValid(firstValid),
		_autoquick(autoquick),
		_sortable(true),
		_editMode(false),
		_jogRodent(true)
{
	_fullScreen = false;

	_window     = new Window(this, 320, 200, 0, 0, POPUP_BOTH);

	_txtTitle   = new Text(310, 16, 5,  8);
	_txtDelete  = new Text(310,  9, 5, 24);

	_txtName    = new Text(176, 9,  16, 33);
	_txtDate    = new Text( 84, 9, 204, 33);
	_sortName   = new ArrowButton(ARROW_NONE, 11, 8,  16, 33);
	_sortDate   = new ArrowButton(ARROW_NONE, 11, 8, 204, 33);

	_lstSaves   = new TextList(285, 121, 16, 42);

	_txtDetails = new Text(288, 9, 16, 165);

	_btnCancel  = new TextButton(134, 16, 16, 177);

	setInterface(
			"geoscape",
			true,
			_origin == OPT_BATTLESCAPE);

	add(_window,     "window", "saveMenus");
	add(_txtTitle,   "text",   "saveMenus");
	add(_txtDelete,  "text",   "saveMenus");
	add(_txtName,    "text",   "saveMenus");
	add(_txtDate,    "text",   "saveMenus");
	add(_sortName,   "text",   "saveMenus");
	add(_sortDate,   "text",   "saveMenus");
	add(_lstSaves,   "list",   "saveMenus");
	add(_txtDetails, "text",   "saveMenus");
	add(_btnCancel,  "button", "saveMenus");


	_window->setBackground(_game->getResourcePack()->getSurface("BACK01.SCR"));

	_btnCancel->setText(tr("STR_CANCEL_UC"));
	_btnCancel->onMouseClick(   static_cast<ActionHandler>(&ListGamesState::btnCancelClick));
	_btnCancel->onKeyboardPress(static_cast<ActionHandler>(&ListGamesState::btnCancelKeypress),
								Options::keyCancel);

	_txtTitle->setBig();
	_txtTitle->setAlign(ALIGN_CENTER);

	_txtDelete->setAlign(ALIGN_CENTER);
	_txtDelete->setText(tr("STR_RIGHT_CLICK_TO_DELETE"));

	_txtName->setText(tr("STR_NAME"));

	_txtDate->setText(tr("STR_DATE"));

	_lstSaves->setColumns(3, 188,60,29);
	_lstSaves->setBackground(_window);
	_lstSaves->setSelectable();
	_lstSaves->onMouseOver( static_cast<ActionHandler>(&ListGamesState::lstMouseOver));
	_lstSaves->onMouseOut(  static_cast<ActionHandler>(&ListGamesState::lstMouseOut));
	_lstSaves->onMousePress(static_cast<ActionHandler>(&ListGamesState::lstPress));

	_txtDetails->setWordWrap();
	_txtDetails->setText(tr("STR_DETAILS").arg(L""));

	_sortName->setX(_sortName->getX() + _txtName->getTextWidth() + 5);
	_sortName->onMouseClick(static_cast<ActionHandler>(&ListGamesState::sortNameClick));

	_sortDate->setX(_sortDate->getX() + _txtDate->getTextWidth() + 5);
	_sortDate->onMouseClick(static_cast<ActionHandler>(&ListGamesState::sortDateClick));

	updateArrows();
}

/**
 * dTor.
 */
ListGamesState::~ListGamesState() // virtual.
{}

/**
 * Refreshes the List.
 */
void ListGamesState::init()
{
	State::init();

	if (_origin == OPT_BATTLESCAPE)
		applyBattlescapeColors();

	try
	{
		_info = SavedGame::getList(
								_game->getLanguage(),
								_autoquick);
		sortList(Options::saveOrder);
	}
	catch (Exception& e)
	{
		Log(LOG_ERROR) << e.what();
	}
}

/**
 * Checks when popup is done and gives the cursor a jog if so.
 */
void ListGamesState::think()
{
	if (_window->isPopupDone() == false)
		_window->think();
	else if (_jogRodent == true)
	{
		_jogRodent = false;
		refreshMousePosition();
	}
	else
		State::think(); // NOTE: TextEdit needs to think() to blink() the caret in ListSaveState.
}

/**
 * Updates the sorting arrows per the current setting.
 * @note The arrow directions are reversed. Sort-ascending is a down arrow and
 * sort-descending is an up arrow.
 */
void ListGamesState::updateArrows() // private.
{
	switch (Options::saveOrder)
	{
		case SORT_NAME_ASC:
			_sortName->setShape(ARROW_SMALL_DOWN);
			_sortDate->setShape(ARROW_NONE);
			break;
		case SORT_NAME_DESC:
			_sortName->setShape(ARROW_SMALL_UP);
			_sortDate->setShape(ARROW_NONE);
			break;
		case SORT_DATE_ASC:
			_sortName->setShape(ARROW_NONE);
			_sortDate->setShape(ARROW_SMALL_DOWN);
			break;
		case SORT_DATE_DESC:
			_sortName->setShape(ARROW_NONE);
			_sortDate->setShape(ARROW_SMALL_UP);
	}
}

/**
 * Sorts the List.
 * @param order - order to sort the games in
 */
void ListGamesState::sortList(SaveSort order) // private.
{
	_lstSaves->clearList();

	switch (order)
	{
		case SORT_NAME_ASC:
			std::sort(
					_info.begin(),
					_info.end(),
					compareSaveName(false));
			break;
		case SORT_NAME_DESC:
			std::sort(
					_info.rbegin(),
					_info.rend(),
					compareSaveName(true));
			break;
		case SORT_DATE_ASC:
			std::sort(
					_info.begin(),
					_info.end(),
					compareSaveTimestamp(false));
			break;
		case SORT_DATE_DESC:
			std::sort(
					_info.rbegin(),
					_info.rend(),
					compareSaveTimestamp(true));
	}
	updateList();
}

/**
 * Updates the List with available saved-games.
 */
void ListGamesState::updateList() // virtual.
{
	size_t r (0u);
	Uint8 color (_lstSaves->getSecondaryColor());

	for (std::vector<SaveInfo>::const_iterator
			i  = _info.begin();
			i != _info.end();
			++i, ++r)
	{
		_lstSaves->addRow(
						3,
						i->label.c_str(),
						i->isoDate.c_str(),
						i->isoTime.c_str());
		if (i->reserved == true
			&& _origin != OPT_BATTLESCAPE)
		{
			_lstSaves->setRowColor(r, color);
		}
	}
}

/**
 * Exits to the previous screen.
 * @param action - pointer to an Action
 */
void ListGamesState::btnCancelClick(Action*)
{
	_game->popState();
}

/**
 * Reverts text-edit or returns to the previous screen.
 * @param action - pointer to an Action
 */
void ListGamesState::btnCancelKeypress(Action*)
{
	if (_editMode == true) // revert TextEdit first onEscape, see ListSaveState::keySavePress()
		_editMode = false;
	else
		_game->popState(); // 2nd Escape releases state
}

/**
 * Shows the details of the currently hovered entry.
 * @param action - pointer to an Action
 */
void ListGamesState::lstMouseOver(Action*)
{
	if (_editMode == false)
	{
		std::wstring wst;
		const int sel (static_cast<int>(_lstSaves->getSelectedRow()) - static_cast<int>(_firstValid));
		if (sel > -1 && sel < static_cast<int>(_info.size()))
			wst = _info[static_cast<size_t>(sel)].details;

		_txtDetails->setText(tr("STR_DETAILS").arg(wst));
	}
}

/**
 * Hides details of hovered entry.
 */
void ListGamesState::lstMouseOut(Action*)
{
	if (_editMode == false)
		_txtDetails->setText(tr("STR_DETAILS").arg(L""));
}

/**
 * Asks to confirm deletion of the pressed entry.
 * @param action - pointer to an Action
 */
void ListGamesState::lstPress(Action* action) // virtual.
{
	if (_editMode == false
		&& action->getDetails()->button.button == SDL_BUTTON_RIGHT
		&& _lstSaves->getSelectedRow() >= _firstValid)
	{
		_game->pushState(new DeleteSaveState(
										_origin,
										_info[_lstSaves->getSelectedRow() - _firstValid].file));
	}
}

/**
 * Sorts the List entries by name.
 * @param action - pointer to an Action
 */
void ListGamesState::sortNameClick(Action*) // private.
{
	if (_sortable == true)
	{
		switch (Options::saveOrder)
		{
			case SORT_NAME_ASC:
				Options::saveOrder = SORT_NAME_DESC;
				break;
			default:
				Options::saveOrder = SORT_NAME_ASC;
		}

		updateArrows();
		sortList(Options::saveOrder);
	}
}

/**
 * Sorts the List entries by date.
 * @param action - pointer to an Action
 */
void ListGamesState::sortDateClick(Action*) // private.
{
	if (_sortable == true)
	{
		switch (Options::saveOrder)
		{
			case SORT_DATE_ASC:
				Options::saveOrder = SORT_DATE_DESC;
				break;
			default:
				Options::saveOrder = SORT_DATE_ASC;
		}

		updateArrows();
		sortList(Options::saveOrder);
	}
}

/**
 * Enables/disables sorting the list.
 * @param sortable - true if sortable (default true)
 */
void ListGamesState::setSortable(bool sortable) // protected.
{
	_sortable = sortable;
}

/**
 * Hides/shows the list.
 * @param hide - true to hide all elements of the state (default true)
 */
void ListGamesState::hideList(bool hide) // virtual.
{
	_txtTitle  ->setVisible(!hide);
	_txtDelete ->setVisible(!hide);
	_txtName   ->setVisible(!hide);
	_txtDate   ->setVisible(!hide);
	_sortName  ->setVisible(!hide);
	_sortDate  ->setVisible(!hide);
	_lstSaves  ->setVisible(!hide);
	_txtDetails->setVisible(!hide);
	_btnCancel ->setVisible(!hide);
}

}
