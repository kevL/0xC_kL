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

#include "MediTargetState.h"

#include "MediKitState.h"
#include "TileEngine.h"

#include "../Engine/Action.h"
#include "../Engine/Game.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Options.h"

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextList.h"
#include "../Interface/Window.h"

#include "../Resource/ResourcePack.h"

#include "../Savegame/BattleUnit.h"
#include "../Savegame/SavedBattleGame.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/Tile.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the MediTargetState screen.
 * @param action - pointer to a BattleAction (BattlescapeGame.h)
 */
MediTargetState::MediTargetState(BattleAction* const action)
	:
		_action(action)
{
	_fullScreen = false;

	_window		= new Window(this, 270, 96, 25, 50);
	_txtTitle	= new Text(200, 9, 60, 58);

	_txtWounds	= new Text(13, 9, 156, 70);
	_txtHealth	= new Text(42, 9, 174, 70);
	_txtEnergy	= new Text(32, 9, 221, 70);
	_txtMorale	= new Text(18, 9, 258, 70);

	_lstTarget	= new TextList(234, 33, 42, 80);

	_btnCancel	= new TextButton(120, 14, 100, 125);

	setPalette(PAL_BATTLESCAPE);

	add(_window,	"messageWindowBorder",	"battlescape");
	add(_txtTitle,	"messageWindows",		"battlescape");
	add(_txtWounds,	"messageWindows",		"battlescape");
	add(_txtHealth,	"messageWindows",		"battlescape");
	add(_txtEnergy,	"messageWindows",		"battlescape");
	add(_txtMorale,	"messageWindows",		"battlescape");
	add(_lstTarget);
	add(_btnCancel,	"messageWindowButtons",	"battlescape");

	centerSurfaces();


	_window->setBackground(_game->getResourcePack()->getSurface("BACK02.SCR"));
	_window->setHighContrast();

	_txtTitle->setText(tr("STR_MEDITARGET"));
	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setHighContrast();

	_txtWounds->setText(L"fw"); // STR_FATAL_WOUNDS
	_txtWounds->setHighContrast();

	_txtHealth->setText(L"health"); // STR_HEALTH
	_txtHealth->setHighContrast();

	_txtEnergy->setText(L"sta"); // STR_STAMINA
	_txtEnergy->setHighContrast();

	_txtMorale->setText(L"mrl"); // STR_MORALE
	_txtMorale->setHighContrast();

	_lstTarget->setColumns(5, 108,18,47,37,18);
	_lstTarget->setColor(PINK);
	_lstTarget->setBackground(_window);
	_lstTarget->setSelectable();
	_lstTarget->setMargin(6);
	_lstTarget->setHighContrast();
	_lstTarget->onMousePress(static_cast<ActionHandler>(&MediTargetState::lstTargetPress));

	_btnCancel->setText(tr("STR_CANCEL"));
	_btnCancel->setHighContrast();
	_btnCancel->onMouseClick(	static_cast<ActionHandler>(&MediTargetState::btnCancelClick));
	_btnCancel->onKeyboardPress(static_cast<ActionHandler>(&MediTargetState::btnCancelClick),
								Options::keyCancel);
	_btnCancel->onKeyboardPress(static_cast<ActionHandler>(&MediTargetState::btnCancelClick),
								Options::keyOk);
	_btnCancel->onKeyboardPress(static_cast<ActionHandler>(&MediTargetState::btnCancelClick),
								Options::keyOkKeypad);
}

/**
 * dTor.
 */
MediTargetState::~MediTargetState()
{}

/**
 * Resets the palette and adds targets to the TextList.
 */
void MediTargetState::init()
{
	State::init();

	bool
		addToList,
		actorFound (false); // adds actor to top of MediTargetList.

	SavedBattleGame* const battleSave (_game->getSavedGame()->getBattleSave());

	const std::vector<BattleUnit*>* const targetUnits (battleSave->getUnits());
	for (std::vector<BattleUnit*>::const_iterator
			i = targetUnits->begin();
			i != targetUnits->end();
			)
	{
		if ((*i)->isHealable() == true
			&& ((*i)->getOriginalFaction() != FACTION_HOSTILE
				|| _game->getSavedGame()->isResearched((*i)->getRaceString() + "_AUTOPSY")))
		{
			addToList = false;

			if (actorFound == false && *i == _action->actor)
				addToList = true;

			if (actorFound == true && *i != _action->actor)
			{
				if (((*i)->getUnitStatus() == STATUS_UNCONSCIOUS
						&& (*i)->getPosition() == _action->actor->getPosition())
					|| ((*i)->getFaction() != FACTION_HOSTILE
						&& battleSave->getTileEngine()->validMeleeRange(
																	_action->actor,
																	_action->actor->getUnitDirection(),
																	*i) == true))
				{
					addToList = true;
				}
			}

			if (addToList == true)
			{
				_targetUnits.push_back(*i);

				std::wostringstream
					woststr1,
					woststr2;

				woststr1 << _targetUnits.back()->getHealth()
						 << L"|"
						 << _targetUnits.back()->getBattleStats()->health
						 << L"|"
						 << _targetUnits.back()->getStun();

				woststr2 << _targetUnits.back()->getEnergy()
						 << L"|"
						 << _targetUnits.back()->getBattleStats()->stamina;

				_lstTarget->addRow(
								5,
								(*i)->getLabel(_game->getLanguage()).c_str(),
								Text::intWide(_targetUnits.back()->getFatalsTotal()).c_str(),
								woststr1.str().c_str(),
								woststr2.str().c_str(),
								Text::intWide(_targetUnits.back()->getMorale()).c_str());

				if (actorFound == false)
					_lstTarget->setRowColor(0u, ORANGE, true);
			}
		}

		if (actorFound == false && *i == targetUnits->back()) // in case medikit user is not 'Healable'
		{
			actorFound = true;
			i = targetUnits->begin();
		}
		else
			++i;
	}

	if (_targetUnits.size() == 1u)
	{
		_action->posTarget = _targetUnits[0u]->getPosition(); // jic.
		_action->targetUnit = _targetUnits[0u];

		_game->popState();
		_game->pushState(new MedikitState(_action));
	}
	else if (_targetUnits.empty() == true)
		_game->popState();
}

/**
 * Chooses a unit to apply Medikit to.
 * @param action - pointer to an Action
 */
void MediTargetState::lstTargetPress(Action* action)
{
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
	{
		BattleUnit* const targetUnit (_targetUnits[_lstTarget->getSelectedRow()]);
		_action->posTarget = targetUnit->getPosition(); // jic.
		_action->targetUnit = targetUnit;

		_game->popState();
		_game->pushState(new MedikitState(_action));
	}
}

/**
 * Exits to the previous screen.
 * @param action - pointer to an Action
 */
void MediTargetState::btnCancelClick(Action*)
{
	_game->popState();
}

}
