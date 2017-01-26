# 0xC_kL

a liberal adaptation of [XCOM:Ufo Defense / UFO:Enemy Unknown][1]

[1]: https://en.wikipedia.org/wiki/UFO:_Enemy_Unknown

[0xC_kL][2] is an independent fork of [OpenXcom][3], which is an open-source clone of the popular 1994 [Microprose][4] videogame. It is licensed under the GPL and written in C++. It requires SDL 1.2, YAML-cpp 0.5.3, OpenGL and various 3rd party libraries.

[2]: https://github.com/kevL/0xC_kL
[3]: https://github.com/SupSuper/OpenXcom
[4]: https://en.wikipedia.org/wiki/MicroProse

Nothing here has any guaranteed compatibility in any way shape or form with OpenXcom or any of its modifications -- regard this as a public personal library -- a way for me to fool around with GitHub.

The sourcecode is substantially different than SupSuper's repository, which is largely an attempt to remain faithful to the original. This doesn't. The two will not merge easily. I build on a Windows 7 64-bit system. I do not maintain CMakes etc. or particularly care about cross-platform compatibility.

General rules are yours to discover. Tanks have a reverse gear for example.

Plenty of credit needs to go to many persons both acknowledged and unacknowledged for [code][5] and [modified resources][6].

[5]: http://openxcom.org/
[6]: http://openxcom.org/forum/

[The X-COM wiki][7] -- details have changed but the essence is still here.

[7]: http://www.ufopaedia.org/index.php/Main_Page

Obligatory DISCLAIMER for ANY AND ALL RESPONSIBILITY for what you do with a computer and the material available in this repository.


### Installation

0xC_kL requires a vanilla copy of the original XCOM resources.

Modified versions of original XCOM's resources may cause bugs and crashes. Unmodified versions of original XCOM's resources may cause bugs and crashes. Mods for OpenXcom may also cause bugs and crashes. Pretty much anything may cause bugs and crashes. The program is very stable when I play it.

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


The code supports these operating systems:
- Windows

The code may or may not support these operating systems:
- !Windows


### Development

0xC_kL requires the following developer libraries:

		SDL 1.2
		SDL_mixer 1.2
		SDL_gfx 1.2
		SDL_image 1.2
		yaml-cpp 0.5.3

This repository contains no tools for building. It's source-code that I compile on a Windows 7 Sp1 64-bit machine using [MinGW-w64][8] + [MSYS2][9]. While it still has defines for other systems they have not been updated and so are likely buggy, but could be resolved by investigating the vanilla [OpenXcom][10] repository. Also there are Windows API calls but these are blocked inside _WIN32 defines -- there's a call to IrfanView for viewing battlefield LoFTs IG, and if you want that I trust that you can find it and deal with it your own way.

[8]: https://sourceforge.net/projects/mingw-w64/
[9]: https://msys2.github.io/
[10]: https://github.com/SupSuper/OpenXcom

These are the GCC 6.3 compiler-arguments that I use regularly:

release config

		-m64 -Wno-deprecated-declarations -Wno-reorder -Wno-switch -Wno-parentheses
		-mwindows -O3 -Wall -s -DNDEBUG -DUNICODE -D_UNICODE
		-I/C/msys64/mingw64/include -include src/pch.h -std=c++11

debug config

		-m64 -fno-omit-frame-pointer -Wno-deprecated-declarations -Wno-reorder
		-Wno-switch -Wno-parentheses -mwindows -g -Wall -DUNICODE -D_DEBUG -D_UNICODE
		-D__NO_MUSIC -I/C/msys64/mingw64/include -std=c++11