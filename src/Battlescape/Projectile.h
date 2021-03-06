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

#ifndef OPENXCOM_PROJECTILE_H
#define OPENXCOM_PROJECTILE_H

//#include <vector>

#include "BattlescapeGame.h"
#include "Position.h"

#include "../Engine/Surface.h"

#include "../Ruleset/MapData.h"


namespace OpenXcom
{

class BattleItem;
class ResourcePack;
class SavedBattleGame;
class Surface;
class Tile;


/**
 * A class that represents a Projectile.
 * @note Map is the owner of an instance of this class during its short life.
 * It calculates a trajectory and then steps along this pre-calculated
 * trajectory in voxel space.
 */
class Projectile
{

private:
	static const double PCT;
	static Position targetVoxel_cache;


//	bool _reversed;
	bool _forced;
	int
		_bulletSprite,
		_speed;
	size_t _trjId;

	BattleAction _action;
	Position
		_posOrigin,
		_targetVoxel;
	const SavedBattleGame* _battleSave;
	Surface* _throwSprite;

	std::vector<Position> _trj;

	/// Calculates a final target in voxel-space.
	void applyAccuracy(
			const Position& originVoxel,
			Position* const targetVoxel,
			double accuracy,
			const Tile* const tileTarget);
	/// Gets distance modifiers to accuracy.
	double rangeAccuracy( // private.
			const RuleItem* const itRule,
			int dist) const;
	/// Gets target-terrain and/or target-unit modifiers to accuracy.
	double targetAccuracy(
			const BattleUnit* const targetUnit,
			int elevation,
			const Tile* tileTarget) const;
	/// Verifies that a targeted position has LoF.
	bool verifyTarget(
			const Position& originVoxel,
			bool excludeActor = true);


	public:
		/// Creates a Projectile.
		Projectile(
				const ResourcePack* const res,
				const SavedBattleGame* const battleSave,
				const BattleAction& action,
				const Position& posOrigin,
				const Position& targetVoxel);
		/// Cleans up the Projectile.
		~Projectile();

		/// Calculates the trajectory of a line-path for a non-BL weapon.
		VoxelType calculateShot(double accuracy);
		/// Calculates the trajectory of a line-path for a Blaster Launch.
		VoxelType calculateShot(
				double accuracy,
				const Position& originVoxel,
				bool useExclude = true);
		/// Calculates the trajectory for a curved path.
		VoxelType calculateThrow(double accuracy);

		/// Forwards the Projectile one step along its trajectory.
		bool traceProjectile();
		/// Skips flight.
		void skipTrajectory();

		/// Gets the Projectile's current position in voxel-space.
		Position getPosition(int offset = 0) const;
		/// Gets an ID for the Projectile's current surface.
		int getBulletSprite(int id) const;
		/// Gets the thrown item.
		BattleItem* getThrowItem() const;
		/// Gets the thrown item's sprite.
		Surface* getThrowSprite() const;

		/// Gets the BattleAction associated with the Projectile.
		BattleAction* getBattleAction();

		/// Gets the ACTUAL target-position for the Projectile.
		Position getFinalPosition() const;
		/// Gets the final direction of the Projectile's trajectory as a unit-vector.
		Position getStrikeVector() const;

		/// Sets a forced-shot against a Unit.
		void setForced();

		/// Stores the final direction of a missile or thrown-object.
//		void storeProjectileDirection() const;
		/// Gets the Position of origin for the Projectile.
//		Position getOrigin() const;
		/// Gets the INTENDED target-position for the Projectile.
//		Position getTarget() const;
		/// Gets if this the Projectile is being drawn back-to-front or front-to-back.
//		bool isReversed() const;
};

}

#endif
