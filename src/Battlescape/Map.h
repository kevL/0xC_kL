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

#ifndef OPENXCOM_MAP_H
#define OPENXCOM_MAP_H

//#include <vector>

#include "Position.h"

#include "../Engine/Options.h"
#include "../Engine/InteractiveSurface.h"


namespace OpenXcom
{

class BattlescapeGame;
class BattlescapeMessage;
class BattleUnit;
class Camera;
class Explosion;
class NumberText;
class Projectile;
class ResourcePack;
class SavedBattleGame;
class SurfaceSet;
class Tile;
class TileEngine;
class Timer;


enum SelectorType
{
	CT_NONE,	// 0
	CT_CUBOID,	// 1
	CT_TARGET,	// 2
	CT_PSI,		// 3
	CT_LAUNCH,	// 4
	CT_TOSS		// 5
};


/**
 * Interactive Map of the battlescape.
 */
class Map final
	:
		public InteractiveSurface
{

private:
	static const int
		BULLET_SPRITES	= 35,
		SHADE_BLACK		= 16,

//		DIST_ARC_SMOOTH_Sqr = 64,

		WHITE = 1,
		RED   = 3;

	const int // can't be static (debug cfg.) go fucking figure.
		SHADE_UNIT = 5,
		SHADE_DOOR = 6;

	static const Uint8
		WHITE_u		=  1u,
		BLACK		= 14u,
		ORANGE		= 16u,
		ACU_ORANGE	= 18u,
		ACU_RED		= 35u,
		ACU_GREEN	= 51u;

	static const Uint32
		SCROLL_INTERVAL	= 15u,

		SCREEN_WHITE =  1u,
		SCREEN_BLACK = 15u;

	SelectorType _selectorType;

	bool
		_bulletStart,
		_explosionInFOV,
		_flashScreen,
		_mapIsHidden,
		_noDraw,
		_projectileInFOV,
		_reveal,
		_showProjectile,
		_smoothingEngaged,
		_unitDying;
	int
		_aniCycle,
		_selectorSize,
		_iconHeight,
		_iconWidth,
		_mX,
		_mY,
		_selectorX,
		_selectorY,
		_spriteWidth,
		_spriteWidth_2,
		_spriteHeight,
		_playableHeight;
	Uint8
		_fuseColor;

	PathPreview _previewSetting;

	BattlescapeGame* _battleGame;
	BattlescapeMessage* _hiddenScreen;
	BattleUnit* _unit;
	Camera* _camera;
	const Game* _game;
	NumberText
		* _numAccuracy,
		* _numExposed,
		* _numWaypoint;
	Projectile* _projectile;
	ResourcePack* _res;
	SavedBattleGame* _battleSave;
	Surface
		* _arrow,
		* _arrow_kneel,
		* _srfCross,
		* _srfFuse,
		* _srfRookiBadge;
	SurfaceSet* _projectileSet;
	Tile* _tile;
	TileEngine* _te;
	Timer
		* _scrollMouseTimer,
		* _scrollKeyTimer;

	std::list<Explosion*> _explosions;
	std::vector<Position> _waypoints;

	/// Draws the battlefield.
	void drawTerrain(Surface* const surface);
	/// Draws a Soldier's rank-icon above its sprite on the Map.
	void drawRankIcon(
			const BattleUnit* const unit,
			int offset_x,
			int offset_y,
			bool tLevel = true);
	/// Checks if a southwesterly wall should suppress unit-sprite drawing.
	bool checkWest(
			const Tile* const tile6,
			const Tile* const tile5,
			const BattleUnit* unit = nullptr,
			bool* const halfRight = nullptr) const;
	 /// Checks if a northeasterly wall should suppress unit-sprite drawing.
	bool checkNorth(
			const Tile* const tile0,
//			const Tile* const tile1,
			const BattleUnit* unit = nullptr,
			bool* const halfLeft = nullptr) const;

	/// Gets if a Tile is a/the true location of unit.
	bool isTrueLoc(
			const BattleUnit* const unit,
			const Tile* const tile) const;
	/// Gets the unit's quadrant for drawing.
	int getQuadrant(
			const BattleUnit* const unit,
			const Tile* const tile,
			bool trueLoc) const;
	/// Calculates the offset of a unit-sprite when it is moving between 2 tiles.
	void calcWalkOffset(
			const BattleUnit* const unit,
			Position* const offset,
			bool trueLoc) const;
	///
	int getTerrainLevel(
			const Position& pos,
			int unitSize) const;


	public:
		/// Creates a Map at the specified position and size.
		Map(
				const Game* const game,
				int width,
				int height,
				int x,
				int y,
				int playableHeight);
		/// Cleans up the Map.
		~Map();

		/// Initializes the Map.
		void init();
		/// Handles Timers.
		void think() override;
		/// Draws the Surface.
		void draw() override;

		/// Sets the Palette.
		void setPalette(
				SDL_Color* const colors,
				int firstcolor = 0,
				int ncolors = 256) override;

		/// Special handling for mouse-presses.
		void mousePress(Action* action, State* state) override;
		/// Special handling for mouse-releases.
		void mouseRelease(Action* action, State* state) override;
		/// Special handling for mouse-overs.
		void mouseOver(Action* action, State* state) override;

		/// Finds the current mouse-position x/y on the Map.
		void findMousePointer(Position& pos);

		/// Special handling for key-presses.
		void keyboardPress(Action* action, State* state) override;
		/// Special handling for key-releases.
		void keyboardRelease(Action* action, State* state) override;

		/// Cycles the animation-frames of all Tiles and BattleUnits.
		void animateMap(bool redraw = true);

		/// Sets the selector-position relative to current mouse-position.
		void refreshSelectorPosition();
		/// Gets the currently selected position.
		void getSelectorPosition(Position* const pos) const;
		/// Sets the 3D selector-type.
		void setSelectorType(
				SelectorType type,
				int sideSize = 1);
		/// Gets the 3D selector-type.
		SelectorType getSelectorType() const;

		/// Caches all unit-sprites.
		void cacheUnits();
		/// Caches a unit's sprite.
		void cacheUnit(BattleUnit* const unit);

		/// Sets a projectile.
		void setProjectile(Projectile* const projectile = nullptr);
		/// Gets the projectile.
		Projectile* getProjectile() const;

		/// Gets any explosion set.
		std::list<Explosion*>* getExplosions();

		/// Gets a pointer to the Camera.
		Camera* getCamera();
		/// Mouse-scrolls the Camera.
		void scrollMouse();
		/// Keyboard-scrolls the Camera.
		void scrollKey();

		/// Gets any waypoints-vector.
		std::vector<Position>* getWaypoints();

		/// Sets the mouse-buttons' pressed state.
		void setButtonsPressed(
				Uint8 btn,
				bool pressed);

		/// Sets the unit-dying flag.
		void setUnitDying(bool flag = true);

		/// Special handling for updating Map height.
		void setHeight(int height) override;
		/// Special handling for updating Map width.
		void setWidth(int width) override;

		/// Gets the vertical position of the hidden-movement screen.
		int getMessageY() const;

		/// Gets the icon-height.
		int getIconHeight() const;
		/// Gets the icon-width.
		int getIconWidth() const;

		/// Converts a map-position to a sound-angle.
		int getSoundAngle(const Position& pos) const;

		/// Resets the camera smoothing bool.
//		void resetCameraSmoothing();

		/// Sets whether the screen should "flash" or not.
		void setBlastFlash(bool flash);
		/// Checks if the screen is flashing this.
		bool getBlastFlash() const;

		/// Sets whether to draw or not.
		void setNoDraw(bool noDraw = true);
		/// Gets if the Hidden Movement screen is displayed.
		bool getMapHidden() const;

		/// Gets the SavedBattleGame.
		SavedBattleGame* getBattleSave() const;

		/// Sets the BattlescapeGame.
		void setBattleGame(BattlescapeGame* const battleGame);

		/// Tells the Map to remain revealed because there's a duration-type action going down.
		void setReveal(bool reveal = true);

		/// Sets whether to allow drawing a projectile on the Map.
		void showProjectile(bool show = true);
};

}

#endif
