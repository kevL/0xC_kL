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
#include "../Savegame/Manufacture.h"


namespace OpenXcom
{

/**
 * Initializes all elements in the ManufactureInfo screen (start Manufacture).
 * @param base		- pointer to the Base to get info from
 * @param mfRule	- pointer to the RuleManufacture to start
 */
ManufactureInfoState::ManufactureInfoState(
		Base* const base,
		const RuleManufacture* const mfRule)
	:
		_base(base),
		_mfRule(mfRule),
		_manufacture(nullptr),
		_valueProduct(0),
		_start(true)
{
	_manufacture = new Manufacture(_mfRule);
	_base->addManufactureProject(_manufacture);

	buildUi();
}

/**
 * Initializes all elements in the ManufactureInfo screen (adjust Manufacture).
 * @param base			- pointer to the Base to get info from
 * @param manufacture	- pointer to the Manufacture to adjust
 */
ManufactureInfoState::ManufactureInfoState(
		Base* const base,
		Manufacture* const manufacture)
	:
		_base(base),
		_mfRule(nullptr),
		_manufacture(manufacture),
		_valueProduct(0),
		_start(false)
{
	_mfRule = _manufacture->getRules();

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

	_txtDurationDesc	= new Text(30, 17, 244, 44);
	_txtDuration		= new Text(30, 17, 274, 44);
	_btnSell			= new ToggleTextButton(60, 16, 244, 64);

	_txtFreeEngineer	= new Text(100, 9, 16, 55);
	_txtFreeSpace		= new Text(100, 9, 16, 65);
	_txtProfit			= new Text(160, 9, 16, 75);

	_txtEngineersDesc	= new Text(84, 17,  16, 88);
	_txtEngineers		= new Text(50, 17, 100, 88);

	_txtUnitsDesc		= new Text(84, 17, 176, 88);
	_txtUnits			= new Text(50, 17, 260, 88);

	_btnEngineerMore	= new ArrowButton(ARROW_BIG_UP,   100, 16,  30, 119);
	_btnEngineerLess	= new ArrowButton(ARROW_BIG_DOWN, 100, 16,  30, 143);
	_btnUnitMore		= new ArrowButton(ARROW_BIG_UP,   100, 16, 190, 119);
	_btnUnitLess		= new ArrowButton(ARROW_BIG_DOWN, 100, 16, 190, 143);

	_btnStop			= new TextButton(130, 16,  20, 170);
	_btnOk				= new TextButton(130, 16, 170, 170);

	setInterface("manufactureInfo");

	add(_window,			"window",	"manufactureInfo");
	add(_txtTitle,			"text",		"manufactureInfo");
	add(_txtDurationDesc,	"text",		"manufactureInfo");
	add(_txtDuration,		"text",		"manufactureInfo");
	add(_btnSell,			"button1",	"manufactureInfo");
	add(_txtFreeEngineer,	"text",		"manufactureInfo");
	add(_txtFreeSpace,		"text",		"manufactureInfo");
	add(_txtProfit,			"text",		"manufactureInfo");
	add(_txtEngineersDesc,	"text",		"manufactureInfo");
	add(_txtEngineers,		"text",		"manufactureInfo");
	add(_txtUnitsDesc,		"text",		"manufactureInfo");
	add(_txtUnits,			"text",		"manufactureInfo");
	add(_btnEngineerMore,	"button1",	"manufactureInfo");
	add(_btnEngineerLess,	"button1",	"manufactureInfo");
	add(_btnUnitMore,		"button1",	"manufactureInfo");
	add(_btnUnitLess,		"button1",	"manufactureInfo");
	add(_btnStop,			"button2",	"manufactureInfo");
	add(_btnOk,				"button2",	"manufactureInfo");

	centerSurfaces();


	_window->setBackground(_game->getResourcePack()->getSurface("BACK17.SCR"));

	_txtTitle->setText(tr(_mfRule->getType()));
	_txtTitle->setBig();
	_txtTitle->setAlign(ALIGN_CENTER);

	_txtDurationDesc->setText(tr("STR_DAYS_HOURS_LEFT"));

	_btnSell->setText(tr("STR_SELL_PRODUCTION"));
	_btnSell->onMouseRelease(static_cast<ActionHandler>(&ManufactureInfoState::btnSellRelease));

	_txtEngineersDesc->setText(tr("STR_ENGINEERS_ALLOCATED"));
	_txtEngineersDesc->setBig();

	_txtUnitsDesc->setText(tr("STR_UNITS_TO_PRODUCE"));
	_txtUnitsDesc->setBig();

	_txtEngineers->setBig();
	_txtUnits->setBig();

	_btnEngineerMore->onMousePress(		static_cast<ActionHandler>(&ManufactureInfoState::engineersMorePress));
	_btnEngineerMore->onMouseRelease(	static_cast<ActionHandler>(&ManufactureInfoState::engineersMoreRelease));

	_btnEngineerLess->onMousePress(		static_cast<ActionHandler>(&ManufactureInfoState::engineersLessPress));
	_btnEngineerLess->onMouseRelease(	static_cast<ActionHandler>(&ManufactureInfoState::engineersLessRelease));

	_btnUnitMore->onMousePress(		static_cast<ActionHandler>(&ManufactureInfoState::unitsMorePress));
	_btnUnitMore->onMouseRelease(	static_cast<ActionHandler>(&ManufactureInfoState::unitsMoreRelease));
	_btnUnitMore->onMouseClick(		static_cast<ActionHandler>(&ManufactureInfoState::unitsMoreClick),
									0u);

	_btnUnitLess->onMousePress(		static_cast<ActionHandler>(&ManufactureInfoState::unitsLessPress));
	_btnUnitLess->onMouseRelease(	static_cast<ActionHandler>(&ManufactureInfoState::unitsLessRelease));
	_btnUnitLess->onMouseClick(		static_cast<ActionHandler>(&ManufactureInfoState::unitsLessClick),
									0u);

	_btnStop->setText(tr("STR_STOP_PRODUCTION"));
	_btnStop->onMouseClick(		static_cast<ActionHandler>(&ManufactureInfoState::btnStopClick));
	_btnStop->onKeyboardPress(	static_cast<ActionHandler>(&ManufactureInfoState::btnStopClick),
								Options::keyCancel);

	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick(	static_cast<ActionHandler>(&ManufactureInfoState::btnOkClick));
	_btnOk->onKeyboardPress(static_cast<ActionHandler>(&ManufactureInfoState::btnOkClick),
							Options::keyOk);
	_btnOk->onKeyboardPress(static_cast<ActionHandler>(&ManufactureInfoState::btnOkClick),
							Options::keyOkKeypad);

	_btnSell->setPressed(_manufacture->getAutoSales() == true);

	initProfit();
	assignEngineers();


	_timerEngineersMore = new Timer(Timer::SCROLL_SLOW);
	_timerEngineersLess = new Timer(Timer::SCROLL_SLOW);
	_timerEngineersMore->onTimer(static_cast<StateHandler>(&ManufactureInfoState::onEngineersMore));
	_timerEngineersLess->onTimer(static_cast<StateHandler>(&ManufactureInfoState::onEngineersLess));

	_timerUnitsMore = new Timer(Timer::SCROLL_SLOW);
	_timerUnitsLess = new Timer(Timer::SCROLL_SLOW);
	_timerUnitsMore->onTimer(static_cast<StateHandler>(&ManufactureInfoState::onUnitsMore));
	_timerUnitsLess->onTimer(static_cast<StateHandler>(&ManufactureInfoState::onUnitsLess));
}

/**
 * Caches manufacture-value for profit calculations.
 */
void ManufactureInfoState::initProfit() // private.
{
	int sellValue;
	for (std::map<std::string, int>::const_iterator
			i = _mfRule->getManufacturedItems().begin();
			i != _mfRule->getManufacturedItems().end();
			++i)
	{
		if (_mfRule->isCraft() == true)
			sellValue = _game->getRuleset()->getCraft(i->first)->getSellCost();
		else
			sellValue = _game->getRuleset()->getItemRule(i->first)->getSellCost();

		_valueProduct += sellValue * i->second;
	}
}

/**
 * Stops this Manufacture and exits to the previous screen.
 * @param action - pointer to an Action
 */
void ManufactureInfoState::btnStopClick(Action*) // private.
{
	_base->clearManufactureProject(_manufacture);
	exitState();
}

/**
 * Starts the Manufacture if '_start' and exits to the previous screen.
 * @param action - pointer to an Action
 */
void ManufactureInfoState::btnOkClick(Action*) // private.
{
	if (_start == true)
		_manufacture->startManufacture(
								_base,
								_game->getSavedGame());
	exitState();
}

/**
 * Exits to the previous screen.
 */
void ManufactureInfoState::exitState() // private.
{
	_game->popState();
	if (_start == true) _game->popState();
}

/**
 * Flags auto-sales of manufacture.
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
			_manufacture->setAutoSales(_btnSell->getPressed() == true);
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

	woststr << L"> \x01" << _manufacture->getAssignedEngineers();
	_txtEngineers->setText(woststr.str());

	woststr.str(L"");
	woststr << L"> \x01";
	if (_manufacture->getInfinite() == true)
		woststr << L"oo";
	else
		woststr << _manufacture->getManufactureTotal();
	_txtUnits->setText(woststr.str());

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
	if (_manufacture->getInfinite() == true) //|| _manufacture->getAutoSales() == true
		woststr << L" per";
	_txtProfit->setText(tr(st).arg(woststr.str()));

	woststr.str(L"");
	int
		days,
		hours;
	if (_manufacture->tillFinish(days, hours) == true)
	{
		woststr << days << L"\n" << hours;
		if (_manufacture->getInfinite() == true) //|| _manufacture->getAutoSales() == true
			woststr << L" per";
	}
	else
		woststr << L"oo";
	_txtDuration->setText(woststr.str());
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

	if (_manufacture->getAutoSales() == true)
		sellValue = _valueProduct;
	else
		sellValue = 0;

	if (_manufacture->getInfinite() == true)
		qty = 1;
	else
		qty = _manufacture->getManufactureTotal() - _manufacture->getQuantityManufactured();

	return qty * (sellValue - _mfRule->getManufactureCost());
}

/**
 * Formats the profit-value.
 * @param profit	- integer value ($$$) to format
 * @param woststr	- reference to the result-string
 * @return, true if profit else cost
 */
bool ManufactureInfoState::formatProfit( // private/static.
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
	if (_manufacture->getInfinite() == false // We can't increase over infinite :) [cf. Cantor]
		&& action->getDetails()->button.button == SDL_BUTTON_LEFT)
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
	if (_manufacture->getInfinite() == false) // We can't increase over infinite :) [cf. Cantor]
	{
		switch (action->getDetails()->button.button)
		{
			case SDL_BUTTON_LEFT:
				unitsMoreByValue(stepDelta());
				break;

			case SDL_BUTTON_RIGHT:
				if (_manufacture->getRules()->isCraft() == true)
					unitsMoreByValue(std::numeric_limits<int>::max());
				else
				{
					_manufacture->setInfinite(true);
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
			if (_manufacture->getInfinite() == false)
			{
				unitsLessByValue(stepDelta());
				break;
			}
			// no break;

		case SDL_BUTTON_RIGHT:
			_manufacture->setInfinite(false);
			_manufacture->setManufactureTotal(_manufacture->getQuantityManufactured() + 1);
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
 * Adds engineers to the manufacture.
 */
void ManufactureInfoState::onEngineersMore() // private.
{
	_timerEngineersMore->setInterval(Timer::SCROLL_FAST);
	engineersMoreByValue(stepDelta());
}

/**
 * Adds a given quantity of engineers to the manufacture if possible.
 * @param delta - quantity to add
 */
void ManufactureInfoState::engineersMoreByValue(int delta) // private.
{
	const int
		availableEngineers (_base->getEngineers()),
		availableWorkSpace (_base->getFreeWorkshops());

	if (availableEngineers != 0 && availableWorkSpace != 0)
	{
		delta = std::min(delta,
						 std::min(availableEngineers,
								  availableWorkSpace));
		_manufacture->setAssignedEngineers(_manufacture->getAssignedEngineers() + delta);
		_base->setEngineers(_base->getEngineers() - delta);
		assignEngineers();
	}
}

/**
 * Subtracts engineers from the manufacture.
 */
void ManufactureInfoState::onEngineersLess() // private.
{
	_timerEngineersLess->setInterval(Timer::SCROLL_FAST);
	engineersLessByValue(stepDelta());
}

/**
 * Subtracts a given quantity of engineers from the manufacture if possible.
 * @param delta - quantity to subtract
 */
void ManufactureInfoState::engineersLessByValue(int delta) // private.
{
	const int assigned (_manufacture->getAssignedEngineers());
	if (assigned != 0)
	{
		delta = std::min(delta, assigned);
		_manufacture->setAssignedEngineers(assigned - delta);
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
	if (_mfRule->isCraft() == true && _base->getFreeHangars() == 0)
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
		const int total (_manufacture->getManufactureTotal());
		delta = std::min(delta,
						 std::numeric_limits<int>::max() - total);

		if (_mfRule->isCraft() == true)
			delta = std::min(delta,
							_base->getFreeHangars());
		_manufacture->setManufactureTotal(total + delta);
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
	const int total (_manufacture->getManufactureTotal());
	delta = std::min(delta,
					 total - (_manufacture->getQuantityManufactured() + 1));
	_manufacture->setManufactureTotal(total - delta);
	assignEngineers();
}

}
