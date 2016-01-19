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
//#include "../Engine/Screen.h"
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
		scanbordOffsetX =  3,
		scanbordOffsetY = 12;

	_bg			= new InteractiveSurface(
									Options::baseXResolution,
									Options::baseYResolution,
									scanbordOffsetX, scanbordOffsetY);
	_miniView	= new MiniMapView(
//								223, 150, 47, 15,
								Options::baseXResolution,
								Options::baseYResolution,
								-(Options::baseXResolution - 320) / 2,
								-(Options::baseYResolution - 200) / 2,
								_game,
								camera,
								battleSave);

	_btnLvlUp	= new BattlescapeButton(18, 20,  24 + scanbordOffsetX,  62 + scanbordOffsetY);
	_btnLvlDown	= new BattlescapeButton(18, 20,  24 + scanbordOffsetX,  88 + scanbordOffsetY);
	_btnOk		= new BattlescapeButton(32, 32, 275 + scanbordOffsetX, 145 + scanbordOffsetY);

	_txtLevel	= new Text(28, 16, 281 + scanbordOffsetX, 73 + scanbordOffsetY);

	setPalette(PAL_BATTLESCAPE);

	add(_miniView); // put miniView *under* the background.
	add(_bg);
	_game->getResourcePack()->getSurface("Scanbord")->blit(_bg);

	add(_btnLvlUp,		"buttonUp",		"minimap", _bg);
	add(_btnLvlDown,	"buttonDown",	"minimap", _bg);
	add(_btnOk,			"buttonOK",		"minimap", _bg);
	add(_txtLevel,		"textLevel",	"minimap", _bg);

	centerAllSurfaces();


	_btnLvlUp->onMouseClick((ActionHandler)& MiniMapState::btnLevelUpClick);
	_btnLvlUp->onKeyboardPress(
					(ActionHandler)& MiniMapState::btnLevelUpClick,
					Options::keyBattleLevelUp);
	_btnLvlDown->onMouseClick((ActionHandler)& MiniMapState::btnLevelDownClick);
	_btnLvlDown->onKeyboardPress(
					(ActionHandler)& MiniMapState::btnLevelDownClick,
					Options::keyBattleLevelDown);

	_btnOk->onMouseClick((ActionHandler)& MiniMapState::btnOkClick);
	_btnOk->onKeyboardPress(
					(ActionHandler)& MiniMapState::btnOkClick,
					Options::keyOk);
	_btnOk->onKeyboardPress(
					(ActionHandler)& MiniMapState::btnOkClick,
					Options::keyOkKeypad);
	_btnOk->onKeyboardPress(
					(ActionHandler)& MiniMapState::btnOkClick,
					Options::keyCancel);
	_btnOk->onKeyboardPress(
					(ActionHandler)& MiniMapState::btnOkClick,
					Options::keyBattleMap);

	_txtLevel->setBig();
	_txtLevel->setHighContrast();
	std::wostringstream level;
	level << ((camera->getViewLevel() + 1) % 10);
	_txtLevel->setText(level.str());

	_timerAnimate = new Timer(125);
	_timerAnimate->onTimer((StateHandler)& MiniMapState::animate);
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
		if (action->getDetails()->button.button == SDL_BUTTON_WHEELUP)
			btnLevelDownClick(action);
		else if (action->getDetails()->button.button == SDL_BUTTON_WHEELDOWN)
			btnLevelUpClick(action);
	}
}

/**
 * Returns to the previous screen.
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

	BattlescapeState* const battleState = _game->getSavedGame()->getBattleSave()->getBattleState();
	battleState->getMap()->setNoDraw(false);
//	battleState->clearShowMapBtn();

	_game->popState();

	action->getDetails()->type = SDL_NOEVENT; // consume the event
}

/**
 * Changes the currently displayed MiniMap level.
 * @param action - pointer to an Action
 */
void MiniMapState::btnLevelUpClick(Action* action)
{
	std::wostringstream level;
	level << ((_miniView->up() + 1) % 10);
	_txtLevel->setText(level.str());

	action->getDetails()->type = SDL_NOEVENT; // consume the event
}

/**
 * Changes the currently displayed MiniMap level.
 * @param action - pointer to an Action
 */
void MiniMapState::btnLevelDownClick(Action* action)
{
	std::wostringstream level;
	level << ((_miniView->down() + 1) % 10);
	_txtLevel->setText(level.str());

	action->getDetails()->type = SDL_NOEVENT; // consume the event
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
