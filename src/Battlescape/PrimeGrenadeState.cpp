/*
 * Copyright 2010-2017 OpenXcom Developers.
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

#include "PrimeGrenadeState.h"

//#include <sstream>

#include "Inventory.h"

#include "../Engine/Action.h"
#include "../Engine/Game.h"
#include "../Engine/InteractiveSurface.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Options.h"

#include "../Interface/Frame.h"
#include "../Interface/Text.h"

#include "../Ruleset/RuleInterface.h"
#include "../Ruleset/Ruleset.h"

#include "../Savegame/BattleItem.h"
#include "../Savegame/SavedBattleGame.h"
#include "../Savegame/SavedGame.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the PrimeGrenade window.
 * @param action			- pointer to the BattleAction (BattlescapeGame.h)
 * @param inInventoryView	- true if called from inventory
 * @param grenade			- pointer to associated grenade
 * @param inventory			- pointer to Inventory (default nullptr)
 */
PrimeGrenadeState::PrimeGrenadeState(
		BattleAction* action,
		bool inInventoryView,
		BattleItem* grenade,
		Inventory* inventory)
	:
		_action(action),
		_inInventoryView(inInventoryView),
		_grenade(grenade),
		_inventory(inventory)
{
	_fullScreen = false;

	_fraTop		= new Frame(192, 27, 65, 37);
	_txtTitle	= new Text(192, 18, 65, 43);
	_srfBG		= new Surface(192, 93, 65, 45);
	_isfBtn0	= new InteractiveSurface(190, 22, 66, 65);

	const int
		x = 67,
		y = 92;

	for (size_t
			i = 0u;
			i != 16u;
			++i)
	{
		_isfBtn[i] = new InteractiveSurface(
										22,22,
										x - 1 + ((static_cast<int>(i) % 8) * 24),
										y - 4 + ((static_cast<int>(i) / 8) * 24));
		_txtTurn[i] = new Text(
							20,20,
							x + ((static_cast<int>(i) % 8) * 24),
							y + ((static_cast<int>(i) / 8) * 24) - 3);
	}

	setPalette(PAL_BATTLESCAPE);

	const Element* const el (_game->getRuleset()->getInterface("battlescape")->getElement("grenadeBackground"));

	add(_srfBG);
	_srfBG->drawRect(
					0,0,
					static_cast<Sint16>(_srfBG->getWidth()),
					static_cast<Sint16>(_srfBG->getHeight()),
					static_cast<Uint8>(el->color));

	add(_fraTop, "grenadeMenu", "battlescape");
	_fraTop->setSecondaryColor(BLUE);
	_fraTop->setThickness(3);
	_fraTop->setHighContrast();

	add(_txtTitle, "grenadeMenu", "battlescape");
	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setBig();
	_txtTitle->setText(tr("STR_SET_TIMER"));
	_txtTitle->setHighContrast();

	add(_isfBtn0);
	_isfBtn0->drawRect(
					0,0,
					static_cast<Sint16>(_isfBtn0->getWidth()),
					static_cast<Sint16>(_isfBtn0->getHeight()),
					BLACK);
	_isfBtn0->drawRect(
					1,1,
					static_cast<Sint16>(_isfBtn0->getWidth() - 2),
					static_cast<Sint16>(_isfBtn0->getHeight() - 2),
					ORANGE);

	if (Options::battleInstantGrenade == true)
	{
		_isfBtn0->onMouseClick(static_cast<ActionHandler>(&PrimeGrenadeState::btnClick));

		_txtTurn0 = new Text(190, 18, 66, 67);
		add(_txtTurn0);

		_txtTurn0->setText(L"0");
		_txtTurn0->setBig();
		_txtTurn0->setColor(BLACK);
		_txtTurn0->setHighContrast();
		_txtTurn0->setAlign(ALIGN_CENTER);
		_txtTurn0->setVerticalAlign(ALIGN_MIDDLE);
	}

	for (size_t
			i = 0u;
			i != 16u;
			++i)
	{
		add(_isfBtn[i]);
		_isfBtn[i]->onMouseClick(static_cast<ActionHandler>(&PrimeGrenadeState::btnClick));

		SDL_Rect rect;

		rect.x = // dark border
		rect.y = 0;
		rect.w = static_cast<Uint16>(_isfBtn[i]->getWidth());
		rect.h = static_cast<Uint16>(_isfBtn[i]->getHeight());
		_isfBtn[i]->drawRect(
						&rect,
						static_cast<Uint8>(el->border));

		++rect.x; // inside fill
		++rect.y;
		rect.w = static_cast<Uint16>(rect.w - 2u);
		rect.h = static_cast<Uint16>(rect.h - 2u);
		_isfBtn[i]->drawRect(
						&rect,
						static_cast<Uint8>(el->color2));

		add(_txtTurn[i], "grenadeMenu", "battlescape");
		_txtTurn[i]->setText(Text::intWide(static_cast<int>(i) + 1));
		_txtTurn[i]->setHighContrast();
		_txtTurn[i]->setAlign(ALIGN_CENTER);
		_txtTurn[i]->setVerticalAlign(ALIGN_MIDDLE);
	}

	centerSurfaces();
	lowerSurfaces();
}

/**
 * Deletes the Prime Grenade window object.
 */
PrimeGrenadeState::~PrimeGrenadeState()
{}

/**
 * Closes the window on right-click.
 * @param action - pointer to an Action
 */
void PrimeGrenadeState::handle(Action* action)
{
	State::handle(action);

	if (action->getDetails()->type == SDL_MOUSEBUTTONUP
		&& action->getDetails()->button.button == SDL_BUTTON_RIGHT)
	{
		if (_inInventoryView == false)
		{
			_action->value = -1;
			_action->type = BA_NONE;
		}

		_game->popState();
	}
}

/**
 * Executes the action corresponding to this action menu item.
 * @param action - pointer to an Action
 */
void PrimeGrenadeState::btnClick(Action* action)
{
	int btnId = -1;

	if (action->getDetails()->type == SDL_MOUSEBUTTONUP
		&& action->getDetails()->button.button == SDL_BUTTON_RIGHT)
	{
		if (_inInventoryView == false)
		{
			_action->value = -1;
			_action->type = BA_NONE;
		}

		_game->popState();
		return;
	}

	if (action->getSender() == _isfBtn0)
		btnId = 0;
	else
	{
		for (size_t // find out which button was pressed
				i = 0u;
				i != 16u && btnId == -1;
				++i)
		{
			if (action->getSender() == _isfBtn[i])
				btnId = static_cast<int>(i) + 1;
		}
	}

	if (btnId != -1)
	{
		if (_inInventoryView == true)
		{
			_grenade->setFuse(btnId);
			_inventory->setPrimeGrenade(btnId);
		}
		else
			_action->value = btnId;

		_game->popState(); // get rid of the Set Timer menu

		if (_inInventoryView == false)
			_game->popState(); // get rid of the Action menu.
	}
}

}
