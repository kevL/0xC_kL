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

#ifndef OPENXCOM_SURFACE_H
#define OPENXCOM_SURFACE_H

#include <string>

#include <SDL.h>


namespace OpenXcom
{

class Font;
class Language;


/**
 * Element that is blit (rendered) onto the screen.
 * @note Mainly an encapsulation for SDL's SDL_Surface struct so it borrows a
 * lot of its terminology. Takes care of all the common rendering tasks and
 * color effects while serving as the base class for more specialized screen
 * elements.
 */
class Surface
{

protected:
	bool
		_hidden,
		_redraw,
		_visible;
	int
		_x,
		_y;

//	std::string _tooltip;

	SDL_Rect
		_crop,
		_clear;
	SDL_Surface* _surface;
	void* _alignedBuffer;

	///
	void resize(
			int width,
			int height);


	public:
		/// Creates a Surface with the specified size and position.
		Surface(
				int width,
				int height,
				int x = 0,
				int y = 0,
				int bpp = 8);
		/// Creates a Surface from an existing one.
		Surface(const Surface& other);
		/// Cleans up the Surface.
		virtual ~Surface();

		/// Loads an X-Com SCR graphic.
		void loadScr(const std::string& file);
		/// Loads an X-Com SPK graphic.
		void loadSpk(const std::string& file);
		/// Loads a TFTD BDY graphic.
//		void loadBdy(const std::string& file);
		/// Loads a general image file.
		void loadImage(const std::string& file);

		/// Clears the Surface's contents with a specified colour.
		void clear(Uint32 color = 0);
		/// Offsets the Surface's colors by a set amount.
		void offset(
				int shift,
				int colorLow = -1,
				int colorHigh = -1,
				int multer = 1);
		/// Inverts the Surface's colors.
		void invert(Uint8 mid);

		/// Runs Surface functionality every cycle
		virtual void think();
		/// Draws the Surface's graphic.
		virtual void draw();
		/// Blits the Surface onto another one.
		virtual void blit(Surface* surface);
		/// Copies a portion of another Surface into this one.
		void copy(Surface* surface);

		/// Initializes the Surface's various text resources.
		virtual void initText(
				Font*,
				Font*,
				const Language*)
		{};

		/// Draws a filled rectangle on the Surface.
		void drawRect(
				SDL_Rect* rect,
				Uint8 color);
		/// Draws a filled rectangle on the Surface.
		void drawRect(
				Sint16 x,
				Sint16 y,
				Sint16 w,
				Sint16 h,
				Uint8 color);
		/// Draws a line on the Surface.
		void drawLine(
				Sint16 x1,
				Sint16 y1,
				Sint16 x2,
				Sint16 y2,
				Uint8 color);
		/// Draws a filled circle on the Surface.
		void drawCircle(
				Sint16 x,
				Sint16 y,
				Sint16 r,
				Uint8 color);
		/// Draws a filled polygon on the Surface.
		void drawPolygon(
				Sint16* x,
				Sint16* y,
				size_t n,
				Uint8 color);
		/// Draws a textured polygon on the Surface.
		void drawTexturedPolygon(
				Sint16* x,
				Sint16* y,
				size_t n,
				Surface* texture,
				int dx,
				int dy);
		/// Draws a string on the Surface.
		void drawString(
				Sint16 x,
				Sint16 y,
				const char* s,
				Uint8 color);

		/// Sets the Surface's palette.
		virtual void setPalette(
				SDL_Color* const colors,
				int firstcolor = 0,
				int ncolors = 256);

		/**
		 * Returns the Surface's 8-bpp palette.
		 * @return, pointer to the palette's colors
		 */
		SDL_Color* getPalette() const
		{ return _surface->format->palette->colors; }

		/// Sets the X position of the Surface.
		virtual void setX(int x);
		/**
		 * Returns the position of the Surface in the x-axis.
		 * @return, X position in pixels
		 */
		int getX() const
		{ return _x; }

		/// Sets the Y position of the Surface.
		virtual void setY(int y);
		/**
		 * Returns the position of the Surface in the y-axis.
		 * @return, Y position in pixels
		 */
		int getY() const
		{ return _y; }

		/// Sets the Surface's visibility.
		virtual void setVisible(bool visible = true);
		/// Gets the Surface's visibility.
		bool getVisible() const;

		/// Resets the cropping rectangle for the Surface.
		void resetCrop();
		/// Gets the cropping rectangle for the Surface.
		SDL_Rect* getCrop();

		/**
		 * Changes the color of a pixel in the Surface relative to the top-left
		 * corner.
		 * @param x		- X position of the pixel
		 * @param y		- Y position of the pixel
		 * @param color	- color for the pixel
		 */
		void setPixelColor(
				int x,
				int y,
				Uint8 color)
		{
			if (   x > -1 && x < _surface->w
				&& y > -1 && y < _surface->h)
			{
				static_cast<Uint8*>(_surface->pixels)
						 [y * static_cast<int>(_surface->pitch)
						+ x * static_cast<int>(_surface->format->BytesPerPixel)] = color;
			}
		}


		/**
		 * Changes the color of a pixel in the Surface and returns the next
		 * pixel position.
		 * @note Useful when changing a lot of pixels in a row - eg. loading
		 * images.
		 * @param x		- pointer to the X position of the pixel; changes to the next X position in the sequence
		 * @param y		- pointer to the Y position of the pixel; changes to the next Y position in the sequence
		 * @param color	- color for the pixel
		 */
		void setPixelIterative( // setPixelColorIterative
				int* x,
				int* y,
				Uint8 color)
		{
			setPixelColor(*x,*y, color);

			++(*x);
			if (*x == _surface->w)
			{
				++(*y);
				*x = 0;
			}
		}

		/**
		 * Returns the color of a specified pixel in the Surface.
		 * @param x - X position of the pixel
		 * @param y - Y position of the pixel
		 * @return, color of the pixel
		 */
		Uint8 getPixelColor(
				int x,
				int y) const
		{
			if (   x > -1 && x < _surface->w
				&& y > -1 && y < _surface->h)
			{
				return static_cast<Uint8*>(_surface->pixels)
							 [y * static_cast<int>(_surface->pitch)
							+ x * static_cast<int>(_surface->format->BytesPerPixel)];
			}
			return 0;
		}

		/**
		 * Returns the internal SDL_Surface for SDL calls.
		 * @return, pointer to the surface
		 */
		SDL_Surface* getSurface() const
		{ return _surface; }

		/**
		 * Returns the width of the Surface.
		 * @return, width in pixels
		 */
		int getWidth() const
		{ return _surface->w; }
		/// Sets the width of the Surface.
		virtual void setWidth(int width); // should be Unit16
		/**
		 * Returns the height of the Surface.
		 * @return, height in pixels
		 */
		int getHeight() const
		{ return _surface->h; }
		/// Sets the height of the Surface.
		virtual void setHeight(int height); // should be Unit16

		/// Sets the Surface's special hidden flag.
		void setHidden(bool hidden = true); // note: is not virtual.
		/// Gets the Surface's special hidden flag.
		bool getHidden() const;

		/// Locks the Surface.
		void lock();
		/// Unlocks the Surface.
		void unlock();

		/// Specific blit function to blit battlescape terrain data in different shades in a fast way.
		void blitNShade(
				Surface* surface,
				int x,
				int y,
				int colorOffset = 0,
				bool halfRight = false,
				int colorGroup = 0,
				bool halfLeft = false);

		/// Invalidates the Surface which forces it to be redrawn
		void invalidate(bool redraw = true);

		/// Gets the tooltip of the Surface.
//		std::string getTooltip() const;
		/// Sets the tooltip of the Surface.
//		void setTooltip(const std::string& tooltip);

		/// Sets the color of the Surface.
		virtual void setColor(Uint8 /*color*/)
		{};
		/// Sets the secondary color of the Surface.
		virtual void setSecondaryColor(Uint8 /*color*/)
		{};
		/// Sets the border color of the Surface.
		virtual void setBorderColor(Uint8 /*color*/)
		{};
};

}

#endif
