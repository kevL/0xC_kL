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

#include "MonthlyCostsState.h"

//#include <sstream>

#include "../Engine/Game.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Options.h"

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextList.h"
#include "../Interface/Window.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/RuleCraft.h"
#include "../Ruleset/Ruleset.h"
#include "../Ruleset/RuleSoldier.h"

#include "../Savegame/Base.h"
#include "../Savegame/SavedGame.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the MonthlyCosts screen.
 * @param base - pointer to the Base to get info from
 */
MonthlyCostsState::MonthlyCostsState(Base* base)
{
	_window         = new Window(this);

	_txtTitle       = new Text(300, 17, 10, 8);

	_txtUnitCost    = new Text(62, 9, 140, 26);
	_txtQuantity    = new Text(43, 9, 202, 26);
	_txtCost        = new Text(56, 9, 245, 26);

	_txtRental      = new Text(150, 9, 16, 26);
	_lstCrafts      = new TextList(285, 65, 16, 36);

	_txtSalaries    = new Text(150, 9, 16, 107);
	_lstSalaries    = new TextList(285, 25, 16, 117);

	_lstMaintenance = new TextList(285, 9, 16, 144);

	_lstBaseCost    = new TextList(103, 9, 198, 155);
	_lstTotal       = new TextList(103, 9, 198, 165);
	_txtIncome      = new Text(150, 9, 16, 165);

	_btnOk          = new TextButton(288, 16, 16, 177);

	setInterface("costsInfo");

	add(_window,         "window", "costsInfo");
	add(_txtTitle,       "text1",  "costsInfo");
	add(_txtUnitCost,    "text1",  "costsInfo");
	add(_txtQuantity,    "text1",  "costsInfo");
	add(_txtCost,        "text1",  "costsInfo");
	add(_txtRental,      "text1",  "costsInfo");
	add(_lstCrafts,      "list",   "costsInfo");
	add(_txtSalaries,    "text1",  "costsInfo");
	add(_lstSalaries,    "list",   "costsInfo");
	add(_lstMaintenance, "text1",  "costsInfo");
	add(_lstBaseCost,    "text2",  "costsInfo");
	add(_lstTotal,       "text2",  "costsInfo");
	add(_txtIncome,      "text2",  "costsInfo");
	add(_btnOk,          "button", "costsInfo");

	centerSurfaces();


	_window->setBackground(_game->getResourcePack()->getSurface("BACK13.SCR"));

	_txtTitle->setText(tr("STR_MONTHLY_COSTS_").arg(base->getLabel()));
	_txtTitle->setBig();

	_txtUnitCost->setText(tr("STR_COST_PER_UNIT"));
	_txtQuantity->setText(tr("STR_QUANTITY"));
	_txtCost->setText(tr("STR_COST_LC"));
	_txtRental->setText(tr("STR_CRAFT_RENTAL"));

	int
		cost,
		qty;

	_lstCrafts->setColumns(4, 124,62,43,56);
	_lstCrafts->setMargin();
	_lstCrafts->setDot();
	const RuleCraft* crRule;
	const std::vector<std::string>& allCraft (_game->getRuleset()->getCraftsList());
	for (std::vector<std::string>::const_iterator
			i  = allCraft.begin();
			i != allCraft.end();
			++i)
	{
		crRule = _game->getRuleset()->getCraft(*i);
		if (_game->getSavedGame()->isResearched(crRule->getRequiredResearch()) == true
			&& (cost = crRule->getRentCost()) != 0
			&& (qty = base->getCraftCount(*i)) != 0)
		{
			_lstCrafts->addRow(
							4,
							tr(*i).c_str(),
							Text::formatCurrency(cost).c_str(),
							Text::intWide(qty).c_str(),
							Text::formatCurrency(cost * qty).c_str());
		}
	}


	_txtSalaries->setText(tr("STR_SALARIES"));

	_lstSalaries->setColumns(4, 124,62,43,56);
	_lstSalaries->setMargin();
	_lstSalaries->setDot();

	const RuleSoldier* solRule;
	const std::vector<std::string>& soldierTypes (_game->getRuleset()->getSoldiersList());
	for (std::vector<std::string>::const_iterator
			i = soldierTypes.begin(), j = soldierTypes.end();
			i != j;
			++i)
	{
		solRule = _game->getRuleset()->getSoldier(*i);
		if (_game->getSavedGame()->isResearched(solRule->getRequiredResearch()) == true
			&& (cost = solRule->getSalaryCost()) != 0
			&& (qty = base->getSoldierCount(*i)) != 0)
		{
			std::string type;
			if (i == soldierTypes.begin())
				type = "STR_SOLDIERS";
			else
				type = *i;

			int total (0);
			for (std::vector<Soldier*>::const_iterator
					k = base->getSoldiers()->begin(), l = base->getSoldiers()->end();
					k != l;
					++k)
			{
				total += cost + (*k)->getRankCost();
			}

			_lstSalaries->addRow(
							4,
							tr(type).c_str(),
							Text::formatCurrency(cost).c_str(),
							Text::intWide(qty).c_str(),
							Text::formatCurrency(total).c_str());
		}
	}

	if ((cost = _game->getRuleset()->getEngineerCost()) != 0
		&& (qty = base->getTotalEngineers()) != 0)
	{
		_lstSalaries->addRow(
						4,
						tr("STR_ENGINEERS").c_str(),
						Text::formatCurrency(cost).c_str(),
						Text::intWide(qty).c_str(),
						Text::formatCurrency(cost * qty).c_str());
	}

	if ((cost = _game->getRuleset()->getScientistCost()) != 0
		&& (qty = base->getTotalScientists()) != 0)
	{
		_lstSalaries->addRow(
						4,
						tr("STR_SCIENTISTS").c_str(),
						Text::formatCurrency(cost).c_str(),
						Text::intWide(qty).c_str(),
						Text::formatCurrency(cost * qty).c_str());
	}

	std::wostringstream woststr;

	_lstMaintenance->setColumns(2, 229,56);
	_lstMaintenance->setMargin();
	_lstMaintenance->setDot();
	woststr << L'\x01' << Text::formatCurrency(base->getFacilityMaintenance());
	_lstMaintenance->addRow(
						2,
						tr("STR_BASE_MAINTENANCE").c_str(),
						woststr.str().c_str());

	_lstBaseCost->setColumns(2, 47,56);
	_lstBaseCost->setMargin();
	_lstBaseCost->setDot();
	_lstBaseCost->addRow(
					2,
					tr("STR_BASE").c_str(),
					Text::formatCurrency(base->getMonthlyMaintenace()).c_str());

	woststr.str(L"");
	woststr << tr("STR_INCOME") << L" " << Text::formatCurrency(_game->getSavedGame()->getTotalCountryFunds() * 1000);
	_txtIncome->setText(woststr.str());

	_lstTotal->setColumns(2, 47,56);
	_lstTotal->setMargin();
	_lstTotal->setDot();
	_lstTotal->addRow(
					2,
					tr("STR_TOTAL").c_str(),
					Text::formatCurrency(_game->getSavedGame()->getBaseMaintenances()).c_str());

	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick(   static_cast<ActionHandler>(&MonthlyCostsState::btnOkClick));
	_btnOk->onKeyboardPress(static_cast<ActionHandler>(&MonthlyCostsState::btnOkClick),
							Options::keyOk);
	_btnOk->onKeyboardPress(static_cast<ActionHandler>(&MonthlyCostsState::btnOkClick),
							Options::keyOkKeypad);
	_btnOk->onKeyboardPress(static_cast<ActionHandler>(&MonthlyCostsState::btnOkClick),
							Options::keyCancel);
}

/**
 * dTor.
 */
MonthlyCostsState::~MonthlyCostsState()
{}

/**
 * Exits to the previous screen.
 * @param action - pointer to an Action
 */
void MonthlyCostsState::btnOkClick(Action*)
{
	_game->popState();
}

}
