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

#include "../Ruleset/RuleAlienDeployment.h"
#include "../Ruleset/RuleArmor.h"
#include "../Ruleset/RuleCountry.h"
//#include "../Ruleset/RuleCraft.h"
#include "../Ruleset/RuleInterface.h"
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
#include "../Savegame/Region.h"
#include "../Savegame/SavedBattleGame.h"
#include "../Savegame/SoldierDiary.h"
#include "../Savegame/TerrorSite.h"
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
		_battleSave(_game->getSavedGame()->getBattleSave()),
		_unitList(_game->getSavedGame()->getBattleSave()->getUnits()),
		_aborted(_game->getSavedGame()->getBattleSave()->isAborted()),
		_diff(static_cast<int>(_game->getSavedGame()->getDifficulty())),
		_isQuickBattle(_game->getSavedGame()->getMonthsPassed() == -1),
		_region(nullptr),
		_country(nullptr),
		_base(nullptr),
		_craft(nullptr),
		_alienDies(false),
		_manageContainment(false),
		_destroyPlayerBase(false),
		_missionCost(0),
		_aliensStunned(0),
		_playerDead(0),
		_playerLive(0),
		_isHostileStanding(0)
{
	Options::baseXResolution = Options::baseXGeoscape;
	Options::baseYResolution = Options::baseYGeoscape;
	_game->getScreen()->resetDisplay(false);

	// Restore the cursor in case something weird happened.
	_game->getCursor()->setVisible();

	// Clean up the leftover states from BattlescapeGame; was done in
	// ~BattlescapeGame but that causes CTD under reLoad situation. Now done
	// here and in NextTurnState; not ideal: should find a safe place when
	// BattlescapeGame is really dTor'd and not reLoaded ...... uh, i guess.
	_battleSave->getBattleGame()->cleanBattleStates();

	_tactical		= new MissionStatistics();

	_window			= new Window(this, 320, 200);

	_txtTitle		= new Text(280, 16,  16, 8);
	_txtBaseLabel	= new Text( 80,  9, 216, 8);

	_txtItem		= new Text(184, 9,  16, 24);
	_txtQuantity	= new Text( 60, 9, 200, 24);
	_txtScore		= new Text( 36, 9, 260, 24);

	_lstStats		= new TextList(288, 81, 16, 32);

	_lstRecovery	= new TextList(288, 81, 16, 32);
	_txtRecovery	= new Text(180, 9, 16, 60);

	_lstTotal		= new TextList(288, 9, 16, 12);

	_txtCost		= new Text(76, 9, 16, 180);
	_btnOk			= new TextButton(136, 16, 92, 177);
	_txtRating		= new Text(76, 9, 228, 180);

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

	std::wstring wst (_battleSave->getOperation());
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

	_txtBaseLabel->setAlign(ALIGN_RIGHT);	// NOTE: Text is set in prepareDebriefing() before
											// a possibly failed BaseDefense dangles '_base' ptr.


	prepareDebriefing(); // <- |-- GATHER ALL DATA HERE <- < ||


	int
		stats_offY		(0),
		recov_offY		(0),
		aliensKilled	(0),
		civiliansSaved	(0),
		civiliansDead	(0);

	for (std::vector<DebriefingStat*>::const_iterator
			i = _statList.begin();
			i != _statList.end();
			++i)
	{
		if ((*i)->qty != 0)
		{
			_tactical->score += (*i)->score;

			std::wostringstream
				woststr1,
				woststr2;

			woststr1 << L'\x01' << (*i)->qty;	// quantity of recovered item Type
			woststr2 << L'\x01' << (*i)->score;	// score for items of Type
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

			if ((*i)->type == "STR_ALIENS_KILLED")
				aliensKilled = (*i)->qty;
			else if ((*i)->type == "STR_CIVILIANS_SAVED")
				civiliansSaved = (*i)->qty;
			else if ((*i)->type == "STR_CIVILIANS_KILLED_BY_ALIENS"
				|| (*i)->type == "STR_CIVILIANS_KILLED_BY_PLAYER")
			{
				civiliansDead += (*i)->qty;
			}
		}
	}

	std::wostringstream woststr;
	woststr << _tactical->score;
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
		if (_destroyPlayerBase == true)
		{
			_region->addActivityAlien((_diff + 1) * 235);
			_region->recentActivityAlien();
		}
		else
		{
			_region->addActivityXCom(_tactical->score);
			_region->recentActivityXCom();
		}
	}

	if (_country != nullptr)
	{
		if (_destroyPlayerBase == true)
		{
			_country->addActivityAlien((_diff + 1) * 235);
			_country->recentActivityAlien();
		}
		else
		{
			_country->addActivityXCom(_tactical->score);
			_country->recentActivityXCom();
		}
	}

	if (_tactical->score < -99)
	{
		_music = OpenXcom::res_MUSIC_TAC_DEBRIEFING_BAD;
		_tactical->rating = "STR_RATING_TERRIBLE";
	}
	else
	{
		_music = OpenXcom::res_MUSIC_TAC_DEBRIEFING;
		if		(_tactical->score <  101)	_tactical->rating = "STR_RATING_POOR";
		else if	(_tactical->score <  351)	_tactical->rating = "STR_RATING_OK";
		else if	(_tactical->score <  751)	_tactical->rating = "STR_RATING_GOOD";
		else if	(_tactical->score < 1251)	_tactical->rating = "STR_RATING_EXCELLENT";
		else								_tactical->rating = "STR_RATING_STUPENDOUS";
	}

	if (_isQuickBattle == false && _missionCost != 0)
	{
//		_txtCost->setText(tr("STR_COST_").arg(Text::formatCurrency(_missionCost)));
		_txtCost->setText(Text::formatCurrency(_missionCost));
		_txtCost->setAlign(ALIGN_CENTER);
	}
	else
		_txtCost->setVisible(false);

//	_txtRating->setText(tr("STR_RATING_").arg(tr(_tactical->rating)));
	_txtRating->setText(tr(_tactical->rating));
	_txtRating->setAlign(ALIGN_CENTER);


	// Soldier Diary ->
	if (_isQuickBattle == false) // TODO: Show some stats for quick-battles.
	{
		_tactical->id			= _gameSave->getMissionStatistics()->size();
		_tactical->timeStat		= *_gameSave->getTime();
		_tactical->type			= _battleSave->getTacticalType();
		_tactical->shade		= _battleSave->getTacticalShade();
		_tactical->alienRace	= _battleSave->getAlienRace();

		if (civiliansSaved != 0 && civiliansDead == 0)
			_tactical->valiantCrux = true;

		const bool checkIron (_playerDead == 0
						   && _aborted == false
						   && _aliensStunned + aliensKilled > 1 + _diff);

		//Log(LOG_INFO) << "DebriefingState::cTor";
		Soldier* sol;
		SoldierDead* solDead;
		std::vector<MissionStatistics*>* const tacticals (_game->getSavedGame()->getMissionStatistics());
		BattleUnitStatistics* diaryStats;

		for (std::vector<BattleUnit*>::const_iterator
				i = _unitList->begin();
				i != _unitList->end();
				++i)
		{
			//Log(LOG_INFO) << ". iter BattleUnits";
			// NOTE: In the case of a dead soldier this pointer is Valid but points to garbage.
			// Use that. Pointer is NULL'd below_
			if ((sol = (*i)->getGeoscapeSoldier()) != nullptr)
			{
				//Log(LOG_INFO) << ". . id = " << (*i)->getId();
				diaryStats = (*i)->getStatistics();

				if (_playerLive == 1)
				{
					switch ((*i)->getUnitStatus())
					{
						case STATUS_STANDING:
							if (checkIron == true)
							{
								diaryStats->ironMan = true;
								break;
							}
							// no break;
						case STATUS_UNCONSCIOUS:
							if (_playerDead != 0
								&& diaryStats->hasFriendlyFired() == false)
							{
								diaryStats->loneSurvivor = true;
							}
					}
				}

				if (_isHostileStanding == false)
				{
					int take (0);
					for (std::vector<BattleUnitKill*>::const_iterator
							j = diaryStats->kills.begin();
							j != diaryStats->kills.end();
							++j)
					{
						if ((*j)->_faction == FACTION_HOSTILE)
							++take;
					}

					// TODO: re. Nike Cross:
					// This can be exploited by MC'ing all aLiens and then
					// executing all aLiens with a single Soldier.
					if (take == _aliensStunned + aliensKilled
						&& take > 3 + _diff)
					{
						diaryStats->nikeCross = true;
					}
				}


				switch ((*i)->getUnitStatus())
				{
					case STATUS_STANDING:
					case STATUS_UNCONSCIOUS:
						//Log(LOG_INFO) << ". . . alive";
						if ((diaryStats->daysWounded = sol->getSickbay()) != 0)
							_tactical->injuryList[sol->getId()] = diaryStats->daysWounded;

						sol->getDiary()->updateDiary(
												diaryStats,
												_tactical,
												_rules);
						if (sol->getDiary()->manageAwards(_rules, tacticals) == true)
							_soldiersFeted.push_back(sol);
						break;

					case STATUS_DEAD:
						//Log(LOG_INFO) << ". . . dead";
						sol = nullptr;	// Zero out the BattleUnit from the geoscape Soldiers list
										// in this State; it's already gone from his/her former Base.
										// This makes them ineligible for promotion.
										// PS, there is no "geoscape Soldiers list" really; it's
										// just a variable stored on each xCom-agent/BattleUnit ....

						solDead = nullptr; // avoid vc++ linker warning.
						for (std::vector<SoldierDead*>::const_iterator
								j = _gameSave->getDeadSoldiers()->begin();
								j != _gameSave->getDeadSoldiers()->end();
								++j)
						{
							if ((*j)->getId() == (*i)->getId())
							{
								solDead = *j;
								break;
							}
						}

						diaryStats->daysWounded = 0;

						// NOTE: Safety on *solDead shall not be needed. see above^
						if (diaryStats->KIA == true)
							_tactical->injuryList[solDead->getId()] = -1; // kia
						else
							_tactical->injuryList[solDead->getId()] = -2; // mia

						solDead->getDiary()->updateDiary(
														diaryStats,
														_tactical,
														_rules);
						solDead->getDiary()->manageAwards(_rules, tacticals);
						_soldiersLost.push_back(solDead);
				}
			}
		}
		_gameSave->getMissionStatistics()->push_back(_tactical);
		// Soldier Diary_end.
	}
}

/**
 * dTor.
 */
DebriefingState::~DebriefingState()
{
	if (_game->isQuitting() == true)
		_gameSave->setBattleSave();

	for (std::vector<DebriefingStat*>::const_iterator
			i = _statList.begin();
			i != _statList.end();
			++i)
		delete *i;

	for (std::map<TileType, SpecialType*>::const_iterator
			i = _specialTypes.begin();
			i != _specialTypes.end();
			++i)
		delete i->second;
}

/**
 * Initializes the State.
 */
void DebriefingState::init()
{
	State::init();
	_game->getResourcePack()->playMusic(_music, "", 1);
}

/**
 * Post-tactical info- and notice-screens.
 * @note Pops state and changes music-track.
 * @param action - pointer to an Action
 */
void DebriefingState::btnOkClick(Action*)
{
	_game->getResourcePack()->fadeMusic(_game, 863);
	_game->popState();

	if (_isQuickBattle == true)
		_game->setState(new MainMenuState());
	else
	{
		if (_destroyPlayerBase == false) // deathly silence if Base is destroyed.
		{
			bool playAwardMusic (false);

			// NOTE: These push to player in reverse order.
			if (_cannotReequip.empty() == false)
				_game->pushState(new CannotReequipState(_cannotReequip));

			if (_soldiersLost.empty() == false)
			{
				playAwardMusic = true;
				_game->pushState(new CeremonyDeadState(_soldiersLost));
			}

			if (_soldiersFeted.empty() == false)
			{
				playAwardMusic = true;
				_game->pushState(new CeremonyState(_soldiersFeted));
			}

			std::vector<Soldier*> participants;
			for (std::vector<BattleUnit*>::const_iterator
					i = _unitList->begin();
					i != _unitList->end();
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

			if (_alienDies == true)
				_game->pushState(new NoContainmentState());
			else if (_manageContainment == true)
			{
				_game->pushState(new AlienContainmentState(_base, OPT_BATTLESCAPE));
				_game->pushState(new ErrorMessageState(
												tr("STR_CONTAINMENT_EXCEEDED").arg(_base->getName()),
												_palette,
												_rules->getInterface("debriefing")->getElement("errorMessage")->color,
												"BACK04.SCR",
												_rules->getInterface("debriefing")->getElement("errorPalette")->color));
			}

			_game->pushState(new DebriefExtraState(
												_base,
												_battleSave->getOperation(),
												_itemsGained,
												_itemsLostProperty,
												_soldierStatInc));

			if (_base->storesOverfull() == true)
			{
//				_game->pushState(new SellState(_base, OPT_BATTLESCAPE));
				_game->pushState(new ErrorMessageState(
												tr("STR_STORAGE_EXCEEDED").arg(_base->getName()),
												_palette,
												_rules->getInterface("debriefing")->getElement("errorMessage")->color,
												_game->getResourcePack()->getBackgroundRand(),
												_rules->getInterface("debriefing")->getElement("errorPalette")->color));
			}

			if (playAwardMusic == true)
				_game->getResourcePack()->playMusic(
												OpenXcom::res_MUSIC_TAC_AWARDS,
												"", 1);
			else
				_game->getResourcePack()->playMusic(OpenXcom::res_MUSIC_GEO_GLOBE);
		}


		if (_gameSave->isIronman() == true) // save after mission
			_game->pushState(new SaveGameState(
											OPT_GEOSCAPE,
											SAVE_IRONMAN,
											_palette));
		else if (Options::autosave == true) // NOTE: Auto-save points are fucked; they should be done *before* important events, not after.
			_game->pushState(new SaveGameState(
											OPT_GEOSCAPE,
											SAVE_AUTO_GEOSCAPE,
											_palette));
	}
	_gameSave->setBattleSave();	// delete SavedBattleGame.
}								// NOTE: BattlescapeState and BattlescapeGame are still VALID here. Okay ......
								// State will be deleted by the engine and the battle will be deleted by the state.
/**
 * Adds to the debriefing-stats.
 * @param type	- reference to the untranslated type-ID of the stat
 * @param score	- the score to add
 * @param qty	- the quantity to add (default 1)
 */
void DebriefingState::addStat( // private.
		const std::string& type,
		int score,
		int qty)
{
	for (std::vector<DebriefingStat*>::const_iterator
			i = _statList.begin();
			i != _statList.end();
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
 ** FUNCTOR ***
 * Clears any supply missions from an aLien-base.
 */
class ClearAlienBase
	:
		public std::unary_function<AlienMission*, void>
{
private:
	const AlienBase* _aBase;

	public:
		/// Caches a pointer to the aLien-base.
		explicit ClearAlienBase(const AlienBase* aBase)
			:
				_aBase(aBase)
		{}

		/// Clears the aLien-base if required.
		void operator() (AlienMission* const mission) const;
};

/**
 * Removes the link between the AlienMission and an AlienBase if one existed.
 * @param mission - pointer to the AlienMission
 */
void ClearAlienBase::operator() (AlienMission* const mission) const
{
	if (mission->getAlienBase() == _aBase)
		mission->setAlienBase();
}


/**
 * Prepares debriefing: Determines success or failure; gathers Aliens, Corpses,
 * Artefacts, UFO Components; handles units and re-equipping any crafts. If
 * aborted only the things on the exit-area are recovered.
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
		const TileType tileType (itRule->getTileType());
		switch (tileType)
		{
//			case TILE
//			case START_POINT:
			case UFO_POWER_SOURCE:		//  2
			case UFO_NAVIGATION:		//  3
			case UFO_CONSTRUCTION:		//  4
			case ALIEN_FOOD:			//  5
			case ALIEN_REPRODUCTION:	//  6
			case ALIEN_ENTERTAINMENT:	//  7
			case ALIEN_SURGERY:			//  8
			case EXAM_ROOM:				//  9
			case ALIEN_ALLOYS:			// 10
			case ALIEN_HABITAT:			// 11
			case DEAD_TILE:				// 12 -> give half-Alloy value for ruined alloy-tiles.
//			case END_POINT:
//			case MUST_DESTROY:
			{
				SpecialType* const specialType (new SpecialType());
				specialType->type = *i;
				specialType->value = itRule->getRecoveryScore();

				_specialTypes[tileType] = specialType;
			}
		}
	}

	std::string
		objectiveText,
		objectiveFailedText;
	int
		objectiveScore		 (0), // dang vc++ compiler warnings.
		objectiveFailedScore (0); // dang vc++ compiler warnings.

	const RuleAlienDeployment* const ruleDeploy (_rules->getDeployment(_battleSave->getTacticalType()));
	if (ruleDeploy != nullptr)
	{
		if (ruleDeploy->getObjectiveCompleteInfo(
											objectiveText,
											objectiveScore) == true)
		{
			_statList.push_back(new DebriefingStat(objectiveText));
		}

		if (ruleDeploy->getObjectiveFailedInfo(
											objectiveFailedText,
											objectiveFailedScore) == false)
		{
			_statList.push_back(new DebriefingStat(objectiveFailedText));
		}
	}

	_statList.push_back(new DebriefingStat("STR_ALIENS_KILLED"));
	_statList.push_back(new DebriefingStat("STR_ALIEN_CORPSES_RECOVERED"));
	_statList.push_back(new DebriefingStat("STR_LIVE_ALIENS_RECOVERED"));
	_statList.push_back(new DebriefingStat("STR_ALIEN_ARTIFACTS_RECOVERED"));
	_statList.push_back(new DebriefingStat("STR_ALIEN_BASE_CONTROL_DESTROYED"));
	_statList.push_back(new DebriefingStat("STR_CIVILIANS_KILLED_BY_ALIENS"));
	_statList.push_back(new DebriefingStat("STR_CIVILIANS_KILLED_BY_PLAYER"));
	_statList.push_back(new DebriefingStat("STR_CIVILIANS_SAVED"));
	_statList.push_back(new DebriefingStat("STR_XCOM_AGENTS_KILLED"));
//	_statList.push_back(new DebriefingStat("STR_XCOM_AGENTS_RETIRED_THROUGH_INJURY"));
	_statList.push_back(new DebriefingStat("STR_XCOM_AGENTS_MISSING"));
	_statList.push_back(new DebriefingStat("STR_TANKS_DESTROYED"));
	_statList.push_back(new DebriefingStat("STR_XCOM_CRAFT_LOST"));

	for (std::map<TileType, SpecialType*>::const_iterator
			i = _specialTypes.begin();
			i != _specialTypes.end();
			++i)
	{
		if (i->first != DEAD_TILE)
			_statList.push_back(new DebriefingStat((*i).second->type, true));
	}
	_statList.push_back(new DebriefingStat(_rules->getAlienFuelType(), true));
//	_statList.push_back(new DebriefingStat("STR_UFO_POWER_SOURCE", true)); // ->> SpecialTileTypes <<-|||
//	_statList.push_back(new DebriefingStat("STR_UFO_NAVIGATION", true));
//	_statList.push_back(new DebriefingStat("STR_UFO_CONSTRUCTION", true));
//	_statList.push_back(new DebriefingStat("STR_ALIEN_FOOD", true));
//	_statList.push_back(new DebriefingStat("STR_ALIEN_REPRODUCTION", true));
//	_statList.push_back(new DebriefingStat("STR_ALIEN_ENTERTAINMENT", true));
//	_statList.push_back(new DebriefingStat("STR_ALIEN_SURGERY", true));
//	_statList.push_back(new DebriefingStat("STR_EXAMINATION_ROOM", true));
//	_statList.push_back(new DebriefingStat("STR_ALIEN_ALLOYS", true));
//	_statList.push_back(new DebriefingStat("STR_ALIEN_HABITAT", true));


	// Resolve tiles for Latent and Latent_Start units. Aka post-2nd-stage
	Tile
		* const tileEquipt (_battleSave->getBattleInventory()),
		* const tileOrigin (_battleSave->getTile(Position(0,0,0)));
	for (std::vector<BattleUnit*>::const_iterator
			i = _unitList->begin();
			i != _unitList->end();
			++i)
	{
		switch ((*i)->getUnitStatus())
		{
			case STATUS_LATENT:
				(*i)->setUnitTile(tileOrigin);

				if ((*i)->getHealth() > (*i)->getStun())
					(*i)->setUnitStatus(STATUS_STANDING);
				else
					(*i)->setUnitStatus(STATUS_UNCONSCIOUS);
				break;

			case STATUS_LATENT_START:
				(*i)->setUnitTile(tileEquipt);

				if ((*i)->getHealth() > (*i)->getStun())
					(*i)->setUnitStatus(STATUS_STANDING);
				else
					(*i)->setUnitStatus(STATUS_UNCONSCIOUS);
		}
	}


	// Resolve tiles for other units that do not have a Tile.
	Position pos;
	const Position& posBogus (Position(-1,-1,-1));
	for (std::vector<BattleUnit*>::const_iterator
			i = _unitList->begin();
			i != _unitList->end();
			++i)
	{
		if ((*i)->getUnitTile() == nullptr)										// This unit is not on a tile ... give it one.
		{
			if ((pos = (*i)->getPosition()) == posBogus)						// in fact, this Unit is in limbo ... ie, is carried.
			{
				switch ((*i)->getUnitStatus())
				{
					case STATUS_DEAD:
					case STATUS_UNCONSCIOUS:
					{
						for (std::vector<BattleItem*>::const_iterator			// so look for its body or corpse ...
								j = _battleSave->getItems()->begin();
								j != _battleSave->getItems()->end();
								++j)
						{
							if ((*j)->getItemUnit() != nullptr					// found it: corpse is a dead or unconscious BattleUnit!!
								&& (*j)->getItemUnit() == *i)
							{
								if ((*j)->getOwner() != nullptr)				// corpse of BattleUnit has an Owner (ie. is being carried by another BattleUnit)
									pos = (*j)->getOwner()->getPosition();		// Put the corpse down .. slowly.
								else if ((*j)->getItemTile() != nullptr)		// corpse of BattleUnit is laying around somewhere
									pos = (*j)->getItemTile()->getPosition();	// you're not vaporized yet, Get up.

								break;
							}
						}
					}
				}
			}
			(*i)->setUnitTile(_battleSave->getTile(pos));	// why, exactly, do all units need a Tile:
		}													// because it drops still-standing-aLiens' inventories to the ground in preparation for the item-recovery phase.
	}														// And Unconscious/latent civies need to check if they're on an Exit-tile ...
															// And isOnTiletype() requires that unit have a tile set.

	for (std::vector<BattleUnit*>::const_iterator
			i = _unitList->begin();
			i != _unitList->end();
			++i)
	{
		switch ((*i)->getOriginalFaction())
		{
			case FACTION_PLAYER:
				switch ((*i)->getUnitStatus())
				{
					case STATUS_DEAD:
						++_playerDead;			// <- only to check for IronMan and LoneSurvivor awards.
						break;

					case STATUS_STANDING:
					case STATUS_UNCONSCIOUS:
						++_playerLive;			// <- only to check for IronMan and LoneSurvivor awards.
				}
				break;

			case FACTION_HOSTILE:
				if ((*i)->getUnitStatus() == STATUS_STANDING)
					_isHostileStanding = true;
		}
	}
	_tactical->success = _isHostileStanding == false // NOTE: Can still be a playerWipe and lose Craft/recovery (but not a Base).
					  || _battleSave->allObjectivesDestroyed() == true;


	const TacticalType tacType (_battleSave->getTacType());

	bool isPlayerWipe (true);
	for (std::vector<BattleUnit*>::const_iterator
			i = _unitList->begin();
			i != _unitList->end();
			++i)
	{
		if ((*i)->getOriginalFaction() == FACTION_PLAYER
			&& (*i)->getUnitStatus() == STATUS_STANDING
			&& (*i)->isMindControlled() == false
			&& (_aborted == false || (*i)->isOnTiletype(START_POINT) == true))	// NOTE: On a BaseDefense module with Start_Points defn'd this
		{																		// would also need to do a check for (_tactical->success==true).
			isPlayerWipe = false;
			break;
		}
	}


	double
		lon (0.), // avoid vc++ linker warnings.
		lat (0.); // avoid vc++ linker warnings.

	std::vector<Craft*>::const_iterator pCraft;

	for (std::vector<Base*>::const_iterator
			i = _gameSave->getBases()->begin();
			i != _gameSave->getBases()->end();
			++i)
	{
		if ((*i)->getTactical() == true) // in case this DON'T have a Craft, ie. BaseDefense
		{
			_base = *i;
			_txtBaseLabel->setText(_base->getName());

			lon = _base->getLongitude();
			lat = _base->getLatitude();

			if (_isHostileStanding == false)
			{
				_base->setTactical(false);

				bool facDestroyed (false);
				for (std::vector<BaseFacility*>::const_iterator
						j = _base->getFacilities()->begin();
						j != _base->getFacilities()->end();
						)
				{
					if (_battleSave->baseDestruct()[(*j)->getX()]
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
				_destroyPlayerBase = true;

			break;
		}

		for (std::vector<Craft*>::const_iterator
				j = (*i)->getCrafts()->begin();
				j != (*i)->getCrafts()->end();
				++j)
		{
			if ((*j)->getTactical() == true) // has Craft, ergo NOT BaseDefense
			{
				if (_isQuickBattle == false)
					_missionCost = (*i)->craftExpense(*j);

				lon = (*j)->getLongitude();
				lat = (*j)->getLatitude();

				_base = *i;
				_craft = *j;

				if (isPlayerWipe == false)
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
				if (_tactical->success == true)
				{
					const Ufo* const ufo (dynamic_cast<Ufo*>((*j)->getDestination()));
					if (ufo != nullptr && ufo->getTactical() == true)
						(*j)->returnToBase();
				}

				const TerrorSite* const site (dynamic_cast<TerrorSite*>((*j)->getDestination()));
				if (site != nullptr && site->getTactical() == true)
					(*j)->returnToBase();
			}
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
			_tactical->region = _region->getRules()->getType();
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
			_tactical->country = _country->getRules()->getType();
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
			_tactical->ufo = (*i)->getRules()->getType();

			if (_tactical->success == true)
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
		for (std::vector<TerrorSite*>::const_iterator // Second - search for TerrorSite.
				i = _gameSave->getTerrorSites()->begin();
				i != _gameSave->getTerrorSites()->end();
				++i)
		{
			if ((*i)->getTactical() == true)
			{
				found = true;
				delete *i;
				_gameSave->getTerrorSites()->erase(i);
				break;
			}
		}
	}

	if (found == false)
	{
		for (std::vector<AlienBase*>::const_iterator // Third - search for aLienBase.
				i = _gameSave->getAlienBases()->begin();
				i != _gameSave->getAlienBases()->end();
				++i)
		{
			if ((*i)->getTactical() == true)
			{
				_txtRecovery->setText(tr("STR_ALIEN_BASE_RECOVERY"));

				if (_tactical->success == true)
				{
					if (objectiveText.empty() == false)
					{
						objectiveScore = std::max((objectiveScore + 9) / 10,
												   objectiveScore - (_diff * 50));
						addStat(
							objectiveText,
							objectiveScore);
					}

					std::for_each(
							_gameSave->getAlienMissions().begin(),
							_gameSave->getAlienMissions().end(),
							ClearAlienBase(*i));

					for (std::vector<Target*>::const_iterator
							j = (*i)->getTargeters()->begin();
							j != (*i)->getTargeters()->end();
							)
					{
						Craft* const craft (dynamic_cast<Craft*>(*j));
						if (craft != nullptr)
						{
							craft->returnToBase();
							j = (*i)->getTargeters()->begin();
						}
						else
							++j;
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


	Soldier* sol;
	for (std::vector<BattleUnit*>::const_iterator // resolve all BattleUnits ->
			i = _unitList->begin();
			i != _unitList->end();
			++i)
	{
		switch ((*i)->getOriginalFaction())
		{
			case FACTION_PLAYER:
				switch ((*i)->getUnitStatus())
				{
					case STATUS_DEAD:
						//Log(LOG_INFO) << ". unitDead id-" << (*i)->getId() << " " << (*i)->getType();
						if ((sol = (*i)->getGeoscapeSoldier()) != nullptr)
						{
							(*i)->getStatistics()->KIA = true;
							_soldierStatInc[sol->getName()] = (*i)->postMissionProcedures(true);

							if (_isQuickBattle == false)
								_missionCost += _base->soldierExpense(sol, true);

							addStat(
								"STR_XCOM_AGENTS_KILLED",
								-(*i)->getValue());

							for (std::vector<Soldier*>::const_iterator
									j = _base->getSoldiers()->begin();
									j != _base->getSoldiers()->end();
									++j)
							{
								if (*j == sol) // NOTE: Could return any armor the Soldier was wearing to Stores. CHEATER!!!!!
								{
									(*j)->die(_gameSave);
									delete *j;
									_base->getSoldiers()->erase(j);
									break;
								}
							}
						}
						else // support unit.
						{
							if (_isQuickBattle == false)
								_missionCost += _base->supportExpense(
																(*i)->getArmor()->getSize() * (*i)->getArmor()->getSize(),
																true);
							addStat(
								"STR_TANKS_DESTROYED",
								-(*i)->getValue());

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

					case STATUS_STANDING:
					case STATUS_UNCONSCIOUS:
						//Log(LOG_INFO) << ". unitLive id-" << (*i)->getId() << " " << (*i)->getType();
						if (isPlayerWipe == false)
						{
							recoverItems((*i)->getInventory());

							if ((sol = (*i)->getGeoscapeSoldier()) != nullptr)
							{
								_soldierStatInc[sol->getName()] = (*i)->postMissionProcedures();

								if (_isQuickBattle == false)
									_missionCost += _base->soldierExpense(sol);

//								sol->calcStatString(
//												_rules->getStatStrings(),
//												Options::psiStrengthEval
//													&& _gameSave->isResearched(_rules->getPsiRequirements()));
							}
							else // support unit.
							{
								if (_isQuickBattle == false)
								{
									const int unitSize ((*i)->getArmor()->getSize());
									_missionCost += _base->supportExpense(unitSize * unitSize);
								}

								_base->getStorageItems()->addItem((*i)->getType());	// return the support-unit to base-stores.

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
										_base->getStorageItems()->addItem(			// return any load from the support-unit's fixed-weapon to base-stores.
																		itRule->getType(),
																		qtyLoad);
										if (qtyLoad < clip)
											_itemsLostProperty[itRule] += clip - qtyLoad;
									}
								}
							}
						}
						else // playerWipe.
						{
							//Log(LOG_INFO) << ". . playerWipe set MIA / Status_Dead";
							(*i)->setUnitStatus(STATUS_DEAD);

							if ((sol = (*i)->getGeoscapeSoldier()) != nullptr)
							{
								(*i)->getStatistics()->MIA = true;
								_soldierStatInc[sol->getName()] = (*i)->postMissionProcedures(true);

								addStat(
									"STR_XCOM_AGENTS_MISSING",
									-(*i)->getValue());

								for (std::vector<Soldier*>::const_iterator
										j = _base->getSoldiers()->begin();
										j != _base->getSoldiers()->end();
										++j)
								{
									if (*j == sol) // NOTE: Could return any armor the Soldier was wearing to Stores. CHEATER!!!!!
									{
										(*j)->die(_gameSave);
										delete *j;
										_base->getSoldiers()->erase(j);
										break;
									}
								}
							}
							else // support unit.
							{
								addStat(
									"STR_TANKS_DESTROYED",
									-(*i)->getValue());

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
				}
				break;

			case FACTION_HOSTILE:
				switch ((*i)->getUnitStatus())
				{
					case STATUS_DEAD:
						//Log(LOG_INFO) << ". unitDead id-" << (*i)->getId() << " " << (*i)->getType();
						if ((*i)->killerFaction() == FACTION_PLAYER)
						{
							//Log(LOG_INFO) << ". . killed by xCom";
							addStat(
								"STR_ALIENS_KILLED",
								(*i)->getValue());
						}
						break;

					case STATUS_UNCONSCIOUS:
						//Log(LOG_INFO) << ". unitLive id-" << (*i)->getId() << " " << (*i)->getType();
						++_aliensStunned; // for Nike Cross determination.
				}
				break;

			case FACTION_NEUTRAL:
				switch ((*i)->getUnitStatus())
				{
					case STATUS_DEAD:
						//Log(LOG_INFO) << ". unitDead id-" << (*i)->getId() << " " << (*i)->getType();
						switch ((*i)->killerFaction())
						{
							case FACTION_PLAYER:
								//Log(LOG_INFO) << ". . killed by xCom";
								addStat(
									"STR_CIVILIANS_KILLED_BY_PLAYER",
									-((*i)->getValue() << 1u));
								break;

							case FACTION_HOSTILE:
							case FACTION_NEUTRAL:
								addStat(
									"STR_CIVILIANS_KILLED_BY_ALIENS",
									-(*i)->getValue());
						}
						break;

					case STATUS_STANDING:
						for (std::vector<BattleItem*>::const_iterator // in case Civie was MC'd and given stuff.
								j = (*i)->getInventory()->begin();
								j != (*i)->getInventory()->end();
								++j)
						{
							if ((*j)->getRules()->isFixed() == false)
							{
								(*j)->setInventorySection(_rules->getInventoryRule(ST_GROUND));
								(*i)->getUnitTile()->addItem(*j);
							}
						}
						// no break;
					case STATUS_UNCONSCIOUS:
						//Log(LOG_INFO) << ". unitLive id-" << (*i)->getId() << " " << (*i)->getType();
						if (_isHostileStanding == false
							|| (_aborted == true && (*i)->isOnTiletype(START_POINT) == true)) // NOTE: Even if isPlayerWipe.
						{
							addStat(
								"STR_CIVILIANS_SAVED",
								(*i)->getValue());
						}
						else
							addStat(
								"STR_CIVILIANS_KILLED_BY_ALIENS",
								-(*i)->getValue());
				}
		}
	} //End loop BattleUnits.


	if (_craft != nullptr && isPlayerWipe == true && _aborted == false)
	{
		addStat(
			"STR_XCOM_CRAFT_LOST",
			-(_craft->getRules()->getScore()));

		delete _craft;
		_craft = nullptr;
		_base->getCrafts()->erase(pCraft);
	}

	std::string tacResult;
	if (_tactical->success == true)
	{
		switch (tacType)
		{
			case TCT_DEFAULT: //-1
				tacResult = "Warning: no TacType";
				break;

			case TCT_BASEDEFENSE:
				tacResult = "STR_BASE_IS_SAVED";
				break;

			case TCT_TERRORSITE:
				tacResult = "STR_ALIENS_DEFEATED";
				break;

			case TCT_BASEASSAULT:
//			case TCT_MARS1: // <- there is never any debriefing for this. Because (lose= loseGame) and (win= 2nd stage)
//			case TCT_MARS2: // <- there is never any debriefing for this.
				tacResult = "STR_ALIEN_BASE_DESTROYED";
				break;

			default:
			case TCT_UFOCRASHED:
			case TCT_UFOLANDED:
				tacResult = "STR_UFO_IS_RECOVERED";	// ... But not if player completed objectives
		}											// and aborted while leaving a hostile standing.
		_txtTitle->setText(tr(tacResult));			// Fortunately crashed and landed UFOs do not have objectives.

		if (objectiveText.empty() == false)
			addStat(
				objectiveText,
				objectiveScore);
	}
	else // hostiles Standing AND objective NOT completed
	{
		switch (tacType)
		{
			case TCT_DEFAULT: //-1
				tacResult = "Warning: no TacType";
				break;

			case TCT_BASEDEFENSE:
				tacResult = "STR_BASE_IS_LOST";

				for (std::vector<Craft*>::const_iterator
						i = _base->getCrafts()->begin();
						i != _base->getCrafts()->end();
						++i)
				{
					addStat(
						"STR_XCOM_CRAFT_LOST",
						-(*i)->getRules()->getScore());
				}
				break;

			case TCT_TERRORSITE:
				tacResult = "STR_TERROR_CONTINUES";
				break;

			case TCT_BASEASSAULT:
//			case TCT_MARS1: // Note that these Mars tacticals are really Lose GAME.
//			case TCT_MARS2: // And there is still never a debriefing for this <-
				tacResult = "STR_ALIEN_BASE_STILL_INTACT";
				break;

			default:
			case TCT_UFOCRASHED:
			case TCT_UFOLANDED:
				if (_aborted == true)
					tacResult = "STR_UFO_IS_NOT_RECOVERED";
				else
					tacResult = "STR_CRAFT_IS_LOST";
		}
		_txtTitle->setText(tr(tacResult));

		if (objectiveFailedText.empty() == false)
			addStat(
				objectiveFailedText,
				objectiveFailedScore);
	}

	if (_isHostileStanding == false || _aborted == true)
	{
		recoverItems(_battleSave->recoverGuaranteed());

		if (_destroyPlayerBase == false)
		{
			Tile* tile;
			for (size_t
					i = 0u;
					i != _battleSave->getMapSizeXYZ();
					++i)
			{
				tile = _battleSave->getTiles()[i];
				if (tile->getMapData(O_FLOOR) != nullptr
					&& tile->getMapData(O_FLOOR)->getTileType() == START_POINT)
				{
					recoverItems(_battleSave->getTiles()[i]->getInventory());
				}
			}
		}
	}

	if (_isHostileStanding == false)
	{
		recoverItems(_battleSave->recoverConditional());

		const int parts (static_cast<int>(Tile::PARTS_TILE));
		MapDataType partType;
		int qtyRuinedAlloys (0);

		for (size_t
				i = 0u;
				i != _battleSave->getMapSizeXYZ();
				++i)
		{
			recoverItems(_battleSave->getTiles()[i]->getInventory());

			for (int
					j = 0;
					j != parts;
					++j)
			{
				partType = static_cast<MapDataType>(j);
				if (_battleSave->getTiles()[i]->getMapData(partType) != nullptr)
				{
					const TileType tileType (_battleSave->getTiles()[i]->getMapData(partType)->getTileType());

					switch (tileType)
					{
						case DEAD_TILE:
							++qtyRuinedAlloys;
							break;

						default: // TODO: Expand these using legit values above^
							if (_specialTypes.find(tileType) != _specialTypes.end())
								addStat(
									_specialTypes[tileType]->type,
									_specialTypes[tileType]->value);
					}
				}
			}
		}

		for (std::vector<DebriefingStat*>::const_iterator
				i = _statList.begin();
				i != _statList.end();
				++i)
		{
			if ((*i)->type == _specialTypes[ALIEN_ALLOYS]->type)
			{
				int alloyDivisor; // TODO: Subtract diff*10 for gameDifficulty.
				switch (tacType)
				{
					case TCT_BASEASSAULT:	alloyDivisor = 150; break;
					default:				alloyDivisor = 15;
				}

				(*i)->qty = ((*i)->qty + (qtyRuinedAlloys >> 1u)) / alloyDivisor;
				(*i)->score = ((*i)->score + ((qtyRuinedAlloys * _specialTypes[DEAD_TILE]->value) >> 1u)) / alloyDivisor;

				if ((*i)->qty != 0 && (*i)->recover == true)
					_itemsGained[_rules->getItemRule((*i)->type)] = (*i)->qty; // NOTE: Elerium is handled in recoverItems().
			}

			if ((*i)->qty != 0 && (*i)->recover == true)
				_base->getStorageItems()->addItem((*i)->type, (*i)->qty);
		}
	}


	for (std::vector<BattleItem*>::const_iterator
			i = _battleSave->deletedProperty().begin();
			i != _battleSave->deletedProperty().end();
			++i)
	{
		if (_itemsGained.find(itRule = (*i)->getRules()) == _itemsGained.end())
			++_itemsLostProperty[itRule];
		else if (--_itemsGained[itRule] == 0)	// NOTE: '_itemsGained' shall never contain clips - vid. recoverItems()
			_itemsGained.erase(itRule);			// ... clips handled immediately below_
	}											// TODO: Extensive testing on item-gains/losses ....

	int
		qtyFullClip,
		clipsTotal;
	for (std::map<const RuleItem*, int>::const_iterator
			i = _clips.begin();	// '_clips' is a tally of both xcomProperty + found clips
			i != _clips.end();	// so '_clipsProperty' needs to be subtracted to find clipsGained.
			++i)
	{
		if ((qtyFullClip = i->first->getFullClip()) != 0) // safety.
		{
			clipsTotal = i->second / qtyFullClip;
			switch (clipsTotal)
			{
				case 0:					// all clips-of-type are lost, including those brought on the mission
					if (i->second != 0)	// and if there's a partial clip, that needs to be added to the lost-vector too.
						++_itemsLostProperty[i->first];
					break;

				default:				// clips were found whether xcomProperty or not. Add them to Base-stores!
				{
					_itemsLostProperty.erase(i->first);

					int roundsProperty;
					std::map<const RuleItem*, int>::const_iterator pClipsProperty (_clipsProperty.find(i->first));
					if (pClipsProperty != _clipsProperty.end())
						roundsProperty = pClipsProperty->second;
					else
						roundsProperty = 0;

					int clipsGained ((i->second - roundsProperty) / qtyFullClip);
					if (clipsGained != 0)
						_itemsGained[i->first] = clipsGained;		// these clips are over & above those brought as xcomProperty.

					_base->getStorageItems()->addItem(
													i->first->getType(),
													clipsTotal);	// these clips include both xcomProperty and found clips.
				}
			}
		}
	}

	switch (tacType)
	{
		case TCT_BASEDEFENSE:
			if (_destroyPlayerBase == false || _isQuickBattle == true)
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
			else
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

			if (_region != nullptr && _isQuickBattle == false)
			{
				const AlienMission* const retalMission (_gameSave->findAlienMission(
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

//		case TCT_DEFAULT:			// -1
		case TCT_UFOCRASHED:
		case TCT_UFOLANDED:
		case TCT_BASEASSAULT:
		case TCT_TERRORSITE:
		case TCT_MARS1:				// for QuickBattle i guess (if not, Debriefing will not run)
		case TCT_MARS2:				// for QuickBattle i guess (if not, Debriefing will not run)
			reequipCraft(_craft);	// not a BaseDefense.
	}
}

/**
 * Reequips a Craft after tactical.
 * TODO: Figure out what's going on w/ QuickBattles and base-storage.
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
		if ((baseQty = _base->getStorageItems()->getItemQuantity(i->first)) < i->second)
		{
			_base->getStorageItems()->removeItem(i->first, baseQty);

			qtyLost = i->second - baseQty;
			craftContainer->removeItem(i->first, qtyLost);

			const ReequipStat stat
			{
				i->first,
				qtyLost,
				craft->getName(_game->getLanguage())
			};
			_cannotReequip.push_back(stat);
		}
		else
			_base->getStorageItems()->removeItem(i->first, i->second);
	}

	// First account for all craft-vehicles and delete each. Then re-add as many
	// as possible while redistributing all available ammunition. Note that the
	// Vehicles and their ammunition have already been sent to Base-storage.
	if (craft->getRules()->getVehicleCapacity() != 0)
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
			qtySupport,
			quadrants;
		for (std::map<std::string, int>::const_iterator
				i = craftVehicles.getContents()->begin();
				i != craftVehicles.getContents()->end();
				++i)
		{
			if ((baseQty = _base->getStorageItems()->getItemQuantity(i->first)) < i->second)
			{
				const ReequipStat stat
				{
					i->first,
					i->second - baseQty,
					craft->getName(_game->getLanguage())
				};
				_cannotReequip.push_back(stat);
			}

			quadrants = _rules->getArmor(_rules->getUnitRule(i->first)->getArmorType())->getSize();
			quadrants *= quadrants;

			qtySupport = std::min(baseQty, i->second);

			const RuleItem* const itRule (_rules->getItemRule(i->first));

			if (itRule->getFullClip() < 1)
			{
				for (int
						j = 0;
						j != qtySupport;
						++j)
				{
					craft->getVehicles()->push_back(new Vehicle(
															itRule,
															itRule->getFullClip(),
															quadrants));
				}
				_base->getStorageItems()->removeItem(i->first, qtySupport);
			}
			else
			{
				const std::string type (itRule->getCompatibleAmmo()->front());
				const int
					clipsRequired (itRule->getFullClip()),
					baseClips (_base->getStorageItems()->getItemQuantity(type));

				if ((qtyLost = (clipsRequired * i->second) - baseClips) > 0)
				{
					const ReequipStat stat
					{
						type,
						qtyLost,
						craft->getName(_game->getLanguage())
					};
					_cannotReequip.push_back(stat);
				}

				qtySupport = std::min(qtySupport,
									  baseClips / clipsRequired);
				if (qtySupport != 0)
				{
					for (int
							j = 0;
							j != qtySupport;
							++j)
					{
						craft->getVehicles()->push_back(new Vehicle(
																itRule,
																clipsRequired,
																quadrants));
					}
					_base->getStorageItems()->removeItem(i->first, qtySupport);
					_base->getStorageItems()->removeItem(type, clipsRequired * qtySupport);
				}
			}
		}
	}
}

/**
 * Recovers items from tactical.
 * TODO: Figure out what's going on w/ QuickBattles and base-storage.
 * @note Transfers the contents of a battlefield-inventory to the Base's stores.
 * This bypasses fixed-weapons and non-recoverable items.
 * @param battleItems - pointer to a vector of pointers to BattleItems on the battlefield
 */
void DebriefingState::recoverItems(std::vector<BattleItem*>* const battleItems) // private.
{
	const RuleItem* itRule;
	const BattleUnit* unit;
	BattleType bType;
	std::string type;

	for (std::vector<BattleItem*>::const_iterator
			i = battleItems->begin();
			i != battleItems->end();
			++i)
	{
		itRule = (*i)->getRules();

		if (itRule->isFixed() == false && itRule->isRecoverable() == true)
		{
			bType = itRule->getBattleType();

			switch (bType)
			{
				case BT_FUEL:
					_itemsGained[itRule] += _rules->getAlienFuelQuantity();
					addStat(
						_rules->getAlienFuelType(),
						itRule->getRecoveryScore(),
						_rules->getAlienFuelQuantity());
					break;

				default:
					type = itRule->getType();

					if (bType != BT_CORPSE && _gameSave->isResearched(type) == false)
						addStat(
							"STR_ALIEN_ARTIFACTS_RECOVERED",
							itRule->getRecoveryScore());

					switch (bType) // shuttle all times instantly to the Base
					{
						case BT_CORPSE:
						{
							if ((unit = (*i)->getItemUnit()) != nullptr)
							{
								switch (unit->getUnitStatus())
								{
									case STATUS_DEAD:
									{
										addStat(
											"STR_ALIEN_CORPSES_RECOVERED",
											unit->getValue() / 3);	// TODO: This should rather be the 'recoveryPoints' of the corpse-item;
																	// but at present all the corpse-rules spec. default values of 3 or 5 pts.
										std::string corpse (unit->getArmor()->getCorpseGeoscape());
										if (corpse.empty() == false) // safety.
										{
											_base->getStorageItems()->addItem(corpse);
											++_itemsGained[_rules->getItemRule(corpse)];
										}
										break;
									}

									case STATUS_UNCONSCIOUS:
										if (unit->getOriginalFaction() == FACTION_HOSTILE)
											recoverLiveAlien(unit);
								}
							}
							break;
						}

						case BT_AMMO:
							_clips[itRule] += (*i)->getAmmoQuantity();
							if ((*i)->getProperty() == true)
								_clipsProperty[itRule] += (*i)->getAmmoQuantity();
							break;

						case BT_FIREARM:
							if ((*i)->selfPowered() == false)
							{
								const BattleItem* const clip ((*i)->getAmmoItem());
								if (clip != nullptr) //&& clip->getRules()->getFullClip() != 0) // <- nobody be stupid and make a clip with 0 ammo-capacity.
								{
									_clips[clip->getRules()] += clip->getAmmoQuantity();
									if ((*i)->getProperty() == true)
										_clipsProperty[clip->getRules()] += clip->getAmmoQuantity();
								}
							}
							// no break;

						default:
							_base->getStorageItems()->addItem(type);
							if ((*i)->getProperty() == false)
								++_itemsGained[itRule];
					}
			}
		}
	}
}

/**
 * Recovers a live aLien from the battlefield.
 * TODO: Figure out what's going on w/ QuickBattles and base-storage.
 * @param unit - pointer to a BattleUnit to recover
 */
void DebriefingState::recoverLiveAlien(const BattleUnit* const unit) // private.
{
	if (_base->hasContainment() == true || _isQuickBattle == true)
	{
		//Log(LOG_INFO) << ". . . alienLive id-" << unit->getId() << " " << unit->getType();
		const std::string type (unit->getType());

		int value;
		if (_rules->getResearch(type) != nullptr
			&& _gameSave->isResearched(type) == false)
		{
			value = unit->getValue() << 1u;
		}
		else
			value = unit->getValue();

		addStat(
			"STR_LIVE_ALIENS_RECOVERED",
			value);

		_base->getStorageItems()->addItem(type);
		_manageContainment = _base->getFreeContainment() < 0;

		++_itemsGained[_rules->getItemRule(type)];
	}
	else
	{
		//Log(LOG_INFO) << ". . . alienDead id-" << unit->getId() << " " << unit->getType();
		_alienDies = true;
		addStat(
			"STR_ALIEN_CORPSES_RECOVERED",
			unit->getValue() / 3);

		std::string corpse (unit->getArmor()->getCorpseGeoscape());
		if (corpse.empty() == false) // safety. (Or error-out if there isn't one.)
		{
			_base->getStorageItems()->addItem(corpse);
			++_itemsGained[_rules->getItemRule(corpse)];
		}
	}
}

}
