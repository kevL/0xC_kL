/*
 * Copyright 2010-2020 OpenXcom Developers.
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

#include "AdlibMusic.h"

//#include <algorithm>
#include <fstream>

#include "Exception.h"
#include "Game.h"
#include "Logger.h"
#include "Options.h"

#include "Adlib/adlplayer.h"
#include "Adlib/fmopl.h"


extern FM_OPL* opl[2u];


namespace OpenXcom
{

int AdlibMusic::delay = 0;
int AdlibMusic::rate  = 0;

std::map<int, int> AdlibMusic::delayRates;


/**
 * Initializes a new music track.
 * @param volume - music volume modifier (1.0 = 100%) (default 1.f)
 */
AdlibMusic::AdlibMusic(float volume)
	:
		Music(),
		_data(0),
		_size(0),
		_volume(volume)
{
	rate = Options::audioSampleRate;

	if (!opl[0])
		opl[0] = OPLCreate(
						OPL_TYPE_YM3812,
						3579545,
						rate);

	if (!opl[1])
		opl[1] = OPLCreate(
						OPL_TYPE_YM3812,
						3579545,
						rate);

	// magical value - length of 1 tick per sample-rate
	if (delayRates.empty())
	{
		delayRates[8000]	= 114 * 4;
		delayRates[11025]	= 157 * 4;
		delayRates[16000]	= 228 * 4;
		delayRates[22050]	= 314 * 4;
		delayRates[32000]	= 456 * 4;
		delayRates[44100]	= 629 * 4;
		delayRates[48000]	= 685 * 4;
	}
}

/**
 * Deletes the loaded music content.
 */
AdlibMusic::~AdlibMusic()
{
	if (opl[0])
	{
		stop();
		OPLDestroy(opl[0]);
		opl[0] = 0;
	}

	if (opl[1])
	{
		OPLDestroy(opl[1]);
		opl[1] = 0;
	}

	delete[] _data;
}

/**
 * Loads a music file from a specified filename.
 * @param file - reference to the name of the music file
 */
void AdlibMusic::load(const std::string& file)
{
	std::ifstream ifstr (file.c_str(), std::ios::binary);
	if (ifstr.fail() == true)
	{
		throw Exception(file + " not found");
	}

	ifstr.seekg(0, std::ifstream::end);
	_size = static_cast<size_t>(ifstr.tellg());
	ifstr.seekg(0);

	_data = new char[_size];
	ifstr.read(_data, _size);
}

/**
 * Loads a music file from a specified memory chunk.
 * @param data	- pointer to the music file in memory
 * @param bytes	- size of the music file in bytes
 */
void AdlibMusic::load(
		const void* data,
		int bytes)
{
	_data = (char*)data;

	if (*(unsigned char*)_data <= 56)
		bytes += *(unsigned char*)_data;

	_size = static_cast<size_t>(bytes);
}

/**
 * Plays the contained music track.
 * @param loop - not used (default -1). But needs to be here because there is a
 * virtual function in base class Music which has a default set - meaning this
 * won't overload the virtual function unless that default is removed. But that
 * ain't gonna happen ....
 */
void AdlibMusic::play(int /*loop*/) const
{
#ifndef __NO_MUSIC
	if (Options::mute == false)
	{
		stop();
		func_setup_music(reinterpret_cast<unsigned char*>(_data), static_cast<int>(_size));
		func_set_music_volume(static_cast<int>(127.f * _volume));
		Mix_HookMusic(player, (void*)this);
	}
#endif
}

/**
 * Custom audio player.
 * @param udata		- user data to send to the player
 * @param stream	- raw audio to output
 * @param len		- length of audio to output
 */
void AdlibMusic::player(
		void* udata,
		Uint8* stream,
		int len)
{
#ifndef __NO_MUSIC
	if (Options::volMusic == 0)
		return;

	if (Options::musicAlwaysLoop
		&& !func_is_music_playing())
	{
		AdlibMusic* music = (AdlibMusic*)udata;
		music->play();

		return;
	}

	while (len != 0)
	{
		if (!opl[0] || !opl[1])
			return;

		int i = std::min(delay, len);
		if (i)
		{
			float volume (static_cast<float>(Game::volExp(Options::volMusic)));
			YM3812UpdateOne(
						opl[0],
						(INT16*)stream,
						i / 2,
						2,
						volume);
			YM3812UpdateOne(
						opl[1],
						((INT16*)stream) + 1,
						i / 2,
						2,
						volume);

			stream += i;
			delay -= i;
			len -= i;
		}

		if (!len)
			return;

		func_play_tick();

		delay = delayRates[rate];
	}
#endif
}

bool AdlibMusic::isPlaying()
{
#ifndef __NO_MUSIC
	if (Options::mute == false)
	{
		return func_is_music_playing();
	}
#endif
	return false;
}

}
