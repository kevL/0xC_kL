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

#ifndef OPENXCOM_OPTIONSBASESTATE_H
#define OPENXCOM_OPTIONSBASESTATE_H

//#include <string>

#include "../Engine/State.h"


namespace OpenXcom
{

enum OptionsOrigin
{
	OPT_MAIN_START,	// 0
	OPT_GEOSCAPE,	// 1
	OPT_BATTLESCAPE	// 2
};


class Text;
class TextButton;
class Window;


/**
 * OptionsBaseState for common stuff across Options windows.
 */
class OptionsBaseState
	:
		public State
{

protected:
	std::string _currentTooltip;

	OptionsOrigin _origin;

	Window* _window;
	Text* _txtTooltip;
	TextButton
		* _btnVideo,
		* _btnAudio,
		* _btnControls,
		* _btnGeoscape,
		* _btnBattlescape,
		* _btnAdvanced,
		* _btnMods,

		* _btnOk,
		* _btnCancel,
		* _btnDefault,
		* _group;


	public:
		/// Creates the OptionsBase state.
		explicit OptionsBaseState(OptionsOrigin origin);
		/// Cleans up the OptionsBase state.
		~OptionsBaseState();

		/// Restarts the game states.
		static void restart(OptionsOrigin origin);

		/// Initializes palettes.
		void init() override;

		/// Presses a certain category button.
		void setCategory(TextButton* const button);

		/// Handler for clicking the Ok button.
		void btnOkClick(Action* action);
		/// Handler for clicking the Cancel button.
		void btnCancelClick(Action* action);
		/// Handler for clicking the Restore Defaults button.
		void btnDefaultClick(Action* action);
		/// Handler for clicking one of the grouping buttons.
		void btnGroupPress(Action* action);

		/// Handler for showing tooltip.
//		void txtTooltipIn(Action* action);
		/// Handler for hiding tooltip.
//		void txtTooltipOut(Action* action);

		/// Update the resolution settings, we just resized the window.
		void resize(
				int& dX,
				int& dY) override;
};

}

#endif
