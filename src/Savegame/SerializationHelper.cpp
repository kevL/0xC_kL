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

#include "SerializationHelper.h"

#include <assert.h>
#include <limits>
#include <sstream>


namespace OpenXcom
{

/**
 *
 */
int unserializeInt(
		Uint8** buffer,
		Uint8 sizeKey)
{
	// The C-spec explicitly requires *(Type*) pointer-accesses to be
	// * sizeof(Type) aligned. This is not guaranteed by the Uint8** buffer
	// * passed in here.
	// * memcpy() is explicitly designed to cope with any address alignment so
	// * use that to avoid undefined behaviour.

	int ret (0);
	switch (sizeKey)
	{
		case 1u:
			ret = static_cast<int>(**buffer);
			break;
		case 2u:
		{
//			ret = *(reinterpret_cast<Sint16*>(*buffer));
			Sint16 t;
			std::memcpy(&t, *buffer, sizeof(t));
			ret = t;
			break;
		}
		case 3u:
			assert(false); // no.
			break;
		case 4u:
		{
//			ret = static_cast<int>(*(reinterpret_cast<Uint32*>(*buffer)));
			Uint32 t;
			std::memcpy(&t, *buffer, sizeof(t));
			ret = t;
			break;
		}

		default:
			assert(false); // get out.
	}

	*buffer += sizeKey;

	return ret;
}

/**
 *
 */
void serializeInt(
		Uint8** buffer,
		Uint8 sizeKey,
		int value)
{
	// The C-spec explicitly requires *(Type*) pointer-accesses to be
	// * sizeof(Type) aligned. This is not guaranteed by the Uint8** buffer
	// * passed in here.
	// * memcpy() is explicitly designed to cope with any address alignment so
	// * use that to avoid undefined behaviour.

	switch (sizeKey)
	{
		case 1u:
			assert(value < 256);
			**buffer = static_cast<Uint8>(value);
			break;
		case 2u:
		{
			assert(value < 65536);
//			*(reinterpret_cast<Sint16*>(*buffer)) = static_cast<Sint16>(value);
			Sint16 s16Value (value);
			std::memcpy(*buffer, &s16Value, sizeof(Sint16));
			break;
		}
		case 3u:
			assert(false); // no.
			break;
		case 4u:
		{
			assert(value < 4294967296);
//			*(reinterpret_cast<Uint32*>(*buffer)) = static_cast<Uint32>(value);
			Uint32 u32Value (value);
			std::memcpy(*buffer, &u32Value, sizeof(Uint32));
			break;
		}

		default:
			assert(false); // get out.
	}

	*buffer += sizeKey;
}

/**
 *
 */
std::string serializeDouble(double value)
{
	std::ostringstream oststr;
	oststr.precision(std::numeric_limits<double>::digits10 + 2);
	oststr << value;

	return oststr.str();
}

}
