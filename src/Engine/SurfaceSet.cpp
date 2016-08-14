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
 * @param pck - reference to filename of the PCK image
 * @param tab - reference to filename of the TAB offsets (default "")
 * @sa http://www.ufopaedia.org/index.php?title=Image_Formats#PCK
 */
void SurfaceSet::loadPck(
		const std::string& pck,
		const std::string& tab)
{
	//Log(LOG_INFO) << "SurfaceSet::loadPck() " << pck;
	int q;

	if (tab.empty() == false) // Load TAB and get image-offsets
	{
		std::ifstream ifstr (tab.c_str(), std::ios::in | std::ios::binary);
		if (ifstr.fail() == true)
		{
			throw Exception(tab + " not found");
		}

		std::streampos
			start (ifstr.tellg()),
			stop;

		int offset;
		ifstr.read(
				reinterpret_cast<char*>(&offset),
				sizeof(offset));
		ifstr.seekg(0, std::ios::end);

		stop = ifstr.tellg();
		const int tabSize (static_cast<int>(stop - start));

		if (offset != 0)
			q = tabSize >> 1u; // 16-bit offsets
		else
			q = tabSize >> 2u; // 32-bit offsets

		ifstr.close();

		for (int
				i = 0;
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
		ifstr.read(reinterpret_cast<char*>(&val), 1);
		for (Uint8
				j = 0u;
				j != val;
				++j)
		{
			for (int
					k = 0;
					k != _width;
					++k)
			{
				_frames[i]->setPixelIterative(&x,&y, 0u);
			}
		}

		while (ifstr.read(reinterpret_cast<char*>(&val), 1)
			&& val != 255u)
		{
			if (val == 254u)
			{
				ifstr.read(reinterpret_cast<char*>(&val), 1);
				for (Uint8
						j = 0u;
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
 * Unlike the PCK, a DAT file is an uncompressed image with no
 * offsets so these have to be figured out manually, usually
 * by splitting the image into equal portions.
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
	_frames[i] = new Surface(_width, _height);
	return _frames[i];
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
 * Replaces a certain amount of colors in all of the frames.
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
