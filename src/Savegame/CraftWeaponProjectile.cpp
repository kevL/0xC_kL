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
		_finished(false),
		_passed(false),
		_dist(0) // TODO: sort out the distance-units thing.
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
	if ((_dir = dir) == PD_CRAFT)
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
				_passed = true;

			int delta; // check if projectile will reach its max-range this tick

			if (_range > _dist && _range <= _dist + _speed)
				delta = _range - _dist;
			else
				delta = _speed;

//			switch (_dir)	// -> at present there are no PGT_MISSILE w/ PD_UFO for UFOs, which fire PGT_BREAM exclusively;
//			{				// in fact I don't think the Dogfight code even accounts for such a case currently.
//				case PD_CRAFT:	_pos += delta; break;
//				case PD_UFO:	_pos -= delta;
//			}
			_pos += delta;

			_dist += delta;
			break;
		}

		case PGT_BEAM:
			if ((_beamPhase >>= 1u) == 1u)
				_finished = true;
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
 * @param pixels - true to convert from ruleset's value to pixels (default false)
 * @return, the y-position
 */
int CraftWeaponProjectile::getCwpPosition(bool pixels) const
{
	if (pixels == true)
		return _pos >> 3u;

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
 * Sets this CraftWeaponProjectile as finished and should be deleted.
 */
void CraftWeaponProjectile::setFinished()
{
	_finished = true;
}

/**
 * Checks if this CraftWeaponProjectile has finished and should be deleted.
 * @return, true to delete
 */
bool CraftWeaponProjectile::getFinished() const
{
	return _finished;
}

/**
 * Gets the animation-phase of this CraftWeaponProjectile if beam-type.
 * @return, the current phase
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
 * Sets this CraftWeaponProjectile as having passed its target.
 * @note Only for projectiles not beams.
 */
void CraftWeaponProjectile::setPassed()
{
	_passed = true;
}

/**
 * Checks if this CraftWeaponProjectile passed its target.
 * @note Only for projectiles not beams.
 * @return, true if the projectile missed its target
 */
bool CraftWeaponProjectile::getPassed() const
{
	return _passed;
}

/**
 * Sets the maximum range of this CraftWeaponProjectile.
 * @param range		- the range
 * @param convert	- true to convert from "kilometers" to Dogfight distance (default false)
 */
void CraftWeaponProjectile::setCwpRange(
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
int CraftWeaponProjectile::getCwpRange() const
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
