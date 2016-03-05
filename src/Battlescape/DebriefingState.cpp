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

#ifndef _USE_MATH_DEFINES
#	define _USE_MATH_DEFINES
#endif

#include "DebriefingState.h"

//#include <algorithm>
//#include <sstream>

#include "CannotReequipState.h"
#include "CeremonyDeadState.h"
#include "CeremonyState.h"
#include "DebriefExtraState.h"
#include "NoContainmentState.h"
#include "PromotionsState.h"

#include "../Basescape/AlienContainmentState.h"
#include "../Basescape/SellState.h"

#include "../Engine/Game.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Options.h"
#include "../Engine/Screen.h"

#include "../Interface/Cursor.h"
#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextList.h"
#include "../Interface/Window.h"

#include "../Menu/ErrorMessageState.h"
#include "../Menu/MainMenuState.h"
#include "../Menu/SaveGameState.h"

#include "../Resource/XcomResourcePack.h"

#include "../Ruleset/AlienDeployment.h"
#include "../Ruleset/RuleArmor.h"
#include "../Ruleset/RuleCountry.h"
//#include "../Ruleset/RuleCraft.h"
#include "../Ruleset/RuleInterface.h"
#include "../Ruleset/RuleItem.h"
#include "../Ruleset/RuleRegion.h"
#include "../Ruleset/Ruleset.h"
#include "../Ruleset/RuleUfo.h"

#include "../Savegame/AlienBase.h"
#include "../Savegame/AlienMission.h"
#include "../Savegame/Base.h"
#include "../Savegame/BaseFacility.h"
#include "../Savegame/BattleItem.h"
#include "../Savegame/BattleUnitStatistics.h"
#include "../Savegame/Country.h"
#include "../Savegame/Craft.h"
#include "../Savegame/ItemContainer.h"
#include "../Savegame/MissionSite.h"
#include "../Savegame/Region.h"
#include "../Savegame/SavedBattleGame.h"
//#include "../Savegame/SavedGame.h"
#include "../Savegame/SoldierDiary.h"
#include "../Savegame/Tile.h"
#include "../Savegame/Ufo.h"
#include "../Savegame/Vehicle.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the Debriefing screen.
 */
DebriefingState::DebriefingState()
	:
		_rules(_game->getRuleset()),
		_gameSave(_game->getSavedGame()),
		_diff(static_cast<int>(_game->getSavedGame()->getDifficulty())),
		_skirmish(_game->getSavedGame()->getMonthsPassed() == -1),
		_region(nullptr),
		_country(nullptr),
		_base(nullptr),
		_craft(nullptr),
		_alienDies(false),
		_manageContainment(false),
		_destroyXComBase(false),
		_aliensControlled(0),
		_aliensKilled(0),
		_aliensStunned(0),
		_missionCost(0)
{
	Options::baseXResolution = Options::baseXGeoscape;
	Options::baseYResolution = Options::baseYGeoscape;
	_game->getScreen()->resetDisplay(false);

	// Restore the cursor in case something weird happened
	_game->getCursor()->setVisible();

	// Clean up the leftover states from BattlescapeGame; was done in
	// ~BattlescapeGame but that causes CTD under reLoad situation. Now done
	// here and in NextTurnState; not ideal: should find a safe place when
	// BattlescapeGame is really dTor'd and not reLoaded ...... uh, i guess.
	_gameSave->getBattleSave()->getBattleGame()->cleanBattleStates();

	_missionStatistics	= new MissionStatistics();

	_window				= new Window(this, 320, 200);

	_txtTitle			= new Text(280, 16,  16, 8);
	_txtBaseLabel		= new Text( 80,  9, 216, 8);

	_txtItem			= new Text(184, 9,  16, 24);
	_txtQuantity		= new Text( 60, 9, 200, 24);
	_txtScore			= new Text( 36, 9, 260, 24);

	_lstStats			= new TextList(288, 81, 16, 32);

	_lstRecovery		= new TextList(288, 81, 16, 32);
	_txtRecovery		= new Text(180, 9, 16, 60);

	_lstTotal			= new TextList(288, 9, 16, 12);

	_txtCost			= new Text(76, 9, 16, 180);
	_btnOk				= new TextButton(136, 16, 92, 177);
	_txtRating			= new Text(76, 9, 228, 180);

	setInterface("debriefing");

	add(_window,		"window",	"debriefing");
	add(_txtTitle,		"heading",	"debriefing");
	add(_txtBaseLabel,	"text",		"debriefing");
	add(_txtItem,		"text",		"debriefing");
	add(_txtQuantity,	"text",		"debriefing");
	add(_txtScore,		"text",		"debriefing");
	add(_txtRecovery,	"text",		"debriefing");
	add(_lstStats,		"list",		"debriefing");
	add(_lstRecovery,	"list",		"debriefing");
	add(_lstTotal,		"list",		"debriefing");
	add(_txtCost,		"text",		"debriefing");
	add(_btnOk,			"button",	"debriefing");
	add(_txtRating,		"text",		"debriefing");

	centerAllSurfaces();


	_window->setBackground(_game->getResourcePack()->getSurface("BACK01.SCR"));

	std::wstring wst (_gameSave->getBattleSave()->getOperation());
	if (wst.empty() == true) wst = tr("STR_OK");
	_btnOk->setText(wst);
	_btnOk->onMouseClick((ActionHandler)& DebriefingState::btnOkClick);
	_btnOk->onKeyboardPress(
					(ActionHandler)& DebriefingState::btnOkClick,
					Options::keyOk);
	_btnOk->onKeyboardPress(
					(ActionHandler)& DebriefingState::btnOkClick,
					Options::keyOkKeypad);
	_btnOk->onKeyboardPress(
					(ActionHandler)& DebriefingState::btnOkClick,
					Options::keyCancel);

	_txtTitle->setBig();

	_txtItem->setText(tr("STR_LIST_ITEM"));
	_txtQuantity->setText(tr("STR_QUANTITY_UC"));
	_txtScore->setText(tr("STR_SCORE"));

	_lstStats->setColumns(3, 176,60,36);
	_lstStats->setDot();

	_lstRecovery->setColumns(3, 176,60,36);
	_lstRecovery->setDot();

	_lstTotal->setColumns(2, 244,36);
	_lstTotal->setDot();
	_lstTotal->setMargin(0);

	_txtBaseLabel->setAlign(ALIGN_RIGHT);	// note: Text is set in prepareDebriefing() before
											// a possibly failed BaseDefense dangles '_base' ptr.

	prepareDebriefing(); // <- |-- GATHER ALL DATA HERE <- < ||


	int
		total			(0),
		stats_offY		(0),
		recov_offY		(0),
		civiliansSaved	(0),
		civiliansDead	(0);

	for (std::vector<DebriefingStat*>::const_iterator
			i = _stats.begin();
			i != _stats.end();
			++i)
	{
		if ((*i)->qty != 0)
		{
			std::wostringstream
				woststr1,
				woststr2;

			woststr1 << L'\x01' << (*i)->qty;	// quantity of recovered item Type
			woststr2 << L'\x01' << (*i)->score;	// score for items of Type
			total += (*i)->score;				// total score

			if ((*i)->recover == true)
			{
				_lstRecovery->addRow(
								3,
								tr((*i)->type).c_str(),
								woststr1.str().c_str(),
								woststr2.str().c_str());
				recov_offY += 8;
			}
			else
			{
				_lstStats->addRow(
								3,
								tr((*i)->type).c_str(),
								woststr1.str().c_str(),
								woststr2.str().c_str());
				stats_offY += 8;
			}

			if ((*i)->type == "STR_CIVILIANS_SAVED")
				civiliansSaved = (*i)->qty;

			if ((*i)->type == "STR_CIVILIANS_KILLED_BY_XCOM_OPERATIVES"
				|| (*i)->type == "STR_CIVILIANS_KILLED_BY_ALIENS")
			{
				civiliansDead += (*i)->qty;
			}

			if ((*i)->type == "STR_ALIENS_KILLED")
				_aliensKilled += (*i)->qty;
		}
	}

	_missionStatistics->score = total;

	if (civiliansDead == 0
		&& civiliansSaved != 0
		&& _missionStatistics->success == true)
	{
		_missionStatistics->valiantCrux = true;
	}

	std::wostringstream woststr;
	woststr << total;
	_lstTotal->addRow(
					2,
					tr("STR_TOTAL_UC").c_str(),
					woststr.str().c_str());

	if (recov_offY != 0)
	{
		_txtRecovery->setY(_lstStats->getY() + stats_offY + 5);
		_lstRecovery->setY(_txtRecovery->getY() + 8);
		_lstTotal->setY(_lstRecovery->getY() + recov_offY + 5);
	}
	else
	{
		_txtRecovery->setText(L"");
		_lstTotal->setY(_lstStats->getY() + stats_offY + 5);
	}


	if (_region != nullptr)
	{
		if (_destroyXComBase == true)
		{
			_region->addActivityAlien((_diff + 1) * 235);
			_region->recentActivityAlien();
		}
		else
		{
			_region->addActivityXCom(total);
			_region->recentActivityXCom();
		}
	}

	if (_country != nullptr)
	{
		if (_destroyXComBase == true)
		{
			_country->addActivityAlien((_diff + 1) * 235);
			_country->recentActivityAlien();
		}
		else
		{
			_country->addActivityXCom(total);
			_country->recentActivityXCom();
		}
	}

	std::string rating;
	if (total < -99)
	{
		_music = OpenXcom::res_MUSIC_TAC_DEBRIEFING_BAD;
		rating = "STR_RATING_TERRIBLE";
	}
	else
	{
		_music = OpenXcom::res_MUSIC_TAC_DEBRIEFING;

		if (total < 101)
			rating = "STR_RATING_POOR";
		else if (total < 351)
			rating = "STR_RATING_OK";
		else if (total < 751)
			rating = "STR_RATING_GOOD";
		else if (total < 1251)
			rating = "STR_RATING_EXCELLENT";
		else
			rating = "STR_RATING_STUPENDOUS";
	}

	if (_skirmish == false && _missionCost != 0)
	{
//		_txtCost->setText(tr("STR_COST_").arg(Text::formatCurrency(_missionCost)));
		_txtCost->setText(Text::formatCurrency(_missionCost));
		_txtCost->setAlign(ALIGN_CENTER);
	}
	else
		_txtCost->setVisible(false);

//	_txtRating->setText(tr("STR_RATING").arg(tr(rating)));
	_txtRating->setText(tr(rating));
	_txtRating->setAlign(ALIGN_CENTER);


	// Soldier Diary ->
	SavedBattleGame* const battleSave (_gameSave->getBattleSave());

	_missionStatistics->rating = rating;
	_missionStatistics->id = _gameSave->getMissionStatistics()->size();
	_missionStatistics->shade = battleSave->getTacticalShade();

	//Log(LOG_INFO) << "DebriefingState::cTor";
	Soldier* sol;

	for (std::vector<BattleUnit*>::const_iterator
			i = battleSave->getUnits()->begin();
			i != battleSave->getUnits()->end();
			++i)
	{
		//Log(LOG_INFO) << ". iter BattleUnits";
		sol = (*i)->getGeoscapeSoldier();
		// NOTE: In the case of a dead soldier this pointer is Valid but points to garbage.
		// Use that.
		if (sol != nullptr)
		{
			//Log(LOG_INFO) << ". . id = " << (*i)->getId();
			BattleUnitStatistics* const statistics ((*i)->getStatistics());

			int soldierAlienKills (0);

			for (std::vector<BattleUnitKill*>::const_iterator
					j = statistics->kills.begin();
					j != statistics->kills.end();
					++j)
			{
				if ((*j)->_faction == FACTION_HOSTILE)
					++soldierAlienKills;
			}

			// NOTE re. Nike Cross:
			// This can be exploited by MC'ing a bunch of aLiens while having
			// Option "psi-control ends battle" TRUE. ... Patched.
			//
			// NOTE: This can still be exploited by MC'ing and
			// executing a bunch of aLiens with a single Soldier.
			if (_aliensControlled == 0
				&& _aliensKilled + _aliensStunned > 3 + _diff
				&& _aliensKilled + _aliensStunned == soldierAlienKills
				&& _missionStatistics->success == true)
			{
				statistics->nikeCross = true;
			}


			if ((*i)->getUnitStatus() == STATUS_DEAD)
			{
				//Log(LOG_INFO) << ". . . dead";
				sol = nullptr;	// Zero out the BattleUnit from the geoscape Soldiers list
								// in this State; it's already gone from his/her former Base.
								// This makes them ineligible for promotion.
								// PS, there is no 'geoscape Soldiers list' really; it's
								// just a variable stored in each xCom-agent/BattleUnit ....
				SoldierDead* deadSoldier (nullptr); // avoid vc++ linker warning.

				for (std::vector<SoldierDead*>::const_iterator
						j = _gameSave->getDeadSoldiers()->begin();
						j != _gameSave->getDeadSoldiers()->end();
						++j)
				{
					if ((*j)->getId() == (*i)->getId())
					{
						deadSoldier = *j;
						break;
					}
				}

				statistics->daysWounded = 0;

				// note: Safety on *deadSoldier should not be needed. see above^
				if (statistics->KIA == true)
					_missionStatistics->injuryList[deadSoldier->getId()] = -1;
				else // MIA
					_missionStatistics->injuryList[deadSoldier->getId()] = -2;

				deadSoldier->getDiary()->updateDiary(
												statistics,
												_missionStatistics,
												_rules);
				deadSoldier->getDiary()->manageAwards(_rules);
				_soldiersLost.push_back(deadSoldier);
			}
			else
			{
				//Log(LOG_INFO) << ". . . alive";
				statistics->daysWounded =
				_missionStatistics->injuryList[sol->getId()] = sol->getSickbay();

				sol->getDiary()->updateDiary(
											statistics,
											_missionStatistics,
											_rules);
				if (sol->getDiary()->manageAwards(_rules) == true)
					_soldiersMedalled.push_back(sol);
			}
		}
	}

	_gameSave->getMissionStatistics()->push_back(_missionStatistics);
	// Soldier Diary_end.
}

/**
 * dTor.
 */
DebriefingState::~DebriefingState()
{
	if (_game->isQuitting() == true)
		_gameSave->setBattleSave();

	for (std::vector<DebriefingStat*>::const_iterator
			i = _stats.begin();
			i != _stats.end();
			++i)
		delete *i;

	for (std::map<int, SpecialType*>::const_iterator
			i = _specialTypes.begin();
			i != _specialTypes.end();
			++i)
		delete i->second;
}

/**
 * Initializes the state.
 */
void DebriefingState::init()
{
	State::init();
	_game->getResourcePack()->playMusic(_music, "", 1);
}

/**
 * Returns to the previous screen.
 * @param action - pointer to an Action
 */
void DebriefingState::btnOkClick(Action*)
{
	_game->getResourcePack()->fadeMusic(_game, 863);
	_game->popState();

	if (_skirmish == true)
		_game->setState(new MainMenuState());
	else
	{
		if (_destroyXComBase == false) // TODO: Meridian's stat-increases + equip't-haul
		{
			bool playAwardMusic (false);

			// Current Order:
			// - stores overfull
			// - containment
			// - cannot re-equip.
			// - soldier stats
			// - equip't gained
			// - property lost
			// - promotions
			// - ceremony
			// - taps
			//
			// Ideal Order:
			// - stores overfull
			// - containment
			// - promotions
			// - ceremony
			// - taps
			// - soldier stats
			// - equip't gained
			// - property lost
			// - cannot re-equip.
			//
			// NOTE: Rearranging these states would require considerable fade/playMusic adjustment.

			SavedBattleGame* const battleSave (_gameSave->getBattleSave());

			// NOTE: These push to player in reverse order.
			if (_soldiersLost.empty() == false)
			{
				playAwardMusic = true;
				_game->pushState(new CeremonyDeadState(_soldiersLost));
			}

			if (_soldiersMedalled.empty() == false)
			{
				playAwardMusic = true;
				_game->pushState(new CeremonyState(_soldiersMedalled));
			}

			std::vector<Soldier*> participants;
			for (std::vector<BattleUnit*>::const_iterator
					i = battleSave->getUnits()->begin();
					i != battleSave->getUnits()->end();
					++i)
			{
				if ((*i)->getGeoscapeSoldier() != nullptr)
					participants.push_back((*i)->getGeoscapeSoldier());
			}

			if (_gameSave->handlePromotions(participants) == true)
			{
				playAwardMusic = true;
				_game->pushState(new PromotionsState());
			}

			_game->pushState(new DebriefExtraState(
												_base,
												battleSave->getOperation(),
												_itemsLostProperty,
												_itemsGained,
												_soldierStatInc));

			if (_missingItems.empty() == false)
				_game->pushState(new CannotReequipState(_missingItems));

			if (_alienDies == true)
				_game->pushState(new NoContainmentState());
			else if (_manageContainment == true)
			{
				_game->pushState(new AlienContainmentState(_base, OPT_BATTLESCAPE));
				_game->pushState(new ErrorMessageState(
												tr("STR_CONTAINMENT_EXCEEDED").arg(_base->getName(nullptr)),
												_palette,
												_rules->getInterface("debriefing")->getElement("errorMessage")->color,
												"BACK04.SCR",
												_rules->getInterface("debriefing")->getElement("errorPalette")->color));
			}

			if (_base->storesOverfull() == true) //_manageContainment == false &&
			{
//				_game->pushState(new SellState(_base, OPT_BATTLESCAPE));
				_game->pushState(new ErrorMessageState(
												tr("STR_STORAGE_EXCEEDED").arg(_base->getName(nullptr)),
												_palette,
												_rules->getInterface("debriefing")->getElement("errorMessage")->color,
												_game->getResourcePack()->getRandomBackground(),
												_rules->getInterface("debriefing")->getElement("errorPalette")->color));
			}

			if (playAwardMusic == true)
				_game->getResourcePack()->playMusic(
												OpenXcom::res_MUSIC_TAC_AWARDS,
												"", 1);
			else
				_game->getResourcePack()->playMusic(OpenXcom::res_MUSIC_GEO_GLOBE);
		}

		if (_gameSave->isIronman() == true) // Autosave after mission
			_game->pushState(new SaveGameState(
											OPT_GEOSCAPE,
											SAVE_IRONMAN,
											_palette));
		else if (Options::autosave == true)
			_game->pushState(new SaveGameState(
											OPT_GEOSCAPE,
											SAVE_AUTO_GEOSCAPE,
											_palette));
	}
	_gameSave->setBattleSave();	// delete SavedBattleGame.
}								// NOTE: BattlescapeState and BattlescapeGame are still VALID here.

/**
 * Adds to the debriefing stats.
 * @param type	- reference the untranslated name of the stat
 * @param score	- the score to add
 * @param qty	- the quantity to add (default 1)
 */
void DebriefingState::addStat( // private.
		const std::string& type,
		int score,
		int qty)
{
	for (std::vector<DebriefingStat*>::const_iterator
			i = _stats.begin();
			i != _stats.end();
			++i)
	{
		if ((*i)->type == type)
		{
			(*i)->score += score;
			(*i)->qty += qty;
			break;
		}
	}
}


/**
 * *** FUNCTOR ***
 * Clears any supply missions from an aLien base.
 */
class ClearAlienBase
	:
		public std::unary_function<AlienMission*, void>
{

private:
	const AlienBase* _aBase;

	public:
		/// Caches a pointer to the aLien base.
		explicit ClearAlienBase(const AlienBase* aBase)
			:
				_aBase(aBase)
		{}

		/// Clears the aLien Base if required.
		void operator() (AlienMission* mission) const;
};

/**
 * Removes the association between the alien mission and the alien base if one existed.
 * @param mission - pointer to the AlienMission
 */
void ClearAlienBase::operator() (AlienMission* mission) const
{
	if (mission->getAlienBase() == _aBase)
		mission->setAlienBase(nullptr);
}


/**
 * Prepares debriefing: gathers Aliens, Corpses, Artefacts, UFO Components.
 * Adds the items to the craft.
 * @note Also calculates the soldiers' experience and possible promotions. If
 * aborted only the things on the exit area are recovered.
 */
void DebriefingState::prepareDebriefing() // private.
{
	const RuleItem* itRule;
	for (std::vector<std::string>::const_iterator
			i = _rules->getItemsList().begin();
			i != _rules->getItemsList().end();
			++i)
	{
		itRule = _rules->getItemRule(*i);
		const SpecialTileType tileType (itRule->getSpecialType());
		if (tileType > 1)
		{
			SpecialType* const specialType (new SpecialType());
			specialType->type = *i;
			specialType->value = itRule->getRecoveryPoints();

			_specialTypes[tileType] = specialType;
		}
	}

	std::string
		objectiveCompleteText,
		objectiveFailedText;
	int
		objectiveCompleteScore (0), // dang vc++ compiler warnings.
		objectiveFailedScore   (0); // dang vc++ compiler warnings.

	SavedBattleGame* const battleSave (_gameSave->getBattleSave());
	const AlienDeployment* const deployRule (_rules->getDeployment(battleSave->getTacticalType()));
	// kL_note: I have a strong suspicion that although checks are made for a
	// valid deployRule below if there isn't one you're borked anyway.
	if (deployRule != nullptr)
	{
		if (deployRule->getObjectiveCompleteInfo(
											objectiveCompleteText,
											objectiveCompleteScore) == true)
		{
			_stats.push_back(new DebriefingStat(objectiveCompleteText));
		}

		if (deployRule->getObjectiveFailedInfo(
											objectiveFailedText,
											objectiveFailedScore) == false)
		{
			_stats.push_back(new DebriefingStat(objectiveFailedText));
		}
	}

	_stats.push_back(new DebriefingStat("STR_ALIENS_KILLED"));
	_stats.push_back(new DebriefingStat("STR_ALIEN_CORPSES_RECOVERED"));
	_stats.push_back(new DebriefingStat("STR_LIVE_ALIENS_RECOVERED"));
	_stats.push_back(new DebriefingStat("STR_ALIEN_ARTIFACTS_RECOVERED"));
	_stats.push_back(new DebriefingStat("STR_ALIEN_BASE_CONTROL_DESTROYED"));
	_stats.push_back(new DebriefingStat("STR_CIVILIANS_KILLED_BY_ALIENS"));
	_stats.push_back(new DebriefingStat("STR_CIVILIANS_KILLED_BY_XCOM_OPERATIVES"));
	_stats.push_back(new DebriefingStat("STR_CIVILIANS_SAVED"));
	_stats.push_back(new DebriefingStat("STR_XCOM_OPERATIVES_KILLED"));
//	_stats.push_back(new DebriefingStat("STR_XCOM_OPERATIVES_RETIRED_THROUGH_INJURY"));
	_stats.push_back(new DebriefingStat("STR_XCOM_OPERATIVES_MISSING_IN_ACTION"));
	_stats.push_back(new DebriefingStat("STR_TANKS_DESTROYED"));
	_stats.push_back(new DebriefingStat("STR_XCOM_CRAFT_LOST"));

	for (std::map<int, SpecialType*>::const_iterator
			i = _specialTypes.begin();
			i != _specialTypes.end();
			++i)
	{
		_stats.push_back(new DebriefingStat((*i).second->type, true));
	}
	_stats.push_back(new DebriefingStat(_rules->getAlienFuelType(), true));
/*	_stats.push_back(new DebriefingStat("STR_UFO_POWER_SOURCE", true)); // ->> SpecialTileTypes <<-|
	_stats.push_back(new DebriefingStat("STR_UFO_NAVIGATION", true));
	_stats.push_back(new DebriefingStat("STR_UFO_CONSTRUCTION", true));
	_stats.push_back(new DebriefingStat("STR_ALIEN_FOOD", true));
	_stats.push_back(new DebriefingStat("STR_ALIEN_REPRODUCTION", true));
	_stats.push_back(new DebriefingStat("STR_ALIEN_ENTERTAINMENT", true));
	_stats.push_back(new DebriefingStat("STR_ALIEN_SURGERY", true));
	_stats.push_back(new DebriefingStat("STR_EXAMINATION_ROOM", true));
	_stats.push_back(new DebriefingStat("STR_ALIEN_ALLOYS", true));
	_stats.push_back(new DebriefingStat("STR_ALIEN_HABITAT", true)); */

	_missionStatistics->timeStat = *_gameSave->getTime();
	_missionStatistics->type = battleSave->getTacticalType();

	if (_skirmish == false) // Do all aLienRace types here for SoldierDiary stat.
	{
		if (battleSave->getAlienRace().empty() == false) // safety.
			_missionStatistics->alienRace = battleSave->getAlienRace();
		else
			_missionStatistics->alienRace = "STR_UNKNOWN";
	}

	int
		playerExit (0),
		playerLive (0),
		playerDead (0), // Soldier Diary.
		playerOut  (0);

	// Evaluate how many surviving xCom units there are and if they are
	// unconscious and how many have died - for the Awards Ceremony.
	for (std::vector<BattleUnit*>::const_iterator
			i = battleSave->getUnits()->begin();
			i != battleSave->getUnits()->end();
			++i)
	{
		if ((*i)->getOriginalFaction() == FACTION_PLAYER)
		{
			if ((*i)->getUnitStatus() != STATUS_DEAD)
			{
				++playerLive;
				if ((*i)->getUnitStatus() == STATUS_UNCONSCIOUS
					|| (*i)->getFaction() == FACTION_HOSTILE)
				{
					++playerOut;
				}
			}
			else
			{
				++playerDead;
				if ((*i)->getGeoscapeSoldier() != nullptr)
					(*i)->getStatistics()->KIA = true;
			}
		}
	}

	// if all xCom units are unconscious the aliens get to have their way with them
	if (playerOut == playerLive)
	{
		playerLive = 0;
		for (std::vector<BattleUnit*>::const_iterator
				i = battleSave->getUnits()->begin();
				i != battleSave->getUnits()->end();
				++i)
		{
			if ((*i)->getOriginalFaction() == FACTION_PLAYER
				&& (*i)->getUnitStatus() != STATUS_DEAD)
			{
				(*i)->setUnitStatus(STATUS_DEAD);
				if ((*i)->getGeoscapeSoldier() != nullptr)
					(*i)->getStatistics()->MIA = true;
			}
		}
	}

	const bool aborted (battleSave->isAborted());

	if (playerLive == 1)
	{
		for (std::vector<BattleUnit*>::const_iterator
				i = battleSave->getUnits()->begin();
				i != battleSave->getUnits()->end();
				++i)
		{
			if ((*i)->getGeoscapeSoldier() != nullptr
				&& (*i)->getUnitStatus() != STATUS_DEAD)
			{
				if (playerDead == 0
					&& aborted == false
					&& _aliensControlled == 0
					&& _aliensKilled + _aliensStunned > 1 + _diff)
				{
					(*i)->getStatistics()->ironMan = true;
					break;
				}
				else if (playerDead != 0									// if only one Soldier survived give him a medal!
					&& (*i)->getStatistics()->hasFriendlyFired() == false)	// unless he killed all the others ...
				{
					(*i)->getStatistics()->loneSurvivor = true;
					break;
				}
			}
		}
	}


	_missionStatistics->success = (playerLive != 0 && aborted == false)
								|| battleSave->allObjectivesDestroyed() == true;
	const bool playerWipe (playerLive == 0
					   || (aborted == true && playerExit == 0));

	double
		lon (0.), // avoid vc++ linker warnings.
		lat (0.); // avoid vc++ linker warnings.

	std::vector<Craft*>::const_iterator pCraft;
	for (std::vector<Base*>::const_iterator
			i = _gameSave->getBases()->begin();
			i != _gameSave->getBases()->end();
			++i)
	{
		for (std::vector<Craft*>::const_iterator
				j = (*i)->getCrafts()->begin();
				j != (*i)->getCrafts()->end();
				++j)
		{
			if ((*j)->getTactical() == true) // has Craft, ergo NOT BaseDefense
			{
				if (_skirmish == false)
					_missionCost = (*i)->craftExpense(*j);

				lon = (*j)->getLongitude();
				lat = (*j)->getLatitude();

				_base = *i;
				_craft = *j;

				if (playerWipe == false)
				{
					_craft->returnToBase();
					_craft->setTacticalReturn();
					_craft->setTactical(false);
				}
				else
					pCraft = j; // to delete the Craft below_
			}
			else if ((*j)->getDestination() != nullptr)
			{
				if (_missionStatistics->success == true)
				{
					const Ufo* const ufo (dynamic_cast<Ufo*>((*j)->getDestination()));
					if (ufo != nullptr && ufo->getTactical() == true)
						(*j)->returnToBase();
				}

				const MissionSite* const site (dynamic_cast<MissionSite*>((*j)->getDestination()));
				if (site != nullptr && site->getTactical() == true)
					(*j)->returnToBase();
			}
		}

		if ((*i)->getTactical() == true) // in case this DON'T have a Craft, ie. BaseDefense
		{
			_base = *i;
			_txtBaseLabel->setText(_base->getName(_game->getLanguage()));

			lon = _base->getLongitude();
			lat = _base->getLatitude();

			if (playerLive != 0 && aborted == false)
			{
				_base->setTactical(false);

				bool facDestroyed (false);
				for (std::vector<BaseFacility*>::const_iterator
						j = _base->getFacilities()->begin();
						j != _base->getFacilities()->end();
						)
				{
					if (battleSave->baseDestruct()[(*j)->getX()]
												  [(*j)->getY()].second == 0) // this facility was demolished
					{
						facDestroyed = true;
						j = _base->destroyFacility(j);
					}
					else
						++j;
				}

				if (facDestroyed == true)
					_base->destroyDisconnectedFacilities(); // this may cause the base to become disjointed; destroy the disconnected parts.
			}
			else
				_destroyXComBase = true;
		}
	}

	for (std::vector<Region*>::const_iterator
			i = _gameSave->getRegions()->begin();
			i != _gameSave->getRegions()->end();
			++i)
	{
		if ((*i)->getRules()->insideRegion(lon, lat) == true)
		{
			_region = *i;
			_missionStatistics->region = _region->getRules()->getType();
			break;
		}
	}

	for (std::vector<Country*>::const_iterator
			i = _gameSave->getCountries()->begin();
			i != _gameSave->getCountries()->end();
			++i)
	{
		if ((*i)->getRules()->insideCountry(lon, lat) == true)
		{
			_country = *i;
			_missionStatistics->country = _country->getRules()->getType();
			break;
		}
	}


	// Determine aLien tactical mission.
	bool found (false);
	for (std::vector<Ufo*>::const_iterator // First - search for UFO.
			i = _gameSave->getUfos()->begin();
			i != _gameSave->getUfos()->end();
			++i)
	{
		if ((*i)->getTactical() == true)
		{
			found = true;
			_txtRecovery->setText(tr("STR_UFO_RECOVERY"));
			_missionStatistics->ufo = (*i)->getRules()->getType();

			if (playerLive != 0 && aborted == false)
			{
				delete *i;
				_gameSave->getUfos()->erase(i);
			}
			else
			{
				(*i)->setTactical(false);
				if ((*i)->getUfoStatus() == Ufo::LANDED)
					(*i)->setSecondsLeft(5);
			}
			break;
		}
	}

	if (found == false)
	{
		for (std::vector<MissionSite*>::const_iterator // Second - search for MissionSite.
				i = _gameSave->getMissionSites()->begin();
				i != _gameSave->getMissionSites()->end();
				++i)
		{
			if ((*i)->getTactical() == true)
			{
				found = true;
				delete *i;
				_gameSave->getMissionSites()->erase(i);
				break;
			}
		}
	}

	if (found == false)
	{
		for (std::vector<AlienBase*>::const_iterator // Third - search for AlienBase.
				i = _gameSave->getAlienBases()->begin();
				i != _gameSave->getAlienBases()->end();
				++i)
		{
			if ((*i)->getTactical() == true)
			{
				_txtRecovery->setText(tr("STR_ALIEN_BASE_RECOVERY"));

				if (_missionStatistics->success == true)
				{
					if (objectiveCompleteText.empty() == false)
					{
						objectiveCompleteScore = std::max(
													(objectiveCompleteScore + 9) / 10,
													 objectiveCompleteScore - (_diff * 50));
						addStat(
							objectiveCompleteText,
							objectiveCompleteScore);
					}

					std::for_each(
							_gameSave->getAlienMissions().begin(),
							_gameSave->getAlienMissions().end(),
							ClearAlienBase(*i));

					for (std::vector<Target*>::const_iterator
							j = (*i)->getFollowers()->begin();
							j != (*i)->getFollowers()->end();
							++j)
					{
						Craft* const craft (dynamic_cast<Craft*>(*j));
						if (craft != nullptr)
							craft->returnToBase();
					}

					delete *i;
					_gameSave->getAlienBases()->erase(i);
				}
				else
					(*i)->setTactical(false);

				break;
			}
		}
	}


	const TacticalType tacType (battleSave->getTacType());

	for (std::vector<BattleUnit*>::const_iterator
			i = battleSave->getUnits()->begin();
			i != battleSave->getUnits()->end();
			++i)
	{
		if ((*i)->getTile() == nullptr)								// This unit is not on a tile ... give it one.
		{
			Position pos ((*i)->getPosition());
			if (pos == Position(-1,-1,-1))							// in fact, this Unit is in limbo ... ie, is carried.
			{
				for (std::vector<BattleItem*>::const_iterator		// so look for its body or corpse ...
						j = battleSave->getItems()->begin();
						j != battleSave->getItems()->end();
						++j)
				{
					if ((*j)->getUnit() != nullptr
						&& (*j)->getUnit() == *i)					// found it: corpse is a dead or unconscious BattleUnit!!
					{
						if ((*j)->getOwner() != nullptr)			// corpse of BattleUnit has an Owner (ie. is being carried by another BattleUnit)
							pos = (*j)->getOwner()->getPosition();	// Put the corpse down .. slowly.
						else if ((*j)->getTile() != nullptr)		// corpse of BattleUnit is laying around somewhere
							pos = (*j)->getTile()->getPosition();	// you're not vaporized yet, Get up.
					}
				}
			}
			(*i)->setTile(battleSave->getTile(pos));
		}


		const UnitFaction orgFaction ((*i)->getOriginalFaction());
		const int value ((*i)->getValue());

		if ((*i)->getUnitStatus() == STATUS_DEAD)
		{
			//Log(LOG_INFO) << ". unitDead " << (*i)->getId() << " type = " << (*i)->getType();
			switch (orgFaction)
			{
				case FACTION_HOSTILE:
					if ((*i)->killedBy() == FACTION_PLAYER)
					{
						//Log(LOG_INFO) << ". . killed by xCom";
						addStat(
							"STR_ALIENS_KILLED",
							value);
					}
					break;

				case FACTION_PLAYER:
				{
					Soldier* const sol ((*i)->getGeoscapeSoldier());
					if (sol != nullptr)
					{
						_soldierStatInc[sol->getName()] = (*i)->postMissionProcedures(true);

						if (_skirmish == false)
							_missionCost += _base->soldierExpense(sol, true);

						addStat(
							"STR_XCOM_OPERATIVES_KILLED",
							-value);

						for (std::vector<Soldier*>::const_iterator
								j = _base->getSoldiers()->begin();
								j != _base->getSoldiers()->end();
								++j)
						{
							if (*j == sol) // note: Could return any armor the Soldier was wearing to Stores. CHEATER!!!!!
							{
								(*j)->die(_gameSave);
								delete *j;
								_base->getSoldiers()->erase(j);
								break;
							}
						}
					}
					else // support unit
					{
						if (_skirmish == false)
							_missionCost += _base->supportExpense(
															(*i)->getArmor()->getSize() * (*i)->getArmor()->getSize(),
															true);
						addStat(
							"STR_TANKS_DESTROYED",
							-value);

						++_itemsLostProperty[_rules->getItemRule((*i)->getType())];

						const BattleItem* ordnance ((*i)->getItem(ST_RIGHTHAND));
						if (ordnance != nullptr)
						{
							itRule = ordnance->getRules();
							if (itRule->isFixed() == true
								&& itRule->getFullClip() > 0
								&& (ordnance = ordnance->getAmmoItem()) != nullptr)
							{
								_itemsLostProperty[ordnance->getRules()] += itRule->getFullClip();
							}
						}
					}
					break;
				}

				case FACTION_NEUTRAL:
					if ((*i)->killedBy() == FACTION_PLAYER)
						addStat(
							"STR_CIVILIANS_KILLED_BY_XCOM_OPERATIVES",
							-(value * 2));
					else
						addStat(
							"STR_CIVILIANS_KILLED_BY_ALIENS",
							-value);
			}
		}
		else // alive units possible unconscious.
		{
			//Log(LOG_INFO) << ". unitLive " << (*i)->getId() << " type = " << (*i)->getType();
			switch (orgFaction)
			{
				case FACTION_PLAYER:
					if (aborted == false
						|| ((tacType != TCT_BASEDEFENSE /*|| _missionStatistics->success == true*/)
							&& ((*i)->isInExitArea() == true || (*i)->getUnitStatus() == STATUS_LIMBO)))
					{
						++playerExit;
						recoverItems((*i)->getInventory());

						Soldier* const sol ((*i)->getGeoscapeSoldier());
						if (sol != nullptr)
						{
							_soldierStatInc[sol->getName()] = (*i)->postMissionProcedures();

							if (_skirmish == false)
								_missionCost += _base->soldierExpense(sol);

//							sol->calcStatString(
//											_rules->getStatStrings(),
//											Options::psiStrengthEval
//												&& _gameSave->isResearched(_rules->getPsiRequirements()));
						}
						else
						{
							if (_skirmish == false)
							{
								const int quadrants ((*i)->getArmor()->getSize());
								_missionCost += _base->supportExpense(quadrants * quadrants);
							}

							_base->getStorageItems()->addItem((*i)->getType());	// return the support-unit to Stores.

							const BattleItem* ordnance ((*i)->getItem(ST_RIGHTHAND));
							if (ordnance != nullptr)
							{
								int clip;
								itRule = ordnance->getRules();
								if (itRule->isFixed() == true
									&& (clip = itRule->getFullClip()) > 0
									&& (ordnance = ordnance->getAmmoItem()) != nullptr)
								{
									itRule = ordnance->getRules();
									const int qtyLoad (ordnance->getAmmoQuantity());
									_base->getStorageItems()->addItem(			// return any load from the support-unit's fixed-weapon to Stores.
																	itRule->getType(),
																	qtyLoad);
									if (qtyLoad < clip)
										_itemsLostProperty[itRule] += clip - qtyLoad;
								}
							}

/*							if ((weapon = (*i)->getItem(ST_LEFTHAND)) != nullptr)
							{
								itRule = weapon->getRules();
								if (itRule->getCompatibleAmmo()->empty() == false)
								{
									ordnance = weapon->getAmmoItem();
									if (ordnance != nullptr) //&& ordnance->getAmmoQuantity() > 0)
									{
										int total (ordnance->getAmmoQuantity());
										if (itRule->getFullClip() != 0) // meaning this tank can store multiple  clips-of-clips
											total /= ordnance->getRules()->getFullClip();

										_base->getStorageItems()->addItem(
																		itRule->getCompatibleAmmo()->front(),
																		total);
									}
								}
							} */
						}
					}
					else
					{
						(*i)->setUnitStatus(STATUS_DEAD);

						Soldier* const sol ((*i)->getGeoscapeSoldier());
						if (sol != nullptr)
						{
							_soldierStatInc[sol->getName()] = (*i)->postMissionProcedures(true);
							(*i)->getStatistics()->MIA = true;

							addStat(
								"STR_XCOM_OPERATIVES_MISSING_IN_ACTION",
								-value);

							for (std::vector<Soldier*>::const_iterator
									j = _base->getSoldiers()->begin();
									j != _base->getSoldiers()->end();
									++j)
							{
								if (*j == sol) // note: Could return any armor the Soldier was wearing to Stores. CHEATER!!!!!
								{
									(*j)->die(_gameSave);
									delete *j;
									_base->getSoldiers()->erase(j);
									break;
								}
							}
						}
						else // support unit
						{
							addStat(
								"STR_TANKS_DESTROYED",
								-value);

							++_itemsLostProperty[_rules->getItemRule((*i)->getType())];

							const BattleItem* ordnance ((*i)->getItem(ST_RIGHTHAND));
							if (ordnance != nullptr)
							{
								itRule = ordnance->getRules();
								if (itRule->isFixed() == true
									&& itRule->getFullClip() > 0
									&& (ordnance = ordnance->getAmmoItem()) != nullptr)
								{
									_itemsLostProperty[ordnance->getRules()] += itRule->getFullClip();
								}
							}
						}
					}
					break;

				case FACTION_HOSTILE:
					if ((*i)->isMindControlled() == true
						&& (*i)->isOut_t(OUT_STAT) == false
						&& (aborted == false || (*i)->isInExitArea() == true))
						// This never actually runs unless the early psi-exit option,
						// where all aliens are dead or mind-controlled, is turned on --
						// except for the two-stage Cydonia mission in which case all
						// this does not matter.
					{
						++_aliensControlled;
						for (std::vector<BattleItem*>::const_iterator
								j = (*i)->getInventory()->begin();
								j != (*i)->getInventory()->end();
								++j)
						{
							if ((*j)->getRules()->isFixed() == false)
							{
								(*j)->setInventorySection(_rules->getInventoryRule(ST_GROUND));
								(*i)->getTile()->addItem(*j);
							}
						}
						recoverLiveAlien(*i);
					}
					break;

				case FACTION_NEUTRAL:
					if (playerLive == 0
						|| (aborted == true && (*i)->isInExitArea() == false))
					{
						addStat(
							"STR_CIVILIANS_KILLED_BY_ALIENS",
							-value);
					}
					else
						addStat(
							"STR_CIVILIANS_SAVED",
							value); // duplicated below.
			}
		}
	}

	if (_craft != nullptr && playerWipe == true)
	{
		addStat(
			"STR_XCOM_CRAFT_LOST",
			-(_craft->getRules()->getScore()));

		for (std::vector<Soldier*>::const_iterator
				i = _base->getSoldiers()->begin();
				i != _base->getSoldiers()->end();
				)
		{
			if ((*i)->getCraft() == _craft)
			{
				delete *i;
				i = _base->getSoldiers()->erase(i);
			}
			else
				++i;
		}

		delete _craft;
		_craft = nullptr;
		_base->getCrafts()->erase(pCraft);
		_txtTitle->setText(tr("STR_CRAFT_IS_LOST"));

		return; // -> EXIT <--|||
	}

	if (aborted == true && tacType == TCT_BASEDEFENSE)
	{
		for (std::vector<Craft*>::const_iterator
				i = _base->getCrafts()->begin();
				i != _base->getCrafts()->end();
				++i)
		{
			addStat(
				"STR_XCOM_CRAFT_LOST",
				-(*i)->getRules()->getScore());
		}
	}


	std::string tacResult;
	if (_missionStatistics->success == true)
	{
		switch (tacType)
		{
			case TCT_BASEDEFENSE:
				tacResult = "STR_BASE_IS_SAVED";
				break;

			case TCT_MISSIONSITE:
				tacResult = "STR_ALIENS_DEFEATED";
				break;

			case TCT_BASEASSAULT:
			case TCT_MARS1:
			case TCT_MARS2:
				tacResult = "STR_ALIEN_BASE_DESTROYED";
				break;

			default:
				tacResult = "STR_UFO_IS_RECOVERED";
		}
		_txtTitle->setText(tr(tacResult));

		if (objectiveCompleteText.empty() == false)
			addStat(
				objectiveCompleteText,
				objectiveCompleteScore);
	}
	else
	{
		switch (tacType)
		{
			case TCT_BASEDEFENSE:
				tacResult = "STR_BASE_IS_LOST";
				break;

			case TCT_MISSIONSITE:
				tacResult = "STR_TERROR_CONTINUES";
				break;

			case TCT_BASEASSAULT:
			case TCT_MARS1: // Note that these Mars tacticals are really Lose GAME.
			case TCT_MARS2:
				tacResult = "STR_ALIEN_BASE_STILL_INTACT";
				break;

			default:
				tacResult = "STR_UFO_IS_NOT_RECOVERED";
		}
		_txtTitle->setText(tr(tacResult));

		if (objectiveFailedText.empty() == false)
			addStat(
				objectiveFailedText,
				objectiveFailedScore);
	}

	if (playerLive != 0)
	{
		recoverItems(battleSave->guaranteedItems());

		if (aborted == false)
		{
			recoverItems(battleSave->conditionalItems());

			const int parts (static_cast<int>(Tile::PARTS_TILE));
			MapDataType partType;
			for (size_t
					i = 0;
					i != battleSave->getMapSizeXYZ();
					++i)
			{
				recoverItems(battleSave->getTiles()[i]->getInventory());

				for (int
						j = 0;
						j != parts;
						++j)
				{
					partType = static_cast<MapDataType>(j);
					if (battleSave->getTiles()[i]->getMapData(partType) != nullptr)
					{
						const SpecialTileType tileType (battleSave->getTiles()[i]->getMapData(partType)->getSpecialType());
						if (_specialTypes.find(tileType) != _specialTypes.end())
							addStat(
								_specialTypes[tileType]->type,
								_specialTypes[tileType]->value);
					}
				}
			}

			for (std::vector<DebriefingStat*>::const_iterator
					i = _stats.begin();
					i != _stats.end();
					++i)
			{
				if ((*i)->type == _specialTypes[ALIEN_ALLOYS]->type)
				{
					int alloyDivisor;
					if (tacType == TCT_BASEASSAULT)
						alloyDivisor = 150;
					else
						alloyDivisor = 15;

					(*i)->qty /= alloyDivisor;
					(*i)->score /= alloyDivisor;

					_itemsGained[_rules->getItemRule((*i)->type)] = (*i)->qty; // NOTE: Elerium is handled in recoverItems().
				}

				if ((*i)->recover == true && (*i)->qty != 0)
					_base->getStorageItems()->addItem((*i)->type, (*i)->qty);
			}
		}
		else if (_destroyXComBase == false)
		{
			for (size_t
					i = 0;
					i != battleSave->getMapSizeXYZ();
					++i)
			{
				if (battleSave->getTiles()[i]->getMapData(O_FLOOR) != nullptr
					&& battleSave->getTiles()[i]->getMapData(O_FLOOR)->getSpecialType() == START_POINT)
				{
					recoverItems(battleSave->getTiles()[i]->getInventory());
				}
			}
		}
	}


	for (std::vector<BattleItem*>::const_iterator
			i = _gameSave->getBattleSave()->getDeletedItems().begin();
			i != _gameSave->getBattleSave()->getDeletedItems().end();
			++i)
	{
		if ((*i)->getProperty() == true)
			++_itemsLostProperty[(*i)->getRules()];
	}

	int
		qtyFullClip,
		clipsTotal;
	for (std::map<const RuleItem*, int>::const_iterator
			i = _clips.begin();
			i != _clips.end();
			++i)
	{
		if ((qtyFullClip = i->first->getFullClip()) != 0) // safety.
		{
			clipsTotal = i->second / qtyFullClip;
			if (clipsTotal == 0)
			{
				if (i->second != 0)
					++_itemsLostProperty[i->first];
			}
			else
			{
				_itemsLostProperty.erase(i->first);

				int roundsProperty;
				std::map<const RuleItem*, int>::const_iterator pClipsProperty (_clipsProperty.find(i->first));
				if (pClipsProperty != _clipsProperty.end())
					roundsProperty = pClipsProperty->second;
				else
					roundsProperty = 0;

				_itemsGained[i->first] = (i->second - roundsProperty) / qtyFullClip;

				_base->getStorageItems()->addItem(
												i->first->getType(),
												clipsTotal);
			}
		}
	}

	switch (tacType)
	{
		case TCT_BASEDEFENSE:
			if (_destroyXComBase == false)
			{
				for (std::vector<Craft*>::const_iterator
						i = _base->getCrafts()->begin();
						i != _base->getCrafts()->end();
						++i)
				{
					if ((*i)->getCraftStatus() != CS_OUT)
						reequipCraft(*i);
				}
			}
			else if (_skirmish == false)
			{
				for (std::vector<Base*>::const_iterator
						i = _gameSave->getBases()->begin();
						i != _gameSave->getBases()->end();
						++i)
				{
					if (*i == _base)
					{
						delete *i;
						_gameSave->getBases()->erase(i);
						break;
					}
				}
			}

			if (_region != nullptr)
			{
				const AlienMission* const retalMission (_game->getSavedGame()->findAlienMission(
																						_region->getRules()->getType(),
																						alm_RETAL));
				for (std::vector<Ufo*>::const_iterator
						i = _gameSave->getUfos()->begin();
						i != _gameSave->getUfos()->end();
						)
				{
					if ((*i)->getAlienMission() == retalMission)
					{
						delete *i;
						i = _gameSave->getUfos()->erase(i);
					}
					else
						++i;
				}

				for (std::vector<AlienMission*>::const_iterator
						i = _gameSave->getAlienMissions().begin();
						i != _gameSave->getAlienMissions().end();
						++i)
				{
					if (*i == retalMission)
					{
						delete *i;
						_gameSave->getAlienMissions().erase(i);
						break;
					}
				}
			}
			break;

		default: // not a BaseDefense.
			reequipCraft(_craft);
	}
}

/**
 * Reequips a Craft after tactical.
 * @param craft - pointer to a Craft
 */
void DebriefingState::reequipCraft(Craft* const craft) // private.
{
	if (craft == nullptr) return; // required.

	int
		baseQty,
		qtyLost;

	ItemContainer* craftContainer (craft->getCraftItems());
	const std::map<std::string, int> craftContents (*craftContainer->getContents()); // <- make a copy so you don't have to screw around with iteration here.
	for (std::map<std::string, int>::const_iterator
			i = craftContents.begin();
			i != craftContents.end();
			++i)
	{
		baseQty = _base->getStorageItems()->getItemQuantity(i->first);
		if (baseQty < i->second)
		{
			_base->getStorageItems()->removeItem(i->first, baseQty);

			qtyLost = i->second - baseQty;
			craftContainer->removeItem(i->first, qtyLost);

			const ReequipStat stat =
			{
				i->first,
				qtyLost,
				craft->getName(_game->getLanguage())
			};
			_missingItems.push_back(stat);
		}
		else
			_base->getStorageItems()->removeItem(i->first, i->second);
	}

	// First account for all craft-vehicles and delete each. Then re-add as many
	// as possible while redistributing all available ammunition. Note that the
	// Vehicles and their ammunition have already been sent to Base-storage.
	if (craft->getRules()->getVehicles() != 0)
	{
		ItemContainer craftVehicles;
		for (std::vector<Vehicle*>::const_iterator
				i = craft->getVehicles()->begin();
				i != craft->getVehicles()->end();
				++i)
		{
			craftVehicles.addItem((*i)->getRules()->getType());
			delete *i;
		}
		craft->getVehicles()->clear();

		int
			tanks,
			quadrants;
		for (std::map<std::string, int>::const_iterator
				i = craftVehicles.getContents()->begin();
				i != craftVehicles.getContents()->end();
				++i)
		{
			if ((baseQty = _base->getStorageItems()->getItemQuantity(i->first)) < i->second)
			{
				const ReequipStat stat =
				{
					i->first,
					i->second - baseQty,
					craft->getName(_game->getLanguage())
				};
				_missingItems.push_back(stat);
			}

			quadrants = _rules->getArmor(_rules->getUnitRule(i->first)->getArmorType())->getSize();
			quadrants *= quadrants;

			tanks = std::min(baseQty, i->second);

			const RuleItem* const itRule (_rules->getItemRule(i->first));

			if (itRule->getFullClip() < 1)
			{
				for (int
						j = 0;
						j != tanks;
						++j)
				{
					craft->getVehicles()->push_back(new Vehicle(
															itRule,
															itRule->getFullClip(),
															quadrants));
				}
				_base->getStorageItems()->removeItem(i->first, tanks);
			}
			else
			{
				const std::string type (itRule->getCompatibleAmmo()->front());
				const int
					clipsRequired (itRule->getFullClip()),
					baseClips (_base->getStorageItems()->getItemQuantity(type));

				if ((qtyLost = (clipsRequired * i->second) - baseClips) > 0)
				{
					const ReequipStat stat =
					{
						type,
						qtyLost,
						craft->getName(_game->getLanguage())
					};
					_missingItems.push_back(stat);
				}

				tanks = std::min(tanks,
								 baseClips / clipsRequired);
				if (tanks != 0)
				{
					for (int
							j = 0;
							j != tanks;
							++j)
					{
						craft->getVehicles()->push_back(new Vehicle(
																itRule,
																clipsRequired,
																quadrants));
					}
					_base->getStorageItems()->removeItem(i->first, tanks);
					_base->getStorageItems()->removeItem(type, clipsRequired * tanks);
				}
			}
		}
	}
}

/**
 * Recovers items from tactical.
 * @note Transfers the contents of a battlefield-inventory to the Base's stores.
 * This does not handle fixed-weapons/items.
 * @param battleItems - pointer to a vector of pointers to BattleItems on the battlefield
 */
void DebriefingState::recoverItems(std::vector<BattleItem*>* const battleItems) // private.
{
	const RuleItem* itRule;
	std::string type;

	for (std::vector<BattleItem*>::const_iterator
			i = battleItems->begin();
			i != battleItems->end();
			++i)
	{
		itRule = (*i)->getRules();

		if (itRule->isFixed() == false)
		{
			if (itRule->getBattleType() == BT_FUEL)
			{
				if (itRule->isRecoverable() == true)
				{
					_itemsGained[itRule] += _rules->getAlienFuelQuantity();
					addStat(
						_rules->getAlienFuelType(),
						100,
						_rules->getAlienFuelQuantity());
				}
			}
			else
			{
				type = itRule->getType();

				if (itRule->isRecoverable() == true // add pts. for unresearched items only
					&& itRule->getRecoveryPoints() != 0
					&& itRule->getBattleType() != BT_CORPSE
					&& _gameSave->isResearched(type) == false)
				{
					//Log(LOG_INFO) << ". . artefact = " << type;
					addStat(
						"STR_ALIEN_ARTIFACTS_RECOVERED",
						itRule->getRecoveryPoints());
				}

				switch (itRule->getBattleType()) // put items back in the Base
				{
					case BT_CORPSE:
					{
						BattleUnit* const unit ((*i)->getUnit());
						if (unit != nullptr)
						{
							if (itRule->isRecoverable() == true
								&& (unit->getUnitStatus() == STATUS_DEAD
									|| (unit->getUnitStatus() == STATUS_LIMBO // kL_tentative.
										&& unit->isOut_t(OUT_HEALTH) == true)))
							{
								//Log(LOG_INFO) << ". . corpse = " << type << " id-" << unit->getId();
								addStat(
									"STR_ALIEN_CORPSES_RECOVERED",
									unit->getValue() / 3); // TODO: This should rather be the 'recoveryPoints' of the corpse-item!

								std::string corpse (unit->getArmor()->getCorpseGeoscape());
								if (corpse.empty() == false) // safety.
								{
									_base->getStorageItems()->addItem(corpse);
									++_itemsGained[itRule];
								}
							}
							else if (unit->getUnitStatus() == STATUS_UNCONSCIOUS
								|| (unit->getUnitStatus() == STATUS_LIMBO
									&& unit->isOut_t(OUT_STUNNED) == true)) // kL_tentative.
							{
								switch (unit->getOriginalFaction()) // TODO: Add captured alien-types to a DebriefExtra screen.
								{
									case FACTION_HOSTILE:
										if (itRule->isRecoverable() == true)
										{
											++_aliensStunned; // for Nike Cross determination.
											recoverLiveAlien(unit);
										}
										break;

									case FACTION_NEUTRAL:
										//Log(LOG_INFO) << ". . unconsciousCivie = " << type;
										addStat(
											"STR_CIVILIANS_SAVED",
											unit->getValue()); // duplicated above.
								}
							}
						}
						break;
					}

					case BT_AMMO:
						if (itRule->isRecoverable() == true)
						{
							_clips[itRule] += (*i)->getAmmoQuantity();
							if ((*i)->getProperty() == true)
								_clipsProperty[itRule] += (*i)->getAmmoQuantity();
						}
						break;

					case BT_FIREARM:
						if (itRule->isRecoverable() == true
							&& (*i)->selfPowered() == false)
						{
							const BattleItem* const clip ((*i)->getAmmoItem());
							if (clip != nullptr) //&& clip->getRules()->getFullClip() != 0) // <- nobody be stupid and make a clip with 0 ammo-capacity.
							{
								_clips[clip->getRules()] += clip->getAmmoQuantity();
								if ((*i)->getProperty() == true)
									_clipsProperty[clip->getRules()] += clip->getAmmoQuantity();
							}
						} // no break;

					default:
						if (itRule->isRecoverable() == true)
						{
							_base->getStorageItems()->addItem(type);
							if ((*i)->getProperty() == false)
								++_itemsGained[itRule];
						}
				}
			}
		}
	}
}

/**
 * Recovers a live aLien from the battlefield.
 * @param unit - pointer to a BattleUnit to recover
 */
void DebriefingState::recoverLiveAlien(BattleUnit* const unit) // private.
{
//	std::string type;
//	if ((*i)->getSpawnType().empty() == false)	// btw. This should never happen.
//		type = (*i)->getSpawnType();			// Zombies can't be MC'd basically. Can't be stunned either.
//	else										// And Soldiers should spawn into zombies ~immediately.
//		type = (*i)->getType();					// Plus aLiens can't be zombified.

//	if (unit->getSpawnType().empty() == false) // DON'T USE THIS IT CAN BREAK THE ITERATOR.
//	{
//		BattleUnit* const conUnit (_gameSave->getBattleSave()->getBattleGame()->speedyConvert(unit));
//		conUnit->setFaction(FACTION_PLAYER);
//		return;
//	}

	if (_base->hasContainment() == true)
	{
		//Log(LOG_INFO) << ". . . alienLive = " << unit->getType() << " id-" << unit->getId();
		const std::string type (unit->getType());

		int value;
		if (_gameSave->isResearched(type) == false)
			value = unit->getValue() * 2;
		else
			value = unit->getValue();

		addStat(
			"STR_LIVE_ALIENS_RECOVERED",
			value);

		_base->getStorageItems()->addItem(type);
		_manageContainment = _base->getFreeContainment() < 0;
	}
	else
	{
		//Log(LOG_INFO) << ". . . alienDead = " << unit->getType();
		_alienDies = true;
		addStat(
			"STR_ALIEN_CORPSES_RECOVERED",
			unit->getValue() / 3);

//		std::string corpseItem;
//		if (unit->getSpawnType().empty() == false)
//			corpseItem = _rules->getArmor(_rules->getUnit(unit->getSpawnType())->getArmor())->getCorpseGeoscape();
//		else
//			corpseItem = unit->getArmor()->getCorpseGeoscape();
//		const std::string corpseItem (unit->getArmor()->getCorpseGeoscape());
//
//		if (corpseItem.empty() == false) // safety.
//			_base->getStorageItems()->addItem(corpseItem);

		std::string corpse (unit->getArmor()->getCorpseGeoscape());
		if (corpse.empty() == false) // safety. [Or error-out if there isn't one.]
		{
			_base->getStorageItems()->addItem(corpse);
			++_itemsGained[_rules->getItemRule(corpse)];
		}
	}
}

}
