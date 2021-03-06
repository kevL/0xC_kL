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

#ifndef OPENXCOM_SOUND_H
#define OPENXCOM_SOUND_H

#include <string>

#include <SDL/SDL_mixer.h>


namespace OpenXcom
{

/**
 * Container for sound-effects.
 * @note Handles loading and playing various formats through SDL_mixer.
 */
class Sound
{

private:
	Mix_Chunk* _sound;


	public:
		/// Creates a sound-effect.
		Sound();
		/// Cleans up the sound-effect.
		~Sound();

		/// Loads a sound-effect from a specified file.
		void load(const std::string& file);
		/// Loads a sound-effect from a chunk of memory.
		void load(
				const void* data,
				unsigned bytes);

		/// Plays the sound-effect.
		void play(
				int channel = -1,
				int angle = 0,
				int distance = 0) const;
		/// Stops all sound-effects.
		static void stop();

		/// Plays the sound-effect repeatedly.
//		void loop();
		/// Stops the looping sound-effect.
//		void stopLoop();
};

}

#endif
