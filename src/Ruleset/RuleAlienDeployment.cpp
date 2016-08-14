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

#include "RuleAlienDeployment.h"

#include "../Savegame/Target.h"


namespace YAML
{

template<>
struct convert<OpenXcom::ItemSet>
{
	///
	static Node encode(const OpenXcom::ItemSet& rhs)
	{
		Node node;
		node = rhs.items;

		return node;
	}

	///
	static bool decode(const Node& node, OpenXcom::ItemSet& rhs)
	{
		if (node.IsSequence() == false)
			return false;

		rhs.items = node.as<std::vector<std::string>>(rhs.items);

		return true;
	}
};

template<>
struct convert<OpenXcom::DeploymentData>
{
	///
	static Node encode(const OpenXcom::DeploymentData& rhs)
	{
		Node node;

		node["alienRank"]		= rhs.alienRank;
		node["lowQty"]			= rhs.lowQty;
		node["highQty"]			= rhs.highQty;
		node["dQty"]			= rhs.dQty;
		node["extraQty"]		= rhs.extraQty;
		node["pctOutsideUfo"]	= rhs.pctOutsideUfo;
		node["itemSets"]		= rhs.itemSets;

		return node;
	}

	///
	static bool decode(
			const Node& node,
			OpenXcom::DeploymentData& rhs)
	{
		if (node.IsMap() == false)
			return false;

		rhs.alienRank		= node["alienRank"]		.as<int>(rhs.alienRank);
		rhs.lowQty			= node["lowQty"]		.as<int>(rhs.lowQty);
		rhs.highQty			= node["highQty"]		.as<int>(rhs.highQty);
		rhs.dQty			= node["dQty"]			.as<int>(rhs.dQty);
		rhs.extraQty		= node["extraQty"]		.as<int>(0); // give this a default, as it's not 100% needed, unlike the others.
		rhs.pctOutsideUfo	= node["pctOutsideUfo"]	.as<int>(rhs.pctOutsideUfo);
		rhs.itemSets		= node["itemSets"]		.as<std::vector<OpenXcom::ItemSet>>(rhs.itemSets);

		return true;
	}
};

template<>
struct convert<OpenXcom::BriefingData>
{
	///
	static Node encode(const OpenXcom::BriefingData& rhs)
	{
		Node node;

		node["palette"]			= rhs.palette;
		node["textOffset"]		= rhs.textOffset;
		node["title"]			= rhs.title;
		node["desc"]			= rhs.desc;
		node["music"]			= rhs.music;
		node["background"]		= rhs.background;
		node["showCraftText"]	= rhs.showCraftText;
		node["showTargetText"]	= rhs.showTargetText;

		return node;
	}

	///
	static bool decode(const Node& node, OpenXcom::BriefingData& rhs)
	{
		if (node.IsMap() == false)
			return false;

		rhs.palette			= node["palette"]		.as<int>(rhs.palette);
		rhs.textOffset		= node["textOffset"]	.as<int>(rhs.textOffset);
		rhs.title			= node["title"]			.as<std::string>(rhs.title);
		rhs.desc			= node["desc"]			.as<std::string>(rhs.desc);
		rhs.music			= node["music"]			.as<std::string>(rhs.music);
		rhs.background		= node["background"]	.as<std::string>(rhs.background);
		rhs.showCraftText	= node["showCraftText"]	.as<bool>(rhs.showCraftText);
		rhs.showTargetText	= node["showTargetText"].as<bool>(rhs.showTargetText);

		return true;
	}
};

}


namespace OpenXcom
{

/**
 * Creates a blank ruleset for a certain type of deployment data.
 * @param type - reference a string defining the type of AlienMission
 */
RuleAlienDeployment::RuleAlienDeployment(const std::string& type)
	:
		_type(type),
		_width(0),
		_length(0),
		_height(0),
		_civilians(0),
		_shade(-1),
		_noRetreat(false),
		_finalDestination(false),
		_finalMission(false),
		_durationMin(0),
		_durationMax(0),
		_objectiveTile(TILE),
		_objectivesRequired(0),
		_objectiveSuccessScore(0),
		_objectiveFailedScore(0),
		_despawnPenalty(0),
		_turnLimit(0),
		_chronoResult(FORCE_LOSE),
		_cheatTurn(CHEAT_TURN_DEFAULT),
		_markerIcon(-1),
		_alert("STR_ALIENS_TERRORISE"),
		_alertBg("BACK03.SCR"),
		_isAlienBase(false),
		_generatedMissionPct(0)
{}

/**
 * dTor.
 */
RuleAlienDeployment::~RuleAlienDeployment()
{}

/**
 * Loads the Deployment from a YAML file.
 * @param node - reference a YAML node
 */
void RuleAlienDeployment::load(const YAML::Node& node)
{
	_type				= node["type"]				.as<std::string>(_type);
	_data				= node["data"]				.as<std::vector<DeploymentData>>(_data);
	_width				= node["width"]				.as<int>(_width);
	_length				= node["length"]			.as<int>(_length);
	_height				= node["height"]			.as<int>(_height);
	_civilians			= node["civilians"]			.as<int>(_civilians);
	_terrains			= node["terrains"]			.as<std::vector<std::string>>(_terrains);
	_shade				= node["shade"]				.as<int>(_shade);
	_nextStage			= node["nextStage"]			.as<std::string>(_nextStage);
	_race				= node["race"]				.as<std::string>(_race);
	_noRetreat			= node["noRetreat"]			.as<bool>(_noRetreat);
	_finalDestination	= node["finalDestination"]	.as<bool>(_finalDestination);
	_finalMission		= node["finalMission"]		.as<bool>(_finalMission);
	_script				= node["script"]			.as<std::string>(_script);
	_briefingData		= node["briefing"]			.as<BriefingData>(_briefingData);
	_alert				= node["alert"]				.as<std::string>(_alert);
	_alertBg			= node["alertBg"]			.as<std::string>(_alertBg);
	_markerIcon			= node["markerIcon"]		.as<int>(_markerIcon);
	_markerType			= node["markerType"]		.as<std::string>("");

	if (_markerType.empty() == true)
	{
		if		(_type == "STR_ALIEN_BASE_ASSAULT")	_markerType = Target::stTarget[2u]; // "STR_ALIEN_BASE"
		else if	(_type == "STR_TERROR_MISSION"
			||	 _type == "STR_PORT_ATTACK")		_markerType = Target::stTarget[3u]; // "STR_TERROR_SITE"
		// not relevant:
		// STR_BASE_DEFENSE
		// STR_MARS_CYDONIA_LANDING
		// STR_MARS_THE_FINAL_ASSAULT
		//
		// unhandled (ie. handled by Ufo instantiations):
		// STR_SMALL_SCOUT
		// STR_MEDIUM_SCOUT
		// STR_LARGE_SCOUT
		// STR_HARVESTER
		// STR_ABDUCTOR
		// STR_TERROR_SHIP
		// STR_BATTLESHIP
		// STR_SUPPLY_SHIP
	}

	if (node["duration"])
	{
		_durationMin = node["duration"][0u].as<int>(_durationMin);
		_durationMax = node["duration"][1u].as<int>(_durationMax);
	}

	_musics = node["music"].as<std::vector<std::string>>(_musics); // NOTE: might not be compatible w/ sza_MusicRules.

	_objectiveTile = static_cast<TileType>(node["objectiveTile"].as<int>(_objectiveTile));
	_objectivesRequired	= node["objectivesRequired"].as<int>(_objectivesRequired);
	_objectiveNotice	= node["objectiveNotice"].as<std::string>(_objectiveNotice);

	_despawnPenalty	= node["despawnPenalty"].as<int>(_despawnPenalty);

	if (node["objectiveSuccess"])
	{
		_objectiveSuccessText	= node["objectiveSuccess"][0u].as<std::string>(_objectiveSuccessText);
		_objectiveSuccessScore	= node["objectiveSuccess"][1u].as<int>(_objectiveSuccessScore);
	}
	if (node["objectiveFailed"])
	{
		_objectiveFailedText	= node["objectiveFailed"][0u].as<std::string>(_objectiveFailedText);
		_objectiveFailedScore	= node["objectiveFailed"][1u].as<int>(_objectiveFailedScore);
	}

	_cheatTurn		= node["cheatTurn"].as<int>(_cheatTurn);
	_turnLimit		= node["turnLimit"].as<int>(_turnLimit);
	_chronoResult	= static_cast<ChronoResult>(node["chronoResult"].as<int>(_chronoResult));

	_isAlienBase			= node["isAlienBase"]			.as<bool>(_isAlienBase);
	_generatedMissionPct	= node["generatedMissionPct"]	.as<int>(_generatedMissionPct);
	if (node["generatedMission"])
		_generatedMission.load(node["generatedMission"]);
}

/**
 * Gets the string that types this deployment.
 * @note Each deployment has a unique type.
 * @return, reference to the deployment-type
 */
const std::string& RuleAlienDeployment::getType() const
{
	return _type;
}

/**
 * Gets a pointer to the data.
 * @return, pointer to a vector holding the DeploymentData
 */
const std::vector<DeploymentData>* RuleAlienDeployment::getDeploymentData() const
{
	return &_data;
}

/**
 * Gets the dimensions of this deployment's battlefield.
 * @param width		- pointer to width
 * @param lenght	- pointer to length
 * @param heigth	- pointer to height
 */
void RuleAlienDeployment::getDimensions(
		int* width,
		int* lenght,
		int* heigth) const
{
	*width	= _width;
	*lenght	= _length;
	*heigth	= _height;
}

/**
 * Gets the number of civilians.
 * @return, the number of civilians
 */
int RuleAlienDeployment::getCivilians() const
{
	return _civilians;
}

/**
 * Gets eligible terrains for battlescape generation.
 * @return, vector of terrain-type strings
 */
const std::vector<std::string>& RuleAlienDeployment::getDeployTerrains() const
{
	return _terrains;
}

/**
 * Gets the shade-level for Battlescape generation.
 * @return, the shade-level
 */
int RuleAlienDeployment::getShade() const
{
	return _shade;
}

/**
 * Gets the next-stage of this RuleAlienDeployment.
 * @return, the next-stage of the mission
 */
const std::string& RuleAlienDeployment::getNextStage() const
{
	return _nextStage;
}

/**
 * Gets the next-stage's aLien race.
 * @return, the alien race
 */
const std::string& RuleAlienDeployment::getRace() const
{
	return _race;
}

/**
 * Gets the script-type to use to generate a mission for this RuleAlienDeployment.
 * @return, the script-type
 */
const std::string& RuleAlienDeployment::getScriptType() const
{
	return _script;
}

/**
 * Checks if this RuleAlienDeployment is where to send Craft via the ConfirmCydonia btn.
 * @return, true if Cydonia
 */
bool RuleAlienDeployment::isFinalDestination() const
{
	return _finalDestination;
}

/**
 * Checks if aborting or losing this RuleAlienDeployment will lose the game.
 * @return, true if fail
 */
bool RuleAlienDeployment::isNoRetreat() const
{
	return _noRetreat;
}

/**
 * Checks if this RuleAlienDeployment finishes the game.
 * @return, true if final tactical
 */
bool RuleAlienDeployment::isFinalMission() const
{
	return _finalMission;
}

/**
 * Gets the alert-message displayed when this RuleAlienDeployment spawns.
 * @return, ID for the message
 */
const std::string& RuleAlienDeployment::getAlertMessage() const
{
	return _alert;
}

/**
 * Gets the alert-background displayed when this RuleAlienDeployment spawns.
 * @return, ID for the background
 */
const std::string& RuleAlienDeployment::getAlertBackground() const
{
	return _alertBg;
}

/**
 * Gets the briefing-data for this RuleAlienDeployment.
 * @return, data for the briefing window to use
 */
BriefingData RuleAlienDeployment::getBriefingData() const
{
	return _briefingData;
}

/**
 * Gets the globe-marker-type for this RuleAlienDeployment.
 * @return, reference to the marker-type
 */
const std::string& RuleAlienDeployment::getMarkerType() const
{
	return _markerType;
}

/**
 * Gets the globe-marker for this RuleAlienDeployment.
 * @return, marker-ID (-1 if not defined in ruleset)
 */
int RuleAlienDeployment::getMarkerIcon() const
{
	return _markerIcon;
}

/**
 * Gets the minimum duration for this RuleAlienDeployment to exist on the Globe.
 * @return, minimum duration in hours
 */
int RuleAlienDeployment::getDurationMin() const
{
	return _durationMin;
}

/**
 * Gets the maximum duration for this RuleAlienDeployment to exist on the Globe.
 * @return, maximum duration in hours
 */
int RuleAlienDeployment::getDurationMax() const
{
	return _durationMax;
}

/**
 * Gets the list of musics that this RuleAlienDeployment can choose from.
 * @return, list of tracks
 */
const std::vector<std::string>& RuleAlienDeployment::getDeploymentMusics() const
{
	return _musics;
}

/**
 * Gets the objective-tiletype for this RuleAlienDeployment (eg alien-control-consoles).
 * @return, objective-tiletype (RuleItem.h)
 */
TileType RuleAlienDeployment::getPlayerObjective() const
{
	return _objectiveTile;
}

/**
 * Gets the quantity of objective-tiles required by this RuleAlienDeployment.
 * @return, quantity of objective-tiles
 */
int RuleAlienDeployment::getObjectivesRequired() const
{
	return _objectivesRequired;
}

/**
 * Gets the string for the popup to splash when objective-conditions are met.
 * @return, string to pop
 */
const std::string& RuleAlienDeployment::getObjectiveNotice() const
{
	return _objectiveNotice;
}

/**
 * Fills out the variables associated with mission success and returns if those
 * variables actually contain anything.
 * @param text	- reference to the text to alter
 * @param score	- reference to the score to alter
 * @return, true if anything worthwhile happened
 */
bool RuleAlienDeployment::getObjectiveCompleteInfo(
		std::string& text,
		int& score) const
{
	text  = _objectiveSuccessText;
	score = _objectiveSuccessScore;

	return text.empty() == false;
}

/**
 * Fills out the variables associated with mission failure and returns if those
 * variables actually contain anything.
 * @param text	- reference to the text to alter
 * @param score	- reference to the score to alter
 * @return, true if anything worthwhile happened
 */
bool RuleAlienDeployment::getObjectiveFailedInfo(
		std::string& text,
		int& score) const
{
	text  = _objectiveFailedText;
	score = _objectiveFailedScore;

	return text.empty() == false;
}

/**
 * Gets the score penalty XCom receives for letting a mission despawn.
 * @return, penalty
 */
int RuleAlienDeployment::getDespawnPenalty() const
{
	return _despawnPenalty;
}

/**
 * Gets the maximum turns before forcing a battle to end.
 * @return, the turn-limit
 */
int RuleAlienDeployment::getTurnLimit() const
{
	return _turnLimit;
}

/**
 * Gets the result to perform when the turn-limit is reached.
 * @return, the ChronoResult (RuleAlienDeployment.h)
 */
ChronoResult RuleAlienDeployment::getChronoResult() const
{
	return _chronoResult;
}

/**
 * Gets the turn at which the player's units become exposed to the AI.
 * @return, the turn for the AI to start cheating
 */
int RuleAlienDeployment::getCheatTurn() const
{
	return _cheatTurn;
}

/**
 * Gets if this deployment is for an aLien Base (for quick-battles).
 * @return, true if aLien Base
 */
bool RuleAlienDeployment::isAlienBase() const
{
	return _isAlienBase;
}

/**
 * Gets the type of AlienMission that an AlienBase can generate.
 * @note This is a supply-mission by default.
 * @return, type of AlienMission
 */
std::string RuleAlienDeployment::getBaseGeneratedType() const
{
	const std::string type (_generatedMission.getOptionResult());
	if (type.empty() == false)
		return type;

	return "STR_ALIEN_SUPPLY";
}

/**
 * Gets the chance of an AlienBase generating an AlienMission.
 * @return, percent chance of a mission being generated per day
 */
int RuleAlienDeployment::getBaseGeneratedPct() const
{
	return _generatedMissionPct;
}

}
