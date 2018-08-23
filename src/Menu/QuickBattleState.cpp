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

#include "QuickBattleState.h"

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

#include "../Menu/ErrorMessageState.h"

#include "../Resource/XcomResourcePack.h"

#include "../Ruleset/RuleAlienDeployment.h"
#include "../Ruleset/RuleAlienMission.h"
//#include "../Ruleset/RuleCraft.h"
#include "../Ruleset/RuleCraftWeapon.h"
#include "../Ruleset/RuleGlobe.h"
#include "../Ruleset/RuleInterface.h"
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
#include "../Savegame/Vehicle.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the New Battle window.
 */
QuickBattleState::QuickBattleState()
	:
		_craft(nullptr),
		_base(nullptr),
		_playSave(nullptr),
		_rules(_game->getRuleset()),
		_missionTypes(_game->getRuleset()->getDeploymentsList())
{
	_window				= new Window(this, 320, 200, 0,0, POPUP_BOTH);

	_txtTitle			= new Text(320, 17, 0, 9);

	_txtMapOptions		= new Text(148, 9, 8, 68);
	_txtAlienOptions	= new Text(148, 9, 164, 68);

	_frameL				= new Frame(148, 96, 8, 78);
	_frameR				= new Frame(148, 96, 164, 78);

	_txtMission			= new Text(100, 9, 8, 30);
	_cbxMission			= new ComboBox(this, 214, 16, 98, 26);

	_txtCraft			= new Text(100, 9, 8, 50);
	_cbxCraft			= new ComboBox(this, 106, 16, 98, 46);
	_btnCraft			= new TextButton(106, 16, 206, 46);

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

	_btnCancel			= new TextButton(75, 16,  10, 176);
	_btnGenerate		= new TextButton(75, 16,  85, 176);
	_btnRandom			= new TextButton(75, 16, 160, 176);
	_btnOk				= new TextButton(75, 16, 235, 176);

	setInterface("newBattleMenu");

	add(_window,			"window",	"newBattleMenu");
	add(_txtTitle,			"heading",	"newBattleMenu");
	add(_txtMapOptions,		"heading",	"newBattleMenu");
	add(_frameL,			"frames",	"newBattleMenu");
	add(_txtAlienOptions,	"heading",	"newBattleMenu");
	add(_frameR,			"frames",	"newBattleMenu");

	add(_txtMission,		"text",		"newBattleMenu");
	add(_txtCraft,			"text",		"newBattleMenu");
	add(_btnCraft,			"button1",	"newBattleMenu");

	add(_txtDarkness,		"text",		"newBattleMenu");
	add(_slrDarkness,		"button1",	"newBattleMenu");
	add(_txtTerrain,		"text",		"newBattleMenu");
	add(_txtDifficulty,		"text",		"newBattleMenu");
	add(_txtAlienRace,		"text",		"newBattleMenu");
	add(_txtAlienTech,		"text",		"newBattleMenu");
	add(_slrAlienTech,		"button1",	"newBattleMenu");

	add(_btnCancel,			"button2",	"newBattleMenu");
	add(_btnGenerate,		"button2",	"newBattleMenu");
	add(_btnRandom,			"button2",	"newBattleMenu");
	add(_btnOk,				"button2",	"newBattleMenu");

	add(_cbxTerrain,		"button1",	"newBattleMenu");
	add(_cbxAlienRace,		"button1",	"newBattleMenu");
	add(_cbxDifficulty,		"button1",	"newBattleMenu");
	add(_cbxCraft,			"button1",	"newBattleMenu");
	add(_cbxMission,		"button1",	"newBattleMenu");

	centerSurfaces();


	_window->setBackground(_game->getResourcePack()->getSurface("BACK01.SCR"));

	_txtTitle->setText(tr("STR_MISSION_GENERATOR"));
	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setBig();

	_frameL->setThickness(3);
	_frameR->setThickness(3);

	_txtMapOptions->	setText(tr("STR_MAP_OPTIONS"));
	_txtAlienOptions->	setText(tr("STR_ALIEN_OPTIONS"));
	_txtMission->		setText(tr("STR_MISSION"));
	_txtCraft->			setText(tr("STR_CRAFT"));
	_txtDarkness->		setText(tr("STR_MAP_DARKNESS"));
	_txtTerrain->		setText(tr("STR_MAP_TERRAIN"));
	_txtDifficulty->	setText(tr("STR_DIFFICULTY"));
	_txtAlienRace->		setText(tr("STR_ALIEN_RACE"));
	_txtAlienTech->		setText(tr("STR_ALIEN_TECH_LEVEL"));

	_cbxMission->setOptions(_missionTypes);
	_cbxMission->setBackgroundFill(BROWN_D);
	_cbxMission->onComboChange(static_cast<ActionHandler>(&QuickBattleState::cbxMissionChange));

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
	_cbxCraft->onComboChange(static_cast<ActionHandler>(&QuickBattleState::cbxCraftChange));

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
	_cbxAlienRace->setOptions(_alienRaces);
	_cbxAlienRace->setBackgroundFill(BROWN_D);

	_slrAlienTech->setRange(0,
							static_cast<int>(_rules->getAlienItemLevels().size()) - 1);

	_cbxTerrain->setBackgroundFill(BROWN_D);

	_btnCraft->setText(tr("STR_EQUIP_CRAFT"));
	_btnCraft->onMouseClick(static_cast<ActionHandler>(&QuickBattleState::btnCraftClick));

	_btnGenerate->setText(tr("STR_GENERATE"));
	_btnGenerate->onMouseClick(static_cast<ActionHandler>(&QuickBattleState::btnGenerateClick));

	_btnRandom->setText(tr("STR_RANDOMIZE"));
	_btnRandom->onMouseClick(static_cast<ActionHandler>(&QuickBattleState::btnRandClick));

	_btnOk->setText(tr("STR_COMBAT_UC"));
	_btnOk->onMouseClick(	static_cast<ActionHandler>(&QuickBattleState::btnOkClick));
	_btnOk->onKeyboardPress(static_cast<ActionHandler>(&QuickBattleState::btnOkClick),
							Options::keyOk);
	_btnOk->onKeyboardPress(static_cast<ActionHandler>(&QuickBattleState::btnOkClick),
							Options::keyOkKeypad);

	_btnCancel->setText(tr("STR_CANCEL"));
	_btnCancel->onMouseClick(	static_cast<ActionHandler>(&QuickBattleState::btnCancelClick));
	_btnCancel->onKeyboardPress(static_cast<ActionHandler>(&QuickBattleState::btnCancelClick),
								Options::keyCancel);

	configLoad();
}

/**
 * dTor.
 */
QuickBattleState::~QuickBattleState()
{}

/**
 * Resets the menu music and savegame when coming back from the battlescape.
 */
void QuickBattleState::init()
{
	State::init();

	if (_craft == nullptr)
		configLoad();
}

/**
 * Loads configuration from a YAML file.
 * @param file - reference a YAML filename (default "battle")
 */
void QuickBattleState::configLoad(const std::string& file)
{
	const std::string& config (Options::getConfigFolder() + file + ".cfg");

	if (CrossPlatform::fileExists(config) == false)
		configCreate();
	else
	{
		try
		{
			YAML::Node doc (YAML::LoadFile(config));

			if (doc["rng"] && Options::reSeedOnLoad == false)
				RNG::setSeed(doc["rng"].as<uint64_t>());
			else
				RNG::setSeed(); // savescrub.

			if (doc["base"])
			{
				_playSave = new SavedGame(_rules);
				_game->setSavedGame(_playSave);

				_base = new Base(_rules, _playSave);
				_base->loadBase(doc["base"]);
				_playSave->getBases()->push_back(_base);

				if (_base->getCrafts()->empty() == true) // NOTE: This had better just basically never happen.
				{
					const std::string& craftType (_crafts[_cbxCraft->getSelected()]);
					_craft = new Craft(
									_rules->getCraft(craftType),
									_base,
									_playSave);
					_base->getCrafts()->push_back(_craft);
				}
				else // fix potentially invalid contents. Or this.
				{
					_craft = _base->getCrafts()->front();
					for (std::map<std::string, int>::iterator
							i = _craft->getCraftItems()->getContents()->begin();
							i != _craft->getCraftItems()->getContents()->end();
							++i)
					{
						if (_rules->getItemRule(i->first) == nullptr)
							i->second = 0;
					}
				}

				resetBaseStores();
				resetResearchGenerals();
			}
			else
				configCreate();

			_cbxMission->setSelected(std::min(
										doc["mission"].as<size_t>(0u),
										_missionTypes.size() - 1u));
			cbxMissionChange(nullptr); // NOTE: The '_base' must be instantiated first.

			_cbxCraft->setSelected(std::min(
										doc["craft"].as<size_t>(0u),
										_crafts.size() - 1u));
			_slrDarkness->setValue(doc["darkness"].as<int>(0));
			_cbxTerrain->setSelected(std::min(
										doc["terrain"].as<size_t>(0u),
										_terrainTypes.size() - 1u));
			_cbxAlienRace->setSelected(std::min(
											doc["alienRace"].as<size_t>(0u),
											_alienRaces.size() - 1u));
			_cbxDifficulty->setSelected(doc["difficulty"].as<size_t>(0u));
			_slrAlienTech->setValue(doc["alienTech"].as<int>(0));
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
void QuickBattleState::configSave(const std::string& file)
{
	const std::string& config (Options::getConfigFolder() + file + ".cfg");

	std::ofstream ofstr (config.c_str());
	if (ofstr.fail() == true)
	{
		Log(LOG_WARNING) << "Failed to save " << file << ".cfg";
		return;
	}

	YAML::Emitter out;

	YAML::Node node;
	node["mission"]		= _cbxMission->getSelected();
	node["craft"]		= _cbxCraft->getSelected();
	node["darkness"]	= _slrDarkness->getValue();
	node["terrain"]		= _cbxTerrain->getSelected();
	node["alienRace"]	= _cbxAlienRace->getSelected();
	node["difficulty"]	= _cbxDifficulty->getSelected();
	node["alienTech"]	= _slrAlienTech->getValue();
	node["base"]		= _base->save();
	node["rng"]			= RNG::getSeed();
	out << node;

	ofstr << out.c_str();
	ofstr.close();
}

/**
 * Creates a SavedGame with everything necessary.
 */
void QuickBattleState::configCreate()
{
	RNG::setSeed();

	_playSave = new SavedGame(_rules);
	_game->setSavedGame(_playSave);

	_base = new Base(_rules, _playSave);

	_base->loadBase(_rules->getStartBase());
	_playSave->getBases()->push_back(_base);
	_base->setLabel(L"tactical");

	for (std::vector<Craft*>::const_iterator	// Clear and generate Craft.
			i = _base->getCrafts()->begin();	// NOTE: There's actually only ever 1 Craft at the quick-base.
			i != _base->getCrafts()->end();
			++i)
	{
		delete *i;
	}
	_base->getCrafts()->clear();

	_craft = new Craft(
					_rules->getCraft(_crafts[_cbxCraft->getSelected()]),
					_base,
					_playSave);
	_base->getCrafts()->push_back(_craft);

	_base->setEngineers(0);
	_base->setScientists(0);

	resetSoldiers();
	resetBaseStores();
	resetResearchGenerals();

	cbxMissionChange(nullptr); // NOTE: The '_base' must be instantiated first.
}

/**
 * Clears and generates the Soldiers.
 */
void QuickBattleState::resetSoldiers()
{
	for (std::vector<Soldier*>::const_iterator
			i = _base->getSoldiers()->begin();
			i != _base->getSoldiers()->end();
			++i)
	{
		delete *i;
	}
	_base->getSoldiers()->clear();

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
											_playSave,
											_rules->getSoldiersList().at(RNG::pick(_rules->getSoldiersList().size()))));
		_base->getSoldiers()->push_back(sol);

		hasRookieStats = true;

		stats = sol->getCurrentStats();
		for (int
				j = 0;		// promote rank and stats
				j != 13;	// arbitrary iterations ...
				++j)
		{
			if (RNG::percent(11 - static_cast<int>(sol->getRank()))
				&&  sol->getRank() != RANK_COMMANDER
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

				stats->tu			+= RNG::generate(0, 6);
				stats->stamina		+= RNG::generate(0, 6);
				stats->health		+= RNG::generate(0, 5);
				stats->bravery		+= RNG::generate(0, 5);
				stats->reactions	+= RNG::generate(0, 5);
				stats->firing		+= RNG::generate(0, 7);
				stats->throwing		+= RNG::generate(0, 6);
				stats->strength		+= RNG::generate(0, 8);
				stats->melee		+= RNG::generate(0, 5);
				stats->psiStrength	+= RNG::generate(0, 4);
				stats->psiSkill		+= RNG::generate(0,20);
			}
		}

		if (hasRookieStats == false)
		{
			stats->bravery = stats->bravery / 10 * 10;

			if (stats->tu			> statCaps.tu)			stats->tu			= statCaps.tu;
			if (stats->stamina		> statCaps.stamina)		stats->stamina		= statCaps.stamina;
			if (stats->health		> statCaps.health)		stats->health		= statCaps.health;
			if (stats->bravery		> statCaps.bravery)		stats->bravery		= statCaps.bravery;
			if (stats->reactions	> statCaps.reactions)	stats->reactions	= statCaps.reactions;
			if (stats->firing		> statCaps.firing)		stats->firing		= statCaps.firing;
			if (stats->throwing		> statCaps.throwing)	stats->throwing		= statCaps.throwing;
			if (stats->strength		> statCaps.strength)	stats->strength		= statCaps.strength;
			if (stats->melee		> statCaps.melee)		stats->melee		= statCaps.melee;
			if (stats->psiStrength	> statCaps.psiStrength)	stats->psiStrength	= statCaps.psiStrength;
			if (stats->psiSkill		> statCaps.psiSkill)	stats->psiSkill		= statCaps.psiSkill;
		}

		sol->autoStat();
		sol->setQuickBattle();
	}

	_base->sortSoldiers();
}

/**
 * Clears and generates Base storage-items.
 */
void QuickBattleState::resetBaseStores() const // private.
{
	//Log(LOG_INFO) << "";
	//Log(LOG_INFO) << "QuickBattleState::resetBaseStores()";
	_base->getStorageItems()->getContents()->clear();

	const RuleCraftWeapon* cwRule;
	const std::vector<std::string>& cwList (_rules->getCraftWeaponsList());
	bool craftOrdnance;

	const RuleItem* itRule;
	const std::vector<std::string>& allItems (_rules->getItemsList());
	for (std::vector<std::string>::const_iterator
			i = allItems.begin();
			i != allItems.end();
			++i)
	{
		if (_rules->getAlienFuelType() != *i)
		{
			craftOrdnance = false;
			for (std::vector<std::string>::const_iterator
					j = cwList.begin();
					j != cwList.end();
					++j)
			{
				cwRule = _rules->getCraftWeapon(*j);
				if (cwRule->getLauncherType() == *i)
				{
					craftOrdnance = true;
					break;
				}

				if (cwRule->getClipType() == *i)
				{
					craftOrdnance = true;
					break;
				}
			}

			if (craftOrdnance == false)
			{
				itRule = _rules->getItemRule(*i);
				if (   itRule->getBattleType() != BT_CORPSE
					&& itRule->isLiveAlien() == false
					&& itRule->isRecoverable() == true
					&& itRule->getTileType() == TILE) // ie. is NOT AlienHabitat or whatnot.
				{
					_base->getStorageItems()->addItem(*i);
				}
			}
		}
	}
}

/**
 * Clears and generates all ResearchGenerals.
 */
void QuickBattleState::resetResearchGenerals() const // private.
{
	for (std::vector<ResearchGeneral*>::const_iterator
			i = _playSave->getResearchGenerals().begin();
			i != _playSave->getResearchGenerals().end();
			++i)
	{
		delete *i;
	}
	_playSave->getResearchGenerals().clear();

	for (std::vector<std::string>::const_iterator
			i = _rules->getResearchList().begin();
			i != _rules->getResearchList().end();
			++i)
		_playSave->getResearchGenerals().push_back(new ResearchGeneral(
																	_rules->getResearch(*i),
																	true));
}

/**
 * Starts the battle.
 * @param action - pointer to an Action
 */
void QuickBattleState::btnOkClick(Action*)
{
	configSave();

	if (_craft->getQtySoldiers() == 0)
//		&& _craft->getQtyVehicles() == 0
//		&& _missionTypes[_cbxMission->getSelected()] != "STR_BASE_DEFENSE")
	{
		const RuleInterface* const uiRule (_game->getRuleset()->getInterface("newBattleMenu"));
		_game->pushState(new ErrorMessageState(
											tr("STR_ERROR_MUST_HAVE_SOLDIER"),
											_palette,
											uiRule->getElement("errorMessage")->color,
											"BACK01.SCR",
											uiRule->getElement("errorPalette")->color));
	}
	else
	{
		SavedBattleGame* const battleSave (new SavedBattleGame(
															_playSave,
															nullptr, // &_rules->getOperations(),
															_rules));
		_playSave->setBattleSave(battleSave);
		battleSave->setTacticalType(_missionTypes[_cbxMission->getSelected()]);

		BattlescapeGenerator bGen = BattlescapeGenerator(_game);
		bGen.setTerrain(_rules->getTerrain(_terrainTypes[_cbxTerrain->getSelected()]));

		if (_missionTypes[_cbxMission->getSelected()] == "STR_BASE_DEFENSE")										// BASE DEFENSE
		{
			_craft = nullptr;
			bGen.setBase(_base);
		}
//		else if (_missionTypes[_cbxMission->getSelected()] == "STR_ALIEN_BASE_ASSAULT")
		else if (_game->getRuleset()->getDeployment(battleSave->getTacticalType())->isAlienBaseDeploy() == true)	// ALIEN BASE ASSAULT
		{
			_base = nullptr;

			_craft->setSpeed();
			bGen.setCraft(_craft);

			AlienBase* const alienBase (new AlienBase(_game->getRuleset()->getDeployment(battleSave->getTacticalType())));
			alienBase->setId(1);
			alienBase->setAlienRace(_alienRaces[_cbxAlienRace->getSelected()]);
			_craft->setTarget(alienBase);
			bGen.setAlienBase(alienBase);

			_playSave->getAlienBases()->push_back(alienBase);
		}
		else if (_rules->getUfo(_missionTypes[_cbxMission->getSelected()]) != nullptr)								// UFO CRASHED or LANDED
		{
			_base = nullptr;

			_craft->setSpeed();
			bGen.setCraft(_craft);

			Ufo* const ufo (new Ufo(
								_rules->getUfo(_missionTypes[_cbxMission->getSelected()]),
								_playSave));
			ufo->setQuickBattle();
			ufo->setId(1);
			_craft->setTarget(ufo);
			bGen.setUfo(ufo);

			if (RNG::percent(50) == true) // TODO: Put this as an option in QuickBattleState.
			{
				battleSave->setTacticalType("STR_UFO_GROUND_ASSAULT");
				ufo->setUfoStatus(Ufo::LANDED);
			}
			else
			{
				battleSave->setTacticalType("STR_UFO_CRASH_RECOVERY");
				ufo->setUfoStatus(Ufo::CRASHED);
			}

			_playSave->getUfos()->push_back(ufo);
		}
		else																										// TERROR SITE
		{
			_base = nullptr;

			_craft->setSpeed();
			bGen.setCraft(_craft);

			const RuleAlienDeployment* const ruleDeploy (_rules->getDeployment(battleSave->getTacticalType()));
			const RuleAlienMission* const mission (_rules->getAlienMission(_rules->getAlienMissionList().front())); // doesn't matter

			TerrorSite* const terrorSite (new TerrorSite(mission, ruleDeploy));
			terrorSite->setId(1);
			terrorSite->setAlienRace(_alienRaces[_cbxAlienRace->getSelected()]);
			_craft->setTarget(terrorSite);
			bGen.setTerrorSite(terrorSite);

			_playSave->getTerrorSites()->push_back(terrorSite);
		}

		_playSave->setDifficulty(static_cast<DifficultyLevel>(_cbxDifficulty->getSelected()));

		bGen.setShade(_slrDarkness->getValue());
		bGen.setAlienRace(_alienRaces[_cbxAlienRace->getSelected()]);
		bGen.setAlienItemlevel(_slrAlienTech->getValue());
		bGen.stage();

		_game->popState(); // <- this
		_game->popState(); // <- quah

		_game->pushState(new BriefingState(_craft, _base));

//		_game->getResourcePack()->fadeMusic(_game, 335); // TODO: sort out musicFade from QuickBattleState to Briefing ...
	}
}

/**
 * Exits to the previous screen.
 * @param action - pointer to an Action
 */
void QuickBattleState::btnCancelClick(Action*)
{
	configSave();

	_game->setSavedGame();
	_game->popState();
}

/**
 * Randomize the state
 * @param action - pointer to an Action
 */
void QuickBattleState::btnRandClick(Action*)
{
//	configCreate();	// <- Do NOT reset Base/Craft/Soldiers here.
	RNG::setSeed();	// TODO: Add buttons to run configCreate() and/or reset RNG-seed.
					// -> else do it the old-fashioned way and delete 'battle.cfg'

	_cbxMission->setSelected(RNG::pick(_missionTypes.size()));
	cbxMissionChange(nullptr);

	_cbxCraft->setSelected(RNG::pick(_crafts.size()));
	cbxCraftChange(nullptr);

	_slrDarkness->setValue(RNG::generate(0,15));

	_cbxTerrain->setSelected(RNG::pick(_terrainTypes.size()));

	_cbxAlienRace->setSelected(RNG::pick(_alienRaces.size()));

	_cbxDifficulty->setSelected(static_cast<size_t>(RNG::generate(0,4)));

	_slrAlienTech->setValue(RNG::generate(0,
							static_cast<int>(_rules->getAlienItemLevels().size()) - 1));
}

/**
 * Shows the CraftInfo screen.
 * @param action - pointer to an Action
 */
void QuickBattleState::btnCraftClick(Action*)
{
	_game->pushState(new CraftInfoState(_base, 0u));
}

/**
 * Generates a new group of Soldiers.
 * @param action - pointer to an Action
 */
void QuickBattleState::btnGenerateClick(Action*)
{
	resetSoldiers();
}

/**
 * Updates Map Options based on the current Mission type.
 * @param action - pointer to an Action
 */
void QuickBattleState::cbxMissionChange(Action*)
{
	//Log(LOG_INFO) << "QuickBattleState::cbxMissionChange()";
	if (_missionTypes[_cbxMission->getSelected()] == "STR_BASE_DEFENSE")
		_base->setQuickDefense();		// allow unlimited support-units; Soldiers and Items should be loaded first by player.
	else if (_base->isQuickDefense() == true)
	{
		_base->setQuickDefense(false);

		for (std::vector<Vehicle*>::const_iterator
				i = _craft->getVehicles()->begin();
				i != _craft->getVehicles()->end();
				++i)
		{
			delete *i;
		}
		_craft->getVehicles()->clear();	// clear all support-units from the Craft; only base-defense allows unlimited support.
	}


	const RuleAlienDeployment* const ruleDeploy (_rules->getDeployment(_missionTypes[_cbxMission->getSelected()]));
	//Log(LOG_INFO) << ". ruleDeploy = " << ruleDeploy->getType();

	std::vector<std::string> // Get terrains associated with this mission
		deployTerrains (ruleDeploy->getDeployTerrains()),
		globeTerrains;

	// debug:
	//for (std::vector<std::string>::const_iterator
	//		i = deployTerrains.begin();
	//		i != deployTerrains.end();
	//		++i)
	//	Log(LOG_INFO) << ". . deployTerrain = " << *i;


	if (deployTerrains.empty() == true)
	{
		//Log(LOG_INFO) << ". . deployTerrains invalid; get globeTerrains w/out Deployment rule";
		globeTerrains = _rules->getGlobe()->getGlobeTerrains("");

		// debug:
		//for (std::vector<std::string>::const_iterator
		//		i = globeTerrains.begin();
		//		i != globeTerrains.end();
		//		++i)
		//	Log(LOG_INFO) << ". . . globeTerrain = " << *i;
	}
	else
	{
		//Log(LOG_INFO) << ". . deployTerrains valid; get globeTerrains w/ Deployment rule";
		globeTerrains = _rules->getGlobe()->getGlobeTerrains(ruleDeploy->getType());

		// debug:
		//for (std::vector<std::string>::const_iterator
		//		i = globeTerrains.begin();
		//		i != globeTerrains.end();
		//		++i)
		//	Log(LOG_INFO) << ". . . globeTerrain = " << *i;
	}

	std::set<std::string> terrains;

	for (std::vector<std::string>::const_iterator
			i = deployTerrains.begin();
			i != deployTerrains.end();
			++i)
	{
		//Log(LOG_INFO) << ". . insert deployTerrain = " << *i;
		terrains.insert(*i);
	}

	for (std::vector<std::string>::const_iterator
		i = globeTerrains.begin();
		i != globeTerrains.end();
		++i)
	{
		//Log(LOG_INFO) << ". . insert globeTerrain = " << *i;
		terrains.insert(*i);
	}

	_terrainTypes.clear();

	std::vector<std::string> terrainOptions;
	for (std::set<std::string>::const_iterator
			i = terrains.begin();
			i != terrains.end();
			++i)
	{
		//Log(LOG_INFO) << ". . insert to _terrainTypes Option = " << *i;
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

	//Log(LOG_INFO) << "QuickBattleState::cbxMissionChange() EXIT";
}

/**
 * Updates Craft accordingly.
 * @param action - pointer to an Action
 */
void QuickBattleState::cbxCraftChange(Action*)
{
	_craft->changeRules(_rules->getCraft(_crafts[_cbxCraft->getSelected()]));

	const int solCap (_craft->getRules()->getSoldierCapacity());
	int solCur (_craft->getQtySoldiers());
	if (solCur > solCap)
	{
		std::vector<Soldier*>* const soldiers (_base->getSoldiers()); // NOTE: Perhaps should not reverse-iterate since AutoStat ran.
		for (std::vector<Soldier*>::const_reverse_iterator
				rit = soldiers->rbegin();
				rit != soldiers->rend() && solCur > solCap;
				++rit)
		{
			if ((*rit)->getCraft() == _craft) // TODO: Remove excess luggage/support units also.
			{
				(*rit)->setCraft(
							nullptr,
							_base,
							true);
				--solCur;
			}
		}
	}
}

}
