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

#ifndef OPENXCOM_CATFILE_H
#define OPENXCOM_CATFILE_H

#include <fstream>


namespace OpenXcom
{

/**
 * Subclass of std::ifstream to handle CAT files
 */
class CatFile
	:
		protected std::ifstream
{

private:
	unsigned
		_amount,
		* _offset,
		* _size;

	public:
		/// Creates a CAT file stream.
		explicit CatFile(const char* path);
		/// Cleans up the stream.
		~CatFile();

		/// Inherit operator.
		bool operator! () const
		{ return std::ifstream::operator! (); } // *cough

		/// Get amount of objects.
		int getAmount() const
		{ return _amount; }

		/// Get object size.
		unsigned getObjectSize(unsigned i) const
		{ return (i < _amount) ? _size[i] : 0u; }

		/// Load an object into memory.
		char* load(
				unsigned i,
				bool name = false);
};

}

#endif
