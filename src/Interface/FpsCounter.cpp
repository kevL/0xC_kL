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

#include "FpsCounter.h"

#include <cmath>

#ifdef _WIN32
#	ifndef NOMINMAX
#		define NOMINMAX
#	endif

#	ifndef WIN32_LEAN_AND_MEAN
#		define WIN32_LEAN_AND_MEAN
#	endif

#	include <windows.h>
#endif

#include "NumberText.h"

#include "../Engine/Action.h"
#include "../Engine/Options.h"
#include "../Engine/Screen.h"
#include "../Engine/Timer.h"


namespace OpenXcom
{

/**
 * Creates the FpsCounter with a specified size.
 * @param width		- width in pixels
 * @param height	- height in pixels
 * @param x			- x-position in pixels (default 0)
 * @param y			- y-position in pixels (default 0)
 */
FpsCounter::FpsCounter(
		int width,
		int height,
		int x,
		int y)
	:
		Surface(
			width, height,
			x + 1, y),
		_frames(0u)
{
	_visible = Options::fpsCounter;

	_timer = new Timer(1000u);
	_timer->onTimer((SurfaceHandler)& FpsCounter::update);
	_timer->start();

	_text = new NumberText(width, height, x,y);
}

/**
 * Deletes this FpsCounter.
 */
FpsCounter::~FpsCounter()
{
	delete _text;
	delete _timer;
}

/**
 * Replaces a certain amount of colors in this FpsCounter's palette.
 * @param colors		- pointer to the set of colors
 * @param firstcolor	- offset of the first color to replace (default 0)
 * @param ncolors		- amount of colors to replace (default 256)
 */
void FpsCounter::setPalette(
		SDL_Color* const colors,
		int firstcolor,
		int ncolors)
{
	Surface::setPalette(colors, firstcolor, ncolors);
	_text->setPalette(colors, firstcolor, ncolors);
}

/**
 * Sets the text-color of this FpsCounter.
 * @param color - the color
 */
void FpsCounter::setColor(Uint8 color)
{
	_text->setColor(color);
}

/**
 * Shows/hides this FpsCounter.
 * @param action - pointer to an Action
 */
void FpsCounter::handle(Action* action)
{
	if (action->getDetails()->type == SDL_KEYDOWN
		&& action->getDetails()->key.keysym.sym == Options::keyFps)
	{
#ifdef _WIN32
		MessageBeep(MB_OK);
#endif
		Options::fpsCounter =
		_visible = !_visible;
	}
}

/**
 * Calls update on schedule
 */
void FpsCounter::think()
{
	_timer->think(nullptr, this);
}

/**
 * Updates the Frames per Second.
 */
void FpsCounter::update()
{
	const Uint32 fps (static_cast<int>(std::floor(static_cast<double>(_frames * 1000 / _timer->getTimerElapsed()))));
	_text->setValue(static_cast<unsigned>(fps));

	_frames = 0u;
	_redraw = true;
}

/**
 * Draws this FpsCounter.
 */
void FpsCounter::draw()
{
	Surface::draw();
	_text->blit(this);
}

/**
 * Adds to the frame-count.
 */
void FpsCounter::addFrame()
{
	++_frames;
}

}
