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
 * along with OpenXcom. If not, see <http:///www.gnu.org/licenses/>.
 */

#ifndef OPENXCOM_BATTLEUNITSTATISTICS_H
#define OPENXCOM_BATTLEUNITSTATISTICS_H

//#include <string>

//#include <yaml-cpp/yaml.h>

#include "BattleUnit.h"


namespace OpenXcom
{

/**
 * SoldierDiary container for the statistics of a BattleUnitKill.
 */
struct BattleUnitKill
{
	std::string
		_load,
		_race,
		_rank,
		_weapon;
	int
		_tactical,
		_points,
		_turn;

	UnitFaction _faction;
	UnitStatus _status;


	/// cTor [1].
	BattleUnitKill(
			std::string unitRank,
			std::string race,
			std::string weapon,
			std::string load,
			UnitFaction faction,
			UnitStatus status,
			int tactical,
			int turn,
			int points)
		:
			_rank(unitRank),
			_race(race),
			_weapon(weapon),
			_load(load),
			_faction(faction),
			_status(status),
			_tactical(tactical),
			_turn(turn),
			_points(points)
	{}

	/// cTor [2].
	explicit BattleUnitKill(const YAML::Node& node)
	{
		load(node);
	}

	/// dTor.
	~BattleUnitKill()
	{}

	/// Loads a BattleUnitKill node from YAML.
	void load(const YAML::Node& node)
	{
		_rank		= node["rank"]		.as<std::string>(_rank);
		_race		= node["race"]		.as<std::string>(_race);
		_weapon		= node["weapon"]	.as<std::string>(_weapon);
		_load		= node["load"]		.as<std::string>(_load);
		_tactical	= node["tactical"]	.as<int>(_tactical);
		_turn		= node["turn"]		.as<int>(_turn);
		_points		= node["points"]	.as<int>(_points);

		_status		= static_cast<UnitStatus>(node["status"]	.as<int>());
		_faction	= static_cast<UnitFaction>(node["faction"]	.as<int>());
	}

	/// Saves a BattleUnitKill node to YAML.
	YAML::Node save() const
	{
		YAML::Node node;

		node["rank"]		= _rank;
		node["race"]		= _race;
		node["weapon"]		= _weapon;
		node["load"]		= _load;
		node["tactical"]	= _tactical;
		node["turn"]		= _turn;
		node["points"]		= _points;

		node["status"]		= static_cast<int>(_status);
		node["faction"]		= static_cast<int>(_faction);

		return node;
	}

	/// Converts victim's Status to string.
	std::string getUnitStatusString() const
	{
		switch (_status)
		{
			case STATUS_DEAD:			return "STATUS_DEAD";
			case STATUS_UNCONSCIOUS:	return "STATUS_UNCONSCIOUS";
		}
		return "status error";
	}

	/// Converts victim's Faction to string.
	std::string getUnitFactionString() const
	{
		switch (_faction)
		{
			case FACTION_NONE:		return "FACTION_NONE";
			case FACTION_PLAYER:	return "FACTION_PLAYER";
			case FACTION_HOSTILE:	return "FACTION_HOSTILE";
			case FACTION_NEUTRAL:	return "FACTION_NEUTRAL";
		}
		return "faction error";
	}

	/// Makes turn unique across all kills.
	void setTurn()
	{
		_turn += _tactical * 300;	// maintain divisibility by 3 to determine
	}								// the faction-turn this kill happened on.

	/// Checks to see if turn was during FACTION_HOSTILE side.
	bool hostileTurn() const
	{
		if (_turn % 3 == FACTION_HOSTILE) // cf. UnitFaction (BattleUnit.h)
			return true;

		return false;
	}

}; // end BattleUnitKill.


/**
 * Container for BattleUnitStatistics.
 */
struct BattleUnitStatistics
{

	bool
		ironMan,		// Tracks if the soldier was the only soldier on the tactical
		KIA,			// Tracks if the soldier was killed in battle
		loneSurvivor,	// Tracks if the soldier was the only survivor
		MIA,			// Tracks if the soldier was left behind :(
		nikeCross,		// Tracks if a soldier killed every alien
		wasUnconscious;	// Tracks if the soldier fell unconscious

	int
		daysWounded,			// Tracks how many days the unit was wounded for
		hitCounter,				// Tracks how many times the unit was hit
		longDistanceHitCounter,	// Tracks how many long distance shots were landed
		lowAccuracyHitCounter,	// Tracks how many times the unit landed a low probability shot
		shotAtCounter,			// Tracks how many times the unit was shot at
		shotByFriendlyCounter,	// Tracks how many times the unit was hit by a friendly
		shotFriendlyCounter,	// Tracks how many times the unit was hit a friendly
		shotsFiredCounter,		// Tracks how many times a unit has shot
		shotsLandedCounter,		// Tracks how many times a unit has hit his target
		medikitApplications,	// Tracks how many times a unit has used the medikit
		revivedSoldier;			// Tracks how many times this soldier revived another unit

	std::vector<BattleUnitKill*> kills; // Tracks kills


	/// cTor.
	BattleUnitStatistics()
		:
			wasUnconscious(false),
			shotAtCounter(0),
			hitCounter(0),
			shotByFriendlyCounter(0),
			shotFriendlyCounter(0),
			longDistanceHitCounter(0),
			lowAccuracyHitCounter(0),
			shotsFiredCounter(0),
			shotsLandedCounter(0),
			medikitApplications(0),
			revivedSoldier(0),
			ironMan(false),		// ->> calculated at end of Tactical, do not save ->>
			loneSurvivor(false),
			nikeCross(false),
			KIA(false),
			MIA(false),
			daysWounded(0)
	{}

	///
	explicit BattleUnitStatistics(const YAML::Node& node)
	{
		load(node);
	}

	/// dTor.
	~BattleUnitStatistics()
	{}

	///
	void load(const YAML::Node& node)
	{
		for (YAML::const_iterator
				i = node["kills"].begin();
				i != node["kills"].end();
				++i)
		{
			kills.push_back(new BattleUnitKill(*i));
		}

		wasUnconscious			= node["wasUnconscious"]		.as<bool>(wasUnconscious);
		shotAtCounter			= node["shotAtCounter"]			.as<int>(shotAtCounter);
		hitCounter				= node["hitCounter"]			.as<int>(hitCounter);
		shotByFriendlyCounter	= node["shotByFriendlyCounter"]	.as<int>(shotByFriendlyCounter);
		shotFriendlyCounter		= node["shotFriendlyCounter"]	.as<int>(shotFriendlyCounter);
		longDistanceHitCounter	= node["longDistanceHitCounter"].as<int>(longDistanceHitCounter);
		lowAccuracyHitCounter	= node["lowAccuracyHitCounter"]	.as<int>(lowAccuracyHitCounter);
		shotsFiredCounter		= node["shotsFiredCounter"]		.as<int>(shotsFiredCounter);
		shotsLandedCounter		= node["shotsLandedCounter"]	.as<int>(shotsLandedCounter);
		medikitApplications		= node["medikitApplications"]	.as<int>(medikitApplications);
		revivedSoldier			= node["revivedSoldier"]		.as<int>(revivedSoldier);
		ironMan =
		loneSurvivor =
		nikeCross =
		KIA =
		MIA = false;
		daysWounded = 0;
	}

	///
	YAML::Node save() const
	{
		YAML::Node node;

		for (std::vector<BattleUnitKill*>::const_iterator
				i = kills.begin();
				i != kills.end();
				++i)
		{
			node["kills"].push_back((*i)->save());
		}

		if (wasUnconscious == true)			node["wasUnconscious"]			= wasUnconscious;
		if (shotAtCounter != 0)				node["shotAtCounter"]			= shotAtCounter;
		if (hitCounter != 0)				node["hitCounter"]				= hitCounter;
		if (shotByFriendlyCounter != 0)		node["shotByFriendlyCounter"]	= shotByFriendlyCounter;
		if (shotFriendlyCounter != 0)		node["shotFriendlyCounter"]		= shotFriendlyCounter;
		if (longDistanceHitCounter != 0)	node["longDistanceHitCounter"]	= longDistanceHitCounter;
		if (lowAccuracyHitCounter != 0)		node["lowAccuracyHitCounter"]	= lowAccuracyHitCounter;
		if (shotsFiredCounter != 0)			node["shotsFiredCounter"]		= shotsFiredCounter;
		if (shotsLandedCounter != 0)		node["shotsLandedCounter"]		= shotsLandedCounter;
		if (medikitApplications != 0)		node["medikitApplications"]		= medikitApplications;
		if (revivedSoldier != 0)			node["revivedSoldier"]			= revivedSoldier;

		return node;
	}

	/// Checks if these statistics are default.
	bool statsDefault() const
	{
		if (kills.empty() == true
			&& wasUnconscious == false
			&& shotAtCounter == 0
			&& hitCounter == 0
			&& shotByFriendlyCounter == 0
			&& shotFriendlyCounter == 0
			&& longDistanceHitCounter == 0
			&& lowAccuracyHitCounter == 0
			&& shotsFiredCounter == 0
			&& shotsLandedCounter == 0
			&& medikitApplications == 0
			&& revivedSoldier == 0)
		{
			return true;
		}
		return false;
	}

	/// Checks if unit has fired on a friendly.
	bool hasFriendlyFired() const
	{
		for (std::vector<BattleUnitKill*>::const_iterator
				i = kills.begin();
				i != kills.end();
				++i)
		{
			switch ((*i)->_faction)
			{
				case FACTION_PLAYER:
				case FACTION_NEUTRAL:
					return true;
			}
		}
		return false;
	}

}; // end BattleUnitStatistics.

}

#endif
