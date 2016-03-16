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

#include "MainMenuState.h"

//#include <sstream>

#include "IntroState.h"
#include "ListLoadState.h"
#include "NewBattleState.h"
#include "NewGameState.h"
#include "OptionsVideoState.h"

#include "../version.h"

#include "../Engine/Game.h"
#include "../Engine/Language.h"
#include "../Engine/Options.h"
#include "../Engine/Screen.h"

//#include "../Interface/Cursor.h"
#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/Window.h"

#include "../Resource/XcomResourcePack.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the Main Menu window.
 */
MainMenuState::MainMenuState()
{
	// kL_note: These screen calls were displaced to IntroState &
	// AbandonGameState & StartState & SaveGameState & MainMenuState::resize()
	//
	// This uses baseX/Y options for Geoscape & Basescape:
	Options::baseXResolution = Options::baseXGeoscape; // kL
	Options::baseYResolution = Options::baseYGeoscape; // kL
	// This sets Geoscape and Basescape to default (320x200) IG and the config.
/*	Screen::updateScale(
					Options::geoscapeScale,
					Options::geoscapeScale,
					Options::baseXGeoscape,
					Options::baseYGeoscape,
					true); kL */
	_game->getScreen()->resetDisplay(false); // kL

	_window			= new Window(this, 256, 160, 32, 20, POPUP_BOTH);
	_txtTitle		= new Text(256, 17, 32, 40);
	_txtBuild		= new Text(256,  9, 32, 66);

	_btnStart		= new TextButton(92, 20,  64, 88);
	_btnTactical	= new TextButton(92, 20, 164, 88);

	_btnLoad		= new TextButton(92, 20,  64, 116);
//	_btnOptions		= new TextButton(92, 20, 164, 116);
	_btnIntro		= new TextButton(92, 20, 164, 116);

	_btnQuit		= new TextButton(192, 20, 64, 144);

	setInterface("mainMenu");

	add(_window,		"window",	"mainMenu");
	add(_txtTitle,		"text",		"mainMenu");
	add(_txtBuild,		"text",		"mainMenu");
	add(_btnStart,		"button",	"mainMenu");
	add(_btnTactical,	"button",	"mainMenu");
	add(_btnLoad,		"button",	"mainMenu");
//	add(_btnOptions,	"button",	"mainMenu");
	add(_btnIntro,		"button",	"mainMenu");
	add(_btnQuit,		"button",	"mainMenu");

	centerAllSurfaces();


	_window->setBackground(_game->getResourcePack()->getSurface("BACK01.SCR"));

	_btnStart->setText(tr("STR_NEW_GAME"));
	_btnStart->onMouseClick((ActionHandler)& MainMenuState::btnNewGameClick);
	_btnStart->onKeyboardPress(
					(ActionHandler)& MainMenuState::btnNewGameClick,
					SDLK_n);

	_btnTactical->setText(tr("STR_NEW_BATTLE"));
	_btnTactical->onMouseClick((ActionHandler)& MainMenuState::btnNewBattleClick);
	_btnTactical->onKeyboardPress(
					(ActionHandler)& MainMenuState::btnNewBattleClick,
					SDLK_t);

	_btnLoad->setText(tr("STR_LOAD_SAVED_GAME"));
	_btnLoad->onMouseClick((ActionHandler)& MainMenuState::btnLoadClick);
	_btnLoad->onKeyboardPress(
					(ActionHandler)& MainMenuState::btnLoadClick,
					Options::keyOk);
	_btnLoad->onKeyboardPress(
					(ActionHandler)& MainMenuState::btnLoadClick,
					Options::keyOkKeypad);
	_btnLoad->onKeyboardPress(
					(ActionHandler)& MainMenuState::btnLoadClick,
					SDLK_l);

//	_btnOptions->setText(tr("STR_OPTIONS"));
//	_btnOptions->onMouseClick((ActionHandler)& MainMenuState::btnOptionsClick);
//	_btnOptions->setVisible(false);

	_btnIntro->setText(tr("STR_PLAYINTRO"));
	_btnIntro->onMouseClick((ActionHandler)& MainMenuState::btnPlayIntroClick);
	_btnIntro->onKeyboardPress(
					(ActionHandler)& MainMenuState::btnPlayIntroClick,
					SDLK_i);

	_btnQuit->setText(tr("STR_QUIT"));
	_btnQuit->onMouseClick((ActionHandler)& MainMenuState::btnQuitClick);
	_btnQuit->onKeyboardPress(
					(ActionHandler)& MainMenuState::btnQuitClick,
					Options::keyCancel);
	_btnQuit->onKeyboardPress(
					(ActionHandler)& MainMenuState::btnQuitClick,
					SDLK_q);

	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setBig();
	std::wostringstream title;
	title << tr("STR_OPENXCOM"); //kL << L"\x02";
//	title << Language::utf8ToWstr(OPENXCOM_VERSION_SHORT) << Language::utf8ToWstr(OPENXCOM_VERSION_GIT);
//	title << Language::utf8ToWstr(OPENXCOM_VERSION_GIT); // kL
	_txtTitle->setText(title.str());

	_txtBuild->setAlign(ALIGN_CENTER);
	_txtBuild->setText(Language::cpToWstr(Version::getBuildDate()));


	_game->getResourcePack()->playMusic(
									OpenXcom::res_MUSIC_START_MAINMENU,
									"", 1); // only once, Pls.

	if (SDL_ShowCursor(SDL_QUERY) == SDL_DISABLE)
		SDL_ShowCursor(SDL_ENABLE); // center my cursor. disabled in Game cTor and shown here instead.
}

/**
 * dTor.
 */
MainMenuState::~MainMenuState()
{}

/**
 * Initializes the state.
 *
void MainMenuState::init()
{
	State::init();
} */

/**
 * Opens the New Game window.
 * @param action - pointer to an Action
 */
void MainMenuState::btnNewGameClick(Action*)
{
	_game->pushState(new NewGameState());
}

/**
 * Opens the New Battle screen.
 * @param action - pointer to an Action
 */
void MainMenuState::btnNewBattleClick(Action*)
{
	_game->pushState(new NewBattleState());
}

/**
 * Opens the Load Game screen.
 * @param action - pointer to an Action
 */
void MainMenuState::btnLoadClick(Action*)
{
	_game->pushState(new ListLoadState(OPT_MENU));
}

/**
 * Opens the Options screen.
 * @param action - pointer to an Action
 *
void MainMenuState::btnOptionsClick(Action*)
{
	Options::backupDisplay();
	_game->pushState(new OptionsVideoState(OPT_MENU));
} */

/**
 * Plays the intro video.
 * @param action - pointer to an Action
 */
void MainMenuState::btnPlayIntroClick(Action*)
{
	_game->getResourcePack()->fadeMusic(_game, 924);

	const bool letterbox (Options::keepAspectRatio);
	Options::keepAspectRatio = true;

	Options::baseXResolution = Screen::ORIGINAL_WIDTH;
	Options::baseYResolution = Screen::ORIGINAL_HEIGHT;
	_game->getScreen()->resetDisplay(false);

	_game->setState(new IntroState(letterbox));
}

/**
 * Quits the game.
 * @param action - pointer to an Action
 */
void MainMenuState::btnQuitClick(Action*)
{
	_game->getResourcePack()->fadeMusic(_game, 793);
	_game->quit();
}

/**
 * Updates the scale.
 * @param dX - reference the delta of X
 * @param dY - reference the delta of Y
 *
void MainMenuState::resize(int& dX, int& dY)
{
	dX = Options::baseXResolution;
	dY = Options::baseYResolution;

	// This uses baseX/Y options for Geoscape & Basescape:
//	Options::baseXResolution = Options::baseXGeoscape; // kL
//	Options::baseYResolution = Options::baseYGeoscape; // kL
	// This sets Geoscape and Basescape to default (320x200) IG and the config.
	Screen::updateScale(
					Options::geoscapeScale,
					Options::geoscapeScale,
					Options::baseXGeoscape,
					Options::baseYGeoscape,
					true);
//	_game->getScreen()->resetDisplay(false); // kL: this resets options.cfg!

	dX = Options::baseXResolution - dX;
	dY = Options::baseYResolution - dY;

	State::resize(dX, dY);
} */

}
