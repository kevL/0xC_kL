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

#include "Map.h"

//#include <fstream>

#include "../fmath.h"

#include "BattlescapeState.h"
#include "Camera.h"
#include "Explosion.h"
#include "HiddenMovement.h"
#include "Pathfinding.h"
#include "Projectile.h"
#include "ProjectileFlyBState.h"
#include "TileEngine.h"
#include "UnitSprite.h"

#include "../Engine/Action.h"
#include "../Engine/Game.h"
#include "../Engine/Language.h"
//#include "../Engine/Logger.h"
#include "../Engine/Options.h"
#include "../Engine/Screen.h"
#include "../Engine/SurfaceSet.h"
#include "../Engine/Timer.h"

#include "../Interface/NumberText.h"
#include "../Interface/Text.h"

#include "../Resource/ResourcePack.h"

#include "../Savegame/BattleItem.h"
#include "../Savegame/BattleUnit.h"
#include "../Savegame/SavedBattleGame.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/Tile.h"

#include "../Ruleset/RuleArmor.h"
#include "../Ruleset/MapData.h"
#include "../Ruleset/MapDataSet.h"
#include "../Ruleset/RuleInterface.h"
#include "../Ruleset/RuleItem.h"
#include "../Ruleset/Ruleset.h"


/* Map origin is top point (NW corner).
- x-axis goes downright (width of the Map) eastward
- y-axis goes downleft (height of the Map) southward
- Z axis goes up (depth of the Map) upward

    0,0
    /\
y+ /  \ x+
   \  /
    \/      */

namespace OpenXcom
{

/**
 * Sets up a Map with the specified size and position.
 * @param game				- pointer to the core Game
 * @param width				- width in pixels
 * @param height			- height in pixels
 * @param x					- x-position in pixels
 * @param y					- y-position in pixels
 * @param playableHeight	- Map height above HUD-toolbar (generally clickable)
 */
Map::Map(
		const Game* const game,
		int width,
		int height,
		int x,
		int y,
		int playableHeight)
	:
		InteractiveSurface(
				width,
				height,
				x,y),
		_game(game),
		_playableHeight(playableHeight),
		_arrow(nullptr),
		_arrow_kneel(nullptr),
		_selectorX(0),
		_selectorY(0),
		_mX(0),
		_mY(0),
		_selectorType(CT_CUBOID),
		_selectorSize(1),
		_aniCycle(0),
		_projectile(nullptr),
		_projectileSet(nullptr),
		_projectileInFOV(false),
		_explosionInFOV(false),
		_reveal(false),
		_bulletStart(false),
		_unitDying(false),
		_smoothingEngaged(false),
		_flashScreen(false),
		_mapIsHidden(false),
		_noDraw(false),
		_showProjectile(true),
		_battleSave(game->getSavedGame()->getBattleSave()),
		_te(game->getSavedGame()->getBattleSave()->getTileEngine()),
		_battle(nullptr), // is Set in BattlescapeGame cTor.
		_res(game->getResourcePack()),
		_fuseColor(31u),
		_tile(nullptr),
		_unit(nullptr)
{
	const RuleInterface* const uiRule (_game->getRuleset()->getInterface("battlescape"));
	_toolbarWidth  = uiRule->getElement("toolbar")->w;
	_toolbarHeight = uiRule->getElement("toolbar")->h;

	if (Options::traceAI != 0) // turn everything on to see the markers.
		_previewSetting = PATH_FULL;
	else
		_previewSetting	= Options::battlePreviewPath;

	const Surface* const srf (_res->getSurfaceSet("BLANKS.PCK")->getFrame(0));
	_spriteWidth  = srf->getWidth();
	_spriteHeight = srf->getHeight();
	_spriteWidth_2 = _spriteWidth >> 1u;

	_camera = new Camera(
					_spriteWidth,
					_spriteHeight,
					_battleSave->getMapSizeX(),
					_battleSave->getMapSizeY(),
					_battleSave->getMapSizeZ(),
					this,
					_playableHeight);

	int hiddenHeight;
	if (_playableHeight < 200)
		hiddenHeight = _playableHeight;
	else
		hiddenHeight = 200;
	_hiddenScreen = new HiddenMovement(320, hiddenHeight);
	_hiddenScreen->setX(_game->getScreen()->getDX());
	_hiddenScreen->setY(_game->getScreen()->getDY());
	_hiddenScreen->setTextColor(static_cast<Uint8>(uiRule->getElement("messageWindows")->color));

	_scrollMouseTimer = new Timer(SCROLL_INTERVAL);
	_scrollMouseTimer->onTimer(static_cast<SurfaceHandler>(&Map::scrollMouse));

	_scrollKeyTimer = new Timer(SCROLL_INTERVAL);
	_scrollKeyTimer->onTimer(static_cast<SurfaceHandler>(&Map::scrollKey));

	_camera->setScrollTimers(
						_scrollMouseTimer,
						_scrollKeyTimer);

	_numAccuracy = new NumberText(12,6);
	_numAccuracy->setBordered();

	_numExposed = new NumberText(12,6);

	_numWaypoint = new NumberText(12,6);

	_srfRookiBadge = _res->getSurface("RANK_ROOKIE");
	_srfCross = _res->getSurfaceSet("SCANG.DAT")->getFrame(11);

	_srfFuse = new Surface(3,1);
}

/**
 * Deletes the Map.
 */
Map::~Map()
{
	delete _scrollMouseTimer;
	delete _scrollKeyTimer;
	delete _arrow;
	delete _arrow_kneel;
	delete _hiddenScreen;
	delete _camera;
	delete _numAccuracy;
	delete _numExposed;
	delete _numWaypoint;
	delete _srfFuse;
}

/**
 * Initializes the Map.
 */
void Map::init()
{
	_numAccuracy->setPalette(getPalette());
	_numExposed ->setPalette(getPalette());
	_numWaypoint->setPalette(getPalette());

	static const Uint8
		f (ORANGE),	// Fill
		b (BLACK);	// Border
	static const Uint8 pixels_stand[81u]
	{
		0u, 0u,  b,  b,  b,  b,  b, 0u, 0u,
		0u, 0u,  b,  f,  f,  f,  b, 0u, 0u,
		0u, 0u,  b,  f,  f,  f,  b, 0u, 0u,
		 b,  b,  b,  f,  f,  f,  b,  b,  b,
		 b,  f,  f,  f,  f,  f,  f,  f,  b,
		0u,  b,  f,  f,  f,  f,  f,  b, 0u,
		0u, 0u,  b,  f,  f,  f,  b, 0u, 0u,
		0u, 0u, 0u,  b,  f,  b, 0u, 0u, 0u,
		0u, 0u, 0u, 0u,  b, 0u, 0u, 0u, 0u
	};

	_arrow = new Surface(9,9);
	_arrow->setPalette(getPalette());
	_arrow->lock();
	for (size_t
			y = 0u;
			y != 9u;
			++y)
	{
		for (size_t
				x = 0u;
				x != 9u;
				++x)
		{
			_arrow->setPixelColor(
							static_cast<int>(x),
							static_cast<int>(y),
							pixels_stand[x + (y * 9u)]);
		}
	}
	_arrow->unlock();

	static const Uint8 pixels_kneel[81u]
	{
		0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u,
		0u, 0u, 0u, 0u,  b, 0u, 0u, 0u, 0u,
		0u, 0u, 0u,  b,  f,  b, 0u, 0u, 0u,
		0u, 0u,  b,  f,  f,  f,  b, 0u, 0u,
		0u,  b,  f,  f,  f,  f,  f,  b, 0u,
		 b,  f,  f,  f,  f,  f,  f,  f,  b,
		 b,  b,  b,  f,  f,  f,  b,  b,  b,
		0u, 0u,  b,  f,  f,  f,  b, 0u, 0u,
		0u, 0u,  b,  b,  b,  b,  b, 0u, 0u
	};

	_arrow_kneel = new Surface(9,9);
	_arrow_kneel->setPalette(getPalette());
	_arrow_kneel->lock();
	for (size_t
			y = 0u;
			y != 9u;
			++y)
	{
		for (size_t
				x = 0u;
				x != 9u;
				++x)
		{
			_arrow_kneel->setPixelColor(
									static_cast<int>(x),
									static_cast<int>(y),
									pixels_kneel[x + (y * 9u)]);
		}
	}
	_arrow_kneel->unlock();

	_projectile = nullptr;
	_projectileSet = _res->getSurfaceSet("Projectiles");

//	int // reveal Map's border tiles.
//		size_x = _battleSave->getMapSizeX(),
//		size_y = _battleSave->getMapSizeY(),
//		size_z = _battleSave->getMapSizeZ();
//
//	for (int x = 0; x < size_x; ++x)
//		for (int y = 0; y < size_y; ++y)
//			for (int z = 0; z < size_z; ++z)
//				if (x == 0 || y == 0 || x == size_x - 1 || y == size_y - 1)
//				{
//					Tile* tile = _battleSave->getTile(Position(x,y,z));
//					if (tile) tile->setDiscovered(true,2);
//				}
//			}
//		}
//	}
}

/**
 * Keeps the animation timers running.
 */
void Map::think()
{
	_scrollMouseTimer->think(nullptr, this);
	_scrollKeyTimer->think(nullptr, this);
}

/**
 * Draws the whole Map blit by blit.
 */
void Map::draw()
{
	//Log(LOG_INFO) << "Map::draw()";
	if (_noDraw == false) // don't draw if MiniMap is open. Or if Inventory is open.
	{
	// removed setting this here and in BattlescapeGame::handleBattleState(),
	// Camera::scroll(), ProjectileFlyBState::think() x2.
//	if (_redraw == false) return;
//	_redraw = false;

	// Normally call for a Surface::draw();
	// but don't clear the background with color 0, which is transparent
	// (aka black) -- use color 15 because that actually corresponds to the
	// color you DO want in all variations of the xcom palettes.
	// If you say so ... oh wait.
//	Surface::draw();

		if (_flashScreen == true)
		{
			static int stickyTicks;	// TODO: This should really be some factor of the rate at which calls to
									// ExplosionBState are made against the rate at which calls to Map here are made.
			if (++stickyTicks == 5)
			{
				stickyTicks = 0;
				_flashScreen = false;
			}

			clear(SCREEN_WHITE);
			return;
		}
		clear(SCREEN_BLACK);

//		const Tile* tile;
		if (_projectile != nullptr) //&& _battleSave->getSide() == FACTION_PLAYER)
		{
			_projectileInFOV = true;; // just show the darn projectiles <-
//			if ((tile = _battleSave->getTile(Position::toTileSpace(_projectile->getPosition()))) != nullptr
//				&& (tile->getTileVisible() == true
//					|| _battleSave->getSide() != FACTION_PLAYER)) // shows projectile during aLien berserk
//			{
//				_projectileInFOV = true;
//			}
		}
		else
			_projectileInFOV = false; //_battleSave->getDebugTac(); // reveals Map in debugmode; hides battlefield if no projectiles in flight.

		if (_explosions.empty() == false)
		{
			_explosionInFOV = true; // just show the darn explosions <-
//			for (std::list<Explosion*>::const_iterator
//					i = _explosions.begin();
//					i != _explosions.end();
//					++i)
//			{
//				if ((tile = _battleSave->getTile(Position::toTileSpace((*i)->getPosition()))) != nullptr
//					&& (tile->getTileVisible() == true
//						|| (tile->getTileUnit() != nullptr && tile->getTileUnit()->getUnitVisible() == true)
//						|| (*i)->isAoE() == true
//						|| (*i)->isTorch() == true
//						|| _battleSave->getSide() != FACTION_PLAYER)) // shows hit-explosion during aLien berserk
//				{
//					_explosionInFOV = true;
//					break;
//				}
//			}
		}
		else
			_explosionInFOV = false; //_battleSave->getDebugTac(); // reveals Map in debugmode; hides battlefield if no explosions waiting.


		static bool delayHide (false);

//		static int debug (1);

		if (   _battleSave->getSelectedUnit() == nullptr
			|| _battleSave->getSelectedUnit()->getUnitVisible() == true
			|| _unitDying == true
			|| _explosionInFOV == true
			|| _projectileInFOV == true
			|| _reveal == true // stop flashing the Hidden Movement screen between waypoints and/or autoshots.
			|| _te->isReaction() == true
			|| _battleSave->getDebugTac() == true)
		{
			// REVEAL //
//			if (debug == 1)
//			{
//				debug = 2; Log(LOG_INFO) << ". REVEAL";
//				Log(LOG_INFO)
//						<< "\n\tselUnit=\t"		<< ((_battleSave->getSelectedUnit() != nullptr) ? _battleSave->getSelectedUnit()->getId() : 0)
//						<< "\n\tisVis=\t\t"		<< ((_battleSave->getSelectedUnit() != nullptr) ? _battleSave->getSelectedUnit()->getUnitVisible() : false)
//						<< "\n\treveal=\t\t"	<< _reveal
//						<< "\n\tdying=\t\t"		<< _unitDying
//						<< "\n\texpl=\t\t"		<< _explosionInFOV
//						<< "\n\tprj=\t\t"		<< _projectileInFOV
//						<< "\n\treaction=\t"	<< _te->isReaction();
//			}

			delayHide = true;
			_mapIsHidden = false;
			drawTerrain(this);
		}
		else
		{
			// HIDE //
			if (delayHide == true)
			{
//				if (debug == 2)
//				{
//					debug = 1; Log(LOG_INFO) << ". HIDE";
//					Log(LOG_INFO)
//							<< "\n\tselUnit=\t"		<< ((_battleSave->getSelectedUnit() != nullptr) ? _battleSave->getSelectedUnit()->getId() : 0)
//							<< "\n\tisVis=\t\t"		<< ((_battleSave->getSelectedUnit() != nullptr) ? _battleSave->getSelectedUnit()->getUnitVisible() : false)
//							<< "\n\treveal=\t\t"	<< _reveal
//							<< "\n\tdying=\t\t"		<< _unitDying
//							<< "\n\texpl=\t\t"		<< _explosionInFOV
//							<< "\n\tprj=\t\t"		<< _projectileInFOV
//							<< "\n\treaction=\t"	<< _te->isReaction();
//				}

				delayHide = false;
				SDL_Delay(Screen::SCREEN_PAUSE);
			}

			_mapIsHidden = true;
			_hiddenScreen->blit(this);
		}
	}
	//else Log(LOG_INFO) << ". noDraw TRUE";
}

/**
 * Checks if two positions if have same x/y coords.
 * @param a - reference to position A
 * @param b - reference to position B
 *
static bool isPosition(Position& a, Position& b)
{
	return a.x == b.x
		&& a.y == b.y;
} */

/**
 * Draws the part of a battleunit-sprite that overlaps the current Tile.
 * @param surface
 * @param tileUnit
 * @param tile
 * @param posScreen
 * @param shade
 * @param isTopLayer
 *
void Map::drawUnit(
		Surface* const surface,
		const Tile* const tileUnit,
		const Tile* const tile,
		Position posScreen,
		int shade,
		bool isTopLayer)
{
	const int widthFloor  = 32;
	const int heightFloor = 16;
	const int heightTile  = 40;

	if (tileUnit != nullptr)
	{
		const BattleUnit* unit (tileUnit->getTileUnit());
		const Position& posTileUnit (tileUnit->getPosition());

		bool below (false);

		if (unit == nullptr && posTileUnit.z != 0)
		{
			const Tile* const tileBelow (_battleSave->getTile(posTileUnit + POS_BELOW));
			if (tileBelow != nullptr && tileUnit->isFloored(tileBelow) == false)
			{
				unit = tileBelow->getTileUnit();
				below = true;
			}
		}

		if (unit != nullptr
			&& (unit->getUnitVisible() == true || _battleSave->getDebugTac() == true))
		{
			Position posOffset;

			const Position& posUnit (unit->getPosition());
			posOffset.x = posTileUnit.x - posUnit.x;
			posOffset.y = posTileUnit.y - posUnit.y;

			Surface* srf (unit->getCache(posOffset.x + posOffset.y * 2));
			if (srf != nullptr)
			{
				bool peripatetic (unit->getUnitStatus() == STATUS_WALKING
							   || unit->getUnitStatus() == STATUS_FLYING);
				int
					top,
					bot;

				if (below) // if unit is from below then draw only the area that is in tile
				{
					top =  heightFloor;
					bot = -heightFloor / 2;
				}
				else if (isTopLayer)
				{
					top = heightFloor * 2;
					bot = 0;
				}
				else
				{
					bot = 0;

					const Tile* const tileAbove (_battleSave->getTile(posTileUnit + POS_ABOVE));
					if (tileAbove != nullptr && tileAbove->isFloored(tileUnit) == false)
					{
						top = -heightFloor / 2;
					}
					else
						top = heightFloor;
				}

				int widthExtra ((peripatetic = true) ? 0 : widthFloor);

				GraphSubset range (GraphSubset(
											widthFloor + widthExtra,
											heightTile + top + bot).offset(
																		posScreen.x - widthExtra / 2,
																		posScreen.y - top));

				Position pos;

				if (peripatetic == true)
				{
					GraphSubset left  (range.offset(-widthFloor / 2, 0));
					GraphSubset right (range.offset(+widthFloor / 2, 0));

					pos = tile->getPosition();
					Position posStart (unit->getStartPosition() + posOffset);
					Position posStop  (unit->getStopPosition()  + posOffset);

					// adjusting mask
					if (isPosition(posStart, posStop))
					{
						if (tile != tileUnit) // nothing to draw
							return;
					}
					else if (isPosition(pos, posStart)) // unit is exiting the current tile
					{
						switch (unit->getUnitDirection())
						{
							case 0: // no change
								break;

							case 1:
							case 2:
								range = GraphSubset::intersection(range, left);
								break;

							case 3: // nothing to draw
								return;

							case 4:
							case 5:
								range = GraphSubset::intersection(range, right);
								break;

							case 6: // no change
							case 7: // no change
								break;
						}
					}
					else if (isPosition(pos, posStop)) // unit is moving into the current tile
					{
						switch (unit->getUnitDirection())
						{
							case 0:
							case 1:
								range = GraphSubset::intersection(range, right);
								break;

							case 2: // no change
							case 3: // no change
							case 4: // no change
								break;

							case 5:
							case 6:
								range = GraphSubset::intersection(range, left);
								break;

							case 7: // nothing to draw
								return;
						}
					}
					else
					{
						Position posWest  (pos + POS_WEST);
						Position posNorth (pos + POS_NORTH);

						if (isTopLayer == false
							&& (posStart.z > pos.z || posStop.z > pos.z)) // unit changed level, it will be drawn by upper level not lower.
						{
							return;
						}

						int dir (unit->getUnitDirection());
						if (   (dir == 1 && (posStart == posWest  || posStop == posNorth))
							|| (dir == 5 && (posStart == posNorth || posStop == posWest)))
						{
							range = GraphSubset(
											widthFloor,
											heightTile + heightFloor * 2).offset(
																				posScreen.x,
																				posScreen.y - heightFloor * 2);
						}
						else // unit is not moving close to tile
							return;
					}
				}
				else if (tile != tileUnit)
					return;


				if (below == true)
					pos = POS_BELOW;
				else
					pos = POS_ABOVE;

				Position posScreenTile;
				_camera->convertMapToScreen(posTileUnit + pos, &posScreenTile);
				posScreenTile += _camera->getMapOffset();

				// draw unit
				int shadeOffset;
				calcWalkOffset(unit, &posOffset, isUnitAtTile(unit, tile), &shadeOffset);

				int tileShade ((tile->isRevealed() == true) ? tile->getShade() : SHADE_BLACK);
				int unitShade ((tileShade * (SHADE_BLACK - shadeOffset) + shade * shadeOffset) / SHADE_BLACK);

				srf->blitNShade(
							surface,
							posScreenTile.x + posOffset.x - _spriteWidth_2,
							posScreenTile.y + posOffset.y,
							unitShade,
							range);

				// draw fire
				if (unit->getUnitFire() != 0)
				{
					srf = _res->getSurfaceSet("SMOKE.PCK")->getFrame(4 + (_aniCycle >> 1u));
					srf->blitNShade(
								surface,
								posScreenTile.x + posOffset.x,
								posScreenTile.y + posOffset.y,
								0,
								range);
				}
			}
		}
	}
} */

/**
 * Draws the battlefield.
 * @note Keep this function as optimized as possible - it's big and needs to be
 * fast so minimize overhead of function calls. Etc.
 * @param surface - the Surface on which to draw the entire battlefield
 */
void Map::drawTerrain(Surface* const surface) // private.
{
	//Log(LOG_INFO) << "Map::drawTerrain() " << _camera->getMapOffset();
	Position bullet; // x-y position of bullet on screen.
	int
		bulletLowX	(16000),
		bulletLowY	(16000),
		bulletHighX	(0),
		bulletHighY	(0),
		bulletHighZ	(0),
		viewLevel = _camera->getViewLevel();

	if (_projectile != nullptr // if there is a bullet get the highest x and y tiles to draw it on
		&& _explosions.empty() == true)
	{
		int
			qtyProjectileSpriteFrames,
			trjOffset;

		if (_projectile->getThrowItem() != nullptr)
			qtyProjectileSpriteFrames = 0;
		else
			qtyProjectileSpriteFrames = BULLET_SPRITES - 1;

		for (int
				i = 0;
				i <= qtyProjectileSpriteFrames;
				++i)
		{
			trjOffset = 1 - i;

			if (_projectile->getPosition(trjOffset).x < bulletLowX)
				bulletLowX = _projectile->getPosition(trjOffset).x;

			if (_projectile->getPosition(trjOffset).y < bulletLowY)
				bulletLowY = _projectile->getPosition(trjOffset).y;

			if (_projectile->getPosition(trjOffset).x > bulletHighX)
				bulletHighX = _projectile->getPosition(trjOffset).x;

			if (_projectile->getPosition(trjOffset).y > bulletHighY)
				bulletHighY = _projectile->getPosition(trjOffset).y;

			if (_projectile->getPosition(trjOffset).z > bulletHighZ)
				bulletHighZ = _projectile->getPosition(trjOffset).z;
		}

		bulletLowX  >>= 4; // convert bullet position from voxel-space to tile-space
		bulletLowY  >>= 4;
		bulletHighX >>= 4;
		bulletHighY >>= 4;
		bulletHighZ /= 24;

		if (_projectileInFOV == true) // deal with the Projectile.
		{
			_camera->convertVoxelToScreen(
									_projectile->getPosition(),
									&bullet);

/*			if (Options::battleSmoothCamera == true)
			{ */
			const Position posFinal (_projectile->getFinalPosition());
			const BattleAction* const action (_projectile->getBattleAction());

			if (_bulletStart == true)
			{
				_bulletStart = false;
				if (   bullet.x < 0 // if bullet starts offScreen
					|| bullet.x >= surface->getWidth()
					|| bullet.y < 0
					|| bullet.y >= _playableHeight)
				{
					if (action->type == BA_THROW
						|| action->weapon->getClip() == nullptr // unless it's shotgun pellets.
						|| action->weapon->getClip()->getRules()->getShotgunPellets() == 0)
					{
						_camera->centerPosition(
												Position(
													bulletLowX,
													bulletLowY,
													bulletHighZ),
												false);
					}
				}

				if (_camera->isOnScreen(posFinal) == false)
				{
					_smoothingEngaged = true;
					_camera->setPauseAfterShot();
				}

				if (action->actor->getFaction() == _battleSave->getSide())	// store camera-offset of possible rf-trigger
					_battleSave->rfTriggerOffset(_camera->getMapOffset());
				else if (action->actor->getUnitVisible() == true)			// or set the reaction-fire boolean
					_te->isReaction() = true;
			}
			else if (_smoothingEngaged == true)
				_camera->warp(
							(surface->getWidth() >> 1u) - bullet.x,
							(_playableHeight     >> 1u) - bullet.y);

			if (_smoothingEngaged == true
				|| posFinal.z != action->actor->getPosition().z
				|| ((_projectile->getThrowItem() != nullptr
						|| action->weapon->getRules()->isArcingShot() == true)
					&& std::abs(posFinal.x - action->actor->getPosition().x) > 1
					&& std::abs(posFinal.y - action->actor->getPosition().y) > 1))
			{
				const int posBullet_z (_projectile->getPosition().z / 24);
				if (posBullet_z != viewLevel)
					_camera->setViewLevel(viewLevel = posBullet_z);
			}
/*			}
			else // NOT smoothCamera: I don't use this.
			// Camera remains stationary when xCom actively fires at target;
			// that is, target is already onScreen due to targeting cursor click.
//			if (_projectile->getActor()->getFaction() != FACTION_PLAYER)
			{
				bool enough;
				do
				{
					enough = true;
					if (bullet.x < 0)
					{
						_camera->warp(surface->getWidth(), 0);
						enough = false;
					}
					else if (bullet.x > surface->getWidth())
					{
						_camera->warp(-surface->getWidth(), 0);
						enough = false;
					}
					else if (bullet.y < 0)
					{
						_camera->warp(0, _playableHeight);
						enough = false;
					}
					else if (bullet.y > _playableHeight)
					{
						_camera->warp(0, -_playableHeight);
						enough = false;
					}
					_camera->convertVoxelToScreen(
											_projectile->getPosition(),
											&bullet);
				}
				while (enough == false); // if the projectile is outside the viewport - center back on it
			} */
		}
	}
	else // no projectile OR explosions for Draw.
		_smoothingEngaged = false;

	int // get Map's corner-coordinates for rough boundaries in which to draw tiles.
		beginX,
		beginY,
		beginZ (0),
		endX,
		endY,
		endZ,
		d;

	_camera->convertScreenToMap(
							0,
							0,
							&beginX,
							&d);
	_camera->convertScreenToMap(
							surface->getWidth(),
							0,
							&d,
							&beginY);
	_camera->convertScreenToMap(
							surface->getWidth()  + _spriteWidth,
							surface->getHeight() + _spriteHeight,
							&endX,
							&d);
	_camera->convertScreenToMap(
							0,
							surface->getHeight() + _spriteHeight,
							&d,
							&endY);

	if (_camera->getShowLayers() == true)
	{
		endZ = _battleSave->getMapSizeZ() - 1;
		endY += endZ - viewLevel;
		if (endY > _battleSave->getMapSizeY() - 1)
			endY = _battleSave->getMapSizeY() - 1;
	}
	else
		endZ = viewLevel;

	beginY -= viewLevel << 1u;
	beginX -= viewLevel << 1u;
	if (beginX < 0) beginX = 0;
	if (beginY < 0) beginY = 0;


	static Surface* const srfBorder (_res->getSurfaceSet("SCANG.DAT")->getFrame(330));

	Surface* sprite;
	const Tile* tileBelow;

	Position
		posField,
		posScreen,
		walkOffset;

	int
		spriteId,
		tileShade,
		shade;

	size_t quadrant;	// The quadrant is 0 for small units; large units have quadrants 1,2 & 3 also; describes		0|1
						// the relative x/y Position of the unit's primary quadrant vs. the current tile's Position.	2|3
	bool
		hasUnit, // these denote characteristics of 'tile' as in the current Tile of the loop.
		hasFloor,
		hasObject,
		isLocation;

	surface->lock();
/*	for (int
			itZ = beginZ; // 3. finally lap the levels bottom to top
			itZ <= endZ;
			++itZ)
	{
		for (int
				itX = beginX; // 2. next draw those columns eastward
				itX <= endX;
				++itX)
		{
			for (int
					itY = beginY; // 1. first draw terrain in columns north to south
					itY <= endY;
					++itY)
			{ */
	for (int
			itX = beginX; // 3. finally draw it all eastward
			itX <= endX;
			++itX)
	{
		for (int
				itY = beginY; // 2. next draw those columns north to south
				itY <= endY;
				++itY)
		{
			for (int
					itZ = beginZ; // 1. first draw vertically upward columns
					itZ <= endZ;
					++itZ)
			{
				if ((_tile = _battleSave->getTile(posField = Position(itX,itY,itZ))) != nullptr)
				{
					_camera->convertMapToScreen(posField, &posScreen);
					posScreen += _camera->getMapOffset();

					if (   posScreen.x > -_spriteWidth // only render cells that are inside the surface (ie viewport ala player's monitor)
						&& posScreen.x <  _spriteWidth + surface->getWidth()
						&& posScreen.y > -_spriteHeight
						&& posScreen.y <  _spriteHeight + surface->getHeight())
					{
						if (itZ != 0)
							tileBelow = _battleSave->getTile(posField + POS_BELOW);
						else
							tileBelow = nullptr;

						if (_tile->isRevealed() == true)
							tileShade = _tile->getShade();
						else
							tileShade = SHADE_BLACK;

						_unit = _tile->getTileUnit();

						hasUnit = _unit != nullptr
							  && (_unit->getUnitVisible() == true || _battleSave->getDebugTac() == true);
						hasFloor =
						hasObject = false;

//						bool isTopLayer = (itZ == endZ); // Yankes' code.

// Draw Floor
						if ((sprite = _tile->getSprite(O_FLOOR)) != nullptr)
						{
							hasFloor = true;
							sprite->blitNShade(
									surface,
									posScreen.x,
									posScreen.y - _tile->getMapData(O_FLOOR)->getOffsetY(),
									tileShade);

							// kL_begin #1 of 3:
							// This ensures the rankIcon isn't half-hidden by a floor above & west of soldier. (TODO: floor-tiles etc. further
							// west-north also hide rankIcon if curTerrainLevel < 0) ... unless there's also a floor directly above soldier.
							// Special case: crazy battleship tile+half floors; so check for content object diagonal wall directly above soldier also.
							// Also, make sure the rankIcon isn't half-hidden by a westwall directly above the soldier.
							//
							// bleh. stupid Map ... FIXED.
/*							if (itZ > 0 && itX < endX)
							{
								const Tile* const tileEast (_battleSave->getTile(posField + posEast));
								if (tileEast != nullptr // why.
									&& tileEast->getSprite(O_FLOOR) == nullptr
									&& (tileEast->getMapData(O_CONTENT) == nullptr // special case ->
										|| tileEast->getMapData(O_CONTENT)->getBigwall() != BIGWALL_NWSE))
								{
									const Tile* const tileEastBelow (_battleSave->getTile(posField + posEastBelow));
									const BattleUnit* const unitEastBelow (tileEastBelow->getTileUnit());

									if (unitEastBelow != nullptr
										&& unitEastBelow != _battleSave->getSelectedUnit()
										&& unitEastBelow->getGeoscapeSoldier() != nullptr
										&& unitEastBelow->getFaction() == FACTION_PLAYER)
									{
										drawRankIcon(
												unitEastBelow,
												posScreen.x + 16,
												posScreen.y + 32);
									}
								}
							} */ // kL_end.

// Redraw unitNorth moving NE/SW to stop current-Floor from clipping feet.
							if (itX != 0 && itY != 0)
							{
								const Tile* const tileWest (_battleSave->getTile(posField + POS_WEST));
								const BattleUnit* const unitWest (tileWest->getTileUnit());
								if (unitWest != nullptr
									&& unitWest->getUnitVisible() == true) // don't bother checking DebugMode.
								{
									switch (unitWest->getUnitStatus())
									{
										case STATUS_WALKING:
										case STATUS_FLYING:
										{
											switch (unitWest->getUnitDirection())
											{
												case 1:
												case 5:
												{
													const Tile* tileNorth (_battleSave->getTile(posField + POS_NORTH));
													const BattleUnit* unitNorth (tileNorth->getTileUnit());
													int offsetZ_y;
													if (unitNorth == nullptr && itZ != 0)
													{
														tileNorth = _battleSave->getTile(posField + POS_NORTHBELOW);
														unitNorth = tileNorth->getTileUnit();
														offsetZ_y = 24;
													}
													else
														offsetZ_y = 0;

													if (unitNorth == unitWest)
													{
														const Tile* const tileSouthWest (_battleSave->getTile(posField + POS_SOUTHWEST));
														if (checkWest(tileWest, tileSouthWest, unitNorth) == true)
														{
//															const Tile* const tileNorthEast (_battleSave->getTile(posField + POS_NORTHEAST));
															if (checkNorth(tileNorth, /*tileNorthEast,*/ unitNorth) == true)
															{
																isLocation = isUnitAtTile(unitNorth, tileNorth);
																quadrant = getQuadrant(unitNorth, tileNorth, isLocation);
																sprite = unitNorth->getCache(quadrant);
																//if (sprite != nullptr)
																{
																	if (unitNorth->isOut_t(OUT_HEALTH_STUN) == true)
																		shade = std::min(tileShade, SHADE_UNIT);
																	else
																		shade = tileShade;

																	calcWalkOffset(unitNorth, &walkOffset, isLocation);
																	sprite->blitNShade(
																			surface,
																			posScreen.x + walkOffset.x + 16 - _spriteWidth_2,
																			posScreen.y + walkOffset.y -  8 + offsetZ_y,
																			shade);

																	if (unitNorth->getUnitFire() != 0)
																	{
																		sprite = _res->getSurfaceSet("SMOKE.PCK")->getFrame(4 + (_aniCycle >> 1u));
																		//if (sprite != nullptr)
																			sprite->blitNShade(
																					surface,
																					posScreen.x + walkOffset.x + 16,
																					posScreen.y + walkOffset.y -  8);
																	}
																}
															}
														}
													}
												}
											}
										}
									}
								}
							}
						} // end draw floor

// Draw Cursor Background
						if (_selectorType != CT_NONE
							&& _battleSave->getBattleState()->getMouseOverToolbar() == false
							&& _selectorX >  itX - _selectorSize
							&& _selectorY >  itY - _selectorSize
							&& _selectorX <= itX
							&& _selectorY <= itY)
						{
							if (viewLevel == itZ)
							{
								switch (_selectorType)
								{
									case CT_TARGET:
										if (hasUnit == true)
											spriteId = 7 + (_aniCycle >> 1u);	// yellow animated crosshairs
										else
											spriteId = 6;						// red static crosshairs
										break;

									default:
									case CT_CUBOID:
									case CT_PSI:
									case CT_LAUNCH:
									case CT_TOSS:
										if (hasUnit == true
											&& (_selectorType != CT_PSI
												|| ((_battle->getTacticalAction()->type == BA_PSICOURAGE
														&& _unit->getFaction() != FACTION_HOSTILE
														&& _unit != _battleSave->getSelectedUnit())
													|| (_battle->getTacticalAction()->type != BA_PSICOURAGE
														&& _unit->getFaction() != FACTION_PLAYER))))
										{
											spriteId = (_aniCycle & 1);	// yellow flashing box
										}
										else
											spriteId = 0;				// red static box
								}

								sprite = _res->getSurfaceSet("CURSOR.PCK")->getFrame(spriteId);
								//if (sprite != nullptr)
									sprite->blitNShade(
											surface,
											posScreen.x,
											posScreen.y);
							}
							else if (viewLevel > itZ)
							{
								sprite = _res->getSurfaceSet("CURSOR.PCK")->getFrame(2); // blue static box
								//if (sprite != nullptr)
									sprite->blitNShade(
											surface,
											posScreen.x,
											posScreen.y);
							}
						}
						// end cursor bg
/*
// Yankes: special handling for a moving unit in background of tile.
						const int backs (3);
						Position posback[backs] =
						{
							Position( 0,-1, 0),
							Position(-1,-1, 0),
							Position(-1, 0, 0)
						};

						for (int b = 0; b != backs; ++b)
						{
							drawUnit(
									surface,
									_battleSave->getTile(posField + posback[b]),
									_tile,
									posScreen,
									tileShade,
									isTopLayer);
						}
						// END Yankes.
*/
// Draw Tile Background
						if (_tile->isVoid(true, false) == false)
						{
// Draw West Wall
							if ((sprite = _tile->getSprite(O_WESTWALL)) != nullptr)
							{
								if (_tile->isRevealed(ST_WEST) == true)
								{
									if (_tile->getMapData(O_WESTWALL)->isDoor() == true)
										shade = std::min(_tile->getShade(), SHADE_DOOR);
									else
										shade = tileShade;
								}
								else
									shade = SHADE_BLACK;

								sprite->blitNShade(
										surface,
										posScreen.x,
										posScreen.y - _tile->getMapData(O_WESTWALL)->getOffsetY(),
										shade);
							}

// Draw North Wall
							if ((sprite = _tile->getSprite(O_NORTHWALL)) != nullptr)
							{
								if (_tile->isRevealed(ST_NORTH) == true)
								{
									if (_tile->getMapData(O_NORTHWALL)->isDoor() == true)
										shade = std::min(_tile->getShade(), SHADE_DOOR);
									else
										shade = tileShade;
								}
								else
									shade = SHADE_BLACK;

								sprite->blitNShade(
										surface,
										posScreen.x,
										posScreen.y - _tile->getMapData(O_NORTHWALL)->getOffsetY(),
										shade,
										(_tile->getMapData(O_WESTWALL) != nullptr));
							}

// Draw Object in Background & Center
							if ((sprite = _tile->getSprite(O_CONTENT)) != nullptr)
							{
								switch (_tile->getMapData(O_CONTENT)->getBigwall())
								{
									case BIGWALL_NONE:
									case BIGWALL_BLOCK:
									case BIGWALL_NESW:
									case BIGWALL_NWSE:
									case BIGWALL_WEST:
									case BIGWALL_NORTH:
//									case BIGWALL_W_N: // NOT USED in stock UFO.
										hasObject = true;
										sprite->blitNShade(
												surface,
												posScreen.x,
												posScreen.y - _tile->getMapData(O_CONTENT)->getOffsetY(),
												tileShade);
								}
							}

// Draw Corpse + Item on Floor if any
							bool var;
							if ((spriteId = _tile->getCorpseSprite(&var)) != -1)
							{
								sprite = _res->getSurfaceSet("FLOOROB.PCK")->getFrame(spriteId);
								//if (sprite != nullptr)
									sprite->blitNShade(
											surface,
											posScreen.x,
											posScreen.y + _tile->getTerrainLevel(),
											tileShade);

								if (var == true && _tile->isRevealed() == true)
								{
									sprite = _res->getSurfaceSet("SMOKE.PCK")->getFrame(4 + (_aniCycle >> 1u));
									//if (sprite != nullptr)
										sprite->blitNShade(
												surface,
												posScreen.x,
												posScreen.y + _tile->getTerrainLevel());
								}
							}

							if ((spriteId = _tile->getTopSprite(&var)) != -1)
							{
								sprite = _res->getSurfaceSet("FLOOROB.PCK")->getFrame(spriteId);
								//if (sprite != nullptr)
								{
									sprite->blitNShade(
											surface,
											posScreen.x,
											posScreen.y + _tile->getTerrainLevel(),
											tileShade);

									if (var == true && _tile->isRevealed() == true)
									{
										for (int
												x = 0;
												x != 3;
												++x)
										{
											_srfFuse->setPixelColor(x, 0, _fuseColor);
										}
										_srfFuse->blitNShade(
												surface,
												posScreen.x + 15,
												posScreen.y + 28 + _tile->getTerrainLevel());
									}
								}
							}


							// kL_begin #2 of 3: Make sure the rankIcon isn't half-hidden by a westwall directly above the soldier.
							// TODO: Or a westwall (ie, bulging UFO hull) in tile above & south of the soldier.
							if (itZ > 0
								&& hasFloor == false
								&& hasObject == false)
							{
								const BattleUnit* const unitBelow (tileBelow->getTileUnit());
								if (unitBelow != nullptr
									&& unitBelow != _battleSave->getSelectedUnit()
									&& unitBelow->getGeoscapeSoldier() != nullptr
									&& unitBelow->getFaction() == FACTION_PLAYER)
								{
									drawRankIcon(
											unitBelow,
											posScreen.x,
											posScreen.y + 24);
								}
							}
						} // Void <- end.

// Draw Bullet if in Field of View
						if (_projectile != nullptr && _showProjectile == true) // <- used to hide Celatid glob while its spitting animation plays.
						{
							Position voxel;
							if (_projectile->getThrowItem() != nullptr)
							{
								sprite = _projectile->getThrowSprite();
								//if (sprite != nullptr)
								{
									voxel = _projectile->getPosition(); // draw shadow on the floor
									voxel.z = _te->castShadow(voxel);
									if (   (voxel.x >> 4) >= itX
										&& (voxel.y >> 4) >= itY
										&& (voxel.x >> 4) <  itX + 2
										&& (voxel.y >> 4) <  itY + 2
										&&  voxel.z / 24 ==  itZ)
									{
										_camera->convertVoxelToScreen(voxel, &bullet);
										sprite->blitNShade(
												surface,
												bullet.x - 16,
												bullet.y - 26,
												SHADE_BLACK);
									}

									voxel = _projectile->getPosition(); // draw thrown object
									if (   (voxel.x >> 4) >= itX
										&& (voxel.y >> 4) >= itY
										&& (voxel.x >> 4) <  itX + 2
										&& (voxel.y >> 4) <  itY + 2
										&&  voxel.z / 24 ==  itZ)
									{
										_camera->convertVoxelToScreen(voxel, &bullet);
										sprite->blitNShade(
												surface,
												bullet.x - 16,
												bullet.y - 26);
									}
								}
							}
							else if (_projectile->getBulletSprite(0) != -1) // fired projectile (a bullet-sprite, not a thrown item)
							{
								if (   itX >= bulletLowX // draw bullet on the correct tile
									&& itX <= bulletHighX
									&& itY >= bulletLowY
									&& itY <= bulletHighY)
								{
									for (int
											id = 0;
											id != BULLET_SPRITES;
											++id)
									{
										sprite = _projectileSet->getFrame(_projectile->getBulletSprite(id));
										//if (sprite != nullptr) // fusion-torch has no bullet.
										{
											voxel = _projectile->getPosition(1 - id); // draw shadow on the floor
											voxel.z = _te->castShadow(voxel);
											if (   (voxel.x >> 4) == itX
												&& (voxel.y >> 4) == itY
												&&  voxel.z / 24  == itZ)
											{
												_camera->convertVoxelToScreen(voxel, &bullet);

												bullet.x -= sprite->getWidth() >> 1u;
												bullet.y -= sprite->getHeight() >> 1u;
												sprite->blitNShade(
														surface,
														bullet.x,
														bullet.y,
														SHADE_BLACK);
											}

											voxel = _projectile->getPosition(1 - id); // draw bullet itself
											if (   (voxel.x >> 4) == itX
												&& (voxel.y >> 4) == itY
												&&  voxel.z / 24  == itZ)
											{
												_camera->convertVoxelToScreen(voxel, &bullet);

												bullet.x -= sprite->getWidth() >> 1u;
												bullet.y -= sprite->getHeight() >> 1u;
												sprite->blitNShade(
														surface,
														bullet.x,
														bullet.y);
											}
										}
									}
								}
							}
						}
						// end draw bullet.

// Main Draw BattleUnit ->
						if (hasUnit == true)
						{
/*
							// Yankes' code START ->
							drawUnit(
									surface,
									_tile,
									_tile,
									posScreen,
									tileShade,
									isTopLayer);

// Yankes: special handling for a moving unit in foreground of tile.
							const int fronts (5);
							Position posfront[fronts] =
							{
								Position(-1, 1, 0),
								Position( 0, 1, 0),
								Position( 1, 1, 0),
								Position( 1, 0, 0),
								Position( 1,-1, 0)
							};

							for (int f = 0; f != fronts; ++f)
							{
								drawUnit(
										surface,
										_battleSave->getTile(posField + posfront[f]),
										_tile,
										posScreen,
										tileShade,
										isTopLayer);
							}
							// END Yankes' code.
*/

							bool
								halfRight		(false),
								halfLeft		(false),
								redrawEastwall	(false),
								redrawSouthwall	(false),
								draw			(true);

							if (_unit->getVerticalDirection() == Pathfinding::DIR_VERT_NONE)
							{
								switch (_unit->getUnitStatus()) // don't clip through north/northwest/west UFO hulls etc.
								{
									case STATUS_WALKING:
									case STATUS_FLYING:
										//Log(LOG_INFO) << "pos " << _tile->getPosition();
										//Log(LOG_INFO) << "dir= " << _unit->getUnitDirection() << " phase= " << _unit->getWalkPhase();

//										switch (_unit->getUnitDirection())
//										{
//											default:
//												if (_unit->getWalkPhase() == 0) break; // ... think I took care of this better in UnitWalkBState.
//											case 1:
//											case 2: // weird.
//											case 4:
												switch (_unit->getUnitDirection())
												{
													case 0:
													case 4:
														redrawEastwall =
														draw = checkNorth(
																		_battleSave->getTile(posField + POS_NORTH),
//																		_battleSave->getTile(posField + POS_NORTHEAST),
																		nullptr,
																		&halfLeft);
														break;

													case 2:
													case 6:
														redrawSouthwall =
														draw = checkWest(
																		_battleSave->getTile(posField + POS_WEST),
																		_battleSave->getTile(posField + POS_SOUTHWEST),
																		nullptr,
																		&halfRight);
														//Log(LOG_INFO) << ". drawUnit/redrawWall= " << draw << " hRight= " << halfRight;
														break;

													case 1:
													case 5:
														draw = checkWest(
																		_battleSave->getTile(posField + POS_SOUTHWEST),
																		_battleSave->getTile(posField + POS_SOUTHSOUTHWEST));
														draw &= checkNorth(
																		_battleSave->getTile(posField + POS_NORTHEAST));
//																		_battleSave->getTile(posField + posNorthNorthEast));
												}
//										}
								}
							}

							if (draw == true)
							{
								isLocation = isUnitAtTile(_unit, _tile);
								quadrant = getQuadrant(_unit, _tile, isLocation);
								sprite = _unit->getCache(quadrant);
								if (sprite != nullptr) // <- check is needed for start of Tactical.
								{
									if (_unit->isOut_t(OUT_HEALTH_STUN) == true)
										shade = std::min(tileShade, SHADE_UNIT);
									else
										shade = tileShade;

									calcWalkOffset(_unit, &walkOffset, isLocation);
									sprite->blitNShade(
											surface,
											posScreen.x + walkOffset.x - _spriteWidth_2,
											posScreen.y + walkOffset.y,
											shade, halfRight,
											0, halfLeft);

									if (_unit->getUnitFire() != 0)
									{
										sprite = _res->getSurfaceSet("SMOKE.PCK")->getFrame(4 + (_aniCycle >> 1u));
										//if (sprite != nullptr)
											sprite->blitNShade(
													surface,
													posScreen.x + walkOffset.x,
													posScreen.y + walkOffset.y,
													0, halfRight,
													0, halfLeft);
									}

									if (redrawEastwall == true
										&& (   _tile->getMapData(O_CONTENT) == nullptr
											|| _tile->getMapData(O_CONTENT)->getBigwall() != BIGWALL_EAST))
									{
										const Tile* const tileNorth (_battleSave->getTile(posField + POS_NORTH));
										if (tileNorth != nullptr // safety. perhaps
											&& tileNorth->getMapData(O_CONTENT) != nullptr
											&& tileNorth->getMapData(O_CONTENT)->getBigwall() == BIGWALL_EAST)
										{
											sprite = tileNorth->getSprite(O_CONTENT);
											sprite->blitNShade(
													surface,
													posScreen.x + 16,
													posScreen.y -  8 - tileNorth->getMapData(O_CONTENT)->getOffsetY(),
													tileNorth->getShade());
										}
									}
									else if (redrawSouthwall == true
										&& (   _tile->getMapData(O_CONTENT) == nullptr
											|| _tile->getMapData(O_CONTENT)->getBigwall() != BIGWALL_SOUTH))
									{
										const Tile* const tileWest (_battleSave->getTile(posField + POS_WEST));
										if (tileWest != nullptr
											&& tileWest->getMapData(O_CONTENT) != nullptr
											&& tileWest->getMapData(O_CONTENT)->getBigwall() == BIGWALL_SOUTH)
										{
											sprite = tileWest->getSprite(O_CONTENT);
											sprite->blitNShade(
													surface,
													posScreen.x - 16,
													posScreen.y -  8 - tileWest->getMapData(O_CONTENT)->getOffsetY(),
													tileWest->getShade());
										}
									}

									// kL_begin #3 of 3:
									if (_unit->getFaction() == FACTION_PLAYER
										&& _unit->isMindControlled() == false)
									{
										const Tile* const tileAbove (_battleSave->getTile(posField + POS_ABOVE));
										if ((viewLevel == itZ
												&& (_camera->getShowLayers() == false || itZ == endZ))
											|| (tileAbove != nullptr && tileAbove->getSprite(O_FLOOR) == nullptr))
										{
											if (_unit->getGeoscapeSoldier() != nullptr
												&& _unit != _battleSave->getSelectedUnit())
											{
												drawRankIcon(
														_unit,
														posScreen.x + walkOffset.x,
														posScreen.y + walkOffset.y,
														false);
											}

											// Draw Exposed mark
											if (_battleSave->getSide() == FACTION_PLAYER
												&& (_unit->getArmor()->getSize() == 1 || quadrant == 1u)
//												&& _projectileInFOV == false && _explosionInFOV == false)
//												&& _battleSave->getBattleState()->allowButtons() == true
												&& _battle->getTacticalAction()->type == BA_NONE)
												// well that's quirky. The exposed value of actor gets drawn on the
												// defender (at least when within one tile) for a brief flash, even
												// before projectile and explosion is taken into account. If projectile
												// and explosion are taken into consideration, the flash lasts several
												// engine-ticks. But currentAction=NONE makes it all go away ofc.
											{
												const int exposure (_unit->getExposed());
												if (exposure != -1)
												{
													Uint8 color;
													switch (_aniCycle)
													{
														default:
														case 0: case 1: case 2: case 3: color = WHITE; break;
														case 4: case 5: case 6: case 7: color = BLACK;
													}

													_numExposed->setValue(static_cast<unsigned>(exposure));
													_numExposed->setColor(color);
													_numExposed->draw();
													_numExposed->blitNShade(
															surface,
															posScreen.x + walkOffset.x + 21,
															posScreen.y + walkOffset.y + 5);
												}
											}
										}
									} // kL_end.
								}
							}
						}
						// end Main Draw BattleUnit.

// Draw Unconscious Soldier icon.
						if (_unit == nullptr)
						{
							const int hurt (_tile->hasUnconsciousUnit());
							if (hurt != 0)
							{
								_srfRookiBadge->blitNShade(			// background panel for red cross icon.
													surface,
													posScreen.x,
													posScreen.y);
								int color;
								switch (hurt)
								{
									default:
									case 1: color = WHITE_i; break;	// unconscious soldier here
									case 2: color = RED_i;			// wounded unconscious soldier
								}
								_srfCross->blitNShade(				// small gray cross
												surface,
												posScreen.x + 2,
												posScreen.y + 1,
												(_aniCycle << 1u),
												false, color);
							}
						}
						// end unconscious soldier icon.

// re-Draw unitBelow if it is on raised ground (on tileBelow) & there is no Floor.
						if (itZ > 0 && _tile->isFloored(tileBelow) == false
							&& _camera->getShowLayers() == false)
						{
							const int tLevel (tileBelow->getTerrainLevel());
							if (tLevel < 0) // probly more like -4 or -8
							{
								const BattleUnit* const unitBelow (tileBelow->getTileUnit());
								if (unitBelow != nullptr
									&& unitBelow->getUnitVisible() == true // don't bother checking DebugMode
									&& unitBelow->getHeight(true) - tLevel > Pathfinding::UNIT_HEIGHT)
								{
									isLocation = isUnitAtTile(unitBelow, tileBelow);
									quadrant = getQuadrant(unitBelow, tileBelow, isLocation);
									sprite = unitBelow->getCache(quadrant);
									//if (sprite != nullptr)
									{
										if (tileBelow->isRevealed() == true)
											shade = tileBelow->getShade();
										else
											shade = SHADE_BLACK;

										calcWalkOffset(unitBelow, &walkOffset, isLocation);
										sprite->blitNShade(
												surface,
												posScreen.x + walkOffset.x - _spriteWidth_2,
												posScreen.y + walkOffset.y + 24,
												shade);

										if (unitBelow->getUnitFire() != 0)
										{
											sprite = _res->getSurfaceSet("SMOKE.PCK")->getFrame(4 + (_aniCycle >> 1u));
											//if (sprite != nullptr)
												sprite->blitNShade(
														surface,
														posScreen.x + walkOffset.x,
														posScreen.y + walkOffset.y + 24);
										}
									}
								}
							}
						}

// Draw SMOKE & FIRE
						if (_tile->isRevealed() == true)
						{
							std::string st;
							if (_tile->getFire() != 0) // check & draw fire first.
							{
								st       = "SMOKE.PCK"; // TODO: Breakout fire-sprites to their own SurfaceSet.
								spriteId =
								shade    = 0;
							}
							else if (_tile->getSmoke() != 0
								&& (hasUnit == false || _unit->getUnitFire() == 0))
							{
								st       = "SmokeCloud";
								spriteId = ((_tile->getSmoke() - 1) >> 2u) << 2u; //+ ResourcePack::SMOKE_OFFSET
								shade    = tileShade;
							}
							else
								spriteId = -1;

							if (spriteId != -1)
							{
								spriteId += ((_aniCycle >> 1u) + _tile->getAnimationOffset()) % 4;
								sprite = _res->getSurfaceSet(st)->getFrame(spriteId);
								//if (sprite != nullptr)
									sprite->blitNShade(
											surface,
											posScreen.x,
											posScreen.y, //+ _tile->getTerrainLevel(),
											shade);
							}
						}// end Smoke & Fire

// Draw pathPreview (arrows solid)
						if ((_previewSetting & PATH_ARROWS)
							&& _tile->getPreviewDir() != -1
							&& _tile->isRevealed(ST_WEST) == true) // what.
						{
							if (itZ > 0 && _tile->isFloored(tileBelow) == false)
							{
								sprite = _res->getSurfaceSet("Pathfinding")->getFrame(11);
								//if (sprite != nullptr)
									sprite->blitNShade(
											surface,
											posScreen.x,
											posScreen.y + 2,
											0, false,
											_tile->getPreviewColor());
							}

							sprite = _res->getSurfaceSet("Pathfinding")->getFrame(_tile->getPreviewDir());
							//if (sprite != nullptr)
								sprite->blitNShade(
										surface,
										posScreen.x,
										posScreen.y,
										0, false,
										_tile->getPreviewColor());
						}

// Draw Front Object
						if ((sprite = _tile->getSprite(O_CONTENT)) != nullptr)
						{
							switch (_tile->getMapData(O_CONTENT)->getBigwall())
							{
								case BIGWALL_EAST:
								case BIGWALL_SOUTH:
								case BIGWALL_E_S:
									sprite->blitNShade(
											surface,
											posScreen.x,
											posScreen.y - _tile->getMapData(O_CONTENT)->getOffsetY(),
											tileShade);
							}
						}

// Draw Cursor Front
						if (_selectorType != CT_NONE
							&& _battleSave->getBattleState()->getMouseOverToolbar() == false
							&& _selectorX >  itX - _selectorSize
							&& _selectorY >  itY - _selectorSize
							&& _selectorX <= itX
							&& _selectorY <= itY)
						{
							if (viewLevel > itZ)
							{
								sprite = _res->getSurfaceSet("CURSOR.PCK")->getFrame(5); // blue static box
								sprite->blitNShade(
										surface,
										posScreen.x,
										posScreen.y);
							}
							else if (viewLevel == itZ)
							{
								switch (_selectorType)
								{
									case CT_TARGET:
										if (hasUnit == true)
											spriteId = 7 + (_aniCycle >> 1u);	// yellow animated crosshairs
										else
											spriteId = 6;						// red static crosshairs
										break;

									default:
									case CT_CUBOID:
									case CT_PSI:
									case CT_LAUNCH:
									case CT_TOSS:
										if (hasUnit == true
											&& (_selectorType != CT_PSI
												|| ((_battle->getTacticalAction()->type == BA_PSICOURAGE
														&& _unit->getFaction() != FACTION_HOSTILE
														&& _unit != _battleSave->getSelectedUnit())
													|| (_battle->getTacticalAction()->type != BA_PSICOURAGE
														&& _unit->getFaction() != FACTION_PLAYER))))
										{
											spriteId = 3 + (_aniCycle & 1);	// yellow flashing box
										}
										else
											spriteId = 3;					// red static box
								}

								sprite = _res->getSurfaceSet("CURSOR.PCK")->getFrame(spriteId);
								sprite->blitNShade(
										surface,
										posScreen.x,
										posScreen.y);// + vertOffset);

// UFOExtender Accuracy
								// display adjusted accuracy value on crosshair (and more).
//								if (Options::battleUFOExtenderAccuracy == true) // -> one less condition to check
								switch (_selectorType)
								{
									case CT_TARGET:
									{
										// draw targetUnit overtop cursor's front if Tile is blacked-out.
										if (hasUnit == true && _tile->isRevealed() == false)
										{
											isLocation = isUnitAtTile(_unit, _tile);
											quadrant = getQuadrant(_unit, _tile, isLocation);
											sprite = _unit->getCache(quadrant);
											//if (sprite != nullptr)
											{
												calcWalkOffset(_unit, &walkOffset, isLocation);
												sprite->blitNShade(
														surface,
														posScreen.x + walkOffset.x - _spriteWidth_2,
														posScreen.y + walkOffset.y,
														tileShade);
											}
										}

										// TODO: Use stuff from ProjectileFlyBState::init()
										// as well as TileEngine::doTargetUnit() & TileEngine::doTargetTilepart()
										// to turn accuracy to 'red 0' if target is out of LoS/LoF.
										//
										// TODO: use Projectile::rangeAccuracy() as a static function.
										const BattleAction* const action (_battle->getTacticalAction());
										int accuracy;
										Uint8 color;

										bool zero;
										if (hasUnit == true
											&& _tile->getTileUnit() != _battleSave->getSelectedUnit())
										{
											const Position originVoxel (_te->getSightOriginVoxel(action->actor));
											Position scanVoxel;
											zero = _te->doTargetUnit(
																&originVoxel,
																_tile,
																&scanVoxel,
																action->actor) == false;
										}
										else
											zero = false;

										if (zero == false)
										{
											const RuleItem* const weaponRule (action->weapon->getRules());
											const int dist (TileEngine::distance(
																			Position(itX,itY,itZ),
																			action->actor->getPosition()));
											if (dist <= weaponRule->getMaxRange())
											{
												const int lowerLimit (weaponRule->getMinRange());
												int upperLimit;
												switch (action->type)
												{
													case BA_AIMEDSHOT:
														upperLimit = weaponRule->getAimRange();
														break;
													case BA_SNAPSHOT:
														upperLimit = weaponRule->getSnapRange();
														break;
													case BA_AUTOSHOT:
														upperLimit = weaponRule->getAutoRange();
														break;

													default:
														upperLimit = 200; // arbitrary.
												}

												accuracy = static_cast<int>(Round(action->actor->getAccuracy(*action) * 100.));
												if (dist > upperLimit)
												{
													accuracy -= (dist - upperLimit) * weaponRule->getDropoff();
													if (accuracy > 0)
														color = ACU_ORANGE;
													else
														zero = true;
												}
												else if (dist < lowerLimit)
												{
													accuracy -= (lowerLimit - dist) * weaponRule->getDropoff();
													if (accuracy > 0)
														color = ACU_ORANGE;
													else
														zero = true;
												}
												else
													color = ACU_GREEN;
											}
											else
												zero = true;
										}

										if (zero == true)
										{
											accuracy = 0;
											color = ACU_RED;
										}

										_numAccuracy->setValue(static_cast<unsigned>(accuracy));
										_numAccuracy->setColor(color);
										_numAccuracy->draw();
										_numAccuracy->blitNShade(
												surface,
												posScreen.x,
												posScreen.y);
										break;
									}

									case CT_TOSS:
									{
										BattleAction* const action (_battle->getTacticalAction());
										action->posTarget = Position(itX,itY,itZ);

										unsigned accuracy;
										Uint8 color;
										const Position
											originVoxel (_te->getOriginVoxel(*action)),
											targetVoxel (Position::toVoxelSpaceCentered( // TODO: conform this to ProjectileFlyBState (modifier keys) & Projectile::_targetVoxel
																					Position(itX,itY,itZ),
																					FLOOR_TLEVEL - _battleSave->getTile(action->posTarget)->getTerrainLevel()));
										if (hasFloor == true
											&& _te->validateThrow(
																*action,
																originVoxel,
																targetVoxel) == true)
										{
											accuracy = static_cast<unsigned int>(Round(_battleSave->getSelectedUnit()->getAccuracy(*action) * 100.));
											color = ACU_GREEN;
										}
										else
										{
											accuracy = 0;
											color = ACU_RED;
										}

										_numAccuracy->setValue(accuracy);
										_numAccuracy->setColor(color);
										_numAccuracy->draw();
										_numAccuracy->blitNShade(
												surface,
												posScreen.x,
												posScreen.y);
									}
								}

								switch (_selectorType)
								{
									case CT_PSI:
									case CT_LAUNCH:
									case CT_TOSS:
									{
										static const int cursorSprites[6u] {0,0,0,11,13,15};
										sprite = _res->getSurfaceSet("CURSOR.PCK")->getFrame(cursorSprites[_selectorType] + (_aniCycle >> 2u));
										sprite->blitNShade(
												surface,
												posScreen.x,
												posScreen.y);
									}
								}
							}
						}
						// end cursor front.

// Draw WayPoints if any on current Tile
						int
							offset_x (2),
							offset_y (2);
						unsigned wpVal (1u);

						for (std::vector<Position>::const_iterator
								i = _waypoints.begin();
								i != _waypoints.end();
								++i, ++wpVal)
						{
							if (*i == posField) // note that 2 BL waypoints can be at the same Position.
							{
								if (offset_x == 2 && offset_y == 2)
								{
									sprite = _res->getSurfaceSet("Targeter")->getFrame(0); // was "CURSOR.PCK" spriteId= 7
									sprite->blitNShade(
											surface,
											posScreen.x,
											posScreen.y);
								}

								_numWaypoint->setValue(wpVal);
								_numWaypoint->draw();
								_numWaypoint->blitNShade(
										surface,
										posScreen.x + offset_x,
										posScreen.y + offset_y);

								if (wpVal > 9u)	offset_x += 8;
								else			offset_x += 6;

								if (offset_x > 25)
								{
									offset_x = 2;
									offset_y += 6;
								}
							}
						}
						// end waypoints.

// Draw battlefield border-marks only on ground-level tiles
						if (_tile->getTileUnit() == nullptr
							&& (itZ == _battleSave->getGroundLevel()
								|| (itZ == 0 && _battleSave->getGroundLevel() == -1)))
						{
							if (   itX == 0
								|| itX == _battleSave->getMapSizeX() - 1
								|| itY == 0
								|| itY == _battleSave->getMapSizeY() - 1)
							{
								srfBorder->blitNShade(
										surface,
										posScreen.x + 14,
										posScreen.y + 31);
							}
						}
						// end border icon.

// Draw scanner dots
//						if (itZ == viewLevel && _battleSave->scannerDots().empty() == false)
//						{
//							const std::pair<int,int> dotTest (std::make_pair(
//																		posField.x,
//																		posField.y));
//							if (std::find(
//									_battleSave->scannerDots().begin(),
//									_battleSave->scannerDots().end(),
//									dotTest) != _battleSave->scannerDots().end())
//							{
//								srfBorder->blitNShade(
//										surface,
//										posScreen.x + 14,
//										posScreen.y + 30,
//										0, false, RED_i);
//							}
//						}
						// end scanner dots.
					}
					// is inside the Surface
				}
				// tile is Valid
			}
			// end Tiles_y looping. -> Tiles_z
		}
		// end Tiles_x looping. -> Tiles_y
	}
	// end Tiles_z looping. -> Tiles_x

	// Draw Bouncing Arrow over selected unit.
	if (_selectorType != CT_NONE
		&& (_battleSave->getSide() == FACTION_PLAYER
			|| _battleSave->getDebugTac() == true))
	{
		if ((_unit = _battleSave->getSelectedUnit()) != nullptr
			&& _unit->getPosition().z <= viewLevel)
		{
			switch (_unit->getUnitStatus())
			{
				case STATUS_STANDING:
				case STATUS_TURNING:
				{
					_camera->convertMapToScreen(
											_unit->getPosition(),
											&posScreen);
					posScreen += _camera->getMapOffset();

					posScreen.y += getTerrainLevel(
												_unit->getPosition(),
												_unit->getArmor()->getSize());
					posScreen.y += 21 - _unit->getHeight();

					if (_unit->getArmor()->getSize() == 2)
					{
						posScreen.y += 10;
						if (_unit->getFloatHeight() != 0)
							posScreen.y -= _unit->getFloatHeight() + 1;
					}

					posScreen.x += _spriteWidth_2;

//					const int phaseCycle (static_cast<int>(4. * std::sin(22.5 / static_cast<double>(_aniCycle + 1))));
					static const int phaseCycle[8u] {0,-3,3,-2,-3,-2,0,1};

					if (_unit->isKneeled() == true)
						_arrow_kneel->blitNShade(
								surface,
								posScreen.x - (_arrow_kneel->getWidth() >> 1u),
								posScreen.y -  _arrow_kneel->getHeight() - 4 - phaseCycle[_aniCycle]);
//								posScreen.y -  _arrow_kneel->getHeight() - 4 - phaseCycle);
					else
						_arrow->blitNShade(
								surface,
								posScreen.x - (_arrow->getWidth() >> 1u),
								posScreen.y -  _arrow->getHeight() + phaseCycle[_aniCycle]);
//								posScreen.y -  _arrow->getHeight() + phaseCycle);
				}
			}
		}
	}
	// end arrow.

	if (_battleSave->getPathfinding()->isPathPreviewed() == true
		|| Options::traceAI != 0)
	{
		_numWaypoint->setBordered(); // make a border for the pathPreview display

		for (int
				itZ = beginZ;
				itZ <= endZ;
				++itZ)
		{
			for (int
					itX = beginX;
					itX <= endX;
					++itX)
			{
				for (int
						itY = beginY;
						itY <= endY;
						++itY)
				{
					posField = Position(itX,itY,itZ);
					_camera->convertMapToScreen(posField, &posScreen);
					posScreen += _camera->getMapOffset();

					if (   posScreen.x > -_spriteWidth // only render pathPreview inside the screen-surface
						&& posScreen.x <  _spriteWidth + surface->getWidth()
						&& posScreen.y > -_spriteHeight
						&& posScreen.y <  _spriteHeight + surface->getHeight())
					{
						if ((_tile = _battleSave->getTile(posField)) != nullptr
							&& _tile->isRevealed() == true
							&& _tile->getPreviewDir() != -1)
						{
							int offset_y (-_tile->getTerrainLevel());

							if (_previewSetting & PATH_ARROWS) // arrows semi-transparent
							{
								tileBelow = _battleSave->getTile(posField + POS_BELOW);
								if (itZ > 0 && _tile->isFloored(tileBelow) == false)
								{
									sprite = _res->getSurfaceSet("Pathfinding")->getFrame(23);
									//if (sprite != nullptr)
										sprite->blitNShade(
												surface,
												posScreen.x,
												posScreen.y + 2,
												0, false,
												_tile->getPreviewColor());
								}

								sprite = _res->getSurfaceSet("Pathfinding")->getFrame(_tile->getPreviewDir() + 12);
								//if (sprite != nullptr)
									sprite->blitNShade(
											surface,
											posScreen.x,
											posScreen.y - offset_y,
											0, false,
											_tile->getPreviewColor());
							}

							if ((_previewSetting & PATH_TU_COST) && _tile->getPreviewTu() > -1)
							{
								int offset_x (2);
								if (_tile->getPreviewTu() > 9)
									offset_x += 2;

								if (_battleSave->getSelectedUnit() != nullptr
									&& _battleSave->getSelectedUnit()->getArmor()->getSize() > 1)
								{
									if (_previewSetting & PATH_ARROWS)
										offset_y += 1;
									else
										offset_y += 8;
								}

								_numWaypoint->setValue(static_cast<unsigned>(_tile->getPreviewTu()));
								_numWaypoint->draw();

								if (_previewSetting & PATH_ARROWS)
									_numWaypoint->blitNShade(
												surface,
												posScreen.x + 16 - offset_x,
												posScreen.y + 30 - offset_y);
								else
									_numWaypoint->blitNShade(
												surface,
												posScreen.x + 16 - offset_x,
												posScreen.y + 37 - offset_y,
												0, false,
												_tile->getPreviewColor());
							}
						}
					}
				}
			}
		}
		_numWaypoint->setBordered(false); // remove border for BL-wp's
	}
	// end Path Preview.

	if (_explosionInFOV == true) // check if hit or explosion animations
	{
/*		// big explosions cause the screen to flash as bright as possible before any explosions
		// are actually drawn. This causes everything to look like EGA for a single frame.
		if (_flashScreen == true)
		{
			Uint8 color;
			for (int x = 0, y = 0; x < surface->getWidth() && y < surface->getHeight();)
			{
//				surface->setPixelIterative(&x,&y, surface->getPixelColor(x,y) & 0xF0); // <- Volutar's, Lol good stuf.
				color = (surface->getPixelColor(x,y) / 16) * 16; // get the brightest color in each colorgroup.
				surface->setPixelIterative(&x,&y, color);
			}
		}
		else { */
		for (std::list<Explosion*>::const_iterator
				i = _explosions.begin();
				i != _explosions.end();
				++i)
		{
			_camera->convertVoxelToScreen(
									(*i)->getPosition(),
									&bullet);
			if ((spriteId = (*i)->getCurrentSprite()) > -1)
			{
				switch ((*i)->getExplosionType())
				{
					case ET_AOE: // Explosions, http://ufopaedia.org/index.php?title=X1.PCK
						sprite = _res->getSurfaceSet("X1.PCK")->getFrame(spriteId);
						sprite->blitNShade(
								surface,
								bullet.x - (sprite->getWidth()  >> 1u),
								bullet.y - (sprite->getHeight() >> 1u));
						break;

					case ET_BULLET: // Bullet-hits, http://ufopaedia.org/index.php?title=SMOKE.PCK
					case ET_TORCH:
						sprite = _res->getSurfaceSet("ProjectileHits")->getFrame(spriteId); // was "SMOKE.PCK" w/ different sprite-offsets.
						sprite->blitNShade(
								surface,
								bullet.x - 15,
								bullet.y - 15);
						break;

					case ET_MELEE_HIT:	// Melee success, http://ufopaedia.org/index.php?title=HIT.PCK
					case ET_PSI:		// Psiamp strikes
							sprite = _res->getSurfaceSet("HIT.PCK")->getFrame(spriteId);
							sprite->blitNShade(
									surface,
									bullet.x - 15,
									bullet.y - 25);
						break;

					case ET_MELEE_ATT: // Melee swing
							sprite = _res->getSurfaceSet("ClawTooth")->getFrame(spriteId);
							sprite->blitNShade(
									surface,
									bullet.x - 15,
									bullet.y - 25);
				}
			}
		}
//		}
	}
	surface->unlock();
}

/**
 * Draws a Soldier's rank-icon above its sprite on the Map.
 * @param unit		- pointer to a BattleUnit
 * @param offset_x	- how much to offset in the x-direction
 * @param offset_y	- how much to offset in the y-direction
 * @param tLevel	- true to add terrain-level, false if calcWalkOffset (default true)
 */
void Map::drawRankIcon( // private.
		const BattleUnit* const unit,
		int offset_x,
		int offset_y,
		bool tLevel)
{
	if (tLevel == true)
		offset_y += getTerrainLevel(
								unit->getPosition(),
								unit->getArmor()->getSize());

	switch (unit->getFatalsTotal())
	{
		case 0:
		{
			std::string solRank (unit->getRankString()); // eg. STR_COMMANDER -> RANK_COMMANDER
			solRank = "RANK" + solRank.substr(3u, solRank.length() - 3u);

			Surface* const sprite (_res->getSurface(solRank));
			if (sprite != nullptr)
				sprite->blitNShade(
								this,
								offset_x + 2,
								offset_y + 3);
			break;
		}

		default:
			_srfRookiBadge->blitNShade(		// background panel for red cross icon.
								this,
								offset_x + 2,
								offset_y + 3);

			_srfCross->blitNShade(			// small gray cross drawn RED.
								this,
								offset_x + 4,
								offset_y + 4,
								(_aniCycle << 1u),
								false, RED_i);
	}
}

/**
 * Checks if a southwesterly wall should suppress unit-sprite drawing.
 * @note When a unit moves east-west it can clip through a previously drawn wall.
 * @param tile6		- pointer to the tile west of current
 * @param tile5		- pointer to the tile southwest of current
 * @param unit		- pointer to BattleUnit not '_unit' (default nullptr)
 * @param halfRight	- pointer to store whether to draw halfRight only (used for large units) (default nullptr)
 * @return, true to allow drawing the unit's sprite
 */
bool Map::checkWest( // private.
		const Tile* const tile6,
		const Tile* const tile5,
		const BattleUnit* unit,
		bool* const halfRight) const
{
	bool ret;

	if (unit == nullptr) // '_unit' is Valid.
	{
		ret = tile6 == nullptr
		   || tile6->getTileUnit() != _unit;
	}
	else
		ret = false; // '_unit' is NOT Valid.
	//Log(LOG_INFO) << "ret[1] " << ret;

	if (ret == false)
		ret = (tile6->getMapData(O_CONTENT) == nullptr
				|| tile6->getMapData(O_CONTENT)->getBigwall() != BIGWALL_SOUTH)
			&& (tile5 == nullptr
				|| ((tile5->getMapData(O_NORTHWALL) == nullptr
						|| (tile5->getMapData(O_NORTHWALL)->getTuCostPart(MT_WALK) != 255
							&& tile5->getMapData(O_NORTHWALL)->isSlideDoor() == false)
						|| tile5->isSlideDoorOpen(O_NORTHWALL) == true)
					&& (tile5->getMapData(O_CONTENT) == nullptr
						|| (tile5->getMapData(O_CONTENT)->getBigwall() & 0x33) == 0))); // Block/NeSw/North/East.
	//Log(LOG_INFO) << "ret[2] " << ret;

	if (ret == false) // unit might actually be too far away from wall to clip despite above conditions - now don't let the floor clip it
	{
		if (unit == nullptr) unit = _unit;
		switch (unit->getUnitDirection())
		{
			case 1:
			case 2: ret = unit->getPosition() == unit->getStopPosition() || unit->getWalkPhase() == 0;
				break;
			case 5:
			case 6: ret = unit->getPosition() == unit->getStartPosition();
		}
		//Log(LOG_INFO) << ". ret[3] " << ret;

/*		if (halfRight != nullptr && unit->getArmor()->getSize() == 2)
		{
			if (ret == true)
			{
//				if (dir != 2) *halfRight = true; // could allow this. Maybe !=1 also ... need this:
				if (unit->getUnitDirection() != 2 && unit->getUnitDirection() != 6)
				{
					const Position pos (tile6->getPosition() + Position(1,0,0));
					const Tile
						* const tile (_battleSave->getTile(pos)),
						* const tileSouth (_battleSave->getTile(pos + Position(0,1,0)));
					if (!
						((tile->getMapData(O_CONTENT) == nullptr
							|| tile->getMapData(O_CONTENT)->getBigwall() != BIGWALL_SOUTH)
						&& (tileSouth == nullptr
							|| ((tileSouth->getMapData(O_NORTHWALL) == nullptr
									|| tileSouth->isSlideDoorOpen(O_NORTHWALL) == true
									|| (tileSouth->getMapData(O_NORTHWALL)->getTuCostPart(MT_WALK) != 255
										&& tileSouth->getMapData(O_NORTHWALL)->isSlideDoor() == false))
								&& (tileSouth->getMapData(O_CONTENT) == nullptr
									|| (tileSouth->getMapData(O_CONTENT)->getBigwall() & 0x3) == 0))))) // Block/NeSw.
						// All that causes clipping when the large unit moves out eastward from along the northern side
						// of an EW barrier but it's better than leaving a big hole in the 3rd quadrant as it moves out.
						// And anything is better than re-drawing tile-parts.
						//
						// oh God i redrew an eastwall up there ...
					{
						*halfRight = true; // but only if a wall is directly south
						Log(LOG_INFO) << ". . hRight[1] " << halfRight;
					}
				}
			}
			else
			{
				switch (unit->getUnitDirection())
				{
					case 1: // NOTE: Only dir= 2 (and dir= 6) have the half-ptr ...
					case 2:
						ret = true;
						Log(LOG_INFO) << ". . ret[4] " << ret;
						if (unit->getWalkPhase() != 0)
						{
							Log(LOG_INFO) << ". . hRight[2] " << halfRight;
							*halfRight = true;
						}
				}
			}
		} */
	}
	return ret;
}

/**
 * Checks if a northeasterly wall should suppress unit-sprite drawing.
 * @note When a unit moves north-south it can clip through a previously drawn wall.
 * @param tile0		- pointer to the tile north of current
// * @param tile1	- pointer to the tile northeast of current
 * @param unit		- pointer to BattleUnit not '_unit' (default nullptr)
 * @param halfLeft	- pointer to store whether to draw halfLeft only (used for large units) (default nullptr)
 * @return, true to allow drawing the unit's sprite
 */
bool Map::checkNorth( // private.
		const Tile* const tile0,
//		const Tile* const tile1,	// wait a sec, shouldn't have to check any tiles-eastward since
		const BattleUnit* unit,		// they draw after the whole unit.
		bool* const halfLeft) const
{
	bool ret;

	if (unit == nullptr) // '_unit' is Valid.
	{
		ret = tile0 == nullptr
		   || tile0->getTileUnit() != _unit;
	}
	else
		ret = false; // '_unit' is NOT Valid.

	if (ret == false)
		ret = (tile0->getMapData(O_CONTENT) == nullptr
				|| tile0->getMapData(O_CONTENT)->getBigwall() != BIGWALL_EAST);
//			&& (tile1 == nullptr
//				|| ((tile1->getMapData(O_WESTWALL) == nullptr
//						|| tile1->getMapData(O_WESTWALL)->getTuCostPart(MT_WALK) == 0 // <- darn those UFO-westwall-struts.
//						|| tile1->isSlideDoorOpen(O_WESTWALL) == true)
//					&& (tile1->getMapData(O_CONTENT) == nullptr
//						|| (tile1->getMapData(O_CONTENT)->getBigwall() & 0xb) == 0)))); // Block/NeSw/West.

	if (ret == false) // unit might actually be too far away from wall to clip despite above conditions - now don't let the floor clip it
	{
		if (unit == nullptr) unit = _unit;
		switch (unit->getUnitDirection())
		{
			case 0:
			case 1: ret = unit->getPosition() == unit->getStartPosition();
				break;
			case 4:
			case 5: ret = unit->getPosition() == unit->getStopPosition() || unit->getWalkPhase() == 0;
		}

/*		if (ret == false
			&& halfLeft != nullptr && unit->getArmor()->getSize() == 2)
		{
			switch (unit->getUnitDirection())
			{
				case 4: // NOTE: Only dir= 4 (and dir= 0) have the half-ptr ...
				case 5:
					*halfLeft =
					ret = true;
			}
		} */
	}
	return ret;
}

/**
 * Replaces a specified quantity of colors in this Surface's palette.
 * @param colors		- pointer to the set of colors
 * @param firstcolor	- offset of the first color to replace (default 0)
 * @param ncolors		- amount of colors to replace (default 256)
 */
void Map::setPalette( // override
		SDL_Color* const colors,
		int firstcolor,
		int ncolors)
{
	Surface::setPalette(colors, firstcolor, ncolors);

	for (std::vector<MapDataSet*>::const_iterator
			i  = _battleSave->getBattleDataSets()->begin();
			i != _battleSave->getBattleDataSets()->end();
			++i)
	{
		(*i)->getSurfaceset()->setPalette(colors, firstcolor, ncolors);
	}

	_hiddenScreen->setPalette(colors, firstcolor, ncolors);
	_hiddenScreen->setBackground(_res->getSurface("Diehard"));
	_hiddenScreen->initText(
						_res->getFont("FONT_BIG"),
						_res->getFont("FONT_SMALL"),
						_game->getLanguage());
	_hiddenScreen->setText(_game->getLanguage()->getString("STR_HIDDEN_MOVEMENT"));
}

/**
 * Handles mouse-presses on this Map.
 * @param action	- pointer to an Action
 * @param state		- State that the ActionHandlers belong to
 */
void Map::mousePress(Action* action, State* state)
{
	InteractiveSurface::mousePress(action, state);
	_camera->mousePress(action);
}

/**
 * Handles mouse-releases on this Map.
 * @param action	- pointer to an Action
 * @param state		- State that the ActionHandlers belong to
 */
void Map::mouseRelease(Action* action, State* state)
{
	InteractiveSurface::mouseRelease(action, state);
	_camera->mouseRelease(action);
}

/**
 * Handles mouse-over events on this Map.
 * @param action	- pointer to an Action
 * @param state		- State that the ActionHandlers belong to
 */
void Map::mouseOver(Action* action, State* state)
{
	InteractiveSurface::mouseOver(action, state);
	_camera->mouseOver(action);

	_mX = static_cast<int>(action->getAbsoluteMouseX());
	_mY = static_cast<int>(action->getAbsoluteMouseY());
	refreshSelectorPosition();
}

/**
 * Finds the current mouse-position x/y on this Map.
 * @param pos - reference to the mouse-position
 */
void Map::findMousePointer(Position& pos) const
{
	pos.x = _mX;
	pos.y = _mY;
	pos.z = 0;
}

/**
 * Handles keyboard-presses on this Map.
 * @param action	- pointer to an Action
 * @param state		- State that the ActionHandlers belong to
 */
void Map::keyboardPress(Action* action, State* state)
{
	InteractiveSurface::keyboardPress(action, state);
	_camera->keyboardPress(action);
}

/**
 * Handles keyboard-releases on this Map.
 * @param action	- pointer to an Action
 * @param state		- State that the ActionHandlers belong to
 */
void Map::keyboardRelease(Action* action, State* state)
{
	InteractiveSurface::keyboardRelease(action, state);
	_camera->keyboardRelease(action);
}

/**
 * Animates any/all Tiles and BattleUnits that require regular animation and
 * steps the fuse-color.
 * @note There are 8 ticks per cycle [0..7].
 * @param redraw - true to redraw the battlefield (default true)
 */
void Map::animateMap(bool redraw)
{
 	if (++_aniCycle == 8) _aniCycle = 0;

	for (size_t
			i = 0u;
			i != _battleSave->getMapSizeXYZ();
			++i)
	{
		_battleSave->getTiles()[i]->animateTile();
	}

	for (std::vector<BattleUnit*>::const_iterator	// animate certain units
			i = _battleSave->getUnits()->begin();	// eg. large flying units have a propulsion animation
			i != _battleSave->getUnits()->end();
			++i)
	{
		if ((*i)->getUnitTile() != nullptr
			&& (*i)->getArmor()->getConstantAnimation() == true)
		{
			(*i)->setCacheInvalid();
			cacheUnitSprite(*i);
		}
	}

	if (--_fuseColor == 15u) _fuseColor = 31u;
	if (redraw == true) _redraw = true;
}

/**
 * Sets the cuboid-selector to the current mouse-position.
 */
void Map::refreshSelectorPosition()
{
	const int
		pre_X (_selectorX),
		pre_Y (_selectorY);

	_camera->convertScreenToMap(
							_mX,
							_mY + (_spriteHeight >> 2u),
							&_selectorX,
							&_selectorY);

	if (pre_X != _selectorX || pre_Y != _selectorY)
		_redraw = true;
}


/**
 * Gets the position of the cuboid-selector.
 * @param pos - reference to a Position
 */
void Map::getSelectorPosition(Position& pos) const
{
	pos.x = _selectorX;
	pos.y = _selectorY;
	pos.z = _camera->getViewLevel();
}

/**
 * Gets if a Tile is a/the location of a specified unit.
 * @param unit - pointer to a unit
 * @param tile - pointer to a tile
 * @return, true if true location
 */
bool Map::isUnitAtTile(
		const BattleUnit* const unit,
		const Tile* const tile) const // private.
{
	if (unit->getUnitTile() == tile)
		return true;

	if (unit->getArmor()->getSize() == 2)
	{
		const Position& posUnit (unit->getPosition());
		const Position& posTile (tile->getPosition());
		if (   posTile + POS_WEST      == posUnit
			|| posTile + POS_NORTH     == posUnit
			|| posTile + POS_NORTHWEST == posUnit)
		{
			return true;
		}
	}
	return false;
}

/**
 * Gets a specified unit's quadrant for drawing.
 * @param unit       - pointer to a unit
 * @param tile       - pointer to a tile
 * @param isLocation - true if real location; false if transient
 * @return, quadrant
 */
size_t Map::getQuadrant( // private.
		const BattleUnit* const unit,
		const Tile* const tile,
		bool isLocation) const
{
	Position
		posTile (tile->getPosition()),
		posUnit (unit->getPosition());

	if (isLocation == true //unit->getUnitStatus() == STATUS_STANDING ||
		|| unit->getVerticalDirection() != 0)
	{
		return static_cast<size_t>(posTile.x - posUnit.x
							   + ((posTile.y - posUnit.y) << 1u));
	}

	int dir (unit->getUnitDirection());
	if (posUnit == unit->getStopPosition())
		dir = (dir + 4) % 8;

	Position posVect;
	Pathfinding::directionToVector(dir, &posVect);
	posUnit += posVect;

	return static_cast<size_t>(posTile.x - posUnit.x
						   + ((posTile.y - posUnit.y) << 1u));
}

/**
 * Calculates the screen-offset of a unit-sprite when it is moving between tiles.
 * @param unit        - pointer to a BattleUnit
 * @param offset      - pointer to the Position that will be the calculation result
 * @param isLocation  - true if there's a unit-tile link for the current location; false if transient
// * @param shadeOffset - point to color-offset
 */
void Map::calcWalkOffset( // private.
		const BattleUnit* const unit,
		Position* const offset,
		bool isLocation) const
//		int* shadeOffset // Yankes addition: shadeOffset
{
	*offset = Position(0,0,0);

	switch (unit->getUnitStatus())
	{
		case STATUS_WALKING:
		case STATUS_FLYING:
		{
			static const int
				offsetX[8u] {1, 1, 1, 0,-1,-1,-1, 0},
				offsetY[8u] {1, 0,-1,-1,-1, 0, 1, 1},

				offsetFalseX[8u] {16, 32, 16,  0,-16,-32,-16,  0}, // for duplicate drawing units from their transient
				offsetFalseY[8u] {-8,  0,  8, 16,  8,  0, -8,-16}, // destination & last positions. See UnitWalkBState.
				offsetFalseVert (24);
			const int
				dir (unit->getUnitDirection()),			// 0..7
				dirVert (unit->getVerticalDirection()),	// 0= none, 8= up, 9= down
				armorSize (unit->getArmor()->getSize());
			int
				walkPhase (unit->getWalkPhaseTrue()),
				halfPhase (unit->getWalkPhaseHalf()),
				fullPhase (unit->getWalkPhaseFull());

//			if (shadeOffset != nullptr) // new Yankes' code.
//			{
//				*shadeOffset = walkPhase;
//				if (fullPhase != 16)
//					*shadeOffset *= 2;
//			}

			const bool start (walkPhase < halfPhase);
			if (dirVert == 0)
			{
				if (start == true)
				{
					offset->x =  walkPhase * offsetX[dir] * 2 + ((isLocation == true) ? 0 : -offsetFalseX[dir]);
					offset->y = -walkPhase * offsetY[dir]     + ((isLocation == true) ? 0 : -offsetFalseY[dir]);
				}
				else
				{
					offset->x =  (walkPhase - fullPhase) * offsetX[dir] * 2 + ((isLocation == true) ? 0 : offsetFalseX[dir]);
					offset->y = -(walkPhase - fullPhase) * offsetY[dir]     + ((isLocation == true) ? 0 : offsetFalseY[dir]);
				}
			}

			// if unit is between tiles interpolate its terrain level (y-adjustment).
			const int
				posStartZ (unit->getStartPosition().z),
				posStopZ  (unit->getStopPosition().z);
			int
				levelStart,
				levelStop;

			if (start == true)
			{
				switch (dirVert)
				{
					case Pathfinding::DIR_UP:
						offset->y += (isLocation == true) ? 0 : offsetFalseVert;
						break;
					case Pathfinding::DIR_DOWN:
						offset->y += (isLocation == true) ? 0 : -offsetFalseVert;
				}

				levelStop = getTerrainLevel(
										unit->getStopPosition(),
										armorSize);

				if (posStartZ > posStopZ)		// going down a level so 'levelStop' 0 becomes +24 (-8 becomes 16)
					levelStop += 24 * (posStartZ - posStopZ);
				else if (posStartZ < posStopZ)	// going up a level so 'levelStop' 0 becomes -24 (-8 becomes -16)
					levelStop = -24 * (posStopZ - posStartZ) + std::abs(levelStop);

				levelStart = getTerrainLevel(
										unit->getPosition(),
										armorSize);
				offset->y += ((levelStart * (fullPhase - walkPhase)) / fullPhase) + ((levelStop * walkPhase) / fullPhase);
				if (isLocation == false && dirVert == 0)
				{
					if (posStartZ > posStopZ)
						offset->y -= offsetFalseVert;
					else if (posStartZ < posStopZ)
						offset->y += offsetFalseVert;
				}
			}
			else
			{
				switch (dirVert)
				{
					case Pathfinding::DIR_UP:
						offset->y += (isLocation == true) ? 0 : -offsetFalseVert;
						break;
					case Pathfinding::DIR_DOWN:
						offset->y += (isLocation == true) ? 0 : offsetFalseVert;
				}

				levelStart = getTerrainLevel(
										unit->getStartPosition(),
										armorSize);

				if (posStartZ > posStopZ)		// going down a level so 'levelStart' 0 becomes -24 (-8 becomes -32)
					levelStart -= 24 * (posStartZ - posStopZ);
				else if (posStartZ < posStopZ)	// going up a level so 'levelStart' 0 becomes +24 (-8 becomes 16)
					levelStart =  24 * (posStopZ - posStartZ) - std::abs(levelStart);

				levelStop = getTerrainLevel(
										unit->getStopPosition(),
										armorSize);
				offset->y += ((levelStart * (fullPhase - walkPhase)) / fullPhase) + ((levelStop * walkPhase) / fullPhase);

				if (isLocation == false && dirVert == 0)
				{
					if (posStartZ > posStopZ)
						offset->y += offsetFalseVert;
					else if (posStartZ < posStopZ)
						offset->y -= offsetFalseVert;
				}
			}
			break;
		}

		default: // standing, Etc.
			offset->y += getTerrainLevel(
									unit->getPosition(),
									unit->getArmor()->getSize());
	}
}

/**
 * Terrainlevel goes from 0 to -24 (bottom to top).
 * @note For a large sized unit pick the highest terrain level which is the
 * lowest number.
 * @param pos		- reference to a Position
 * @param unitSize	- size of the unit at @a pos
 * @return, terrain height
 */
int Map::getTerrainLevel( // private.
		const Position& pos,
		int unitSize) const
{
	int
		lowLevel (0),
		lowTest;

	for (int
			x = 0;
			x != unitSize;
			++x)
	{
		for (int
				y = 0;
				y != unitSize;
				++y)
		{
			lowTest = _battleSave->getTile(pos + Position(x,y,0))->getTerrainLevel();
			if (lowTest < lowLevel)
				lowLevel = lowTest;
		}
	}
	return lowLevel;
}

/**
 * Sets the 3D selector-type.
 * @param type		- SelectorType (Map.h)
 * @param length	- size of the cursor (default 1)
 */
void Map::setSelectorType(
		SelectorType type,
		int length)
{
	switch (_selectorType = type)
	{
		case CT_CUBOID:
			_selectorSize = length;
			break;

		default:
//		case CT_NONE:
//		case CT_TARGET:
//		case CT_PSI:
//		case CT_LAUNCH:
//		case CT_TOSS:
			_selectorSize = 1;
	}
}

/**
 * Gets the 3D selector-type.
 * @return, SelectorType (Map.h)
 */
SelectorType Map::getSelectorType() const
{
	return _selectorType;
}

/**
 * Checks all units for need to be redrawn and if so updates their sprite(s).
 */
void Map::cacheUnitSprites()
{
	for (std::vector<BattleUnit*>::const_iterator
			i  = _battleSave->getUnits()->begin();
			i != _battleSave->getUnits()->end();
			++i)
	{
		cacheUnitSprite(*i);
	}
}

/**
 * Checks if a BattleUnit needs to be redrawn and if so updates its sprite(s).
 * @param unit - pointer to a BattleUnit
 */
void Map::cacheUnitSprite(BattleUnit* const unit)
{
	if (unit->isCacheInvalid() == true)
	{
		UnitSprite* const sprite (new UnitSprite(
											_spriteWidth << 1u,
											_spriteHeight));
		sprite->setPalette(this->getPalette());

		const size_t quadrants (static_cast<size_t>(unit->getArmor()->getSize() * unit->getArmor()->getSize()));
		for (size_t
				i = 0u;
				i != quadrants;
				++i)
		{
			Surface* cache (unit->getCache(i));
			if (cache == nullptr)
			{
				cache = new Surface(
								_spriteWidth << 1u,
								_spriteHeight);
				cache->setPalette(this->getPalette());
			}

			sprite->setBattleUnit(unit, i);

			if (i == 0u)
			{
				const BattleItem
					* const rtItem (unit->getItem(ST_RIGHTHAND)),
					* const ltItem (unit->getItem(ST_LEFTHAND));

				if (rtItem != nullptr && rtItem->getRules()->isFixed() == false)
					sprite->setBattleItRH(rtItem);
				else
					sprite->setBattleItRH();

				if (ltItem != nullptr && ltItem->getRules()->isFixed() == false)
					sprite->setBattleItLH(ltItem);
				else
					sprite->setBattleItLH();
			}

			sprite->setSurfaces(
							_res->getSurfaceSet(unit->getArmor()->getSpriteSheet()),
							_res->getSurfaceSet("HANDOB.PCK"),
							_res->getSurfaceSet("HANDOB2.PCK"));
			sprite->setSpriteCycle(_aniCycle);

			cache->clear();
			sprite->blit(cache);

			unit->setCache(cache, i);
		}
		delete sprite;
	}
}

/**
 * Puts a Projectile on this Map.
 * @param projectile - projectile to place (default nullptr)
 */
void Map::setProjectile(Projectile* const projectile)
{
	if ((_projectile = projectile) != nullptr) //&& Options::battleSmoothCamera == true)
		_bulletStart = true;
}

/**
 * Gets the current Projectile.
 * @return, pointer to Projectile or nullptr
 */
Projectile* Map::getProjectile() const
{
	return _projectile;
}

/**
 * Gets a list of Explosions.
 * @return, pointer to a list of explosions to display
 */
std::list<Explosion*>* Map::getExplosions()
{
	return &_explosions;
}

/**
 * Gets the Camera.
 * @return, pointer to Camera
 */
Camera* Map::getCamera()
{
	return _camera;
}

/**
 * Timers only work on Surfaces so this has to be passed to the Camera.
 */
void Map::scrollMouse()
{
	_camera->scrollMouse();
}

/**
 * Timers only work on Surfaces so this has to be passed to the Camera.
 */
void Map::scrollKey()
{
	_camera->scrollKey();
}

/**
 * Gets a list of waypoint-positions on this Map.
 * @return, pointer to a vector of Positions
 */
std::vector<Position>* Map::getWaypoints()
{
	return &_waypoints;
}

/**
 * Sets the mouse-buttons' pressed state.
 * @param button	- index of the button
 * @param pressed	- the state of the button
 */
void Map::setButtonsPressed(
		Uint8 btn,
		bool pressed)
{
	setButtonPressed(btn, pressed);
}

/**
 * Sets the unit-dying flag.
 * @note This reveals the dying unit during Hidden Movement.
 * @param flag - true if a unit is dying (default true)
 */
void Map::setUnitDying(bool flag)
{
	_unitDying = flag;
}

/**
 * Special handling for setting the width of the Map viewport.
 * @param width - the new base screen width
 */
void Map::setWidth(int width) // override
{
	Surface::setWidth(width);
	_hiddenScreen->setX(_hiddenScreen->getX() + (width - getWidth()) >> 1u);
}

/**
 * Special handling for setting the height of the Map viewport.
 * @param height - the new base screen height
 */
void Map::setHeight(int height) // override
{
	Surface::setHeight(height);

	_playableHeight = height - _toolbarHeight;
	if (_playableHeight < 200)
		height = _playableHeight;
	else
		height = 200;

	_hiddenScreen->setHeight(height);
	_hiddenScreen->setY((_playableHeight - _hiddenScreen->getHeight()) >> 1u);
}

/**
 * Gets the hidden-movement screen's vertical position.
 * @return, the vertical position
 *
int Map::getHiddenY() const
{
	return _hiddenScreen->getY();
} */

/**
 * Gets the toolbar-height.
 * @return, toolbar-panel height
 */
int Map::getToolbarHeight() const
{
	return _toolbarHeight;
}

/**
 * Gets the toolbar-width.
 * @return, toolbar-panel width
 *
int Map::getToolbarWidth() const
{
	return _iconWidth;
} */

/**
 * Gets the angle (left/right balance) of a sound-effect based on a map-position.
 * @param pos - reference to the map-position to calculate a sound-angle from
 * @return, the angle of the sound (360 = 0 degrees center)
 */
int Map::getSoundAngle(const Position& pos) const
{
	const int midPoint (getWidth() >> 1u);

	Position screenPos;
	_camera->convertMapToScreen(pos, &screenPos);

	// cap the position to the screen edges relative to the center,
	// negative values indicating a left-shift, and positive values shifting to the right.
	screenPos.x = Vicegrip(screenPos.x + _camera->getMapOffset().x - midPoint, -midPoint, midPoint);

	// Convert the relative distance left or right to a relative angle off-center.
	// Since Mix_SetPosition() uses modulo 360 can't feed it a negative number so add 360.
	// The integer-factor below is the allowable maximum deflection from center.
	return (screenPos.x * 35 / midPoint) + 360;
}

/**
 * Resets the camera smoothing bool.
 *
void Map::resetCameraSmoothing()
{
	_smoothingEngaged = false;
} */

/**
 * Sets the "explosion flash" bool.
 * @param flash - true to render the screen in EGA this frame
 */
void Map::setBlastFlash(bool flash)
{
	_flashScreen = flash;
}

/**
 * Checks if the screen is still being rendered in EGA.
 * @return, true if still in EGA mode
 */
bool Map::getBlastFlash() const
{
	return _flashScreen;
}

/**
 * Sets whether to redraw this Map or not.
 * @param noDraw - true to stop the battlefield from drawing (default true)
 */
void Map::setNoDraw(bool noDraw)
{
	_noDraw = noDraw;
}

/**
 * Gets if the Hidden Movement screen is displayed or not.
 * @return, true if hidden
 */
bool Map::getMapHidden() const
{
	return _mapIsHidden;
}

/**
 * Gets the SavedBattleGame.
 * @return, pointer to SavedBattleGame
 */
SavedBattleGame* Map::getBattleSave() const
{
	return _battleSave;
}

/**
 * Sets the BattlescapeGame.
 * @param battle - pointer to BattlescapeGame
 */
void Map::setBattleGame(BattlescapeGame* const battle)
{
	_battle = battle;
}

/**
 * Sets this Map to remain revealed because there's a duration-type action going down.
 * @param reveal - true if there is waypoint/missile/autoshot action in progress (default true)
 */
void Map::setReveal(bool reveal)
{
	_reveal = reveal;
}

/**
 * Sets whether to allow drawing any Projectile on this Map.
 * @param show - true to show the projectile (default true)
 */
void Map::showProjectile(bool show)
{
	_showProjectile = show;
}

}
