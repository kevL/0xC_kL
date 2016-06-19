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

#include "AbortMissionState.h"

//#include <sstream>
//#include <vector>

#include "BattlescapeState.h"

#include "../Engine/Action.h"
#include "../Engine/Game.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Options.h"

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/Window.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/RuleAlienDeployment.h"
#include "../Ruleset/Ruleset.h"

#include "../Savegame/SavedBattleGame.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the AbortMission window.
 * @param battleSave	- pointer to the SavedBattleGame
 * @param state			- pointer to the BattlescapeState
 */
AbortMissionState::AbortMissionState(
		SavedBattleGame* const battleSave,
		BattlescapeState* const state)
	:
		_battleSave(battleSave),
		_state(state),
		_insideExit(0)
{
	_fullScreen = false;

	_window			= new Window(this, 320, 144);

	_txtInsideExit	= new Text(304, 17, 16, 25);
	_txtOutsideExit	= new Text(304, 17, 16, 50);

	_txtAbort		= new Text(320, 17, 0, 84);

	_btnCancel		= new TextButton(134, 18,  16, 116);
	_btnOk			= new TextButton(134, 18, 170, 116);

	setPalette(PAL_BATTLESCAPE);

	add(_window,			"messageWindowBorder",	"battlescape");
	add(_txtInsideExit,		"messageWindows",		"battlescape");
	add(_txtOutsideExit,	"messageWindows",		"battlescape");
	add(_txtAbort,			"messageWindows",		"battlescape");
	add(_btnCancel,			"messageWindowButtons",	"battlescape");
	add(_btnOk,				"messageWindowButtons",	"battlescape");

	centerAllSurfaces();


	_window->setBackground(_game->getResourcePack()->getSurface("Diehard"));
	_window->setHighContrast();

	std::string nextStage;
	switch (_battleSave->getTacType())
	{
		case TCT_BASEASSAULT: // no check for next-stage if Ufo_Crashed or _Landed.
		case TCT_BASEDEFENSE:
		case TCT_TERRORSITE:
		case TCT_MARS1:
//		case TCT_MARS2:
			nextStage = _game->getRuleset()->getDeployment(_battleSave->getTacticalType())->getNextStage();
	}

	int outsideExit (0);
	for (std::vector<BattleUnit*>::const_iterator
			i = _battleSave->getUnits()->begin();
			i != _battleSave->getUnits()->end();
			++i)
	{
		if ((*i)->getOriginalFaction() == FACTION_PLAYER
			&& (*i)->isOut_t(OUT_STAT) == false)
		{
			if (   (nextStage.empty() == true  && (*i)->isInExitArea() == true) // TODO: Ability to retreat from multi-stage !noRetreat battles.
				|| (nextStage.empty() == false && (*i)->isInExitArea(END_POINT)))
			{
				++_insideExit;
			}
			else
				++outsideExit;
		}
	}

	switch (_battleSave->getTacType())
	{
		case TCT_BASEDEFENSE:
			_txtInsideExit->setVisible(false);
			_txtOutsideExit->setVisible(false);
			break;

		default:
			_txtInsideExit->setText(tr("STR_UNITS_IN_EXIT_AREA", _insideExit));
			_txtInsideExit->setBig();
			_txtInsideExit->setAlign(ALIGN_CENTER);
			_txtInsideExit->setHighContrast();

			_txtOutsideExit->setText(tr("STR_UNITS_OUTSIDE_EXIT_AREA", outsideExit));
			_txtOutsideExit->setBig();
			_txtOutsideExit->setAlign(ALIGN_CENTER);
			_txtOutsideExit->setHighContrast();
	}

	_txtAbort->setText(tr("STR_ABORT_MISSION_QUESTION"));
	_txtAbort->setAlign(ALIGN_CENTER);
	_txtAbort->setHighContrast();
	_txtAbort->setBig();

	_btnOk->setText(tr("STR_OK"));
	_btnOk->setHighContrast();
	_btnOk->onMouseClick((ActionHandler)& AbortMissionState::btnOkClick);
	_btnOk->onKeyboardPress(
					(ActionHandler)& AbortMissionState::btnOkClick,
					Options::keyOk);
	_btnOk->onKeyboardPress(
					(ActionHandler)& AbortMissionState::btnOkClick,
					Options::keyOkKeypad);
	_btnOk->onKeyboardPress(
					(ActionHandler)& AbortMissionState::btnOkClick,
					Options::keyBattleAbort);

	_btnCancel->setText(tr("STR_CANCEL_UC"));
	_btnCancel->setHighContrast();
	_btnCancel->onMouseClick((ActionHandler)& AbortMissionState::btnCancelClick);
	_btnCancel->onKeyboardPress(
					(ActionHandler)& AbortMissionState::btnCancelClick,
					Options::keyCancel);
}

/**
 * dTor.
 */
AbortMissionState::~AbortMissionState()
{}

/**
 * Confirms tactical abortion.
 * @param action - pointer to an Action
 */
void AbortMissionState::btnOkClick(Action*)
{
	_game->popState();

	_battleSave->setAborted();
	_state->finishBattle(true, _insideExit);
}

/**
 * Exits to the previous screen.
 * @param action - pointer to an Action
 */
void AbortMissionState::btnCancelClick(Action*)
{
	_state->clearAbortBtn();
	_game->popState();
}

}
