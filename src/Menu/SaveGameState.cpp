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
 * @param origin	- section that originated this state
 * @param file		- reference to name of the save-file without extension
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
		_firstRun(0)
{
	build(palette);
}

/**
 * Initializes all the elements in the SaveGame screen.
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
		_firstRun(0)
{
	switch (type)
	{
		case SAVE_QUICK:
			_file = SavedGame::QUICKSAVE;
			break;

		case SAVE_AUTO_GEOSCAPE:
			_file = SavedGame::AUTOSAVE_GEOSCAPE;
			break;

		case SAVE_AUTO_BATTLESCAPE:
			_file = SavedGame::AUTOSAVE_BATTLESCAPE;
			break;

		case SAVE_IRONMAN:
		case SAVE_IRONMAN_END:
			_file = CrossPlatform::sanitizeFilename(Language::wstrToFs(_game->getSavedGame()->getLabel())) + SavedGame::SAVE_EXT;
	}

	build(palette);
}

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
//#ifdef _WIN32
//	MessageBeep(MB_OK); // <- done in BattlescapeState::handle() for Fkeys
//#endif
	_fullScreen = false;

	_txtStatus = new Text(320, 17, 0, 92);

	setPalette(palette);

	switch (_origin)
	{
		case OPT_MENU:
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

	if (_firstRun < 10) // Make sure it gets drawn properly
		++_firstRun;
	else
	{
		_game->popState();
		_game->getCursor()->setVisible();

		switch (_type)
		{
			case SAVE_DEFAULT: // manual save - Close the save screen.
				_game->popState();

				if (_game->getSavedGame()->isIronman() == false) // And pause screen too.
					_game->popState();
				break;

			case SAVE_QUICK: // automatic save - Give it a default name.
			case SAVE_AUTO_GEOSCAPE:
			case SAVE_AUTO_BATTLESCAPE:
				_game->getSavedGame()->setLabel(Language::fsToWstr(_file));
		}


		try // Save the game
		{
			const std::string backup (_file + ".bak");
			_game->getSavedGame()->save(backup);

			const std::string
				fullPath (Options::getUserFolder() + _file),
				backPath (Options::getUserFolder() + backup);
			if (CrossPlatform::moveFile(
									backPath,
									fullPath) == false)
			{
				throw Exception("Save backed up in " + backup);
			}

			if (_type == SAVE_IRONMAN_END)
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
