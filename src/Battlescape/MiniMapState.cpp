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
 * @param camera		- pointer to the battlescape Camera
 * @param battleSave	- pointer to the SavedBattleGame
 */
MiniMapState::MiniMapState(
		Camera* const camera,
		const SavedBattleGame* const battleSave)
{
/*	if (Options::maximizeInfoScreens)
	{
		Options::baseXResolution = Screen::ORIGINAL_WIDTH;
		Options::baseYResolution = Screen::ORIGINAL_HEIGHT;
		_game->getScreen()->resetDisplay(false);
	} */
	static const int
//		scanbordOffsetX =  3, // these center the original Scanbord better in relation to the crosshairs.
//		scanbordOffsetY = 12;
		bgOffsetX = -((Options::baseXResolution - Screen::ORIGINAL_WIDTH)  >> 1u),
		bgOffsetY = -((Options::baseYResolution - Screen::ORIGINAL_HEIGHT) >> 1u);

	_bg			= new InteractiveSurface(
									Options::baseXResolution,
									Options::baseYResolution,
									bgOffsetX, bgOffsetY);
//									scanbordOffsetX, scanbordOffsetY);
	_miniView	= new MiniMapView(
//								223,150, 47,15,
								Options::baseXResolution,
								Options::baseYResolution,
								bgOffsetX, bgOffsetY,
								_game,
								camera,
								battleSave);

//	_btnLvlUp	= new BattlescapeButton(18, 20,  24 + scanbordOffsetX,  62 + scanbordOffsetY);
//	_btnLvlDown	= new BattlescapeButton(18, 20,  24 + scanbordOffsetX,  88 + scanbordOffsetY);
//	_btnOk		= new BattlescapeButton(32, 32, 275 + scanbordOffsetX, 145 + scanbordOffsetY);
//	_txtLevel	= new Text(28, 16, 281 + scanbordOffsetX, 73 + scanbordOffsetY);

	setPalette(PAL_BATTLESCAPE);

	add(_miniView); // put miniView *under* the background.
	add(_bg);

	Surface* const srf (_game->getResourcePack()->getSurface("Scanbord_640")); // "Scanbord" original 320px
	srf->blit(_bg);

	centerSurfaces();

	_btnLvlUp	= new BattlescapeButton(36, 39,  15, 121); // do not 'center' these ->
	_btnLvlDown	= new BattlescapeButton(36, 39,  15, 171);
	_btnOk		= new BattlescapeButton(55, 55, 551, 287);
	_txtLevel	= new Text(25, 16, 556, 147);
	add(_btnLvlUp,		"buttonUp",		"minimap", _bg);
	add(_btnLvlDown,	"buttonDown",	"minimap", _bg);
	add(_btnOk,			"buttonOK",		"minimap", _bg);
	add(_txtLevel,		"textLevel",	"minimap", _bg);


	_btnLvlUp->onMouseClick(	static_cast<ActionHandler>(&MiniMapState::btnLevelUpClick));
	_btnLvlDown->onMouseClick(	static_cast<ActionHandler>(&MiniMapState::btnLevelDownClick));

	_btnOk->onMouseClick(static_cast<ActionHandler>(&MiniMapState::btnOkClick));

	_txtLevel->setHighContrast();
	_txtLevel->setBig();
	_txtLevel->setAlign(ALIGN_CENTER);
	_txtLevel->setText(Text::intWide((camera->getViewLevel() + 1) /*% 10*/));

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

	BattlescapeState* const battleState (_game->getSavedGame()->getBattleSave()->getBattleState());
	battleState->getMap()->setNoDraw(false);
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
//	_miniView->up();
	_txtLevel->setText(Text::intWide((_miniView->up() + 1) /*% 10*/));

	action->getDetails()->type = SDL_NOEVENT; // consume the event
}

/**
 * Changes the currently displayed MiniMap level.
 * @param action - pointer to an Action
 */
void MiniMapState::btnLevelDownClick(Action* action)
{
//	_miniView->down();
	_txtLevel->setText(Text::intWide((_miniView->down() + 1) /*% 10*/));

	action->getDetails()->type = SDL_NOEVENT; // consume the event
}

/**
 * Centers the MiniMapView on the currently selected BattleUnit if any w/ key-press.
 * @param action - pointer to an Action
 */
void MiniMapState::keyCenterUnitPress(Action*)
{
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
