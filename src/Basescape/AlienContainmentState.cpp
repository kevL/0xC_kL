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

#include "BasescapeState.h"
#include "MiniBaseView.h"

#include "../Engine/Action.h"
#include "../Engine/Game.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Options.h"
#include "../Engine/Timer.h"

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextList.h"
#include "../Interface/Window.h"

#include "../Resource/XcomResourcePack.h"

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
 * @param state		- pointer to the BasescapeState (default nullptr if
 *					  Battlescape-invoked or ResearchState-invoked)
 */
AlienContainmentState::AlienContainmentState(
		Base* const base,
		OptionsOrigin origin,
		BasescapeState* const state)
	:
		_base(base),
		_origin(origin),
		_state(state),
		_sel(0u),
		_fishFood(0),
		_totalSpace(0),
		_usedSpace(0),
		_baseList(_game->getSavedGame()->getBases())
{
	_window			= new Window(this, 320, 200);
	_mini			= new MiniBaseView(128, 16, 180, 27, MBV_CONTAINMENT);

	_txtTitle		= new Text(300, 17, 10, 10);
	_txtBaseLabel	= new Text( 80,  9, 16, 10);
	_txtHoverBase	= new Text( 80,  9, 224, 10);

	_txtSpace		= new Text(81, 9, 16, 30);
	_txtResearch	= new Text(80, 9, 97, 30);

	_txtItem		= new Text(138, 9,  16, 45);
	_txtLiveAliens	= new Text( 50, 9, 154, 45);
	_txtDeadAliens	= new Text( 50, 9, 204, 45);
	_txtInResearch	= new Text( 47, 9, 254, 45);

	_lstAliens		= new TextList(285, 121, 16, 55);

	_btnCancel		= new TextButton(134, 16,  16, 177);
	_btnOk			= new TextButton(134, 16, 170, 177);

	setInterface("manageContainment");

	add(_window,		"window",	"manageContainment");
	add(_mini,			"miniBase",	"basescape"); // <-
	add(_txtTitle,		"text",		"manageContainment");
	add(_txtBaseLabel,	"text",		"manageContainment");
	add(_txtHoverBase,	"numbers",	"baseInfo"); // <-
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


	_mini->setTexture(_game->getResourcePack()->getSurfaceSet("BASEBITS.PCK"));
	_mini->setBases(_baseList);
	for (size_t
			i = 0u;
			i != _baseList->size();
			++i)
	{
		if (_baseList->at(i) == _base)
		{
			_mini->setSelectedBase(i);
			break;
		}
	}
	_mini->onMouseClick(static_cast<ActionHandler>(&AlienContainmentState::miniClick),
						SDL_BUTTON_LEFT);
	_mini->onMouseOver(	static_cast<ActionHandler>(&AlienContainmentState::miniMouseOver));
	_mini->onMouseOut(	static_cast<ActionHandler>(&AlienContainmentState::miniMouseOut));

	_txtHoverBase->setAlign(ALIGN_RIGHT);

	std::string st;
	switch (_origin)
	{
		case OPT_BATTLESCAPE:
			_window->setBackground(_game->getResourcePack()->getSurface("BACK04.SCR"));
			st = "STR_LIQUIDATE";
			break;

		default:
		case OPT_MENU:
		case OPT_GEOSCAPE:
			_window->setBackground(_game->getResourcePack()->getSurface("BACK05.SCR"));
			st = "STR_REMOVE_SELECTED";
	}
	_btnOk->setText(tr(st));
	_btnOk->onMouseClick(	static_cast<ActionHandler>(&AlienContainmentState::btnOkClick));
	_btnOk->onKeyboardPress(static_cast<ActionHandler>(&AlienContainmentState::btnOkClick),
							Options::keyOk);
	_btnOk->onKeyboardPress(static_cast<ActionHandler>(&AlienContainmentState::btnOkClick),
							Options::keyOkKeypad);

	_btnCancel->setText(tr("STR_CANCEL"));
	_btnCancel->onMouseClick(	static_cast<ActionHandler>(&AlienContainmentState::btnCancelClick));
	_btnCancel->onKeyboardPress(static_cast<ActionHandler>(&AlienContainmentState::btnCancelClick),
								Options::keyCancel);

	_txtTitle->setText(tr("STR_MANAGE_CONTAINMENT"));
	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setBig();

	_txtItem->setText(tr("STR_ALIEN"));
	_txtLiveAliens->setText(tr("STR_LIVE_ALIENS"));
	_txtDeadAliens->setText(tr("STR_DEAD_ALIENS"));
	_txtInResearch->setText(tr("STR_RESEARCH"));

	_lstAliens->setColumns(4, 130,50,50,47);
	_lstAliens->setArrow(158, ARROW_HORIZONTAL);
	_lstAliens->setBackground(_window);
	_lstAliens->setSelectable();

	_lstAliens->onLeftArrowPress(	static_cast<ActionHandler>(&AlienContainmentState::lstLeftArrowPress));
	_lstAliens->onLeftArrowRelease(	static_cast<ActionHandler>(&AlienContainmentState::lstLeftArrowRelease));

	_lstAliens->onRightArrowPress(	static_cast<ActionHandler>(&AlienContainmentState::lstRightArrowPress));
	_lstAliens->onRightArrowRelease(static_cast<ActionHandler>(&AlienContainmentState::lstRightArrowRelease));


	_timerLeft = new Timer(Timer::SCROLL_SLOW);
	_timerLeft->onTimer(static_cast<StateHandler>(&AlienContainmentState::onLeft));

	_timerRight = new Timer(Timer::SCROLL_SLOW);
	_timerRight->onTimer(static_cast<StateHandler>(&AlienContainmentState::onRight));
}

/**
 * dTor.
 */
AlienContainmentState::~AlienContainmentState()
{
	delete _timerRight;
	delete _timerLeft;
}

/**
 * Updates the list of Live aLiens after clicking the MiniBase view.
 */
void AlienContainmentState::init()
{
	State::init();

	_txtBaseLabel->setText(_base->getName());

	_lstAliens->clearList();
	_qtysCorpsify.clear();
	_aliens.clear();

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
			&& itRule->isLiveAlien() == true)
		{
			interrogations.push_back(type);
		}
	}

	if (interrogations.empty() == true)
		_txtInResearch->setVisible(false);

	int qtyAliens;
	size_t r (0u);

	const std::vector<std::string>& allItems (_game->getRuleset()->getItemsList());
	for (std::vector<std::string>::const_iterator
			i = allItems.begin();
			i != allItems.end();
			++i)
	{
		itRule = _game->getRuleset()->getItemRule(*i);
		if (itRule->isLiveAlien() == true)
		{
			if ((qtyAliens = _base->getStorageItems()->getItemQuantity(*i)) != 0) // get Qty of each aLien-type in this Base's stores.
			{
				_qtysCorpsify.push_back(0);
				_aliens.push_back(*i);

				std::wstring rpQty;
				const std::vector<std::string>::const_iterator j (std::find(
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
					_lstAliens->setRowColor(r, YELLOW);

				++r;
			}
		}
	}

	for (std::vector<std::string>::const_iterator // add research aLiens that are not in Containment.
			i = interrogations.begin();
			i != interrogations.end();
			++i)
	{
		_qtysCorpsify.push_back(0);
		_aliens.push_back(*i);

		_lstAliens->addRow(
						4,
						tr(*i).c_str(),
						L"0",L"0",
						tr("STR_YES").c_str());
		_lstAliens->setRowColor(
							_qtysCorpsify.size() - 1u,
							_lstAliens->getSecondaryColor());
	}

	_fishFood = 0;
	_totalSpace = _base->getTotalContainment();
	_usedSpace = _base->getUsedContainment();
	_txtSpace->setText(tr("STR_SPACE_USED_SPACE_FREE_")
						.arg(_usedSpace)
						.arg(_totalSpace - _usedSpace));

	_txtResearch->setText(tr("STR_INTERROGATION_")
							.arg(_base->getInterrogatedAliens()));

	_btnOk->setVisible(false);

	if (_totalSpace < _usedSpace) // for post-tactical
		_btnCancel->setVisible(false);
}

/**
 * Deals with the selected aliens.
 * @param action - pointer to an Action
 */
void AlienContainmentState::btnOkClick(Action*)
{
	for (size_t
			i = 0u;
			i != _qtysCorpsify.size();
			++i)
	{
		if (_qtysCorpsify[i] != 0)
		{
			_base->getStorageItems()->removeItem(_aliens[i], _qtysCorpsify[i]);
			_base->getStorageItems()->addItem(
										_game->getRuleset()->getArmor(_game->getRuleset()->getUnitRule(_aliens[i])->getArmorType())->getCorpseGeoscape(),
										_qtysCorpsify[i]);
		}
	}

	if (_game->getQtyStates() == 2u // ie: (1) this, (2) Geoscape
		&& _game->getResourcePack()->isMusicPlaying(OpenXcom::res_MUSIC_TAC_AWARDS))
	{
		_game->getResourcePack()->fadeMusic(_game, 863);
//		_game->getResourcePack()->playMusic(OpenXcom::res_MUSIC_GEO_GLOBE);
	}
	_game->popState();
}

/**
 * Exits to the previous screen.
 * @param action - pointer to an Action
 */
void AlienContainmentState::btnCancelClick(Action*)
{
	_game->popState();
}

/**
 * Starts decreasing the alien count.
 * @param action - pointer to an Action
 */
void AlienContainmentState::lstLeftArrowPress(Action* action)
{
	_sel = _lstAliens->getSelectedRow();

	switch (action->getDetails()->button.button)
	{
		case SDL_BUTTON_LEFT:
			leftByValue(1);
			_timerLeft->setInterval(Timer::SCROLL_SLOW);
			_timerLeft->start();
			break;

		case SDL_BUTTON_RIGHT:
			leftByValue(std::numeric_limits<int>::max());
	}
}

/**
 * Stops decreasing the alien count.
 * @param action - pointer to an Action
 */
void AlienContainmentState::lstLeftArrowRelease(Action* action)
{
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
		_timerLeft->stop();
}

/**
 * Starts increasing the alien count.
 * @param action - pointer to an Action
 */
void AlienContainmentState::lstRightArrowPress(Action* action)
{
	_sel = _lstAliens->getSelectedRow();

	switch (action->getDetails()->button.button)
	{
		case SDL_BUTTON_LEFT:
			rightByValue(1);
			_timerRight->setInterval(Timer::SCROLL_SLOW);
			_timerRight->start();
			break;

		case SDL_BUTTON_RIGHT:
			rightByValue(std::numeric_limits<int>::max());
	}
}

/**
 * Stops increasing the alien count.
 * @param action - pointer to an Action
 */
void AlienContainmentState::lstRightArrowRelease(Action* action)
{
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
		_timerRight->stop();
}

/**
 * Runs the arrow timers.
 */
void AlienContainmentState::think()
{
	State::think();

	_timerRight->think(this, nullptr);
	_timerLeft->think(this, nullptr);
}

/**
 * Decreases the quantity of the selected alien to exterminate by Timer tick.
 */
void AlienContainmentState::onLeft()
{
	_timerLeft->setInterval(Timer::SCROLL_FAST);
	leftByValue(1);
}

/**
 * Decreases the quantity of the selected alien to exterminate.
 * @param delta - quantity to remove
 */
void AlienContainmentState::leftByValue(int delta)
{
	if (_qtysCorpsify[_sel] > 0)
	{
		delta = std::min(delta, _qtysCorpsify[_sel]);
		_qtysCorpsify[_sel] -= delta;
		_fishFood -= delta;
		updateListrow();
	}
}

/**
 * Increases the quantity of the selected alien to exterminate by Timer tick.
 */
void AlienContainmentState::onRight()
{
	_timerRight->setInterval(Timer::SCROLL_FAST);
	rightByValue(1);
}

/**
 * Increases the quantity of the selected alien to exterminate.
 * @param delta - quantity to add
 */
void AlienContainmentState::rightByValue(int delta)
{
	const int qtyType (getBaseQuantity() - _qtysCorpsify[_sel]);
	if (qtyType > 0)
	{
		delta = std::min(delta, qtyType);
		_qtysCorpsify[_sel] += delta;
		_fishFood += delta;
		updateListrow();
	}
}

/**
 * Gets the quantity of the currently selected alien at the Base.
 * @return, quantity of alien-type
 */
int AlienContainmentState::getBaseQuantity() // private.
{
	return _base->getStorageItems()->getItemQuantity(_aliens[_sel]);
}

/**
 * Updates the currently selected-row's quantity & color.
 * @note Also determines if the OK button should be in/visible.
 */
void AlienContainmentState::updateListrow() // private.
{
	Uint8 color;
	if (_qtysCorpsify[_sel] != 0)
		color = _lstAliens->getSecondaryColor();
	else if (_game->getSavedGame()->isResearched(_game->getRuleset()->getItemRule(_aliens[_sel])->getType()) == false)
		color = YELLOW;
	else
		color = _lstAliens->getColor();

	_lstAliens->setCellText(_sel, 1u, Text::intWide(getBaseQuantity() - _qtysCorpsify[_sel]));	// qty still in Containment
	_lstAliens->setCellText(_sel, 2u, Text::intWide(_qtysCorpsify[_sel]));						// qty to exterminate
	_lstAliens->setRowColor(_sel, color);


	const int freeSpace (_totalSpace - _usedSpace + _fishFood);
	_txtSpace->setText(tr("STR_SPACE_USED_SPACE_FREE_")
						.arg(_usedSpace - _fishFood)
						.arg(freeSpace));

	_btnOk->setVisible(_fishFood > 0 && freeSpace > -1);
	_btnCancel->setVisible(_totalSpace - _usedSpace > -1);
}

/**
 * Selects a different Base to display.
 * TODO: Implement key-presses to switch bases.
 * @param action - pointer to an Action
 */
void AlienContainmentState::miniClick(Action*)
{
	if (_state != nullptr) // cannot switch bases if origin is Battlescape/Debriefing OR Research state.
	{
		const size_t baseId (_mini->getHoveredBase());
		if (baseId < _baseList->size())
		{
			Base* const base (_baseList->at(baseId));
			if (base != _base && base->hasContainment() == true)
			{
				_txtHoverBase->setText(L"");

				_mini->setSelectedBase(baseId);
				_state->setBase(_base = base);

				_state->resetStoresWarning();
				init();
			}
		}
	}
}

/**
 * Displays the name of the Base the mouse is over.
 * @param action - pointer to an Action
 */
void AlienContainmentState::miniMouseOver(Action*)
{
	const size_t baseId (_mini->getHoveredBase());
	const Base* const base (_baseList->at(baseId));

	if (baseId < _baseList->size()
		&& base != _base
		&& base->hasContainment() == true)
	{
		_txtHoverBase->setText(base->getName().c_str());
	}
	else
		_txtHoverBase->setText(L"");
}

/**
 * Clears the hovered Base name.
 * @param action - pointer to an Action
 */
void AlienContainmentState::miniMouseOut(Action*)
{
	_txtHoverBase->setText(L"");
}

}
