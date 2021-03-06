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

#ifndef OPENXCOM_CROSSPLATFORM_H
#define OPENXCOM_CROSSPLATFORM_H

#include <string> // std::string, std::wstring
#include <vector> // std::vector

#include <SDL/SDL_events.h>


namespace OpenXcom
{

/**
 * Generic-purpose functions that need different implementations for different
 * platforms.
 */
namespace CrossPlatform
{

#ifdef _WIN32
const char PATH_SEPARATOR = '\\';
#else
const char PATH_SEPARATOR = '/';
#endif

/// Displays an error-message.
void showFatalError(const std::string& error);

/// Finds the game's data-folders in the system.
std::vector<std::string> findDataFolders();
/// Finds the game's user-folders in the system.
std::vector<std::string> findUserFolders();
/// Finds the game's config-folder in the system.
std::string findConfigFolder();

/// Tries to find a file.
std::string caseInsensitive(
		const std::string& base,
		const std::string& path);
/// Tries to find a folder.
std::string caseInsensitiveFolder(
		const std::string& base,
		const std::string& path);

/// Gets the path for a data-file.
std::string getDataFile(const std::string& file);
/// Gets the path for a data-folder
std::string getDataFolder(const std::string& folder);

/// Creates a folder.
bool createFolder(const std::string& path);

/// Terminates a path.
std::string endPath(const std::string& path);

/// Gets the list of files in a folder.
std::vector<std::string> getFolderContents(
		const std::string& path,
		const std::string& ext = "");
/// Gets the list of files in a data-folder.
std::vector<std::string> getDataContents(
		const std::string& path,
		const std::string& ext = "");

/// Checks if the path is an existing folder.
bool folderExists(const std::string& path);
/// Checks if the path is an existing file.
bool fileExists(const std::string& path);

/// Deletes the specified file.
bool deleteFile(const std::string& path);

/// Gets the basename of a file.
std::string baseFilename(
		const std::string& path,
		int(*transform)(int) = nullptr);

/// Sanitizes the characters in a filename.
std::string sanitizeFilename(const std::string& file);

/// Removes the extension from a file.
std::string noExt(const std::string& file);

/// Gets the system-locale.
std::string getLocale();

/// Checks if an event is a quit-shortcut.
bool isQuitShortcut(const SDL_Event& ev);

/// Gets the modified date of a file.
time_t getDateModified(const std::string& path);

/// Converts a timestamp to a pair of widestrings.
std::pair<std::wstring, std::wstring> timeToString(time_t timeIn);
/// Converts the time into a string.
std::string timeString();
/// Gets the current/local time as a string.
std::string now();

/// Compares two strings by natural order.
bool naturalCompare(
		const std::wstring& a,
		const std::wstring& b);

/// Moves/renames a file between paths.
bool moveFile(
		const std::string& src,
		const std::string& dest);

/// Flashes the game window.
void flashWindow();

/// Gets the DOS-style executable path.
std::string getDosPath();

/// Sets the window-icon for _WIN32 build-configuration.
#ifdef _WIN32
void setWindowIcon(int winResource);
#else
/// Sets the window-icon if not _WIN32 build.
void setWindowIcon(const std::string& unixPath);
#endif

} // CrossPlatform

}

#endif
