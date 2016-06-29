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

#include "MiniMapView.h"

//#include <algorithm>

//#include "../fmath.h"

#include "Camera.h"
#include "MiniMapState.h"

#include "../Engine/Action.h"
#include "../Engine/Game.h"
#include "../Engine/Logger.h"
#include "../Engine/Options.h"
//#include "../Engine/Screen.h"
#include "../Engine/SurfaceSet.h"
#include "../Engine/Timer.h"

#include "../Interface/Cursor.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/RuleArmor.h"

#include "../Savegame/SavedBattleGame.h"
#include "../Savegame/Tile.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the MiniMapView.
 * @param w				- the width
 * @param h				- the height
 * @param x				- the x-origin
 * @param y				- the y-origin
 * @param game			- pointer to the core Game
 * @param camera		- pointer to the battlescape Camera
 * @param battleSave	- pointer to the SavedBattleGame
 */
MiniMapView::MiniMapView(
		int w,
		int h,
		int x,
		int y,
		const Game* const game,
		Camera* const camera,
		const SavedBattleGame* const battleSave)
	:
		InteractiveSurface(
			w,h,
			x,y),
		_game(game),
		_camera(camera),
		_battleSave(battleSave),
		_cycle(0),
		_isMouseScrolling(false),
		_isMouseScrolled(false),
		_mouseScrollX(0),
		_mouseScrollY(0),
		_scrollKeyX(0),
		_scrollKeyY(0),
		_totalMouseMoveX(0),
		_totalMouseMoveY(0),
		_mouseOverThreshold(false),
		_set(game->getResourcePack()->getSurfaceSet("SCANG.DAT"))
{
	_timerScroll = new Timer(SCROLL_INTERVAL);
	_timerScroll->onTimer((SurfaceHandler)& MiniMapView::keyScroll);
}

/**
 * dTor.
 */
MiniMapView::~MiniMapView()
{
	delete _timerScroll;
}

/**
 * Draws the MiniMap.
 */
void MiniMapView::draw()
{
	InteractiveSurface::draw();

	const int
		width  (getWidth()),
		height (getHeight()),
		startX (_camera->getCenterPosition().x - ((width  >> 1u) / CELL_WIDTH)),
		startY (_camera->getCenterPosition().y - ((height >> 1u) / CELL_HEIGHT)),
		camera_Z (_camera->getCenterPosition().z);

	drawRect(
		0,0,
		static_cast<Sint16>(width),
		static_cast<Sint16>(height),
		0u);

	if (_set != nullptr)
	{
		static const int parts (static_cast<int>(Tile::PARTS_TILE));

		Tile* tile;
		Surface* srf;
		const MapData* data;
		const BattleUnit* unit;

		this->lock();
		for (int
				lvl = 0;
				lvl <= camera_Z;
				++lvl)
		{
			int py (startY);
			for (int
					y = Surface::getY();
					y < height + Surface::getY();
					y += CELL_HEIGHT)
			{
				int px (startX);
				for (int
						x = Surface::getX();
						x < width + Surface::getX();
						x += CELL_WIDTH)
				{
					tile = _battleSave->getTile(Position(px, py, lvl));

					if (tile != nullptr)
					{
						int
							colorGroup,
							colorOffset;

						if (   px == 0 // edge markers
							|| px == _battleSave->getMapSizeX() - 1
							|| py == 0
							|| py == _battleSave->getMapSizeY() - 1)
						{
							colorGroup  = 1; // greyscale
							colorOffset = 5;
						}
						else if (tile->isRevealed() == true)
						{
							colorGroup  = 0;
							colorOffset = tile->getShade();
						}
						else
						{
							colorGroup  = 0;
							colorOffset = 15; // paint it ... black !
						}

						if (colorGroup == 1								// is along the edge
							&& lvl == 0									// is ground level
							&& tile->getMapData(O_OBJECT) == nullptr)	// but has no content-object
						{
							srf = _set->getFrame(377);					// draw edge marker
							srf->blitNShade(
										this,
										x,y,
										colorOffset,
										false,
										colorGroup);
						}
						else // draw tile parts
						{
							for (int
									i = 0;
									i != parts;
									++i)
							{
								data = tile->getMapData(static_cast<MapDataType>(i));
								if (data != nullptr && data->getMiniMapIndex() != 0)
								{
									srf = _set->getFrame(data->getMiniMapIndex() + 35);
									if (srf != nullptr)
										srf->blitNShade(
													this,
													x,y,
													colorOffset,
													false,
													colorGroup);
									else Log(LOG_WARNING) << "MiniMapView::draw() no data for Tile["
														  << i << "] pos " << tile->getPosition()
														  << " frame = " << data->getMiniMapIndex() + 35;
								}
							}

							if (tile->getFire() != 0 && tile->isRevealed() == true) // draw fire
							{
								int fire;
								switch (_cycle)
								{
									default:
									case 0:
										fire = 469; // custom scanG's
										break;
									case 1:
										fire = 470;
								}
								srf = _set->getFrame(fire);
								srf->blitNShade(this, x,y);
							}
						}

						if ((unit = tile->getTileUnit()) != nullptr && unit->getUnitVisible() == true) // alive visible units
						{
							const int
								armorSize (unit->getArmor()->getSize()),
								frame (unit->getMiniMapSpriteIndex()
									 + tile->getPosition().x - unit->getPosition().x
									 + (tile->getPosition().y - unit->getPosition().y) * armorSize
									 + _cycle * armorSize * armorSize);

							srf = _set->getFrame(frame);

							if (unit == _battleSave->getSelectedUnit())		// selected unit
							{
								colorGroup  = 4;							// pale green palette-block
								colorOffset = 0;
							}
							else if (unit->getFaction() == FACTION_PLAYER	// Mc'd aLien or civie
								&& unit->isMindControlled() == true)
							{
								colorGroup  = 11;							// brown palette-block
								colorOffset =  1;
							}
							else if (unit->getFaction() == FACTION_HOSTILE	// Mc'd xCom
								&& unit->isMindControlled() == true)
							{
								colorGroup  = 8;							// steel blue palette-block
								colorOffset = 0;
							}
							else											// else aLien.
							{
								colorGroup =
								colorOffset = 0;
							}

							srf->blitNShade(
										this,
										x,y,
										colorOffset,
										false,
										colorGroup);
						}

						if (tile->isRevealed() == true
							&& tile->getInventory()->empty() == false)		// at least one item on this tile
						{
							int color;
							if (tile->hasPrimedGrenade() == true)
								color = YELLOW;
							else
								color = 0;

							srf = _set->getFrame(_cycle + 9);				// white cross
							srf->blitNShade(this, x,y, 0, false, color);
						}

						if (_cycle == 0 && _battleSave->scannerDots().empty() == false)
						{
							std::pair<int,int> dotTest (std::make_pair(px,py));
							if (std::find(
									_battleSave->scannerDots().begin(),
									_battleSave->scannerDots().end(),
									dotTest) != _battleSave->scannerDots().end())
							{
								srf = _set->getFrame(_cycle + 9);			// white cross
								srf->blitNShade(this, x,y, 0, false, RED);
							}
						}
					}
					++px;
				}
				++py;
			}
		}
		this->unlock();
	}
	else Log(LOG_INFO) << "ERROR: MiniMapView SCANG.DAT not available";


	// looks like the crosshairs for the MiniMap
	const Sint16
		centerX (static_cast<Sint16>((width  >> 1u) + 2)),
		centerY (static_cast<Sint16>((height >> 1u) + 2)),
		xOffset (static_cast<Sint16>(CELL_WIDTH  >> 1u)),
		yOffset (static_cast<Sint16>(CELL_HEIGHT >> 1u));

//	const Uint8 color = static_cast<Uint8>(WHITE + _cycle * 3); // <- if you really want the crosshair to blink.
	drawLine(
			centerX - static_cast<Sint16>(CELL_WIDTH),	// top left
			centerY - static_cast<Sint16>(CELL_HEIGHT),
			centerX - xOffset,
			centerY - yOffset,
			WHITE);
	drawLine(
			centerX + xOffset,							// top right
			centerY - yOffset,
			centerX + static_cast<Sint16>(CELL_WIDTH),
			centerY - static_cast<Sint16>(CELL_HEIGHT),
			WHITE);
	drawLine(
			centerX - static_cast<Sint16>(CELL_WIDTH),	// bottom left
			centerY + static_cast<Sint16>(CELL_HEIGHT),
			centerX - xOffset,
			centerY + yOffset,
			WHITE);
	drawLine(
			centerX + static_cast<Sint16>(CELL_WIDTH),	 // bottom right
			centerY + static_cast<Sint16>(CELL_HEIGHT),
			centerX + xOffset,
			centerY + yOffset,
			WHITE);
}

/**
 * Scrolls the MiniMap w/ keyboard handlers.
 */
void MiniMapView::think()
{
	_timerScroll->think(nullptr, this);
}

/**
 * Increments the displayed level.
 * @return, new display level
 */
int MiniMapView::up()
{
	_camera->setViewLevel(_camera->getViewLevel() + 1);
	_redraw = true;

	return _camera->getViewLevel();
}

/**
 * Decrements the displayed level.
 * @return, new display level
 */
int MiniMapView::down()
{
	_camera->setViewLevel(_camera->getViewLevel() - 1);
	_redraw = true;

	return _camera->getViewLevel();
}

/**
 * Centers on the currently selected BattleUnit if any.
 */
void MiniMapView::centerUnit()
{
	const BattleUnit* const unit (_battleSave->getSelectedUnit());
	if (unit != nullptr)
	{
		_camera->centerOnPosition(
								unit->getPosition(),
								false);
		_redraw = true;
	}
}

/**
 * Handles mouse presses on the MiniMap.
 * @note Enters mouse-moving mode when the drag-scroll button is used.
 * @param action	- pointer to an Action
 * @param state		- State that the ActionHandlers belong to
 */
void MiniMapView::mousePress(Action* action, State* state) // private.
{
	InteractiveSurface::mousePress(action, state);

	if (action->getDetails()->button.button == Options::battleDragScrollButton)
	{
		_isMouseScrolling = true;
		_isMouseScrolled = false;

		_posPreDragScroll = _camera->getCenterPosition();

		_mouseScrollX =
		_mouseScrollY =
		_totalMouseMoveX =
		_totalMouseMoveY = 0;

		_mouseOverThreshold = false;
		_mouseScrollStartTime = SDL_GetTicks();
	}
}

/**
 * Handles mouse-clicks on the MiniMap.
 * @note Will change the camera center to the clicked point.
 * @param action	- pointer to an Action
 * @param state		- State that the ActionHandlers belong to
 */
void MiniMapView::mouseClick(Action* action, State* state) // private.
{
	InteractiveSurface::mouseClick(action, state);

	// What follows is a workaround for a rare problem where sometimes the
	// mouse-release event is missed for some reason. However if SDL also
	// missed the release event then this won't work.
	//
	// This part handles the release if it's missed and another button is used.
	if (_isMouseScrolling == true)
	{
		if (action->getDetails()->button.button != Options::battleDragScrollButton
			&& (SDL_GetMouseState(nullptr,nullptr) & SDL_BUTTON(Options::battleDragScrollButton)) == 0)
		{
			// Check if the scrolling has to be revoked because it was too short in time and hence was a click.
			if (_mouseOverThreshold == false
				&& SDL_GetTicks() - _mouseScrollStartTime <= static_cast<Uint32>(Options::dragScrollTimeTolerance))
			{
				_camera->centerOnPosition(_posPreDragScroll, false);
				_redraw = true;
			}

			_isMouseScrolled =
			_isMouseScrolling = false;
		}
	}

	if (_isMouseScrolling == true) // dragScroll-button release: release mouse-scroll-mode
	{
		if (action->getDetails()->button.button != Options::battleDragScrollButton) // other buttons are ineffective while scrolling
			return;

		_isMouseScrolling = false;

		// Check if the scrolling has to be revoked because it was too short in time and hence was a click.
		if (_mouseOverThreshold == false
			&& SDL_GetTicks() - _mouseScrollStartTime <= static_cast<Uint32>(Options::dragScrollTimeTolerance))
		{
			_isMouseScrolled = false;

			_camera->centerOnPosition(_posPreDragScroll, false);
			_redraw = true;
		}

		if (_isMouseScrolled == true) return;
	}

	switch (action->getDetails()->button.button)
	{
		case SDL_BUTTON_LEFT:
		{
			const int
				mX (static_cast<int>(action->getRelativeMouseX() / action->getScaleX())),
				mY (static_cast<int>(action->getRelativeMouseY() / action->getScaleY())),
				offsetX ((mX / CELL_WIDTH)  - (_surface->w / 2 / CELL_WIDTH)), // get offset (in cells) of the click relative to center of screen
				offsetY ((mY / CELL_HEIGHT) - (_surface->h / 2 / CELL_HEIGHT));

			_camera->centerOnPosition(
									Position(
										_camera->getCenterPosition().x + offsetX,
										_camera->getCenterPosition().y + offsetY,
										_camera->getViewLevel()),
									false);
			_redraw = true;
			break;
		}

		case SDL_BUTTON_RIGHT:
			dynamic_cast<MiniMapState*>(state)->btnOkClick(action); // close the parent state.
	}
}

/**
 * Handles moving over the MiniMap.
 * @note Will change the camera center when the mouse is moved in mouse-moving mode.
 * @param action	- pointer to an Action
 * @param state		- State that the ActionHandlers belong to
 */
void MiniMapView::mouseOver(Action* action, State* state) // private.
{
	InteractiveSurface::mouseOver(action, state);

	if (_isMouseScrolling == true
		&& action->getDetails()->type == SDL_MOUSEMOTION)
	{
		// What follows is a workaround for a rare problem where sometimes the
		// mouse-release event is missed for some reason. However if SDL also
		// missed the release event then this won't work.
		//
		// This part handles the release if it's missed and another button is used.
		if ((SDL_GetMouseState(nullptr, nullptr) & SDL_BUTTON(Options::battleDragScrollButton)) == 0)
		{
			if (_mouseOverThreshold == false
				&& SDL_GetTicks() - _mouseScrollStartTime <= static_cast<Uint32>(Options::dragScrollTimeTolerance))
			{
				_camera->centerOnPosition(_posPreDragScroll, false);
				_redraw = true;
			}

			_isMouseScrolled =
			_isMouseScrolling = false;
			return;
		}

		_isMouseScrolled = true;

		_totalMouseMoveX += static_cast<int>(action->getDetails()->motion.xrel);
		_totalMouseMoveY += static_cast<int>(action->getDetails()->motion.yrel);

		if (_mouseOverThreshold == false)
			_mouseOverThreshold = std::abs(_totalMouseMoveX) > Options::dragScrollPixelTolerance
							   || std::abs(_totalMouseMoveY) > Options::dragScrollPixelTolerance;

		_mouseScrollX -= static_cast<int>(action->getDetails()->motion.xrel);
		_mouseScrollY -= static_cast<int>(action->getDetails()->motion.yrel);

		_camera->centerOnPosition(
								Position(
									_posPreDragScroll.x + (_mouseScrollX / 11),
									_posPreDragScroll.y + (_mouseScrollY / 11),
									_camera->getViewLevel()),
								false);
		_redraw = true;

		_game->getCursor()->handle(action);
	}
}

/**
 * Handles moving into the MiniMap.
 * @note Stops the mouse-scrolling mode if it was left on.
 * @param action	- pointer to an Action
 * @param state		- State that the ActionHandlers belong to
 */
void MiniMapView::mouseIn(Action* action, State* state) // private.
{
	InteractiveSurface::mouseIn(action, state);

	_isMouseScrolling = false;
	setButtonPressed(SDL_BUTTON_RIGHT, false);
}

/**
 * Scrolls the MiniMap by keyboard & Timer.
 */
void MiniMapView::keyScroll() // private.
{
	_camera->centerOnPosition(
							Position(
								_camera->getCenterPosition().x - _scrollKeyX,
								_camera->getCenterPosition().y - _scrollKeyY,
								_camera->getViewLevel()),
							false);
	_redraw = true;
}

/**
 * Handles keyboard presses for the MiniMap.
 * @param action	- pointer to an Action
 * @param state		- State that the ActionHandlers belong to
 */
void MiniMapView::keyboardPress(Action* action, State* state) // private.
{
	InteractiveSurface::keyboardPress(action, state);

	static const int scrollSpeed (1);
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
	handleTimer();
}

/**
 * Handles keyboard releases for the MiniMap.
 * @param action	- pointer to an Action
 * @param state		- State that the ActionHandlers belong to
 */
void MiniMapView::keyboardRelease(Action* action, State* state) // private.
{
	InteractiveSurface::keyboardRelease(action, state);

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
	handleTimer();
}

/**
 * Controls timer-start and timer-stop.
 */
void MiniMapView::handleTimer() // private.
{
	if ((_scrollKeyX != 0 || _scrollKeyY != 0)
		&& _timerScroll->isRunning() == false
		&& (SDL_GetMouseState(nullptr,nullptr) & SDL_BUTTON(Options::battleDragScrollButton)) == 0)
	{
		_timerScroll->start();
	}
	else if (_timerScroll->isRunning() == true
		&& _scrollKeyX == 0
		&& _scrollKeyY == 0)
	{
		_timerScroll->stop();
	}
}

/**
 * Animates the MiniMap.
 */
void MiniMapView::animate()
{
	if (++_cycle == CYCLE)
		_cycle = 0;

	_redraw = true;
}

}
