/*
 * Copyright 2010-2018 OpenXcom Developers.
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

#include "NextTurnState.h"

//#include <sstream>

#include "BattlescapeGame.h"			// for terrain explosions
#include "BattlescapeState.h"
#include "ExplosionBState.h"			// for terrain explosions
#include "TileEngine.h"					// for terrain explosions
#include "Position.h"					// for terrain explosions

#include "../Engine/Action.h"
#include "../Engine/Game.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Options.h"
//#include "../Engine/Palette.h"
#include "../Engine/Screen.h"

#include "../Interface/Cursor.h"
#include "../Interface/Text.h"
#include "../Interface/Window.h"

#include "../Resource/XcomResourcePack.h"

#include "../Ruleset/RuleInterface.h"
#include "../Ruleset/Ruleset.h"

#include "../Savegame/SavedBattleGame.h"
#include "../Savegame/Tile.h"			// for terrain explosions


namespace OpenXcom
{

/**
 * Initializes all the elements in the NextTurn screen.
 * @param battleSave		- pointer to the SavedBattleGame
 * @param state				- pointer to the BattlescapeState
 * @param aliensPacified	- true if all remaining aliens are mind-controlled (default false)
 */
NextTurnState::NextTurnState(
		SavedBattleGame* const battleSave,
		BattlescapeState* const state,
		bool aliensPacified)
	:
		_battleSave(battleSave),
		_state(state),
		_aliensPacified(aliensPacified)
{
	//Log(LOG_INFO) << "NextTurnState:cTor";
	switch (_battleSave->getSide())
	{
		case FACTION_PLAYER:					// in case a Hostile/Neutral unit just revealed the screen
			SDL_Delay(Screen::SCREEN_PAUSE);	// sustain the reveal.
			break;

		case FACTION_HOSTILE:
			if (_battleSave->getDebugTac() == false)
				_game->getCursor()->setVisible(false);
	}

	_window = new Window(this);

	if (_aliensPacified == false)
	{
		_txtTitle   = new Text(320, 16, 0,  68);
		_txtTurn    = new Text(320, 16, 0,  93);
		_txtSide    = new Text(320, 16, 0, 109);
		_txtMessage = new Text(320, 16, 0, 149);
	}
	else
		_txtMessage	= new Text();

	setPalette(PAL_BATTLESCAPE);

	add(_window);
	add(_txtMessage, "messageWindows", "battlescape");
	if (_aliensPacified == false)
	{
		add(_txtTitle, "messageWindows", "battlescape");
		add(_txtTurn,  "messageWindows", "battlescape");
		add(_txtSide,  "messageWindows", "battlescape");
	}

	centerSurfaces();


	_window->setBackground(_game->getResourcePack()->getSurface("Diehard"));
	_window->setHighContrast();

	if (_aliensPacified == false)
	{
		_txtTitle->setBig();
		_txtTitle->setAlign(ALIGN_CENTER);
		_txtTitle->setHighContrast();
		_txtTitle->setText(tr("STR_OPENXCOM"));

		_txtTurn->setBig();
		_txtTurn->setAlign(ALIGN_CENTER);
		_txtTurn->setHighContrast();

		std::wostringstream woststr;
		woststr << tr("STR_TURN").arg(_battleSave->getTurn());
		if (_battleSave->getTurnLimit() != 0)
		{
			woststr << L" / " << _battleSave->getTurnLimit();
			if (_battleSave->getTurnLimit() - _battleSave->getTurn() < 4) // borrow 'overweight' color ...
				_txtTurn->setColor(static_cast<Uint8>(_game->getRuleset()->getInterface("inventory")->getElement("weight")->color2));
		}
		_txtTurn->setText(woststr.str());

		_txtSide->setBig();
		_txtSide->setAlign(ALIGN_CENTER);
		_txtSide->setHighContrast();
		_txtSide->setText(tr("STR_SIDE")
							.arg(tr(_battleSave->getSide() == FACTION_PLAYER ? "STR_XCOM" : "STR_ALIENS")));

		_txtMessage->setText(tr("STR_PRESS_BUTTON_TO_CONTINUE"));
	}
	else
	{
		_txtMessage->setText(tr("STR_ALIENS_PACIFIED"));
		_txtMessage->setVerticalAlign(ALIGN_MIDDLE);
	}
	_txtMessage->setAlign(ALIGN_CENTER);
	_txtMessage->setHighContrast();
	_txtMessage->setBig();

	_state->clearDragScroll();
}

/**
 * dTor.
 */
NextTurnState::~NextTurnState()
{}

/**
 * Closes the window.
 * @param action - pointer to an Action
 */
void NextTurnState::handle(Action* action)
{
	State::handle(action);

	switch (action->getDetails()->type)
	{
		case SDL_KEYDOWN:
		case SDL_MOUSEBUTTONDOWN:
			_state->updateTurnText();
			nextTurn();
	}
}

/**
 * Does turn-start stuff.
 */
void NextTurnState::nextTurn() // private.
{
	//Log(LOG_INFO) << "NextTurnState:nextTurn() side= " << _battleSave->getSide();
	static bool switchMusic (true);

	// Done here and in DebriefingState but removed from ~BattlescapeGame.
	_battleSave->getBattleGame()->clearBattleStates();
	_game->popState();

	int
		liveHostile,
		livePlayer;
	_state->getBattleGame()->tallyUnits(
									liveHostile,
									livePlayer);
	if (  (liveHostile == 0 && _battleSave->getObjectiveTilepartType() != OBJECTIVE_TILE) // <- not the final mission
		|| livePlayer  <  1)	// final tactical determination done in BattlescapeGame::endTurn() -> finishBattle()
	{							// or AbortMissionState::btnOkClick() -> finishBattle().
		switchMusic = true;
		_state->finishBattle(false, livePlayer);
	}
	else
	{
		_state->btnCenterPress(nullptr);

		switch (_battleSave->getSide())
		{
			case FACTION_PLAYER: // start Player turn.
			{
				_state->getBattleGame()->setupSelector();
				_game->getCursor()->setVisible();

				const int turn (_battleSave->getTurn());
				if (_battleSave->getSide() == FACTION_PLAYER
					&& (turn == 1 || (turn % Options::autosaveFrequency) == 0))
				{
					_state->requestAutosave(); // NOTE: Auto-save points are fucked; they should be done *before* important events, not after.
				}

				if (turn != 1)
					_battleSave->getBattleGame()->setPlayerPanic(); // what about 2-stage missions ...

				if (turn != 1 && switchMusic == true)
				{
					_game->getResourcePack()->fadeMusic(_game, 473);

					std::string
						trackType,
						terrainType;
					_battleSave->calibrateMusic(trackType, terrainType);
					_game->getResourcePack()->playMusic(trackType, terrainType);
				}
				else
					switchMusic = true;


				Tile* const tile (_battleSave->getTileEngine()->checkForTerrainExplosives());
				if (tile != nullptr)
					_battleSave->getBattleGame()->stateBPushBack(new ExplosionBState(
																				_battleSave->getBattleGame(),
																				Position::toVoxelSpaceCentered(tile->getPosition(), 10),
																				nullptr, nullptr,
																				tile,
																				false, false, true));
				break;
			}

			case FACTION_HOSTILE: // start non-Player turn.
			case FACTION_NEUTRAL:
				if (_aliensPacified == false)
				{
					_game->getResourcePack()->fadeMusic(_game, 473);
					_game->getResourcePack()->playMusic(OpenXcom::res_MUSIC_TAC_BATTLE_ALIENTURN);
				}
				else
					switchMusic = false;
		}
	}
}

/**
 *
 *
void NextTurnState::resize(int& dX, int& dY)
{
	State::resize(dX, dY);
//	_bg->setX(0);
//	_bg->setY(0);
} */

}
