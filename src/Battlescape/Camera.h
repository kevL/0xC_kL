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
	static const int SCROLL_DIAGONAL_EDGE = 65;	// distance in Px from screen-corners less
												// than which diagonal scroll takes effect.
	bool
		_pauseAfterShot,
		_scrollTrigger,
		_showLayers;
	int
		_mapsize_x,
		_mapsize_y,
		_mapsize_z,

		_playableHeight,

		_screenHeight,
		_screenWidth,
		_spriteHeight,
		_spriteWidth,

		_scrollKeyX,
		_scrollKeyY,
		_scrollMouseX,
		_scrollMouseY;

	Map* _map;
	Position
		_centerField,
		_offsetField;
	Timer
		* _scrollKeyTimer,
		* _scrollMouseTimer;

	/// Sets the value to min if it is below min and to max if it is above max.
	static void intMinMax(
			int* value,
			int minValue,
			int maxValue);

	/// Checks if x/y coordinates are inside a central bounding-box.
	bool isInFocus(const Position& posField) const;


	public:
		static const int SCROLL_BORDER = 2;	// distance in Px from screen-edges less
											// than which scrolling takes effect.
		/// Creates a Camera.
		Camera(
				int spriteWidth,
				int spriteHeight,
				int mapsize_x,
				int mapsize_y,
				int mapsize_z,
				Map* const battleField,
				int playableHeight);
		/// Cleans up the Camera.
		~Camera();

		/// Special handling for mouse-presses.
		void mousePress(Action* action);
		/// Special handling for mouse-releases.
		void mouseRelease(Action* action);
		/// Special handling for mouse-overs.
		void mouseOver(Action* action);
		/// Special handling for key-presses.
		void keyboardPress(Action* action);
		/// Special handling for key-releases.
		void keyboardRelease(Action* action);

		/// Sets the Camera's scroll-timers.
		void setScrollTimers(
				Timer* const mouseTimer,
				Timer* const keyboardTimer);
		/// Scrolls the battlefield-screen by mouse-motion.
		void scrollMouse();
		/// Scrolls the battlefield-screen by keyboard-presses.
		void scrollKey();
		/// Scrolls the battlefield-screen by a specified x/y delta.
		void scroll(
				int x,
				int y,
				bool redraw);
		/// Warps the battlefield-screen by a specified x/y delta.
		void warp(
				int x,
				int y);

		/// Moves the map-level up.
		bool up();
		/// Moves the map-level down.
		bool down();

		/// Gets the Map's view-level.
		int getViewLevel() const;
		/// Sets the Map's view-level.
		void setViewLevel(int level);

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

		/// Gets the Camera's center-position.
		Position getCenterPosition();
		/// Centers the Camera on a Position.
		void centerPosition(
				const Position& posField,
				bool draw = true);
		/// Focuses the Camera on a Position.
		bool focusPosition(
				const Position& posField,
				bool checkScreen = true,
				bool draw = true);

		/// Gets the Map's size-x.
		int getMapSizeX() const;
		/// Gets the Map's size-y.
		int getMapSizeY() const;

		/// Gets the Map's x/y screen-offset.
		Position getMapOffset() const;
		/// Sets the map x/y screen-offset.
		void setMapOffset(const Position& posOffset);

		/// Toggles showing all view-levels.
		bool toggleShowLayers();
		/// Checks if the Camera is showing all view-levels.
		bool getShowLayers() const;

		/// Checks if x/y coordinates are inside the Screen.
		bool isOnScreen(const Position& posField) const;

		/// Resizes the viewable area.
		void resize();

		/// Cancels mouse-scrolling.
		void stopMouseScroll();

		/// Sets whether to pause the Camera before reverting its Position.
		void setPauseAfterShot(bool pause = true);
		/// Gets whether to pause the Camera before reverting its Position.
		bool getPauseAfterShot() const;
};

}

#endif
