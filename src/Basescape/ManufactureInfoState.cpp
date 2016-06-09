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
	delete _timerIncEngineers;
	delete _timerDecEngineers;
	delete _timerIncUnits;
	delete _timerDecUnits;
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

//	_txtEngineerUp		= new Text(100, 17, 32, 111);
//	_btnEngineerUp		= new ArrowButton(ARROW_BIG_UP, 14, 14, 145, 111);
//	_txtEngineerDown	= new Text(100, 17, 32, 135);
//	_btnEngineerDown	= new ArrowButton(ARROW_BIG_DOWN, 14, 14, 145, 135);
//	_txtUnitUp			= new Text(100, 17, 205, 111);
//	_btnUnitUp			= new ArrowButton(ARROW_BIG_UP, 14, 14, 280, 111);
//	_txtUnitDown		= new Text(100, 17, 205, 135);
//	_btnUnitDown		= new ArrowButton(ARROW_BIG_DOWN, 14, 14, 280, 135);
	_btnEngineerUp		= new ArrowButton(ARROW_BIG_UP,   100, 16,  30, 119);
	_btnEngineerDown	= new ArrowButton(ARROW_BIG_DOWN, 100, 16,  30, 143);
	_btnUnitUp			= new ArrowButton(ARROW_BIG_UP,   100, 16, 190, 119);
	_btnUnitDown		= new ArrowButton(ARROW_BIG_DOWN, 100, 16, 190, 143);

	_btnStop			= new TextButton(130, 16,  20, 170);
	_btnOk				= new TextButton(130, 16, 170, 170);

//	_surfaceEngineers = new InteractiveSurface(160, 150, 0, 25);
//	_surfaceEngineers->onMouseClick((ActionHandler)& ManufactureInfoState::handleWheelEngineer, 0);

//	_surfaceUnits = new InteractiveSurface(160, 150, 160, 25);
//	_surfaceUnits->onMouseClick((ActionHandler)& ManufactureInfoState::handleWheelUnit, 0);

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
//	add(_txtEngineerUp,		"text",		"manufactureInfo");
	add(_btnEngineerUp,		"button1",	"manufactureInfo");
//	add(_txtEngineerDown,	"text",		"manufactureInfo");
	add(_btnEngineerDown,	"button1",	"manufactureInfo");
//	add(_txtUnitUp,			"text",		"manufactureInfo");
	add(_btnUnitUp,			"button1",	"manufactureInfo");
//	add(_txtUnitDown,		"text",		"manufactureInfo");
	add(_btnUnitDown,		"button1",	"manufactureInfo");
	add(_btnStop,			"button2",	"manufactureInfo");
	add(_btnOk,				"button2",	"manufactureInfo");

//	add(_surfaceEngineers);
//	add(_surfaceUnits);

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

//	_txtEngineerUp->setText(tr("STR_INCREASE_UC"));
//	_txtEngineerDown->setText(tr("STR_DECREASE_UC"));

	_btnEngineerUp->onMousePress((ActionHandler)& ManufactureInfoState::incEngineersPress);
	_btnEngineerUp->onMouseRelease((ActionHandler)& ManufactureInfoState::incEngineersRelease);
	_btnEngineerUp->onMouseClick((ActionHandler)& ManufactureInfoState::incEngineersClick, 0u);

	_btnEngineerDown->onMousePress((ActionHandler)& ManufactureInfoState::decEngineersPress);
	_btnEngineerDown->onMouseRelease((ActionHandler)& ManufactureInfoState::decEngineersRelease);
	_btnEngineerDown->onMouseClick((ActionHandler)& ManufactureInfoState::decEngineersClick, 0u);

//	_txtUnitUp->setText(tr("STR_INCREASE_UC"));
//	_txtUnitDown->setText(tr("STR_DECREASE_UC"));

	_btnUnitUp->onMousePress((ActionHandler)& ManufactureInfoState::incUnitsPress);
	_btnUnitUp->onMouseRelease((ActionHandler)& ManufactureInfoState::incUnitsRelease);
	_btnUnitUp->onMouseClick((ActionHandler)& ManufactureInfoState::incUnitsClick, 0u);

	_btnUnitDown->onMousePress((ActionHandler)& ManufactureInfoState::decUnitsPress);
	_btnUnitDown->onMouseRelease((ActionHandler)& ManufactureInfoState::decUnitsRelease);
	_btnUnitDown->onMouseClick((ActionHandler)& ManufactureInfoState::decUnitsClick, 0u);

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

	_timerIncEngineers = new Timer(Timer::SCROLL_SLOW);
	_timerDecEngineers = new Timer(Timer::SCROLL_SLOW);
	_timerIncEngineers->onTimer((StateHandler)& ManufactureInfoState::onIncEngineers);
	_timerDecEngineers->onTimer((StateHandler)& ManufactureInfoState::onDecEngineers);

	_timerIncUnits = new Timer(Timer::SCROLL_SLOW);
	_timerDecUnits = new Timer(Timer::SCROLL_SLOW);
	_timerIncUnits->onTimer((StateHandler)& ManufactureInfoState::onIncUnits);
	_timerDecUnits->onTimer((StateHandler)& ManufactureInfoState::onDecUnits);
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
 * Stops this Production. Returns to the previous screen.
 * @param action - pointer to an Action
 */
void ManufactureInfoState::btnStopClick(Action*) // private.
{
	_base->removeProduction(_production);
	exitState();
}

/**
 * Starts this Production (if new). Returns to the previous screen.
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
 * Returns to the previous screen.
 */
void ManufactureInfoState::exitState() // private.
{
	_game->popState();

	if (_manfRule != nullptr)
		_game->popState();
}

/**
 * Handler for releasing the Sell button.
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
/*	// does not take into account leap years
	static const int AVG_HOURS_PER_MONTH = (365 * 24) / 12;

	const RuleManufacture* const manfRule = _production->getRules();
	int
		sellValue = _btnSell->getPressed() ? _producedValue : 0,
		numEngineers = _production->getAssignedEngineers(),
		manHoursPerMonth = AVG_HOURS_PER_MONTH * numEngineers;

	if (_production->getInfinite() == false)
	{
		// scale down to actual number of man hours required if the job takes less than one month
		const int manHoursRemaining = manfRule->getManufactureTime()
									* (_production->getTotalQuantity() - _production->getProducedQuantity());
		manHoursPerMonth = std::min(
								manHoursPerMonth,
								manHoursRemaining);
	}

	const int itemsPerMonth = static_cast<int>(static_cast<float>(manHoursPerMonth)
							/ static_cast<float>(manfRule->getManufactureTime()));
	return itemsPerMonth * (sellValue - manfRule->getManufactureCost()); */

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
/*	float profit_f (static_cast<float>(profit));

	bool ret;
	if (profit_f < 0.f)
	{
		profit_f = -profit_f;
		ret = false;
	}
	else
		ret = true;

	std::wstring suffix;
	if (profit_f >= 1e9f)
	{
		profit_f /= 1e9f;
		suffix = L" b";
	}
	else if (profit_f >= 1e6f)
	{
		profit_f /= 1e6f;
		suffix = L" m";
	}
	else if (profit_f >= 1e3f)
	{
		profit_f /= 1e3f;
		suffix = L" k";
	}

	woststr << L"$" << std::fixed << std::setprecision(1) << profit_f << suffix;
	return ret; */

/**
 * Adds given number of engineers to the project if possible.
 * @param change - how much to add
 */
void ManufactureInfoState::incEngineers(int change) // private.
{
	const int
		availableEngineers (_base->getEngineers()),
		availableWorkSpace (_base->getFreeWorkshops());

	if (availableEngineers > 0 && availableWorkSpace > 0)
	{
		change = std::min(change,
						  std::min(availableEngineers,
								   availableWorkSpace));
		_production->setAssignedEngineers(_production->getAssignedEngineers() + change);
		_base->setEngineers(_base->getEngineers() - change);
		assignEngineers();
	}
}

/**
 * Starts the timerIncEngineers.
 * @param action - pointer to an Action
 */
void ManufactureInfoState::incEngineersPress(Action* action) // private.
{
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
		_timerIncEngineers->start();
}

/**
 * Stops the timerIncEngineers.
 * @param action - pointer to an Action
 */
void ManufactureInfoState::incEngineersRelease(Action* action) // private.
{
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
	{
		_timerIncEngineers->setInterval(Timer::SCROLL_SLOW);
		_timerIncEngineers->stop();
	}
}

/**
 * Allocates all engineers.
 * @param action - pointer to an Action
 */
void ManufactureInfoState::incEngineersClick(Action* action) // private.
{
	switch (action->getDetails()->button.button)
	{
		case SDL_BUTTON_LEFT:
			incEngineers(stepDelta());
			break;

		case SDL_BUTTON_RIGHT:
			incEngineers(std::numeric_limits<int>::max());
	}
}

/**
 * Removes the given number of engineers from the project if possible.
 * @param change - how much to subtract
 */
void ManufactureInfoState::decEngineers(int change) // private.
{
	const int assigned (_production->getAssignedEngineers());
	if (assigned > 0)
	{
		change = std::min(change, assigned);
		_production->setAssignedEngineers(assigned - change);
		_base->setEngineers(_base->getEngineers() + change);
		assignEngineers();
	}
}

/**
 * Starts the timerDecEngineers.
 * @param action - pointer to an Action
 */
void ManufactureInfoState::decEngineersPress(Action* action) // private.
{
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
		_timerDecEngineers->start();
}

/**
 * Stops the timerDecEngineers.
 * @param action - pointer to an Action
 */
void ManufactureInfoState::decEngineersRelease(Action* action) // private.
{
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
	{
		_timerDecEngineers->setInterval(Timer::SCROLL_SLOW);
		_timerDecEngineers->stop();
	}
}

/**
 * Removes engineers from the production.
 * @param action - pointer to an Action
 */
void ManufactureInfoState::decEngineersClick(Action* action) // private.
{
	switch (action->getDetails()->button.button)
	{
		case SDL_BUTTON_LEFT:
			decEngineers(stepDelta());
			break;

		case SDL_BUTTON_RIGHT:
			decEngineers(std::numeric_limits<int>::max());
	}
}

/**
 * Adds given number of units to produce to the project if possible.
 * @param change - how much to add
 */
void ManufactureInfoState::incUnits(int change) // private.
{
	if (_production->getRules()->isCraft() == true
		&& _base->getFreeHangars() < 1)
	{
		_timerIncUnits->stop();

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
		change = std::min(change,
						  std::numeric_limits<int>::max() - total);

		if (_production->getRules()->isCraft() == true)
			change = std::min(change,
							  _base->getFreeHangars());
		_production->setProductionTotal(total + change);
		assignEngineers();
	}
}

/**
 * Starts the timerIncUnits.
 * @param action - pointer to an Action
 */
void ManufactureInfoState::incUnitsPress(Action* action) // private.
{
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT
		&& _production->getProductionTotal() < std::numeric_limits<int>::max())
	{
		_timerIncUnits->start();
	}
}

/**
 * Stops the timerIncUnits.
 * @param action - pointer to an Action
 */
void ManufactureInfoState::incUnitsRelease(Action* action) // private.
{
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
	{
		_timerIncUnits->setInterval(Timer::SCROLL_SLOW);
		_timerIncUnits->stop();
	}
}

/**
 * Increases the units to produce by 1 LMB or to infinite RMB.
 * @param action - pointer to an Action
 */
void ManufactureInfoState::incUnitsClick(Action* action) // private.
{
	if (_production->getInfinite() == false) // We can't increase over infinite :) [cf. Cantor]
	{
		switch (action->getDetails()->button.button)
		{
			case SDL_BUTTON_RIGHT:
				if (_production->getRules()->isCraft() == true)
					incUnits(std::numeric_limits<int>::max());
				else
				{
					_production->setInfinite(true);
					assignEngineers();
				}
				break;

			case SDL_BUTTON_LEFT:
				incUnits(stepDelta());
		}
	}
}

/**
 * Removes the given number of units to produce from the total if possible.
 * @param change - how many to subtract
 */
void ManufactureInfoState::decUnits(int change) // private.
{
	const int total (_production->getProductionTotal());
	change = std::min(change,
					  total - (_production->getProducedQuantity() + 1));
	_production->setProductionTotal(total - change);
	assignEngineers();
}

/**
 * Starts the timerDecUnits.
 * @param action - pointer to an Action
 */
void ManufactureInfoState::decUnitsPress(Action* action) // private.
{
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
		_timerDecUnits->start();
}

/**
 * Stops the timerDecUnits.
 * @param action - pointer to an Action
 */
void ManufactureInfoState::decUnitsRelease(Action* action) // private.
{
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
	{
		_timerDecUnits->setInterval(Timer::SCROLL_SLOW);
		_timerDecUnits->stop();
	}
}

/**
 * Decreases the units to produce.
 * @param action - pointer to an Action
 */
void ManufactureInfoState::decUnitsClick(Action* action) // private.
{
	const int btnId (action->getDetails()->button.button);
	switch (btnId)
	{
		case SDL_BUTTON_LEFT:
		case SDL_BUTTON_RIGHT:
		{
			const int produced (_production->getProducedQuantity());
			if (_production->getProductionTotal() <= produced)
			{
				_production->setInfinite(false);
				_production->setProductionTotal(produced + 1);
				assignEngineers();
			}
			else
			{
				switch (btnId)
				{
					case SDL_BUTTON_LEFT:
						if (_production->getInfinite() == false)
						{
							decUnits(stepDelta());
							break;
						}
						// no break;

					case SDL_BUTTON_RIGHT:
						_production->setInfinite(false);
						_production->setProductionTotal(produced + 1);
						assignEngineers();
				}
			}
		}
	}
}

/**
 * Assigns more engineers if possible.
 */
void ManufactureInfoState::onIncEngineers() // private.
{
	_timerIncEngineers->setInterval(Timer::SCROLL_FAST);
	incEngineers(stepDelta());
}

/**
 * Removes engineers if possible.
 */
void ManufactureInfoState::onDecEngineers() // private.
{
	_timerDecEngineers->setInterval(Timer::SCROLL_FAST);
	decEngineers(stepDelta());
}

/**
 * Orders more units to be built.
 */
void ManufactureInfoState::onIncUnits() // private.
{
	_timerIncUnits->setInterval(Timer::SCROLL_FAST);
	incUnits(stepDelta());
}

/**
 * Ordes less units to be built if possible.
 */
void ManufactureInfoState::onDecUnits() // private.
{
	_timerDecUnits->setInterval(Timer::SCROLL_FAST);
	decUnits(stepDelta());
}

/**
 * Returns the quantity by which to increase/decrease.
 * @note what were these guys smokin'
 * @return, 10 if CTRL is pressed else 1
 */
int ManufactureInfoState::stepDelta() const // private.
{
	if ((SDL_GetModState() & KMOD_CTRL) == 0)
		return 1;

	return 10;
}

/**
 * Runs state functionality every cycle - updates the timer.
 */
void ManufactureInfoState::think() // private.
{
	State::think();

	_timerIncEngineers->think(this, nullptr);
	_timerDecEngineers->think(this, nullptr);

	_timerIncUnits->think(this, nullptr);
	_timerDecUnits->think(this, nullptr);
}

}

/**
 * Increases or decreases the Engineers according the mouse-wheel used.
 * @param action - pointer to an Action
 *
void ManufactureInfoState::handleWheelEngineer(Action* action)
{
	if (action->getDetails()->button.button == SDL_BUTTON_WHEELUP)
		incEngineers(Options::changeValueByMouseWheel);
	else if (action->getDetails()->button.button == SDL_BUTTON_WHEELDOWN)
		decEngineers(Options::changeValueByMouseWheel);
} */
/**
 * Increases or decreases the Units to produce according the mouse-wheel used.
 * @param action - pointer to an Action
 */
/* void ManufactureInfoState::handleWheelUnit(Action* action)
{
	if (action->getDetails()->button.button == SDL_BUTTON_WHEELUP)
		incUnits(Options::changeValueByMouseWheel);
	else if (action->getDetails()->button.button == SDL_BUTTON_WHEELDOWN)
		decUnits(Options::changeValueByMouseWheel);
} */
