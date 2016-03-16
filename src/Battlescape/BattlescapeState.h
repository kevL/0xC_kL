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
 * along with OpenXcom. If not, see <http:///www.gnu.org/licenses/>.
 */

#ifndef OPENXCOM_BATTLESCAPESTATE_H
#define OPENXCOM_BATTLESCAPESTATE_H

//#include <string>
//#include <vector>

#include "Position.h"

#include "../Engine/State.h"


namespace OpenXcom
{

class Bar;
class BattlescapeButton;
class BattlescapeGame;
class BattleItem;
class BattleUnit;
class ImageButton;
class InteractiveSurface;
class Map;
class NumberText;
class Ruleset;
class SavedBattleGame;
class SavedGame;
class Surface;
class SurfaceSet;
class Text;
class TextList;
class Tile;
class Timer;
class WarningMessage;


/**
 * Battlescape screen which shows the tactical battle.
 */
class BattlescapeState final
	:
		public State
{

private:
	static const size_t
		HOTSQRS			= 20,
		WOUNDED			= 10,
		TARGET_FRAMES	=  6,
		PULSE_FRAMES	= 22;

	static const Uint8
		ORANGE		=  16,
		ORANGE_D	=  20,
		RED			=  32,
		RED_D		=  43,
		GREEN		=  48,
		GREEN_D		=  52,
		BROWN_L		=  80,
		BLUE		= 128,
		YELLOW		= 144,
		YELLOW_D	= 148,
		BROWN		= 160;

	bool
		_autosave,
		_firstInit,
		_iconsHidden,
		_isMouseScrolled,
		_isMouseScrolling,
		_isOverweight,
		_mouseOverIcons,
		_mouseOverThreshold,
		_showSoldierData;
	int
		_showConsole,
		_totalMouseMoveX,
		_totalMouseMoveY;
//		_xBeforeMouseScrolling,
//		_yBeforeMouseScrolling;
	size_t
		_fuseFrame,
		_targeterFrame;

	Uint32 _mouseScrollStartTime;

//	std::string _currentTooltip;

	Bar
		* _barTimeUnits,
		* _barEnergy,
		* _barHealth,
		* _barMorale;
	BattlescapeButton
		* _btnUnitUp,
		* _btnUnitDown,
		* _btnMapUp,
		* _btnMapDown,
		* _btnShowMap,
		* _btnKneel,
		* _btnInventory,
		* _btnCenter,
		* _btnNextUnit,
		* _btnNextStop,
		* _btnShowLayers,
		* _btnOptions,
		* _btnEndTurn,
		* _btnAbort,

		* _btnLaunch,
		* _btnPsi;

//		* _reserve;
//		* _btnReserveNone, * _btnReserveSnap, * _btnReserveAimed, * _btnReserveAuto, * _btnReserveKneel, * _btnZeroTUs;
	BattlescapeGame* _battleGame;
	BattleUnit* _hostileUnit[HOTSQRS];
	InteractiveSurface
		* _icons,
		* _btnLeftHandItem,
		* _btnRightHandItem,
		* _btnStats,
		* _btnHostileUnit[HOTSQRS],
		* _btnWounded[WOUNDED],
		* _btnLogo;
//	ImageButton* _reserve;
//	ImageButton* _btnReserveNone, * _btnReserveSnap, * _btnReserveAimed, * _btnReserveAuto, * _btnReserveKneel, * _btnZeroTUs;
	Map* _map;
	NumberText
		* _numHostileUnit[HOTSQRS],
		* _numWounded[WOUNDED],

		* _numTUAim,
		* _numTUAuto,
		* _numTUSnap,
		* _numTULaunch,

		* _numTimeUnits,
		* _numEnergy,
		* _numHealth,
		* _numMorale,

		* _numAmmoL,
		* _numAmmoR,
		* _numFuseL,
		* _numFuseR,
		* _numTwohandL,
		* _numTwohandR,

		* _numMediL1,
		* _numMediL2,
		* _numMediL3,
		* _numMediR1,
		* _numMediR2,
		* _numMediR3,

		* _numDir,
		* _numDirTur,
		* _numLayers;
	Ruleset* _rules;
	SavedBattleGame* _battleSave;
	SavedGame* _gameSave;
	Surface
		* _alienMark,
		* _bigBtnBorder,
		* _overWeight,
		* _rank,
		* _targeter;
	SurfaceSet* _overlay;
	Tile* _tileWounded[WOUNDED];
	Text
		* _txtBaseLabel,
		* _txtConsole1,
		* _txtConsole2,
		* _txtControlDestroyed,
		* _txtDebug,
		* _txtMissionLabel,
		* _txtName,
		* _txtOperationTitle,
		* _txtOrder,
		* _txtRegion,
		* _txtShade,
		* _txtTerrain,
//		* _txtTooltip;
		* _txtTurn;
	TextList
		* _lstSoldierInfo,
		* _lstTileInfo;
	Timer
		* _aniTimer,
		* _tacticalTimer;
	WarningMessage* _warning;

	std::vector<State*> _popups;

	Position _offsetPreDragScroll;
//		_cursorPosition,


	/// Prints contents of hovered Tile's inventory to screen.
	void printTileInventory(Tile* const tile);

	/// Checks if the player is allowed to press buttons.
	bool allowButtons(bool allowSave = false) const;

	/// Animates a red cross icon when an injured soldier is selected.
	void flashMedic();
	/// Blinks the health bar when selected unit has fatal wounds.
	void blinkHealthBar();
	/// Shows primer warnings on hand-held live grenades.
	void cycleFuses(BattleUnit* const selUnit);
	/// Shifts the colors of the visible unit buttons' backgrounds.
	void hotSqrsCycle(BattleUnit* const selUnit);
	/// Animates a target cursor over hostile unit when hostileUnit indicator is clicked.
	void hostileTargeter();
	/// Draws an execution explosion on the Map.
	void liquidationExplosion();
	/// Draws a shotgun blast explosion on the Map.
	void shotgunExplosion();

	/// Popups a context sensitive list of actions the player can choose from.
	void handAction(
			BattleItem* const item,
			bool injured = false);


	public:
//		static const Uint32 STATE_INTERVAL_STANDARD = 90; // for fast shaders - Raw, Quillez, etc.
		static const Uint32
			STATE_INTERVAL_STANDARD		=  76, // for slow shaders - 4xHQX & above. TODO: Ruleset.
			STATE_INTERVAL_FAST			=  15,
			STATE_INTERVAL_DEATHSPIN	=  21,
			STATE_INTERVAL_EXPLOSION	= 100,
			STATE_INTERVAL_TILE			=  87;
		Uint32
			STATE_INTERVAL_XCOM,
			STATE_INTERVAL_XCOMDASH,
			STATE_INTERVAL_ALIEN;


		/// Creates a BattlescapeState.
		BattlescapeState();
		/// Cleans up the BattlescapeState.
		~BattlescapeState();

		/// Initializes this BattlescapeState.
		void init() override;
		/// Runs the timers and handles popups.
		void think() override;

		/// Handler for moving mouse over the map.
		void mapOver(Action* action);
		/// Handler for pressing the map.
		void mapPress(Action* action);
		/// Handler for clicking the map.
		void mapClick(Action* action);
		/// Handler for entering with mouse to the map surface.
		void mapIn(Action* action);

		/// Move the mouse back to where it started after we finish drag scrolling.
//		void stopScrolling(Action* action);

		/// Handles keypresses.
		void handle(Action* action) override;

		/// Handler for pressing the Unit Up button.
		void btnUnitUpPress(Action* action);
		/// Handler for releasing the Unit Up button.
		void btnUnitUpRelease(Action* action);
		/// Handler for pressing the Unit Down button.
		void btnUnitDownPress(Action* action);
		/// Handler for releasing the Unit Down button.
		void btnUnitDownRelease(Action* action);
		/// Handler for pressing the Map Up button.
		void btnMapUpPress(Action* action);
		/// Handler for releasing the Map Up button.
		void btnMapUpRelease(Action* action);
		/// Handler for pressing the Map Down button.
		void btnMapDownPress(Action* action);
		/// Handler for releasing the Map Down button.
		void btnMapDownRelease(Action* action);

		/// Handler for clicking the Show Map button.
		void btnShowMapClick(Action* action);
		/// Clears the ShowMap btn.
//		void clearShowMapBtn();

		/// Handler for clicking the Kneel button.
		void btnKneelClick(Action* action);

		/// Handler for clicking the Soldier button.
		void btnInventoryClick(Action* action);

		/// Forces a transparent SDL mouse-motion event.
		void refreshMousePosition() const;

		/// Handler for pressing the Center button.
		void btnCenterPress(Action* action);
		/// Handler for releasing the Center button.
		void btnCenterRelease(Action* action);
		/// Handler for pressing the Next Soldier button.
		void btnNextUnitPress(Action* action);
		/// Handler for releasing the Next Soldier button.
		void btnNextUnitRelease(Action* action);
		/// Handler for pressing the Next Stop button.
		void btnNextStopPress(Action* action);
		/// Handler for releasing the Next Stop button.
		void btnNextStopRelease(Action* action);
		/// Handler for pressing the Previous Soldier button.
		void btnPrevUnitPress(Action* action);
		/// Handler for releasing the Previous Soldier button.
		void btnPrevUnitRelease(Action* action);
		/// Handler for pressing the Previous Stop button.
		void btnPrevStopPress(Action* action);
		/// Handler for releasing the Previous Stop button.
		void btnPrevStopRelease(Action* action);

		/// Selects the player's next BattleUnit.
		void selectNextPlayerUnit(
				bool checkReselect = false,
				bool dontReselect = false,
				bool checkInventory = false);
		/// Selects the player's previous BattleUnit.
		void selectPreviousPlayerUnit(
				bool checkReselect = false,
				bool dontReselect = false,
				bool checkInventory = false);

		/// Handler for clicking the Show Layers button.
		void btnShowLayersClick(Action* action);
		/// Sets the level on the icons' Layers button.
		void setLayerValue(int level);

		/// Handler for clicking the Options button.
		void btnBattleOptionsClick(Action* action);
		/// Clears the Options btn.
		void clearOptionsBtn();

		/// Handler for clicking the End Turn button.
		void btnEndTurnClick(Action* action);
		/// Clears the EndTurn btn.
//		void clearEndTurnBtn();

		/// Handler for clicking the Abort button.
		void btnAbortClick(Action* action);
		/// Clears the Abort btn.
		void clearAbortBtn();

		/// Handler for clicking the stats.
		void btnStatsClick(Action* action);

		/// Handler for left-clicking the left hand item button.
		void btnLeftHandLeftClick(Action* action);
		/// Handler for right-clicking the left hand item button.
		void btnLeftHandRightClick(Action* action);
		/// Handler for left-clicking the right hand item button.
		void btnRightHandLeftClick(Action* action);
		/// Handler for right-clicking the right hand item button.
		void btnRightHandRightClick(Action* action);

		/// Handler for clicking a hostile unit button.
		void btnHostileUnitPress(Action* action);
		/// Handler for clicking the wounded unit button.
		void btnWoundedPress(Action* action);

		/// Handler for clicking the launch rocket button.
		void btnLaunchPress(Action* action);
		/// Handler for clicking the use psi button.
		void btnPsiClick(Action* action);

		/// Handler for clicking a reserved button.
//		void btnReserveClick(Action* action);
		/// Handler for clicking the reserve TUs to kneel button.
//		void btnReserveKneelClick(Action* action);

		/// Handler for clicking the reload button.
//		void btnReloadClick(Action* action);
		/// Handler for clicking the expend all TUs button.
		void btnZeroTuClick(Action* action);
		/// Handler for clicking the UfoPaedia button.
		void btnUfoPaediaClick(Action* action);
		/// Handler for clicking the lighting button.
		void btnPersonalLightingClick(Action* action);
		/// Handler for toggling the console.
		void btnConsoleToggle(Action* action);
		/// Handler for showing tooltip.

//		void txtTooltipIn(Action* action);
		/// Handler for hiding tooltip.
//		void txtTooltipOut(Action* action);

		/// Determines whether a playable unit is selected.
		bool playableUnitSelected();

		/// Updates unit stat display and other stuff.
		void updateSoldierInfo(bool calcFoV = true);
		/// Clears the hostile unit indicator squares.
		void hotSqrsClear();
		/// Updates the hostile unit indicator squares.
		void hotSqrsUpdate();

		/// Refreshes the wounded units indicators.
		void hotWoundsRefresh();

		/// Shows a selected unit's kneeled state.
		void toggleKneelButton(BattleUnit* unit);

		/// Animates map objects on the map, also smoke,fire, ...
		void animate();
		/// Handles the top battle game state.
		void handleState();
		/// Sets the state timer interval.
		void setStateInterval(Uint32 interval);

		/// Gets game.
		Game* getGame() const;
		/// Gets pointer to the SavedGame.
		SavedGame* getSavedGame() const;
		/// Gets pointer to the SavedBattleGame.
		SavedBattleGame* getSavedBattleGame() const;
		/// Gets map.
		Map* getMap() const;

		/// Show debug message onScreen.
		void printDebug(const std::wstring& wst);
		/// Show warning message.
		void warning(
				const std::string& st,
				bool useArg = false,
				int arg = 0);

		/// Displays a popup window.
		void popup(State* state);

		/// Finishes a tactical battle.
		void finishBattle(
				const bool abort,
				const int inExitArea);

		/// Shows the launch button.
		void showLaunchButton(bool show = true);
		/// Shows the PSI button.
		void showPsiButton(bool show = true);

		/// Clears mouse-scrolling state.
		void clearMouseScrollingState();

		/// Returns a pointer to the battlegame, in case we need its functions.
		BattlescapeGame* getBattleGame();

		/// Handler for the mouse moving over the icons, disables the tile selection cube.
		void mouseInIcons(Action* action);
		/// Handler for the mouse going out of the icons, enabling the tile selection cube.
		void mouseOutIcons(Action* action);
		/// Checks if the mouse is over the icons.
		bool getMouseOverIcons() const;

		/// Updates the resolution settings, the window was just resized.
		void resize(
				int& dX,
				int& dY) override;

		/// Updates the turn text.
		void updateTurn();

		/// Toggles the icons' surfaces' visibility for Hidden Movement.
		void toggleIcons(bool vis);

		/// Gets the TimeUnits field from icons.
		NumberText* getTuField() const;
		/// Gets the TimeUnits bar from icons.
		Bar* getTuBar() const;
		/// Gets the Energy field from icons.
		NumberText* getEnergyField() const;
		/// Gets the Energy bar from icons.
		Bar* getEnergyBar() const;

		/// Checks if it's okay to show a rookie's kill/stun alien icon.
		bool allowAlienMark() const;
		/// Updates experience data for the currently selected soldier.
		void updateExperienceInfo();
		/// Updates tile info for the tile under mouseover.
		void updateTileInfo(const Tile* const tile);

		/// Autosave next turn.
		void autosave();

		/// Saves a map as used by the AI.
		void saveAIMap();
		/// Saves each layer of voxels on the bettlescape as a png.
		void saveVoxelMap();
		/// Saves a first-person voxel view of the battlescape.
		void saveVoxelView();
};

}

#endif
