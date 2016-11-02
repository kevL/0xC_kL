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

#include "CatFile.h"

#include <SDL/SDL.h>


namespace OpenXcom
{

/**
 * Creates a CAT-file stream.
 * @note A CAT-file starts with an index of the offset and size of every file
 * contained within. Each file consists of a filename followed by its contents.
 * @param path - pointer to the full path of the CAT-file
 */
CatFile::CatFile(const char* const path)
	:
		std::ifstream(
					path,
					std::ios::in | std::ios::binary),
		_qtyObjects(0u),
		_offset(nullptr),
		_size(nullptr)
{
	// Get amount of files
	std::ifstream::read(
					reinterpret_cast<char*>(&_qtyObjects),
					sizeof(_qtyObjects));

	_qtyObjects = SDL_SwapLE32(_qtyObjects);
	_qtyObjects /= 2u * sizeof(_qtyObjects);

	// Get object offsets
	std::ifstream::seekg(0, std::ios::beg);

	_offset = new unsigned[_qtyObjects];
	_size   = new unsigned[_qtyObjects];

	for (unsigned
			i = 0u;
			i != _qtyObjects;
			++i)
	{
		std::ifstream::read(
						reinterpret_cast<char*>(&_offset[i]),
						sizeof(*_offset));
		_offset[i] = SDL_SwapLE32(_offset[i]);

		std::ifstream::read(
						reinterpret_cast<char*>(&_size[i]),
						sizeof(*_size));
		_size[i] = SDL_SwapLE32(_size[i]);
	}
}

/**
 * Frees associated memory.
 */
CatFile::~CatFile()
{
	delete[] _offset;
	delete[] _size;

	std::ifstream::close();
}

/**
 * Loads an object into memory.
 * @param id		- object-ID to load
 * @param keepLabel	- true to preserve internal filename (default false)
 * @return, pointer to the loaded object
 */
char* CatFile::load(
		unsigned id,
		bool keepLabel)
{
	if (id < _qtyObjects)
	{
		std::ifstream::seekg(_offset[id], std::ios::beg);

		unsigned char labelSize (static_cast<unsigned char>(peek()));
		if (labelSize < 57u)
		{
			if (keepLabel == false)
			{
				std::ifstream::seekg(
								labelSize + 1u,
								std::ios::cur);
			}
			else
				_size[id] += labelSize + 1u;
		}

		char* const object (new char[_size[id]]);
		std::ifstream::read(object, static_cast<int>(_size[id]));
		return object;
	}
	return nullptr;
}

}
