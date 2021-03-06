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

#ifndef OPENXCOM_UNITFALLBSTATE_H
#define OPENXCOM_UNITFALLBSTATE_H

//#include <vector> // std::vector

#include "BattleState.h"


namespace OpenXcom
{

class BattlescapeGame;
class BattleUnit;
class SavedBattleGame;
class Tile;
class TileEngine;


/**
 * State for bonking units.
 */
class UnitBonkBState
	:
		public BattleState
{

private:
	std::list<BattleUnit*>* _unitsBonking;
	std::vector<BattleUnit*> _unitsBonked;
	std::vector<Tile*> _tilesToBonkInto;

	SavedBattleGame* _battleSave;
	TileEngine* _te;

	/// Checks if a BattleUnit can fall to the next lower level.
	bool canFall(const BattleUnit* const unit);


	public:
		/// Creates a UnitBonkBState.
		explicit UnitBonkBState(BattlescapeGame* const battle);
		/// Cleans up the UnitBonkBState.
		~UnitBonkBState();

		/// Gets the label of the BattleState.
		std::string getBattleStateLabel() const override;

		/// Initializes the BattleState.
		void init() override;
		/// Runs BattleState functionality every cycle. Returns when finished.
		void think() override;
};

}

#endif
