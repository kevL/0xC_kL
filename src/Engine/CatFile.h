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
 * Subclass of std::ifstream to handle CAT-files
 */
class CatFile
	:
		protected std::ifstream
{

private:
	unsigned
		_qtyObjects,
		* _offset,
		* _size;

	public:
		/// Creates a CAT-file.
		explicit CatFile(const char* const path);
		/// Cleans up the CAT-file.
		~CatFile();

		/// Inherits operator ....
		bool operator! () const
		{ return std::ifstream::operator! (); } // *cough

		/// Gets the quantity of objects in the CAT-file.
		unsigned getQuantityObjects() const
		{ return _qtyObjects; }

		/// Gets an internal object's size.
		unsigned getObjectSize(unsigned i) const
		{ return (i < _qtyObjects) ? _size[i] : 0u; }

		/// Loads an object into memory.
		char* load(
				unsigned id,
				bool keepLabel = false);
};

}

#endif
