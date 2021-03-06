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

#include "version.h"

#include <ctime>
#include <sstream>

#ifdef _WIN32
#	ifndef NOMINMAX
#		define NOMINMAX
#	endif

#	ifndef WIN32_LEAN_AND_MEAN
#		define WIN32_LEAN_AND_MEAN
#	endif

//#	include <shlobj.h>
//#	include <shlwapi.h>
#	include <windows.h>
#endif


namespace OpenXcom
{

namespace Version
{

/**
 * Gets a date/time in a human-readable string using the ISO 8601 standard.
 * @note This uses current runtime and is therefore apropos only as a time-stamp
 * for a saved-file eg.
 * @return, string of local-time useful for saved-files
 */
std::string timeStamp()
{
	time_t timeOut (std::time(nullptr));
	const struct tm* const timeInfo (std::localtime(&timeOut));

	char verDate[7u];
	std::strftime(
				verDate,
				7u,
				"%y%m%d",
				timeInfo);

	char verTime[7u];
	std::strftime(
				verTime,
				7u,
				"%H%M%S",
				timeInfo);

	std::ostringstream stamp;
	stamp << verDate << "-" << verTime;

	return stamp.str();
}

/**
 * Gets version as a time-string.
 * @note This is the (local) compile-time date & time.
 * @param built - true to add "built" preface (default true)
 * @return, current build-date of executable
 */
std::string getBuildDate(bool built)
{
	std::ostringstream oststr;

#ifdef _DEBUG
	oststr << "dBug ";
#endif

	if (built == true)
		oststr << "b> ";

	std::string tz;

#ifdef _WIN32
	TIME_ZONE_INFORMATION tziTest;
	const DWORD dwRet (GetTimeZoneInformation(&tziTest));
	if (dwRet == TIME_ZONE_ID_DAYLIGHT)
		tz = " MDT"; // wprintf(L"%s\n", tziTest.DaylightName);
	else if (dwRet == TIME_ZONE_ID_STANDARD || dwRet == TIME_ZONE_ID_UNKNOWN)
		tz = " MST"; // wprintf(L"%s\n", tziTest.StandardName);
//	else printf("GTZI failed (%d)\n", GetLastError());
#endif

	oststr << __DATE__ << " " << __TIME__ << tz;
	std::string st (oststr.str());

	const size_t pos (st.find("  ")); // remove possible double-space between month & single-digit days
	if (pos != std::string::npos)
		st.erase(pos, 1u);

	return st;
}

}

}
