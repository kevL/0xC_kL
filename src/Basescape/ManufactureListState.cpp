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

#include "ManufactureListState.h"

#include "ManufactureCostsState.h"
#include "ManufactureStartState.h"

#include "../Engine/Game.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Options.h"

#include "../Interface/ComboBox.h"
#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextList.h"
#include "../Interface/Window.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/RuleManufacture.h"

#include "../Savegame/Base.h"
#include "../Savegame/SavedGame.h"


namespace OpenXcom
{

const std::string ManufactureListState::ALL_ITEMS ("STR_ALL_ITEMS");					// private/static.
std::string ManufactureListState::_recallCategory (ManufactureListState::ALL_ITEMS);	// private/static.


/**
 * List that allows player to choose what to Manufacture.
 * @note Initializes all the elements in the Manufacture list screen.
 * @param base - pointer to the Base to get info from
 */
ManufactureListState::ManufactureListState(
		Base* const base)
	:
		_base(base),
		_scroll(0u)
{
	_fullScreen = false;

	_window			= new Window(this, 320, 170, 0, 23, POPUP_BOTH);

	_txtTitle		= new Text(320, 16, 16, 30);

	_txtItem		= new Text(80, 9,  16, 46);
	_txtCategory	= new Text(80, 9, 156, 46);

	_cbxCategory	= new ComboBox(this, 140, 16, 173, 30);

	_lstManufacture	= new TextList(285, 113, 16, 55);

	_btnCostTable	= new TextButton(130, 16,  20, 170);
	_btnCancel		= new TextButton(130, 16, 170, 170);

	setInterface("selectNewManufacture");

	add(_window,			"window",	"selectNewManufacture");
	add(_txtTitle,			"text",		"selectNewManufacture");
	add(_txtItem,			"text",		"selectNewManufacture");
	add(_txtCategory,		"text",		"selectNewManufacture");
	add(_lstManufacture,	"list",		"selectNewManufacture");
	add(_cbxCategory,		"catBox",	"selectNewManufacture");
	add(_btnCostTable,		"button",	"selectNewManufacture");
	add(_btnCancel,			"button",	"selectNewManufacture");

	centerSurfaces();


	_window->setBackground(_game->getResourcePack()->getSurface("BACK17.SCR"));

	_txtTitle->setText(tr("STR_PRODUCTION_ITEMS"));
	_txtTitle->setBig();

	_txtItem->setText(tr("STR_ITEM"));

	_txtCategory->setText(tr("STR_CATEGORY"));

	_lstManufacture->setColumns(2, 132,145);
	_lstManufacture->setBackground(_window);
	_lstManufacture->setSelectable();
	_lstManufacture->onMouseClick(static_cast<ActionHandler>(&ManufactureListState::lstStartClick));

	_btnCostTable->setText(tr("STR_PRODUCTION_COSTS"));
	_btnCostTable->onMouseClick(static_cast<ActionHandler>(&ManufactureListState::btnCostsClick));

	_btnCancel->setText(tr("STR_OK"));
	_btnCancel->onMouseClick(	static_cast<ActionHandler>(&ManufactureListState::btnCancelClick));
	_btnCancel->onKeyboardPress(static_cast<ActionHandler>(&ManufactureListState::btnCancelClick),
								Options::keyOk);
	_btnCancel->onKeyboardPress(static_cast<ActionHandler>(&ManufactureListState::btnCancelClick),
								Options::keyOkKeypad);
	_btnCancel->onKeyboardPress(static_cast<ActionHandler>(&ManufactureListState::btnCancelClick),
								Options::keyCancel);


	_categoryTypes.push_back(ALL_ITEMS); // #0

	std::string cat;
	_game->getSavedGame()->tabulateStartableManufacture(_unlocked, _base);
	for (std::vector<const RuleManufacture*>::const_iterator
			i = _unlocked.begin();
			i != _unlocked.end();
			++i)
	{
		cat = (*i)->getCategory();
		if (std::find(
				_categoryTypes.begin(),
				_categoryTypes.end(),
				cat) == _categoryTypes.end())
		{
			_categoryTypes.push_back(cat);
		}
	}
	_cbxCategory->setOptions(_categoryTypes);
	_cbxCategory->setBackgroundFill(GREEN); // TODO: put this in Interfaces.rul
	_cbxCategory->onComboChange(static_cast<ActionHandler>(&ManufactureListState::cbxCategoryChange));

	std::vector<std::string>::iterator i (std::find( // <- std::distance() below_ does not accept a const_iterator.
												_categoryTypes.begin(),
												_categoryTypes.end(),
												_recallCategory));
	if (i != _categoryTypes.end())
	{
		const size_t j (static_cast<size_t>(std::distance(_categoryTypes.begin(), i)));
		_cbxCategory->setSelected(j);
	}
	else // safety.
	{
		_recallCategory = ALL_ITEMS;
		_cbxCategory->setSelected(0u);
	}
}

/**
 * dTor.
 */
ManufactureListState::~ManufactureListState()
{}

/**
 * Initializes state - fills list with possible productions.
 */
void ManufactureListState::init()
{
	State::init();

	fillProjectList();
	_lstManufacture->scrollTo(_scroll);
}

/**
 * Go to the Costs table.
 * @param action - pointer to an Action
 */
void ManufactureListState::btnCostsClick(Action*)
{
	_game->pushState(new ManufactureCostsState());
}

/**
 * Exits to the previous screen.
 * @param action - pointer to an Action
 */
void ManufactureListState::btnCancelClick(Action*)
{
	_game->popState();
}

/**
 * Opens the ManufactureStart screen for assigning engineers.
 * @param action - pointer to an Action
 */
void ManufactureListState::lstStartClick(Action*) // private.
{
	_scroll = _lstManufacture->getScroll();

	std::vector<const RuleManufacture*>::const_iterator i (_unlocked.begin());
	for (
			;
			i != _unlocked.end();
			++i)
	{
		if ((*i)->getType() == _unlockedTypes[_lstManufacture->getSelectedRow()])
			break;
	}
	_game->pushState(new ManufactureStartState(_base, *i));
}

/**
 * Updates the project-list to match the category-filter.
 */
void ManufactureListState::cbxCategoryChange(Action*) // private.
{
	fillProjectList();
}

/**
 * Fills the list with available Manufacture.
 */
void ManufactureListState::fillProjectList() // private.
{
	_recallCategory = _categoryTypes[_cbxCategory->getSelected()];

	_lstManufacture->clearList();

	_unlocked.clear();
	_unlockedTypes.clear();

	_game->getSavedGame()->tabulateStartableManufacture(_unlocked, _base);
	for (std::vector<const RuleManufacture*>::const_iterator
			i = _unlocked.begin();
			i != _unlocked.end();
			++i)
	{
		if (_recallCategory == ALL_ITEMS || _recallCategory == (*i)->getCategory())
		{
			_lstManufacture->addRow(
								2,
								tr((*i)->getType()).c_str(),
								tr((*i)->getCategory()).c_str());
			_unlockedTypes.push_back((*i)->getType());
		}
	}
}

}
