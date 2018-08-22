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

#include "DebriefingState.h"

//#include <algorithm>
//#include <sstream>

#include "AlienDiesState.h"
#include "CannotReequipState.h"
#include "CeremonyDeadState.h"
#include "CeremonyState.h"
#include "DebriefExtraState.h"
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

const char* const DebriefingState::TAC_RESULT[12u] // static.
{
	"STR_ALIENS_KILLED",				//  0
	"STR_LIVE_ALIENS_RECOVERED",		//  1
	"STR_ALIEN_CORPSES_RECOVERED",		//  2
	"STR_ALIEN_ARTIFACTS_RECOVERED",	//  3
	"STR_ALIEN_BASE_CONTROL_DESTROYED",	//  4
	"STR_CIVILIANS_KILLED_BY_ALIENS",	//  5
	"STR_CIVILIANS_KILLED_BY_PLAYER",	//  6
	"STR_CIVILIANS_SAVED",				//  7
	"STR_XCOM_AGENTS_KILLED",			//  8
	"STR_XCOM_AGENTS_MISSING",			//  9
	"STR_TANKS_DESTROYED",				// 10
	"STR_XCOM_CRAFT_LOST"				// 11
//	"STR_XCOM_AGENTS_RETIRED_THROUGH_INJURY"
};


/**
 * Initializes all the elements in the Debriefing screen.
 */
DebriefingState::DebriefingState()
	:
		_rules(_game->getRuleset()),
		_playSave(_game->getSavedGame()),
		_battleSave(_game->getSavedGame()->getBattleSave()),
		_unitList(_game->getSavedGame()->getBattleSave()->getUnits()),
		_aborted(_game->getSavedGame()->getBattleSave()->isAborted()),
		_diff(_game->getSavedGame()->getDifficultyInt()),
		_isQuickBattle(_game->getSavedGame()->getMonthsElapsed() == -1),
		_region(nullptr),
		_country(nullptr),
		_base(nullptr),
		_craft(nullptr),
		_capture(ARR_NONE),
		_destroyPlayerBase(-1),
		_missionCost(0),
		_aliensStunned(0),
		_playerDead(0),
		_playerLive(0),
		_isHostileStanding(false)
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
	_battleSave->getBattleGame()->clearBattleStates();

	_window        = new Window(this);

	_txtTitle      = new Text(280, 16,  16, 8);
	_txtBaseLabel  = new Text( 80,  9, 216, 8);

	_txtCasualties = new Text(184, 9,  16, 24);
	_txtQuantity   = new Text( 60, 9, 200, 24);
	_txtScore      = new Text( 36, 9, 260, 24);
	_lstStats      = new TextList(288, 81, 16, 32);

	_txtRecovery   = new Text(180, 9, 16, 0);		// set y below_
	_lstRecovery   = new TextList(288, 81, 16, 0);	// set y below_

	_lstTotal      = new TextList(288, 9, 16, 0);	// set y below_

	_txtCost       = new Text(76, 9, 16, 181);
	_txtRating     = new Text(76, 9, 228, 181);
	_btnOk         = new TextButton(136, 16, 92, 177);

	setInterface("debriefing");

	add(_window,        "window",  "debriefing");

	add(_txtTitle,      "heading", "debriefing");
	add(_txtBaseLabel,  "text",    "debriefing");

	add(_txtCasualties, "text",    "debriefing");
	add(_txtQuantity,   "text",    "debriefing");
	add(_txtScore,      "text",    "debriefing");
	add(_lstStats,      "list",    "debriefing");

	add(_txtRecovery,   "text",    "debriefing");
	add(_lstRecovery,   "list",    "debriefing");

	add(_lstTotal,      "list",    "debriefing");

	add(_txtCost,       "text",    "debriefing");
	add(_txtRating,     "text",    "debriefing");
	add(_btnOk,         "button",  "debriefing");

	centerSurfaces();


	_window->setBackground(_game->getResourcePack()->getSurface("BACK01.SCR"));

	std::wstring wst (_battleSave->getOperation());
	if (wst.empty() == true) wst = tr("STR_OK");
	_btnOk->setText(wst);
	_btnOk->onMouseClick(   static_cast<ActionHandler>(&DebriefingState::btnOkClick));
	_btnOk->onKeyboardPress(static_cast<ActionHandler>(&DebriefingState::btnOkClick),
							Options::keyOk);
	_btnOk->onKeyboardPress(static_cast<ActionHandler>(&DebriefingState::btnOkClick),
							Options::keyOkKeypad);
	_btnOk->onKeyboardPress(static_cast<ActionHandler>(&DebriefingState::btnOkClick),
							Options::keyCancel);

	_txtTitle->setBig();

	_txtCasualties->setText(tr("STR_CASUALTIES"));
	_txtQuantity->setText(tr("STR_QUANTITY"));
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


	_tactical = new TacticalStatistics();
	prepareDebriefing(); // <- |-- GATHER ALL DATA HERE <- < ||



	int
		offsetY_stats	(0),
		offsetY_recover	(0),
		aliensKilled	(0),
		civiliansSaved	(0),
		civiliansDead	(0);

	for (std::vector<DebriefStat*>::const_iterator
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
				offsetY_recover += 8;
			}
			else
			{
				_lstStats->addRow(
								3,
								tr((*i)->type).c_str(),
								woststr1.str().c_str(),
								woststr2.str().c_str());
				offsetY_stats += 8;
			}

			if		((*i)->type == TAC_RESULT[0u]) aliensKilled   = (*i)->qty;	// aLiens killed
			else if	((*i)->type == TAC_RESULT[7u]) civiliansSaved = (*i)->qty;	// civilians saved
			else if	((*i)->type == TAC_RESULT[5u]								// civilians killed by aLiens
				||   (*i)->type == TAC_RESULT[6u]) civiliansDead += (*i)->qty;	// civilians killed by player
		}
	}

	_lstTotal->addRow(
					2,
					tr("STR_TOTAL_UC").c_str(),
					Text::intWide(_tactical->score).c_str());

	switch (offsetY_recover)
	{
		case 0:
			_txtRecovery->setVisible(false);
			_lstRecovery->setVisible(false);
			_lstTotal->setY(_lstStats->getY() + offsetY_stats + 5);
			break;
		default:
			_txtRecovery->setY(_lstStats->getY() + offsetY_stats + 5);
			_lstRecovery->setY(_txtRecovery->getY() + 8);
			_lstTotal->setY(_lstRecovery->getY() + offsetY_recover + 5);
	}


	switch (_destroyPlayerBase)	// TODO: Amalgamate "Base is Lost" w/ _tactical->score
	{							// and display "Base is Lost" as a result in '_statsList'.
		case -1: // base okay
			if (_region != nullptr)
				_region->addActivityXCom(_tactical->score);	// NOTE: Could use SavedGame::scorePoints() for these but since
															// Region and Country are already defined here let it go through as is.
			if (_country != nullptr)
				_country->addActivityXCom(_tactical->score);

			break;

		case 0: // base is lost - no point delta
			break;

		default: // player Base was destroyed.
			if (_region != nullptr)
				_region->addActivityAlien(_destroyPlayerBase);

			if (_country != nullptr)
				_country->addActivityAlien(_destroyPlayerBase);
	}

	if (_tactical->score < -99)
	{
		_music = OpenXcom::res_MUSIC_TAC_DEBRIEFING_BAD;
		_tactical->rating = TAC_RATING[0u]; // terrible
	}
	else
	{
		_music = OpenXcom::res_MUSIC_TAC_DEBRIEFING;
		if      (_tactical->score <  101) _tactical->rating = TAC_RATING[1u]; // poor
		else if (_tactical->score <  351) _tactical->rating = TAC_RATING[2u]; // okay
		else if (_tactical->score <  751) _tactical->rating = TAC_RATING[3u]; // good
		else if (_tactical->score < 1251) _tactical->rating = TAC_RATING[4u]; // excellent
		else                              _tactical->rating = TAC_RATING[5u]; // terrific
	}

	switch (_missionCost)
	{
		case 0:
			_txtCost->setVisible(false);
			break;
		default:
			_txtCost->setText(Text::formatCurrency(_missionCost));
			_txtCost->setAlign(ALIGN_CENTER);
	}

//	_txtRating->setText(tr("STR_RATING_").arg(tr(_tactical->rating)));
	_txtRating->setText(tr(_tactical->rating));
	_txtRating->setAlign(ALIGN_CENTER);


	// Soldier Diary ->
	if (_isQuickBattle == false) // TODO: Show some stats for quick-battles.
	{
		_tactical->id        = static_cast<int>(_playSave->getTacticalStatistics().size());
		_tactical->timeStat  = *_playSave->getTime();
		_tactical->type      = _battleSave->getTacticalType();
		_tactical->shade     = _battleSave->getTacticalShade();
		_tactical->alienRace = _battleSave->getAlienRace();

		if (civiliansSaved != 0 && civiliansDead == 0)
			_tactical->valiantCrux = true;

		const bool checkIron (_playerDead == 0
						   && _aborted == false
						   && _aliensStunned + aliensKilled > 1 + _diff);

		//Log(LOG_INFO) << "DebriefingState::cTor";
		Soldier* sol;
		SoldierDead* solDead;
		std::vector<TacticalStatistics*>& tacticals (_playSave->getTacticalStatistics());
		BattleUnitStatistics* tacstats;

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
				tacstats = (*i)->getStatistics();

				if (_playerLive == 1)
				{
					switch ((*i)->getUnitStatus())
					{
						case STATUS_STANDING:
							if (checkIron == true)
							{
								tacstats->ironMan = true;
								break;
							}
							// no break;
						case STATUS_UNCONSCIOUS:
							if (_playerDead != 0
								&& tacstats->hasFriendlyFired() == false) // Not sure I want this ....
							{
								tacstats->loneSurvivor = true;
							}
					}
				}

				if (_isHostileStanding == false)
				{
					int take (0);
					for (std::vector<BattleUnitKill*>::const_iterator
							j  = tacstats->kills.begin();
							j != tacstats->kills.end();
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
						tacstats->nikeCross = true;
					}
				}


				switch ((*i)->getUnitStatus())
				{
					case STATUS_STANDING:
					case STATUS_UNCONSCIOUS:
						//Log(LOG_INFO) << ". . . alive";
						if ((tacstats->daysWounded = sol->getSickbay()) != 0)
							_tactical->injuryList[sol->getId()] = tacstats->daysWounded;

						sol->getDiary()->postTactical(tacstats, _tactical);
						if (sol->getDiary()->updateAwards(_rules, tacticals) == true)
							_soldiersFeted.push_back(sol);
						break;

					case STATUS_DEAD:
						//Log(LOG_INFO) << ". . . dead";
						sol = nullptr;	// Zero out the BattleUnit from the geoscape-soldiers list
										// in this State; it's already gone from his/her former Base.
										// This makes them ineligible for promotion.
										// PS, there is no "geoscape-soldiers list" really; it's
										// just a pointer stored on each xCom-agent/BattleUnit ....

						solDead = nullptr; // avoid vc++ linker warning.
						for (std::vector<SoldierDead*>::const_iterator
								j  = _playSave->getDeadSoldiers()->begin();
								j != _playSave->getDeadSoldiers()->end();
								++j)
						{
							if ((*j)->getId() == (*i)->getId())
							{
								solDead = *j;
								break;
							}
						}

						tacstats->daysWounded = 0;

						// NOTE: Safety on *solDead shall not be needed. see above^
						if (tacstats->KIA == true)
							_tactical->injuryList[solDead->getId()] = -1; // kia
						else
							_tactical->injuryList[solDead->getId()] = -2; // mia

						solDead->getDiary()->postTactical(tacstats, _tactical);
						solDead->getDiary()->updateAwards(_rules, tacticals);
						_soldiersLost.push_back(solDead);
				}
			}
		}
		tacticals.push_back(_tactical);
		// Soldier Diary_end.
	}
}

/**
 * dTor.
 */
DebriefingState::~DebriefingState()
{
	if (_game->isQuitting() == true)
		_playSave->setBattleSave();

	for (std::vector<DebriefStat*>::const_iterator
			i  = _statList.begin();
			i != _statList.end();
			++i)
		delete *i;

	for (std::map<TileType, SpecialType*>::const_iterator
			i  = _specialTypes.begin();
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
		if (_destroyPlayerBase == -1) // else deathly silence if Base was destroyed.
		{
			bool playAwardMusic (false);

			// NOTE: These show to player in reverse order.
			if (_unreplaced.empty() == false)
				_game->pushState(new CannotReequipState(_unreplaced));

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
			Soldier* sol;
			for (std::vector<BattleUnit*>::const_iterator
					i  = _unitList->begin();
					i != _unitList->end();
					++i)
			{
				if ((sol = (*i)->getGeoscapeSoldier()) != nullptr)
					participants.push_back(sol);
			}

			if (_playSave->handlePromotions(participants) == true)
			{
				playAwardMusic = true;
				_game->pushState(new PromotionsState());
			}

			switch (_capture)
			{
				case ARR_DIES:
					_game->pushState(new AlienDiesState());
					break;

				case ARR_CONTAINMENT:
					_game->pushState(new AlienContainmentState(
															_base,
															OPT_BATTLESCAPE,
															nullptr,
															0));
					_game->pushState(new ErrorMessageState(
													tr("STR_CONTAINMENT_EXCEEDED").arg(_base->getLabel()),
													_palette,
													_rules->getInterface("debriefing")->getElement("errorMessage")->color,
													"BACK04.SCR",
													_rules->getInterface("debriefing")->getElement("errorPalette")->color));
			}

			if (_solStatIncr.empty() == false
				|| _lostProperty.empty() == false
				|| _surplus.empty() == false)
			{
				_game->pushState(new DebriefExtraState(
													_base,
													_battleSave->getOperation(),
													_surplus, // NOTE: Does not include special tile-types.
													_lostProperty,
													_solStatIncr));
			}

			if (_base->storesOverfull() == true)
			{
//				_game->pushState(new SellState(_base, OPT_BATTLESCAPE));
				_game->pushState(new ErrorMessageState(
												tr("STR_STORAGE_EXCEEDED").arg(_base->getLabel()),
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


		if (_playSave->isIronman() == true) // save after mission
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
	_playSave->setBattleSave();	// delete SavedBattleGame.
}								// NOTE: BattlescapeState and BattlescapeGame are still VALID here. Okay ......
								// State will be deleted by the engine and the battle will be deleted by the state.
/**
 * Adds a result to the debriefing-stats.
 * @param type	- reference to the untranslated type-ID of the stat
 * @param score	- the score to add
 * @param qty	- the quantity to add (default 1)
 */
void DebriefingState::addResultStat( // private.
		const std::string& type,
		int score,
		int qty)
{
	for (std::vector<DebriefStat*>::const_iterator
			i  = _statList.begin();
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
 ** FUNCTOR **
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
		void operator ()(AlienMission* const mission) const;
};

/**
 * Removes the link between the AlienMission and an AlienBase if one existed.
 * @param mission - pointer to the AlienMission
 */
void ClearAlienBase::operator ()(AlienMission* const mission) const
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
			i  = _rules->getItemsList().begin();
			i != _rules->getItemsList().end();
			++i)
	{
		itRule = _rules->getItemRule(*i);
		const TileType tileType (itRule->getTileType());
		switch (tileType)
		{
//			case TILE
//			case START_TILE:
			case UFO_POWER_SOURCE:		//  2
			case UFO_NAVIGATION:		//  3
			case UFO_CONSTRUCTION:		//  4
			case ALIEN_FOOD:			//  5
			case ALIEN_REPRODUCTION:	//  6
			case ALIEN_ENTERTAINMENT:	//  7
			case ALIEN_SURGERY:			//  8
			case ALIEN_EXAMINATION:		//  9
			case ALIEN_ALLOYS:			// 10
			case ALIEN_HABITAT:			// 11
			case RUINED_ALLOYS:			// 12 -> give half-Alloy value for ruined alloy-tiles.
//			case EXIT_TILE:
//			case OBJECTIVE_TILE:
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
		objectiveTextFail;
	int
		objectiveScore     (0), // dang vc++ compiler warnings.
		objectiveScoreFail (0); // dang vc++ compiler warnings.

	const RuleAlienDeployment* const ruleDeploy (_rules->getDeployment(_battleSave->getTacticalType()));
	if (ruleDeploy != nullptr)
	{
		if (ruleDeploy->getObjectiveCompleteInfo(
											objectiveText,
											objectiveScore) == true)
		{
			_statList.push_back(new DebriefStat(objectiveText));
		}

		if (ruleDeploy->getObjectiveFailedInfo(
											objectiveTextFail,
											objectiveScoreFail) == false)
		{
			_statList.push_back(new DebriefStat(objectiveTextFail));
		}
	}

	_statList.push_back(new DebriefStat(TAC_RESULT[ 0u])); // aLiens killed
	_statList.push_back(new DebriefStat(TAC_RESULT[ 1u])); // aLiens captured
	_statList.push_back(new DebriefStat(TAC_RESULT[ 2u])); // aLien corpses recovered
	_statList.push_back(new DebriefStat(TAC_RESULT[ 3u])); // aLien artefacts recovered
	_statList.push_back(new DebriefStat(TAC_RESULT[ 4u])); // aLien base control destroyed
	_statList.push_back(new DebriefStat(TAC_RESULT[ 5u])); // civilians killed by aLiens
	_statList.push_back(new DebriefStat(TAC_RESULT[ 6u])); // civilians killed by player
	_statList.push_back(new DebriefStat(TAC_RESULT[ 7u])); // civilians rescued
	_statList.push_back(new DebriefStat(TAC_RESULT[ 8u])); // agents killed
	_statList.push_back(new DebriefStat(TAC_RESULT[ 9u])); // agents left behind
	_statList.push_back(new DebriefStat(TAC_RESULT[10u])); // supports destroyed
	_statList.push_back(new DebriefStat(TAC_RESULT[11u])); // craft lost
	// TODO: "Base is Lost".
//	_statList.push_back(new DebriefStat("STR_XCOM_AGENTS_RETIRED_THROUGH_INJURY"));

	for (std::map<TileType, SpecialType*>::const_iterator
			i  = _specialTypes.begin();
			i != _specialTypes.end();
			++i)
	{
		if (i->first != RUINED_ALLOYS)
			_statList.push_back(new DebriefStat((*i).second->type, true));
	}
	_statList.push_back(new DebriefStat(_rules->getAlienFuelType(), true));


	// Resolve tiles for Latent and Latent_Start units. Aka post-2nd-stage
	Tile
		* const tileOrigin (_battleSave->getTile(Position(0,0,0))),
		* const tileEquipt (_battleSave->getBattleInventory());
	for (std::vector<BattleUnit*>::const_iterator
			i  = _unitList->begin();
			i != _unitList->end();
			++i)
	{
		//Log(LOG_INFO) << ". check Latent id-" << (*i)->getId();
		switch ((*i)->getUnitStatus())
		{
			case STATUS_LATENT:
				//Log(LOG_INFO) << ". . is Latent";
				(*i)->setUnitTile(tileOrigin);
				break;

			case STATUS_LATENT_START:
				//Log(LOG_INFO) << ". . is Latent_Start";
				(*i)->setUnitTile(tileEquipt);
				break;

			default:
				continue;
		}

		if ((*i)->isStunned() == true)
			(*i)->setUnitStatus(STATUS_UNCONSCIOUS);
		else
			(*i)->setUnitStatus(STATUS_STANDING);
	}


	// Resolve tiles for other units that do not have a Tile.
	Position pos;
	const Position& posBogus (Position(-1,-1,-1));
	for (std::vector<BattleUnit*>::const_iterator
			i  = _unitList->begin();
			i != _unitList->end();
			++i)
	{
		//Log(LOG_INFO) << ". check Tile id-" << (*i)->getId() << " " << (*i)->getPosition();
		if ((*i)->getUnitTile() == nullptr)										// This unit is not on a tile ... give it one.
		{
			if ((pos = (*i)->getPosition()) == posBogus)						// in fact, this Unit is in limbo ... ie, is carried.
			{
				//Log(LOG_INFO) << ". . . cur pos " << pos;
				switch ((*i)->getUnitStatus())
				{
					case STATUS_DEAD:
					case STATUS_UNCONSCIOUS:
					{
						//Log(LOG_INFO) << ". . . . is Dead or Unconscious - find body";
						for (std::vector<BattleItem*>::const_iterator			// so look for its body or corpse ...
								j  = _battleSave->getItems()->begin();
								j != _battleSave->getItems()->end();
								++j)
						{
							if (   (*j)->getBodyUnit() != nullptr				// found it: corpse is a dead or unconscious BattleUnit!!
								&& (*j)->getBodyUnit() == *i)
							{
								if ((*j)->getOwner() != nullptr)				// corpse of BattleUnit has an Owner (ie. is being carried by another BattleUnit)
								{
									//Log(LOG_INFO) << ". . . . . is carried";
									pos = (*j)->getOwner()->getPosition();		// Put the corpse down .. slowly.
								}
								else if ((*j)->getTile() != nullptr)			// corpse of BattleUnit is lying around somewhere <- THIS SHOULD NEVER RUN except post-2nd-stage ->
								{
									//Log(LOG_INFO) << ". . . . . is fielded";
									pos = (*j)->getTile()->getPosition();		// you're not vaporized yet, Get up.
								}
								break;
							}
						}
					}
				}
			}
			//Log(LOG_INFO) << ". . assign Tile @ " << pos;
			(*i)->setUnitTile(_battleSave->getTile(pos));	// why, exactly, do all units need a Tile:
		}													// because it drops still-standing-aLiens' inventories to the ground in preparation for the item-recovery phase.
	}														// And Unconscious/latent civies need to check if they're on an Exit-tile ...
															// And isOnTiletype() requires that unit have a tile set.

	for (std::vector<BattleUnit*>::const_iterator
			i  = _unitList->begin();
			i != _unitList->end();
			++i)
	{
		if ((*i)->getOriginalFaction() == FACTION_HOSTILE
			&& (*i)->getUnitStatus() == STATUS_STANDING)
		{
			_isHostileStanding = true;
			break;
		}
	}
	//Log(LOG_INFO) << ". isHostileStanding= " << _isHostileStanding;
	//Log(LOG_INFO) << ". allObjectivesDestroyed= " << _battleSave->allObjectivesDestroyed();
	_tactical->success = _isHostileStanding == false // NOTE: Can still be a playerWipe and lose Craft/recovery (but not a Base).
					  || _battleSave->allObjectivesDestroyed() == true;
	//Log(LOG_INFO) << ". success= " << _tactical->success;


	const TacticalType tacType (_battleSave->getTacType());
	//Log(LOG_INFO) << ". tactical type " << _battleSave->getTacticalType();


	double
		lon (0.), // avoid vc++ linker warnings.
		lat (0.); // avoid vc++ linker warnings.

	std::vector<Craft*>::const_iterator pCraft;

	bool found (false);
	for (std::vector<Base*>::const_iterator
			i  = _playSave->getBases()->begin();
			i != _playSave->getBases()->end() && found == false;
			++i)
	{
		if ((*i)->getTactical() == true) // in case this DON'T have a Craft, ie. BaseDefense
		{
			//Log(LOG_INFO) << ". Base " << Language::wstrToFs((*i)->getLabel());
			found = true;

			_base = *i;
			_txtBaseLabel->setText(_base->getLabel());

			lon = _base->getLongitude();
			lat = _base->getLatitude();

			if (_isHostileStanding == false)
			{
				_base->setTactical(false);

				bool facDestroyed (false);
				for (std::vector<BaseFacility*>::const_iterator
						j  = _base->getFacilities()->begin();
						j != _base->getFacilities()->end();
						)
				{
					if (_battleSave->baseDestruct()[static_cast<size_t>((*j)->getX())]
												   [static_cast<size_t>((*j)->getY())].second == 0) // this facility was demolished
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
			else // player's Base will be destroyed if any aLiens are left standing.
				_destroyPlayerBase = _base->calcLostScore();
		}

		for (std::vector<Craft*>::const_iterator
				j  = (*i)->getCrafts()->begin();
				j != (*i)->getCrafts()->end() && found == false;
				++j)
		{
			if ((*j)->getTactical() == true) // has Craft, ergo NOT BaseDefense
			{
				//Log(LOG_INFO) << ". Craft " << Language::wstrToFs((*j)->getLabel(_game->getLanguage()));
				found = true;

				if (_isQuickBattle == false)
					_missionCost = (*i)->expenseCraft(*j);

				lon = (*j)->getLongitude();
				lat = (*j)->getLatitude();

				_craft = *j;
				_base = *i;
				_txtBaseLabel->setText(_base->getLabel());

				if (_isHostileStanding == false || _aborted == true)
					_craft->setTacticalReturn();
				else
					pCraft = j; // to delete the Craft below_
			}
		}
	}

	for (std::vector<Region*>::const_iterator
			i  = _playSave->getRegions()->begin();
			i != _playSave->getRegions()->end();
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
			i  = _playSave->getCountries()->begin();
			i != _playSave->getCountries()->end();
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
	found = false;
	for (std::vector<Ufo*>::const_iterator // First - search for UFO.
			i  = _playSave->getUfos()->begin();
			i != _playSave->getUfos()->end() && found == false;
			++i)
	{
		if ((*i)->getTactical() == true)
		{
			//Log(LOG_INFO) << ". Ufo " << Language::wstrToFs((*i)->getLabel(_game->getLanguage()));
			found = true;

			_txtRecovery->setText(tr("STR_UFO_RECOVERY"));
			_tactical->ufo = (*i)->getRules()->getType();

			if (_tactical->success == true)
			{
				delete *i; // NOTE: dTor sends Craft targeters back to Base.
				_playSave->getUfos()->erase(i);
			}
			else
			{
				(*i)->setTactical(false);

				if ((*i)->getUfoStatus() == Ufo::LANDED)
					(*i)->setSecondsLeft(5);
			}
		}
	}

	for (std::vector<TerrorSite*>::const_iterator // Second - search for TerrorSite.
			i  = _playSave->getTerrorSites()->begin();
			i != _playSave->getTerrorSites()->end() && found == false;
			++i)
	{
		if ((*i)->getTactical() == true)
		{
			//Log(LOG_INFO) << ". TerrorSite " << Language::wstrToFs((*i)->getLabel(_game->getLanguage()));
			found = true;

			delete *i; // NOTE: dTor sends Craft targeters back to Base.
			_playSave->getTerrorSites()->erase(i);
		}
		// else TerrorSite vanishes.
	}

	for (std::vector<AlienBase*>::const_iterator // Third - search for aLienBase.
			i  = _playSave->getAlienBases()->begin();
			i != _playSave->getAlienBases()->end() && found == false;
			++i)
	{
		if ((*i)->getTactical() == true)
		{
			//Log(LOG_INFO) << ". AlienBase " << Language::wstrToFs((*i)->getLabel(_game->getLanguage()));
			found = true;

			_txtRecovery->setText(tr("STR_ALIEN_BASE_RECOVERY"));

			if (_tactical->success == true)
			{
				if (objectiveText.empty() == false)
				{
					objectiveScore = std::max((objectiveScore + 9) / 10, // round up.
											   objectiveScore - (_diff * 50));
					addResultStat(
								objectiveText,
								objectiveScore);
				}

				std::for_each(
						_playSave->getAlienMissions().begin(),
						_playSave->getAlienMissions().end(),
						ClearAlienBase(*i));

				delete *i; // NOTE: dTor sends Craft targeters back to Base.
				_playSave->getAlienBases()->erase(i);
			}
			else
				(*i)->setTactical(false);
		}
	}


	Soldier* sol;
	for (std::vector<BattleUnit*>::const_iterator // resolve all BattleUnits ->
			i  = _unitList->begin();
			i != _unitList->end();
			++i)
	{
		switch ((*i)->getOriginalFaction())
		{
			case FACTION_PLAYER:
				switch ((*i)->getUnitStatus())
				{
					case STATUS_DEAD:
						//Log(LOG_INFO) << ". player Dead id-" << (*i)->getId() << " " << (*i)->getType();
						if ((sol = (*i)->getGeoscapeSoldier()) != nullptr)
						{
							(*i)->getStatistics()->KIA = true;
							(*i)->postMissionProcedures(true);
//							_solStatIncr[sol->getLabel()] = (*i)->postMissionProcedures(true); // don't bother showing dead soldier stat increases.

							if (_isQuickBattle == false)
								_missionCost += _base->expenseSoldier(sol, true);

							addResultStat(
										TAC_RESULT[8u], // agent killed
										-(*i)->getValue());

							for (std::vector<Soldier*>::const_iterator
									j  = _base->getSoldiers()->begin();
									j != _base->getSoldiers()->end();
									++j)
							{
								if (*j == sol) // NOTE: Could return any armor the Soldier was wearing to Stores. CHEATER!!!!!
								{
									(*j)->die(_playSave);
									delete *j;
									_base->getSoldiers()->erase(j);
									break;
								}
							}
						}
						else // support unit.
						{
							if (_isQuickBattle == false)
							{
								const int quad ((*i)->getArmor()->getSize());
								_missionCost += _base->expenseSupport(
																quad * quad,
																true);
							}
							addResultStat(
										TAC_RESULT[10u], // support destroyed
										-(*i)->getValue());

							++_lostProperty[_rules->getItemRule((*i)->getType())];

							const BattleItem* ordnance ((*i)->getItem(ST_RIGHTHAND));
							if (ordnance != nullptr)
							{
								itRule = ordnance->getRules();
								if (itRule->isFixed() == true
									&& itRule->getFullClip() > 0
									&& (ordnance = ordnance->getAmmoItem()) != nullptr)
								{
									_lostProperty[ordnance->getRules()] += itRule->getFullClip();
								}
							}
						}
						break;

					case STATUS_STANDING:
					case STATUS_UNCONSCIOUS:
						//Log(LOG_INFO) << ". player Standing or Unconscious id-" << (*i)->getId() << " " << (*i)->getType();
						if (_isHostileStanding == false
							|| (_aborted == true && (*i)->isOnTiletype(START_TILE) == true))	// NOTE: And not BaseDefense if anyone is stupid
						{																		// enough to design a base-block with Start_Tile's.
							recoverItems((*i)->getInventory());

							if ((sol = (*i)->getGeoscapeSoldier()) != nullptr)
							{
								_solStatIncr[sol->getLabel()] = (*i)->postMissionProcedures();

								if (_isQuickBattle == false)
									_missionCost += _base->expenseSoldier(sol);

//								sol->calcStatString(
//												_rules->getStatStrings(),
//												Options::psiStrengthEval
//													&& _playSave->isResearched(_rules->getPsiRequirements()));
							}
							else // support unit.
							{
								if (_isQuickBattle == false)
								{
									const int unitSize ((*i)->getArmor()->getSize());
									_missionCost += _base->expenseSupport(unitSize * unitSize);
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
											_lostProperty[itRule] += clip - qtyLoad;
									}
								}
							}
						}
						else // unit is MIA.
						{
							//Log(LOG_INFO) << ". . playerWipe set unit MIA & Status_Dead";
							(*i)->setUnitStatus(STATUS_DEAD);

							if ((sol = (*i)->getGeoscapeSoldier()) != nullptr)
							{
								(*i)->getStatistics()->MIA = true;
								(*i)->postMissionProcedures(true);
//								_solStatIncr[sol->getLabel()] = (*i)->postMissionProcedures(true); // don't bother showing dead soldier stat increases.

								addResultStat(
											TAC_RESULT[9u], // agent left behind
											-(*i)->getValue());

								for (std::vector<Soldier*>::const_iterator
										j  = _base->getSoldiers()->begin();
										j != _base->getSoldiers()->end();
										++j)
								{
									if (*j == sol) // NOTE: Could return any armor the Soldier was wearing to Stores. CHEATER!!!!!
									{
										(*j)->die(_playSave);
										delete *j;
										_base->getSoldiers()->erase(j);
										break;
									}
								}
							}
							else // support unit.
							{
								addResultStat(
											TAC_RESULT[10u], // support destroyed
											-(*i)->getValue());

								++_lostProperty[_rules->getItemRule((*i)->getType())];

								const BattleItem* ordnance ((*i)->getItem(ST_RIGHTHAND));
								if (ordnance != nullptr)
								{
									itRule = ordnance->getRules();
									if (itRule->isFixed() == true
										&& itRule->getFullClip() > 0
										&& (ordnance = ordnance->getAmmoItem()) != nullptr)
									{
										_lostProperty[ordnance->getRules()] += itRule->getFullClip();
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
						//Log(LOG_INFO) << ". hostile Dead id-" << (*i)->getId() << " " << (*i)->getType();
						if ((*i)->killerFaction() == FACTION_PLAYER)
						{
							//Log(LOG_INFO) << ". . killed by xCom";
							addResultStat(
										TAC_RESULT[0u], // aLien killed
										(*i)->getValue());
						}
						break;

					case STATUS_UNCONSCIOUS:
						//Log(LOG_INFO) << ". hostile Unconscious id-" << (*i)->getId() << " " << (*i)->getType();
						++_aliensStunned; // for Nike Cross determination.
				}
				break;

			case FACTION_NEUTRAL:
				switch ((*i)->getUnitStatus())
				{
					case STATUS_DEAD:
						//Log(LOG_INFO) << ". neutral Dead id-" << (*i)->getId() << " " << (*i)->getType();
						switch ((*i)->killerFaction())
						{
							case FACTION_PLAYER:
								//Log(LOG_INFO) << ". . killed by xCom";
								addResultStat(
											TAC_RESULT[6u], // civilian killed by player
											-((*i)->getValue() << 1u));
								break;

							case FACTION_HOSTILE:
							case FACTION_NEUTRAL:
								addResultStat(
											TAC_RESULT[5u], // civilian killed by aLiens
											-(*i)->getValue());
						}
						break;

					case STATUS_STANDING:
						for (std::vector<BattleItem*>::const_iterator // in case Civie was MC'd and given stuff.
								j  = (*i)->getInventory()->begin();
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
						//Log(LOG_INFO) << ". neutral Standing or Unconscious id-" << (*i)->getId() << " " << (*i)->getType();
						if (_isHostileStanding == false
							|| (_aborted == true && (*i)->isOnTiletype(START_TILE) == true))
						{
							addResultStat(
										TAC_RESULT[7u], // civilian rescued
										(*i)->getValue());
						}
						else
							addResultStat(
										TAC_RESULT[5u], // civilian killed by aLiens
										-(*i)->getValue());
				}
		}
	} // End loop BattleUnits.

	for (std::vector<BattleUnit*>::const_iterator // only to check IronMan and LoneSurvivor awards ->
			i  = _unitList->begin();
			i != _unitList->end();
			++i)
	{
		if ((*i)->getOriginalFaction() == FACTION_PLAYER)
		{
			switch ((*i)->getUnitStatus())
			{
				case STATUS_DEAD:
					//Log(LOG_INFO) << ". . player Dead";
					++_playerDead;
					break;

				case STATUS_STANDING:
				case STATUS_UNCONSCIOUS:
					//Log(LOG_INFO) << ". . player Standing or Unconscious";
					++_playerLive;
			}
		}
	}


	if (_craft != nullptr
		&& (_isHostileStanding == true && _aborted == false))
	{
		//Log(LOG_INFO) << ". craft LOST";
		addResultStat(
					TAC_RESULT[11u], // craft lost
					-(_craft->getRules()->getScore()));

		delete _craft;
		_craft = nullptr;
		_base->getCrafts()->erase(pCraft);
	}

	std::string tacResult;
	if (_tactical->success == true)
	{
		//Log(LOG_INFO) << ". success TRUE";
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
			addResultStat(
						objectiveText,
						objectiveScore);
	}
	else // hostiles Standing AND objective NOT completed
	{
		//Log(LOG_INFO) << ". success FALSE";
		switch (tacType)
		{
			case TCT_DEFAULT: //-1
				tacResult = "Warning: no TacType";
				break;

			case TCT_BASEDEFENSE:
				tacResult = "STR_BASE_IS_LOST";

				for (std::vector<Craft*>::const_iterator
						i  = _base->getCrafts()->begin();
						i != _base->getCrafts()->end();
						++i)
				{
					addResultStat(
								TAC_RESULT[11u], // craft lost
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

		if (objectiveTextFail.empty() == false)
			addResultStat(
						objectiveTextFail,
						objectiveScoreFail);
	}

	if (_isHostileStanding == false || _aborted == true)	// NOTE: And not BaseDefense if anyone is stupid
	{														// enough to design a base-block with Start_Tile's.
		//Log(LOG_INFO) << ". recover Guaranteed";
		recoverItems(_battleSave->recoverGuaranteed());

		if (_destroyPlayerBase == -1)
		{
			Tile* tile;
			for (size_t
					i = 0u;
					i != _battleSave->getMapSizeXYZ();
					++i)
			{
				tile = _battleSave->getTiles()[i];
				if (   tile->getMapData(O_FLOOR) != nullptr
					&& tile->getMapData(O_FLOOR)->getTileType() == START_TILE)
				{
					recoverItems(tile->getInventory());
				}
			}
		}
	}

	if (_isHostileStanding == false)
	{
		//Log(LOG_INFO) << ". recover Conditional";
		recoverItems(_battleSave->recoverConditional());

		TileType
			tileType,
			objectType;

		if (ruleDeploy != nullptr)
			objectType = ruleDeploy->getPlayerObjective();
		else
			objectType = TILE;

		const int parts (static_cast<int>(Tile::PARTS_TILE));
		const MapData* part;

		int qtyAlloysRuined (0);

		Tile* tile;
		for (size_t
				i = 0u;
				i != _battleSave->getMapSizeXYZ();
				++i)
		{
			tile = _battleSave->getTiles()[i];

			if (   tile->getMapData(O_FLOOR) == nullptr
				|| tile->getMapData(O_FLOOR)->getTileType() != START_TILE)
			{
				recoverItems(tile->getInventory());
			}

			for (int
					j = 0;
					j != parts;
					++j)
			{
				if ((part = tile->getMapData(static_cast<MapDataType>(j))) != nullptr
					&& (tileType = part->getTileType()) != objectType)	// not sure why parts of objectType should not be recovered.
				{														// in fact I think it's just another wotWarboy.
					switch (tileType)
					{
						case TILE:
						case START_TILE:
						case EXIT_TILE:
						case OBJECTIVE_TILE:
							break;				// NOTE: These are not included in '_specialTypes' above^

						case RUINED_ALLOYS:		// NOTE: This is in '_specialTypes' above^ (adds half-value to Alloys' pts)
							++qtyAlloysRuined;	// but not included on the statList displayed for Debriefing.
							break;

//						default:
						case UFO_POWER_SOURCE:	// NOTE: These are all in '_specialTypes' above^
						case UFO_NAVIGATION:
						case UFO_CONSTRUCTION:
						case ALIEN_FOOD:
						case ALIEN_REPRODUCTION:
						case ALIEN_ENTERTAINMENT:
						case ALIEN_SURGERY:
						case ALIEN_EXAMINATION:
						case ALIEN_ALLOYS:
						case ALIEN_HABITAT:
//							if (_specialTypes.find(tileType) != _specialTypes.end())
							addResultStat(
										_specialTypes[tileType]->type,
										_specialTypes[tileType]->value);
					}
				}
			}
		}

		for (std::vector<DebriefStat*>::const_iterator
				i  = _statList.begin();
				i != _statList.end();
				++i)
		{
			if ((*i)->type == _specialTypes[ALIEN_ALLOYS]->type)
			{
				int alloyDivisor; // TODO: Subtract diff*10 for gameDifficulty.
				switch (tacType)
				{
					case TCT_BASEASSAULT: alloyDivisor = 150; break;
					default:              alloyDivisor = 15;
				}

				(*i)->qty = ((*i)->qty + (qtyAlloysRuined >> 1u)) / alloyDivisor;
				(*i)->score = ((*i)->score + ((qtyAlloysRuined * _specialTypes[RUINED_ALLOYS]->value) >> 1u)) / alloyDivisor;

				if ((*i)->qty != 0 && (*i)->recover == true)
					_surplus[_rules->getItemRule((*i)->type)] = (*i)->qty; // NOTE: Elerium is handled in recoverItems().
			}

			if ((*i)->qty != 0 && (*i)->recover == true)
				_base->getStorageItems()->addItem((*i)->type, (*i)->qty);
		}
	}


	for (std::vector<BattleItem*>::const_iterator
			i  = _battleSave->deletedProperty().begin();
			i != _battleSave->deletedProperty().end();
			++i)
	{
		//Log(LOG_INFO) << ". handle deleted property";
		if (_surplus.find(itRule = (*i)->getRules()) == _surplus.end())
			++_lostProperty[itRule];
		else if (--_surplus[itRule] == 0)	// NOTE: '_surplusItems' shall never contain clips - vid. recoverItems()
			_surplus.erase(itRule);			// ... clips handled immediately below_
	}										// TODO: Extensive testing on item-gains/losses ....

	int
		qtyFullClip,
		clipsTotal;
	for (std::map<const RuleItem*, int>::const_iterator
			i  = _clips.begin();	// '_clips' is a tally of both xcomProperty + found clips
			i != _clips.end();		// so '_clipsProperty' needs to be subtracted to find clipsGained.
			++i)					// and clipsLost needs to be bypassed if NOT '_clipsProperty'.
	{
		//Log(LOG_INFO) << ". handle clips";
		if ((qtyFullClip = i->first->getFullClip()) != 0) // safety.
		{
			clipsTotal = i->second / qtyFullClip;
			switch (clipsTotal)
			{
				case 0:					// all clips-of-type are lost including those brought on the mission
					if (i->second != 0)	// and if there's a partial-clip that needs to be added to the lost-vector too iff it's xCom Property.
					{
						std::map<const RuleItem*, int>::const_iterator j (_clipsProperty.find(i->first));
						if (j != _clipsProperty.end())
							++_lostProperty[i->first];
					}
					break;

				default:				// clips were found whether xcomProperty or not. Add them to Base-stores!
				{
					_lostProperty.erase(i->first);

					int roundsProperty;
					std::map<const RuleItem*, int>::const_iterator j (_clipsProperty.find(i->first));
					if (j != _clipsProperty.end())
						roundsProperty = j->second;
					else
						roundsProperty = 0;

					int clipsGained ((i->second - roundsProperty) / qtyFullClip);
					if (clipsGained != 0)
						_surplus[i->first] = clipsGained;			// these clips are over & above those brought as xcomProperty.

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
			if (_destroyPlayerBase == -1 || _isQuickBattle == true)
			{
				//Log(LOG_INFO) << ". BaseDefense - reequip craft";
				for (std::vector<Craft*>::const_iterator
						i  = _base->getCrafts()->begin();
						i != _base->getCrafts()->end();
						++i)
				{
					if ((*i)->getCraftStatus() != CS_OUT)
						reequipCraft(*i);
				}
			}
			else
			{
				//Log(LOG_INFO) << ". BaseDefense - delete base";
				for (std::vector<Base*>::const_iterator
						i  = _playSave->getBases()->begin();
						i != _playSave->getBases()->end();
						++i)
				{
					if (*i == _base) // IMPORTANT: player's Base is destroyed here! (but not if QuickBattle)
					{
						// TODO: Try to route any Craft/Soldiers/Equipment that are out on a tactical mission
						// to other Bases.
						// - search other bases for hangars
						// - search other bases for free personnel space
						// - search other bases for available equipment stores
						//
						// If hangar(s) not available destroy Craft(s).
						// If personnel space unavailable add Soldiers anyway but set up an "overfull" nag-notice like overfull-storage has.
						// If storage for equipment not available add Items to other base-stores and let the overfull-storage nag-notice inform player.

						delete *i;
						_playSave->getBases()->erase(i);
						break;
					}
				}
			}

			if (_region != nullptr && _isQuickBattle == false)
			{
				//Log(LOG_INFO) << ". BaseDefense - delete retal Ufos & AlienMission";
				const AlienMission* const retalMission (_playSave->findAlienMission(
																				_region->getRules()->getType(),
																				alm_RETAL));
				for (std::vector<Ufo*>::const_iterator
						i  = _playSave->getUfos()->begin();
						i != _playSave->getUfos()->end();
						)
				{
					if ((*i)->getAlienMission() == retalMission)
					{
						delete *i; // NOTE: dTor sends Craft targeters back to Base.
						i = _playSave->getUfos()->erase(i);
					}
					else
						++i;
				}

				for (std::vector<AlienMission*>::const_iterator
						i  = _playSave->getAlienMissions().begin();
						i != _playSave->getAlienMissions().end();
						++i)
				{
					if (*i == retalMission)
					{
						delete *i;
						_playSave->getAlienMissions().erase(i);
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
			//Log(LOG_INFO) << ". reequip Craft";
			reequipCraft(_craft);	// not a BaseDefense.
	}
}

/**
 * Reequips a Craft after tactical.
 * TODO: Figure out what's going on w/ QuickBattles and base-storage. Eg, the
 * proper tabulation of gains/losses might rely on returning stuff to Base stores.
 * @param craft - pointer to a Craft
 */
void DebriefingState::reequipCraft(Craft* const craft) // private.
{
	if (craft != nullptr) // required.
	{
		int
			baseQty,
			qtyUnreplaced;

		ItemContainer* const craftContainer (craft->getCraftItems());
		const std::map<std::string, int> craftContents (*craftContainer->getContents()); // <- make a copy so you don't have to screw around with iteration here.
		for (std::map<std::string, int>::const_iterator
				i  = craftContents.begin();
				i != craftContents.end();
				++i)
		{
			if ((baseQty = _base->getStorageItems()->getItemQuantity(i->first)) < i->second)
			{
				_base->getStorageItems()->removeItem(i->first, baseQty);

				qtyUnreplaced = i->second - baseQty;
				craftContainer->removeItem(i->first, qtyUnreplaced);

				const UnreplacedStat stat
				{
					i->first,
					qtyUnreplaced,
					craft->getLabel(_game->getLanguage())
				};
				_unreplaced.push_back(stat);
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
					i  = craft->getVehicles()->begin();
					i != craft->getVehicles()->end();
					++i)
			{
				craftVehicles.addItem((*i)->getRules()->getType());
				delete *i;
			}
			craft->getVehicles()->clear();

			int
				qtySupport,
				quads;
			for (std::map<std::string, int>::const_iterator
					i  = craftVehicles.getContents()->begin();
					i != craftVehicles.getContents()->end();
					++i)
			{
				if ((baseQty = _base->getStorageItems()->getItemQuantity(i->first)) < i->second)
				{
					const UnreplacedStat stat
					{
						i->first,
						i->second - baseQty,
						craft->getLabel(_game->getLanguage())
					};
					_unreplaced.push_back(stat);
				}

				quads = _rules->getArmor(_rules->getUnitRule(i->first)->getArmorType())->getSize();
				quads *= quads;

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
																quads));
					}
					_base->getStorageItems()->removeItem(i->first, qtySupport);
				}
				else
				{
					const std::string& type (itRule->getAcceptedLoadTypes()->front());
					const int
						clipsRequired (itRule->getFullClip()),
						baseClips (_base->getStorageItems()->getItemQuantity(type));

					if ((qtyUnreplaced = (clipsRequired * i->second) - baseClips) > 0)
					{
						const UnreplacedStat stat
						{
							type,
							qtyUnreplaced,
							craft->getLabel(_game->getLanguage())
						};
						_unreplaced.push_back(stat);
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
																	quads));
						}
						_base->getStorageItems()->removeItem(i->first, qtySupport);
						_base->getStorageItems()->removeItem(type, clipsRequired * qtySupport);
					}
				}
			}
		}
	}
}

/**
 * Recovers items from tactical.
 * TODO: Figure out what's going on w/ QuickBattles and base-storage. Eg, the
 * proper tabulation of gains/losses might rely on returning stuff to Base stores.
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
			i  = battleItems->begin();
			i != battleItems->end();
			++i)
	{
		itRule = (*i)->getRules();

		if (itRule->isFixed() == false && itRule->isRecoverable() == true)
		{
			_base->refurbishCraft(type = itRule->getType());

			bType = itRule->getBattleType();
			switch (bType)
			{
				case BT_FUEL:
					_surplus[itRule] += _rules->getAlienFuelQuantity();
					addResultStat(
								_rules->getAlienFuelType(),
								itRule->getRecoveryScore(),
								_rules->getAlienFuelQuantity());
					break;

				default:
					if (bType != BT_CORPSE && _playSave->isResearched(type) == false)
						addResultStat(
									TAC_RESULT[3u], // aLien artefacts recovered
									itRule->getRecoveryScore());

					switch (bType) // shuttle all times instantly to the Base
					{
						case BT_CORPSE:
							if ((unit = (*i)->getBodyUnit()) != nullptr)
							{
								switch (unit->getUnitStatus())
								{
									case STATUS_DEAD:
									{
										addResultStat(
													TAC_RESULT[2u], // aLien corpses recovered
													unit->getValue() / 3);	// TODO: This should rather be the 'recoveryPoints' of the corpse-item;
																			// but at present all the corpse-rules spec. default values of 3 or 5 pts. Cf, below_
										const std::string& corpse (unit->getArmor()->getCorpseGeoscape());
										if (corpse.empty() == false) // safety.
										{
											_base->getStorageItems()->addItem(corpse);
											++_surplus[_rules->getItemRule(corpse)];
										}
										break;
									}

									case STATUS_UNCONSCIOUS:
										if (unit->getOriginalFaction() == FACTION_HOSTILE)
											recoverLiveAlien(unit);

									// TODO: have to handle STATUS_LATENT and/or STATUS_LATENT_START here if
									// I ever bother to implement 2-stage missions that aren't the final battle.
								}
							}
							break;

						case BT_AMMO:
							_clips[itRule] += (*i)->getAmmoQuantity();
							if ((*i)->isProperty() == true)
								_clipsProperty[itRule] += (*i)->getAmmoQuantity();
							break;

						case BT_FIREARM:
							if ((*i)->selfPowered() == false)
							{
								const BattleItem* const clip ((*i)->getAmmoItem());
								if (clip != nullptr) //&& clip->getRules()->getFullClip() != 0) // <- nobody be stupid and make a clip with 0 ammo-capacity.
								{
									_clips[clip->getRules()] += clip->getAmmoQuantity();
									if ((*i)->isProperty() == true)
										_clipsProperty[clip->getRules()] += clip->getAmmoQuantity();
								}
							}
							// no break;

						default:
							_base->getStorageItems()->addItem(type);
							if ((*i)->isProperty() == false)
								++_surplus[itRule];
					}
			}
		}
	}
}

/**
 * Recovers a live aLien from the battlefield.
 * TODO: Figure out what's going on w/ QuickBattles and base-storage. Eg, the
 * proper tabulation of gains/losses might rely on returning stuff to Base stores.
 * @param unit - pointer to a BattleUnit to recover
 */
void DebriefingState::recoverLiveAlien(const BattleUnit* const unit) // private.
{
	if (_isQuickBattle == true || _base->hasContainment() == true)
	{
		//Log(LOG_INFO) << ". . . alienLive id-" << unit->getId() << " " << unit->getType();
		const std::string& type (unit->getType());

		int value;
		if (_rules->getResearch(type) != nullptr
			&& _playSave->isResearched(type) == false)
		{
			value = unit->getValue() << 1u;
		}
		else
			value = unit->getValue();

		addResultStat(
					TAC_RESULT[1u], // aLien captured
					value);

		_base->getStorageItems()->addItem(type);
		++_surplus[_rules->getItemRule(type)];

		if (_base->getFreeContainment() < 0)
			_capture = ARR_CONTAINMENT;
		else
			_capture = ARR_NONE;
	}
	else
	{
		//Log(LOG_INFO) << ". . . alienDead id-" << unit->getId() << " " << unit->getType();
		_capture = ARR_DIES;

		addResultStat(
					TAC_RESULT[2u], // aLien corpse recovered
					unit->getValue() / 3);	// TODO: This should rather be the 'recoveryPoints' of the corpse-item;
											// but at present all the corpse-rules spec. default values of 3 or 5 pts. Cf, above^
		const std::string& corpse (unit->getArmor()->getCorpseGeoscape());
		if (corpse.empty() == false) // safety. (Or error-out if there isn't one.)
		{
			_base->getStorageItems()->addItem(corpse); // NOTE: This won't be a quick-battle here, okay to add to Base stores.
			++_surplus[_rules->getItemRule(corpse)];
		}
	}
}

}
