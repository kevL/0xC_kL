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

#include "ScannerView.h"

#include "../Engine/Action.h"
#include "../Engine/Game.h"
#include "../Engine/SurfaceSet.h"

#include "../Resource/ResourcePack.h"

#include "../Savegame/BattleUnit.h"
#include "../Savegame/SavedBattleGame.h"
#include "../Savegame/Tile.h"


namespace OpenXcom
{

/**
 * Initializes the ScannerView.
 * @param w			- the width
 * @param h			- the height
 * @param x			- the x-origin
 * @param y			- the y-origin
 * @param game		- pointer to the core Game
 * @param selUnit	- pointer to the currently selected BattleUnit
 */
ScannerView::ScannerView(
		int w,
		int h,
		int x,
		int y,
		const Game* const game,
		const BattleUnit* const selUnit)
	:
		InteractiveSurface(
			w,h,
			x,y),
		_game(game),
		_selUnit(selUnit),
		_cycle(0),
		_dotsDone(false)
{
	_redraw = true;
}

/**
 * Draws this ScannerView.
 */
void ScannerView::draw()
{
	SurfaceSet* const srt (_game->getResourcePack()->getSurfaceSet("DETBLOB.DAT"));
	Surface* srf;

	clear();

	const Tile* tile;
	int
		xPos,
		yPos;

	SavedBattleGame* const battleSave (_game->getSavedGame()->getBattleSave());
	std::vector<std::pair<int,int>>& scanDots (battleSave->scannerDots());

	this->lock();
	for (int
			x = -9;
			x != 10;
			++x)
	{
		for (int
				y = -9;
				y != 10;
				++y)
		{
			for (int
					z = 0;
					z != battleSave->getMapSizeZ();
					++z)
			{
				xPos = _selUnit->getPosition().x + x;
				yPos = _selUnit->getPosition().y + y;

				if ((tile = battleSave->getTile(Position(xPos,yPos,z))) != nullptr
					&& tile->getTileUnit() != nullptr
					&& tile->getTileUnit()->getMotionPoints() != 0)
				{
					if (_dotsDone == false)
					{
						std::pair<int,int> dot (std::make_pair(xPos,yPos));
						if (std::find(
									scanDots.begin(),
									scanDots.end(),
									dot) == scanDots.end())
						{
							scanDots.push_back(dot);
						}
					}

					int blobId (std::min(tile->getTileUnit()->getMotionPoints() / 5, 5));
					srf = srt->getFrame(blobId + _cycle);
					srf->blitNShade(
								this,
								Surface::getX() + ((9 + x) << 3u) - 4,
								Surface::getY() + ((9 + y) << 3u) - 4,
								0);
				}
			}
		}
	}
	_dotsDone = true;

	srf = srt->getFrame(_selUnit->getUnitDirection() + 7); // draw arrow in the direction that selUnit is faced
	srf->blitNShade(
				this,
				Surface::getX() + 68, //(9 * 8) - 4,
				Surface::getY() + 68, //(9 * 8) - 4,
				0);
	this->unlock();
}

/**
 * Handles clicks on this ScannerView.
 * @param action	- pointer to an Action
 * @param state		- State that the ActionHandlers belong to
 */
void ScannerView::mouseClick(Action*, State*)
{}

/**
 * Cycles the scanner-animation.
 */
void ScannerView::animate()
{
	if (++_cycle == 2) _cycle = 0;

	_redraw = true;
}

}
