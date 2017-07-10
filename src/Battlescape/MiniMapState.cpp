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

#include "MiniMapState.h"

//#include <iostream>
//#include <sstream>

#include "Camera.h"
#include "MiniMapView.h"

#include "../Battlescape/BattlescapeState.h"
#include "../Battlescape/Map.h"

#include "../Engine/Action.h"
#include "../Engine/Game.h"
#include "../Engine/InteractiveSurface.h"
//#include "../Engine/LocalizedText.h"
//#include "../Engine/Options.h"
//#include "../Engine/Palette.h"
#include "../Engine/Screen.h"
#include "../Engine/Timer.h"

#include "../Interface/BattlescapeButton.h"
#include "../Interface/Text.h"

#include "../Resource/ResourcePack.h"

#include "../Savegame/SavedBattleGame.h"
#include "../Savegame/SavedGame.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the MiniMapState screen.
 */
MiniMapState::MiniMapState()
{
/*	if (Options::maximizeInfoScreens)
	{
		Options::baseXResolution = Screen::ORIGINAL_WIDTH;
		Options::baseYResolution = Screen::ORIGINAL_HEIGHT;
		_game->getScreen()->resetDisplay(false);
	} */
	static const int
		xOff ((Options::baseXResolution - 640) >> 1u),
		yOff ((Options::baseYResolution - 360) >> 1u);

	_bgScanbord	= new InteractiveSurface(
								Options::baseXResolution, // create the Surface the entire size of Screen so RMB-exit works anywhere.
								Options::baseYResolution,
								xOff, yOff);
	_miniView	= new MiniMapView(
								Options::baseXResolution,
								Options::baseYResolution,
								0,0,
								_game);

	_btnLvlUp	= new BattlescapeButton(36, 39,  15 + xOff, 121 + yOff);
	_btnLvlDown	= new BattlescapeButton(36, 39,  15 + xOff, 171 + yOff);
	_btnOk		= new BattlescapeButton(55, 55, 551 + xOff, 287 + yOff);

	_txtLevel	= new Text(25, 16, 556 + xOff, 148 + yOff);

	setPalette(PAL_BATTLESCAPE);

	add(_miniView);
	add(_bgScanbord); // put Scanbord over the MiniMap.
	add(_btnLvlUp,		"buttonUp",		"minimap", _bgScanbord);
	add(_btnLvlDown,	"buttonDown",	"minimap", _bgScanbord);
	add(_btnOk,			"buttonOK",		"minimap", _bgScanbord);
	add(_txtLevel,		"textLevel",	"minimap", _bgScanbord);

	Surface* const srf (_game->getResourcePack()->getSurface("Scanbord_640"));
	srf->blit(_bgScanbord);

	_btnLvlUp->onMouseClick(	static_cast<ActionHandler>(&MiniMapState::btnLevelUpClick));
	_btnLvlDown->onMouseClick(	static_cast<ActionHandler>(&MiniMapState::btnLevelDownClick));

	_btnOk->onMouseClick(static_cast<ActionHandler>(&MiniMapState::btnOkClick));

	Camera* const camera (_game->getSavedGame()->getBattleSave()->getBattleGame()->getMap()->getCamera());
	_txtLevel->setText(Text::intWide((camera->getViewLevel() + 1))); // %10
	_txtLevel->setAlign(ALIGN_CENTER);
	_txtLevel->setHighContrast();
	_txtLevel->setBig();

	_miniView->onKeyboardPress(	static_cast<ActionHandler>(&MiniMapState::btnOkClick),
								Options::keyBattleMap);
	_miniView->onKeyboardPress(	static_cast<ActionHandler>(&MiniMapState::btnOkClick),
								Options::keyCancel);
	_miniView->onKeyboardPress(	static_cast<ActionHandler>(&MiniMapState::btnOkClick),
								Options::keyOk);
	_miniView->onKeyboardPress(	static_cast<ActionHandler>(&MiniMapState::btnOkClick),
								Options::keyOkKeypad);
	_miniView->onKeyboardPress(	static_cast<ActionHandler>(&MiniMapState::btnLevelUpClick),
								Options::keyBattleLevelUp);
	_miniView->onKeyboardPress(	static_cast<ActionHandler>(&MiniMapState::btnLevelDownClick),
								Options::keyBattleLevelDown);
	_miniView->onKeyboardPress(	static_cast<ActionHandler>(&MiniMapState::keyCenterUnitPress),
								Options::keyBattleCenterUnit);
	_miniView->onKeyboardPress(	static_cast<ActionHandler>(&MiniMapState::keyCenterUnitPress),
								SDLK_KP5);

	_timerAnimate = new Timer(125u);
	_timerAnimate->onTimer(static_cast<StateHandler>(&MiniMapState::animate));
	_timerAnimate->start();

	_game->getSavedGame()->getBattleSave()->getBattleState()->getMap()->setNoDraw();
	_miniView->draw();
}

/**
 * dTor.
 */
MiniMapState::~MiniMapState()
{
	delete _timerAnimate;
}

/**
 * Handles mouse-wheeling.
 * @param action - pointer to an Action
 */
void MiniMapState::handle(Action* action)
{
	State::handle(action);

	if (action->getDetails()->type == SDL_MOUSEBUTTONDOWN)
	{
		switch (action->getDetails()->button.button)
		{
			case SDL_BUTTON_WHEELUP:
				btnLevelDownClick(action);
				break;
			case SDL_BUTTON_WHEELDOWN:
				btnLevelUpClick(action);
		}
	}
}

/**
 * Exits to the previous screen.
 * @param action - pointer to an Action
 */
void MiniMapState::btnOkClick(Action* action)
{
/*	if (Options::maximizeInfoScreens)
	{
		Screen::updateScale(
						Options::battlescapeScale,
						Options::battlescapeScale,
						Options::baseXBattlescape,
						Options::baseYBattlescape,
						true);
		_game->getScreen()->resetDisplay(false);
	} */

	_game->getSavedGame()->getBattleSave()->getBattleState()->getMap()->setNoDraw(false);
//	battleState->clearMinimapBtn();

	_game->getScreen()->fadeScreen();
	_game->popState();

	action->getDetails()->type = SDL_NOEVENT; // consume the event
}

/**
 * Changes the currently displayed MiniMap level.
 * @param action - pointer to an Action
 */
void MiniMapState::btnLevelUpClick(Action* action)
{
	if (_miniView->isScrollActive() == false)
		_txtLevel->setText(Text::intWide((_miniView->up() + 1))); // %10

	action->getDetails()->type = SDL_NOEVENT; // consume the event
}

/**
 * Changes the currently displayed MiniMap level.
 * @param action - pointer to an Action
 */
void MiniMapState::btnLevelDownClick(Action* action)
{
	if (_miniView->isScrollActive() == false)
		_txtLevel->setText(Text::intWide((_miniView->down() + 1))); // %10

	action->getDetails()->type = SDL_NOEVENT; // consume the event
}

/**
 * Centers the MiniMapView on the currently selected BattleUnit if any w/ key-press.
 * @param action - pointer to an Action
 */
void MiniMapState::keyCenterUnitPress(Action*)
{
	if (_miniView->isScrollActive() == false)
		_miniView->centerUnit();
}

/**
 * Updates the MiniMapView animation.
 */
void MiniMapState::animate() // private.
{
	_miniView->animate();
}

/**
 * Handles timers.
 */
void MiniMapState::think()
{
	State::think();
	_timerAnimate->think(this, nullptr);
}

}
