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

#include "DebriefExtraState.h"

#include "../fmath.h"

#include "../Engine/Action.h"
#include "../Engine/Game.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Options.h"
#include "../Engine/Timer.h"

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextList.h"
#include "../Interface/Window.h"

#include "../Menu/ErrorMessageState.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/RuleInterface.h"
#include "../Ruleset/RuleItem.h"
#include "../Ruleset/Ruleset.h"

#include "../SaveGame/Base.h"
#include "../SaveGame/SavedGame.h"


namespace OpenXcom
{

/**
 * Creates the DebriefExtra state.
 * @param base			- pointer to the Base that was in tactical
 * @param operation		- the operation title
 * @param itemsLost		- a map of pointers to RuleItems & ints for lost items
 * @param itemsGained	- a map of pointers to RuleItems & ints for gained items
 */
DebriefExtraState::DebriefExtraState(
		Base* const base,
		std::wstring operation,
		std::map<const RuleItem*, int> itemsLost,
		std::map<const RuleItem*, int> itemsGained,
		std::map<std::wstring, std::vector<int>> solStatDeltas)
	:
		_base(base),
		_itemsLost(itemsLost),
		_itemsGained(itemsGained),
		_solStatDeltas(solStatDeltas),
		_curScreen(DES_SOL_STATS),
		_sel(0u),
		_costTotal(0),
		_storeSize(0.)
{
	_window			= new Window(this, 320, 200);

	_txtTitle		= new Text(280, 16,  16, 8);
	_txtBaseLabel	= new Text( 80,  9,  16, 8);
	_txtScreen		= new Text( 80,  9, 224, 8);

	_lstSolStats	= new TextList(285, 145, 16, 32);
	_lstGained		= new TextList(285, 145, 16, 32);
	_lstLost		= new TextList(285, 145, 16, 32);

	_btnOk			= new TextButton(136, 16, 92, 177);

	setInterface("debriefing");

	add(_window,		"window",	"debriefing");
	add(_txtTitle,		"heading",	"debriefing");
	add(_txtBaseLabel,	"text",		"debriefing");
	add(_txtScreen,		"text",		"debriefing");
	add(_lstSolStats,	"list",		"debriefing");
	add(_lstGained,		"list",		"debriefing");
	add(_lstLost,		"list",		"debriefing");
	add(_btnOk,			"button",	"debriefing");

	centerAllSurfaces();


	_window->setBackground(_game->getResourcePack()->getSurface("BACK01.SCR"));

	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)& DebriefExtraState::btnOkClick);
	_btnOk->onKeyboardPress(
					(ActionHandler)& DebriefExtraState::btnOkClick,
					Options::keyOk);
	_btnOk->onKeyboardPress(
					(ActionHandler)& DebriefExtraState::btnOkClick,
					Options::keyOkKeypad);
	_btnOk->onKeyboardPress(
					(ActionHandler)& DebriefExtraState::btnOkClick,
					Options::keyCancel);

	_txtTitle->setText(operation);
	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setBig();

	_txtBaseLabel->setText(_base->getName());

	_txtScreen->setText(L"stats");
	_txtScreen->setAlign(ALIGN_RIGHT);

	_lstSolStats->setColumns(12, 90,17,17,17,17,17,17,17,17,17,17,17);
	_lstSolStats->setBackground(_window);
	_lstSolStats->setSelectable();

	buildSoldierStats();


	_lstGained->setColumns(3, 192,54,30);
	_lstGained->setBackground(_window);
	_lstGained->setSelectable();
	_lstGained->setVisible(false);

	_lstGained->setArrowColumn(220, ARROW_VERTICAL);

	_lstGained->onLeftArrowPress(	(ActionHandler)& DebriefExtraState::lstLeftArrowPress);
	_lstGained->onLeftArrowRelease(	(ActionHandler)& DebriefExtraState::lstLeftArrowRelease);

	_lstGained->onRightArrowPress(	(ActionHandler)& DebriefExtraState::lstRightArrowPress);
	_lstGained->onRightArrowRelease((ActionHandler)& DebriefExtraState::lstRightArrowRelease);

	if (_itemsGained.empty() == false)
		styleList(_itemsGained, _lstGained);


	_lstLost->setColumns(3, 192,54,30);
	_lstLost->setBackground(_window);
	_lstLost->setSelectable();
	_lstLost->setVisible(false);

	_lstLost->setArrowColumn(220, ARROW_VERTICAL);

	_lstLost->onLeftArrowPress(		(ActionHandler)& DebriefExtraState::lstLeftArrowPress);
	_lstLost->onLeftArrowRelease(	(ActionHandler)& DebriefExtraState::lstLeftArrowRelease);

	_lstLost->onRightArrowPress(	(ActionHandler)& DebriefExtraState::lstRightArrowPress);
	_lstLost->onRightArrowRelease(	(ActionHandler)& DebriefExtraState::lstRightArrowRelease);

	if (_itemsLost.empty() == false)
		styleList(_itemsLost, _lstLost);


	_timerInc = new Timer(Timer::SCROLL_SLOW);
	_timerInc->onTimer((StateHandler)& DebriefExtraState::increase);

	_timerDec = new Timer(Timer::SCROLL_SLOW);
	_timerDec->onTimer((StateHandler)& DebriefExtraState::decrease);
}

/**
 * dTor.
 */
DebriefExtraState::~DebriefExtraState()
{
	delete _timerInc;
	delete _timerDec;
}

/**
 * Starts increasing the item.
 * @param action - pointer to an Action
 */
void DebriefExtraState::lstLeftArrowPress(Action* action) // private.
{
	switch (_curScreen)
	{
		case DES_LOOT_GAINED:
			_sel = _lstGained->getSelectedRow();
			break;
		case DES_LOOT_LOST:
			_sel = _lstLost->getSelectedRow();
	}

	switch (action->getDetails()->button.button)
	{
		case SDL_BUTTON_RIGHT:
			increaseByValue(std::numeric_limits<int>::max());
			break;

		case SDL_BUTTON_LEFT:
			if (_timerInc->isRunning() == false)
			{
				if ((SDL_GetModState() & KMOD_CTRL) != 0)
					increaseByValue(10);
				else
					increaseByValue(1);

				_timerInc->setInterval(Timer::SCROLL_SLOW);
				_timerInc->start();
			}
	}
}

/**
 * Stops increasing the item.
 * @param action - pointer to an Action
 */
void DebriefExtraState::lstLeftArrowRelease(Action* action) // private.
{
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
		_timerInc->stop();
}

/**
 * Starts decreasing the item.
 * @param action - pointer to an Action
 */
void DebriefExtraState::lstRightArrowPress(Action* action) // private.
{
	switch (_curScreen)
	{
		case DES_LOOT_GAINED:
			_sel = _lstGained->getSelectedRow();
			break;
		case DES_LOOT_LOST:
			_sel = _lstLost->getSelectedRow();
	}

	switch (action->getDetails()->button.button)
	{
		case SDL_BUTTON_RIGHT:
			decreaseByValue(std::numeric_limits<int>::max());
			break;

		case SDL_BUTTON_LEFT:
			if ((SDL_GetModState() & KMOD_CTRL) != 0)
				decreaseByValue(10);
			else
				decreaseByValue(1);

			_timerDec->setInterval(Timer::SCROLL_SLOW);
			_timerDec->start();
	}
}

/**
 * Stops decreasing the item.
 * @param action - pointer to an Action
 */
void DebriefExtraState::lstRightArrowRelease(Action* action) // private.
{
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
		_timerDec->stop();
}

/**
 * Increases the quantity of the selected item to buy by one.
 */
void DebriefExtraState::increase() // private.
{
	_timerInc->setInterval(Timer::SCROLL_FAST);

	if ((SDL_GetModState() & KMOD_CTRL) != 0)
		increaseByValue(10);
	else
		increaseByValue(1);
}

/**
 * Increases the quantity of the selected item to buy.
 * @param qtyDelta - how many to add
 */
void DebriefExtraState::increaseByValue(int qtyDelta) // private.
{
	if (qtyDelta > 0)
	{
		switch (_curScreen)
		{
			case DES_LOOT_GAINED:
			{
				const RuleItem* itRule (nullptr);
				size_t j (0u);
				for (std::map<const RuleItem*, int>::const_iterator
						i = _itemsGained.begin();
						i != _itemsGained.end();
						++i, ++j)
				{
					if (j == _sel)
					{
						itRule = i->first;
						break;
					}
				}

				if (itRule != nullptr && itRule->getSellCost() != 0
					&& _qtysSell[_sel] < _itemsGained[itRule])
				{
					qtyDelta = std::min(qtyDelta,
										_itemsGained[itRule] - _qtysSell[_sel]);

					_qtysSell[_sel] += qtyDelta;
					_costTotal += itRule->getSellCost() * qtyDelta;

					update();
				}
				break;
			}

			case DES_LOOT_LOST:
			{
				if (_error.empty() == false)
					_error.clear();
				else
				{
					const RuleItem* itRule (nullptr);
					size_t j (0u);
					for (std::map<const RuleItem*, int>::const_iterator
							i = _itemsLost.begin();
							i != _itemsLost.end();
							++i, ++j)
					{
						if (j == _sel)
						{
							itRule = i->first;
							break;
						}
					}

					if (itRule != nullptr && itRule->getBuyCost() != 0)
					{
						if (_costTotal + itRule->getBuyCost() > _game->getSavedGame()->getFunds())
							_error = tr("STR_NOT_ENOUGH_MONEY");
						else if (_storeSize + itRule->getStoreSize()
								> static_cast<double>(_base->getTotalStores()) - _base->getUsedStores() + 0.05)
						{
							_error = tr("STR_NOT_ENOUGH_STORE_SPACE");
						}

						if (_error.empty() == false)
						{
							_timerInc->stop();

							const RuleInterface* const uiRule (_game->getRuleset()->getInterface("debriefing"));
							_game->pushState(new ErrorMessageState(
																_error,
																_palette,
																uiRule->getElement("errorMessage")->color,
																"BACK13.SCR",
																uiRule->getElement("errorPalette")->color));
						}
						else
						{
							qtyDelta = std::min(qtyDelta,
											   (static_cast<int>(_game->getSavedGame()->getFunds()) - _costTotal) / itRule->getBuyCost()); // NOTE: (int)cast renders int64_t useless.

							const double storeSizePer (itRule->getStoreSize());
							double allowed;

							if (AreSame(storeSizePer, 0.) == false)
								allowed = (static_cast<double>(_base->getTotalStores()) - _base->getUsedStores() - _storeSize + 0.05) / storeSizePer;
							else
								allowed = std::numeric_limits<double>::max();

							qtyDelta = std::min(qtyDelta,
												static_cast<int>(allowed));
							_storeSize += static_cast<double>(qtyDelta) * storeSizePer;

							_qtysBuy[_sel] += qtyDelta;
							_costTotal += itRule->getBuyCost() * qtyDelta;

							update();
						}
					}
				}
			}
		}
	}
}

/**
 * Decreases the quantity of the selected item to buy by one.
 */
void DebriefExtraState::decrease() // private.
{
	_timerDec->setInterval(Timer::SCROLL_FAST);

	if ((SDL_GetModState() & KMOD_CTRL) != 0)
		decreaseByValue(10);
	else
		decreaseByValue(1);
}

/**
 * Decreases the quantity of the selected item to buy.
 * @param qtyDelta - how many to subtract
 */
void DebriefExtraState::decreaseByValue(int qtyDelta) // private.
{
	if (qtyDelta > 0)
	{
		switch (_curScreen)
		{
			case DES_LOOT_GAINED:
			{
				if (_qtysSell[_sel] > 0)
				{
					const RuleItem* itRule (nullptr);
					size_t j (0u);
					for (std::map<const RuleItem*, int>::const_iterator
							i = _itemsGained.begin();
							i != _itemsGained.end();
							++i, ++j)
					{
						if (j == _sel)
						{
							itRule = i->first;
							break;
						}
					}

					if (itRule != nullptr) // safety.
					{
						qtyDelta = std::min(qtyDelta, _qtysSell[_sel]);

						_qtysSell[_sel] -= qtyDelta;
						_costTotal -= itRule->getSellCost() * qtyDelta;

						update();
					}
				}
				break;
			}

			case DES_LOOT_LOST:
			{
				if (_qtysBuy[_sel] > 0)
				{
					const RuleItem* itRule (nullptr);
					size_t j (0u);
					for (std::map<const RuleItem*, int>::const_iterator
							i = _itemsLost.begin();
							i != _itemsLost.end();
							++i, ++j)
					{
						if (j == _sel)
						{
							itRule = i->first;
							break;
						}
					}

					if (itRule != nullptr) // safety.
					{
						qtyDelta = std::min(qtyDelta, _qtysBuy[_sel]);

						_storeSize -= itRule->getStoreSize() * static_cast<double>(qtyDelta);

						_qtysBuy[_sel] -= qtyDelta;
						_costTotal -= itRule->getBuyCost() * qtyDelta;

						update();
					}
				}
			}
		}
	}
}

/**
 * Updates the buy/sell quantities.
 */
void DebriefExtraState::update() // private.
{
	switch (_curScreen)
	{
		case DES_LOOT_GAINED:
			_lstGained->setCellText(_sel, 2u, Text::intWide(_qtysSell[_sel]));
			break;

		case DES_LOOT_LOST:
			_lstLost->setCellText(_sel, 2u, Text::intWide(_qtysBuy[_sel]));
	}
}

/**
 * Runs the arrow timers.
 */
void DebriefExtraState::think() // private.
{
	State::think();

	_timerInc->think(this, nullptr);
	_timerDec->think(this, nullptr);
}

/**
 * Returns to the previous screen.
 * @param action - pointer to an Action
 */
void DebriefExtraState::btnOkClick(Action*)
{
	switch (_curScreen)
	{
		case DES_SOL_STATS:
			_lstSolStats->setVisible(false);
			if (_itemsGained.empty() == false)
			{
				_curScreen = DES_LOOT_GAINED;
				_txtScreen->setText(L"loot");
//				_lstGained->scrollTo();
				_lstGained->setVisible();
				break;
			} // no break;

		case DES_LOOT_GAINED:
			_lstGained->setVisible(false);
			if (_itemsLost.empty() == false)
			{
				_curScreen = DES_LOOT_LOST;
				_txtScreen->setText(L"lost");
//				_lstLost->scrollTo();
				_lstLost->setVisible();
				break;
			} // no break;

		case DES_LOOT_LOST:
			_game->popState();
	}
}

/**
 * Builds the soldier-stat-changes screen.
 */
void DebriefExtraState::buildSoldierStats() // private.
{
	_lstSolStats->addRow( // vid. BattleUnit::postMissionProcedures().
					12,
					L"",
					L"BR",
					L"FR",
					L"RA",
					L"ML",
					L"PA",
					L"PD",
					L"TR",
					L"TU",
					L"HL",
					L"ST",
					L"EN");

	size_t row (1u);
	for (std::map<std::wstring, std::vector<int>>::const_iterator
			i = _solStatDeltas.begin();
			i != _solStatDeltas.end();
			++i, ++row)
	{
		_lstSolStats->addRow(
						12,
						i->first.c_str(),
						i->second[ 0u] ? Text::intWide(i->second[ 0u]).c_str() : L"",
						i->second[ 1u] ? Text::intWide(i->second[ 1u]).c_str() : L"",
						i->second[ 2u] ? Text::intWide(i->second[ 2u]).c_str() : L"",
						i->second[ 3u] ? Text::intWide(i->second[ 3u]).c_str() : L"",
						i->second[ 4u] ? Text::intWide(i->second[ 4u]).c_str() : L"",
						i->second[ 5u] ? Text::intWide(i->second[ 5u]).c_str() : L"",
						i->second[ 6u] ? Text::intWide(i->second[ 6u]).c_str() : L"",
						i->second[ 7u] ? Text::intWide(i->second[ 7u]).c_str() : L"",
						i->second[ 8u] ? Text::intWide(i->second[ 8u]).c_str() : L"",
						i->second[ 9u] ? Text::intWide(i->second[ 9u]).c_str() : L"",
						i->second[10u] ? Text::intWide(i->second[10u]).c_str() : L"");
		_lstSolStats->setRowColor(row, YELLOW);
	}
}

/**
 * Formats mapped input to a TextList.
 * @param input	- reference to the mapped-input of pointers-to-RuleItems & quantities
 * @param list	- pointer to a 3-column TextList to format w/ data
 */
void DebriefExtraState::styleList( // private.
		const std::map<const RuleItem*, int>& input,
		TextList* const list)
{
	std::string type;
	std::wstring
		wst1, // type
		wst2; // quantity to buy/sell
	Uint8 color;
	bool contrast;
	size_t row (0u);

	for (std::map<const RuleItem*, int>::const_iterator
			i = input.begin();
			i != input.end();
			++i, ++row)
	{
		type = i->first->getType();

		if (list == _lstGained)
		{
			_typesGained.push_back(type);
			_qtysSell.push_back(0);
		}
		else
		{
			_typesLost.push_back(type);
			_qtysBuy.push_back(0);
		}


		wst1 = tr(type);

		if (i->first->isLiveAlien() == true)
		{
			color = GRAY;
			contrast = true;
		}
		else if (_game->getSavedGame()->isResearched(type) == false							// not researched or is research exempt
			&& (_game->getSavedGame()->isResearched(i->first->getRequirements()) == false	// and has requirements to use that have not been researched
				|| i->first->getBattleType() == BT_CORPSE))										// or is a corpse
		{
			color = GREEN;
			contrast = false;

			if (i->first->getBattleType() == BT_AMMO)
				wst1.insert(0, L"  ");
		}
		else if (i->first->getBattleType() == BT_AMMO)
		{
			color = BROWN;
			contrast = true;
			wst1.insert(0, L"  ");
		}
		else
		{
			color = YELLOW;
			contrast = false;
		}

		if ((list == _lstGained && i->first->getSellCost() != 0)
			|| (list == _lstLost && i->first->getBuyCost() != 0))
		{
			wst2 = Text::intWide(0);
		}
		else
			wst2 = L"-";

		list->addRow(
				3,
				wst1.c_str(),
				Text::intWide(i->second).c_str(),
				wst2.c_str());
		list->setRowColor(row, color, contrast);
	}
}

}
