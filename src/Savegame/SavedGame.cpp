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

#include "SavedGame.h"

//#include <cmath>
//#include <algorithm>
#include <fstream>
#include <iomanip>
//#include <sstream>

#include "../version.h"

//#include <yaml-cpp/yaml.h>

#include "AlienBase.h"
#include "AlienMission.h"
#include "AlienStrategy.h"
#include "Base.h"
#include "Country.h"
#include "Craft.h"
#include "GameTime.h"
#include "ItemContainer.h"
#include "MissionStatistics.h"
#include "Production.h"
#include "Region.h"
#include "ResearchProject.h"
#include "SavedBattleGame.h"
#include "SerializationHelper.h"
#include "SoldierDead.h"
#include "TerrorSite.h"
#include "Transfer.h"
#include "Ufo.h"
#include "Waypoint.h"

#include "../Engine/CrossPlatform.h"
#include "../Engine/Exception.h"
#include "../Engine/Language.h"
#include "../Engine/Logger.h"
#include "../Engine/Options.h"
#include "../Engine/RNG.h"

#include "../Ruleset/RuleCountry.h"
#include "../Ruleset/RuleManufacture.h"
#include "../Ruleset/RuleRegion.h"
#include "../Ruleset/RuleResearch.h"
#include "../Ruleset/Ruleset.h"


namespace OpenXcom
{

const std::string
	SavedGame::AUTOSAVE_GEOSCAPE	= "_autogeo_.asav",
	SavedGame::AUTOSAVE_BATTLESCAPE	= "_autobattle_.asav",
	SavedGame::QUICKSAVE			= "_quick_.asav",
	SavedGame::SAVE_EXT				= ".sav";


/// *** FUNCTOR ***
struct findRuleResearch
	:
		public std::unary_function<ResearchProject*, bool>
{
	const RuleResearch* _resRule;
	explicit findRuleResearch(const RuleResearch* const resRule);

	bool operator() (const ResearchProject* const rp) const;
};
///
findRuleResearch::findRuleResearch(const RuleResearch* const resRule)
	:
		_resRule(resRule)
{}
///
bool findRuleResearch::operator() (const ResearchProject* const rp) const
{
	return (_resRule == rp->getRules());
}


/// *** FUNCTOR ***
struct equalProduction
	:
		public std::unary_function<Production*, bool>
{
	const RuleManufacture* _manfRule;
	explicit equalProduction(const RuleManufacture* const manfRule);

	bool operator() (const Production* const p) const;
};
///
equalProduction::equalProduction(const RuleManufacture* const manfRule)
	:
		_manfRule(manfRule)
{}
///
bool equalProduction::operator() (const Production* const p) const
{
	return (p->getRules() == _manfRule);
}


/**
 * Initializes the player's SavedGame when starting or re-loading.
 * @param rules - pointer to the Ruleset
 */
SavedGame::SavedGame(const Ruleset* const rules)
	:
		_rules(rules),
		_difficulty(DIFF_BEGINNER),
		_end(END_NONE),
		_ironman(false),
		_globeLon(0.),
		_globeLat(0.),
		_globeZoom(0u),
		_dfLon(0.),
		_dfLat(0.),
		_dfZoom(0u),
		_battleSave(nullptr),
		_debugGeo(false),
		_warned(false),
		_monthsPassed(-1),
		_debugArgDone(false)
//		_detail(true),
//		_radarLines(false),
//		_selectedBase(0),
//		_lastselectedArmor("STR_ARMOR_NONE_UC")
{
//	_time = new GameTime(6,1,1,1999,12,0,0);
	_time = new GameTime(1,1,1999,12,0,0);

	_alienStrategy = new AlienStrategy();

	_funds.push_back(0);
	_maintenance.push_back(0);
	_researchScores.push_back(0);
	_income.push_back(0);
	_expenditure.push_back(0);
}

/**
 * Deletes this object from memory.
 */
SavedGame::~SavedGame()
{
	delete _time;

	for (std::vector<Country*>::const_iterator
			i = _countries.begin();
			i != _countries.end();
			++i)
		delete *i;

	for (std::vector<Region*>::const_iterator
			i = _regions.begin();
			i != _regions.end();
			++i)
		delete *i;

	for (std::vector<Base*>::const_iterator
			i = _bases.begin();
			i != _bases.end();
			++i)
		delete *i;

	for (std::vector<Ufo*>::const_iterator
			i = _ufos.begin();
			i != _ufos.end();
			++i)
		delete *i;

	for (std::vector<Waypoint*>::const_iterator
			i = _waypoints.begin();
			i != _waypoints.end();
			++i)
		delete *i;

	for (std::vector<TerrorSite*>::const_iterator
			i = _terrorSites.begin();
			i != _terrorSites.end();
			++i)
		delete *i;

	for (std::vector<AlienBase*>::const_iterator
			i = _alienBases.begin();
			i != _alienBases.end();
			++i)
		delete *i;

	delete _alienStrategy;

	for (std::vector<AlienMission*>::const_iterator
			i = _activeMissions.begin();
			i != _activeMissions.end();
			++i)
		delete *i;

	for (std::vector<SoldierDead*>::const_iterator
			i = _deadSoldiers.begin();
			i != _deadSoldiers.end();
			++i)
		delete *i;

	for (std::vector<MissionStatistics*>::const_iterator
			i = _missionStatistics.begin();
			i != _missionStatistics.end();
			++i)
		delete *i;

	for (std::vector<ResearchGeneral*>::const_iterator
			i = _research.begin();
			i != _research.end();
			++i)
		delete *i;

	if (_battleSave != nullptr) delete _battleSave;
}

/**
 * Gets all the info of the saves found in the user folder.
 * @param lang		- pointer to the loaded Language
 * @param autoquick	- true to include autosaves and quicksaves
 * @return, vector of SaveInfo structs (SavedGame.h)
 */
std::vector<SaveInfo> SavedGame::getList( // static.
		const Language* const lang,
		bool autoquick)
{
	std::vector<SaveInfo> info;

	if (autoquick == true)
	{
		const std::vector<std::string> saves (CrossPlatform::getFolderContents(
																			Options::getUserFolder(),
																			"asav"));
		for (std::vector<std::string>::const_iterator
				i = saves.begin();
				i != saves.end();
				++i)
		{
			try
			{
				info.push_back(getSaveInfo(*i, lang));
			}
			catch (Exception& e)
			{
				Log(LOG_ERROR) << e.what();
				continue;
			}
			catch (YAML::Exception& e)
			{
				Log(LOG_ERROR) << e.what();
				continue;
			}
		}
	}

	const std::vector<std::string> saves (CrossPlatform::getFolderContents(
																		Options::getUserFolder(),
																		"sav"));
	for (std::vector<std::string>::const_iterator
			i = saves.begin();
			i != saves.end();
			++i)
	{
		try
		{
			info.push_back(getSaveInfo(*i, lang));
		}
		catch (Exception& e)
		{
			Log(LOG_ERROR) << e.what();
			continue;
		}
		catch (YAML::Exception& e)
		{
			Log(LOG_ERROR) << e.what();
			continue;
		}
	}

	return info;
}

/**
 * Gets the info of a specific save file.
 * @param file - reference a save by filename
 * @param lang - pointer to the loaded Language
 * @return, the SaveInfo (SavedGame.h)
 */
SaveInfo SavedGame::getSaveInfo( // private/static.
		const std::string& file,
		const Language* const lang)
{
	const std::string path (Options::getUserFolder() + file);
	const YAML::Node doc (YAML::LoadFile(path));

	SaveInfo save;
	save.file = file;

	if (save.file == QUICKSAVE)
	{
		save.label = lang->getString("STR_QUICK_SAVE_SLOT");
		save.reserved = true;
	}
	else if (save.file == AUTOSAVE_GEOSCAPE)
	{
		save.label = lang->getString("STR_AUTO_SAVE_GEOSCAPE_SLOT");
		save.reserved = true;
	}
	else if (save.file == AUTOSAVE_BATTLESCAPE)
	{
		save.label = lang->getString("STR_AUTO_SAVE_BATTLESCAPE_SLOT");
		save.reserved = true;
	}
	else
	{
		if (doc["label"])
			save.label = Language::utf8ToWstr(doc["label"].as<std::string>());
		else
			save.label = Language::fsToWstr(CrossPlatform::noExt(file));

		save.reserved = false;
	}

	save.timestamp = CrossPlatform::getDateModified(path);
	const std::pair<std::wstring, std::wstring> timePair (CrossPlatform::timeToString(save.timestamp));
	save.isoDate = timePair.first;
	save.isoTime = timePair.second;

	std::wostringstream details;
	if (doc["base"])
		details << Language::utf8ToWstr(doc["base"].as<std::string>())
				<< L" - ";

	GameTime gt (GameTime(1,1,1999,12,0,0));
	gt.load(doc["time"]);
	details << gt.getDayString(lang)
			<< L" "
			<< lang->getString(gt.getMonthString())
			<< L" "
			<< gt.getYear()
			<< L" "
			<< gt.getHour()
			<< L":"
			<< std::setfill(L'0')
			<< std::setw(2)
			<< gt.getMinute();

	if (doc["turn"])
		details << L" - "
				<< lang->getString(doc["mission"].as<std::string>())
				<< L" "
				<< lang->getString("STR_TURN").arg(doc["turn"].as<int>());

	if (doc["ironman"].as<bool>(false))
		details << L" "
				<< lang->getString("STR_IRONMAN");

	save.details = details.str();

	if (doc["rulesets"])
		save.rulesets = doc["rulesets"].as<std::vector<std::string>>();

	save.mode = static_cast<SaveMode>(doc["mode"].as<int>());

	return save;
}

/**
 * Loads a SavedGame's contents from a YAML file.
 * @note Assumes the saved game is blank.
 * @param file	- reference a YAML file
 * @param rules	- pointer to Ruleset
 */
void SavedGame::load(
		const std::string& file,
		Ruleset* const rules) // <- used only to obviate const if loading a battleSave.
{
	//Log(LOG_INFO) << "SavedGame::load()";
	std::string type (Options::getUserFolder() + file);
	const std::vector<YAML::Node> nodes (YAML::LoadAllFromFile(type));
	if (nodes.empty() == true)
	{
		throw Exception(file + " is not a valid save file");
	}

	YAML::Node brief (nodes[0u]); // get data from brief-info section

//	std::string version (brief["version"].as<std::string>());
//	if (version != OPENXCOM_VERSION_SHORT)
//	{
//		throw Exception("Version mismatch");
//	}

	_time->load(brief["time"]);

	if (brief["label"])
		_label = Language::utf8ToWstr(brief["label"].as<std::string>());
	else
		_label = Language::fsToWstr(file);

	_ironman = brief["ironman"].as<bool>(_ironman);


	YAML::Node doc (nodes[1u]); // get data from non-brief section

	if (doc["rng"]
		&& (Options::reSeedOnLoad == false || _ironman == true))
	{
		RNG::setSeed(doc["rng"].as<uint64_t>());
		//Log(LOG_INFO) << ". seed x= " << RNG::getSeed();
	}
	else
	{
		RNG::setSeed();
		//Log(LOG_INFO) << ". reSeed x= " << RNG::getSeed();
	}

	int diff (doc["difficulty"].as<int>(_difficulty));
	if (diff < 0) // safety.
	{
		diff = 0;
		Log(LOG_WARNING) << "Difficulty in the save file is negative ... loading as BEGINNER.";
	}
	_difficulty = static_cast<DifficultyLevel>(diff);

	_end = static_cast<EndType>(doc["end"].as<int>(_end));

	_monthsPassed			= doc["monthsPassed"]		.as<int>(_monthsPassed);
	_warned					= doc["warned"]				.as<bool>(_warned);
	_graphRegionToggles		= doc["graphRegionToggles"]	.as<std::string>(_graphRegionToggles);
	_graphCountryToggles	= doc["graphCountryToggles"].as<std::string>(_graphCountryToggles);
	_graphFinanceToggles	= doc["graphFinanceToggles"].as<std::string>(_graphFinanceToggles);
	_funds					= doc["funds"]				.as<std::vector<int64_t>>(_funds);
	_maintenance			= doc["maintenance"]		.as<std::vector<int64_t>>(_maintenance);
	_researchScores			= doc["researchScores"]		.as<std::vector<int>>(_researchScores);
	_income					= doc["income"]				.as<std::vector<int64_t>>(_income);
	_expenditure			= doc["expenditure"]		.as<std::vector<int64_t>>(_expenditure);
	_ids					= doc["ids"]				.as<std::map<std::string, int>>(_ids);
//	_radarLines				= doc["radarLines"]			.as<bool>(_radarLines);
//	_detail					= doc["detail"]				.as<bool>(_detail);

	_globeLon	= doc["globeLon"].as<double>(_globeLon);
	_globeLat	= doc["globeLat"].as<double>(_globeLat);
	_globeZoom	= static_cast<size_t>(doc["globeZoom"].as<int>(_globeZoom));


	Log(LOG_INFO) << ". load countries";
	for (YAML::const_iterator
			i = doc["countries"].begin();
			i != doc["countries"].end();
			++i)
	{
		type = (*i)["type"].as<std::string>();
		if (_rules->getCountry(type) != nullptr)
		{
			Country* const country (new Country(_rules->getCountry(type)));
			country->load(*i);
			_countries.push_back(country);
		}
		else Log(LOG_ERROR) << "Failed to load country: Type [" << type << "]";
	}

	Log(LOG_INFO) << ". load regions";
	for (YAML::const_iterator
			i = doc["regions"].begin();
			i != doc["regions"].end();
			++i)
	{
		type = (*i)["type"].as<std::string>();
		if (_rules->getRegion(type) != nullptr)
		{
			Region* const region (new Region(_rules->getRegion(type)));
			region->load(*i);
			_regions.push_back(region);
		}
		else Log(LOG_ERROR) << "Failed to load region: Type [" << type << "]";
	}

	Log(LOG_INFO) << ". load alien bases"; // AlienBases must be loaded before AlienMissions.
	for (YAML::const_iterator
			i = doc["alienBases"].begin();
			i != doc["alienBases"].end();
			++i)
	{
		type = (*i)["deployment"].as<std::string>("STR_ALIEN_BASE_ASSAULT"); // default.
		if (_rules->getDeployment(type) != nullptr)
		{
			AlienBase* const aBase (new AlienBase(_rules->getDeployment(type)));
			aBase->load(*i);
			_alienBases.push_back(aBase);
		}
		else Log(LOG_ERROR) << "Failed to load deployment for alien base: Type [" << type << "]";
	}

	Log(LOG_INFO) << ". load missions"; // AlienMissions must be loaded before Ufos.
	const YAML::Node& missions (doc["alienMissions"]);
	for (YAML::const_iterator
			i = missions.begin();
			i != missions.end();
			++i)
	{
		type = (*i)["type"].as<std::string>();
		if (_rules->getAlienMission(type) != nullptr)
		{
			const RuleAlienMission& missionRule (*_rules->getAlienMission(type));
			std::auto_ptr<AlienMission> mission (new AlienMission(missionRule, *this));
			mission->load(*i);
			_activeMissions.push_back(mission.release());
		}
		else Log(LOG_ERROR) << "Failed to load mission: Type [" << type << "]";
	}

	Log(LOG_INFO) << ". load ufos"; // Ufos must be loaded after AlienMissions.
	for (YAML::const_iterator
			i = doc["ufos"].begin();
			i != doc["ufos"].end();
			++i)
	{
		type = (*i)["type"].as<std::string>();
		if (_rules->getUfo(type) != nullptr)
		{
			Ufo* const ufo (new Ufo(
								_rules->getUfo(type),
								this));
			ufo->loadUfo(*i, *_rules);
			_ufos.push_back(ufo);
		}
		else Log(LOG_ERROR) << "Failed to load UFO: Type [" << type << "]";
	}

	Log(LOG_INFO) << ". load waypoints";
	for (YAML::const_iterator
			i = doc["waypoints"].begin();
			i != doc["waypoints"].end();
			++i)
	{
		Waypoint* const wp (new Waypoint());
		wp->load(*i);
		_waypoints.push_back(wp);
	}

	Log(LOG_INFO) << ". load terror sites";
	for (YAML::const_iterator
			i = doc["terrorSites"].begin();
			i != doc["terrorSites"].end();
			++i)
	{
		type = (*i)["type"].as<std::string>();
		const std::string deployment ((*i)["deployment"].as<std::string>("STR_TERROR_MISSION"));
		if (_rules->getAlienMission(type) && _rules->getDeployment(deployment))
		{
			TerrorSite* const site (new TerrorSite(
											_rules->getAlienMission(type),
											_rules->getDeployment(deployment)));
			site->load(*i);
			_terrorSites.push_back(site);
		}
		else Log(LOG_ERROR) << "Failed to load terror-site: Type [" << type << "] Deployment [" << deployment << "]";
	}

	Log(LOG_INFO) << ". load research generals";
	// note: Discovered Techs should be loaded before Bases (e.g. for PSI evaluation) <-
	// I don't use psi-evaluation.
	for (YAML::const_iterator
			i = doc["research"].begin();
			i != doc["research"].end();
			++i)
	{
		type = (*i)["type"].as<std::string>();
		if (_rules->getResearch(type) != nullptr)
		{
			ResearchGeneral* const resGen (new ResearchGeneral(_rules->getResearch(type)));
			resGen->load(*i);
			_research.push_back(resGen);
		}
		else Log(LOG_ERROR) << "Failed to load research: Type [" << type << "]";
	}

	Log(LOG_INFO) << ". load xcom bases";
	for (YAML::const_iterator
			i = doc["bases"].begin();
			i != doc["bases"].end();
			++i)
	{
		Base* const base (new Base(_rules, this));
		base->loadBase(*i);
		_bases.push_back(base);
	}

	Log(LOG_INFO) << ". load alien strategy";
	_alienStrategy->load(doc["alienStrategy"]);

	Log(LOG_INFO) << ". load dead soldiers";
	for (YAML::const_iterator
			i = doc["deadSoldiers"].begin();
			i != doc["deadSoldiers"].end();
			++i)
	{
		SoldierDead* const solDead (new SoldierDead());
		solDead->load(*i);
		_deadSoldiers.push_back(solDead);
	}

	Log(LOG_INFO) << ". load mission statistics";
	for (YAML::const_iterator
			i = doc["missionStatistics"].begin();
			i != doc["missionStatistics"].end();
			++i)
	{
		MissionStatistics* const tacticalStats (new MissionStatistics());
		tacticalStats->load(*i);
		_missionStatistics.push_back(tacticalStats);
	}

	if (const YAML::Node& battle = doc["battleGame"])
	{
		Log(LOG_INFO) << "SavedGame: loading tactical";
		_battleSave = new SavedBattleGame(
									this,
									nullptr,
									_rules);
//		Ruleset* const rules (const_cast<Ruleset*>(_rules)); // strip const.
		_battleSave->load(battle, rules);
		Log(LOG_INFO) << "SavedGame: loading tactical DONE";
	}
}

/**
 * Saves a SavedGame's contents to a YAML file.
 * @param file - reference to a YAML file
 */
void SavedGame::save(const std::string& file) const
{
	const std::string st (Options::getUserFolder() + file);
	std::ofstream ofstr (st.c_str());
	if (ofstr.fail() == true)
	{
		throw Exception("Failed to save " + file);
	}

	YAML::Emitter emit;

	YAML::Node brief; // the brief-info used for the saves list

	brief["label"]		= Language::wstrToUtf8(_label);
	brief["edition"]	= OPENXCOM_VERSION_GIT;
//	brief["version"]	= OPENXCOM_VERSION_SHORT;

//	std::string git_sha (OPENXCOM_VERSION_GIT);
//	if (git_sha.empty() == false && git_sha[0u] ==  '.')
//		git_sha.erase(0u,1u);
//	brief["build"] = git_sha;

	brief["build"]		= Version::getBuildDate(false);
	brief["savedate"]	= Version::timeStamp();
	brief["time"]		= _time->save();

	const Base* const base (_bases.front());
	brief["base"] = Language::wstrToUtf8(base->getLabel());

	if (_battleSave != nullptr)
	{
		brief["mission"]	= _battleSave->getTacticalType();
		brief["turn"]		= _battleSave->getTurn();
		brief["mode"]		= static_cast<int>(SM_BATTLESCAPE);
	}
	else
		brief["mode"]		= static_cast<int>(SM_GEOSCAPE);

	brief["rulesets"]		= Options::rulesets;

	if (_ironman == true)
		brief["ironman"]	= _ironman;

	emit << brief;
	emit << YAML::BeginDoc; // saves the full game-data to the save

	YAML::Node node;

	node["rng"]			= RNG::getSeed();
	node["difficulty"]	= static_cast<int>(_difficulty);

	if (_end != END_NONE)		node["end"]				= static_cast<int>(_end);
	if (_monthsPassed != -1)	node["monthsPassed"]	= _monthsPassed;
	if (_warned == true)		node["warned"]			= _warned;

	node["graphRegionToggles"]	= _graphRegionToggles;
	node["graphCountryToggles"]	= _graphCountryToggles;
	node["graphFinanceToggles"]	= _graphFinanceToggles;
	node["funds"]				= _funds;
	node["maintenance"]			= _maintenance;
	node["researchScores"]		= _researchScores;
	node["income"]				= _income;
	node["expenditure"]			= _expenditure;
	node["ids"]					= _ids;
//	node["radarLines"]			= _radarLines;
//	node["detail"]				= _detail;

	node["globeLon"]	= serializeDouble(_globeLon);
	node["globeLat"]	= serializeDouble(_globeLat);
	node["globeZoom"]	= static_cast<int>(_globeZoom);


	for (std::vector<Country*>::const_iterator
			i = _countries.begin();
			i != _countries.end();
			++i)
		node["countries"].push_back((*i)->save());

	for (std::vector<Region*>::const_iterator
			i = _regions.begin();
			i != _regions.end();
			++i)
		node["regions"].push_back((*i)->save());

	for (std::vector<Base*>::const_iterator
			i = _bases.begin();
			i != _bases.end();
			++i)
		node["bases"].push_back((*i)->save());

	for (std::vector<Waypoint*>::const_iterator
			i = _waypoints.begin();
			i != _waypoints.end();
			++i)
		node["waypoints"].push_back((*i)->save());

	for (std::vector<TerrorSite*>::const_iterator
			i = _terrorSites.begin();
			i != _terrorSites.end();
			++i)
		node["terrorSites"].push_back((*i)->save());

	for (std::vector<AlienBase*>::const_iterator // AlienBases must be saved before AlienMissions.
			i = _alienBases.begin();
			i != _alienBases.end();
			++i)
		node["alienBases"].push_back((*i)->save());

	for (std::vector<AlienMission*>::const_iterator // AlienMissions must be saved before Ufos but after AlienBases.
			i = _activeMissions.begin();
			i != _activeMissions.end();
			++i)
		node["alienMissions"].push_back((*i)->save());

	for (std::vector<Ufo*>::const_iterator // Ufos must be saved after AlienMissions.
			i = _ufos.begin();
			i != _ufos.end();
			++i)
		node["ufos"].push_back((*i)->save());

	for (std::vector<ResearchGeneral*>::const_iterator
			i = _research.begin();
			i != _research.end();
			++i)
		node["research"].push_back((*i)->save());

	node["alienStrategy"] = _alienStrategy->save();

	for (std::vector<SoldierDead*>::const_iterator
			i = _deadSoldiers.begin();
			i != _deadSoldiers.end();
			++i)
		node["deadSoldiers"].push_back((*i)->save());

	for (std::vector<MissionStatistics*>::const_iterator
			i = _missionStatistics.begin();
			i != _missionStatistics.end();
			++i)
		node["missionStatistics"].push_back((*i)->save());

	if (_battleSave != nullptr)
		node["battleGame"] = _battleSave->save();

	emit << node;
	ofstr << emit.c_str();
	ofstr.close();
}

/**
 * Gets this SavedGame's label shown in save-screens.
 * @return, label-string
 */
std::wstring SavedGame::getLabel() const
{
	return _label;
}

/**
 * Sets this SavedGame's label shown in save-screens.
 * @param label - reference to the label-string
 */
void SavedGame::setLabel(const std::wstring& label)
{
	_label = label;
}

/**
 * Gets this SavedGame's Ruleset.
 * @return, pointer to the Ruleset
 *
const Ruleset* SavedGame::getRules() const
{
	return _rules;
} */

/**
 * Sets this SavedGame's difficulty-level.
 * @param difficulty - difficulty setting (SavedGame.h)
 */
void SavedGame::setDifficulty(DifficultyLevel diff)
{
	_difficulty = diff;
}

/**
 * Gets this SavedGame's difficulty setting.
 * @return, diff-level (SavedGame.h)
 */
DifficultyLevel SavedGame::getDifficulty() const
{
	return _difficulty;
}

/**
 * Gets the SavedGame's difficulty as an integer.
 * @return, diff-level as integer
 */
int SavedGame::getDifficultyInt() const
{
	switch (_difficulty)
	{
		case DIFF_BEGINNER:		return 0;
		case DIFF_EXPERIENCED:	return 1;
		case DIFF_VETERAN:		return 2;
		case DIFF_GENIUS:		return 3;
		default:
		case DIFF_SUPERHUMAN:	return 4;
	}
}

/**
 * Gets this SavedGame's current end-type.
 * @return, end-type (SavedGame.h)
 */
EndType SavedGame::getEnding() const
{
	return _end;
}

/**
 * Sets this SavedGame's current end-type.
 * @param end - end-type (SavedGame.h)
 */
void SavedGame::setEnding(EndType end)
{
	_end = end;
}

/**
 * Checks if this SavedGame is set to ironman-mode.
 * @note Ironman games cannot be manually saved.
 * @return, Tony Stark
 */
bool SavedGame::isIronman() const
{
	return _ironman;
}

/**
 * Sets if this SavedGame is set to ironman-mode.
 * @note Ironman games cannot be manually saved.
 * @param ironman - Tony Stark
 */
void SavedGame::setIronman(bool ironman)
{
	_ironman = ironman;
}

/**
 * Gets the current longitude of the geoscape Globe.
 * @return, longitude
 */
double SavedGame::getGlobeLongitude() const
{
	return _globeLon;
}

/**
 * Sets the current longitude of the geoscape Globe.
 * @param lon - longitude
 */
void SavedGame::setGlobeLongitude(double lon)
{
	_globeLon = lon;
}

/**
 * Gets the current latitude of the geoscape Globe.
 * @return, latitude
 */
double SavedGame::getGlobeLatitude() const
{
	return _globeLat;
}

/**
 * Sets the current latitude of the geoscape Globe.
 * @param lat - latitude
 */
void SavedGame::setGlobeLatitude(double lat)
{
	_globeLat = lat;
}

/**
 * Gets the current zoom-level of the geoscape Globe.
 * @return, zoom level
 */
size_t SavedGame::getGlobeZoom() const
{
	return _globeZoom;
}

/**
 * Sets the current zoom-level of the geoscape Globe.
 * @param zoom - zoom level
 */
void SavedGame::setGlobeZoom(size_t zoom)
{
	_globeZoom = zoom;
}

/**
 * Gets the preDogfight longitude of the geoscape Globe.
 * @return, longitude
 */
double SavedGame::getDfLongitude() const
{
	return _dfLon;
}

/**
 * Sets the preDogfight longitude of the geoscape Globe.
 * @param lon - longitude
 */
void SavedGame::setDfLongitude(double lon)
{
	_dfLon = lon;
}

/**
 * Gets the preDogfight latitude of the geoscape Globe.
 * @return, latitude
 */
double SavedGame::getDfLatitude() const
{
	return _dfLat;
}

/**
 * Sets the preDogfight latitude of the geoscape Globe.
 * @param lat - latitude
 */
void SavedGame::setDfLatitude(double lat)
{
	_dfLat = lat;
}

/**
 * Gets the preDogfight zoom-level of the geoscape Globe.
 * @return, zoom level
 */
size_t SavedGame::getDfZoom() const
{
	return _dfZoom;
}

/**
 * Sets the preDogfight zoom-level of the geoscape Globe.
 * @param zoom - zoom level
 */
void SavedGame::setDfZoom(size_t zoom)
{
	_dfZoom = zoom;
}

/**
 * Resets all monthly accounts and gives the player his/her monthly funds.
 * @note Takes into account all maintenance and profit/expenditure costs. Also
 * stores monthly totals for the GraphsState.
 */
void SavedGame::balanceBudget()
{
	int
		cashIn  (0),
		cashOut (0);

	for (std::vector<Base*>::const_iterator
			i = _bases.begin();
			i != _bases.end();
			++i)
	{
		cashIn  += (*i)->getCashIncome();
		cashOut += (*i)->getCashSpent();

		(*i)->zeroCashIncome();
		(*i)->zeroCashSpent();
	}

	_income.back() = cashIn; // INCOME
	_income.push_back(0);
	if (_income.size() > 12u)
		_income.erase(_income.begin());

	_expenditure.back() = cashOut; // EXPENDITURE
	_expenditure.push_back(0);
	if (_expenditure.size() > 12u)
		_expenditure.erase(_expenditure.begin());


	const int maintenance (getBaseMaintenances());

	_maintenance.back() = maintenance; // MAINTENANCE
	_maintenance.push_back(0);
	if (_maintenance.size() > 12u)
		_maintenance.erase(_maintenance.begin());

	_funds.back() += getCountryFunding() * 1000 - maintenance; // BALANCE
	_funds.push_back(_funds.back());
	if (_funds.size() > 12)
		_funds.erase(_funds.begin());


	_researchScores.push_back(0); // SCORE (doesn't include xCom - aLien activity)
	if (_researchScores.size() > 12u)
		_researchScores.erase(_researchScores.begin());
}

/**
 * Adds up the monthly funding of all the Countries.
 * @return, total funding
 */
int SavedGame::getCountryFunding() const
{
	int total (0);
	for (std::vector<Country*>::const_iterator
			i = _countries.begin();
			i != _countries.end();
			++i)
	{
		total += (*i)->getFunding().back();
	}
	return total;
}

/**
 * Sets the player's funds to a new value.
 * @param funds - new funds
 */
void SavedGame::setFunds(int64_t funds)
{
	_funds.back() = funds;
}

/**
 * Gets the player's current funds.
 * @return, current funds
 */
int64_t SavedGame::getFunds() const
{
	return _funds.back();
}

/**
 * Gets the player's funds for the last 12 months.
 * @return, reference a vector of funds
 */
std::vector<int64_t>& SavedGame::getFundsList()
{
	return _funds;
}

/**
 * Gets the list of monthly maintenance costs.
 * @return, reference a vector of maintenances
 */
std::vector<int64_t>& SavedGame::getMaintenanceList()
{
	return _maintenance;
}

/**
 * Gets the list of monthly income values.
 * @return, reference a vector of incomes
 */
std::vector<int64_t>& SavedGame::getIncomeList()
{
	return _income;
}

/**
 * Gets the list of monthly expenditure values.
 * @return, reference a vector of expenditures
 */
std::vector<int64_t>& SavedGame::getExpenditureList()
{
	return _expenditure;
}

/**
 * Gets the current time of this SavedGame.
 * @return, pointer to the GameTime
 */
GameTime* SavedGame::getTime() const
{
	return _time;
}

/**
 * Sets the current time of this SavedGame.
 * @param time - GameTime
 */
void SavedGame::setTime(GameTime gt)
{
	delete _time;
	_time = new GameTime(gt);
}

/**
 * Gets the high-ID for a specified Target type and then increments the
 * cache to the next ID value.
 * @param objectType - reference to a Target type
 * @return, the high-ID
 */
int SavedGame::getCanonicalId(const std::string& targetType)
{
	std::map<std::string, int>::iterator i (_ids.find(targetType));
	if (i != _ids.end())
		return i->second++;

	_ids[targetType] = 1;
	return _ids[targetType]++;
}

/**
 * Gets a list of all canonical-IDs.
 * @return, reference to a map of strings w/ ints
 */
const std::map<std::string, int>& SavedGame::getTargetIds() const
{
	return _ids;
}

/**
 * Resets the list of unique object-IDs.
 * @param ids - new ID list as a reference to a map of strings & ints
 *
void SavedGame::setCanonicalIds(const std::map<std::string, int>& ids)
{
	_ids = ids;
} */

/**
 * Gets the list of Countries in this SavedGame's world.
 * @return, pointer to a vector of pointers to the countries
 */
std::vector<Country*>* SavedGame::getCountries()
{
	return &_countries;
}

/**
 * Gets the list of world Regions.
 * @return, pointer to a vector of pointers to the regions
 */
std::vector<Region*>* SavedGame::getRegions()
{
	return &_regions;
}

/**
 * Gets the list of player's current Bases.
 * @return, pointer to a vector of pointers to all player-bases
 */
std::vector<Base*>* SavedGame::getBases()
{
	return &_bases;
}

/**
 * Gets an read-only list of player-bases.
 * @return, pointer to a vector of pointers to all player-bases
 */
const std::vector<Base*>* SavedGame::getBases() const
{
	return &_bases;
}

/**
 * Adds up the monthly maintenance of all the Bases.
 * @return, total maintenance
 */
int SavedGame::getBaseMaintenances() const
{
	int total (0);
	for (std::vector<Base*>::const_iterator
			i = _bases.begin();
			i != _bases.end();
			++i)
	{
		total += (*i)->getMonthlyMaintenace();
	}
	return total;
}

/**
 * Gets the list of current aLien UFOs.
 * @return, pointer to a vector of pointers to all ufos
 */
std::vector<Ufo*>* SavedGame::getUfos()
{
	return &_ufos;
}

/**
 * Gets the list of current Craft waypoints.
 * @return, pointer to a vector of pointers to all Waypoints
 */
std::vector<Waypoint*>* SavedGame::getWaypoints()
{
	return &_waypoints;
}

/**
 * Gets the list of current TerrorSites.
 * @return, pointer to a vector of pointers to all mission-sites
 */
std::vector<TerrorSite*>* SavedGame::getTerrorSites()
{
	return &_terrorSites;
}

/**
 * Set SavedBattleGame object.
 * @param battleSave - pointer to a SavedBattleGame (default nullptr)
 */
void SavedGame::setBattleSave(SavedBattleGame* const battleSave)
{
	if (_battleSave != nullptr)
		delete _battleSave;

	_battleSave = battleSave;
}

/**
 * Get pointer to the SavedBattleGame object.
 * @return, pointer to the SavedBattleGame
 */
SavedBattleGame* SavedGame::getBattleSave()
{
	return _battleSave;
}

/**
 * Gets the ResearchGenerals.
 * @return, reference to a vector of pointers to the research-generals
 */
std::vector<ResearchGeneral*>& SavedGame::getResearchGenerals()
{
	return _research;
}

/**
 * Searches through ResearchGenerals for specified research-type & status.
 * @param type		- reference to a research-type
 * @param status	- ResearchStatus (default RS_COMPLETED) (ResearchGeneral.h)
 * @return, true if found
 */
bool SavedGame::searchResearch(
		const std::string& type,
		const ResearchStatus status) const
{
	for (std::vector<ResearchGeneral*>::const_iterator
			i = _research.begin();
			i != _research.end();
			++i)
	{
		if ((*i)->getType() == type && (*i)->getStatus() == status)
			return true;
	}
	return false;
}

/**
 * Searches through ResearchGenerals for specified research-rule & status.
 * @param resRule	- pointer to a RuleResearch
 * @param status	- ResearchStatus (default RS_COMPLETED) (ResearchGeneral.h)
 * @return, true if found
 */
bool SavedGame::searchResearch(
		const RuleResearch* resRule,
		const ResearchStatus status) const
{
	for (std::vector<ResearchGeneral*>::const_iterator
			i = _research.begin();
			i != _research.end();
			++i)
	{
		if ((*i)->getRules() == resRule && (*i)->getStatus() == status)
			return true;
	}
	return false;
}

/**
 * Sets the status of a ResearchGeneral by research-type.
 * @param type		- reference to a research-type
 * @param status	- ResearchStatus (default RS_COMPLETED) (ResearchGeneral.h)
 * @return, true if status changed
 */
bool SavedGame::setResearchStatus(
		const std::string& type,
		const ResearchStatus status)
{
	bool ret (false);
	for (std::vector<ResearchGeneral*>::const_iterator
			i = _research.begin();
			i != _research.end();
			++i)
	{
		if ((*i)->getType() == type)
		{
			if ((*i)->getStatus() != status)
				ret = true;

			(*i)->setStatus(status);
			if (status == RS_COMPLETED)
			{
				const std::string uPed ((*i)->getRules()->getUfopaediaEntry());
				if (uPed != type)
					setResearchStatus(_rules->getResearch(uPed));
			}
			return ret;
		}
	}
	return false;
}

/**
 * Sets the status of a ResearchGeneral by research-rule.
 * @param resRule	- pointer to a RuleResearch
 * @param status	- ResearchStatus (default RS_COMPLETED) (ResearchGeneral.h)
 * @return, true if status changed
 */
bool SavedGame::setResearchStatus(
		const RuleResearch* resRule,
		const ResearchStatus status)
{
	bool ret (false);
	for (std::vector<ResearchGeneral*>::const_iterator
			i = _research.begin();
			i != _research.end();
			++i)
	{
		if ((*i)->getRules() == resRule)
		{
			if ((*i)->getStatus() != status)
				ret = true;

			(*i)->setStatus(status);
			if (status == RS_COMPLETED)
			{
				const std::string uPed (resRule->getUfopaediaEntry());
				if (uPed != resRule->getType())
					setResearchStatus(_rules->getResearch(uPed));
			}
			return ret;
		}
	}
	return false;
}

/**
 * Adds a RuleResearch to the list of already discovered RuleResearch's.
 * @param resRule - pointer to the newly found RuleResearch
 */
void SavedGame::addFinishedResearch(const RuleResearch* const resRule)
{
	if (setResearchStatus(resRule) == true)
	{
		_researchScores.back() += resRule->getPoints();

		std::vector<const RuleResearch*> dependents;
		for (std::vector<Base*>::const_iterator
				i = _bases.begin();
				i != _bases.end();
				++i)
		{
			getDependentResearchBasic(dependents, resRule, *i); // fills 'dependents' vector
		}

		const std::vector<std::string>* required;
		for (std::vector<const RuleResearch*>::const_iterator
				i = dependents.begin();
				i != dependents.end();
				++i)
		{
			if ((*i)->getCost() == 0) // fake project.
			{
				required = &((*i)->getRequiredResearch()); // gawd i hate c++
				if (required->empty() == true)
					addFinishedResearch(*i);
				else
				{
					size_t id (0);
					for (std::vector<std::string>::const_iterator
							j = required->begin();
							j != required->end();
							++j, ++id)
					{
						if (*j == required->at(id)) // wtf this.
							addFinishedResearch(*i);
					}
				}
			}
		}
	}
}

/**
 * Gets a list of RuleResearch's that can be started at a particular Base.
 * @note Used in Basescape's 'start project' as well as (indirectly) in
 * Geoscape's 'we can now research'.
 * @param availableProjects	- reference to a vector of pointers to RuleResearch in
 *							  which to put the projects that are currently available
 * @param base				- pointer to a Base
 */
void SavedGame::getAvailableResearchProjects(
		std::vector<const RuleResearch*>& availableProjects,
		Base* const base) const
{
	const RuleResearch* resRule;
	bool cullProject;

	const std::vector<std::string> allResearch (_rules->getResearchList());
	for (std::vector<std::string>::const_iterator
			i = allResearch.begin();
			i != allResearch.end();
			++i)
	{
		resRule = _rules->getResearch(*i);

		if (_debugGeo == true)
		{
			availableProjects.push_back(resRule);
			continue;
		}

		if (std::find_if(
					base->getResearch().begin(),
					base->getResearch().end(),
					findRuleResearch(resRule)) != base->getResearch().end())
		{
			continue;
		}

		if (resRule->needsItem() == true
			&& base->getStorageItems()->getItemQuantity(resRule->getType()) == 0)
		{
			continue;
		}

		if (hasRequiredResearch(resRule) == false)
			continue;

		if (_rules->getUnitRule(resRule->getType()) != nullptr)
		{
			availableProjects.push_back(resRule);
			continue;
		}

		if (isProjectAvailable(resRule) == false)
			continue;

		if (searchResearch(resRule) == true) // if resRule is completed
		{
			cullProject = true;

			for (std::vector<std::string>::const_iterator
					j = resRule->getGetOneFree().begin();
					j != resRule->getGetOneFree().end();
					++j)
			{
				if (searchResearch(_rules->getResearch(*j)) == false) // resRule's getOneFree not completed yet.
				{
					cullProject = false;
					break;
				}
			}

			if (cullProject == true)
			{
				for (std::vector<std::string>::const_iterator
						j = resRule->getForcedResearch().begin();
						j != resRule->getForcedResearch().end();
						++j)
				{
					if (searchResearch(
								_rules->getResearch(*j),
								RS_HIDDEN) == true) // resRule's forced-research still hidden.
					{
						cullProject = false;
						break;
					}
				}
			}

			if (cullProject == true) continue;
		}

		availableProjects.push_back(resRule);
	}
}

/**
 * Checks whether a RuleResearch can be started as a project.
 * @note If it's forced it is available. If it's not forced and has a getOneFree
 * it's also considered available. But if it's not forced and does not have a
 * getOneFree and its prerequisites are as yet undiscovered it is not available.
 * @param resRule - pointer to a RuleResearch
 * @return, true if available
 */
bool SavedGame::isProjectAvailable(const RuleResearch* const resRule) const // private.
{
	if (resRule != nullptr)
	{
		std::vector<const RuleResearch*> forcedList;
		tabulateForced(forcedList);

		if (std::find(
				forcedList.begin(),
				forcedList.end(),
				resRule) != forcedList.end())
		{
			return true;
		}

		const RuleResearch* gofRule;
		std::vector<std::string> gofTypes (resRule->getGetOneFree());
		for (std::vector<std::string>::const_iterator
				i = gofTypes.begin();
				i != gofTypes.end();
				++i)
		{
			gofRule = _rules->getResearch(*i);
			if (std::find(
					forcedList.begin(),
					forcedList.end(),
					gofRule) == forcedList.end())
			{
				return true;
			}
		}

		if (checkPrerequisites(resRule) == false)
			return false;

		return true;
	}

	return false;
}

/**
 * Checks whether a RuleResearch has had all its prerequisites met.
 * @param resRule - pointer to RuleResearch
 * @return, true if good to go
 */
bool SavedGame::checkPrerequisites(const RuleResearch* const resRule) const // private.
{
	for (std::vector<std::string>::const_iterator
			i = resRule->getPrerequisites().begin();
			i != resRule->getPrerequisites().end();
			++i)
	{
		for (std::vector<ResearchGeneral*>::const_iterator
				j = _research.begin();
				j != _research.end();
				++j)
		{
			if ((*j)->getStatus() != RS_COMPLETED
				&& (*j)->getType() == *i)
			{
				return false;
			}
		}
	}
	return true;
}

/**
 * Checks whether or not required research has been met.
 * @param resRule - pointer to RuleResearch
 * @return, true if good to go
 */
bool SavedGame::hasRequiredResearch(const RuleResearch* const resRule) const // private.
{
	for (std::vector<std::string>::const_iterator
			i = resRule->getRequiredResearch().begin();
			i != resRule->getRequiredResearch().end();
			++i)
	{
		for (std::vector<ResearchGeneral*>::const_iterator
				j = _research.begin();
				j != _research.end();
				++j)
		{
			if ((*j)->getStatus() != RS_COMPLETED
				&& (*j)->getType() == *i)
			{
				return false;
			}
		}
	}
	return true;
}

/**
 * Fills a vector with the forced-types of completed ResearchProjects.
 * @param forced - reference to a vector to fill with pointers to RuleResearch
 */
void SavedGame::tabulateForced(std::vector<const RuleResearch*>& forced) const // private.
{
	for (std::vector<ResearchGeneral*>::const_iterator
			i = _research.begin();
			i != _research.end();
			++i)
	{
		if ((*i)->getStatus() == RS_COMPLETED)
		{
			for (std::vector<std::string>::const_iterator
					j = (*i)->getRules()->getForcedResearch().begin();
					j != (*i)->getRules()->getForcedResearch().end();
					++j)
			{
				forced.push_back(_rules->getResearch(*j)); // load up forced-research from completed-research.
			}
		}
	}
}

/**
 * Assigns a list of RuleManufacture's that can be manufactured at a particular
 * Base.
 * @param availableProductions	- reference to a vector of pointers to
 * RuleManufacture in which to put productions that are currently available
 * @param base					- pointer to a Base
 */
void SavedGame::getAvailableProductions(
		std::vector<const RuleManufacture*>& availableProductions,
		const Base* const base) const
{
	const RuleManufacture* manfRule;
	const std::vector<Production*> baseProductions (base->getProductions());
	const std::vector<std::string> manufList (_rules->getManufactureList());
	for (std::vector<std::string>::const_iterator
			i = manufList.begin();
			i != manufList.end();
			++i)
	{
		manfRule = _rules->getManufacture(*i);
		if (isResearched(manfRule->getRequirements()) == true
			&& std::find_if(
					baseProductions.begin(),
					baseProductions.end(),
					equalProduction(manfRule)) == baseProductions.end())
		{
			availableProductions.push_back(manfRule);
		}
	}
}

/**
 * Tabulates a list of newly available ResearchProjects that appear when a
 * ResearchProject is completed.
 * @note This function checks for completed no-cost research and adds its
 * prerequisites. wtf.
 * @note Called from GeoscapeState::time1Day() for NewPossibleResearchInfo screen.
 * @param nowAvailable	- reference to a vector of pointers to the RuleResearch's
 *						  that are now available
 * @param resRule		- pointer to the RuleResearch that has just been discovered
 * @param base			- pointer to a Base
 */
void SavedGame::getPopupResearch(
		std::vector<const RuleResearch*>& nowAvailable,
		const RuleResearch* const resRule,
		Base* const base) const
{
	getDependentResearchBasic(nowAvailable, resRule, base);

	for (std::vector<ResearchGeneral*>::const_iterator
			i = _research.begin();
			i != _research.end();
			++i)
	{
		if ((*i)->getStatus() == RS_COMPLETED
			&& (*i)->getRules()->getCost() == 0
			&& std::find(
					(*i)->getRules()->getPrerequisites().begin(),
					(*i)->getRules()->getPrerequisites().end(),
					resRule->getType()) != (*i)->getRules()->getPrerequisites().end())
		{
			getDependentResearchBasic(nowAvailable, (*i)->getRules(), base);
		}
	}
}

/**
 * Tabulats a list of newly available ResearchProjects that appear when a
 * ResearchProject is completed.
 * @note This function does not check for no-cost research.
 * @note Called from addFinishedResearch(), getPopupResearch(), and itself.
 * @param nowAvailable	- reference to a vector of pointers to the RuleResearch's
 *						  that are now available
 * @param resRule		- pointer to the RuleResearch that has just been discovered
 * @param base			- pointer to a Base
 */
void SavedGame::getDependentResearchBasic( // private.
		std::vector<const RuleResearch*>& nowAvailable,
		const RuleResearch* const resRule,
		Base* const base) const
{
	std::vector<const RuleResearch*> availableProjects;
	getAvailableResearchProjects(availableProjects, base); // <---|			should be: any research that

	for (std::vector<const RuleResearch*>::const_iterator					//	(1) has all its prerequisites discovered
			i = availableProjects.begin();									//	(2) or has been forced,
			i != availableProjects.end();									//	(3) and has all its requirements discovered -
			++i)															//	(4) but has *not* been discovered itself.
	{
		if (std::find(
					(*i)->getPrerequisites().begin(),
					(*i)->getPrerequisites().end(),
					resRule->getType()) != (*i)->getPrerequisites().end()	// if an availableProject has resRule as a prerequisite
			|| std::find(
					(*i)->getForcedResearch().begin(),
					(*i)->getForcedResearch().end(),
					resRule->getType()) != (*i)->getForcedResearch().end())	// or an availableProject forces resRule
		{
			nowAvailable.push_back(*i);										// push_back the availableProject as a prerequisite

			if ((*i)->getCost() == 0)										// and if that's a fake project -> repeat
				getDependentResearchBasic(nowAvailable, *i, base);
		}
	}
}

/**
 * Assigns a list of newly available RuleManufacture's that appear when a
 * research is completed.
 * @note This function checks for fake research.
 * @param dependents	- reference to a vector of pointers to the RuleManufacture's
 *						  that are now available
 * @param resRule		- pointer to the RuleResearch that has just been discovered
 */
void SavedGame::getPopupManufacture(
		std::vector<const RuleManufacture*>& dependents,
		const RuleResearch* const resRule) const
{
	const RuleManufacture* manfRule;
	const std::vector<std::string>& manfList (_rules->getManufactureList());
	for (std::vector<std::string>::const_iterator
			i = manfList.begin();
			i != manfList.end();
			++i)
	{
		manfRule = _rules->getManufacture(*i);
		const std::vector<std::string>& reqs (manfRule->getRequirements());
		if (isResearched(manfRule->getRequirements()) == true
			&& std::find(
					reqs.begin(),
					reqs.end(),
					resRule->getType()) != reqs.end())
		{
			dependents.push_back(manfRule);
		}
	}
}

/**
 * Checks if a RuleResearch is discovered.
 * @param resType - reference to a research-type
 * @return, true if type has been researched
 */
bool SavedGame::isResearched(const std::string& resType) const
{
	if (_debugGeo == true || resType.empty() == true
		|| _rules->getResearch(resType) == nullptr
		|| (_rules->getItemRule(resType) != nullptr
			&& _rules->getItemRule(resType)->isResearchExempt() == true))
	{
		return true;
	}
	return searchResearch(resType);
}

/**
 * Checks if a list of RuleResearch is discovered.
 * @param resTypes - reference to a vector of strings of research-types
 * @return, true if all types have been researched
 */
bool SavedGame::isResearched(const std::vector<std::string>& resTypes) const
{
	if (_debugGeo == false && resTypes.empty() == false)
	{
		for (std::vector<std::string>::const_iterator
				j = resTypes.begin();
				j != resTypes.end();
				++j)
		{
			if (isResearched(*j) == false)
				return false;
		}
	}
	return true;
}

/**
 * Gets the Soldier matching an ID.
 * @note Used to instance BattleUnits from Soldiers when re-loading tactical.
 * @param id - a soldier's unique id
 * @return, pointer to the Soldier
 */
Soldier* SavedGame::getSoldier(int id) const
{
	for (std::vector<Base*>::const_iterator
			i = _bases.begin();
			i != _bases.end();
			++i)
	{
		for (std::vector<Soldier*>::const_iterator
				j = (*i)->getSoldiers()->begin();
				j != (*i)->getSoldiers()->end();
				++j)
		{
			if ((*j)->getId() == id)
				return *j;
		}
	}
	return nullptr;
}

/**
 * Handles the higher promotions - not the rookie-squaddie ones.
 * @param participants - reference to a list of pointers to Soldiers that were
 *						 actually present at the battle
 * @return, true if a Ceremony is to happen
 */
bool SavedGame::handlePromotions(std::vector<Soldier*>& participants)
{
	PromotionInfo data;
	std::vector<Soldier*> soldiers;

	for (std::vector<Base*>::const_iterator
			i = _bases.begin();
			i != _bases.end();
			++i)
	{
		for (std::vector<Soldier*>::const_iterator
				j = (*i)->getSoldiers()->begin();
				j != (*i)->getSoldiers()->end();
				++j)
		{
			soldiers.push_back(*j);
			processSoldier(*j, data);
		}

		for (std::vector<Transfer*>::const_iterator
				j = (*i)->getTransfers()->begin();
				j != (*i)->getTransfers()->end();
				++j)
		{
			if ((*j)->getTransferType() == PST_SOLDIER)
			{
				soldiers.push_back((*j)->getSoldier());
				processSoldier((*j)->getSoldier(), data);
			}
		}
	}


	int pro (0);

	Soldier* fragBait (nullptr);
	const int totalSoldiers (static_cast<int>(soldiers.size()));

	if (data.totalCommanders == 0 // There can be only one.
		&& totalSoldiers > 29)
	{
		fragBait = inspectSoldiers(soldiers, participants, RANK_COLONEL);
		if (fragBait != nullptr)
		{
			fragBait->promoteRank();
			++pro;
			++data.totalCommanders;
			--data.totalColonels;
		}
	}

	while (data.totalColonels < totalSoldiers / 23)
	{
		fragBait = inspectSoldiers(soldiers, participants, RANK_CAPTAIN);
		if (fragBait == nullptr)
			break;

		fragBait->promoteRank();
		++pro;
		++data.totalColonels;
		--data.totalCaptains;
	}

	while (data.totalCaptains < totalSoldiers / 11)
	{
		fragBait = inspectSoldiers(soldiers, participants, RANK_SERGEANT);
		if (fragBait == nullptr)
			break;

		fragBait->promoteRank();
		++pro;
		++data.totalCaptains;
		--data.totalSergeants;
	}

	while (data.totalSergeants < totalSoldiers / 5)
	{
		fragBait = inspectSoldiers(soldiers, participants, RANK_SQUADDIE);
		if (fragBait == nullptr)
			break;

		fragBait->promoteRank();
		++pro;
		++data.totalSergeants;
	}

	return (pro != 0);
}

/**
 * Processes a soldier and adds their rank to the promotions data struct.
 * @param soldier	- pointer to the Soldier to process
 * @param promoData	- reference to the PromotionInfo data struct to put the info into
 */
void SavedGame::processSoldier(
		const Soldier* const soldier,
		PromotionInfo& promoData)
{
	switch (soldier->getRank())
	{
		case RANK_COMMANDER:
			++promoData.totalCommanders;
			break;

		case RANK_COLONEL:
			++promoData.totalColonels;
			break;

		case RANK_CAPTAIN:
			++promoData.totalCaptains;
			break;

		case RANK_SERGEANT:
			++promoData.totalSergeants;
	}
}

/**
 * Checks how many soldiers of a rank exist and which one has the highest score.
 * @param soldiers		- reference a vector of pointers to Soldiers for full list of live soldiers
 * @param participants	- reference a vector of pointers to Soldiers for list of participants on a mission
 * @param soldierRank	- rank to inspect
 * @return, pointer to the highest ranked soldier
 */
Soldier* SavedGame::inspectSoldiers(
		const std::vector<Soldier*>& soldiers,
		const std::vector<Soldier*>& participants,
		SoldierRank soldierRank)
{
	Soldier* fragBait (nullptr);
	int
		score (0),
		scoreTest;

	for (std::vector<Soldier*>::const_iterator
			i = soldiers.begin();
			i != soldiers.end();
			++i)
	{
		if ((*i)->getRank() == soldierRank)
		{
			scoreTest = getSoldierScore(*i);
			if (scoreTest > score
				&& (Options::fieldPromotions == false
					|| std::find(
							participants.begin(),
							participants.end(),
							*i) != participants.end()))
			{
				score = scoreTest;
				fragBait = *i;
			}
		}
	}
	return fragBait;
}

/**
 * Evaluate the score of a Soldier based on all of his/her stats, missions and
 * kills.
 * @param soldier - pointer to the soldier to get a score for
 * @return, the score
 */
int SavedGame::getSoldierScore(Soldier* const soldier)
{
	const UnitStats* const stats (soldier->getCurrentStats());
	const int score (  stats->health * 2
					+  stats->stamina * 2
					+  stats->reactions * 4
					+  stats->bravery * 4
					+ (stats->tu + stats->firing * 2) * 3
					+  stats->melee
					+  stats->throwing
					+  stats->strength
					+  stats->psiStrength * 4	// include psi even if not revealed.
					+  stats->psiSkill * 2);	// include psi even if not revealed.

//	if (stats->psiSkill > 0)
//		score += stats->psiStrength + stats->psiSkill * 2;

	return ((score + (soldier->getMissions() + soldier->getKills())) << 3u);
}

/**
 * Gets a list of the AlienBases.
 * @return, pointer to a vector of pointers to AlienBases
 */
std::vector<AlienBase*>* SavedGame::getAlienBases()
{
	return &_alienBases;
}

/**
 * Toggles and returns the Geoscape debug-flag.
 * @return, true if debug active
 */
bool SavedGame::toggleDebugActive()
{
	return (_debugGeo = !_debugGeo);
}

/**
 * Gets the current state of the Geoscape debug-flag.
 * @return, true if debug active
 */
bool SavedGame::getDebugGeo() const
{
	return _debugGeo;
}


/**
 * *** FUNCTOR ***
 * @brief Match a mission based on Region and objective-type.
 * This function object will match AlienMissions based on Region and objective-type.
 */
class matchRegionAndType
	:
		public std::unary_function<AlienMission*, bool>
{
private:
	const std::string& _region;
	MissionObjective _objective;

	public:
		/// Store the region and type.
		matchRegionAndType(
				const std::string& region,
				MissionObjective objective)
			:
				_region(region),
				_objective(objective)
		{}

		/// Match against stored values.
		bool operator() (const AlienMission* const mission) const
		{
			return mission->getRegion() == _region
				&& mission->getRules().getObjectiveType() == _objective;
		}
};

/**
 * Find a mission type in the active alien missions.
 * @param region	- the region's string ID
 * @param objective	- the active mission's objective
 * @return, pointer to the AlienMission or nullptr
 */
AlienMission* SavedGame::findAlienMission(
		const std::string& region,
		MissionObjective objective) const
{
	std::vector<AlienMission*>::const_iterator i (std::find_if(
															_activeMissions.begin(),
															_activeMissions.end(),
															matchRegionAndType(region, objective)));
	if (i != _activeMissions.end())
		return *i;

	return nullptr;
}

/**
 * Gets the list of research scores
 * @return, list of research scores
 */
std::vector<int>& SavedGame::getResearchScores()
{
	return _researchScores;
}

/**
 * Gets if the player has been warned about poor performance.
 * @return, true if warned
 */
bool SavedGame::getWarned() const
{
	return _warned;
}

/**
 * Sets the player's warned status.
 * @param warned - true if warned (default true)
 */
void SavedGame::setWarned(bool warned)
{
	_warned = warned;
}

/**
 * *** FUNCTOR ***
 * Checks if a point is contained in a region.
 * @note This function object checks if a point is contained inside a region.
 */
class ContainsPoint
	:
		public std::unary_function<const Region*, bool>
{
private:
	double
		_lon,_lat;

	public:
		/// Remember the coordinates.
		ContainsPoint(
				double lon,
				double lat)
			:
				_lon(lon),
				_lat(lat)
		{}

		/// Check if the region contains the stored point.
		bool operator() (const Region* const region) const
		{
			return region->getRules()->insideRegion(_lon,_lat);
		}
};

/**
 * Finds the Region containing the specified coordinates.
 * @param lon - the longtitude
 * @param lat - the latitude
 * @return, pointer to the region or nullptr
 */
Region* SavedGame::locateRegion(
		double lon,
		double lat) const
{
	const std::vector<Region*>::const_iterator i (std::find_if(
															_regions.begin(),
															_regions.end(),
															ContainsPoint(lon,lat)));
	if (i != _regions.end())
		return *i;

	return nullptr;
}

/**
 * Finds the Region containing a specified Target.
 * @param target - the target to locate
 * @return, pointer to the region or nullptr
 */
Region* SavedGame::locateRegion(const Target& target) const
{
	return locateRegion(
					target.getLongitude(),
					target.getLatitude());
}

/**
 * Gets the month-count.
 * @return, the month-count
 */
int SavedGame::getMonthsElapsed() const
{
	return _monthsPassed;
}

/**
 * Increments the month-count.
 */
void SavedGame::addMonth()
{
	++_monthsPassed;
}

/**
 * Gets the GraphRegionToggles.
 * @return, reference to the GraphRegionToggles
 */
const std::string& SavedGame::getGraphRegionToggles() const
{
	return _graphRegionToggles;
}

/**
 * Gets the GraphCountryToggles.
 * @return, reference to the GraphCountryToggles
 */
const std::string& SavedGame::getGraphCountryToggles() const
{
	return _graphCountryToggles;
}

/**
 * Gets the GraphFinanceToggles.
 * @return, reference to the GraphFinanceToggles
 */
const std::string& SavedGame::getGraphFinanceToggles() const
{
	return _graphFinanceToggles;
}

/**
 * Sets the GraphRegionToggles.
 * @param value - reference to the new value for GraphRegionToggles
 */
void SavedGame::setGraphRegionToggles(const std::string& value)
{
	_graphRegionToggles = value;
}

/**
 * Sets the GraphCountryToggles.
 * @param value - reference to the new value for GraphCountryToggles
 */
void SavedGame::setGraphCountryToggles(const std::string& value)
{
	_graphCountryToggles = value;
}

/**
 * Sets the GraphFinanceToggles.
 * @param value - reference to the new value for GraphFinanceToggles
 */
void SavedGame::setGraphFinanceToggles(const std::string& value)
{
	_graphFinanceToggles = value;
}

/**
 * Toggles the state of the radar-line drawing.
 *
void SavedGame::toggleRadarLines()
{
	_radarLines = !_radarLines;
} */
/**
 * Gets the state of the radar-line drawing.
 * @return, the state of the radar-line drawing.
 *
bool SavedGame::getRadarLines()
{
	return _radarLines;
} */
/**
 * Toggles the state of the detail drawing.
 *
void SavedGame::toggleDetail()
{
	_detail = !_detail;
} */
/**
 * Gets the state of the detail drawing.
 * @return, the state of the detail drawing.
 *
bool SavedGame::getDetail()
{
	return _detail;
} */

/**
 * Gets the list of dead Soldiers.
 * @return, pointer to a vector of pointers to SoldierDead.
 */
std::vector<SoldierDead*>* SavedGame::getDeadSoldiers()
{
	return &_deadSoldiers;
}

/**
 * Gets the last selected player Base.
 * @return, pointer to base
 *
Base* SavedGame::getRecallBase()
{
	// in case a base was destroyed or something...
	if (_selectedBase < _bases.size())
		return _bases.at(_selectedBase);
	else
		return _bases.front();
} */
/**
 * Sets the last selected player Base.
 * @param base - # of the base
 *
void SavedGame::setRecallBase(size_t base)
{
	_selectedBase = base;
} */
/**
 * Sets the last selected armor.
 * @param value - the new value for last selected armor - armor-type string
 *
void SavedGame::setRecallArmor(const std::string& value)
{
	_lastselectedArmor = value;
} */
/**
 * Gets the last selected armor.
 * @return, last used armor-type string
 *
std::string SavedGame::getRecallArmor()
{
	return _lastselectedArmor;
} */

/**
 * Sets a debug-argument from Globe for GeoscapeState.
 * @param arg - debug string
 */
void SavedGame::setDebugArg(const std::string& debug)
{
	_debugArgDone = true;
	_debugArg = debug;
}

/**
 * Gets a debug-argument from Globe for GeoscapeState.
 * @return, debug string
 */
std::string SavedGame::getDebugArg() const
{
	return _debugArg;
}

/**
 * Gets if the debug-argument has just been set - and resets flag if true.
 * @return, true if debug-arg is ready for display
 */
bool SavedGame::getDebugArgDone()
{
	const bool ret (_debugArgDone);

	if (_debugArgDone == true)
		_debugArgDone = false;

	return ret;
}

/**
 * Gets mission-statistics for use by SoldierDiary.
 * @return, pointer to a vector of pointers to MissionStatistics
 */
std::vector<MissionStatistics*>* SavedGame::getMissionStatistics()
{
	return &_missionStatistics;
}

/**
 * Scores points for XCOM or aLiens at specified coordinates.
 * @param lon	- longitude
 * @param lat	- latitude
 * @param pts	- points to award
 * @param aLien	- true if aLienPts; false if XCOM points
 */
void SavedGame::scorePoints(
		double lon,
		double lat,
		int pts,
		bool aLien) const
{
	for (std::vector<Region*>::const_iterator
			i = _regions.begin();
			i != _regions.end();
			++i)
	{
		if ((*i)->getRules()->insideRegion(lon,lat) == true)
		{
			if (aLien == true)
			{
				(*i)->addActivityAlien(pts);
				(*i)->recentActivityAlien();
			}
			else // XCom
			{
				(*i)->addActivityXCom(pts);
				(*i)->recentActivityXCom();
			}
			break;
		}
	}

	for (std::vector<Country*>::const_iterator
			i = _countries.begin();
			i != _countries.end();
			++i)
	{
		if ((*i)->getRules()->insideCountry(lon,lat) == true)
		{
			if (aLien == true)
			{
				(*i)->addActivityAlien(pts);
				(*i)->recentActivityAlien();
			}
			else // XCom
			{
				(*i)->addActivityXCom(pts);
				(*i)->recentActivityXCom();
			}
			break;
		}
	}
}

/**
 * Scores points for XCOM or aLiens in specified Region/Country.
 * @param region	- pointer to Region
 * @param country	- pointer to Country
 * @param pts		- points to award
 * @param aLien		- true if aLienPts; false if XCOM points
 */
void SavedGame::scorePoints(
		Region* const region,
		Country* const country,
		int pts,
		bool aLien) const
{
	if (region != nullptr)
	{
		if (aLien == true)
		{
			region->addActivityAlien(pts);
			region->recentActivityAlien();
		}
		else // XCom
		{
			region->addActivityXCom(pts);
			region->recentActivityXCom();
		}
	}

	if (country != nullptr)
	{
		if (aLien == true)
		{
			country->addActivityAlien(pts);
			country->recentActivityAlien();
		}
		else // XCom
		{
			country->addActivityXCom(pts);
			country->recentActivityXCom();
		}
	}
}

/**
 * Formats hours into days/hours for Craft refurbishing.
 * @param hrsTotal	- total hours refurbishing
 * @param isDelayed	- true if Craft will be delayed due to lack of materials
 * @param lang		- pointer to a Language
 */
std::wstring SavedGame::formatCraftDowntime(
		int hrsTotal,
		bool isDelayed,
		const Language* const lang) const
{
	std::wostringstream woststr;
	woststr << L"(";

	const int
		dys (hrsTotal / 24),
		hrs (hrsTotal % 24);

	if (dys != 0)
	{
		woststr << lang->getString("STR_DAY", static_cast<unsigned>(dys));
		if (hrs != 0)
			woststr << L" ";
	}

	if (hrs != 0)
		woststr << lang->getString("STR_HOUR", static_cast<unsigned>(hrs));

	if (isDelayed == true)
		woststr << L" +";

	woststr << L")";
	return woststr.str();
}

/**
 * Gets the craft corresponding to the specified ID.
 * @param craftId - the unique craft id to look up
 * @return, the craft with the specified id, or nullptr
 *
Craft* SavedGame::findCraftByUniqueId(const CraftId& craftId) const
{
	for (std::vector<Base*>::const_iterator i = _bases.begin(); i != _bases.end(); ++i)
		for (std::vector<Craft*>::const_iterator j = (*i)->getCrafts()->begin(); j != (*i)->getCrafts()->end(); ++j)
			if ((*j)->getUniqueId() == craftId) return *j;
	return nullptr;
} */

}
