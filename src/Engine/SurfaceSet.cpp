/*
 * Copyright 2010-2019 OpenXcom Developers.
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

#include "SurfaceSet.h"

#include <fstream>

#include "Exception.h"
//#include "Logger.h"
#include "Surface.h"


namespace OpenXcom
{

/**
 * Sets up an empty SurfaceSet for frames of the specified size.
 * @param width		- frame-width in pixels
 * @param height	- frame-height in pixels
 */
SurfaceSet::SurfaceSet(
		int width,
		int height)
	:
		_width(width),
		_height(height)
{}

/**
 * Performs a deep copy of an existing SurfaceSet.
 * @param other - reference to a SurfaceSet to copy
 */
SurfaceSet::SurfaceSet(const SurfaceSet& other)
{
	_width = other._width;
	_height = other._height;

	for (std::map<int, Surface*>::const_iterator
			i = other._frames.begin();
			i != other._frames.end();
			++i)
		_frames[i->first] = new Surface(*i->second);
}

/**
 * Deletes the images from memory.
 */
SurfaceSet::~SurfaceSet()
{
	for (std::map<int, Surface*>::iterator
			i = _frames.begin();
			i != _frames.end();
			++i)
		delete i->second;
}

/**
 * Loads the contents of an X-Com set of PCK/TAB image-files into this SurfaceSet.
 * The PCK file contains an RLE compressed image while the TAB file contains
 * the offsets to each frame in the image.
 * @param pck - reference to filename of the PCK sprite-file or spriteset-file
 * @param tab - reference to filename of the TAB offsets (default "")
 * @sa http://www.ufopaedia.org/index.php?title=Image_Formats#PCK
 */
void SurfaceSet::loadPck(
		const std::string& pck,
		const std::string& tab)
{
	//Log(LOG_INFO) << "SurfaceSet::loadPck() " << pck;
	int q;

	if (tab.empty() == false)	// Load TAB and get image-offsets -> actually, don't get the offsets; just
	{							// use the TAB to deter if the spriteset uses 2-byte or 4-type tabword-lengths.
		std::ifstream ifstr (tab.c_str(), std::ios::in | std::ios::binary);
		if (ifstr.fail() == true)
		{
			throw Exception(tab + " not found");
		}

		ifstr.seekg(0, std::ios::end);
		const int len (static_cast<int>(ifstr.tellg()));

		if (len == 2) // has 1 ufo-sprite
		{
			q = 1; // definitely 2-byte offset
		}
		else // has 2+ ufo-sprites or 1+ tftd-sprites
		{
			ifstr.seekg(0, std::ios::beg);

			// this test relies on the fact that the first sprite-offset for UFO
			// is always
			// 00 00
			// but the first sprite-offset for TFTD is always ... that is, the
			// first + second sprite-offsets for UFO can never be
			// 00 00 00 00
			// (as long as the tabfile contains >2 bytes)

			int offsetTest;
			ifstr.read(reinterpret_cast<char*>(&offsetTest), 4);

			if (offsetTest != 0)	// is UFO
				q = len >> 1u;		// 2-byte offsets
			else					// is TFTD
				q = len >> 2u;		// 4-byte offsets
		}

		ifstr.close();

		for (int
				i  = 0;
				i != q;
				++i)
		{
			_frames[i] = new Surface(_width, _height);
		}
	}
	else
	{
		q = 1;
		_frames[0] = new Surface(_width, _height);
	}

	std::ifstream ifstr (pck.c_str(), std::ios::in | std::ios::binary); // Load PCK and put pixels in surfaces
	if (ifstr.fail() == true)
	{
		throw Exception(pck + " not found");
	}

	Uint8 val;
	int
		x,y;

	for (int
			i = 0;
			i != q;
			++i)
	{
		x =
		y = 0;

		_frames[i]->lock();
		ifstr.read(reinterpret_cast<char*>(&val), 1);	// the first byte in a PCK-sprite's data denotes
		for (Uint8										// an initial quantity of lines that are transparent
				j  = 0u;
				j != val;
				++j)
		{
			for (int
					k  = 0;
					k != _width;
					++k)
			{
				_frames[i]->setPixelIterative(&x,&y, 0u);
			}
		}

		while (ifstr.read(reinterpret_cast<char*>(&val), 1)
			&& val != 255u) // 255u is the end-of-sprite data marker
		{
			if (val == 254u) // 254u is a marker that says, the next byte's quantity of pixels shall be transparent
			{
				ifstr.read(reinterpret_cast<char*>(&val), 1);
				for (Uint8
						j  = 0u;
						j != val;
						++j)
				{
					_frames[i]->setPixelIterative(&x,&y, 0u);
				}
			}
			else
				_frames[i]->setPixelIterative(&x,&y, val);
		}
		_frames[i]->unlock();
	}

	ifstr.close();
}

/**
 * Loads the contents of an X-Com DAT image-file into this SurfaceSet.
 * Unlike the PCK, a DAT file is an uncompressed image with no offsets so these
 * have to be figured out manually, usually by splitting the image into equal
 * portions.
 * @param file - reference to the filename of the DAT image
 * @sa http://www.ufopaedia.org/index.php?title=Image_Formats#SCR_.26_DAT
 */
void SurfaceSet::loadDat(const std::string& file)
{
	// Load file and put pixels in surface
	std::ifstream ifstr (file.c_str(), std::ios::in | std::ios::binary);
	if (ifstr.fail() == true)
	{
		throw Exception(file + " not found");
	}

	ifstr.seekg(0, std::ios::end);
	std::streamoff datSize (ifstr.tellg());
	ifstr.seekg(0, std::ios::beg);

	int q (static_cast<int>(datSize) / (_width * _height));
	//Log(LOG_INFO) << "loadDat total = " << q;

	for (int
			i = 0;
			i != q;
			++i)
	{
		_frames[i] = new Surface(_width, _height);
	}

	Uint8 val;
	int
		x (0),
		y (0),
		i (0);

	_frames[i]->lock();
	while (ifstr.read(reinterpret_cast<char*>(&val), 1))
	{
		_frames[i]->setPixelIterative(&x,&y, val);

		if (y >= _height)
		{
			_frames[i]->unlock();

			x =
			y = 0;

			if (++i < q)
				_frames[i]->lock();
			else
				break;
		}
	}

	ifstr.close();
}

/**
 * Returns a particular frame from this SurfaceSet.
 * @param i - frame-index in the set
 * @return, pointer to the respective Surface
 */
Surface* SurfaceSet::getFrame(int i)
{
	if (_frames.find(i) != _frames.end())
		return _frames[i];

	return nullptr;
}

/**
 * Creates and returns a particular frame in this SurfaceSet.
 * @param i - frame-index in the set
 * @return, pointer to the respective Surface
 */
Surface* SurfaceSet::addFrame(int i)
{
	return (_frames[i] = new Surface(_width, _height));
}

/**
 * Gets the full width of a frame in this SurfaceSet.
 * @return, width in pixels
 */
int SurfaceSet::getWidth() const
{
	return _width;
}

/**
 * Gets the full height of a frame in this SurfaceSet.
 * @return, height in pixels
 */
int SurfaceSet::getHeight() const
{
	return _height;
}

/**
 * Gets the total quantity of frames in this SurfaceSet.
 * @return, quantity of frames
 *
size_t SurfaceSet::getTotalFrames() const
{
	return _frames.size();
} */

/**
 * Replaces a specified quantity of colors in all of the frames.
 * @param colors		- pointer to the set of colors
 * @param firstcolor	- offset of the first color to replace (default 0)
 * @param ncolors		- amount of colors to replace (default 256)
 */
void SurfaceSet::setPalette(
		SDL_Color* const colors,
		int firstcolor,
		int ncolors)
{
	for (std::map<int, Surface*>::iterator
			i = _frames.begin();
			i != _frames.end();
			++i)
	{
		(*i).second->setPalette(
							colors,
							firstcolor,
							ncolors);
	}
}

/**
 * Gets a map of the frames in this SurfaceSet.
 * @return, pointer to a map of ints w/ pointers to Surface corresponding to the frames
 */
std::map<int, Surface*>* SurfaceSet::getFrames()
{
	return &_frames;
}

}
