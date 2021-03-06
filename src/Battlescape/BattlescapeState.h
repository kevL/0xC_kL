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
 * along with OpenXcom. If not, see <http:///www.gnu.org/licenses/>.
 */

#ifndef OPENXCOM_BATTLESCAPESTATE_H
#define OPENXCOM_BATTLESCAPESTATE_H

//#include <string>
//#include <vector>

#include "Position.h"

#include "../Engine/State.h"

#include "../Savegame/BattleUnit.h"


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
 * Battlescape screen that shows tactical-battle info and controls.
 */
class BattlescapeState final
	:
		public State
{

private:
	static const size_t
		ICONS_HOSTILE = 20u,
		ICONS_MEDIC   = 10u,
		PHASE_TARGET  =  6u,
		PHASE_FUSE    = 22u;

	static const Uint8
		TRANSP   =   0u,
		WHITE    =   2u,
		BLACK    =  14u,
		ORANGE   =  16u,
		ORANGE_D =  20u,
		RED      =  32u,
		RED_M    =  38u,
		RED_D    =  42u,
		GREEN    =  48u,
		GREEN_D  =  52u,
		BROWN_L  =  80u,
		BLUE     = 128u,
		YELLOW   = 144u,
		YELLOW_D = 148u,
		BROWN    = 160u;

	bool
		_autosave,
		_toolbarHidden,
		_init,
		_dragScrollActivated,
//		_dragScrollStepped,
		_dragScrollPastPixelThreshold,
		_isOverweight,
		_mouseOverToolbar,
		_showSoldierData;
	int
		_showConsole,
		_dragScrollX,
		_dragScrollY;
	size_t
		_cycleFuse,
		_cycleTargeter;

	Uint32 _dragScrollStartTick;

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
		* _btnMiniMap,
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
	BattlescapeGame* _battle;
	BattleUnit* _hostileUnits[ICONS_HOSTILE];
	InteractiveSurface
		* _toolbar,
		* _isfLeftHand,
		* _isfRightHand,
		* _isfStats,
		* _isfHostiles[ICONS_HOSTILE],
		* _isfMedic[ICONS_MEDIC],
		* _isfLogo;
//	ImageButton* _reserve;
//	ImageButton* _btnReserveNone, * _btnReserveSnap, * _btnReserveAimed, * _btnReserveAuto, * _btnReserveKneel, * _btnZeroTUs;
	Map* _map;
	NumberText
		* _numHostiles[ICONS_HOSTILE],
		* _numMedic[ICONS_MEDIC],

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
	SavedGame* _playSave;
	Surface
		* _srfAlienIcon,
		* _srfBtnBorder,
		* _srfOverweight,
		* _srfRank,
		* _srfTargeter,
		* _srfTitle,
		* _srfAhL, // ActiveHand Left highlight
		* _srfAhR; // ActiveHand Right highlight
	SurfaceSet
		* _srtBigobs,
		* _srtToolbarOverlay,
		* _srtScanG,
		* _srtTargeter;
	Tile* _tileMedic[ICONS_MEDIC];
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
		* _timerAnimate,
		* _timerTactical;
	WarningMessage* _warning;

	std::vector<State*> _popups;

	Position _dragScrollStartPos;
//		_cursorPosition,


	/// Prints contents of hovered Tile's inventory to screen.
	void printTileInventory(Tile* const tile);
	/// Updates mouse-overed tile-data and inventory-info.
	void handleTileInfo();

	/// Activates the left or right hand on a hand-click.
	void activateHand(
			BattleUnit* const unit,
			ActiveHand hand);

	/// Checks if the player is allowed to press buttons.
	bool allowButtons(bool allowSave = false) const;

	/// Animates a red cross icon when an injured soldier is selected.
	void cycleMedic();
	/// Blinks the health bar when selected unit has fatal wounds.
	void cycleHealthBar();
	/// Shows primer warnings on hand-held live grenades.
	void cycleFuses(BattleUnit* const selUnit);
	/// Shifts the colors of the visible unit buttons' backgrounds.
	void cycleHostileIcons(BattleUnit* const selUnit);
	/// Animates a targeter over a hostile unit.
	void cycleTargeter();
	/// Draws an execution explosion on the Map.
	void liquidationExplosion();
	/// Draws a shotgun blast explosion on the Map.
	void shotgunExplosion();

	/// Popups a context-sensitive-list of BattleActions for the player.
	void showActionMenu(
			BattleItem* const item,
			bool injured = false);

	/// Checks if it's okay to show the aLien-icons for a Soldier's kills/stuns.
	bool allowAlienIcons() const;
	/// Updates tile-info for mouse-overs.
	void updateTileInfo(const Tile* const tile);

	/// Saves a first-person voxel view of the battlescape.
	void saveVoxelView();
	/// Saves each layer of voxels on the battlescape as a png.
	void saveVoxelMaps();
	/// Saves a map as used by the AI.
	void saveAIMap();


	public:
		static const Uint32
//			STATE_INTERVAL_STANDARD  =  90u; // for fast shaders - Raw, Quillez, etc.
			STATE_INTERVAL_STANDARD  =  76u, // for slow shaders - 4xHQX & above. TODO: Ruleset.
			STATE_INTERVAL_FAST      =  15u,
			STATE_INTERVAL_DEATHSPIN =  21u,
			STATE_INTERVAL_EXPLOSION = 100u,
			STATE_INTERVAL_TILE      =  87u;
		Uint32
			STATE_INTERVAL_XCOM,
			STATE_INTERVAL_XCOMDASH,
			STATE_INTERVAL_ALIEN;


		/// Creates a BattlescapeState.
		BattlescapeState();
		/// Cleans up the BattlescapeState.
		~BattlescapeState();

		/// Initializes the BattlescapeState.
		void init() override;
		/// Sets a flag to re-initialize the BattlescapeState.
		void reinit();

		/// Runs the timers and handles popups.
		void think() override;

		/// Displays a popup window.
		void popupTac(State* const state);

		/// Handler for moving mouse over the Map.
		void mapOver(Action* action);
		/// Handler for pressing the Map.
		void mapPress(Action* action);
		/// Handler for clicking the Map.
		void mapClick(Action* action);
		/// Handler for the mouse entering the Map surface.
//		void mapIn(Action* action);

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

		/// Handler for clicking the MiniMap button.
		void btnMinimapClick(Action* action);
		/// Clears the MiniMap btn.
//		void clearMinimapBtn();

		/// Handler for clicking the Kneel button.
		void btnKneelClick(Action* action);

		/// Handler for clicking the Soldier button.
		void btnInventoryClick(Action* action);

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
		BattleUnit* selectNextPlayerUnit(
				bool dontReselect = false,
				bool checkReselect = false,
				bool checkInventory = false);
		/// Selects the player's previous BattleUnit.
		BattleUnit* selectPreviousPlayerUnit(
				bool dontReselect = false,
				bool checkReselect = false,
				bool checkInventory = false);

		/// Handler for clicking the Show Layers button.
		void btnShowLayersClick(Action* action);
		/// Sets the level on the icons' Layers button.
		void setLayerValue(int level);

		/// Handler for clicking the Options button.
		void btnBattleOptionsClick(Action* action);
		/// Clears the Options btn.
		void clearOptionsOverlay();

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

		/// Handler for clicking the expend all TUs button.
		void btnZeroTuClick(Action* /*action*/);
		/// Handler for pressing the expend all TUs key.
		void keyZeroTuPress(Action* action);

		/// Handler for clicking the UfoPaedia button.
		void btnUfoPaediaClick(Action* action);

		/// Handler for clicking the lighting button.
		void keyUnitLight(Action* action);

		/// Handler for toggling the console.
		void keyConsoleToggle(Action* action);

		/// Handler for turning a selected BattleUnit with the keyboard.
		void keyTurnUnit(Action* action);

		/// Determines whether a playable unit is selected.
		bool playableUnitSelected();

		/// Updates stat-display and other stuff.
		void updateSoldierInfo(bool spot = true);
		/// Clears the hostile unit indicator squares.
		void clearHostileIcons();
		/// Updates the hostile unit indicator squares.
		void updateHostileIcons();

		/// Updates the wounded units indicators.
		void updateMedicIcons();

		/// Shows a selected BattleUnit's kneeled state.
		void toggleKneelButton(const BattleUnit* const unit);

		/// Animates map objects on the Map, also smoke,fire, ...
		void animate();
		/// Handles the top battle game state.
		void handleState();
		/// Sets the state-timer interval.
		void setStateInterval(Uint32 interval);

		/// Gets game.
		Game* getGame() const;
		/// Gets pointer to the SavedGame.
		SavedGame* getSavedGame() const;
		/// Gets pointer to the SavedBattleGame.
		SavedBattleGame* getSavedBattleGame() const;
		/// Gets map.
		Map* getMap() const;

		/// Prints a debug-message onScreen.
		void printDebug(const std::wstring& wst);
		/// Shows a red warning message that fades overlaid on the HUD-toolbar.
		void warning(
				const std::string& st,
				int arg = std::numeric_limits<int>::max());

		/// Finishes tactical battle.
		void finishBattle(
				bool aborted,
				int playerUnits);

		/// Shows the launch button.
		void showLaunchButton(bool show = true);
		/// Shows the PSI button.
		void showPsiButton(bool show);

		/// Clears drag-scrolling.
		void clearDragScroll(bool doInfo = false);

		/// Returns a pointer to the battlegame, in case we need its functions.
		BattlescapeGame* getBattleGame();

		/// Handler for the mouse moving over the toolbar, disables the tile selection cube.
		void mouseInToolbar(Action* action);
		/// Handler for the mouse going out of the toolbar, enabling the tile selection cube.
		void mouseOutToolbar(Action* action);
		/// Checks if the mouse is over the toolbar.
		bool getMouseOverToolbar() const;

		/// Updates the resolution settings, the window was just resized.
		void resize(
				int& dX,
				int& dY) override;

		/// Updates the turn text.
		void updateTurnText();

		/// Toggles the icons' surfaces' visibility for Hidden Movement.
		void toggleToolbar(bool vis);

		/// Gets the TimeUnits field of the toolbar.
		NumberText* getTuField() const;
		/// Gets the TimeUnits bar of the toolbar.
		Bar* getTuBar() const;
		/// Gets the Energy field of the toolbar.
		NumberText* getEnergyField() const;
		/// Gets the Energy bar of the toolbar.
		Bar* getEnergyBar() const;

		/// Updates experience data for the currently selected soldier.
		void updateExperienceInfo();

		/// Autosave next turn.
		void requestAutosave();

		/// Handler for clicking a reserved button.
//		void btnReserveClick(Action* action);
		/// Handler for clicking the reserve TUs to kneel button.
//		void btnReserveKneelClick(Action* action);
		/// Handler for clicking the reload button.
//		void btnReloadClick(Action* action);

		/// Handler for showing tooltip.
//		void txtTooltipIn(Action* action);
		/// Handler for hiding tooltip.
//		void txtTooltipOut(Action* action);
};

}

#endif
