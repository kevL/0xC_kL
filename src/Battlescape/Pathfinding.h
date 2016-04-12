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

#ifndef OPENXCOM_PATHFINDING_H
#define OPENXCOM_PATHFINDING_H

//#include <vector>

#include "PathfindingNode.h"
#include "Position.h"

#include "../Ruleset/MapData.h"


namespace OpenXcom
{

enum UpDownCheck
{
	FLY_CANT,		// 0
	FLY_BLOCKED,	// 1
	FLY_GRAVLIFT,	// 2
	FLY_GOOD		// 3
};

class BattleUnit;
class SavedBattleGame;
class Tile;


/**
 * A utility class that calculates the shortest path between two points on the
 * battlefield.
 */
class Pathfinding
{

private:
	static const int FAIL = 255;

	bool
		_alt,
		_ctrl,
		_previewed,
		_strafe,
		_zPath;
	int
		_openDoor, // to get an accurate preview when dashing through doors.
		_tuCostTotal;

	BattleUnit* _unit;
	SavedBattleGame* _battleSave;

	BattleAction* _pathAction;

	MoveType _mType;

	std::vector<int> _path;

	std::vector<PathfindingNode> _nodes;

	/// Sets the movement type for the path.
	void setMoveType();

	/// Tries to find a straight line path between two positions.
	bool bresenhamPath(
			const Position& origin,
			const Position& target,
			const BattleUnit* const launchTarget);
//			bool sneak);
	/// Tries to find a path between two positions.
	bool aStarPath(
			const Position& posOrigin,
			const Position& posTarget,
			const BattleUnit* const launchTarget,
			int maxTuCost);
//			bool sneak);

	/// Gets the (Pathfinding) node at a (tile) Position.
	PathfindingNode* getNode(const Position& pos);

	/// Determines whether a tile blocks a movementType.
	bool isBlocked(
			const Tile* const tile,
			const MapDataType partType,
			const BattleUnit* const launchTarget = nullptr,
			const BigwallType diagExclusion = BIGWALL_NONE) const;

	/// Determines whether a unit can fall down from this tile.
	bool canFallDown(
			const Tile* const tile,
			int armorSize) const;
	/// Determines whether a unit can fall down from this tile.
	bool canFallDown(const Tile* const tile) const;


	public:
		static const int
			TU_INFINITE		= std::numeric_limits<int>::max(),
			TU_FIRE_AVOID	= 32,
			TU_KNEEL		=  3,
			TU_STAND		= 10,
			EN_STAND		=  5,

			DIR_UP			=  8,
			DIR_DOWN		=  9,
			CLIP_HEIGHT		= 26;

		static Uint8
			red,
			green,
			yellow;


		/// cTor
		explicit Pathfinding(SavedBattleGame* const battleSave);
		/// Cleans up the Pathfinding.
		~Pathfinding();

		/// Sets unit in order to exploit low-level pathing functions.
		void setPathingUnit(BattleUnit* const unit);
		/// Sets keyboard input modifiers.
		void setInputModifiers();

		/// Aborts the current path.
		void abortPath();

		/// Calculates the shortest path.
		void calculatePath(
				const BattleUnit* const unit,
				Position posStop,
				int maxTuCost = TU_INFINITE,
				const BattleUnit* const launchTarget = nullptr,
				bool strafeRejected = false);

		/// Gets all reachable tile-indices based on TU.
		std::vector<size_t> findReachable(
				const BattleUnit* const unit,
				int maxTuCost);

		/// Gets the TU cost to move from 1 tile to the other.
		int getTuCostPf(
				const Position& posStart,
				int dir,
				Position* const posStop,
				const BattleUnit* const launchTarget = nullptr,
				bool bresenh = true);
		/// Gets _tuCostTotal; finds out whether we can hike somewhere in this turn or not.
		int getTuCostTotalPf() const
		{ return _tuCostTotal; }

		/// Determines whether or not movement between startTile and endTile is possible in the direction.
		bool isBlockedPath(
				const Tile* const startTile,
				const int dir,
				const BattleUnit* const launchTarget = nullptr) const;

		/// Checks if the movement is valid, for the up/down button.
		UpDownCheck validateUpDown(
				const Position& posStart,
				const int dir);
//				const bool launch = false);

		/// Previews the path.
		bool previewPath(bool discard = false);
		/// Clears the path-preview.
		bool clearPreview();
		/// Gets the path-preview setting.
		bool isPathPreviewed() const;

		/// Gets the CTRL-modifier setting.
		bool isModCtrl() const;
		/// Gets the ALT-modifier setting.
		bool isModAlt() const;
		/// Gets the zPath-modifier setting.
		bool isZPath() const;

		/// Gets the current MoveType.
		MoveType getMoveTypePf() const;

		/// Gets TU-cost for opening a door.
		int getOpenDoor() const;

		/// Checks whether a path is ready and returns the direction.
		int getStartDirection();
		/// Dequeues a path and returns the direction.
		int dequeuePath();
		/// Gets the path.
		const std::vector<int>& getPath() const;
		/// Gets a copy of the path.
		std::vector<int> copyPath() const;

		/// Converts direction to a unit-vector.
		static void directionToVector(
				const int dir,
				Position* const posVect);
		/// Converts a unit-vector to a direction.
		static void vectorToDirection(
				const Position& posVect,
				int& dir);
};

}

#endif
