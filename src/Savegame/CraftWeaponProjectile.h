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

#ifndef OPENXCOM_WEAPONPROJECTILE_H
#define OPENXCOM_WEAPONPROJECTILE_H

#include <SDL.h> // Uint8


namespace OpenXcom
{

// Do not change the order of these enums because they are related to blob order.
enum CwpGlobal
{
	PGT_MISSILE,	// 0
	PGT_BEAM		// 1
};

enum CwpType
{
	PT_STINGRAY_MISSILE,	// 0
	PT_AVALANCHE_MISSILE,	// 1
	PT_CANNON_ROUND,		// 2
	PT_FUSION_BALL,			// 3
	PT_LASER_BEAM,			// 4
	PT_PLASMA_BEAM			// 5
};

enum CwpDirection
{
	PD_NONE,	// 0
	PD_UP,		// 1
	PD_DOWN		// 2
};

const int
	PH_LEFT		= -1,
	PH_CENTER	=  0,
	PH_RIGHT	=  1;


class CraftWeaponProjectile
{

private:
	bool
		_done,
		_missed;
	int
		_accuracy,
		_dist,		// large #
		_pos,		// large #
					// relative to interceptor, apparently, which is a problem
					// when the interceptor disengages while projectile is in flight.

					// kL_note: also, something screws with when a missile is launched
					// but UFO is downed, by other weapon, before it hits; the missile
					// is then not removed from the craft's ordnance.
		_posHori,
		_power,
		_range,		// small #
		_speed;		// large #

	Uint8 _beamPhase;

	CwpDirection _dir;
	CwpGlobal _globalType;
	CwpType _type;


	public:
		/// Creates a CraftWeaponProjectile.
		CraftWeaponProjectile();
		/// dTor.
		~CraftWeaponProjectile();

		/// Sets the CraftWeaponProjectile's type. This determines its speed.
		void setType(CwpType type);
		/// Gets the CraftWeaponProjectile's type.
		CwpType getType() const;
		/// Gets the CraftWeaponProjectile's GlobalType.
		CwpGlobal getGlobalType() const;

		/// Sets the CraftWeaponProjectile's direction. This determines its initial position.
		void setDirection(CwpDirection dir);
		/// Gets the CraftWeaponProjectile's direction.
		CwpDirection getDirection() const;

		/// Moves the CraftWeaponProjectile in direction '_dir' with speed '_speed'.
		void stepProjectile();

		/// Sets the CraftWeaponProjectile's y-position.
		void setCwpPosition(int pos);
		/// Gets the CraftWeaponProjectile's y-position.
		int getCwpPosition() const;
		/// Sets the CraftWeaponProjectile's horizontal position. This determines from which weapon it has been fired.
		void setHorizontalPosition(int pos);
		/// Gets the CraftWeaponProjectile's horizontal position.
		int getHorizontalPosition() const;

		/// Flags the CraftWeaponProjectile for removal.
		void endProjectile();
		/// Checks if the CraftWeaponProjectile should be removed.
		bool isFinished() const;

		/// Gets the state of beam-type CraftWeaponProjectile.
		Uint8 getBeamPhase() const;

		/// Sets the power of the CraftWeaponProjectile.
		void setPower(int power);
		/// Gets the power of the CraftWeaponProjectile.
		int getPower() const;

		/// Sets the accuracy of the CraftWeaponProjectile.
		void setAccuracy(int accuracy);
		/// Gets the accuracy of the CraftWeaponProjectile.
		int getAccuracy() const;

		/// Sets the CraftWeaponProjectile to a 'missed' status.
		void setMissed(bool missed = true);
		/// Gets the CraftWeaponProjectile's 'missed' status.
		bool getMissed() const;

		/// Sets the maximum range of the CraftWeaponProjectile.
		void setRange(
				int range,
				bool convert = false);
		/// Gets the maximum range of the CraftWeaponProjectile.
		int getRange() const;

		/// Sets the speed of a missile-type CraftWeaponProjectile.
		void setSpeed(const int speed);
};

}

#endif
