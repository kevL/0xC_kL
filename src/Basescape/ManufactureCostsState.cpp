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

#include "ManufactureCostsState.h"

#include <iomanip>

#include "../Engine/Game.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Options.h"

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextList.h"
#include "../Interface/Window.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/RuleCraft.h"
#include "../Ruleset/RuleInterface.h"
#include "../Ruleset/RuleItem.h"
#include "../Ruleset/RuleManufacture.h"
#include "../Ruleset/Ruleset.h"

#include "../Savegame/SavedGame.h"


namespace OpenXcom
{

/**
 * Screen that displays a table of Manufacture costs.
 */
ManufactureCostsState::ManufactureCostsState()
{
	_window			= new Window(this);

	_txtTitle		= new Text(300, 17, 10, 10);

	_txtItem		= new Text(96, 9,  10, 30);
	_txtCost		= new Text(50, 9, 111, 30);
	_txtManHours	= new Text(30, 9, 161, 30);
	_txtSpace		= new Text(15, 9, 196, 30);
	_txtRequired	= new Text(95, 9, 206, 30);

	_lstProduction	= new TextList(291, 129, 10, 40);

	_btnCancel		= new TextButton(288, 16, 16, 177);


	setInterface("allocateManufacture");

	add(_window,		"window",	"allocateManufacture"); // <- using allocateManufacture colors ->
	add(_txtTitle,		"text",		"allocateManufacture");
	add(_txtItem,		"text",		"allocateManufacture");
	add(_txtManHours,	"text",		"allocateManufacture");
	add(_txtSpace,		"text",		"allocateManufacture");
	add(_txtCost,		"text",		"allocateManufacture");
	add(_txtRequired,	"text",		"allocateManufacture");
	add(_lstProduction,	"list",		"allocateManufacture");
	add(_btnCancel,		"button",	"allocateManufacture");

	centerSurfaces();


	_window->setBackground(_game->getResourcePack()->getSurface("BACK17.SCR"));

	_txtTitle->setText(tr("STR_COSTS_UC"));
	_txtTitle->setBig();
	_txtTitle->setAlign(ALIGN_CENTER);

	const Uint8 color (static_cast<Uint8>(_game->getRuleset()->getInterface("allocateManufacture")->getElement("text")->color2));

	_txtItem->setText(tr("STR_LIST_ITEM"));
	_txtItem->setColor(color);

	_txtManHours->setText(tr("STR_HOURS_PER"));
	_txtManHours->setColor(color);

	_txtSpace->setText(tr("STR_SPACE_LC"));
	_txtSpace->setColor(color);

	_txtCost->setText(tr("STR_COST_LC"));
	_txtCost->setColor(color);

	_txtRequired->setText(tr("STR_REQUIRED_LC"));
	_txtRequired->setColor(color);

	_lstProduction->setColumns(5, 96,50,30,15,95);
	_lstProduction->setMargin(5);
	_lstProduction->setBackground(_window);
	_lstProduction->setSelectable();

	_btnCancel->setText(tr("STR_CANCEL_UC"));
	_btnCancel->onMouseClick(	static_cast<ActionHandler>(&ManufactureCostsState::btnCancelClick));
	_btnCancel->onKeyboardPress(static_cast<ActionHandler>(&ManufactureCostsState::btnCancelClick),
								Options::keyCancel);
	_btnCancel->onKeyboardPress(static_cast<ActionHandler>(&ManufactureCostsState::btnCancelClick),
								Options::keyOk);
	_btnCancel->onKeyboardPress(static_cast<ActionHandler>(&ManufactureCostsState::btnCancelClick),
								Options::keyOkKeypad);
}

/**
 * dTor.
 */
ManufactureCostsState::~ManufactureCostsState()
{}

/**
 * Populates the table with Manufacture info.
 */
void ManufactureCostsState::init()
{
	const Ruleset* const rules (_game->getRuleset());

	std::vector<const RuleManufacture*> unlocked;
	const RuleManufacture* mfRule;

	for (std::vector<std::string>::const_iterator
			i = rules->getManufactureList().begin();
			i != rules->getManufactureList().end();
			++i)
	{
		mfRule = rules->getManufacture(*i);

		if (_game->getSavedGame()->isResearched(mfRule->getRequiredResearch()) == true)
			unlocked.push_back(mfRule);
	}

	size_t r (0u);
	std::wostringstream woststr;
	int
		profit,
		costDebit,
		costCredit;
	float profitAspect;

	for (std::vector<const RuleManufacture*>::const_iterator
			i = unlocked.begin();
			i != unlocked.end();
			++i, r += 3u)
	{
		woststr.str(L"");
		woststr << L"> " << tr((*i)->getType());

		_lstProduction->addRow(
							5,
							woststr.str().c_str(),
							Text::formatCurrency((*i)->getManufactureCost()).c_str(),
							Text::intWide((*i)->getManufactureHours()).c_str(),
							Text::intWide((*i)->getSpaceRequired()).c_str(),
							L"");
//							tr((*i)->getCategory ()).c_str());

		costDebit = 0;

		for (std::map<std::string, int>::const_iterator
				j = (*i)->getPartsRequired().begin();
				j != (*i)->getPartsRequired().end();
				++j)
		{
			if (rules->getItemRule(j->first) == nullptr && rules->getCraft(j->first) != nullptr)
				costDebit += rules->getCraft(j->first)->getSellCost() * j->second;
			else
				costDebit += rules->getItemRule(j->first)->getSellCost() * j->second;

			woststr.str(L"");
			woststr << L"(" << j->second << L") " << tr(j->first);

			if (j == (*i)->getPartsRequired().begin())
				_lstProduction->setCellText(r, 4u, woststr.str());
			else
			{
				_lstProduction->addRow(
									5,
									L">",L"",L"",L"",
									woststr.str().c_str());
				++r;
			}
			_lstProduction->setRowColor(r, YELLOW);
		}
		// NOTE: Productions that require parts show as yellow; those that don't show as blue.

		profit = 0;

		for (std::map<std::string, int>::const_iterator
				j = (*i)->getPartsProduced().begin();
				j != (*i)->getPartsProduced().end();
				++j)
		{
			woststr.str(L"");
			woststr << L"< " << tr(j->first);

			if ((*i)->isCraftProduced() == true)
				costCredit = rules->getCraft(j->first)->getSellCost(); // NOTE: RuleManufacture caps each iteration of craft-production at "1".
			else
				costCredit = rules->getItemRule(j->first)->getSellCost() * j->second;

			profit += costCredit;

			std::wostringstream qty;
			qty << L"*" << j->second;

			_lstProduction->addRow(
								5,
								woststr.str().c_str(),
								Text::formatCurrency(costCredit).c_str(),
								qty.str().c_str(),
								L"",L"");
			_lstProduction->setRowColor(++r, GREEN, true);
		}

		profit -= (*i)->getManufactureCost();
		profit -= costDebit;
		profitAspect = static_cast<float>(profit) / static_cast<float>((*i)->getManufactureHours());

		woststr.str(L"");
		woststr << std::fixed << std::setprecision(2) << profitAspect;

		_lstProduction->addRow(
							5,
							L"<",
							woststr.str().c_str(),
							L"",L"",
							Text::formatCurrency(profit).c_str());
		_lstProduction->setRowColor(r + 1u, BROWN, true);

		_lstProduction->addRow(5, L"",L"",L"",L"",L""); // hori-spacer.
	}
}

/**
 * Exits to the previous screen.
 * @param action - pointer to an Action
 */
void ManufactureCostsState::btnCancelClick(Action*)
{
	_game->popState();
}

}
