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

#include "VictoryState.h"

//#include <sstream>

#include "../Engine/CrossPlatform.h"
#include "../Engine/Game.h"
#include "../Engine/InteractiveSurface.h"
#include "../Engine/Language.h"
#include "../Engine/Options.h"
//#include "../Engine/Screen.h"
//#include "../Engine/Timer.h"

#include "../Interface/Text.h"

//#include "../Menu/MainMenuState.h"
#include "../Menu/StatisticsState.h"

#include "../Resource/XcomResourcePack.h"

#include "../Savegame/SavedGame.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the Victory screen.
 */
VictoryState::VictoryState()
	:
		_curScreen(std::numeric_limits<size_t>::max())
{
/*	Options::baseXResolution = Screen::ORIGINAL_WIDTH;
	Options::baseYResolution = Screen::ORIGINAL_HEIGHT;
	_game->getScreen()->resetDisplay(false); */

	const char* files[]
	{
		"PICT1.LBM",
		"PICT2.LBM",
		"PICT3.LBM",
		"PICT6.LBM",
		"PICT7.LBM"
	};

//	_timer = new Timer(30000u);

	_text[0u] = new Text(195,  56,  5,   0);
	_text[1u] = new Text(232,  64, 88, 136);
	_text[2u] = new Text(254,  48, 66, 152);
	_text[3u] = new Text(300, 200,  5,   0);
	_text[4u] = new Text(310,  42,  5, 158);

	for (size_t
			i = 0u;
			i != SCREENS;
			++i)
	{
		Surface* const screen (_game->getResourcePack()->getSurface(files[i]));

		_bg[i] = new InteractiveSurface();

		setPalette(screen->getPalette());

		add(_bg[i]);
		add(_text[i], "victoryText", "gameOver");

		screen->blit(_bg[i]);
		_bg[i]->setVisible(false);
		_bg[i]->onMousePress(	static_cast<ActionHandler>(&VictoryState::screenPress));
		_bg[i]->onKeyboardPress(static_cast<ActionHandler>(&VictoryState::screenPress));

		std::ostringstream oststr;
		oststr << "STR_VICTORY_" << static_cast<int>(i + 1u);
		_text[i]->setText(tr(oststr.str()));
		_text[i]->setWordWrap();
		_text[i]->setVisible(false);
	}

	_game->getResourcePack()->playMusic(OpenXcom::res_MUSIC_WIN);

	centerSurfaces();


//	_timer->onTimer((StateHandler)& VictoryState::screenTimer);
//	_timer->start();

	screenPress(nullptr);

	if (_game->getSavedGame()->isIronman() == true) // Ironman is over, rambo
	{
		const std::string file (CrossPlatform::sanitizeFilename(Language::wstrToFs(_game->getSavedGame()->getLabel())));
		CrossPlatform::deleteFile(Options::getUserFolder() + file + SavedGame::SAVE_ExtDot);
	}
}

/**
 * dTor.
 */
VictoryState::~VictoryState()
{
//	delete _timer;
}

/**
 * Shows the next screen on a timed basis.
 *
void VictoryState::screenTimer()
{
	screenPress(nullptr);
} */

/**
 * Handle timers.
 *
void VictoryState::think()
{
	_timer->think(this, nullptr);
} */

/**
 * Shows the next screen in the slideshow or goes back to the Main Menu.
 * @param action - pointer to an Action
 */
void VictoryState::screenPress(Action*)
{
//	_timer->start();

	if (_curScreen != std::numeric_limits<size_t>::max())
	{
		_bg[_curScreen]->setVisible(false);
		_text[_curScreen]->setVisible(false);
	}

	if (++_curScreen < SCREENS) // next screen
	{
		setPalette(_bg[_curScreen]->getPalette());
		_bg[_curScreen]->setVisible();
		_text[_curScreen]->setVisible();

		init();
	}
	else // to StatisticsState -> MainMenuState.
	{
		_game->getResourcePack()->fadeMusic(_game, 1157);

		_game->popState();
/*		Screen::updateScale(
						Options::geoscapeScale,
						Options::geoscapeScale,
						Options::baseXGeoscape,
						Options::baseYGeoscape,
						true);
		_game->getScreen()->resetDisplay(false); */

		_game->setState(new StatisticsState());
//		_game->setState(new MainMenuState());
//		_game->setSavedGame();
	}
}

}
