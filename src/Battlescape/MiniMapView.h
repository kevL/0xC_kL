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
		CELL_WIDTH	= 4,
		CELL_HEIGHT	= 4,
		MAX_FRAME	= 2;

	static const Uint32 SCROLL_INTERVAL	= 63;

	bool
		_isMouseScrolled,
		_isMouseScrolling,
		_mouseOverThreshold;

	int
		_cycle,
		_mouseScrollX,
		_mouseScrollY,
		_scrollKeyX,
		_scrollKeyY,
		_totalMouseMoveX,
		_totalMouseMoveY;
//		_xBeforeMouseScrolling,
//		_yBeforeMouseScrolling;

	Camera* _camera;
	const Game* _game;
	const SavedBattleGame* _battleSave;
	SurfaceSet* _set;
	Timer* _timerScroll;

	Position _posPreDragScroll;
//		_cursorPosition;

	Uint32 _mouseScrollStartTime;


	/// Handles pressing on the MiniMap.
	void mousePress(Action* action, State* state) override;
	/// Handles clicking on the MiniMap.
	void mouseClick(Action* action, State* state) override;
	/// Handles moving mouse over the MiniMap.
	void mouseOver(Action* action, State* state) override;
	/// Handles moving the mouse into the MiniMap surface.
	void mouseIn(Action* action, State* state) override;
	///
//	void stopScrolling(Action* action);
	/// Scrolls the MiniMap by keyboard.
	void keyScroll();
	/// Handling for keyboard presses.
	void keyboardPress(Action* action, State* state) override;
	/// Handling for keyboard releases.
	void keyboardRelease(Action* action, State* state) override;


	public:
		/// Creates the MiniMapView.
		MiniMapView(
				int w,
				int h,
				int x,
				int y,
				const Game* const game,
				Camera* const camera,
				const SavedBattleGame* const battleSave);
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

		/// Animates the MiniMap.
		void animate();
};

}

#endif
