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

#ifndef OPENXCOM_OPTIONSCONTROLSSTATE_H
#define OPENXCOM_OPTIONSCONTROLSSTATE_H

//#include <string>
//#include <vector>

#include "OptionsBaseState.h"

#include "../Engine/OptionInfo.h"


namespace OpenXcom
{

class TextList;

/**
 * Controls screen which allows the user to
 * customize the various key shortcuts in the game.
 */
class OptionsControlsState
	:
		public OptionsBaseState
{

private:
	int _selected; // TODO: This should be size_t; indepth consideration required ....
	Uint8
		_colorGroup,
		_colorSel,
		_colorNormal;

	OptionInfo* _selKey;
	TextList* _lstControls;

	std::vector<OptionInfo>
		_controlsGeneral,
		_controlsGeo,
		_controlsBattle;
	std::string ucWords(std::string str);

	///
	void addControls(const std::vector<OptionInfo>& keys);
	///
	OptionInfo* getControl(size_t sel);


	public:
		/// Creates the Controls state.
		explicit OptionsControlsState(OptionsOrigin origin);
		/// Cleans up the Controls state.
		~OptionsControlsState();

		/// Fills controls list.
		void init() override;

		/// Handler for clicking the Controls list.
		void lstControlsClick(Action* action);
		/// Handler for pressing a key in the Controls list.
		void lstControlsKeyPress(Action* action);
};

}

#endif
