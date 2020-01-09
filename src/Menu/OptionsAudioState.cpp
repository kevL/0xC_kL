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

#include "OptionsAudioState.h"

//#include <sstream>
//#include <SDL/SDL_mixer.h>

#include "../Engine/Action.h"
#include "../Engine/Game.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Options.h"
#include "../Engine/Sound.h"

#include "../Interface/ComboBox.h"
#include "../Interface/Slider.h"
#include "../Interface/Text.h"
#include "../Interface/TextButton.h"

#include "../Resource/ResourcePack.h"

//#include "../Ruleset/Ruleset.h"


namespace OpenXcom
{

/* MUS_NONE, MUS_CMD, MUS_WAV, MUS_MOD, MUS_MID, MUS_OGG, MUS_MP3, MUS_MP3_MAD, MUS_FLAC, MUS_MODPLUG */
const std::wstring OptionsAudioState::musFormats[] =
{
	L"Adlib",
	L"?",
	L"WAV",
	L"MOD",
	L"MIDI",
	L"OGG",
	L"MP3",
	L"MP3",
	L"FLAC",
	L"MOD"
};

const std::wstring OptionsAudioState::sndFormats[] =
{
	L"?",
	L"1.4",
	L"1.0"
};


/**
 * Initializes all the elements in the Audio Options screen.
 * @param origin - game section that originated this state
 */
OptionsAudioState::OptionsAudioState(OptionsOrigin origin)
	:
		OptionsBaseState(origin)
{
	setCategory(_btnAudio);

	_txtMusicVolume		= new Text(114, 9, 94, 8);
	_slrMusicVolume		= new Slider(104, 16, 94, 18);

	_txtSoundVolume		= new Text(114, 9, 206, 8);
	_slrSoundVolume		= new Slider(104, 16, 206, 18);

	_txtUiVolume		= new Text(114, 9, 94, 40);
	_slrUiVolume		= new Slider(104, 16, 94, 50);

	_txtSampleRate		= new Text(114, 9, 206, 40);
	_cbxSampleRate		= new ComboBox(this, 104, 16, 206, 50);

	_txtMusicFormat		= new Text(114, 9, 94, 72);
	_cbxMusicFormat		= new ComboBox(this, 104, 16, 94, 82);
	_txtCurrentMusic	= new Text(114, 9, 94, 100);

	_txtSoundFormat		= new Text(114, 9, 206, 72);
	_cbxSoundFormat		= new ComboBox(this, 104, 16, 206, 82);
	_txtCurrentSound	= new Text(114, 9, 206, 100);

	add(_txtMusicVolume, "text", "audioMenu");
	add(_slrMusicVolume, "button", "audioMenu");

	add(_txtSoundVolume, "text", "audioMenu");
	add(_slrSoundVolume, "button", "audioMenu");

	add(_txtUiVolume, "text", "audioMenu");
	add(_slrUiVolume, "button", "audioMenu");

	add(_txtSampleRate, "text", "audioMenu");
	add(_cbxSampleRate, "button", "audioMenu");

	add(_txtMusicFormat, "text", "audioMenu");
	add(_txtCurrentMusic, "text", "audioMenu");
	add(_txtSoundFormat, "text", "audioMenu");
	add(_txtCurrentSound, "text", "audioMenu");

	add(_cbxMusicFormat, "button", "audioMenu");
	add(_cbxSoundFormat, "button", "audioMenu");

	centerSurfaces();


	_txtMusicVolume->setText(tr("STR_MUSIC_VOLUME"));

	_slrMusicVolume->setRange(0, SDL_MIX_MAXVOLUME);
	_slrMusicVolume->setValue(Options::volMusic);
	_slrMusicVolume->onSliderChange(static_cast<ActionHandler>(&OptionsAudioState::slrMusicVolumeChange));
//	_slrMusicVolume->onMouseIn(		static_cast<ActionHandler>(&OptionsAudioState::txtTooltipIn));
//	_slrMusicVolume->onMouseOut(	static_cast<ActionHandler>(&OptionsAudioState::txtTooltipOut));
//	_slrMusicVolume->setTooltip("STR_MUSIC_VOLUME_DESC");

	_txtSoundVolume->setText(tr("STR_SFX_VOLUME"));

	_slrSoundVolume->setRange(0, SDL_MIX_MAXVOLUME);
	_slrSoundVolume->setValue(Options::volFx);
	_slrSoundVolume->onSliderChange(static_cast<ActionHandler>(&OptionsAudioState::slrSoundVolumeChange));
	_slrSoundVolume->onMouseRelease(static_cast<ActionHandler>(&OptionsAudioState::slrSoundVolumeRelease));
//	_slrSoundVolume->onMouseIn(		static_cast<ActionHandler>(&OptionsAudioState::txtTooltipIn));
//	_slrSoundVolume->onMouseOut(	static_cast<ActionHandler>(&OptionsAudioState::txtTooltipOut));
//	_slrSoundVolume->setTooltip("STR_SFX_VOLUME_DESC");

	_txtUiVolume->setText(tr("STR_UI_VOLUME"));

	_slrUiVolume->setRange(0, SDL_MIX_MAXVOLUME);
	_slrUiVolume->setValue(Options::volUi);
	_slrUiVolume->onSliderChange(	static_cast<ActionHandler>(&OptionsAudioState::slrUiVolumeChange));
	_slrUiVolume->onMouseRelease(	static_cast<ActionHandler>(&OptionsAudioState::slrUiVolumeRelease));
//	_slrUiVolume->onMouseIn(		static_cast<ActionHandler>(&OptionsAudioState::txtTooltipIn));
//	_slrUiVolume->onMouseOut(		static_cast<ActionHandler>(&OptionsAudioState::txtTooltipOut));
//	_slrUiVolume->setTooltip("STR_UI_VOLUME_DESC");

	std::wostringstream ss;
	std::vector<std::wstring> samplesText;

	int samples[]
	{
		8000,
		11025,
		16000,
		22050,
		32000,
		44100,
		48000
	};

	for (size_t
			i = 0u;
			i < (sizeof(samples) / sizeof(samples[0u]));
			++i)
	{
		_sampleRates.push_back(samples[i]);
		ss << samples[i] << L" Hz";
		samplesText.push_back(ss.str());
		ss.str(L"");

		if (Options::audioSampleRate == samples[i])
			_cbxSampleRate->setSelected(i);
	}

	_txtSampleRate->setText(tr("STR_AUDIO_SAMPLE_RATE"));

	_cbxSampleRate->setOptions(samplesText);
	_cbxSampleRate->onComboChange(static_cast<ActionHandler>(&OptionsAudioState::cbxSampleRateChange));
//	_cbxSampleRate->setTooltip("STR_AUDIO_SAMPLE_RATE_DESC");
//	_cbxSampleRate->onMouseIn(static_cast<ActionHandler>(&OptionsAudioState::txtTooltipIn));
//	_cbxSampleRate->onMouseOut(static_cast<ActionHandler>(&OptionsAudioState::txtTooltipOut));

	std::vector<std::wstring>
		musicText,
		soundText;

	/* MUSIC_AUTO, MUSIC_FLAC, MUSIC_OGG, MUSIC_MP3, MUSIC_MOD, MUSIC_WAV, MUSIC_ADLIB, MUSIC_MIDI */
	musicText.push_back(tr("STR_PREFERRED_FORMAT_AUTO"));
	musicText.push_back(L"FLAC");
	musicText.push_back(L"OGG");
	musicText.push_back(L"MP3");
	musicText.push_back(L"MOD");
	musicText.push_back(L"WAV");
	musicText.push_back(L"Adlib");
	musicText.push_back(L"MIDI");

	soundText.push_back(tr("STR_PREFERRED_FORMAT_AUTO"));
	soundText.push_back(L"1.4");
	soundText.push_back(L"1.0");

	_txtMusicFormat->setText(tr("STR_PREFERRED_MUSIC_FORMAT"));

	_cbxMusicFormat->setOptions(musicText);
	_cbxMusicFormat->setSelected(Options::preferredMusic);
	_cbxMusicFormat->onComboChange(static_cast<ActionHandler>(&OptionsAudioState::cbxMusicFormatChange));
//	_cbxMusicFormat->onMouseIn(static_cast<ActionHandler>(&OptionsAudioState::txtTooltipIn));
//	_cbxMusicFormat->onMouseOut(static_cast<ActionHandler>(&OptionsAudioState::txtTooltipOut));
//	_cbxMusicFormat->setTooltip("STR_PREFERRED_MUSIC_FORMAT_DESC");

	std::wstring curMusic (musFormats[Mix_GetMusicType(nullptr)]);
	_txtCurrentMusic->setText(tr("STR_CURRENT_FORMAT").arg(curMusic));

	_txtSoundFormat->setText(tr("STR_PREFERRED_SFX_FORMAT"));

	_cbxSoundFormat->setOptions(soundText);
	_cbxSoundFormat->setSelected(Options::preferredSound);
	_cbxSoundFormat->onComboChange(static_cast<ActionHandler>(&OptionsAudioState::cbxSoundFormatChange));
//	_cbxSoundFormat->onMouseIn(static_cast<ActionHandler>(&OptionsAudioState::txtTooltipIn));
//	_cbxSoundFormat->onMouseOut(static_cast<ActionHandler>(&OptionsAudioState::txtTooltipOut));
//	_cbxSoundFormat->setTooltip("STR_PREFERRED_SFX_FORMAT_DESC");

	std::wstring curSound (sndFormats[Options::currentSound]);
	_txtCurrentSound->setText(tr("STR_CURRENT_FORMAT").arg(curSound));

	// These options require a restart, so don't enable them in-game
	bool vis (_origin == OPT_MAIN_START);
	_txtSampleRate->setVisible(vis);
	_cbxSampleRate->setVisible(vis);
	_txtMusicFormat->setVisible(vis);
	_cbxMusicFormat->setVisible(vis);
	_txtCurrentMusic->setVisible(vis);
	_txtSoundFormat->setVisible(vis);
	_cbxSoundFormat->setVisible(vis);
	_txtCurrentSound->setVisible(vis);
}

/**
 * dTor.
 */
OptionsAudioState::~OptionsAudioState()
{}

/**
 * Updates the music volume.
 * @param action - pointer to an Action
 */
void OptionsAudioState::slrMusicVolumeChange(Action*)
{
	Options::volMusic = _slrMusicVolume->getValue();
	_game->setVolume(
				Options::volMusic,
				Options::volFx,
				Options::volUi);
}

/**
 * Updates the sound volume with the slider.
 * @param action - pointer to an Action
 */
void OptionsAudioState::slrSoundVolumeChange(Action*)
{
	Options::volFx = _slrSoundVolume->getValue();
	_game->setVolume(
				Options::volMusic,
				Options::volFx,
				Options::volUi);
}

/**
 * Plays a game sound for volume preview.
 * @param action - pointer to an Action
 */
void OptionsAudioState::slrSoundVolumeRelease(Action*)
{
	_game->getResourcePack()->getSound("GEO.CAT", ResourcePack::UFO_FIRE)->play();
}

/**
 * Updates the UI volume with the slider.
 * @param action - pointer to an Action
 */
void OptionsAudioState::slrUiVolumeChange(Action*)
{
	Options::volUi = _slrUiVolume->getValue();
	_game->setVolume(
				Options::volMusic,
				Options::volFx,
				Options::volUi);
}

/**
 * Plays a UI sound for volume preview.
 * @param action - pointer to an Action
 */
void OptionsAudioState::slrUiVolumeRelease(Action*)
{
//	_game->getResourcePack()->getSound("GEO.CAT", 0u)->play(Mix_GroupAvailable(0));
	TextButton::soundPress->play(Mix_GroupAvailable(0));
}

/**
 * Changes the Audio Sample Rate option.
 * @param action - pointer to an Action
 */
void OptionsAudioState::cbxSampleRateChange(Action*)
{
	Options::audioSampleRate = _sampleRates[_cbxSampleRate->getSelected()];
	Options::reload = true;
}

/**
 * Changes the Music Format option.
 * @param action - pointer to an Action
 */
void OptionsAudioState::cbxMusicFormatChange(Action*)
{
	Options::preferredMusic = static_cast<MusicFormat>(_cbxMusicFormat->getSelected());
	Options::reload = true;
}

/**
 * Changes the Sound Format option.
 * @param action - pointer to an Action
 */
void OptionsAudioState::cbxSoundFormatChange(Action*)
{
	Options::preferredSound = static_cast<SoundFormat>(_cbxSoundFormat->getSelected());
	Options::reload = true;
}

}
