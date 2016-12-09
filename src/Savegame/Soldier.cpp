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

#include "Soldier.h"

#include "Base.h"
#include "Craft.h"
#include "ItemContainer.h"
#include "SavedGame.h"
#include "SoldierDead.h"
#include "SoldierDeath.h"
#include "SoldierDiary.h"
#include "SoldierLayout.h"

#include "../fmath.h"

#include "../Engine/Language.h"
//#include "../Engine/Logger.h"
#include "../Engine/RNG.h"

#include "../Interface/Text.h"

#include "../Ruleset/RuleArmor.h"
#include "../Ruleset/Ruleset.h"
#include "../Ruleset/RuleSoldier.h"
//#include "../Ruleset/SoldierNamePool.h"
//#include "../Ruleset/StatString.h"


namespace OpenXcom
{

/**
 * Creates a brand new Soldier from scratch.
 * @param solRule	- pointer to RuleSoldier
 * @param arRule	- pointer to RuleArmor
 * @param id		- ID from soldier-generation
// * @param names	- pointer to a vector of pointers to SoldierNamePool (default nullptr)
 */
Soldier::Soldier(
		const RuleSoldier* const solRule,
		const RuleArmor* const arRule,
		int id)
//		const std::vector<SoldierNamePool*>* const names,
	:
		_solRule(solRule),
		_arRule(arRule),
		_id(id),
		_rank(RANK_ROOKIE),
		_craft(nullptr),
		_missions(0),
		_kills(0),
		_recovery(0),
		_psiTraining(false),
		_recentlyPromoted(false),
		_isQuickBattle(false)
//		_gender(GENDER_MALE),
//		_look(LOOK_BLONDE),
{
	_diary = new SoldierDiary(); // diary remains default-initialized.

//	if (names != nullptr)
//	{
	const UnitStats
		statsLow  (_solRule->getMinStats()),
		statsHigh (_solRule->getMaxStats());

	_initialStats.tu			= RNG::generate(statsLow.tu,			statsHigh.tu);
	_initialStats.stamina		= RNG::generate(statsLow.stamina,		statsHigh.stamina);
	_initialStats.health		= RNG::generate(statsLow.health,		statsHigh.health);
	_initialStats.bravery		= RNG::generate(statsLow.bravery / 10,	statsHigh.bravery / 10) * 10;
	_initialStats.reactions		= RNG::generate(statsLow.reactions,		statsHigh.reactions);
	_initialStats.firing		= RNG::generate(statsLow.firing,		statsHigh.firing);
	_initialStats.throwing		= RNG::generate(statsLow.throwing,		statsHigh.throwing);
	_initialStats.strength		= RNG::generate(statsLow.strength,		statsHigh.strength);
	_initialStats.psiStrength	= RNG::generate(statsLow.psiStrength,	statsHigh.psiStrength);
	_initialStats.melee			= RNG::generate(statsLow.melee,			statsHigh.melee);

	_initialStats.psiSkill = 0;

	_currentStats = _initialStats;


	_look = static_cast<SoldierLook>(RNG::generate(0,3));

//	_gender = (SoldierGender)RNG::generate(0,1);
	// gender Ratios
	const RuleGender* const gRatio (_solRule->getGenderRatio());
	const double
		male  (static_cast<double>(gRatio->male)),
		total (static_cast<double>(gRatio->female) + male);

	if (AreSame(total, 0.)
		|| RNG::percent(static_cast<int>(Round(male / total * 100.))))
	{
		_gender = GENDER_MALE;
		_label = L"pfc.Fritz";
	}
	else
	{
		_gender = GENDER_FEMALE;
		_label = L"pfc.Frita";
	}
//		if (names->empty() == false)
//		{
//			size_t nationality (RNG::generate(0, names->size() - 1));
//			_label = names->at(nationality)->genName(&_gender, _solRule->getFemaleFrequency());
//
//			// Once the ability to mod in extra looks is added this will need to
//			// reference the ruleset for a max-quantity of look-types.
//			_look = (SoldierLook)names->at(nationality)->genLook(4);
//		}
//		else
//		{
//			_gender = (RNG::percent(_solRule->getFemaleFrequency()) ? GENDER_FEMALE : GENDER_MALE);
//			_look = (SoldierLook)RNG::generate(0, 3);
//			_label = (_gender == GENDER_FEMALE) ? L"Astrid" : L"Rupert";
//			_label += L" Hutzinger";
//		}
//	}
}

/**
 * Creates a Soldier to be filled w/ YAML data.
 * @note I believe this will finally resolve that RNG-reloading discrepancy. And
 * cause faster loading generally.
 * @param solRule - pointer to RuleSoldier
 */
Soldier::Soldier(const RuleSoldier* const solRule)
	:
		_solRule(solRule),
		_arRule(nullptr),
		_id(0),
		_rank(RANK_ROOKIE),
		_gender(GENDER_MALE),
		_look(LOOK_BLONDE),
		_craft(nullptr),
		_missions(0),
		_kills(0),
		_recovery(0),
		_psiTraining(false),
		_recentlyPromoted(false),
		_isQuickBattle(false),
		_initialStats(UnitStats()),
		_currentStats(UnitStats())
{
	_diary = new SoldierDiary(); // empty diary. Shall fill by YAML.
}

/**
 * dTor.
 */
Soldier::~Soldier()
{
	for (std::vector<SoldierLayout*>::const_iterator
			i = _layout.begin();
			i != _layout.end();
			++i)
		delete *i;

	delete _diary;
}

/**
 * Loads this Soldier from a YAML file.
 * @param node	- reference a YAML node
 * @param rules	- pointer to the Ruleset
 */
void Soldier::load(
		const YAML::Node& node,
		const Ruleset* const rules)
{
	//Log(LOG_INFO) << "Soldier::load()";
	_label = Language::utf8ToWstr(node["label"].as<std::string>(""));

	_id				= node["id"]			.as<int>(_id);
	_missions		= node["missions"]		.as<int>(_missions);
	_kills			= node["kills"]			.as<int>(_kills);
	_recovery		= node["recovery"]		.as<int>(_recovery);
	_psiTraining	= node["psiTraining"]	.as<bool>(_psiTraining);
	_isQuickBattle	= node["isQuickBattle"]	.as<bool>(_isQuickBattle);

	_rank	= static_cast<SoldierRank>(node["rank"]		.as<int>(0));
	_gender	= static_cast<SoldierGender>(node["gender"]	.as<int>(0));
	_look	= static_cast<SoldierLook>(node["look"]		.as<int>(0));

	_initialStats = node["initialStats"].as<UnitStats>(_initialStats);
	_currentStats = node["currentStats"].as<UnitStats>(_currentStats);

	_arRule = rules->getArmor(node["armor"].as<std::string>(""));
	//if (_arRule != nullptr) Log(LOG_INFO) << ". armor [1] = " << _arRule->getType();
	if (_arRule == nullptr)
	{
		_arRule = rules->getArmor(_solRule->getArmor());
		//if (_arRule != nullptr) Log(LOG_INFO) << ". armor [2] = " << _arRule->getType();
	}

	//Log(LOG_INFO) << ". load layout";
	if (const YAML::Node& layout = node["layout"])
	{
		SoldierLayout* layoutItem;
		for (YAML::const_iterator
				i = layout.begin();
				i != layout.end();
				++i)
		{
			layoutItem = new SoldierLayout(*i);
			if (rules->getInventory(layoutItem->getLayoutSection()) != nullptr)
				_layout.push_back(layoutItem);
			else
				delete layoutItem;
		}
	}

	//Log(LOG_INFO) << ". load diary";
	if (node["diary"] != nullptr)
	{
		//Log(LOG_INFO) << ". . diary exists";
		_diary->load(node["diary"]);
	}
	//else Log(LOG_INFO) << ". . diary invalid";

//	calcStatString(
//			rules->getStatStrings(),
//			(Options::psiStrengthEval && save->isResearched(rule->getPsiRequirements())));
}

/**
 * Saves this Soldier to a YAML file.
 * @return, YAML node
 */
YAML::Node Soldier::save() const
{
	YAML::Node node;

	node["label"]	= Language::wstrToUtf8(_label);

	node["type"]	= _solRule->getType();
	node["id"]		= _id;

	if (_missions != 0)			node["missions"]		= _missions;
	if (_kills != 0)			node["kills"]			= _kills;
	if (_recovery != 0)			node["recovery"]		= _recovery;
	if (_psiTraining == true)	node["psiTraining"]		= _psiTraining;
	if (_isQuickBattle == true)	node["isQuickBattle"]	= _isQuickBattle;

	if (_rank	!= RANK_ROOKIE) node["rank"]	= static_cast<int>(_rank);
	if (_gender	!= GENDER_MALE) node["gender"]	= static_cast<int>(_gender);
	if (_look	!= LOOK_BLONDE) node["look"]	= static_cast<int>(_look);

	node["initialStats"] = _initialStats;
	node["currentStats"] = _currentStats;

	node["armor"] = _arRule->getType();

	if (_craft != nullptr) node["craft"] = _craft->saveIdentificator();

	for (std::vector<SoldierLayout*>::const_iterator
			i = _layout.begin();
			i != _layout.end();
			++i)
	{
		node["layout"].push_back((*i)->save());
	}

	if (   _diary->getTacticalIdList().empty() == false
		|| _diary->getSoldierAwards().empty()  == false)
	{
		node["diary"] = _diary->save();
	}

	return node;
}

/**
 * Gets this Soldier's rules.
 * @return, pointer to RuleSoldier
 */
const RuleSoldier* Soldier::getRules() const
{
	return _solRule;
}

/**
 * Gets this Soldier's initial stats.
 * @return, address of UnitStats
 */
const UnitStats* Soldier::getInitStats()
{
	return &_initialStats;
}

/**
 * Gets this Soldier's current stats.
 * @return, address of UnitStats
 */
UnitStats* Soldier::getCurrentStats()
{
	return &_currentStats;
}

/**
 * Gets this Soldier's ID.
 * @note Each soldier is uniquely identified by its ID.
 * @return, the ID
 */
int Soldier::getId() const
{
	return _id;
}

/**
 * Sets this Soldier's label.
 * @param label - reference to the label
 */
void Soldier::setLabel(const std::wstring& label)
{
	_label = label;
}

/**
 * Gets this Soldier's label and optionally statString.
// * @param statstring	- true to add stat string
// * @param maxLength	- restrict length to this value
 * @return, soldier label
 */
std::wstring Soldier::getLabel() const
{
	return _label;
}
/* std::wstring Soldier::getLabel(
		bool statstring,
		size_t maxLength) const
{
	if (statstring == true && _statString.empty() == false)
	{
		if (_label.length() + _statString.length() > maxLength)
			return _label.substr(0, maxLength - _statString.length()) + L"/" + _statString;
		else return _label + L"/" + _statString;
	}
	return _label;
} */

/**
 * Gets the Craft this Soldier is assigned to.
 * @return, pointer to the craft
 */
Craft* Soldier::getCraft() const
{
	return _craft;
}

/**
 * Assigns this Soldier to a specified Craft.
 * @note Also tries to load/unload the soldier's layout-items.
 * @param craft			- pointer to a craft (default nullptr)
 * @param base			- pointer to the craft's Base (default nullptr)
 * @param isQuickBattle	- true if quick-battle (default false)
 */
void Soldier::setCraft(
		Craft* const craft,
		Base* const base,
		bool isQuickBattle)
{
	if (base != nullptr) // load/unload layout-items w/ Soldier
	{
		std::string type;
		if (craft != nullptr) // load items
		{
			for (std::vector<SoldierLayout*>::const_iterator
					i = _layout.begin();
					i != _layout.end();
					++i)
			{
				if (craft->calcLoadCurrent() < craft->getLoadCapacity())
				{
					type = (*i)->getItemType();
					if (base->getStorageItems()->getItemQuantity(type) != 0) //|| isQuickBattle == true)
					{
						craft->getCraftItems()->addItem(type);
						if (isQuickBattle == false)
							base->getStorageItems()->removeItem(type);

						type = (*i)->getAmmoType();
						if (type.empty() == false)
						{
							if (craft->calcLoadCurrent() < craft->getLoadCapacity())
							{
								if (base->getStorageItems()->getItemQuantity(type) != 0) //|| isQuickBattle == true)
								{
									craft->getCraftItems()->addItem(type);
									if (isQuickBattle == false)
										base->getStorageItems()->removeItem(type);
								}
							}
							else
								break;
						}
					}
				}
				else
					break;
			}
		}
		else // unload items
		{
			for (std::vector<SoldierLayout*>::const_iterator
					i = _layout.begin();
					i != _layout.end();
					++i)
			{
				type = (*i)->getItemType();
				if (_craft->getCraftItems()->getItemQuantity(type) != 0)
				{
					_craft->getCraftItems()->removeItem(type);
					if (isQuickBattle == false)
						base->getStorageItems()->addItem(type);

					type = (*i)->getAmmoType();
					if (type.empty() == false
						&& _craft->getCraftItems()->getItemQuantity(type) != 0)
					{
						_craft->getCraftItems()->removeItem(type);
						if (isQuickBattle == false)
							base->getStorageItems()->addItem(type);
					}
				}
			}
		}
	}
	_craft = craft;
}

/**
 * Gets this Soldier's craft-string, which is either the
 * soldier's wounded status, the assigned craft-label, or none.
 * @param lang - pointer to Language to get translations from
 * @return, craft-label as a wide-string
 */
std::wstring Soldier::getCraftLabel(const Language* const lang) const
{
	if (_recovery != 0)
		return lang->getString("STR_WOUNDED").arg(_recovery);

	if (_craft != nullptr)
		return _craft->getLabel(lang, _isQuickBattle == false);

	return lang->getString("STR_NONE_UC");
}

/**
 * Gets a localizable-string representation of this Soldier's military rank.
 * @return, string-ID for rank
 */
std::string Soldier::getRankString() const
{
	switch (_rank)
	{
		case RANK_ROOKIE:		return "STR_ROOKIE";
		case RANK_SQUADDIE:		return "STR_SQUADDIE";
		case RANK_SERGEANT:		return "STR_SERGEANT";
		case RANK_CAPTAIN:		return "STR_CAPTAIN";
		case RANK_COLONEL:		return "STR_COLONEL";
		case RANK_COMMANDER:	return "STR_COMMANDER";
	}
	return "";
}

/**
 * Gets a graphic representation of this Soldier's military rank.
 * @note THE MEANING OF LIFE
 * @return, sprite-ID for rank
 */
int Soldier::getRankSprite() const
{
	return 42 + _rank;
}

/**
 * Gets this Soldier's military rank.
 * @return, rank (Soldier.h)
 */
SoldierRank Soldier::getRank() const
{
	return _rank;
}

/**
 * Increases this Soldier's military rank.
 */
void Soldier::promoteRank()
{
	_rank = static_cast<SoldierRank>(static_cast<int>(_rank) + 1);

	if (_rank > RANK_SQUADDIE) // only promotions above SQUADDIE are worth mentioning.
		_recentlyPromoted = true;
}

/**
 * Adds kills and a mission to this Soldier's stats.
 * @param kills - qty of kills
 */
void Soldier::postTactical(int kills)
{
	++_missions;
	_kills += kills;
}

/**
 * Gets this Soldier's quantity of missions.
 * @return, missions
 */
int Soldier::getMissions() const
{
	return _missions;
}

/**
 * Gets this Soldier's quantity of kills.
 * @return, kills
 */
int Soldier::getKills() const
{
	return _kills;
}

/**
 * Gets this Soldier's gender.
 * @return, gender (Soldier.h)
 */
SoldierGender Soldier::getGender() const
{
	return _gender;
}

/**
 * Gets this Soldier's look.
 * @return, look (Soldier.h)
 */
SoldierLook Soldier::getLook() const
{
	return _look;
}

/**
 * Returns this Soldier's promotion status and resets it.
 * @return, true if recently promoted
 */
bool Soldier::isPromoted()
{
	const bool ret (_recentlyPromoted);
	_recentlyPromoted = false;

	return ret;
}

/**
 * Gets this Soldier's current armor.
 * @return, pointer to Armor rule
 */
const RuleArmor* Soldier::getArmor() const
{
	return _arRule;
}

/**
 * Sets this Soldier's current armor.
 * @param arRule - pointer to Armor rule
 */
void Soldier::setArmor(const RuleArmor* const arRule)
{
	_arRule = arRule;
}

/**
 * Gets the amount of time until this Soldier is fully healed.
 * @return, quantity of days
 */
int Soldier::getSickbay() const
{
	return _recovery;
}

/**
 * Sets the amount of time until this Soldier is fully healed.
 * @param recovery - quantity of days
 */
void Soldier::setSickbay(int recovery)
{
	if ((_recovery = recovery) != 0)
	{
		_craft = nullptr;		// dismiss from craft
		_psiTraining = false;	// dismiss from psi-training
	}
}

/**
 * Gets the color for the Soldier's wound-recovery time.
 * @return, color
 */
Uint8 Soldier::getSickbayColor()
{
	static const Uint8
		GREEN	=  48u,
		ORANGE	=  96u,
		YELLOW	= 144u;

	const int pct (getPctWounds());
	if (pct > 50)
		return ORANGE;

	if (pct > 10)
		return YELLOW;

	return GREEN;
}

/**
 * Gets this Soldier's remaining woundage as a percent.
 * @return, recovery as percent
 */
int Soldier::getPctWounds() const
{
	return static_cast<int>(std::floor(
		   static_cast<float>(_recovery) / static_cast<float>(_currentStats.health) * 100.f));
}

/**
 * Heals this Soldier's wounds a single day.
 */
void Soldier::heal()
{
	if (_recovery != 0) --_recovery;
}

/**
 * Gets the list of EquipmentLayoutItems of this Soldier.
 * @return, pointer to a vector of pointers to SoldierLayout
 */
std::vector<SoldierLayout*>* Soldier::getLayout()
{
	return &_layout;
}

/**
 * Trains this Soldier's psychic abilities once per day.
 * @note Called from GeoscapeState::time1Day().
 * @return, true if Soldier's psi-stat(s) increased
 */
bool Soldier::trainPsiDay()
{
	static const int
		PSI_PCT    (5),		// pct per day to get psionically active
		PSI_FACTOR (500);	// coefficient for chance of improvement

	bool ret (false);
	if (_psiTraining == true
		&& _currentStats.psiSkill < _solRule->getStatCaps().psiSkill) // hard cap. Note this auto-caps psiStrength also.
	{
		switch (_currentStats.psiSkill)
		{
			case 0:
				if (RNG::percent(PSI_PCT) == true)
				{
					ret = true;
					int psiSkill (RNG::generate(
											_solRule->getMinStats().psiSkill,
											_solRule->getMaxStats().psiSkill));
					if (psiSkill < 1) psiSkill = 1;

					_currentStats.psiSkill =
					_initialStats.psiSkill = psiSkill;
				}
				break;

			default: // Psi unlocked already.
			{
				int pct (std::max(1, PSI_FACTOR / _currentStats.psiSkill));
				if (RNG::percent(pct) == true)
				{
					ret = true;
					++_currentStats.psiSkill;
				}

				if (_currentStats.psiStrength < _solRule->getStatCaps().psiStrength) //&& Options::allowPsiStrengthImprovement
				{
					pct = std::max(1, PSI_FACTOR / _currentStats.psiStrength);
					if (RNG::percent(pct) == true)
					{
						ret = true;
						++_currentStats.psiStrength;
					}
				}
			}
		}
	}
	return ret;
}

/**
 * Gets whether or not this Soldier is in psi-training.
 * @return, true if training
 */
bool Soldier::inPsiTraining() const
{
	return _psiTraining;
}

/**
 * Toggles whether or not this Soldier is in psi-training.
 */
void Soldier::togglePsiTraining()
{
	_psiTraining = !_psiTraining;
}

/**
 * Sets this Soldier as a quick-battle soldier.
 */
void Soldier::setQuickBattle()
{
	_isQuickBattle = true;
}

/**
 * Kills this Soldier in Debriefing or the Geoscape.
 * @param gameSave - pointer to the SavedGame
 */
void Soldier::die(SavedGame* const gameSave)
{
	SoldierDeath* const death (new SoldierDeath());
	death->setTime(*gameSave->getTime());

	SoldierDead* const deadSoldier (new SoldierDead(
												_label,
												_id,
												_rank,
												_gender,
												_look,
												_missions,
												_kills,
												death,
												_initialStats,
												_currentStats,
												*_diary)); // base if I want to ... TODO: Use "&&" operator.
	gameSave->getDeadSoldiers()->push_back(deadSoldier);
}

/**
 * Gets this Soldier's diary.
 * @return, pointer to SoldierDiary
 */
SoldierDiary* Soldier::getDiary() const
{
	return _diary;
}

/**
 * Calculates this Soldier's statString.
 * @param statStrings		- reference to a vector of pointers to statString rules
 * @param psiStrengthEval	- true if psi stats are available
 *
void Soldier::calcStatString(const std::vector<StatString*>& statStrings, bool psiStrengthEval)
{
	_statString = StatString::calcStatString(_currentStats, statStrings, psiStrengthEval);
} */

/**
 * Automatically renames this Soldier according to his/her current statistics.
 */
void Soldier::autoStat()
{
	std::wostringstream stat;

	switch (_rank)
	{
		case 0: stat << L"r"; break;
		case 1: stat << L"q"; break;
		case 2: stat << L"t"; break;
		case 3: stat << L"p"; break;
		case 4: stat << L"n"; break;
		case 5: stat << L"x"; break;

		default: stat << L"z";
	}

	stat << _currentStats.firing << L".";
	stat << _currentStats.reactions << L".";
	stat << _currentStats.strength;

	switch (_currentStats.bravery)
	{
		case  10: stat << L"a";	break;
		case  20: stat << L"b";	break;
		case  30: stat << L"c";	break;
		case  40: stat << L"d";	break;
		case  50: stat << L"e";	break;
		case  60: stat << L"f";	break;
		case  70: stat << L"g";	break;
		case  80: stat << L"h";	break;
		case  90: stat << L"i";	break;
		case 100: stat << L"j";	break;

		default: stat << L"z";
	}

	if (_currentStats.psiSkill != 0)
	{
		stat << (_currentStats.psiStrength + _currentStats.psiSkill / 5);

		if (_currentStats.psiSkill >= _solRule->getStatCaps().psiSkill)
			stat << L":";
		else
			stat << L".";

		stat << (_currentStats.psiStrength * _currentStats.psiSkill / 100);
	}

	_label = stat.str();
}

/**
 * Gets this Soldier's wage for battles or salary.
 * @param tactical - true for tactical-cost, false for monthly salary (default true)
 */
int Soldier::getSoldierExpense(bool tactical) const
{
	if (tactical == true)
		return static_cast<int>(_rank) * 1500;

	return static_cast<int>(_rank) * 5000;
}

}
