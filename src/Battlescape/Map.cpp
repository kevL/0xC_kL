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

#include "Map.h"

//#define _USE_MATH_DEFINES
//#include <cmath>
//#include <cstddef> // nullptr (for NB code-assistant only)
//#include <fstream>

#include "../fmath.h"

#include "BattlescapeMessage.h"
#include "BattlescapeState.h"
#include "Camera.h"
#include "Explosion.h"
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

/*
Map origin is top corner (NW corner).
- x-axis goes downright (width of the Map) eastward
- y-axis goes downleft (length of the Map) southward
- Z axis goes up (height of the Map) upward

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
 * @param playableHeight	- current visible Map height
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
		_aniFrame(0),
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
		_res(game->getResourcePack()),
		_fuseColor(31),
		_tile(nullptr),
		_unit(nullptr)
{
	_iconWidth = _game->getRuleset()->getInterface("battlescape")->getElement("icons")->w;
	_iconHeight = _game->getRuleset()->getInterface("battlescape")->getElement("icons")->h;

	if (Options::traceAI == true) // turn everything on to see the markers.
		_previewSetting = PATH_FULL;
	else
		_previewSetting	= Options::battlePreviewPath;

	_spriteWidth = _res->getSurfaceSet("BLANKS.PCK")->getFrame(0)->getWidth();
	_spriteHeight = _res->getSurfaceSet("BLANKS.PCK")->getFrame(0)->getHeight();
	_spriteWidth_2 = _spriteWidth / 2;

	_camera = new Camera(
					_spriteWidth,
					_spriteHeight,
					_battleSave->getMapSizeX(),
					_battleSave->getMapSizeY(),
					_battleSave->getMapSizeZ(),
					this,
					playableHeight);

	int hiddenHeight;
	if (playableHeight < 200)
		hiddenHeight = playableHeight;
	else
		hiddenHeight = 200;
	_hiddenScreen = new BattlescapeMessage(320, hiddenHeight); // "Hidden Movement..." screen
	_hiddenScreen->setX(_game->getScreen()->getDX());
	_hiddenScreen->setY(_game->getScreen()->getDY());
	_hiddenScreen->setTextColor(static_cast<Uint8>(_game->getRuleset()->getInterface("battlescape")->getElement("messageWindows")->color));

	_scrollMouseTimer = new Timer(SCROLL_INTERVAL);
	_scrollMouseTimer->onTimer((SurfaceHandler)& Map::scrollMouse);

	_scrollKeyTimer = new Timer(SCROLL_INTERVAL);
	_scrollKeyTimer->onTimer((SurfaceHandler)& Map::scrollKey);

	_camera->setScrollTimers(
						_scrollMouseTimer,
						_scrollKeyTimer);

	_numAccuracy = new NumberText(12,6);
	_numAccuracy->setBordered();

	_numExposed = new NumberText(12,6);

	_numWaypoint = new NumberText(12,6);

	_srfRookiBadge = _res->getSurface("RANK_ROOKIE");
	_srfCross = _res->getSurfaceSet("SCANG.DAT")->getFrame(11);
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
}

/**
 * Initializes the Map.
 */
void Map::init()
{
	_numAccuracy->setPalette(getPalette());
	_numExposed->setPalette(getPalette());
	_numWaypoint->setPalette(getPalette());

	static const Uint8
		f (ORANGE),	// Fill
		b (BLACK);	// Border
	static const Uint8 pixels_stand[81u] { 0, 0, b, b, b, b, b, 0, 0,
										   0, 0, b, f, f, f, b, 0, 0,
										   0, 0, b, f, f, f, b, 0, 0,
										   b, b, b, f, f, f, b, b, b,
										   b, f, f, f, f, f, f, f, b,
										   0, b, f, f, f, f, f, b, 0,
										   0, 0, b, f, f, f, b, 0, 0,
										   0, 0, 0, b, f, b, 0, 0, 0,
										   0, 0, 0, 0, b, 0, 0, 0, 0 };
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

	static const Uint8 pixels_kneel[81u] { 0, 0, 0, 0, 0, 0, 0, 0, 0,
										   0, 0, 0, 0, b, 0, 0, 0, 0,
										   0, 0, 0, b, f, b, 0, 0, 0,
										   0, 0, b, f, f, f, b, 0, 0,
										   0, b, f, f, f, f, f, b, 0,
										   b, f, f, f, f, f, f, f, b,
										   b, b, b, f, f, f, b, b, b,
										   0, 0, b, f, f, f, b, 0, 0,
										   0, 0, b, b, b, b, b, 0, 0 };

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
	if (_noDraw == false) // don't draw if MiniMap is open. Or if Inventory is open.
	{
	// removed setting this here and in BattlescapeGame::handleState(),
	// Camera::scrollXY(), ProjectileFlyBState::think() x2.
//	if (_redraw == false) return;
//	_redraw = false;

	// Normally call for a Surface::draw();
	// but don't clear the background with color 0, which is transparent
	// (aka black) -- use color 15 because that actually corresponds to the
	// color you DO want in all variations of the xcom palettes.
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
//			tile = _battleSave->getTile(Position::toTileSpace(_projectile->getPosition()));
//			if (tile != nullptr
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
//				tile = _battleSave->getTile(Position::toTileSpace((*i)->getPosition()));
//				if (tile != nullptr
//					&& (tile->getTileVisible() == true
//						|| (tile->getTileUnit() != nullptr
//							&& tile->getTileUnit()->getUnitVisible() == true)
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


		static bool delayHide;

		if (_battleSave->getSelectedUnit() == nullptr
			|| _battleSave->getSelectedUnit()->getUnitVisible() == true
			|| _unitDying == true
			|| _explosionInFOV == true
			|| _projectileInFOV == true
			|| _reveal == true // stop flashing the Hidden Movement screen between waypoints and/or autoshots.
			|| _battleSave->getDebugTac() == true)
		{
			// REVEAL //
			delayHide = true;
			_mapIsHidden = false;
			drawTerrain(this);
		}
		else
		{
			// HIDE //
			if (delayHide == true)
			{
				delayHide = false;
				SDL_Delay(369u);
			}

			_mapIsHidden = true;
			_hiddenScreen->blit(this);
		}
	}
}

/**
 * Draws the battlefield.
 * @note Keep this function as optimised as possible - it's big and needs to be
 * fast so minimize overhead of function calls. Etc.
 * @param surface - the Surface on which to draw the entire battlefield
 */
void Map::drawTerrain(Surface* const surface) // private.
{
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

		// convert bullet position from voxel-space to tile-space
		bulletLowX  >>= 4;
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
						|| action->weapon->getAmmoItem() == nullptr // unless its shotgun pellets.
						|| action->weapon->getAmmoItem()->getRules()->getShotgunPellets() == 0)
					{
						_camera->centerOnPosition(
												Position(
													bulletLowX,
													bulletLowY,
													bulletHighZ),
												false);
					}
//					_camera->convertVoxelToScreen(_projectile->getPosition(), &bullet);
				}

				const bool offScreen_final (_camera->isOnScreen(posFinal) == false);
				if (offScreen_final == true										// moved here from TileEngine::reactionShot() because this is the
					&& action->actor->getFaction() != _battleSave->getSide())	// accurate position of the bullet-shot-actor's Camera mapOffset.
				{
					std::map<int, Position>* const rfShotPos (_battleSave->getTileEngine()->getRfShooterPositions());
					rfShotPos->insert(std::pair<int, Position>(
															action->actor->getId(),
															_camera->getMapOffset()));
				}

				if (offScreen_final == true
					|| ((_projectile->getThrowItem() != nullptr
							|| action->weapon->getRules()->getArcingShot() == true)
						&& TileEngine::distance(
											action->actor->getPosition(),
											posFinal) > DIST_ARC_SMOOTH)) // no smoothing unless throw > 8 tiles
				{
					_smoothingEngaged = true;
					_camera->setPauseAfterShot();
				}
			}
			else if (_smoothingEngaged == true)
				_camera->jumpXY(
							surface->getWidth() / 2 - bullet.x,
							_playableHeight / 2 - bullet.y);

			if (_smoothingEngaged == true
				|| posFinal.z != action->actor->getPosition().z
				|| _projectile->getThrowItem() != nullptr
				|| action->weapon->getRules()->getArcingShot() == true)
			{
				const int posBullet_z ((_projectile->getPosition().z) / 24);
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
						_camera->jumpXY(surface->getWidth(), 0);
						enough = false;
					}
					else if (bullet.x > surface->getWidth())
					{
						_camera->jumpXY(-surface->getWidth(), 0);
						enough = false;
					}
					else if (bullet.y < 0)
					{
						_camera->jumpXY(0, _playableHeight);
						enough = false;
					}
					else if (bullet.y > _playableHeight)
					{
						_camera->jumpXY(0, -_playableHeight);
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
	else
		_smoothingEngaged = false; // no projectile OR explosions-waiting

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
							surface->getWidth() + _spriteWidth,
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

	beginY -= (viewLevel * 2);
	beginX -= (viewLevel * 2);
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
		frame,
		tileShade,
		shade,
		aniOffset,
		quadrant;	// The quadrant is 0 for small units; large units have quadrants 1,2 & 3 also; describes		0|1
					// the relative x/y Position of the unit's primary quadrant vs. the current tile's Position.	2|3
	bool
		hasUnit, // these denote characteristics of 'tile' as in the current Tile of the loop.
		hasFloor,
		hasObject,
		trueLoc;


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
				posField = Position(itX,itY,itZ);
				_camera->convertMapToScreen(posField, &posScreen);
				posScreen += _camera->getMapOffset();

				if (   posScreen.x > -_spriteWidth // only render cells that are inside the surface (ie viewport ala player's monitor)
					&& posScreen.x <  _spriteWidth + surface->getWidth()
					&& posScreen.y > -_spriteHeight
					&& posScreen.y <  _spriteHeight + surface->getHeight())
				{
					_tile = _battleSave->getTile(posField);
					if (_tile == nullptr)
						continue;

					if (itZ != 0)
						tileBelow = _battleSave->getTile(posField + Position(0,0,-1));
					else
						tileBelow = nullptr;

					if (_tile->isRevealed(ST_CONTENT) == true)
						tileShade = _tile->getShade();
					else
						tileShade = SHADE_BLACK;

					_unit = _tile->getTileUnit();

					hasUnit = _unit != nullptr
						  && (_unit->getUnitVisible() == true || _battleSave->getDebugTac() == true);
					hasFloor =
					hasObject = false;

// Draw Floor
					sprite = _tile->getSprite(O_FLOOR);
					if (sprite != nullptr)
					{
						hasFloor = true;
						sprite->blitNShade(
								surface,
								posScreen.x,
								posScreen.y - _tile->getMapData(O_FLOOR)->getYOffset(),
								tileShade);

						// kL_begin #1 of 3:
						// This ensures the rankIcon isn't half-hidden by a floor above & west of soldier. (TODO: floor-tiles etc. further
						// west-north also hide rankIcon if curTerrainLevel < 0) ... unless there's also a floor directly above soldier.
						// Special case: crazy battleship tile+half floors; so check for content object diagonal wall directly above soldier also.
						// Also, make sure the rankIcon isn't half-hidden by a westwall directly above the soldier.
						//
						// bleh. stupid Map ... FIXED.
/*						if (itZ > 0 && itX < endX)
						{
							const Tile* const tileEast (_battleSave->getTile(posField + Position(1,0,0)));
							if (tileEast != nullptr // why.
								&& tileEast->getSprite(O_FLOOR) == nullptr
								&& (tileEast->getMapData(O_OBJECT) == nullptr // special case ->
									|| tileEast->getMapData(O_OBJECT)->getBigwall() != BIGWALL_NWSE))
							{
								const Tile* const tileEastBelow (_battleSave->getTile(posField + Position(1,0,-1)));
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
					} // end draw floor

// Redraw unitNorth moving NE/SW to stop current-Floor from clipping feet.
					if (itX != 0 && itY != 0
						&& hasFloor == true)
					{
						const Tile* const tileWest (_battleSave->getTile(posField + Position(-1,0,0)));
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
											const Tile* tileNorth (_battleSave->getTile(posField + Position(0,-1,0)));
											const BattleUnit* unitNorth (tileNorth->getTileUnit());
											int offsetZ_y;
											if (unitNorth == nullptr && itZ != 0)
											{
												tileNorth = _battleSave->getTile(posField + Position(0,-1,-1));
												unitNorth = tileNorth->getTileUnit();
												offsetZ_y = 24;
											}
											else
												offsetZ_y = 0;

											if (unitNorth == unitWest)
											{
												const Tile* const tileSouthWest (_battleSave->getTile(posField + Position(-1,1,0)));
												if (checkWest(tileWest, tileSouthWest, unitNorth) == true)
												{
													const Tile* const tileNorthEast (_battleSave->getTile(posField + Position(1,-1,0)));
													if (checkNorth(tileNorth, tileNorthEast, unitNorth) == true)
													{
														trueLoc = isTrueLoc(unitNorth, tileNorth);
														quadrant = getQuadrant(unitNorth, tileNorth, trueLoc);
														sprite = unitNorth->getCache(quadrant);
														//if (sprite != nullptr)
														{
															if (unitNorth->isOut_t(OUT_HLTH_STUN) == true)
																shade = std::min(tileShade, SHADE_UNIT);
															else
																shade = tileShade;

															calcWalkOffset(unitNorth, &walkOffset, trueLoc);
															sprite->blitNShade(
																	surface,
																	posScreen.x + walkOffset.x + 16 - _spriteWidth_2,
																	posScreen.y + walkOffset.y -  8 + offsetZ_y,
																	shade);

															if (unitNorth->getFireUnit() != 0)
															{
																frame = 4 + (_aniFrame / 2);
																sprite = _res->getSurfaceSet("SMOKE.PCK")->getFrame(frame);
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

// Draw Cursor Background
					if (_selectorType != CT_NONE
						&& _battleSave->getBattleState()->getMouseOverIcons() == false
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
										frame = 7 + (_aniFrame / 2);	// yellow animated crosshairs
									else
										frame = 6;						// red static crosshairs
									break;

								default:
								case CT_CUBOID:
								case CT_PSI:
								case CT_LAUNCH:
								case CT_TOSS:
									if (hasUnit == true
										&& (_selectorType != CT_PSI
											|| ((_battleSave->getBattleGame()->getTacticalAction()->type == BA_PSICOURAGE
													&& _unit->getFaction() != FACTION_HOSTILE)
												|| (_battleSave->getBattleGame()->getTacticalAction()->type != BA_PSICOURAGE
													&& _unit->getFaction() != FACTION_PLAYER))))
									{
										frame = (_aniFrame % 2);	// yellow flashing box
									}
									else
										frame = 0;					// red static box
							}

							sprite = _res->getSurfaceSet("CURSOR.PCK")->getFrame(frame);
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

// Draw Tile Background
// Draw west wall
					if (_tile->isVoid(true, false) == false)
					{
						sprite = _tile->getSprite(O_WESTWALL);
						if (sprite != nullptr)
						{
							if (_tile->isRevealed(ST_WEST) == true
								&& (_tile->getMapData(O_WESTWALL)->isDoor() == true
									|| _tile->getMapData(O_WESTWALL)->isUfoDoor() == true))
							{
								shade = std::min(_tile->getShade(), SHADE_DOOR);
							}
							else
								shade = tileShade;

							sprite->blitNShade(
									surface,
									posScreen.x,
									posScreen.y - _tile->getMapData(O_WESTWALL)->getYOffset(),
									shade);
						}

// Draw North Wall
						sprite = _tile->getSprite(O_NORTHWALL);
						if (sprite != nullptr)
						{
							if (_tile->isRevealed(ST_NORTH) == true
								&& (_tile->getMapData(O_NORTHWALL)->isDoor() == true
									|| _tile->getMapData(O_NORTHWALL)->isUfoDoor() == true))
							{
								shade = std::min(_tile->getShade(), SHADE_DOOR);
							}
							else
								shade = tileShade;

							sprite->blitNShade(
									surface,
									posScreen.x,
									posScreen.y - _tile->getMapData(O_NORTHWALL)->getYOffset(),
									shade,
									(_tile->getMapData(O_WESTWALL) != nullptr));
						}

// Draw Object in Background & Center
						sprite = _tile->getSprite(O_OBJECT);
						if (sprite != nullptr)
						{
							switch (_tile->getMapData(O_OBJECT)->getBigwall())
							{
								case BIGWALL_NONE:
								case BIGWALL_BLOCK:
								case BIGWALL_NESW:
								case BIGWALL_NWSE:
								case BIGWALL_WEST:
								case BIGWALL_NORTH:
//								case BIGWALL_W_N: // NOT USED in stock UFO.
									hasObject = true;
									sprite->blitNShade(
											surface,
											posScreen.x,
											posScreen.y - _tile->getMapData(O_OBJECT)->getYOffset(),
											tileShade);
							}
						}

// Draw Corpse + Item on Floor if any
						bool var;
						int spriteId (_tile->getCorpseSprite(&var));
						if (spriteId != -1)
						{
							sprite = _res->getSurfaceSet("FLOOROB.PCK")->getFrame(spriteId);
							//if (sprite != nullptr)
								sprite->blitNShade(
										surface,
										posScreen.x,
										posScreen.y + _tile->getTerrainLevel(),
										tileShade);

							if (var == true && _tile->isRevealed(ST_CONTENT) == true)
							{
								frame = 4 + (_aniFrame / 2);
								sprite = _res->getSurfaceSet("SMOKE.PCK")->getFrame(frame);
								//if (sprite != nullptr)
									sprite->blitNShade(
											surface,
											posScreen.x,
											posScreen.y + _tile->getTerrainLevel());
							}
						}

						spriteId = _tile->getTopSprite(&var);
						if (spriteId != -1)
						{
							sprite = _res->getSurfaceSet("FLOOROB.PCK")->getFrame(spriteId);
							//if (sprite != nullptr)
							{
								sprite->blitNShade(
										surface,
										posScreen.x,
										posScreen.y + _tile->getTerrainLevel(),
										tileShade);

								if (var == true && _tile->isRevealed(ST_CONTENT) == true)
								{
									for (int
											x = 0;
											x != 3;
											++x)
									{
										sprite->setPixelColor(
															15 + x, 28,
															_fuseColor);
									}
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

// Draw Bullet if in Field Of View
					if (_projectile != nullptr && _showProjectile == true) // <- used to hide Celatid glob while its spitting animation plays.
					{
						Position voxel;
						if (_projectile->getThrowItem() != nullptr)
						{
							sprite = _projectile->getThrowSprite();
							//if (sprite != nullptr)
							{
								voxel = _projectile->getPosition(); // draw shadow on the floor
								voxel.z = _battleSave->getTileEngine()->castShadow(voxel);
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
										voxel.z = _battleSave->getTileEngine()->castShadow(voxel);
										if (   (voxel.x >> 4) == itX
											&& (voxel.y >> 4) == itY
											&&  voxel.z / 24  == itZ)
										{
											_camera->convertVoxelToScreen(voxel, &bullet);

											bullet.x -= sprite->getWidth() / 2;
											bullet.y -= sprite->getHeight() / 2;
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

											bullet.x -= sprite->getWidth() / 2;
											bullet.y -= sprite->getHeight() / 2;
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
						bool
							halfRight (false),
							draw      (true);

						switch (_unit->getUnitStatus()) // don't clip through north/northwest/west UFO hulls etc.
						{
							case STATUS_WALKING:
							case STATUS_FLYING:
							{
								if (_unit->getWalkPhase() != 0
									|| _unit->getUnitDirection() == 4
									|| _unit->getUnitDirection() == 1
									|| _unit->getUnitDirection() == 2) // weird.
								{
									switch (_unit->getUnitDirection())
									{
										case 0:
										case 4:
										{
											const Tile
												* const tileNorth (_battleSave->getTile(posField + Position(0,-1,0))),
												* const tileNorthEast (_battleSave->getTile(posField + Position(1,-1,0)));
											draw = checkNorth(tileNorth, tileNorthEast);
											break;
										}

										case 2:
										case 6:
										{
											const Tile
												* const tileWest (_battleSave->getTile(posField + Position(-1,0,0))),
												* const tileSouthWest (_battleSave->getTile(posField + Position(-1,1,0)));
											draw = checkWest(tileWest, tileSouthWest, nullptr, &halfRight);
											break;
										}

										case 1:
										case 5:
										{
											const Tile
												* const tileSouthWest (_battleSave->getTile(posField + Position(-1,1,0))),
												* const tileSouthSouthWest (_battleSave->getTile(posField + Position(-1,2,0)));
											draw = checkWest(tileSouthWest, tileSouthSouthWest);

											const Tile
												* const tileNorthEast (_battleSave->getTile(posField + Position(1,-1,0))),
												* const tileNorthNorthEast (_battleSave->getTile(posField + Position(1,-2,0)));
											draw = draw
												&& checkNorth(tileNorthEast, tileNorthNorthEast);
										}
									}
								}
							}
						}

						if (draw == true)
						{
							trueLoc = isTrueLoc(_unit, _tile);
							quadrant = getQuadrant(_unit, _tile, trueLoc);
							sprite = _unit->getCache(quadrant);
							if (sprite != nullptr) // <- check is needed for start of Tactical.
							{
								if (_unit->isOut_t(OUT_HLTH_STUN) == true)
									shade = std::min(tileShade, SHADE_UNIT);
								else
									shade = tileShade;

								calcWalkOffset(_unit, &walkOffset, trueLoc);
								sprite->blitNShade(
										surface,
										posScreen.x + walkOffset.x - _spriteWidth_2,
										posScreen.y + walkOffset.y,
										shade, halfRight);

								if (_unit->getFireUnit() != 0)
								{
									sprite = _res->getSurfaceSet("SMOKE.PCK")->getFrame(4 + (_aniFrame / 2));
									//if (sprite != nullptr)
										sprite->blitNShade(
												surface,
												posScreen.x + walkOffset.x,
												posScreen.y + walkOffset.y,
												0, halfRight);
								}

								// kL_begin #3 of 3:
								if (_unit->getFaction() == FACTION_PLAYER
									&& _unit->isMindControlled() == false)
								{
									const Tile* const tileAbove (_battleSave->getTile(posField + Position(0,0,1)));
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
											&& (_unit->getArmor()->getSize() == 1 || quadrant == 1)
//											&& _projectileInFOV == false && _explosionInFOV == false)
//											&& _battleSave->getBattleState()->allowButtons() == true
											&& _battleSave->getBattleGame()->getTacticalAction()->type == BA_NONE)
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
												if (_aniFrame < 4)
													color = WHITE_u;
												else
													color = BLACK;

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

// Draw Unconscious Soldier icon - might want to redundant this, like rankIcons.
					if (_unit == nullptr)
					{
						const int hurt (_tile->hasUnconsciousUnit());
						if (hurt != 0)
						{
							_srfRookiBadge->blitNShade( // background panel for red cross icon.
												surface,
												posScreen.x,
												posScreen.y);
							int color;
							if (hurt == 2)
								color = RED;	// wounded unconscious soldier
							else
								color = WHITE;	// unconscious soldier here

							_srfCross->blitNShade( // small gray cross
											surface,
											posScreen.x + 2,
											posScreen.y + 1,
											_aniFrame * 2,
											false, color);
						}
					}
					// end unconscious soldier icon.

// Draw unitBelow if it is on raised ground & there is no Floor above.
					if (itZ > 0 && _tile->hasNoFloor(tileBelow) == true)
					{
						const int tLevel (tileBelow->getTerrainLevel());
						if (tLevel < 0) // probly more like -4 or -8
						{
							const BattleUnit* const unitBelow (tileBelow->getTileUnit());
							if (unitBelow != nullptr
								&& unitBelow->getUnitVisible() == true // don't bother checking DebugMode
								&& unitBelow->getHeight(true) - tLevel > 23) // head sticks up, probly more like 26 or 28 before clipping background walls above occurs
							{
								trueLoc = isTrueLoc(unitBelow, tileBelow);
								quadrant = getQuadrant(unitBelow, tileBelow, trueLoc);
								sprite = unitBelow->getCache(quadrant);
								//if (sprite != nullptr)
								{
									if (tileBelow->isRevealed(ST_CONTENT) == true)
										shade = tileBelow->getShade();
									else
										shade = SHADE_BLACK;

									calcWalkOffset(unitBelow, &walkOffset, trueLoc);
									sprite->blitNShade(
											surface,
											posScreen.x + walkOffset.x - _spriteWidth_2,
											posScreen.y + walkOffset.y + 24,
											shade);

									if (unitBelow->getFireUnit() != 0)
									{
										frame = 4 + (_aniFrame / 2);
										sprite = _res->getSurfaceSet("SMOKE.PCK")->getFrame(frame);
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
					if (_tile->isRevealed(ST_CONTENT) == true
						&& (_tile->getSmoke() != 0 || _tile->getFire() != 0))
					{
						if (_tile->getFire() == 0)
						{
							frame = ResourcePack::SMOKE_OFFSET;
							frame += (_tile->getSmoke() + 1) / 2;
							shade = tileShade;
						}
						else
						{
							frame =
							shade = 0;
						}

						aniOffset = _aniFrame / 2 + _tile->getAnimationOffset();
						if (aniOffset > 3) aniOffset -= 4;
						frame += aniOffset;

						sprite = _res->getSurfaceSet("SMOKE.PCK")->getFrame(frame);
						//if (sprite != nullptr)
							sprite->blitNShade(
									surface,
									posScreen.x,
									posScreen.y + _tile->getTerrainLevel(),
									shade);
					} // end Smoke & Fire

// Draw pathPreview (arrows solid)
					if ((_previewSetting & PATH_ARROWS)
						&& _tile->getPreviewDir() != -1
						&& _tile->isRevealed(ST_WEST) == true) // what.
					{
						if (itZ > 0 && _tile->hasNoFloor(tileBelow) == true)
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
					sprite = _tile->getSprite(O_OBJECT);
					if (sprite != nullptr)
					{
						switch (_tile->getMapData(O_OBJECT)->getBigwall())
						{
							case BIGWALL_EAST:
							case BIGWALL_SOUTH:
							case BIGWALL_E_S:
									sprite->blitNShade(
											surface,
											posScreen.x,
											posScreen.y - _tile->getMapData(O_OBJECT)->getYOffset(),
											tileShade);
						}
					}

// Draw Cursor Front
					if (_selectorType != CT_NONE
						&& _battleSave->getBattleState()->getMouseOverIcons() == false
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
										frame = 7 + (_aniFrame / 2);	// yellow animated crosshairs
									else
										frame = 6;						// red static crosshairs
									break;

								default:
								case CT_CUBOID:
								case CT_PSI:
								case CT_LAUNCH:
								case CT_TOSS:
									if (hasUnit == true
										&& (_selectorType != CT_PSI
											|| ((_battleSave->getBattleGame()->getTacticalAction()->type == BA_PSICOURAGE
													&& _unit->getFaction() != FACTION_HOSTILE)
												|| (_battleSave->getBattleGame()->getTacticalAction()->type != BA_PSICOURAGE
													&& _unit->getFaction() != FACTION_PLAYER))))
									{
										frame = 3 + (_aniFrame % 2);	// yellow flashing box
									}
									else
										frame = 3;						// red static box
							}

							sprite = _res->getSurfaceSet("CURSOR.PCK")->getFrame(frame);
							sprite->blitNShade(
									surface,
									posScreen.x,
									posScreen.y);// + vertOffset);

// UFOExtender Accuracy
							// display adjusted accuracy value on crosshair (and more).
//							if (Options::battleUFOExtenderAccuracy == true) // -> one less condition to check
							switch (_selectorType)
							{
								case CT_TARGET:
								{
									// draw targetUnit overtop cursor's front if Tile is blacked-out.
									if (hasUnit == true && _tile->isRevealed(ST_CONTENT) == false)
									{
										trueLoc = isTrueLoc(_unit, _tile);
										quadrant = getQuadrant(_unit, _tile, trueLoc);
										sprite = _unit->getCache(quadrant);
										//if (sprite != nullptr)
										{
											calcWalkOffset(_unit, &walkOffset, trueLoc);
											sprite->blitNShade(
													surface,
													posScreen.x + walkOffset.x - _spriteWidth_2,
													posScreen.y + walkOffset.y,
													tileShade);
										}
									}

									// TODO: Use stuff from ProjectileFlyBState::init()
									// as well as TileEngine::canTargetUnit() & TileEngine::canTargetTilepart()
									// to turn accuracy to 'red 0' if target is out of LoS/LoF.
									//
									// TODO: use Projectile::rangeAccuracy() as a static function.
									const BattleAction* const action (_battleSave->getBattleGame()->getTacticalAction());
									int accuracy;
									Uint8 color;

									bool zero;
									if (hasUnit == true)
									{
										TileEngine* te (_battleSave->getTileEngine());

										const Position originVoxel (te->getSightOriginVoxel(action->actor));
										Position scanVoxel;
										zero = te->canTargetUnit(
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
									BattleAction* const action (_battleSave->getBattleGame()->getTacticalAction());
									action->posTarget = Position(itX,itY,itZ);

									unsigned accuracy;
									Uint8 color;
									const Position
										originVoxel (_battleSave->getTileEngine()->getOriginVoxel(*action)),
										targetVoxel (Position::toVoxelSpaceCentered( // TODO: conform this to ProjectileFlyBState (modifier keys) & Projectile::_targetVoxel
																				Position(itX,itY,itZ),
																				2 - _battleSave->getTile(action->posTarget)->getTerrainLevel())); // LoFT of floor is typically 2 voxels thick.
									if (hasFloor == true
										&& _battleSave->getTileEngine()->validateThrow(
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
									static const int cursorSprites[6] = {0,0,0,11,13,15};
									sprite = _res->getSurfaceSet("CURSOR.PCK")->getFrame(cursorSprites[_selectorType] + (_aniFrame / 4));
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
					unsigned wpVal (1);

					for (std::vector<Position>::const_iterator
							i = _waypoints.begin();
							i != _waypoints.end();
							++i, ++wpVal)
					{
						if (*i == posField) // note that 2 BL waypoints can be at the same Position.
						{
							if (offset_x == 2 && offset_y == 2)
							{
								sprite = _res->getSurfaceSet("TARGET.PCK")->getFrame(0); // was "CURSOR.PCK" frame= 7
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

							if (wpVal > 9)	offset_x += 8;
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
					if ((itZ == _battleSave->getGroundLevel()
							|| (itZ == 0 && _battleSave->getGroundLevel() == -1))
						&& _tile->getTileUnit() == nullptr)
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
//					if (itZ == viewLevel && _battleSave->scannerDots().empty() == false)
//					{
//						const std::pair<int,int> dotTest (std::make_pair(
//																	posField.x,
//																	posField.y));
//						if (std::find(
//								_battleSave->scannerDots().begin(),
//								_battleSave->scannerDots().end(),
//								dotTest) != _battleSave->scannerDots().end())
//						{
//							srfBorder->blitNShade(
//									surface,
//									posScreen.x + 14,
//									posScreen.y + 30,
//									0, false, RED);
//						}
//					}
					// end scanner dots.
				}
				// is inside the Surface
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
		_unit = _battleSave->getSelectedUnit();
		if (_unit != nullptr
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

					if (_unit->getArmor()->getSize() > 1)
					{
						posScreen.y += 10;
						if (_unit->getFloatHeight() != 0)
							posScreen.y -= _unit->getFloatHeight() + 1;
					}

					posScreen.x += _spriteWidth_2;

					static const int phaseCycle[8u] {0,-3,3,-2,-3,-2,0,1};
//					const int phaseCycle (static_cast<int>(4. * std::sin(22.5 / static_cast<double>(_aniFrame + 1))));

					if (_unit->isKneeled() == true)
						_arrow_kneel->blitNShade(
								surface,
								posScreen.x - (_arrow_kneel->getWidth() / 2),
								posScreen.y - _arrow_kneel->getHeight() - 4 - phaseCycle[_aniFrame]);
//								posScreen.y - _arrow_kneel->getHeight() - 4 - phaseCycle);
					else
						_arrow->blitNShade(
								surface,
								posScreen.x - _arrow->getWidth() / 2,
								posScreen.y - _arrow->getHeight() + phaseCycle[_aniFrame]);
//								posScreen.y - _arrow->getHeight() + phaseCycle);
				}
			}
		}
	}
	// end arrow.

	if (_battleSave->getPathfinding()->isPathPreviewed() == true
		|| Options::traceAI == true)
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
						_tile = _battleSave->getTile(posField);

						if (_tile != nullptr
							&& _tile->isRevealed(ST_CONTENT) == true
							&& _tile->getPreviewDir() != -1)
						{
							int offset_y (-_tile->getTerrainLevel());

							if (_previewSetting & PATH_ARROWS) // arrows semi-transparent
							{
								tileBelow = _battleSave->getTile(posField + Position(0,0,-1));
								if (itZ > 0 && _tile->hasNoFloor(tileBelow) == true)
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
		int spriteId;
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
								bullet.x - (sprite->getWidth() / 2),
								bullet.y - (sprite->getHeight() / 2));
						break;

					case ET_BULLET: // Bullet-hits, http://ufopaedia.org/index.php?title=SMOKE.PCK
					case ET_TORCH:
						sprite = _res->getSurfaceSet("SMOKE.PCK")->getFrame(spriteId);
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

	if (unit->getFatalWounds() != 0)
	{
		_srfRookiBadge->blitNShade(		// background panel for red cross icon.
							this,
							offset_x + 2,
							offset_y + 3);

		_srfCross->blitNShade(			// small gray cross drawn RED.
							this,
							offset_x + 4,
							offset_y + 4,
							_aniFrame * 2,
							false, RED);
	}
	else
	{
		std::string solRank (unit->getRankString()); // eg. STR_COMMANDER -> RANK_COMMANDER
		solRank = "RANK" + solRank.substr(3, solRank.length() - 3);

		Surface* const sprite (_res->getSurface(solRank));
		if (sprite != nullptr)
			sprite->blitNShade(
							this,
							offset_x + 2,
							offset_y + 3);
	}
}

/**
 * Checks if a southwesterly wall should suppress unit-sprite drawing.
 * @note When a unit moves west it can clip through a previously drawn wall.
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
		bool* halfRight) const
{
	bool ret;

	if (unit == nullptr) // '_unit' is Valid.
	{
		ret = tile6 == nullptr
		   || tile6->getTileUnit() != _unit;
	}
	else
		ret = false; // '_unit' is NOT Valid.

	ret = ret
		|| ((tile6->getMapData(O_OBJECT) == nullptr
				|| tile6->getMapData(O_OBJECT)->getBigwall() != BIGWALL_SOUTH)
			&& (tile5 == nullptr
				|| ((tile5->getMapData(O_NORTHWALL) == nullptr
						|| tile5->isUfoDoorOpen(O_NORTHWALL) == true)
					&& (tile5->getMapData(O_OBJECT) == nullptr
						|| (tile5->getMapData(O_OBJECT)->getBigwall() & 0x33) == 0)))); // Block/NeSw/North/East.
//						|| (   tile5->getMapData(O_OBJECT)->getBigwall() != BIGWALL_BLOCK
//							&& tile5->getMapData(O_OBJECT)->getBigwall() != BIGWALL_NESW
//							&& tile5->getMapData(O_OBJECT)->getBigwall() != BIGWALL_EAST)))));
//							&& tile5->getMapData(O_OBJECT)->getBigwall() != BIGWALL_NORTH // not in stock UFO.

	if (ret == false) // unit might actually be too far away from wall to clip despite above conditions - now don't let the floor clip it
	{
		if (unit == nullptr) unit = _unit;
		switch (unit->getUnitDirection())
		{
			case 1:
			case 2: ret = unit->getPosition() == unit->getStopPosition();
				break;
			case 5:
			case 6: ret = unit->getPosition() == unit->getStartPosition();
		}

		if (halfRight != nullptr && unit->getArmor()->getSize() == 2)
		{
			if (ret == true)
			{
//				if (dir != 2) *halfRight = true; // could allow this. Maybe !=1 also ...

				const Position pos (tile6->getPosition() + Position(1,0,0));
				const Tile
					* const tile (_battleSave->getTile(pos)),
					* const tileSouth (_battleSave->getTile(pos + Position(0,1,0)));
				if (!
					((tile->getMapData(O_OBJECT) == nullptr
						|| tile->getMapData(O_OBJECT)->getBigwall() != BIGWALL_SOUTH)
					&& (tileSouth == nullptr
						|| ((tileSouth->getMapData(O_NORTHWALL) == nullptr
								|| tileSouth->isUfoDoorOpen(O_NORTHWALL) == true)
							&& (tileSouth->getMapData(O_OBJECT) == nullptr
								|| (tileSouth->getMapData(O_OBJECT)->getBigwall() & 0x3) == 0))))) // Block/NeSw.
//								|| (tileSouth->getMapData(O_OBJECT)->getBigwall() != BIGWALL_NESW
//									&& tileSouth->getMapData(O_OBJECT)->getBigwall() != BIGWALL_BLOCK))))))
								// All that causes clipping when the large unit moves out eastward from along the northern side
								// of an EW barrier but it's better than leaving a big hole in the 3rd quadrant as it moves out.
				{
					*halfRight = true; // but only if a wall is directly south
				}
			}
			else
			{
				switch (unit->getUnitDirection())
				{
					case 1:
					case 2:
						*halfRight =
						ret = true;
				}
			}
		}
	}
	return ret;
}

/**
 * Checks if a northeasterly wall should suppress unit-sprite drawing.
 * @note When a unit moves north it can clip through a previously drawn wall.
 * @param tile0	- pointer to the tile north of current
 * @param tile1	- pointer to the tile northeast of current
 * @param unit	- pointer to BattleUnit not '_unit' (default nullptr)
 * @return, true to allow drawing the unit's sprite
 */
bool Map::checkNorth( // private.
		const Tile* const tile0,
		const Tile* const tile1,
		const BattleUnit* unit) const
{
	bool ret;

	if (unit == nullptr) // '_unit' is Valid.
	{
		ret = tile0 == nullptr
		   || tile0->getTileUnit() != _unit;
	}
	else
		ret = false; // '_unit' is NOT Valid.

	ret = ret
		|| ((tile0->getMapData(O_OBJECT) == nullptr
				|| tile0->getMapData(O_OBJECT)->getBigwall() != BIGWALL_EAST)
			&& (tile1 == nullptr
				|| ((tile1->getMapData(O_WESTWALL) == nullptr
						|| tile1->isUfoDoorOpen(O_WESTWALL) == true)
					&& (tile1->getMapData(O_OBJECT) == nullptr
						|| (tile1->getMapData(O_OBJECT)->getBigwall() & 0xb) == 0)))); // Block/NeSw/West.
//						|| (   tile1->getMapData(O_OBJECT)->getBigwall() != BIGWALL_NESW
//							&& tile1->getMapData(O_OBJECT)->getBigwall() != BIGWALL_BLOCK)))));
//							&& tile1->getMapData(O_OBJECT)->getBigwall() != BIGWALL_WEST // not in stock UFO.

	if (ret == false) // unit might actually be too far away from wall to clip despite above conditions - now don't let the floor clip it
	{
		if (unit == nullptr) unit = _unit;
		switch (unit->getUnitDirection())
		{
			case 0:
			case 1: ret = unit->getPosition() == unit->getStartPosition();
				break;
			case 4:
			case 5: ret = unit->getPosition() == unit->getStopPosition();
		}
	}
	return ret;
}

/**
 * Replaces a certain amount of colors in the surface's palette.
 * @param colors		- pointer to the set of colors
 * @param firstcolor	- offset of the first color to replace
 * @param ncolors		- amount of colors to replace
 */
void Map::setPalette(
		SDL_Color* const colors,
		int firstcolor,
		int ncolors)
{
	Surface::setPalette(colors, firstcolor, ncolors);

	for (std::vector<MapDataSet*>::const_iterator
			i = _battleSave->getMapDataSets()->begin();
			i != _battleSave->getMapDataSets()->end();
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
 * Handles mouse presses on the Map.
 * @param action	- pointer to an Action
 * @param state		- State that the ActionHandlers belong to
 */
void Map::mousePress(Action* action, State* state)
{
	InteractiveSurface::mousePress(action, state);
	_camera->mousePress(action, state);
}

/**
 * Handles mouse releases on the Map.
 * @param action	- pointer to an Action
 * @param state		- State that the ActionHandlers belong to
 */
void Map::mouseRelease(Action* action, State* state)
{
	InteractiveSurface::mouseRelease(action, state);
	_camera->mouseRelease(action, state);
}

/**
 * Handles mouse over events on the Map.
 * @param action	- pointer to an Action
 * @param state		- State that the ActionHandlers belong to
 */
void Map::mouseOver(Action* action, State* state)
{
	InteractiveSurface::mouseOver(action, state);
	_camera->mouseOver(action, state);

	_mX = static_cast<int>(action->getAbsoluteXMouse());
	_mY = static_cast<int>(action->getAbsoluteYMouse());
	refreshSelectorPosition();
}

/**
 * Finds the current mouse position XY on this Map.
 * @param point - reference the mouse position
 */
void Map::findMousePointer(Position& point)
{
	point.x = _mX;
	point.y = _mY;
	point.z = 0;
}

/**
 * Handles keyboard presses on the Map.
 * @param action	- pointer to an Action
 * @param state		- State that the ActionHandlers belong to
 */
void Map::keyboardPress(Action* action, State* state)
{
	InteractiveSurface::keyboardPress(action, state);
	_camera->keyboardPress(action, state);
}

/**
 * Handles keyboard releases on the Map.
 * @param action	- pointer to an Action
 * @param state		- State that the ActionHandlers belong to
 */
void Map::keyboardRelease(Action* action, State* state)
{
	InteractiveSurface::keyboardRelease(action, state);
	_camera->keyboardRelease(action, state);
}

/**
 * Animates any/all Tiles and BattleUnits that need animating.
 * @note 8 frames per animation [0..7].
 * @param redraw - true to redraw the battlefield (default true)
 */
void Map::animateMap(bool redraw)
{
 	if (++_aniFrame == 8) _aniFrame = 0;

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
		if ((*i)->isOut_t(OUT_STAT) == false
			&& (*i)->getArmor()->getConstantAnimation() == true)
		{
			(*i)->clearCache();
			cacheUnit(*i);
		}
	}

	if (--_fuseColor == 15u) _fuseColor = 31u;
	if (redraw == true) _redraw = true;
}

/**
 * Sets the rectangular selector to the current mouse-position.
 */
void Map::refreshSelectorPosition()
{
	const int
		pre_X (_selectorX),
		pre_Y (_selectorY);

	_camera->convertScreenToMap(
							_mX,
							_mY + _spriteHeight / 4,
							&_selectorX,
							&_selectorY);

	if (pre_X != _selectorX || pre_Y != _selectorY)
		_redraw = true;
}


/**
 * Gets the position of the rectangular selector.
 * @param pos - pointer to a Position
 */
void Map::getSelectorPosition(Position* const pos) const
{
	pos->x = _selectorX;
	pos->y = _selectorY;
	pos->z = _camera->getViewLevel();
}

/**
 * Gets if a Tile is a/the true location of current unit.
 * @param unit - pointer to a unit
 * @param tile - pointer to a tile
 * @return, true if true location
 */
bool Map::isTrueLoc(
		const BattleUnit* const unit,
		const Tile* const tile) const // private.
{
	if (unit->getTile() == tile
		|| (unit->getArmor()->getSize() == 2
			&& (   tile->getPosition() + Position(-1, 0,0) == unit->getPosition()
				|| tile->getPosition() + Position( 0,-1,0) == unit->getPosition()
				|| tile->getPosition() + Position(-1,-1,0) == unit->getPosition())))
	{
		return true;
	}
	return false;
}

/**
 * Gets the unit's quadrant for drawing.
 * @param unit		- pointer to a unit
 * @param tile		- pointer to a tile
 * @param trueLoc	- true if real location; false if transient
 * @return, quadrant
 */
int Map::getQuadrant( // private.
		const BattleUnit* const unit,
		const Tile* const tile,
		bool trueLoc) const
{
	if (trueLoc == true //unit->getUnitStatus() == STATUS_STANDING ||
		|| unit->getVerticalDirection() != 0)
	{
		return tile->getPosition().x - unit->getPosition().x
			+ (tile->getPosition().y - unit->getPosition().y) * 2;
	}

	int dir (unit->getUnitDirection());
	if (unit->getPosition() == unit->getStopPosition())
		dir = (dir + 4) % 8;

	Position
		posUnit,
		posVect;
	Pathfinding::directionToVector(dir, &posVect);
	posUnit = unit->getPosition() + posVect;

	return tile->getPosition().x - posUnit.x
		+ (tile->getPosition().y - posUnit.y) * 2;
}

/**
 * Calculates the offset of a unit-sprite when it is moving between 2 tiles.
 * @param unit		- pointer to a BattleUnit
 * @param offset	- pointer to the Position that will be the calculation result
 * @param trueLoc	- true if real location; false if transient
 */
void Map::calcWalkOffset( // private.
		const BattleUnit* const unit,
		Position* const offset,
		bool trueLoc) const
{
	*offset = Position(0,0,0);

	switch (unit->getUnitStatus())
	{
		case STATUS_WALKING:
		case STATUS_FLYING:
		{
			static const int
				offsetX[8] {1, 1, 1, 0,-1,-1,-1, 0},
				offsetY[8] {1, 0,-1,-1,-1, 0, 1, 1},

				offsetFalseX[8] {16, 32, 16,  0,-16,-32,-16,  0}, // for duplicate drawing units from their transient
				offsetFalseY[8] {-8,  0,  8, 16,  8,  0, -8,-16}, // destination & last positions. See UnitWalkBState.
				offsetFalseVert (24);
			const int
				dir (unit->getUnitDirection()),			// 0..7
				dirVert (unit->getVerticalDirection()),	// 0= none, 8= up, 9= down
				armorSize (unit->getArmor()->getSize());
			int
				walkPhase (unit->getWalkPhaseTrue()),
				halfPhase (unit->getWalkPhaseHalf()),
				fullPhase (unit->getWalkPhaseFull());

			const bool start (walkPhase < halfPhase);
			if (dirVert == 0)
			{
				if (start == true)
				{
					offset->x =  walkPhase * offsetX[dir] * 2 + ((trueLoc == true) ? 0 : -offsetFalseX[dir]);
					offset->y = -walkPhase * offsetY[dir]     + ((trueLoc == true) ? 0 : -offsetFalseY[dir]);
				}
				else
				{
					offset->x =  (walkPhase - fullPhase) * offsetX[dir] * 2 + ((trueLoc == true) ? 0 : offsetFalseX[dir]);
					offset->y = -(walkPhase - fullPhase) * offsetY[dir]     + ((trueLoc == true) ? 0 : offsetFalseY[dir]);
				}
			}

			// if unit is between tiles interpolate its terrain level (y-adjustment).
			const int
				posStartZ (unit->getStartPosition().z),
				posStopZ (unit->getStopPosition().z);
			int
				levelStart,
				levelStop;

			if (start == true)
			{
				if (dirVert == Pathfinding::DIR_UP)
					offset->y += (trueLoc == true) ? 0 : offsetFalseVert;
				else if (dirVert == Pathfinding::DIR_DOWN)
					offset->y += (trueLoc == true) ? 0 : -offsetFalseVert;

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
				if (trueLoc == false && dirVert == 0)
				{
					if (posStartZ > posStopZ)
						offset->y -= offsetFalseVert;
					else if (posStartZ < posStopZ)
						offset->y += offsetFalseVert;
				}
			}
			else
			{
				if (dirVert == Pathfinding::DIR_UP)
					offset->y += (trueLoc == true) ? 0 : -offsetFalseVert;
				else if (dirVert == Pathfinding::DIR_DOWN)
					offset->y += (trueLoc == true) ? 0 : offsetFalseVert;

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

				if (trueLoc == false && dirVert == 0)
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
  * @param pos			- reference the Position
  * @param armorSize	- size of the unit at @a pos
  * @return, terrain height
  */
int Map::getTerrainLevel( // private.
		const Position& pos,
		int armorSize) const
{
	int
		lowLevel (0),
		lowTest;

	for (int
			x = 0;
			x != armorSize;
			++x)
	{
		for (int
				y = 0;
				y != armorSize;
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
 * @param type	- SelectorType (Map.h)
 * @param quads	- size of the cursor (default 1)
 */
void Map::setSelectorType(
		SelectorType type,
		int quads)
{
	switch (_selectorType = type)
	{
		case CT_CUBOID:
			_selectorSize = quads;
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
 * Checks all units for need to be redrawn and if so updates sprite(s).
 */
void Map::cacheUnits()
{
	for (std::vector<BattleUnit*>::const_iterator
			i = _battleSave->getUnits()->begin();
			i != _battleSave->getUnits()->end();
			++i)
	{
		cacheUnit(*i);
	}
}

/**
 * Check if a BattleUnit needs to be redrawn and if so updates its sprite(s).
 * @param unit - pointer to a BattleUnit
 */
void Map::cacheUnit(BattleUnit* const unit)
{
	if (unit->getCacheInvalid() == true)
	{
		UnitSprite* const sprite (new UnitSprite(
											_spriteWidth * 2,
											_spriteHeight));
		sprite->setPalette(this->getPalette());

		const int quadrants (unit->getArmor()->getSize() * unit->getArmor()->getSize());
		for (int
				i = 0;
				i != quadrants;
				++i)
		{
			Surface* cache (unit->getCache(i));
			if (cache == nullptr)
			{
				cache = new Surface(
								_spriteWidth * 2,
								_spriteHeight);
				cache->setPalette(this->getPalette());
			}

			sprite->setBattleUnit(unit, i);

			if (i == 0)
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
			sprite->setAnimationFrame(_aniFrame);
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
	_projectile = projectile;

	if (projectile != nullptr
		&& Options::battleSmoothCamera == true)
	{
		_bulletStart = true;
	}
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
 * Special handling for setting the height of the Map viewport.
 * @param height - the new base screen height
 */
void Map::setHeight(int height)
{
	Surface::setHeight(height);

	_playableHeight = height - _iconHeight;
	if (_playableHeight < 200)
		height = _playableHeight;
	else
		height = 200;

	_hiddenScreen->setHeight(height);
	_hiddenScreen->setY((_playableHeight - _hiddenScreen->getHeight()) / 2);
}

/**
 * Special handling for setting the width of the Map viewport.
 * @param width - the new base screen width
 */
void Map::setWidth(int width)
{
	Surface::setWidth(width);
	_hiddenScreen->setX(_hiddenScreen->getX() + (width - getWidth()) / 2);
}

/**
 * Gets the hidden-movement screen's vertical position.
 * @return, the vertical position
 */
int Map::getMessageY() const
{
	return _hiddenScreen->getY();
}

/**
 * Gets the icon-height.
 * @return, icon-panel height
 */
int Map::getIconHeight() const
{
	return _iconHeight;
}

/**
 * Gets the icon-width.
 * @return, icon-panel width
 */
int Map::getIconWidth() const
{
	return _iconWidth;
}

/**
 * Returns the angle (left/right balance) of a sound-effect based on a map-position.
 * @param pos - reference the Map position to calculate the sound angle from
 * @return, the angle of the sound (360 = 0 degrees center)
 */
int Map::getSoundAngle(const Position& pos) const
{
	const int midPoint (getWidth() / 2);

	Position screenPos;
	_camera->convertMapToScreen(pos, &screenPos);

	// cap the position to the screen edges relative to the center,
	// negative values indicating a left-shift, and positive values shifting to the right.
	screenPos.x = std::max(
						-midPoint,
						std::min(
								midPoint,
								screenPos.x + _camera->getMapOffset().x - midPoint));

	// Convert the relative distance left or right to a relative angle off-center.
	// Since Mix_SetPosition() uses modulo 360 can't feed it a negative number so add 360.
	// The integer-factor below is the allowable maximum deflection from center
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
