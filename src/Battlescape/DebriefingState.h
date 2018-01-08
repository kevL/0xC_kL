/*
 * Copyright 2010-2018 OpenXcom Developers.
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
//#include "../Savegame/Soldier.h"
#include "../Savegame/SoldierDead.h"
#include "../Savegame/TacticalStatistics.h"


namespace OpenXcom
{

/// Rating strings are used by MonthlyReportState also.
static const char* const TAC_RATING[6u]
{
	"STR_RATING_TERRIBLE",	// 0
	"STR_RATING_POOR",		// 1
	"STR_RATING_OK",		// 2
	"STR_RATING_GOOD",		// 3
	"STR_RATING_EXCELLENT",	// 4
	"STR_RATING_TERRIFIC"	// 5
};

/**
 * Container for tracking results incl/ loot after tactical.
 */
struct DebriefStat
{
	bool recover;
	int
		qty,
		score;
	std::string type;

	/**
	 * cTor.
	 * @param typeId	- reference to a type of result
	 * @param reco		- true to recover the result and send it to base-stores
	 */
	DebriefStat(
			const std::string& typeId,
			bool reco = false)
		:
			type(typeId),
			score(0),
			qty(0),
			recover(reco)
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
class SavedBattleGame;
class Soldier;
class Text;
class TextButton;
class TextList;
class Window;


/**
 * Debriefing screen shown after a Battlescape mission that displays results.
 */
class DebriefingState
	:
		public State
{

private:
	static const char* const TAC_RESULT[12u];

	enum AlienCaptureResult
	{
		ACR_NONE,	// 0
		ACR_DIES,	// 1
		ACR_SHOW	// 2
	} _capture;

	bool
		_aborted,
		_isHostileStanding,
		_isQuickBattle;
	int
		_aliensStunned,
		_destroyPlayerBase,
		_diff,
		_missionCost,
		_playerDead,
		_playerLive;

	std::string _music;

	Base* _base;
	Craft* _craft;
	Country* _country;
	Region* _region;
	Ruleset* _rules;
	SavedBattleGame* _battleSave;
	SavedGame* _playSave;

	Text
		* _txtBaseLabel,
		* _txtCost,
		* _txtCasualties,
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

	TacticalStatistics* _tactical;

	std::map<TileType, SpecialType*> _specialTypes;
	std::map<const RuleItem*, int>
		_clips,
		_clipsProperty,
		_surplusItems,
		_lostProperty;


	std::map<std::wstring, std::vector<int>> _solStatIncr;

	std::vector<ReequipStat> _cannotReequip;

	std::vector<BattleUnit*>* _unitList;
	std::vector<DebriefStat*> _statList;
	std::vector<Soldier*> _soldiersFeted;
	std::vector<SoldierDead*> _soldiersLost;

	/// Adds a DebriefStat.
	void addResultStat(
			const std::string& type,
			int score,
			int qty = 1);
	/// One of the longest and most complicated functions in the entire codebase.
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

		/// Initializes the State.
		void init() override;

		/// Handler for clicking the Ok button.
		void btnOkClick(Action* action);
};

}

#endif
