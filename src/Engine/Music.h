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

#ifndef OPENXCOM_MUSIC_H
#define OPENXCOM_MUSIC_H

#include <string>

#include <SDL/SDL_mixer.h>


namespace OpenXcom
{

/**
 * Container for music tracks.
 * @note Handles loading and playing various formats through SDL_mixer.
 */
class Music final
{

private:
	Mix_Music* _music;


	public:
		/// Creates a blank music track.
		Music();
		/// Cleans up the music track.
		virtual ~Music();

		/// Loads music from the specified file.
		virtual void load(const std::string& file);
		/// Loads music from a chunk of memory.
		virtual void load(
				const void* data,
				int byteSize);

		/// Plays the music.
		virtual void play(int loop = -1) const;
		/// Stops all music.
		static void stop();
		/// Pauses all music.
		static void pause();
		/// Resumes all music.
		static void resume();

		///
		virtual bool isPlaying();
};

}

#endif
