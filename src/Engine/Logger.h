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

#ifndef OPENXCOM_LOGGER_H
#define OPENXCOM_LOGGER_H

#ifdef _MSC_VER
#	ifndef _CRT_SECURE_NO_WARNINGS
#		define _CRT_SECURE_NO_WARNINGS
#	endif
#endif

//#include <cstdio>		// std::fprintf(), std::fflush(), std::fopen(), std::fclose()
//#include <ostream>	// std::endl
//#include <string>		// std::string
#include <sstream>		// std::ostringstream

#include "CrossPlatform.h"


namespace OpenXcom
{

/**
 * Defines the various severity-levels of information logged by the game.
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
	/// Logger copy-constructor.
//	Logger(const Logger&);
	/// Logger assignment-operator.
//	Logger& operator= (const Logger&);


	/*protected:*/
	std::ostringstream _oststr;


		public:
			/// cTor.
			Logger();
			/// dTor.
			/*virtual*/ ~Logger();

			///
			std::ostringstream& get(SeverityLevel level = LOG_INFO);

			///
			static SeverityLevel& reportLevel();
			///
			static std::string& logFile();
			///
			static std::string toString(SeverityLevel level);
};


/* In modern C++, inline tells the linker that, if multiple definitions (not
	declarations) are found in different translation units, they are all the
	same, and the linker can freely keep one and discard all the other ones. */
// http://stackoverflow.com/questions/145838/benefits-of-inline-functions-in-c/7418299#7418299
/* inline allows you to place a function definition in a header file and
	#include that header file in multiple source files without violating the
	one definition rule. */
// http://stackoverflow.com/questions/145838/benefits-of-inline-functions-in-c/7414495#7414495
/* inline is more like static or extern than a directive telling the compiler to
	inline your functions. extern, static, inline are linkage directives, used
	almost exclusively by the linker, not the compiler.

   It is said that inline hints to the compiler that you think the function
	should be inlined. That may have been true in 1998, but a decade later the
	compiler needs no such hints. Not to mention humans are usually wrong when
	it comes to optimizing code, so most compilers flat out ignore the 'hint'.

   static - the variable/function name cannot be used in other compilation units.
	Linker needs to make sure it doesn't accidentally use a statically defined
	variable/function from another compilation unit.

   extern - use this variable/function name in this compilation unit but don't
	complain if it isn't defined. The linker will sort it out and make sure all
	the code that tried to use some extern symbol has its address.

   inline - this function will be defined in multiple compilation units, don't
	worry about it. The linker needs to make sure all compilation units use a
	single instance of the variable/function.

   Note: Generally declaring templates inline is pointless, as they have the
	linkage semantics of inline already. However, explicit specialization and
	instantiation of templates require inline to be used. */
// http://stackoverflow.com/questions/1759300/when-should-i-write-the-keyword-inline-for-a-function-method/1759575#1759575

/**
 *
 */
inline Logger::Logger()
{}

/**
 * Converts the SeverityLevel-type to a string.
 * @param level - the severity-level (default LOG_INFO)
 */
inline std::ostringstream& Logger::get(SeverityLevel level)
{
	_oststr << "[" << toString(level) << "]" << "\t";
	return _oststr;
}

/**
 * dTor.
 */
inline Logger::~Logger() // virtual. NOTE: This need not be virtual.
{
	_oststr << std::endl;

	std::ostringstream oststr;
	oststr << "[" << CrossPlatform::now() << "]" << "\t" << _oststr.str();
	FILE* const file (std::fopen(logFile().c_str(), "a"));
	if (file != nullptr)
	{
		std::fprintf(
					file,
					"%s",
					oststr.str().c_str());

		std::fflush(file);
		std::fclose(file);
	}

	switch (reportLevel())
	{
		default:
			if (file != nullptr) break;
			// no break;

		case LOG_DEBUG:
		case LOG_VERBOSE:
			std::fprintf(			// FIX: Print to console is not working as expected in MinGW-w64.
						stdout,		// was 'stderr'
						"%s",
						_oststr.str().c_str());
			std::fflush(stdout);	// was 'stderr'
	}
}

/**
 * Gets/Sets the maximum SeverityLevel.
 * @return, reference to the current maximum SeverityLevel
 */
inline SeverityLevel& Logger::reportLevel() // static.
{
	static SeverityLevel reportLevel (LOG_DEBUG);
	return reportLevel;
}

/**
 * Returns the log-file string.
 * @return, filename + extension
 */
inline std::string& Logger::logFile() // static.
{
	static std::string logFile ("openxcom.log");
	return logFile;
}

/**
 * Converts a severity enumerator-key to a string.
 * @param level - the severity-level (default LOG_INFO)
 * @return, severity-level as a string
 */
inline std::string Logger::toString(SeverityLevel level) // static.
{
	static const char* const buffer[]
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

// macro: Log
#define Log(level) if (level > Logger::reportLevel()) ; else Logger().get(level)

}

#endif
