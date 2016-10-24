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

#include "LoadGameState.h"

//#include <sstream>

#include "ErrorMessageState.h"
#include "ListLoadState.h"
#include "StatisticsState.h"

#include "../Battlescape/BattlescapeState.h"

#include "../Engine/CrossPlatform.h"
#include "../Engine/Exception.h"
#include "../Engine/Game.h"
#include "../Engine/Language.h"
#include "../Engine/Logger.h"
#include "../Engine/Options.h"
#include "../Engine/Screen.h"

#include "../Geoscape/GeoscapeState.h"

#include "../Interface/Cursor.h"
#include "../Interface/Text.h"

#include "../Ruleset/RuleInterface.h"
#include "../Ruleset/Ruleset.h"

#include "../Savegame/SavedBattleGame.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the LoadGame screen.
 * @param origin	- section that originated this state
 * @param file		- reference to the name of the save-file without extension
 * @param palette	- pointer to parent-state palette
 * @param parent	- pointer to parent ListLoadState to hide its elements
 */
LoadGameState::LoadGameState(
		OptionsOrigin origin,
		const std::string& file,
		SDL_Color* const palette,
		ListLoadState* const parent)
	:
		_origin(origin),
		_file(file),
		_parent(parent),
		_firstRun(0)
{
	buildUi(palette);
}

/**
 * Initializes all the elements in the LoadGame screen.
 * @param origin	- section that originated this state
 * @param type		- type of quick-load being used
 * @param palette	- pointer to parent-state palette
 */
LoadGameState::LoadGameState(
		OptionsOrigin origin,
		SaveType type,
		SDL_Color* const palette)
	:
		_origin(origin),
		_firstRun(0)
{
	switch (type) // can't auto-load ironman games
	{
		case SAVE_QUICK:
			_file = SavedGame::QUICKSAVE;
			break;

		case SAVE_AUTO_GEOSCAPE:
			_file = SavedGame::AUTOSAVE_GEOSCAPE;
			break;

		case SAVE_AUTO_BATTLESCAPE:
			_file = SavedGame::AUTOSAVE_BATTLESCAPE;
	}
	buildUi(palette);
}

/**
 * dTor.
 */
LoadGameState::~LoadGameState()
{
#ifdef _WIN32
	MessageBeep(MB_ICONASTERISK);
#endif
}

/**
 * Builds the interface.
 * @param palette - pointer to parent-state palette
 */
void LoadGameState::buildUi(SDL_Color* const palette)
{
//#ifdef _WIN32
//	MessageBeep(MB_OK); // <- done in BattlescapeState::handle() for Fkeys
//#endif
	_fullScreen = false;

	_txtStatus = new Text(320, 17, 0, 92);

	setPalette(palette);

	switch (_origin)
	{
		case OPT_GEOSCAPE:
		case OPT_MENU:
			add(_txtStatus, "textLoad", "geoscape");
			break;

		case OPT_BATTLESCAPE:
			add(_txtStatus, "textLoad", "battlescape");
			_txtStatus->setHighContrast();
	}

	centerSurfaces();


	_txtStatus->setText(tr("STR_LOADING_GAME"));
	_txtStatus->setAlign(ALIGN_CENTER);
	_txtStatus->setBig();

	_game->getCursor()->setVisible(false);
}

/**
 * Ignores quick-loads without a save available.
 */
void LoadGameState::init()
{
	State::init();

	if (_file == SavedGame::QUICKSAVE
		&& CrossPlatform::fileExists(Options::getUserFolder() + _file) == false)
	{
		_game->popState();
		_game->getCursor()->setVisible();
	}
}

/**
 * Loads the specified entry.
 */
void LoadGameState::think()
{
	State::think();

	if (_firstRun < 7) // pause a bit to Ensure this gets drawn properly
		++_firstRun;
	else
	{
		_game->popState();
		_game->getCursor()->setVisible();

		SavedGame* const gameSave (new SavedGame(_game->getRuleset()));
		try
		{
			Log(LOG_INFO) << "LoadGameState: loading";
			_game->setSavedGame(gameSave);
			gameSave->load(_file, _game->getRuleset());

			switch (gameSave->getEnding())
			{
				case END_WIN:
				case END_LOSE:
					Options::baseXResolution = Screen::ORIGINAL_WIDTH;
					Options::baseYResolution = Screen::ORIGINAL_HEIGHT;
					_game->getScreen()->resetDisplay(false);

					_game->setState(new StatisticsState); // TODO: A way of saving non-Ironman saves for reviewing post-game statistics.
					break;

				case END_NONE:
					Options::baseXResolution = Options::baseXGeoscape;
					Options::baseYResolution = Options::baseYGeoscape;
					_game->getScreen()->resetDisplay(false);

					_game->setState(new GeoscapeState());

					SavedBattleGame* const battleSave (gameSave->getBattleSave());
					if (battleSave != nullptr)
					{
						Log(LOG_INFO) << "LoadGameState: loading battlescape map";
						battleSave->loadMapResources(_game);

						Options::baseXResolution = Options::baseXBattlescape;
						Options::baseYResolution = Options::baseYBattlescape;
						_game->getScreen()->resetDisplay(false);

						BattlescapeState* const battleState (new BattlescapeState());
						_game->pushState(battleState);
						battleSave->setBattleState(battleState);
					}
			}
		}
		catch (Exception& e)
		{
			Log(LOG_INFO) << "LoadGame error";
			_parent->hideElements(false);

			Log(LOG_ERROR) << e.what();
			std::wostringstream error;
			error << tr("STR_LOAD_UNSUCCESSFUL") << L'\x02' << Language::fsToWstr(e.what());

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

			if (_game->getSavedGame() == gameSave)
				_game->setSavedGame();
			else
				delete gameSave;
		}
		catch (YAML::Exception& e)
		{
			Log(LOG_INFO) << "LoadGame error YAML";
			_parent->hideElements(false);

			Log(LOG_ERROR) << e.what();
			std::wostringstream error;
			error << tr("STR_LOAD_UNSUCCESSFUL") << L'\x02' << Language::fsToWstr(e.what());

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

			if (_game->getSavedGame() == gameSave)
				_game->setSavedGame();
			else
				delete gameSave;
		}

		CrossPlatform::flashWindow();
	}
}

}
