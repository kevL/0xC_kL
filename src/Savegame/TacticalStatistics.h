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
 * along with OpenXcom. If not, see <http:///www.gnu.org/licenses/>.
 */

#ifndef OPENXCOM_TACTICALSTATISTICS_H
#define OPENXCOM_TACTICALSTATISTICS_H

//#include <map>
//#include <string>

//#include <yaml-cpp/yaml.h>

#include "GameTime.h"


namespace OpenXcom
{

/**
 * Container for SoldierDiary's TacticalStatistics.
 */
struct TacticalStatistics
{
	static const int NIGHT_SHADE = 9; // cf. TileEngine::MAX_SHADE_TO_SEE_UNITS

	bool
		success,
		valiantCrux;

	int
		id,
		score,
		shade;

	std::string
		alienRace,
		country,
		rating,
		region,
		type,
		ufo;

	std::map<int,int> injuryList;

	GameTime timeStat;

//	std::string getMissionTypeLowerCase()
//	{
//		if		(type == "STR_UFO_CRASH_RECOVERY")	return "STR_UFO_CRASH_RECOVERY_LC";
//		else if (type == "STR_UFO_GROUND_ASSAULT")	return "STR_UFO_GROUND_ASSAULT_LC";
//		else if (type == "STR_BASE_DEFENSE")		return "STR_BASE_DEFENSE_LC";
//		else if (type == "STR_ALIEN_BASE_ASSAULT")	return "STR_ALIEN_BASE_ASSAULT_LC";
//		else if (type == "STR_TERROR_MISSION")		return "STR_TERROR_MISSION_LC";
//		else if (type == "STR_PORT_ATTACK")			return "STR_PORT_ATTACK_LC";
//		else										return "type error";
//	}


	/**
	 * Creates a blank TacticalStatistics.
	 * @note TacticalStatistics holds data about a tactical battle.
	 */
	TacticalStatistics()
		:
			id(0),
			timeStat(0,0,0,0,0,0),
			region("STR_UNKNOWN"),
			country("STR_UNKNOWN"),
			ufo("NO_UFO"),
			success(false),
			score(0),
			alienRace("STR_UNKNOWN"),
			shade(0),
			valiantCrux(false)
	{}

	/**
	 * Creates TacticalStatistics from a YAML node.
	 * @note TacticalStatistics holds data about a tactical battle.
	 * @param node - reference a YAML node
	 */
	explicit TacticalStatistics(const YAML::Node& node)
		:
			timeStat(0,0,0,0,0,0)
	{
		load(node);
	}

	/**
	 * dTor.
	 */
	~TacticalStatistics()
	{}

	/**
	 * Loads a TacticalStatistics node from YAML.
	 * @param node - reference a YAML node
	 */
	void load(const YAML::Node& node)
	{
		timeStat.load(node["time"]);

		id			= node["id"]			.as<int>(id);
		region		= node["region"]		.as<std::string>(region);
		country		= node["country"]		.as<std::string>(country);
		type		= node["type"]			.as<std::string>(type);
		ufo			= node["ufo"]			.as<std::string>(ufo);
		success		= node["success"]		.as<bool>(success);
		score		= node["score"]			.as<int>(score);
		rating		= node["rating"]		.as<std::string>(rating);
		alienRace	= node["alienRace"]		.as<std::string>(alienRace);
		shade		= node["shade"]			.as<int>(shade);
		injuryList	= node["injuryList"]	.as<std::map<int,int>>(injuryList);
		valiantCrux	= node["valiantCrux"]	.as<bool>(valiantCrux);
	}

	/**
	 * Saves a TacticalStatistics node to YAML.
	 * @return, a YAML node
	 */
	YAML::Node save() const
	{
		YAML::Node node;

		node["id"]			= id;
		node["time"]		= timeStat.save();
		node["region"]		= region;
		node["country"]		= country;
		node["type"]		= type;
		node["ufo"]			= ufo;
		node["success"]		= success;
		node["score"]		= score;
		node["rating"]		= rating;
		node["alienRace"]	= alienRace;
		node["shade"]		= shade;

		if (injuryList.empty() == false)
			node["injuryList"] = injuryList;

		if (valiantCrux == true)
			 node["valiantCrux"] = valiantCrux;

		return node;
	}

	/**
	 * Checks if these TacticalStatistics are for an AlienBase tactical.
	 * @return, true if data is for an AlienBase tactical
	 */
	bool isAlienBase() const
	{
		if (type == "STR_ALIEN_BASE_ASSAULT")
			return true;

		return false;
	}

	/**
	 * Checks if these TacticalStatistics are for a BaseDefense tactical.
	 * @return, true if data is for a BaseDefense tactical
	 */
	bool isBaseDefense() const
	{
		if (type == "STR_BASE_DEFENSE")
			return true;

		return false;
	}

	/**
	 * Checks if these TacticalStatistics are for a UFO tactical.
	 * @return, true if data is for a UFO tactical
	 */
	bool isUfoMission() const
	{
		if (ufo != "NO_UFO")
			return true;

		return false;
	}
};

}

#endif
