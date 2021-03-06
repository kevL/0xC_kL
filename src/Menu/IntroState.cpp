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

#include "IntroState.h"

#include <cstring>

//#include <SDL/SDL_mixer.h>

#include "MainMenuState.h"

#include "../Engine/CrossPlatform.h"
#include "../Engine/FlcPlayer.h"
#include "../Engine/Game.h"
#include "../Engine/Logger.h"
#include "../Engine/Music.h"
#include "../Engine/Options.h"
#include "../Engine/Screen.h"
#include "../Engine/Sound.h"
#include "../Engine/Surface.h"

//#include "../Engine/Adlib/adlplayer.h"

#include "../Resource/XcomResourcePack.h"

#include "../Ruleset/Ruleset.h"
#include "../Ruleset/RuleVideo.h"


namespace OpenXcom
{

/**
 * Initializes all the elements for the Intro video.
 * @param wasLetterBoxed - true if the Screen was letterboxed
 */
IntroState::IntroState(const bool wasLetterBoxed)
	:
		_wasLetterBoxed(wasLetterBoxed),
		_flcPlayer(nullptr)
{
//	_oldMusic = Options::volMusic;
//	_oldSound = Options::volFx;
	// music volume is the main, and sound volume as a fallback
//	Options::volMusic = Options::volFx = std::max(_oldMusic, _oldSound/8);

	_game->setVolume(
				Options::volMusic + 19, // my music files need a bit of a boost for these intro segs.
				Options::volFx + 9);

	const std::map<std::string, RuleVideo*>* const videoRulesets (_game->getRuleset()->getVideos());
	const std::map<std::string, RuleVideo*>::const_iterator pRule (videoRulesets->find("intro"));
	if (pRule != videoRulesets->end())
	{
		const RuleVideo* const videoRule (pRule->second);
		const std::vector<std::string>* const videos (videoRule->getVideos());
		for (std::vector<std::string>::const_iterator
				i = videos->begin();
				i != videos->end();
				++i)
		{
			_introFiles.push_back(CrossPlatform::getDataFile(*i));
		}
	}
	_introSoundFileDOS = CrossPlatform::getDataFile("SOUND/INTRO.CAT");
	_introSoundFileWin = CrossPlatform::getDataFile("SOUND/SAMPLE3.CAT");
}

/**
 * dTor.
 */
IntroState::~IntroState()
{}


typedef struct
{
	std::string catFile;
	int sound;
	int volume;
} soundInFile;


// the pure MS-DOS experience
static soundInFile introCatOnlySounds[]
{
	{"INTRO.CAT",  0x0, 32},
	{"INTRO.CAT",  0x1, 32},
	{"INTRO.CAT",  0x2, 32},
	{"INTRO.CAT",  0x3, 32},
	{"INTRO.CAT",  0x4, 32},
	{"INTRO.CAT",  0x5, 32},
	{"INTRO.CAT",  0x6, 32},
	{"INTRO.CAT",  0x7, 32},
	{"INTRO.CAT",  0x8, 32},
	{"INTRO.CAT",  0x9, 32},
	{"INTRO.CAT",  0xa, 32},
	{"INTRO.CAT",  0xb, 32},
	{"INTRO.CAT",  0xc, 32},
	{"INTRO.CAT",  0xd, 32},
	{"INTRO.CAT",  0xe, 32},
	{"INTRO.CAT",  0xf, 32},
	{"INTRO.CAT", 0x10, 32},
	{"INTRO.CAT", 0x11, 32},
	{"INTRO.CAT", 0x12, 32},
	{"INTRO.CAT", 0x13, 32},
	{"INTRO.CAT", 0x14, 32},
	{"INTRO.CAT", 0x15, 32},
	{"INTRO.CAT", 0x16, 32},
	{"INTRO.CAT", 0x17, 32},
	{"INTRO.CAT", 0x18, 32},
	{"INTRO.CAT", 0x18, 32}
};


static soundInFile sample3CatOnlySounds[]
{
	{"SAMPLE3.CAT",	 24,  32},	// machine gun
	{"SAMPLE3.CAT",	  5,  32},	// plasma rifle
	{"SAMPLE3.CAT",	 23,  32},	// rifle
	{"SAMPLE3.CAT",	  6,  32},	// some kind of death noise, urgh?
	{"SAMPLE3.CAT",	  9,  64},	// mutdie
	{"SAMPLE3.CAT",	  7,  64},	// dying alien
	{"SAMPLE3.CAT",	 27,  64},	// another dying alien
	{"SAMPLE3.CAT",	  4,  32},	// ??? ship flying? alien screech?
	{"SAMPLE3.CAT",	0x8,  32},	// fscream
	{"SAMPLE3.CAT",	 11,  32},	// alarm
	{"SAMPLE3.CAT",	  4,  32},	// gun spinning up?
	{"INTRO.CAT",	0xb,  32},	// reload; this one's not even in sample3
	{"SAMPLE3.CAT",	 19,  48},	// whoosh
	{"INTRO.CAT",	0xd,  32},	// feet, also not in sample3
	{"SAMPLE3.CAT",	  2,  32},	// low pulsating hum
	{"SAMPLE3.CAT",	 30,  32},	// energise
	{"SAMPLE3.CAT",	 21,  32},	// hatch
	{"SAMPLE3.CAT",	  0,  64},	// phizz -- no equivalent in sample3.cat?
	{"SAMPLE3.CAT",	 13,  32},	// warning
	{"SAMPLE3.CAT",	 14,  32},	// detected
	{"SAMPLE3.CAT",	 19,  64},	// UFO flyby whoosh?
	{"SAMPLE3.CAT",	  3,  32},	// growl
	{"SAMPLE3.CAT",	 15, 128},	// voice
	{"SAMPLE3.CAT",	 12,  32},	// beep 1
	{"SAMPLE3.CAT",	 18,  32},	// takeoff
	{"SAMPLE3.CAT",	 20,  32}	// another takeoff/landing sound?? if it exists?
};


// an attempt at a mix of (subjectively) the best sounds from the two versions
// difficult because we can't find a definitive map from old sequence numbers to SAMPLE3.CAT indices
// probably only the Steam version of the game comes with both INTRO.CAT and SAMPLE3.CAT
static soundInFile hybridIntroSounds[]
{
	{"SAMPLE3.CAT",   24,  32},	// machine gun
	{"SAMPLE3.CAT",    5,  32},	// plasma rifle
	{"SAMPLE3.CAT",   23,  32},	// rifle
	{"INTRO.CAT",      3,  32},	// some kind of death noise, urgh?
	{"INTRO.CAT",    0x4,  64},	// mutdie
	{"INTRO.CAT",    0x5,  64},	// dying alien
	{"INTRO.CAT",    0x6,  64},	// another dying alien
	{"INTRO.CAT",    0x7,  32},	// ??? ship flying? alien screech?
	{"SAMPLE3.CAT",  0x8,  32},	// fscream
	{"SAMPLE3.CAT",   11,  32},	// alarm
	{"SAMPLE3.CAT",    4,  32},	// gun spinning up?
	{"INTRO.CAT",    0xb,  32},	// reload; this one's not even in sample3
	{"SAMPLE3.CAT",   19,  48},	// whoosh
	{"INTRO.CAT",    0xd,  32},	// feet, also not in sample3
	{"INTRO.CAT",    0xe,  32},	// low pulsating hum
	{"SAMPLE3.CAT",   30,  32},	// energise
	{"SAMPLE3.CAT",   21,  32},	// hatch
	{"INTRO.CAT",   0x11,  64},	// phizz
	{"SAMPLE3.CAT",   13,  32},	// warning
	{"SAMPLE3.CAT",   14,  32},	// detected
	{"SAMPLE3.CAT",   19,  64},	// UFO flyby whoosh?
	{"INTRO.CAT",   0x15,  32},	// growl
	{"SAMPLE3.CAT",   15, 128},	// voice
	{"SAMPLE3.CAT",   12,  32},	// beep 1
	{"SAMPLE3.CAT",   18,  32},	// takeoff
	{"SAMPLE3.CAT",   20,  32}	// another takeoff/landing sound?? if it exists?
};
// sample3: 18 is takeoff, 20 is landing; 19 is flyby whoosh sound, not sure for which craft


static soundInFile* introSounds[]
{
	hybridIntroSounds,
	introCatOnlySounds,
	sample3CatOnlySounds,
	nullptr
};


typedef struct
{
	int frameNumber;
	int sound;
} introSoundEffect;


static introSoundEffect introSoundTrack[]
{
	{    0,   0x200}, // inserting this to keep the code simple
	{  149,    0x11}, // searchlight *whoosh*
	{  150,    0x11}, // searchlight doubling
	{  151,    0x11}, // searchlight tripling
	{  152,    0x11}, // searchlight quadrupling
	{  173,    0x0C},
	{  183,    0x0E},
	{  205,    0x15},
	{  211,   0x201},
	{  211,   0x407},
	{  223,     0x7},
	{  250,     0x1},
	{  253,     0x1},
	{  255,     0x1},
	{  257,     0x1},
	{  260,     0x1},
	{  261,     0x3},
	{  262,     0x1},
	{  264,     0x1},
	{  268,     0x1},
	{  270,     0x1},
	{  272,     0x5},
	{  272,     0x1},
	{  274,     0x1},
	{  278,     0x1},
	{  280,     0x1},
	{  282,     0x8},
	{  282,     0x1},
	{  284,     0x1},
	{  286,     0x1},
	{  288,     0x1},
	{  290,     0x1},
	{  292,     0x6},
	{  292,     0x1},
	{  296,     0x1},
	{  298,     0x1},
	{  300,     0x1},
	{  302,     0x1},
	{  304,     0x1},
	{  306,     0x1},
	{  308,     0x1},
	{  310,     0x1},
	{  312,     0x1},
	{  378,   0x202},
	{  378,     0x9}, // alarm
	{  386,     0x9},
	{  393,     0x9},
	{  399,    0x17}, // bleeps
	{  433,    0x17},
	{  463,    0x12}, // warning
	{  477,    0x12},
	{  487,    0x13}, // ufo detected
	{  495,    0x16}, // voice
	{  501,    0x16},
//	{  512,     0xd}, // feet -- not in original
//	{  514,     0xd}, // feet -- not in original
//	{  523,    0x0B}, // rifle grab
	{  528,    0x0B}, // rifle grab, delayed (lock & load)
//	{  523,     0xd}, // feet -- not in original
//	{  525,     0xd}, // feet -- not in original
	{  534,    0x18},
	{  535,   0x405},
	{  560,   0x407},
	{  577,    0x14},
	{  582,   0x405},
//	{  582,    0x18}, // landing!
	{  582,    0x19}, // corrected landing sound!
	{  613,   0x407},
	{  615,    0x10},
	{  635,    0x14},
	{  638,    0x14},
	{  639,    0x14},
	{  644,     0x2},
	{  646,     0x2},
	{  648,     0x2},
	{  650,     0x2},
	{  652,     0x2},
	{  654,     0x2},
	{  656,     0x2},
	{  658,     0x2},
	{  660,     0x2},
	{  662,     0x2},
	{  664,     0x2},
	{  666,     0x2},
	{  668,   0x401},
	{  681,   0x406},
	{  687,   0x402},
	{  689,   0x407},
	{  694,    0x0A},
	{  711,   0x407},
	{  711,     0x0},
	{  714,     0x0},
	{  716,     0x4},
	{  717,     0x0},
	{  720,     0x0},
	{  723,     0x0},
	{  726,     0x5},
	{  726,     0x0},
	{  729,     0x0},
	{  732,     0x0},
	{  735,     0x0},
	{  738,     0x0},
	{  741,     0x0},
	{  742,     0x6},
	{  744,     0x0},
	{  747,     0x0},
	{  750,     0x0},
	{  753,     0x0},
	{  756,     0x0},
	{  759,     0x0},
	{  762,     0x0},
	{  765,     0x0},
	{  768,     0x0},
	{  771,     0x0},
	{  774,     0x0},
	{  777,     0x0},
	{  780,     0x0},
	{  783,     0x0},
	{  786,     0x0},
	{  789,    0x15}, // big growl
	{  790,    0x15}, // 2x loud growl, chorus
	{  807,     0x2},
	{  810,     0x2},
	{  812,     0x2},
	{  814,     0x2},
	{  816,     0x0},
	{  819,     0x0},
	{  822,     0x0},
	{  824,   0x40A},
//	{  824,     0x5}, // out of place alien yell, gaaach
//	{  827,     0x6}, // out of place alien yell
	{  835,    0x0F}, // beam up aliens
	{  841,    0x0F},
	{  845,    0x0F},
//	{  850,    0x0F},
	{  855,   0x407},
	{  879,    0x0C},
	{65535,  0xFFFF}
};

/**
 * The AudioSequence struct.
 */
static struct AudioSequence
{

int trackPosition;

const ResourcePack* rp;

Music* pMusic;
Sound* pSound;
FlcPlayer* _flcPlayer;


/**
 * Contructs an AudioSequence.
 */
AudioSequence(
		const ResourcePack* const res,
		FlcPlayer* const flcPlayer)
	:
		rp(res),
		pMusic(nullptr),
		pSound(nullptr),
		trackPosition(0),
		_flcPlayer(flcPlayer)
{}

/**
 * Overloads operator().
 */
void operator ()()
{
	while (_flcPlayer->getFrameCount() >= introSoundTrack[trackPosition].frameNumber)
	{
		const int soundTrigger (introSoundTrack[trackPosition].sound);
		if (soundTrigger & 0x200)
		{
#ifndef __NO_MUSIC
			switch (soundTrigger)
			{
				case 0x200:
					Log(LOG_DEBUG) << "Playing gmintro1";
					pMusic = rp->getMusic(OpenXcom::res_MUSIC_START_INTRO1);
					pMusic->play(1);
					break;

				case 0x201:
					Log(LOG_DEBUG) << "Playing gmintro2";
					pMusic = rp->getMusic(OpenXcom::res_MUSIC_START_INTRO2);
					pMusic->play(1);
					break;

				case 0x202:
					Log(LOG_DEBUG) << "Playing gmintro3";
					pMusic = rp->getMusic(OpenXcom::res_MUSIC_START_INTRO3);
					pMusic->play(1);
//					Mix_HookMusicFinished(_flcPlayer::stop);
			}
#endif
		}
		else if (soundTrigger & 0x400)
		{
			const int speed (soundTrigger & 0xff);
			_flcPlayer->setHeaderSpeed(speed);
			Log(LOG_DEBUG) << "Frame delay now: " << speed;
		}
		else if (soundTrigger <= 0x19)
		{
			for (soundInFile // try hybrid sound set then intro.cat or sample3.cat alone
					**sounds = introSounds;
					*sounds != nullptr;
					++sounds)
			{
				const soundInFile* const sf ((*sounds) + soundTrigger);
				Log(LOG_DEBUG) << "playing: " << sf->catFile << ":" << sf->sound << " for index " << soundTrigger;

				if ((pSound = rp->getSound(sf->catFile, static_cast<unsigned>(sf->sound))) != nullptr)
				{
					pSound->play(-1); // kL
//					int channel = trackPosition %4; // use at most four channels to play sound effects
//					pSound->play(channel);
//					double ratio = static_cast<double>(Options::volFx) / static_cast<double>(MIX_MAX_VOLUME);
//					Mix_Volume(channel, static_cast<int>(static_cast<double>(sf->volume) * ratio));

					break;
				}
				else Log(LOG_DEBUG) << "Couldn't play " << sf->catFile << ":" << sf->sound;
			}
		}
		++trackPosition;
	}
}

} *audioSequence;


/**
 * c++ magic-syntax
 */
static void audioHandler()
{
	(*audioSequence)();
}


/**
 * Plays the intro video(s).
 */
void IntroState::init()
{
	State::init();

	Options::keepAspectRatio = _wasLetterBoxed;

	const size_t videoFilesQty (_introFiles.size());
	if (videoFilesQty != 0u)
	{
		const int
			dx ((Options::baseXResolution - Screen::ORIGINAL_WIDTH)  >> 1u),
			dy ((Options::baseYResolution - Screen::ORIGINAL_HEIGHT) >> 1u);

		if (videoFilesQty == 1u // Original introduction video.
			&& (   CrossPlatform::fileExists(_introSoundFileDOS) == true
				|| CrossPlatform::fileExists(_introSoundFileWin) == true))
		{
			//Log(LOG_INFO) << "ORIGINAL INTRO";
			const std::string& videoFile (_introFiles[0u]);
			if (CrossPlatform::fileExists(videoFile) == true)
			{
				_flcPlayer = new FlcPlayer();
				audioSequence = new AudioSequence(
											_game->getResourcePack(),
											_flcPlayer);
				_flcPlayer->init(
								videoFile.c_str(),
								&audioHandler,
								_game,
								dx,dy);
				_flcPlayer->play(true);
				_flcPlayer->delay(FINISH_PERSIST);

				delete _flcPlayer;
				delete audioSequence;
				endVideo();
			}
		}
		else
		{
			//Log(LOG_INFO) << "not ORIGINAL";
			_flcPlayer = new FlcPlayer();

			for (std::vector<std::string>::const_iterator
					i = _introFiles.begin();
					i != _introFiles.end();
					++i)
			{
				const std::string& videoFile (*i);
				if (CrossPlatform::fileExists(videoFile) == true)
				{
					_flcPlayer->init(
								videoFile.c_str(),
								nullptr,
								_game,
								dx,dy);
					_flcPlayer->play(false);
					_flcPlayer->deInit();
				}

				if (_flcPlayer->wasSkipped() == true)
					break;
			}

			delete _flcPlayer;
			endVideo();
		}
	}
//	else // TODO: slides
//	{}

	// This uses baseX/Y options for Geoscape & Basescape:
	Options::baseXResolution = Options::baseXGeoscape; // kL
	Options::baseYResolution = Options::baseYGeoscape; // kL
	// This sets Geoscape and Basescape to default (320x200) IG and the config.
/*kL
	Screen::updateScale(
					Options::geoscapeScale,
					Options::geoscapeScale,
					Options::baseXGeoscape,
					Options::baseYGeoscape,
					true); */
	_game->getScreen()->resetDisplay(false);

	// I can't get a real update to work here or on forward to MainMenu,
	// unfortunately. So this ensures merely that the cursor appears centered.
	SDL_WarpMouse(
			static_cast<Uint16>(_game->getScreen()->getWidth()  >> 1u),
			static_cast<Uint16>(_game->getScreen()->getHeight() >> 1u));

	_game->setState(new MainMenuState());
}

/**
 * Ends the video - postvideo niceties.
 * @note Music at least has already stopped abruptly if skipped-by-user before
 * this is called.
 */
void IntroState::endVideo()
{
	//Log(LOG_INFO) << "intro: endVideo()";
	//Log(LOG_INFO) << ". playing Music= " << Mix_PlayingMusic();

	// These fades can be done only in 8-bpp otherwise instantly black it.
	// NOTE: See also Ufopaedia/UfopaediaStartState cTor.
	// This algorithm should be consolidated in Screen. And there should be
	// some sort of corresponding fade-in function also.
	if (_game->getScreen()->getSurface()->getSurface()->format->BitsPerPixel == 8u)
	{
		//Log(LOG_INFO) << ". 8bpp TRUE";
		static const Uint8  FADE_STEPS (10u);
		static const Uint32 FADE_DELAY (45u);
/*
		const int fadeDur (static_cast<int>(FADE_DELAY) * static_cast<int>(FADE_STEPS));

		Mix_FadeOutChannel(-1, fadeDur);

#ifndef __NO_MUSIC
//		_game->getResourcePack()->fadeMusic(_game, FADE_DELAY * FADE_STEPS);
		if (Mix_GetMusicType(nullptr) != MUS_MID)
		{
			//Log(LOG_INFO) << ". . fade music";
			Mix_FadeOutMusic(fadeDur);
//			func_fade();

//			while (Mix_PlayingMusic() == 1)
//			{
//				Log(LOG_INFO) << ". . music still playing ... loop";
//			}
		}
		else
		{
			//Log(LOG_INFO) << ". . halt music";
			Mix_HaltMusic();
		}
#endif
*/
//		int deltaVol (Mix_VolumeMusic(-1));
//		deltaVol /= FADE_STEPS;
		//Log(LOG_INFO) << ". . deltaVol= " << deltaVol;


		SDL_Color
			pal[256u],
			pal2[256u];

		std::memcpy(
				pal,
				_game->getScreen()->getPalette(),
				sizeof(SDL_Color) * 256u);

		for (Uint8
				i = FADE_STEPS;
				i != 0u;
				--i)
		{
			//Log(LOG_INFO) << ". . fade step= " << (int)i;
//			SDL_Event event;
//			if (SDL_PollEvent(&event) != 0 && event.type == SDL_KEYDOWN) // cf. FlcPlayer::SDLPolling()
//				break; // don't bother w/ escaping the fade-out ...

			for (size_t
					j = 0u;
					j != 256u;
					++j)
			{
				pal2[j].r = static_cast<Uint8>(pal[j].r * i / FADE_STEPS);
				pal2[j].g = static_cast<Uint8>(pal[j].g * i / FADE_STEPS);
				pal2[j].b = static_cast<Uint8>(pal[j].b * i / FADE_STEPS);
				pal2[j].unused = pal[j].unused;
			}

			_game->getScreen()->setPalette(
										pal2,
										0,
										256,
										true);
			_game->getScreen()->flip();

//			int vol (Mix_VolumeMusic(-1));
			//Log(LOG_INFO) << ". . vol= " << vol;
//			Mix_VolumeMusic(vol - deltaVol);

			SDL_Delay(FADE_DELAY);
		}
	}
/*	else
	{
		//Log(LOG_INFO) << ". 8bpp FALSE";
		Mix_HaltChannel(-1);

#ifndef __NO_MUSIC
 		Mix_HaltMusic();
#endif
	} */

	_game->getScreen()->clear();
	_game->getScreen()->flip();

//	Options::volMusic = _oldMusic;
//	Options::volFx = _oldSound;
	//Log(LOG_INFO) << ". set Game Volume";
	_game->setVolume(
				Options::volMusic,
				Options::volFx,
				Options::volUi);

/*	//Log(LOG_INFO) << ". stop Sound";
	Sound::stop();

#ifndef __NO_MUSIC
	//Log(LOG_INFO) << ". stop Music";
	Music::stop();
#endif */
}

}
