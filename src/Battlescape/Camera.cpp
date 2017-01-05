/*
 * Copyright 2010-2017 OpenXcom Developers.
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

#include "Camera.h"

//#include <cmath>
//#include <fstream>

#include "../fmath.h"

#include "BattlescapeState.h"
#include "Map.h"

#include "../Engine/Action.h"
#include "../Engine/Options.h"
#include "../Engine/Timer.h"

#include "../Savegame/SavedBattleGame.h"


namespace OpenXcom
{

/**
 * Sets up the Camera.
 * @note Goebbels wants pics.
 * @param spriteWidth		- width of map sprite
 * @param spriteHeight		- height of map sprite
 * @param mapsize_x			- current map-size in x-axis
 * @param mapsize_y			- current map-size in y-axis
 * @param mapsize_z			- current map-size in z-axis
 * @param battleField		- pointer to Map
 * @param playableHeight	- height of map-surface minus icons-height
 */
Camera::Camera(
		int spriteWidth,
		int spriteHeight,
		int mapsize_x,
		int mapsize_y,
		int mapsize_z,
		Map* const battleField,
		int playableHeight)
	:
		_spriteWidth(spriteWidth),
		_spriteHeight(spriteHeight),
		_mapsize_x(mapsize_x),
		_mapsize_y(mapsize_y),
		_mapsize_z(mapsize_z),
		_map(battleField),
		_playableHeight(playableHeight),
		_screenWidth(battleField->getWidth()),
		_screenHeight(battleField->getHeight()),
		_offsetField(-250,250,0),
		_scrollMouseTimer(nullptr),
		_scrollKeyTimer(nullptr),
		_scrollMouseX(0),
		_scrollMouseY(0),
		_scrollKeyX(0),
		_scrollKeyY(0),
		_scrollTrigger(false),
		_showLayers(false),
		_pauseAfterShot(false)
{}

/**
 * Deletes this Camera.
 */
Camera::~Camera()
{}

/**
 * Sets this Camera's mouse- and keyboard-scrolling Timers for use in the
 * battlefield.
 * @param mouseTimer	- pointer to mouse-timer
 * @param keyboardTimer	- pointer to keyboard-timer
 */
void Camera::setScrollTimers(
		Timer* const mouseTimer,
		Timer* const keyboardTimer)
{
	_scrollMouseTimer = mouseTimer;
	_scrollKeyTimer = keyboardTimer;
}

/**
 * Handles this Camera's mouse-press events.
 * @param action - pointer to an Action
 */
void Camera::mousePress(Action* action)
{
	switch (action->getDetails()->button.button)
	{
		case SDL_BUTTON_WHEELUP:
			_map->getBattleSave()->getBattleState()->clearDragScroll();
			down();
			break;

		case SDL_BUTTON_WHEELDOWN:
			_map->getBattleSave()->getBattleState()->clearDragScroll();
			up();
			break;

		case SDL_BUTTON_LEFT:
			if (Options::battleEdgeScroll == MAP_SCROLL_TRIGGER)
			{
				_scrollTrigger = true;
				mouseOver(action);
			}
	}
}

/**
 * Handles this Camera's mouse-release events.
 * @param action - pointer to an Action
 */
void Camera::mouseRelease(Action* action)
{
	if (Options::battleEdgeScroll == MAP_SCROLL_TRIGGER
		&& action->getDetails()->button.button == SDL_BUTTON_LEFT)
	{
		_scrollMouseX =
		_scrollMouseY = 0;
		_scrollMouseTimer->stop();
		_scrollTrigger = false;

		const int
			posX (action->getMouseX()),
			posY (action->getMouseY());

		if (   (posX > 0 && posX < SCROLL_BORDER * action->getScaleX())		// NOTE: I think that (x/y= 0) should be included there.
			||  posX > (_screenWidth - SCROLL_BORDER) * action->getScaleX()	// But then i don't use Scroll_Trigger .....
			|| (posY > 0 && posY < SCROLL_BORDER * action->getScaleY())
			||  posY > (_screenHeight - SCROLL_BORDER) * action->getScaleY())
		{
			action->getDetails()->button.button = 0u;	// do not handle this mouse-release as a map-event
		}												// if the cursor is within the Scroll_Trigger border.
	}
}

/**
 * Handles this Camera's mouse-over events.
 * @param action - pointer to an Action
 */
void Camera::mouseOver(Action* action)
{
	if (_map->getSelectorType() != CT_NONE
		&& (Options::battleEdgeScroll == MAP_SCROLL_AUTO || _scrollTrigger == true))
	{
		const int
			posX (action->getMouseX()),
			posY (action->getMouseY()),
			scrollSpeed (Options::battleScrollSpeed);

		if (posX < SCROLL_BORDER * action->getScaleX())										// left scroll
		{
			_scrollMouseX = scrollSpeed;

			// if close to top or bottom, also scroll diagonally
			if (posY < SCROLL_DIAGONAL_EDGE * action->getScaleY())							// down left
				_scrollMouseY = (scrollSpeed >> 1u);
			else if (posY > (_screenHeight - SCROLL_DIAGONAL_EDGE) * action->getScaleY())	// up left
				_scrollMouseY = -(scrollSpeed >> 1u);
			else
				_scrollMouseY = 0;															// stop vert scroll.
		}
		else if (posX > (_screenWidth - SCROLL_BORDER) * action->getScaleX())				// right scroll
		{
			_scrollMouseX = -scrollSpeed;

			// if close to top or bottom, also scroll diagonally
			if (posY < SCROLL_DIAGONAL_EDGE * action->getScaleY())							// down right
				_scrollMouseY = (scrollSpeed >> 1u);
			else if (posY > (_screenHeight - SCROLL_DIAGONAL_EDGE) * action->getScaleY())	// up right
				_scrollMouseY = -(scrollSpeed >> 1u);
			else
				_scrollMouseY = 0;															// stop vert scroll.
		}
		else if (posX != 0)																	// stop hori scroll.
			_scrollMouseX = 0;

		if (posY < SCROLL_BORDER * action->getScaleY())										// up scroll
		{
			_scrollMouseY = scrollSpeed;

			// if close to left or right edge, also scroll diagonally
			if (posX < SCROLL_DIAGONAL_EDGE * action->getScaleX())							// up left
			{
				_scrollMouseX = scrollSpeed;
				_scrollMouseY >>= 1u;
			}
			else if (posX > (_screenWidth - SCROLL_DIAGONAL_EDGE) * action->getScaleX())	// up right
			{
				_scrollMouseX = -scrollSpeed;
				_scrollMouseY >>= 1u;
			}
		}
		else if (posY > (_screenHeight - SCROLL_BORDER) * action->getScaleY())				// down scroll
		{
			_scrollMouseY = -scrollSpeed;

			// if close to left or right edge, also scroll diagonally
			if (posX < SCROLL_DIAGONAL_EDGE * action->getScaleX())							// down left
			{
				_scrollMouseX = scrollSpeed;
				_scrollMouseY >>= 1u;
			}
			else if (posX > (_screenWidth - SCROLL_DIAGONAL_EDGE) * action->getScaleX())	// down right
			{
				_scrollMouseX = -scrollSpeed;
				_scrollMouseY >>= 1u;
			}
		}
		else if (posY != 0 && _scrollMouseX == 0)											// stop vert scroll.
			_scrollMouseY = 0;


		if ((_scrollMouseX != 0 || _scrollMouseY != 0)
			&& _scrollMouseTimer->isRunning() == false
			&& _scrollKeyTimer->isRunning() == false
			&& action->isMouseButton(Options::battleDragScrollButton) == false)
		{
			_scrollMouseTimer->start();
		}
		else if (_scrollMouseTimer->isRunning() == true
			&& _scrollMouseX == 0
			&& _scrollMouseY == 0)
		{
			_scrollMouseTimer->stop();
		}
	}
}

/**
 * Handles this Camera's keyboard-press events.
 * @param action - pointer to an Action
 */
void Camera::keyboardPress(Action* action)
{
	if (_map->getSelectorType() != CT_NONE)
	{
		const int scrollSpeed (Options::battleScrollSpeed);
		switch (action->getDetails()->key.keysym.sym)
		{
			case SDLK_LEFT: // hardcoding these ... ->
			case SDLK_KP4:
				_scrollKeyX = scrollSpeed;
				break;

			case SDLK_RIGHT:
			case SDLK_KP6:
				_scrollKeyX = -scrollSpeed;
				break;

			case SDLK_UP:
			case SDLK_KP8:
				_scrollKeyY = scrollSpeed;
				break;

			case SDLK_DOWN:
			case SDLK_KP2:
				_scrollKeyY = -scrollSpeed;
				break;

			case SDLK_KP7:
				_scrollKeyX =
				_scrollKeyY = scrollSpeed;
				break;

			case SDLK_KP9:
				_scrollKeyX = -scrollSpeed;
				_scrollKeyY =  scrollSpeed;
				break;

			case SDLK_KP1:
				_scrollKeyX =  scrollSpeed;
				_scrollKeyY = -scrollSpeed;
				break;

			case SDLK_KP3:
				_scrollKeyX =
				_scrollKeyY = -scrollSpeed;
		}

		if ((_scrollKeyX != 0 || _scrollKeyY != 0)
			&& _scrollKeyTimer->isRunning() == false
			&& _scrollMouseTimer->isRunning() == false
			&& action->isMouseButton(Options::battleDragScrollButton) == false)
		{
			_scrollKeyTimer->start();
		}
		else if (_scrollKeyTimer->isRunning() == true
			&& _scrollKeyX == 0
			&& _scrollKeyY == 0)
		{
			_scrollKeyTimer->stop();
			_map->getBattleSave()->getBattleState()->refreshMousePosition();
		}
	}
}

/**
 * Handles this Camera's keyboard-release events.
 * @param action - pointer to an Action
 */
void Camera::keyboardRelease(Action* action)
{
	if (_map->getSelectorType() != CT_NONE)
	{
		switch (action->getDetails()->key.keysym.sym)
		{
			case SDLK_LEFT: // hardcoding these ... ->
			case SDLK_KP4:
			case SDLK_RIGHT:
			case SDLK_KP6:
				_scrollKeyX = 0;
				break;

			case SDLK_UP:
			case SDLK_KP8:
			case SDLK_DOWN:
			case SDLK_KP2:
				_scrollKeyY = 0;
				break;

			default:
				_scrollKeyX =
				_scrollKeyY = 0;
		}

		if ((_scrollKeyX != 0 || _scrollKeyY != 0)
			&& _scrollKeyTimer->isRunning() == false
			&& _scrollMouseTimer->isRunning() == false
			&& action->isMouseButton(Options::battleDragScrollButton) == false)
		{
			_scrollKeyTimer->start();
		}
		else if (_scrollKeyTimer->isRunning() == true
			&& _scrollKeyX == 0
			&& _scrollKeyY == 0)
		{
			_scrollKeyTimer->stop();
			_map->getBattleSave()->getBattleState()->refreshMousePosition();
		}
	}
}

/**
 * Handles this Camera's mouse-scrolling.
 */
void Camera::scrollMouse()
{
	scroll(
			_scrollMouseX,
			_scrollMouseY,
			true);
}

/**
 * Handles this Camera's keyboard-scrolling.
 */
void Camera::scrollKey()
{
	scroll(
			_scrollKeyX,
			_scrollKeyY,
			true);
}

/**
 * Handles scrolling the Screen with specified delta.
 * @param x			- x-delta
 * @param y			- y-delta
 * @param redraw	- true to redraw Map
 */
void Camera::scroll(
		int x,
		int y,
		bool redraw)
{
	_offsetField.x += x;
	_offsetField.y += y;

/*	convertScreenToMap( // convert center of screen to center of battleField
				_screenWidth / 2,
				_playableHeight / 2,
				&_centerField.x,
				&_centerField.y);

	if (_centerField.x < 0)
	{
		_offsetField.x += _centerField.x;
		_offsetField.y += _centerField.x;
	}
	else if (_centerField.x > _mapsize_x - 1)
	{
		_offsetField.x -= _centerField.x;
		_offsetField.y -= _centerField.x;
	}
	else if (_centerField.y < 0)
	{
		_offsetField.x -= _centerField.y;
		_offsetField.y += _centerField.y;
	}
	else if (_centerField.y > _mapsize_y - 1)
	{
		_offsetField.x += _centerField.y;
		_offsetField.y -= _centerField.y;
	} */
	bool stop (false);
	do
	{
		convertScreenToMap( // convert center of screen to center of battleField
					_screenWidth    >> 1u,
					_playableHeight >> 1u,
					&_centerField.x,
					&_centerField.y);
		// Handling map bounds...
		// Ok, this is a prototype, it should be optimized.
		// Actually this should be calculated instead of a slow-approximation:
		if (_centerField.x < 0)
		{
			_offsetField.x -= 1;
			_offsetField.y -= 1;
			continue;
		}

		if (_centerField.x > _mapsize_x - 1)
		{
			_offsetField.x += 1;
			_offsetField.y += 1;
			continue;
		}

		if (_centerField.y < 0)
		{
			_offsetField.x += 1;
			_offsetField.y -= 1;
			continue;
		}

		if (_centerField.y > _mapsize_y - 1)
		{
			_offsetField.x -= 1;
			_offsetField.y += 1;
			continue;
		}

		stop = true;
	}
	while (stop == false);

	_map->refreshSelectorPosition();

	if (redraw == true) _map->draw(); // old code.
//		_map->invalidate();
}

/**
 * Warps the Screen for projectile-motion and when resizing the battlefield-window.
 * @param x - x-delta
 * @param y - y-delta
 */
void Camera::warp(
		int x,
		int y)
{
	_offsetField.x += x;
	_offsetField.y += y;

	convertScreenToMap(
				_screenWidth    >> 1u,
				_playableHeight >> 1u,
				&_centerField.x,
				&_centerField.y);
}

/**
 * Goes one level up.
 * @return, true if not already top-level
 */
bool Camera::up()
{
	if (_offsetField.z < _mapsize_z - 1)
	{
		_map->getBattleSave()->getBattleState()->setLayerValue(++_offsetField.z);
		_offsetField.y += (_spriteHeight >> 1u) + 4;
		_map->draw();
		return true;
	}
	return false;
}

/**
 * Goes one level down.
 * @return, true if not already lowest-level
 */
bool Camera::down()
{
	if (_offsetField.z > 0)
	{
		_map->getBattleSave()->getBattleState()->setLayerValue(--_offsetField.z);
		_offsetField.y -= (_spriteHeight >> 1u) + 4;
		_map->draw();
		return true;
	}
	return false;
}

/**
 * Gets the view-level.
 * @return, the displayed level
 */
int Camera::getViewLevel() const
{
	return _offsetField.z;
}

/**
 * Sets the view-level.
 * @param level - level to display
 */
void Camera::setViewLevel(int level)	// The call from Map::drawTerrain() causes a stack overflow loop when projectile in FoV.
{										// Solution: remove draw() call below_
										// - might have to pass in a 'redraw' bool to compensate for other calls ...
	_offsetField.z = Vicegrip(level, 0, _mapsize_z - 1);

	_map->getBattleSave()->getBattleState()->setLayerValue(_offsetField.z);
//	_map->draw();
}

/**
 * Gets this Camera's center-position.
 * @note Called only by MiniMapView.
 * @return, center Position
 */
Position Camera::getCenterPosition()
{
	_centerField.z = _offsetField.z;
	return _centerField;
}

/**
 * Centers this Camera on a specified battlefield Position.
 * @param posField	- reference to the position to center on
 * @param draw		- true to redraw the map (default true)
 */
void Camera::centerPosition(
		const Position& posField,
		bool draw)
{
	_centerField.x = Vicegrip(posField.x, -1, _mapsize_x);
	_centerField.y = Vicegrip(posField.y, -1, _mapsize_y);

	_centerField.z =
	_offsetField.z = posField.z;

	Position posScreen;
	convertMapToScreen(
					_centerField,
					&posScreen);

	_offsetField.x = -(posScreen.x - (_screenWidth    >> 1u) + 16);
	_offsetField.y = -(posScreen.y - (_playableHeight >> 1u));

	_map->getBattleSave()->getBattleState()->setLayerValue(_offsetField.z);

	if (draw == true) _map->draw();
}

/**
 * Focuses this Camera on a specified battlefield Position.
 * @note A position will be considered OnScreen even if it is on a different
 * view-level than current. This function will correct the view-level if it's
 * different than @a posField.
 * @param posField		- reference to the position to center on
 * @param checkScreen	- true to use screen-edge as bounding box
 *						  false to use "focus" dimensions (default true)
 * @param draw			- true to redraw the map (default true)
 * @return, true if @a posField was NOT OnScreen or InFocus, ie. the camera warps
 */
bool Camera::focusPosition(
		const Position& posField,
		bool checkScreen,
		bool draw)
{
	if (   (checkScreen == true  && isOnScreen(posField) == false)
		|| (checkScreen == false && isInFocus(posField)  == false))
	{
		centerPosition(posField, draw);
		return true;
	}

	if (posField.z != _offsetField.z)
		setViewLevel(posField.z);

	return false;
}

/**
 * Converts screen-coordinates x/y to map-coordinates x/y/z.
 * @param screenX	- screen x-position
 * @param screenY	- screen y-position
 * @param mapX		- pointer to the map x-position
 * @param mapY		- pointer to the map y-position
 */
void Camera::convertScreenToMap(
		int screenX,
		int screenY,
		int* mapX,
		int* mapY) const
{
	const int width_4 (_spriteWidth >> 2u);

	// add half a tile-height to the screen-position per layer above floor-level
	screenY += -(_spriteWidth >> 1u) + _offsetField.z * ((_spriteHeight + width_4) >> 1u);

	// calculate the actual x/y pixel-position on a diamond shaped map
	// taking the viewport-offset into account
	*mapY = -screenX + _offsetField.x + (screenY << 1u) - (_offsetField.y << 1u);
	*mapX =  screenY - _offsetField.y - (*mapY >> 2u) - width_4;

	// to get the row&col itself divide by the size of a tile
	*mapX /= width_4;
	*mapY /= _spriteWidth;

	*mapX = Vicegrip(*mapX, -1, _mapsize_x);
	*mapY = Vicegrip(*mapY, -1, _mapsize_y);
}

/**
 * Converts map-coordinates x/y/z to screen-positions x/y.
 * @param posField	- reference to the x/y/z coordinates on the Map (tile-space)
 * @param posScreen	- pointer to the screen-position pixel (upper left corner of sprite-rectangle)
 */
void Camera::convertMapToScreen(
		const Position& posField,
		Position* const posScreen) const
{
	const int
		width_2 (_spriteWidth >> 1u),
		width_4 (_spriteWidth >> 2u);

	posScreen->x = posField.x * width_2 - posField.y * width_2;
	posScreen->y = posField.x * width_4 + posField.y * width_4 - posField.z * ((_spriteHeight + width_4) >> 1u);
	posScreen->z = 0; // not used
}

/**
 * Converts voxel-coordinates x/y/z to screen-position x/y.
 * @param posVoxel	- reference to the x/y/z coordinates of a voxel
 * @param posScreen	- pointer to the screen-position
 */
void Camera::convertVoxelToScreen(
		const Position& posVoxel,
		Position* const posScreen) const
{
	const Position mapPosition (Position(
										posVoxel.x >> 4u, // yeh i know: just say no.
										posVoxel.y >> 4u,
										posVoxel.z / 24));
	convertMapToScreen(
					mapPosition,
					posScreen);

	const float
		dx (static_cast<float>(posVoxel.x - (mapPosition.x << 4u))),
		dy (static_cast<float>(posVoxel.y - (mapPosition.y << 4u))),
		dz (static_cast<float>(posVoxel.z - (mapPosition.z  * 24)));

	posScreen->x += static_cast<int>(dx - dy) + (_spriteWidth >> 1u);
	posScreen->y += static_cast<int>(((static_cast<float>(_spriteHeight) / 2.f)) + (dx / 2.f) + (dy / 2.f) - dz);
	posScreen->x += _offsetField.x;
	posScreen->y += _offsetField.y;
}

/**
 * Gets the map-size x.
 * @return, the map-size x
 */
int Camera::getMapSizeX() const
{
	return _mapsize_x;
}

/**
 * Gets the map-size y.
 * @return, the map-size y
 */
int Camera::getMapSizeY() const
{
	return _mapsize_y;
}

/**
 * Gets the map-offset.
 * @return, the map-position offset
 */
Position Camera::getMapOffset() const
{
	return _offsetField;
}

/**
 * Sets the map-offset.
 * @param posOffset - the map-position offset
 */
void Camera::setMapOffset(const Position& posOffset)
{
	_offsetField = posOffset;
}

/**
 * Toggles showing all map-layers.
 * @return, true if all layers showed
 */
bool Camera::toggleShowLayers()
{
	return (_showLayers = !_showLayers);
}

/**
 * Checks if this Camera is showing all map-layers.
 * @return, true if all layers are currently showing
 */
bool Camera::getShowLayers() const
{
	return _showLayers;
}

/**
 * Checks if a specified position is displayed on-screen.
 * @note This does not care about the Map's z-level, only whether @a posField
 * is OnScreen.
 * @sa Camera::isInFocus()
 * @param posField - reference to the coordinates to check
 * @return, true if the coordinates are on-screen
 */
bool Camera::isOnScreen(const Position& posField) const
{
//	if (posField.z == _offsetField.z)
//	{
	Position posScreen;
	convertMapToScreen(
					posField,
					&posScreen);
	posScreen.x += _offsetField.x;
	posScreen.y += _offsetField.y;

	return posScreen.x > 16 // NOTE: The top-left corner pixel of a "sprite" is used to determine screen-position.
		&& posScreen.x < _screenWidth - _spriteWidth - 16
		&& posScreen.y > 16
		&& posScreen.y < _playableHeight - _spriteHeight;
//	}
//	return false;
}

/**
 * Checks if a specified position is displayed within a tighter bounding-box
 * than isOnScreen().
 * @note This does not care about the Map's z-level, only whether @a posField
 * is InFocus.
 * @sa Camera::isOnScreen()
 * @param posField - reference to the coordinates to check
 * @return, true if the coordinates are in-focus
 */
bool Camera::isInFocus(const Position& posField) const // private.
{
//	if (posField.z == _offsetField.z)
//	{
	Position posScreen;
	convertMapToScreen(
					posField,
					&posScreen);
	posScreen.x += _offsetField.x;
	posScreen.y += _offsetField.y;

	const int
		border_x ((_screenWidth  - 360) >> 1u), // arbitrary.
		border_y ((_screenHeight - 280) >> 1u);

	return posScreen.x > 16 + border_x // NOTE: The top-left corner pixel of a "sprite" is used to determine screen-position.
		&& posScreen.x < _screenWidth - _spriteWidth - 16 - border_x
		&& posScreen.y > 16 + border_y
		&& posScreen.y < _playableHeight - _spriteHeight - border_y;
//	}
//	return false;
}

/**
 * Checks if map coordinates X/Y are on screen.
 * @param posField Coordinates to check.
 * @param unitWalking True to offset coordinates for a unit walking.
 * @param unitSize size of unit (0 - single, 1 - 2x2, etc, used for walking only
 * @param boundary True if it's for caching calculation
 * @return True if the map coordinates are on screen.
 *
 bool Camera::isOnScreen(const Position &posField, const bool unitWalking, const int unitSize, const bool boundary) const
{
	Position posScreen;
	convertMapToScreen(posField, &posScreen);
	int posx = _spriteWidth/2, posy = _spriteHeight - _spriteWidth/4;
	int sizex = _spriteWidth/2, sizey = _spriteHeight/2;
	if (unitSize > 0)
	{
		posy -= _spriteWidth /4;
		sizex = _spriteWidth*unitSize;
		sizey = _spriteWidth*unitSize/2;
	}
	posScreen.x += _offsetField.x + posx;
	posScreen.y += _offsetField.y + posy;
	if (unitWalking)
	{
//pretty hardcoded hack to handle overlapping by icons
//(they are always in the center at the bottom of the screen)
//Free positioned icons would require more complex workaround.
//__________
//|________|
//||      ||
//|| ____ ||
//||_|XX|_||
//|________|
//
		if (boundary) //to make sprite updates even being slightly outside of screen
		{
			sizex += _spriteWidth;
			sizey += _spriteWidth/2;
		}
		if ( posScreen.x < 0 - sizex
			|| posScreen.x >= _screenWidth + sizex
			|| posScreen.y < 0 - sizey
			|| posScreen.y >= _screenHeight + sizey ) return false; //totally outside
		int side = ( _screenWidth - _map->getIconWidth() ) / 2;
		if ( (posScreen.y < (_screenHeight - _map->getIconHeight()) + sizey) ) return true; //above icons
		if ( (side > 1) && ( (posScreen.x < side + sizex) || (posScreen.x >= (_screenWidth - side - sizex)) ) ) return true; //at sides (if there are any)
		return false;
	}
	else
	{
		return posScreen.x >= 0
			&& posScreen.x <= _screenWidth - 10
			&& posScreen.y >= 0
			&& posScreen.y <= _screenHeight - 10;
	}
} */

/**
 * Resizes the viewable window of this Camera.
 */
void Camera::resize()
{
	_screenWidth = _map->getWidth();
	_screenHeight = _map->getHeight();

	_playableHeight = _map->getHeight() - _map->getIconHeight();
}

/**
 * Stops any mouse-scrolling.
 */
void Camera::stopMouseScroll()
{
	_scrollMouseTimer->stop();
}

/**
 * Sets whether to pause this Camera a moment before reverting to the position
 * it originally had before following a shot per Map tracing.
 * @sa BattleAction::posCamera
 * @param pause - true to pause (default true)
 */
void Camera::setPauseAfterShot(bool pause)
{
	_pauseAfterShot = pause;
}

/**
 * Gets whether to pause this Camera a moment before reverting to the position
 * it originally had before following a shot per Map tracing.
 * @sa BattleAction::posCamera
 * @return, true to pause
 */
bool Camera::getPauseAfterShot() const
{
	return _pauseAfterShot;
}

}
