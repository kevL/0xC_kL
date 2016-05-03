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

#ifndef OPENXCOM_NEWBATTLESTATE_H
#define OPENXCOM_NEWBATTLESTATE_H

//#include <string>
//#include <vector>

#include "../Engine/State.h"


namespace OpenXcom
{

class ComboBox;
class Craft;
class Frame;
class Ruleset;
class Slider;
class Text;
class TextButton;
class Window;


/**
 * New Battle that displays a list of options to configure a standalone mission.
 */
class NewBattleState
	:
		public State
{

private:
	static const Uint8 BROWN_D = 58u;

	ComboBox
		* _cbxMission,
		* _cbxCraft,
		* _cbxTerrain,
		* _cbxDifficulty,
		* _cbxAlienRace;
	Craft* _craft;
	Frame
		* _frameLeft,
		* _frameRight;
	Ruleset* _rules;
	Slider
		* _slrDarkness,
		* _slrAlienTech;
	Text
		* _txtTitle,
		* _txtMapOptions,
		* _txtAlienOptions,
		* _txtMission,
		* _txtCraft,
		* _txtDarkness,
		* _txtTerrain,
		* _txtDifficulty,
		* _txtAlienRace,
		* _txtAlienTech;
	TextButton
		* _btnOk,
		* _btnCancel,
		* _btnEquip,
		* _btnRandom;
	Window* _window;

	std::vector<std::string>
		_missionTypes,
		_terrainTypes,
		_alienRaces,
		_crafts;


	public:
		/// Creates the NewBattle state.
		NewBattleState();
		/// Cleans up the NewBattle state.
		~NewBattleState();

		/// Resets state.
		void init() override;
		/// Loads NewBattle settings.
		void load(const std::string& file = "battle");
		/// Saves NewBattle settings.
		void save(const std::string& file = "battle");
		/// Initializes a blank savegame.
		void initPlay();

		/// Handler for clicking the OK button.
		void btnOkClick(Action* action);
		/// Handler for clicking the Cancel button.
		void btnCancelClick(Action* action);
		/// Handler for clicking the Randomize button.
		void btnRandClick(Action* action);
		/// Handler for clicking the Equip Craft button.
		void btnEquipClick(Action* action);
		/// Handler for changing the Mission combobox.
		void cbxMissionChange(Action* action);
		/// Handler for changing the Craft combobox.
		void cbxCraftChange(Action* action);
};

}

#endif
