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

#include "Music.h"

#include "Exception.h"
#include "Language.h"
#include "Logger.h"
#include "Options.h"

//#include "Adlib/adlplayer.h"
//#include "AdlibMusic.h"


namespace OpenXcom
{

/**
 * Initializes a new music track.
 */
Music::Music()
	:
		_music(nullptr)
{}

/**
 * Deletes the loaded music content.
 */
Music::~Music()
{
#ifndef __NO_MUSIC
	stop();
	Mix_FreeMusic(_music);
#endif
}

/**
 * Loads this Music track from a specified file.
 * @param file - reference to the file to load
 */
void Music::load(const std::string& file)
{
#ifndef __NO_MUSIC
	// SDL only takes UTF-8 filenames
	// so here's an ugly hack to match this ugly reasoning
	const std::string utf8 (Language::wstrToUtf8(Language::fsToWstr(file)));

	_music = Mix_LoadMUS(utf8.c_str());
	if (_music == nullptr)
	{
		throw Exception(Mix_GetError());
	}
#endif
}

/**
 * Loads this Music track from a specified memory chunk.
 * @param data		- pointer to the music file in memory
 * @param byteSize	- size of the music file in bytes
 */
void Music::load(
		const void* data,
		int byteSize)
{
#ifndef __NO_MUSIC
	SDL_RWops* const rwops (SDL_RWFromConstMem(
											data,
											byteSize));
	_music = Mix_LoadMUS_RW(rwops);
	SDL_FreeRW(rwops);

	if (_music == nullptr)
	{
		throw Exception(Mix_GetError());
	}
#endif
}

/**
 * Plays this Music track.
 * @param loop - number of times to loop (default -1 infinite)
 */
void Music::play(int loop) const
{
#ifndef __NO_MUSIC
	if (Options::mute == false
		&& _music != nullptr)
	{
		stop();

		if (Mix_PlayMusic(_music, loop) == -1)
			Log(LOG_WARNING) << Mix_GetError();
	}
#endif
}

/**
 * Stops all music playing.
 */
void Music::stop()
{
#ifndef __NO_MUSIC
	if (Options::mute == false)
	{
//kL	func_mute();
//kL	Mix_HookMusic(nullptr, nullptr);
		Mix_HaltMusic();
	}
#endif
}

/**
 * Pauses music playback when game loses focus.
 */
void Music::pause()
{
#ifndef __NO_MUSIC
	if (Options::mute == false)
	{
		Mix_PauseMusic();
//kL	if (Mix_GetMusicType(nullptr) == MUS_NONE)
//kL		Mix_HookMusic(nullptr, nullptr);
	}
#endif
}

/**
 * Resumes music playback when game gains focus.
 */
void Music::resume()
{
#ifndef __NO_MUSIC
	if (Options::mute == false)
	{
		Mix_ResumeMusic();
//kL	if (Mix_GetMusicType(nullptr) == MUS_NONE)
//kL		Mix_HookMusic(AdlibMusic::player, nullptr);
	}
#endif
}

/**
 * Returns true if music is playing.
 * @return, true if playing
 */
bool Music::isPlaying()
{
#ifndef __NO_MUSIC
	if (Options::mute == false)
		return (Mix_Playing(-1) != 0);
#endif

	return false;
}

}
