# 0xC_kL

a liberal adaptation of [XCOM:Ufo Defense / UFO:Enemy Unknown][1]

[1]: https://en.wikipedia.org/wiki/UFO:_Enemy_Unknown

[0xC_kL][2] is an independent fork of [OpenXcom][3], which is an open-source clone of the popular 1994 [Microprose][4] videogame. It is licensed under the GPL and written in C++. It requires SDL 1.2, YAML-cpp 0.5.3, OpenGL and various 3rd party libraries.

[2]: https://github.com/kevL/0xC_kL
[3]: https://github.com/SupSuper/OpenXcom
[4]: https://en.wikipedia.org/wiki/MicroProse

Nothing here has any guaranteed compatibility in any way shape or form with OpenXcom or any of its modifications -- regard this as a simple personal library and a way for me to fool around with GitHub.

The sourcecode is substantially different than SupSuper's repository, which is largely an attempt to remain faithful to the original. This doesn't. They will not merge easily. I build on a Windows XP 32-bit system. I do not maintain cmakes etc. or particularly care about cross-platform capability. That said, I do not purposely take out the latter.

General rules are yours to discover. Tanks have a reverse gear, for example.

Plenty of credit needs to go to many persons both acknowledged and unacknowledged for [code][5] and [modified resources][6].

[5]: http://openxcom.org/
[6]: http://openxcom.org/forum/

[The X-COM wiki][7]

[7]: http://www.ufopaedia.org/index.php/Main_Page


1. Installation
================

Like OpenXcom, 0xC_kL requires a vanilla copy of the original XCOM resources.

If you have the Steam version you can find the XCOM game folder in

		Steam\steamapps\common\xcom ufo defense\XCOM

Note that modified versions of original XCOM's resources may cause bugs and crashes. Mods for OpenXcom may also cause bugs and crashes. Pretty much anything may cause bugs and crashes.

DISCLAIMER for ANY AND ALL RESPONSIBILITY for what you do with a computer and the material available in this repository.

Copy the original XCOM subfolders

		GEODATA
		GEOGRAPH
		MAPS
		ROUTES
		SOUND
		TERRAIN
		UFOGRAPH
		UFOINTRO
		UNITS

to 0xC_kL's Data folder:

		<0xC_kL>\data\


The source supports these operating systems:
- Windows

The source may or may not support these operating systems:
- !Windows


2. Development
===============

OpenXcom requires the following developer libraries:

		SDL 1.2
		SDL_mixer 1.2
		SDL_gfx 1.2
		SDL_image 1.2
		yaml-cpp 0.5.3

This repository contains no tools for building. It's source-code only that I compile on a Win32 XP Sp3 machine. While it still has defines for other systems they have not been updated and so are likely buggy. Also there are Windows API calls but these are blocked inside _WIN32 defines.

These are the GCC 6.2 compiler-arguments that I use regularly:

release config

		-m32 -Wno-deprecated-declarations -Wno-reorder -Wno-switch -Wno-parentheses -mwindows -O3 -Wall -s -DNDEBUG -DUNICODE -D_UNICODE -I/C/msys32/mingw32/include -include src/pch.h -std=c++11

debug config

		-m32 -fno-omit-frame-pointer -Wno-deprecated-declarations -Wno-reorder -Wno-switch -Wno-parentheses -mwindows -g -Wall -DUNICODE -D_DEBUG -D_UNICODE -D__NO_MUSIC -I/C/msys32/mingw32/include -std=c++11