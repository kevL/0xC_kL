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

#ifndef OPENXCOM_ALIENDEPLOYMENT_H
#define OPENXCOM_ALIENDEPLOYMENT_H

//#include <string>
//#include <vector>
//#include <yaml-cpp/yaml.h>

#include "RuleItem.h"

#include "../Resource/XcomResourcePack.h"


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
 * - deployment type can be a craft's name but also alien base or cydonia
 * - alienRank is used to check which nodeRanks can be used to deploy this unit
 *   + to match to a specific unit (=race/rank combination) that should be deployed
 * @sa Node
 */
class AlienDeployment
{

private:
	bool
		_finalDestination,
		_finalMission,
		_noRetreat;
	int
		_cheatTurn,
		_civilians,
		_despawnPenalty,
		_durationMax,
		_durationMin,
		_height,
		_length,
		_markerIcon,
		_objectiveCompleteScore,
		_objectiveFailedScore,
		_objectivesReqd,
		_pointsPer30,
		_shade,
		_turnLimit,
		_width;

	std::string
		_alert,
		_alertBg,
		_markerType,
		_nextStage,
		_objectiveCompleteText,
		_objectiveFailedText,
		_objectivePopup,
		_race,
		_script,
		_type;

	std::vector<std::string>
		_musics,
		_terrains;
	std::vector<DeploymentData> _data;

	BriefingData _briefingData;
	ChronoResult _chronoResult;
	SpecialTileType _objectiveType;

	public:
		/// Creates an AlienDeployment ruleset.
		explicit AlienDeployment(const std::string& type);
		/// Cleans up the AlienDeployment ruleset.
		~AlienDeployment();

		/// Loads AlienDeployment data from YAML.
		void load(const YAML::Node& node);

		/// Gets the AlienDeployment's type.
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

		/// Gets the script-type to use for the AlienDeployment.
		const std::string& getScriptType() const;

		/// Checks if the AlienDeployment is where to send Craft via the ConfirmCydonia btn.
		bool isFinalDestination() const;
		/// Checks if aborting or losing the AlienDeployment will lose the game.
		bool isNoRetreat() const;
		/// Checks if the AlienDeployment finishes the game.
		bool isFinalMission() const;

		/// Gets the alert-message for the AlienDeployment.
		const std::string& getAlertMessage() const;
		/// Gets the alert-background for the AlienDeployment.
		const std::string& getAlertBackground() const;

		/// Gets the briefing-data for the AlienDeployment.
		BriefingData getBriefingData() const;

		/// Gets the marker-type for the AlienDeployment.
		const std::string& getMarkerType() const;
		/// Gets the marker-icon for the AlienDeployment.
		int getMarkerIcon() const;

		/// Gets the minimum duration for the AlienDeployment.
		int getDurationMin() const;
		/// Gets the maximum duration for the AlienDeployment.
		int getDurationMax() const;

		/// Gets the list of music to pick from.
		const std::vector<std::string>& getDeploymentMusics() const;

		/// Gets the objective-type for the AlienDeployment.
		SpecialTileType getObjectiveType() const;
		/// Gets a fixed number of objectives required if any.
		int getObjectivesRequired() const;
		/// Gets the string to pop up when the mission-objectives are complete.
		const std::string& getObjectivePopup() const;
		/// Fills out the objective-complete info.
		bool getObjectiveCompleteInfo(
				std::string& text,
				int& score) const;
		/// Fills out the objective-failed info.
		bool getObjectiveFailedInfo(
				std::string& text,
				int& score) const;

		/// Gets the score penalty xCom receives for ignoring a site.
		int getDespawnPenalty() const;
		/// Gets the half-hourly score penalty xCom receives for a site existing.
		int getPointsPer30() const;

		/// Gets the turn-limit for the AlienDeployment.
		int getTurnLimit() const;
		/// Gets the result when the turn-timer runs out.
		ChronoResult getChronoResult() const;

		/// Gets which turn the aLiens start cheating on.
		int getCheatTurn() const;
};

}

#endif
