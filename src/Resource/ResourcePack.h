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

#ifndef OPENXCOM_RESOURCEPACK_H
#define OPENXCOM_RESOURCEPACK_H

#include <map>
#include <string>
#include <vector>

#include <SDL.h>

#include "../Engine/Palette.h"


namespace OpenXcom
{

class Font;
class Game;
class Music;
class Palette;
class Sound;
class SoundSet;
class Surface;
class SurfaceSet;


/**
 * Packs of external game media.
 * @note Resource packs contain all the game media that's loaded externally like
 * graphics, fonts, languages, audio and globe data. The game is still hardcoded
 * to X-Com resources so for now this just serves to load all the files in one
 * place.
 */
class ResourcePack
{

private:
	std::string _playingMusic;

	Music* _muteMusic;
	Sound* _muteSound;

	/// Gets a random music. This is private to prevent access. Use playMusic() instead.
	Music* getMusicRand(
			const std::string& trackType,
			const std::string& terrainType) const;


	protected:
		std::map<std::string, Font*> _fonts;
		std::map<std::string, Music*> _musics;
		std::map<std::string, Palette*> _palettes;
		std::map<std::string, Surface*> _surfaces;
		std::map<std::string, SurfaceSet*> _sets;
		std::map<std::string, SoundSet*> _sounds;

		std::map<std::string, Music*> _musicFile;
		std::map<std::string, std::map<std::string, std::vector<std::pair<std::string, int>>>> _musicAssignment;

		std::vector<Uint16> _voxelData;

		std::map<PaletteType, Palette*> _palettesPt;


		public:
			static constexpr size_t // TODO: relabel these identifiers w/ appropriate prefixes, eg. sfx_GRAVLIFT, gfx_SMOKE, etc.
				BUTTON_PRESS		=   0u,
				WINDOW_POPUP[3u]	=  {1u,2u,3u},

				EXPLOSION_OFFSET	=   0u,
//				SMOKE_OFFSET		=   7u, // replaced w/ external resource file

				SMALL_EXPLOSION		=   2u,
				DOOR_OPEN			=   3u,
				LARGE_EXPLOSION		=   5u,
				FLYING_SOUND		=  15u,
				FLYING_SOUND_HQ		=  70u,
				ITEM_RELOAD			=  17u,
				ITEM_UNLOAD_HQ		=  74u,
				SLIDING_DOOR_OPEN	=  20u,
				SLIDING_DOOR_CLOSE	=  21u,
				GRAVLIFT_SOUND		=  40u,
				WALK_OFFSET			=  22u,
				ITEM_DROP			=  38u,
				ITEM_THROW			=  39u,
				MALE_SCREAM[3u]		= {41u,42u,43u},
				FEMALE_SCREAM[3u]	= {44u,45u,46u},

				UFO_FIRE			=   9u,
				UFO_HIT				=  12u,
				UFO_CRASH			=  11u,
				UFO_EXPLODE			=  11u,
				INTERCEPTOR_HIT		=  10u,
				INTERCEPTOR_EXPLODE	=  13u,

				GEOSCAPE_CURSOR		= 252u,
				BASESCAPE_CURSOR	= 252u,
				BATTLESCAPE_CURSOR	= 144u,
				UFOPAEDIA_CURSOR	= 252u,
				GRAPHS_CURSOR		= 252u;


			/// Creates a ResourcePack with a folder's contents.
			ResourcePack();
			/// Cleans up the ResourcePack.
			virtual ~ResourcePack();

			/// Gets a particular Font.
			Font* getFont(const std::string& type) const;

			/// Gets a particular Surface.
			Surface* getSurface(const std::string& type) const;
			/// Gets a particular SurfaceSet.
			SurfaceSet* getSurfaceSet(const std::string& type) const;

			/// Gets a particular music.
			Music* getMusic(const std::string& trackType) const;
			/// Checks if a particular music track is playing.
			bool isMusicPlaying(const std::string& trackType) const;
			/// Plays a particular music.
			void playMusic(
					const std::string& trackType,
					const std::string& terrainType = "",
					int loops = -1);
			/// Fades the currently playing music.
			void fadeMusic(
					Game* const game,
					const int millisec);

			/// Clear a music assignment
			void clearMusicAssignment(
					const std::string& trackType,
					const std::string& terrainType);
			/// Make a music assignment
			void makeMusicAssignment(
					const std::string& trackType,
					const std::string& terrainType,
					const std::vector<std::string>& files,
					const std::vector<int>& midiIdc);

			/// Gets a particular sound.
			Sound* getSound(
					const std::string& soundSet,
					size_t soundId) const;
			/// Plays a sound effect in stereo.
			void playSoundFx(
					const int sound,
					const bool randAngle = false) const;

			/// Gets a particular palette by PaletteType.
			Palette* getPalette(const PaletteType palType) const;
			/// Sets a the colors of a Palette.
			void setPalette(
					SDL_Color* const colors,
					int firstcolor = 0,
					int ncolors = 256);

			/// Gets list of voxel-data.
			const std::vector<Uint16>* getVoxelData() const;

			/// Gets a random background.
			const std::string& getBackgroundRand() const;
};

}

#endif
