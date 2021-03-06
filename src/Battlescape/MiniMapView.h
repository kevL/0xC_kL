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

#ifndef OPENXCOM_MINIMAPVIEW_H
#define OPENXCOM_MINIMAPVIEW_H

#include "Position.h"

#include "../Engine/InteractiveSurface.h"


namespace OpenXcom
{

class Camera;
class Game;
class SavedBattleGame;
class SurfaceSet;
class Timer;


/**
 * MiniMapView is the class used to display the map in the MiniMapState.
 */
class MiniMapView final
	:
		public InteractiveSurface
{

private:
	static const int
		CELL_WIDTH	=  4,
		CELL_HEIGHT	=  4,
		CYCLE		=  2,
		RED			=  3,
		YELLOW		= 10;

	static const Uint8 WHITE = 1u;

	static const Uint32 SCROLL_INTERVAL	= 63u;

	bool
		_dragScrollActivated,
		_dragScrollPastPixelThreshold;

	int
		_anicycle,

		_dragScrollX,
		_dragScrollY,

		_keyScrollX,
		_keyScrollY,
		_keyScrollBits;

	static const int
		OFF       = 0x00,
		UP        = 0x01,
		UPRIGHT   = 0x02,
		RIGHT     = 0x04,
		DOWNRIGHT = 0x08,
		DOWN      = 0x10,
		DOWNLEFT  = 0x20,
		LEFT      = 0x40,
		UPLEFT    = 0x80;

	Uint32 _dragScrollStartTick;

	Position _dragScrollStartPos;

	Camera* _camera;
	const Game* _game;
	const SavedBattleGame* _battleSave;
	SurfaceSet* _srtScanG;
	Timer* _timerScroll;


	/// Handles mouse-presses on the MiniMap.
	void mousePress(Action* action, State* state) override;
	/// Handles mouse-clicks on the MiniMap.
	void mouseClick(Action* action, State* state) override;
	/// Handles mouse-overs on the MiniMap.
	void mouseOver(Action* action, State* state) override;
	/// Handles mouse-ins on the MiniMap.
//	void mouseIn(Action* action, State* state) override;

	/// Scrolls the MiniMap by keyboard.
	void keyScroll();
	/// Handling for keyboard-presses.
	void keyboardPress(Action* action, State* state) override;
	/// Handling for keyboard-releases.
	void keyboardRelease(Action* action, State* state) override;

	/// Controls timer-start and timer-stop.
	void handleTimer();


	public:
		/// Creates a MiniMapView.
		MiniMapView(
				int width,
				int height,
				int x,
				int y,
				const Game* const game);
		/// dTor.
		~MiniMapView();

		/// Draws the MiniMap.
		void draw() override;
		/// Thinks the MiniMap.
		void think() override;

		/// Changes the displayed MiniMap level.
		int up();
		/// Changes the displayed MiniMap level.
		int down();

		/// Centers on the currently selected BattleUnit if any.
		void centerUnit();

		/// Checks if drag- or key-scroll is active.
		bool isScrollActive();

		/// Animates the MiniMap.
		void animate();
};

}

#endif
