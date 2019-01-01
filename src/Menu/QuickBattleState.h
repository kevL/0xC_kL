/*
 * Copyright 2010-2019 OpenXcom Developers.
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

#ifndef OPENXCOM_QUICKBATTLESTATE_H
#define OPENXCOM_QUICKBATTLESTATE_H

//#include <string>
//#include <vector>

#include "../Engine/State.h"


namespace OpenXcom
{

class Base;
class ComboBox;
class Craft;
class Frame;
class Ruleset;
class SavedGame;
class Slider;
class Text;
class TextButton;
class Window;


/**
 * QuickBattle that displays a list of options to configure a standalone mission.
 */
class QuickBattleState
	:
		public State
{

private:
	static const Uint8 BROWN_D = 58u;

	Base* _base;
	Craft* _craft;
	SavedGame* _playSave;

	const Ruleset* _rules;

	ComboBox
		* _cbxMission,
		* _cbxCraft,
		* _cbxTerrain,
		* _cbxDifficulty,
		* _cbxAlienRace;
	Frame
		* _frameL,
		* _frameR;
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
		* _btnCancel,
		* _btnCraft,
		* _btnResetSoldiers,
		* _btnRandMission,
		* _btnScrub,
		* _btnStart;
	Window* _window;

	std::vector<std::string>
		_missionTypes,
		_terrainTypes,
		_alienRaces,
		_crafts;

	/// Clears and generates Base storage-items.
	void resetBaseStores() const;
	/// Clears and generates all ResearchGenerals.
	void resetResearchGenerals() const;


	public:
		/// Creates a NewBattle state.
		QuickBattleState();
		/// Cleans up the NewBattle state.
		~QuickBattleState();

		/// Resets state.
		void init() override;
		/// Loads NewBattle settings.
		void configLoad(const std::string& file = "battle");
		/// Saves NewBattle settings.
		void configSave(const std::string& file = "battle");
		/// Creates the necessary SavedGame.
		void configCreate();
		/// Clears and generates the Soldiers.
		void resetSoldiers();

		/// Handler for clicking the Start button.
		void btnStartClick(Action* action);
		/// Handler for clicking the Cancel button.
		void btnCancelClick(Action* action);
		/// Handler for clicking the Randomize button.
		void btnRandMissionClick(Action* action);
		/// Handler for clicking the Equip Craft button.
		void btnCraftClick(Action* action);
		/// Handler for clicking the Reset Soldiers button.
		void btnResetSoldiersClick(Action* action);
		/// Handler for clicking the Scrub button.
		void btnScrubClick(Action* action);

		/// Handler for changing the Mission combobox.
		void cbxMissionChange(Action* action);
		/// Handler for changing the Craft combobox.
		void cbxCraftChange(Action* action);
};

}

#endif
