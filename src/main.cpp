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

//#define SDL_MAIN_HANDLED (valid for SDL2 only)

#include <exception>	// std::exception
#include <sstream>		// std::ostringstream

#include "version.h"

#include "Engine/CrossPlatform.h"
#include "Engine/Game.h"
#include "Engine/Logger.h"
#include "Engine/Options.h"
#include "Engine/Screen.h" // kL

#include "Menu/StartState.h"


/**
 * @mainpage
 * @author OpenXcom Developers
 *
 * OpenXcom/0xC is an open-source clone of the original X-Com written entirely
 * in C++ and SDL. This documentation contains info on every class contained in
 * the source code and its public methods. The code itself also contains in-line
 * comments for more complicated code-blocks. Hopefully all of this will make
 * the code a lot more readable for you in case you wish to learn or fork or
 * make use of it in your own projects - though note that all the orginal
 * source-code is licensed under the GNU General Public License. Enjoy!!!!
 */

using namespace OpenXcom;

Game* ptrG (nullptr);

// If you can't tell what the main() is for you should have your programming
// license revoked ... and be stripped naked, slathered with ice-cream, and tied
// down in the middle of a room full of kittens. See "entry point".
int main(
		int argc,
		char* argv[])
{
	// To check memory leaks in VS
//	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

#ifndef _DEBUG
	try
	{
		Logger::reportLevel() = LOG_INFO;
#else
		Logger::reportLevel() = LOG_DEBUG;
#endif

		if (Options::init(argc, argv) == false)
			return EXIT_SUCCESS;

		if (Options::verboseLogging == true)
			Logger::reportLevel() = LOG_VERBOSE;

//		Options::baseXResolution = Options::displayWidth;
//		Options::baseYResolution = Options::displayHeight;
		Options::baseXResolution = Screen::ORIGINAL_WIDTH;	// kL
		Options::baseYResolution = Screen::ORIGINAL_HEIGHT;	// kL

		std::ostringstream title;
		title << OPENXCOM_VERSION_GIT << " " << Version::getBuildDate();

		ptrG = new Game(title.str());

		State::setGamePtr(ptrG);

		ptrG->setState(new StartState());
		ptrG->run();

#ifndef _DEBUG
	}
	catch (std::exception& e)
	{
		CrossPlatform::showFatalError(e.what());
		exit(EXIT_FAILURE);
	}
#endif

//	Options::save();	// -> wtf keeps rewriting my options.cfg .....
						// yet it works fine in Game::run() before quitting.
						// Ps. why are they even doing Options::save() twice
						// ... now they both fuck up. BYE!
						// ah, Sup finally caught that eh.

	// Comment this for faster exit. by an attosecond ...
	delete ptrG;

	Log(LOG_INFO) << "0xC_kL is shutting down.";
	return EXIT_SUCCESS;
}


#ifdef __MORPHOS__
const char Version[] = "$VER: OpenXCom " OPENXCOM_VERSION_SHORT " (" __AMIGADATE__  ")";
#endif // __MORPHOS__
