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

#ifndef OPENXCOM_DEBRIEFINGSTATE_H
#define OPENXCOM_DEBRIEFINGSTATE_H

//#include <map>
//#include <string>
//#include <vector>

#include "../Engine/State.h"

#include "../Ruleset/RuleItem.h"

//#include "../Savegame/BattleItem.h"
#include "../Savegame/MissionStatistics.h"
//#include "../Savegame/Soldier.h"
#include "../Savegame/SoldierDead.h"


namespace OpenXcom
{

/**
 * Container for tracking loot after tactical.
 */
struct DebriefingStat
{
	bool recover;
	int
		qty,
		score;
	std::string type;

	/**
	 * cTor.
	 * @param typeId	- reference to a type of stuff
	 * @param recovery	- true to recover the stuff and send it to base-stores
	 */
	DebriefingStat(
			const std::string& typeId,
			bool recovery = false)
		:
			type(typeId),
			score(0),
			qty(0),
			recover(recovery)
	{};
};

/**
 * Container for tracking tile-part types after tactical.
 */
struct SpecialType
{
	std::string type;
	int value;
};

/**
 * Container for tracking non-replaced items on Craft after tactical.
 */
struct ReequipStat
{
	std::string type;
	int qtyLost;
	std::wstring craft;
};


class Base;
class BattleItem;
class BattleUnit;
class Country;
class Craft;
class Region;
class RuleItem;
class Ruleset;
class SavedGame;
class Soldier;
class Text;
class TextButton;
class TextList;
class Window;


/**
 * Debriefing screen shown after a Battlescape mission that displays the results.
 */
class DebriefingState
	:
		public State
{

private:
	bool
		_alienDies,
		_destroyPlayerBase,
		_manageContainment,
		_isQuickBattle;
	int
		_aliensControlled,
		_aliensKilled,
		_aliensStunned,
		_diff,
		_missionCost;

	std::string _music;

	Base* _base;
	Craft* _craft;
	Country* _country;
	Region* _region;
	Ruleset* _rules;
	SavedGame* _gameSave;
	Text
		* _txtBaseLabel,
		* _txtCost,
		* _txtItem,
		* _txtQuantity,
		* _txtRating,
		* _txtRecovery,
		* _txtScore,
		* _txtTitle;
	TextButton* _btnOk;
	TextList
		* _lstRecovery,
		* _lstStats,
		* _lstTotal;
	Window* _window;

	MissionStatistics* _tactical;

	std::map<SpecialTileType, SpecialType*> _specialTypes;
	std::map<const RuleItem*, int>
		_clips,
		_clipsProperty,
		_itemsGained,
		_itemsLostProperty;


	std::map<std::wstring, std::vector<int>> _soldierStatInc;

	std::vector<ReequipStat> _missingItems;

	std::vector<DebriefingStat*> _statList;
	std::vector<Soldier*> _soldiersFeted;
	std::vector<SoldierDead*> _soldiersLost;

	/// Adds a DebriefingStat.
	void addStat(
			const std::string& type,
			int score,
			int qty = 1);
	/// Prepares the debriefing.
	void prepareDebriefing();
	/// Recovers items from tactical.
	void recoverItems(std::vector<BattleItem*>* const battleItems);
	/// Recovers an aLien from the battlefield.
	void recoverLiveAlien(const BattleUnit* const unit);
	/// Reequips a Craft after tactical.
	void reequipCraft(Craft* const craft);


	public:
		/// Creates a Debriefing state.
		DebriefingState();
		/// Cleans up the Debriefing state.
		~DebriefingState();

		/// Initializes the state.
		void init() override;

		/// Handler for clicking the OK button.
		void btnOkClick(Action* action);
};

}

#endif
