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

#ifndef OPENXCOM_CAMERA_H
#define OPENXCOM_CAMERA_H

#include "Position.h"


namespace OpenXcom
{

class Action;
class Map;
class State;
class Timer;


/**
 * Class handling camera-movement either by mouse or by events on the battlefield.
 */
class Camera
{

private:
	static const int SCROLL_DIAGONAL_EDGE = 65;

	bool
		_pauseAfterShot,
		_scrollTrigger,
		_showLayers;
	int
		_mapsize_x,
		_mapsize_y,
		_mapsize_z,
		_screenHeight,
		_screenWidth,
		_scrollMouseX,
		_scrollMouseY,
		_scrollKeyX,
		_scrollKeyY,
		_spriteHeight,
		_spriteWidth,
		_playableHeight;

	Map* _map;
	Position
		_offsetField,
		_centerField;
	Timer
		* _scrollMouseTimer,
		* _scrollKeyTimer;

	///
	void intMinMax(
			int* value,
			int minValue,
			int maxValue) const;


	public:
		static const int SCROLL_BORDER = 2;

		/// Creates a Camera.
		Camera(
				int spriteWidth,
				int spriteHeight,
				int mapsize_x,
				int mapsize_y,
				int mapsize_z,
				Map* battleField,
				int playableHeight);
		/// Cleans up the Camera.
		~Camera();

		/// Special handling for mouse-presses.
		void mousePress(Action* action, State* state);
		/// Special handling for mouse-releases.
		void mouseRelease(Action* action, State* state);
		/// Special handling for mouse-overs.
		void mouseOver(Action* action, State* state);
		/// Special handling for key-presses.
		void keyboardPress(Action* action, State* state);
		/// Special handling for key-releases.
		void keyboardRelease(Action* action, State* state);

		/// Sets the Camera's scroll-timers.
		void setScrollTimers(
				Timer* const mouseTimer,
				Timer* const keyboardTimer);
		/// Scrolls the view for mouse-scrolling.
		void scrollMouse();
		/// Scrolls the view for keyboard-scrolling.
		void scrollKey();
		/// Scrolls the view a specified amount.
		void scrollXY(
				int x,
				int y,
				bool redraw);
		/// Handles jumping the view-port by a given deviation.
		void jumpXY(
				int x,
				int y);

		/// Moves the map-layer up.
		bool up();
		/// Moves the map-layer down.
		bool down();

		/// Gets the Map's displayed level.
		int getViewLevel() const;
		/// Sets the view-level.
		void setViewLevel(int viewLevel);

		/// Converts map-coordinates to screen-coordinates.
		void convertMapToScreen(
				const Position& posField,
				Position* const posScreen) const;
		/// Converts voxel-coordinates to screen-coordinates.
		void convertVoxelToScreen(
				const Position& posVoxel,
				Position* const posScreen) const;
		/// Converts screen-coordinates to map-coordinates.
		void convertScreenToMap(
				int screenX,
				int screenY,
				int* mapX,
				int* mapY) const;

		/// Centers the Map on a position.
		void centerOnPosition(
				const Position& posField,
				bool draw = true);
		/// Gets the Map's center-position.
		Position getCenterPosition();

		/// Gets the Map's size-x.
		int getMapSizeX() const;
		/// Gets the Map's size-y.
		int getMapSizeY() const;

		/// Gets the Map's x/y screen-offset.
		Position getMapOffset() const;
		/// Sets the map x/y screen-offset.
		void setMapOffset(const Position& posOffset);

		/// Toggles showing all map-layers.
		bool toggleShowLayers();
		/// Checks if the Camera is showing all map-layers.
		bool getShowLayers() const;

		/// Checks if map-coordinates x/y/z are on-screen.
		bool isOnScreen(const Position& posField) const;
		/// Checks if map-coordinates x/y/z are on-focus.
		bool isInFocus(const Position& posField) const;

		/// Resizes the viewable area.
		void resize();

		/// Stops mouse-scrolling.
		void stopMouseScrolling();

		/// Sets whether to pause the Camera before reverting its position.
		void setPauseAfterShot(bool pause = true);
		/// Gets whether to pause the Camera before reverting its position.
		bool getPauseAfterShot() const;
};

}

#endif
