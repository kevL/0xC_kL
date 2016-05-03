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

#include "CraftWeaponProjectile.h"


namespace OpenXcom
{

/**
 * Creates the CraftWeaponProjectile.
 */
CraftWeaponProjectile::CraftWeaponProjectile()
	:
		_type(PT_CANNON_ROUND),
		_globalType(PGT_MISSILE),
		_speed(0),
		_dir(PD_NONE),
		_pos(0),
		_posHori(PH_CENTER),
		_beamPhase(0),
		_accuracy(0),
		_power(0),
		_range(0),
		_done(false),
		_missed(false),
		_dist(0)
{}

/**
 * dTor.
 */
CraftWeaponProjectile::~CraftWeaponProjectile()
{}

/**
 * Sets the type of this CraftWeaponProjectile according to the type of weapon
 * that shot it.
 * @note This is used for drawing.
 * @param type - projectile-type (CraftWeaponProjectile.h)
 */
void CraftWeaponProjectile::setType(CwpType type)
{
	switch (_type = type)
	{
		case PT_LASER_BEAM:
		case PT_PLASMA_BEAM:
			_globalType = PGT_BEAM;
			_beamPhase = 8;
	}
}

/**
 * Gets the type of this CraftWeaponProjectile.
 * @return, projectile-type (CraftWeaponProjectile.h)
 */
CwpType CraftWeaponProjectile::getType() const
{
	return _type;
}

/**
 * Gets the GlobalType of this CraftWeaponProjectile.
 * @return, 0 if missile-type, 1 if beam-type (CraftWeaponProjectile.h)
 */
CwpGlobal CraftWeaponProjectile::getGlobalType() const
{
	return _globalType;
}

/**
 * Sets the y-direction of this CraftWeaponProjectile.
 * @param direction - y-direction
 */
void CraftWeaponProjectile::setDirection(CwpDirection dir)
{
	if ((_dir = dir) == PD_UP)
		_pos = 0;
}

/**
 * Gets the y-direction of this CraftWeaponProjectile.
 * @return, the y-direction
 */
CwpDirection CraftWeaponProjectile::getDirection() const
{
	return _dir;
}

/**
 * Moves this CraftWeaponProjectile according to its speed or changes the phase
 * of its beam animation as applicable to its GlobalType.
 */
void CraftWeaponProjectile::moveProjectile()
{
	switch (_globalType)
	{
		case PGT_MISSILE:
		{
			// Check if projectile would reach its maximum range this tick.
			int delta;
			if (_range > (_dist >> 3u)
				&& _range <= ((_dist + _speed) >> 3u))
			{
				delta = (_range << 3u) - _dist;
			}
			else
				delta = _speed;

			// Check if projectile passed its maximum range on previous tick.
			if (_range <= (_dist >> 3u))
				_missed = true;

			if (_dir == PD_UP)
				_pos += delta;
			else if (_dir == PD_DOWN)
				_pos -= delta;

			_dist += delta;
			break;
		}

		case PGT_BEAM:
			if ((_beamPhase >>= 1u) == 1)
				_done = true;
	}
}

/**
 * Sets the y-position of this CraftWeaponProjectile on the player's
 * dogfight-radar.
 * @param pos - y-position
 */
void CraftWeaponProjectile::setPosition(int pos)
{
	_pos = pos;
}

/**
 * Gets the y-position of this CraftWeaponProjectile on the player's
 * dogfight-radar.
 * @return, the y-position
 */
int CraftWeaponProjectile::getPosition() const
{
	return _pos;
}

/**
 * Sets the x-position of this CraftWeaponProjectile on the player's
 * dogfight-radar.
 * @note This is used only once for each projectile during firing.
 * @param pos - the x-position
 */
void CraftWeaponProjectile::setHorizontalPosition(int pos)
{
	_posHori = pos;
}

/**
 * Gets the x-position of this CraftWeaponProjectile on the player's
 * dogfight-radar.
 * @return, the x-position
 */
int CraftWeaponProjectile::getHorizontalPosition() const
{
	return _posHori;
}

/**
 * Flags this CraftWeaponProjectile for removal.
 */
void CraftWeaponProjectile::removeProjectile()
{
	_done = true;
}

/**
 * Checks if this CraftWeaponProjectile should be removed.
 * @return, true to remove
 */
bool CraftWeaponProjectile::toBeRemoved() const
{
	return _done;
}

/**
 * Gets the animation-phase of this CraftWeaponProjectile if beam-type.
 * @return, the phase
 */
int CraftWeaponProjectile::getBeamPhase() const
{
	return _beamPhase;
}

/**
 * Sets the amount of damage this CraftWeaponProjectile can do if it hits its
 * target.
 * @param power - the damage
 */
void CraftWeaponProjectile::setPower(int power)
{
	_power = power;
}

/**
 * Gets the amount of damage this CraftWeaponProjectile can do if it hits its
 * target.
 * @return, the damage
 */
int CraftWeaponProjectile::getPower() const
{
	return _power;
}

/**
 * Sets the accuracy of this CraftWeaponProjectile.
 * @param accuracy - accuracy
 */
void CraftWeaponProjectile::setAccuracy(int accuracy)
{
	_accuracy = accuracy;
}

/**
 * Gets the accuracy of this CraftWeaponProjectile.
 * @return, the accuracy
 */
int CraftWeaponProjectile::getAccuracy() const
{
	return _accuracy;
}

/**
 * Sets this CraftWeaponProjectile as having missed its target.
 * @param missed - true for missed (default true)
 */
void CraftWeaponProjectile::setMissed(bool missed)
{
	_missed = missed;
}

/**
 * Gets if this CraftWeaponProjectile missed its target.
 * @return, true if missed
 */
bool CraftWeaponProjectile::getMissed() const
{
	return _missed;
}

/**
 * Sets the maximum range of this CraftWeaponProjectile.
 * @param range - the range
 */
void CraftWeaponProjectile::setRange(int range)
{
	_range = range;
}

/**
 * Gets the maximum range of this CraftWeaponProjectile.
 * @return, the range
 */
int CraftWeaponProjectile::getRange() const
{
	return _range;
}

/**
 * Sets the speed of this CraftWeaponProjectile if projectile-type.
 * @param speed - the speed
 */
void CraftWeaponProjectile::setSpeed(int speed)
{
	_speed = speed;
}

}
