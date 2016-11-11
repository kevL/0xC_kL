/*
 * Copyright 2010-2016 OpenXcom Developers.
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

#include "Options.h"

//#include <algorithm>	// std::transform()
//#include <cstdio>		// std::fopen(), fflush(), fclose()
#include <fstream>		// std::ofstream
#include <iostream>		// std::cout
//#include <map>		// std::map
//#include <ostream>	// std::endl
//#include <sstream>	// std::ostringstream
//#include <utility>	// std::swap()

#include <SDL/SDL.h>
//#include <SDL/SDL_keysym.h>
#include <SDL/SDL_mixer.h>

//#include <yaml-cpp/yaml.h>

#include "../version.h"

#include "CrossPlatform.h"
#include "Exception.h"
#include "Logger.h"
#include "Screen.h"


namespace OpenXcom
{

namespace Options
{

#define OPT
#include "Options.inc.h"
#undef OPT


// Options VARIABLES //
std::string
	_dirConfig,
	_dirData,
	_dirPic,
	_dirUser;

std::vector<std::string> _dataFolders;

std::vector<OptionInfo> _info;

std::map<std::string, std::string> _cLine;


// Options FUNCTIONS //

/**
 * Handles the initialization of default-options as well as finding and loading
 * any existing ones.
 * @note This is the first call in main()!
 * @param argc - quantity of arguments
 * @param argv - pointer to an array of argument-strings
 * @return, true if initialization happened, false if help was invoked
 */
bool init(
		int argc,
		char* argv[])
{
	if (showHelp(argc, argv) == false)
	{
		loadArgs(argc, argv);

		setFolders();

		createOptions();

//		resetDefaults();
//		rulesets.clear();
//		rulesets.push_back("Xcom1Ruleset");
		backupDisplay();

		userOptions();


		const std::string st (getUserFolder() + "openxcom.log");
		Logger::logFile() = st;
		FILE* const file (std::fopen(Logger::logFile().c_str(), "w"));
		if (file != nullptr)
		{
			std::fflush(file);
			std::fclose(file);
		}
		else
			throw Exception(st + " not found");


		Log(LOG_INFO) << "Data-folders:";
		for (std::vector<std::string>::const_iterator
				i = _dataFolders.begin();
				i != _dataFolders.end();
				++i)
		{
			Log(LOG_INFO) << "- " << *i;
		}

		//Log(LOG_INFO) << "Data folder: "	<< _dirData;
		Log(LOG_INFO) << "User-folder: "	<< _dirUser;
		Log(LOG_INFO) << "Config-folder: "	<< _dirConfig;
		Log(LOG_INFO) << "Picture-folder: "	<< _dirPic;

		Log(LOG_INFO) << "Options loaded.";
		return true;
	}
	return false;
}

/**
 * Displays command-line help when requested.
 * @note Help can be invoked with
 *		/?
 *		/help
 *		-?
 *		-help
 *		--?
 *		--help
 * 'help' is a funny word. who came up with 'help'. who's the guy who first
 * shouted "help!" and did anyone care. did they even know what 'help' is
 * @param argc - quantity of arguments
 * @param argv - pointer to an array of argument-strings
 * @return, true if help shown
 */
bool showHelp(
		int argc,
		char* argv[])
{
	bool show = false;

	std::string arg;
	const size_t argc_t (static_cast<size_t>(argc));
	for (size_t
			i = 1u;
			i != argc_t;
			++i)
	{
		arg = argv[i];

		if (arg.length() > 1u && (arg[0u] == '-' || arg[0u] == '/'))
		{
			if (arg.length() > 2u && arg[1u] == '-')
				arg = arg.substr(2u, arg.length() - 1u);
			else
				arg = arg.substr(1u, arg.length() - 1u);

			std::transform(
						arg.begin(),
						arg.end(),
						arg.begin(),
						::tolower);

			if (arg == "help" || arg == "?")
				show = true;
		}
	}

	if (show == true)
	{
		std::ostringstream help;

//		help << "OpenXcom v" << OPENXCOM_VERSION_SHORT << std::endl;
		help << OPENXCOM_VERSION_GIT << " " << Version::getBuildDate() << std::endl;
		help << "Usage: openxcom [OPTION] ..." << std::endl << std::endl;
		help << "-data PATH" << std::endl;
		help << " use PATH as the default Data Folder instead of auto-detecting" << std::endl << std::endl;
		help << "-user PATH" << std::endl;
		help << " use PATH as the default User Folder instead of auto-detecting" << std::endl << std::endl;
		help << "-cfg PATH" << std::endl;
		help << " use PATH as the default Config Folder instead of auto-detecting" << std::endl << std::endl;
		help << "-KEY VALUE" << std::endl;
		help << " set option KEY to VALUE instead of default/loaded value (eg. -displayWidth 640)" << std::endl << std::endl;
		help << "-help" << std::endl;
		help << "-?" << std::endl;
		help << " show command-line help" << std::endl;

		std::cout << help.str();
	}
	return show;
}

/**
 * Loads Options from command-line parameters.
 * @note Accepted formats:
 *	"-option value"
 *	"--option value"
 *	"/option value"
 * @param argc - quantity of arguments
 * @param argv - pointer to an array of argument-strings
 */
void loadArgs(
		int argc,
		char* argv[])
{
	std::string arg;

	const size_t argc_t (static_cast<size_t>(argc));
	for (size_t
			i = 1u;
			i != argc_t;
			++i)
	{
		arg = argv[i];

		if (arg.length() > 1u && (arg[0u] == '-' || arg[0u] == '/'))
		{
			if (arg.length() > 2u && arg[1u] == '-')
				arg = arg.substr(2u, arg.length() - 1u);
			else
				arg = arg.substr(1u, arg.length() - 1u);

			std::transform(
						arg.begin(),
						arg.end(),
						arg.begin(),
						::tolower);

			if (argc_t > i + 1u)
			{
				if		(arg == "data")
					_dirData = CrossPlatform::endPath(argv[i + 1u]);
				else if	(arg == "user")
					_dirUser = CrossPlatform::endPath(argv[i + 1u]);
				else if	(arg == "cfg")
					_dirConfig = CrossPlatform::endPath(argv[i + 1u]);
				else if	(arg == "pic")
					_dirPic = CrossPlatform::endPath(argv[i + 1u]);
				else
					_cLine[arg] = argv[i + 1u]; // save this command line option for now, apply it later
			}
			else
				Log(LOG_WARNING) << "Unknown option: " << arg;
		}
	}
}

/**
 * Sets up the data-folder for data-files, the user-folder for saves, the
 * config-folder for Options, and the picture-folder for screenshots.
 */
void setFolders()
{
	_dataFolders = CrossPlatform::findDataFolders();

	if (_dirData.empty() == false)
		_dataFolders.insert(
						_dataFolders.begin(),
						_dirData);

	if (_dirUser.empty() == true)
	{
		const std::vector<std::string> userFolders (CrossPlatform::findUserFolders());

		if (_dirConfig.empty() == true)
			_dirConfig = CrossPlatform::findConfigFolder();

		for (std::vector<std::string>::const_reverse_iterator // look for an existing user-folder
				rit = userFolders.rbegin();
				rit != userFolders.rend();
				++rit)
		{
			if (CrossPlatform::folderExists(*rit) == true)
			{
				_dirUser = *rit;
				break;
			}
		}

		if (_dirUser.empty() == true) // set up a user-folder
		{
			for (std::vector<std::string>::const_iterator
					i = userFolders.begin();
					i != userFolders.end();
					++i)
			{
				if (CrossPlatform::createFolder(*i) == true)
				{
					_dirUser = *i;
					break;
				}
			}
		}
	}

	if (_dirUser.empty() == false)
	{
		if (_dirPic.empty() == true) // set up picture-folder
		{
			_dirPic = _dirUser + "pic\\";
			if (CrossPlatform::folderExists(_dirPic) == false)
				CrossPlatform::createFolder(_dirPic);
		}

		if (_dirConfig.empty() == true)
			_dirConfig = _dirUser;
	}
	else
		Log(LOG_FATAL) << "Buy a new computer.";
}

/**
 * Updates Options with the stuff in the config-file if it exists or the
 * command-line if it exists.
 */
void userOptions()
{
	if (CrossPlatform::folderExists(_dirConfig)) // Load Options in config-file or create one.
	{
		if (CrossPlatform::fileExists(_dirConfig + "options.cfg") == true)
			load();
		else
			save();
	}
	else // Create config-folder and save Options to file.
	{
		CrossPlatform::createFolder(_dirConfig);
		save();
	}

	// apply options that were set on the command-line
	// override defaults and stuff loaded in the config-file
//	if (!_cLine.empty())
	for (std::vector<OptionInfo>::const_iterator
			i = _info.begin();
			i != _info.end();
			++i)
	{
		i->load(_cLine);
	}
}

/**
 * Saves options to a YAML file.
 * @param file - reference a YAML file
 */
void save(const std::string& file)
{
	const std::string st (_dirConfig + file + ".cfg");
	std::ofstream ofstr (st.c_str());
	if (ofstr.fail() == true)
	{
		Log(LOG_WARNING) << "Failed to save " << file << ".cfg";
		return;
	}
	Log(LOG_INFO) << "Saving Options to " << file << ".cfg";

	try
	{
		YAML::Emitter output;
		YAML::Node
			doc,
			node;

		for (std::vector<OptionInfo>::const_iterator
				i = _info.begin();
				i != _info.end();
				++i)
		{
			i->save(node);
		}

		doc["options"]	= node;
		doc["rulesets"]	= rulesets;
		output << doc;

		ofstr << output.c_str();
	}
	catch (YAML::Exception& e)
	{
		Log(LOG_WARNING) << e.what();
	}

	ofstr.close();
}

/**
 * Loads options from a YAML file.
 * @param file - reference a YAML filename
 */
void load(const std::string& file)
{
	const std::string st (_dirConfig + file + ".cfg");
	try
	{
		const YAML::Node doc (YAML::LoadFile(st));
		for (std::vector<OptionInfo>::const_iterator
				i = _info.begin();
				i != _info.end();
				++i)
		{
			i->load(doc["options"]);
		}
		rulesets = doc["rulesets"].as<std::vector<std::string>>(rulesets);
	}
	catch (YAML::Exception& e)
	{
		Log(LOG_WARNING) << e.what();
	}
}

/**
 * Gets a list of possible data-folders.
 * @return, reference to a vector of paths
 */
const std::vector<std::string>& getDataFolders()
{
	return _dataFolders;
}

/**
 * Sets the data-folder from which resources are loaded.
 * @param folder - reference to full path
 */
void setDataFolder(const std::string& folder)
{
	_dirData = folder;
}

/**
 * Gets the data-folder from which resources are loaded.
 * @return, full path
 */
std::string getDataFolder()
{
	return _dirData;
}

/**
 * Gets the user-folder where saves are stored.
 * @return, full path
 */
std::string getUserFolder()
{
	return _dirUser;
}

/**
 * Gets the picture-folder where screenshots are stored.
 * @return, full path
 */
std::string getPictureFolder()
{
	return _dirPic;
}

/**
 * Gets the config-folder where Options are stored.
 * @note Usually the same as the user-folder.
 * @return, full path
 */
std::string getConfigFolder()
{
	return _dirConfig;
}

/**
 * Gets the list of all available OptionInfo.
 * @return, reference to a vector of options
 */
const std::vector<OptionInfo>& getOptionInfo()
{
	return _info;
}

/**
 * Saves display-settings temporarily in case player's monitor explodes.
 */
void backupDisplay()
{
	safeDisplayWidth		= displayWidth;
	safeDisplayHeight		= displayHeight;

	safeGeoscapeScale		= geoscapeScale;
	safeBattlescapeScale	= battlescapeScale;

	safeScaleFilter			= useScaleFilter;
	safeHQXFilter			= useHQXFilter;
	safeXBRZFilter			= useXBRZFilter;
	safeOpenGL				= useOpenGL;

	safeOpenGLShader		= openGLShader;
}

/**
 * Switches old/new display-options for temporarily testing a new display-setup.
 */
void switchDisplay()
{
	std::swap(displayWidth,		safeDisplayWidth);
	std::swap(displayHeight,	safeDisplayHeight);

	std::swap(geoscapeScale,	safeGeoscapeScale);
	std::swap(battlescapeScale,	safeBattlescapeScale);

	std::swap(useScaleFilter,	safeScaleFilter);
	std::swap(useHQXFilter,		safeHQXFilter);
	std::swap(useXBRZFilter,	safeXBRZFilter);
	std::swap(useOpenGL,		safeOpenGL);

	std::swap(openGLShader,		safeOpenGLShader);
}

/**
 * Resets the Options back to their defaults and backs up critical display
 * settings, also reinstates the global ruleset.
 */
void resetDefaults()
{
	for (std::vector<OptionInfo>::const_iterator
			i = _info.begin();
			i != _info.end();
			++i)
		i->reset();


	backupDisplay();

	rulesets.clear();
//	rulesets.push_back("Xcom1Ruleset");
}

/**
 * Sets up the Options by creating their OptionInfo metadata.
 */
void createOptions()
{
#ifdef DINGOO
	_info.push_back(OptionInfo("displayWidth",							&displayWidth,  Screen::ORIGINAL_WIDTH));
	_info.push_back(OptionInfo("displayHeight",							&displayHeight, Screen::ORIGINAL_HEIGHT));
	_info.push_back(OptionInfo("fullscreen",							&fullscreen, true));
	_info.push_back(OptionInfo("asyncBlit",								&asyncBlit, false));
	_info.push_back(OptionInfo("keyboardMode",							reinterpret_cast<int*>(&keyboardMode), KEYBOARD_OFF));
#else
	_info.push_back(OptionInfo("displayWidth",							&displayWidth,  Screen::ORIGINAL_WIDTH  * 2));
	_info.push_back(OptionInfo("displayHeight",							&displayHeight, Screen::ORIGINAL_HEIGHT * 2));
	_info.push_back(OptionInfo("fullscreen",							&fullscreen, false));
	_info.push_back(OptionInfo("asyncBlit",								&asyncBlit, true));
	_info.push_back(OptionInfo("keyboardMode",							reinterpret_cast<int*>(&keyboardMode), KEYBOARD_ON));
#endif

	_info.push_back(OptionInfo("engineLooper",							&engineLooper, "wilecoyote"));
	_info.push_back(OptionInfo("traceAI",								&traceAI, 0));
	_info.push_back(OptionInfo("verboseLogging",						&verboseLogging, false));
	_info.push_back(OptionInfo("stereoSound",							&stereoSound, true));
	_info.push_back(OptionInfo("audioSampleRate",						&audioSampleRate, 22050));
	_info.push_back(OptionInfo("audioBitDepth",							&audioBitDepth,      16));
	_info.push_back(OptionInfo("audioChunkSize",						&audioChunkSize,   2048));
	_info.push_back(OptionInfo("baseXResolution",						&baseXResolution,  Screen::ORIGINAL_WIDTH));
	_info.push_back(OptionInfo("baseYResolution",						&baseYResolution,  Screen::ORIGINAL_HEIGHT));
	_info.push_back(OptionInfo("baseXGeoscape",							&baseXGeoscape,    Screen::ORIGINAL_WIDTH));
	_info.push_back(OptionInfo("baseYGeoscape",							&baseYGeoscape,    Screen::ORIGINAL_HEIGHT));
	_info.push_back(OptionInfo("baseXBattlescape",						&baseXBattlescape, Screen::ORIGINAL_WIDTH));
	_info.push_back(OptionInfo("baseYBattlescape",						&baseYBattlescape, Screen::ORIGINAL_HEIGHT));
	_info.push_back(OptionInfo("geoscapeScale",							&geoscapeScale,    0));
	_info.push_back(OptionInfo("battlescapeScale",						&battlescapeScale, 0));
	_info.push_back(OptionInfo("useScaleFilter",						&useScaleFilter,     false));
	_info.push_back(OptionInfo("useHQXFilter",							&useHQXFilter,       false));
	_info.push_back(OptionInfo("useXBRZFilter",							&useXBRZFilter,      false));
	_info.push_back(OptionInfo("useOpenGL",								&useOpenGL,          false));
	_info.push_back(OptionInfo("checkOpenGLErrors",						&checkOpenGLErrors,  false));
	_info.push_back(OptionInfo("vSyncForOpenGL",						&vSyncForOpenGL,     true));
	_info.push_back(OptionInfo("useOpenGLSmoothing",					&useOpenGLSmoothing, true));
	_info.push_back(OptionInfo("openGLShader",							&openGLShader, "Shaders/Raw.OpenGL.shader"));
	_info.push_back(OptionInfo("debug",									&debug,   false));
	_info.push_back(OptionInfo("debugUi",								&debugUi, false));
	_info.push_back(OptionInfo("soundVolume",							&soundVolume, 2 * (MIX_MAX_VOLUME / 3)));
	_info.push_back(OptionInfo("musicVolume",							&musicVolume, 2 * (MIX_MAX_VOLUME / 3)));
	_info.push_back(OptionInfo("uiVolume",								&uiVolume,         MIX_MAX_VOLUME / 3));
	_info.push_back(OptionInfo("language",								&language, ""));
	_info.push_back(OptionInfo("battleScrollSpeed",						&battleScrollSpeed, 8));
	_info.push_back(OptionInfo("battleEdgeScroll",						reinterpret_cast<int*>(&battleEdgeScroll), MAP_SCROLL_AUTO));
	_info.push_back(OptionInfo("battleDragScrollButton",				&battleDragScrollButton, SDL_BUTTON_MIDDLE));
	_info.push_back(OptionInfo("dragScrollTimeTolerance",				&dragScrollTimeTolerance, 300)); // milliSecond
	_info.push_back(OptionInfo("dragScrollPixelTolerance",				&dragScrollPixelTolerance, 10)); // count of pixels
	_info.push_back(OptionInfo("battleFireSpeed",						&battleFireSpeed,  12));
	_info.push_back(OptionInfo("battleThrowSpeed",						&battleThrowSpeed,  6)); // NOTE: Not represented in Options state.
	_info.push_back(OptionInfo("battleXcomSpeed",						&battleXcomSpeed,  30));
	_info.push_back(OptionInfo("battleAlienSpeed",						&battleAlienSpeed, 30));
	_info.push_back(OptionInfo("battlePreviewPath",						reinterpret_cast<int*>(&battlePreviewPath), PATH_NONE)); // requires double-click to confirm moves
	_info.push_back(OptionInfo("fpsCounter",							&fpsCounter,       false));
	_info.push_back(OptionInfo("globeDetail",							&globeDetail,      true));
	_info.push_back(OptionInfo("globeRadarLines",						&globeRadarLines,  true));
	_info.push_back(OptionInfo("globeFlightPaths",						&globeFlightPaths, true));
	_info.push_back(OptionInfo("globeAllRadarsOnBaseBuild",				&globeAllRadarsOnBaseBuild, true));
	_info.push_back(OptionInfo("pauseMode",								&pauseMode, 0));
	_info.push_back(OptionInfo("battleNotifyDeath",						&battleNotifyDeath, false));
	_info.push_back(OptionInfo("showFundsOnGeoscape",					&showFundsOnGeoscape, false));
	_info.push_back(OptionInfo("allowResize",							&allowResize, false));
	_info.push_back(OptionInfo("windowedModePositionX",					&windowedModePositionX, -1));
	_info.push_back(OptionInfo("windowedModePositionY",					&windowedModePositionY, -1));
	_info.push_back(OptionInfo("borderless",							&borderless, false));
	_info.push_back(OptionInfo("captureMouse",							reinterpret_cast<bool*>(&captureMouse), false));
	_info.push_back(OptionInfo("battleTooltips",						&battleTooltips, true));
	_info.push_back(OptionInfo("keepAspectRatio",						&keepAspectRatio, true));
	_info.push_back(OptionInfo("nonSquarePixelRatio",					&nonSquarePixelRatio, false));
	_info.push_back(OptionInfo("cursorInBlackBandsInFullscreen",		&cursorInBlackBandsInFullscreen,       false));
	_info.push_back(OptionInfo("cursorInBlackBandsInWindow",			&cursorInBlackBandsInWindow,           true));
	_info.push_back(OptionInfo("cursorInBlackBandsInBorderlessWindow",	&cursorInBlackBandsInBorderlessWindow, false));
	_info.push_back(OptionInfo("saveOrder",								reinterpret_cast<int*>(&saveOrder), SORT_DATE_DESC));
	_info.push_back(OptionInfo("geoClockSpeed",							&geoClockSpeed,  80));
	_info.push_back(OptionInfo("dogfightSpeed",							&dogfightSpeed,  30));
	_info.push_back(OptionInfo("geoScrollSpeed",						&geoScrollSpeed, 20));
	_info.push_back(OptionInfo("geoDragScrollButton",					&geoDragScrollButton, SDL_BUTTON_MIDDLE));
	_info.push_back(OptionInfo("preferredMusic",						reinterpret_cast<int*>(&preferredMusic), MUSIC_AUTO));
	_info.push_back(OptionInfo("preferredSound",						reinterpret_cast<int*>(&preferredSound), SOUND_AUTO));
	_info.push_back(OptionInfo("musicAlwaysLoop",						&musicAlwaysLoop, false));

	// advanced options
	_info.push_back(OptionInfo("playIntro",								&playIntro,      true, "STR_PLAYINTRO",          "STR_GENERAL"));
	_info.push_back(OptionInfo("autosave",								&autosave,       true, "STR_AUTOSAVE",           "STR_GENERAL"));
	_info.push_back(OptionInfo("autosaveFrequency",						&autosaveFrequency, 5, "STR_AUTOSAVE_FREQUENCY", "STR_GENERAL"));
	_info.push_back(OptionInfo("reSeedOnLoad",							&reSeedOnLoad,  false, "STR_NEWSEEDONLOAD",      "STR_GENERAL"));
	_info.push_back(OptionInfo("mousewheelSpeed",						&mousewheelSpeed,   3, "STR_MOUSEWHEEL_SPEED",   "STR_GENERAL"));

// this should probably be any small screen touch-device, i don't know the defines for all of them so i'll cover android and IOS as i imagine they're more common
#ifdef __ANDROID_API__
	_info.push_back(OptionInfo("maximizeInfoScreens",					&maximizeInfoScreens,  true, "STR_MAXIMIZE_INFO_SCREENS", "STR_GENERAL"));
#elif __APPLE__
	// todo: ask grussel how badly i messed this up.
	#include "TargetConditionals.h"
	#if TARGET_IPHONE_SIMULATOR || TARGET_OS_IPHONE
	_info.push_back(OptionInfo("maximizeInfoScreens",					&maximizeInfoScreens,  true, "STR_MAXIMIZE_INFO_SCREENS", "STR_GENERAL"));
	#else
	_info.push_back(OptionInfo("maximizeInfoScreens",					&maximizeInfoScreens, false, "STR_MAXIMIZE_INFO_SCREENS", "STR_GENERAL"));
	#endif
#else
	_info.push_back(OptionInfo("maximizeInfoScreens",					&maximizeInfoScreens, false, "STR_MAXIMIZE_INFO_SCREENS", "STR_GENERAL"));
#endif

	// true drags away from the cursor, false drags towards (like a grab)
	_info.push_back(OptionInfo("geoDragScrollInvert",					&geoDragScrollInvert,   false, "STR_DRAGSCROLLINVERT",      "STR_GEOSCAPE"));
	_info.push_back(OptionInfo("aggressiveRetaliation",					&aggressiveRetaliation, false, "STR_AGGRESSIVERETALIATION", "STR_GEOSCAPE"));
	_info.push_back(OptionInfo("allowBuildingQueue",					&allowBuildingQueue,    false, "STR_ALLOWBUILDINGQUEUE",    "STR_GEOSCAPE"));
	_info.push_back(OptionInfo("craftLaunchAlways",						&craftLaunchAlways,     false, "STR_CRAFTLAUNCHALWAYS",     "STR_GEOSCAPE"));
	_info.push_back(OptionInfo("canSellLiveAliens",						&canSellLiveAliens,     false, "STR_CANSELLLIVEALIENS",     "STR_GEOSCAPE"));
	_info.push_back(OptionInfo("globeSeasons",							&globeSeasons,          false, "STR_GLOBESEASONS",          "STR_GEOSCAPE"));
	_info.push_back(OptionInfo("psiStrengthEval",						&psiStrengthEval,       false, "STR_PSISTRENGTHEVAL",       "STR_GEOSCAPE"));
	_info.push_back(OptionInfo("grantCorpses",							&grantCorpses,           true, "STR_GRANTCORPSES",          "STR_GEOSCAPE"));
	_info.push_back(OptionInfo("fieldPromotions",						&fieldPromotions,       false, "STR_FIELDPROMOTIONS",       "STR_GEOSCAPE"));

	// true drags away from the cursor, false drags towards (like a grab)
	_info.push_back(OptionInfo("battleDragScrollInvert",				&battleDragScrollInvert,         false, "STR_DRAGSCROLLINVERT",                   "STR_BATTLESCAPE"));
//	_info.push_back(OptionInfo("sneakyAI",								&sneakyAI,                       false, "STR_SNEAKYAI",                           "STR_BATTLESCAPE"));
	_info.push_back(OptionInfo("battleUFOExtenderAccuracy",				&battleUFOExtenderAccuracy,      false, "STR_BATTLEUFOEXTENDERACCURACY",          "STR_BATTLESCAPE"));
	_info.push_back(OptionInfo("battleHairBleach",						&battleHairBleach,                true, "STR_BATTLEHAIRBLEACH",                   "STR_BATTLESCAPE"));
	_info.push_back(OptionInfo("battleInstantGrenade",					&battleInstantGrenade,           false, "STR_BATTLEINSTANTGRENADE",               "STR_BATTLESCAPE"));
	_info.push_back(OptionInfo("includePrimeStateInSavedLayout",		&includePrimeStateInSavedLayout, false, "STR_INCLUDE_PRIMESTATE_IN_SAVED_LAYOUT", "STR_BATTLESCAPE"));
	_info.push_back(OptionInfo("battleExplosionHeight",					&battleExplosionHeight,              0, "STR_BATTLEEXPLOSIONHEIGHT",              "STR_BATTLESCAPE"));
//	_info.push_back(OptionInfo("battleSmoothCamera",					&battleSmoothCamera,             false, "STR_BATTLESMOOTHCAMERA",                 "STR_BATTLESCAPE"));
	_info.push_back(OptionInfo("battleWeaponSelfDestruction",			&battleWeaponSelfDestruction,    false, "STR_WEAPONSELFDESTRUCTION",              "STR_BATTLESCAPE"));
	_info.push_back(OptionInfo("battleStrafe",							&battleStrafe,                   false, "STR_STRAFE",                             "STR_BATTLESCAPE"));
	_info.push_back(OptionInfo("battleTFTDDamage",						&battleTFTDDamage,               false, "STR_TFTDDAMAGE",                         "STR_BATTLESCAPE"));
	_info.push_back(OptionInfo("battleRangeBasedAccuracy",				&battleRangeBasedAccuracy,       false, "STR_BATTLERANGEBASEDACCURACY",           "STR_BATTLESCAPE"));
	_info.push_back(OptionInfo("battleAlienPanicMessages",				&battleAlienPanicMessages,        true, "STR_ALIENPANICMESSAGES",                 "STR_BATTLESCAPE"));
	_info.push_back(OptionInfo("battleAlienBleeding",					&battleAlienBleeding,            false, "STR_ALIENBLEEDING",                      "STR_BATTLESCAPE"));

	// controls
	_info.push_back(OptionInfo("keyOk",									&keyOk,						SDLK_RETURN,	"STR_OK",			"STR_GENERAL"));
	_info.push_back(OptionInfo("keyOkKeypad",							&keyOkKeypad,				SDLK_KP_ENTER,	"STR_OK_KEYPAD",	"STR_GENERAL"));
	_info.push_back(OptionInfo("keyCancel",								&keyCancel,					SDLK_ESCAPE,	"STR_CANCEL",		"STR_GENERAL"));
	_info.push_back(OptionInfo("keyScreenshot",							&keyScreenshot,				SDLK_F12,		"STR_SCREENSHOT",	"STR_GENERAL"));
	_info.push_back(OptionInfo("keyFps",								&keyFps,					SDLK_F7,		"STR_FPS_COUNTER",	"STR_GENERAL"));
	_info.push_back(OptionInfo("keyQuickSave",							&keyQuickSave,				SDLK_F6,		"STR_QUICK_SAVE",	"STR_GENERAL"));
	_info.push_back(OptionInfo("keyQuickLoad",							&keyQuickLoad,				SDLK_F5,		"STR_QUICK_LOAD",	"STR_GENERAL"));

	_info.push_back(OptionInfo("keyGeoLeft",							&keyGeoLeft,				SDLK_LEFT,		"STR_ROTATE_LEFT",				"STR_GEOSCAPE"));
	_info.push_back(OptionInfo("keyGeoRight",							&keyGeoRight,				SDLK_RIGHT,		"STR_ROTATE_RIGHT",				"STR_GEOSCAPE"));
	_info.push_back(OptionInfo("keyGeoUp",								&keyGeoUp,					SDLK_UP,		"STR_ROTATE_UP",				"STR_GEOSCAPE"));
	_info.push_back(OptionInfo("keyGeoDown",							&keyGeoDown,				SDLK_DOWN,		"STR_ROTATE_DOWN",				"STR_GEOSCAPE"));
	_info.push_back(OptionInfo("keyGeoZoomIn",							&keyGeoZoomIn,				SDLK_PLUS,		"STR_ZOOM_IN",					"STR_GEOSCAPE"));
	_info.push_back(OptionInfo("keyGeoZoomOut",							&keyGeoZoomOut,				SDLK_MINUS,		"STR_ZOOM_OUT",					"STR_GEOSCAPE"));
	_info.push_back(OptionInfo("keyGeoSpeed1",							&keyGeoSpeed1,				SDLK_1,			"STR_5_SECONDS",				"STR_GEOSCAPE"));
	_info.push_back(OptionInfo("keyGeoSpeed2",							&keyGeoSpeed2,				SDLK_2,			"STR_1_MINUTE",					"STR_GEOSCAPE"));
	_info.push_back(OptionInfo("keyGeoSpeed3",							&keyGeoSpeed3,				SDLK_3,			"STR_5_MINUTES",				"STR_GEOSCAPE"));
	_info.push_back(OptionInfo("keyGeoSpeed4",							&keyGeoSpeed4,				SDLK_4,			"STR_30_MINUTES",				"STR_GEOSCAPE"));
	_info.push_back(OptionInfo("keyGeoSpeed5",							&keyGeoSpeed5,				SDLK_5,			"STR_1_HOUR",					"STR_GEOSCAPE"));
	_info.push_back(OptionInfo("keyGeoSpeed6",							&keyGeoSpeed6,				SDLK_6,			"STR_1_DAY",					"STR_GEOSCAPE"));
	_info.push_back(OptionInfo("keyGeoIntercept",						&keyGeoIntercept,			SDLK_i,			"STR_INTERCEPT",				"STR_GEOSCAPE"));
	_info.push_back(OptionInfo("keyGeoBases",							&keyGeoBases,				SDLK_b,			"STR_BASES",					"STR_GEOSCAPE"));
	_info.push_back(OptionInfo("keyGeoGraphs",							&keyGeoGraphs,				SDLK_g,			"STR_GRAPHS",					"STR_GEOSCAPE"));
	_info.push_back(OptionInfo("keyGeoUfopedia",						&keyGeoUfopedia,			SDLK_u,			"STR_UFOPAEDIA_UC",				"STR_GEOSCAPE"));
	_info.push_back(OptionInfo("keyGeoOptions",							&keyGeoOptions,				SDLK_ESCAPE,	"STR_OPTIONS_UC",				"STR_GEOSCAPE"));
	_info.push_back(OptionInfo("keyGeoFunding",							&keyGeoFunding,				SDLK_f,			"STR_FUNDING_UC",				"STR_GEOSCAPE"));
	_info.push_back(OptionInfo("keyGeoToggleDetail",					&keyGeoToggleDetail,		SDLK_TAB,		"STR_TOGGLE_COUNTRY_DETAIL",	"STR_GEOSCAPE"));
	_info.push_back(OptionInfo("keyGeoToggleRadar",						&keyGeoToggleRadar,			SDLK_r,			"STR_TOGGLE_RADAR_RANGES",		"STR_GEOSCAPE"));
	_info.push_back(OptionInfo("keyBaseSelect1",						&keyBaseSelect1,			SDLK_1,			"STR_SELECT_BASE_1",			"STR_GEOSCAPE"));
	_info.push_back(OptionInfo("keyBaseSelect2",						&keyBaseSelect2,			SDLK_2,			"STR_SELECT_BASE_2",			"STR_GEOSCAPE"));
	_info.push_back(OptionInfo("keyBaseSelect3",						&keyBaseSelect3,			SDLK_3,			"STR_SELECT_BASE_3",			"STR_GEOSCAPE"));
	_info.push_back(OptionInfo("keyBaseSelect4",						&keyBaseSelect4,			SDLK_4,			"STR_SELECT_BASE_4",			"STR_GEOSCAPE"));
	_info.push_back(OptionInfo("keyBaseSelect5",						&keyBaseSelect5,			SDLK_5,			"STR_SELECT_BASE_5",			"STR_GEOSCAPE"));
	_info.push_back(OptionInfo("keyBaseSelect6",						&keyBaseSelect6,			SDLK_6,			"STR_SELECT_BASE_6",			"STR_GEOSCAPE"));
	_info.push_back(OptionInfo("keyBaseSelect7",						&keyBaseSelect7,			SDLK_7,			"STR_SELECT_BASE_7",			"STR_GEOSCAPE"));
	_info.push_back(OptionInfo("keyBaseSelect8",						&keyBaseSelect8,			SDLK_8,			"STR_SELECT_BASE_8",			"STR_GEOSCAPE"));

	_info.push_back(OptionInfo("keyBattleLeft",							&keyBattleLeft,				SDLK_LEFT,		"STR_SCROLL_LEFT",							"STR_BATTLESCAPE"));
	_info.push_back(OptionInfo("keyBattleRight",						&keyBattleRight,			SDLK_RIGHT,		"STR_SCROLL_RIGHT",							"STR_BATTLESCAPE"));
	_info.push_back(OptionInfo("keyBattleUp",							&keyBattleUp,				SDLK_UP,		"STR_SCROLL_UP",							"STR_BATTLESCAPE"));
	_info.push_back(OptionInfo("keyBattleDown",							&keyBattleDown,				SDLK_DOWN,		"STR_SCROLL_DOWN",							"STR_BATTLESCAPE"));
	_info.push_back(OptionInfo("keyBattleLevelUp",						&keyBattleLevelUp,			SDLK_PAGEUP,	"STR_VIEW_LEVEL_ABOVE",						"STR_BATTLESCAPE"));
	_info.push_back(OptionInfo("keyBattleLevelDown",					&keyBattleLevelDown,		SDLK_PAGEDOWN,	"STR_VIEW_LEVEL_BELOW",						"STR_BATTLESCAPE"));
	_info.push_back(OptionInfo("keyBattleCenterUnit",					&keyBattleCenterUnit,		SDLK_HOME,		"STR_CENTER_SELECTED_UNIT",					"STR_BATTLESCAPE"));
	_info.push_back(OptionInfo("keyBattlePrevUnit",						&keyBattlePrevUnit,			SDLK_LSHIFT,	"STR_PREVIOUS_UNIT",						"STR_BATTLESCAPE"));
	_info.push_back(OptionInfo("keyBattleNextUnit",						&keyBattleNextUnit,			SDLK_TAB,		"STR_NEXT_UNIT",							"STR_BATTLESCAPE"));
	_info.push_back(OptionInfo("keyBattleDeselectUnit",					&keyBattleDeselectUnit,		SDLK_BACKSLASH,	"STR_DESELECT_UNIT",						"STR_BATTLESCAPE"));
	_info.push_back(OptionInfo("keyBattleUseLeftHand",					&keyBattleUseLeftHand,		SDLK_q,			"STR_USE_LEFT_HAND",						"STR_BATTLESCAPE"));
	_info.push_back(OptionInfo("keyBattleUseRightHand",					&keyBattleUseRightHand,		SDLK_e,			"STR_USE_RIGHT_HAND",						"STR_BATTLESCAPE"));
	_info.push_back(OptionInfo("keyBattleInventory",					&keyBattleInventory,		SDLK_i,			"STR_INVENTORY",							"STR_BATTLESCAPE"));
	_info.push_back(OptionInfo("keyBattleMap",							&keyBattleMap,				SDLK_m,			"STR_MINIMAP",								"STR_BATTLESCAPE"));
	_info.push_back(OptionInfo("keyBattleOptions",						&keyBattleOptions,			SDLK_ESCAPE,	"STR_OPTIONS",								"STR_BATTLESCAPE"));
	_info.push_back(OptionInfo("keyBattleEndTurn",						&keyBattleEndTurn,			SDLK_BACKSPACE,	"STR_END_TURN",								"STR_BATTLESCAPE"));
	_info.push_back(OptionInfo("keyBattleAbort",						&keyBattleAbort,			SDLK_a,			"STR_ABORT_MISSION",						"STR_BATTLESCAPE"));
	_info.push_back(OptionInfo("keyBattleStats",						&keyBattleStats,			SDLK_s,			"STR_UNIT_STATS",							"STR_BATTLESCAPE"));
	_info.push_back(OptionInfo("keyBattleKneel",						&keyBattleKneel,			SDLK_k,			"STR_KNEEL",								"STR_BATTLESCAPE"));
	_info.push_back(OptionInfo("keyBattleReload",						&keyBattleReload,			SDLK_r,			"STR_RELOAD",								"STR_BATTLESCAPE"));
	_info.push_back(OptionInfo("keyBattlePersonalLighting",				&keyBattlePersonalLighting,	SDLK_l,			"STR_TOGGLE_PERSONAL_LIGHTING",				"STR_BATTLESCAPE"));
	_info.push_back(OptionInfo("keyBattleReserveNone",					&keyBattleReserveNone,		SDLK_F1,		"STR_DONT_RESERVE_TIME_UNITS",				"STR_BATTLESCAPE"));
	_info.push_back(OptionInfo("keyBattleReserveSnap",					&keyBattleReserveSnap,		SDLK_F2,		"STR_RESERVE_TIME_UNITS_FOR_SNAP_SHOT",		"STR_BATTLESCAPE"));
	_info.push_back(OptionInfo("keyBattleReserveAimed",					&keyBattleReserveAimed,		SDLK_F3,		"STR_RESERVE_TIME_UNITS_FOR_AIMED_SHOT",	"STR_BATTLESCAPE"));
	_info.push_back(OptionInfo("keyBattleReserveAuto",					&keyBattleReserveAuto,		SDLK_F4,		"STR_RESERVE_TIME_UNITS_FOR_AUTO_SHOT",		"STR_BATTLESCAPE"));
	_info.push_back(OptionInfo("keyBattleReserveKneel",					&keyBattleReserveKneel,		SDLK_j,			"STR_RESERVE_TIME_UNITS_FOR_KNEEL",			"STR_BATTLESCAPE"));
	_info.push_back(OptionInfo("keyBattleZeroTUs",						&keyBattleZeroTUs,			SDLK_DELETE,	"STR_EXPEND_ALL_TIME_UNITS",				"STR_BATTLESCAPE"));
	_info.push_back(OptionInfo("keyBattleCenterEnemy1",					&keyBattleCenterEnemy1,		SDLK_1,			"STR_CENTER_ON_ENEMY_1",					"STR_BATTLESCAPE"));
	_info.push_back(OptionInfo("keyBattleCenterEnemy2",					&keyBattleCenterEnemy2,		SDLK_2,			"STR_CENTER_ON_ENEMY_2",					"STR_BATTLESCAPE"));
	_info.push_back(OptionInfo("keyBattleCenterEnemy3",					&keyBattleCenterEnemy3,		SDLK_3,			"STR_CENTER_ON_ENEMY_3",					"STR_BATTLESCAPE"));
	_info.push_back(OptionInfo("keyBattleCenterEnemy4",					&keyBattleCenterEnemy4,		SDLK_4,			"STR_CENTER_ON_ENEMY_4",					"STR_BATTLESCAPE"));
	_info.push_back(OptionInfo("keyBattleCenterEnemy5",					&keyBattleCenterEnemy5,		SDLK_5,			"STR_CENTER_ON_ENEMY_5",					"STR_BATTLESCAPE"));
	_info.push_back(OptionInfo("keyBattleCenterEnemy6",					&keyBattleCenterEnemy6,		SDLK_6,			"STR_CENTER_ON_ENEMY_6",					"STR_BATTLESCAPE"));
	_info.push_back(OptionInfo("keyBattleCenterEnemy7",					&keyBattleCenterEnemy7,		SDLK_7,			"STR_CENTER_ON_ENEMY_7",					"STR_BATTLESCAPE"));
	_info.push_back(OptionInfo("keyBattleCenterEnemy8",					&keyBattleCenterEnemy8,		SDLK_8,			"STR_CENTER_ON_ENEMY_8",					"STR_BATTLESCAPE"));
	_info.push_back(OptionInfo("keyBattleCenterEnemy9",					&keyBattleCenterEnemy9,		SDLK_9,			"STR_CENTER_ON_ENEMY_9",					"STR_BATTLESCAPE"));
	_info.push_back(OptionInfo("keyBattleCenterEnemy10",				&keyBattleCenterEnemy10,	SDLK_0,			"STR_CENTER_ON_ENEMY_10",					"STR_BATTLESCAPE"));
	_info.push_back(OptionInfo("keyBattleVoxelView",					&keyBattleVoxelView,		SDLK_F11,		"STR_SAVE_VOXEL_VIEW",						"STR_BATTLESCAPE"));
	_info.push_back(OptionInfo("keyBattleConsole",						&keyBattleConsole,			SDLK_o,			"STR_CONSOLE",								"STR_BATTLESCAPE"));
	_info.push_back(OptionInfo("keyBattlePivotCcw",						&keyBattlePivotCcw,			SDLK_COMMA,		"STR_PIVOT_CCW",							"STR_BATTLESCAPE"));
	_info.push_back(OptionInfo("keyBattlePivotCw",						&keyBattlePivotCw,			SDLK_PERIOD,	"STR_PIVOT_CW",								"STR_BATTLESCAPE"));
	_info.push_back(OptionInfo("keyInvClear",							&keyInvClear,				SDLK_x,			"STR_CLEAR_INVENTORY",						"STR_BATTLESCAPE"));

//	_info.push_back(OptionInfo("keyInvCreateTemplate",					&keyInvCreateTemplate,		SDLK_c,			"STR_CREATE_INVENTORY_TEMPLATE",			"STR_BATTLESCAPE"));
//	_info.push_back(OptionInfo("keyInvApplyTemplate",					&keyInvApplyTemplate,		SDLK_v,			"STR_APPLY_INVENTORY_TEMPLATE",				"STR_BATTLESCAPE"));


// hardcoded keys:
// BattlescapeState::handle() only if debug=TRUE in 'options.cfg'
//	- SDLK_F10, map layers by voxel (.png)
//	- SDLK_F9, ai dump
//	- SDLK_d, enable debug mode
//	- SDLK_v, reset tile visibility [doubles w/ apply inv template, above]
//	- SDLK_k, kill all aliens [doubles w/ kneel, above]
//	- SDLK_KP_PERIOD, zero-Tu
// Screen::handle()
//	- SDLK_F8, animation speed switch (3 position flag)
// GeoscapeState::GeoscapeState()
//	- SDLK_SPACE, geoscape hard-pause
//	- SDLK_d, enable debug mode
//	- SDLK_c, cycle country lines
//	- SDLK_a, delete living soldier-awards

#ifdef __MORPHOS__
	_info.push_back(OptionInfo("FPS",			&FPS,			15, "STR_FPS_LIMIT",			"STR_GENERAL"));
	_info.push_back(OptionInfo("FPSUnfocused",	&FPSUnfocused,	15, "STR_FPS_UNFOCUSED_LIMIT",	"STR_GENERAL"));
#else
	_info.push_back(OptionInfo("FPS",			&FPS,			60, "STR_FPS_LIMIT",			"STR_GENERAL"));
	_info.push_back(OptionInfo("FPSUnfocused",	&FPSUnfocused,	30, "STR_FPS_UNFOCUSED_LIMIT",	"STR_GENERAL"));
#endif
}

} // Options

}
