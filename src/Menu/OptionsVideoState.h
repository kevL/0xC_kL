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

#ifndef OPENXCOM_OPTIONSVIDEOSTATE_H
#define OPENXCOM_OPTIONSVIDEOSTATE_H

//#include <SDL/SDL.h>

#include "OptionsBaseState.h"

#include "../Engine/State.h"


namespace OpenXcom
{

enum DisplayMode
{
	FULLSCREEN,				// 0
	WINDOWED_STATIC,		// 1
	WINDOWED_BORDERLESS,	// 2
	WINDOWED_RESIZEABLE		// 3
};

/*
enum DisplayFilter
{
	FILTER_SHADER,	// 0
	FILTER_SCALE,	// 1
	FILTER_HQX,		// 2
	FILTER_XBRZ		// 3
					// 4+ OpenGL shaders ...
};
*/


class ArrowButton;
class ComboBox;
class InteractiveSurface;
class Text;
//class TextButton;
class TextEdit;
class ToggleTextButton;


/**
 * Screen that lets the user configure various Video options.
 */
class OptionsVideoState
	:
		public OptionsBaseState
{

private:
	static const std::string
		GL_EXT,
		GL_FOLDER,
		GL_STRING;

	int
		_resQuantity,
		_resCurrent;

	std::vector<std::string>
		_langs,
		_filters;
	std::vector<std::wstring> _gameRes;

	ArrowButton
		* _btnDisplayResolutionUp,
		* _btnDisplayResolutionDown;
	ComboBox
		* _cbxLanguage,
		* _cbxFilter,
		* _cbxDisplayMode,
		* _cbxGeoScale,
		* _cbxBattleScale;
	Text
		* _txtDisplayResolution,
		* _txtDisplayX,
		* _txtLanguage,
		* _txtFilter,
		* _txtGeoScale,
		* _txtBattleScale,
		* _txtMode,
		* _txtOptions;
//	TextButton
//		* _displayMode,
//		* _btnWindowed,
//		* _btnFullscreen,
//		* _btnBorderless;
	TextEdit
		* _edtDisplayWidth,
		* _edtDisplayHeight;
	ToggleTextButton
		* _btnLetterbox,
		* _btnLockMouse;

	InteractiveSurface* _displaySurface;
	SDL_Rect** _res;


	/// Capitalizes each word in a string.
	std::string ucWords(std::string st);

	/// Updates the display resolution based on the selection.
	void updateDisplayResolution();


	public:
		/// Creates an OptionsVideo state.
		explicit OptionsVideoState(OptionsOrigin origin);
		/// Cleans up the OptionsVideo state.
		~OptionsVideoState();

		/// Handler for clicking the Next Resolution button.
		void btnDisplayResolutionUpClick(Action* action);
		/// Handler for clicking the Previous Resolution button.
		void btnDisplayResolutionDownClick(Action* action);
		/// Handler for changing the Display Width text.
		void txtDisplayWidthChange(Action* action);
		/// Handler for changing the Display Height text.
		void txtDisplayHeightChange(Action* action);
		/// Handler for changing the Language combobox.
		void cbxLanguageChange(Action* action);
		/// Handler for changing the Filter combobox.
		void cbxFilterChange(Action* action);
		///Handler for clicking the Display Mode combobox.
		void updateDisplayMode(Action* action);
		/// Handler for clicking the Letterboxed button.
		void btnLetterboxClick(Action* action);
		/// Handler for clicking the Lock Mouse button.
		void btnLockMouseClick(Action* action);
		/// Handler for updating the selected battlescape scale.
		void updateBattlescapeScale(Action* action);
		/// Handler for updating the selected geoscape scale.
		void updateGeoscapeScale(Action* action);

		/// Update the resolution settings, they just resized the window.
		void resize(
				int&,
				int&) override;

		/// Handles keypresses.
		void handle(Action* action) override;
};

}

#endif
