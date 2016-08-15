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

#ifndef OPENXCOM_RULEALIENDEPLOYMENT_H
#define OPENXCOM_RULEALIENDEPLOYMENT_H

//#include <string>
//#include <vector>
//#include <yaml-cpp/yaml.h>

#include "RuleItem.h"

#include "../Resource/XcomResourcePack.h"

#include "../Savegame/WeightedOptions.h"


namespace OpenXcom
{

/**
 *
 */
struct ItemSet
{
	std::vector<std::string> items;
};

/**
 *
 */
struct DeploymentData
{
	int
		alienRank,
		lowQty,
		highQty,
		dQty,
		extraQty,
		pctOutsideUfo;

	std::vector<ItemSet> itemSets;
};

/**
 *
 */
struct BriefingData
{
	bool
		showCraftText,
		showTargetText;
	int
		palette,
		textOffset;
	std::string
		background,
		desc,
		music,
		title;

	BriefingData()
		:
			palette(0),
			textOffset(0),
			music(OpenXcom::res_MUSIC_GEO_BRIEFING),
			background("BACK16.SCR"),
			showCraftText(true),
			showTargetText(true)
	{};
};

enum ChronoResult
{
	FORCE_WIN,	// 0
	FORCE_LOSE,	// 1
	FORCE_ABORT	// 2
};

static const int CHEAT_TURN_DEFAULT = 20;


class Ruleset;
class RuleTerrain;

/**
 * Represents a specific type of Alien Deployment.
 * Contains constant info about an Alien Deployment like the number of aliens
 * for each alien type and what items they carry (itemset depends on alien
 * technology advancement level).
 * - deployment type can be a craft's label but also alien base or cydonia
 * - alienRank is used to check which nodeRanks can be used to deploy this unit
 *   + to match to a specific unit (=race/rank combination) that should be deployed
 * @sa Node
 */
class RuleAlienDeployment
{

private:
	bool
		_finalDestination,
		_finalMission,
		_isAlienBase,
		_noRetreat;
	int
		_cheatTurn,
		_civilians,
		_despawnPenalty,
		_durationMax,
		_durationMin,
		_generatedMissionPct,
		_height,
		_length,
		_markerIcon,
		_objectiveSuccessScore,
		_objectiveFailedScore,
		_objectivesRequired,
		_shade,
		_turnLimit,
		_width;

	std::string
		_alert,
		_alertBg,
		_markerType,
		_nextStage,
		_objectiveSuccessText,
		_objectiveFailedText,
		_objectiveNotice,
		_race,
		_script,
		_type;

	std::vector<std::string>
		_musics,
		_terrains;
	std::vector<DeploymentData> _data;

	BriefingData _briefingData;
	ChronoResult _chronoResult;
	TileType _objectiveTile;
	WeightedOptions _generatedMission;

	public:
		/// Creates an RuleAlienDeployment ruleset.
		explicit RuleAlienDeployment(const std::string& type);
		/// Cleans up the RuleAlienDeployment ruleset.
		~RuleAlienDeployment();

		/// Loads RuleAlienDeployment data from YAML.
		void load(const YAML::Node& node);

		/// Gets the RuleAlienDeployment's type.
		const std::string& getType() const;
		/// Gets a pointer to the data.
		const std::vector<DeploymentData>* getDeploymentData() const;

		/// Gets dimensions.
		void getDimensions(
				int* width,
				int* lenght,
				int* heigth) const;

		/// Gets civilians.
		int getCivilians() const;

		/// Gets the terrains for Battlescape generation.
		const std::vector<std::string>& getDeployTerrains() const;

		/// Gets the shade-level for Battlescape generation.
		int getShade() const;

		/// Gets the next-stage of the mission.
		const std::string& getNextStage() const;

		/// Gets the aLien-race.
		const std::string& getRace() const;

		/// Gets the script-type to use for the RuleAlienDeployment.
		const std::string& getScriptType() const;

		/// Checks if the RuleAlienDeployment is where to send Craft via the ConfirmCydonia btn.
		bool isFinalDestination() const;
		/// Checks if aborting or losing the RuleAlienDeployment will lose the game.
		bool isNoRetreat() const;
		/// Checks if the RuleAlienDeployment finishes the game.
		bool isFinalMission() const;

		/// Gets the alert-message for the RuleAlienDeployment.
		const std::string& getAlertMessage() const;
		/// Gets the alert-background for the RuleAlienDeployment.
		const std::string& getAlertBackground() const;

		/// Gets the briefing-data for the RuleAlienDeployment.
		BriefingData getBriefingData() const;

		/// Gets the marker-type for the RuleAlienDeployment.
		const std::string& getMarkerType() const;
		/// Gets the marker-icon for the RuleAlienDeployment.
		int getMarkerIcon() const;

		/// Gets the minimum duration for the RuleAlienDeployment.
		int getDurationMin() const;
		/// Gets the maximum duration for the RuleAlienDeployment.
		int getDurationMax() const;

		/// Gets the list of music-tracks to pick from.
		const std::vector<std::string>& getDeploymentMusics() const;

		/// Gets the objective-tiletype for the RuleAlienDeployment.
		TileType getPlayerObjective() const;
		/// Gets a fixed quantity of objective-tiles required if any.
		int getObjectivesRequired() const;
		/// Gets the string to show when enough objective-tiles are destroyed.
		const std::string& getObjectiveNotice() const;
		/// Gets the objective-complete info.
		bool getObjectiveCompleteInfo(
				std::string& text,
				int& score) const;
		/// Gets the objective-failed info.
		bool getObjectiveFailedInfo(
				std::string& text,
				int& score) const;

		/// Gets the score-penalty xCom receives for ignoring a site.
		int getDespawnPenalty() const;

		/// Gets the turn-limit for the RuleAlienDeployment.
		int getTurnLimit() const;
		/// Gets the result to force when the turn-limit is reached.
		ChronoResult getChronoResult() const;

		/// Gets which turn the aLiens start cheating on.
		int getCheatTurn() const;

		/// Gets if the deployment is for an AlienBase (for quick-battles).
		bool isAlienBase() const;
		/// Gets the type of AlienMission that an AlienBase can generate.
		std::string getBaseGeneratedType() const;
		/// Gets the chance of an AlienBase generating an AlienMission.
		int getBaseGeneratedPct() const;
};

}

#endif
