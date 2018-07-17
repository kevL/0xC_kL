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

#include "Base.h"

//#include <algorithm>
#include <iomanip>
#include <stack>

//#include "../fmath.h"

#include "BaseFacility.h"
#include "Craft.h"
#include "CraftWeapon.h"
#include "ItemContainer.h"
#include "ManufactureProject.h"
#include "ResearchProject.h"
#include "Soldier.h"
#include "Target.h"
#include "Transfer.h"
#include "Ufo.h"
#include "Vehicle.h"

#include "../Engine/Game.h"
#include "../Engine/Language.h"
#include "../Engine/Logger.h"
#include "../Engine/Options.h"
#include "../Engine/RNG.h"

#include "../Geoscape/GeoscapeState.h"
#include "../Geoscape/Globe.h" // Globe::GLM_BASE

#include "../Ruleset/RuleArmor.h"
#include "../Ruleset/RuleBaseFacility.h"
#include "../Ruleset/RuleCraft.h"
#include "../Ruleset/RuleCraftWeapon.h"
#include "../Ruleset/RuleItem.h"
#include "../Ruleset/RuleManufacture.h"
#include "../Ruleset/RuleResearch.h"
#include "../Ruleset/Ruleset.h"
#include "../Ruleset/RuleSoldier.h"

#include "../Savegame/SavedGame.h"


namespace OpenXcom
{


/**
 * Instantiates a Base.
 * @param rule - pointer to Ruleset
 */
Base::Base(
		const Ruleset* const rules,
		SavedGame* const playSave)
	:
		Target(),
		_rules(rules),
		_playSave(playSave),
		_scientists(0),
		_engineers(0),
		_tactical(false),
		_exposed(false),
		_cashIncome(0),
		_cashSpent(0),
		_defenseReduction(0),
		_recallPurchase(0),
		_recallSell(0),
		_recallSoldier(0),
		_recallTransfer(0),
		_placed(false),
		_isQuickDefense(false)
{
	_items = new ItemContainer();
}

/**
 * Deletes the contents of this Base.
 */
Base::~Base()
{
	for (std::vector<BaseFacility*>::const_iterator
			i = _facilities.begin();
			i != _facilities.end();
			++i)
		delete *i;

	for (std::vector<Soldier*>::const_iterator
			i = _soldiers.begin();
			i != _soldiers.end();
			++i)
		delete *i;

	for (std::vector<Craft*>::const_iterator
			i = _crafts.begin();
			i != _crafts.end();
			++i)
		delete *i;

	for (std::vector<Transfer*>::const_iterator
			i = _transfers.begin();
			i != _transfers.end();
			++i)
		delete *i;

	for (std::vector<ManufactureProject*>::const_iterator
			i = _projectsManufacture.begin();
			i != _projectsManufacture.end();
			++i)
		delete *i;

	for (std::vector<ResearchProject*>::const_iterator
			i = _projectsResearch.begin();
			i != _projectsResearch.end();
			++i)
		delete *i;

	delete _items;
}

/**
 * Loads this Base from a YAML file.
 * @param node			- reference a YAML node
 * @param isFirstBase	- true if this is the first Base of a new game (default false)
 */
void Base::loadBase(
		const YAML::Node& node,
		bool isFirstBase)
{
	//Log(LOG_INFO) << "Base load()";
	Target::load(node);
	//Log(LOG_INFO) << ". target loaded";

	_label = Language::utf8ToWstr(node["label"].as<std::string>(""));
	//Log(LOG_INFO) << ". label is set";

	std::string type;

	//Log(LOG_INFO) << ". load facilities";
	if (isFirstBase == false)
	{
		_placed = true;
		for (YAML::const_iterator
				i = node["facilities"].begin();
				i != node["facilities"].end();
				++i)
		{
			type = (*i)["type"].as<std::string>();
			if (_rules->getBaseFacility(type) != nullptr)
			{
				BaseFacility* const facility (new BaseFacility(
															_rules->getBaseFacility(type),
															this));
				facility->load(*i);
				_facilities.push_back(facility);
			}
			else Log(LOG_ERROR) << "Base::loadBase() Failed to load facility " << type;
		}
	}

	//Log(LOG_INFO) << ". load crafts";
	for (YAML::const_iterator
			i = node["crafts"].begin();
			i != node["crafts"].end();
			++i)
	{
		type = (*i)["type"].as<std::string>();
		if (_rules->getCraft(type) != nullptr)
		{
			Craft* const craft (new Craft(
										_rules->getCraft(type),
										this,
										_playSave,
										true));
			craft->loadCraft(*i, _rules);
			_crafts.push_back(craft);
		}
		else Log(LOG_ERROR) << "Base::loadBase() Failed to load craft " << type;
	}

	//Log(LOG_INFO) << ". load soldiers";
	for (YAML::const_iterator
			i = node["soldiers"].begin();
			i != node["soldiers"].end();
			++i)
	{
		type = (*i)["type"].as<std::string>(_rules->getSoldiersList().front());
		//Log(LOG_INFO) << ". . type = " << type;
		if (_rules->getSoldier(type) != nullptr)
		{
			//Log(LOG_INFO) << ". . . load Soldier";
			Soldier* const sol (new Soldier(_rules->getSoldier(type)));
			sol->load(*i, _rules);
			sol->setCraft();

			//Log(LOG_INFO) << ". . . set Craft";
			if (const YAML::Node& craft = (*i)["craft"])
			{
				const CraftId craftId (Craft::loadIdentificator(craft));
				//Log(LOG_INFO) << ". . . . Craft-ID = " << craftId.first << "-" << craftId.second;
				for (std::vector<Craft*>::const_iterator
						j = _crafts.begin();
						j != _crafts.end();
						++j)
				{
					if ((*j)->getIdentificator() == craftId)
					{
						sol->setCraft(*j);
						break;
					}
				}
			}
			_soldiers.push_back(sol);
		}
		else Log(LOG_ERROR) << "Base::loadBase() Failed to load soldier " << type;
	}

	//Log(LOG_INFO) << ". load items";
	_items->load(node["items"]);
	for (std::map<std::string, int>::const_iterator
			i = _items->getContents()->begin();
			i != _items->getContents()->end();)
	{
		if (_rules->getItemRule(i->first) == nullptr)
		{
			Log(LOG_ERROR) << "Base::loadBase() Failed to load item " << i->first;
			i = _items->getContents()->erase(i);
		}
		else
			++i;
	}

	//Log(LOG_INFO) << ". load transfers";
	for (YAML::const_iterator
			i = node["transfers"].begin();
			i != node["transfers"].end();
			++i)
	{
		const int hours ((*i)["hours"].as<int>());
		Transfer* const transfer (new Transfer(hours));
		if (transfer->load(*i, this, _rules) == true)
			_transfers.push_back(transfer);
	}

	//Log(LOG_INFO) << ". load research projects";
	for (YAML::const_iterator
			i = node["research"].begin();
			i != node["research"].end();
			++i)
	{
		type = (*i)["project"].as<std::string>();
		if (_rules->getResearch(type) != nullptr)
		{
			ResearchProject* const research (new ResearchProject(_rules->getResearch(type)));
			research->load(*i);
			_projectsResearch.push_back(research);
		}
		else
		{
			Log(LOG_ERROR) << "Base::loadBase() Failed to load research " << type;
			_scientists += (*i)["assigned"].as<int>(0);
		}
	}

	//Log(LOG_INFO) << ". load manufacturing projects";
	for (YAML::const_iterator
			i = node["manufacture"].begin();
			i != node["manufacture"].end();
			++i)
	{
		type = (*i)["item"].as<std::string>();
		if (_rules->getManufacture(type) != nullptr)
		{
			ManufactureProject* const production (new ManufactureProject(_rules->getManufacture(type)));
			production->load(*i);
			_projectsManufacture.push_back(production);
		}
		else
		{
			Log(LOG_ERROR) << "Base::loadBase() Failed to load manufacture " << type;
			_engineers += (*i)["assigned"].as<int>(0);
		}
	}

	//Log(LOG_INFO) << ". load vars";
	_tactical		= node["tactical"]		.as<bool>(_tactical);
	_exposed		= node["exposed"]		.as<bool>(_exposed);
	_scientists		= node["scientists"]	.as<int>(_scientists);
	_engineers		= node["engineers"]		.as<int>(_engineers);

	_cashIncome		= node["cashIncome"]	.as<int>(_cashIncome);
	_cashSpent		= node["cashSpent"]		.as<int>(_cashSpent);

	_isQuickDefense	= node["isQuickDefense"].as<bool>(_isQuickDefense);
}

/**
 * Saves this Base to a YAML file.
 * @return, YAML node
 */
YAML::Node Base::save() const
{
	YAML::Node node (Target::save());

	node["label"] = Language::wstrToUtf8(_label);

	for (std::vector<BaseFacility*>::const_iterator
			i = _facilities.begin();
			i != _facilities.end();
			++i)
		node["facilities"].push_back((*i)->save());

	for (std::vector<Soldier*>::const_iterator
			i = _soldiers.begin();
			i != _soldiers.end();
			++i)
		node["soldiers"].push_back((*i)->save());

	for (std::vector<Craft*>::const_iterator
			i = _crafts.begin();
			i != _crafts.end();
			++i)
		node["crafts"].push_back((*i)->save());

	node["items"] = _items->save();

	for (std::vector<Transfer*>::const_iterator
			i = _transfers.begin();
			i != _transfers.end();
			++i)
		node["transfers"].push_back((*i)->save());

	for (std::vector<ResearchProject*>::const_iterator
			i = _projectsResearch.begin();
			i != _projectsResearch.end();
			++i)
		node["research"].push_back((*i)->save());

	for (std::vector<ManufactureProject*>::const_iterator
			i = _projectsManufacture.begin();
			i != _projectsManufacture.end();
			++i)
		node["manufacture"].push_back((*i)->save());

	if (_tactical == true)	node["tactical"]	= _tactical;
	if (_exposed == true)	node["exposed"]		= _exposed;
	if (_scientists != 0)	node["scientists"]	= _scientists;
	if (_engineers != 0)	node["engineers"]	= _engineers;

	if (_cashIncome != 0)	node["cashIncome"]	= _cashIncome;
	if (_cashSpent != 0)	node["cashSpent"]	= _cashSpent;

	if (_isQuickDefense == true) node["isQuickDefense"] = _isQuickDefense;

	return node;
}

/**
 * Saves this Base's identificator to a YAML file.
 * @return, YAML node
 */
YAML::Node Base::saveIdentificator() const
{
	YAML::Node node (Target::save());

	node["type"] = Target::stTarget[1u];
	node["id"]   = 0;

	return node;
}

/**
 * Gets the player-specified label for this Base.
 * @note The Language-ptr is neither used nor needed for Base-Targets.
 * @param lang	- pointer to Language to get strings from (default nullptr)
 * @param id	- true to show the Id (default true)
 * @return, the base-label as a wide-string
 */
std::wstring Base::getLabel(
		const Language* const,
		bool) const
{
	return _label;
}

/**
 * Sets the player-specified label for this Base.
 * @param label - reference to a wide-string
 */
void Base::setLabel(const std::wstring& label)
{
	_label = label;
}

/**
 * Gets the globe-marker for this Base.
 * @return, marker-ID (-1 if not placed)
 */
int Base::getMarker() const
{
	if (_placed == true)
		return Globe::GLM_BASE;

	return -1;
}

/**
 * Gets the list of BaseFacilities in this Base.
 * @return, pointer to a vector of pointers to facilities at this base
 */
std::vector<BaseFacility*>* Base::getFacilities()
{
	return &_facilities;
}

/**
 * Gets the list of Soldiers in this Base.
 * @return, pointer to a vector of pointers to soldiers at this base
 */
std::vector<Soldier*>* Base::getSoldiers()
{
	return &_soldiers;
}

/**
 * Gets the list of Crafts in this Base.
 * @return, pointer to a vector of pointers to crafts at this base
 */
std::vector<Craft*>* Base::getCrafts()
{
	return &_crafts;
}

/**
 * Gets the list of Transfers destined to this Base.
 * @return, pointer to a vector of pointers to transfers to this base
 */
std::vector<Transfer*>* Base::getTransfers()
{
	return &_transfers;
}

/**
 * Gets the list of items in this Base.
 * @return, pointer to the ItemContainer for this base
 */
ItemContainer* Base::getStorageItems()
{
	return _items;
}

/**
 * Gets the quantity of scientists currently in this Base.
 * @return, scientists not at work
 */
int Base::getScientists() const
{
	return _scientists;
}

/**
 * Sets the quantity of scientists currently in this Base.
 * @param scientists - scientists
 */
void Base::setScientists(int scientists)
{
	 _scientists = scientists;
}

/**
 * Gets the quantity of engineers currently in this Base.
 * @return, engineers not at work
 */
int Base::getEngineers() const
{
	return _engineers;
}

/**
 * Sets the quantity of engineers currently in this Base.
 * @param engineers - engineers
 */
void Base::setEngineers(int engineers)
{
	 _engineers = engineers;
}

/**
 * Gets the quantity of Soldiers neither on a sortie nor wounded at this Base.
 * @return, quantity of combat-ready soldiers
 */
int Base::getAvailableSoldiers() const
{
	int total (0);
	for (std::vector<Soldier*>::const_iterator
			i = _soldiers.begin();
			i != _soldiers.end();
			++i)
	{
		if (   ((*i)->getCraft() == nullptr && (*i)->getSickbay() == 0)
			|| ((*i)->getCraft() != nullptr && (*i)->getCraft()->getCraftStatus() != CS_OUT))
		{
			++total;
		}
	}
	return total;
}

/**
 * Gets the quantity of healthy Soldiers at this Base.
 * @return, quantity of non-wounded soldiers
 */
int Base::getHealthySoldiers() const
{
	int total (0);
	for (std::vector<Soldier*>::const_iterator
			i = _soldiers.begin();
			i != _soldiers.end();
			++i)
	{
		if ((*i)->getSickbay() == 0)
			++total;
	}
	return total;
}

/**
 * Gets the total quantity of Soldiers at this Base.
 * @return, quantity of total soldiers incl/ transfers
 */
int Base::getTotalSoldiers() const
{
	int total (static_cast<int>(_soldiers.size()));
	for (std::vector<Transfer*>::const_iterator
			i = _transfers.begin();
			i != _transfers.end();
			++i)
	{
		if ((*i)->getTransferType() == PST_SOLDIER)
			++total;
	}
	return total;
}

/**
 * Gets the total quantity of scientists contained in this Base.
 * @return, total scientists incl/ transfers & at work
 */
int Base::getTotalScientists() const
{
	int total (_scientists);
	for (std::vector<Transfer*>::const_iterator
			i = _transfers.begin();
			i != _transfers.end();
			++i)
	{
		if ((*i)->getTransferType() == PST_SCIENTIST)
			total += (*i)->getQuantity();
	}

	for (std::vector<ResearchProject*>::const_iterator
			i = _projectsResearch.begin();
			i != _projectsResearch.end();
			++i)
	{
		total += (*i)->getAssignedScientists();
	}

	return total;
}

/**
 * Gets the quantity of scientists currently in use.
 * @return, quantity of scientists
 */
int Base::getAllocatedScientists() const
{
	int total (0);
	for (std::vector<ResearchProject*>::const_iterator
			i = _projectsResearch.begin();
			i != _projectsResearch.end();
			++i)
	{
		total += (*i)->getAssignedScientists();
	}
	return total;
}

/**
 * Gets the total quantity of engineers contained in this Base.
 * @return, total engineers incl/ transfers & at work
 */
int Base::getTotalEngineers() const
{
	int total (_engineers);
	for (std::vector<Transfer*>::const_iterator
			i = _transfers.begin();
			i != _transfers.end();
			++i)
	{
		if ((*i)->getTransferType() == PST_ENGINEER)
			total += (*i)->getQuantity();
	}

	for (std::vector<ManufactureProject*>::const_iterator
			i = _projectsManufacture.begin();
			i != _projectsManufacture.end();
			++i)
	{
		total += (*i)->getAssignedEngineers();
	}

	return total;
}

/**
 * Gets the quantity of engineers currently in use.
 * @return, quantity of engineers
 */
int Base::getAllocatedEngineers() const
{
	int total (0);
	for (std::vector<ManufactureProject*>::const_iterator
			i = _projectsManufacture.begin();
			i != _projectsManufacture.end();
			++i)
	{
		total += (*i)->getAssignedEngineers();
	}
	return total;
}

/**
 * Gets the quantity of living quarters used up by personnel in this Base.
 * @return, occupied personnel space
 */
int Base::getUsedQuarters() const
{
	return getTotalSoldiers() + getTotalScientists() + getTotalEngineers();
}

/**
 * Gets the total quantity of living quarters in this Base.
 * @return, total personnel space
 */
int Base::getTotalQuarters() const
{
	int total (0);
	for (std::vector<BaseFacility*>::const_iterator
			i = _facilities.begin();
			i != _facilities.end();
			++i)
	{
		if ((*i)->buildFinished() == true)
			total += (*i)->getRules()->getPersonnel();
	}
	return total;
}

/**
 * Gets personnel space not used.
 * @return, free space
 */
int Base::getFreeQuarters() const
{
	return getTotalQuarters() - getUsedQuarters();
}

/**
 * Gets the quantity of storage space used by equipment in this Base.
 * @note This includes equipment on Craft and about to arrive in Transfers as
 * well as any armor that this Base's Soldiers are currently wearing.
 * @return, used storage space
 */
double Base::getUsedStores() const
{
	double total (_items->getTotalSize(_rules)); // items

	const RuleItem* itRule;
	for (std::vector<Craft*>::const_iterator
			i = _crafts.begin();
			i != _crafts.end();
			++i)
	{
		total += (*i)->getCraftItems()->getTotalSize(_rules); // craft items

		for (std::vector<Vehicle*>::const_iterator // craft vehicles (vehicles were counted as items if not on a craft)
				j = (*i)->getVehicles()->begin();
				j != (*i)->getVehicles()->end();
				++j)
		{
			itRule = (*j)->getRules();
			total += itRule->getStoreSize();

			if (itRule->getFullClip() > 0) // craft vehicle ammo
			{
				itRule = _rules->getItemRule(itRule->getAcceptedLoadTypes()->front());
				total += itRule->getStoreSize() * static_cast<double>((*j)->getLoad());
			}
		}
	}

	for (std::vector<Transfer*>::const_iterator // transfers
			i = _transfers.begin();
			i != _transfers.end();
			++i)
	{
		if ((*i)->getTransferType() == PST_ITEM)
		{
			total += _rules->getItemRule((*i)->getTransferItems())->getStoreSize()
				   * static_cast<double>((*i)->getQuantity());
		}
	}

	for (std::vector<Soldier*>::const_iterator // soldier armor
			i = _soldiers.begin();
			i != _soldiers.end();
			++i)
	{
		if ((*i)->getArmor()->isBasic() == false)
			total += _rules->getItemRule((*i)->getArmor()->getStoreItem())->getStoreSize();
	}

	return total;
}

/**
 * Gets the total quantity of stores in this Base.
 * @return, total storage space
 */
int Base::getTotalStores() const
{
	int total (0);
	for (std::vector<BaseFacility*>::const_iterator
			i = _facilities.begin();
			i != _facilities.end();
			++i)
	{
		if ((*i)->buildFinished() == true)
			total += (*i)->getRules()->getStorage();
	}
	return total;
}

/**
 * Checks if this Base's stores are over their limit.
 * @note Supplying an offset will add/subtract to the used capacity before
 * performing the check. A positive offset simulates adding items to the stores
 * whereas a negative offset can be used to check whether sufficient items have
 * been removed to stop stores from overflowing.
 * @param offset - adjusts used capacity (default 0.)
 * @return, true if overfull
 */
bool Base::storesOverfull(double offset) const
{
	return getUsedStores() + offset > static_cast<double>(getTotalStores()) + 0.05;
}

/**
 * Formats the storage-space field for the various buy/sell States.
 * @param delta - the change in store-space (default 0.)
 * @return, the field as a wide-string
 */
std::wstring Base::storesDeltaFormat(double delta) const
{
	std::wostringstream woststr;
	woststr << getTotalStores() << L":" << std::fixed << std::setprecision(1) << getUsedStores();
	if (std::fabs(delta) > 0.05)
	{
		woststr << L" ";
		if (delta > 0.) woststr << L"+";
		woststr << std::fixed << std::setprecision(1) << delta;
	}
	return woststr.str();
}

/**
 * Gets the quantity of laboratories used up by research projects in this Base.
 * @return, used laboratory space
 */
int Base::getUsedLaboratories() const
{
	int total (0);
	const std::vector<ResearchProject*>& research(getResearch());
	for (std::vector<ResearchProject*>::const_iterator
			i = research.begin();
			i != research.end();
			++i)
	{
		total += (*i)->getAssignedScientists();
	}
	return total;
}

/**
 * Gets the total quantity of laboratories in this Base.
 * @return, total laboratory space
 */
int Base::getTotalLaboratories() const
{
	int total (0);
	for (std::vector<BaseFacility*>::const_iterator
			i = _facilities.begin();
			i != _facilities.end();
			++i)
	{
		if ((*i)->buildFinished() == true)
			total += (*i)->getRules()->getLaboratories();
	}
	return total;
}

/**
 * Gets laboratory space not used by ResearchProjects.
 * @return, free space
 */
int Base::getFreeLaboratories() const
{
	return getTotalLaboratories() - getUsedLaboratories();
}

/**
 * Checks if this Base is equipped with research facilities.
 * @return, true if capable of research
 */
bool Base::hasResearch() const
{
	for (std::vector<BaseFacility*>::const_iterator
			i = _facilities.begin();
			i != _facilities.end();
			++i)
	{
		if ((*i)->buildFinished() == true
			&& (*i)->getRules()->getLaboratories() != 0)
		{
			return true;
		}
	}
	return false;
}

/**
 * Gets the quantity of workshops used up by manufacturing projects in this Base.
 * @return, used workshop space
 */
int Base::getUsedWorkshops() const
{
	int total (0);
	for (std::vector<ManufactureProject*>::const_iterator
			i = _projectsManufacture.begin();
			i != _projectsManufacture.end();
			++i)
	{
		total += (*i)->getAssignedEngineers() + (*i)->getRules()->getSpaceRequired();
	}
	return total;
}

/**
 * Gets the total quantity of workshops in this Base.
 * @return, total workshop space
 */
int Base::getTotalWorkshops() const
{
	int total (0);
	for (std::vector<BaseFacility*>::const_iterator
			i = _facilities.begin();
			i != _facilities.end();
			++i)
	{
		if ((*i)->buildFinished() == true)
			total += (*i)->getRules()->getWorkshops();
	}
	return total;
}

/**
 * Gets workshop space not used by Productions.
 * @return, free space
 */
int Base::getFreeWorkshops() const
{
	return getTotalWorkshops() - getUsedWorkshops();
}

/**
 * Checks if this Base is equipped with production facilities.
 * @return, true if capable of production
 */
bool Base::hasProduction() const
{
	for (std::vector<BaseFacility*>::const_iterator
			i = _facilities.begin();
			i != _facilities.end();
			++i)
	{
		if ((*i)->buildFinished() == true
			&& (*i)->getRules()->getWorkshops() != 0)
		{
			return true;
		}
	}
	return false;
}

/**
 * Gets the total quantity of used psilab-space in this Base.
 * @return, used psilab space
 */
int Base::getUsedPsiLabs() const
{
	int total (0);
	for (std::vector<Soldier*>::const_iterator
			i = _soldiers.begin();
			i != _soldiers.end();
			++i)
	{
		if ((*i)->inPsiTraining() == true)
			++total;
	}
	return total;
}

/**
 * Gets the total quantity of psilab-space in this Base.
 * @return, total psilab space
 */
int Base::getTotalPsiLabs() const
{
	int total (0);
	for (std::vector<BaseFacility*>::const_iterator
			i = _facilities.begin();
			i != _facilities.end();
			++i)
	{
		if ((*i)->buildFinished() == true)
			total += (*i)->getRules()->getPsiLaboratories();
	}
	return total;
}

/**
 * Gets psilab-space not in use.
 * @return, free space
 */
int Base::getFreePsiLabs() const
{
	return getTotalPsiLabs() - getUsedPsiLabs();
}

/**
 * Checks if this Base has Psionic Laboratories.
 * @return, true if psiLabs exist
 */
bool Base::hasPsiLabs() const
{
	for (std::vector<BaseFacility*>::const_iterator
			i = _facilities.begin();
			i != _facilities.end();
			++i)
	{
		if ((*i)->buildFinished() == true
			&& (*i)->getRules()->getPsiLaboratories() != 0)
		{
			return true;
		}
	}
	return false;
}

/**
 * Gets the total quantity of used containment-space in this Base.
 * @return, used containment space incl/ transfers & interrogations
 */
int Base::getUsedContainment() const
{
	int total (0);
	for (std::map<std::string, int>::const_iterator
			i = _items->getContents()->begin();
			i != _items->getContents()->end();
			++i)
	{
		if (_rules->getItemRule(i->first)->isLiveAlien() == true)
			total += i->second;
	}

	for (std::vector<Transfer*>::const_iterator
			i = _transfers.begin();
			i != _transfers.end();
			++i)
	{
		if ((*i)->getTransferType() == PST_ITEM
			&& _rules->getItemRule((*i)->getTransferItems())->isLiveAlien() == true)
		{
			total += (*i)->getQuantity();
		}
	}

	return (total += getInterrogatedAliens());
}

/**
 * Gets the total quantity of containment-space in this Base.
 * @return, total containment space
 */
int Base::getTotalContainment() const
{
	int total (0);
	for (std::vector<BaseFacility*>::const_iterator
			i = _facilities.begin();
			i != _facilities.end();
			++i)
	{
		if ((*i)->buildFinished() == true)
			total += (*i)->getRules()->getAliens();
	}
	return total;
}

/**
 * Gets alien containment-space not in use.
 * @return, free space
 */
int Base::getFreeContainment() const
{
	return getTotalContainment() - getUsedContainment();
}

/**
 * Checks if this Base has alien-containment.
 * @return, true if containment exists
 */
bool Base::hasContainment() const
{
	for (std::vector<BaseFacility*>::const_iterator
			i = _facilities.begin();
			i != _facilities.end();
			++i)
	{
		if ((*i)->buildFinished() == true
			&& (*i)->getRules()->getAliens() != 0)
		{
			return true;
		}
	}
	return false;
}

/**
 * Gets the quantity of aLiens currently under interrogation.
 * @return, aliens ...
 */
int Base::getInterrogatedAliens() const
{
	int total (0);
	const RuleResearch* resRule;
	for (std::vector<ResearchProject*>::const_iterator
			i = _projectsResearch.begin();
			i != _projectsResearch.end();
			++i)
	{
		resRule = (*i)->getRules();
		if (resRule->needsItem() == true
			&& _rules->getUnitRule(resRule->getType()) != nullptr)
		{
			++total;
		}
	}
	return total;
}

/**
 * Gets the quantity of hangars used by Craft at this Base.
 * @return, used hangars incl/ Transfers & Manufacture
 */
int Base::getUsedHangars() const
{
	int
		total (0),
		subTotal;

	const RuleManufacture* mfRule;
	for (std::vector<ManufactureProject*>::const_iterator	// Craft production requires 1 hangar + 1 hanger for
			i = _projectsManufacture.begin();		// every input-craft over (1 if there's an output-craft).
			i != _projectsManufacture.end();
			++i)
	{
		subTotal = 0;

		mfRule = (*i)->getRules();
		for (std::map<std::string, int>::const_iterator
				j = mfRule->getPartsRequired().begin();
				j != mfRule->getPartsRequired().end();
				++j)
		{
			if (_rules->getItemRule(j->first) == nullptr && _rules->getCraft(j->first) != nullptr)
				subTotal += j->second;

			if (subTotal == 0 && (*i)->getRules()->isCraftProduced() == true)
				++subTotal;

			total += subTotal;
		}
	}

	total += static_cast<int>(_crafts.size());

	for (std::vector<Transfer*>::const_iterator
			i = _transfers.begin();
			i != _transfers.end();
			++i)
	{
		if ((*i)->getTransferType() == PST_CRAFT)
			total += (*i)->getQuantity();
	}

	return total;
}

/**
 * Gets the total quantity of hangars in this Base.
 * @return, total hangars
 */
int Base::getTotalHangars() const
{
	int total (0);
	for (std::vector<BaseFacility*>::const_iterator
			i = _facilities.begin();
			i != _facilities.end();
			++i)
	{
		if ((*i)->buildFinished() == true)
			total += (*i)->getRules()->getCrafts();
	}
	return total;
}

/**
 * Gets hangar-space not in use.
 * @return, free space
 */
int Base::getFreeHangars() const
{
	return getTotalHangars() - getUsedHangars();
}

/**
 * Gets the total quantity of Soldiers of a specified type occupying or being
 * transfered to this Base.
 * @param type - reference to the soldier-type
 * @return, quantity of soldiers of @a type
 */
int Base::getSoldierCount(const std::string& type) const
{
	int total (0);
	for (std::vector<Transfer*>::const_iterator
			i = _transfers.begin();
			i != _transfers.end();
			++i)
	{
		if ((*i)->getTransferType() == PST_SOLDIER
			&& (*i)->getSoldier()->getRules()->getType() == type)
		{
			++total;
		}
	}

	for (std::vector<Soldier*>::const_iterator
			i = _soldiers.begin();
			i != _soldiers.end();
			++i)
	{
		if ((*i)->getRules()->getType() == type)
			++total;
	}

	return total;
}

/**
 * Gets the total quantity of Craft of a specified type stored at or being
 * transfered to this Base.
 * @param type			- reference to the craft-type
 * @param exclTransfers	- true to count only craft in hangars (default false)
 * @return, quantity of craft of @a type
 */
int Base::getCraftCount(
		const std::string& type,
		bool exclTransfers) const
{
	int total (0);

	if (exclTransfers == false)
	{
		for (std::vector<Transfer*>::const_iterator
				i = _transfers.begin();
				i != _transfers.end();
				++i)
		{
			if ((*i)->getTransferType() == PST_CRAFT
				&& (*i)->getCraft()->getRules()->getType() == type)
			{
				total += (*i)->getQuantity();
			}
		}
	}

	for (std::vector<Craft*>::const_iterator
			i = _crafts.begin();
			i != _crafts.end();
			++i)
	{
		if ((*i)->getRules()->getType() == type)
			++total;
	}

	return total;
}

/**
 * Adds a specified Manufacture project to this Base.
 * @param project - pointer to a project
 */
void Base::addManufactureProject(ManufactureProject* const project)
{
	_projectsManufacture.push_back(project);
}

/**
 * Clears a specified Manufacture project from this Base.
 * @param project - pointer to the project
 */
void Base::clearManufactureProject(const ManufactureProject* const project)
{
	_engineers += project->getAssignedEngineers();

	for (std::vector<ManufactureProject*>::const_iterator
			i = _projectsManufacture.begin();
			i != _projectsManufacture.end();
			++i)
	{
		if (*i == project)
		{
			delete *i;
			_projectsManufacture.erase(i);
			return;
		}
	}
}

/**
 * Gets the list of this Base's Manufacture projects.
 * @return, reference to the list of projects
 */
const std::vector<ManufactureProject*>& Base::getManufacture() const
{
	return _projectsManufacture;
}

/**
 * Gets the list of this Base's ResearchProjects.
 * @return, reference to the list of projects
 */
const std::vector<ResearchProject*>& Base::getResearch() const
{
	return _projectsResearch;
}

/**
 * Adds a fresh ResearchProject to this Base.
 * @param project - pointer to a project to add
 */
void Base::addResearchProject(ResearchProject* const project)
{
	_projectsResearch.push_back(project);
}

/**
 * Clears a ResearchProject from this Base.
 * @note Live Alien research never goes offline; they are returned to Containment
 * and the project is cancelled.
 * @param project	- pointer to the project to clear
 * @param grantHelp	- true to apply researchHelp() (default false)
 * @param goOffline	- true to hide project but not remove it from the vector (default false)
 */
void Base::clearResearchProject(
		ResearchProject* const project,
		bool grantHelp,
		bool goOffline)
{
	_scientists += project->getAssignedScientists();

	std::vector<ResearchProject*>::const_iterator i (std::find(
															_projectsResearch.begin(),
															_projectsResearch.end(),
															project));
	if (i != _projectsResearch.end())
	{
		if (goOffline == true)
		{
			project->setAssignedScientists(0);
			project->setOffline(); // NOTE: Does *not* return a/the neededItem to base-stores.
		}
		else
		{
			if (grantHelp == true)
				researchHelp(project->getRules()->getType());

			delete *i;
			_projectsResearch.erase(i);
		}
	}
}

/**
 * Research Help ala XcomUtil.
 * @param aLien - reference to the type of an aLien that got prodded
 */
void Base::researchHelp(const std::string& aLien) // private.
{
	std::string resType;
	double coef;

	for (std::vector<ResearchProject*>::const_iterator
			i = _projectsResearch.begin();
			i != _projectsResearch.end();
			++i)
	{
		if ((*i)->getOffline() == false)
		{
			resType = (*i)->getRules()->getType();

			if		(aLien.find("_SOLDIER")		!= std::string::npos)	coef = getSoldierHelp(resType);
			else if	(aLien.find("_NAVIGATOR")	!= std::string::npos)	coef = getNavigatorHelp(resType);
			else if	(aLien.find("_MEDIC")		!= std::string::npos)	coef = getMedicHelp(resType);
			else if	(aLien.find("_ENGINEER")	!= std::string::npos)	coef = getEngineerHelp(resType);
			else if	(aLien.find("_LEADER")		!= std::string::npos)	coef = getLeaderHelp(resType);
			else if	(aLien.find("_COMMANDER")	!= std::string::npos)	coef = getCommanderHelp(resType);
			else														coef = 0.;

			if (AreSame(coef, 0.) == false)
			{
				const int cost ((*i)->getCost());
				const double spent (static_cast<double>((*i)->getSpent()));

				coef = RNG::generate(0., coef);
				(*i)->setSpent(static_cast<int>(Round(
							spent + ((static_cast<double>(cost) - spent) * coef))));

				if ((*i)->getSpent() > cost - 1)
					(*i)->setSpent(cost - 1);

				return;
			}
		}
	}
}

/**
 * Gets soldier coefficient for Research Help.
 * @return, help coef
 */
double Base::getSoldierHelp(const std::string& resType) // static.
{
	if (   resType.compare("STR_ALIEN_GRENADE") == 0
		|| resType.compare("STR_ALIEN_ENTERTAINMENT") == 0
		|| resType.compare("STR_PERSONAL_ARMOR") == 0)
	{
		return 0.5;
	}

	if (   resType.compare("STR_HEAVY_PLASMA_CLIP") == 0
		|| resType.compare("STR_PLASMA_RIFLE_CLIP") == 0
		|| resType.compare("STR_PLASMA_PISTOL_CLIP") == 0)
	{
		return 0.4;
	}

	if (resType.compare("STR_POWER_SUIT") == 0)
		return 0.25;

	if (resType.compare("STR_ALIEN_ORIGINS") == 0)
		return 0.2;

	if (   resType.compare("STR_THE_MARTIAN_SOLUTION") == 0
		|| resType.compare("STR_HEAVY_PLASMA") == 0
		|| resType.compare("STR_PLASMA_RIFLE") == 0
		|| resType.compare("STR_PLASMA_PISTOL") == 0)
	{
		return 0.1;
	}

	return 0.;
}

/**
 * Gets navigator coefficient for Research Help.
 * @return, help coef
 */
double Base::getNavigatorHelp(const std::string& resType) // static.
{
	if (   resType.compare("STR_HYPER_WAVE_DECODER") == 0
		|| resType.compare("STR_UFO_NAVIGATION") == 0)
	{
		return 0.8;
	}

	if (   resType.compare("STR_MOTION_SCANNER") == 0
		|| resType.compare("STR_ALIEN_ENTERTAINMENT") == 0)
	{
		return 0.5;
	}

	if (   resType.compare("STR_GRAV_SHIELD") == 0
		|| resType.compare("STR_ALIEN_ALLOYS") == 0)
	{
		return 0.4;
	}

	if (resType.compare("STR_ALIEN_ORIGINS") == 0)
		return 0.35;

	if (resType.compare("STR_FLYING_SUIT") == 0)
		return 0.3;

	if (   resType.compare("STR_UFO_POWER_SOURCE") == 0
		|| resType.compare("STR_UFO_CONSTRUCTION") == 0
		|| resType.compare("STR_THE_MARTIAN_SOLUTION") == 0)
	{
		return 0.25;
	}

	if (   resType == "STR_HEAVY_PLASMA"
		|| resType == "STR_HEAVY_PLASMA_CLIP"
		|| resType == "STR_PLASMA_RIFLE"
		|| resType == "STR_PLASMA_RIFLE_CLIP"
		|| resType == "STR_PLASMA_PISTOL"
		|| resType == "STR_PLASMA_PISTOL_CLIP"
		|| resType == "STR_NEW_FIGHTER_CRAFT" // + "STR_IMPROVED_INTERCEPTOR" <- uses Alien Alloys.
		|| resType == "STR_NEW_FIGHTER_TRANSPORTER"
		|| resType == "STR_ULTIMATE_CRAFT"
		|| resType == "STR_PLASMA_CANNON"
		|| resType == "STR_FUSION_MISSILE") // what about _Defense
//		|| resType == "hovertank-plasma" // <-
//		|| resType == "hovertank-fusion" // <-
	{
		return 0.2;
	}

	if (   resType.compare("STR_CYDONIA_OR_BUST") == 0
		|| resType.compare("STR_POWER_SUIT") == 0)
	{
		return 0.15;
	}

	if (   resType.compare("STR_ELERIUM_EXTRACTION") == 0
		|| resType.compare("STR_115_REACTOR") == 0)
	{
		return 0.1;
	}

	return 0.;
}

/**
 * Gets medic coefficient for Research Help.
 * @return, help coef
 */
double Base::getMedicHelp(const std::string& resType) // static.
{
	if (   resType.compare("STR_ALIEN_FOOD") == 0
		|| resType.compare("STR_ALIEN_SURGERY") == 0
		|| resType.compare("STR_EXAMINATION_ROOM") == 0
		|| resType.compare("STR_ALIEN_REPRODUCTION") == 0)
	{
		return 0.8;
	}

	if (   resType.compare("STR_PSI_AMP") == 0
		|| resType.compare("STR_SMALL_LAUNCHER") == 0
		|| resType.compare("STR_STUN_BOMB") == 0
		|| resType.compare("STR_MIND_PROBE") == 0
		|| resType.compare("STR_MIND_SHIELD") == 0
		|| resType.compare("STR_PSI_LAB") == 0)
	{
		return 0.6;
	}

	if (resType.compare(resType.length() - 7, 7, "_CORPSE") == 0)
		return 0.5;

	if (resType == "STR_MEDI_KIT")
		return 0.4;

	if (   resType.compare("STR_ALIEN_ORIGINS") == 0
		|| resType.compare("STR_ALIEN_ENTERTAINMENT") == 0)
	{
		return 0.2;
	}

	if (resType.compare("STR_THE_MARTIAN_SOLUTION") == 0)
		return 0.1;

	return 0.;
}

/**
 * Gets engineer coefficient for Research Help.
 * @return, help coef
 */
double Base::getEngineerHelp(const std::string& resType) // static.
{
	if (   resType.compare("STR_BLASTER_LAUNCHER") == 0
		|| resType.compare("STR_BLASTER_BOMB") == 0
		|| resType.compare("STR_ELERIUM_EXTRACTION") == 0
		|| resType.compare("STR_115_REACTOR") == 0)
	{
		return 0.7;
	}

	if (   resType.compare("STR_MOTION_SCANNER") == 0
		|| resType.compare("STR_HEAVY_PLASMA") == 0
		|| resType.compare("STR_HEAVY_PLASMA_CLIP") == 0
		|| resType.compare("STR_PLASMA_RIFLE") == 0
		|| resType.compare("STR_PLASMA_RIFLE_CLIP") == 0
		|| resType.compare("STR_PLASMA_PISTOL") == 0
		|| resType.compare("STR_PLASMA_PISTOL_CLIP") == 0
		|| resType.compare("STR_ALIEN_GRENADE") == 0
		|| resType.compare("STR_ELERIUM_115") == 0
		|| resType.compare("STR_UFO_POWER_SOURCE") == 0
		|| resType.compare("STR_UFO_CONSTRUCTION") == 0
		|| resType.compare("STR_ALIEN_ALLOYS") == 0
		|| resType.compare("STR_PLASMA_CANNON") == 0
		|| resType.compare("STR_FUSION_MISSILE") == 0
		|| resType.compare("STR_PLASMA_DEFENSE") == 0
		|| resType.compare("STR_FUSION_DEFENSE") == 0
		|| resType.compare("STR_GRAV_SHIELD") == 0
		|| resType.compare("STR_PERSONAL_ARMOR") == 0
		|| resType.compare("STR_POWER_SUIT") == 0
		|| resType.compare("STR_FLYING_SUIT") == 0)
	{
		return 0.5;
	}

	if (   resType.compare("STR_NEW_FIGHTER_CRAFT") == 0 // + "STR_IMPROVED_INTERCEPTOR" <- uses Alien Alloys.
		|| resType.compare("STR_NEW_FIGHTER_TRANSPORTER") == 0
		|| resType.compare("STR_ULTIMATE_CRAFT") == 0)
	{
		return 0.3;
	}

	if (   resType.compare("STR_ALIEN_ORIGINS") == 0
		|| resType.compare("STR_SMALL_LAUNCHER") == 0
		|| resType.compare("STR_STUN_BOMB") == 0)
	{
		return 0.2;
	}

	if (resType.compare("STR_THE_MARTIAN_SOLUTION") == 0)
		return 0.1;

	return 0.;
}

/**
 * Gets leader coefficient for Research Help.
 * @return, help coef
 */
double Base::getLeaderHelp(const std::string& resType) // static.
{
	if (resType.compare("STR_EXAMINATION_ROOM") == 0)
		return 0.8;

	if (resType.compare("STR_BLASTER_LAUNCHER") == 0)
		return 0.6;

	if (resType.compare("STR_ALIEN_ORIGINS") == 0)
		return 0.5;

	if (resType.compare("STR_THE_MARTIAN_SOLUTION") == 0)
		return 0.3;

	if (resType.compare("STR_PSI_AMP") == 0)
		return 0.25;

	if (   resType.compare("STR_HEAVY_PLASMA") == 0
		|| resType.compare("STR_HEAVY_PLASMA_CLIP") == 0
		|| resType.compare("STR_PLASMA_RIFLE") == 0
		|| resType.compare("STR_PLASMA_RIFLE_CLIP") == 0
		|| resType.compare("STR_PLASMA_PISTOL") == 0
		|| resType.compare("STR_PLASMA_PISTOL_CLIP") == 0
		|| resType.compare("STR_BLASTER_BOMB") == 0
		|| resType.compare("STR_SMALL_LAUNCHER") == 0
		|| resType.compare("STR_STUN_BOMB") == 0
		|| resType.compare("STR_ELERIUM_115") == 0
		|| resType.compare("STR_ALIEN_ALLOYS") == 0
		|| resType.compare("STR_PLASMA_CANNON") == 0
		|| resType.compare("STR_FUSION_MISSILE") == 0
		|| resType.compare("STR_CYDONIA_OR_BUST") == 0
		|| resType.compare("STR_PERSONAL_ARMOR") == 0
		|| resType.compare("STR_POWER_SUIT") == 0
		|| resType.compare("STR_FLYING_SUIT") == 0
		|| resType.compare("STR_ELERIUM_EXTRACTION") == 0
		|| resType.compare("STR_115_REACTOR") == 0)
	{
		return 0.2;
	}

	if (   resType.compare("STR_NEW_FIGHTER_CRAFT") == 0 // + "STR_IMPROVED_INTERCEPTOR" <- uses Alien Alloys.
		|| resType.compare("STR_NEW_FIGHTER_TRANSPORTER") == 0
		|| resType.compare("STR_ULTIMATE_CRAFT") == 0)
	{
		return 0.1;
	}

	return 0.;
}

/**
 * Gets commander coefficient for Research Help.
 * @return, help coef
 */
double Base::getCommanderHelp(const std::string& resType) // static.
{
	if (   resType.compare("STR_BLASTER_LAUNCHER") == 0
		|| resType.compare("STR_EXAMINATION_ROOM") == 0)
	{
		return 0.8;
	}

	if (resType.compare("STR_ALIEN_ORIGINS") == 0)
		return 0.7;

	if (resType.compare("STR_THE_MARTIAN_SOLUTION") == 0)
		return 0.6;

	if (   resType.compare("STR_PSI_AMP") == 0
		|| resType.compare("STR_CYDONIA_OR_BUST") == 0)
	{
		return 0.5;
	}

	if (   resType.compare("STR_ELERIUM_EXTRACTION") == 0
		|| resType.compare("STR_115_REACTOR") == 0)
	{
		return 0.3;
	}

	if (   resType.compare("STR_BLASTER_BOMB") == 0
		|| resType.compare("STR_ELERIUM_115") == 0
		|| resType.compare("STR_ALIEN_ALLOYS") == 0
		|| resType.compare("STR_PERSONAL_ARMOR") == 0
		|| resType.compare("STR_POWER_SUIT") == 0
		|| resType.compare("STR_FLYING_SUIT") == 0)
	{
		return 0.25;
	}

	if (   resType.compare("STR_HEAVY_PLASMA") == 0
		|| resType.compare("STR_HEAVY_PLASMA_CLIP") == 0
		|| resType.compare("STR_PLASMA_RIFLE") == 0
		|| resType.compare("STR_PLASMA_RIFLE_CLIP") == 0
		|| resType.compare("STR_PLASMA_PISTOL") == 0
		|| resType.compare("STR_PLASMA_PISTOL_CLIP") == 0
		|| resType.compare("STR_SMALL_LAUNCHER") == 0
		|| resType.compare("STR_STUN_BOMB") == 0
		|| resType.compare("STR_NEW_FIGHTER_CRAFT") == 0 // + "STR_IMPROVED_INTERCEPTOR" <- uses Alien Alloys.
		|| resType.compare("STR_NEW_FIGHTER_TRANSPORTER") == 0
		|| resType.compare("STR_ULTIMATE_CRAFT") == 0
		|| resType.compare("STR_PLASMA_CANNON") == 0
		|| resType.compare("STR_FUSION_MISSILE") == 0)
	{
		return 0.2;
	}

	return 0.;
}

/**
 * Sets this Base's battlescape-status.
 * @param tactical - true if in the battlescape (default true)
 */
void Base::setTactical(bool tactical)
{
	_tactical = tactical;
}

/**
 * Checks this Base's battlescape-status.
 * @return, true if Base is the battlescape
 */
bool Base::getTactical() const
{
	return _tactical;
}

/**
 * Sets if this Base is a valid alien-retaliation target.
 * @param exposed - true if eligible for retaliation (default true)
 */
void Base::setBaseExposed(bool exposed)
{
	_exposed = exposed;
}

/**
 * Checks if this Base is a valid alien-retaliation target.
 * @return, true if eligible for retaliation
 */
bool Base::getBaseExposed() const
{
	return _exposed;
}

/**
 * Flags this Base as placed and in operation.
 */
void Base::placeBase()
{
	_placed = true;
}

/**
 * Checks if this Base has been placed and is in operation.
 * @return, true if placed
 */
bool Base::isBasePlaced() const
{
	return _placed;
}

/**
 * Checks if this Base is equipped with hyper-wave detection facilities.
 * @return, true if hyper-wave detection
 */
bool Base::getHyperDetection() const
{
	for (std::vector<BaseFacility*>::const_iterator
			i = _facilities.begin();
			i != _facilities.end();
			++i)
	{
		if ((*i)->buildFinished() == true
			&& (*i)->getRules()->isHyperwave() == true)
		{
			return true;
		}
	}
	return false;
}

/**
 * Gets the total quantity of short range detection facilities in this Base.
 * @return, quantity of shortrange detection facilities
 *
int Base::getShortRangeDetection() const
{
	int
		total = 0,
		range = 0;
	int minRadarRange = _rules->getMinRadarRange();
	if (minRadarRange == 0)
		return 0;

	for (std::vector<BaseFacility*>::const_iterator
			i = _facilities.begin();
			i != _facilities.end();
			++i)
	{
		if ((*i)->buildFinished() == true
			&& (*i)->getRules()->getRadarRange() > 0)
		{
			range = (*i)->getRules()->getRadarRange();
			// kL_note: that should be based off a string or Ruleset value.

			if ((*i)->getRules()->getRadarRange() <= minRadarRange)
//			if (range < 1501) // was changed to 1701
			{
				total++;
			}
		}
	}
	return total;
} */

/**
 * Gets the total value of short range detection facilities at this Base.
 * @note Used for BaseInfoState bar.
 * @return, shortrange detection value as percent
 */
int Base::getShortRangeTotal() const
{
	int
		total (0),
		range;
	for (std::vector<BaseFacility*>::const_iterator
			i = _facilities.begin();
			i != _facilities.end();
			++i)
	{
		if ((*i)->buildFinished() == true
			&& (range = (*i)->getRules()->getRadarRange()) != 0
			&& range <= _rules->getRadarRangeCutoff()
			&& (total += (*i)->getRules()->getRadarChance()) >= 100)
		{
			return 100;
		}
	}
	return total;
}

/**
 * Gets the total quantity of long range detection facilities in this Base.
 * @return, quantity of longrange detection facilities
 *
int Base::getLongRangeDetection() const
{
	int total = 0;
	int minRadarRange = _rules->getMinRadarRange();
	for (std::vector<BaseFacility*>::const_iterator
			i = _facilities.begin();
			i != _facilities.end();
			++i)
	{
		if ((*i)->getRules()->getRadarRange() > minRadarRange
			&& (*i)->buildFinished() == true)
//			&& (*i)->getRules()->getRadarRange() > 1500) // was changed to 1700
		{
			total++;
		}
	}
	return total;
} */

/**
 * Gets the total value of long range detection facilities at this Base.
 * @note Used for BaseInfoState bar.
 * @return, longrange detection value as percent
 */
int Base::getLongRangeTotal() const
{
	int total (0);
	for (std::vector<BaseFacility*>::const_iterator
			i = _facilities.begin();
			i != _facilities.end();
			++i)
	{
		if ((*i)->buildFinished() == true
			&& (*i)->getRules()->getRadarRange() > _rules->getRadarRangeCutoff()
			&& (total += (*i)->getRules()->getRadarChance()) >= 100)
		{
			return 100;
		}
	}
	return total;
}

/**
 * Gets if a specified Target is detected inside this Base's radar-range.
 * @param target - pointer to a UFO to attempt detection against
 * @return,	0 undetected
 *			1 hyperdetected only
 *			2 detected
 *			3 detected & hyperdetected
 */
int Base::detect(const Target* const target) const
{
	int ret (0);
	double dist (insideRadarRange(target));
	if (AreSame(dist, 0.) == false) // lets hope UFO is not *right on top of Base* Lol
	{
		if (dist < 0.)
		{
			++ret;
			dist = -dist;
		}

		int pct (0);
		for (std::vector<BaseFacility*>::const_iterator
				i = _facilities.begin();
				i != _facilities.end();
				++i)
		{
			if ((*i)->buildFinished() == true
				&& dist <= static_cast<double>((*i)->getRules()->getRadarRange()))
			{
				pct += (*i)->getRules()->getRadarChance();
			}
		}

		const Ufo* const ufo (dynamic_cast<const Ufo*>(target));
		if (ufo != nullptr)
		{
			pct += ufo->getVisibility();
			pct = static_cast<int>(Round(static_cast<double>(pct) / 3.)); // per 10 minutes
			if (RNG::percent(pct) == true)
				ret += 2;
		}
	}
	return ret;
}

/**
 * Gets if a specified Target is inside this Base's radar-range.
 * @param target - pointer to UFO
 * @return, great-circle distance to UFO (negative if hyperdetected)
 */
double Base::insideRadarRange(const Target* const target) const
{
	double ret (0.); // lets hope UFO is not *right on top of Base* Lol
	const double dist (getDistance(target) * radius_earth);
	if (dist <= static_cast<double>(_rules->getRadarRangeBest()))
	{
		bool hyperDet (false);
		for (std::vector<BaseFacility*>::const_iterator
				i = _facilities.begin();
				i != _facilities.end();
				++i)
		{
			if ((*i)->buildFinished() == true
				&& dist <= static_cast<double>((*i)->getRules()->getRadarRange()))
			{
				ret = dist; // identical value for every *i; looking only for hyperDet after 1st successful iteration ->
				if ((*i)->getRules()->isHyperwave() == true)
				{
					hyperDet = true;
					break;
				}
			}
		}
		if (hyperDet == true) ret = -ret; // <- use negative value to pass (hyperdetection= true)
	}
	return ret;
}

/**
 ** FUNCTOR ***
 * Functor to check for mind shield capability.
 *
struct isMindShield
	:
		public std::unary_function<BaseFacility*, bool>
{
	/// Check isMindShield() for @a facility.
	bool operator()(const BaseFacility* facility) const;
}; */
/**
 * Only fully operational facilities are checked.
 * @param facility Pointer to the facility to check.
 * @return, If @a facility can act as a mind shield.
 *
bool isMindShield::operator()(const BaseFacility* facility) const
{
	if (facility->buildFinished() == false)
		return false; // Still building this

	return facility->getRules()->isMindShield();
} */

/**
 ** FUNCTOR ***
 * Functor to check for completed facilities.
 *
struct isCompleted
	:
		public std::unary_function<BaseFacility*, bool>
{
	/// Check isCompleted() for @a facility.
	bool operator()(const BaseFacility* facility) const;
}; */
/**
 * Facilities are checked for construction completion.
 * @param, facility Pointer to the facility to check.
 * @return, If @a facility has completed construction.
 *
bool isCompleted::operator()(const BaseFacility* facility) const
{
	return facility->buildFinished() == true;
} */

/**
 * Calculates the chance for aLiens to detect this base.
 * @note Big bases without mindshields are easier to detect.
 * @param diff		- the game's difficulty setting
 * @param facQty	- pointer to the quantity of facilities (default nullptr)
 * @param shields	- pointer to the quantity of mind-shield facilities (default nullptr)
 * @return, detection chance
 */
int Base::getExposedChance(
		int diff,
		int* facQty,
		int* shields) const
{
	int facSize;

	if (facQty != nullptr)
	{
		*facQty  =
		*shields = 0;
		for (std::vector<BaseFacility*>::const_iterator	// NOTE: Used by BaseDetectionState.
				i = _facilities.begin();
				i != _facilities.end();
				++i)
		{
			facSize = static_cast<int>((*i)->getRules()->getSize());
			facSize *= facSize;

			if ((*i)->buildFinished() == false)
				facSize *= 2;
			else if ((*i)->getRules()->isMindShield() == true)
				++(*shields);

			*facQty += facSize;
		}
		return exposedChance(diff, *facQty, *shields);
	}

	int
		facQty0  (0),
		shields0 (0);
	for (std::vector<BaseFacility*>::const_iterator		// NOTE: Used by GeoscapeState.
			i = _facilities.begin();
			i != _facilities.end();
			++i)
	{
		facSize = static_cast<int>((*i)->getRules()->getSize());
		facSize *= facSize;

		if ((*i)->buildFinished() == false)
			facSize *= 2;
		else if ((*i)->getRules()->isMindShield() == true)
			++shields0;

		facQty0 += facSize;
	}
	return exposedChance(diff, facQty0, shields0);
}

/**
 * Calculates the chance that aLiens have to detect this Base.
 * @note Helper for getExposedChance() to ensure consistency.
 * @param diff		- the game's difficulty setting
 * @param facQty	- the quantity of facilities
 * @param shields	- the quantity of shield facilities
 */
int Base::exposedChance( // private/static.
		int diff,
		int facQty,
		int shields)
{
	return (facQty / 6 + 9) / (shields * 2 + 1) + diff;
}

/**
 * Gets the number of grav-shields at this base.
 * @return, total grav-shields
 */
size_t Base::getGravShields() const
{
	size_t total (0u);
	for (std::vector<BaseFacility*>::const_iterator
			i = _facilities.begin();
			i != _facilities.end();
			++i)
	{
		if ((*i)->buildFinished() == true
			&& (*i)->getRules()->isGravShield() == true)
		{
			++total;
		}
	}
	return total;
}

/**
 * Gets the total defense-value of all the facilities at this Base.
 * @note Used for BaseInfoState bar.
 * @return, defense value
 */
int Base::getDefenseTotal() const
{
	int total (0);
	for (std::vector<BaseFacility*>::const_iterator
			i = _facilities.begin();
			i != _facilities.end();
			++i)
	{
		if ((*i)->buildFinished() == true)
			total += (*i)->getRules()->getDefenseValue();
	}
	return total;
}

/**
 * Sets up this Base's defenses against a UFO attack.
 * @return, true if there are defenses to defend with
 */
bool Base::setupBaseDefense()
{
	_defenses.clear(); // safety. should be clear already
	for (std::vector<BaseFacility*>::const_iterator
			i = _facilities.begin();
			i != _facilities.end();
			++i)
	{
		if ((*i)->buildFinished() == true
			&& (*i)->getRules()->getDefenseValue() != 0)
		{
			_defenses.push_back(*i);
		}
	}
	return (_defenses.empty() == false);
}

/**
 * Clears the base-defenses vector.
 */
void Base::clearBaseDefense()
{
	_defenses.clear();
}

/**
 * Sets a reduction to the quantity of aLiens deployed in a base-defense tactical.
 * @param result - the defense reduction (default 0)
 */
void Base::setDefenseReduction(int reduction)
{
	_defenseReduction = reduction;
}

/**
 * Gets a reduction to the quantity of aLiens deployed in a base-defense tactical.
 * @return, the defense reduction
 */
int Base::getDefenseReduction() const
{
	return _defenseReduction;
}

/**
 * Gets the Facilities for this Base's defense.
 * @return, pointer to a vector of pointers to BaseFacility
 */
std::vector<BaseFacility*>* Base::getDefenses()
{
	return &_defenses;
}

/**
 * Destroys all disconnected facilities in this Base.
 */
void Base::destroyDisconnectedFacilities()
{
	std::list<std::vector<BaseFacility*>::const_iterator> discoFacs (getDisconnectedFacilities());
	for (std::list<std::vector<BaseFacility*>::const_iterator>::const_reverse_iterator
			rit = discoFacs.rbegin();
			rit != discoFacs.rend();
			++rit)
	{
		destroyFacility(*rit);
	}
}

/**
 * Gets a sorted list of the facilities(=iterators) NOT connected to the Access Lift.
 * @param ignoreFac - a BaseFacility to ignore in case of intentional dismantling (default nullptr)
 * @return, a sorted list of iterators pointing to elements in '_facilities'
 */
std::list<std::vector<BaseFacility*>::const_iterator> Base::getDisconnectedFacilities(const BaseFacility* const ignoreFac)
{
	std::list<std::vector<BaseFacility*>::const_iterator> ret;

	if (ignoreFac != nullptr
		&& ignoreFac->getRules()->isLift() == true) // Theoretically this is impossible, but sanity check is good :)
	{
		for (std::vector<BaseFacility*>::const_iterator
				i = _facilities.begin();
				i != _facilities.end();
				++i)
		{
			if (*i != ignoreFac)
				ret.push_back(i);
		}
		return ret;
	}


	std::pair<std::vector<BaseFacility*>::const_iterator, bool>* facBool_coord[BASE_SIZE][BASE_SIZE];
	for (size_t
			x = 0u;
			x != BASE_SIZE;
			++x)
	{
		for (size_t
				y = 0u;
				y != BASE_SIZE;
				++y)
		{
			facBool_coord[x][y] = nullptr;
		}
	}

	const BaseFacility* lift (nullptr);

	std::vector<std::pair<std::vector<BaseFacility*>::const_iterator, bool>*> facConnections;
	for (std::vector<BaseFacility*>::const_iterator // fill up the facBool_coord (+facConnections), and search for the Lift
			i = _facilities.begin();
			i != _facilities.end();
			++i)
	{
		if (*i != ignoreFac)
		{
			if ((*i)->getRules()->isLift() == true)
				lift = *i;

			for (size_t
					x = 0u;
					x != (*i)->getRules()->getSize();
					++x)
			{
				for (size_t
						y = 0u;
						y != (*i)->getRules()->getSize();
						++y)
				{
					std::pair<std::vector<BaseFacility*>::const_iterator, bool>* pairsOfFacBool =
									new std::pair<std::vector<BaseFacility*>::const_iterator, bool>(i, false);
					facConnections.push_back(pairsOfFacBool);
					facBool_coord[static_cast<size_t>((*i)->getX()) + x]
								 [static_cast<size_t>((*i)->getY()) + y] = pairsOfFacBool; // loli
				}
			}
		}
	}

	if (lift == nullptr)
		return ret; // TODO: something clever.


	// make the recursion manually using a stack
	const BaseFacility
		* fac,
		* borLeft,
		* borRight,
		* borTop,
		* borBottom;
	size_t
		x,y;
	std::stack<std::pair<size_t, size_t>> stuff;

	stuff.push(std::make_pair(
						static_cast<size_t>(lift->getX()),
						static_cast<size_t>(lift->getY())));
	while (stuff.empty() == false)
	{
		x = stuff.top().first,
		y = stuff.top().second;

		stuff.pop();

		if (//   x > -1			// -> hopefully x&y will never point outside the baseGrid
			//&& x < BASE_SIZE	// ... looks atm like it does!! It does. FIX inc!!!!
			//&& y > -1
			//&& y < BASE_SIZE &&
			   facBool_coord[x][y] != nullptr
			&& facBool_coord[x][y]->second == false)
		{
			facBool_coord[x][y]->second = true;

			fac = *(facBool_coord[x][y]->first);

			if (x > 0u
				&& facBool_coord[x - 1u][y] != nullptr)
			{
				borLeft = *(facBool_coord[x - 1u][y]->first);
			}
			else
				borLeft = nullptr;

			if (x + 1u < BASE_SIZE
				&& facBool_coord[x + 1u][y] != nullptr)
			{
				borRight = *(facBool_coord[x + 1u][y]->first);
			}
			else
				borRight = nullptr;

			if (y > 0u
				&& facBool_coord[x][y - 1u] != nullptr)
			{
				borTop = *(facBool_coord[x][y - 1u]->first);
			}
			else
				borTop = nullptr;

			if (y + 1u < BASE_SIZE
				&& facBool_coord[x][y + 1u] != nullptr)
			{
				borBottom = *(facBool_coord[x][y + 1u]->first);
			}
			else
				borBottom = nullptr;


			if (x > 0u
				&& (fac->buildFinished() == true
					|| (borLeft != nullptr
						&& (borLeft == fac
							|| borLeft->getBuildTime() > borLeft->getRules()->getBuildTime()))))
			{
				stuff.push(std::make_pair(x - 1u, y));
			}

			if (x < BASE_SIZE - 1u
				&& (fac->buildFinished() == true
					|| (borRight != nullptr
						&& (borRight == fac
							|| borRight->getBuildTime() > borRight->getRules()->getBuildTime()))))
			{
				stuff.push(std::make_pair(x + 1u, y));
			}

			if (y > 0u
				&& (fac->buildFinished() == true
					|| (borTop != nullptr
						&& (borTop == fac
							|| borTop->getBuildTime() > borTop->getRules()->getBuildTime()))))
			{
				stuff.push(std::make_pair(x, y - 1u));
			}

			if (y < BASE_SIZE - 1u
				&& (fac->buildFinished() == true
					|| (borBottom != nullptr
						&& (borBottom == fac
							|| borBottom->getBuildTime() > borBottom->getRules()->getBuildTime()))))
			{
				stuff.push(std::make_pair(x, y + 1u));
			}
		}
	}

	const BaseFacility* el_pre (nullptr);
	for (std::vector<std::pair<std::vector<BaseFacility*>::const_iterator, bool>*>::const_iterator
			i = facConnections.begin();
			i != facConnections.end();
			++i)
	{
		if (*((*i)->first) != el_pre	// not a connected fac -> push its iterator onto the list!
			&& (*i)->second == false)	// And don't take duplicates of large-sized facilities.
		{
			ret.push_back((*i)->first);
		}

		el_pre = *((*i)->first);
		delete *i;
	}

	return ret;
}

/**
 * Removes a base-module and deals with ramifications.
 * @param pFac - an iterator reference to the facility that's destroyed
 * @return, const_iterator to the BaseFacility* that was occupied by @a pFac
 */
std::vector<BaseFacility*>::const_iterator Base::destroyFacility(std::vector<BaseFacility*>::const_iterator pFac)
{
	// TODO: Stop Manufacture projects that rely on a quantity of BaseFacilities (eg, Elerium Extraction).
	if ((*pFac)->getRules()->getCrafts() != 0)	// TODO: Handle hangars that can hold more than one Craft. Or
	{											// decree that a hangar can hold only one Craft in RuleBaseFacility.
		// Destroy Craft or production of Craft since there will no longer be a hangar for it.
		if ((*pFac)->getCraft() != nullptr)
		{
			if ((*pFac)->getCraft()->getQtySoldiers() != 0)
			{
				for (std::vector<Soldier*>::const_iterator
						i = _soldiers.begin();
						i != _soldiers.end();
						++i)
				{
					if ((*i)->getCraft() == (*pFac)->getCraft()) // remove Soldiers
						(*i)->setCraft();
				}
			}

			const std::map<std::string, int>* const craftContents ((*pFac)->getCraft()->getCraftItems()->getContents());
			for (std::map<std::string, int>::const_iterator
					i = craftContents->begin();
					i != craftContents->end();
					++i)
			{
				_items->addItem(i->first, i->second); // transfer Craft-items to Base
			}

			for (std::vector<Craft*>::const_iterator
					i = _crafts.begin();
					i != _crafts.end();
					++i)
			{
				if (*i == (*pFac)->getCraft())
				{
					delete *i;
					_crafts.erase(i);
					break;
				}
			}
		}
		else if ((*pFac)->getRules()->getCrafts() - getFreeHangars() > 0)
		{
			bool checkTransfers (true);

			for (std::vector<ManufactureProject*>::const_reverse_iterator // check ManufactureProject
					rit = _projectsManufacture.rbegin();
					rit != _projectsManufacture.rend();
					++rit)
			{
				if ((*rit)->getRules()->isCraftProduced() == true)
				{
					checkTransfers = false;
					_engineers += (*rit)->getAssignedEngineers();

					delete *rit;
					_projectsManufacture.erase((++rit).base());
					break;
				}
			}

			if (checkTransfers == true) // check Transfers
			{
				for (std::vector<Transfer*>::const_reverse_iterator
						rit = _transfers.rbegin();
						rit != _transfers.rend();
						++rit)
				{
					if ((*rit)->getTransferType() == PST_CRAFT)
					{
						delete (*rit)->getCraft();
						delete *rit;
						_transfers.erase((++rit).base());
						break;
					}
				}
			}
		}
	}


	int
		del,
		personnel,
		destroyed;

	if ((destroyed = (*pFac)->getRules()->getPsiLaboratories()) != 0)
	{
		del = destroyed - getFreePsiLabs();
		for (std::vector<Soldier*>::const_iterator
				i = _soldiers.begin();
				i != _soldiers.end() && del > 0;
				++i)
		{
			if ((*i)->inPsiTraining() == true)
			{
				(*i)->togglePsiTraining();
				--del;
			}
		}
	}

	if ((destroyed = (*pFac)->getRules()->getLaboratories()) != 0)
	{
		if (getTotalLaboratories() - destroyed == 0)
		{
			for (std::vector<ResearchProject*>::const_iterator
					i = _projectsResearch.begin();
					i != _projectsResearch.end();
					++i)
			{
				_scientists += (*i)->getAssignedScientists();
				delete *i;
			}
			_projectsResearch.clear();
		}
		else
		{
			del = destroyed - getFreeLaboratories();
			for (std::vector<ResearchProject*>::const_iterator // TODO: Reverse iteration.
					i = _projectsResearch.begin();
					i != _projectsResearch.end() && del > 0;
					++i)
			{
				personnel = (*i)->getAssignedScientists();
				if (personnel < del)
				{
					del -= personnel;
					(*i)->setAssignedScientists(0);
					_scientists += personnel;
				}
				else
				{
					(*i)->setAssignedScientists(personnel - del);
					_scientists += del;
					break;
				}
			}
		}
	}

	if ((destroyed = (*pFac)->getRules()->getWorkshops()) != 0)
	{
		if (getTotalWorkshops() - destroyed == 0)
		{
			for (std::vector<ManufactureProject*>::const_iterator
					i = _projectsManufacture.begin();
					i != _projectsManufacture.end();
					++i)
			{
				_engineers += (*i)->getAssignedEngineers();
				delete *i;
			}
			_projectsManufacture.clear();
		}
		else
		{
			del = destroyed - getFreeWorkshops();
			for (std::vector<ManufactureProject*>::const_iterator // TODO: Reverse iteration.
					i = _projectsManufacture.begin();
					i != _projectsManufacture.end() && del > 0;
					++i)
			{
				personnel = (*i)->getAssignedEngineers();
				if (personnel < del)
				{
					del -= personnel;
					(*i)->setAssignedEngineers(0);
					_engineers += personnel;
				}
				else
				{
					(*i)->setAssignedEngineers(personnel - del);
					_engineers += del;
					break;
				}
			}
		}
		// TODO: Start removing '_projectsManufacture' if their space-required still
		// exceeds space-available.
	}

/*	// Let the Transfer-items arrive and then start issuing the Warnings. That
	// is let DebriefingState::btnOkClick() handle it.
	destroyed = (*pFac)->getRules()->getStorage();
	if (destroyed != 0)
	{
		if (storesOverfull(static_cast<double>(destroyed)) == true)
		{
			double del_d = static_cast<double>(destroyed - getTotalStores()) + getUsedStores();

			for (std::vector<Transfer*>::const_reverse_iterator
					i = _transfers.rbegin();
					i != _transfers.rend();
					)
			{
				switch ((*i)->getTransferType())
				{
					case PST_ITEM:
						int qty = (*i)->getQuantity();
						delete *i;
						i = _transfers.erase(i);
						break;
					default:
						++i;
				}
			}
		}
	} */

	if ((destroyed = (*pFac)->getRules()->getPersonnel()) != 0)
	{
		// Could get cramped in here; current personnel are not removed.
		// TODO: Issue a stream of warnings ala storesOverfull.
		del = destroyed - getFreeQuarters();
		for (std::vector<Transfer*>::const_reverse_iterator
				rit = _transfers.rbegin();
				rit != _transfers.rend() && del > 0;
				)
		{
			switch ((*rit)->getTransferType())
			{
				case PST_SOLDIER:
				{
					--del;
					delete (*rit)->getSoldier();
					delete *rit;
					const std::vector<Transfer*>::const_iterator i (_transfers.erase((++rit).base()));
					rit = std::vector<Transfer*>::const_reverse_iterator(i);
					break;
				}

				case PST_SCIENTIST:
				case PST_ENGINEER:
				{
					del -= (*rit)->getQuantity();
					delete *rit;
					const std::vector<Transfer*>::const_iterator i (_transfers.erase((++rit).base()));
					rit = std::vector<Transfer*>::const_reverse_iterator(i);
					break;
				}

				default:
					++rit;
			}
		}
	}

	delete *pFac;
	return _facilities.erase(pFac);
}

/**
 * Gets the total quantity of monthly costs for maintaining the Craft in this
 * Base.
 * @note Used for monthly maintenance expenditure.
 * @return, maintenance costs
 */
int Base::getCraftMaintenance() const
{
	int total (0);
	for (std::vector<Transfer*>::const_iterator
			i = _transfers.begin();
			i != _transfers.end();
			++i)
	{
		if ((*i)->getTransferType() == PST_CRAFT)
			total += (*i)->getQuantity() * (*i)->getCraft()->getRules()->getRentCost();
	}

	for (std::vector<Craft*>::const_iterator
			i = _crafts.begin();
			i != _crafts.end();
			++i)
	{
		total += (*i)->getRules()->getRentCost();
	}
	return total;
}

/**
 * Gets the total quantity of monthly costs for maintaining the personnel in
 * this Base.
 * @note Used for monthly maintenance expenditure.
 * @return, maintenance costs
 */
int Base::getPersonnelMaintenance() const
{
	int total (0);
	for (std::vector<Transfer*>::const_iterator
			i = _transfers.begin();
			i != _transfers.end();
			++i)
	{
		if ((*i)->getTransferType() == PST_SOLDIER)
			total += (*i)->getSoldier()->getRules()->getSalaryCost();
	}

	for (std::vector<Soldier*>::const_iterator
			i = _soldiers.begin();
			i != _soldiers.end();
			++i)
	{
		total += (*i)->getRules()->getSalaryCost();
	}

	total += getTotalEngineers() * _rules->getEngineerCost();
	total += getTotalScientists() * _rules->getScientistCost();
	total += getOperationalExpenses();

	return total;
}

/**
 * Gets the total quantity of monthly costs for maintaining the facilities in
 * this Base.
 * @note Used for monthly maintenance expenditure.
 * @return, maintenance costs
 */
int Base::getFacilityMaintenance() const
{
	int total (0);
	for (std::vector<BaseFacility*>::const_iterator
			i = _facilities.begin();
			i != _facilities.end();
			++i)
	{
		if ((*i)->buildFinished() == true)
			total += (*i)->getRules()->getMonthlyCost();
	}

	const float factor (static_cast<float>(_playSave->getDifficultyInt()) * 0.1f); // +10% per diff.
	return static_cast<int>(static_cast<float>(total) * factor);
}

/**
 * Gets the total quantity of all the maintenance monthly costs in this Base.
 * @note Used for monthly maintenance expenditure.
 * @return, maintenance costs
 */
int Base::getMonthlyMaintenace() const
{
	return getCraftMaintenance() + getPersonnelMaintenance() + getFacilityMaintenance();
}

/**
 * Adds @a cash to this Base's cash-income value.
 * @param cash - quantity of income
 */
void Base::addCashIncome(int cash)
{
	_cashIncome += cash;
}

/**
 * Gets this Base's current cash-income value.
 * @return, current income value
 */
int Base::getCashIncome() const
{
	return _cashIncome;
}

/**
 * Zeros this Base's income-value.
 * @note Called by SavedGame::balanceBudget() for each new month.
 */
void Base::zeroCashIncome()
{
	_cashIncome = 0;
}

/**
 * Adds @a cash to this Base's cash-spent value.
 * @param cash - quantity of expenditure
 */
void Base::addCashSpent(int cash)
{
	_cashSpent += cash;
}

/**
 * Gets this Base's current cash-spent value.
 * @return, current cash spent value
 */
int Base::getCashSpent() const
{
	return _cashSpent;
}

/**
 * Zeros this Base's expenditure-value.
 * @note Called by SavedGame::balanceBudget() for each new month.
 */
void Base::zeroCashSpent()
{
	_cashSpent = 0;
}

/**
 * Sets various recalls for this Base.
 * @param recallType	- recall type (Base.h)
 * @param row			- row
 */
void Base::setRecallRow(
		RecallType recallType,
		size_t row)
{
	switch (recallType)
	{
		case RCL_SOLDIER:	_recallSoldier	= row; break;
		case RCL_TRANSFER:	_recallTransfer	= row; break;
		case RCL_PURCHASE:	_recallPurchase	= row; break;
		case RCL_SELL:		_recallSell		= row;
	}
}

/**
 * Gets various recalls for this Base.
 * @param recallType - recall type (Base.h)
 * @return, row
 */
size_t Base::getRecallRow(RecallType recallType) const
{
	switch (recallType)
	{
		case RCL_SOLDIER:	return _recallSoldier;
		case RCL_TRANSFER:	return _recallTransfer;
		case RCL_PURCHASE:	return _recallPurchase;
		case RCL_SELL:		return _recallSell;
	}
	return 0;
}

/**
 * Calculates the salaries or tactical-cost for Soldiers, and adds the expense
 * for Vehicles if tactical.
 * @note If 'craft' is specified this returns the cost for a tactical battle; if
 * 'craft' is nullptr it returns this Base's monthly costs only for Soldiers'
 * salaries.
 * @param craft - pointer to a Craft (default nullptr)
 * @return, total cost
 */
int Base::getOperationalExpenses(const Craft* const craft) const
{
	int total (0);
	for (std::vector<Soldier*>::const_iterator
			i = _soldiers.begin();
			i != _soldiers.end();
			++i)
	{
		total += (*i)->getSoldierExpense(craft != nullptr);
	}

	if (craft != nullptr)
		total += craft->getQtyVehicles(true) * 750;

	return total;
}

/**
 * Gets a Soldier's bonus pay for going on a tactical mission; subtracts
 * the value from current funds.
 * @param sol	- pointer to a Soldier
 * @param dead	- true if soldier dies while on tactical (default false)
 * @return, the expense
 */
int Base::expenseSoldier(
		const Soldier* const sol,
		bool dead)
{
	int cost (sol->getSoldierExpense());
	if (dead == true) cost /= 2;

	_cashSpent += cost;
	_rules->getGame()->getSavedGame()->setFunds(_rules->getGame()->getSavedGame()->getFunds() - static_cast<int64_t>(cost));

	return cost;
}
/*	switch (sol->getRank())
	{
		case RANK_ROOKIE:
		break;
		case RANK_SQUADDIE:
		break;
		case RANK_SERGEANT:
		break;
		case RANK_CAPTAIN:
		break;
		case RANK_COLONEL:
		break;
		case RANK_COMMANDER:
		break;
		default:
	} */

/**
 * Gets the expense of sending HWPs/doggies on a tactical mission;
 * subtracts the value from current funds.
 * @param quadrants	- size of the HWP/doggie in tiles
 * @param dead		- true if HWP got destroyed while on tactical (default false)
 * @return, the expense
 */
int Base::expenseSupport(
		const int quadrants,
		bool dead)
{
	int cost (quadrants * 750);
	if (dead == true) cost /= 2;

	_cashSpent += cost;
	_rules->getGame()->getSavedGame()->setFunds(_rules->getGame()->getSavedGame()->getFunds() - static_cast<int64_t>(cost));

	return cost;
}

/**
 * Gets the expense of sending a transport craft on a tactical mission;
 * subtracts the value from current funds.
 * @param craft - pointer to a Craft
 * @return, the expense
 */
int Base::expenseCraft(const Craft* const craft)
{
	const int cost (craft->getRules()->getSoldierCapacity() * 1000);
	_cashSpent += cost;
	_rules->getGame()->getSavedGame()->setFunds(_rules->getGame()->getSavedGame()->getFunds() - static_cast<int64_t>(cost));

	return cost;
}

/**
 * Sorts the soldiers according to a pre-determined algorithm.
 */
void Base::sortSoldiers()
{
	std::multimap<int, Soldier*> soldiersOrdered;
	const UnitStats* stats;
	int weight;

	for (std::vector<Soldier*>::const_iterator
			i = _soldiers.begin();
			i != _soldiers.end();
			++i)
	{
		stats = (*i)->getCurrentStats();
		weight = stats->tu			*  8 // old values from CWXCED
			   + stats->stamina		*  5
			   + stats->health		*  7
			   + stats->bravery		*  3
			   + stats->reactions	* 16
			   + stats->firing		* 19
			   + stats->throwing	*  1
			   + stats->strength	* 23
			   + stats->melee		*  6;
		// also: rank, missions, kills

		if (stats->psiSkill != 0 // don't include Psi unless revealed.
			&& stats->psiStrength > 45)
		{
			weight += stats->psiStrength * 27
					+ stats->psiSkill	 * 99;
		}

		soldiersOrdered.insert(std::pair<int, Soldier*>(weight, *i));
		// NOTE: unsure if multimap loses a player-preferred
		// order of two soldiers with the same weight (to preserve that
		// would have to use vector of key-weights, stable_sort'd,
		// referenced back to a vector of <weight,Soldier> pairs,
		// possibly using a comparoperator functor. /cheers)
	}

	size_t j (0u);
	for (std::multimap<int, Soldier*>::const_iterator
			i = soldiersOrdered.begin();
			i != soldiersOrdered.end();
			++i, ++j)
	{
		_soldiers.at(j) = (*i).second;
	}
}

/**
 * Calculates the penalty-score for losing this Base.
 * @return, penalty for losing the base
 */
int Base::calcLostScore() const
{
	int ret (0);
	for (std::vector<BaseFacility*>::const_iterator
			i = _facilities.begin();
			i != _facilities.end();
			++i)
	{
		if ((*i)->buildFinished() == true)
			++ret;
	}
	ret *= _rules->getBaseLostScore() * (_playSave->getDifficultyInt() + 1);

	if (ret < 0) ret = 0; // safety for DebriefingState.

	return ret;
}

/**
 * Checks if any Craft at this Base needs to and can be refurbished.
 * @note Called by DebriefingState, ItemsArrivingState, ManufactureProject.
 * @param itType - reference to an item-type
 */
void Base::refurbishCraft(const std::string& itType)
{
	std::vector<Craft*>::const_iterator last (_crafts.end());
	for (std::vector<Craft*>::const_iterator
			i = _crafts.begin();
			i != last;
			++i)
	{
		if ((*i)->getWarned() == true)
		{
			switch ((*i)->getCraftStatus())
			{
				case CS_REFUELLING:
					if ((*i)->getRules()->getRefuelItem() == itType)
						(*i)->setWarned(false);
					break;

				case CS_REARMING:
					if ((*i)->getRules()->getWeaponCapacity() != 0u) // safety. is Rearming, ergo has weaponry.
					{
						for (std::vector<CraftWeapon*>::const_iterator
								j = (*i)->getCraftWeapons()->begin();
								j != (*i)->getCraftWeapons()->end();
								++j)
						{
							if (*j != nullptr && (*j)->getRules()->getClipType() == itType)
							{
								(*j)->setCantLoad(false);
								(*i)->setWarned(false);
							}
						}
					}
			}
		}
	}
}

/**
 * Checks if this Base is a quick-battle base for a base-defense tactical.
 * @return, true if quick-battle base-defense
 */
bool Base::isQuickDefense() const
{
	return _isQuickDefense;
}

/**
 * Sets the Base as a quick-battle base for a base-defense tactical.
 * @param - quickDefense - true if quick-battle base-defense (default true)
 */
void Base::setQuickDefense(bool quickDefense)
{
	_isQuickDefense = quickDefense;
}

}
