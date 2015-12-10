/*
 * Copyright 2010-2015 OpenXcom Developers.
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


/**
 * MiniMapView is the class used to display the map in the MiniMapState.
 */
class MiniMapView final
	:
		public InteractiveSurface
{

private:
	bool
		_isMouseScrolled,
		_isMouseScrolling,
		_mouseOverThreshold;

	int
		_frame,
		_mouseScrollX,
		_mouseScrollY,
		_totalMouseMoveX,
		_totalMouseMoveY;
//		_xBeforeMouseScrolling,
//		_yBeforeMouseScrolling;

	Camera* _camera;
	const Game* _game;
	const SavedBattleGame* _battleSave;
	SurfaceSet* _set;

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

		/// Draws the minimap.
		void draw() override;

		/// Changes the displayed minimap level.
		int up();
		/// Changes the displayed minimap level.
		int down();

		/// Animates the minimap.
		void animate();
};

}

#endif
