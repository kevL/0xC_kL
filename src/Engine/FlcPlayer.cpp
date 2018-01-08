/*
 * Copyright 2010-2018 OpenXcom Developers.
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

/*
 * Based on http://www.libsdl.org/projects/flxplay/
 */

#ifdef _MSC_VER
#	ifndef _SCL_SECURE_NO_WARNINGS
#		define _SCL_SECURE_NO_WARNINGS
#	endif
#endif

#include "FlcPlayer.h"

//#include <algorithm>
//#include <cassert>
//#include <cmath>
#include <cstring>
#include <fstream>
//#include <string.h>

#include <SDL/SDL_mixer.h>

#include "Game.h"
#include "Logger.h"
#include "Options.h"
#include "Screen.h"
#include "Surface.h"


namespace OpenXcom
{

// Taken from: http://www.compuphase.com/flic.htm
enum FileTypes
{
	FLI_TYPE = 0xAF11,
	FLC_TYPE = 0xAF12
};

enum ChunkTypes
{
	COLOR_256	= 0x04,
	FLI_SS2		= 0x07, // or DELTA_FLC
	COLOR_64	= 0x0B,
	FLI_LC		= 0x0C, // or DELTA_FLI
	BLACK		= 0x0D,
	FLI_BRUN	= 0x0F, // or BYTE_RUN
	FLI_COPY	= 0x10,

	AUDIO_CHUNK		= 0xAAAA, // This is the only exception, it's from TFTD.
	PREFIX_CHUNK	= 0xF100,
	FRAME_TYPE		= 0xF1FA
};

enum ChunkOpcodes
{
	PACKETS_COUNT	= 0x0000, // 0000000000000000
	LAST_PIXEL		= 0x8000, // 1000000000000000
	SKIP_LINES		= 0xc000, // 1100000000000000

	MASK = SKIP_LINES
};


/**
 * Creates and initializes FlcPlayer.
 */
FlcPlayer::FlcPlayer()
	:
		_fileBuf(nullptr),
		_mainScreen(nullptr),
		_realScreen(nullptr),
		_game(nullptr),
		_videoFrameData(nullptr),
		_chunkData(nullptr),
		_audioFrameData(nullptr),
		_screenWidth(0),
		_screenHeight(0),
		_screenDepth(0),
		_dx(0),
		_dy(0),
		_offset(0),
		_playingState(PLAYING),
		_hasAudio(false),
		_videoDelay(0)
{}

/**
 * dTor.
 */
FlcPlayer::~FlcPlayer()
{
	deInit();
}

/**
 * Initializes data structures needed buy the player and read the whole file into memory.
 * @param file			- video filename
 * @param frameCallback	- function to call each video frame
 * @param game			- pointer to the Game instance
 * @param dx			- an offset on the x-axis for the video to be rendered
 * @param dy			- an offset on the y-axis for the video to be rendered
 */
bool FlcPlayer::init(
		const char* file,
		void (*frameCallBack)(),
		Game* game,
		int dx,
		int dy)
{
	if (_fileBuf != nullptr)
	{
		Log(LOG_ERROR) << "FlcPlayer::init() Tried to init a video player that was already initialized.";
		return false;
	}

	_frameCallBack = frameCallBack;
	_realScreen = game->getScreen();
	_realScreen->clear();
	_game = game;
	_dx = dx;
	_dy = dy;

	_fileSize = 0u;
	_frameCount = 0u;
	_hasAudio = false;
	_audioFrameData = nullptr;
	_audioData.loadingBuffer =
	_audioData.playingBuffer = nullptr;

	std::ifstream ifstr;
	ifstr.open(file, std::ifstream::in | std::ifstream::binary | std::ifstream::ate);
	if (ifstr.is_open() == false)
	{
		Log(LOG_ERROR) << "Could not open FLI/FLC file: " << file;
		return false;
	}

	const std::streamoff streamSize (ifstr.tellg());
	ifstr.seekg(0, std::ifstream::beg);

	_fileBuf = new Uint8[static_cast<size_t>(streamSize)]; // TODO: substitute with a cross-platform memory mapped file.
	_fileSize = static_cast<Uint32>(streamSize);
	ifstr.read(
			reinterpret_cast<char*>(_fileBuf),
			static_cast<std::streamsize>(streamSize));
	ifstr.close();

	_audioFrameData = _fileBuf + 128;

	readFileHeader(); // read the first 128 bytes

	switch (_headerType) // if it's a FLC or FLI file it's gtg.
	{
		case SDL_SwapLE16(FLI_TYPE):
		case SDL_SwapLE16(FLC_TYPE):
			_screenWidth = _headerWidth;
			_screenHeight = _headerHeight;
			_screenDepth = 8;
			Log(LOG_DEBUG) << "Playing Flix: " << _screenWidth << "x" << _screenHeight << " w/ " << _headerFrames << " frames";
			break;

		default:
			Log(LOG_ERROR) << "Flix file failed header check. oops";
			return false;
	}

	if (_realScreen->getSurface()->getSurface()->format->BitsPerPixel == 8u)	// if the current surface used is at 8-bpp use it
		_mainScreen = _realScreen->getSurface()->getSurface();
	else																		// otherwise create a new one
		_mainScreen = SDL_AllocSurface(
									SDL_SWSURFACE,
									_realScreen->getSurface()->getWidth(),
									_realScreen->getSurface()->getHeight(),
									8,0u,0u,0u,0u);
	return true;
}

/**
 *
 */
void FlcPlayer::deInit()
{
	if (_mainScreen != nullptr
		&& _realScreen != nullptr)
	{
		if (_mainScreen != _realScreen->getSurface()->getSurface())
			SDL_FreeSurface(_mainScreen);

		_mainScreen = nullptr;
	}

	delete[] _fileBuf;
	_fileBuf = nullptr;

	deInitAudio();
}

/**
 * Starts decoding and playing the FLI/FLC file.
 */
void FlcPlayer::play(bool skipLastFrame)
{
	_playingState = PLAYING;

	_dy = (_mainScreen->h - _headerHeight) / 2; // vertically center the video
	_offset = (_dy * _mainScreen->pitch) + (_dx * _mainScreen->format->BytesPerPixel);

	_videoFrameData = _fileBuf + 128; // skip file header
	_audioFrameData = _videoFrameData;

	while (shouldQuit() == false)
	{
		if (_frameCallBack != nullptr)
			(*_frameCallBack)();
		else
			decodeAudio(2); // TODO: support both in the case where the callback is not audio.

		if (shouldQuit() == false)
			decodeVideo(skipLastFrame);

		if (shouldQuit() == false)
			SDLPolling();
	}
}

/**
 *
 */
void FlcPlayer::delay(Uint32 millisec)
{
	const Uint32 pauseStart (SDL_GetTicks());

	while (_playingState != SKIPPED
		&& SDL_GetTicks() < (pauseStart + millisec))
	{
		SDLPolling();
	}
}

/**
 *
 */
void FlcPlayer::SDLPolling()
{
	SDL_Event event;
	while (SDL_PollEvent(&event))
	{
		switch (event.type)
		{
//			case SDL_MOUSEBUTTONDOWN:
			case SDL_KEYDOWN:
				if (event.key.keysym.sym == SDLK_ESCAPE) // cf. IntroState::endVideo()
					_playingState = SKIPPED;
				break;

			case SDL_VIDEORESIZE:
				if (Options::allowResize == true)
				{
// G++ linker wants it this way ...
//#ifdef _DEBUG
					const int
						screenWidth  (Screen::ORIGINAL_WIDTH),
						screenHeight (Screen::ORIGINAL_HEIGHT);

					Options::safeDisplayWidth =
					Options::displayWidth    = std::max(screenWidth,
														event.resize.w);
					Options::safeDisplayHeight =
					Options::displayHeight    = std::max(screenHeight,
														 event.resize.h);
//#else
//					Options::safeDisplayWidth =
//					Options::displayWidth    = std::max(Screen::ORIGINAL_WIDTH,
//														event.resize.w);
//					Options::safeDisplayHeight =
//					Options::displayHeight    = std::max(Screen::ORIGINAL_HEIGHT,
//														 event.resize.h);
//#endif

					if (_mainScreen != _realScreen->getSurface()->getSurface())
						_realScreen->resetDisplay();
					else
					{
						_realScreen->resetDisplay();
						_mainScreen = _realScreen->getSurface()->getSurface();
					}
				}
				break;

			case SDL_QUIT:
				exit(0);
		}
	}
}

/**
 *
 */
bool FlcPlayer::shouldQuit()
{
	switch (_playingState)
	{
		case FINISHED:
		case SKIPPED:
			return true;
	}
	return false;
}

/**
 *
 */
void FlcPlayer::readFileHeader()
{
	readU32(_headerSize,	_fileBuf);
	readU16(_headerType,	_fileBuf +  4);
	readU16(_headerFrames,	_fileBuf +  6);
	readU16(_headerWidth,	_fileBuf +  8);
	readU16(_headerHeight,	_fileBuf + 10);
	readU16(_headerDepth,	_fileBuf + 12);
	readU16(_headerSpeed,	_fileBuf + 16);
}

/**
 *
 */
bool FlcPlayer::isValidFrame(
		Uint8* frameHeader,
		Uint32& frameSize,
		Uint16& frameType)
{
	readU32(frameSize, frameHeader);
	readU16(frameType, frameHeader + 4);

	switch (frameType)
	{
		case FRAME_TYPE:
		case AUDIO_CHUNK:
		case PREFIX_CHUNK:
			return true;
	}
	return false;
}

/**
 *
 */
void FlcPlayer::decodeAudio(int frames)
{
	int audioFramesFound (0);

	while (audioFramesFound < frames
		&& isEndOfFile(_audioFrameData) == false)
	{
		if (isValidFrame(
					_audioFrameData,
					_audioFrameSize,
					_audioFrameType) == false)
		{
			_playingState = FINISHED;
			break;
		}

		switch (_audioFrameType)
		{
			case FRAME_TYPE:
			case PREFIX_CHUNK:
				_audioFrameData += _audioFrameSize;
				break;

			case AUDIO_CHUNK:
				Uint16 sampleRate;
				readU16(sampleRate, _audioFrameData + 8);
				_chunkData = _audioFrameData + 16;
				playAudioFrame(sampleRate);
				_audioFrameData += _audioFrameSize + 16;
				++audioFramesFound;
		}
	}
}

/**
 *
 */
void FlcPlayer::decodeVideo(bool skipLastFrame)
{
	bool videoFrameFound (false);
	while (videoFrameFound == false)
	{
		if (isValidFrame(
					_videoFrameData,
					_videoFrameSize,
					_videoFrameType) == false)
		{
			_playingState = FINISHED;
			break;
		}

		switch (_videoFrameType)
		{
			case FRAME_TYPE:
				readU16(_frameChunks, _videoFrameData + 6);
				readU16(_delayOverride, _videoFrameData + 8);

				Uint32 delay;
				if (_headerType == FLI_TYPE)
					delay = _delayOverride > 0u ? static_cast<Uint32>(_delayOverride)
												: static_cast<Uint32>(static_cast<double>(_headerSpeed) * (1000. / 70.));
				else
					delay = static_cast<Uint32>(_videoDelay);

				waitForNextFrame(delay);

				_chunkData = _videoFrameData + 16; // skip the frame header, not interested in the rest

				_videoFrameData += _videoFrameSize;

				if (isEndOfFile(_videoFrameData) == true) // if this frame is the last one don't play it
					_playingState = FINISHED;

				if (shouldQuit() == false || skipLastFrame == false)
					playVideoFrame();

				videoFrameFound = true;
				break;

			case AUDIO_CHUNK:
				_videoFrameData += _videoFrameSize + 16u;
				break;

			case PREFIX_CHUNK:
				_videoFrameData += _videoFrameSize; // just skip it
		}
	}
}

/**
 *
 */
void FlcPlayer::playVideoFrame()
{
	++_frameCount;
	if (SDL_LockSurface(_mainScreen) < 0)
		return;

	for (Uint16
			i = 0u;
			i != _frameChunks;
			++i)
	{
		readU32(_chunkSize, _chunkData);
		readU16(_chunkType, _chunkData + 4);

		switch (_chunkType)
		{
			case COLOR_256:	color256(); break;
			case FLI_SS2:	fliSS2();	break;
			case COLOR_64:	color64();	break;
			case FLI_LC:	fliLC();	break;
			case BLACK:		black();	break;
			case FLI_BRUN:	fliBRun();	break;
			case FLI_COPY:	fliCopy();	break;
			case 18u:					break;

			default:
				Log(LOG_WARNING) << "Teek! a non implemented chunk type: " << _chunkType;
		}
		_chunkData += _chunkSize;
	}

	SDL_UnlockSurface(_mainScreen);

	if (_mainScreen != _realScreen->getSurface()->getSurface()) // TODO: Track which rectangles have really changed.
		SDL_BlitSurface(
					_mainScreen,
					nullptr,
					_realScreen->getSurface()->getSurface(),
					nullptr);
	_realScreen->flip();
}

/**
 * TFTD audio header (10 bytes)
 * Uint16 unknown1 - always 0
 * Uint16 sampleRate
 * Uint16 unknown2 - always 1 (Channels? bytes per sample?)
 * Uint16 unknown3 - always 10 (No idea)
 * Uint16 unknown4 - always 0
 * Uint8[] unsigned 1-byte 1-channel PCM data of length _chunkSize_ (so the total chunk is _chunkSize_ + 6-byte flc header + 10 byte audio header
 */
void FlcPlayer::playAudioFrame(Uint16 sampleRate)
{
	if (_hasAudio == false)
	{
		_audioData.sampleRate = sampleRate;
		_hasAudio = true;
		initAudio(AUDIO_S16SYS, 1u);
	}
	else
	{
		// Cannot change sample rate mid-video
		assert(sampleRate == _audioData.sampleRate);
	}

	SDL_SemWait(_audioData.sharedLock);
	AudioBuffer* const loadingBuff = _audioData.loadingBuffer;
	assert(loadingBuff->currSamplePos == 0);

	const Uint32 newSize (_audioFrameSize + loadingBuff->sampleCount + 2u);
	if (newSize > loadingBuff->sampleBufSize)
	{
		// If the sample count has changed, reallocation is needed (Handles initial
		// state of '0' sample count too since realloc(nullptr, size) == malloc(size)
		loadingBuff->samples = static_cast<Sint16*>(realloc(loadingBuff->samples, newSize));
		loadingBuff->sampleBufSize = newSize;
	}

	const float vol (static_cast<float>(Game::volExp(Options::volMusic)));
	for (Uint32
		i = 0u;
		i != _audioFrameSize;
		++i)
	{
		loadingBuff->samples[loadingBuff->sampleCount + i] = static_cast<Sint16>(static_cast<float>(_chunkData[i] - 128u) * vol * 240.f);
	}

	loadingBuff->sampleCount += _audioFrameSize;	// i wish someone would learn to make sensible casts instead of merely riding this hobby-horse.
	SDL_SemPost(_audioData.sharedLock);				// okay I learned.
}

/**
 *
 */
void FlcPlayer::color256()
{
	Uint8
		qtyColorsSkip,
		* pSrc;
	Uint16
		qtyColorPackets,
		qtyColors (0u);

	pSrc = _chunkData + 6;
	readU16(qtyColorPackets, pSrc);
	pSrc += 2;

	while (qtyColorPackets-- != 0u)
	{
		qtyColorsSkip = static_cast<Uint8>(*(pSrc++) + qtyColors);
		if ((qtyColors = *(pSrc++)) == 0u) qtyColors = 256u;
		for (Uint16
				i = 0u;
				i < qtyColors;
				++i)
		{
			_colors[i].r = *(pSrc++);
			_colors[i].g = *(pSrc++);
			_colors[i].b = *(pSrc++);
		}

		if (_mainScreen != _realScreen->getSurface()->getSurface())
			SDL_SetColors(
						_mainScreen,
						_colors,
						qtyColorsSkip,
						qtyColors);

		_realScreen->setPalette(
							_colors,
							qtyColorsSkip,
							qtyColors,
							true);

		if (qtyColorPackets > 0u)
			++qtyColors;
	}
}

/**
 *
 */
void FlcPlayer::fliSS2()
{
	bool setLastByte (false);
	Sint8 countData;
	Sint16 counter;
	Uint8
		columSkip,
		fill1,
		fill2,
		lastByte (0u),
		* pSrc,
		* pDst,
		* pTmpDst;
	Uint16 lines;

	pSrc = _chunkData + 6;
	pDst = static_cast<Uint8*>(_mainScreen->pixels) + _offset;
	readU16(lines, pSrc);

	pSrc += 2;

	while (lines-- != 0)
	{
		readS16(counter, reinterpret_cast<Sint8*>(pSrc));
		pSrc += 2;

		if ((counter & MASK) == SKIP_LINES)
		{
			pDst += (-counter) * _mainScreen->pitch;
			++lines;
			continue;
		}

		if ((counter & MASK) == LAST_PIXEL)
		{
			setLastByte = true;
			lastByte = static_cast<Uint8>(counter & 0x00FF);
			readS16(counter, reinterpret_cast<Sint8*>(pSrc));
			pSrc += 2;
		}

		if ((counter & MASK) == PACKETS_COUNT)
		{
			pTmpDst = pDst;
			while (counter-- != 0)
			{
				columSkip = *(pSrc++);
				pTmpDst += columSkip;

				if ((countData = static_cast<Sint8>(*(pSrc++))) > 0)
				{
					std::copy(
							pSrc,
							pSrc + (2 * countData),
							pTmpDst);
					pTmpDst += (2 * countData);
					pSrc += (2 * countData);
				}
				else if (countData < 0)
				{
					countData = static_cast<Sint8>(-countData);

					fill1 = *(pSrc++);
					fill2 = *(pSrc++);
					while (countData-- != 0)
					{
						*(pTmpDst++) = fill1;
						*(pTmpDst++) = fill2;
					}
				}
			}

			if (setLastByte == true)
			{
				setLastByte = false;
				*(pDst + _mainScreen->pitch - 1) = lastByte;
			}
			pDst += _mainScreen->pitch;
		}
	}
}

/**
 *
 */
void FlcPlayer::fliBRun()
{
	int heightCount;
	Uint8
		* pSrc,
		* pDst,
		* pTmpDst,
		filler;
	Sint8 countData;

	heightCount = _headerHeight;
	pSrc = _chunkData + 6; // skip chunk header
	pDst = static_cast<Uint8*>(_mainScreen->pixels) + _offset;

	while (heightCount-- != 0)
	{
		pTmpDst = pDst;
		++pSrc; // read and skip the packet count value

		int pixels (0);
		while (pixels != _headerWidth)
		{
			if ((countData = static_cast<Sint8>(*(pSrc++))) > 0)
			{
				filler = *(pSrc++);

				std::fill_n(
						pTmpDst,
						countData,
						filler);
				pTmpDst += countData;
				pixels += countData;
			}
			else if (countData < 0)
			{
				countData = static_cast<Sint8>(-countData);

				std::copy(
						pSrc,
						pSrc + countData,
						pTmpDst);
				pTmpDst += countData;
				pSrc += countData;
				pixels += countData;
			}
		}
		pDst += _mainScreen->pitch;
	}
}

/**
 *
 */
void FlcPlayer::fliLC()
{
	int packetsCount;
	Sint8 countData;
	Uint8
		* pSrc,
		* pDst,
		* pTmpDst,
		countSkip,
		filler;
	Uint16
		lines,
		temp;

	pSrc = _chunkData + 6;
	pDst = static_cast<Uint8*>(_mainScreen->pixels) + _offset;

	readU16(temp, pSrc);
	pSrc += 2;
	pDst += temp * _mainScreen->pitch;
	readU16(lines, pSrc);
	pSrc += 2;

	while (lines-- != 0)
	{
		pTmpDst = pDst;
		packetsCount = *(pSrc++);

		while (packetsCount-- != 0)
		{
			countSkip = *(pSrc++);
			pTmpDst += countSkip;
			countData = static_cast<Sint8>(*(pSrc++));
			if (countData > 0)
			{
				while (countData-- != 0)
					*(pTmpDst++) = *(pSrc++);
			}
			else if (countData < 0)
			{
				countData = static_cast<Sint8>(-countData);

				filler = *(pSrc++);
				while (countData-- != 0)
					*(pTmpDst++) = filler;
			}
		}
		pDst += _mainScreen->pitch;
	}
}

/**
 *
 */
void FlcPlayer::color64()
{
	Uint8
		qtyColorsSkip,
		* pSrc;
	Uint16
		qtyColors,
		qtyColorPackets;

	pSrc = _chunkData + 6;
	readU16(qtyColorPackets, pSrc);
	pSrc += 2;

	while (qtyColorPackets--)
	{
		qtyColorsSkip = *(pSrc++);
		if ((qtyColors = *(pSrc++)) == 0u) qtyColors = 256u;
		for (Uint16
				i = 0u;
				i != qtyColors;
				++i)
		{
			_colors[i].r = static_cast<Uint8>(*(pSrc++) << 2u);
			_colors[i].g = static_cast<Uint8>(*(pSrc++) << 2u);
			_colors[i].b = static_cast<Uint8>(*(pSrc++) << 2u);
		}

		if (_mainScreen != _realScreen->getSurface()->getSurface())
			SDL_SetColors(
						_mainScreen,
						_colors,
						qtyColorsSkip,
						qtyColors);

		_realScreen->setPalette(
							_colors,
							qtyColorsSkip,
							qtyColors,
							true);
	}
}

/**
 *
 */
void FlcPlayer::fliCopy()
{
	Uint8
		* pSrc,
		* pDst;
	int lines (_screenHeight);
	pSrc = _chunkData + 6;
	pDst = static_cast<Uint8*>(_mainScreen->pixels) + _offset;

	while (lines-- != 0)
	{
		std::memcpy(
				pDst,
				pSrc,
				static_cast<size_t>(_screenWidth));
		pSrc += _screenWidth;
		pDst += _mainScreen->pitch;
	}
}

/**
 *
 */
void FlcPlayer::black()
{
	Uint8* pDst;
	int lines (_screenHeight);
	pDst = static_cast<Uint8*>(_mainScreen->pixels) + _offset;

	while (lines-- != 0)
	{
		std::memset(
				pDst,
				0,
				static_cast<size_t>(_screenHeight));
		pDst += _mainScreen->pitch;
	}
}

/**
 *
 */
void FlcPlayer::audioCallback(
		void* userData,
		Uint8* stream,
		int len)
{
	AudioData* audio (static_cast<AudioData*>(userData));
	AudioBuffer* playBuff (audio->playingBuffer);

	while (len > 0)
	{
		if (playBuff->sampleCount > 0u)
		{
			const int bytesToCopy (std::min(len,
											static_cast<int>(playBuff->sampleCount) * 2));
			std::memcpy(
					stream,
					playBuff->samples + playBuff->currSamplePos,
					static_cast<size_t>(bytesToCopy));

			playBuff->currSamplePos += bytesToCopy / 2;
			playBuff->sampleCount = playBuff->sampleCount - static_cast<Uint32>(bytesToCopy / 2);
			len -= bytesToCopy;

			assert(playBuff->sampleCount >= 0u);
		}

		if (len > 0) // need to swap buffers
		{
			playBuff->currSamplePos = 0;
			SDL_SemWait(audio->sharedLock);
			AudioBuffer* const tempBuff = playBuff;
			audio->playingBuffer = playBuff = audio->loadingBuffer;
			audio->loadingBuffer = tempBuff;
			SDL_SemPost(audio->sharedLock);

			if (playBuff->sampleCount == 0u)
				break;
		}
	}
}

/**
 *
 */
void FlcPlayer::initAudio(
		Uint16 audioFormat,
		Uint8 channels)
{
	const int err (Mix_OpenAudio(
							_audioData.sampleRate,
							audioFormat,
							channels,
							static_cast<int>(_audioFrameSize * 2u)));
	_videoDelay = 1000 / (_audioData.sampleRate / static_cast<int>(_audioFrameSize));

	if (err != 0)
	{
		Log(LOG_WARNING) << Mix_GetError();
		Log(LOG_WARNING) << "Failed to init cutscene audio.";
		return;
	}

	// Start runnable
	_audioData.sharedLock = SDL_CreateSemaphore(1u);

	_audioData.loadingBuffer = new AudioBuffer();
	_audioData.loadingBuffer->currSamplePos = 0;
	_audioData.loadingBuffer->sampleCount = 0u;
	_audioData.loadingBuffer->samples = static_cast<Sint16*>(malloc(_audioFrameSize * 2u));
	_audioData.loadingBuffer->sampleBufSize = _audioFrameSize * 2u;

	_audioData.playingBuffer = new AudioBuffer();
	_audioData.playingBuffer->currSamplePos = 0;
	_audioData.playingBuffer->sampleCount = 0u;
	_audioData.playingBuffer->samples = static_cast<Sint16*>(malloc(_audioFrameSize * 2u));
	_audioData.playingBuffer->sampleBufSize = _audioFrameSize * 2u;

	Mix_HookMusic(
				FlcPlayer::audioCallback,
				&_audioData);
}

/**
 *
 */
void FlcPlayer::deInitAudio()
{
	if (_game != nullptr)
	{
		Mix_HookMusic(nullptr, nullptr);
		Mix_CloseAudio();
		_game->initAudio();
	}
	else if (_audioData.sharedLock != nullptr)
		SDL_DestroySemaphore(_audioData.sharedLock);

	if (_audioData.loadingBuffer != nullptr)
	{
		free(_audioData.loadingBuffer->samples);
		delete _audioData.loadingBuffer;
		_audioData.loadingBuffer = nullptr;
	}

	if (_audioData.playingBuffer != nullptr)
	{
		free(_audioData.playingBuffer->samples);
		delete _audioData.playingBuffer;
		_audioData.playingBuffer = nullptr;
	}
}

/**
 *
 */
void FlcPlayer::stop()
{
	_playingState = FINISHED;
}

/**
 *
 */
bool FlcPlayer::isEndOfFile(Uint8* pos) const
{
	return (pos - _fileBuf) == static_cast<int>(_fileSize); // should be Sint64 lol
}

/**
 *
 */
int FlcPlayer::getFrameCount() const
{
	return _frameCount;
}

/**
 *
 */
void FlcPlayer::setHeaderSpeed(int speed)
{
	_headerSpeed = static_cast<Uint16>(speed);
}

/**
 *
 */
bool FlcPlayer::wasSkipped() const
{
	return (_playingState == SKIPPED);
}

/**
 *
 */
void FlcPlayer::waitForNextFrame(Uint32 delay)
{
	static Uint32 oldTick (0u);
	Uint32
		newTick,
		currentTick;

	currentTick = SDL_GetTicks();
	if (oldTick == 0u)
	{
		oldTick =
		newTick = currentTick;
	}
	else
		newTick = oldTick + delay;

	if (_hasAudio == true)
	{
		while (currentTick < newTick)
		{
			while (
					(newTick - currentTick) > 10u
					&& isEndOfFile(_audioFrameData) == false)
			{
				decodeAudio(1);
				currentTick = SDL_GetTicks();
			}
			SDL_Delay(1u);
			currentTick = SDL_GetTicks();
		}
	}
	else
	{
		while (currentTick < newTick)
		{
			SDL_Delay(1u);
			currentTick = SDL_GetTicks();
		}
	}
	oldTick = SDL_GetTicks();
}

/**
 * inline stuff
 */
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
inline void FlcPlayer::readU16(Uint16& dst, const Uint8* const src)
{
	dst = (src[0u] << 8u) | src[1u];
}
inline void FlcPlayer::readU32(Uint32& dst, const Uint8* const src)
{
	dst = (src[0u] << 24u) | (src[1u] << 16u) | (src[2u] << 8u) | src[3u];
}
inline void FlcPlayer::readS16(Sint16& dst, const Sint8* const src)
{
	dst = (src[0u] << 8u) | src[1u];
}
inline void FlcPlayer::readS32(Sint32& dst, const Sint8* const src)
{
	dst = (src[0u] << 24u) | (src[1u] << 16u) | (src[2u] << 8u) | src[3u];
}
#else
inline void FlcPlayer::readU16(Uint16& dst, const Uint8* const src)
{
	dst = static_cast<Uint16>((src[1u] << 8u) | src[0u]);
}
inline void FlcPlayer::readU32(Uint32& dst, const Uint8* const src)
{
	dst = static_cast<Uint32>((src[3u] << 24u) | (src[2u] << 16u) | (src[1u] << 8u) | src[0u]);
}
inline void FlcPlayer::readS16(Sint16& dst, const Sint8* const src)
{
	dst = static_cast<Sint16>((src[1u] << 8u) | src[0u]);
}
inline void FlcPlayer::readS32(Sint32& dst, const Sint8* const src)
{
	dst = (src[3u] << 24u) | (src[2u] << 16u) | (src[1u] << 8u) | src[0u];
}
#endif

}
