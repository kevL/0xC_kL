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

#include "StartState.h"

//#include <SDL/SDL_mixer.h>
//#include <SDL/SDL_thread.h>

#include "ErrorMessageState.h"
#include "IntroState.h"
#include "MainMenuState.h"

#include "../version.h"

#include "../Engine/Action.h"
#include "../Engine/CrossPlatform.h"
#include "../Engine/Exception.h"
#include "../Engine/Font.h"
#include "../Engine/Game.h"
#include "../Engine/Language.h"
#include "../Engine/Logger.h"
#include "../Engine/Music.h"
#include "../Engine/Options.h"
#include "../Engine/Screen.h"
#include "../Engine/Sound.h"
//#include "../Engine/Surface.h"
#include "../Engine/Timer.h"

#include "../Interface/FpsCounter.h"
#include "../Interface/Cursor.h"
#include "../Interface/Text.h"

#include "../Resource/XcomResourcePack.h"

#include "../Ruleset/RuleInterface.h"
#include "../Ruleset/Ruleset.h"


namespace OpenXcom
{

LoadingPhase StartState::_loadPhase	= LOADING_STARTED;	// static/private.
std::string StartState::_error		= "";				// static/private.
bool StartState::_ready				= false;			// static/private.
// why are those static along with load()
// do their values really have to be cached between invokations of this state.


/**
 * Initializes all the elements in the StartState screen.
 */
StartState::StartState()
	:
		_thread(nullptr),
		_dosStep(0u)
{
	_loadPhase = LOADING_STARTED;	// leave these in until I figure out if they
	_error = "";					// actually need to be static.
	_ready = false;

	// updateScale() uses safeDisplayWidth/Height and needs to be set ahead of time
//kL	Options::safeDisplayWidth	= Options::displayWidth;
//kL	Options::safeDisplayHeight	= Options::displayHeight;

//kL	Options::baseXResolution = Options::displayWidth;
//kL	Options::baseYResolution = Options::displayHeight;
	Options::baseXResolution = 640;	// kL
	Options::baseYResolution = 400;	// kL
	_game->getScreen()->resetDisplay(false);

	const int
//		dx = (Options::baseXResolution - 320) / 2,	// kL
//		dy = (Options::baseYResolution - 200) / 2;	// kL
		dx (10), // kL
		dy (20); // kL
//	_surface = new Surface(320, 200, dx, dy);		// kL


	_font = new Font();
	_font->loadTerminal();

	_charWidth = _font->getSpacing() + _font->getWidth();

	_lang = new Language();
	_text = new Text(
//kL				Options::baseXResolution,
//kL				Options::baseYResolution,
//kL				0,0);
					630,380,
					dx,dy);
	_caret = new Text(
					_font->getWidth(),
					_font->getHeight());

	_timer = new Timer(1u);

	setPalette(
			_font->getSurface()->getPalette(),
			0,2);

	add(_text);
	add(_caret);

	_text->initText(_font, _font, _lang);
	_text->setColor(0u);
	_text->setWordWrap();

//	wostringstream woststr;
//	woststr << L"Why hello there, I am a custom monospaced bitmap font, yet I"
//			<< L" can still take advantage of wordwrapping and all the bells and"
//			<< L" whistles that regular text does, because I am actually being"
//			<< L" generated by a regular Text, golly gee whiz, we sure are"
//			<< L" living in the future now!\n\n"
//			<< L"!\"#$%&'()*+,-./0123456789:;<=>?\n"
//			<< L"@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_\n"
//			<< L"`abcdefghijklmnopqrstuvwxyz{|}~\n\n"
//			<< L"Loading 0xC_kL ...";
//	_text->setText(woststr.str());

	_caret->initText(_font, _font, _lang);
	_caret->setColor(0u);
	_caret->setText(L"_");

	_timer->onTimer(static_cast<StateHandler>(&StartState::doDosart));
	_timer->start();

	_game->getCursor()->setVisible(false);
	_game->getFpsCounter()->setVisible(false);

//	if (Options::reload == true) // NOTE: No reloading possible unless Options screens get re-instated.
//	{
//		addLine(L"Restarting ...");
//		addLine(L"");
//	}
//	else
	addLine(Language::utf8ToWstr(CrossPlatform::getDosPath()));
}

/**
 * Kill the thread in case the game is quit early.
 */
StartState::~StartState()
{
	if (_thread != nullptr)
		SDL_KillThread(_thread);

	delete _font;
	delete _timer;
	delete _lang;
}

/**
 * Reset and reload data.
 */
void StartState::init()
{
	State::init();

	// Silence!
	Sound::stop();
	Music::stop();

	_game->setResourcePack();
//	if (Options::mute == false && Options::reload == true) // NOTE: No reloading possible unless Options screens get re-instated.
//	{
//		Mix_CloseAudio();
//		_game->initAudio();
//	}

//	std::wostringstream woststr;
//	woststr	<< L"Loading OpenXcom "
//			<< L"Language::utf8ToWstr(OPENXCOM_VERSION_SHORT)"
//			<< L"Language::utf8ToWstr(OPENXCOM_VERSION_GIT)"
//			<< L" ...";
//	addLine(woststr.str());

	if ((_thread = SDL_CreateThread(									// load the game data in a separate thread
								load,
								static_cast<void*>(_game))) == nullptr)	// if thread can't create just load as usual
	{
		load(static_cast<void*>(_game));
	}
}

/**
 * Loads game-data and updates loading-status accordingly.
 * @param ptrG - pointer to Game!
 * @return, thread status (0 = ok)
 */
int StartState::load(void* ptrG) // static/private.
{
	Game* const game (static_cast<Game*>(ptrG));
	try
	{
		Log(LOG_INFO) << "Loading rulesets ...";
		game->loadRulesets();
		Log(LOG_INFO) << "Rulesets loaded.";

		Log(LOG_INFO) << "Loading resources ...";
		game->setResourcePack(new XcomResourcePack(game->getRuleset()));
		Log(LOG_INFO) << "Resources loaded.";

		Log(LOG_INFO) << "Loading language ...";
		game->defaultLanguage();
		Log(LOG_INFO) << "Language loaded.";

		if (_ready == false)
			_ready = true;
		else
		{
			_ready = false;
			_loadPhase = LOADING_SUCCESSFUL;
		}
	}
	catch (Exception& e)
	{
		_error = e.what();
		Log(LOG_ERROR) << _error;

		_loadPhase = LOADING_FAILED;
	}
	catch (YAML::Exception& e)
	{
		_error = e.what();
		Log(LOG_ERROR) << _error;

		_loadPhase = LOADING_FAILED;
	}
	return 0;
}

/**
 * Displays fake ASCII print.
 * @note If the loading fails it shows an error otherwise moves on to the game.
 */
void StartState::think()
{
	State::think();
	_timer->think(this, nullptr);

	switch (_loadPhase)
	{
		case LOADING_FAILED:
			CrossPlatform::flashWindow();

			addLine(L"");
			addLine(L"ERROR: " + Language::utf8ToWstr(_error) + L"\n");
//			addLine(L"Make sure you installed OpenXcom correctly.");
//			addLine(L"Check the wiki documentation for more details.");
//			addLine(L"");
//			addLine(L"Press any key to continue.");

			_loadPhase = LOADING_DONE;
			break;

		case LOADING_SUCCESSFUL:
			CrossPlatform::flashWindow();

			Log(LOG_INFO) << "0xC_kL started!";
			if (Options::reload == false && Options::playIntro == true)
			{
				Log(LOG_INFO) << "Playing intro video ...";
				const bool letterbox (Options::keepAspectRatio);
				Options::keepAspectRatio = true;

				Options::baseXResolution = Screen::ORIGINAL_WIDTH;
				Options::baseYResolution = Screen::ORIGINAL_HEIGHT;
				_game->getScreen()->resetDisplay(false);

				_game->setState(new IntroState(letterbox));
			}
			else
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

				State* const state (new MainMenuState());
				_game->setState(state);

				if (Options::badRules.empty() == false) // check for ruleset loading errors
				{
					std::wostringstream error;
					error << tr("STR_MOD_UNSUCCESSFUL") << L'\x02';
					for (std::vector<std::string>::const_iterator
							i = Options::badRules.begin();
							i != Options::badRules.end();
							++i)
					{
						error << Language::fsToWstr(*i) << L'\n';
					}

					Options::badRules.clear();

					const RuleInterface* const uiRule (_game->getRuleset()->getInterface("errorMessages"));
					_game->pushState(new ErrorMessageState(
														error.str(),
														state->getPalette(),
														uiRule->getElement("geoscapeColor")->color,
														"BACK01.SCR",
														uiRule->getElement("geoscapePalette")->color));
				}
				Options::reload = false;
			}

			_game->getCursor()->setVisible();
			_game->getFpsCounter()->setVisible(Options::fpsCounter);
	}
}

/**
 * The game quits if the player presses the cancel-key [Esc] when an error
 * message is on display.
 * @param action - pointer to an Action
 */
void StartState::handle(Action* action)
{
	State::handle(action);

	if (_loadPhase == LOADING_DONE
		&& action->getDetails()->type == SDL_KEYDOWN
		&& action->getDetails()->key.keysym.sym == Options::keyCancel)
	{
		_game->quit();
	}
}

/**
 * Prints terminal output and blinks the cursor.
 */
void StartState::doDosart() // private.
{
	if (_loadPhase == LOADING_STARTED)
	{
		if (++_dosStep % 15 == 0)
			_caret->setVisible(!_caret->getVisible());

/*		if (Options::reload == true) // NOTE: No reloading possible unless Options screens get re-instated.
		{
			if (_dosStep == 2)
			{
				std::wostringstream woststr;
				woststr << L"Loading " << Language::utf8ToWstr(OPENXCOM_VERSION_GIT);
//				woststr << L"Loading OpenXcom " << Language::utf8ToWstr(OPENXCOM_VERSION_SHORT) << Language::utf8ToWstr(OPENXCOM_VERSION_GIT) << "...";
				addLine(woststr.str());
			}

			// NOTE: may need to set this:
//			if (_ready)
//				_loadPhase = LOADING_SUCCESSFUL;
//			else
//				_ready = true;
		} */
/*		if (Options::reload == true)
		{
			if (_dosStep == 1)
				addNewline();

			if (_dosStep < 20)
			{
				addNewline();
				_dosart = L"Loading oXc_kL ..."; // 18 chars
				addChar(_dosStep - 2);
			}
			else
				addCursor();

			// NOTE: may need to set this:
//			if (_ready)
//				_loadPhase = LOADING_SUCCESSFUL;
//			else
//				_ready = true;
		} */
//		else
//		{
		switch (_dosStep)
		{
			case   1u: // start.
				addCaret();
				// no break;
			case  44u:
			case  91u:
			case 115u:
			case 142u:
			case 161u:
			case 187u:
				addNewline();
		}

		if (_dosStep < 44u) // 1..43
		{
			_dosart = L"DOS/4GW Protected Mode Run-time Version 1.9"; // 43 chars
			addChar(_dosStep - 1u);
		}
		else if (_dosStep == 44u)
			addCaret();

		else if (_dosStep < 91u) // 45..90
		{
			_dosart = L"Copyright (c) Rational Systems, Inc. 1990-1993"; // 46 chars
			addChar(_dosStep - 45u);
		}
		else if (_dosStep == 91u)
		{
			addNewline();
			addCaret();
		}

		else if (_dosStep < 115u) // 92..114
		{
			_dosart = L"_0xC_kL_ initialization"; // 23 chars
			addChar(_dosStep - 92u);
		}
		else if (_dosStep == 115u)
		{
			addNewline();
			addCaret();
		}

		else if (_dosStep < 142u) // 116..141
		{
/*			if (Options::mute)
				_dosart = L"No Sound Detected";
			else */
			_dosart = L"SoundBlaster Sound Effects"; // 26 chars
			addChar(_dosStep - 116u);
		}
		else if (_dosStep == 142u)
			addCaret();

		else if (_dosStep < 161u) // 143..160
		{
/*			if (Options::preferredMusic == MUSIC_MIDI)
				_dosart = L"General MIDI Music";
			else */
			_dosart = L"SoundBlaster Music"; // 18 chars
			addChar(_dosStep - 143u);
		}
		else if (_dosStep == 161u)
			addCaret();

		else if (_dosStep < 187u) // 162..186
		{
/*			if (Options::preferredMusic != MUSIC_MIDI)
				_dosart = L"Base Port 220 Irq 5 Dma 1";
			else */
			_dosart = L"Base Port 220 Irq 5 Dma 1"; // 25 chars
			addChar(_dosStep - 162u);
		}
		else if (_dosStep == 187u)
		{
			addNewline();
			addCaret();
		}

		else if (_dosStep < 203) // 188..202
		{
			_dosart = L"Loading 0xC_kL "; // 15 chars
			addChar(_dosStep - 188u);
		}
		else if (_dosStep == 203)
		{
			addCaret();

			if (_ready == false)
				_ready = true;
			else
			{
				_ready = false;
				_loadPhase = LOADING_SUCCESSFUL;
			}
		}
		else if (_dosStep % 60 == 0)
		{
			if (_dosStep == 2100) // roughly.
				addNewline();

			addWait();
			addCaret();
		}
//		}
	}
}

/**
 * Adds a line of text to the dos-terminal and moves the cursor appropriately.
 * @param line - reference to a line of text
 */
void StartState::addLine(const std::wstring& line) // private.
{
	_output << L"\n" << line;
	_text->setText(_output.str());

	addCaret();
}

/**
 * Adds a newline to the dos-terminal.
 */
void StartState::addNewline() // private.
{
	_output << L"\n";
}

/**
 * Adds another charater to the dos-terminal.
 * @param pos - position to get a character
 */
void StartState::addChar(size_t pos) // private.
{
	_output << _dosart.at(pos);
	_text->setText(_output.str());
}

/**
 * Adds the caret to the dos-terminal.
 */
void StartState::addCaret() // private.
{
	int y (_text->getTextHeight() - _font->getHeight());

	_caret->setX(_text->getTextWidth(y / _font->getHeight()) + _charWidth);
	_caret->setY(y + _font->getHeight() + 4);//20);
}

/**
 * Adds a wait-glyph to the dos-terminal.
 */
void StartState::addWait() // private.
{
	_output << L".";
	_text->setText(_output.str());
}

}
