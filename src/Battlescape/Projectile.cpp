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

#include "Projectile.h"

#include "../fmath.h"

#include "Map.h"
#include "Camera.h"
#include "Pathfinding.h"
#include "TileEngine.h"

//#include "../Engine/Logger.h"
//#include "../Engine/Options.h"
#include "../Engine/RNG.h"
#include "../Engine/SurfaceSet.h"

#include "../Resource/ResourcePack.h"

//#include "../Ruleset/RuleItem.h"
#include "../Ruleset/Ruleset.h"
#include "../Ruleset/RuleSoldier.h"

#include "../Savegame/BattleItem.h"
//#include "../Savegame/BattleUnit.h"
#include "../Savegame/SavedBattleGame.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/Tile.h"


namespace OpenXcom
{

Position Projectile::targetVoxel_cache; // static across all Projectile invokations <-

const double Projectile::PCT = 0.01;


/**
 * Creates a Projectile on the battlefield and calculates its trajectory.
 * @param res			- pointer to ResourcePack
 * @param battleSave	- pointer to SavedBattleGame
 * @param action		- reference the BattleAction (BattlescapeGame.h)
 * @param posOrigin		- reference position that this projectile originates at in tile-space
 * @param targetVoxel	- reference position that this projectile is targeted at in voxel-space
 */
Projectile::Projectile(
		const ResourcePack* const res,
		const SavedBattleGame* const battleSave,
		const BattleAction& action,
		const Position& posOrigin,
		const Position& targetVoxel)
	:
		_battleSave(battleSave),
		_action(action),
		_posOrigin(posOrigin),
		_targetVoxel(targetVoxel),
		_forced(false),
		_trjId(0u),
		_bulletSprite(-1)
{
	//Log(LOG_INFO) << "";
	//Log(LOG_INFO) << "cTor origin = " << posOrigin;
	//Log(LOG_INFO) << "cTor target = " << targetVoxel << " tSpace " << (targetVoxel / Position(16,16,24));

	//Log(LOG_INFO) << "Projectile cTor";
	//Log(LOG_INFO) << ". action.weapon = " << _action.weapon->getRules()->getType();
	//Log(LOG_INFO) << ". bullet = " << _action.weapon->getAmmoItem()->getRules()->getType();

	_speed = Options::battleFireSpeed; // this is the distance in pixels that the sprite will move between frames

	if (_action.weapon != nullptr)
	{
		//Log(LOG_INFO) << "Create Projectile -> " << BattleAction::debugBat(_action.type);
		switch (_action.type) // NOTE: Melee and Psi attacks won't get this far.
		{
			case BA_THROW:
				_throwSprite = res->getSurfaceSet("FLOOROB.PCK")->getFrame(_action.weapon->getRules()->getFloorSprite());
				_speed /= 5;
				break;

//			default: // ba_SHOOT!! or hit, or spit
			case BA_SNAPSHOT:
			case BA_AUTOSHOT:
			case BA_AIMEDSHOT:
				if (_action.weapon->getRules()->isArcingShot() == true)
					_speed >>= 1u;
				// no break;
			case BA_LAUNCH:
			{
				const BattleItem* const bullet (_action.weapon->getAmmoItem()); // the weapon itself if not-req'd. eg, lasers
				if (bullet != nullptr) // try to get the required info from the bullet
				{
					_bulletSprite = bullet->getRules()->getBulletSprite();
					_speed += bullet->getRules()->getBulletSpeed();
				}

				// if no bullet or the bullet doesn't contain the required info see what the weapon has to offer.
				if (_bulletSprite == -1)
					_bulletSprite = _action.weapon->getRules()->getBulletSprite();

				if (_speed == Options::battleFireSpeed)
					_speed += _action.weapon->getRules()->getBulletSpeed();

//				if (_action.type == BA_AUTOSHOT) _speed *= 3;

//				if (_bulletSprite == -1) // shotguns don't have bullet-sprites ...
//				{
//					std::ostringstream oststr;
//					oststr << "Missing bullet sprite for " << _action.weapon->getRules()->getType();
//					if (_action.weapon->getAmmoItem() != nullptr
//						&& _action.weapon->getAmmoItem() != _action.weapon)
//					{
//						oststr << " w/ " << _action.weapon->getAmmoItem()->getRules()->getType();
//					}
//					Log(LOG_WARNING) << oststr.str();
//				}
			}
		}
	}

	if (_speed < 1) _speed = 1;
	//Log(LOG_INFO) << "Projectile cTor EXIT";
}

/**
 * Deletes the Projectile.
 */
Projectile::~Projectile()
{}

/**
 * Calculates the trajectory for a straight/line path.
 * @note Accuracy affects the VoxelType result. This is a wrapper for
 * calculateShot() below - it calculates and passes on the acting unit's
 * originVoxel.
 * @param accuracy - accuracy of the projectile's trajectory (a battleunit's accuracy)
 * @return, VoxelType (MapData.h) see calculateShot() below_
 */
VoxelType Projectile::calculateShot(double accuracy)
{
	return calculateShot(
					accuracy,
					_battleSave->getTileEngine()->getOriginVoxel(
															_action,
															_battleSave->getTile(_posOrigin)));
}

/**
 * Calculates the trajectory for a straight/line path.
 * @note Accuracy affects the VoxelType result. First determines if there is LoF
 * then calculates and stores a modified trajectory that is actually pathed.
 * @param accuracy		- accuracy of the projectile's trajectory (a BattleUnit's accuracy)
 * @param originVoxel	- for Blaster launch; ie trajectories that start at a position other than unit's
 * @param useExclude	- true for normal shots; false for BL-waypoints (default true)
 * @return, VoxelType (MapData.h)
 *			 -1 nothing to hit / no line of fire
 *			0-3 tile-part (floor / westwall / northwall / object)
 *			  4 unit
 *			  5 out-of-map
 * VOXEL_EMPTY			// -1
 * VOXEL_FLOOR			//  0
 * VOXEL_WESTWALL		//  1
 * VOXEL_NORTHWALL		//  2
 * VOXEL_OBJECT			//  3
 * VOXEL_UNIT			//  4
 * VOXEL_OUTOFBOUNDS	//  5
 */
VoxelType Projectile::calculateShot(
		double accuracy,
		const Position& originVoxel,
		bool useExclude)
{
	//Log(LOG_INFO) << "Projectile::calculateShot() accuracy = " << accuracy;
	// test for LoF
	if (_action.actor->getFaction() == FACTION_PLAYER // aLiens don't even get in here!
		&& _action.autoShotCount == 1
		&& _action.type != BA_LAUNCH
		&& _battleSave->getBattleGame()->playerPanicHandled() == true
		&& ((	   (SDL_GetModState() & KMOD_CTRL)	== 0
				&& (SDL_GetModState() & KMOD_ALT)	== 0
				&& (SDL_GetModState() & KMOD_SHIFT)	== 0)
			|| Options::battleForceFire == false))
	{
		//Log(LOG_INFO) << "originVoxel " << originVoxel << " useExclude = " << useExclude;
		if (verifyTarget(originVoxel, useExclude) == false)
		{
			//Log(LOG_INFO) << ". ret VOXEL_EMPTY";
			return VOXEL_EMPTY; // <- that is, FAIL.
		}
	}

	_trj.clear();
	//Log(LOG_INFO) << ". autoshotCount[1] = " << _action.autoShotCount;

	// guided missiles drift, but how much is based on the shooter's faction rather than accuracy.
//	if (_action.type == BA_LAUNCH)
//	{
//		extendLine = _action.waypoints.size() < 2;
//		if (_action.actor->getFaction() == FACTION_PLAYER) accuracy = 0.60;
//		else accuracy = 0.55;
//	}

	//Log(LOG_INFO) << "";
	//Log(LOG_INFO) << ". preAcu target = " << _targetVoxel << " tSpace " << (_targetVoxel / Position(16,16,24));
	if (_action.type != BA_LAUNCH // Could base BL.. on psiSkill, or sumthin'
		&& Position::toTileSpace(originVoxel) != Position::toTileSpace(_targetVoxel))
	{
		//Log(LOG_INFO) << ". preAcu target = " << _targetVoxel << " tSpace " << (_targetVoxel / Position(16,16,24));
		applyAccuracy( // apply some accuracy modifiers. This will result in a new target voxel:
					originVoxel,
					&_targetVoxel,
					accuracy,
					_battleSave->getTile(_action.posTarget));
		//Log(LOG_INFO) << ". postAcu target = " << _targetVoxel << " tSpace " << (_targetVoxel / Position(16,16,24));
	}

	const VoxelType impactType (_battleSave->getTileEngine()->plotLine( // finally do a line calculation and store the trajectory.
																	originVoxel,
																	_targetVoxel,
																	true,
																	&_trj,
																	_action.actor));
	//Log(LOG_INFO) << ". trajBegin = " << _trj.front() << " tSpace " << (_trj.front() / Position(16,16,24));
	//Log(LOG_INFO) << ". trajFinal = " << _trj.back() << " tSpace " << (_trj.back() / Position(16,16,24));
	if (_action.type == BA_AUTOSHOT)
	{
		//Log(LOG_INFO) << "set targetVoxel_cache = " << (_trj.back());
		targetVoxel_cache = _trj.back();
	}

	//Log(LOG_INFO) << ". RET voxelType = " << impactType;
	return impactType;
}

/**
 * Calculates the trajectory for a parabolic path.
 * @note Accuracy affects the VoxelType result.
 * @sa TileEngine::validateThrow()
 * @param accuracy - accuracy of the projectile's trajectory (a battleunit's accuracy)
 * @return, VoxelType (MapData.h)
 *			 -1 nothing to hit / no line of fire
 *			0-3 tile-part (floor / westwall / northwall / object)
 *			  4 unit
 *			  5 out-of-map
 * VOXEL_EMPTY			// -1
 * VOXEL_FLOOR			//  0
 * VOXEL_WESTWALL		//  1
 * VOXEL_NORTHWALL		//  2
 * VOXEL_OBJECT			//  3
 * VOXEL_UNIT			//  4
 * VOXEL_OUTOFBOUNDS	//  5
 */
VoxelType Projectile::calculateThrow(double accuracy)
{
	//Log(LOG_INFO) << "Projectile::calculateThrow()";
	const Position originVoxel (_battleSave->getTileEngine()->getOriginVoxel(_action));

	VoxelType impactType (VOXEL_OUTOFBOUNDS);
	double arc;
	//Log(LOG_INFO) << ". call TileEngine::validateThrow()";
	//Log(LOG_INFO) << "";
	if (_battleSave->getTileEngine()->validateThrow(
												_action,
												originVoxel,
												_targetVoxel,
												&arc,
												&impactType) == true)
	{
		//Log(LOG_INFO) << ". VALID";
		VoxelType impactTest (VOXEL_OUTOFBOUNDS);
		while (impactTest == VOXEL_OUTOFBOUNDS) // Do a parabola calculation and store the trajectory.
		{
			//Log(LOG_INFO) << ". . iter OUTOFBOUNDS";
			_trj.clear();

			Position deltaVoxel (_targetVoxel);
			applyAccuracy(
						originVoxel,
						&deltaVoxel,
						accuracy,
						_battleSave->getTile(_action.posTarget));

			deltaVoxel -= _targetVoxel;
			//Log(LOG_INFO) << ". call TileEngine::plotParabola() arc = " << arc;
			//Log(LOG_INFO) << "";
			impactTest = _battleSave->getTileEngine()->plotParabola(
																originVoxel,
																_targetVoxel,
																true,
																&_trj,
																_action.actor,
																arc,
																_action.type != BA_THROW,
																deltaVoxel);

			//Log(LOG_INFO) << ". . test = " << MapData::debugVoxelType(impactTest);
			// Don't let thrown items land on diagonal bigWalls.
			// This prevents exploiting the blast-propagation routine out from both sides of a diagonal bigWall.
			// Prevent Grenades from landing on diagonal bigWalls.
			// See also TileEngine::validateThrow().
			if (_action.type == BA_THROW && impactTest == VOXEL_OBJECT)
			{
				//Log(LOG_INFO) << ". . . NOT outofbounds";
				const Tile* const tile (_battleSave->getTile(Position::toTileSpace(_trj.back()))); // _trj.at(0)
				if (tile != nullptr && tile->getMapData(O_OBJECT) != nullptr) // safety. Should be unnecessary because impactTest=VOXEL_OBJECT ... bzzzttT.
//					&& tile->getMapData(O_OBJECT)->getTuCostPart(MT_WALK) == 255
//					&& action.weapon->getRules()->isGrenade() == true)
				{
					switch (tile->getMapData(O_OBJECT)->getBigwall())
					{
						case BIGWALL_NESW:
						case BIGWALL_NWSE:
							impactTest = VOXEL_OUTOFBOUNDS;
					}
				}
			}
		}
		//Log(LOG_INFO) << "ret = " << MapData::debugVoxelType(impactType);
		//Log(LOG_INFO) << "";
		return impactType;
	}
	//Log(LOG_INFO) << "ret VOXEL_OUTOFBOUNDS";
	//Log(LOG_INFO) << "";
	return VOXEL_OUTOFBOUNDS;
}

/**
 * Calculates a final target in voxel-space based on a given accuracy modifier.
 * @param originVoxel	- reference the start position of the trajectory in voxelspace
 * @param targetVoxel	- pointer to a position to store the end of the trajectory in voxelspace
 * @param accuracy		- accuracy modifier
 * @param tileTarget	- pointer to tile of the target
 */
void Projectile::applyAccuracy( // private.
		const Position& originVoxel,
		Position* const targetVoxel,
		double accuracy,
		const Tile* const tileTarget)
{
	//Log(LOG_INFO) << "Projectile::applyAccuracy() id-" << _action.actor->getId() << " acu= " << accuracy;
	//Log(LOG_INFO) << ". in x= " << targetVoxel->x;
	//Log(LOG_INFO) << ". in y= " << targetVoxel->y;
	//Log(LOG_INFO) << ". in z= " << targetVoxel->z;

	//Log(LOG_INFO) << "input Target = " << (*targetVoxel);
	static const double ACU_MIN (0.01);
	const int
		delta_x (originVoxel.x - targetVoxel->x),
		delta_y (originVoxel.y - targetVoxel->y);
	// Do not use Z-axis. It messes up pure vertical shots.
	const double targetDist (std::sqrt(
							 static_cast<double>((delta_x * delta_x) + (delta_y * delta_y))));
	//Log(LOG_INFO) << ". targetDist = " << targetDist;

	const RuleItem* const itRule (_action.weapon->getRules()); // <- after reading up, 'const' is basically worthless & wordy waste of effort.
	if (_action.type != BA_THROW
		&& itRule->isArcingShot() == false)
	{
		if (_action.autoShotCount == 1)
		{
			if (_action.actor->getFaction() == FACTION_PLAYER)	// kL: only for xCom heheh ->
//				&& Options::battleUFOExtenderAccuracy == true)	// not so sure that this should be Player only
			{													// if not the problem is that the AI does not adequately consider weapon ranges.
				accuracy += rangeAccuracy(
									itRule,
									static_cast<int>(Round(targetDist / 16.)));
			}

//			if (Options::battleRangeBasedAccuracy == true)
			accuracy += targetAccuracy(
								tileTarget->getTileUnit(), //_battleSave->getTileEngine()->getTargetUnit(tileTarget),
								targetVoxel->z - originVoxel.z,
								tileTarget);

			accuracy = std::max(ACU_MIN, accuracy);
			//Log(LOG_INFO) << "acu= " << accuracy;
		}
		else // 2nd+ shot of burst
		{
			*targetVoxel = targetVoxel_cache;
			//Log(LOG_INFO) << "get targetVoxel = " << (*targetVoxel) << " shot = " << _action.autoShotCount;
		}

		double
			deltaHori,
			deltaVert;
		static const double
			div_HORI (6.),
			div_VERT (div_HORI * 1.69);

		if (_action.autoShotCount == 1)
		{
//			const int autoHit (static_cast<int>(std::ceil(accuracy * 21.))); // chance for Bulls-eye.
//			if (RNG::percent(autoHit) == false)
//			{
				//Log(LOG_INFO) << ". NOT autoHit";
			double deviation;
			if (_action.actor->getFaction() == FACTION_HOSTILE)
				deviation = 0.15;	// give the poor aLiens an aiming advantage over xCom & Mc'd units
									// DO IT IN RULESET! not here. Okay do it here because of faction/MC-thing ...
			else
				deviation = 0.21;	// for Player & civies.

			deviation /= accuracy + 0.16;

			const int autoHit (static_cast<int>(std::ceil(accuracy * 21.))); // chance for Bulls-eye.
			//Log(LOG_INFO) << ". calc bulls-eye x= " << RNG::getSeed();
			if (RNG::percent(autoHit) == true)
				deviation /= 10.;

			deviation = std::max(ACU_MIN, deviation);
			//Log(LOG_INFO) << ". deviation= " << deviation;

			// The angle deviations are spread using a normal distribution:
			deltaHori = RNG::boxMuller(0., deviation / div_HORI); // horizontal miss in radians
			deltaVert = RNG::boxMuller(0., deviation / div_VERT); // vertical miss in radians
//			}
//			else
//			{
				//Log(LOG_INFO) << ". autoHit";
//				deltaHori =
//				deltaVert = 0.;
//			}
		}
		else // 2nd+ shot of burst.
		{
			// The angle deviations are spread using a normal distribution:
			double kick ((static_cast<double>(itRule->getAutoKick()) * PCT)
					   - (static_cast<double>(_action.actor->getStrength()) / 1000.));
			kick = std::max(kick, ACU_MIN);
			deltaHori = RNG::boxMuller(0., kick / div_HORI); // horizontal miss in radians
			deltaVert = RNG::boxMuller(0., kick / div_VERT); // vertical miss in radians
		}
		//Log(LOG_INFO) << "deltaHori= " << deltaHori;
		//Log(LOG_INFO) << "deltaVert= " << deltaVert;

		double
			te,fi,
			cos_fi;
		bool
			calcHori,
			calcVert;

		if (targetVoxel->y != originVoxel.y || targetVoxel->x != originVoxel.x)
		{
			calcHori = true;
			te = std::atan2(
						static_cast<double>(targetVoxel->y - originVoxel.y),
						static_cast<double>(targetVoxel->x - originVoxel.x)) + deltaHori;
		}
		else
		{
			calcHori = false;
			te = 0.; // avoid vc++ linker warnings
		}

		if (targetVoxel->z != originVoxel.z
			|| AreSame(targetDist, 0.) == false)
		{
			calcVert = true;
			fi = std::atan2(
						static_cast<double>(targetVoxel->z - originVoxel.z),
						targetDist) + deltaVert,
			cos_fi = std::cos(fi);
		}
		else
		{
			calcVert = false;
			cos_fi = 1.;
			fi = 0.; // avoid vc++ linker warnings
		}

		static const double OUTER_LIMIT (3200.);
		if (calcHori == true)
		{
			targetVoxel->x = static_cast<int>(Round(static_cast<double>(originVoxel.x)
						   + OUTER_LIMIT * std::cos(te) * cos_fi));
			targetVoxel->y = static_cast<int>(Round(static_cast<double>(originVoxel.y)
						   + OUTER_LIMIT * std::sin(te) * cos_fi));
		}
		if (calcVert == true)
			targetVoxel->z = static_cast<int>(Round(static_cast<double>(originVoxel.z)
						   + OUTER_LIMIT * std::sin(fi)));

		//Log(LOG_INFO) << ". out x= " << targetVoxel->x;
		//Log(LOG_INFO) << ". out y= " << targetVoxel->y;
		//Log(LOG_INFO) << ". out z= " << targetVoxel->z;
	}
	else // *** This is for Throwing /*and AcidSpitt*/ only ***
	{
		double perfect;

		if (itRule->isArcingShot() == true)
		{
			accuracy += rangeAccuracy(
									itRule,
									static_cast<int>(Round(targetDist / 16.)));
			accuracy += targetAccuracy(
									tileTarget->getTileUnit(), //_battleSave->getTileEngine()->getTargetUnit(tileTarget),
									targetVoxel->z - originVoxel.z,
									tileTarget);
			accuracy = std::max(ACU_MIN, accuracy);

			if (_action.actor->getGeoscapeSoldier() != nullptr)
				perfect = static_cast<double>(_battleSave->getBattleGame()->getRuleset()->getSoldier("STR_SOLDIER")->getStatCaps().firing);
			else
				perfect = 150.; // higher value makes non-Soldiers less accurate at spitting/arcing shot.
		}
		else // Throw
		{
			if (_action.actor->getGeoscapeSoldier() != nullptr)
				perfect = static_cast<double>(_battleSave->getBattleGame()->getRuleset()->getSoldier("STR_SOLDIER")->getStatCaps().throwing);
			else
				perfect = 150.; // higher value makes non-Soldiers less accurate at throwing.
		}

		accuracy = accuracy * 50. + 73.1; // arbitrary adjustment.

		double deviation (perfect - accuracy);
		deviation = std::max(ACU_MIN,
							 deviation * targetDist * PCT);

		const double
			dx (RNG::boxMuller(0., deviation) / 4.),
			dy (RNG::boxMuller(0., deviation) / 4.),
			dz (RNG::boxMuller(0., deviation) / 6.);

		//Log(LOG_INFO) << "Proj: applyAccuracy target[1] " << *targetVoxel;
		targetVoxel->x += static_cast<int>(Round(dx));
		targetVoxel->y += static_cast<int>(Round(dy));
		targetVoxel->z += static_cast<int>(Round(dz));
		//Log(LOG_INFO) << "Proj: applyAccuracy target[2] " << *targetVoxel;

		targetVoxel->x = std::max(0,
								  std::min(((_battleSave->getMapSizeX() - 1) << 4u) + 15,
											 targetVoxel->x));
		targetVoxel->y = std::max(0,
								  std::min(((_battleSave->getMapSizeY() - 1) << 4u) + 15,
											 targetVoxel->y));
		targetVoxel->z = std::max(0,
								  std::min(((_battleSave->getMapSizeZ() - 1)  * 24) + 23,
											 targetVoxel->z));

		if (_action.type == BA_THROW) // center end-point graphically on the tile it's headed for.
		{
			const Tile* const tile (_battleSave->getTile(Position::toTileSpace(*targetVoxel)));
			const int lift (std::max(2,
									 -tile->getTerrainLevel()));

			targetVoxel->x = (targetVoxel->x & 0xfff0) + 8;
			targetVoxel->y = (targetVoxel->y & 0xfff0) + 8;
			targetVoxel->z = (targetVoxel->z / 24 * 24) + lift;
		}
		//Log(LOG_INFO) << "Proj: applyAccuracy target[3] " << *targetVoxel;
	}
}
/*		if (extendLine == true)
		{
//			double maxDeviation = 2.5; // maxDeviation is the max angle deviation for accuracy 0% in degrees
//			double minDeviation = 0.4; // minDeviation is the min angle deviation for accuracy 100% in degrees
//			double dRot, dTilt;
//			double rotation, tilt;
//			double baseDeviation = (maxDeviation - (maxDeviation * accuracy)) + minDeviation;

//			// the angle deviations are spread using a normal distribution between 0 and baseDeviation
//			if (RNG::generate(0., 1.) < accuracy) // check if hit
//			{
//				dRot = 0.; // hit, so no deviation
//				dTilt = 0.;
//			}
//			else
//			{
//				dRot = RNG::boxMuller(0., baseDeviation);
//				dTilt = RNG::boxMuller(0., baseDeviation / 2.); // tilt deviation is halved
//			}
//			rotation += dRot; // add deviations
//			tilt += dTilt;

			const double
				rotation = std::atan2(
									static_cast<double>(target->y - origin.y),
									static_cast<double>(target->x - origin.x))
								* 180. / M_PI,
				tilt = std::atan2(
								static_cast<double>(target->z - origin.z),
								std::sqrt(
									  static_cast<double>(target->x - origin.x) * static_cast<double>(target->x - origin.x)
									+ static_cast<double>(target->y - origin.y) * static_cast<double>(target->y - origin.y)))
								* 180. / M_PI;

			// calculate new target
			// the new target can be very far out of the map, but disregard that
			const double
				cos_fi = std::cos(tilt * M_PI / 180.),
				sin_fi = std::sin(tilt * M_PI / 180.),
				cos_te = std::cos(rotation * M_PI / 180.),
				sin_te = std::sin(rotation * M_PI / 180.);

			target->x = static_cast<int>(static_cast<double>(origin.x) + range * cos_te * cos_fi);
			target->y = static_cast<int>(static_cast<double>(origin.y) + range * sin_te * cos_fi);
			target->z = static_cast<int>(static_cast<double>(origin.z) + range * sin_fi);
		} */

/**
 * Gets distance-modifier to accuracy.
 * @param itRule	- pointer to RuleItem of weapon
 * @param dist		- distance to target in tilespace
 * @return, linear accuracy modification
 */
double Projectile::rangeAccuracy( // private.
		const RuleItem* const itRule,
		int dist) const
{
	const int shortRange (itRule->getMinRange());
	if (dist < shortRange)
		return static_cast<double>((shortRange - dist) * itRule->getDropoff()) * PCT;

	int longRange;
	switch (_action.type)
	{
		case BA_SNAPSHOT: longRange = itRule->getSnapRange(); break;
		case BA_AUTOSHOT: longRange = itRule->getAutoRange(); break;
		default:		  longRange = itRule->getAimRange();
	}

	if (longRange < dist)
		return static_cast<double>((dist - longRange) * itRule->getDropoff()) * PCT;

	return 0.;
}

/**
 * Gets target-terrain and/or target-unit modifier to accuracy.
 * @param targetUnit	- pointer to a BattleUnit
 * @param elevation		- vertical offset of target in voxels
 * @param tileTarget	- pointer to a Tile (nullptr unless force-firing on tile-parts)
 * @return, linear accuracy modification
 */
double Projectile::targetAccuracy( // private.
		const BattleUnit* const targetUnit,
		int elevation,
		const Tile* tileTarget) const
{
	int ret (0);

	if (targetUnit != nullptr)
	{
		if (targetUnit->isKneeled() == true)
			ret -= 7;

		if (targetUnit->isDashing() == true)
			ret -= 16;

		if (tileTarget == nullptr) tileTarget = targetUnit->getTile();
	}

	if (tileTarget != nullptr)
	{
		ret -= tileTarget->getSmoke(); // TODO: add Smoke-values for tiles enroute [per TE::visible()].
		ret -= tileTarget->getShade();
	}

	if (_action.actor->isMindControlled() == false)
		ret -= 10 - ((_action.actor->getMorale() + 9) / 10); // low morale decreases accuracy unless Mc'd

	ret -= elevation / 6; // +/-1 per 6 delta. // bonus for elevation advantage, penalty for disadvantage.

	return static_cast<double>(ret) * PCT;
}

/**
 * Verifies that a targeted position really has LoF.
 * @note Go figure. Checks if the voxel with a/the previously determined
 * VoxelType is really a voxel in the target tile and if not then if it's an
 * acceptable substitute in an adjacent tile.
 * @param originVoxel	- origin in voxel-space
 * @param useExclude	- true for normal shots; false for BL-waypoints (default true)
 * @return, true if shot allowed
 */
bool Projectile::verifyTarget( // private.
		const Position& originVoxel,
		bool useExclude)
{
	const BattleUnit* excludeUnit;
	if (useExclude == true)
		excludeUnit = _action.actor;
	else
		excludeUnit = nullptr;

	//Log(LOG_INFO) << "Verify ... call plotLine() targetVoxel " << _targetVoxel;
	const VoxelType voxelType (_battleSave->getTileEngine()->plotLine(
																	originVoxel,
																	_targetVoxel,
																	false,
																	&_trj,
																	excludeUnit));
	//Log(LOG_INFO) << ". voxelType = " << (int)voxelType;
	//Log(LOG_INFO) << ". isTrj = " << (int)(_trj.empty() == false);
	if (voxelType != VOXEL_EMPTY && _trj.empty() == false)
	{
		Position posTest (Position::toTileSpace(_trj.at(0u)));
		//Log(LOG_INFO) << ". posTest " << posTest << " posTarget" << _action.target;

//		if (voxelType == VOXEL_UNIT) // <- moved below ->
//		{
//			const Tile* const tileTest (_battleSave->getTile(posTest));
//			if (tileTest != nullptr && tileTest->getTileUnit() == nullptr) // must be poking head up from tileBelow
//				posTest += Position(0,0,-1);
//		}

		//Log(LOG_INFO) << ". result = " << _action.result;
		if (posTest != _action.posTarget) // && _action.result.empty() == true) // -> a non-empty _action.result should never get here.
		{
			switch (voxelType)
			{
				case VOXEL_NORTHWALL:
					if (posTest.y - 1 != _action.posTarget.y && _forced == false)
						return false;
					break;

				case VOXEL_WESTWALL:
					if (posTest.x - 1 != _action.posTarget.x && _forced == false)
						return false;
					break;

				case VOXEL_UNIT:
				{
					const Tile* const tileTest (_battleSave->getTile(posTest));		// -> from above <-
					if (tileTest != nullptr && tileTest->getTileUnit() == nullptr)	// must be poking head up from tileBelow
						posTest += Position(0,0,-1);

					const BattleUnit
						* const targetUnit (_battleSave->getTile(_action.posTarget)->getTileUnit()),
						* const testUnit (_battleSave->getTile(posTest)->getTileUnit());
					if (testUnit != targetUnit && testUnit->getUnitVisible() == true)
						return false;
					break;
				}

				case VOXEL_FLOOR:
				case VOXEL_OBJECT:
					if (_forced == false) return false; // TODO: This correctly.
					break;

				default:
				case VOXEL_OUTOFBOUNDS:
					return false;
			}
		}
	}
	return true;
}

/**
 * Steps this Projectile further along its trajectory.
 * @return, true if projectile is still pathing
 */
bool Projectile::traceProjectile()
{
	for (int
			i = 0;
			i != _speed;
			++i)
	{
		if (++_trjId == _trj.size())
		{
			--_trjId; // don't pass the end of the _trj vector
			return false;
		}
	}
	return true;
}

/**
 * Skips to the end of this Projectile's trajectory.
 */
void Projectile::skipTrajectory()
{
	_trjId = _trj.size() - 1u;
}

/**
 * Gets the current position of this Projectile in voxel space.
 * @param offset - offset of the trajectory-vector entry (default 0)
 * @return, position in voxel-space
 */
Position Projectile::getPosition(int offset) const
{
	if (offset == 0) return _trj.at(_trjId);

	offset = std::max(0,
					  std::min(
							static_cast<int>(_trjId) + offset,
							static_cast<int>(_trj.size() - 1u)));

	return _trj.at(static_cast<size_t>(offset));
}

/**
 * Gets an ID from this Projectile's surfaces.
 * @param id - index
 * @return, particle id
 */
int Projectile::getBulletSprite(int id) const
{
	if (_bulletSprite != -1)
		return _bulletSprite + id;

	return -1;
}

/**
 * Gets this Projectile's thrown item if it exists.
 * @return, pointer to a BattleItem or nullptr
 */
BattleItem* Projectile::getThrowItem() const
{
	if (_action.type == BA_THROW)
		return _action.weapon;

	return nullptr;
}

/**
 * Gets the thrown object's sprite.
 * @return, pointer to Surface
 */
Surface* Projectile::getThrowSprite() const
{
	return _throwSprite;
}

/**
 * Gets the BattleAction associated with this Projectile.
 * @return, pointer to the BattleAction
 */
BattleAction* Projectile::getBattleAction()
{
	return &_action;
}

/**
 * Gets the ACTUAL target-position for this Projectile.
 * @note It is important to note that we use the final position of the
 * projectile here.
 * @return, trajectory finish as a tile position
 */
Position Projectile::getFinalPosition() const
{
	return Position::toTileSpace(_trj.back());
}

/**
 * Gets the final direction of this Projectile's trajectory as a unit-vector.
 * @return, a unit vector indicating final direction
 */
Position Projectile::getStrikeVector() const
{
	Position posVect; // inits to Position(0,0,0)

	const size_t trjSize (_trj.size());
	if (trjSize > 2u)
	{
		const Position
			posFinal (_trj.back()),
			posPre (_trj.at(trjSize - 3u));

		int
			x,y;

		if (posFinal.x - posPre.x != 0)
		{
			if (posFinal.x - posPre.x > 0)
				x = 1;
			else
				x = -1;
		}
		else
			x = 0;

		if (posFinal.y - posPre.y != 0)
		{
			if (posFinal.y - posPre.y > 0)
				y = 1;
			else
				y = -1;
		}
		else
			y = 0;

		posVect = Position(x,y,0);
	}

	// TODO: Put some kind of safety if (x=0,y=0) after return to calling function;
	// that is, isolate vertical shots & figure out what to do.
	return posVect;
}

/**
 * Sets a forced-shot against a Unit.
 * @note This is used in rare circumstances when TileEngine::canTargetUnit()
 * determines a targetVoxel that's inbetween a targetUnit's upper and lower
 * exposed areas -- so that verifyTarget() will allow a shot against a Voxel_Unit
 * despite the plotted voxel belonging to an object-part. In this case you'll
 * need to *miss* the voxel to actually hit the targeted BattleUnit.
 * TODO: that.
 */
void Projectile::setForced()
{
	_forced = true;
}

}

/**
 * Stores the final direction of a missile or thrown-object for use by
 * TileEngine blast propagation.
 * @note This is to prevent blasts from propagating on both sides of diagonal
 * BigWalls. TODO: the blast itself needs tweaking in TileEngine ....
 * @note Superceded by getStrikeVector() above^
 *
void Projectile::storeProjectileDirection() const
{
	int dir = -1;
	const size_t trjSize = _trj.size();
	if (trjSize > 2)
	{
		const Position
			posFinal = _trj.back(),
			posPre = _trj.at(trjSize - 3);
		int x,y;
		if (posFinal.x - posPre.x != 0)
		{
			if (posFinal.x - posPre.x > 0) x = 1;
			else x = -1;
		}
		else x = 0;
		if (posFinal.y - posPre.y != 0)
		{
			if (posFinal.y - posPre.y > 0) y = 1;
			else y = -1;
		}
		else y = 0;
		Pathfinding::vectorToDirection(Position(x,y,0), dir);
	}
	_battleSave->getTileEngine()->setProjectileDirection(dir);
} */

/**
 * Gets the Position of origin for the projectile.
 * @note Instead of using the actor's position use the voxel origin translated
 * to a tile position - this is a workaround for large units.
 * @return, origin as a tile position
 *
Position Projectile::getOrigin() const
{
	return _trj.front() / Position(16,16,24); // returning this by const& might be okay due to 'extended temporaries' in C++
} */

/**
 * Gets the INTENDED target for this projectile.
 * @note It is important to note that we do not use the final position of the
 * projectile here but rather the targeted tile.
 * @return, intended target as a tile position
 *
Position Projectile::getTarget() const
{
	return _action.target; // returning this by const& might be okay
} */

/**
 * Gets if this projectile is to be drawn left to right or right to left.
 * @return, true if this is to be drawn in reverse order
 *
bool Projectile::isReversed() const
{
	return _reversed;
} */
/*	if ((targetVoxel.x - origin.x) + (targetVoxel.y - origin.y) > -1) // was in cTor.
//		_reversed = true;
//	else
//		_reversed = false; */
/*	NE	 0	reversed
	ENE		reversed
	E	 1	reversed
	ESE		reversed
	SE	 2	reversed
	SSE		reversed
	S	 1	reversed
	SSW		reversed
	SW	 0	reversed
	WSW		not reversed
	W	-1	not reversed
	WNW		not reversed
	NW	-2	not reversed
	NNW		not reversed
	N	-1	not reversed
	NNE		not reversed */
