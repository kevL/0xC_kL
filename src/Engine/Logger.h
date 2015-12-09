/*
 * Copyright 2010-2015 OpenXcom Developers.
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

#ifndef OPENXCOM_LOGGER_H
#define OPENXCOM_LOGGER_H

#include <sstream>
//#include <string>
//#include <stdio.h>

#ifdef _WIN32 // see also: pch.h ... & Engine/Language.cpp & Engine/CrossPlatform.cpp
#	ifndef NOMINMAX
#		define NOMINMAX
#	endif

#	ifndef WIN32_LEAN_AND_MEAN
#		define WIN32_LEAN_AND_MEAN
#	endif

#	include <windows.h>

	// the following macros interfere with std::max and std::min as used throughout ...
	// Should be taken care of by NOMINMAX above^
//	#undef min
//	#undef max

#	ifndef LOCALE_INVARIANT
#		define LOCALE_INVARIANT 0x007f
#	endif
#else
#	include <time.h>
#endif


namespace OpenXcom
{

///
inline std::string now();

/**
 * Defines the various severity levels of information logged by the game.
 */
enum SeverityLevel
{
	LOG_FATAL,		// 0 - Something horrible has happened and the game is going to die!
	LOG_ERROR,		// 1 - Something bad happened but we can still move on.
	LOG_WARNING,	// 2 - Something weird happened, nothing special but it's good to know.
	LOG_INFO,		// 3 - Useful information for users/developers to help debug and figure stuff out.
	LOG_DEBUG,		// 4 - Purely test stuff to help developers implement, not really relevant to users.
	LOG_VERBOSE     // 5 - Extra details that even Planck wouldn't have cared about 90% of the time.
};

/**
 * A basic logging and debugging class.
 * @note Prints output to stdout/files and can capture stack traces of fatal
 * errors too. Wasn't really satisfied with any of the libraries around so I
 * rolled my own. Based on http://www.drdobbs.com/cpp/logging-in-c/201804215
 */
class Logger
{

private:
	/// Logger copy constructor
	Logger(const Logger&);
	/// Logger assignment operator
	Logger& operator= (const Logger&);


	protected:
		std::ostringstream _oststr;


		public:
			/// cTor.
			Logger();
			/// dTor.
			virtual ~Logger();

			///
			std::ostringstream& get(SeverityLevel level = LOG_INFO);

			///
			static SeverityLevel& reportingLevel();
			///
			static std::string& logFile();
			///
			static std::string toString(SeverityLevel level);
};


/**
 *
 */
inline Logger::Logger()
{}

/**
 *
 */
inline std::ostringstream& Logger::get(SeverityLevel level)
{
	_oststr << "[" << toString(level) << "]" << "\t";
	return _oststr;
}

/**
 *
 */
inline Logger::~Logger() // virtual.
{
	_oststr << std::endl;
	if (reportingLevel() == LOG_DEBUG || reportingLevel() == LOG_VERBOSE)
	{
		// FIX: Print to console is not working as expected in MinGW-w64.
		std::fprintf(
					stdout, // was 'stderr'
					"%s",
					_oststr.str().c_str());

		std::fflush(stdout); // was 'stderr'
	}

	std::ostringstream oststr;
	oststr << "[" << now() << "]" << "\t" << _oststr.str();
	FILE* file = std::fopen(logFile().c_str(), "a");
	std::fprintf(
				file,
				"%s",
				oststr.str().c_str());

	std::fflush(file);
	std::fclose(file);
}

/**
 *
 */
inline SeverityLevel& Logger::reportingLevel() // static.
{
	static SeverityLevel reportingLevel = LOG_DEBUG;
	return reportingLevel;
}

/**
 *
 */
inline std::string& Logger::logFile() // static.
{
	static std::string logFile = "openxcom.log";
	return logFile;
}

/**
 *
 */
inline std::string Logger::toString(SeverityLevel level) // static.
{
	static const char* const buffer[] =
	{
		"FATAL",	// 0 - LOG_FATAL
		"ERROR",	// 1 - LOG_ERROR
		"WARN",		// 2 - LOG_WARNING
		"INFO",		// 3 - LOG_INFO
		"DEBUG",	// 4 - LOG_DEBUG
		"VERBOSE"	// 5 - LOG_VERBOSE
	};

	return buffer[level];
}

#define Log(level) if (level > Logger::reportingLevel()) ; else Logger().get(level)

/**
 *
 */
inline std::string now()
{
	const int
		MAX_LEN = 25,
		MAX_RESULT = 80;
#ifdef _WIN32
	char
		date[MAX_LEN],
		tyme[MAX_LEN];

	if (GetDateFormatA(
					LOCALE_INVARIANT,
					0, nullptr,
					"dd'-'MM'-'yyyy",
					date,
					MAX_LEN) == 0)
	{
		return "Error in Now() [1]";
	}

	if (GetTimeFormatA(
					LOCALE_INVARIANT,
					TIME_FORCE24HOURFORMAT,
					nullptr,
					"HH':'mm':'ss",
					tyme,
					MAX_LEN) == 0)
	{
		return "Error in Now() [2]";
	}

	char result[MAX_RESULT] = {0};
	std::sprintf(
				result,
				"%s %s",
				date,
				tyme);
#else
	char buffer[MAX_LEN];
	time_t rawtime;
	struct tm* timeinfo;
	std::time(&rawtime);
	timeinfo = std::localtime(&rawtime);
	std::strftime(
				buffer,
				MAX_LEN,
				"%d-%m-%Y %H:%M:%S",
				timeinfo);
	char result[MAX_RESULT] = {0};
	std::sprintf(
				result,
				"%s",
				buffer);
#endif

	return result;
}

}

#endif
