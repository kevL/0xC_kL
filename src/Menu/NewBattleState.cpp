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

#include "NewBattleState.h"

//#include <algorithm>
//#include <cmath>
#include <fstream>
//#include <yaml-cpp/yaml.h>

#include "../Basescape/CraftInfoState.h"

#include "../Battlescape/BattlescapeGenerator.h"
#include "../Battlescape/BriefingState.h"

#include "../Engine/Action.h"
#include "../Engine/CrossPlatform.h"
#include "../Engine/Game.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Logger.h"
#include "../Engine/Options.h"
#include "../Engine/RNG.h"

#include "../Interface/ComboBox.h"
#include "../Interface/Frame.h"
#include "../Interface/Slider.h"
#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/Window.h"

#include "../Resource/XcomResourcePack.h"

#include "../Ruleset/RuleAlienDeployment.h"
#include "../Ruleset/RuleAlienMission.h"
//#include "../Ruleset/RuleCraft.h"
#include "../Ruleset/RuleGlobe.h"
#include "../Ruleset/RuleItem.h"
#include "../Ruleset/Ruleset.h"
#include "../Ruleset/RuleSoldier.h"
#include "../Ruleset/RuleTerrain.h"

#include "../Savegame/AlienBase.h"
#include "../Savegame/Base.h"
#include "../Savegame/Craft.h"
#include "../Savegame/ItemContainer.h"
#include "../Savegame/SavedBattleGame.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/Soldier.h"
#include "../Savegame/TerrorSite.h"
#include "../Savegame/Ufo.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the New Battle window.
 */
NewBattleState::NewBattleState()
	:
		_craft(nullptr)
{
	_rules = _game->getRuleset();

	_window				= new Window(this, 320, 200, 0,0, POPUP_BOTH);
	_txtTitle			= new Text(320, 17, 0, 9);

	_txtMapOptions		= new Text(148, 9, 8, 68);
	_frameLeft			= new Frame(148, 96, 8, 78);
	_txtAlienOptions	= new Text(148, 9, 164, 68);
	_frameRight			= new Frame(148, 96, 164, 78);

	_txtMission			= new Text(100, 9, 8, 30);
	_cbxMission			= new ComboBox(this, 214, 16, 98, 26);

	_txtCraft			= new Text(100, 9, 8, 50);
	_cbxCraft			= new ComboBox(this, 106, 16, 98, 46);
	_btnEquip			= new TextButton(106, 16, 206, 46);

	_txtDarkness		= new Text(120, 9, 22, 83);
	_slrDarkness		= new Slider(120, 16, 22, 93);

	_txtTerrain			= new Text(120, 9, 22, 113);
	_cbxTerrain			= new ComboBox(this, 140, 16, 12, 123, 5);

	_txtDifficulty		= new Text(120, 9, 178, 83);
	_cbxDifficulty		= new ComboBox(this, 120, 16, 178, 93);

	_txtAlienRace		= new Text(120, 9, 178, 113);
	_cbxAlienRace		= new ComboBox(this, 120, 16, 178, 123);

	_txtAlienTech		= new Text(120, 9, 178, 143);
	_slrAlienTech		= new Slider(120, 16, 178, 153);

	_btnCancel			= new TextButton(100, 16,   8, 176);
	_btnRandom			= new TextButton(100, 16, 110, 176);
	_btnOk				= new TextButton(100, 16, 212, 176);

	setInterface("newBattleMenu");

	add(_window,			"window",	"newBattleMenu");
	add(_txtTitle,			"heading",	"newBattleMenu");
	add(_txtMapOptions,		"heading",	"newBattleMenu");
	add(_frameLeft,			"frames",	"newBattleMenu");
	add(_txtAlienOptions,	"heading",	"newBattleMenu");
	add(_frameRight,		"frames",	"newBattleMenu");

	add(_txtMission,		"text",		"newBattleMenu");
	add(_txtCraft,			"text",		"newBattleMenu");
	add(_btnEquip,			"button1",	"newBattleMenu");

	add(_txtDarkness,		"text",		"newBattleMenu");
	add(_slrDarkness,		"button1",	"newBattleMenu");
	add(_txtTerrain,		"text",		"newBattleMenu");
	add(_txtDifficulty,		"text",		"newBattleMenu");
	add(_txtAlienRace,		"text",		"newBattleMenu");
	add(_txtAlienTech,		"text",		"newBattleMenu");
	add(_slrAlienTech,		"button1",	"newBattleMenu");

	add(_btnOk,				"button2",	"newBattleMenu");
	add(_btnCancel,			"button2",	"newBattleMenu");
	add(_btnRandom,			"button2",	"newBattleMenu");

	add(_cbxTerrain,		"button1",	"newBattleMenu");
	add(_cbxAlienRace,		"button1",	"newBattleMenu");
	add(_cbxDifficulty,		"button1",	"newBattleMenu");
	add(_cbxCraft,			"button1",	"newBattleMenu");
	add(_cbxMission,		"button1",	"newBattleMenu");

	centerAllSurfaces();


	_window->setBackground(_game->getResourcePack()->getSurface("BACK01.SCR"));

	_txtTitle->setText(tr("STR_MISSION_GENERATOR"));
	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setBig();

	_frameLeft->setThickness(3);
	_frameRight->setThickness(3);

	_txtMapOptions->setText(tr("STR_MAP_OPTIONS"));
	_txtAlienOptions->setText(tr("STR_ALIEN_OPTIONS"));
	_txtMission->setText(tr("STR_MISSION"));
	_txtCraft->setText(tr("STR_CRAFT"));
	_txtDarkness->setText(tr("STR_MAP_DARKNESS"));
	_txtTerrain->setText(tr("STR_MAP_TERRAIN"));
	_txtDifficulty->setText(tr("STR_DIFFICULTY"));
	_txtAlienRace->setText(tr("STR_ALIEN_RACE"));
	_txtAlienTech->setText(tr("STR_ALIEN_TECH_LEVEL"));

	_missionTypes = _rules->getDeploymentsList();
	_cbxMission->setOptions(_missionTypes);
	_cbxMission->setBackgroundFill(BROWN_D);
	_cbxMission->onComboChange((ActionHandler)& NewBattleState::cbxMissionChange);

	const std::vector<std::string>& allCraft (_rules->getCraftsList());
	for (std::vector<std::string>::const_iterator
			i = allCraft.begin();
			i != allCraft.end();
			++i)
	{
		if (_rules->getCraft(*i)->getSoldierCapacity() != 0)
			_crafts.push_back(*i);
	}
	_cbxCraft->setOptions(_crafts);
	_cbxCraft->setBackgroundFill(BROWN_D);
	_cbxCraft->onComboChange((ActionHandler)& NewBattleState::cbxCraftChange);

	_slrDarkness->setRange(0,15);

	std::vector<std::string> difficulty;
	difficulty.push_back("STR_1_BEGINNER");
	difficulty.push_back("STR_2_EXPERIENCED");
	difficulty.push_back("STR_3_VETERAN");
	difficulty.push_back("STR_4_GENIUS");
	difficulty.push_back("STR_5_SUPERHUMAN");
	_cbxDifficulty->setOptions(difficulty);
	_cbxDifficulty->setBackgroundFill(BROWN_D);

	_alienRaces = _rules->getAlienRacesList();
//	for (std::vector<std::string>::const_iterator
//			i = _alienRaces.begin();
//			i != _alienRaces.end();
//			)
//	{
//		if ((*i).find("_UNDERWATER") != std::string::npos)
//			i = _alienRaces.erase(i);
//		else
//			++i;
//	}
	_cbxAlienRace->setOptions(_alienRaces);
	_cbxAlienRace->setBackgroundFill(BROWN_D);

	_slrAlienTech->setRange(
						0,
						static_cast<int>(_rules->getAlienItemLevels().size()) - 1u);

	_cbxTerrain->setBackgroundFill(BROWN_D);

	_btnEquip->setText(tr("STR_EQUIP_CRAFT"));
	_btnEquip->onMouseClick((ActionHandler)& NewBattleState::btnEquipClick);

	_btnRandom->setText(tr("STR_RANDOMIZE"));
	_btnRandom->onMouseClick((ActionHandler)& NewBattleState::btnRandClick);

	_btnOk->setText(tr("STR_COMBAT_UC"));
	_btnOk->onMouseClick((ActionHandler)& NewBattleState::btnOkClick);
	_btnOk->onKeyboardPress(
					(ActionHandler)& NewBattleState::btnOkClick,
					Options::keyOk);
	_btnOk->onKeyboardPress(
					(ActionHandler)& NewBattleState::btnOkClick,
					Options::keyOkKeypad);

	_btnCancel->setText(tr("STR_CANCEL"));
	_btnCancel->onMouseClick((ActionHandler)& NewBattleState::btnCancelClick);
	_btnCancel->onKeyboardPress(
					(ActionHandler)& NewBattleState::btnCancelClick,
					Options::keyCancel);

	configLoad();
}

/**
 * dTor.
 */
NewBattleState::~NewBattleState()
{}

/**
 * Resets the menu music and savegame when coming back from the battlescape.
 */
void NewBattleState::init()
{
	State::init();

	if (_craft == nullptr)
		configLoad();
}

/**
 * Loads configuration from a YAML file.
 * @param file - reference a YAML filename (default "battle")
 */
void NewBattleState::configLoad(const std::string& file)
{
	const std::string config (Options::getConfigFolder() + file + ".cfg");

	if (CrossPlatform::fileExists(config) == false)
		configCreate();
	else
	{
		try
		{
			YAML::Node doc (YAML::LoadFile(config));

			_cbxMission->setSelected(std::min(
										doc["mission"].as<size_t>(0),
										_missionTypes.size() - 1u));
			cbxMissionChange(nullptr);

			_cbxCraft->setSelected(std::min(
										doc["craft"].as<size_t>(0),
										_crafts.size() - 1u));
			_slrDarkness->setValue(doc["darkness"].as<size_t>(0));
			_cbxTerrain->setSelected(std::min(
										doc["terrain"].as<size_t>(0),
										_terrainTypes.size() - 1u));
			_cbxAlienRace->setSelected(std::min(
											doc["alienRace"].as<size_t>(0),
											_alienRaces.size() - 1u));
			_cbxDifficulty->setSelected(doc["difficulty"].as<size_t>(0));
			_slrAlienTech->setValue(doc["alienTech"].as<size_t>(0));

			if (doc["rng"] && Options::reSeedOnLoad == false)
				RNG::setSeed(doc["rng"].as<uint64_t>());
			else
				RNG::setSeed(0u);

			if (doc["base"])
			{
				SavedGame* const gameSave (new SavedGame(_rules));
				_game->setSavedGame(gameSave);

				Base* const base (new Base(_rules));
				base->load(
						doc["base"],
						gameSave); // NOTE: considered as neither a 'firstBase' nor a 'quick-battle' ...
				gameSave->getBases()->push_back(base);

				if (base->getCrafts()->empty() == true)
				{
					const std::string craftType (_crafts[_cbxCraft->getSelected()]);
					_craft = new Craft(
									_rules->getCraft(craftType),
									base,
									gameSave,
									gameSave->getCanonicalId(craftType));
					base->getCrafts()->push_back(_craft);
				}
				else // fix potentially invalid contents
				{
					_craft = base->getCrafts()->front();
					for (std::map<std::string, int>::iterator
							i = _craft->getCraftItems()->getContents()->begin();
							i != _craft->getCraftItems()->getContents()->end();
							++i)
					{
						if (_rules->getItemRule(i->first) == nullptr)
							i->second = 0;
					}
				}

				resetStorage(base);			// Clear and generate Base storage.
				resetResearch(gameSave);	// Add research - setup ResearchGenerals.
			}
			else
				configCreate();
		}
		catch (YAML::Exception& e)
		{
			Log(LOG_WARNING) << e.what();
			configCreate();
		}
	}
}

/**
 * Saves configuration to a YAML file.
 * @param file - reference a YAML filename (default "battle")
 */
void NewBattleState::configSave(const std::string& file)
{
	const std::string config (Options::getConfigFolder() + file + ".cfg");

	std::ofstream ofstr (config.c_str());
	if (ofstr.fail() == true)
	{
		Log(LOG_WARNING) << "Failed to save " << file << ".cfg";
		return;
	}

	YAML::Emitter emit;

	YAML::Node node;
	node["mission"]		= _cbxMission->getSelected();
	node["craft"]		= _cbxCraft->getSelected();
	node["darkness"]	= _slrDarkness->getValue();
	node["terrain"]		= _cbxTerrain->getSelected();
	node["alienRace"]	= _cbxAlienRace->getSelected();
	node["difficulty"]	= _cbxDifficulty->getSelected();
	node["alienTech"]	= _slrAlienTech->getValue();
	node["base"]		= _game->getSavedGame()->getBases()->front()->save();
	node["rng"]			= RNG::getSeed();
	emit << node;

	ofstr << emit.c_str();
	ofstr.close();
}

/**
 * Creates a SavedGame with everything necessary.
 */
void NewBattleState::configCreate()
{
	RNG::setSeed(0u);

	SavedGame* const gameSave (new SavedGame(_rules));
	_game->setSavedGame(gameSave);

	Base* const base (new Base(_rules));

	const YAML::Node& node (_rules->getStartingBase());
	base->load(
			node,
			gameSave,
			true,
			true);
	gameSave->getBases()->push_back(base);
	base->setName(L"tactical");

	// Clear and generate Craft.
	for (std::vector<Craft*>::const_iterator
			i = base->getCrafts()->begin();
			i != base->getCrafts()->end();
			++i)
	{
		delete *i;
	}
	base->getCrafts()->clear();

	_craft = new Craft(
					_rules->getCraft(_crafts[_cbxCraft->getSelected()]),
					base,
					gameSave,
					1);
	base->getCrafts()->push_back(_craft);


	// Clear and generate Soldiers.
	for (std::vector<Soldier*>::const_iterator
			i = base->getSoldiers()->begin();
			i != base->getSoldiers()->end();
			++i)
	{
		delete *i;
	}
	base->getSoldiers()->clear();

	const UnitStats statCaps (_rules->getSoldier("STR_SOLDIER")->getStatCaps());
	UnitStats* stats;
	bool
		hasCdr (false),
		hasRookieStats;

	for (int
			i = 0;
			i != 50; // qty Soldiers on call
			++i)
	{
		Soldier* const sol (_rules->genSoldier(
											gameSave,
											_rules->getSoldiersList().at(RNG::pick(_rules->getSoldiersList().size()))));
		stats = sol->getCurrentStats();

		hasRookieStats = true;
		for (int
				j = 0;
				j != 13; // arbitrary ....
				++j)
		{
			if (RNG::percent(11 - static_cast<int>(sol->getRank()))
				&& sol->getRank() != RANK_COMMANDER
				&& (sol->getRank() != RANK_COLONEL || hasCdr == false))
			{
				sol->promoteRank();
				if (sol->getRank() == RANK_COMMANDER)
					hasCdr = true;
			}

			if (sol->getRank() != RANK_ROOKIE
				&& (RNG::percent(11 + static_cast<int>(sol->getRank()) * 27) == true
					|| hasRookieStats == true))
			{
				hasRookieStats = false;
//				UnitStats* const stats = sol->getCurrentStats();
				stats->tu			+= RNG::generate(0,6);
				stats->stamina		+= RNG::generate(0,6);
				stats->health		+= RNG::generate(0,5);
				stats->bravery		+= RNG::generate(0,5);
				stats->reactions	+= RNG::generate(0,5);
				stats->firing		+= RNG::generate(0,7);
				stats->throwing		+= RNG::generate(0,6);
				stats->strength		+= RNG::generate(0,8);
				stats->melee		+= RNG::generate(0,5);
				stats->psiStrength	+= RNG::generate(0,4);
				stats->psiSkill		+= RNG::generate(0,25);
			}
		}

		if (hasRookieStats == false)
		{
			stats->bravery = stats->bravery / 10 * 10;

			if (stats->tu > statCaps.tu)					stats->tu = statCaps.tu;
			if (stats->stamina > statCaps.stamina)			stats->stamina = statCaps.stamina;
			if (stats->health > statCaps.health)			stats->health = statCaps.health;
			if (stats->bravery > statCaps.bravery)			stats->bravery = statCaps.bravery;
			if (stats->reactions > statCaps.reactions)		stats->reactions = statCaps.reactions;
			if (stats->firing > statCaps.firing)			stats->firing = statCaps.firing;
			if (stats->throwing > statCaps.throwing)		stats->throwing = statCaps.throwing;
			if (stats->strength > statCaps.strength)		stats->strength = statCaps.strength;
			if (stats->melee > statCaps.melee)				stats->melee = statCaps.melee;
			if (stats->psiStrength > statCaps.psiStrength)	stats->psiStrength = statCaps.psiStrength;
			if (stats->psiSkill > statCaps.psiSkill)		stats->psiSkill = statCaps.psiSkill;
		}

		sol->autoStat();

		base->getSoldiers()->push_back(sol);
//		if (i < _craft->getRules()->getSoldiers())
//			sol->setCraft(_craft);
	}
	base->sortSoldiers();


	resetStorage(base);			// Clear and generate Base storage.
	resetResearch(gameSave);	// Add research - setup ResearchGenerals.

	cbxMissionChange(nullptr);
}

/**
 * Clears and generates Base storage-items.
 * @param base - pointer to the base
 */
void NewBattleState::resetStorage(Base* const base) const // private.
{
	base->getStorageItems()->getContents()->clear();

	const RuleItem* itRule;
	const std::vector<std::string>& allItems (_rules->getItemsList());
	for (std::vector<std::string>::const_iterator
			i = allItems.begin();
			i != allItems.end();
			++i)
	{
		itRule = _rules->getItemRule(*i);
		if (   itRule->getBattleType() != BT_CORPSE
			&& itRule->isLiveAlien() == false
			&& itRule->isRecoverable() == true)
		{
			base->getStorageItems()->addItem(*i);
		}
	}
}

/**
 * Generates all research.
 * @note The Game and SavedGame objects take care of deletion.
 * @param gameSave - pointer to the SavedGame
 */
void NewBattleState::resetResearch(SavedGame* const gameSave) const // private.
{
	const std::vector<std::string>& allResearch (_rules->getResearchList());
	for (std::vector<std::string>::const_iterator
			i = allResearch.begin();
			i != allResearch.end();
			++i)
	{
		gameSave->getResearchGenerals().push_back(new ResearchGeneral(
																_rules->getResearch(*i),
																true));
	}
}

/**
 * Starts the battle.
 * @param action - pointer to an Action
 */
void NewBattleState::btnOkClick(Action*)
{
	configSave();

	if (_missionTypes[_cbxMission->getSelected()] != "STR_BASE_DEFENSE"
		&& _craft->getQtySoldiers() == 0)
//		&& _craft->getQtyVehicles() == 0)
	{
		return; // TODO: Popup that tells player why no-workie.
	}

	SavedBattleGame* const battleSave (new SavedBattleGame(
														nullptr, // &_rules->getOperations(),
														_rules));
	_game->getSavedGame()->setBattleSave(battleSave);
	battleSave->setTacticalType(_missionTypes[_cbxMission->getSelected()]);

	BattlescapeGenerator bGen = BattlescapeGenerator(_game);
	bGen.setTerrain(_rules->getTerrain(_terrainTypes[_cbxTerrain->getSelected()]));

	Base* base;
	if (_missionTypes[_cbxMission->getSelected()] == "STR_BASE_DEFENSE")
	{
		base = _craft->getBase();
		bGen.setBase(base);
		_craft = nullptr; // kL_note: may need to remove this for .. some reason.
	}
//	else if (_missionTypes[_cbxMission->getSelected()] == "STR_ALIEN_BASE_ASSAULT")
	else if (_game->getRuleset()->getDeployment(battleSave->getTacticalType())->isAlienBase() == true)
	{
		base = nullptr;

		AlienBase* const aBase (new AlienBase(_game->getRuleset()->getDeployment(battleSave->getTacticalType())));
		aBase->setId(1);
		aBase->setAlienRace(_alienRaces[_cbxAlienRace->getSelected()]);
		_craft->setDestination(aBase);
		bGen.setAlienBase(aBase);

		_game->getSavedGame()->getAlienBases()->push_back(aBase);
	}
	else if (_craft != nullptr
		&& _rules->getUfo(_missionTypes[_cbxMission->getSelected()]) != nullptr)
	{
		base = nullptr;

		Ufo* const ufo (new Ufo(
							_rules->getUfo(_missionTypes[_cbxMission->getSelected()]),
							_game->getSavedGame()));
		ufo->setQuickBattle();
		ufo->setId(1);
		_craft->setDestination(ufo);
		bGen.setUfo(ufo);

		if (RNG::percent(50) == true)
			battleSave->setTacticalType("STR_UFO_GROUND_ASSAULT");
		else
			battleSave->setTacticalType("STR_UFO_CRASH_RECOVERY");

		_game->getSavedGame()->getUfos()->push_back(ufo);
	}
	else
	{
		base = nullptr;

		const RuleAlienDeployment* const ruleDeploy (_rules->getDeployment(battleSave->getTacticalType()));
		const RuleAlienMission* const mission (_rules->getAlienMission(_rules->getAlienMissionList().front())); // doesn't matter
		TerrorSite* const terrorSite (new TerrorSite(
													mission,
													ruleDeploy));
		terrorSite->setId(1);
		terrorSite->setAlienRace(_alienRaces[_cbxAlienRace->getSelected()]);

		_craft->setDestination(terrorSite);

		bGen.setTerrorSite(terrorSite);
		_game->getSavedGame()->getTerrorSites()->push_back(terrorSite);
	}

	if (_craft != nullptr)
	{
		_craft->setSpeed(0);
		bGen.setCraft(_craft);
	}

	_game->getSavedGame()->setDifficulty(static_cast<DifficultyLevel>(_cbxDifficulty->getSelected()));

	bGen.setShade(_slrDarkness->getValue());
	bGen.setAlienRace(_alienRaces[_cbxAlienRace->getSelected()]);
	bGen.setAlienItemlevel(_slrAlienTech->getValue());
	bGen.run();

	_game->popState();
	_game->popState();

	_game->pushState(new BriefingState(_craft, base));

	_craft = nullptr;
//	_game->getResourcePack()->fadeMusic(_game, 335); // TODO: sort out musicFade from NewBattleState to Briefing ...
}

/**
 * Exits to the previous screen.
 * @param action - pointer to an Action
 */
void NewBattleState::btnCancelClick(Action*)
{
	configSave();

	_game->setSavedGame();
	_game->popState();
}

/**
 * Randomize the state
 * @param action - pointer to an Action
 */
void NewBattleState::btnRandClick(Action*)
{
//	configCreate();		// <- Do NOT reset Soldiers/Craft/etc (the Base) here.
	RNG::setSeed(0u);	// TODO: Add buttons to run configCreate() and/or to reset RNG-seed.
						// else do it the old-fashioned way and delete 'battle.cfg' ...

	_cbxMission->setSelected(RNG::pick(_missionTypes.size()));
	cbxMissionChange(nullptr);

	_cbxCraft->setSelected(RNG::pick(_crafts.size()));
	cbxCraftChange(nullptr);

	_slrDarkness->setValue(RNG::generate(0,15));

	_cbxTerrain->setSelected(RNG::pick(_terrainTypes.size()));

	_cbxAlienRace->setSelected(RNG::pick(_alienRaces.size()));

	_cbxDifficulty->setSelected(static_cast<size_t>(RNG::generate(0,4)));

	_slrAlienTech->setValue(RNG::generate(0,
							static_cast<int>(_rules->getAlienItemLevels().size()) - 1u));
}

/**
 * Shows the Craft Info screen.
 * @param action - pointer to an Action
 */
void NewBattleState::btnEquipClick(Action*)
{
	_game->pushState(new CraftInfoState(
									_game->getSavedGame()->getBases()->front(),
									0u));
}

/**
 * Updates Map Options based on the current Mission type.
 * @param action - pointer to an Action
 */
void NewBattleState::cbxMissionChange(Action*)
{
	Log(LOG_INFO) << "NewBattleState::cbxMissionChange()";
	const RuleAlienDeployment* const ruleDeploy (_rules->getDeployment(_missionTypes[_cbxMission->getSelected()]));
	Log(LOG_INFO) << ". ruleDeploy = " << ruleDeploy->getType();

	std::vector<std::string> // Get terrains associated with this mission
		deployTerrains (ruleDeploy->getDeployTerrains()),
		globeTerrains;

	// debug:
	for (std::vector<std::string>::const_iterator
			i = deployTerrains.begin();
			i != deployTerrains.end();
			++i)
		Log(LOG_INFO) << ". . deployTerrain = " << *i;


	if (deployTerrains.empty() == true)
	{
		Log(LOG_INFO) << ". . deployTerrains invalid; get globeTerrains w/out Deployment rule";
		globeTerrains = _rules->getGlobe()->getGlobeTerrains("");

		// debug:
		for (std::vector<std::string>::const_iterator
				i = globeTerrains.begin();
				i != globeTerrains.end();
				++i)
			Log(LOG_INFO) << ". . . globeTerrain = " << *i;
	}
	else
	{
		Log(LOG_INFO) << ". . deployTerrains valid; get globeTerrains w/ Deployment rule";
		globeTerrains = _rules->getGlobe()->getGlobeTerrains(ruleDeploy->getType());

		// debug:
		for (std::vector<std::string>::const_iterator
				i = globeTerrains.begin();
				i != globeTerrains.end();
				++i)
			Log(LOG_INFO) << ". . . globeTerrain = " << *i;
	}

	std::set<std::string> terrains;

	for (std::vector<std::string>::const_iterator
			i = deployTerrains.begin();
			i != deployTerrains.end();
			++i)
	{
		Log(LOG_INFO) << ". . insert deployTerrain = " << *i;
		terrains.insert(*i);
	}

	for (std::vector<std::string>::const_iterator
		i = globeTerrains.begin();
		i != globeTerrains.end();
		++i)
	{
		Log(LOG_INFO) << ". . insert globeTerrain = " << *i;
		terrains.insert(*i);
	}

	_terrainTypes.clear();

	std::vector<std::string> terrainOptions;
	for (std::set<std::string>::const_iterator
			i = terrains.begin();
			i != terrains.end();
			++i)
	{
		Log(LOG_INFO) << ". . insert to _terrainTypes Option = " << *i;
		_terrainTypes.push_back(*i);
		terrainOptions.push_back("MAP_" + *i);
	}

	bool vis (ruleDeploy->getShade() == -1); // Hide controls that don't apply to mission
	_txtDarkness->setVisible(vis);
	_slrDarkness->setVisible(vis);

	vis = _terrainTypes.size() > 1u;
	_txtTerrain->setVisible(vis);
	_cbxTerrain->setVisible(vis);

	_cbxTerrain->setOptions(terrainOptions);
	_cbxTerrain->setSelected(0u);

	Log(LOG_INFO) << "NewBattleState::cbxMissionChange() EXIT";
}

/**
 * Updates Craft accordingly.
 * @param action - pointer to an Action
 */
void NewBattleState::cbxCraftChange(Action*)
{
	_craft->changeRules(_rules->getCraft(_crafts[_cbxCraft->getSelected()]));

	const int maxSoldiers (_craft->getRules()->getSoldierCapacity());
	int curSoldiers (_craft->getQtySoldiers());
	if (curSoldiers > maxSoldiers)
	{
		for (std::vector<Soldier*>::const_reverse_iterator
				rit = _craft->getBase()->getSoldiers()->rbegin();
				rit != _craft->getBase()->getSoldiers()->rend() && curSoldiers > maxSoldiers;
				++rit)
		{
			if ((*rit)->getCraft() == _craft) // TODO: Remove excess luggage/support units also.
			{
				(*rit)->setCraft(
							nullptr,
							_craft->getBase(),
							true);
				--curSoldiers;
			}
		}
	}
}

}
