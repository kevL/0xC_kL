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
		_beamPhase(8u),
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
void CraftWeaponProjectile::stepProjectile()
{
	switch (_globalType)
	{
		case PGT_MISSILE:
		{
			if (_dist > _range) // check if projectile passed its max-range on previous tick
				_missed = true;

			int delta; // check if projectile will reach its max-range this tick

			if (_range > _dist && _range <= _dist + _speed)
				delta = _range - _dist;
			else
				delta = _speed;

//			switch (_dir)	// -> at present there are no PGT_MISSILE w/ PD_DOWN for UFOs, which fire PGT_BREAM exclusively;
//			{				// in fact I don't think the Dogfight code even accounts for such a case currently.
//				case PD_UP:
			_pos += delta; //break;
//				case PD_DOWN:
//					_pos -= delta;
//			}

			_dist += delta;
			break;
		}

		case PGT_BEAM:
			if ((_beamPhase = static_cast<Uint8>(_beamPhase >> 1u)) == 1u)
				_done = true;
	}
}

/**
 * Sets the y-position of this CraftWeaponProjectile on the player's
 * dogfight-radar.
 * @param pos - y-position
 */
void CraftWeaponProjectile::setCwpPosition(int pos)
{
	_pos = pos;
}

/**
 * Gets the y-position of this CraftWeaponProjectile on the player's
 * dogfight-radar.
 * @return, the y-position
 */
int CraftWeaponProjectile::getCwpPosition() const
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
void CraftWeaponProjectile::endProjectile()
{
	_done = true;
}

/**
 * Checks if this CraftWeaponProjectile should be removed.
 * @return, true to remove
 */
bool CraftWeaponProjectile::isFinished() const
{
	return _done;
}

/**
 * Gets the animation-phase of this CraftWeaponProjectile if beam-type.
 * @return, the phase
 */
Uint8 CraftWeaponProjectile::getBeamPhase() const
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
 * @param range		- the range
 * @param convert	- true to convert from "kilometers" to Dogfight distance (default false)
 */
void CraftWeaponProjectile::setRange(
		int range,
		bool convert)
{
	if (convert == true) range <<= 3u;
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
