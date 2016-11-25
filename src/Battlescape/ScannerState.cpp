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

#include "ScannerState.h"

#include "ScannerView.h"

#include "../Engine/Action.h"
#include "../Engine/Game.h"
#include "../Engine/InteractiveSurface.h"
#include "../Engine/Options.h"
#include "../Engine/Screen.h"
#include "../Engine/Timer.h"

#include "../Resource/ResourcePack.h"

//#include "../Savegame/BattleUnit.h"


namespace OpenXcom
{

/**
 * Initializes the Scanner state.
 * @param action - pointer to an Action
 */
ScannerState::ScannerState(const BattleUnit* const selUnit)
{
/*	if (Options::maximizeInfoScreens)
	{
		Options::baseXResolution = Screen::ORIGINAL_WIDTH;
		Options::baseYResolution = Screen::ORIGINAL_HEIGHT;
		_game->getScreen()->resetDisplay(false);
	} */

	_bg			= new InteractiveSurface();
	_scan		= new InteractiveSurface();
	_scanView	= new ScannerView(
								152,152,
								56,24,
								_game,
								selUnit);

	if (_game->getScreen()->getDY() > 50)
		_fullScreen = false;

	setPalette(PAL_BATTLESCAPE);

	add(_scan);
	add(_scanView);
	add(_bg);

	centerSurfaces();


	_game->getResourcePack()->getSurface("DETBORD.PCK")->blit(_bg);
	_game->getResourcePack()->getSurface("DETBORD2.PCK")->blit(_scan);

	_bg->onMouseClick(		static_cast<ActionHandler>(&ScannerState::exitClick),
							SDL_BUTTON_RIGHT);
	_bg->onKeyboardPress(	static_cast<ActionHandler>(&ScannerState::exitClick),
							Options::keyCancel);
	_bg->onKeyboardPress(	static_cast<ActionHandler>(&ScannerState::exitClick),
							Options::keyOk);
	_bg->onKeyboardPress(	static_cast<ActionHandler>(&ScannerState::exitClick),
							Options::keyOkKeypad);

	_timer = new Timer(125u);
	_timer->onTimer(static_cast<StateHandler>(&ScannerState::animate));
	_timer->start();
}

/**
 * dTor.
 */
ScannerState::~ScannerState()
{
	delete _timer;
}

/**
 * Closes the window on right-click.
 * @param action - pointer to an Action
 */
void ScannerState::handle(Action* action)
{
	State::handle(action);

	if (action->getDetails()->type == SDL_MOUSEBUTTONDOWN // Might not be needed, cf. UnitInfoState::handle().
		&& action->getDetails()->button.button == SDL_BUTTON_RIGHT)
	{
		exitClick();
	}
}

/**
 * Cycles the radar-blob animations.
 */
void ScannerState::animate() // private.
{
	_scanView->animate();
}

/**
 * Handles the Timer.
 */
void ScannerState::think()
{
	State::think();
	_timer->think(this, nullptr);
}

/**
 * Exits the state.
 * @param action - pointer to an Action (default nullptr)
 */
void ScannerState::exitClick(Action*) // private.
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
	_game->popState();
}

}
