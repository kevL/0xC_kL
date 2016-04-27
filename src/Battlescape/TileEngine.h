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

#ifndef OPENXCOM_TILEENGINE_H
#define OPENXCOM_TILEENGINE_H

//#include <vector>
//#include <SDL.h>

#include "BattlescapeGame.h"
#include "Position.h"

#include "../Ruleset/MapData.h"
#include "../Ruleset/RuleItem.h"

#include "../Savegame/Tile.h"


namespace OpenXcom
{

class BattleItem;
class BattleUnit;
class SavedBattleGame;
class Tile;

struct BattleAction;


/**
 * A utility class that handles lighting and calculations in 3D-space on the
 * battlefield - as well as opening and closing doors.
 * @note This function does not handle any graphics or sounds - except doggie
 * bark in calcFov().
 */
class TileEngine
{
	public:
		static const int
			SIGHTDIST_TSp		= 20,							// tile-space
			SIGHTDIST_TSp_Sqr	= SIGHTDIST_TSp * SIGHTDIST_TSp;

private:
	static const int
		SIGHTDIST_VSp		= SIGHTDIST_TSp * 16,				// voxel-space
		SIGHTDIST_VSp_Sqr	= SIGHTDIST_VSp * SIGHTDIST_VSp,

		MAX_SHADE_TO_SEE_UNITS = 8, // cf. MissionStatistics::NIGHT_SHADE

		scanOffsetZ[11u],

		HARD_BLOCK = 100000,	// this is a hardblock for HE; hence it has to be higher
								// than the highest HE power in the Rulesets, but it also
								// needs to be able to add without overflowing.
		LIGHT_FIRE	= 15,
		LIGHT_SUN	= 15,
		LIGHT_UNIT	= 12,

		EYE_OFFSET	= -4;

	static const size_t
		LIGHT_LAYER_AMBIENT	= 0u,
		LIGHT_LAYER_STATIC	= 1u,
		LIGHT_LAYER_DYNAMIC	= 2u,

		LOFT_LAYERS = 12u;

	bool
		_spotSound,
		_unitLighting;
	int
//		_missileDirection,
		_dirRay,
		_powerE, // effective power that actually explodes on a tile that's hit by HE etc.
		_powerT; // test power that checks if _powerE even makes it to the next tile.

	SavedBattleGame* _battleSave;
	Tile* _trueTile;

	BattleAction* _rfAction;
	std::map<int, Position> _rfShotPos;

	const std::vector<Uint16>* _voxelData;

	/// Adds a pseudo-circular light pattern to the battlefield.
	void addLight(
			const Position& pos,
			int power,
			size_t layer) const;

	/// Calculates blockage of various persuasions.
	int blockage(
			const Tile* const tile,
			const MapDataType partType,
			/*const*/ DamageType dType,
			const int dir = -1,
			const bool isStartTile = false,
			const bool dirTrue = false) const;

	/// Opens any doors this door is connected to.
	void openAdjacentDoors(
			const Position& pos,
			MapDataType partType) const;

	/// Calculates the maximum throwing range.
	static int getThrowDistance(
			int weight,
			int strength,
			int elevation);

	/// Gets a Tile within melee-range.
	Tile* getVerticalTile(
			const Position& posOrigin,
			const Position& posTarget) const;


	public:
		/// Creates a TileEngine.
		TileEngine(
				SavedBattleGame* const battleSave,
				const std::vector<Uint16>* const voxelData);
		/// Cleans up the TileEngine.
		~TileEngine();

		/// Calculates sun-shading of the entire battlefield.
		void calculateSunShading() const;
		/// Calculates sun-shading of a single Tile.
		void calculateSunShading(Tile* const tile) const;
		/// Calculates lighting of the battlefield for terrain.
		void calculateTerrainLighting() const;
		/// Calculates lighting of the battlefield for units.
		void calculateUnitLighting() const;

		/// Toggles xCom units' personal lighting on/off.
		void togglePersonalLighting();

		/// Calculates Field of View of a single BattleUnit.
		bool calcFov(
				BattleUnit* const unit,
				bool revealTiles = true) const;
		/// Calculates Field of View of all conscious units within range of a specified Position.
		void calcFovPos(
				const Position& pos,
				bool spotSound = false,
				bool revealTiles = false);
		/// Calculates Field of View of all conscious units on the battlefield.
		void calcFovAll(
				bool spotSound = false,
				bool revealTiles = false);

		/// Checks visibility of a BattleUnit to a Tile.
		bool visible(
				const BattleUnit* const unit,
				const Tile* const tile) const;
		/// Gets a valid target-unit given a Tile.
		const BattleUnit* getTargetUnit(const Tile* const tile) const;

		/// Gets the origin-voxel of a specified BattleUnit's sight.
		Position getSightOriginVoxel(
				const BattleUnit* const unit,
				const Position* pos = nullptr) const;
		/// Gets the origin-voxel of a specified battle-action.
		Position getOriginVoxel(
				const BattleAction& action,
				const Tile* const tile = nullptr) const;
		/// Gets the validity of targeting a BattleUnit by voxel-type.
		bool canTargetUnit(
				const Position* const originVoxel,
				const Tile* const tileTarget,
				Position* const scanVoxel,
				const BattleUnit* const excludeUnit,
				const BattleUnit* targetUnit = nullptr,
				bool* const force = nullptr) const;
		/// Gets the validity of targeting a tile-part by voxel-type.
		bool canTargetTilepart(
				const Position* const originVoxel,
				const Tile* const tileTarget,
				const MapDataType tilePart,
				Position* const scanVoxel,
				const BattleUnit* const excludeUnit) const;
		/// Checks a unit's % exposure on a tile.
//		int checkVoxelExposure(Position* originVoxel, Tile* tile, BattleUnit* excludeUnit, BattleUnit* excludeAllBut);

		/// Checks reaction-fire.
		bool checkReactionFire(
				BattleUnit* const triggerUnit,
				int tuSpent = 0,
				bool autoSpot = true);
		/// Creates a vector of units that can spot this unit.
		std::vector<BattleUnit*> getSpottingUnits(const BattleUnit* const unit);
		/// Given a vector of spotters, and a unit, picks the spotter with the highest reaction-score.
		BattleUnit* getReactor(
				std::vector<BattleUnit*> spotters,
				BattleUnit* const defender,
				const int tuSpent = 0,
				bool autoSpot = true) const;
		/// Fires a reaction-shot if possible.
		bool reactionShot(
				BattleUnit* const unit,
				const BattleUnit* const targetUnit);
		/// Selects a reaction-fire-method based on TU & range.
		void chooseFireMethod();
		/// Gets the unique reaction-fire BattleAction struct.
//		BattleAction* getRfAction();
		/// Gets the reaction-fire shot-list.
		std::map<int, Position>* getRfShooterPositions();

		/// Handles bullet/weapon hits.
		void hit(
				const Position& targetVoxel,
				int power,
				DamageType dType,
				BattleUnit* const attacker,
				bool melee = false,
				bool shotgun = false,
				const std::string& infection = "");
		/// Handles explosions.
		void explode(
				const Position& targetVoxel,
				int power,
				DamageType dType,
				int radius,
				BattleUnit* const attacker = nullptr,
				bool grenade = false,
				bool defusePulse = false);
		/// Checks the horizontal blockage of a Tile.
		int horizontalBlockage(
				const Tile* const tileStart,
				const Tile* const tileStop,
				const DamageType dType) const;
		/// Checks the vertical blockage of a Tile.
		int verticalBlockage(
				const Tile* const tileStart,
				const Tile* const tileStop,
				const DamageType dType) const;
		/// Sets the final direction from which a missile or thrown-object came.
		void setProjectileDirection(const int dir);
		/// Blows a Tile up.
		void detonate(Tile* const tile) const;
		/// Checks if a destroyed Tile starts an explosion.
		Tile* checkForTerrainExplosions() const;

		/// Tries to open a door.
		DoorResult unitOpensDoor(
				BattleUnit* const unit,
				const bool rtClick = true,
				int dir = -1);
		/// Checks for a door connected to a wall at the specified position.
/*		bool TileEngine::testAdjacentDoor(
				Position pos,
				int part,
				int dir); */
		/// Closes ufo doors.
		bool closeUfoDoors() const;

		/// Calculates a line trajectory.
		VoxelType plotLine(
				const Position& origin,
				const Position& target,
				const bool storeTrj,
				std::vector<Position>* const trj,
				const BattleUnit* const excludeUnit,
				const bool doVoxelCheck = true,
				const bool onlyVisible = false,
				const BattleUnit* const excludeAllBut = nullptr) const;
		/// Calculates a parabola trajectory.
		VoxelType plotParabola(
				const Position& originVoxel,
				const Position& targetVoxel,
				bool storeTrj,
				std::vector<Position>* const trj,
				const BattleUnit* const excludeUnit,
				const double arc,
				const bool allowCeil = false,
				const Position& deltaVoxel = Position(0,0,0)) const;

		/// Validates a throwing action.
		bool validateThrow(
				const BattleAction& action,
				const Position& originVoxel,
				const Position& targetVoxel,
				double* const arc = nullptr,
				VoxelType* const impactType = nullptr) const;
		/// Validates the throwing range.
		static bool validThrowRange(
				const BattleAction* const action,
				const Position& originVoxel,
				const Tile* const tile);

		/// Validates the melee range between two BattleUnits.
		bool validMeleeRange(
				const BattleUnit* const actor,
				const int dir = -1,
				const BattleUnit* const targetUnit = nullptr) const;
		/// Validates the melee range between a Position and a BattleUnit.
		bool validMeleeRange(
				const Position& pos,
				const int dir,
				const BattleUnit* const actor,
				const BattleUnit* const targetUnit = nullptr) const;

		/// Gets an adjacent Position that can be attacked with melee.
		Position getMeleePosition(const BattleUnit* const actor) const;

		/// Gets an adjacent Tile with an unconscious unit if any.
		Tile* getExecutionTile(const BattleUnit* const actor) const;

		/// Performs a psionic action.
		bool psiAttack(BattleAction* const action);

		/// Applies gravity to anything that occupies a Tile.
		Tile* applyGravity(Tile* const tile) const;

		/// Gets the AI to look through a window.
		int faceWindow(const Position& pos) const;

		/// Marks a region of the map as "dangerous to aliens" for a turn.
		void setDangerZone(
				const Position& pos,
				const int radius,
				const BattleUnit* const unit) const;

		/// Calculates the z-voxel for shadows.
		int castShadow(const Position& voxel) const;

		/// Checks the visibility of a given voxel.
//		bool isVoxelVisible(const Position& voxel) const;
		/// Checks what type of voxel occupies a specified voxel.
		VoxelType detVoxelType(
				const Position& targetVoxel,
				const BattleUnit* const excludeUnit = nullptr,
				const bool excludeAllUnits = false,
				const bool onlyVisible = false,
				const BattleUnit* const excludeAllBut = nullptr) const;

		/// Checks the distance between two positions.
		static int distance(
				const Position& pos1,
				const Position& pos2,
				const bool considerZ = true);
		/// Checks the distance squared between two positions.
		static int distSqr(
				const Position& pos1,
				const Position& pos2,
				const bool considerZ = true);
		/// Checks the distance between two positions to double-accuracy.
//		static double distancePrecise(
//				const Position& pos1,
//				const Position& pos2) const;

		/// Gets direction to a target-point.
		static int getDirectionTo(
				const Position& posOrigin,
				const Position& posTarget);

		/// Sets a Tile with a diagonal bigwall as the true epicenter of an explosion.
		void setTrueTile(Tile* const tile = nullptr);
};

}

#endif
