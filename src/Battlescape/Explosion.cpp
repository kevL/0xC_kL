/*
 * Copyright 2010-2019 OpenXcom Developers.
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

#include "Explosion.h"


namespace OpenXcom
{

/**
 * Sets up an Explosion sprite to animate at a specified position.
 * @param type			- the ExplosionType (Explosion.h)
 * @param pos			- explosion's center position in voxel-space
 * @param startSprite	- used to offset different explosions to different frames on its spritesheet
 * @param startDelay	- used to delay the start of explosion (default 0)
 */
Explosion::Explosion(
		const ExplosionType type,
		const Position pos,
		int startSprite,
		int startDelay)
	:
		_type(type),
		_pos(pos),
		_startSprite(startSprite),
		_currentSprite(startSprite),
		_startDelay(startDelay)
{}

/**
 * Deletes the Explosion.
 */
Explosion::~Explosion()
{}

/**
 * Steps the Explosion forward frame-by-frame.
 * @note These animations are played by Map::drawTerrain() at the end of that
 * function.
 * @return, true if the animation is queued or playing
 */
bool Explosion::animate()
{
	if (_startDelay == 0)
	{
		++_currentSprite;
		switch (_type)
		{
			case ET_TORCH: // special handling for Fusion Torch - it has 6 frames that cycle 6 times
			{
				static int torchCycle;
				if (torchCycle == 7)
				{
					torchCycle = 0;
					return false;
				}

				if (_currentSprite == _startSprite + FRAMES_TORCH - 1)
				{
					_currentSprite = START_FUSION;
					++torchCycle;
				}
				break;
			}

			case ET_AOE:
				if (_currentSprite == _startSprite + FRAMES_AOE)
					return false;
				break;

			case ET_BULLET:
				if (_currentSprite == _startSprite + FRAMES_BULLET)
					return false;
				break;

			case ET_MELEE_ATT:
			case ET_MELEE_HIT:
			case ET_PSI:
				if (_currentSprite == _startSprite + FRAMES_MELEE_PSI)
					return false;
		}
	}
	else
		--_startDelay;

	return true;
}

/**
 * Gets this Explosion's position in voxel-space.
 * @return, position in voxel-space
 */
const Position Explosion::getPosition() const
{
	return _pos;
}

/**
 * Gets the currently playing sprite-ID.
 * @return, sprite-ID
 */
int Explosion::getCurrentSprite() const
{
	if (_startDelay == 0) return _currentSprite;

	return -1;
}

/**
 * Gets this Explosion's type.
 * @return, the ExplosionType (Explosion.h)
 */
ExplosionType Explosion::getExplosionType() const
{
	return _type;
}

}
