/*
 * Copyright 2010-2019 OpenXcom Developers.
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

#ifndef OPENXCOM_SAVEDGAME_H
#define OPENXCOM_SAVEDGAME_H

//#include <map>
//#include <string>
//#include <vector>
//#include <time.h>
//#include <stdint.h>

#include "ResearchGeneral.h"
#include "Soldier.h"

#include "../Ruleset/RuleAlienMission.h"


namespace OpenXcom
{

class AlienBase;
class AlienMission;
class AlienStrategy;
class Base;
class Country;
class Craft;
class GameTime;
class Language;
class TerrorSite;
class Region;
class ResearchGeneral;
class ResearchProject;
class RuleManufacture;
class RuleResearch;
class Ruleset;
class SavedBattleGame;
class Soldier;
class SoldierDead;
class Target;
class TextList;
class Ufo;
class Waypoint;

struct TacticalStatistics;


/**
 * Enumerator containing all the possible difficulty-levels.
 */
enum DifficultyLevel
{
	DIFF_BEGINNER,		// 0
	DIFF_EXPERIENCED,	// 1
	DIFF_VETERAN,		// 2
	DIFF_GENIUS,		// 3
	DIFF_SUPERHUMAN		// 4
};

/**
 * Enumerator for the various save-types.
 */
enum SaveType
{
	SAVE_DEFAULT,			// 0
	SAVE_QUICK,				// 1
	SAVE_AUTO_GEOSCAPE,		// 2
	SAVE_AUTO_BATTLESCAPE,	// 3
	SAVE_IRONMAN,			// 4
	SAVE_IRONMAN_QUIT		// 5
};

/**
 * Enumerator for the End.
 */
enum EndType
{
	END_NONE,	// 0
	END_WIN,	// 1
	END_LOSE	// 2
};

/**
 * Enumerator for the radar-detail shown on the Globe.
 */
enum GlobeRadarDetail
{
	GRD_NONE,	// 0
	GRD_CRAFT,	// 1
	GRD_BASE,	// 2
	GRD_ALL		// 3
};

/**
 * Enumerator for the save-mode.
 */
enum SaveMode
{
	SM_GEOSCAPE,	// 0
	SM_BATTLESCAPE	// 1
};


/**
 * Container for SaveGame info displayed on listings.
 */
struct SaveInfo
{
	int mode;
	bool reserved;
	time_t timestamp;
	std::string file;
	std::wstring
		label,
		details,
		isoDate,
		isoTime;
	std::vector<std::string> rulesets;
};

/**
 * Container for soldier-promotion-info.
 */
struct PromotionInfo
{
	bool hasCO;
	int
		totalColonels,
		totalCaptains,
		totalSergeants;

	/// Builds this struct.
	PromotionInfo()
		:
			hasCO          (false),
			totalColonels  (0),
			totalCaptains  (0),
			totalSergeants (0)
	{}
};


/**
 * The game-data that gets written to disk when the game is saved.
 * @note A saved game holds all the variable info in the current game like
 * funds, game-time, current bases and contents, world activities, score, etc.
 */
class SavedGame
{

private:
	bool
		_debugArgDone,
		_debugCountryLines,
		_debugGeo,
		_detailGlobe,
		_ironman,
		_warnedFunds;
	int
		_monthsElapsed,
		_statTallyScore,
		_statTallyScoreResearch;
	size_t
		_dfZoom,
		_globeZ;
//		_selectedBase;
	double
		_dfLat,
		_dfLon,
		_globeLat,
		_globeLon;

	int64_t
		_statTallyFundsEarned,
		_statTallyFundsSpent;

	DifficultyLevel _difficulty;
	EndType _end;
	GlobeRadarDetail _detailRadar;

	AlienStrategy* _alienStrategy;
	GameTime* _time;
	const Ruleset* _rules;
	SavedBattleGame* _battleSave;

	std::wstring _label;
	std::string
		_debugArg,
		_graphRegionToggles,
		_graphCountryToggles,
		_graphFinanceToggles;
//		_lastselectedArmor;

	std::map<std::string, int> _ids;

	std::vector<int> _researchScores;
	std::vector<int64_t>
		_expenditure,
		_funds,
		_income,
		_maintenance;

	std::vector<AlienBase*>          _alienBases;
	std::vector<AlienMission*>       _activeMissions;
	std::vector<Base*>               _bases;
	std::vector<Country*>            _countries;
	std::vector<TerrorSite*>         _terrorSites;
	std::vector<TacticalStatistics*> _tacticalStats;
	std::vector<Region*>             _regions;
	std::vector<ResearchGeneral*>    _research;
	std::vector<SoldierDead*>        _deadSoldiers;
	std::vector<Ufo*>                _ufos;
	std::vector<Waypoint*>           _waypoints;


	/// Gets the info of a specified save-file.
	static SaveInfo getSaveInfo(
			const std::string& file,
			const Language* const lang);

	/// Gets the ResearchGeneral corresponding to a specified research-rule.
//	ResearchGeneral* getResearchGeneral(const RuleResearch* const resRule) const;
	/// Checks if a RuleResearch has had all of its required-research discovered.
	bool checkRequiredResearch(const RuleResearch* const resRule) const;

	/// Adds a specified Soldier's rank to the PromotionInfo struct.
	void tallySoldier(
			const Soldier* const soldier,
			PromotionInfo& data);
	/// Searches Soldiers of a specified rank and returns the one with the highest overall score.
	Soldier* inspectSoldiers(
			const std::vector<Soldier*>& soldiers,
			const std::vector<Soldier*>& participants,
			SoldierRank soldierRank);
	/// Evaluates the score of a Soldier based on all of his stats, missions and kills.
	int getSoldierScore(Soldier* const soldier);


	public:
		static const std::string
			SAVE_AUTO_Geo,
			SAVE_AUTO_Tac,
			SAVE_Quick,
			SAVE_Ext_AQ,
			SAVE_Ext,
			SAVE_ExtDot,
			SAVE_BakDot,
			SAVE_TacMission,
			SAVE_TacTurnBeg,
			SAVE_TacTurnEnd;
		static const std::wstring
			SAVELABEL_Ironballs,
			SAVELABEL_TacMission,
			SAVELABEL_TacTurnBeg,
			SAVELABEL_TacTurnEnd;

		/// Creates a SavedGame.
		explicit SavedGame(const Ruleset* const rules);
		/// Cleans up the SavedGame.
		~SavedGame();

		/// Gets list of saves in the user directory.
		static std::vector<SaveInfo> getList(
				const Language* const lang,
				bool autoquick);

		/// Loads the SavedGame from YAML.
		void load(
				const std::string& file,
				Ruleset* const rules);
		/// Saves the SavedGame to YAML.
		void save(const std::string& file) const;

		/// Gets the SavedGame's label.
		std::wstring getLabel() const;
		/// Sets the SavedGame's label.
		void setLabel(const std::wstring& label);

		/// Gets this SavedGame's Ruleset.
		const Ruleset* getRules() const;

		/// Sets the SavedGame's difficulty.
		void setDifficulty(const DifficultyLevel diff);
		/// Gets the SavedGame's difficulty.
		DifficultyLevel getDifficulty() const;
		/// Gets the SavedGame's difficulty as an integer.
		int getDifficultyInt() const;

		/// Gets the SavedGame's ending-type.
		EndType getEnding() const;
		/// Sets the SavedGame's ending-type.
		void setEnding(const EndType end);

		/// Gets if the SavedGame is in ironman-mode.
		bool isIronman() const;
		/// Sets if the SavedGame is in ironman-mode.
		void setIronman(bool ironman);

		/// Gets the current Globe longitude.
		double getGlobeLongitude() const;
		/// Sets Globe longitude.
		void setGlobeLongitude(double lon);
		/// Gets the current Globe latitude.
		double getGlobeLatitude() const;
		/// Sets Globe latitude.
		void setGlobeLatitude(double lat);
		/// Gets the current Globe zoom.
		size_t getGlobeZoom() const;
		/// Sets Globe zoom.
		void setGlobeZoom(size_t gZ);

		/// Sets the pre-dogfight coordinates and zoom-level.
		void setPreDogfightCoords(size_t zoom);
		/// Gets the preDogfight Globe longitude.
		double getDfLongitude() const;
		/// Gets the preDogfight Globe latitude.
		double getDfLatitude() const;
		/// Gets the preDogfight Globe zoom.
		size_t getDfZoom() const;

		/// Handles player's monthly funding.
		void balanceBudget();
		/// Gets the total country-funding.
		int getTotalCountryFunds() const;

		/// Sets new funds.
		void setFunds(int64_t funds);
		/// Gets the current funds.
		int64_t getFunds() const;
		/// Gets the list of funds from previous months.
		std::vector<int64_t>& getFundsList();

		/// Gets the list of monthly maintenance-values.
		std::vector<int64_t>& getMaintenanceList();
		/// Gets the list of monthly income-values.
		std::vector<int64_t>& getIncomeList();
		/// Gets the list of monthly expenditure-values.
		std::vector<int64_t>& getExpenditureList();

		/// Adds monthly score to a cumulative tally.
		void tallyScore(int score);
		/// Adds monthly research score to a cumulative tally.
		void tallyResearch(int score);
		/// Gets the total tallied score.
		int getTallyScore();
		/// Gets the total tallied research score.
		int getTallyResearch();
		/// Gets the total tallied funds earned.
		int64_t getTallyEarned();
		/// Gets the total tallied funds spent.
		int64_t getTallySpent();

		/// Gets the current time/date.
		GameTime* getTime() const;
		/// Sets the current time/date.
		void setTime(const GameTime& gt);

		/// Gets an ID to assign to a Target type.
		int getCanonicalId(const std::string& typeTarget);
	 	/// Gets a list of all canonical-IDs.
		const std::map<std::string, int>& getTargetIds() const;
		/// Resets the list of object-IDs.
//		void setCanonicalIds(const std::map<std::string, int>& ids);

		/// Gets the list of Countries.
		std::vector<Country*>* getCountries();
		/// Gets the list of Regions.
		std::vector<Region*>* getRegions();
		/// Gets the list of Bases.
		std::vector<Base*>* getBases();
		/// Gets the list of Bases const.
		const std::vector<Base*>* getBases() const;

		/// Gets the total base-maintenance.
		int getBaseMaintenances() const;

		/// Gets the list of current UFOs.
		std::vector<Ufo*>* getUfos();
		/// Gets the list of current waypoints.
		std::vector<Waypoint*>* getWaypoints();
		/// Gets the list of current mission-sites.
		std::vector<TerrorSite*>* getTerrorSites();

		/// Sets the current SavedBattleGame.
		void setBattleSave(SavedBattleGame* const battleSave = nullptr);
		/// Gets the current SavedBattleGame.
		SavedBattleGame* getBattleSave();

		/// Gets the ResearchGenerals.
		std::vector<ResearchGeneral*>& getResearchGenerals();

		/// Searches the ResearchGenerals for a specified research-type & status.
		bool searchResearch(
				const std::string& resType,
				const ResearchStatus status = RG_DISCOVERED) const;
		/// Searches the ResearchGenerals for a specified research-rule & status.
		bool searchResearch(
				const RuleResearch* const resRule,
				const ResearchStatus status = RG_DISCOVERED) const;
		/// Sets the status of a ResearchGeneral by research-type.
		bool setResearchStatus(
				const std::string& resType,
				const ResearchStatus status = RG_DISCOVERED) const;
		/// Sets the status of a ResearchGeneral by research-rule.
		bool setResearchStatus(
				const RuleResearch* const resRule,
				const ResearchStatus status = RG_DISCOVERED) const;

		/// Adds a RuleResearch to the list of discovered-research.
		void discoverResearch(const RuleResearch* const resRule);

		/// Tabulates a list of ResearchProjects that appears when a ResearchProject is completed.
		void tabulatePopupResearch(
				std::vector<const RuleResearch*>& projects,
				const RuleResearch* const resRule,
				bool crackRequested = true);
		/// Tabulates a list of Manufacture projects that appears when research is discovered.
		void tabulatePopupManufacture(
				std::vector<const RuleManufacture*>& projects,
				const RuleResearch* const resRule) const;

		/// Tabulates a list of ResearchProjects that can be started at a Base.
		void tabulateStartableResearch(
				std::vector<const RuleResearch*>& projects,
				Base* const base) const;
		/// Tabulates a list of Manufacture that can be started at a Base.
		void tabulateStartableManufacture(
				std::vector<const RuleManufacture*>& projects,
				Base* const base) const;

		/// Checks if a research-type is discovered.
		bool isResearched(const std::string& resType) const;
		/// Checks if a list of research-types have all been discovered.
		bool isResearched(const std::vector<std::string>& resTypes) const;

		/// Gets the Soldier matching an ID.
		Soldier* getSoldier(int id) const;

		/// Handles promotions above Squaddie.
		bool handlePromotions(std::vector<Soldier*>& participants);

		///  Gets the list of AlienBases.
		std::vector<AlienBase*>* getAlienBases();

		/// Toggles and returns the Geoscape debug-flag.
		bool toggleDebugActive();
		/// Gets the current state of the Geoscape debug-flag.
		bool getDebugGeo() const;

		/// Gets the list of research-scores.
		std::vector<int>& getResearchScores();

		/// Flags player as warned of low funds.
		void flagLowFunds(bool warned = true);
		/// Checks if player has been warned of low funds.
		bool hasLowFunds() const;

		/// Full access to the AlienStrategy data.
		AlienStrategy& getAlienStrategy()
		{ return *_alienStrategy; }
		/// Read-only access to the AlienStrategy data.
		const AlienStrategy& getAlienStrategy() const
		{ return *_alienStrategy; }
		/// Full access to the current AlienMissions.
		std::vector<AlienMission*>& getAlienMissions()
		{ return _activeMissions; }
		/// Read-only access to the current AlienMissions.
		const std::vector<AlienMission*>& getAlienMissions() const
		{ return _activeMissions; }
		/// Finds an AlienMission by Region and objective-type.
		AlienMission* findAlienMission(
				const std::string& region,
				MissionObjective objective) const;

		/// Locates a Region containing the specified coordinates.
		Region* locateRegion(
				double lon,
				double lat) const;
		/// Locates a Region containing a Target.
		Region* locateRegion(
				const Target& target) const;

		/// Gets the month-count.
		int getMonthsElapsed() const;
		/// Increments the month-count.
		void elapseMonth();

		/// Gets the GraphRegionToggles.
		const std::string& getGraphRegionToggles() const;
		/// Gets the GraphCountryToggles.
		const std::string& getGraphCountryToggles() const;
		/// Gets the GraphFinanceToggles.
		const std::string& getGraphFinanceToggles() const;
		/// Sets the GraphRegionToggles.
		void setGraphRegionToggles(const std::string& toggles);
		/// Sets the GraphCountryToggles.
		void setGraphCountryToggles(const std::string& toggles);
		/// Sets the GraphFinanceToggles.
		void setGraphFinanceToggles(const std::string& toggles);

		/// Toggles globe-detail.
		bool toggleGlobeDetail();
		/// Checks globe-detail.
		bool isGlobeDetail() const;
		/// Sets the level of radar-detail.
		void setRadarDetail(GlobeRadarDetail detail);
		/// Gets the level of radar-detail.
		GlobeRadarDetail getRadarDetail() const;

		/// Gets the list of dead-soldiers.
		std::vector<SoldierDead*>* getDeadSoldiers();

		/// Gets the last selected player-base.
//		Base* getRecallBase();
		/// Set the last selected player-base.
//		void setRecallBase(size_t base);

		/// Sets the last selected armor
//		void setRecallArmor(const std::string& value);
		/// Gets the last selected armor
//		std::string getRecallArmor();

		/// Sets a debug-arg for GeoscapeState.
		void setDebugArg(const std::string& debug);
		/// Gets a debug-arg from Globe.
		std::string getDebugArg() const;
		/// Gets if a debug-arg has been set.
		bool getDebugArgDone();
		/// Toggles showing the country-lines in debugGeo.
		void toggleCountryLines();
		/// Checks if the country-lines should be drawn in debugGeo.
		bool debugCountryLines();

		/// Gets the list of TacticalStatistics.
		std::vector<TacticalStatistics*>& getTacticalStatistics();

		/// Scores points for xCom or aLiens at specified coordinates.
		void scorePoints(
				double lon,
				double lat,
				int pts,
				bool aLien) const;
		/// Scores points for xCom or aLiens in Region/Country.
		void scorePoints(
				Region* const region,
				Country* const country,
				int pts,
				bool aLien) const;

		/// Formats hours into days+hours for Craft refurbishing.
		std::wstring formatCraftDowntime(
				int hrsTotal,
				bool isDelayed,
				const Language* const lang) const;

		/// Gets the Craft corresponding to a specified identificator.
//		Craft* getCraftByIdentificator(const CraftId& craftId) const;
};

}

#endif
