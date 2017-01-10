################
#    0xC_kL    #
################

0xC_kL is an independent fork of an open-source clone of the popular
UFO:Enemy_Unknown aka XCOM:UFO_Defense videogame by Microprose(1994). It is
licensed under the GPL and written in C++. It requires SDL 1.2, YAML-cpp 0.5.3,
OpenGL and various 3rd party libraries.

SupSuper's originating repository:
https://github.com/SupSuper/OpenXcom

Their website and forum:
http://openxcom.org/
http://openxcom.org/forum/

The XCOM wiki:
http://www.ufopaedia.org/index.php/Main_Page


1. Installation
================

Like OpenXcom, 0xC_kL requires a vanilla copy of the original XCOM resources.

If you have the Steam version you can find the XCOM game folder in

	Steam\steamapps\common\xcom ufo defense\XCOM

Or search your system for "tactical.exe" and that should get to the general
vicinity.

Note that modified versions of orginal XCOM's resources may cause bugs and
crashes. Mods for OpenXcom may also cause bugs and crashes. Pretty much anything
may cause bugs and crashes.

0xC_kL DISCLAIMS ALL RESPONSIBILITY for what you do with the information and
material available in this repository.

Copy the original XCOM subfolders
- GEODATA
- GEOGRAPH
- MAPS
- ROUTES
- SOUND
- TERRAIN
- UFOGRAPH
- UFOINTRO
- UNITS
to 0xC_kL's Data folder:

	<0xC_kL>\data\


The source supports these operating systems:
- Windows

The source may or may not support these operating systems:
- !Windows


2. Development
===============

OpenXcom requires the following developer libraries:

- SDL (libsdl1.2):
http://www.libsdl.org
- SDL_mixer (libsdl-mixer1.2):
http://www.libsdl.org/projects/SDL_mixer/
- SDL_gfx (libsdl-gfx1.2), version 2.0.22 or later:
http://www.ferzkopp.net/joomla/content/view/19/14/
- SDL_image (libsdl-image1.2):
http://www.libsdl.org/projects/SDL_image/
- yaml-cpp, version 0.5.3 or later:
http://code.google.com/p/yaml-cpp/

This repository contains no tools for building. It's source-code only that I
compile on a Win32 XP Sp3 machine. While it still has defines for other systems
they have not been updated and so are likely buggy. Also there are Windows API
calls but these are blocked inside _WIN32 defines.

These are the GCC 6.2 compiler-arguments that I use regularly:

- release config
-m32 -Wno-deprecated-declarations -Wno-reorder -Wno-switch -Wno-parentheses -mwindows -O3 -Wall -s -DNDEBUG -DUNICODE -D_UNICODE -I/C/msys32/mingw32/include -include src/pch.h -std=c++11

- debug config
-m32 -fno-omit-frame-pointer -Wno-deprecated-declarations -Wno-reorder -Wno-switch -Wno-parentheses -mwindows -g -Wall -DUNICODE -D_DEBUG -D_UNICODE -D__NO_MUSIC -I/C/msys32/mingw32/include -std=c++11
