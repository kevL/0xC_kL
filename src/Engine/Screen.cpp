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

#include "Screen.h"

//#include <algorithm>
//#include <cmath>
//#include <cstddef> // nullptr (for NB code-assistant only)
#include <cstring>
//#include <iomanip>
//#include <sstream>
//#include <limits.h>
//#include <SDL.h>

#include "../lodepng.h"

#include "Action.h"
#include "CrossPlatform.h"
#include "Exception.h"
#include "Logger.h"
#include "Options.h"
#include "Surface.h"
#include "Timer.h"
#include "Zoom.h"


namespace OpenXcom
{

/**
 * Sets up all the internal display flags depending on current video settings.
 */
void Screen::setVideoFlags() // private.
{
//	_flags = SDL_HWSURFACE | SDL_DOUBLEBUF | SDL_HWPALETTE;
//	if (Options::asyncBlit == true)
//		_flags |= SDL_ASYNCBLIT;

	if (isOpenGLEnabled() == true)
	{
		_flags = SDL_OPENGL; // NOTE: This overwrites the flags set above^. Let's make that explicit below_

		SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 5);
		SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 5);
		SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 5);
		SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
		SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	}
	else
	{
		_flags = SDL_HWSURFACE | SDL_DOUBLEBUF | SDL_HWPALETTE;

		if (Options::asyncBlit == true)
			_flags |= SDL_ASYNCBLIT;
	}

	if (Options::allowResize == true)
		_flags |= SDL_RESIZABLE;

	// Handle window positioning
	if (   Options::windowedModePositionX != -1
		|| Options::windowedModePositionY != -1)
	{
		std::ostringstream oststr;
		oststr << "SDL_VIDEO_WINDOW_POS=" << std::dec << Options::windowedModePositionX << "," << Options::windowedModePositionY;
		SDL_putenv(const_cast<char*>(oststr.str().c_str()));
		SDL_putenv(const_cast<char*>("SDL_VIDEO_CENTERED="));
	}
	else if (Options::borderless == true)
	{
		SDL_putenv(const_cast<char*>("SDL_VIDEO_WINDOW_POS="));
		SDL_putenv(const_cast<char*>("SDL_VIDEO_CENTERED=center"));
	}
	else
	{
		SDL_putenv(const_cast<char*>("SDL_VIDEO_WINDOW_POS="));
		SDL_putenv(const_cast<char*>("SDL_VIDEO_CENTERED="));
	}

	// Handle display mode
	if (Options::fullscreen == true)
		_flags |= SDL_FULLSCREEN;

	if (Options::borderless == true)
	{
		_flags |= SDL_NOFRAME;
		SDL_putenv(const_cast<char*>("SDL_VIDEO_CENTERED=center"));
	}
	else
		SDL_putenv(const_cast<char*>("SDL_VIDEO_CENTERED="));

	_bpp = (is32bitEnabled() == true || isOpenGLEnabled() == true) ? 32 : 8;

	_baseWidth = Options::baseXResolution;
	_baseHeight = Options::baseYResolution;
}


/**
 * Creates the display Screen for the engine to render contents to.
 * @note The Screen is initialized based on the current options.
 */
Screen::Screen()
	:
		_baseWidth(ORIGINAL_WIDTH),
		_baseHeight(ORIGINAL_HEIGHT),
		_scaleX(1.),
		_scaleY(1.),
		_flags(0),
		_numColors(0),
		_firstColor(0),
		_pushPalette(false),
		_surface(nullptr)
{
	resetDisplay();
	std::memset(
			_deferredPalette,
			0,
			256 * sizeof(SDL_Color));
}

/**
 * Deletes the buffer from memory.
 * @note The display Screen itself is automatically freed once SDL shuts down.
 */
Screen::~Screen()
{
	delete _surface;
}

/**
 * Gets this Screen's internal buffer surface.
 * @note Any contents that need to be shown will be blitted to this.
 * @return, pointer to the buffer surface
 */
Surface* Screen::getSurface()
{
	_pushPalette = true;
	return _surface;
}

/**
 * Handles this Screen's key-shortcuts.
 * @param action - pointer to an Action
 */
void Screen::handle(Action* action)
{
	if (action->getDetails()->type == SDL_KEYDOWN)
	{
		if (action->getDetails()->key.keysym.sym == SDLK_F8) // && Options::debug == true
		{
#ifdef _WIN32
			MessageBeep(MB_OK);
#endif
			switch (Timer::coreInterval)
			{
				case 1: Timer::coreInterval =  6u; break;
				case 6: Timer::coreInterval = 14u; break;

				default:
					Timer::coreInterval = 1u;
			}
		}
		else if (action->getDetails()->key.keysym.sym == SDLK_RETURN
			&& (SDL_GetModState() & KMOD_ALT) != 0)
		{
			Options::fullscreen = !Options::fullscreen;
			resetDisplay();
		}
		else if (action->getDetails()->key.keysym.sym == Options::keyScreenshot)
		{
#ifdef _WIN32
			MessageBeep(MB_ICONASTERISK); // start ->
#endif
			std::ostringstream oststr;
/*			int i = 0;
			do {
				oststr.str("");
				oststr << Options::getPictureFolder() << "oxc_" << CrossPlatform::timeString() << "_" << i << ".png";
				++i; }
			while (CrossPlatform::fileExists(oststr.str()) == true); */
			// ... too slow to take & write more than one screenshot per second @ 1920x1080 ...
			// Skip the do-while Loop:
			oststr << Options::getPictureFolder() << "0xC_kL_" << CrossPlatform::timeString() << ".png";
			screenshot(oststr.str());
#ifdef _WIN32
			MessageBeep(MB_OK); // end.
#endif
/*			std::ostringstream oststr;
			int i = 0;
			do
			{
				oststr.str("");
				oststr << Options::getUserFolder() << "screen" << std::setfill('0') << std::setw(3) << i << ".png";
				++i;
			} while (CrossPlatform::fileExists(oststr.str()) == true);
			screenshot(oststr.str()); */
		}
	}
}

/**
 * Renders the buffer's contents onto this Screen applying any necessary filters
 * or conversions in the process.
 * @note If the scaling factor is bigger than 1 the entire contents of the
 * buffer are resized by that factor (eg. 2 = doubled) before being put on screen.
 */
void Screen::flip()
{
	if (isOpenGLEnabled() == true
		|| getWidth() != _baseWidth
		|| getHeight() != _baseHeight)
	{
		Zoom::flipWithZoom(
					_surface->getSurface(),
					_screen,
					_topBlackBand,
					_bottomBlackBand,
					_leftBlackBand,
					_rightBlackBand,
					&_glOutput);
	}
	else
		SDL_BlitSurface(
				_surface->getSurface(),
				nullptr,
				_screen,
				nullptr);

	// perform any requested palette update
	if (_pushPalette == true
		&& _numColors != 0
		&& _screen->format->BitsPerPixel == 8)
	{
		if (_screen->format->BitsPerPixel == 8
			&& SDL_SetColors(
						_screen,
						&(_deferredPalette[_firstColor]),
						_firstColor,
						_numColors) == 0)
		{
			Log(LOG_INFO) << "Display palette doesn't match requested palette";
		}

		_numColors = 0;
		_pushPalette = false;
	}

	if (SDL_Flip(_screen) == -1)
	{
		throw Exception(SDL_GetError());
	}
}

/**
 * Clears all the contents out of the internal buffer.
 */
void Screen::clear()
{
	_surface->clear();

	if (_screen->flags & SDL_SWSURFACE)
		std::memset(
				_screen->pixels,
				0,
				_screen->h * _screen->pitch);
	else
		SDL_FillRect(
				_screen,
				&_clear,
				0);
}

/**
 * Sets the 8-bpp palette used to render this Screen's contents.
 * @param colors		- pointer to the set of colors
 * @param firstcolor	- offset of the first color to replace
 * @param ncolors		- quantity of colors to replace
 * @param immediately	- apply palette changes immediately otherwise wait for next blit
 */
void Screen::setPalette(
		SDL_Color* const colors,
		int firstcolor,
		int ncolors,
		bool immediately)
{
	if (_numColors != 0
		&& _numColors != ncolors
		&& _firstColor != firstcolor)
	{
		// an initial palette setup has not been committed to the screen yet
		// just update it with whatever colors are being sent now
		std::memmove(
				&(_deferredPalette[firstcolor]),
				colors,
				sizeof(SDL_Color) * ncolors);
		_numColors = 256; // all the use cases are just a full palette with 16-color follow-ups
		_firstColor = 0;
	}
	else
	{
		std::memmove(
				&(_deferredPalette[firstcolor]),
				colors,
				sizeof(SDL_Color) * ncolors);
		_numColors = ncolors;
		_firstColor = firstcolor;
	}

	_surface->setPalette(
					colors,
					firstcolor,
					ncolors);

	// defer actual update of screen until SDL_Flip()
	if (immediately == true
		&& _screen->format->BitsPerPixel == 8
		&& SDL_SetColors(
					_screen,
					colors,
					firstcolor,
					ncolors) == 0)
	{
		Log(LOG_DEBUG) << "Display palette doesn't match requested palette";
	}

	// Sanity check
//	SDL_Color* newcolors (_screen->format->palette->colors);
//	for (int i = firstcolor, j = 0; i < firstcolor + ncolors; i++, j++)
//	{
//		Log(LOG_DEBUG) << (int)newcolors[i].r << " - " << (int)newcolors[i].g << " - " << (int)newcolors[i].b;
//		Log(LOG_DEBUG) << (int)colors[j].r << " + " << (int)colors[j].g << " + " << (int)colors[j].b;
//		if (newcolors[i].r != colors[j].r ||
//			newcolors[i].g != colors[j].g ||
//			newcolors[i].b != colors[j].b)
//		{
//			Log(LOG_ERROR) << "Display palette doesn't match requested palette";
//			break;
//		}
//	}
}

/**
 * Gets this Screen's 8-bpp palette.
 * @return, pointer to the palette's colors
 */
SDL_Color* Screen::getPalette() const
{
	return const_cast<SDL_Color*>(_deferredPalette);
}

/**
 * Gets the width of this Screen.
 * @return, width in pixels
 */
int Screen::getWidth() const
{
	return _screen->w;
}

/**
 * Gets the height of this Screen.
 * @return, height in pixels
 */
int Screen::getHeight() const
{
	return _screen->h;
}

/**
 * Resets this Screen's surfaces based on the current display options since they
 * don't automatically take effect.
 * @param resetVideo - true to reset display surface (default true)
 */
void Screen::resetDisplay(bool resetVideo)
{
	const int
		width (Options::displayWidth),
		height (Options::displayHeight);

#ifdef __linux__
	Uint32 oldFlags (_flags);
#endif

	setVideoFlags();

	if (_surface == nullptr // don't reallocate _surface if not necessary, it's a waste of CPU cycles
		|| _surface->getSurface()->format->BitsPerPixel != static_cast<Uint8>(_bpp)
		|| _surface->getSurface()->w != _baseWidth
		|| _surface->getSurface()->h != _baseHeight)
	{
		if (_surface != nullptr)
			delete _surface;

		_surface = new Surface( // only HQX needs 32-bpp for this surface; the OpenGL class has its own 32-bpp buffer
							_baseWidth,
							_baseHeight,
							0,0,
							Screen::is32bitEnabled() ? 32 : 8);

		if (_surface->getSurface()->format->BitsPerPixel == 8)
			_surface->setPalette(_deferredPalette);
	}

	SDL_SetColorKey( // turn off color key!
				_surface->getSurface(),
				0,0);

	if (resetVideo == true
		|| _screen->format->BitsPerPixel != _bpp)
	{
#ifdef __linux__
		// Workaround for segfault when switching to opengl
		if (!(oldFlags & SDL_OPENGL) && (_flags & SDL_OPENGL))
		{
			Uint8 cursor = 0;
			char* _oldtitle = 0;
			SDL_WM_GetCaption(&_oldtitle, nullptr);
			std::string title(_oldtitle);
			SDL_QuitSubSystem(SDL_INIT_VIDEO);
			SDL_InitSubSystem(SDL_INIT_VIDEO);
			SDL_ShowCursor(SDL_ENABLE);
			SDL_EnableUNICODE(1);
			SDL_WM_SetCaption(title.c_str(), 0);
			SDL_SetCursor(SDL_CreateCursor(&cursor, &cursor, 1,1,0,0));
		}
#endif

		Log(LOG_INFO) << "Attempting to set display to " << width << "x" << height << "x" << _bpp << " ...";
		_screen = SDL_SetVideoMode(
								width, height,
								_bpp, _flags);
		if (_screen == nullptr)
		{
			Log(LOG_ERROR) << SDL_GetError();
			Log(LOG_INFO) << "Attempting to set display to default resolution [640x400x" << _bpp << "] ...";
			_screen = SDL_SetVideoMode(
									640,400,
									_bpp, _flags);
			if (_screen == nullptr)
			{
				throw Exception(SDL_GetError());
			}
		}
		Log(LOG_INFO) << "Display set to " << getWidth() << "x" << getHeight() << "x" << (int)_screen->format->BitsPerPixel << ".";
	}
	else
		clear();

	Options::displayWidth  = getWidth();
	Options::displayHeight = getHeight();

	_scaleX = static_cast<double>(getWidth()) / static_cast<double>(_baseWidth);
	_scaleY = static_cast<double>(getHeight()) / static_cast<double>(_baseHeight);

	_clear.x =
	_clear.y = 0;
	_clear.w = static_cast<Uint16>(getWidth());
	_clear.h = static_cast<Uint16>(getHeight());

	double pixelRatioY;
	if (Options::nonSquarePixelRatio && Options::allowResize == false)
		pixelRatioY = 1.2;
	else
		pixelRatioY = 1.;

	bool cursorInBlackBands;
	if (Options::keepAspectRatio == false)
		cursorInBlackBands = false;
	else if (Options::fullscreen == true)
		cursorInBlackBands = Options::cursorInBlackBandsInFullscreen;
	else if (Options::borderless == false)
		cursorInBlackBands = Options::cursorInBlackBandsInWindow;
	else
		cursorInBlackBands = Options::cursorInBlackBandsInBorderlessWindow;

	if (Options::keepAspectRatio == true && _scaleX != _scaleY)
	{
		if (_scaleX > _scaleY)
		{
			const int targetWidth (static_cast<int>(std::floor(_scaleY * static_cast<double>(_baseWidth))));

			_topBlackBand =
			_bottomBlackBand = 0;
			_leftBlackBand = (getWidth() - targetWidth) / 2;
			if (_leftBlackBand < 0) _leftBlackBand = 0;

			_rightBlackBand = getWidth() - targetWidth - _leftBlackBand;
			_cursorTopBlackBand = 0;

			if (cursorInBlackBands == true)
			{
				_scaleX = _scaleY;
				_cursorLeftBlackBand = _leftBlackBand;
			}
			else
				_cursorLeftBlackBand = 0;
		}
		else if (_scaleX < _scaleY)
		{
			const int targetHeight (static_cast<int>(std::floor(_scaleX * static_cast<double>(_baseHeight) * pixelRatioY)));

			_topBlackBand = (getHeight() - targetHeight) / 2;
			if (_topBlackBand < 0) _topBlackBand = 0;

			_bottomBlackBand = getHeight() - targetHeight - _topBlackBand;
			if (_bottomBlackBand < 0) _bottomBlackBand = 0;

			_leftBlackBand =
			_rightBlackBand =
			_cursorLeftBlackBand = 0;

			if (cursorInBlackBands == true)
			{
				_scaleY = _scaleX;
				_cursorTopBlackBand = _topBlackBand;
			}
			else
				_cursorTopBlackBand = 0;
		}
	}
	else
		_topBlackBand =
		_bottomBlackBand =
		_leftBlackBand =
		_rightBlackBand =
		_cursorTopBlackBand =
		_cursorLeftBlackBand = 0;

#ifndef __NO_OPENGL
	if (isOpenGLEnabled() == true)
	{
		_glOutput.init(_baseWidth, _baseHeight);
		_glOutput.linear = Options::useOpenGLSmoothing; // setting from shader file will override this, though
		_glOutput.setVSync(Options::vSyncForOpenGL);

#	ifdef _DEBUG
		_glOutput.set_shader(CrossPlatform::getDataFile("Shaders/Raw.OpenGL.shader").c_str());
#	else
		_glOutput.set_shader(CrossPlatform::getDataFile(Options::useOpenGLShader).c_str());
#	endif

		OpenGL::checkErrors = Options::checkOpenGLErrors;
	}
#endif

	if (_screen->format->BitsPerPixel == 8)
		setPalette(getPalette());
}

/**
 * Gets this Screen's x-scale.
 * @return, x-scale factor
 */
double Screen::getXScale() const
{
	return _scaleX;
}

/**
 * Gets this Screen's y-scale.
 * @return, y-scale factor
 */
double Screen::getYScale() const
{
	return _scaleY;
}

/**
 * Gets this Screen's top black forbidden-to-cursor band's height.
 * @return, height in pixels
 */
int Screen::getCursorTopBlackBand() const
{
	return _cursorTopBlackBand;
}

/**
 * Gets this Screen's left black forbidden-to-cursor band's width.
 * @return, width in pixels
 */
int Screen::getCursorLeftBlackBand() const
{
	return _cursorLeftBlackBand;
}

/**
 * Saves a screenshot of this Screen's contents.
 * @param file - name of the PNG output file
 */
void Screen::screenshot(const std::string& file) const
{
	SDL_Surface* const screenshot (SDL_AllocSurface(
												0,
												getWidth() - getWidth() % 4,
												getHeight(),
												24,
												0xff,
												0xff00,
												0xff0000,
												0));

	if (isOpenGLEnabled() == true)
	{
#ifndef __NO_OPENGL
		GLenum screenFormat (GL_RGB);

		for (int
				y = 0;
				y != getHeight();
				++y)
		{
			glReadPixels(
					0,
					getHeight() - (y + 1),
					getWidth() - getWidth() % 4,
					1,
					screenFormat,
					GL_UNSIGNED_BYTE,
					static_cast<Uint8*>(screenshot->pixels) + y * screenshot->pitch);
		}

		glErrorCheck();
#endif
	}
	else
		SDL_BlitSurface(
					_screen,
					nullptr,
					screenshot,
					nullptr);

	unsigned error (lodepng::encode(
								file,
								(const unsigned char*)(screenshot->pixels),
								getWidth() - getWidth() % 4,
								getHeight(),
								LCT_RGB));
	if (error != 0)
	{
		Log(LOG_ERROR) << "Saving to PNG failed: " << lodepng_error_text(error);
	}

	SDL_FreeSurface(screenshot);
}


/**
 * Checks whether a 32-bpp scaler has been selected.
 * @return, true if it is enabled with a compatible resolution
 */
bool Screen::is32bitEnabled() // static.
{
	const int
		w (Options::displayWidth),
		h (Options::displayHeight),
		baseW (Options::baseXResolution),
		baseH (Options::baseYResolution);

	return ((Options::useHQXFilter == true || Options::useXBRZFilter == true)
			&& ((	   w == baseW * 2
					&& h == baseH * 2)
				|| (   w == baseW * 3
					&& h == baseH * 3)
				|| (   w == baseW * 4
					&& h == baseH * 4)
				|| (   w == baseW * 5
					&& h == baseH * 5
					&& Options::useXBRZFilter == true)));
}

/**
 * Checks if OpenGL is enabled.
 * @return, true if enabled
 */
bool Screen::isOpenGLEnabled() // static.
{
#ifdef __NO_OPENGL
	return false;
#else
	return Options::useOpenGL;
#endif
}

/**
 * Gets the horizontal-offset from the mid-point of this Screen in pixels.
 * @return, horizontal-offset
 */
int Screen::getDX() const
{
	return (_baseWidth - ORIGINAL_WIDTH) / 2;
}

/**
 * Gets the vertical-offset from the mid-point of this Screen in pixels.
 * @return, vertical-offset
 */
int Screen::getDY() const
{
	return (_baseHeight - ORIGINAL_HEIGHT) / 2;
}

/**
 * Changes a given scale and if necessary switches the current base-resolution.
 * @param type		- reference which scale option is in use (Battlescape or Geoscape)
 * @param selection	- the new scale level
 * @param width		- reference which x scale to adjust
 * @param height	- reference which y scale to adjust
 * @param change	- true to change the current scale
 */
void Screen::updateScale( // static.
		int& type,
		int selection,
		int& width,
		int& height,
		bool change)
{
	double pixelRatioY;
	if (Options::nonSquarePixelRatio == true)
		pixelRatioY = 1.2;
	else
		pixelRatioY = 1.;

	type = selection;
	switch (type)
	{
		case SCALE_15X:
			width	= static_cast<int>(static_cast<double>(Screen::ORIGINAL_WIDTH) * 1.5);
			height	= static_cast<int>(static_cast<double>(Screen::ORIGINAL_HEIGHT) * 1.5);
			break;

		case SCALE_2X:
			width	= static_cast<int>(static_cast<double>(Screen::ORIGINAL_WIDTH) * 2.);
			height	= static_cast<int>(static_cast<double>(Screen::ORIGINAL_HEIGHT) * 2.);
			break;

		case SCALE_SCREEN_DIV_3:
			width	= static_cast<int>(static_cast<double>(Options::displayWidth) / 3.);
			height	= static_cast<int>(static_cast<double>(Options::displayHeight) / pixelRatioY / 3.);
			break;

		case SCALE_SCREEN_DIV_2:
			width	= static_cast<int>(static_cast<double>(Options::displayWidth) / 2.);
			height	= static_cast<int>(static_cast<double>(Options::displayHeight) / pixelRatioY / 2.);
			break;

		case SCALE_SCREEN:
			width	= Options::displayWidth;
			height	= static_cast<int>(static_cast<double>(Options::displayHeight / pixelRatioY));
			break;

		default:
		case SCALE_ORIGINAL:
			width	= Screen::ORIGINAL_WIDTH;
			height	= Screen::ORIGINAL_HEIGHT;
	}

// G++ linker wants it this way ...
#ifdef _DEBUG
	const int
		screenWidth (Screen::ORIGINAL_WIDTH),
		screenHeight (Screen::ORIGINAL_HEIGHT);

	width = std::max(width,
					 screenWidth);
	height = std::max(height,
					  screenHeight);
#else
	width = std::max(width,
					 Screen::ORIGINAL_WIDTH);
	height = std::max(height,
					  Screen::ORIGINAL_HEIGHT);
#endif

	if (change == true
		&& (   Options::baseXResolution != width
			|| Options::baseYResolution != height))
	{
		Options::baseXResolution = width;
		Options::baseYResolution = height;
	}
}

}
