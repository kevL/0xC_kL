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

#ifdef _MSC_VER
#	ifndef _CRT_SECURE_NO_WARNINGS
#		define _CRT_SECURE_NO_WARNINGS
#	endif
#endif

#include "CrossPlatform.h"

#include <algorithm>	// std::replace(), std::sort(), std::transform()
#include <ctime>		// std::localtime(), std::time(), std::strftime(), std::wcsftime(), std::time_t
//#include <fstream>	// std::ifstream, std::ofstream, [std::ifstream::failbit, std::ifstream::badbit]
//#include <ios>		// std::fstream::failure, std::ios::binary
//#include <iostream>	// std::cerr()
//#include <iterator>	// std::string::iterator
//#include <locale>		// std::locale()
//#include <ostream>	// std::endl
#include <set>			// std::set
#include <sstream>		// std::ostringstream
//#include <stdexcept>	// std::runtime_error
//#include <utility>	// std::pair, std::make_pair()

#include <sys/stat.h>	// stat()

#include <SDL/SDL_image.h>
#include <SDL/SDL_syswm.h>

#include "../dirent.h"

#include "Exception.h"
#include "Language.h"
#include "Logger.h"
#include "Options.h"

#ifdef _WIN32
#	ifndef NOMINMAX
#		define NOMINMAX
#	endif

#	ifndef WIN32_LEAN_AND_MEAN
#		define WIN32_LEAN_AND_MEAN
#	endif

//#	include <windows.h>
#	include <shlobj.h>
#	include <shlwapi.h>
//#	include <dbghelp.h>

#	ifndef SHGFP_TYPE_CURRENT
#		define SHGFP_TYPE_CURRENT 0
#	endif

#	ifndef LOCALE_INVARIANT
#		define LOCALE_INVARIANT 0x007f
#	endif

//#	define EXCEPTION_CODE_CXX 0xe06d7363

#	ifndef __GNUC__
#		pragma comment(lib, "advapi32.lib")
#		pragma comment(lib, "shell32.lib")
#		pragma comment(lib, "shlwapi.lib")
//#		pragma comment(lib, "dbghelp.lib")
#	endif
#else
#	include <cstdio>
#	include <cstdlib>
#	include <cstring>
//#	include <execinfo.h>
//#	include <fstream>
//#	include <iostream>
#	include <pwd.h>
#	include <unistd.h>

#	include <sys/param.h>
#	include <sys/types.h>

//#	include <SDL_image.h>
#endif


namespace OpenXcom
{

namespace CrossPlatform
{

/**
 * Displays a message-box with an error-message.
 * @param error - reference to an error-message
 */
void showFatalError(const std::string& error)
{
#ifdef _WIN32
	MessageBoxA(
			nullptr,
			error.c_str(),
			"OpenXcom Error",
			MB_ICONERROR | MB_OK);
#else
	std::cerr << error << std::endl;
#endif

	Log(LOG_FATAL) << error;
}

/**
 * Gets the user's home-folder according to the operating-system.
 * @return, char-array for absolute path to home-folder
 */
#ifndef _WIN32
static char* const getHome()
{
	char* const home (getenv("HOME"));
	if (home == nullptr)
	{
		struct passwd* const pwd (getpwuid(getuid()));
		home = pwd->pw_dir;
	}
	return home;
}
#endif

/**
 * Builds a list of predefined paths for the data-folders according to the
 * operating-system.
 * @return, list of data-paths
 */
std::vector<std::string> findDataFolders()
{
	std::vector<std::string> pathList;

#ifdef __MORPHOS__
	pathList.push_back("PROGDIR:data/");
	return pathList;
#endif

#ifdef _WIN32
	char path[MAX_PATH];

//	if (SUCCEEDED(SHGetFolderPathA( // get documents-folder
//								nullptr,
//								CSIDL_PERSONAL,
//								nullptr,
//								SHGFP_TYPE_CURRENT,
//								path)))
//	{
//		PathAppendA(
//				path,
//				"0xC_kL\\data\\");
//		pathList.push_back(path);
//	}

	if (GetModuleFileNameA( // get binary-directory
						nullptr,
						path,
						MAX_PATH) != 0)
	{
		PathRemoveFileSpecA(path);
		PathAppendA(
				path,
				"data\\");
		pathList.push_back(path);
	}

//	if (GetCurrentDirectoryA( // get working-directory
//						MAX_PATH,
//						path) != 0)
//	{
//		PathAppendA(
//				path,
//				"data\\");
//		pathList.push_back(path);
//	}
#else
	char const* home (getHome());
#	ifdef __HAIKU__
	pathList.push_back("/boot/apps/OpenXcom/data/");
#	endif

	char path[MAXPATHLEN];

	// Get user-specific data folders
	if (char const* const xdg_data_home = getenv("XDG_DATA_HOME"))
		snprintf(path, MAXPATHLEN, "%s/openxcom/data/", xdg_data_home);
	else
#	ifdef __APPLE__
		snprintf(path, MAXPATHLEN, "%s/Library/Application Support/OpenXcom/data/", home);
#	else
		snprintf(path, MAXPATHLEN, "%s/.local/share/openxcom/data/", home);
#	endif

	pathList.push_back(path);

	// Get global data folders
	if (char* xdg_data_dirs = getenv("XDG_DATA_DIRS"))
	{
		char* dir (strtok(xdg_data_dirs, ":"));
		while (dir != nullptr)
		{
			snprintf(path, MAXPATHLEN, "%s/openxcom/data/", dir);
			pathList.push_back(path);
			dir = strtok(0, ":");
		}
	}

#	ifdef __APPLE__
	snprintf(path, MAXPATHLEN, "%s/Users/Shared/OpenXcom/data/", home);
	pathList.push_back(path);
#	else
	pathList.push_back("/usr/local/share/openxcom/data/");
	pathList.push_back("/usr/share/openxcom/data/");

#		ifdef DATADIR
	snprintf(path, MAXPATHLEN, "%s/data/", DATADIR);
	pathList.push_back(path);
#		endif
#	endif

	// Get working directory
	pathList.push_back("./data/");
#endif

	return pathList;
}

/**
 * Builds a list of predefined paths for the user-folder according to the
 * operating-system.
 * @return, list of data-paths
 */
std::vector<std::string> findUserFolders()
{
	std::vector<std::string> pathList;

#ifdef __MORPHOS__
	pathList.push_back("PROGDIR:");
	return pathList;
#endif

#ifdef _WIN32
	char path[MAX_PATH];

//	if (SUCCEEDED(SHGetFolderPathA( // get documents-folder
//								nullptr,
//								CSIDL_PERSONAL,
//								nullptr,
//								SHGFP_TYPE_CURRENT,
//								path)))
//	{
//		PathAppendA(
//				path,
//				"OpenXcom\\");
//		pathList.push_back(path);
//	}

	if (GetModuleFileNameA( // get binary-directory
						nullptr,
						path,
						MAX_PATH) != 0)
	{
		PathRemoveFileSpecA(path);
		PathAppendA(
				path,
				"user\\");
		pathList.push_back(path);
	}

//	if (GetCurrentDirectoryA( // get working-directory
//						MAX_PATH,
//						path) != 0)
//	{
//		PathAppendA(
//				path,
//				"user\\");
//		pathList.push_back(path);
//	}
#else
#	ifdef __HAIKU__
	pathList.push_back("/boot/apps/OpenXcom/");
#	endif

	char const* home (getHome());
	char path[MAXPATHLEN];

	// get user-folders
	if (char const* const xdg_data_home = getenv("XDG_DATA_HOME"))
		snprintf(path, MAXPATHLEN, "%s/openxcom/", xdg_data_home);
	else
#	ifdef __APPLE__
		snprintf(path, MAXPATHLEN, "%s/Library/Application Support/OpenXcom/", home);
#	else
		snprintf(path, MAXPATHLEN, "%s/.local/share/openxcom/", home);
#	endif

	pathList.push_back(path);

	// Get old-style folder
	snprintf(path, MAXPATHLEN, "%s/.openxcom/", home);
	pathList.push_back(path);

	// Get working directory
	pathList.push_back("./user/");
#endif

	return pathList;
}

/**
 * Finds the config-folder according to the operating-system.
 * @return, config-path
 */
std::string findConfigFolder()
{
#ifdef __MORPHOS__
	return "PROGDIR:";
#endif

#if defined(_WIN32) || defined(__APPLE__)
	return "";
#elif defined (__HAIKU__)
	return "/boot/home/config/settings/openxcom/";
#else
	char const* home (getHome());
	char path[MAXPATHLEN];

	if (char const* const xdg_config_home = getenv("XDG_CONFIG_HOME")) // get config-folders on Linux
	{
		snprintf(path, MAXPATHLEN, "%s/openxcom/", xdg_config_home);
		return path;
	}
	else
	{
		snprintf(path, MAXPATHLEN, "%s/.config/openxcom/", home);
		return path;
	}
#endif
}

/**
 * Takes a path and tries to find it based on the system's case-sensitivity.
 * @note There's no actual method for figuring out the correct
 * filename on case-sensitive systems; this is just a workaround.
 * @param base - reference to base unaltered path
 * @param path - reference to full path to check for casing
 * @return, correct filename or "" if it doesn't exist
 */
std::string caseInsensitive(
		const std::string& base,
		const std::string& path)
{
	std::string
		fullPath (base + path),
		newPath (path);

	// Try all various case mutations
	if (fileExists(fullPath.c_str()) == true) // Normal unmangled
		return fullPath;

	std::transform( // UPPERCASE
				newPath.begin(),
				newPath.end(),
				newPath.begin(),
				toupper);
	fullPath = base + newPath;
	if (fileExists(fullPath.c_str()) == true)
		return fullPath;

	std::transform( // lowercase
				newPath.begin(),
				newPath.end(),
				newPath.begin(),
				tolower);
	fullPath = base + newPath;
	if (fileExists(fullPath.c_str()) == true)
		return fullPath;

	return ""; // If it got here nothing can help it
}

/**
 * Takes a path and tries to find it based on the system's case-sensitivity.
 * @note There's no actual method for figuring out the correct
 * foldername on case-sensitive systems; this is just a workaround.
 * @param base - reference to base unaltered path
 * @param path - reference to full path to check for casing
 * @return, correct foldername or "" if it doesn't exist
 */
std::string caseInsensitiveFolder(
		const std::string& base,
		const std::string& path)
{
	std::string
		fullPath (base + path),
		newPath (path);

	// Try all various case mutations
	if (folderExists(fullPath.c_str()) == true) // normal unmangled
		return fullPath;

	std::transform( // UPPERCASE
				newPath.begin(),
				newPath.end(),
				newPath.begin(),
				toupper);
	fullPath = base + newPath;
	if (folderExists(fullPath.c_str()) == true)
		return fullPath;

	std::transform( // lowercase
				newPath.begin(),
				newPath.end(),
				newPath.begin(),
				tolower);
	fullPath = base + newPath;
	if (folderExists(fullPath.c_str()) == true)
		return fullPath;

	return ""; // if it got here nothing can help it
}

/**
 * Takes a filename and tries to find it in the game's data-folders
 * accounting for the system's case-sensitivity and path-style.
 * @param file - reference to original filename
 * @return, correct filename or "" if it doesn't exist
 */
std::string getDataFile(const std::string& file)
{
	std::string st (file); // correct folder-separator

#ifdef _WIN32
	std::replace(
			st.begin(),
			st.end(),
			'/',
			PATH_SEPARATOR);
#endif

	std::string path (caseInsensitive(
									Options::getDataFolder(),
									st));
	if (path.empty() == false) // check current data-path
		return path;

	for (std::vector<std::string>::const_iterator // check every other path
			i = Options::getDataFolders().begin();
			i != Options::getDataFolders().end();
			++i)
	{
		path = caseInsensitive(*i, st);
		if (path.empty() == false)
		{
			Options::setDataFolder(*i);
			return path;
		}
	}
	return file; // give up
}

/**
 * Takes a foldername and tries to find it in the game's data-folders
 * accounting for the system's case-sensitivity and path-style.
 * @param folder - reference to original foldername
 * @return, correct foldername or "" if it doesn't exist
 */
std::string getDataFolder(const std::string& folder)
{
	std::string st (folder); // correct folder-separator

#ifdef _WIN32
	std::replace(
			st.begin(),
			st.end(),
			'/',
			PATH_SEPARATOR);
#endif

	std::string path (caseInsensitiveFolder(
										Options::getDataFolder(),
										st));
	if (path.empty() == false) // check current data-path
		return path;

	for (std::vector<std::string>::const_iterator // check every other path
			i = Options::getDataFolders().begin();
			i != Options::getDataFolders().end();
			++i)
	{
		path = caseInsensitiveFolder(*i, st);
		if (path.empty() == false)
		{
			Options::setDataFolder(*i);
			return path;
		}
	}
	return folder; // give up
}

/**
 * Creates a folder at the specified path.
 * @note Only creates the last folder on the path.
 * @param path - reference to full path
 * @return, true if folder was created
 */
bool createFolder(const std::string& path)
{
#ifdef _WIN32
	if (CreateDirectoryA(path.c_str(), nullptr) != 0)
		return true;

	return false;
#else
	mode_t process_mask (umask(0));
	const int result (mkdir(path.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH));
	umask(process_mask);
	if (result == 0) return true;
	return false;
#endif
}

/**
 * Adds an ending slash to a path if necessary.
 * @param path - reference to folder path
 * @return, terminated path
 */
std::string endPath(const std::string& path)
{
	if (path.empty() == false
		&& path.at(path.size() - 1) != PATH_SEPARATOR)
	{
		return path + PATH_SEPARATOR;
	}
	return path;
}

/**
 * Gets the name of all the files contained in a specified folder.
 * @param path	- reference to full path to folder
 * @param ext	- reference to extension of files ("" if it doesn't matter)
 * @return, ordered list of all the files
 */
std::vector<std::string> getFolderContents(
		const std::string& path,
		const std::string& ext)
{
	std::vector<std::string> files;
	std::string extlower (ext);
	std::transform(
				extlower.begin(),
				extlower.end(),
				extlower.begin(),
				::tolower);

	DIR* const pDir (opendir(path.c_str()));
	if (pDir == nullptr)
	{
	#ifdef __MORPHOS__
		return files;
	#else
		std::string error ("Failed to open directory: " + path);
		throw Exception(error);
	#endif
	}

	std::string
		file,
		filedotext;

	struct dirent* pDirent;
	while ((pDirent = readdir(pDir)) != nullptr)
	{
		if ((file = pDirent->d_name) != "." && file != "..")
		{
			if (extlower.empty() == false)
			{
				if (file.length() < extlower.length() + 1u)
					continue;

				filedotext = file.substr(file.length() - extlower.length() - 1u);
				std::transform(
							filedotext.begin(),
							filedotext.end(),
							filedotext.begin(),
							::tolower);
				if (filedotext != "." + extlower)
					continue;
			}
			files.push_back(file);
		}
	}

	closedir(pDir);
	std::sort(
			files.begin(),
			files.end());

	return files;
}

/**
 * Gets the name of all the files contained in a data-subfolder.
 * @note Repeated files are ignored.
 * @param folder	- reference to path to the data-folder
 * @param ext		- reference to extension of files ("" if it doesn't matter)
 * @return, ordered list of all the files
 */
std::vector<std::string> getDataContents(
		const std::string& folder,
		const std::string& ext)
{
	std::set<std::string> uniqueFiles;

	const std::string current (caseInsensitiveFolder( // check current data-path
												Options::getDataFolder(),
												folder));
	if (current.empty() == false)
	{
		const std::vector<std::string> contents (getFolderContents(current, ext));
		for (std::vector<std::string>::const_iterator
				i = contents.begin();
				i != contents.end();
				++i)
		{
			uniqueFiles.insert(*i);
		}
	}

	for (std::vector<std::string>::const_iterator // check every other path
			i = Options::getDataFolders().begin();
			i != Options::getDataFolders().end();
			++i)
	{
		const std::string path (caseInsensitiveFolder(*i, folder));
		if (path != current
			&& path.empty() == false)
		{
			const std::vector<std::string> contents (getFolderContents(path, ext));
			for (std::vector<std::string>::const_iterator
					j = contents.begin();
					j != contents.end();
					++j)
			{
				uniqueFiles.insert(*j);
			}
		}
	}
	return std::vector<std::string> (uniqueFiles.begin(), uniqueFiles.end());
}

/**
 * Checks if a certain path exists and is a folder.
 * @param path - reference to full path to folder
 * @return, true if it exists
 */
bool folderExists(const std::string& path)
{
#ifdef _WIN32
	return (PathIsDirectoryA(path.c_str()) != FALSE);
#elif __MORPHOS__
	BPTR l (Lock(path.c_str(), SHARED_LOCK));
	if (l != nullptr)
	{
		UnLock(l);
		return 1;
	}
	return 0;
#else
	struct stat info;
	return (stat(path.c_str(), &info) == 0 && S_ISDIR(info.st_mode));
#endif
}

/**
 * Checks if a certain path exists and is a file.
 * @param path - reference to full path to file
 * @return, true if it exists
 */
bool fileExists(const std::string& path)
{
#ifdef _WIN32
	return (PathFileExistsA(path.c_str()) != FALSE);
#elif __MORPHOS__
	BPTR l (Lock(path.c_str(), SHARED_LOCK));
	if (l != nullptr)
	{
		UnLock(l);
		return 1;
	}
	return 0;
#else
	struct stat info;
	return (stat(path.c_str(), &info) == 0 && S_ISREG(info.st_mode));
#endif
}

/**
 * Removes a file from the specified path.
 * @param path - reference to a file-path
 * @return, true if the operation succeeded
 */
bool deleteFile(const std::string& path)
{
#ifdef _WIN32
	return (DeleteFileA(path.c_str()) != 0);
#else
	return (remove(path.c_str()) == 0);
#endif
}

/**
 * Gets the base-filename of a path.
 * @param path		- reference to the full path of file
 * @param transform	- pointer to optional function to transform the filename (default nullptr)
 * @return, base filename
 */
std::string baseFilename(
		const std::string& path,
		int (*transform)(int))
{
	std::string file;
	const size_t sep (path.find_last_of(PATH_SEPARATOR));
	if (sep == std::string::npos)
		file = path;
	else if (sep != path.size() - 1u)
		file = path.substr(sep + 1u);
//	else // recurse to get directory. NOTE: This will drill down to "c:" eg.
//		return baseFilename(path.substr(0u, path.size() - 1u));
	else
		return "";

	if (transform != nullptr)
		std::transform( // NOTE: 'std::transform' is not necessarily '(*transform)'.
					file.begin(),
					file.end(),
					file.begin(),
					transform);
	return file;
}

/**
 * Replaces invalid file-system characters with an underscore "_".
 * @param file - reference to a filename
 * @return, filename without invalid characters
 */
std::string sanitizeFilename(const std::string& file)
{
	std::string fileOut (file);
	for (std::string::iterator
			i = fileOut.begin();
			i != fileOut.end();
			++i)
	{
		if (   *i == '<'
			|| *i == '>'
			|| *i == ':'
			|| *i == '"'
			|| *i == '/'
			|| *i == '?'
			|| *i == '\\')
		{
			*i = '_';
		}
	}
	return fileOut;
}

/**
 * Removes the extension from a filename.
 * @param file - reference to a filename
 * @return, filename without the extension
 */
std::string noExt(const std::string& file)
{
	const size_t dot (file.find_last_of('.'));
	if (dot == std::string::npos)
		return file;

	return file.substr(0u, file.find_last_of('.'));
}

/**
 * Gets the current locale of the system in language-COUNTRY format.
 * @return, locale-string
 */
std::string getLocale()
{
#ifdef _WIN32
	char
		language[9u],
		country[9u];

	GetLocaleInfoA(
				LOCALE_USER_DEFAULT,
				LOCALE_SISO639LANGNAME,
				language,
				9);
	GetLocaleInfoA(
				LOCALE_USER_DEFAULT,
				LOCALE_SISO3166CTRYNAME,
				country,
				9);

	std::ostringstream local;
	local << language << "-" << country;

	return local.str();

/*	wchar_t local[LOCALE_NAME_MAX_LENGTH];
	LCIDToLocaleName(GetUserDefaultUILanguage(), local, LOCALE_NAME_MAX_LENGTH, 0);

	return Language::wstrToUtf8(local); */
#else
	std::locale l;
	try
	{
		l = std::locale("");
	}
	catch (std::runtime_error)
	{
		return "x-";
	}
	std::string name (l.name());
	size_t
		dash (name.find_first_of('_')),
		dot (name.find_first_of('.'));

	if (dot != std::string::npos)
		name = name.substr(0, dot - 1);

	if (dash != std::string::npos)
	{
		std::string language (name.substr(0, dash - 1));
		std::string country (name.substr(dash - 1));
		std::ostringstream local;
		local << language << "-" << country;
		return local.str();
	}
	else
		return name + "-";
#endif
}

/**
 * Checks if the system's default-quit-shortcut was pressed.
 * @param ev - reference to SDL event
 * @return, true to quit
 */
bool isQuitShortcut(const SDL_Event& ev)
{
#ifdef _WIN32
	// Alt + F4
	return (ev.type == SDL_KEYDOWN && ev.key.keysym.sym == SDLK_F4 && ev.key.keysym.mod & KMOD_ALT);
#elif __APPLE__
	// Command + Q
	return (ev.type == SDL_KEYDOWN && ev.key.keysym.sym == SDLK_q && ev.key.keysym.mod & KMOD_LMETA);
#else
	// TODO: Add other OSs shortcuts.
	(void)ev;
	return false;
#endif
}

/**
 * Gets the last modified date of a file.
 * @param path - reference to full path to file
 * @return, the timestamp in integral format
 */
std::time_t getDateModified(const std::string& path)
{
/*
#ifdef _WIN32
	WIN32_FILE_ATTRIBUTE_DATA info;
	if (GetFileAttributesExA(path.c_str(), GetFileExInfoStandard, &info))
	{
		FILETIME ft = info.ftLastWriteTime;
		LARGE_INTEGER li;
		li.HighPart = ft.dwHighDateTime;
		li.LowPart = ft.dwLowDateTime;
		return li.QuadPart;
	}
	return 0;
#endif
*/
	struct stat info;
	if (stat(path.c_str(), &info) == 0)
		return info.st_mtime;

	return 0;
}

/**
 * Converts a date/time into a human-readable string using the ISO-8601 standard.
 * @param timeIn - value in timestamp format
 * @return, string pair with date and time
 */
std::pair<std::wstring, std::wstring> timeToString(std::time_t timeIn)
{
	wchar_t
		localDate[25u],
		localTime[25u];
/*
#ifdef _WIN32
	LARGE_INTEGER li;
	li.QuadPart = timeIn;
	FILETIME ft;
	ft.dwHighDateTime = li.HighPart;
	ft.dwLowDateTime = li.LowPart;
	SYSTEMTIME st;
	FileTimeToLocalFileTime(&ft, &ft);
	FileTimeToSystemTime(&ft, &st);

	GetDateFormatW(LOCALE_USER_DEFAULT, DATE_SHORTDATE, &st, nullptr, localDate, 25);
	GetTimeFormatW(LOCALE_USER_DEFAULT, TIME_NOSECONDS, &st, nullptr, localTime, 25);
#endif
*/
	const struct tm* const timeInfo (std::localtime(&timeIn));
	std::wcsftime(
				localDate,
				25,
				L"%Y-%m-%d",
				timeInfo);
	std::wcsftime(
				localTime,
				25,
				L"%H:%M",
				timeInfo);

	return std::make_pair(
						localDate,
						localTime);
}

/**
 * Gets a date/time in a human-readable string using the ISO-8601 standard.
 * @return, string of Time
 */
std::string timeString()
{
	char curTime[13u];

	std::time_t timeOut (std::time(nullptr));
	struct tm* const timeInfo (std::localtime(&timeOut));
	std::strftime(
				curTime,
				13,
				"%y%m%d%H%M%S",
				timeInfo);
	return curTime;
}

/**
 * Generates a time-string of the current time.
 * @return String in D-M-Y_H-M-S format.
 */
std::string now()
{
	const size_t
		MAX_LEN    (25u),
		MAX_RESULT (80u);

	char result[MAX_RESULT] {0};

#ifdef _WIN32
	char
		date[MAX_LEN],
		time[MAX_LEN];

	if (GetDateFormatA(
					LOCALE_INVARIANT,
					0, nullptr,
					"dd'-'MM'-'yyyy", date,
					static_cast<int>(MAX_LEN)) == 0)
		return "00-00-0000";

	if (GetTimeFormatA(
					LOCALE_INVARIANT,
					TIME_FORCE24HOURFORMAT, nullptr,
					"HH'-'mm'-'ss", time,
					static_cast<int>(MAX_LEN)) == 0)
		return "00-00-00";

	sprintf(result, "%s_%s", date, time);

#else
	char buffer[MAX_LEN];
	std::time_t rawtime;
	struct tm* timeinfo;
	time(&rawtime);
	timeinfo = localtime(&rawtime);
	strftime(buffer, MAX_LEN, "%d-%m-%Y_%H-%M-%S", timeinfo);
	sprintf(result, "%s", buffer);
#endif

	return result;
}

/**
 * Compares two Unicode strings using natural human ordering.
 * @param a - reference to string-A
 * @param b - reference to string-B
 * @return, true if string-A comes before string-B
 */
bool naturalCompare(
		const std::wstring& a,
		const std::wstring& b)
{
#if defined (_WIN32) && (!defined(__MINGW32__) || defined(__MINGW64_VERSION_MAJOR))
	return (StrCmpLogicalW(
						a.c_str(),
						b.c_str()) < 0);
#else
	// sorry unix users you get ASCII sort
	std::wstring::const_iterator
		i,j;
	for (
			i = a.begin(), j = b.begin();
			i != a.end() && j != b.end() && tolower(*i) == tolower(*j);
			i++, j++);

	return (i != a.end() && j != b.end()
			&& tolower(*i) < tolower(*j));
#endif
}

/**
 * Moves a file from one path to another replacing any existing file.
 * @param src - reference to the source-path
 * @param dst - reference to the destination-path
 * @return, true if the operation succeeded
 */
bool moveFile(
		const std::string& src,
		const std::string& dst)
{
#ifdef _WIN32
	return (MoveFileExA(
					src.c_str(),
					dst.c_str(),
					MOVEFILE_REPLACE_EXISTING) != 0);
#else
//	return (rename(src.c_str(), dest.c_str()) == 0);
	std::ifstream srcStream;
	std::ofstream destStream;
	srcStream.exceptions(std::ifstream::failbit | std::ifstream::badbit);
	destStream.exceptions(std::ofstream::failbit | std::ofstream::badbit);
	try
	{
		srcStream.open(src.c_str(), std::ios::binary);
		destStream.open(dst.c_str(), std::ios::binary);
		destStream << srcStream.rdbuf();
		srcStream.close();
		destStream.close();
	}
	catch (std::fstream::failure)
	{
		return false;
	}
	return deleteFile(src);
#endif
}

/**
 * Notifies the user that maybe he/she should have a look.
 */
void flashWindow()
{
#ifdef _WIN32
	SDL_SysWMinfo wminfo;
	SDL_VERSION(&wminfo.version)

	if (SDL_GetWMInfo(&wminfo))
	{
		HWND hwnd (wminfo.window);
		FlashWindow(hwnd, true);
	}
#endif
}

/**
 * Gets the executable path in DOS-style (short) form.
 * @note For non-Windows systems, just use a dummy path.
 * @return, executable path
 */
std::string getDosPath()
{
#ifdef _WIN32
	char path[MAX_PATH];
	if (GetModuleFileNameA(nullptr, path, MAX_PATH) != 0)
		return path;

	return "oh boy you're fucked now Lulzorcopter.";

	// Some people just love to code ... suggest: dog, take walk.
/*	std::string
		path,
		bufstr;
	char buf[MAX_PATH];

	if (GetModuleFileNameA(0, buf, MAX_PATH) != 0)
	{
		bufstr = buf;
		size_t c1 = bufstr.find_first_of('\\');
		path += bufstr.substr(0, c1 + 1);
		size_t c2 = bufstr.find_first_of('\\', c1 + 1);
		while (c2 != std::string::npos)
		{
			std::string dir = bufstr.substr(c1 + 1, c2 - c1 - 1);
			if (dir == "..")
			{
				path = path.substr(0, path.find_last_of('\\', path.length() - 2));
			}
			else
			{
				if (dir.length() > 8)
					dir = dir.substr(0,6) + "~1";

//				std::transform(dir.begin(), dir.end(), dir.begin(), ::toupper);
				path += dir;
			}

			c1 = c2;

			c2 = bufstr.find_first_of('\\', c1 + 1);
			if (c2 != std::string::npos)
				path += '\\';
		}
	}
	else
		path = "c:\\games\\OpenXcom";

	return path; */
#else
	return "c:\\games\\OpenXcom";
#endif
}

#ifdef _WIN32
/**
 * Sets the window-icon for _WIN32 build-configuration.
 * @note Uses the embedded resource-icon.
 * @param winResource - ID for Windows icon
 */
void setWindowIcon(int winResource)
{
	const HINSTANCE handle (GetModuleHandle(nullptr));
	const HICON icon (LoadIcon(
							handle,
							MAKEINTRESOURCE(winResource)));
	SDL_SysWMinfo wminfo;
	SDL_VERSION(&wminfo.version)
	if (SDL_GetWMInfo(&wminfo) != 0)
	{
		const HWND hwnd (wminfo.window);
		SetClassLongPtr(
					hwnd,
					GCLP_HICON,
					reinterpret_cast<LONG_PTR>(icon));
	}
}
#else
/**
 * Sets the window-icon if not _WIN32 build.
 * @param unixPath - reference to path to PNG-icon for Unix
 */
void setWindowIcon(const std::string& unixPath)
{
	// SDL only takes UTF-8 filenames so here's an ugly hack to match this ugly.
	const std::string utf8 (Language::wstrToUtf8(Language::fsToWstr(CrossPlatform::getDataFile(unixPath))));
	SDL_Surface* const icon (IMG_Load(utf8.c_str()));
	if (icon != nullptr)
	{
		SDL_WM_SetIcon(icon, nullptr);
		SDL_FreeSurface(icon);
	}
}
#endif

}

}
