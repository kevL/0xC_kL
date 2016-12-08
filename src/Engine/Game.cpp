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

//#include <algorithm>	// std::max(), std::swap()
//#include <cmath>		// std::exp(), std::log()
//#include <exception>	// std::exception
//#include <map>		// std::map
//#include <sstream>	// std::ostringstream
//#include <vector>		// std::vector

#include <SDL/SDL_mixer.h>

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

//#include "../Menu/TestState.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/Ruleset.h"

#include "../Savegame/SavedGame.h"


namespace OpenXcom
{

const double Game::VOLUME_GRADIENT (10.); // static.

SDL_Event
	Game::eventD, // static.
	Game::eventU; // static.

Action
	Game::syntheticD (Action(&eventD, 0.,0., 0,0)), // static.
	Game::syntheticU (Action(&eventU, 0.,0., 0,0)); // static.

/**
 * Starts up SDL with all the subsystems and SDL_mixer for audio-processing,
 * creates the display-screen and sets up the cursor.
 * @param title - reference to the title of the app-window
 */
Game::Game(const std::string& title)
	:
		_res(nullptr),
		_gameSave(nullptr),
		_rules(nullptr),
		_quit(false),
		_init(true),
		_inputActive(true),
		_ticksTillNextSlice(0),
		_tickOfLastSlice(0u),
		_debugCycle(-1),
		_debugCycle_b(-1),
		_rodentState(0u)
{
	Options::reload = false;
#ifdef _DEBUG
	Options::mute = true;
#endif

	// Initialize SDL.
	if (SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		throw Exception(SDL_GetError());
	}
	Log(LOG_INFO) << "SDL initialized.";

	// Initialize SDL_mixer.
	if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0)
	{
		Log(LOG_ERROR) << SDL_GetError();
		Log(LOG_WARNING) << "No sound device detected, audio disabled.";
		Options::mute = true;
	}
	else
		initAudio();

#ifdef _DEBUG
	SDL_WM_GrabInput(SDL_GRAB_OFF);
#else
	// trap the mouse inside the window (if *not* _DEBUG)
	SDL_WM_GrabInput(Options::captureMouse);
#endif

	// Set the app-icon.
#ifdef _WIN32
	CrossPlatform::setWindowIcon(2);
#else
	CrossPlatform::setWindowIcon("openxcom.png");
#endif

	// Set the window caption
	SDL_WM_SetCaption(title.c_str(), nullptr);

	SDL_EnableUNICODE(1);

	// Create display.
	_screen = new Screen();
/*	Screen::BASE_WIDTH  = Options::getInt("baseXResolution");
	Screen::BASE_HEIGHT = Options::getInt("baseYResolution");
	_screen = new Screen(
					Options::getInt("displayWidth"),
					Options::getInt("displayHeight"), 0,
					Options::getBool("fullscreen"),
					Options::getInt("windowedModePositionX"),
					Options::getInt("windowedModePositionY")); */ // kL

	// Create cursor.
	_cursor = new Cursor(9,13);
	// Create invisible hardware-cursor to workaround a bug with absolute
	// positioning pointing devices.
	SDL_ShowCursor(SDL_DISABLE);
	Uint8 cursor (0u);
	SDL_SetCursor(SDL_CreateCursor(
								&cursor, &cursor,
								1,1,0,0));

	// Create FPS-counter.
	_fpsCounter = new FpsCounter(15,5);

	// Create blank Language.
	_lang = new Language();


	// Create the synthetic mouse-events.
	eventD.type = SDL_MOUSEBUTTONDOWN;
	eventD.button.button = SDL_BUTTON_LEFT;
	syntheticD = Action(&eventD, 0.,0., 0,0);

	eventU.type = SDL_MOUSEBUTTONUP;
	eventU.button.button = SDL_BUTTON_LEFT;
	syntheticU = Action(&eventU, 0.,0., 0,0);
}

/**
 * Deletes the display-screen, cursor, states, etc etc etc, and shuts down all
 * the SDL-subsystems.
 */
Game::~Game()
{
	Sound::stop();
	Music::stop();

	for (std::list<State*>::const_iterator
			i = _states.begin();
			i != _states.end();
			++i)
		delete *i;

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
 * The state-machine takes care of passing all the events from SDL to the
 * active State, running any code within and blitting all the States and
 * Cursor to the screen. This is run indefinitely until the game quits. VrooooM ->
 */
void Game::run()
{
	enum ApplicationState
	{
		STANDARD,	// 0
		SLOWED,		// 1
		PAUSED		// 2
	} runState = STANDARD;

	static const ApplicationState appFocusLost[4u]
	{
		SLOWED,		// 0 - could change to STANDARD
		PAUSED,		// 1 - could change to SLOWED
		PAUSED,		// 2
		PAUSED,		// 3
	};

	static const ApplicationState keyboardFocusLost[4u]
	{
		STANDARD,	// 0
		STANDARD,	// 1
		SLOWED,		// 2
		PAUSED		// 3
	};

	// This will avoid processing SDL's resize-event on startup, a workaround
	// for the heap-allocation-error it causes.
	bool startupEvent (Options::allowResize == true);

	static SDL_Event event;

	if (Options::engineLooper == "roadrunner")
	{
		Log(LOG_INFO) << "Starting roadrunner engine.";
		while (_quit == false)
		{
			while (_deleted.empty() == false)		// clean up States
			{
				delete _deleted.back();
				_deleted.pop_back();
			}

			if (_init == true)						// initialize the active State
			{
				_init = false;
				_states.back()->resetSurfaces();	// unpress buttons -- NOTE: This goes before init() if you want '_isFocused' to toggle properly.
				_states.back()->init();				// Globe (geoscape) OR Map (battlescape) will reset (_isFocused=TRUE) here. If you want it to.

				int									// update mouse-position
					x,y;
				SDL_GetMouseState(&x,&y);
				event.type = SDL_MOUSEMOTION;
				event.motion.x = static_cast<Uint16>(x);
				event.motion.y = static_cast<Uint16>(y);
				Action action (Action(
									&event,
									_screen->getScaleX(),
									_screen->getScaleY(),
									_screen->getBorderTop(),
									_screen->getBorderLeft()));
//									_rodentState));
				_states.back()->handle(&action);
			}

			while (SDL_PollEvent(&event) == 1)		// process SDL input-events
			{
				if (_inputActive == false && event.type != SDL_MOUSEMOTION)
				{
//					event.type = SDL_IGNORE;		// discard buffered events
					continue;
				}

				if (CrossPlatform::isQuitShortcut(event) == true)
					event.type = SDL_QUIT;

				switch (event.type)
				{
//					case SDL_IGNORE: break;
					case SDL_QUIT: quit(); break;

					case SDL_VIDEORESIZE:
						if (Options::allowResize == true)
						{
							if (startupEvent == true)
								startupEvent = false;
							else
							{
// G++ linker wants it this way ...
//#ifdef _DEBUG
								const int
									screenWidth  (Screen::ORIGINAL_WIDTH),
									screenHeight (Screen::ORIGINAL_HEIGHT);

								Options::safeDisplayWidth	=
								Options::displayWidth		= std::max(screenWidth,
																	   event.resize.w);
								Options::safeDisplayHeight	=
								Options::displayHeight		= std::max(screenHeight,
																	   event.resize.h);
//#else
//								Options::safeDisplayWidth	=
//								Options::displayWidth		= std::max(Screen::ORIGINAL_WIDTH,
//																	   event.resize.w);
//								Options::safeDisplayHeight	=
//								Options::displayHeight		= std::max(Screen::ORIGINAL_HEIGHT,
//																	   event.resize.h);
//#endif
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
								int
									dX (0),
									dY (0);
								for (std::list<State*>::const_iterator
										i = _states.begin();
										i != _states.end();
										++i)
									(*i)->resize(dX, dY);

								_screen->resetDisplay();
							}
						}
						break;

//					case SDL_MOUSEBUTTONDOWN:
//					case SDL_MOUSEBUTTONUP:
//						if (   static_cast<Uint32>(event.button.button) != 0u
//							&& static_cast<Uint32>(event.button.button) < 32u) // uhhhh i don't have a 32-button mouse.
//						{
//							switch (event.button.state)
//							{
//								case SDL_PRESSED:
//									_rodentState |= SDL_BUTTON(event.button.button);
//									break;
//								case SDL_RELEASED:
//									_rodentState &= !SDL_BUTTON(event.button.button);
//							}
//						}
						// no break;

//					case SDL_MOUSEMOTION:
					default:
						Action action (Action(
											&event,
											_screen->getScaleX(),
											_screen->getScaleY(),
											_screen->getBorderTop(),
											_screen->getBorderLeft()));
//											_rodentState));
						_screen->handle(&action);
						_cursor->handle(&action);
						_fpsCounter->handle(&action);

						if (action.getDetails()->type == SDL_KEYDOWN
							&& (SDL_GetModState() & KMOD_CTRL) != 0)
						{
							const SDLKey key (action.getDetails()->key.keysym.sym);
							switch (key)
							{
								case SDLK_g: // "ctrl-g" grab input
									Options::captureMouse = static_cast<SDL_GrabMode>(!Options::captureMouse);
									SDL_WM_GrabInput(Options::captureMouse);
									break;

								case SDLK_u: // "ctrl-u" debug UI
									Options::debugUi = !Options::debugUi;
									_states.back()->redrawText();
									break;

								case SDLK_m: // "ctrl-m" mute
									if (Options::mute = !Options::mute)
										setVolume(0,0,-1, true);
									else
										setVolume(
												Options::volMusic,
												Options::volFx,
												Options::volUi);
									break;

								default:
									if (Options::debug == true
										&& _gameSave != nullptr
										&& _gameSave->getBattleSave() == nullptr	// TODO: Merge w/ GeoscapeState::handle() in Geoscape.
										&& _gameSave->getDebugGeo() == true)		// kL-> note: 'c' doubles as CreateInventoryTemplate (remarked @ InventoryState).
									{
										switch (key)
										{
											case SDLK_c: // <-- also handled in GeoscapeState::handle() where decisions are made about what info to show.
												// "ctrl-c"			- increment to show next area's boundaries
												// "ctrl-shift-c"	- decrement to show previous area's boundaries
												// "ctrl-alt-c"		- toggles between show all areas' boundaries & show current area's boundaries (roughly)
												if ((SDL_GetModState() & KMOD_ALT) != 0)
													std::swap(_debugCycle, _debugCycle_b);
												else if ((SDL_GetModState() & KMOD_SHIFT) != 0)
												{
													switch (_debugCycle)
													{
														case -1:
															_debugCycle_b = -1; // semi-convenient reset for Cycle_b .... hey, at least there *is* one.
															break;
														default:
															--_debugCycle;
													}
												}
												else
													++_debugCycle;
												break;

											case SDLK_l: // "ctrl-l" reload country lines
												if (_rules != nullptr)
													_rules->reloadCountryLines();
										}
									}
							}
						}

						_states.back()->handle(&action);
				} // end event-type switch.
			} // end polling loop.

			_inputActive = true;

			_states.back()->think();					// process logic
			_fpsCounter->think();

			if (_init == false)							// process rendering
			{
#ifdef _DEBUG
				// The player's ActionMenu won't blit/draw/flip without this. g++ (4.2) debug configuration
				if (false) { Log(LOG_INFO) << "go fucking figure."; }
#endif

				_fpsCounter->addFrame();

				_screen->clear();

				std::list<State*>::const_iterator i (_states.end());
				do
				{
					--i;								// find top underlying fullscreen-state
				}
				while (i != _states.begin() && (*i)->isFullScreen() == false);

				for (
						;
						i != _states.end();
						++i)
				{
					(*i)->blit();						// blit top underlying fullscreen-state and those on top of it
				}

				_fpsCounter->blit(_screen->getSurface());
				_cursor->blit(_screen->getSurface());

				_screen->flip();
			}

			SDL_Delay(1u);
		} // end run loop.
	}
	else // anything other than Options::engineLooper= "roadrunner"
	{
		Log(LOG_INFO) << "Starting wilecoyote engine.";
		while (_quit == false)
		{
			while (_deleted.empty() == false)		// clean up States
			{
				delete _deleted.back();
				_deleted.pop_back();
			}

			if (_init == true)						// initialize active State
			{
				_init = false;
				_states.back()->resetSurfaces();	// unpress buttons -- NOTE: This goes before init() if you want '_isFocused' to toggle properly.
				_states.back()->init();				// Globe (geoscape) OR Map (battlescape) will reset (_isFocused=TRUE) here. If you want it to.

				int									// update mouse-position
					x,y;
				SDL_GetMouseState(&x,&y);
				event.type = SDL_MOUSEMOTION;
				event.motion.x = static_cast<Uint16>(x);
				event.motion.y = static_cast<Uint16>(y);
				Action action (Action(
									&event,
									_screen->getScaleX(),
									_screen->getScaleY(),
									_screen->getBorderTop(),
									_screen->getBorderLeft()));
//									_rodentState));
				_states.back()->handle(&action);
			}

			while (SDL_PollEvent(&event) == 1)		// process SDL input-events
			{
				if (_inputActive == false && event.type != SDL_MOUSEMOTION)
				{
//					event.type = SDL_IGNORE;		// discard buffered events
					continue;
				}

				if (CrossPlatform::isQuitShortcut(event) == true)
					event.type = SDL_QUIT;

				switch (event.type)
				{
//					case SDL_IGNORE: break;
					case SDL_QUIT: quit(); break;

					case SDL_VIDEORESIZE:
						if (Options::allowResize == true)
						{
							if (startupEvent == true)
								startupEvent = false;
							else
							{
// G++ linker wants it this way ...
//#ifdef _DEBUG
								const int
									screenWidth  (Screen::ORIGINAL_WIDTH),
									screenHeight (Screen::ORIGINAL_HEIGHT);

								Options::safeDisplayWidth	=
								Options::displayWidth		= std::max(screenWidth,
																	   event.resize.w);
								Options::safeDisplayHeight	=
								Options::displayHeight		= std::max(screenHeight,
																	   event.resize.h);
//#else
//								Options::safeDisplayWidth	=
//								Options::displayWidth		= std::max(Screen::ORIGINAL_WIDTH,
//																	   event.resize.w);
//								Options::safeDisplayHeight	=
//								Options::displayHeight		= std::max(Screen::ORIGINAL_HEIGHT,
//																	   event.resize.h);
//#endif
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
								int
									dX (0),
									dY (0);
								for (std::list<State*>::const_iterator
										i = _states.begin();
										i != _states.end();
										++i)
									(*i)->resize(dX, dY);

								_screen->resetDisplay();
							}
						}
						break;

					case SDL_ACTIVEEVENT:
						switch (reinterpret_cast<SDL_ActiveEvent*>(&event)->state) // NOTE: Neither of these always want to *re-gain* focus.
						{
							case SDL_APPACTIVE:		// 0xC app is minimized or restored NOTE: Getting a response from this only when the taskbar's context menu is open for this app.
								runState = (reinterpret_cast<SDL_ActiveEvent*>(&event)->gain) ? STANDARD : appFocusLost[Options::pauseMode];
								break;

							case SDL_APPINPUTFOCUS:	// 0xC app loses or gains focus NOTE: This seems reversed w/ SDL_APPACTIVE.
								runState = (reinterpret_cast<SDL_ActiveEvent*>(&event)->gain) ? STANDARD : keyboardFocusLost[Options::pauseMode];
//								break;

//							case SDL_APPMOUSEFOCUS: // 0xC app gains or loses mouse-over; sub-Consciously ignore it.
//								break;
						}
						break; // NOTE: That's just bad news. On WinXP here.

					case SDL_MOUSEBUTTONDOWN:
					case SDL_MOUSEBUTTONUP:
//						if (   static_cast<Uint32>(event.button.button) != 0u
//							&& static_cast<Uint32>(event.button.button) < 32u) // uhhhh i don't have a 32-button mouse.
//						{
//							switch (event.button.state)
//							{
//								case SDL_PRESSED:
//									_rodentState |=  (1u << (static_cast<Uint32>(event.button.button) - 1u));
//									break;
//								case SDL_RELEASED:
//									_rodentState &= !(1u << (static_cast<Uint32>(event.button.button) - 1u));
//							}
//						}
						// no break;

					case SDL_MOUSEMOTION:
						runState = STANDARD;		// re-gain focus on mouse-over or mouse-press.
						// no break;				// feed the event to others ->>>

					default:
						Action action (Action(
											&event,
											_screen->getScaleX(),
											_screen->getScaleY(),
											_screen->getBorderTop(),
											_screen->getBorderLeft()));
//											_rodentState));
						_screen->handle(&action);
						_cursor->handle(&action);
						_fpsCounter->handle(&action);

						if (action.getDetails()->type == SDL_KEYDOWN
							&& (SDL_GetModState() & KMOD_CTRL) != 0)
						{
							const SDLKey key (action.getDetails()->key.keysym.sym);
							switch (key)
							{
								case SDLK_g: // "ctrl-g" grab input
									Options::captureMouse = static_cast<SDL_GrabMode>(!Options::captureMouse);
									SDL_WM_GrabInput(Options::captureMouse);
									break;

								case SDLK_u: // "ctrl-u" debug UI
									Options::debugUi = !Options::debugUi;
									_states.back()->redrawText();
									break;

								case SDLK_m: // "ctrl-m" mute
									if (Options::mute = !Options::mute)
										setVolume(0,0,-1, true);
									else
										setVolume(
												Options::volMusic,
												Options::volFx,
												Options::volUi);
									break;

								default:
									if (Options::debug == true)
									{
										switch (key)
										{
//											case SDLK_t: // "ctrl-t" engage TestState
//												setState(new TestState());
//												break;

											default:
												if (_gameSave != nullptr
													&& _gameSave->getBattleSave() == nullptr	// TODO: Merge w/ GeoscapeState::handle() in Geoscape.
													&& _gameSave->getDebugGeo() == true)		// kL-> note: 'c' doubles as CreateInventoryTemplate (remarked @ InventoryState).
												{
													switch (key)
													{
														case SDLK_c: // <-- also handled in GeoscapeState::handle() where decisions are made about what info to show.
															// "ctrl-c"			- increment to show next area's boundaries
															// "ctrl-shift-c"	- decrement to show previous area's boundaries
															// "ctrl-alt-c"		- toggles between show all areas' boundaries & show current area's boundaries (roughly)
															if ((SDL_GetModState() & KMOD_ALT) != 0)
																std::swap(_debugCycle, _debugCycle_b);
															else if ((SDL_GetModState() & KMOD_SHIFT) != 0)
															{
																switch (_debugCycle)
																{
																	case -1:
																		_debugCycle_b = -1; // semi-convenient reset for Cycle_b .... hey, at least there *is* one.
																		break;
																	default:
																		--_debugCycle;
																}
															}
															else
																++_debugCycle;
															break;

														case SDLK_l: // "ctrl-l" reload country lines
															if (_rules != nullptr)
																_rules->reloadCountryLines();
													}
												}
										}
									}
							}
						}

						_states.back()->handle(&action);
				} // end event-type switch.
			} // end polling loop.

			_inputActive = true;


			if (runState != PAUSED)
			{
				_states.back()->think();							// process logic
				_fpsCounter->think();

				if (_init == false)									// process rendering
				{
					if (Options::FPS != 0							// update slice-delay-time based on the time of the last draw
						&& !(Options::useOpenGL == true
							&& Options::vSyncForOpenGL == true))	// if not using OpenGL or if not using OpenGL vSync
					{
						int fpsUser;
						if ((SDL_GetAppState() & SDL_APPINPUTFOCUS))
							fpsUser = Options::FPS;
						else
							fpsUser = Options::FPSUnfocused;

						if (fpsUser < 1) fpsUser = 1; // safety.
						_ticksTillNextSlice = static_cast<int>(
											  1000.f / static_cast<float>(fpsUser)
											  - static_cast<float>(SDL_GetTicks() - _tickOfLastSlice));
					}
					else
						_ticksTillNextSlice = 0;

					if (_ticksTillNextSlice < 1)
					{
						_tickOfLastSlice = SDL_GetTicks();			// store when this slice occurred.
						_fpsCounter->addFrame();

						_screen->clear();

						std::list<State*>::const_iterator i (_states.end());
						do
						{
							--i;									// find top underlying fullscreen-state
						}
						while (i != _states.begin() && (*i)->isFullScreen() == false);

						for (
								;
								i != _states.end();
								++i)
						{
							(*i)->blit();							// blit top underlying fullscreen-state and those on top of it
						}

						_fpsCounter->blit(_screen->getSurface());
						_cursor->blit(_screen->getSurface());

						_screen->flip();
					}
				}
			}

			switch (runState)			// save on CPU
			{
				case STANDARD:
					SDL_Delay(1u);		// save CPU from going 100%
					break;
				case SLOWED:
				case PAUSED:
					SDL_Delay(100u);	// more slowing down
			}
		} // end run loop.
	}

//	Options::save();	// kL_note: why this work here but not in main() EXIT,
						// where it clears & rewrites my options.cfg
						// Ps. why are they even doing Options::save() twice
						// ... now they both fuck up. BYE!
}

/**
 * Stops the state-machine and this Game is shut down.
 * @param force - true to force-quit after an error-message (default false)
 */
void Game::quit(bool force)
{
	if (force == false
		&& _gameSave != nullptr // always save ironman
		&& _gameSave->isIronman() == true
		&& _gameSave->getLabel().empty() == false)
	{
		const std::string file (CrossPlatform::sanitizeFilename(Language::wstrToFs(_gameSave->getLabel())));
		_gameSave->save(file + SavedGame::SAVE_EXT);
	}
	_quit = true;
}

/**
 * Checks if this Game is quitting.
 * @return, true if the game is in the process of shutting down
 */
bool Game::isQuitting() const
{
	return _quit;
}

/**
 * Sets whether SDL-input is activated.
 * @note All input-events are processed if true otherwise only mouse-motion is
 * handled.
 * @param active - true if SDL-input is active/responsive
 */
void Game::setInputActive(bool active)
{
	_inputActive = active;
}

/**
 * Pops all the states currently in the state-stack and pushes in a new state.
 * @note A shortcut for cleaning up all the old states when they're not
 * necessary like in one-way transitions.
 * @param state - pointer to the new State
 */
void Game::setState(State* const state)
{
	while (_states.empty() == false)
		popState();

	pushState(state);
	_init = true;
}

/**
 * Pushes a new state into the top of the state-stack and initializes it.
 * @note The new state will be used once the next run-cycle starts.
 * @param state - pointer to the new State
 */
void Game::pushState(State* const state)
{
	_states.push_back(state);
	_init = true;
}

/**
 * Pops the last state from the top of the state-stack.
 * @note Since states can't actually be deleted mid-cycle it's moved into a
 * separate queue which is cleared at the start of every cycle so the transition
 * should be seamless.
 */
void Game::popState()
{
	_deleted.push_back(_states.back());
	_states.pop_back();
	_init = true;
}

/**
 * Gets the current (top) State.
 * @return, current state
 *
State* Game::getState() const
{
	if (_states.empty() == false)
		return _states.back();

	return nullptr;
} */

/**
 * Gets the quantity of currently running states.
 * @return, qty of states
 */
size_t Game::getQtyStates() const
{
	return _states.size();
}

/**
 * Checks whether a state is the current state.
 * @param state - pointer to a State to test against the stack-state
 * @return, true if current
 */
bool Game::isState(const State* const state) const
{
	return _states.empty() == false
		&& _states.back() == state;
}

/**
 * Initializes the audio-subsystem.
 */
void Game::initAudio()
{
	Uint16 audioFormat;
	switch (Options::audioBitDepth)
	{
		case 8:  audioFormat = AUDIO_S8; break;
		default: audioFormat = AUDIO_S16SYS;
	}

	if (Options::audioSampleRate >= 44100)
		Options::audioChunkSize = std::max(2048, Options::audioChunkSize);
	else if (Options::audioSampleRate >= 22050)
		Options::audioChunkSize = std::max(1028, Options::audioChunkSize);
	else if (Options::audioSampleRate >= 11025)
		Options::audioChunkSize = std::max(512, Options::audioChunkSize);
	else
		Options::audioChunkSize = std::max(256, Options::audioChunkSize);

	if (Mix_OpenAudio(
				Options::audioSampleRate,
				audioFormat,
				2,
				Options::audioChunkSize) != 0)
	{
		Log(LOG_WARNING) << "No sound device detected, audio disabled.";
		Log(LOG_ERROR) << Mix_GetError();
		Options::mute = true;
	}
	else
	{
		Mix_AllocateChannels(16);
		Mix_ReserveChannels(3); // reserve channels 0..2 for UI
		Mix_GroupChannels(
						0,	// low-channel
						2,	// high-channel
						0);	// channel-group for UI

		setVolume(
				Options::volMusic,
				Options::volFx,
				Options::volUi);
		Log(LOG_INFO) << "SDL_mixer initialized.";
	}
}

/**
 * Changes the audio-volume of the music and sound-effects channels.
 * @note Range is from 0 to MIX_MAX_VOLUME.
 * @param music	- music-volume
 * @param fx	- fx-volume
 * @param ui	- ui-volume (default -1)
 * @param force	- true to affect volume even w/ Options::mute=TRUE (default false)
 */
void Game::setVolume(
		int music,
		int fx,
		int ui,
		bool force)
{
	if (Options::mute == false || force == true)
	{
		if (music != -1)
		{
			music = static_cast<int>(volExp(music) * static_cast<double>(SDL_MIX_MAXVOLUME));
			Mix_VolumeMusic(music);
//			func_set_music_volume(music);
		}

		if (fx != -1)
		{
			fx = static_cast<int>(volExp(fx) * static_cast<double>(SDL_MIX_MAXVOLUME));
			Mix_Volume(-1, fx); // sets volume on *all channels*
		}

		if (ui != -1)
		{
			ui = static_cast<int>(volExp(ui) * static_cast<double>(SDL_MIX_MAXVOLUME));
			Mix_Volume(0, ui); // channel-0 to ui-Volume
			Mix_Volume(1, ui); // channel-1 to ui-Volume
			Mix_Volume(2, ui); // channel-2 to ui-Volume
		}
	}
}

/**
 * Adjusts a linear volume-level to an exponential one.
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
 * Loads the most appropriate Language for the current options and
 * operating-system.
 */
void Game::defaultLanguage()
{
	const std::string defaultLang ("en-US");

	if (Options::language.empty() == true) // No language set, detect based on OS.
	{
		const std::string
			local (CrossPlatform::getLocale()),
			lang (local.substr(
							0u,
							local.find_first_of('-')));

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
		try // Use Options' language
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
 * Changes the Language currently in use by this Game.
 * @param file - reference to the name of the language-file
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
	catch (YAML::Exception& e)
	{
		throw Exception(path + ": " + std::string(e.what()));
	}

	Options::language = file;
}

/**
 * Gets the Language currently in use by this Game.
 * @return, pointer to the Language
 */
Language* Game::getLanguage() const
{
	return _lang;
}

/**
 * Sets a ResourcePack for this Game to use.
 * @param res - pointer to the ResourcePack (default nullptr)
 */
void Game::setResourcePack(ResourcePack* const res)
{
	delete _res;
	_res = res;
}

/**
 * Gets the ResourcePack currently in use by this Game.
 * @return, pointer to the ResourcePack
 */
ResourcePack* Game::getResourcePack() const
{
	return _res;
}

/**
 * Loads the rulesets specified in Options.
 */
void Game::loadRulesets()
{
	Options::badRules.clear();

	_rules = new Ruleset(this);

//	if (Options::rulesets.empty() == true)
//		Options::rulesets.push_back("Xcom1Ruleset");

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

			Options::badRules.push_back(*i);
			Options::badRules.push_back(e.what());

			i = Options::rulesets.erase(i);
		}
	}

	if (Options::rulesets.empty() == true)
	{
		throw Exception("Failed to load ruleset");
	}

	// Prints listOrder to LOG.
//	Log(LOG_INFO) << "\n";
//	std::vector<std::string> pedList = _rules->getUfopaediaList();
//	for (std::vector<std::string>::const_iterator
//			i = pedList.begin();
//			i != pedList.end();
//			++i)
//	{
//		Log(LOG_INFO) << *i;
//	}

	_rules->validateMissions();
	_rules->sortLists();

	_rules->convertInventories();

	// Prints listOrder to LOG.
//	Log(LOG_INFO) << "\n";
//	std::vector<std::string> pedList = _rules->getUfopaediaList();
//	for (std::vector<std::string>::const_iterator
//			i = pedList.begin();
//			i != pedList.end();
//			++i)
//	{
//		Log(LOG_INFO) << *i;
//	}
//	Log(LOG_INFO) << "\n";
}

/**
 * Gets the Ruleset currently in use by this Game.
 * @return, pointer to the Ruleset
 */
Ruleset* Game::getRuleset() const
{
	return _rules;
}

/**
 * Sets a SavedGame for this Game to use.
 * @param gameSave - pointer to the SavedGame (default nullptr)
 */
void Game::setSavedGame(SavedGame* const gameSave)
{
	delete _gameSave;
	_gameSave = gameSave;
}

/**
 * Gets the SavedGame currently in use by this Game.
 * @return, pointer to the SavedGame
 */
SavedGame* Game::getSavedGame() const
{
	return _gameSave;
}

/**
 * Gets the display-screen used by this Game.
 * @return, pointer to the Screen
 */
Screen* Game::getScreen() const
{
	return _screen;
}

/**
 * Gets the mouse-cursor used by this Game.
 * @return, pointer to the Cursor
 */
Cursor* Game::getCursor() const
{
	return _cursor;
}

/**
 * Gets the FpsCounter used by this Game.
 * @return, pointer to the FpsCounter
 */
FpsCounter* Game::getFpsCounter() const
{
	return _fpsCounter;
}

/**
 * Gets the country-cycle for debugging country-zones.
 * @return, the current country-cycle value
 */
int Game::getDebugCycle() const
{
	return _debugCycle;
}

/**
 * Sets the country-cycle for debugging country-zones.
 * @param cycle - the country-cycle value to set
 */
void Game::setDebugCycle(int cycle)
{
	_debugCycle = cycle;
}

/**
 * Gets a synthetic mouse-down Action.
 * @note If you ever get your hands on a 32-button mouse you might be able to
 * rig it up by passing the btnId's into Action via '_rodentState'.
 * @return, pointer to an Action
 */
Action* Game::getSynthMouseDown() // static.
{
	return &syntheticD;

//	Action(
//			&event,
//			_screen->getScaleX(),
//			_screen->getScaleY(),
//			_screen->getBorderTop(),
//			_screen->getBorderLeft(),
//			_rodentState | (1 << (btnId - 1)));
}

/**
 * Gets a synthetic mouse-up Action.
 * @note If you ever get your hands on a 32-button mouse you might be able to
 * rig it up by passing the btnId's into Action via '_rodentState'.
 * @return, pointer to an Action
 */
Action* Game::getSynthMouseUp() // static.
{
	return &syntheticU;

//	Action(
//			&event,
//			_screen->getScaleX(),
//			_screen->getScaleY(),
//			_screen->getBorderTop(),
//			_screen->getBorderLeft(),
//			_rodentState & ~(1 << (btnId - 1)));
}

}
