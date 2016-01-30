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

#include "Game.h"

// kL_begin: Old
/*
#ifdef _WIN32
#	define WIN32_LEAN_AND_MEAN
#	include <windows.h>
#	include <SDL_syswm.h>
#endif
*/ // kL_end.

//#include <cmath>
//#include <cstddef> // nullptr (for NB code-assistant only)
//#include <sstream>

#include <SDL_mixer.h>

#include "Action.h"
#include "CrossPlatform.h"
#include "Exception.h"
#include "Language.h"
#include "Logger.h"
#include "Music.h"
#include "Options.h"
#include "Screen.h"
#include "Sound.h"
#include "State.h"

#include "../Interface/Cursor.h"
#include "../Interface/FpsCounter.h"
#include "../Interface/NumberText.h"

#include "../Menu/TestState.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/Ruleset.h"

#include "../Savegame/SavedGame.h"


namespace OpenXcom
{

const double Game::VOLUME_GRADIENT = 10.;


/**
 * Starts up SDL with all the subsystems and SDL_mixer for audio processing,
 * creates the display screen and sets up the cursor.
 * @param title - reference the title of the game window
 */
Game::Game(const std::string& title)
	:
		_screen(nullptr),
		_cursor(nullptr),
		_lang(nullptr),
		_res(nullptr),
		_gameSave(nullptr),
		_rules(nullptr),
		_quit(false),
		_init(false),
		_inputActive(true),
		_ticksTillNextSlice(0),
		_tickOfLastSlice(0),
		_debugCycle(-1),
		_debugCycle_b(-1),
		_blitDelay(false)
{
	Options::reload = false;
#ifdef _DEBUG
	Options::mute = true;
#else
	Options::mute = false;
#endif

	// Initialize SDL
	if (SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		throw Exception(SDL_GetError());
	}
	Log(LOG_INFO) << "SDL initialized.";

	// Initialize SDL_mixer
	if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0)
	{
		Log(LOG_ERROR) << SDL_GetError();
		Log(LOG_WARNING) << "No sound device detected, audio disabled.";
		Options::mute = true;
	}
	else
		initAudio();

	// trap the mouse inside the window (if *not* _DEBUG)
#ifndef _DEBUG
	SDL_WM_GrabInput(Options::captureMouse);
#else
	SDL_WM_GrabInput(static_cast<SDL_GrabMode>(false));
#endif

	// Set the window icon
#ifdef _WIN32
	CrossPlatform::setWindowIcon(2);
#else
	CrossPlatform::setWindowIcon("openxcom.png");
#endif

	// Set the window caption
	SDL_WM_SetCaption(title.c_str(), nullptr);

	SDL_EnableUNICODE(1);

	// Create display
	_screen = new Screen();
/*	Screen::BASE_WIDTH = Options::getInt("baseXResolution");
	Screen::BASE_HEIGHT = Options::getInt("baseYResolution");
	_screen = new Screen(
			Options::getInt("displayWidth"),
			Options::getInt("displayHeight"), 0,
			Options::getBool("fullscreen"),
			Options::getInt("windowedModePositionX"),
			Options::getInt("windowedModePositionY")); */ // kL

	// Create cursor
	_cursor = new Cursor(9,13);

	// Create invisible hardware cursor to workaround bug with absolute positioning pointing devices
//	SDL_ShowCursor(SDL_ENABLE);
	SDL_ShowCursor(SDL_DISABLE); // kL
	Uint8 cursor = 0;
	SDL_SetCursor(SDL_CreateCursor(
								&cursor, &cursor,
								1,1,0,0));

	// Create fps counter
	_fpsCounter = new FpsCounter(15,5,0,0);

	// Create blank language
	_lang = new Language();
}

/**
 * Deletes the display screen, cursor, states, etc etc etc, and shuts down all
 * the SDL subsystems.
 */
Game::~Game()
{
	Sound::stop();
	Music::stop();

	for (std::list<State*>::const_iterator
			i = _states.begin();
			i != _states.end();
			++i)
	{
		delete *i;
	}

	SDL_FreeCursor(SDL_GetCursor());

	delete _cursor;
	delete _lang;
	delete _res;
	delete _gameSave;
	delete _rules;
	delete _screen;
	delete _fpsCounter;

	NumberText::deleteStaticSurfaces();

	Mix_CloseAudio();

	SDL_Quit();
}

/**
 * The state machine takes care of passing all the events from SDL to the
 * active state, running any code within and blitting all the states and
 * cursor to the screen. This is run indefinitely until the game quits.
 */
void Game::run()
{
	enum ApplicationState
	{
		RUNNING,	// 0
		SLOWED,		// 1
		PAUSED		// 2
	} runningState = RUNNING;

	static const ApplicationState kbFocusRun[4] =
	{
		RUNNING,	// 0
		RUNNING,	// 0
		SLOWED,		// 1
		PAUSED		// 2
	};

	static const ApplicationState stateRun[4] =
	{
		SLOWED,		// 1
		PAUSED,		// 2
		PAUSED,		// 2
		PAUSED		// 2
	};

	// This will avoid processing SDL's resize event on startup, workaround for the heap allocation error it causes.
	bool startupEvent = Options::allowResize;

	while (_quit == false)
	{
		while (_deleted.empty() == false) // Clean up states
		{
			delete _deleted.back();
			_deleted.pop_back();
		}


		if (_init == false) // Initialize active state
		{
			_init = true;
			_states.back()->init();
			_states.back()->resetAll(); // Unpress buttons

			SDL_Event event; // Update mouse position
			int
				x,y;
			SDL_GetMouseState(&x,&y);
			event.type = SDL_MOUSEMOTION;
			event.motion.x = static_cast<Uint16>(x);
			event.motion.y = static_cast<Uint16>(y);
			Action action = Action(
								&event,
								_screen->getXScale(),
								_screen->getYScale(),
								_screen->getCursorTopBlackBand(),
								_screen->getCursorLeftBlackBand());
			_states.back()->handle(&action);
		}

		while (SDL_PollEvent(&_event) == 1) // Process SDL input events
		{
			if (_inputActive == false // kL->
				&& _event.type != SDL_MOUSEMOTION)
			{
//				_event.type = SDL_IGNORE; // discard buffered events
				continue;
			}

			if (CrossPlatform::isQuitShortcut(_event) == true)
				_event.type = SDL_QUIT;

			switch (_event.type)
			{
//				case SDL_IGNORE:
//				break;
				case SDL_QUIT:
					quit();
				break;

				case SDL_ACTIVEEVENT:
					switch (reinterpret_cast<SDL_ActiveEvent*>(&_event)->state)
					{
						case SDL_APPACTIVE:
							runningState = reinterpret_cast<SDL_ActiveEvent*>(&_event)->gain ? RUNNING : stateRun[Options::pauseMode];
							break;
						case SDL_APPINPUTFOCUS:
							runningState = reinterpret_cast<SDL_ActiveEvent*>(&_event)->gain ? RUNNING : kbFocusRun[Options::pauseMode];
//							break;
//						case SDL_APPMOUSEFOCUS: // sub-Consciously ignore it.
//							break;
					}
				break;

				case SDL_VIDEORESIZE:
					if (Options::allowResize == true)
					{
						if (startupEvent == false)
						{
// G++ linker wants it this way ...
#ifdef _DEBUG
							const int
								screenWidth = Screen::ORIGINAL_WIDTH,
								screenHeight = Screen::ORIGINAL_HEIGHT;

							Options::newDisplayWidth = Options::displayWidth = std::max(
																					screenWidth,
																					_event.resize.w);
							Options::newDisplayHeight = Options::displayHeight = std::max(
																					screenHeight,
																					_event.resize.h);
#else
							Options::newDisplayWidth = Options::displayWidth = std::max(
																					Screen::ORIGINAL_WIDTH,
																					_event.resize.w);
							Options::newDisplayHeight = Options::displayHeight = std::max(
																					Screen::ORIGINAL_HEIGHT,
																					_event.resize.h);
#endif
							int
								dX = 0,
								dY = 0;

							Screen::updateScale(
											Options::battlescapeScale,
											Options::battlescapeScale,
											Options::baseXBattlescape,
											Options::baseYBattlescape,
											false);
							Screen::updateScale(
											Options::geoscapeScale,
											Options::geoscapeScale,
											Options::baseXGeoscape,
											Options::baseYGeoscape,
											false);

							for (std::list<State*>::const_iterator
									i = _states.begin();
									i != _states.end();
									++i)
							{
								(*i)->resize(dX, dY);
							}

							_screen->resetDisplay();
						}
						else
							startupEvent = false;
					}
				break;

				case SDL_MOUSEMOTION:
				case SDL_MOUSEBUTTONDOWN:
				case SDL_MOUSEBUTTONUP:
//					if (_inputActive == false) // Skip [ie. postpone] mouse events if they're disabled. Moved below_
//						continue;
					runningState = RUNNING;	// re-gain focus on mouse-over or keypress.
											// Go on, feed the event to others ->>>
				default:
					Action action = Action(
										&_event,
										_screen->getXScale(),
										_screen->getYScale(),
										_screen->getCursorTopBlackBand(),
										_screen->getCursorLeftBlackBand());
					_screen->handle(&action);
					_cursor->handle(&action);
					_fpsCounter->handle(&action);
					_states.back()->handle(&action);

					if ((SDL_GetModState() & KMOD_CTRL) != 0
						&& action.getDetails()->type == SDL_KEYDOWN)
					{
						if (action.getDetails()->key.keysym.sym == SDLK_g)			// "ctrl-g" grab input
						{
							Options::captureMouse = static_cast<SDL_GrabMode>(!Options::captureMouse);
							SDL_WM_GrabInput(Options::captureMouse);
						}
						else if (Options::debug == true)
						{
//							if (action.getDetails()->key.keysym.sym == SDLK_t)		// "ctrl-t" engage TestState
//								setState(new TestState());
//							else
							if (action.getDetails()->key.keysym.sym == SDLK_u)		// "ctrl-u" debug UI
							{
								Options::debugUi = !Options::debugUi;
								_states.back()->redrawText();
							}
							else if (_gameSave != nullptr
								&& _gameSave->getBattleSave() == nullptr			// TODO: Merge w/ GeoscapeState::handle() in Geoscape.
								&& _gameSave->getDebugGeo() == true)				// kL-> note: 'c' doubles as CreateInventoryTemplate (remarked @ InventoryState).
							{
								switch (action.getDetails()->key.keysym.sym)
								{
									case SDLK_c:									// ctrl+c is also handled in GeoscapeState::handle()
									{												// where decisions are made about what info to show.
										// "ctrl-c"									- increment to show next area's boundaries
										// "ctrl-shift-c"							- decrement to show previous area's boundaries
										// "ctrl-alt-c"								- toggles between show all areas' boundaries & show current area's boundaries (roughly)
										if ((SDL_GetModState() & KMOD_ALT) != 0)
											std::swap(_debugCycle, _debugCycle_b);
										else if ((SDL_GetModState() & KMOD_SHIFT) != 0)
										{
											if (_debugCycle != -1)
												--_debugCycle;
											else
												_debugCycle_b = -1;					// semi-convenient reset for Cycle_b .... hey, at least there *is* one.
										}
										else
											++_debugCycle;

										break;
									}
									case SDLK_l:									// "ctrl-l" reload country lines
										if (_rules != nullptr)
											_rules->reloadCountryLines();
								}
							}
						}
					}
			}
		}

//		if (_inputActive == false)	// kL->
		_inputActive = true;


		if (runningState != PAUSED) // Process rendering
		{
			_states.back()->think(); // Process logic
			_fpsCounter->think();

			if (!(Options::useOpenGL && Options::vSyncForOpenGL)
				&& Options::FPS > 0)
			{
				// Update FPS delay time based on the time of the last draw.
				int fps = (SDL_GetAppState() & SDL_APPINPUTFOCUS) ? Options::FPS : Options::FPSInactive;
				if (fps < 1) fps = 1;
				_ticksTillNextSlice = static_cast<int>(
									 1000.f / static_cast<float>(fps)
										- static_cast<float>(SDL_GetTicks() - _tickOfLastSlice));
			}
			else
				_ticksTillNextSlice = 0;

			if (_ticksTillNextSlice < 1 && _init == true)
			{
				_tickOfLastSlice = SDL_GetTicks(); // store when this slice occurred.
				_fpsCounter->addFrame();

				_screen->clear();

				if (_blitDelay == true)
				{
					_blitDelay = false;
					SDL_Delay(369);
				}

				std::list<State*>::const_iterator i = _states.end();
				do
				{
					--i; // find top underlying fullscreen state,
				}
				while (i != _states.begin() && (*i)->isFullScreen() == false);

				for (
						;
						i != _states.end();
						++i)
				{
					(*i)->blit(); // blit top underlying fullscreen state and those on top of it.
				}

				_fpsCounter->blit(_screen->getSurface());
				_cursor->blit(_screen->getSurface());

				_screen->flip();
			}
		}


		switch (runningState)	// Save on CPU
		{
			case RUNNING:
				SDL_Delay(1);	// Save CPU from going 100%
			break;
			case SLOWED:
			case PAUSED:
				SDL_Delay(100);	// More slowing down.
		}
	}

//	Options::save();	// kL_note: why this work here but not in main() EXIT,
						// where it clears & rewrites my options.cfg
						// Ps. why are they even doing Options::save() twice
						// ... now they both fuck up. BYE!
}

/**
 * Stops the state machine and the game is shut down.
 * @param force - true to force Quit after an error message (default false)
 */
void Game::quit(bool force)
{
	if (force == false
		&& _gameSave != nullptr // Always save ironman
		&& _gameSave->isIronman() == true
		&& _gameSave->getName().empty() == false)
	{
		const std::string file = CrossPlatform::sanitizeFilename(Language::wstrToFs(_gameSave->getName())) + ".sav";
		_gameSave->save(file);
	}

	_quit = true;
}

/**
 * Changes the audio volume of the music and sound effect channels.
 * @param sound	- sound volume, from 0 to MIX_MAX_VOLUME
 * @param music	- music volume, from 0 to MIX_MAX_VOLUME
 * @param ui	- ui volume, from 0 to MIX_MAX_VOLUME
 */
void Game::setVolume(
		int sound,
		int music,
		int ui)
{
	if (Options::mute == false)
	{
		if (music > -1)
		{
			music = static_cast<int>(volExp(music) * static_cast<double>(SDL_MIX_MAXVOLUME));
			Mix_VolumeMusic(music);
//			func_set_music_volume(music);
		}

		if (sound > -1)
		{
			sound = static_cast<int>(volExp(sound) * static_cast<double>(SDL_MIX_MAXVOLUME));
			Mix_Volume(-1, sound);		// sets volume on *all channels*
			Mix_Volume(3, sound / 2);	// channel 3: reserved for ambient sound effect.
		}

		if (ui > -1)
		{
			ui = static_cast<int>(volExp(ui) * static_cast<double>(SDL_MIX_MAXVOLUME));
			// ... they use channels #1 & #2 btw. and group them accordingly in initAudio() below_
			Mix_Volume(0, ui); // this sets channel-0 to ui-Volume
			Mix_Volume(1, ui); // and this sets channel-1 to ui-Volume!
			Mix_Volume(2, ui); // and this sets channel-2 to ui-Volume!
		}
	}
}

/**
 * Adjusts a linear volume level to an exponential one.
 * @param vol - volume to adjust
 * @return, adjusted volume
 */
double Game::volExp(int vol)
{
	return (std::exp(std::log(Game::VOLUME_GRADIENT + 1.) * static_cast<double>(vol) / static_cast<double>(SDL_MIX_MAXVOLUME)) - 1.)
		   / Game::VOLUME_GRADIENT;
}
// VOLUME_GRADIENT   = 10.
// SDL_MIX_MAXVOLUME = 128
//
// if vol=256, out = 0.70267954037450624769969098909243 <- overMax in
// if vol=128, out = 0.18331599679059886217106240983842
// if vol= 64, out = 0.068319932506699533244707537053418
// if vol= 32, out = 0.029738171910467249825497157272772
// if vol= 16, out = 0.0139026654255585231541758004438
// if vol=  8, out = 0.0067251916960370341756990957283772
// if vol=  4, out = 0.0033078853215169796293706980006635
// if vol=  2, out = 0.0038177854627124397781279131961105

/**
 * Returns the display screen used by the game.
 * @return, pointer to the Screen
 */
Screen* Game::getScreen() const
{
	return _screen;
}

/**
 * Returns the mouse cursor used by the game.
 * @return, pointer to the Cursor
 */
Cursor* Game::getCursor() const
{
	return _cursor;
}

/**
 * Returns the FpsCounter used by the game.
 * @return, pointer to the FpsCounter
 */
FpsCounter* Game::getFpsCounter() const
{
	return _fpsCounter;
}

/**
 * Pops all the states currently in stack and pushes in the new state.
 * @note A shortcut for cleaning up all the old states when they're not
 * necessary like in one-way transitions.
 * @param state - pointer to the new State
 */
void Game::setState(State* const state)
{
	while (_states.empty() == false)
		popState();

	pushState(state);

	_init = false;
}

/**
 * Pushes a new state into the top of the stack and initializes it.
 * @note The new state will be used once the next game cycle starts.
 * @param state - pointer to the new State
 */
void Game::pushState(State* const state)
{
	_states.push_back(state);
	_init = false;
}

/**
 * Pops the last state from the top of the stack.
 * @note Since states can't actually be deleted mid-cycle it's moved into a
 * separate queue which is cleared at the start of every cycle so the transition
 * is seamless.
 */
void Game::popState()
{
	_deleted.push_back(_states.back());
	_states.pop_back();
	_init = false;
}

/**
 * Gets the current (top) State.
 * @return, current state
 */
/* State* Game::getState() const
{
	if (_states.empty() == false)
		return _states.back();

	return nullptr;
} */

/**
 * Gets the quantity of currently running states.
 * @return, qty of states
 */
int Game::getQtyStates() const
{
	return _states.size();
}

/**
 * Returns whether current state is *state
 * @param state - pointer to a State to test against the stack state
 * @return, true if *state is the current state
 */
bool Game::isState(State* const state) const
{
	return _states.empty() == false
		&& _states.back() == state;
}

/**
 * Checks if the game is currently quitting.
 * @return, true if the game is in the process of shutting down
 */
bool Game::isQuitting() const
{
	return _quit;
}

/**
 * Returns the Language currently in use by the game.
 * @return, pointer to the Language
 */
Language* Game::getLanguage() const
{
	return _lang;
}

/**
* Changes the Language currently in use by the game.
* @param file - reference the name of the language file
*/
void Game::loadLanguage(const std::string& file)
{
	std::ostringstream oststr;
	oststr << "Language/" << file << ".yml";

	ExtraStrings* extras (nullptr);

	std::map<std::string, ExtraStrings*> extraStrings (_rules->getExtraStrings());
	if (extraStrings.empty() == false)
	{
		if (extraStrings.find(file) != extraStrings.end())
			extras = extraStrings[file];
		else if (extraStrings.find("en-US") != extraStrings.end()) // fallbacks ->
			extras = extraStrings["en-US"];
		else if (extraStrings.find("en-GB") != extraStrings.end())
			extras = extraStrings["en-GB"];
		else
			extras = extraStrings.begin()->second;
	}

	const std::string path (CrossPlatform::getDataFile(oststr.str()));
	try
	{
		_lang->load(path, extras);
	}
	catch (YAML::Exception &e)
	{
		throw Exception(path + ": " + std::string(e.what()));
	}

	Options::language = file;
}

/**
 * Returns the resource pack currently in use by the game.
 * @return, pointer to the ResourcePack
 */
ResourcePack* Game::getResourcePack() const
{
	return _res;
}

/**
 * Sets a new resource pack for the game to use.
 * @param res - pointer to the ResourcePack
 */
void Game::setResourcePack(ResourcePack* const res)
{
	delete _res;
	_res = res;
}

/**
 * Returns the saved game currently in use by the game.
 * @return, pointer to the SavedGame
 */
SavedGame* Game::getSavedGame() const
{
	return _gameSave;
}

/**
 * Sets a new saved game for the game to use.
 * @param save - pointer to the SavedGame
 */
void Game::setSavedGame(SavedGame* const save)
{
	delete _gameSave;
	_gameSave = save;
}

/**
 * Returns the ruleset currently in use by the game.
 * @return, pointer to the Ruleset
 */
Ruleset* Game::getRuleset() const
{
	return _rules;
}

/**
 * Loads the rulesets specified in the game options.
 */
void Game::loadRuleset()
{
	Options::badMods.clear();

	_rules = new Ruleset(this);

	if (Options::rulesets.empty() == true)
		Options::rulesets.push_back("Xcom1Ruleset");

	for (std::vector<std::string>::const_iterator
			i = Options::rulesets.begin();
			i != Options::rulesets.end();
			)
	{
		try
		{
			_rules->load(*i);
			++i;
		}
		catch (YAML::Exception& e)
		{
			Log(LOG_WARNING) << e.what();

			Options::badMods.push_back(*i);
			Options::badMods.push_back(e.what());

			i = Options::rulesets.erase(i);
		}
	}

	if (Options::rulesets.empty() == true)
	{
		throw Exception("Failed to load ruleset");
	}

	// Prints listOrder to LOG.
/*	Log(LOG_INFO) << "\n";
	std::vector<std::string> pedList = _rules->getUfopaediaList();
	for (std::vector<std::string>::const_iterator
			i = pedList.begin();
			i != pedList.end();
			++i)
	{
		Log(LOG_INFO) << *i;
	} */

	_rules->validateMissions();
	_rules->sortLists();

	_rules->convertInventories();

	// Prints listOrder to LOG.
/*	Log(LOG_INFO) << "\n";
	std::vector<std::string> pedList = _rules->getUfopaediaList();
	for (std::vector<std::string>::const_iterator
			i = pedList.begin();
			i != pedList.end();
			++i)
	{
		Log(LOG_INFO) << *i;
	}
	Log(LOG_INFO) << "\n"; */
}

/**
 * Sets whether SDL input is activated.
 * @note All input events are processed if true otherwise only mouse motion is
 * handled.
 * @param active - true if SDL input is active
 */
void Game::setInputActive(bool active)
{
	_inputActive = active;
}

/**
 * Loads the most appropriate language given current system and game options.
 */
void Game::defaultLanguage()
{
	const std::string defaultLang = "en-US";

	if (Options::language.empty() == true) // No language set, detect based on system
	{
		const std::string
			local = CrossPlatform::getLocale(),
			lang = local.substr(
							0,
							local.find_first_of('-'));

		try // Try to load full locale
		{
			loadLanguage(local);
		}
		catch (std::exception)
		{
			try // Try to load language locale
			{
				loadLanguage(lang);
			}
			catch (std::exception) // Give up, use default
			{
				loadLanguage(defaultLang);
			}
		}
	}
	else
	{
		try // Use options language
		{
			loadLanguage(Options::language);
		}
		catch (std::exception) // Language not found, use default
		{
			loadLanguage(defaultLang);
		}
	}
}

/**
 * Initializes the audio subsystem.
 */
void Game::initAudio()
{
	Uint16 audioFormat;
	if (Options::audioBitDepth == 8)
		audioFormat = AUDIO_S8;
	else
		audioFormat = AUDIO_S16SYS;

	const int coefBuffer = std::max(1,
									Options::audioBuffer1k);
	if (Mix_OpenAudio(
				Options::audioSampleRate,
				audioFormat,
				2,
				1024 * coefBuffer) != 0)
	{
		Log(LOG_WARNING) << "No sound device detected, audio disabled.";
		Log(LOG_ERROR) << Mix_GetError();
		Options::mute = true;
	}
	else
	{
		Log(LOG_INFO) << "SDL_mixer initialized.";
		Mix_AllocateChannels(16);
		Mix_ReserveChannels(4); // 4th channel (#3) is for ambient sFx
		Mix_GroupChannels(0,2,0);

		setVolume(
				Options::soundVolume,
				Options::musicVolume,
				Options::uiVolume);
	}
}

/**
 * Gets the country cycle for debugging country regions.
 * @return, the current country cycle setting
 */
int Game::getDebugCycle() const
{
	return _debugCycle;
}

/**
 * Sets the country cycle for debugging country regions.
 * @param cycle - the country cycle to set
 */
void Game::setDebugCycle(const int cycle)
{
	_debugCycle = cycle;
}

/**
 * Causes the engine to delay blitting top state.
 */
void Game::delayBlit()
{
	_blitDelay = true;
}

}
