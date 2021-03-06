#ifndef __OXC_PCH_H
#define __OXC_PCH_H

// kL: heads up!
//#pragma message("kL: Compiling PCH.H")
//#pragma message("...")
#pragma message("... jerking off ... please wait.")

// c++ headers
//
// NOTE: Something before <cmath>/<math.h> has defined M_PI along with
// _MATH_DEFINES_DEFINED, so M_PI_2 etc. does not get defined. Or maybe somebody
// merely jacked off all over their keyboard without using a keyboard-guard
// somewhere. Also,
// NOTE: That the math-constants are defined *outside* the header-guard in
// 'math.h' ....
#ifndef _USE_MATH_DEFINES
#	define _USE_MATH_DEFINES
#endif
// NOTE: Define _USE_MATH_DEFINES in the IDE's pre-processor def'ns. and
// Makefile's CXXFLAGS variable and be done w/ it.

#ifdef _MSC_VER
#	ifndef _CRT_SECURE_NO_WARNINGS
#		define _CRT_SECURE_NO_WARNINGS
#	endif

#	ifndef _SCL_SECURE_NO_WARNINGS
#		define _SCL_SECURE_NO_WARNINGS
#	endif
#endif

#include <algorithm>	// min/max
#include <cassert>		// asserts
#include <climits>		// integer limits
#include <cmath>		// math functions
#include <cstddef>		// defs, nullptr etc.
#include <cstdint>		// integer typedefs
#include <cstring>		// strings & arrays
#include <ctime>		// time, CrossPlatform
#include <cctype>		// character tests & conversions
#include <exception>	// exceptions
#include <fstream>		// file streams
#include <functional>	// functors
#include <iomanip>		// precision etc.
#include <limits>		// numeric_limits
#include <list>			// lists
#include <locale>		// localization, CrossPlatform
#include <map>			// maps
#include <queue>		// queues
#include <set>			// sets
#include <sstream>		// streams
#include <stack>		// lifo stacks
#include <string>		// strings
#include <utility>		// pairs
#include <vector>		// vectors

#if HAVE_ALLOCA_H
#	include <alloca.h>	// scalers ->
#endif
#include <assert.h>		// <cassert>
#if HAVE_CONFIG_H
#	include <config.h>
#endif
#include <stdint.h>
#include <stdlib.h>

#include <sys/stat.h>	// CrossPlatform
#include <sys/types.h>	// dirent.h
#include <dirent.h>		// note this is not "dirent.h"

#include "lodepng.h"


#ifdef _WIN32
#	ifndef NOMINMAX
#		define NOMINMAX
#	endif

#	ifndef WIN32_LEAN_AND_MEAN
#		define WIN32_LEAN_AND_MEAN
#	endif

#	include <windows.h>
#endif



/*
#include <algorithm>
//#include <assert.h>
#include <bitset>
#include <cassert>
#include <cctype>
#include <climits>
#include <cmath>
#include <cstdarg>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <deque>
#include <errno.h>
#include <exception>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
//#include <limits.h>
#include <limits>
#include <list>
#include <locale>
#include <map>
//#include <math.h>
//#include <memory.h>
#include <queue>
#include <set>
#include <sstream>
#include <stack>
//#include <stdarg.h>
//#include <stdint.h>
//#include <stdio.h>
#include <stdlib.h>
//#include <string.h>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
//#include <time.h>
#include <typeinfo>
#include <utility>
#include <vector>
//#include <wchar.h>
*/

/*
#ifdef _WIN32
#	ifndef NOMINMAX
#		define NOMINMAX
#	endif

#	define WIN32_LEAN_AND_MEAN
#	include <windows.h>
//#	include <shlobj.h>	// No, causes conflict OR
//#	include <shlwapi.h>	// this causes conflict ....
#	include <malloc.h>
#else */
/*
 #ifndef _WIN32
#	include <alloca.h>
//#	if HAVE_CONFIG_H
//#		include <config.h>
//#	endif
#	include <dirent.h>
#	include <pwd.h>
#	include <sys/param.h>
#	include <unistd.h>
#endif */


#ifdef __MORPHOS__
#	include <ppcinline/exec.h>
#endif


/* This is done in Zoom.cpp ->
 #if (_MSC_VER >= 1400) || (defined(__MINGW32__) && defined(__SSE2__))
#	ifndef __SSE2__
#		define __SSE2__
#	endif

// probably Visual Studio (or Intel C++ which should also work)
#	include <intrin.h>
#endif

 #ifdef __GNUC__
#	if (__i386__ || __x86_64__)
#		include <cpuid.h>
#	endif
#endif

#ifdef __SSE2__
	// for SSE2 intrinsics; see http://msdn.microsoft.com/en-us/library/has3d153%28v=vs.71%29.aspx
#	include <emmintrin.h>
#endif */


// library headers
//
#ifndef __NO_OPENGL
#	ifdef __APPLE__
#		include <OpenGL/gl.h>
#		include <OpenGL/glu.h>
#		include <OpenGL/glext.h>
#		include <GLUT/glut.h>
#	endif

#	include <SDL/SDL_opengl.h>
#endif

#include <SDL/SDL.h>
//#include <SDL/SDL_endian.h>
#include <SDL/SDL_events.h>
#include <SDL/SDL_gfxPrimitives.h>
#include <SDL/SDL_image.h>
//#include <SDL/SDL_keysym.h>
#include <SDL/SDL_mixer.h>
#include <SDL/SDL_stdinc.h> // for 'Uint8'(etc) types
//#include <SDL/SDL_syswm.h>
//#include <SDL/SDL_thread.h>
//#include <SDL/SDL_types.h>
#include <SDL/SDL_video.h>


#include <yaml-cpp/yaml.h>



// 0xC ./Engine includes
// subdir /Adlib not included (is disabled).
#include "./Engine/Scalers/common.h"
#include "./Engine/Scalers/config.h"
#include "./Engine/Scalers/hqx.h"
#include "./Engine/Scalers/scale2x.h"
#include "./Engine/Scalers/scale3x.h"
#include "./Engine/Scalers/scalebit.h"
#include "./Engine/Scalers/xbrz.h"

#include "./Engine/Action.h"
//#include "./Engine/AdlibMusic.h"			// is disabled.
#include "./Engine/CatFile.h"
#include "./Engine/CrossPlatform.h"
//#include "./Engine/DosFont.h"				// handled via Font.h
#include "./Engine/Exception.h"
#include "./Engine/FastLineClip.h"
#include "./Engine/FlcPlayer.h"
#include "./Engine/Font.h"
//#include "./Engine/GMCat.h"				// is disabled.
#include "./Engine/Game.h"
//#include "./Engine/GraphSubset.h"			// handled via ShaderDrawHelper.h
#include "./Engine/InteractiveSurface.h"
#include "./Engine/Language.h"
#include "./Engine/LanguagePlurality.h"
#include "./Engine/LocalizedText.h"
#include "./Engine/Logger.h"				// For debug assistance.
#include "./Engine/Music.h"
#include "./Engine/OpenGL.h"
#include "./Engine/OptionInfo.h"
#include "./Engine/Options.h"
//#include "./Engine/Options.inc.h"			// handled via Options.h
#include "./Engine/Palette.h"
#include "./Engine/RNG.h"
#include "./Engine/Screen.h"
//#include "./Engine/ShaderDraw.h"			// handled via ShaderMove.h, ShaderRepeat.h, Surface.h
//#include "./Engine/ShaderDrawHelper.h"	// handled via ShaderDraw.h
#include "./Engine/ShaderMove.h"
#include "./Engine/ShaderRepeat.h"
#include "./Engine/Sound.h"
#include "./Engine/SoundSet.h"
#include "./Engine/State.h"
#include "./Engine/Surface.h"
#include "./Engine/SurfaceSet.h"
#include "./Engine/Timer.h"
#include "./Engine/Zoom.h"


#endif
