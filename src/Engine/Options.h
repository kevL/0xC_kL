/*
 * Copyright 2010-2019 OpenXcom Developers.
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

#ifndef OPENXCOM_OPTIONS_H
#define OPENXCOM_OPTIONS_H

#include <string>	// std::string
//#include <vector>	// std::vector

#include "OptionInfo.h"


namespace OpenXcom
{

/**
 * Battlescape drag-scrolling modes.
 */
enum ScrollType
{
	MAP_SCROLL_NONE,	// 0
	MAP_SCROLL_TRIGGER,	// 1
	MAP_SCROLL_AUTO		// 2
};

/**
 * Keyboard-input modes.
 */
enum KeyboardType
{
	KEYBOARD_OFF,		// 0
	KEYBOARD_ON,		// 1
	KEYBOARD_VIRTUAL	// 2
};

/**
 * Savegame-sorting modes.
 */
enum SaveSort
{
	SORT_NAME_ASC,	// 0
	SORT_NAME_DESC,	// 1
	SORT_DATE_ASC,	// 2
	SORT_DATE_DESC	// 3
};

/**
 * Music-format preferences.
 */
enum MusicFormat
{
	MUSIC_AUTO,		// 0
	MUSIC_FLAC,		// 1
	MUSIC_OGG,		// 2
	MUSIC_MP3,		// 3
	MUSIC_MOD,		// 4
	MUSIC_WAV,		// 5
	MUSIC_ADLIB,	// 6
	MUSIC_MIDI		// 7
};

/**
 * Sound-format preferences.
 */
enum SoundFormat
{
	SOUND_AUTO,	// 0
	SOUND_14,	// 1
	SOUND_10	// 2
};

/**
 * Path-preview Modes for the battlefield.
 * @note Can be OR'd together.
 */
enum PathPreview
{
	PATH_NONE		= 0x00,	// 0000
	PATH_ARROWS		= 0x01,	// 0001
	PATH_TU_COST	= 0x02,	// 0010
	PATH_FULL		= 0x03	// 0011 arrows + tuCost
};

/**
 * Screen-scaling modes.
 */
enum ScaleType
{
	SCALE_ORIGINAL,		// 0
	SCALE_15X,			// 1
	SCALE_2X,			// 2
	SCALE_SCREEN_DIV_3,	// 3
	SCALE_SCREEN_DIV_2,	// 4
	SCALE_SCREEN		// 5
};


/**
 * Container for all the various global game-options and customizable settings.
 */
namespace Options
{

#define OPT extern
#include "Options.inc.h"
#undef OPT

/// Initializes the option-settings.
bool init(
		int argc,
		char* argv[]);

/// Shows help from the command-line.
bool showHelp(
		int argc,
		char* argv[]);

/// Loads arguments from the command-line.
void loadArgs(
		int argc,
		char* argv[]);

/// Sets the data-, user-, config-, and pic-folders.
void setFolders();

/// Updates Options from config-file and command-line.
void userOptions();

/// Saves options to YAML.
void save(const std::string& file = "options");
/// Loads options from YAML.
void load(const std::string& file = "options");

/// Gets a list of possible data-folders.
const std::vector<std::string>& getDataFolders();
/// Sets the data-folder from which resources are loaded.
void setDataFolder(const std::string& folder);
/// Gets the data-folder from which resources are loaded.
std::string getDataFolder();
/// Gets the user-folder where saves are stored.
std::string getUserFolder();
/// Gets the picture-folder where screenshots are stored.
std::string getPictureFolder();
/// Gets the config-folder where Options are stored.
std::string getConfigFolder();

/// Gets the list of all available OptionInfo.
const std::vector<OptionInfo>& getOptionInfo();

/// Backups display-options.
void backupDisplay();
/// Switches display-options.
void switchDisplay();

/// Resets the Options back to their defaults.
void resetDefaults();

/// Creates all OptionInfo.
void createOptions();

} // Options

}

#endif
