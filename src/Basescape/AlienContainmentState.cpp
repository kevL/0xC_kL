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

#include "AlienContainmentState.h"

//#include <algorithm>
//#include <climits>
//#include <sstream>

#include "SellState.h"

#include "../Engine/Action.h"
#include "../Engine/Game.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Options.h"
#include "../Engine/Timer.h"

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextList.h"
#include "../Interface/Window.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/RuleArmor.h"
#include "../Ruleset/RuleItem.h"
#include "../Ruleset/RuleResearch.h"
#include "../Ruleset/Ruleset.h"

#include "../Savegame/Base.h"
#include "../Savegame/ItemContainer.h"
#include "../Savegame/ResearchProject.h"
#include "../Savegame/SavedGame.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the AlienContainment screen.
 * @param base		- pointer to the Base to get info from
 * @param origin	- game section that originated this state
 */
AlienContainmentState::AlienContainmentState(
		Base* const base,
		OptionsOrigin origin)
	:
		_base(base),
		_origin(origin),
		_sel(0),
		_fishFood(0),
		_totalSpace(base->getTotalContainment()),
		_usedSpace(base->getUsedContainment())
{
	_window			= new Window(this, 320, 200);

	_txtTitle		= new Text(300, 17, 10, 10);
	_txtBaseLabel	= new Text( 80,  9, 16, 10);

	_txtSpace		= new Text(144, 9,  16, 30);
	_txtResearch	= new Text(144, 9, 154, 30);

	_txtItem		= new Text(138, 9,  16, 40);
	_txtLiveAliens	= new Text( 50, 9, 154, 40);
	_txtDeadAliens	= new Text( 50, 9, 204, 40);
	_txtInResearch	= new Text( 47, 9, 254, 40);

	_lstAliens		= new TextList(285, 121, 16, 50);

	_btnCancel		= new TextButton(134, 16,  16, 177);
	_btnOk			= new TextButton(134, 16, 170, 177);

	setInterface("manageContainment");

	add(_window,		"window",	"manageContainment");
	add(_txtTitle,		"text",		"manageContainment");
	add(_txtBaseLabel,	"text",		"manageContainment");
	add(_txtSpace,		"text",		"manageContainment");
	add(_txtResearch,	"text",		"manageContainment");
	add(_txtItem,		"text",		"manageContainment");
	add(_txtLiveAliens,	"text",		"manageContainment");
	add(_txtDeadAliens,	"text",		"manageContainment");
	add(_txtInResearch,	"text",		"manageContainment");
	add(_lstAliens,		"list",		"manageContainment");
	add(_btnCancel,		"button",	"manageContainment");
	add(_btnOk,			"button",	"manageContainment");

	centerAllSurfaces();


	std::string st;
	if (_origin == OPT_BATTLESCAPE)
	{
		_window->setBackground(_game->getResourcePack()->getSurface("BACK04.SCR"));
		st = "STR_LIQUIDATE";
	}
	else
	{
		_window->setBackground(_game->getResourcePack()->getSurface("BACK05.SCR"));
		st = "STR_REMOVE_SELECTED";
	}
	_btnOk->setText(tr(st));
	_btnOk->onMouseClick((ActionHandler)& AlienContainmentState::btnOkClick);
	_btnOk->onKeyboardPress(
					(ActionHandler)& AlienContainmentState::btnOkClick,
					Options::keyOk);
	_btnOk->onKeyboardPress(
					(ActionHandler)& AlienContainmentState::btnOkClick,
					Options::keyOkKeypad);
	_btnOk->setVisible(false);

	_btnCancel->setText(tr("STR_CANCEL"));
	_btnCancel->onMouseClick((ActionHandler)& AlienContainmentState::btnCancelClick);
	_btnCancel->onKeyboardPress(
					(ActionHandler)& AlienContainmentState::btnCancelClick,
					Options::keyCancel);
	if (_totalSpace < _usedSpace)
		_btnCancel->setVisible(false);

	_txtTitle->setText(tr("STR_MANAGE_CONTAINMENT"));
	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setBig();

	_txtBaseLabel->setText(_base->getName());

	_txtSpace->setText(tr("STR_SPACE_USED_SPACE_FREE_")
						.arg(_usedSpace)
						.arg(_totalSpace - _usedSpace));

	_txtResearch->setText(tr("STR_INTERROGATION_")
							.arg(_base->getInterrogatedAliens()));

	_txtItem->setText(tr("STR_ALIEN"));
	_txtLiveAliens->setText(tr("STR_LIVE_ALIENS"));
	_txtDeadAliens->setText(tr("STR_DEAD_ALIENS"));
	_txtInResearch->setText(tr("STR_RESEARCH"));

	_lstAliens->setColumns(4, 130,50,50,47);
	_lstAliens->setArrowColumn(158, ARROW_HORIZONTAL);
	_lstAliens->setBackground(_window);
	_lstAliens->setSelectable();
//	_lstAliens->setAllowScrollOnArrowButtons(!_allowChangeListValuesByMouseWheel);
//	_lstAliens->onMousePress((ActionHandler)& AlienContainmentState::lstItemsMousePress);
	_lstAliens->onLeftArrowPress((ActionHandler)& AlienContainmentState::lstItemsLeftArrowPress);
	_lstAliens->onLeftArrowRelease((ActionHandler)& AlienContainmentState::lstItemsLeftArrowRelease);
	_lstAliens->onLeftArrowClick((ActionHandler)& AlienContainmentState::lstItemsLeftArrowClick);
	_lstAliens->onRightArrowPress((ActionHandler)& AlienContainmentState::lstItemsRightArrowPress);
	_lstAliens->onRightArrowRelease((ActionHandler)& AlienContainmentState::lstItemsRightArrowRelease);
	_lstAliens->onRightArrowClick((ActionHandler)& AlienContainmentState::lstItemsRightArrowClick);

	const RuleItem* itRule;
	std::string type;

	std::vector<std::string> interrogations;
	for (std::vector<ResearchProject*>::const_iterator
			i = _base->getResearch().begin();
			i != _base->getResearch().end();
			++i)
	{
		type = (*i)->getRules()->getType();
		if ((itRule = _game->getRuleset()->getItemRule(type)) != nullptr
			&& itRule->isAlien() == true)
		{
			interrogations.push_back(type);
		}
	}

	if (interrogations.empty() == true)
		_txtInResearch->setVisible(false);

	int qtyAliens;
	size_t row (0);

	const std::vector<std::string>& allItems (_game->getRuleset()->getItemsList());
	for (std::vector<std::string>::const_iterator
			i = allItems.begin();
			i != allItems.end();
			++i)
	{
		itRule = _game->getRuleset()->getItemRule(*i);
		if (itRule->isAlien() == true) // it's a live alien...
		{
			qtyAliens = _base->getStorageItems()->getItemQuantity(*i); // get Qty of each aLien-type at this base
			if (qtyAliens != 0)
			{
				_qty.push_back(0);		// put it in the _qty <vector> as (int)
				_aliens.push_back(*i);	// put its name in the _aliens <vector> as (string)

				std::wstring rpQty;
				std::vector<std::string>::const_iterator j (std::find(
																	interrogations.begin(),
																	interrogations.end(),
																	*i));
				if (j != interrogations.end())
				{
					rpQty = tr("STR_YES");
					interrogations.erase(j);
				}

				_lstAliens->addRow( // show its name on the list.
								4,
								tr(*i).c_str(),
								Text::intWide(qtyAliens).c_str(),
								L"0",
								rpQty.c_str());

				if (_game->getSavedGame()->isResearched(itRule->getType()) == false)
					_lstAliens->setRowColor(row, YELLOW);

				++row;
			}
		}
	}

	for (std::vector<std::string>::const_iterator // add research aLiens that are not in Containment.
			i = interrogations.begin();
			i != interrogations.end();
			++i)
	{
		_qty.push_back(0);
		_aliens.push_back(*i);

		_lstAliens->addRow(
						4,
						tr(*i).c_str(),
						L"0",L"0",
						tr("STR_YES").c_str());
		_lstAliens->setRowColor(
							_qty.size() - 1,
							_lstAliens->getSecondaryColor());
	}

	_timerInc = new Timer(Timer::SCROLL_SLOW);
	_timerInc->onTimer((StateHandler)& AlienContainmentState::increase);

	_timerDec = new Timer(Timer::SCROLL_SLOW);
	_timerDec->onTimer((StateHandler)& AlienContainmentState::decrease);
}

/**
 * dTor.
 */
AlienContainmentState::~AlienContainmentState()
{
	delete _timerInc;
	delete _timerDec;
}

/**
 * Runs the arrow timers.
 */
void AlienContainmentState::think()
{
	State::think();

	_timerInc->think(this, nullptr);
	_timerDec->think(this, nullptr);
}

/**
 * Deals with the selected aliens.
 * @param action - pointer to an Action
 */
void AlienContainmentState::btnOkClick(Action*)
{
	for (size_t
			i = 0;
			i != _qty.size();
			++i)
	{
		if (_qty[i] > 0)
		{
			_base->getStorageItems()->removeItem(_aliens[i], _qty[i]);
			_base->getStorageItems()->addItem(
									_game->getRuleset()->getArmor(_game->getRuleset()->getUnitRule(_aliens[i])->getArmorType())->getCorpseGeoscape(),
									_qty[i]);
		}
	}
	_game->popState();
}

/**
 * Returns to the previous screen.
 * @param action - pointer to an Action
 */
void AlienContainmentState::btnCancelClick(Action*)
{
	_game->popState();
}

/**
 * Starts increasing the alien count.
 * @param action - pointer to an Action
 */
void AlienContainmentState::lstItemsRightArrowPress(Action* action)
{
	_sel = _lstAliens->getSelectedRow();

	if (action->getDetails()->button.button == SDL_BUTTON_LEFT
		&& _timerInc->isRunning() == false)
	{
		_timerInc->start();
	}
}

/**
 * Stops increasing the alien count.
 * @param action - pointer to an Action
 */
void AlienContainmentState::lstItemsRightArrowRelease(Action* action)
{
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
		_timerInc->stop();
}

/**
 * Increases the selected alien count;
 * by one on left-click, to max on right-click.
 * @param action - pointer to an Action
 */
void AlienContainmentState::lstItemsRightArrowClick(Action* action)
{
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
	{
		increaseByValue(1);

		_timerInc->setInterval(Timer::SCROLL_SLOW);
		_timerDec->setInterval(Timer::SCROLL_SLOW);
	}
	else if (action->getDetails()->button.button == SDL_BUTTON_RIGHT)
		increaseByValue(std::numeric_limits<int>::max());
}

/**
 * Starts decreasing the alien count.
 * @param action - pointer to an Action
 */
void AlienContainmentState::lstItemsLeftArrowPress(Action* action)
{
	_sel = _lstAliens->getSelectedRow();

	if (action->getDetails()->button.button == SDL_BUTTON_LEFT
		&& _timerDec->isRunning() == false)
	{
		_timerDec->start();
	}
}

/**
 * Stops decreasing the alien count.
 * @param action - pointer to an Action
 */
void AlienContainmentState::lstItemsLeftArrowRelease(Action* action)
{
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
		_timerDec->stop();
}

/**
 * Decreases the selected alien count;
 * by one on left-click, to 0 on right-click.
 * @param action - pointer to an Action
 */
void AlienContainmentState::lstItemsLeftArrowClick(Action* action)
{
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
	{
		decreaseByValue(1);

		_timerInc->setInterval(Timer::SCROLL_SLOW);
		_timerDec->setInterval(Timer::SCROLL_SLOW);
	}
	else if (action->getDetails()->button.button == SDL_BUTTON_RIGHT)
		decreaseByValue(std::numeric_limits<int>::max());
}

/**
 * Gets the quantity of the currently selected alien on the base.
 * @return, quantity of alien
 */
int AlienContainmentState::getQuantity()
{
	return _base->getStorageItems()->getItemQuantity(_aliens[_sel]);
}

/**
 * Increases the quantity of the selected alien to exterminate by one.
 */
void AlienContainmentState::increase()
{
	_timerDec->setInterval(Timer::SCROLL_FAST);
	_timerInc->setInterval(Timer::SCROLL_FAST);

	increaseByValue(1);
}

/**
 * Increases the quantity of the selected alien to exterminate.
 * @param change - how much to add
 */
void AlienContainmentState::increaseByValue(int change)
{
	if (change > 0)
	{
		const int qtyType (getQuantity() - _qty[_sel]);
		if (qtyType > 0)
		{
			change = std::min(change, qtyType);
			_qty[_sel] += change;
			_fishFood += change;

			update();
		}
	}
}

/**
 * Decreases the quantity of the selected alien to exterminate by one.
 */
void AlienContainmentState::decrease()
{
	_timerInc->setInterval(Timer::SCROLL_FAST);
	_timerDec->setInterval(Timer::SCROLL_FAST);

	decreaseByValue(1);
}

/**
 * Decreases the quantity of the selected alien to exterminate.
 * @param change - how much to remove
 */
void AlienContainmentState::decreaseByValue(int change)
{
	if (change > 0 && _qty[_sel] > 0)
	{
		change = std::min(change, _qty[_sel]);
		_qty[_sel] -= change;
		_fishFood -= change;

		update();
	}
}

/**
 * Updates the row (quantity & color) of the selected aLien species.
 * @note Also determines if the OK button should be in/visible.
 */
void AlienContainmentState::update() // private.
{
	Uint8 color;
	if (_qty[_sel] != 0)
		color = _lstAliens->getSecondaryColor();
	else if (_game->getSavedGame()->isResearched(_game->getRuleset()->getItemRule(_aliens[_sel])->getType()) == false)
		color = YELLOW;
	else
		color = _lstAliens->getColor();

	_lstAliens->setCellText(_sel, 1, Text::intWide(getQuantity() - _qty[_sel]));	// qty still in Containment
	_lstAliens->setCellText(_sel, 2, Text::intWide(_qty[_sel]));					// qty to torture
	_lstAliens->setRowColor(_sel, color);


	const int freeSpace (_totalSpace - _usedSpace + _fishFood);
	_txtSpace->setText(tr("STR_SPACE_USED_SPACE_FREE_")
						.arg(_usedSpace - _fishFood)
						.arg(freeSpace));

	_btnOk->setVisible(_fishFood > 0 && freeSpace > -1);
	_btnCancel->setVisible(_totalSpace - _usedSpace > -1);
}

/**
 * Handles the mouse-wheels on the arrow-buttons.
 * @param action - pointer to an Action
 *
void AlienContainmentState::lstItemsMousePress(Action* action)
{
	_sel = _lstAliens->getSelectedRow();

	if (action->getDetails()->button.button == SDL_BUTTON_WHEELUP)
	{
		_timerInc->stop();
		_timerDec->stop();

		if (action->getAbsoluteXMouse() >= _lstAliens->getArrowsLeftEdge()
			&& action->getAbsoluteXMouse() <= _lstAliens->getArrowsRightEdge())
		{
			increaseByValue(Options::changeValueByMouseWheel);
		}
	}
	else if (action->getDetails()->button.button == SDL_BUTTON_WHEELDOWN)
	{
		_timerInc->stop();
		_timerDec->stop();

		if (action->getAbsoluteXMouse() >= _lstAliens->getArrowsLeftEdge()
			&& action->getAbsoluteXMouse() <= _lstAliens->getArrowsRightEdge())
		{
			decreaseByValue(Options::changeValueByMouseWheel);
		}
	}
} */

}
