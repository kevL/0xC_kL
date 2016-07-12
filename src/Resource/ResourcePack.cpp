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

#include "ResourcePack.h"

#include <utility>

//#include "../Engine/Adlib/adlplayer.h" // func_fade()
#include "../Engine/Font.h"
#include "../Engine/Game.h"
#include "../Engine/Music.h"
#include "../Engine/Options.h"
#include "../Engine/RNG.h"
#include "../Engine/Sound.h"
#include "../Engine/SoundSet.h"
#include "../Engine/Surface.h"
#include "../Engine/SurfaceSet.h"

#include "../Resource/XcomResourcePack.h"


namespace OpenXcom
{

constexpr size_t
	ResourcePack::MALE_SCREAM[3u],		// Good Lord, c++
	ResourcePack::FEMALE_SCREAM[3u];	// I won't even ask why/how WINDOW_POPUP[] doesn't require this jiggery.


/**
 * Creates the ResourcePack.
 */
ResourcePack::ResourcePack()
{
	_muteMusic = new Music();
	_muteSound = new Sound();
}

/**
 * Deletes all the loaded resources.
 */
ResourcePack::~ResourcePack()
{
	delete _muteMusic;
	delete _muteSound;

	for (std::map<std::string, Font*>::const_iterator
			i = _fonts.begin();
			i != _fonts.end();
			++i)
		delete i->second;

	for (std::map<std::string, Surface*>::const_iterator
			i = _surfaces.begin();
			i != _surfaces.end();
			++i)
		delete i->second;

	for (std::map<std::string, SurfaceSet*>::const_iterator
			i = _sets.begin();
			i != _sets.end();
			++i)
		delete i->second;

	for (std::map<std::string, Palette*>::const_iterator
			i = _palettes.begin();
			i != _palettes.end();
			++i)
		delete i->second;

	for (std::map<std::string, Music*>::const_iterator
			i = _musics.begin();
			i != _musics.end();
			++i)
		delete i->second;

	for (std::map<std::string, Music*>::const_iterator // sza_MusicRules
			i = _musicFile.begin();
			i != _musicFile.end();
			++i)
		delete i->second;

	for (std::map<std::string, SoundSet*>::const_iterator
			i = _sounds.begin();
			i != _sounds.end();
			++i)
		delete i->second;
}

/**
 * Gets a specific Font from this ResourcePack.
 * @param type - reference to the type of a font
 * @return, pointer to the font
 */
Font* ResourcePack::getFont(const std::string& type) const
{
	const std::map<std::string, Font*>::const_iterator i (_fonts.find(type));

	if (i != _fonts.end())
		return i->second;

	return nullptr;
}

/**
 * Gets a specific Surface from this ResourcePack.
 * @param type - reference to type of a surface
 * @return, pointer to the surface
 */
Surface* ResourcePack::getSurface(const std::string& type) const
{
	const std::map<std::string, Surface*>::const_iterator i (_surfaces.find(type));
	if (i != _surfaces.end())
		return i->second;

	return nullptr;
}

/**
 * Gets a specific SurfaceSet from this ResourcePack.
 * @param type - reference to the type of a surface-set
 * @return, pointer to the surface-set
 */
SurfaceSet* ResourcePack::getSurfaceSet(const std::string& type) const
{
	const std::map<std::string, SurfaceSet*>::const_iterator i (_sets.find(type));
	if (i != _sets.end())
		return i->second;

	return nullptr;
}

/**
 * Gets a specific Music from this ResourcePack.
 * @note This has become redundant w/ getMusicRand() below.
 * @param trackType - reference to the track of a Music
 * @return, pointer to the music
 */
Music* ResourcePack::getMusic(const std::string& trackType) const
{
	if (Options::mute == false)
		return getMusicRand(trackType, "");

	return _muteMusic;
}

/**
 * Checks if a particular music-track is playing.
 * @param trackType - reference to the track to check for
 * @return, true if playing
 */
bool ResourcePack::isMusicPlaying(const std::string& trackType) const
{
	return (_playingMusic == trackType);
//	return _musics[_playingMusic]->isPlaying();
}

/**
 * Plays a specified music-track if it's not already playing.
 * @param trackType		- reference to the track-type
 * @param terrainType	- reference to the terrain-type (default "")
 * @param loops			- number of times to play the track (default -1 infinite)
 */
void ResourcePack::playMusic(
		const std::string& trackType,
		const std::string& terrainType,
		int loops)
{
	//Log(LOG_INFO) << "rp: playMusic current = " << _playingMusic;
	if (_playingMusic != trackType && Options::mute == false)
	{
		//Log(LOG_INFO) << ". new trak = " << trackType;
		const Music* const music (getMusicRand(trackType, terrainType));
		if (music != _muteMusic) // note: '_muteMusic'= nullptr
		{
			if (Options::musicAlwaysLoop == false
				&& (trackType == OpenXcom::res_MUSIC_WIN		// never loop these tracks
					|| trackType == OpenXcom::res_MUSIC_LOSE))	// unless the Option is true
			{
				loops = 1;
			}

			_playingMusic = trackType;
			music->play(loops);
		}
		else
			_playingMusic.clear();
	}
}

/**
 * Fades the currently playing music.
 * @param game		- pointer to the Game object
 * @param millisec	- duration of the fade in milliseconds
 */
void ResourcePack::fadeMusic(
		Game* const game,
		const int millisec)
{
	_playingMusic.clear();

#ifndef __NO_MUSIC
	if (Mix_PlayingMusic() != 0)
	{
		if (Mix_GetMusicType(nullptr) != MUS_MID)
		{
			game->setInputActive(false);
			/* See also:
			SDL_EventState(SDL_MOUSEMOTION, SDL_IGNORE); // <- not MOUSEMOTION tho; want to *keep only* mouse-movements.
			SDL_WarpMouse(static_cast<Uint16>(_xBeforeMouseScrolling), static_cast<Uint16>(_yBeforeMouseScrolling));
			SDL_EventState(SDL_MOUSEMOTION, SDL_ENABLE);
			*/

			Mix_FadeOutMusic(millisec); // fade out!
//			func_fade();

			while (Mix_PlayingMusic() == 1)
			{} // keep fades short (under 1 sec) because things go completely unresponsive to the Player.
		}
		else // SDL_Mixer has trouble with native midi and volume on windows - which is the most likely use case - so f@%# it.
			Mix_HaltMusic();
	}
#endif
}

/**
 * Gets a random Music from this ResourcePack.
 * @param trackType		- reference to the track-type
 * @param terrainType	- reference to the terrain-type
 * @return, pointer to the music
 */
Music* ResourcePack::getMusicRand( // private.
		const std::string& trackType,
		const std::string& terrainType) const
{
	if (Options::mute == false)
	{
		//std::string info = "MUSIC: Request " + trackType;
		//if (terrainType.empty() == false)
		//	info += " for terrainType " + terrainType;
		//Log(LOG_INFO) << info;

		if (_musicAssignment.find(trackType) != _musicAssignment.end())
		{
			const std::map<std::string, std::vector<std::pair<std::string, int>>> assignment (_musicAssignment.at(trackType));
			if (assignment.find(terrainType) != assignment.end())
			{
				const std::vector<std::pair<std::string, int>> terrainMusic (assignment.at(terrainType));
				const std::pair<std::string, int> trackId (terrainMusic[RNG::pick(terrainMusic.size(), true)]);

				//Log(LOG_DEBUG) << "MUSIC: " << trackId.first;
				//Log(LOG_INFO) << "MUSIC: " << trackId.first;

				return _musicFile.at(trackId.first);
			}
			//else Log(LOG_INFO) << "ResourcePack::getMusicRand() No music for terrain - MUTE";
		}
		//else Log(LOG_INFO) << "ResourcePack::getMusicRand() No music assignment - MUTE";
	}

	return _muteMusic;
}

/**
 * Clears a music-assignment.
 * @param trackType		- reference to the track-type
 * @param terrainType	- reference top the terrain-type
 */
void ResourcePack::clearMusicAssignment(
		const std::string& trackType,
		const std::string& terrainType)
{
	if (_musicAssignment.find(trackType) == _musicAssignment.end()
		|| _musicAssignment.at(trackType).find(terrainType) == _musicAssignment.at(trackType).end())
	{
		return;
	}

	_musicAssignment.at(trackType).at(terrainType).clear();
}

/**
 * Makes a music-assignment.
 * @param trackType		- reference to the track-type
 * @param terrainType	- reference to the terrain-type
 * @param files			- reference to a vector of music-files
 * @param midiIdc		- reference to a vector of midi-indices
 */
void ResourcePack::makeMusicAssignment(
		const std::string& trackType,
		const std::string& terrainType,
		const std::vector<std::string>& files,
		const std::vector<int>& midiIdc)
{
	if (_musicAssignment.find(trackType) == _musicAssignment.end())
		_musicAssignment[trackType] = std::map<std::string, std::vector<std::pair<std::string, int>>>();

	if (_musicAssignment.at(trackType).find(terrainType) == _musicAssignment.at(trackType).end())
		_musicAssignment[trackType]
						[terrainType] = std::vector<std::pair<std::string, int>>();

	for (size_t
			i = 0u;
			i != files.size();
			++i)
	{
//		const std::pair<std::string, int> toAdd (std::make_pair<std::string, int>(files.at(i), midiIdc.at(i)));	// pre c++11
		const std::pair<std::string, int> toAdd (std::make_pair(files.at(i), midiIdc.at(i)));					// c++11
		_musicAssignment[trackType]
						[terrainType].push_back(toAdd);
	}
}

/**
 * Gets a specific Sound from this ResourcePack.
 * @param set		- reference to the type of a SoundSet
 * @param soundId	- ID of the sound
 * @return, pointer to the Sound
 */
Sound* ResourcePack::getSound(
		const std::string& soundSet,
		unsigned soundId) const
{
	if (Options::mute == true)
		return _muteSound;
	else
	{
		const std::map<std::string, SoundSet*>::const_iterator i (_sounds.find(soundSet));
		if (i != _sounds.end())
			return i->second->getSound(soundId);
	}
	return nullptr;
}

/**
 * Plays a sound-effect in stereo.
 * @param soundId	- ID of the sound to play
 * @param randAngle	- true to randomize the sound-angle (default false centered)
 */
void ResourcePack::playSoundFx(
		const unsigned soundId,
		const bool randAngle) const
{
	int dir (360); // stereo center
	if (randAngle == true)
	{
		static const int var (67); // maximum deflection left or right
		dir += (RNG::seedless(-var, var)
			  + RNG::seedless(-var, var)) >> 1u;
	}
	getSound("GEO.CAT", soundId)->play(-1, dir);
}

/**
 * Gets a specific Palette from this ResourcePack.
 * @param palType - a PaletteType (Palettes.h)
 * @return, pointer to the palette
 */
Palette* ResourcePack::getPalette(const PaletteType palType) const
{
	return _palettesPt.at(palType);
}

/**
 * Sets the Palette of all the graphics in this ResourcePack.
 * @param colors		- pointer to the set of colors
 * @param firstcolor	- offset of the first color to replace
 * @param ncolors		- amount of colors to replace
 */
void ResourcePack::setPalette(
		SDL_Color* const colors,
		int firstcolor,
		int ncolors)
{
	for (std::map<std::string, Font*>::const_iterator
			i = _fonts.begin();
			i != _fonts.end();
			++i)
	{
		i->second->getSurface()->setPalette(colors, firstcolor, ncolors);
	}

	for (std::map<std::string, Surface*>::const_iterator
			i = _surfaces.begin();
			i != _surfaces.end();
			++i)
	{
		if (i->first.substr(i->first.length() - 3u, i->first.length()) != "LBM")
			i->second->setPalette(colors, firstcolor, ncolors);
	}

	for (std::map<std::string, SurfaceSet*>::const_iterator
			i = _sets.begin();
			i != _sets.end();
			++i)
	{
		i->second->setPalette(colors, firstcolor, ncolors);
	}
}

/**
 * Gets the voxel-data in this ResourcePack.
 * @return, pointer to a vector containing the voxel-data
 */
const std::vector<Uint16>* ResourcePack::getVoxelData() const
{
	return &_voxelData;
}

/**
 * Gets a random background.
 * @note This is mainly used in error messages.
 * @return, reference to a random background screen (.SCR file)
 */
const std::string& ResourcePack::getBackgroundRand() const
{
	static const std::string bg[10u]
	{
		"BACK01.SCR",	// main
		"BACK02.SCR",	// bad boy
		"BACK03.SCR",	// terror
		"BACK04.SCR",	// shooter
		"BACK05.SCR",	// research
		"BACK12.SCR",	// craft in flight
		"BACK13.SCR",	// cash
		"BACK14.SCR",	// service craft
		"BACK16.SCR",	// craft in hangar
		"BACK17.SCR"	// manufacturing
	};

	return bg[static_cast<size_t>(RNG::seedless(0,9))];
}

}
