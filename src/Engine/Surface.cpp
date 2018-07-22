/*
 * Copyright 2010-2018 OpenXcom Developers.
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

#include "Surface.h"

#include <cstring>	// std::memset(), std::memcpy()
#include <fstream>	// std::ifstream
//#include <vector>	// std::vector

#include <SDL/SDL_gfxPrimitives.h>
#include <SDL/SDL_image.h>

#include "../lodepng.h"

#include "Exception.h"
#include "Language.h"
#include "Logger.h"
#include "Palette.h"
#include "ShaderDraw.h"
#include "ShaderMove.h"

#ifdef _WIN32
#	include <malloc.h>
#endif

#if defined(__MINGW32__) && !defined(__MINGW64_VERSION_MAJOR)
#	define _aligned_malloc __mingw_aligned_malloc
#	define _aligned_free   __mingw_aligned_free
#endif

#ifdef __MORPHOS__
#	include <ppcinline/exec.h>
#endif


namespace OpenXcom
{

namespace
{

/**
 * Helper function counting pitch in bytes with 16-byte padding.
 * @param bpp	- bits per pixel
 * @param width	- number of pixel in row
 * @return, pitch in bytes
 */
inline int GetPitch(
		int bpp,
		int width)
{
	return ((bpp >> 3u) * width + 15) & ~0xf;
}

/**
 * Helper function creating aligned buffer.
 * @param bpp		- bits per pixel
 * @param width		- number of pixels in row
 * @param height	- number of rows
 * @return, pointer to memory
 */
inline void* NewAligned(
		int bpp,
		int width,
		int height)
{
	const int
		pitch (GetPitch(bpp, width)),
		total (pitch * height);
	void* buffer (nullptr);

#ifndef _WIN32
#	ifdef __MORPHOS__
	if ((buffer = calloc(total, 1)) == nullptr)
	{
		throw Exception("Failed to allocate surface");
	}
#	else
	int rc;
	if ((rc = posix_memalign(&buffer, 16, total)))
	{
		throw Exception(strerror(rc));
	}
#	endif
#else
	if ((buffer = _aligned_malloc(static_cast<size_t>(total), 16u)) == nullptr) // of course Windows has to be difficult about this!
	{
		throw Exception("Failed to allocate surface");
	}
#endif

	std::memset(
			buffer,
			0,
			static_cast<size_t>(total));

	return buffer;
}

/**
 * Helper function releases aligned memory.
 * @param buffer - buffer to delete
 */
inline void DeleteAligned(void* buffer)
{
	if (buffer != nullptr)
	{
#ifdef _WIN32
		_aligned_free(buffer);
#else
		free(buffer);
#endif
	}
}

}


/**
 * Sets up a blank 8-bpp surface with the specified size and position with pure
 * black as the transparent color.
 * @note Surfaces don't have to fill the whole size since their
 * background is transparent, specially subclasses with their own
 * drawing logic, so it just covers the maximum drawing area.
 * @param width		- width in pixels (default 320)
 * @param height	- height in pixels (default 200)
 * @param x			- x-position in pixels (default 0)
 * @param y			- y-position in pixels (default 0)
 * @param bpp		- bits-per-pixel depth (default 8)
 */
Surface::Surface(
		int width,
		int height,
		int x,
		int y,
		int bpp)
	:
		_x(x),
		_y(y),
		_visible(true),
		_hidden(false),
		_redraw(false),
		_alignedBuffer(nullptr)
{
	_alignedBuffer = NewAligned(bpp, width, height);
	_surface = SDL_CreateRGBSurfaceFrom(
									_alignedBuffer,
									width, height, bpp,
									GetPitch(bpp, width),
									0u,0u,0u,0u);

	if (_surface == nullptr)
	{
		throw Exception(SDL_GetError());
	}

	SDL_SetColorKey(
				_surface,
				SDL_SRCCOLORKEY,
				0u);

	_crop.x =
	_crop.y = 0;
	_crop.w =
	_crop.h = 0u;

	_clear.x =
	_clear.y = 0;
	_clear.w = static_cast<Uint16>(getWidth());
	_clear.h = static_cast<Uint16>(getHeight());
}

/**
 * Performs a deep copy of an existing Surface.
 * @param copyThat - reference to a Surface to copy
 */
Surface::Surface(const Surface& copyThat)
{
	if (copyThat._alignedBuffer) // if native OpenXcom aligned surface
	{
		const int
			width	(copyThat.getWidth()),
			height	(copyThat.getHeight()),
			bpp		(static_cast<int>(copyThat._surface->format->BitsPerPixel)),
			pitch	(GetPitch(bpp, width));

		_alignedBuffer = NewAligned(bpp, width, height);
		_surface = SDL_CreateRGBSurfaceFrom(
										_alignedBuffer,
										width,
										height,
										bpp,
										pitch,
										0u,0u,0u,0u);

		SDL_SetColorKey(
					_surface,
					SDL_SRCCOLORKEY,
					0u);
		// can't call 'setPalette' because it's a virtual function and don't work correctly in constructor
		SDL_SetColors(
					_surface,
					copyThat.getPalette(),
					0,255);

		std::memcpy(
				_alignedBuffer,
				copyThat._alignedBuffer,
				static_cast<size_t>(height * pitch));
	}
	else
	{
		_surface = SDL_ConvertSurface(
									copyThat._surface,
									copyThat._surface->format,
									copyThat._surface->flags);
		_alignedBuffer = nullptr;
	}

	if (_surface == nullptr)
	{
		throw Exception(SDL_GetError());
	}

	_x = copyThat._x;
	_y = copyThat._y;

	_crop.x = copyThat._crop.x;
	_crop.y = copyThat._crop.y;
	_crop.w = copyThat._crop.w;
	_crop.h = copyThat._crop.h;

	_clear.x = copyThat._clear.x;
	_clear.y = copyThat._clear.y;
	_clear.w = copyThat._clear.w;
	_clear.h = copyThat._clear.h;

	_visible = copyThat._visible;
	_hidden  = copyThat._hidden;
	_redraw  = copyThat._redraw;
}

/**
 * Deletes this Surface from memory.
 */
Surface::~Surface() // virtual.
{
	DeleteAligned(_alignedBuffer);
	SDL_FreeSurface(_surface);
}

/**
 * Loads the contents of an X-Com SCR image-file into this Surface.
 * @note SCR files are simply uncompressed images containing the palette-offset
 * of each pixel.
 * @param file - reference to the filename of an SCR image
 * @sa http://www.ufopaedia.org/index.php?title=Image_Formats#SCR_.26_DAT
 */
void Surface::loadScr(const std::string& file)
{
	std::ifstream ifstr (file.c_str(), std::ios::binary);
	if (ifstr.fail() == true)
	{
		throw Exception(file + " not found");
	}

	std::vector<char> buffer(
						(std::istreambuf_iterator<char>(ifstr)),
						(std::istreambuf_iterator<char>()));

	lock();
	int
		x (0),
		y (0);

	for (std::vector<char>::const_iterator
			i = buffer.begin();
			i != buffer.end();
			++i)
	{
		setPixelIterative(&x,&y, static_cast<Uint8>(*i));
	}
	unlock();
}

/**
 * Loads the contents of an image-file of a recognized format into this Surface.
 * @param file - reference to the filename of an image
 */
void Surface::loadImage(const std::string& file)
{
	DeleteAligned(_alignedBuffer); // clear the aligned-buffer

	SDL_FreeSurface(_surface);
	_surface = nullptr;
	_alignedBuffer = nullptr;

	Log(LOG_VERBOSE) << "Loading image w/ LodePNG: " << file;

	std::vector<unsigned char> png; // Try loading with LodePNG first
	unsigned error (lodepng::load_file(png, file));
	if (error == 0u)
	{
		std::vector<unsigned char> image;
		unsigned
			width,
			height;

		lodepng::State state;
		state.decoder.color_convert = 0u;

		error = lodepng::decode(
							image,
							width,
							height,
							state,
							png);
		if (error == 0u)
		{
			const LodePNGColorMode* const color (&state.info_png.color);

			const int bpp (static_cast<int>(lodepng_get_bpp(color)));
			if (bpp == 8)
			{
				_alignedBuffer = NewAligned(
										bpp,
										static_cast<int>(width),
										static_cast<int>(height));
				_surface = SDL_CreateRGBSurfaceFrom(
												_alignedBuffer,
												static_cast<int>(width),
												static_cast<int>(height),
												bpp,
												GetPitch(bpp, static_cast<int>(width)),
												0u,0u,0u,0u);
				if (_surface != nullptr)
				{
					int
						x (0),
						y (0);

					for (std::vector<unsigned char>::const_iterator
							i = image.begin();
							i != image.end();
							++i)
					{
						setPixelIterative(&x,&y, static_cast<Uint8>(*i));
					}

					setPalette(
							reinterpret_cast<SDL_Color*>(color->palette),
							0,
							static_cast<int>(color->palettesize));

					int transparent (0);
					for (int
							i = 0;
							i != _surface->format->palette->ncolors;
							++i)
					{
						const SDL_Color* const palColor (_surface->format->palette->colors + i);
						if (palColor->unused == 0u)
						{
							transparent = i;
							break;
						}
					}
					SDL_SetColorKey(
								_surface,
								SDL_SRCCOLORKEY,
								static_cast<Uint32>(transparent));
				}
			}
		}
	}

	if (_surface == nullptr) // Otherwise default to SDL_Image
	{
		// SDL only takes UTF-8 filenames so here's an ugly hack to match this ugly.
		const std::string utf8 (Language::wstrToUtf8(Language::fsToWstr(file)));
		Log(LOG_VERBOSE) << "LodePNG failed - loading image w/ SDL: " << utf8;
		_surface = IMG_Load(utf8.c_str());
	}

	if (_surface == nullptr)
	{
//		const std::string error (file + ": " + IMG_GetError());
		throw Exception(file + ": " + IMG_GetError());
	}
}

/**
 * Loads the contents of an X-Com SPK image-file into this Surface.
 * @note SPK files are compressed with a custom algorithm since they're usually
 * full-screen images.
 * @param file - reference to the filename of the SPK image
 * @sa http://www.ufopaedia.org/index.php?title=Image_Formats#SPK
 */
void Surface::loadSpk(const std::string& file)
{
	std::ifstream ifstr (file.c_str(), std::ios::in | std::ios::binary);
	if (ifstr.fail() == true)
	{
		throw Exception(file + " not found");
	}

	lock();
	Uint16 flag;
	Uint8 value;
	int
		x (0),
		y (0);

	while (ifstr.read(
					reinterpret_cast<char*>(&flag),
					sizeof(flag)))
	{
		flag = SDL_SwapLE16(flag);
		if (flag == 65535u)
		{
			ifstr.read(
					reinterpret_cast<char*>(&flag),
					sizeof(flag));
			flag = SDL_SwapLE16(flag);

			for (Uint16
					i = 0u;
					i != flag << 1u;
					++i)
			{
				setPixelIterative(&x,&y, 0u);
			}
		}
		else if (flag == 65534u)
		{
			ifstr.read(
					reinterpret_cast<char*>(&flag),
					sizeof(flag));
			flag = SDL_SwapLE16(flag);

			for (Uint16
					i = 0u;
					i != flag << 1u;
					++i)
			{
				ifstr.read(
						reinterpret_cast<char*>(&value),
						1);
				setPixelIterative(&x,&y, value);
			}
		}
	}
	unlock();

	ifstr.close();
}

/**
 * Clears the entire contents of this Surface resulting in a blank image of the
 * specified color (0 for transparent).
 * @param color - the color for the background (default 0)
 */
void Surface::clear(Uint32 color)
{
//	if (_surface->flags & SDL_SWSURFACE)	// NOTE: SDL_SWSURFACE= 0x0 ... so that means (if 0 == true).
//		std::memset(						// ... This never runs. cf, Screen::clear()
//				_surface->pixels,
//				static_cast<int>(color),
//				static_cast<size_t>(_surface->h) * static_cast<size_t>(_surface->pitch));
//	else
	SDL_FillRect(
			_surface,
			&_clear,
			color);
}

/**
 * Shifts all the colors in this Surface by a specified value.
 * @note This is a common method in 8-bpp games to simulate color-effects for
 * cheap.
 * @param shift		- shift
 * @param colorLow	- lowest color-value to shift to (default -1)
 * @param colorHigh	- highest color-value to shift to (default -1)
 * @param multer	- shift-multiplier (default 1)
 */
void Surface::offset(
		int shift,
		int colorLow,
		int colorHigh,
		int multer)
{
	if (shift != 0)
	{
		lock();

		int
			colorPre,
			colorPost;
		for (int
				x = 0, y = 0;
				x < _surface->w && y < _surface->h;
				)
		{
			colorPre = static_cast<int>(getPixelColor(x,y));
			switch (colorPre)
			{
				case 0:
					setPixelIterative(&x,&y, 0u);
					break;

				default:
					if (shift > 0)
						colorPost = (colorPre * multer) + shift;
					else
						colorPost = (colorPre + shift) / multer;

					if		(colorLow  != -1 && colorPost < colorLow)	colorPost = colorLow;
					else if	(colorHigh != -1 && colorPost > colorHigh)	colorPost = colorHigh;

					setPixelIterative(&x,&y, static_cast<Uint8>(colorPost));
			}
		}
		unlock();
	}
}

/**
 * Shifts all the colors in this Surface by a specified value while keeping
 * the colors within a fixed-size color-block.
 * @param shift		- shift
 * @param blocksize	- color-block-size (default 16)
 * @param multer	- shift-multiplier (default 1)
 */
void Surface::offsetBlock(
		int shift,
		int blocksize,
		int multer)
{
	if (shift != 0)
	{
		lock();

		int
			colorPre,
			colorLow,
			colorHigh,
			colorPost;
		for (int
				x = 0, y = 0;
				x < _surface->w && y < _surface->h;
				)
		{
			colorPre = static_cast<int>(getPixelColor(x,y));
			switch (colorPre)
			{
				case 0:
					setPixelIterative(&x,&y, 0u);
					break;

				default:
					colorLow  = colorPre / blocksize * blocksize,
					colorHigh = colorLow + blocksize;

					if (shift > 0)
						colorPost = (colorPre * multer) + shift;
					else
						colorPost = (colorPre + shift) / multer;

					if		(colorLow  != -1 && colorPost < colorLow)	colorPost = colorLow;
					else if	(colorHigh != -1 && colorPost > colorHigh)	colorPost = colorHigh;

					setPixelIterative(&x,&y, static_cast<Uint8>(colorPost));
			}
		}
		unlock();
	}
}

/**
 * Inverts all the colors in this Surface according to a middle point.
 * @note Used for effects like shifting a button between pressed and unpressed.
 * @param mid - middle point color
 */
void Surface::invert(Uint8 mid)
{
	lock();
	for (int
			x = 0, y = 0;
			x < _surface->w && y < _surface->h;
			)
	{
		Uint8 color (getPixelColor(x,y));
		if (color != 0u)
			color = static_cast<Uint8>(color + ((mid - color) << 1u));

		setPixelIterative(&x,&y, color);
	}
	unlock();
}

/**
 * Runs any code this Surface needs to keep updating every game cycle like
 * animations and other real-time elements.
 */
void Surface::think() // virtual.
{}

/**
 * Draws the graphic that this Surface contains before it gets blitted onto
 * other surfaces.
 * @note The surface is only redrawn if the flag is set by a property change to
 * avoid unnecessary drawing.
 */
void Surface::draw() // virtual.
{
	_redraw = false;
	clear();
}

/**
 * Blits this Surface onto another one with its position relative to the
 * top-left corner of the target-surface.
 * @note The cropping rectangle controls the area that is blitted.
 * @param surface - pointer to Surface to blit onto
 */
void Surface::blit(const Surface* const srf) // virtual.
{
	if (_visible == true && _hidden == false)
	{
		if (_redraw == true)
			draw();

		SDL_Rect* crop;
		SDL_Rect target;

		if (_crop.w == 0u && _crop.h == 0u)
			crop = nullptr;
		else
			crop = &_crop;

		target.x = static_cast<Sint16>(getX());
		target.y = static_cast<Sint16>(getY());

		SDL_BlitSurface(
					_surface,
					crop,
					srf->getSurface(),
					&target);
	}
}

/**
 * Copies the exact contents of another Surface onto this one.
 * @note Only the content that would overlap both surfaces is copied in
 * accordance with their positions. This is handy for applying effects over
 * another surface without modifying the original.
 * @param srf - pointer to a Surface to copy from
 */
void Surface::copy(const Surface* const srf)
{
/*	SDL_BlitSurface uses color matching, and is therefore unreliable
	as a means to copy the contents of one surface to another; instead
	you have to do this manually.

	SDL_Rect from;
	from.w = getWidth();
	from.h = getHeight();
	from.x = getX() - srf->getX();
	from.y = getY() - srf->getY();

	SDL_BlitSurface(
				srf->getSurface(),
				&from,
				_surface,
				nullptr); */
	const int
		src_x (_x - srf->getX()),
		src_y (_y - srf->getY());
	Uint8 pixel;

	lock();
	for (int
			x = 0, y = 0;
			x < _surface->w && y < _surface->h;
			)
	{
		pixel = srf->getPixelColor(
									src_x + x,
									src_y + y);
		setPixelIterative(&x,&y, pixel);
	}
	unlock();
}

/**
 * Draws a filled rectangle on this Surface.
 * @param rect	- pointer to SDL_Rect
 * @param color	- fill-color
 */
void Surface::drawRect(
		SDL_Rect* const rect,
		Uint8 color)
{
	SDL_FillRect(
			_surface,
			rect,
			static_cast<Uint32>(color));
}

/**
 * Draws a filled rectangle on this Surface.
 * @param x		- x-position in pixels
 * @param y		- y-position in pixels
 * @param w		- width in pixels
 * @param h		- height in pixels
 * @param color	- color of the rectangle
 */
void Surface::drawRect(
		Sint16 x,
		Sint16 y,
		Sint16 w,
		Sint16 h,
		Uint8 color)
{
	SDL_Rect rect;

	rect.x = x;
	rect.y = y;
	rect.w = static_cast<Uint16>(w);
	rect.h = static_cast<Uint16>(h);

	SDL_FillRect(
			_surface,
			&rect,
			static_cast<Uint32>(color));
}

/**
 * Draws a line on this Surface.
 * @param x1	- start x-coordinate in pixels
 * @param y1	- start y-coordinate in pixels
 * @param x2	- end x-coordinate in pixels
 * @param y2	- end y-coordinate in pixels
 * @param color	- color of the line
 */
void Surface::drawLine(
		Sint16 x1,
		Sint16 y1,
		Sint16 x2,
		Sint16 y2,
		Uint8 color)
{
	lineColor(
			_surface,
			x1,y1,
			x2,y2,
			Palette::getRGBA(getPalette(), color));
}

/**
 * Draws a filled circle on this Surface.
 * @param x		- x-coordinate in pixels
 * @param y		- y-coordinate in pixels
 * @param r		- radius in pixels
 * @param color	- color of the circle
 */
void Surface::drawCircle(
		Sint16 x,
		Sint16 y,
		Sint16 r,
		Uint8 color)
{
	filledCircleColor(
					_surface,
					x,y,
					r,
					Palette::getRGBA(getPalette(), color));
}

/**
 * Draws a filled polygon on this Surface.
 * @param x		- pointer to (an array of) x-coordinate(s)
 * @param y		- pointer to (an array of) y-coordinate(s)
 * @param n		- number of points
 * @param color	- color of the polygon
 *
void Surface::drawPolygon(
		const Sint16* const x,
		const Sint16* const y,
		size_t n,
		Uint8 color)
{
	filledPolygonColor(
					_surface,
					x,y,
					static_cast<int>(n),
					Palette::getRGBA(getPalette(), color));
} */

/**
 * Draws a textured polygon on this Surface.
 * @param x			- pointer to (an array of) x-coordinate(s)
 * @param y			- pointer to (an array of) y-coordinate(s)
 * @param n			- number of points
 * @param texture	- pointer to texture for polygon
 * @param dx		- x-offset of texture relative to the screen
 * @param dy		- y-offset of texture relative to the screen
 */
void Surface::drawTexturedPolygon(
		const Sint16* const x,
		const Sint16* const y,
		size_t n,
		Surface* texture,
		int dx,
		int dy)
{
	texturedPolygon(
				_surface,
				x,y,
				static_cast<int>(n),
				texture->getSurface(),
				dx,dy);
}

/**
 * Draws a text-string on this Surface.
 * @param x		- x-coordinate in pixels
 * @param y		- y-coordinate in pixels
 * @param st	- pointer to a string of char's to render
 * @param color	- color of string
 *
void Surface::drawString(
		Sint16 x,
		Sint16 y,
		const char* const st,
		Uint8 color)
{
	stringColor(
			_surface,
			x,y,
			st,
			Palette::getRGBA(getPalette(), color));
} */

/**
 * Sets the position of this Surface in the x-axis.
 * @param x - x-position in pixels
 */
void Surface::setX(int x) // virtual.	// Inheritance gone wild.
{										// Since '_x' is protected it can be used directly by
	_x = x;								// any derived classes; setX() need not even be virtual.
}										// Care should be taken that setX() in the derived
										// classes do not "hide" setX() in this base class.
/**										// who's that chick who sings ... oh yeh, Julie Andrews.
 * Sets the position of this Surface in the y-axis.
 * @param y - y-position in pixels
 */
void Surface::setY(int y) // virtual.
{
	_y = y;
}

/**
 * Sets the visibility of this Surface.
 * @note An nonvisible surface isn't blitted nor does it receive events.
 * @param visible - visibility (default true)
 */
void Surface::setVisible(bool visible) // virtual.
{
	_visible = visible;
}

/**
 * Gets the visible state of this Surface.
 * @return, current visibility
 */
bool Surface::getVisible() const
{
	return _visible;
}

/**
 * Resets the cropping rectangle set for this Surface
 * so the whole surface is blitted.
 */
void Surface::resetCrop()
{
	_crop.x =
	_crop.y = 0;
	_crop.w =
	_crop.h = 0u;
}

/**
 * Gets the cropping rectangle for this Surface.
 * @return, pointer to the cropping rectangle
 */
SDL_Rect* Surface::getCrop()
{
	return &_crop;
}

/**
 * Replaces a specified quantity of colors in this Surface's palette.
 * @param colors		- pointer to the set of colors
 * @param firstcolor	- offset of the first color to replace (default 0)
 * @param ncolors		- amount of colors to replace (default 256)
 */
void Surface::setPalette( // virtual.
		SDL_Color* const colors,
		int firstcolor,
		int ncolors)
{
	if (_surface->format->BitsPerPixel == 8u)
		SDL_SetColors(
					_surface,
					colors,
					firstcolor,
					ncolors);
}

/**
 * This is a separate visibility setting intended for temporary effects like
 * window popups so as to not override the default visibility setting.
 * @note Do not confuse with setVisible!
 * @param hidden - shown or hidden (default true)
 */
void Surface::setHidden(bool hidden) // note: is not virtual.
{
	_hidden = hidden;
}

/**
 * Gets this Surface's special hidden flag.
 * @return, true if hidden
 */
bool Surface::getHidden() const
{
	return _hidden;
}

/**
 * Locks this Surface from outside access for pixel-level access.
 * @note Must be unlocked afterwards.
 * @sa unlock()
 */
void Surface::lock()
{
	SDL_LockSurface(_surface);
}

/**
 * Unlocks this Surface after it's been locked to resume blitting operations.
 * @sa lock()
 */
void Surface::unlock()
{
	SDL_UnlockSurface(_surface);
}


/**
 * Helper struct used for Surface::blitNShade().
 */
struct ColorReplace
{
/**
 * Sets shade and replaces color in a Surface.
 * @note Function used by ShaderDraw in Surface::blitNShade.
 * @param dest		- destination-pixel
 * @param src		- source-pixel
 * @param shade		- value of shade
 * @param newColor	- new color to set (it should be offseted by 4)
 * @param			- notused
 */
static inline void func(
		Uint8& dest,
		const Uint8& src,
		const int& shade,
		const int& newColor,
		const int&)
{
	if (src != 0)
	{
		const int newShade (static_cast<int>(src & 15u) + shade);
		if (newShade > 15) // so dark it would flip over to another color - make it black instead
			dest = 15u;
		else
			dest = static_cast<Uint8>(newColor | newShade);
	}
}
};


/**
 * Helper struct used for Surface::blitNShade().
 */
struct StandartShade
{
/**
 * Sets shade.
 * Function used by ShaderDraw in Surface::blitNShade.
 * @param dest	- destination-pixel
 * @param src	- source-pixel
 * @param shade	- value of shade
 * @param		- notused
 * @param		- notused
 */
static inline void func(
		Uint8& dest,
		const Uint8& src,
		const int& shade,
		const int&,
		const int&)
{
	if (src != 0)
	{
		const int newShade (static_cast<int>(src & 15u) + shade);
		if (newShade > 15) // so dark it would flip over to another color - make it black instead
			dest = 15u;
		else
			dest = static_cast<Uint8>((static_cast<int>(src) & (15 << 4u)) | newShade);
	}
}
};


/**
 * Specific blit function to blit battlescape sprites in different shades in a
 * fast way.
 * @note There is no surface locking here - you have to make sure to lock the
 * surface at the start of blitting and unlock it when done.
 * @param surface		- Surface to blit to
 * @param x				- x-position of Surface blitted to
 * @param y				- y-position of Surface blitted to
 * @param colorOffset	- color offset (generally 0-16) (default 0 = no offset)
 * @param halfRight		- blit only the right half (default false)
 * @param colorGroup	- the actual colorblock + 1 because 0 is no new base color (default 0)
 * @param halfLeft		- kL_add: blits only the left half - NOTE This conflicts w/ 'halfRight' (default false)
 *						  but i am far too lazy to refactor a gajillion blitNShade calls!
 */
void Surface::blitNShade(
		Surface* const surface,
		int x,
		int y,
		int colorOffset,
		bool halfRight,
		int colorGroup,
		bool halfLeft)
{
	ShaderMove<Uint8> src (this, x,y);

	if (halfRight == true)
	{
		GraphSubset graph (src.getDomain());
		graph._x_beg = graph._x_end >> 1u;
		src.setDomain(graph);
	}
	else if (halfLeft == true) // kL_add->
	{
		GraphSubset graph (src.getDomain());
		graph._x_end = graph._x_end >> 1u;
		src.setDomain(graph);
	}

	if (colorGroup != 0)
	{
		(--colorGroup) <<= 4u;
		ShaderDraw<ColorReplace>(
							ShaderSurface(surface),
							src,
							ShaderScalar(colorOffset),
							ShaderScalar(colorGroup));
	}
	else
		ShaderDraw<StandartShade>(
							ShaderSurface(surface),
							src,
							ShaderScalar(colorOffset));
}

/**
 * Specific blit function to blit battlescape terrain data in different shades in a fast way.
 * @param surface		- Surface to blit to
 * @param x				- x-position of Surface blitted to
 * @param y				- y-position of Surface blitted to
 * @param colorOffset	- color offset (generally 0-16) (default 0 = no offset)
 * @param range			- area that limits draw surface
 */
void Surface::blitNShade( // new Yankes' funct.
		Surface* const surface,
		int x,
		int y,
		int colorOffset,
		GraphSubset range)
{
	ShaderMove<Uint8> src (this, x,y);
	ShaderMove<Uint8> dst (surface);

	dst.setDomain(range);

	ShaderDraw<StandartShade>(dst, src, ShaderScalar(colorOffset));
}

/**
 * Sets this Surface to be redrawn.
 * @param redraw - true means redraw (default true)
 */
void Surface::invalidate(bool redraw)
{
	_redraw = redraw;
}

/**
 * Gets the help-description of this Surface for showing in tooltips eg.
 * @return, string ID
 *
std::string Surface::getTooltip() const
{
	return _tooltip;
} */
/**
 * Sets the help-description of this Surface for showing in tooltips eg.
 * @param tooltip - reference a string ID
 *
void Surface::setTooltip(const std::string& tooltip)
{
	_tooltip = tooltip;
} */

/**
 * Re-creates this Surface with a new size.
 * @note Old contents will not be altered and may be cropped to fit the new size.
 * @param width		- width in pixels
 * @param height	- height in pixels
 */
void Surface::resize(
		int width,
		int height)
{
	const int
		bpp (static_cast<int>(_surface->format->BitsPerPixel)), // set up new surface
		pitch (GetPitch(bpp, width));
	void* alignedBuffer (NewAligned(
								bpp,
								width,
								height));
	SDL_Surface* surface (SDL_CreateRGBSurfaceFrom(
												alignedBuffer,
												width,
												height,
												bpp,
												pitch,
												0u,0u,0u,0u));
	if (surface == nullptr)
	{
		throw Exception(SDL_GetError());
	}


	SDL_SetColorKey( // Copy old contents
				surface,
				SDL_SRCCOLORKEY,
				0u);
	SDL_SetColors(
				surface,
				getPalette(),
				0,256);
	SDL_BlitSurface(
				_surface,
				nullptr,
				surface,
				nullptr);


	DeleteAligned(_alignedBuffer); // clear old aligned-buffer
	SDL_FreeSurface(_surface);

	_alignedBuffer = alignedBuffer;
	_surface = surface;

	_clear.w = static_cast<Uint16>(getWidth());
	_clear.h = static_cast<Uint16>(getHeight());
}

/**
 * Sets the width of this Surface.
 * @warning This is not a trivial setter! It will force the surface to be
 * re-created for the new size.
 * @param width - new width in pixels
 */
void Surface::setWidth(int width) // virtual.
{
	resize(width, _surface->h);
	_redraw = true;
}

/**
 * Sets the height of this Surface.
 * @warning This is not a trivial setter! It will force the surface to be
 * re-created for the new size.
 * @param height - new height in pixels
 */
void Surface::setHeight(int height) // virtual.
{
	resize(_surface->w, height);
	_redraw = true;
}

}
