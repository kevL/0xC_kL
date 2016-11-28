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

#include "ManufactureStartState.h"

//#include <sstream>

#include "ManufactureCostsState.h"
#include "ManufactureInfoState.h"

#include "../Engine/Game.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Options.h"

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextList.h"
#include "../Interface/Window.h"

#include "../Menu/ErrorMessageState.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/RuleInterface.h"
#include "../Ruleset/RuleManufacture.h"
#include "../Ruleset/Ruleset.h"

#include "../Savegame/Base.h"
#include "../Savegame/ItemContainer.h"
#include "../Savegame/SavedGame.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the ManufactureStartState screen.
 * @param base		- pointer to the Base to get info from
 * @param mfRule	- pointer to RuleManufacture
 */
ManufactureStartState::ManufactureStartState(
		Base* const base,
		const RuleManufacture* const mfRule)
	:
		_base(base),
		_mfRule(mfRule),
		_init(true)
{
	_fullScreen = false;

	_window				= new Window(this, 320, 170, 0, 23);

	_txtTitle			= new Text(300, 16, 10, 34);

	_btnCostTable		= new TextButton(130, 16, 170, 53);

	_txtManHour			= new Text(150, 9, 20, 53);
	_txtCost			= new Text(150, 9, 20, 63);
	_txtWorkSpace		= new Text(150, 9, 20, 73);

	_txtRequiredItems	= new Text(150, 9, 20, 83);

	_txtItemRequired	= new Text(60, 9,  40, 93);
	_txtUnitsRequired	= new Text(60, 9, 180, 93);
	_txtUnitsAvailable	= new Text(60, 9, 240, 93);

	_lstRequiredItems	= new TextList(240, 57, 40, 108);

	_btnCancel			= new TextButton(130, 16,  20, 170);
	_btnStart			= new TextButton(130, 16, 170, 170);

	setInterface("allocateManufacture");

	add(_window,		"window",	"allocateManufacture");
	add(_txtTitle,		"text",		"allocateManufacture");
	add(_btnCostTable,	"button",	"allocateManufacture");
	add(_txtManHour,	"text",		"allocateManufacture");
	add(_txtCost,		"text",		"allocateManufacture");
	add(_txtWorkSpace,	"text",		"allocateManufacture");

	add(_txtRequiredItems,	"text", "allocateManufacture");
	add(_txtItemRequired,	"text", "allocateManufacture");
	add(_txtUnitsRequired,	"text", "allocateManufacture");
	add(_txtUnitsAvailable,	"text", "allocateManufacture");
	add(_lstRequiredItems,	"list", "allocateManufacture");

	add(_btnCancel,		"button", "allocateManufacture");
	add(_btnStart,		"button", "allocateManufacture");

	centerSurfaces();


	_window->setBackground(_game->getResourcePack()->getSurface("BACK17.SCR"));

	_txtTitle->setText(tr(_mfRule->getType()));
	_txtTitle->setBig();
	_txtTitle->setAlign(ALIGN_CENTER);

	_btnCostTable->setText(tr("STR_PRODUCTION_COSTS"));
	_btnCostTable->onMouseClick(static_cast<ActionHandler>(&ManufactureStartState::btnCostsClick));

	_txtManHour->setText(tr("STR_ENGINEER_HOURS_TO_PRODUCE_ONE_UNIT_")
							.arg(_mfRule->getManufactureHours()));
	_txtCost->setText(tr("STR_COST_PER_UNIT_")
							.arg(Text::formatCurrency(_mfRule->getManufactureCost())));
	_txtWorkSpace->setText(tr("STR_WORK_SPACE_REQUIRED_")
							.arg(_mfRule->getSpaceRequired()));

	_txtRequiredItems->setText(tr("STR_SPECIAL_MATERIALS_REQUIRED"));

	_txtItemRequired->setText(tr("STR_PART_UC"));
	_txtUnitsRequired->setText(tr("STR_UNITS_UC"));
	_txtUnitsAvailable->setText(tr("STR_STOCK_UC"));

	_lstRequiredItems->setColumns(3, 140,60,40);

	bool
		showStart = _base->getFreeWorkshops() != 0
				 && _game->getSavedGame()->getFunds() >= _mfRule->getManufactureCost(),
		showReqs;

	const std::map<std::string, int>& partsRequired (_mfRule->getPartsRequired());
	if (partsRequired.empty() == false)
	{
		showReqs = true;
		const ItemContainer* const stores (_base->getStorageItems());

		for (std::map<std::string, int>::const_iterator
				i = partsRequired.begin();
				i != partsRequired.end();
				++i)
		{
			std::wostringstream
				woststr1,
				woststr2;
			woststr1 << L'\x01' << i->second;

			if (_game->getRuleset()->getItemRule(i->first) != nullptr)
			{
				const int qty (stores->getItemQuantity(i->first));
				woststr2 << L'\x01' << qty;
				showStart &= (qty >= i->second);
			}
			else if (_game->getRuleset()->getCraft(i->first) != nullptr)
			{
				const int qty (_base->getCraftCount(i->first, true));
				woststr2 << L'\x01' << qty;
				showStart &= (qty >= i->second);
			}
			_lstRequiredItems->addRow(
									3,
									tr(i->first).c_str(),
									woststr1.str().c_str(),
									woststr2.str().c_str());
		}
	}
	else
		showReqs = false;

	_txtRequiredItems	->setVisible(showReqs);
	_txtItemRequired	->setVisible(showReqs);
	_txtUnitsRequired	->setVisible(showReqs);
	_txtUnitsAvailable	->setVisible(showReqs);
	_lstRequiredItems	->setVisible(showReqs);


	_btnCancel->setText(tr("STR_CANCEL_UC"));
	_btnCancel->onMouseClick(	static_cast<ActionHandler>(&ManufactureStartState::btnCancelClick));
	_btnCancel->onKeyboardPress(static_cast<ActionHandler>(&ManufactureStartState::btnCancelClick),
								Options::keyCancel);

	_btnStart->setText(tr("STR_START_PRODUCTION"));
	_btnStart->onMouseClick(	static_cast<ActionHandler>(&ManufactureStartState::btnStartClick));
	_btnStart->onKeyboardPress(	static_cast<ActionHandler>(&ManufactureStartState::btnStartClick),
								Options::keyOk);
	_btnStart->onKeyboardPress(	static_cast<ActionHandler>(&ManufactureStartState::btnStartClick),
								Options::keyOkKeypad);
	_btnStart->setVisible(showStart);
}

/**
 * dTor.
 */
ManufactureStartState::~ManufactureStartState()
{}

/**
 * Initializes state.
 */
void ManufactureStartState::init()
{
	State::init();

	if (_init == true)
	{
		_init = false;
		const Ruleset* const rules (_game->getRuleset());

		std::string error;

		if (_mfRule->getSpaceRequired() > _base->getFreeWorkshops())
			error = "STR_NOT_ENOUGH_WORK_SPACE";
		else if (_mfRule->isCraftProduced() == true && _base->getFreeHangars() == 0)	// NOTE: Hardcaps are done in RuleManufacture
		{																				// to ensure only 1 Craft is produced *total*.
			bool allowCraftProduction (false);
			for (std::map<std::string, int>::const_iterator
					i = _mfRule->getPartsRequired().begin();
					i != _mfRule->getPartsRequired().end();
					++i)
			{
				if (rules->getItemRule(i->first) == nullptr && rules->getCraft(i->first) != nullptr)
				{
					allowCraftProduction = true;
					break;
				}
			}

			if (allowCraftProduction == false)
				error = "STR_NO_FREE_HANGARS_FOR_CRAFT_PRODUCTION";
		}

		if (error.empty() == false)
		{
			_btnStart->setVisible(false);
			_game->pushState(new ErrorMessageState(
											tr(error),
											_palette,
											rules->getInterface("basescape")->getElement("errorMessage")->color,
											"BACK17.SCR",
											rules->getInterface("basescape")->getElement("errorPalette")->color));
		}
	}
}

/**
 * Go to the Costs table.
 * @param action - pointer to an Action
 */
void ManufactureStartState::btnCostsClick(Action*)
{
	_game->pushState(new ManufactureCostsState());
}

/**
 * Exits to the previous screen.
 * @param action - pointer to an Action
 */
void ManufactureStartState::btnCancelClick(Action*)
{
	_game->popState();
}

/**
 * Go to the ManufactureInfo screen.
 * @param action - pointer to an Action
 */
void ManufactureStartState::btnStartClick(Action*)
{
	_game->pushState(new ManufactureInfoState(_base, _mfRule));
}

}
