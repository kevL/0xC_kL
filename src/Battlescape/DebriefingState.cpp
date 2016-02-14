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

//#include <sstream>

#include "CannotReequipState.h"
#include "CeremonyDeadState.h"
#include "CeremonyState.h"
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
#include "../Savegame/MissionStatistics.h"
#include "../Savegame/Region.h"
#include "../Savegame/SavedBattleGame.h"
#include "../Savegame/SoldierDead.h"
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
		_noContainment(false),
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
//	if (_gameSave->getBattleSave()->getBattleGame())
	_gameSave->getBattleSave()->getBattleGame()->cleanupDeleted();

	_missionStatistics	= new MissionStatistics();

	_window				= new Window(this, 320, 200);

	_txtTitle			= new Text(280, 17,  16, 8);
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

	std::wstring wst;
	if (_gameSave->getBattleSave()->getOperation().empty() == false)
		wst = _gameSave->getBattleSave()->getOperation();
	else
		wst = tr("STR_OK");
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
								tr((*i)->item).c_str(),
								woststr1.str().c_str(),
								woststr2.str().c_str());
				recov_offY += 8;
			}
			else
			{
				_lstStats->addRow(
								3,
								tr((*i)->item).c_str(),
								woststr1.str().c_str(),
								woststr2.str().c_str());
				stats_offY += 8;
			}

			if ((*i)->item == "STR_CIVILIANS_SAVED")
				civiliansSaved = (*i)->qty;

			if ((*i)->item == "STR_CIVILIANS_KILLED_BY_XCOM_OPERATIVES"
				|| (*i)->item == "STR_CIVILIANS_KILLED_BY_ALIENS")
			{
				civiliansDead += (*i)->qty;
			}

			if ((*i)->item == "STR_ALIENS_KILLED")
				_aliensKilled += (*i)->qty;
		}
	}

	_missionStatistics->score = total;

	if (civiliansSaved != 0
		&& civiliansDead == 0
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
				_missionStatistics->injuryList[sol->getId()] = sol->getRecovery();

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
		_gameSave->setBattleSave(nullptr);

	for (std::vector<DebriefingStat*>::const_iterator
			i = _stats.begin();
			i != _stats.end();
			++i)
	{
		delete *i;
	}

	for (std::map<int, SpecialType*>::const_iterator
			i = _specialTypes.begin();
			i != _specialTypes.end();
			++i)
	{
		delete i->second;
	}
	_specialTypes.clear();

	_rounds.clear();
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

	std::vector<Soldier*> participants;
	for (std::vector<BattleUnit*>::const_iterator
			i = _gameSave->getBattleSave()->getUnits()->begin();
			i != _gameSave->getBattleSave()->getUnits()->end();
			++i)
	{
		if ((*i)->getGeoscapeSoldier() != nullptr)
			participants.push_back((*i)->getGeoscapeSoldier());
	}

	_gameSave->setBattleSave(nullptr);
	_game->popState();

	if (_skirmish == true)
		_game->setState(new MainMenuState());
	else
	{
		if (_destroyXComBase == false)
		{
			bool playAwardMusic (false);

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

			if (_gameSave->handlePromotions(participants) == true)
			{
				playAwardMusic = true;
				_game->pushState(new PromotionsState());
			}

			if (_missingItems.empty() == false)
				_game->pushState(new CannotReequipState(_missingItems));

			if (_noContainment == true)
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
}

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
		if ((*i)->item == type)
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
	for (std::vector<std::string>::const_iterator
			i = _rules->getItemsList().begin();
			i != _rules->getItemsList().end();
			++i)
	{
		const SpecialTileType tileType (_rules->getItem(*i)->getSpecialType());
		if (tileType > 1)
		{
			SpecialType* const specialType (new SpecialType());
			specialType->type = *i;
			specialType->value = _rules->getItem(*i)->getRecoveryPoints();

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

	const bool aborted (battleSave->isAborted());

	int
		soldierExit (0),
		soldierLive (0),
		soldierDead (0), // Soldier Diary.
		soldierOut  (0);

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
				if ((*i)->getUnitStatus() == STATUS_UNCONSCIOUS
					|| (*i)->getFaction() == FACTION_HOSTILE)
				{
					++soldierOut;
				}
				++soldierLive;
			}
			else
			{
				++soldierDead;
				if ((*i)->getGeoscapeSoldier() != nullptr)
					(*i)->getStatistics()->KIA = true;
			}
		}
	}

	// if all xCom units are unconscious the aliens get to have their way with them
	if (soldierOut == soldierLive)
	{
		soldierLive = 0;

		for (std::vector<BattleUnit*>::const_iterator
				i = battleSave->getUnits()->begin();
				i != battleSave->getUnits()->end();
				++i)
		{
			if ((*i)->getOriginalFaction() == FACTION_PLAYER
				&& (*i)->getUnitStatus() != STATUS_DEAD)
			{
				(*i)->instaKill();

				if ((*i)->getGeoscapeSoldier() != nullptr)
					(*i)->getStatistics()->MIA = true;
			}
		}
	}

	if (soldierLive == 1)
	{
		for (std::vector<BattleUnit*>::const_iterator
				i = battleSave->getUnits()->begin();
				i != battleSave->getUnits()->end();
				++i)
		{
			if ((*i)->getGeoscapeSoldier() != nullptr
				&& (*i)->getUnitStatus() != STATUS_DEAD)
			{
				if (soldierDead == 0			// only one soldier went on the mission if
					&& _aliensControlled == 0	// only one soldier survived AND none have died
					&& _aliensKilled + _aliensStunned > 1 + _diff
					&& aborted == false)
				{
					(*i)->getStatistics()->ironMan = true;
					break;
				}
				else if ((*i)->getStatistics()->hasFriendlyFired() == false	// if only one soldier survived give him a medal!
					&& soldierDead != 0)									// unless he killed all the others ...
				{
					(*i)->getStatistics()->loneSurvivor = true;
					break;
				}
			}
		}
	}


	bool missionAccomplished ((aborted == false && soldierLive != 0)
							|| battleSave->allObjectivesDestroyed() == true);

	std::vector<Craft*>::const_iterator pCraft;

	double
		lon (0.), // avoid vc++ linker warnings.
		lat (0.); // avoid vc++ linker warnings.

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
			if ((*j)->getTactical() == true)
			{
				if (_skirmish == false)
					_missionCost = (*i)->craftExpense(*j);

				lon = (*j)->getLongitude();
				lat = (*j)->getLatitude();

				_base = *i;
				_craft = *j;
				pCraft = j;

				_craft->returnToBase();
				_craft->setTacticalReturn();
				_craft->setTactical(false);
			}
			else if ((*j)->getDestination() != nullptr)
			{
				if (soldierLive != 0 && aborted == false)
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

		if ((*i)->getTactical() == true) // in case this DON'T have a craft (ie. baseDefense)
		{
			_base = *i;
			_txtBaseLabel->setText(_base->getName(_game->getLanguage()));

			lon = _base->getLongitude();
			lat = _base->getLatitude();

			if (soldierLive != 0 && aborted == false)
			{
				_base->setTactical(false);
				_base->cleanupBaseDefense(); // so ... does this mean that each tank's entire 'clip' gets wasted

				bool facDestroyed = false;
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

			if (soldierLive != 0 && aborted == false)
			{
				delete *i;
				_gameSave->getUfos()->erase(i);
			}
			else
			{
				(*i)->setTactical(false);
				if ((*i)->getUfoStatus() == Ufo::LANDED)
					(*i)->setSecondsLeft(15);
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

				if (missionAccomplished == true)
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
							_gameSave->getAlienMissions().begin(), // remove supply missions for the aLien base.
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
							if (*j == sol)
							{
								(*i)->postMissionProcedures(true);
								(*j)->die(_gameSave);

								delete *j;
								_base->getSoldiers()->erase(j);

								// note: Could return any armor the soldier was wearing to Stores. CHEATER!!!!!
								break;
							}
						}
					}
					else
					{
						if (_skirmish == false)
							_missionCost += _base->hwpExpense(
														(*i)->getArmor()->getSize() * (*i)->getArmor()->getSize(),
														true);
						addStat(
							"STR_TANKS_DESTROYED",
							-value);
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
		else
		{
			//Log(LOG_INFO) << ". unitLive " << (*i)->getId() << " type = " << (*i)->getType();
			switch (orgFaction)
			{
				case FACTION_PLAYER:
					if (aborted == false
						|| (((*i)->isInExitArea() == true || (*i)->getUnitStatus() == STATUS_LIMBO)
							&& (/*missionAccomplished == true ||*/ tacType != TCT_BASEDEFENSE)))
					{
						++soldierExit;
						recoverItems((*i)->getInventory());

						Soldier* const sol ((*i)->getGeoscapeSoldier());
						if (sol != nullptr)
						{
							(*i)->postMissionProcedures();

							if (_skirmish == false)
								_missionCost += _base->soldierExpense(sol);

//							sol->calcStatString( // calculate new statString
//											_rules->getStatStrings(),
//											Options::psiStrengthEval
//												&& _gameSave->isResearched(_rules->getPsiRequirements()));
						}
						else
						{
							_base->getStorageItems()->addItem((*i)->getType());

							if (_skirmish == false)
								_missionCost += _base->hwpExpense((*i)->getArmor()->getSize() * (*i)->getArmor()->getSize());

							const RuleItem* supportRule;
							const BattleItem* aItem;

							if ((*i)->getItem(ST_RIGHTHAND) != nullptr)
							{
								supportRule = _rules->getItem((*i)->getType()); // note this is basically the support-unit itself.
								if (supportRule->getCompatibleAmmo()->empty() == false)
								{
									aItem = (*i)->getItem(ST_RIGHTHAND)->getAmmoItem();
									if (aItem != nullptr) //&& aItem->getAmmoQuantity() > 0)
									{
										int total (aItem->getAmmoQuantity());
										if (supportRule->getClipSize() != 0) // meaning this tank can store multiple clips-of-clips, yeh ...
											total /= aItem->getRules()->getClipSize();

										_base->getStorageItems()->addItem(
																		supportRule->getCompatibleAmmo()->front(),
																		total);
									}
								}
							}

							if ((*i)->getItem(ST_LEFTHAND) != nullptr)
							{
								supportRule = (*i)->getItem(ST_LEFTHAND)->getRules();
								if (supportRule->getCompatibleAmmo()->empty() == false)
								{
									aItem = (*i)->getItem(ST_LEFTHAND)->getAmmoItem();
									if (aItem != nullptr) //&& aItem->getAmmoQuantity() > 0)
									{
										int total (aItem->getAmmoQuantity());
										if (supportRule->getClipSize() != 0) // meaning this tank can store multiple  clips-of-clips
											total /= aItem->getRules()->getClipSize();

										_base->getStorageItems()->addItem(
																		supportRule->getCompatibleAmmo()->front(),
																		total);
									}
								}
							}
						}
					}
					else
					{
						addStat(
							"STR_XCOM_OPERATIVES_MISSING_IN_ACTION",
							-value);

						Soldier* const sol ((*i)->getGeoscapeSoldier());
						if (sol != nullptr)
						{
							for (std::vector<Soldier*>::const_iterator
									j = _base->getSoldiers()->begin();
									j != _base->getSoldiers()->end();
									++j)
							{
								if (*j == sol)
								{
									(*i)->postMissionProcedures(true);
									(*i)->getStatistics()->MIA = true;
									(*i)->instaKill();

									(*j)->die(_gameSave);

									delete *j;
									_base->getSoldiers()->erase(j);

									// note: Could return any armor the soldier was wearing to Stores. CHEATER!!!!!
									break;
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
								(*i)->getTile()->addItem(
													*j,
													_rules->getInventoryRule(ST_GROUND));
						}

						recoverLiveAlien(*i);
					}
					break;

				case FACTION_NEUTRAL:
					if (soldierLive == 0
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

	if (_craft != nullptr // Craft lost.
		&& (soldierLive == 0
			|| (aborted == true && soldierExit == 0)))
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

		// Since this is not a Base Defense tactical the Craft can safely be
		// erased/deleted without worrying about its vehicles' destructor called
		// twice (on base defense missions all vehicle objects in the craft are
		// also referenced by base->getVehicles() !!).
		delete _craft;

		_craft = nullptr; // To avoid a crash down there!! uh, not after return; it won't.
		_base->getCrafts()->erase(pCraft);
		_txtTitle->setText(tr("STR_CRAFT_IS_LOST"));

		return;
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
	if (missionAccomplished == true)
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

	if (soldierLive != 0)
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
				if ((*i)->item == _specialTypes[ALIEN_ALLOYS]->type)
				{
					int alloyDivisor;
					if (tacType == TCT_BASEASSAULT)
						alloyDivisor = 150;
					else
						alloyDivisor = 15;

					(*i)->qty /= alloyDivisor;
					(*i)->score /= alloyDivisor;
				}

				if ((*i)->recover == true && (*i)->qty != 0)
					_base->getStorageItems()->addItem((*i)->item, (*i)->qty);
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

	for (std::map<RuleItem*, int>::const_iterator
			i = _rounds.begin();
			i != _rounds.end();
			++i)
	{
		if (i->first->getClipSize() != 0)
		{
			const int fullClips (i->second / i->first->getClipSize());
			if (fullClips != 0)
				_base->getStorageItems()->addItem(
												i->first->getType(),
												fullClips);
		}
	}

	if (_craft != nullptr)
		reequipCraft();

	if (tacType == TCT_BASEDEFENSE)
	{
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

			for (std::vector<Vehicle*>::const_iterator
					i = _base->getVehicles()->begin();
					i != _base->getVehicles()->end();
					++i)
			{
				delete *i;
			}

			_base->getVehicles()->clear();
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
	}

	_missionStatistics->success = missionAccomplished;
}

/**
 * Reequips a craft after a mission.
 * @param craft - pointer to Craft (default nullptr)
 */
void DebriefingState::reequipCraft(Craft* craft) // private.
{
	bool vehicleDestruction;
	if (craft != nullptr)
		vehicleDestruction = false;
	else
	{
		craft = _craft;
		vehicleDestruction = true;
	}

	int
		qtyBase,
		qtyLost;
//	int used; // kL

	const std::map<std::string, int> craftItems (*craft->getCraftItems()->getContents());
	for (std::map<std::string, int>::const_iterator
			i = craftItems.begin();
			i != craftItems.end();
			++i)
	{
		qtyBase = _base->getStorageItems()->getItemQuantity(i->first);

		if (qtyBase >= i->second)
		{
			_base->getStorageItems()->removeItem(i->first, i->second);
//			used = i->second; // kL
		}
		else
		{
			_base->getStorageItems()->removeItem(i->first, qtyBase);

			qtyLost = i->second - qtyBase;
			craft->getCraftItems()->removeItem(i->first, qtyLost);
//											i->second - qtyBase); // kL
//			used = qtyLost; // kL

			const ReequipStat stat =
			{
				i->first,
				qtyLost,
				craft->getName(_game->getLanguage())
			};

			_missingItems.push_back(stat);
		}

		// kL_begin:
		// NOTE: quantifying what was Used on the battlefield is difficult,
		// because *all items recovered* on the 'field, including the craft's
		// stores, have already been added to base->Stores before this function
		// runs. An independent vector of either base->Stores or craft->Stores
		// is NOT maintained.

//		used = i->second - qtyBase;
//		used = qtyBase - i->second;
//		if (used > 0)
/*		{
			ReequipStat stat =
			{
				i->first,
//				i->second,
				used,
				craft->getName(_game->getLanguage())
			};
			_missingItems.push_back(stat);
		} */
		// kL_end.
	}

	ItemContainer craftVehicles;
	for (std::vector<Vehicle*>::const_iterator
			i = craft->getVehicles()->begin();
			i != craft->getVehicles()->end();
			++i)
	{
		craftVehicles.addItem((*i)->getRules()->getType());
	}

	// Now it's known how many vehicles - separated by types - have to be read.
	// Erase the current vehicles because they have to be reAdded - to
	// redistribute their ammo - and generally weave your way through this
	// vehicle-related-spaghetti .......
	if (vehicleDestruction == true)
	{
		for (std::vector<Vehicle*>::const_iterator
				i = craft->getVehicles()->begin();
				i != craft->getVehicles()->end();
				++i)
		{
			delete *i;
		}
	}

	craft->getVehicles()->clear();

	int addTanks;

	for (std::map<std::string, int>::const_iterator // Ok, now read those vehicles!
			i = craftVehicles.getContents()->begin();
			i != craftVehicles.getContents()->end();
			++i)
	{
		qtyBase = _base->getStorageItems()->getItemQuantity(i->first);
		addTanks = std::min(
						qtyBase,
						i->second);

		if (qtyBase < i->second)
		{
			qtyLost = i->second - qtyBase; // missing tanks
			const ReequipStat stat =
			{
				i->first,
				qtyLost,
				craft->getName(_game->getLanguage())
			};
			_missingItems.push_back(stat);
		}


		const RuleItem* const tankRule (_rules->getItem(i->first));

		const RuleUnit* const tankUnit (_rules->getUnitRule(tankRule->getType()));
		int tankSize;
		if (tankUnit != nullptr)
		{
			tankSize = _rules->getArmor(tankUnit->getArmor())->getSize();
			tankSize *= tankSize;
		}
		else
			tankSize = 4; // safety.

		if (tankRule->getCompatibleAmmo()->empty() == true) // so this tank does NOT require ammo
		{
			for (int
					j = 0;
					j != addTanks;
					++j)
			{
				craft->getVehicles()->push_back(new Vehicle(
														tankRule,
														tankRule->getClipSize(),
														tankSize));
			}

			_base->getStorageItems()->removeItem(i->first, addTanks);
		}
		else // so this tank requires ammo
		{
			const RuleItem* const aRule (_rules->getItem(tankRule->getCompatibleAmmo()->front()));
			int
				tankClipSize,
				requiredRounds;
			if (aRule->getClipSize() > 0 && tankRule->getClipSize() > 0)
			{
				requiredRounds = tankRule->getClipSize();
				tankClipSize = requiredRounds / aRule->getClipSize();
			}
			else
			{
				requiredRounds = aRule->getClipSize();
				tankClipSize = requiredRounds;
			}

			const int baseQty (_base->getStorageItems()->getItemQuantity(aRule->getType())); // Ammo Quantity for this vehicle-type at the Base

			if (baseQty < i->second * tankClipSize)
			{
				qtyLost = (i->second * tankClipSize) - baseQty; // missing ammo
				const ReequipStat stat =
				{
					aRule->getType(),
					qtyLost,
					craft->getName(_game->getLanguage())
				};
				_missingItems.push_back(stat);
			}

			addTanks = std::min(
							addTanks,
							baseQty / tankClipSize);

			if (addTanks != 0)
			{
				for (int
						j = 0;
						j != addTanks;
						++j)
				{
					craft->getVehicles()->push_back(new Vehicle(
															tankRule,
															requiredRounds,
															tankSize));
					_base->getStorageItems()->removeItem(
													aRule->getType(),
													tankClipSize);
				}
				_base->getStorageItems()->removeItem(
												i->first,
												addTanks);
			}
		}

		// kL_begin:
/*		ReequipStat stat =
		{
			i->first,
			addTanks,
			craft->getName(_game->getLanguage())
		};
		_missingItems.push_back(stat); */
		// kL_end.
	}
}

/**
 * Recovers items from the battlescape.
 * @note Converts the battlescape inventory into a geoscape ItemContainer.
 * @param battleItems - pointer to a vector of pointers to BattleItems on the battlefield
 */
void DebriefingState::recoverItems(std::vector<BattleItem*>* const battleItems) // private.
{
	RuleItem* itRule;

	for (std::vector<BattleItem*>::const_iterator
			i = battleItems->begin();
			i != battleItems->end();
			++i)
	{
		itRule = (*i)->getRules();

		if (itRule->isFixed() == false)
		{
			if (itRule->getType() == _rules->getAlienFuelType())
			{
				if (itRule->isRecoverable() == true)
					addStat( // special case of an item counted as a stat
						_rules->getAlienFuelType(),
						100,
						_rules->getAlienFuelQuantity());
			}
			else
			{
				if (itRule->isRecoverable() == true // add pts. for unresearched items only
					&& itRule->getRecoveryPoints() != 0
					&& itRule->getBattleType() != BT_CORPSE
					&& _gameSave->isResearched(itRule->getType()) == false)
				{
					//Log(LOG_INFO) << ". . artefact = " << itRule->getType();
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
										&& unit->isOut_t(OUT_HLTH) == true)))
							{
								//Log(LOG_INFO) << ". . corpse = " << itRule->getType() << " id-" << unit->getId();
								addStat(
									"STR_ALIEN_CORPSES_RECOVERED",
									unit->getValue() / 3); // This should rather be the 'recoveryPoints' of the corpse item!

								if (unit->getArmor()->getCorpseGeoscape().empty() == false) // safety.
									_base->getStorageItems()->addItem(unit->getArmor()->getCorpseGeoscape());
							}
							else if (unit->getUnitStatus() == STATUS_UNCONSCIOUS
								|| (unit->getUnitStatus() == STATUS_LIMBO
									&& unit->isOut_t(OUT_STUN) == true)) // kL_tentative.
							{
								if (itRule->isRecoverable() == true
									&& unit->getOriginalFaction() == FACTION_HOSTILE)
								{
									// ADD STUNNED ALIEN COUNTING HERE_kL
									++_aliensStunned; // for Nike Cross determination.

									recoverLiveAlien(unit);
								}
								else if (unit->getOriginalFaction() == FACTION_NEUTRAL)
								{
									//Log(LOG_INFO) << ". . unconsciousCivie = " << itRule->getType();
									addStat(
										"STR_CIVILIANS_SAVED",
										unit->getValue()); // duplicated above.
								}
							}
						}
						break;
					}

					case BT_AMMO: // item is a clip - count rounds left
						if (itRule->isRecoverable() == true)
							_rounds[itRule] += (*i)->getAmmoQuantity();
						break;

					case BT_FIREARM: // item is a weapon - count rounds in clip
//					case BT_MELEE:
						if (itRule->isRecoverable() == true)
						{
							const BattleItem* const clip ((*i)->getAmmoItem());
							if (clip != nullptr
								&& clip != *i
								&& clip->getRules()->getClipSize() > 0)
							{
								_rounds[clip->getRules()] += clip->getAmmoQuantity();
							}
						}

					default:		// fall-through - recover the item/weapon itself
					case BT_MELEE:	// kL_note: I don't want Melee using ammo ... although it's technically possible in the rulesets.
						if (itRule->isRecoverable() == true)
							_base->getStorageItems()->addItem(itRule->getType());
				}
			}
		}
	}
}

/**
 * Recovers a live alien from the battlescape.
 * @param unit - pointer to a BattleUnit to recover
 */
void DebriefingState::recoverLiveAlien(BattleUnit* const unit) // private.
{
	if (unit->getSpawnUnit().empty() == false)
	{
		BattleUnit* const conUnit (_gameSave->getBattleSave()->getBattleGame()->convertUnit(unit)); // TODO: use a sparsed down version of convertUnit() here.
		conUnit->setFaction(FACTION_PLAYER);
	}
	else if (_base->hasContainment() == true)
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
		_noContainment = true;
		addStat(
			"STR_ALIEN_CORPSES_RECOVERED",
			unit->getValue() / 3);

//		std::string corpseItem;
//		if (unit->getSpawnUnit().empty() == false)
//			corpseItem = _rules->getArmor(_rules->getUnit(unit->getSpawnUnit())->getArmor())->getCorpseGeoscape();
//		else
//			corpseItem = unit->getArmor()->getCorpseGeoscape();
		const std::string corpseItem (unit->getArmor()->getCorpseGeoscape());

		if (corpseItem.empty() == false) // safety.
			_base->getStorageItems()->addItem(corpseItem);
	}
}

}
