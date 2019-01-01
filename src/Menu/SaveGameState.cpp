/*
 * Copyright 2010-2019 OpenXcom Developers.
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

#ifdef _WIN32
#	ifndef NOMINMAX
#		define NOMINMAX
#	endif

#	ifndef WIN32_LEAN_AND_MEAN
#		define WIN32_LEAN_AND_MEAN
#	endif

#	include <windows.h>
#endif

#include "SaveGameState.h"

//#include <sstream>

#include "ErrorMessageState.h"
#include "MainMenuState.h"

#include "../Engine/CrossPlatform.h"
#include "../Engine/Exception.h"
#include "../Engine/Game.h"
#include "../Engine/Language.h"
#include "../Engine/Logger.h"
#include "../Engine/Options.h"
//#include "../Engine/Screen.h"

#include "../Interface/Cursor.h"
#include "../Interface/Text.h"

#include "../Ruleset/RuleInterface.h"
#include "../Ruleset/Ruleset.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the SaveGame screen.
 * @note Used by ListSaveState::saveGame() only.
 * @param origin	- section that originated this state
 * @param file		- reference to name of the save-file with extension
 * @param palette	- pointer to parent-state palette
 */
SaveGameState::SaveGameState(
		OptionsOrigin origin,
		const std::string& file,
		SDL_Color* const palette)
	:
		_origin(origin),
		_file(file),
		_type(SAVE_DEFAULT),
		_wait(0)
{
	build(palette);
}

/**
 * Initializes all the elements in the SaveGame screen.
 * @note Used for quicksaves, autosaves, and ironballs saves.
 * @param origin	- section that originated this state
 * @param type		- type of quick-save being used
 * @param palette	- pointer to parent-state palette
 */
SaveGameState::SaveGameState(
		OptionsOrigin origin,
		SaveType type,
		SDL_Color* const palette)
	:
		_origin(origin),
		_type(type),
		_wait(0)
{
	switch (_type)
	{
		case SAVE_QUICK:
			_file = SavedGame::SAVE_Quick;
			break;

		case SAVE_AUTO_GEOSCAPE:
			_file = SavedGame::SAVE_AUTO_Geo;
			break;

		case SAVE_AUTO_BATTLESCAPE:
			_file = SavedGame::SAVE_AUTO_Tac;
			break;

		case SAVE_IRONMAN:
		case SAVE_IRONMAN_QUIT:
			_file = CrossPlatform::sanitizeFilename(Language::wstrToFs(_game->getSavedGame()->getLabel())) + SavedGame::SAVE_ExtDot;
	}

	build(palette);
}
/* enum SaveType
{
	SAVE_DEFAULT,			// 0
	SAVE_QUICK,				// 1
	SAVE_AUTO_GEOSCAPE,		// 2
	SAVE_AUTO_BATTLESCAPE,	// 3
	SAVE_IRONMAN,			// 4
	SAVE_IRONMAN_QUIT		// 5
}; */
/*	SavedGame::SAVE_AUTO_Geo = "_autogeo_.aq"
	SavedGame::SAVE_AUTO_Tac = "_autotac_.aq"
	SavedGame::SAVE_Quick    = "_quick_.aq"
	SavedGame::SAVE_Ext      = "sav"
	SavedGame::SAVE_ExtDot   = ".sav"
	SavedGame::SAVE_Ext_AQ   = "aq"
	SavedGame::SAVE_BakDot   = ".bak" */

/**
 * dTor.
 */
SaveGameState::~SaveGameState()
{
#ifdef _WIN32
	MessageBeep(MB_ICONASTERISK);
#endif
}

/**
 * Builds the interface.
 * @param palette - pointer to parent-state palette
 */
void SaveGameState::build(SDL_Color* const palette) // private.
{
	_fullScreen = false;

	_txtStatus = new Text(320, 16, 0, 92);

	setPalette(palette);

	switch (_origin)
	{
		case OPT_MAIN_START:
		case OPT_GEOSCAPE:
			add(_txtStatus, "textLoad", "geoscape");
			break;

		case OPT_BATTLESCAPE:
			add(_txtStatus, "textLoad", "battlescape");
			_txtStatus->setHighContrast();
	}

	centerSurfaces();


	_txtStatus->setText(tr("STR_SAVING_GAME"));
	_txtStatus->setAlign(ALIGN_CENTER);
	_txtStatus->setBig();

	_game->getCursor()->setVisible(false);
}

/**
 * Saves the current game.
 */
void SaveGameState::think()
{
	State::think();

	if (_wait < WAIT_TICKS) // persist the text to Ensure that player sees it
		++_wait;
	else
	{
		_game->popState(); // this.
		_game->getCursor()->setVisible();

		switch (_type)
		{
			case SAVE_DEFAULT:		// ordinary save from ListSaveState
				_game->popState();	// close the ListSave screen.

				if (_game->getSavedGame()->isIronman() == false) // And the Pause screen too. what - why should an Ironballs game get passed in here as type SAVE_DEFAULT.
					_game->popState();
				break;

			case SAVE_QUICK: // give these a default label ->
			case SAVE_AUTO_GEOSCAPE:
			case SAVE_AUTO_BATTLESCAPE:
				_game->getSavedGame()->setLabel(Language::fsToWstr(_file));
		}


		try // Save the game
		{
			const std::string backup (_file + SavedGame::SAVE_BakDot);
			_game->getSavedGame()->save(backup);

			if (CrossPlatform::moveFile(
									Options::getUserFolder() + backup,
									Options::getUserFolder() + _file) == false)
			{
				throw Exception("SaveGameState::think() has backed up the file as " + backup);
			}

			if (_type == SAVE_IRONMAN_QUIT)
			{
				// This uses baseX/Y options for Geoscape & Basescape:
//				Options::baseXResolution = Options::baseXGeoscape; // kL
//				Options::baseYResolution = Options::baseYGeoscape; // kL
				// This sets Geoscape and Basescape to default (320x200) IG and the config.
/*kL			Screen::updateScale(
								Options::geoscapeScale,
								Options::geoscapeScale,
								Options::baseXGeoscape,
								Options::baseYGeoscape,
								true); */
//				_game->getScreen()->resetDisplay(false);

				_game->setState(new MainMenuState());
				_game->setSavedGame();
			}
		}
		catch (Exception& e)
		{
			// TODO: Show the ListGamesState elements again ....
			Log(LOG_ERROR) << e.what();
			std::wostringstream error;
			error << tr("STR_SAVE_UNSUCCESSFUL") << L'\x02' << Language::fsToWstr(e.what());

			const RuleInterface* const uiRule (_game->getRuleset()->getInterface("errorMessages"));
			if (_origin != OPT_BATTLESCAPE)
				_game->pushState(new ErrorMessageState(
													error.str(),
													_palette,
													uiRule->getElement("geoscapeColor")->color,
													"BACK01.SCR",
													uiRule->getElement("geoscapePalette")->color));
			else
				_game->pushState(new ErrorMessageState(
													error.str(),
													_palette,
													uiRule->getElement("battlescapeColor")->color,
													"Diehard",
													uiRule->getElement("battlescapePalette")->color));
		}
		catch (YAML::Exception& e)
		{
			// TODO: Show the ListGamesState elements again ....
			Log(LOG_ERROR) << e.what();
			std::wostringstream error;
			error << tr("STR_SAVE_UNSUCCESSFUL") << L'\x02' << Language::fsToWstr(e.what());

			const RuleInterface* const uiRule (_game->getRuleset()->getInterface("errorMessages"));
			if (_origin != OPT_BATTLESCAPE)
				_game->pushState(new ErrorMessageState(
													error.str(),
													_palette,
													uiRule->getElement("geoscapeColor")->color,
													"BACK01.SCR",
													uiRule->getElement("geoscapePalette")->color));
			else
				_game->pushState(new ErrorMessageState(
													error.str(),
													_palette,
													uiRule->getElement("battlescapeColor")->color,
													"Diehard",
													uiRule->getElement("battlescapePalette")->color));
		}
	}
}

}
