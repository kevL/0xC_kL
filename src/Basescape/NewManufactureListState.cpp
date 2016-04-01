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

#include "NewManufactureListState.h"

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

const std::string NewManufactureListState::ALL_ITEMS ("STR_ALL_ITEMS");						// private/static.
std::string NewManufactureListState::_recallCatString (NewManufactureListState::ALL_ITEMS);	// private/static.


/**
 * List that allows player to choose what to manufacture.
 * @note Initializes all the elements in the productions list screen.
 * @param base - pointer to the Base to get info from
 */
NewManufactureListState::NewManufactureListState(
		Base* const base)
	:
		_base(base),
		_scroll(0)
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

	centerAllSurfaces();


	_window->setBackground(_game->getResourcePack()->getSurface("BACK17.SCR"));

	_txtTitle->setText(tr("STR_PRODUCTION_ITEMS"));
	_txtTitle->setBig();

	_txtItem->setText(tr("STR_ITEM"));

	_txtCategory->setText(tr("STR_CATEGORY"));

	_lstManufacture->setColumns(2, 132,145);
	_lstManufacture->setBackground(_window);
	_lstManufacture->setSelectable();
	_lstManufacture->onMouseClick((ActionHandler)& NewManufactureListState::lstProdClick);

	_btnCostTable->setText(tr("STR_PRODUCTION_COSTS"));
	_btnCostTable->onMouseClick((ActionHandler)& NewManufactureListState::btnCostsClick);

	_btnCancel->setText(tr("STR_OK"));
	_btnCancel->onMouseClick((ActionHandler)& NewManufactureListState::btnCancelClick);
	_btnCancel->onKeyboardPress(
					(ActionHandler)& NewManufactureListState::btnCancelClick,
					Options::keyOk);
	_btnCancel->onKeyboardPress(
					(ActionHandler)& NewManufactureListState::btnCancelClick,
					Options::keyOkKeypad);
	_btnCancel->onKeyboardPress(
					(ActionHandler)& NewManufactureListState::btnCancelClick,
					Options::keyCancel);


	_catStrings.push_back(ALL_ITEMS); // #0
	std::string cat;
	_game->getSavedGame()->getAvailableProductions(_available, _base);
	for (std::vector<const RuleManufacture*>::const_iterator
			i = _available.begin();
			i != _available.end();
			++i)
	{
		cat = (*i)->getCategory();
		if (std::find(
				_catStrings.begin(),
				_catStrings.end(),
				cat) == _catStrings.end())
		{
			_catStrings.push_back(cat);
		}
	}
	_cbxCategory->setOptions(_catStrings);
	_cbxCategory->setBackgroundFill(58); // green <- TODO: put this in Interfaces.rul
	_cbxCategory->onComboChange((ActionHandler)& NewManufactureListState::cbxCategoryChange);

	std::vector<std::string>::iterator i (std::find( // <- std::distance(below_) does not accept a const_iterator.
												_catStrings.begin(),
												_catStrings.end(),
												_recallCatString));
	if (i != _catStrings.end())
	{
		const size_t j (std::distance(_catStrings.begin(), i));
		_cbxCategory->setSelected(j);
	}
	else // safety.
	{
		_recallCatString = ALL_ITEMS;
		_cbxCategory->setSelected(0);
	}
}

/**
 * dTor.
 */
NewManufactureListState::~NewManufactureListState()
{}

/**
 * Initializes state - fills list with possible productions.
 */
void NewManufactureListState::init()
{
	State::init();

	fillProductionList();
	_lstManufacture->scrollTo(_scroll);
}

/**
 * Go to the Costs table.
 * @param action - pointer to an Action
 */
void NewManufactureListState::btnCostsClick(Action*)
{
	_game->pushState(new ManufactureCostsState());
}

/**
 * Returns to the previous screen.
 * @param action - pointer to an Action
 */
void NewManufactureListState::btnCancelClick(Action*)
{
	_game->popState();
}

/**
 * Opens the Production settings screen.
 * @param action - pointer to an Action
*/
void NewManufactureListState::lstProdClick(Action*)
{
	_scroll = _lstManufacture->getScroll();

	const RuleManufacture* manfRule (nullptr);
	for (std::vector<const RuleManufacture*>::iterator
			i = _available.begin();
			i != _available.end();
			++i)
	{
		if ((*i)->getType() == _manfStrings[_lstManufacture->getSelectedRow()])
		{
			manfRule = *i;
			break;
		}
	}

	if (manfRule != nullptr) // safety.
		_game->pushState(new ManufactureStartState(_base, manfRule));
}

/**
 * Updates the production-list to match the category-filter.
 */
void NewManufactureListState::cbxCategoryChange(Action*)
{

	fillProductionList();
}

/**
 * Fills the list of possible productions.
 */
void NewManufactureListState::fillProductionList() // private.
{
	_recallCatString = _catStrings[_cbxCategory->getSelected()];

	_lstManufacture->clearList();
	_available.clear();
	_manfStrings.clear();

	_game->getSavedGame()->getAvailableProductions(_available, _base);
	for (std::vector<const RuleManufacture*>::const_iterator
			i = _available.begin();
			i != _available.end();
			++i)
	{
		if (_recallCatString == ALL_ITEMS || _recallCatString == (*i)->getCategory())
		{
			_lstManufacture->addRow(
								2,
								tr((*i)->getType()).c_str(),
								tr((*i)->getCategory()).c_str());
			_manfStrings.push_back((*i)->getType());
		}
	}
}

}
