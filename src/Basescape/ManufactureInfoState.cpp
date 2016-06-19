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

#include "ManufactureInfoState.h"

//#include <algorithm>
//#include <limits>
#include <iomanip>

#include "../Interface/ArrowButton.h"
#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/ToggleTextButton.h"
#include "../Interface/Window.h"

#include "../Engine/Action.h"
#include "../Engine/Game.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Options.h"
#include "../Engine/Timer.h"

#include "../Menu/ErrorMessageState.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/RuleCraft.h"
#include "../Ruleset/RuleInterface.h"
#include "../Ruleset/RuleItem.h"
#include "../Ruleset/RuleManufacture.h"
#include "../Ruleset/Ruleset.h"

#include "../Savegame/Base.h"
#include "../Savegame/Production.h"


namespace OpenXcom
{

/**
 * Initializes all elements in the Production settings screen (new Production).
 * @param base		- pointer to the Base to get info from
 * @param manfRule	- pointer to the RuleManufacture to produce
 */
ManufactureInfoState::ManufactureInfoState(
		Base* const base,
		const RuleManufacture* const manfRule)
	:
		_base(base),
		_manfRule(manfRule),
		_production(nullptr),
		_producedValue(0)
{
	buildUi();
}

/**
 * Initializes all elements in the Production settings screen (modifying Production).
 * @param base			- pointer to the Base to get info from
 * @param production	- pointer to the Production to modify
 */
ManufactureInfoState::ManufactureInfoState(
		Base* const base,
		Production* const production)
	:
		_base(base),
		_manfRule(nullptr),
		_production(production),
		_producedValue(0)
{
	buildUi();
}

/**
 * Frees up memory that's not automatically cleaned on exit.
 */
ManufactureInfoState::~ManufactureInfoState()
{
	delete _timerEngineersMore;
	delete _timerEngineersLess;
	delete _timerUnitsMore;
	delete _timerUnitsLess;
}

/**
 * Builds screen User Interface.
 */
void ManufactureInfoState::buildUi() // private.
{
	_fullScreen = false;

	_window				= new Window(this, 320, 170, 0, 23, POPUP_BOTH);

	_txtTitle			= new Text(280, 17, 20, 33);

	_txtTimeDesc		= new Text(30, 17, 244, 44);
	_txtTimeLeft		= new Text(30, 17, 274, 44);
	_btnSell			= new ToggleTextButton(60, 16, 244, 64);

	_txtFreeEngineer	= new Text(100, 9, 16, 55);
	_txtFreeSpace		= new Text(100, 9, 16, 65);
	_txtProfit			= new Text(160, 9, 16, 75);

	_txtEngineersDesc	= new Text(84, 17,  16, 88);
	_txtEngineers		= new Text(50, 17, 100, 88);

	_txtTotalDesc		= new Text(84, 17, 176, 88);
	_txtTotal			= new Text(50, 17, 260, 88);

	_btnEngineerMore	= new ArrowButton(ARROW_BIG_UP,   100, 16,  30, 119);
	_btnEngineerLess	= new ArrowButton(ARROW_BIG_DOWN, 100, 16,  30, 143);
	_btnUnitMore		= new ArrowButton(ARROW_BIG_UP,   100, 16, 190, 119);
	_btnUnitLess		= new ArrowButton(ARROW_BIG_DOWN, 100, 16, 190, 143);

	_btnStop			= new TextButton(130, 16,  20, 170);
	_btnOk				= new TextButton(130, 16, 170, 170);

	setInterface("manufactureInfo");

	add(_window,			"window",	"manufactureInfo");
	add(_txtTitle,			"text",		"manufactureInfo");
	add(_txtTimeDesc,		"text",		"manufactureInfo");
	add(_txtTimeLeft,		"text",		"manufactureInfo");
	add(_btnSell,			"button1",	"manufactureInfo");
	add(_txtFreeEngineer,	"text",		"manufactureInfo");
	add(_txtFreeSpace,		"text",		"manufactureInfo");
	add(_txtProfit,			"text",		"manufactureInfo");
	add(_txtEngineersDesc,	"text",		"manufactureInfo");
	add(_txtEngineers,		"text",		"manufactureInfo");
	add(_txtTotalDesc,		"text",		"manufactureInfo");
	add(_txtTotal,			"text",		"manufactureInfo");
	add(_btnEngineerMore,	"button1",	"manufactureInfo");
	add(_btnEngineerLess,	"button1",	"manufactureInfo");
	add(_btnUnitMore,		"button1",	"manufactureInfo");
	add(_btnUnitLess,		"button1",	"manufactureInfo");
	add(_btnStop,			"button2",	"manufactureInfo");
	add(_btnOk,				"button2",	"manufactureInfo");

	centerAllSurfaces();


	_window->setBackground(_game->getResourcePack()->getSurface("BACK17.SCR"));

	_txtTitle->setText(tr(_manfRule != nullptr ? _manfRule->getType() : _production->getRules()->getType()));
	_txtTitle->setBig();
	_txtTitle->setAlign(ALIGN_CENTER);

	_txtTimeDesc->setText(tr("STR_DAYS_HOURS_LEFT"));

	_btnSell->setText(tr("STR_SELL_PRODUCTION"));
	_btnSell->onMouseRelease((ActionHandler)& ManufactureInfoState::btnSellRelease);

	_txtEngineersDesc->setText(tr("STR_ENGINEERS_ALLOCATED"));
	_txtEngineersDesc->setBig();

	_txtEngineers->setBig();
	_txtTotal->setBig();

	_txtTotalDesc->setText(tr("STR_UNITS_TO_PRODUCE"));
	_txtTotalDesc->setBig();

	_btnEngineerMore->onMousePress(		(ActionHandler)& ManufactureInfoState::engineersMorePress);
	_btnEngineerMore->onMouseRelease(	(ActionHandler)& ManufactureInfoState::engineersMoreRelease);

	_btnEngineerLess->onMousePress(		(ActionHandler)& ManufactureInfoState::engineersLessPress);
	_btnEngineerLess->onMouseRelease(	(ActionHandler)& ManufactureInfoState::engineersLessRelease);

	_btnUnitMore->onMousePress(		(ActionHandler)& ManufactureInfoState::unitsMorePress);
	_btnUnitMore->onMouseRelease(	(ActionHandler)& ManufactureInfoState::unitsMoreRelease);
	_btnUnitMore->onMouseClick(		(ActionHandler)& ManufactureInfoState::unitsMoreClick, 0u);

	_btnUnitLess->onMousePress(		(ActionHandler)& ManufactureInfoState::unitsLessPress);
	_btnUnitLess->onMouseRelease(	(ActionHandler)& ManufactureInfoState::unitsLessRelease);
	_btnUnitLess->onMouseClick(		(ActionHandler)& ManufactureInfoState::unitsLessClick, 0u);

	_btnStop->setText(tr("STR_STOP_PRODUCTION"));
	_btnStop->onMouseClick((ActionHandler)& ManufactureInfoState::btnStopClick);
	_btnStop->onKeyboardPress(
					(ActionHandler)& ManufactureInfoState::btnStopClick,
					Options::keyCancel);

	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)& ManufactureInfoState::btnOkClick);
	_btnOk->onKeyboardPress(
					(ActionHandler)& ManufactureInfoState::btnOkClick,
					Options::keyOk);
	_btnOk->onKeyboardPress(
					(ActionHandler)& ManufactureInfoState::btnOkClick,
					Options::keyOkKeypad);

	if (_production == nullptr)
	{
		_btnOk->setVisible(false);
		_production = new Production(_manfRule, 0);
		_base->addProduction(_production);
	}

	_btnSell->setPressed(_production->getAutoSales() == true);

	initProfit();
	assignEngineers();

	_timerEngineersMore = new Timer(Timer::SCROLL_SLOW);
	_timerEngineersLess = new Timer(Timer::SCROLL_SLOW);
	_timerEngineersMore->onTimer((StateHandler)& ManufactureInfoState::onEngineersMore);
	_timerEngineersLess->onTimer((StateHandler)& ManufactureInfoState::onEngineersLess);

	_timerUnitsMore = new Timer(Timer::SCROLL_SLOW);
	_timerUnitsLess = new Timer(Timer::SCROLL_SLOW);
	_timerUnitsMore->onTimer((StateHandler)& ManufactureInfoState::onUnitsMore);
	_timerUnitsLess->onTimer((StateHandler)& ManufactureInfoState::onUnitsLess);
}

/**
 * Caches production-value for profit calculations.
 */
void ManufactureInfoState::initProfit() // private.
{
	int sellValue;
	const RuleManufacture* const manfRule (_production->getRules());
	for (std::map<std::string, int>::const_iterator
			i = manfRule->getProducedItems().begin();
			i != manfRule->getProducedItems().end();
			++i)
	{
		if (manfRule->isCraft() == true)
			sellValue = _game->getRuleset()->getCraft(i->first)->getSellCost();
		else
			sellValue = _game->getRuleset()->getItemRule(i->first)->getSellCost();

		_producedValue += sellValue * i->second;
	}
}

/**
 * Stops this Production. Exits to the previous screen.
 * @param action - pointer to an Action
 */
void ManufactureInfoState::btnStopClick(Action*) // private.
{
	_base->removeProduction(_production);
	exitState();
}

/**
 * Starts this Production (if new). Exits to the previous screen.
 * @param action - pointer to an Action
 */
void ManufactureInfoState::btnOkClick(Action*) // private.
{
	if (_manfRule != nullptr)
		_production->startProduction(
								_base,
								_game->getSavedGame(),
								_game->getRuleset());
	exitState();
}

/**
 * Exits to the previous screen.
 */
void ManufactureInfoState::exitState() // private.
{
	_game->popState();

	if (_manfRule != nullptr)
		_game->popState();
}

/**
 * Flags auto-sales of production.
 * @note This needs to be done on mouse-release for ... uh, some quirky reason.
 * Probably to get/set an accurate state for pressed/unpressed ....
 * @param action - pointer to an Action
 */
void ManufactureInfoState::btnSellRelease(Action* action) // private.
{
	switch (action->getDetails()->button.button)
	{
		case SDL_BUTTON_LEFT:
		case SDL_BUTTON_RIGHT:
			_production->setAutoSales(_btnSell->getPressed() == true);
			assignEngineers();
	}
}

/**
 * Updates display of assigned/available engineers/workshop-space and calculates
 * expenses/profits.
 */
void ManufactureInfoState::assignEngineers() // private.
{
	_txtFreeEngineer->setText(tr("STR_ENGINEERS_AVAILABLE_UC_").arg(_base->getEngineers()));
	_txtFreeSpace->setText(tr("STR_WORKSHOP_SPACE_AVAILABLE_UC_").arg(_base->getFreeWorkshops()));

	std::wostringstream woststr;

	woststr << L"> \x01" << _production->getAssignedEngineers();
	_txtEngineers->setText(woststr.str());

	woststr.str(L"");
	woststr << L"> \x01";
	if (_production->getInfinite() == true)
		woststr << L"oo";
	else
		woststr << _production->getProductionTotal();
	_txtTotal->setText(woststr.str());

	woststr.str(L"");
	std::string st;
	if (formatProfit(
				calcProfit(),
				woststr) == true)
	{
		st = "STR_MONTHLY_PROFIT_";
	}
	else
		st = "STR_MONTHLY_COST_";
	if (_production->getInfinite() == true) //|| _production->getAutoSales() == true
		woststr << L" per";
	_txtProfit->setText(tr(st).arg(woststr.str()));

	woststr.str(L"");
	int
		days,
		hours;
	if (_production->tillFinish(days, hours) == true)
	{
		woststr << days << L"\n" << hours;
		if (_production->getInfinite() == true) //|| _production->getAutoSales() == true
			woststr << L" per";
	}
	else
		woststr << L"oo";
	_txtTimeLeft->setText(woststr.str());


	_btnOk->setVisible(_production->getProductionTotal() > 0
					|| _production->getInfinite() == true);
}

/**
 * Calculates the monthly change in funds due to profit/expenses.
 * @note That this function calculates only the change in funds not the change
 * in net worth. After discussion in the forums it was decided that focusing
 * only on visible changes in funds was clearer and more valuable to the player
 * than trying to take used materials and maintenance costs into account.
 */
int ManufactureInfoState::calcProfit() // private.
{
	int
		qty,
		sellValue;

	if (_production->getAutoSales() == true)
		sellValue = _producedValue;
	else
		sellValue = 0;

	if (_production->getInfinite() == true)
		qty = 1;
	else
		qty = _production->getProductionTotal() - _production->getProducedQuantity();

	return qty * (sellValue - _production->getRules()->getManufactureCost());
}

/**
 * Formats the profit-value.
 * @param profit	- integer value ($$$) to format
 * @param woststr	- reference to the result-string
 * @return, true if profit else cost
 */
bool ManufactureInfoState::formatProfit( // private.
		int profit,
		std::wostringstream& woststr)
{
	bool ret;
	if (profit < 0)
	{
		profit = -profit;
		ret = false;
	}
	else
		ret = true;

	woststr << Text::formatCurrency(static_cast<int64_t>(profit));
	return ret;
}

/**
 * Starts the more engineers Timer.
 * @param action - pointer to an Action
 */
void ManufactureInfoState::engineersMorePress(Action* action) // private.
{
	switch (action->getDetails()->button.button)
	{
		case SDL_BUTTON_LEFT:
			engineersMoreByValue(stepDelta());
			_timerEngineersMore->setInterval(Timer::SCROLL_SLOW);
			_timerEngineersMore->start();
			break;

		case SDL_BUTTON_RIGHT:
			engineersMoreByValue(std::numeric_limits<int>::max());
	}
}

/**
 * Stops the more engineers Timer.
 * @param action - pointer to an Action
 */
void ManufactureInfoState::engineersMoreRelease(Action* action) // private.
{
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
		_timerEngineersMore->stop();
}

/**
 * Starts the less engineers Timer.
 * @param action - pointer to an Action
 */
void ManufactureInfoState::engineersLessPress(Action* action) // private.
{
	switch (action->getDetails()->button.button)
	{
		case SDL_BUTTON_LEFT:
			engineersLessByValue(stepDelta());
			_timerEngineersLess->setInterval(Timer::SCROLL_SLOW);
			_timerEngineersLess->start();
			break;

		case SDL_BUTTON_RIGHT:
			engineersLessByValue(std::numeric_limits<int>::max());
	}
}

/**
 * Stops the less engineers Timer.
 * @param action - pointer to an Action
 */
void ManufactureInfoState::engineersLessRelease(Action* action) // private.
{
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
		_timerEngineersLess->stop();
}

/**
 * Starts the more units Timer.
 * @param action - pointer to an Action
 */
void ManufactureInfoState::unitsMorePress(Action* action) // private.
{
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT
		&& _production->getProductionTotal() < std::numeric_limits<int>::max())
	{
		_timerUnitsMore->setInterval(Timer::SCROLL_SLOW);
		_timerUnitsMore->start();
	}
}

/**
 * Stops the more units Timer.
 * @param action - pointer to an Action
 */
void ManufactureInfoState::unitsMoreRelease(Action* action) // private.
{
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
		_timerUnitsMore->stop();
}

/**
 * Increases the quantity of units to produce.
 * @param action - pointer to an Action
 */
void ManufactureInfoState::unitsMoreClick(Action* action) // private.
{
	if (_production->getInfinite() == false) // We can't increase over infinite :) [cf. Cantor]
	{
		switch (action->getDetails()->button.button)
		{
			case SDL_BUTTON_LEFT:
				unitsMoreByValue(stepDelta());
				break;

			case SDL_BUTTON_RIGHT:
				if (_production->getRules()->isCraft() == true)
					unitsMoreByValue(std::numeric_limits<int>::max());
				else
				{
					_production->setInfinite(true);
					assignEngineers();
				}
		}
	}
}

/**
 * Starts the less units Timer.
 * @param action - pointer to an Action
 */
void ManufactureInfoState::unitsLessPress(Action* action) // private.
{
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
	{
		_timerUnitsLess->setInterval(Timer::SCROLL_SLOW);
		_timerUnitsLess->start();
	}
}

/**
 * Stops the less units Timer.
 * @param action - pointer to an Action
 */
void ManufactureInfoState::unitsLessRelease(Action* action) // private.
{
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
		_timerUnitsLess->stop();
}

/**
 * Decreases the quantity of units to produce.
 * @param action - pointer to an Action
 */
void ManufactureInfoState::unitsLessClick(Action* action) // private.
{
	switch (action->getDetails()->button.button)
	{
		case SDL_BUTTON_LEFT:
			if (_production->getInfinite() == false)
			{
				unitsLessByValue(stepDelta());
				break;
			}
			// no break;

		case SDL_BUTTON_RIGHT:
			_production->setInfinite(false);
			_production->setProductionTotal(_production->getProducedQuantity() + 1);
			assignEngineers();
	}
}

/**
 * Runs state functionality every cycle - updates the timer.
 */
void ManufactureInfoState::think() // private.
{
	State::think();

	_timerEngineersMore->think(this, nullptr);
	_timerEngineersLess->think(this, nullptr);

	_timerUnitsMore->think(this, nullptr);
	_timerUnitsLess->think(this, nullptr);
}

/**
 * Adds engineers to the production.
 */
void ManufactureInfoState::onEngineersMore() // private.
{
	_timerEngineersMore->setInterval(Timer::SCROLL_FAST);
	engineersMoreByValue(stepDelta());
}

/**
 * Adds a given quantity of engineers to the production if possible.
 * @param delta - quantity to add
 */
void ManufactureInfoState::engineersMoreByValue(int delta) // private.
{
	const int
		availableEngineers (_base->getEngineers()),
		availableWorkSpace (_base->getFreeWorkshops());

	if (availableEngineers > 0 && availableWorkSpace > 0)
	{
		delta = std::min(delta,
						 std::min(availableEngineers,
								  availableWorkSpace));
		_production->setAssignedEngineers(_production->getAssignedEngineers() + delta);
		_base->setEngineers(_base->getEngineers() - delta);
		assignEngineers();
	}
}

/**
 * Subtracts engineers from the production.
 */
void ManufactureInfoState::onEngineersLess() // private.
{
	_timerEngineersLess->setInterval(Timer::SCROLL_FAST);
	engineersLessByValue(stepDelta());
}

/**
 * Subtracts a given quantity of engineers from the production if possible.
 * @param delta - quantity to subtract
 */
void ManufactureInfoState::engineersLessByValue(int delta) // private.
{
	const int assigned (_production->getAssignedEngineers());
	if (assigned > 0)
	{
		delta = std::min(delta, assigned);
		_production->setAssignedEngineers(assigned - delta);
		_base->setEngineers(_base->getEngineers() + delta);
		assignEngineers();
	}
}

/**
 * Increases quantity of units to produce.
 */
void ManufactureInfoState::onUnitsMore() // private.
{
	_timerUnitsMore->setInterval(Timer::SCROLL_FAST);
	unitsMoreByValue(stepDelta());
}

/**
 * Adds a given quantity of units to produce if possible.
 * @param delta - quantity to add
 */
void ManufactureInfoState::unitsMoreByValue(int delta) // private.
{
	if (_production->getRules()->isCraft() == true
		&& _base->getFreeHangars() < 1)
	{
		_timerUnitsMore->stop();

		const RuleInterface* const uiRule (_game->getRuleset()->getInterface("basescape"));
		_game->pushState(new ErrorMessageState(
											tr("STR_NO_FREE_HANGARS_FOR_CRAFT_PRODUCTION"),
											_palette,
											uiRule->getElement("errorMessage")->color,
											"BACK17.SCR",
											uiRule->getElement("errorPalette")->color));
	}
	else
	{
		const int total (_production->getProductionTotal());
		delta = std::min(delta,
						 std::numeric_limits<int>::max() - total);

		if (_production->getRules()->isCraft() == true)
			delta = std::min(delta,
							_base->getFreeHangars());
		_production->setProductionTotal(total + delta);
		assignEngineers();
	}
}

/**
 * Decreases quantity of units to produce.
 */
void ManufactureInfoState::onUnitsLess() // private.
{
	_timerUnitsLess->setInterval(Timer::SCROLL_FAST);
	unitsLessByValue(stepDelta());
}

/**
 * Subtracts a given quantity of units to produce if possible.
 * @param delta - quantity to subtract
 */
void ManufactureInfoState::unitsLessByValue(int delta) // private.
{
	const int total (_production->getProductionTotal());
	delta = std::min(delta,
					 total - (_production->getProducedQuantity() + 1));
	_production->setProductionTotal(total - delta);
	assignEngineers();
}

/**
 * Gets the quantity by which to increase/decrease.
 * @note what were these guys smokin'
 * @return, 10 if CTRL is pressed else 1
 */
int ManufactureInfoState::stepDelta() const // private.
{
	if ((SDL_GetModState() & KMOD_CTRL) == 0)
		return 1;

	return 10;
}

}
