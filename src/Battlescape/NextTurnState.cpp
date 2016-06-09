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

#include "NextTurnState.h"

//#include <sstream>

#include "BattlescapeGame.h"			// kL, for terrain explosions
#include "BattlescapeState.h"
#include "ExplosionBState.h"			// kL, for terrain explosions
#include "TileEngine.h"					// kL, for terrain explosions
#include "Position.h"					// kL, for terrain explosions

#include "../Engine/Action.h"
#include "../Engine/Game.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Options.h"
//#include "../Engine/Palette.h"
//#include "../Engine/Screen.h"

#include "../Interface/Cursor.h"
#include "../Interface/Text.h"
#include "../Interface/Window.h"

#include "../Resource/XcomResourcePack.h"

#include "../Ruleset/RuleInterface.h"
#include "../Ruleset/Ruleset.h"

#include "../Savegame/SavedBattleGame.h"
#include "../Savegame/Tile.h"			// kL, for terrain explosions


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
	_window = new Window(this, 320, 200);

	if (aliensPacified == false)
	{
		_txtTitle	= new Text(320, 17, 0,  68);
		_txtTurn	= new Text(320, 17, 0,  93);
		_txtSide	= new Text(320, 17, 0, 109);
		_txtMessage	= new Text(320, 17, 0, 149);
	}
	else
		_txtMessage	= new Text(320, 200);

//	_bg = new Surface(_game->getScreen()->getWidth(), _game->getScreen()->getHeight());

	setPalette(PAL_BATTLESCAPE);

	add(_window);
	add(_txtMessage,	"messageWindows", "battlescape");
	if (aliensPacified == false)
	{
		add(_txtTitle,	"messageWindows", "battlescape");
		add(_txtTurn,	"messageWindows", "battlescape");
		add(_txtSide,	"messageWindows", "battlescape");
	}
//	add(_bg);

	centerAllSurfaces();


/*	_bg->setX(0);
	_bg->setY(0);
	SDL_Rect rect;
	rect.w = _bg->getWidth();
	rect.h = _bg->getHeight();
	rect.x = rect.y = 0;
	_bg->drawRect(&rect, 15); */
/*	// line this screen up with the hidden movement screen
	const int msg_y = state->getMap()->getMessageY();
	_window->setY(msg_y);
	if (aliensPacified == false)
	{
		_txtTitle->setY(msg_y + 68);
		_txtTurn->setY(msg_y + 93);
		_txtSide->setY(msg_y + 109);
		_txtMessage->setY(msg_y + 149);
	}
	else _txtMessage->setY(msg_y); */

	_window->setBackground(_game->getResourcePack()->getSurface("Diehard"));
	_window->setHighContrast();

	if (aliensPacified == false)
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
			if (_battleSave->getTurnLimit() - _battleSave->getTurn() < 4)
				_txtTurn->setColor(_game->getRuleset()->getInterface("inventory")->getElement("weight")->color2); // borrow 'overweight' color ...
		}
		_txtTurn->setText(woststr.str());

		_txtSide->setBig();
		_txtSide->setAlign(ALIGN_CENTER);
		_txtSide->setHighContrast();
		_txtSide->setText(tr("STR_SIDE")
							.arg(tr(_battleSave->getSide() == FACTION_PLAYER ? "STR_XCOM" : "STR_ALIENS")));
	}

	if (aliensPacified == false)
		_txtMessage->setText(tr("STR_PRESS_BUTTON_TO_CONTINUE"));
	else
	{
		_txtMessage->setText(tr("STR_ALIENS_PACIFIED"));
		_txtMessage->setVerticalAlign(ALIGN_MIDDLE);
	}
	_txtMessage->setAlign(ALIGN_CENTER);
	_txtMessage->setHighContrast();
	_txtMessage->setBig();

	_state->clearMouseScrollingState();


	if (_battleSave->getSide() != FACTION_PLAYER
		&& _battleSave->getDebugTac() == false)
	{
		_game->getCursor()->setVisible(false);
	}
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
			_state->updateTurn();
			nextTurn();
	}
}

/**
 * Does turn-start stuff.
 */
void NextTurnState::nextTurn() // private.
{
	static bool switchMusic (true);

	// Done here and in DebriefingState but removed from ~BattlescapeGame.
	_battleSave->getBattleGame()->cleanBattleStates();
	_game->popState();

	int
		liveHostile,
		livePlayer;
	_state->getBattleGame()->tallyUnits(
									liveHostile,
									livePlayer);
	if (livePlayer < 1
		|| (liveHostile == 0
			&& _battleSave->getObjectiveTileType() != MUST_DESTROY))	// <- not the final mission
	{																// final tactical determination done in BattlescapeGame::endTurn() -> finishBattle()
		switchMusic = true;											// or AbortMissionState::btnOkClick() -> finishBattle().
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
					_state->autosave(); // NOTE: Auto-save points are fucked; they should be done *before* important events, not after.
				}

				if (turn != 1)
					_battleSave->getBattleGame()->setPlayerPanic();

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


				Tile* const tile (_battleSave->getTileEngine()->checkForTerrainExplosions());
				if (tile != nullptr)
				{
					const Position pos (Position::toVoxelSpaceCentered(tile->getPosition(), 10));
					_battleSave->getBattleGame()->statePushBack(new ExplosionBState(
																				_battleSave->getBattleGame(),
																				pos,
																				nullptr, nullptr,
																				tile,
																				false, false, true));
				}
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
