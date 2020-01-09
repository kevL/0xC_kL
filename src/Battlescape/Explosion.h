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

#ifndef OPENXCOM_EXPLOSION_H
#define OPENXCOM_EXPLOSION_H

#include "Position.h"


namespace OpenXcom
{

enum ExplosionType
{
	ET_AOE,			// 0
	ET_BULLET,		// 1
	ET_MELEE_ATT,	// 2
	ET_MELEE_HIT,	// 3
	ET_PSI,			// 4
	ET_TORCH		// 5
};


/**
 * This class represents an Explosion animation.
 * @note Map is the owner of an instance of this class during its short life. It
 * animates any/all types of explosion animations.
 */
class Explosion
{

private:
	static const int
		FRAMES_AOE			=  8,
		FRAMES_BULLET		= 10,
		FRAMES_MELEE_PSI	=  4,
		FRAMES_TORCH		=  6,
		START_FUSION		= 88;

	int
		_currentSprite,
		_startSprite,
		_startDelay;

	const ExplosionType _type;
	const Position _pos;


	public:
		/// Creates an Explosion.
		Explosion(
				const ExplosionType type,
				const Position pos,
				int startSprite,
				int startDelay = 0);
		/// Cleans up the Explosion.
		~Explosion();

		/// Steps the Explosion forward.
		bool animate();

		/// Gets the Explosion's position in voxel-space.
		const Position getPosition() const;

		/// Gets the currently playing sprite-ID.
		int getCurrentSprite() const;

		/// Gets the Explosion's type.
		ExplosionType getExplosionType() const;
};

}

#endif
