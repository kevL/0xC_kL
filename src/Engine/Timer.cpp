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

#include "Timer.h"

//#include "Game.h"
//#include "Logger.h"


namespace OpenXcom
{

Uint32 Timer::coreInterval = 1u; // static.

namespace
{

/**
 * Slows down the CPU.
 * @note This is not the engine; cf, Game::run(). This is for accessories like
 * scrolling-speed, animation-speed, etc. which are of course based off the
 * engine but need to be slowed down.
 * @return, current tick
 */
static Uint32 slowTick()
{
	static const Uint32 ACCURATE (4u);

	static Uint32
		tickOld   (SDL_GetTicks()),
		tickFalse (tickOld << ACCURATE);

	Uint32 tickNew (SDL_GetTicks() << ACCURATE);

	tickFalse += (tickNew - tickOld) / Timer::coreInterval;
	tickOld = tickNew;

	return (tickFalse >> ACCURATE);
}

}


/**
 * Initializes the Timer with a specific interval.
 * @param interval - time interval in milliseconds
 */
Timer::Timer(Uint32 interval)
	:
		_interval(interval),
		_startTick(0u),
		_running(false),
		_state(nullptr),
		_surface(nullptr),
		_debug(false)
{}

/**
 * dTor.
 */
Timer::~Timer()
{}

/**
 * Starts this Timer running and counting time.
 */
void Timer::start()
{
	//if (_debug) Log(LOG_INFO) << "Timer: start() [" << _stDebugObject << "]";
	_startTick = slowTick();
	_running = true;
}

/**
 * Stops this Timer from running.
 */
void Timer::stop()
{
	//if (_debug) Log(LOG_INFO) << "Timer: stop() [" << _stDebugObject << "]";
	_startTick = 0u;
	_running = false;
}

/**
 * Gets the time elapsed since the last interval.
 * @note Used only for the FPS counter.
 * @return, time in milliseconds
 */
Uint32 Timer::getTimerElapsed() const
{
	if (_running == true)
		return slowTick() - _startTick; // WARNING: this will give a single erroneous reading @ ~49 days RT.

	return 0u;
}

/**
 * Checks if this Timer has been started and is currently running.
 * @return, true if running state
 */
bool Timer::isRunning() const
{
	return _running;
}

/**
 * The timer keeps calculating passed-time while it's running - calling the
 * respective Handler (State and/or Surface) at each set-interval-pass.
 * @param state		- State that the action handler belongs to
 * @param surface	- Surface that the action handler belongs to
 */
void Timer::think(
		State* const state,
		Surface* const surface)
{
	//if (_debug) Log(LOG_INFO) << "Timer: think() [" << _stDebugObject << "]" << " interval = " << _interval;
	if (_running == true)
	{
		Uint32 ticks (slowTick());
		//if (_debug) Log(LOG_INFO) << ". ticks = " << ticks;
		if (ticks >= _startTick + _interval)
		{
			//if (_debug) Log(LOG_INFO) << ". . ticks > " << _startTick + _interval;
			if (state != nullptr && _state != nullptr)
			{
				//if (_debug) Log(LOG_INFO) << ". . . call StateHandler";
				(state->*_state)();		// call to *StateHandler.
			}

			if (_running == true && surface != nullptr && _surface != nullptr)
			{
				//if (_debug) Log(LOG_INFO) << ". . . call SurfaceHandler";
				(surface->*_surface)();	// call to *SurfaceHandler.
			}

			//if (_debug) Log(LOG_INFO) << ". . reset _startTick";
			_startTick = ticks;
		}
	}
}

/**
 * Changes this Timer's interval to a different value.
 * @param interval - interval in milliseconds
 */
void Timer::setInterval(Uint32 interval)
{
	_interval = interval;
}

/**
 * Sets a state-function for this Timer to call every interval.
 * @param handler - event handler
 */
void Timer::onTimer(StateHandler handler)
{
	_state = handler;
}

/**
 * Sets a surface-function for this Timer to call every interval.
 * @param handler - event handler
 */
void Timer::onTimer(SurfaceHandler handler)
{
	_surface = handler;
}

/**
 * Debugs this Timer.
 */
void Timer::debug(const std::string& info)
{
	_debug = true;
	_stDebugObject = info;
}

}
