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

#include "Ruleset.h"

//#include <algorithm>
//#include <fstream>

#include "AlienRace.h"
#include "AlienDeployment.h"
#include "ArticleDefinition.h"
//#include "ExtraMusic.h" // sza_ExtraMusic
#include "ExtraSounds.h"
#include "ExtraSprites.h"
#include "ExtraStrings.h"
#include "MapDataSet.h"
#include "MapScript.h"
#include "MCDPatch.h"
#include "OperationPool.h"
#include "RuleAlienMission.h"
#include "RuleArmor.h"
#include "RuleAward.h"
#include "RuleBaseFacility.h"
#include "RuleCountry.h"
//#include "RuleCraft.h"
#include "RuleCraftWeapon.h"
#include "RuleGlobe.h"
#include "RuleInterface.h"
#include "RuleItem.h"
#include "RuleManufacture.h"
#include "RuleMissionScript.h"
#include "RuleMusic.h"
#include "RuleRegion.h"
#include "RuleResearch.h"
#include "RuleSoldier.h"
#include "RuleTerrain.h"
#include "RuleUfo.h"
#include "RuleVideo.h"
#include "SoldierNamePool.h"
//#include "SoundDefinition.h"
//#include "StatString.h"
#include "UfoTrajectory.h"
#include "RuleUnit.h"

#include "../Engine/CrossPlatform.h"
#include "../Engine/Exception.h"
#include "../Engine/Logger.h"
//#include "../Engine/Options.h"
#include "../Engine/RNG.h"

#include "../Resource/ResourcePack.h"

#include "../Savegame/AlienStrategy.h"
#include "../Savegame/Base.h"
#include "../Savegame/Country.h"
#include "../Savegame/Craft.h"
#include "../Savegame/Transfer.h"
#include "../Savegame/GameTime.h"
#include "../Savegame/Region.h"
#include "../Savegame/ResearchGeneral.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/Soldier.h"
#include "../Savegame/SoldierDiary.h"

#include "../Ufopaedia/Ufopaedia.h"


namespace OpenXcom
{

/**
 * Creates a ruleset with blank sets of rules.
 * @param game - pointer to the core Game
 */
Ruleset::Ruleset(const Game* const game)
	:
		_game(game),
		_costEngineer(0),
		_costScientist(0),
		_timePersonnel(0),
//		_initialFunding(0), // 6'000(000)
//		_startingTime(6,1,1,1999,12,0,0),
		_startingTime(1,1,1999,12,0,0),
		_modIndex(0),
		_facilityListOrder(0),
		_craftListOrder(0),
		_itemListOrder(0),
		_researchListOrder(0),
		_manufactureListOrder(0),
		_ufopaediaListOrder(0),
		_invListOrder(0),
		_radarCutoff(1500),
		_firstGrenade(-1),
		_retalCoef(0)
{
	//Log(LOG_INFO) << "Create Ruleset";
	_globe = new RuleGlobe();

	const std::string path (CrossPlatform::getDataFolder("SoldierName/")); // Check in which data dir the folder is stored

	const std::vector<std::string> nation (CrossPlatform::getFolderContents(path, "nam")); // add Soldier names
	for (std::vector<std::string>::const_iterator
			i = nation.begin();
			i != nation.end();
			++i)
	{
		SoldierNamePool* const pool (new SoldierNamePool());
		pool->load(CrossPlatform::noExt(*i));

		_names.push_back(pool);
	}

	const std::vector<std::string> operations (CrossPlatform::getFolderContents(path, "opr")); // add Operation Title words
	for (std::vector<std::string>::const_iterator
			i = operations.begin();
			i != operations.end();
			++i)
	{
		OperationPool* const pool (new OperationPool());
		pool->load(CrossPlatform::noExt(*i));

		_operationTitles.push_back(pool);
	}
}

/**
 * Deletes all the contained rules from memory.
 */
Ruleset::~Ruleset()
{
	delete _globe;

	for (std::vector<SoldierNamePool*>::const_iterator
			i = _names.begin();
			i != _names.end();
			++i)
		delete *i;

	for (std::vector<OperationPool*>::const_iterator
			i = _operationTitles.begin();
			i != _operationTitles.end();
			++i)
		delete *i;

	for (std::map<std::string, RuleCountry*>::const_iterator
			i = _countries.begin();
			i != _countries.end();
			++i)
		delete i->second;

	for (std::map<std::string, RuleRegion*>::const_iterator
			i = _regions.begin();
			i != _regions.end();
			++i)
		delete i->second;

	for (std::map<std::string, RuleBaseFacility*>::const_iterator
			i = _facilities.begin();
			i != _facilities.end();
			++i)
		delete i->second;

	for (std::map<std::string, RuleCraft*>::const_iterator
			i = _crafts.begin();
			i != _crafts.end();
			++i)
		delete i->second;

	for (std::map<std::string, RuleCraftWeapon*>::const_iterator
			i = _craftWeapons.begin();
			i != _craftWeapons.end();
			++i)
		delete i->second;

	for (std::map<std::string, RuleItem*>::const_iterator
			i = _items.begin();
			i != _items.end();
			++i)
		delete i->second;

	for (std::map<std::string, RuleUfo*>::const_iterator
			i = _ufos.begin();
			i != _ufos.end();
			++i)
		delete i->second;

	for (std::map<std::string, RuleTerrain*>::const_iterator
			i = _terrains.begin();
			i != _terrains.end();
			++i)
		delete i->second;

	for (std::vector<std::pair<std::string, RuleMusic*>>::const_iterator // sza_MusicRules
			i = _music.begin();
			i != _music.end();
			++i)
		delete i->second;

	for (std::map<std::string, MapDataSet*>::const_iterator
			i = _mapDataSets.begin();
			i != _mapDataSets.end();
			++i)
		delete i->second;

	for (std::map<std::string, RuleSoldier*>::const_iterator
			i = _soldiers.begin();
			i != _soldiers.end();
			++i)
		delete i->second;

	for (std::map<std::string, RuleUnit*>::const_iterator
			i = _units.begin();
			i != _units.end();
			++i)
		delete i->second;

	for (std::map<std::string, AlienRace*>::const_iterator
			i = _alienRaces.begin();
			i != _alienRaces.end();
			++i)
		delete i->second;

	for (std::map<std::string, AlienDeployment*>::const_iterator
			i = _alienDeployments.begin();
			i != _alienDeployments.end();
			++i)
		delete i->second;

	for (std::map<std::string, RuleArmor*>::const_iterator
			i = _armors.begin();
			i != _armors.end();
			++i)
		delete i->second;

	for (std::map<std::string, ArticleDefinition*>::const_iterator
			i = _ufopaediaArticles.begin();
			i != _ufopaediaArticles.end();
			++i)
		delete i->second;

	for (std::map<std::string, RuleInventory*>::const_iterator
			i = _inventories.begin();
			i != _inventories.end();
			++i)
		delete i->second;

	for (std::map<std::string, RuleResearch*>::const_iterator
			i = _research.begin();
			i != _research.end();
			++i)
		delete i->second;

	for (std::map<std::string, RuleManufacture*>::const_iterator
			i = _manufacture.begin();
			i != _manufacture.end();
			++i)
		delete i->second;

	for (std::map<std::string, UfoTrajectory*>::const_iterator
			i = _ufoTrajectories.begin();
			i != _ufoTrajectories.end();
			++i)
		delete i->second;

	for (std::map<std::string, RuleAlienMission*>::const_iterator
			i = _alienMissions.begin();
			i != _alienMissions.end();
			++i)
		delete i->second;

	for (std::map<std::string, MCDPatch*>::const_iterator
			i = _MCDPatches.begin();
			i != _MCDPatches.end();
			++i)
		delete i->second;

	for (std::vector<std::pair<std::string, ExtraSprites*>>::const_iterator
			i = _extraSprites.begin();
			i != _extraSprites.end();
			++i)
		delete i->second;

	for (std::vector<std::pair<std::string, ExtraSounds*>>::const_iterator
			i = _extraSounds.begin();
			i != _extraSounds.end();
			++i)
		delete i->second;

/*	for (std::vector<std::pair<std::string, ExtraMusic*>>::const_iterator // sza_ExtraMusic
			i = _extraMusic.begin();
			i != _extraMusic.end();
			++i)
		delete i->second; */

	for (std::map<std::string, ExtraStrings*>::const_iterator
			i = _extraStrings.begin();
			i != _extraStrings.end();
			++i)
		delete i->second;

	for (std::map<std::string, RuleAward*>::const_iterator
			i = _awards.begin();
			i != _awards.end();
			++i)
		delete i->second;

	for (std::map<std::string, RuleInterface*>::const_iterator
			i = _interfaces.begin();
			i != _interfaces.end();
			++i)
		delete i->second;

	for (std::map<std::string, std::vector<MapScript*>>::const_iterator
			i = _mapScripts.begin();
			i != _mapScripts.end();
			++i)
		for (std::vector<MapScript*>::const_iterator
				j = (*i).second.begin();
				j != (*i).second.end();
				++j)
			delete *j;

	for (std::map<std::string, RuleMissionScript*>::const_iterator
			i = _missionScripts.begin();
			i != _missionScripts.end();
			++i)
		delete i->second;

	for (std::map<std::string, RuleVideo*>::const_iterator
			i = _videos.begin();
			i != _videos.end();
			++i)
		delete i->second;

/*	for (std::map<std::string, SoundDefinition*>::const_iterator
			i = _soundDefs.begin();
			i != _soundDefs.end();
			++i)
		delete i->second; */

/*	for (std::vector<StatString*>::const_iterator
			i = _statStrings.begin();
			i != _statStrings.end();
			++i)
		delete (*i); */
}

/**
 * Reloads the country lines from Geography rulefile.
 * @note Used in Geoscape's debugmode.
 */
void Ruleset::reloadCountryLines() const
{
	for (std::vector<std::string>::const_iterator
			i = _countriesIndex.begin();
			i != _countriesIndex.end();
			++i)
	{
		RuleCountry* const j (getCountry(*i));
		j->getLonMin().clear();
		j->getLonMax().clear();
		j->getLatMin().clear();
		j->getLatMax().clear();
	}

	const std::string geography (CrossPlatform::getDataFile("Ruleset/Geography.rul"));
	const YAML::Node doc (YAML::LoadFile(geography));
	for (YAML::const_iterator
			i = doc["countries"].begin();
			i != doc["countries"].end();
			++i)
	{
		const std::string type ((*i)["type"].as<std::string>());
		RuleCountry* const j (getCountry(type));

		const std::vector<std::vector<double>> areas ((*i)["areas"].as<std::vector<std::vector<double>>>());
		for (size_t
				k = 0;
				k != areas.size();
				++k)
		{
			j->getLonMin().push_back(areas[k][0] * M_PI / 180.);
			j->getLonMax().push_back(areas[k][1] * M_PI / 180.);
			j->getLatMin().push_back(areas[k][2] * M_PI / 180.);
			j->getLatMax().push_back(areas[k][3] * M_PI / 180.);
		}
	}
}

/**
 * Checks to ensure that MissionScripts don't buttfuck reapers w/out ky-jelly.
 */
void Ruleset::validateMissions() const
{
	// these need to be validated, otherwise we're gonna get into some serious
	// trouble down the line. it may seem like a somewhat arbitrary limitation,
	// but there is a good reason behind it. i'd need to know what results are
	// going to be before they are formulated, and there's a heirarchical
	// structure to the order in which variables are determined for a mission,
	// and the order is DIFFERENT for regular missions vs missions that spawn a
	// mission site. where normally we pick a region, then a mission based on
	// the weights for that region. a terror-type mission picks a mission type
	// FIRST, then a region based on the criteria defined by the mission. there
	// is no way i can conceive of to reconcile this difference to allow mixing
	// and matching, short of knowing the results of calls to the RNG before
	// they're determined. the best solution i can come up with is to disallow
	// it, as there are other ways to acheive what this would amount to anyway,
	// and they don't require time travel. - Warboy
	for (std::map<std::string, RuleMissionScript*>::const_iterator
			i = _missionScripts.begin();
			i != _missionScripts.end();
			++i)
	{
		RuleMissionScript* const scriptRule ((*i).second);

		const std::set<std::string> missionTypes (scriptRule->getAllMissionTypes());
		if (missionTypes.empty() == false)
		{
			std::set<std::string>::const_iterator j (missionTypes.begin());
			if (getAlienMission(*j) == nullptr)
			{
				throw Exception("ERROR with MissionScript: " + (*i).first
					+ ": alien mission type: " + *j + "not defined, "
					+ "so sayeth the wise Alfonso.");
			}

			const bool isSite (getAlienMission(*j)->getObjective() == alm_SITE);
			scriptRule->setSiteType(isSite);

			for (
					;
					j != missionTypes.end();
					++j)
			{
				if (getAlienMission(*j) != nullptr
					&& (getAlienMission(*j)->getObjective() == alm_SITE) != isSite)
				{
					throw Exception("ERROR with MissionScript: " + (*i).first
						+ ": cannot mix Terror/non-Terror missions in one directive, "
						+ "so sayeth the wise Alfonso.");
				}
			}
		}
	}

	// instead of passing a pointer to the region load function and moving the
	// alienMission loading before region loading and sanitizing there, i'll
	// sanitize here, i'm sure this sanitation will grow, and will need to be
	// refactored into its own function at some point, but for now, i'll put it
	// here next to the missionScript sanitation, because it seems the logical
	// place for it, given that this sanitation is required as a result of
	// moving all terror mission handling into missionScripting behaviour.
	// apologies to all the modders that will be getting errors and need to
	// adjust their rulesets, but this will save you weird errors down the line.
	// - Warboy
	for (std::map<std::string, RuleRegion*>::const_iterator
			i = _regions.begin();
			i != _regions.end();
			++i)
	{
		const std::vector<std::string> types ((*i).second->getAvailableMissions().getTypes());
		for (std::vector<std::string>::const_iterator
				j = types.begin();
				j != types.end();
				++j)
		{
			if (getAlienMission(*j)->getObjective() == alm_SITE)
			{
				throw Exception("ERROR with MissionWeights: Region: " + (*i).first
					+ ": has " + *j + " listed. "
					+ "Terror mission can only be invoked via missionScript, "
					+ "so sayeth the wise Alfonso.");
			}
		}
	}
}

/**
 * Loads a ruleset's contents from the given source file.
 * @param src - reference the source file
 */
void Ruleset::load(const std::string& src)
{
	const std::string dir (CrossPlatform::getDataFolder("Ruleset/" + src + '/'));

	if (CrossPlatform::folderExists(dir) == false)
		loadFile(CrossPlatform::getDataFile("Ruleset/" + src + ".rul"));
	else
		loadFiles(dir);

//	_modIndex += 1000;	// note: This prevents extraSprites (etc) from overwriting the indices of original resources.
}						// See also, load() rules ....

/**
 * Loads the contents of all the rule files in the given directory.
 * @param dir - reference the name of an existing directory containing YAML ruleset files
 */
void Ruleset::loadFiles(const std::string& dir) // protected.
{
	std::vector<std::string> files (CrossPlatform::getFolderContents(dir, "rul"));
	for (std::vector<std::string>::const_iterator
			i = files.begin();
			i != files.end();
			++i)
	{
		loadFile(dir + *i);
	}
}

/**
 * Loads a ruleset's contents from a YAML file.
 * @note Rules that match pre-existing rules overwrite them.
 * @param file - reference a YAML file
 */
void Ruleset::loadFile(const std::string& file) // protected.
{
	//Log(LOG_INFO) << "Ruleset::loadFile( -> " << file;
	const YAML::Node doc (YAML::LoadFile(file));
	std::string type;

	for (YAML::const_iterator
			i = doc["countries"].begin();
			i != doc["countries"].end();
			++i)
	{
		RuleCountry* const rule (loadRule(*i, &_countries, &_countriesIndex));
		if (rule != nullptr) rule->load(*i);
	}

	for (YAML::const_iterator
			i = doc["regions"].begin();
			i != doc["regions"].end();
			++i)
	{
		RuleRegion* const rule (loadRule(*i, &_regions, &_regionsIndex));
		if (rule != nullptr) rule->load(*i);
	}

	for (YAML::const_iterator
			i = doc["facilities"].begin();
			i != doc["facilities"].end();
			++i)
	{
		RuleBaseFacility* const rule (loadRule(*i, &_facilities, &_facilitiesIndex));
		if (rule != nullptr)
		{
			_facilityListOrder += 100;
			rule->load(*i, _modIndex, _facilityListOrder);
		}
	}

	for (YAML::const_iterator
			i = doc["crafts"].begin();
			i != doc["crafts"].end();
			++i)
	{
		RuleCraft* const rule (loadRule(*i, &_crafts, &_craftsIndex));
		if (rule != nullptr)
		{
			_craftListOrder += 100;
			rule->load(*i, this, _modIndex, _craftListOrder);
		}
	}

	for (YAML::const_iterator
			i = doc["craftWeapons"].begin();
			i != doc["craftWeapons"].end();
			++i)
	{
		RuleCraftWeapon* const rule (loadRule(*i, &_craftWeapons, &_craftWeaponsIndex));
		if (rule != nullptr) rule->load(*i, _modIndex);
	}

	for (YAML::const_iterator
			i = doc["items"].begin();
			i != doc["items"].end();
			++i)
	{
		RuleItem* const rule (loadRule(*i, &_items, &_itemsIndex));
		if (rule != nullptr)
		{
			_itemListOrder += 100;
			rule->load(*i, _modIndex, _itemListOrder);
		}
	}

	for (YAML::const_iterator
			i = doc["ufos"].begin();
			i != doc["ufos"].end();
			++i)
	{
		RuleUfo* const rule (loadRule(*i, &_ufos, &_ufosIndex));
		if (rule != nullptr) rule->load(*i, this);
	}

	for (YAML::const_iterator
			i = doc["inventories"].begin();
			i != doc["inventories"].end();
			++i)
	{
		RuleInventory* const rule (loadRule(*i, &_inventories, &_invsIndex));
		if (rule != nullptr)
		{
			_invListOrder += 10;
			rule->load(*i, _invListOrder);
		}
	}

	for (YAML::const_iterator
			i = doc["terrains"].begin();
			i != doc["terrains"].end();
			++i)
	{
		RuleTerrain* const rule (loadRule(*i, &_terrains, &_terrainIndex));
		if (rule != nullptr) rule->load(*i, this);
	}

	for (YAML::const_iterator
			i = doc["music"].begin();
			i != doc["music"].end();
			++i)
	{
		std::auto_ptr<RuleMusic> ruleMusic (new RuleMusic());
		ruleMusic->load(*i);

		type = (*i)["type"].as<std::string>();
		_music.push_back(std::make_pair(
									type,
									ruleMusic.release()));
		_musicIndex.push_back(type);
	}

	for (YAML::const_iterator
			i = doc["armors"].begin();
			i != doc["armors"].end();
			++i)
	{
		RuleArmor* const rule (loadRule(*i, &_armors, &_armorsIndex));
		if (rule != nullptr)
			rule->load(*i);
	}

	for (YAML::const_iterator
			i = doc["soldiers"].begin();
			i != doc["soldiers"].end();
			++i)
	{
		RuleSoldier* const rule (loadRule(*i, &_soldiers, &_soldiersIndex));
		if (rule != nullptr) rule->load(*i);
	}

	for (YAML::const_iterator
			i = doc["units"].begin();
			i != doc["units"].end();
			++i)
	{
		RuleUnit* const rule (loadRule(*i, &_units));
		if (rule != nullptr) rule->load(*i, _modIndex);
	}

	for (YAML::const_iterator
			i = doc["alienRaces"].begin();
			i != doc["alienRaces"].end();
			++i)
	{
		AlienRace* const rule (loadRule(*i, &_alienRaces, &_aliensIndex));
		if (rule != nullptr) rule->load(*i);
	}

	for (YAML::const_iterator
			i = doc["alienDeployments"].begin();
			i != doc["alienDeployments"].end();
			++i)
	{
		AlienDeployment* const rule (loadRule(*i, &_alienDeployments, &_deploymentsIndex));
		if (rule != nullptr) rule->load(*i);
	}

	for (YAML::const_iterator
			i = doc["research"].begin();
			i != doc["research"].end();
			++i)
	{
		RuleResearch* const rule (loadRule(*i, &_research, &_researchIndex));
		if (rule != nullptr)
		{
			_researchListOrder += 100;
			rule->load(*i, _researchListOrder);

			if ((*i)["unlockFinalMission"].as<bool>(false))
				_finalResearch = (*i)["type"].as<std::string>(_finalResearch);
		}
	}

	for (YAML::const_iterator
			i = doc["manufacture"].begin();
			i != doc["manufacture"].end();
			++i)
	{
		RuleManufacture* const rule (loadRule(*i, &_manufacture, &_manufactureIndex));
		if (rule != nullptr)
		{
			_manufactureListOrder += 100;
			rule->load(*i, _manufactureListOrder);
		}
	}

	for (YAML::const_iterator
			i = doc["ufopaedia"].begin();
			i != doc["ufopaedia"].end();
			++i)
	{
		if ((*i)["id"])
		{
			type = (*i)["id"].as<std::string>();

			ArticleDefinition* articleRule;
			if (_ufopaediaArticles.find(type) != _ufopaediaArticles.end())
				articleRule = _ufopaediaArticles[type];
			else
			{
				const UfopaediaTypeId typeId (static_cast<UfopaediaTypeId>((*i)["type_id"].as<int>()));
				switch (typeId)
				{
					case UFOPAEDIA_TYPE_CRAFT:			articleRule = new ArticleDefinitionCraft();			break;
					case UFOPAEDIA_TYPE_CRAFT_WEAPON:	articleRule = new ArticleDefinitionCraftWeapon();	break;
					case UFOPAEDIA_TYPE_VEHICLE:		articleRule = new ArticleDefinitionVehicle();		break;
					case UFOPAEDIA_TYPE_ITEM:			articleRule = new ArticleDefinitionItem();			break;
					case UFOPAEDIA_TYPE_ARMOR:			articleRule = new ArticleDefinitionArmor();			break;
					case UFOPAEDIA_TYPE_BASE_FACILITY:	articleRule = new ArticleDefinitionBaseFacility();	break;
					case UFOPAEDIA_TYPE_TEXTIMAGE:		articleRule = new ArticleDefinitionTextImage();		break;
					case UFOPAEDIA_TYPE_TEXT:			articleRule = new ArticleDefinitionText();			break;
					case UFOPAEDIA_TYPE_UFO:			articleRule = new ArticleDefinitionUfo();			break;
					case UFOPAEDIA_TYPE_AWARD:			articleRule = new ArticleDefinitionAward();			break;

					default:
						articleRule = nullptr; // safety.
				}

				if (articleRule != nullptr)
				{
					_ufopaediaArticles[type] = articleRule;
					_ufopaediaIndex.push_back(type);
				}
				else Log(LOG_INFO) << "ERROR: undefined ArticleDefinition typeId [" << typeId << "] for " << type;
			}

			_ufopaediaListOrder += 100;
			//Log(LOG_INFO) << id << " uPed listOrder = " << _ufopaediaListOrder; // Prints listOrder to LOG.
			articleRule->load(*i, _ufopaediaListOrder);
		}
		else if ((*i)["delete"])
		{
			const std::string type ((*i)["delete"].as<std::string>());
			const std::map<std::string, ArticleDefinition*>::const_iterator pArticle (_ufopaediaArticles.find(type));
			if (pArticle != _ufopaediaArticles.end())
				_ufopaediaArticles.erase(pArticle);

			const std::vector<std::string>::const_iterator j (std::find(
																	_ufopaediaIndex.begin(),
																	_ufopaediaIndex.end(),
																	type));
			if (j != _ufopaediaIndex.end())
				_ufopaediaIndex.erase(j);
		}
	}

	// Bases can't be copied so for savegame purposes store the node instead
	const YAML::Node base = doc["startingBase"];
	if (base != 0)
	{
		for (YAML::const_iterator
				i = base.begin();
				i != base.end();
				++i)
		{
			_startingBase[i->first.as<std::string>()] = YAML::Node(i->second);
		}
	}

	if (doc["startingTime"])
		_startingTime.load(doc["startingTime"]);

	_costEngineer	= doc["costEngineer"]	.as<int>(_costEngineer);
	_costScientist	= doc["costScientist"]	.as<int>(_costScientist);
	_timePersonnel	= doc["timePersonnel"]	.as<int>(_timePersonnel);
//	_initialFunding	= doc["initialFunding"]	.as<int>(_initialFunding);
	_alienFuel		= doc["alienFuel"]		.as<std::pair<std::string, int>>(_alienFuel);
	_font			= doc["font"]			.as<std::string>(_font);
	_radarCutoff	= doc["radarCutoff"]	.as<int>(_radarCutoff);
	_firstGrenade	= doc["firstGrenade"]	.as<int>(_firstGrenade);
	_retalCoef		= doc["retalCoef"]		.as<int>(_retalCoef);

	for (YAML::const_iterator
			i = doc["ufoTrajectories"].begin();
			i != doc["ufoTrajectories"].end();
			++i)
	{
		UfoTrajectory* const rule (loadRule(*i, &_ufoTrajectories, nullptr, "id"));
		if (rule != nullptr) rule->load(*i);
	}

	for (YAML::const_iterator
			i = doc["alienMissions"].begin();
			i != doc["alienMissions"].end();
			++i)
	{
		RuleAlienMission* const rule (loadRule(*i, &_alienMissions, &_alienMissionsIndex));
		if (rule != nullptr) rule->load(*i);
	}

	_alienItemLevels = doc["alienItemLevels"].as<std::vector< std::vector<int>>>(_alienItemLevels);

	for (YAML::const_iterator
			i = doc["MCDPatches"].begin();
			i != doc["MCDPatches"].end();
			++i)
	{
		type = (*i)["type"].as<std::string>();
		if (_MCDPatches.find(type) != _MCDPatches.end())
			_MCDPatches[type]->load(*i);
		else
		{
			std::auto_ptr<MCDPatch> patch (new MCDPatch());
			patch->load(*i);
			_MCDPatches[type] = patch.release();
			_MCDPatchesIndex.push_back(type);
		}
	}

	for (YAML::const_iterator
			i = doc["extraSprites"].begin();
			i != doc["extraSprites"].end();
			++i)
	{
		type = (*i)["type"].as<std::string>();
		std::auto_ptr<ExtraSprites> extraSprites (new ExtraSprites());

		if (type != "TEXTURE.DAT") // doesn't support modIndex
			extraSprites->load(*i, _modIndex);
		else
			extraSprites->load(*i, 0);

		_extraSprites.push_back(std::make_pair(type, extraSprites.release()));
		_extraSpritesIndex.push_back(type);
	}

	for (YAML::const_iterator
			i = doc["extraSounds"].begin();
			i != doc["extraSounds"].end();
			++i)
	{
		type = (*i)["type"].as<std::string>();
		std::auto_ptr<ExtraSounds> extraSounds (new ExtraSounds());
		extraSounds->load(*i, _modIndex);
		_extraSounds.push_back(std::make_pair(type, extraSounds.release()));
		_extraSoundsIndex.push_back(type);
	}

	for (YAML::const_iterator
			i = doc["extraStrings"].begin();
			i != doc["extraStrings"].end();
			++i)
	{
		type = (*i)["type"].as<std::string>();
		if (_extraStrings.find(type) != _extraStrings.end())
			_extraStrings[type]->load(*i);
		else
		{
			std::auto_ptr<ExtraStrings> extraStrings (new ExtraStrings());
			extraStrings->load(*i);
			_extraStrings[type] = extraStrings.release();
			_extraStringsIndex.push_back(type);
		}
	}

	for (YAML::const_iterator
			i = doc["awards"].begin();
			i != doc["awards"].end();
			++i)
	{
		type = (*i)["type"].as<std::string>();
		std::auto_ptr<RuleAward> award (new RuleAward());
		award->load(*i);
		_awards[type] = award.release();
	}

/*	for (YAML::const_iterator i = doc["statStrings"].begin(); i != doc["statStrings"].end(); ++i)
	{
		StatString* const statString = new StatString();
		statString->load(*i);
		_statStrings.push_back(statString);
	} */

	for (YAML::const_iterator
			i = doc["interfaces"].begin();
			i != doc["interfaces"].end();
			++i)
	{
		RuleInterface* const rule (loadRule(*i, &_interfaces));
		if (rule != nullptr) rule->load(*i);
	}

	if (doc["globe"])
		_globe->load(doc["globe"]);

	for (YAML::const_iterator
			i = doc["constants"].begin();
			i != doc["constants"].end();
			++i)
	{
		ResourcePack::EXPLOSION_OFFSET		= (*i)["explosionOffset"]	.as<int>(ResourcePack::EXPLOSION_OFFSET);
		ResourcePack::SMALL_EXPLOSION		= (*i)["smallExplosion"]	.as<int>(ResourcePack::SMALL_EXPLOSION);
		ResourcePack::DOOR_OPEN				= (*i)["doorSound"]			.as<int>(ResourcePack::DOOR_OPEN);
		ResourcePack::LARGE_EXPLOSION		= (*i)["largeExplosion"]	.as<int>(ResourcePack::LARGE_EXPLOSION);
		ResourcePack::FLYING_SOUND			= (*i)["flyingSound"]		.as<int>(ResourcePack::FLYING_SOUND);
		ResourcePack::ITEM_RELOAD			= (*i)["itemReload"]		.as<int>(ResourcePack::ITEM_RELOAD);
		ResourcePack::SLIDING_DOOR_OPEN		= (*i)["slidingDoorSound"]	.as<int>(ResourcePack::SLIDING_DOOR_OPEN);
		ResourcePack::SLIDING_DOOR_CLOSE	= (*i)["slidingDoorClose"]	.as<int>(ResourcePack::SLIDING_DOOR_CLOSE);
		ResourcePack::WALK_OFFSET			= (*i)["walkOffset"]		.as<int>(ResourcePack::WALK_OFFSET);
		ResourcePack::ITEM_DROP				= (*i)["itemDrop"]			.as<int>(ResourcePack::ITEM_DROP);
		ResourcePack::ITEM_THROW			= (*i)["itemThrow"]			.as<int>(ResourcePack::ITEM_THROW);
		ResourcePack::SMOKE_OFFSET			= (*i)["smokeOffset"]		.as<int>(ResourcePack::SMOKE_OFFSET);

		if ((*i)["maleScream"])
		{
			int id = 0;
			for (YAML::const_iterator
					j = (*i)["maleScream"].begin();
					j != (*i)["maleScream"].end() && id != 3;
					++j, ++id)
			{
				ResourcePack::MALE_SCREAM[id] = (*j).as<int>(ResourcePack::MALE_SCREAM[id]);
			}
		}

		if ((*i)["femaleScream"])
		{
			int id = 0;
			for (YAML::const_iterator
					j = (*i)["femaleScream"].begin();
					j != (*i)["femaleScream"].end() && id != 3;
					++j, ++id)
			{
				ResourcePack::FEMALE_SCREAM[id] = (*j).as<int>(ResourcePack::FEMALE_SCREAM[id]);
			}
		}

		ResourcePack::BUTTON_PRESS = (*i)["buttonPress"].as<int>(ResourcePack::BUTTON_PRESS);
		if ((*i)["windowPopup"])
		{
			int id = 0;
			for (YAML::const_iterator
					j = (*i)["windowPopup"].begin();
					j != (*i)["windowPopup"].end() && id != 3;
					++j, ++id)
			{
				ResourcePack::WINDOW_POPUP[id] = (*j).as<int>(ResourcePack::WINDOW_POPUP[id]);
			}
		}

		ResourcePack::UFO_FIRE				= (*i)["ufoFire"]			.as<int>(ResourcePack::UFO_FIRE);
		ResourcePack::UFO_HIT				= (*i)["ufoHit"]			.as<int>(ResourcePack::UFO_HIT);
		ResourcePack::UFO_CRASH				= (*i)["ufoCrash"]			.as<int>(ResourcePack::UFO_CRASH);
		ResourcePack::UFO_EXPLODE			= (*i)["ufoExplode"]		.as<int>(ResourcePack::UFO_EXPLODE);
		ResourcePack::INTERCEPTOR_HIT		= (*i)["interceptorHit"]	.as<int>(ResourcePack::INTERCEPTOR_HIT);
		ResourcePack::INTERCEPTOR_EXPLODE	= (*i)["interceptorExplode"].as<int>(ResourcePack::INTERCEPTOR_EXPLODE);

		ResourcePack::GEOSCAPE_CURSOR		= (*i)["geoscapeCursor"]	.as<int>(ResourcePack::GEOSCAPE_CURSOR);
		ResourcePack::BASESCAPE_CURSOR		= (*i)["basescapeCursor"]	.as<int>(ResourcePack::BASESCAPE_CURSOR);
		ResourcePack::BATTLESCAPE_CURSOR	= (*i)["battlescapeCursor"]	.as<int>(ResourcePack::BATTLESCAPE_CURSOR);
		ResourcePack::UFOPAEDIA_CURSOR		= (*i)["ufopaediaCursor"]	.as<int>(ResourcePack::UFOPAEDIA_CURSOR);
		ResourcePack::GRAPHS_CURSOR			= (*i)["graphsCursor"]		.as<int>(ResourcePack::GRAPHS_CURSOR);
	}

	for (YAML::const_iterator
			i = doc["mapScripts"].begin();
			i != doc["mapScripts"].end();
			++i)
	{
		type = (*i)["type"].as<std::string>();
		if ((*i)["delete"])
			type = (*i)["delete"].as<std::string>(type);

		if (_mapScripts.find(type) != _mapScripts.end())
		{
			for (std::vector<MapScript*>::const_iterator
					j = _mapScripts[type].begin();
					j != _mapScripts[type].end();
					)
			{
				delete *j;
				j = _mapScripts[type].erase(j);
			}
		}

		for (YAML::const_iterator
				j = (*i)["commands"].begin();
				j != (*i)["commands"].end();
				++j)
		{
			std::auto_ptr<MapScript> mapScript (new MapScript());
			mapScript->load(*j);
			_mapScripts[type].push_back(mapScript.release());
		}
	}

	for (YAML::const_iterator
			i = doc["missionScripts"].begin();
			i != doc["missionScripts"].end();
			++i)
	{
		RuleMissionScript* const rule (loadRule(*i, &_missionScripts, &_missionScriptIndex));
		if (rule != nullptr) rule->load(*i);
	}

/*	for (std::vector<std::string>::const_iterator // refresh _psiRequirements for psiStrengthEval
			i = _facilitiesIndex.begin();
			i != _facilitiesIndex.end();
			++i)
	{
		const RuleBaseFacility* const rule = getBaseFacility(*i);
		if (rule->getPsiLaboratories() != 0)
		{
			_psiRequirements = rule->getRequirements();
			break;
		}
	} */

	for (YAML::const_iterator
			i = doc["cutscenes"].begin();
			i != doc["cutscenes"].end();
			++i)
	{
		RuleVideo* const rule (loadRule(*i, &_videos));
		if (rule != nullptr) rule->load(*i);
	}
}

/**
 * Loads a ruleset-element adding/removing from vectors as necessary.
 * @param node	- reference a YAML node
 * @param types	- pointer to a map associated to the rule type
 * @param index	- pointer to a vector of indices for the rule type (default nullptr)
 * @param keyId	- reference the rule's key-ID (default "type")
 * @return, pointer to new rule if one was created or nullptr if one was removed
 */
template<typename T>
T* Ruleset::loadRule( // protected.
		const YAML::Node& node,
		std::map<std::string, T*>* types,
		std::vector<std::string>* index,
		const std::string& keyId)
{
	T* rule = nullptr;

	if (node[keyId])
	{
		const std::string type (node[keyId].as<std::string>());
		typename std::map<std::string, T*>::const_iterator i (types->find(type));
		if (i != types->end())
			rule = i->second;
		else
		{
			rule = new T(type);
			(*types)[type] = rule;

			if (index != nullptr)
				index->push_back(type);
		}
	}
	else if (node["delete"])
	{
		const std::string type (node["delete"].as<std::string>());
		typename std::map<std::string, T*>::const_iterator i (types->find(type));
		if (i != types->end())
			types->erase(i);

		if (index != nullptr)
		{
			const std::vector<std::string>::const_iterator j (std::find(
																	index->begin(),
																	index->end(),
																	type));
			if (j != index->end())
				index->erase(j);
		}
	}
	return rule;
}

/**
 * Generates a SavedGame with starting data.
 * @return, pointer to the SavedGame
 */
SavedGame* Ruleset::createSave() const
{
	RNG::setSeed(0);

	SavedGame* const gameSave (new SavedGame(this));

	// Setup research generals.
	for (std::vector<std::string>::const_iterator
			i = _researchIndex.begin();
			i != _researchIndex.end();
			++i)
	{
		gameSave->getResearchGenerals().push_back(new ResearchGeneral(getResearch(*i)));
	}

	// Add countries.
	for (std::vector<std::string>::const_iterator
			i = _countriesIndex.begin();
			i != _countriesIndex.end();
			++i)
	{
		RuleCountry* const country (getCountry(*i));
		if (country->getLonMin().empty() == false) // safety.
			gameSave->getCountries()->push_back(new Country(country, true));
	}

	// Adjust funding to total $6M.
//	int missing = ((_initialFunding - gameSave->getCountryFunding() / 1000) / (int)gameSave->getCountries()->size()) * 1000;
	for (std::vector<Country*>::const_iterator
			i = gameSave->getCountries()->begin();
			i != gameSave->getCountries()->end();
			++i)
	{
//		int funding = (*i)->getFunding().back() + missing;
//		if (funding < 0)
//			funding = (*i)->getFunding().back();

		int funding ((*i)->getFunding().back());
		if (funding < 0) funding = 0; // safety, i guess.

		(*i)->setFunding(funding);
	}

	gameSave->setFunds(gameSave->getCountryFunding());

	// Add regions.
	for (std::vector<std::string>::const_iterator
			i = _regionsIndex.begin();
			i != _regionsIndex.end();
			++i)
	{
		RuleRegion* const region (getRegion(*i));
		if (region->getLonMin().empty() == false) // safety.
			gameSave->getRegions()->push_back(new Region(region));
	}

	// Set up starting base.
	Base* const base (new Base(this));
	base->load(_startingBase, gameSave, true);

	// Correct IDs.
	for (std::vector<Craft*>::const_iterator
			i = base->getCrafts()->begin();
			i != base->getCrafts()->end();
			++i)
	{
		gameSave->getCanonicalId((*i)->getRules()->getType());
	}

	// Determine starting transport craft
/*	Craft* transportCraft (nullptr);
	for (std::vector<Craft*>::const_iterator
			i = base->getCrafts()->begin();
			i != base->getCrafts()->end();
			++i)
	{
		if ((*i)->getRules()->getSoldiers() != 0)
		{
			transportCraft = *i;
			break;
		}
	} */

	// Determine starting soldier types
	std::vector<std::string> soldierTypes (_soldiersIndex);
	for (std::vector<std::string>::const_iterator
			i = soldierTypes.begin();
			i != soldierTypes.end();
			)
	{
		if (getSoldier(*i)->getRequirements().empty() == false)
			i = soldierTypes.erase(i);
		else
			++i;
	}

	const YAML::Node& node (_startingBase["randomSoldiers"]);
	std::vector<std::string> solTypes;
	if (node)
	{
		if (node.IsMap() == true) // Starting soldiers specified by type
		{
			std::map<std::string, int> randSoldiers (node.as<std::map<std::string, int>>(std::map<std::string, int>()));
			for (std::map<std::string, int>::const_iterator
					i = randSoldiers.begin();
					i != randSoldiers.end();
					++i)
			{
				for (int
						j = 0;
						j != i->second;
						++j)
				{
					solTypes.push_back(i->first);
				}
			}
		}
		else if (node.IsScalar() == true) // Starting soldiers specified by amount
		{
			const int randSoldiers (node.as<int>(0));
			for (int
					i = 0;
					i != randSoldiers;
					++i)
			{
				solTypes.push_back(soldierTypes[RNG::pick(soldierTypes.size())]);
			}
		}

		// Generate soldiers
		for (size_t
				i = 0;
				i != solTypes.size();
				++i)
		{
			Soldier* const sol (genSoldier(gameSave, solTypes[i]));

//			if (transportCraft != 0 && i < transportCraft->getRules()->getSoldiers())
//				soldier->setCraft(transportCraft);

			base->getSoldiers()->push_back(sol);

			// Award each Soldier the special Original Eight award.
			SoldierDiary* const diary (sol->getDiary());
			diary->awardOriginalEight();

			for (std::vector<SoldierAward*>::const_iterator
					j = diary->getSoldierAwards()->begin();
					j != diary->getSoldierAwards()->end();
					++j)
			{
				(*j)->clearRecent();
			}
		}
	}

	gameSave->getBases()->push_back(base);
	gameSave->getAlienStrategy().init(this); // Setup aLien strategy.
	gameSave->setTime(_startingTime);

	return gameSave;
}

/**
 * Returns the list of soldier name pools.
 * @return, reference to a vector of SoldierNamePool pointers
 */
const std::vector<SoldierNamePool*>& Ruleset::getPools() const
{
	return _names;
}

/**
 * Returns the list of operation title pools.
 * @return, reference to a vector of OperationPool pointers
 */
const std::vector<OperationPool*>& Ruleset::getOperations() const
{
	return _operationTitles;
}

/**
 * Returns the rules for the specified country.
 * @param id - reference a Country type
 * @return, pointer to Rule for the country
 */
RuleCountry* Ruleset::getCountry(const std::string& id) const
{
	std::map<std::string, RuleCountry*>::const_iterator i (_countries.find(id));
	if (i != _countries.end())
		return i->second;

	return nullptr;
}

/**
 * Returns the list of all countries provided by the ruleset.
 * @return, reference to a vector of Country types
 */
const std::vector<std::string>& Ruleset::getCountriesList() const
{
	return _countriesIndex;
}

/**
 * Returns the rules for the specified region.
 * @param id - reference a Region type
 * @return, pointer to Rule for the region
 */
RuleRegion* Ruleset::getRegion(const std::string& id) const
{
	std::map<std::string, RuleRegion*>::const_iterator i (_regions.find(id));
	if (i != _regions.end())
		return i->second;

	return nullptr;
}

/**
 * Returns the list of all regions provided by the ruleset.
 * @return, reference to a vector of Region types
 */
const std::vector<std::string>& Ruleset::getRegionsList() const
{
	return _regionsIndex;
}

/**
 * Returns the rules for the specified base facility.
 * @param id - reference a BaseFacility type
 * @return, pointer to Rule for the facility
 */
RuleBaseFacility* Ruleset::getBaseFacility(const std::string& id) const
{
	std::map<std::string, RuleBaseFacility*>::const_iterator i (_facilities.find(id));
	if (i != _facilities.end())
		return i->second;

	return nullptr;
}

/**
 * Returns the list of all base facilities provided by the ruleset.
 * @return, reference to a vector of BaseFacility types
 */
const std::vector<std::string>& Ruleset::getBaseFacilitiesList() const
{
	return _facilitiesIndex;
}

/**
 * Returns the rules for the specified craft.
 * @param id - reference a Craft type
 * @return, pointer to Rule for the craft
 */
RuleCraft* Ruleset::getCraft(const std::string& id) const
{
	std::map<std::string, RuleCraft*>::const_iterator i (_crafts.find(id));
	if (i != _crafts.end())
		return i->second;

	return nullptr;
}

/**
 * Returns the list of all crafts provided by the ruleset.
 * @return, reference to a vector of Craft types
 */
const std::vector<std::string>& Ruleset::getCraftsList() const
{
	return _craftsIndex;
}

/**
 * Returns the rules for the specified craft weapon.
 * @param id - reference a CraftWeapon type
 * @return, pointer to Rule for the CraftWeapon
 */
RuleCraftWeapon* Ruleset::getCraftWeapon(const std::string& id) const
{
	std::map<std::string, RuleCraftWeapon*>::const_iterator i (_craftWeapons.find(id));
	if (i != _craftWeapons.end())
		return i->second;

	return nullptr;
}

/**
 * Returns the list of all craft weapons provided by the ruleset.
 * @return, reference to a vector of CraftWeapon types
 */
const std::vector<std::string>& Ruleset::getCraftWeaponsList() const
{
	return _craftWeaponsIndex;
}

/**
 * Returns the rules for the specified item.
 * @param id - reference to an Item type
 * @return, pointer to RuleItem or nullptr if not found
 */
RuleItem* Ruleset::getItemRule(const std::string& id) const
{
	std::map<std::string, RuleItem*>::const_iterator i (_items.find(id));
	if (i != _items.end())
		return i->second;

	return nullptr;
}

/**
 * Returns the list of all items provided by the ruleset.
 * @return, reference to a vector of Item types
 */
const std::vector<std::string>& Ruleset::getItemsList() const
{
	return _itemsIndex;
}

/**
 * Returns the rules for the specified UFO.
 * @param id - reference a Ufo type
 * @return, pointer to Rule for the Ufo
 */
RuleUfo* Ruleset::getUfo(const std::string& id) const
{
	std::map<std::string, RuleUfo*>::const_iterator i (_ufos.find(id));
	if (i != _ufos.end())
		return i->second;

	return nullptr;
}

/**
 * Returns the list of all ufos provided by the ruleset.
 * @return, reference to a vector of Ufo types
 */
const std::vector<std::string>& Ruleset::getUfosList() const
{
	return _ufosIndex;
}

/**
 * Returns the rules for the specified terrain.
 * @param type - reference a Terrain type
 * @return, pointer to Rule for the Terrain
 */
RuleTerrain* Ruleset::getTerrain(const std::string& type) const
{
	std::map<std::string, RuleTerrain*>::const_iterator i (_terrains.find(type));

	if (i != _terrains.end())
		return i->second;

	return nullptr;
}

/**
 * Returns the list of all terrains provided by the ruleset.
 * @return, reference to a vector of Terrain types
 */
const std::vector<std::string>& Ruleset::getTerrainList() const
{
	return _terrainIndex;
}

/**
 * Returns the info about a specific map data file.
 * @param name - reference a MapDataSet type
 * @return, pointer to rule for the MapDataSet
 */
MapDataSet* Ruleset::getMapDataSet(const std::string& name)
{
	std::map<std::string, MapDataSet*>::const_iterator i (_mapDataSets.find(name));
	if (i == _mapDataSets.end())
	{
		MapDataSet* const dataSet (new MapDataSet(name, _game));
		_mapDataSets[name] = dataSet;
		return dataSet;
	}

	return i->second;
}

/**
 * Gets the list of Awards.
 * @return, map of awards
 */
std::map<std::string, RuleAward*> Ruleset::getAwardsList() const
{
	return _awards;
}

/**
 * Returns general info about Soldiers.
 * @param type - reference a Soldier type
 * @return, pointer to Rule for the Soldier
 */
RuleSoldier* Ruleset::getSoldier(const std::string& type) const
{
	std::map<std::string, RuleSoldier*>::const_iterator i (_soldiers.find(type));
	if (i != _soldiers.end())
		return i->second;

	return nullptr;
}

/**
 * Returns the list of all soldiers provided by this Ruleset.
 * @return, reference to a vector of strings - the list of soldier types
 */
const std::vector<std::string>& Ruleset::getSoldiersList() const
{
	return _soldiersIndex;
}

/**
 * Returns general info about non-Soldier units.
 * @param type - reference a Unit type
 * @return, pointer to the Unit rules
 */
RuleUnit* Ruleset::getUnitRule(const std::string& type) const
{
	std::map<std::string, RuleUnit*>::const_iterator i (_units.find(type));
	if (i != _units.end())
		return i->second;

	return nullptr;
}

/**
 * Returns the info about a specific alien race.
 * @param type - reference a Race type
 * @return, pointer to the Rule for AlienRaces
 */
AlienRace* Ruleset::getAlienRace(const std::string& type) const
{
	std::map<std::string, AlienRace*>::const_iterator i (_alienRaces.find(type));
	if (i != _alienRaces.end())
		return i->second;

	return nullptr;
}

/**
 * Returns the list of all alien races provided by the ruleset.
 * @return, reference to the vector of AlienRace types
 */
const std::vector<std::string>& Ruleset::getAlienRacesList() const
{
	return _aliensIndex;
}

/**
 * Returns the info about a specific deployment.
 * @param name - reference the AlienDeployment type
 * @return, pointer to Rule for the AlienDeployment
 */
AlienDeployment* Ruleset::getDeployment(const std::string& name) const
{
	std::map<std::string, AlienDeployment*>::const_iterator i (_alienDeployments.find(name));
	if (i != _alienDeployments.end())
		return i->second;

	return nullptr;
}

/**
 * Returns the list of all alien deployments provided by the ruleset.
 * @return, reference to the vector of AlienDeployments
 */
const std::vector<std::string>& Ruleset::getDeploymentsList() const
{
	return _deploymentsIndex;
}

/**
 * Returns the info about a specific armor.
 * @param name - reference the Armor type
 * @return, pointer to RuleArmor
 */
RuleArmor* Ruleset::getArmor(const std::string& name) const
{
	std::map<std::string, RuleArmor*>::const_iterator i (_armors.find(name));
	if (i != _armors.end())
		return i->second;

	return nullptr;
}

/**
 * Returns the list of all armors provided by the ruleset.
 * @return, reference to the vector of Armors
 */
const std::vector<std::string>& Ruleset::getArmorsList() const
{
	return _armorsIndex;
}

/**
 * Returns the cost of an individual engineer for purchase/maintenance.
 * @return, cost
 */
int Ruleset::getEngineerCost() const
{
	return _costEngineer;
}

/**
 * Returns the cost of an individual scientist for purchase/maintenance.
 * @return, cost
 */
int Ruleset::getScientistCost() const
{
	return _costScientist;
}

/**
 * Returns the time it takes to transfer personnel between bases.
 * @return, time in hours
 */
int Ruleset::getPersonnelTime() const
{
	return _timePersonnel;
}

/**
 * Returns the ArticleDefinition for a given name.
 * @param name - article name
 * @return, pointer to ArticleDefinition
 */
ArticleDefinition* Ruleset::getUfopaediaArticle(const std::string& article_id) const
{
	std::map<std::string, ArticleDefinition*>::const_iterator i (_ufopaediaArticles.find(article_id));
	if (i != _ufopaediaArticles.end())
		return i->second;

	return nullptr;
}

/**
 * Returns the list of all articles provided by the ruleset.
 * @return, reference to a vector of strings as the list of articles
 */
const std::vector<std::string>& Ruleset::getUfopaediaList() const
{
	return _ufopaediaIndex;
}

/**
 * Returns a mapping of the inventories.
 * @return, pointer to a vector of maps of strings and pointers to RuleInventory
 */
std::map<std::string, RuleInventory*>* Ruleset::getInventories()
{
	return &_inventories;
}

/**
 * Returns the RuleInventory for a specific inventory-type (a 'section').
 * @param type - reference the inventory type
 * @return, pointer to RuleInventory
 */
const RuleInventory* Ruleset::getInventory(const std::string& type) const
{
	std::map<std::string, RuleInventory*>::const_iterator i (_inventories.find(type));
	if (i != _inventories.end())
		return i->second;

	return nullptr;
}

/**
 * Returns the RuleInventory for a specific inventory-section.
 * @param sectionId - InventorySection (RuleInventory.h)
 * @return, pointer to RuleInventory
 */
const RuleInventory* Ruleset::getInventoryRule(InventorySection sectionId) const
{
	return _inventories_ST.at(sectionId);
}

/**
 * Converts all inventory mappings from string-keys to enumerated-keys.
 */
void Ruleset::convertInventories()
{
	for (std::map<std::string, RuleInventory*>::const_iterator
			i = _inventories.begin();
			i != _inventories.end();
			++i)
	{
		if (i->first == "STR_GROUND")
			_inventories_ST.emplace(ST_GROUND, i->second);
		else if (i->first == "STR_RIGHT_HAND")
			_inventories_ST.emplace(ST_RIGHTHAND, i->second);
		else if (i->first == "STR_LEFT_HAND")
			_inventories_ST.emplace(ST_LEFTHAND, i->second);
		else if (i->first == "STR_BELT")
			_inventories_ST.emplace(ST_BELT, i->second);
		else if (i->first == "STR_RIGHT_LEG")
			_inventories_ST.emplace(ST_RIGHTLEG, i->second);
		else if (i->first == "STR_LEFT_LEG")
			_inventories_ST.emplace(ST_LEFTLEG, i->second);
		else if (i->first == "STR_RIGHT_SHOULDER")
			_inventories_ST.emplace(ST_RIGHTSHOULDER, i->second);
		else if (i->first == "STR_LEFT_SHOULDER")
			_inventories_ST.emplace(ST_LEFTSHOULDER, i->second);
		else if (i->first == "STR_BACK_PACK")
			_inventories_ST.emplace(ST_BACKPACK, i->second);
		else if (i->first == "STR_QUICK_DRAW")
			_inventories_ST.emplace(ST_QUICKDRAW, i->second);
		else
			Log(LOG_WARNING) << "Ruleset::convertInventories() unknown inventory-type detected [" << i->first << "]";
	}

	_inventories_ST.emplace(ST_NONE, nullptr);
}

/**
 * Returns the list of inventories.
 * @return, reference to a vector of strings as the list of inventories
 *
const std::vector<std::string>& Ruleset::getInventoryList() const
{
	return _invsIndex;
} */

/**
 * Determines the highest TU-value used to move an item from a player-section to
 * the ground-section.
 * @note This is the lowest value a unit will get at the start of its turn
 * unless it has just panicked or revived etc.
 * @return, high TU value
 */
int Ruleset::detHighTuInventoryCost() const
{
	int
		cost,
		costHigh = 0;

	const RuleInventory* const grdRule (getInventoryRule(ST_GROUND));
	for (std::map<std::string, RuleInventory*>::const_iterator
			i = _inventories.begin();
			i != _inventories.end();
			++i)
	{
		cost = (*i).second->getCost(grdRule);
		if (cost > costHigh)
			costHigh = cost;
	}
	return costHigh;
}
/*	for (std::map<std::string, RuleInventory*>::const_iterator
			i = _inventories.begin();
			i != _inventories.end();
			++i)
	{
		for (std::map<std::string, RuleInventory*>::const_iterator
				j = _inventories.begin();
				j != _inventories.end();
				++j)
		{
			cost = (*i).second->getCost((*j).second);
			if (cost > costHigh)
				costHigh = cost;
		}
	} */

/**
 * Returns the rules for the specified research project.
 * @param id - reference a research project type
 * @return, pointer to RuleResearch
 */
const RuleResearch* Ruleset::getResearch(const std::string& id) const
{
	std::map<std::string, RuleResearch*>::const_iterator i (_research.find(id));
	if (i != _research.end())
		return i->second;

	return nullptr;
}

/**
 * Returns the list of research projects.
 * @return, reference to a vector of strings as the list of research projects
 */
const std::vector<std::string>& Ruleset::getResearchList() const
{
	return _researchIndex;
}

/**
 * Returns the rules for the specified manufacture project.
 * @param id - reference the manufacture project type
 * @return, pointer to RuleManufacture
 */
RuleManufacture* Ruleset::getManufacture(const std::string& id) const
{
	std::map<std::string, RuleManufacture*>::const_iterator i (_manufacture.find(id));
	if (i != _manufacture.end())
		return i->second;

	return nullptr;
}

/**
 * Returns the list of manufacture projects.
 * @return, reference to a vector of strings as the list of manufacture projects
 */
const std::vector<std::string>& Ruleset::getManufactureList() const
{
	return _manufactureIndex;
}

/**
 * Generates and returns a list of facilities for custom bases.
 * @note The list contains all the facilities that are listed in the
 * 'startingBase' part of the ruleset.
 * @return, vector of pointers to RuleBaseFacility as the list of facilities for custom bases
 */
std::vector<RuleBaseFacility*> Ruleset::getCustomBaseFacilities() const
{
	std::vector<RuleBaseFacility*> placeList;
	std::string type;
	RuleBaseFacility* fac;

	for (YAML::const_iterator
			i = _startingBase["facilities"].begin();
			i != _startingBase["facilities"].end();
			++i)
	{
		type = (*i)["type"].as<std::string>();
		fac = getBaseFacility(type);
		if (fac->isLift() == false)
			placeList.push_back(fac);
	}

	return placeList;
}

/**
 * Returns the data for the specified ufo trajectory.
 * @param id - reference the UfoTrajectory id
 * @return, a pointer to the data in specified UfoTrajectory
 */
const UfoTrajectory* Ruleset::getUfoTrajectory(const std::string& id) const
{
	std::map<std::string, UfoTrajectory*>::const_iterator i (_ufoTrajectories.find(id));

	if (i != _ufoTrajectories.end())
		return i->second;

	return nullptr;
}

/**
 * Returns the rules for the specified alien mission.
 * @param id - alien mission type
 * @return, pointer to Rules for the AlienMission
 */
const RuleAlienMission* Ruleset::getAlienMission(const std::string& id) const
{
	std::map<std::string, RuleAlienMission*>::const_iterator i (_alienMissions.find(id));
	if (i != _alienMissions.end())
		return i->second;

	return nullptr;
}

/**
 * Returns the rules for a random alien mission based on a specific objective.
 * @param objective		- alien mission objective
 * @param monthsPassed	- the number of months since game start
 * @return, pointer to RuleAlienMission
 */
const RuleAlienMission* Ruleset::getRandomMission(
		MissionObjective objective,
		size_t monthsPassed) const
{
	int totalWeight = 0;

	std::map<int, RuleAlienMission*> eligibleMissions;
	for (std::map<std::string, RuleAlienMission*>::const_iterator
			i = _alienMissions.begin();
			i != _alienMissions.end();
			++i)
	{
		if (i->second->getObjective() == objective
			&& i->second->getWeight(monthsPassed) > 0)
		{
			totalWeight += i->second->getWeight(monthsPassed);
			eligibleMissions[totalWeight] = i->second;
		}
	}

	const int pick (RNG::generate(1, totalWeight));
	for (std::map<int, RuleAlienMission*>::const_iterator
			i = eligibleMissions.begin();
			i != eligibleMissions.end();
			++i)
	{
		if (pick <= i->first)
			return i->second;
	}

	return nullptr;
}

/**
 * Returns the list of alien mission types.
 * @return, reference to a vector of strings as the list of AlienMissions
 */
const std::vector<std::string>& Ruleset::getAlienMissionList() const
{
	return _alienMissionsIndex;
}

/**
 * Gets the alien item level table - a two dimensional array.
 * @return, reference to a vector of vectors containing the alien item levels
 */
const std::vector<std::vector<int>>& Ruleset::getAlienItemLevels() const
{
	return _alienItemLevels;
}

/**
 * Gets the pre-defined starting base.
 * @return, reference to a default base
 */
const YAML::Node& Ruleset::getStartingBase() const
{
	return _startingBase;
}

/**
 * Gets the pre-defined start time of a game.
 * @return, address of the time a game starts
 */
const GameTime& Ruleset::getStartingTime() const
{
	return _startingTime;
}

/**
 * Gets an MCDPatch.
 * @param id - the ID of the MCDPatch
 * @return, pointer to the MCDPatch based on ID or nullptr if none defined
 */
MCDPatch* Ruleset::getMCDPatch(const std::string& id) const
{
	std::map<std::string, MCDPatch*>::const_iterator i (_MCDPatches.find(id));
	if (i != _MCDPatches.end())
		return i->second;

	return nullptr;
}

/**
 * Gets the list of external music rules.
 * @return, vector of pairs of strings & pointers to RuleMusic
 */
std::vector<std::pair<std::string, RuleMusic*>> Ruleset::getMusicTracks() const
{
	return _music;
}

/**
 * Gets the list of external sprites.
 * @return, vector of pairs of strings & pointers to ExtraSprites
 */
std::vector<std::pair<std::string, ExtraSprites*>> Ruleset::getExtraSprites() const
{
	return _extraSprites;
}

/**
 * Gets the list of external sounds.
 * @return, vector of pairs of strings & pointers to ExtraSounds
 */
std::vector<std::pair<std::string, ExtraSounds*>> Ruleset::getExtraSounds() const
{
	return _extraSounds;
}
/**
 * Gets the list of external music.
 * @return, vector of pairs of strings & pointers to ExtraMusic
 */
/* std::vector<std::pair<std::string, ExtraMusic*>> Ruleset::getExtraMusic() const // sza_ExtraMusic
{
	return _extraMusic;
} */

/**
 * Gets the list of external strings.
 * @return, map of strings & pointers to ExtraStrings
 */
std::map<std::string, ExtraStrings*> Ruleset::getExtraStrings() const
{
	return _extraStrings;
}

/*
 * Gets the list of StatStrings.
 * @return, vector of pointers to StatStrings
 *
std::vector<StatString*> Ruleset::getStatStrings() const
{
	return _statStrings;
} */

/**
 * Compares rules based on their list orders.
 */
template<typename T>
struct compareRule
	:
		public std::binary_function<const std::string&, const std::string&, bool>
{
	Ruleset* _ruleset;
	typedef T*(Ruleset::*RuleLookup)(const std::string& id);
	RuleLookup _lookup;

	compareRule(
			Ruleset* const ruleset,
			const RuleLookup lookup)
		:
			_ruleset(ruleset),
			_lookup(lookup)
	{}

	bool operator()(
			const std::string& r1,
			const std::string& r2) const
	{
		const T
			* const rule1 ((_ruleset->*_lookup)(r1)),
			* const rule2 ((_ruleset->*_lookup)(r2));

		return (rule1->getListOrder() < rule2->getListOrder());
	}
};

/**
 * Craft weapons use the list order of their launcher item.
 */
template<>
struct compareRule<RuleCraftWeapon>
	:
		public std::binary_function<const std::string&, const std::string&, bool>
{
	Ruleset* _ruleset;

	compareRule(Ruleset* ruleset)
		:
			_ruleset(ruleset)
	{}

	bool operator()(
			const std::string& r1,
			const std::string& r2) const
	{
		const RuleItem
			* const rule1 (_ruleset->getItemRule(_ruleset->getCraftWeapon(r1)->getLauncherType())),
			* const rule2 (_ruleset->getItemRule(_ruleset->getCraftWeapon(r2)->getLauncherType()));

		return (rule1->getListOrder() < rule2->getListOrder());
	}
};

/**
 * Armor uses the list order of their store item.
 * @note Itemless armor comes before all else.
 */
template<>
struct compareRule<RuleArmor>
	:
		public std::binary_function<const std::string&, const std::string&, bool>
{
	Ruleset* _ruleset;

	compareRule(Ruleset* const ruleset)
		:
			_ruleset(ruleset)
	{}

	bool operator()(
			const std::string& r1,
			const std::string& r2) const
	{
		const RuleArmor
			* const armorRule1 (_ruleset->getArmor(r1)),
			* const armorRule2 (_ruleset->getArmor(r2));
		const RuleItem
			* const itRule1 (_ruleset->getItemRule(armorRule1->getStoreItem())),
			* const itRule2 (_ruleset->getItemRule(armorRule2->getStoreItem()));

/*		if (itRule1 == nullptr && itRule2 == nullptr)
//			return (armorRule1 < armorRule2); // tiebreaker, don't care about order, pointers are good as any.
			return false; // just return true, no false.

		if (itRule1 == nullptr)
			return true;

		if (itRule2 == nullptr)
			return false; */

		if (itRule2 == nullptr)
			return false;

		if (itRule1 == nullptr)
			return true;

		return (itRule1->getListOrder() < itRule2->getListOrder());
	}
};

/**
 * Ufopaedia articles use section and list order.
 */
template<>
struct compareRule<ArticleDefinition>
	:
		public std::binary_function<const std::string&, const std::string&, bool>
{
	Ruleset* _ruleset;
	static std::map<std::string, int> _sections;
	bool _listOrder;

	compareRule(
			Ruleset* const ruleset,
			bool listOrder)
		:
			_ruleset(ruleset),
			_listOrder(listOrder)
	{
		_sections[UFOPAEDIA_XCOM_CRAFT_ARMAMENT]		=  0;
		_sections[UFOPAEDIA_HEAVY_WEAPONS_PLATFORMS]	=  1;
		_sections[UFOPAEDIA_WEAPONS_AND_EQUIPMENT]		=  2;
		_sections[UFOPAEDIA_ALIEN_ARTIFACTS]			=  3;
		_sections[UFOPAEDIA_BASE_FACILITIES]			=  4;
		_sections[UFOPAEDIA_ALIEN_LIFE_FORMS]			=  5;
		_sections[UFOPAEDIA_ALIEN_RESEARCH]				=  6;
		_sections[UFOPAEDIA_UFO_COMPONENTS]				=  7;
		_sections[UFOPAEDIA_UFOS]						=  8;
		_sections[UFOPAEDIA_AWARDS]						=  9;
		_sections[UFOPAEDIA_NOT_AVAILABLE]				= 10;
	}

	bool operator()(
			const std::string& r1,
			const std::string& r2) const
	{
		const ArticleDefinition
			* const rule1 (_ruleset->getUfopaediaArticle(r1)),
			* const rule2 (_ruleset->getUfopaediaArticle(r2));

//		if (_sections[rule1->section] == _sections[rule2->section])
		if (_listOrder == true)
			return (rule1->getListOrder() < rule2->getListOrder());

		return (_sections[rule1->section] < _sections[rule2->section]);
	}
};

std::map<std::string, int> compareRule<ArticleDefinition>::_sections;

/**
 * Sorts all lists according to their weight.
 */
void Ruleset::sortLists()
{
	std::sort(
			_itemsIndex.begin(),
			_itemsIndex.end(),
			compareRule<RuleItem>(
							this,
							(compareRule<RuleItem>::RuleLookup)& Ruleset::getItemRule));
	std::sort(
			_craftsIndex.begin(),
			_craftsIndex.end(),
			compareRule<RuleCraft>(
							this,
							(compareRule<RuleCraft>::RuleLookup)& Ruleset::getCraft));
	std::sort(
			_facilitiesIndex.begin(),
			_facilitiesIndex.end(),
			compareRule<RuleBaseFacility>(
							this,
							(compareRule<RuleBaseFacility>::RuleLookup)& Ruleset::getBaseFacility));
	std::sort(
			_researchIndex.begin(),
			_researchIndex.end(),
			compareRule<RuleResearch>(
							this,
							(compareRule<RuleResearch>::RuleLookup)& Ruleset::getResearch));
	std::sort(
			_manufactureIndex.begin(),
			_manufactureIndex.end(),
			compareRule<RuleManufacture>(
							this,
							(compareRule<RuleManufacture>::RuleLookup)& Ruleset::getManufacture));
	std::sort(
			_invsIndex.begin(),
			_invsIndex.end(),
			compareRule<RuleInventory>(
							this,
							(compareRule<RuleInventory>::RuleLookup)& Ruleset::getInventory));

	// special cases
	std::sort(
			_craftWeaponsIndex.begin(),
			_craftWeaponsIndex.end(),
			compareRule<RuleCraftWeapon>(this));
	std::sort(
			_armorsIndex.begin(),
			_armorsIndex.end(),
			compareRule<RuleArmor>(this));

	std::sort( // sort by listOrder first
			_ufopaediaIndex.begin(),
			_ufopaediaIndex.end(),
			compareRule<ArticleDefinition>(
										this,
										true));
/*	std::sort( // sort by sectionOrder second
			_ufopaediaIndex.begin(),
			_ufopaediaIndex.end(),
			compareRule<ArticleDefinition>(
										this,
										false)); */
}

/**
 * Gets the research-requirements for Psi-Lab (it's a cache for psiStrengthEval)
 * @return, vector of strings that are psi requirements
 *
std::vector<std::string> Ruleset::getPsiRequirements() const
{
	return _psiRequirements;
} */

/**
 * Creates a new randomly-generated soldier.
 * @param gameSave	- pointer to SavedGame
 * @param type		- the soldier type to generate (default "")
 * @return, pointer to the newly generated Soldier
 */
Soldier* Ruleset::genSoldier(
		SavedGame* const gameSave,
		std::string type) const
{
	if (type.empty() == true)
		type = _soldiersIndex.front();

	return new Soldier(
					getSoldier(type),
					getArmor(getSoldier(type)->getArmor()),
					&_names,
					gameSave->getCanonicalId("STR_SOLDIER"));
}
/*	Soldier* soldier = nullptr;
	int newId = save->getId("STR_SOLDIER");
	// Original X-COM gives up after 10 tries so they did the same here
	bool duplicate = true;
	for (int // Check for duplicates
			i = 0;
			i != 10 && duplicate == true;
			++i)
	{
		delete soldier;
		soldier = new Soldier(
							getSoldier("STR_SOLDIER"),
							getArmor("STR_ARMOR_NONE_UC"),
							&_names,
							newId);
		duplicate = false;
		for (std::vector<Base*>::const_iterator
				i = save->getBases()->begin();
				i != save->getBases()->end()
					&& duplicate == false;
				++i)
		{
			for (std::vector<Soldier*>::const_iterator
					j = (*i)->getSoldiers()->begin();
					j != (*i)->getSoldiers()->end()
						&& duplicate == false;
					++j)
			{
				if ((*j)->getName() == soldier->getName())
					duplicate = true;
			}
			for (std::vector<Transfer*>::const_iterator
					j = (*i)->getTransfers()->begin();
					j != (*i)->getTransfers()->end()
						&& duplicate == false;
					++j)
			{
				if ((*j)->getType() == TRANSFER_SOLDIER
					&& (*j)->getSoldier()->getName() == soldier->getName())
				{
					duplicate = true;
				}
			}
		}
	} */
//	soldier->calcStatString( // calculate new statString
//						getStatStrings(),
//						(Options::psiStrengthEval
//							&& save->isResearched(getPsiRequirements())));
//	return soldier;

/**
 * Gets the name of the item to be used as alien fuel (elerium or zyrbite).
 * @return, the name of the fuel
 */
const std::string& Ruleset::getAlienFuelType() const
{
	return _alienFuel.first;
}

/**
 * Gets the amount of alien fuel to recover.
 * @return, the amount to recover
 */
int Ruleset::getAlienFuelQuantity() const
{
	return _alienFuel.second;
}

/**
 * Gets the name of the font collection.
 * @return, the name of font data file
 */
std::string Ruleset::getFontName() const
{
	return _font;
}

/**
 * Gets minimum radar range out of all facilities.
 * @return, the minimum range
 *
int Ruleset::getMinRadarRange() const
{
	static int minRadarRange = -1;

	if (minRadarRange < 0)
	{
		minRadarRange = 0;
		for (std::vector<std::string>::const_iterator
				i = _facilitiesIndex.begin();
				i != _facilitiesIndex.end();
				++i)
		{
			RuleBaseFacility* facRule = getBaseFacility(*i);
			if (facRule == nullptr)
				continue;

			int radarRange = facRule->getRadarRange();
			if (radarRange > 0
				&& (minRadarRange == 0
					|| minRadarRange > radarRange))
			{
				minRadarRange = radarRange;
			}
		}
	}

	return minRadarRange;
} */

/**
 * Gets maximum radar range out of all facilities.
 * @return, maximum range
 */
int Ruleset::getMaxRadarRange() const
{
	int ret (0);
	int range;

	const RuleBaseFacility* facRule;
	for (std::vector<std::string>::const_iterator
			i = _facilitiesIndex.begin();
			i != _facilitiesIndex.end();
			++i)
	{
		if ((facRule = getBaseFacility(*i)) != nullptr)
		{
			range = facRule->getRadarRange();
			if (range > ret)
				ret = range;
		}
	}
	return ret;
}

/**
 * Gets the cutoff between small & large radars for determining base info values.
 * @return, range boundary between small & large radars
 */
int Ruleset::getRadarCutoffRange() const
{
	return _radarCutoff;
}

/**
 * Gets the turn aliens are allowed to throw their first grenades.
 * @return, first grenade turn
 */
int Ruleset::getFirstGrenade() const
{
	return _firstGrenade;
}

/**
 * Gets the basic retaliation chance.
 * @return, basic retaliation chance
 */
int Ruleset::getRetaliation() const
{
	return _retalCoef;
}

/**
 * Gets information on an interface.
 * @param id - reference the interface for info
 * @return, pointer to RuleInterface
 */
RuleInterface* Ruleset::getInterface(const std::string& id) const
{
	std::map<std::string, RuleInterface*>::const_iterator i (_interfaces.find(id));
	if (i != _interfaces.end())
		return i->second;

	return nullptr;
}

/**
 * Gets the rules for the Geoscape globe.
 * @return, pointer to RuleGlobe
 */
RuleGlobe* Ruleset::getGlobe() const
{
	return _globe;
}

/**
 * Gets the sound definition rules.
 * @return, map of strings & pointers to SoundDefinition
 *
const std::map<std::string, SoundDefinition*>* Ruleset::getSoundDefinitions() const
{
	return &_soundDefs;
} */

/**
 * Gets the list of videos.
 * @return, pointer to a map of strings & pointers to RuleVideos
 */
const std::map<std::string, RuleVideo*>* Ruleset::getVideos() const
{
	return &_videos;
}

/**
 * Gets the mission scripts.
 * @return, pointer to a vector of strings of RuleMissionScript types
 */
const std::vector<std::string>* Ruleset::getMissionScriptList() const
{
	return &_missionScriptIndex;
}

/**
 * Gets a specific mission script rule.
 * @param type - reference a mission-script type
 * @return, pointer to the RuleMissionScript
 */
RuleMissionScript* Ruleset::getMissionScript(const std::string& type) const
{
	std::map<std::string, RuleMissionScript*>::const_iterator i (_missionScripts.find(type));
	if (i != _missionScripts.end())
		return i->second;

	return nullptr;
}

/**
 * Gets the list of MapScripts.
 * @param type - reference the map-script type
 * @return, pointer to a vector of pointers to MapScript
 */
const std::vector<MapScript*>* Ruleset::getMapScript(const std::string& type) const
{
	std::map<std::string, std::vector<MapScript*>>::const_iterator i (_mapScripts.find(type));
	if (i != _mapScripts.end())
	{
		//Log(LOG_INFO) << "rules: i->first = " << i->first;
		return &i->second;
	}

	//Log(LOG_ERROR) << "Map Script " << type << "not found";
	return nullptr;
}

/**
 * Gets the final research Id.
 * @return, final research id
 */
const std::string& Ruleset::getFinalResearch() const
{
	return _finalResearch;
}

/**
 * Gets the current Game.
 * @return, pointer to GAME
 */
const Game* Ruleset::getGame() const
{
	return _game;
}

}
