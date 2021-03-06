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
 * @note Used by ListLoadState::lstPress() and ConfirmLoadState::btnYesClick() only.
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
		_wait(0)
{
	build(palette);
}

/**
 * Initializes all the elements in the LoadGame screen.
 * @note Used by GeoscapeState/BattlescapeState::handle() only for loading a quicksave.
 * @param origin	- section that originated this state
 * @param palette	- pointer to parent-state palette
 */
LoadGameState::LoadGameState(
		OptionsOrigin origin,
		SDL_Color* const palette)
	:
		_origin(origin),
		_file(SavedGame::SAVE_Quick),
		_parent(nullptr),
		_wait(0)
{
	build(palette);
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
void LoadGameState::build(SDL_Color* const palette) // private.
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


	_txtStatus->setText(tr("STR_LOADING_GAME"));
	_txtStatus->setAlign(ALIGN_CENTER);
	_txtStatus->setBig();

	_game->getCursor()->setVisible(false);
}

/**
 * Ignores quickloads without a save available.
 */
void LoadGameState::init()
{
	State::init();

	if (CrossPlatform::fileExists(Options::getUserFolder() + _file) == false)
	{
		_game->popState();
		_game->getCursor()->setVisible();
	}
}

/**
 * Loads a clicked entry.
 */
void LoadGameState::think()
{
	State::think();

	if (_wait < WAIT_TICKS) // persist the text to Ensure that player sees it
		++_wait;
	else
	{
		_game->popState();
		_game->getCursor()->setVisible();

		SavedGame* const playSave (new SavedGame(_game->getRuleset()));
		try
		{
			Log(LOG_INFO) << "LoadGameState: loading";
			_game->setSavedGame(playSave);
			playSave->load(_file, _game->getRuleset());

			switch (playSave->getEnding())
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

					SavedBattleGame* const battleSave (playSave->getBattleSave());
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
			if (_parent != nullptr) _parent->hideList(false);

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

			if (_game->getSavedGame() == playSave)
				_game->setSavedGame();
			else
				delete playSave;
		}
		catch (YAML::Exception& e)
		{
			Log(LOG_INFO) << "LoadGame error YAML";
			if (_parent != nullptr) _parent->hideList(false);

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

			if (_game->getSavedGame() == playSave)
				_game->setSavedGame();
			else
				delete playSave;
		}

		CrossPlatform::flashWindow();
	}
}

}
