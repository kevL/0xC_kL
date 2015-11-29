/*
 * Copyright 2010-2015 OpenXcom Developers.
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
 * @param base - pointer to the Base to get info from
 * @param manufRule - pointer to the RuleManufacture to produce
 */
ManufactureInfoState::ManufactureInfoState(
		Base* const base,
		const RuleManufacture* const manufRule)
	:
		_base(base),
		_manufRule(manufRule),
		_production(NULL),
		_producedItemsValue(0)
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
		_manufRule(NULL),
		_production(production),
		_producedItemsValue(0)
{
	buildUi();
}

/**
 * Frees up memory that's not automatically cleaned on exit.
 */
ManufactureInfoState::~ManufactureInfoState()
{
	delete _timerMoreEngineer;
	delete _timerLessEngineer;
	delete _timerMoreUnit;
	delete _timerLessUnit;
}

/**
 * Builds screen User Interface.
 */
void ManufactureInfoState::buildUi() // private.
{
	_screen = false;

	_window					= new Window(this, 320, 170, 0, 15, POPUP_BOTH);

	_txtTitle				= new Text(280, 17, 20, 25);

	_txtTimeDescr			= new Text(30, 17, 244, 36);
	_txtTimeTotal			= new Text(30, 17, 274, 36);
	_btnSell				= new ToggleTextButton(60, 16, 244, 56);

	_txtAvailableEngineer	= new Text(100, 9, 16, 47);
	_txtAvailableSpace		= new Text(100, 9, 16, 57);
	_txtMonthlyProfit		= new Text(160, 9, 16, 67);

	_txtAllocatedEngineer	= new Text(84, 17,  16, 80);
	_txtAllocated			= new Text(50, 17, 100, 80);

	_txtUnitToProduce		= new Text(84, 17, 176, 80);
	_txtTodo				= new Text(50, 17, 260, 80);

//	_txtEngineerUp			= new Text(100, 17, 32, 111);
//	_btnEngineerUp			= new ArrowButton(ARROW_BIG_UP, 14, 14, 145, 111);
//	_txtEngineerDown		= new Text(100, 17, 32, 135);
//	_btnEngineerDown		= new ArrowButton(ARROW_BIG_DOWN, 14, 14, 145, 135);
//	_txtUnitUp				= new Text(100, 17, 205, 111);
//	_btnUnitUp				= new ArrowButton(ARROW_BIG_UP, 14, 14, 280, 111);
//	_txtUnitDown			= new Text(100, 17, 205, 135);
//	_btnUnitDown			= new ArrowButton(ARROW_BIG_DOWN, 14, 14, 280, 135);
	_btnEngineerUp			= new ArrowButton(ARROW_BIG_UP,   100, 16,  30, 111);
	_btnEngineerDown		= new ArrowButton(ARROW_BIG_DOWN, 100, 16,  30, 135);
	_btnUnitUp				= new ArrowButton(ARROW_BIG_UP,   100, 16, 190, 111);
	_btnUnitDown			= new ArrowButton(ARROW_BIG_DOWN, 100, 16, 190, 135);

	_btnStop				= new TextButton(135, 16,  10, 159);
	_btnOk					= new TextButton(135, 16, 175, 159);

//	_surfaceEngineers = new InteractiveSurface(160, 150, 0, 25);
//	_surfaceEngineers->onMouseClick((ActionHandler)& ManufactureInfoState::handleWheelEngineer, 0);

//	_surfaceUnits = new InteractiveSurface(160, 150, 160, 25);
//	_surfaceUnits->onMouseClick((ActionHandler)& ManufactureInfoState::handleWheelUnit, 0);

	setInterface("manufactureInfo");

	add(_window,				"window",	"manufactureInfo");
	add(_txtTitle,				"text",		"manufactureInfo");
	add(_txtTimeDescr,			"text",		"manufactureInfo");
	add(_txtTimeTotal,			"text",		"manufactureInfo");
	add(_btnSell,				"button1",	"manufactureInfo");
	add(_txtAvailableEngineer,	"text",		"manufactureInfo");
	add(_txtAvailableSpace,		"text",		"manufactureInfo");
	add(_txtMonthlyProfit,		"text",		"manufactureInfo");
	add(_txtAllocatedEngineer,	"text",		"manufactureInfo");
	add(_txtAllocated,			"text",		"manufactureInfo");
	add(_txtUnitToProduce,		"text",		"manufactureInfo");
	add(_txtTodo,				"text",		"manufactureInfo");
//	add(_txtEngineerUp,			"text",		"manufactureInfo");
	add(_btnEngineerUp,			"button1",	"manufactureInfo");
//	add(_txtEngineerDown,		"text",		"manufactureInfo");
	add(_btnEngineerDown,		"button1",	"manufactureInfo");
//	add(_txtUnitUp,				"text",		"manufactureInfo");
	add(_btnUnitUp,				"button1",	"manufactureInfo");
//	add(_txtUnitDown,			"text",		"manufactureInfo");
	add(_btnUnitDown,			"button1",	"manufactureInfo");
	add(_btnStop,				"button2",	"manufactureInfo");
	add(_btnOk,					"button2",	"manufactureInfo");

//	add(_surfaceEngineers);
//	add(_surfaceUnits);

	centerAllSurfaces();


	_window->setBackground(_game->getResourcePack()->getSurface("BACK17.SCR"));

	_txtTitle->setText(tr(_manufRule != NULL ? _manufRule->getType() : _production->getRules()->getType()));
	_txtTitle->setBig();
	_txtTitle->setAlign(ALIGN_CENTER);

	_txtTimeDescr->setText(tr("STR_DAYS_HOURS_LEFT"));

	_btnSell->setText(tr("STR_SELL_PRODUCTION"));
	_btnSell->onMouseRelease((ActionHandler)& ManufactureInfoState::btnSellRelease);
//	_btnSell->onMouseClick((ActionHandler)& ManufactureInfoState::btnSellClick, 0);

	_txtAllocatedEngineer->setText(tr("STR_ENGINEERS_ALLOCATED"));
	_txtAllocatedEngineer->setBig();

	_txtAllocated->setBig();

	_txtTodo->setBig();

	_txtUnitToProduce->setText(tr("STR_UNITS_TO_PRODUCE"));
	_txtUnitToProduce->setBig();

//	_txtEngineerUp->setText(tr("STR_INCREASE_UC"));
//	_txtEngineerDown->setText(tr("STR_DECREASE_UC"));

	_btnEngineerUp->onMousePress((ActionHandler)& ManufactureInfoState::moreEngineerPress);
	_btnEngineerUp->onMouseRelease((ActionHandler)& ManufactureInfoState::moreEngineerRelease);
	_btnEngineerUp->onMouseClick((ActionHandler)& ManufactureInfoState::moreEngineerClick, 0);

	_btnEngineerDown->onMousePress((ActionHandler)& ManufactureInfoState::lessEngineerPress);
	_btnEngineerDown->onMouseRelease((ActionHandler)& ManufactureInfoState::lessEngineerRelease);
	_btnEngineerDown->onMouseClick((ActionHandler)& ManufactureInfoState::lessEngineerClick, 0);

//	_txtUnitUp->setText(tr("STR_INCREASE_UC"));
//	_txtUnitDown->setText(tr("STR_DECREASE_UC"));

	_btnUnitUp->onMousePress((ActionHandler)& ManufactureInfoState::moreUnitPress);
	_btnUnitUp->onMouseRelease((ActionHandler)& ManufactureInfoState::moreUnitRelease);
	_btnUnitUp->onMouseClick((ActionHandler)& ManufactureInfoState::moreUnitClick, 0);

	_btnUnitDown->onMousePress((ActionHandler)& ManufactureInfoState::lessUnitPress);
	_btnUnitDown->onMouseRelease((ActionHandler)& ManufactureInfoState::lessUnitRelease);
	_btnUnitDown->onMouseClick((ActionHandler)& ManufactureInfoState::lessUnitClick, 0);

	_btnStop->setText(tr("STR_STOP_PRODUCTION"));
	_btnStop->onMouseClick((ActionHandler)& ManufactureInfoState::btnStopClick);

	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)& ManufactureInfoState::btnOkClick);
	_btnOk->onKeyboardPress(
					(ActionHandler)& ManufactureInfoState::btnOkClick,
					Options::keyOk);
	_btnOk->onKeyboardPress(
					(ActionHandler)& ManufactureInfoState::btnOkClick,
					Options::keyOkKeypad);
	_btnOk->onKeyboardPress(
					(ActionHandler)& ManufactureInfoState::btnOkClick,
					Options::keyCancel);

	if (_production == NULL)
	{
		_btnOk->setVisible(false);
		_production = new Production(_manufRule, 0);
		_base->addProduction(_production);
	}

	_btnSell->setPressed(_production->getSellItems() == true);

	initProfit();
	setAssignedEngineer();


	_timerMoreEngineer = new Timer(Timer::SCROLL_SLOW);
	_timerLessEngineer = new Timer(Timer::SCROLL_SLOW);
	_timerMoreEngineer->onTimer((StateHandler)& ManufactureInfoState::onMoreEngineer);
	_timerLessEngineer->onTimer((StateHandler)& ManufactureInfoState::onLessEngineer);

	_timerMoreUnit = new Timer(Timer::SCROLL_SLOW);
	_timerLessUnit = new Timer(Timer::SCROLL_SLOW);
	_timerMoreUnit->onTimer((StateHandler)& ManufactureInfoState::onMoreUnit);
	_timerLessUnit->onTimer((StateHandler)& ManufactureInfoState::onLessUnit);
}

/**
 *
 */
void ManufactureInfoState::initProfit() // private.
{
	const Ruleset* const rules = _game->getRuleset();
	const RuleManufacture* const manufRule = _production->getRules();

	int sellValue;

	for (std::map<std::string, int>::const_iterator
			i = manufRule->getProducedItems().begin();
			i != manufRule->getProducedItems().end();
			++i)
	{
		if (manufRule->getCategory() == "STR_CRAFT")
			sellValue = rules->getCraft(i->first)->getSellCost();
		else
			sellValue = rules->getItem(i->first)->getSellCost();

		_producedItemsValue += sellValue * i->second;
	}
}

/**
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

	if (_btnSell->getPressed() == true)
		sellValue = _producedItemsValue;
	else
		sellValue = 0;

	if (_production->getInfiniteAmount() == true)
		qty = 1;
	else
		qty = _production->getAmountTotal() - _production->getAmountProduced();

	return qty * (sellValue - _production->getRules()->getManufactureCost());
}
/*	// does not take into account leap years
	static const int AVG_HOURS_PER_MONTH = (365 * 24) / 12;

	const RuleManufacture* const manufRule = _production->getRules();
	int
		sellValue = _btnSell->getPressed() ? _producedItemsValue : 0,
		numEngineers = _production->getAssignedEngineers(),
		manHoursPerMonth = AVG_HOURS_PER_MONTH * numEngineers;

	if (_production->getInfiniteAmount() == false)
	{
		// scale down to actual number of man hours required if the job takes less than one month
		const int manHoursRemaining = manufRule->getManufactureTime()
									* (_production->getAmountTotal() - _production->getAmountProduced());
		manHoursPerMonth = std::min(
								manHoursPerMonth,
								manHoursRemaining);
	}

	const int itemsPerMonth = static_cast<int>(static_cast<float>(manHoursPerMonth)
							/ static_cast<float>(manufRule->getManufactureTime()));
	return itemsPerMonth * (sellValue - manufRule->getManufactureCost()); */

/**
* Refreshes profit values.
* @param action - pointer to an Action
*/
/*void ManufactureInfoState::btnSellClick(Action*)
{
	setAssignedEngineer();
} */

/**
 * Handler for releasing the Sell button.
 * @param action - pointer to an Action
 */
void ManufactureInfoState::btnSellRelease(Action* action) // private.
{
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT
		|| action->getDetails()->button.button == SDL_BUTTON_RIGHT)
	{
		_production->setSellItems(_btnSell->getPressed() == true);
		setAssignedEngineer();
//		updateTimeTotal();
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
	if (_manufRule != NULL)
		_production->startProduction(
								_base,
								_game->getSavedGame());

	exitState();
}

/**
 * Returns to the previous screen.
 */
void ManufactureInfoState::exitState() // private.
{
	_game->popState();

	if (_manufRule != NULL)
		_game->popState();
}

/**
 * Helper function for setAssignedEngineer().
 * @param profit	- integer value ($$$) to format
 * @param woststr	- reference the output string
 * @return, true if profit else cost
 */
static bool _formatProfit( // static.
		int profit,
		std::wostringstream& woststr)
{
	float profit_f = static_cast<float>(profit);

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

//	woststr << (neg ? L"- " : L"+ ") << L"$" << std::fixed << std::setprecision(1) << profit_f << suffix;
	woststr << L"$" << std::fixed << std::setprecision(1) << profit_f << suffix;
	return ret;
}

/**
 * Updates display of assigned/available engineer/workshop space.
 */
void ManufactureInfoState::setAssignedEngineer() // private.
{
	_txtAvailableEngineer->setText(tr("STR_ENGINEERS_AVAILABLE_UC_").arg(_base->getEngineers()));
	_txtAvailableSpace->setText(tr("STR_WORKSHOP_SPACE_AVAILABLE_UC_").arg(_base->getFreeWorkshops()));

	std::wostringstream
		woststr1,
		woststr2,
		woststr3;

	woststr1 << L"> \x01" << _production->getAssignedEngineers();
	_txtAllocated->setText(woststr1.str());

	woststr2 << L"> \x01";
	if (_production->getInfiniteAmount() == true)
		woststr2 << L"oo";
	else
		woststr2 << _production->getAmountTotal();

	_txtTodo->setText(woststr2.str());

	std::string st;
	if (_formatProfit(
					calcProfit(),
					woststr3) == true)
	{
		st = "STR_NET_PROFIT_PER_MONTH_UC_";
	}
	else
		st = "STR_NET_COST_PER_MONTH_UC_";

	_txtMonthlyProfit->setText(tr(st)
						.arg(woststr3.str()));

	_btnOk->setVisible(_production->getAmountTotal() > 0
					|| _production->getInfiniteAmount() == true);

	updateTimeTotal();
}

/**
 * Updates the total time to complete the project.
 */
void ManufactureInfoState::updateTimeTotal() // private.
{
	std::wostringstream woststr;

	if (_production->getAssignedEngineers() > 0)
	{
		int hoursLeft;

		if (_production->getSellItems() == true
			|| _production->getInfiniteAmount() == true)
		{
			hoursLeft = (_production->getAmountProduced() + 1) * _production->getRules()->getManufactureTime()
						- _production->getTimeSpent();
		}
		else
			hoursLeft = _production->getAmountTotal() * _production->getRules()->getManufactureTime()
						- _production->getTimeSpent();


		int engs = _production->getAssignedEngineers();
		if (Options::canManufactureMoreItemsPerHour == false)
			engs = std::min(
						engs,
						_production->getRules()->getManufactureTime());

//		hoursLeft = static_cast<int>(ceil(
//					static_cast<double>(hoursLeft) / static_cast<double>(_production->getAssignedEngineers())));
		// Ensure this rounds up since it takes an entire hour to manufacture any part of that hour's capacity.
		hoursLeft = (hoursLeft + engs - 1) / engs;

		const int daysLeft = hoursLeft / 24;
		hoursLeft %= 24;
		woststr << daysLeft << L"\n" << hoursLeft;
	}
	else
		woststr << L"oo";

	_txtTimeTotal->setText(woststr.str());
}

/**
 * Adds given number of engineers to the project if possible.
 * @param change - how much to add
 */
void ManufactureInfoState::moreEngineer(int change) // private.
{
	if (change > 0)
	{
		const int
			availableEngineers = _base->getEngineers(),
			availableWorkSpace = _base->getFreeWorkshops();

		if (availableEngineers > 0
			&& availableWorkSpace > 0)
		{
			change = std::min(
							change,
							std::min(
									availableEngineers,
									availableWorkSpace));
			_production->setAssignedEngineers(_production->getAssignedEngineers() + change);
			_base->setEngineers(_base->getEngineers() - change);

			setAssignedEngineer();
		}
	}
}

/**
 * Starts the timerMoreEngineer.
 * @param action - pointer to an Action
 */
void ManufactureInfoState::moreEngineerPress(Action* action) // private.
{
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
		_timerMoreEngineer->start();
}

/**
 * Stops the timerMoreEngineer.
 * @param action - pointer to an Action
 */
void ManufactureInfoState::moreEngineerRelease(Action* action) // private.
{
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
	{
		_timerMoreEngineer->setInterval(Timer::SCROLL_SLOW);
		_timerMoreEngineer->stop();
	}
}

/**
 * Allocates all engineers.
 * @param action - pointer to an Action
 */
void ManufactureInfoState::moreEngineerClick(Action* action) // private.
{
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
		moreEngineer(getQty());
	else if (action->getDetails()->button.button == SDL_BUTTON_RIGHT)
		moreEngineer(std::numeric_limits<int>::max());
}

/**
 * Removes the given number of engineers from the project if possible.
 * @param change - how much to subtract
 */
void ManufactureInfoState::lessEngineer(int change) // private.
{
	if (change > 0)
	{
		const int assigned = _production->getAssignedEngineers();
		if (assigned > 0)
		{
			change = std::min(
							change,
							assigned);
			_production->setAssignedEngineers(assigned - change);
			_base->setEngineers(_base->getEngineers() + change);

			setAssignedEngineer();
		}
	}
}

/**
 * Starts the timerLessEngineer.
 * @param action - pointer to an Action
 */
void ManufactureInfoState::lessEngineerPress(Action* action) // private.
{
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
		_timerLessEngineer->start();
}

/**
 * Stops the timerLessEngineer.
 * @param action - pointer to an Action
 */
void ManufactureInfoState::lessEngineerRelease(Action* action) // private.
{
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
	{
		_timerLessEngineer->setInterval(Timer::SCROLL_SLOW);
		_timerLessEngineer->stop();
	}
}

/**
 * Removes engineers from the production.
 * @param action - pointer to an Action
 */
void ManufactureInfoState::lessEngineerClick(Action* action) // private.
{
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
		lessEngineer(getQty());
	else if (action->getDetails()->button.button == SDL_BUTTON_RIGHT)
		lessEngineer(std::numeric_limits<int>::max());
}

/**
 * Adds given number of units to produce to the project if possible.
 * @param change - how much to add
 */
void ManufactureInfoState::moreUnit(int change) // private.
{
	if (change > 0)
	{
		if (_production->getRules()->getCategory() == "STR_CRAFT"
			&& _base->getAvailableHangars() - _base->getUsedHangars() < 1)
		{
			_timerMoreUnit->stop();
			_game->pushState(new ErrorMessageState(
												tr("STR_NO_FREE_HANGARS_FOR_CRAFT_PRODUCTION"),
												_palette,
												_game->getRuleset()->getInterface("basescape")->getElement("errorMessage")->color,
												"BACK17.SCR",
												_game->getRuleset()->getInterface("basescape")->getElement("errorPalette")->color));
		}
		else
		{
			const int units = _production->getAmountTotal();
			change = std::min(
							change,
							std::numeric_limits<int>::max() - units);

			if (_production->getRules()->getCategory() == "STR_CRAFT")
				change = std::min(
								change,
								_base->getAvailableHangars() - _base->getUsedHangars());
			_production->setAmountTotal(units + change);

			setAssignedEngineer();
		}
	}
}

/**
 * Starts the timerMoreUnit.
 * @param action - pointer to an Action
 */
void ManufactureInfoState::moreUnitPress(Action* action) // private.
{
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT
		&& _production->getAmountTotal() < std::numeric_limits<int>::max())
	{
		_timerMoreUnit->start();
	}
}

/**
 * Stops the timerMoreUnit.
 * @param action - pointer to an Action
 */
void ManufactureInfoState::moreUnitRelease(Action* action) // private.
{
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
	{
		_timerMoreUnit->setInterval(Timer::SCROLL_SLOW);
		_timerMoreUnit->stop();
	}
}

/**
 * Increases the units to produce, in the case of a right-click, to infinite, and 1 on left-click.
 * @param action - pointer to an Action
 */
void ManufactureInfoState::moreUnitClick(Action* action) // private.
{
	if (_production->getInfiniteAmount() == false) // We can't increase over infinite :) [cf. Cantor]
	{
		if (action->getDetails()->button.button == SDL_BUTTON_RIGHT)
		{
			if (_production->getRules()->getCategory() == "STR_CRAFT")
			{
				moreUnit(std::numeric_limits<int>::max()); // kL_note: RMB won't start the timer ....
//				_game->pushState(new ErrorMessageState(
//													tr("STR_NO_FREE_HANGARS_FOR_CRAFT_PRODUCTION"),
//													_palette,
//													Palette::blockOffset(15)+1,
//													"BACK17.SCR",
//													6));
			}
			else
			{
				_production->setInfiniteAmount(true);
				setAssignedEngineer();
			}
		}
		else if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
			moreUnit(getQty());
	}
}

/**
 * Removes the given number of units to produce from the total if possible.
 * @param change - how many to subtract
 */
void ManufactureInfoState::lessUnit(int change) // private.
{
	if (change > 0)
	{
		const int units = _production->getAmountTotal();
		change = std::min(
						change,
						units - (_production->getAmountProduced() + 1));
		_production->setAmountTotal(units - change);

		setAssignedEngineer();
	}
}

/**
 * Starts the timerLessUnit.
 * @param action - pointer to an Action
 */
void ManufactureInfoState::lessUnitPress(Action* action) // private.
{
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
		_timerLessUnit->start();
}

/**
 * Stops the timerLessUnit.
 * @param action - pointer to an Action
 */
void ManufactureInfoState::lessUnitRelease(Action* action) // private.
{
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
	{
		_timerLessUnit->setInterval(Timer::SCROLL_SLOW);
		_timerLessUnit->stop();
	}
}

/**
 * Decreases the units to produce.
 * @param action - pointer to an Action
 */
void ManufactureInfoState::lessUnitClick(Action* action) // private.
{
	_production->setInfiniteAmount(false);

	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
		lessUnit(getQty());
	else if (action->getDetails()->button.button == SDL_BUTTON_RIGHT
		|| _production->getAmountTotal() <= _production->getAmountProduced())
	{
		_production->setAmountTotal(_production->getAmountProduced() + 1);
		setAssignedEngineer();
	}
}

/**
 * Assigns one more engineer if possible.
 */
void ManufactureInfoState::onMoreEngineer() // private.
{
	_timerMoreEngineer->setInterval(Timer::SCROLL_FAST);
	moreEngineer(getQty());
}

/**
 * Removes one engineer if possible.
 */
void ManufactureInfoState::onLessEngineer() // private.
{
	_timerLessEngineer->setInterval(Timer::SCROLL_FAST);
	lessEngineer(getQty());
}

/**
 * Increases or decreases the Engineers according the mouse-wheel used.
 * @param action - pointer to an Action
 */
/* void ManufactureInfoState::handleWheelEngineer(Action* action)
{
	if (action->getDetails()->button.button == SDL_BUTTON_WHEELUP)
		moreEngineer(Options::changeValueByMouseWheel);
	else if (action->getDetails()->button.button == SDL_BUTTON_WHEELDOWN)
		lessEngineer(Options::changeValueByMouseWheel);
} */

/**
 * Builds one more unit.
 */
void ManufactureInfoState::onMoreUnit() // private.
{
	_timerMoreUnit->setInterval(Timer::SCROLL_FAST);
	moreUnit(getQty());
}

/**
 * Builds one less unit.
 */
void ManufactureInfoState::onLessUnit() // private.
{
	_timerLessUnit->setInterval(Timer::SCROLL_FAST);
	lessUnit(getQty());
}

/**
 * Gets quantity to change by.
 * @note what were these guys smokin'
 * @return, 10 if CTRL is pressed else 1
 */
int ManufactureInfoState::getQty() const // private.
{
	if ((SDL_GetModState() & KMOD_CTRL) == 0)
		return 1;

	return 10;
}

/**
 * Increases or decreases the Units to produce according the mouse-wheel used.
 * @param action - pointer to an Action
 */
/* void ManufactureInfoState::handleWheelUnit(Action* action)
{
	if (action->getDetails()->button.button == SDL_BUTTON_WHEELUP)
		moreUnit(Options::changeValueByMouseWheel);
	else if (action->getDetails()->button.button == SDL_BUTTON_WHEELDOWN)
		lessUnit(Options::changeValueByMouseWheel);
} */

/**
 * Runs state functionality every cycle - updates the timer.
 */
void ManufactureInfoState::think() // private.
{
	State::think();

	_timerMoreEngineer->think(this, NULL);
	_timerLessEngineer->think(this, NULL);

	_timerMoreUnit->think(this, NULL);
	_timerLessUnit->think(this, NULL);
}

}
