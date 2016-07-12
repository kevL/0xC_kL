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

#include "SoundSet.h"

#include <cstring>
//#include <sstream>

#include "CatFile.h"
#include "Exception.h"
#include "Sound.h"


namespace OpenXcom
{

/**
 * Sets up the empty SoundSet.
 */
SoundSet::SoundSet()
{}

/**
 * Deletes the Sounds from memory.
 */
SoundSet::~SoundSet()
{
	for (std::map<unsigned, Sound*>::const_iterator
			i = _sounds.begin();
			i != _sounds.end();
			++i)
	{
		delete i->second;
	}
}

/**
 * Loads the contents of an X-Com CAT file which usually contains a set of
 * sound-files.
 * @note The CAT starts with an index of the offset and size of every file
 * contained within. Each file consists of a filename followed by its contents.
 * @param file	- reference filename of the CAT set
 * @param wav	- true if sounds are in WAV format (default true)
 * @sa http://www.ufopaedia.org/index.php?title=SOUND
 */
void SoundSet::loadCat(
		const std::string& file,
		bool wav)
{
	// Load CAT file
	CatFile sndFile (file.c_str());
	if (!sndFile)
	{
		throw Exception(file + " not found");
	}

	// Load each sound file
	for (unsigned
			i = 0u;
			i < sndFile.getQuantityObjects();
			++i)
	{
		// Read WAV chunk
		unsigned char* sound (reinterpret_cast<unsigned char*>(sndFile.load(i)));
		unsigned bytes (sndFile.getObjectSize(i));

		// If there's no WAV header (44 bytes), add it
		// Assuming sounds are 8-bit 8000Hz (DOS version)
		unsigned char* newsound (nullptr);
		if (wav == false)
		{
			if (bytes != 0u)
			{
				char header[]
				{
					'R','I','F','F',
					0x00,0x00,0x00,0x00,
					'W','A','V','E',
					'f','m','t',' ',
					0x10,0x00,0x00,0x00,
					0x01,0x00,0x01,0x00,
					0x11,0x2b,0x00,0x00,
					0x11,0x2b,0x00,0x00,
					0x01,0x00,0x08,0x00,
					'd','a','t','a',
					0x00,0x00,0x00,0x00
				};

				for (unsigned int
						j = 0u;
						j != bytes;
						++j)
				{
//					sound[j] *= 4; // scale to 8 bits
					sound[j] = static_cast<unsigned char>(sound[j] << 2u); // scale to 8 bits
				}

				if (bytes > 5u) // skip 5 garbage name bytes at beginning
					bytes -= 5u;

				if (bytes > 0u) // omit trailing null byte
					--bytes;

				unsigned headersize (bytes + 36u);
				std::memcpy(
						header + 4,
						&headersize,
						sizeof(headersize));

				unsigned soundsize (bytes);
				std::memcpy(
						header + 40,
						&soundsize,
						sizeof(soundsize));

				newsound = new unsigned char[44u + bytes * 2u];
				std::memcpy(
						newsound,
						header,
						44u);

				if (bytes > 0u)
					std::memcpy(
							newsound + 44u,
							sound + 5u,
							bytes);

				const Uint32 step16 ((8000u << 16u) / 11025u);
				Uint8* wet (newsound + 44u);
				unsigned newsize (0u);
				for (Uint32
						offset16 = 0u;
						(offset16 >> 16u) < bytes;
						offset16 += step16, ++wet, ++newsize)
				{
					*wet = sound[5u + (offset16 >> 16u)];
				}
				bytes = newsize + 44u;
			}
		}
		else if (0x40 == sound[0x18]
			&& 0x1F == sound[0x19]
			&& 0x00 == sound[0x1A]
			&& 0x00 == sound[0x1B])
		{
			// so it's WAV, but in 8 khz, we have to convert it to 11 khz sound
			unsigned char* const sound2 (new unsigned char[bytes * 2u]);

			// rewrite the samplerate in the header to 11 khz
			sound[0x18] = 0x11;
			sound[0x19] = 0x2B;
			sound[0x1C] = 0x11;
			sound[0x1D] = 0x2B;

			// copy and do the conversion...
			std::memcpy(
					sound2,
					sound,
					bytes);

			const Uint32 step16 ((8000u << 16) / 11025u);
			Uint8* wet (sound2 + 44u);
			unsigned newsize (0u);
			for (Uint32
					offset16 = 0u;
					(offset16 >> 16u) < bytes - 44u;
					offset16 += step16, ++wet, ++newsize)
			{
				*wet = sound[44u + (offset16 >> 16u)];
			}

			bytes = newsize + 44u;

			// Rewrite the number of samples in the WAV file
			std::memcpy(
					sound2 + 0x28,
					&newsize,
					sizeof(newsize));

			// Ok, now replace the original with the converted:
			delete[] sound;

			sound = sound2; // okay!
		}

		Sound* const pSound (new Sound());
		try
		{
			if (bytes == 0u)
			{
				throw Exception("Invalid sound file");
			}

			if (wav == true)
				pSound->load(sound, bytes);
			else
				pSound->load(newsound, bytes);
		}
		catch (Exception)
		{ /* ignore junk in the file */ }

		_sounds[i] = pSound;

		delete[] sound;

		if (wav == false)
			delete[] newsound;
	}
}

/**
 * Gets a particular wave from this SoundSet.
 * @param id - ID in the set
 * @return, pointer to the Sound
 */
Sound* SoundSet::getSound(unsigned id)
{
	if (_sounds.find(id) != _sounds.end())
		return _sounds[id];

	return nullptr;
}

/**
 * Creates and returns a particular wave in the SoundSet.
 * @param id - ID in the set
 * @return, pointer to the Sound
 */
Sound* SoundSet::addSound(unsigned id)
{
	_sounds[id] = new Sound();

	return _sounds[id];
}

/**
 * Gets the total amount of sounds currently stored in the set.
 * @return, number of sounds
 *
size_t SoundSet::getTotalSounds() const // private.
{
	return _sounds.size();
} */

/**
 * Loads individual contents of a TFTD CAT file by index.
 * @note A set of sound files. The CAT starts with an index of the offset and
 * size of every file contained within. Each file consists of a filename
 * followed by its contents.
 * @param file	- reference filename of the CAT set
 * @param index	- which index in the cat file to load
 * @sa http://www.ufopaedia.org/index.php?title=SOUND
 *
void SoundSet::loadCatByIndex(
		const std::string& file,
		int index)
{
	// Load CAT file
	CatFile sndFile (file.c_str());
	if (!sndFile)
	{
		throw Exception(file + " not found");
	}

	if (index >= sndFile.getAmount())
	{
		std::ostringstream err;
		err << file << " does not contain " << index << " sound files.";
		throw Exception(err.str());
	}

	// Read WAV chunk
	unsigned char* const sound = (unsigned char*)sndFile.load(index);
	unsigned int bytes = sndFile.getObjectSize(index);

	// there's no WAV header (44 bytes), add it
	// sounds are 8-bit 11025Hz, signed
	unsigned char* newsound = 0;

	if (bytes != 0)
	{
		char header[] =
		{
			'R','I','F','F',
			0x00,0x00,0x00,0x00,
			'W','A','V','E',
			'f','m','t',' ',
			0x10,0x00,0x00,0x00,
			0x01,0x00,0x01,0x00,
			0x11,0x2b,0x00,0x00,
			0x11,0x2b,0x00,0x00,
			0x01,0x00,0x08,0x00,
			'd','a','t','a',
			0x00,0x00,0x00,0x00
		};

		if (bytes > 5) // skip 5 garbage name bytes at beginning
			bytes -= 5;

		if (bytes > 0) // omit trailing null byte
			--bytes;

		int
			headersize = bytes + 36,
			soundsize = bytes;
		std::memcpy(
			header + 4,
			&headersize,
			sizeof(headersize));
		std::memcpy(
			header + 40,
			&soundsize,
			sizeof(soundsize));

		newsound = new unsigned char[44 + bytes];
		std::memcpy(
			newsound,
			header,
			44);

		// TFTD sounds are signed so convert them.
		for (unsigned int
				i = 5;
				i != bytes + 5;
				++i)
		{
			int value = static_cast<int>(sound[i]) + 128;
			sound[i] = static_cast<uint8_t>(value);
		}

		if (bytes > 0)
			std::memcpy(
					newsound + 44,
					sound + 5,
					bytes);

		bytes = bytes + 44;
	}

	Sound* const ptrSound = new Sound();
	try
	{
		if (bytes == 0)
		{
			throw Exception("Invalid sound file");
		}

		ptrSound->load(newsound, bytes);
	}
	catch (Exception)
	{
		// Ignore junk in the file
	}

	_sounds[getTotalSounds()] = ptrSound;

	delete[] sound;
	delete[] newsound;
} */

}
