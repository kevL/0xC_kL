/*
 * Copyright 2010-2019 OpenXcom Developers.
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

#include "ManufactureProject.h"

//#include <algorithm>
//#include <limits>

#include "Base.h"
#include "BaseFacility.h"
#include "Craft.h"
#include "CraftWeapon.h"
#include "ItemContainer.h"
#include "SavedGame.h"

//#include "../Ruleset/RuleCraft.h"
#include "../Ruleset/RuleCraftWeapon.h"
#include "../Ruleset/RuleItem.h"
#include "../Ruleset/RuleManufacture.h"
#include "../Ruleset/Ruleset.h"


namespace OpenXcom
{

/**
 * Tracks a Manufacture project.
 * @param mfRule - pointer to RuleManufacture
 */
ManufactureProject::ManufactureProject(const RuleManufacture* const mfRule)
	:
		_mfRule(mfRule),
		_qtyTotal(1),
		_engineers(0),
		_hoursSpent(0),
		_sell(false),
		_infinite(false)
{}

/**
 * dTor.
 */
ManufactureProject::~ManufactureProject()
{}

/**
 * Loads from YAML.
 * @param node - reference a YAML node
 */
void ManufactureProject::load(const YAML::Node& node)
{
	_engineers	= node["engineers"]	.as<int>(_engineers);
	_hoursSpent	= node["hoursSpent"].as<int>(_hoursSpent);
	_sell		= node["sell"]		.as<bool>(_sell);
	_infinite	= node["infinite"]	.as<bool>(_infinite);
	_qtyTotal	= node["units"]		.as<int>(_qtyTotal);
}

/**
 * Saves to YAML.
 * @return, a YAML node
 */
YAML::Node ManufactureProject::save() const
{
	YAML::Node node;

	node["item"] = _mfRule->getType();

	if (_engineers != 0)	node["engineers"]	= _engineers;
	if (_hoursSpent != 0)	node["hoursSpent"]	= _hoursSpent;
	if (_sell != false)		node["sell"]		= _sell;
	if (_infinite != false)	node["infinite"]	= _infinite;
	else					node["units"]		= _qtyTotal;

	return node;
}

/**
 * Gets the rules for this Manufacture.
 * @return, pointer to RuleManufacture
 */
const RuleManufacture* ManufactureProject::getRules() const
{
	return _mfRule;
}

/**
 * Sets the total quantity to produce.
 * @param qty - total quantity
 */
void ManufactureProject::setManufactureTotal(int qty)
{
	_qtyTotal = qty;
}

/**
 * Gets the total quantity to produce.
 * @return, total quantity
 */
int ManufactureProject::getManufactureTotal() const
{
	return _qtyTotal;
}

/**
 * Sets if this Manufacture is to produce an infinite quantity.
 * @param infinite - true if infinite (default true)
 */
void ManufactureProject::setInfinite(bool infinite)
{
	_infinite = infinite;
}

/**
 * Gets if this Manufacture is to produce an infinite quantity.
 * @return, true if infinite
 */
bool ManufactureProject::getInfinite() const
{
	return _infinite;
}

/**
 * Sets the quantity of assigned engineers to this Manufacture.
 * @param engineers - quantity of engineers
 */
void ManufactureProject::setAssignedEngineers(int engineers)
{
	_engineers = engineers;
}

/**
 * Gets the quantity of assigned engineers to this Manufacture.
 * @return, quantity of engineers
 */
int ManufactureProject::getAssignedEngineers() const
{
	return _engineers;
}

/**
 * Sets if the produced parts are to be sold immediately.
 * @param sell - true if sell
 */
void ManufactureProject::setAutoSales(bool sell)
{
	_sell = sell;
}

/**
 * Gets if the produced parts are to be sold immediately.
 * @return, true if sell
 */
bool ManufactureProject::getAutoSales() const
{
	return _sell;
}

/**
 * Checks if there is enough funds to start/continue the project.
 * @return, true if funds are available
 */
bool ManufactureProject::hasEnoughMoney(const SavedGame* const playSave) const // private.
{
	return (playSave->getFunds() >= _mfRule->getManufactureCost());
}

/**
 * Checks if there is enough material to start/continue the project.
 * @return, true if materials are available
 */
bool ManufactureProject::hasEnoughMaterials( // private.
		Base* const base,
		const Ruleset* const rules) const
{
	for (std::map<std::string, int>::const_iterator
			i = _mfRule->getPartsRequired().begin();
			i != _mfRule->getPartsRequired().end();
			++i)
	{
		if (   (rules->getItemRule(i->first) != nullptr
				&& base->getStorageItems()->getItemQuantity(i->first) < i->second)
			|| (rules->getCraft(i->first) != nullptr
				&& base->getCraftCount(i->first) < i->second))
		{
			return false;
		}
	}
	return true;
}

/**
 * Gets the quantity of iterations completed so far.
 * @return, iterations completed
 */
int ManufactureProject::getQuantityManufactured() const
{
	return _hoursSpent / _mfRule->getManufactureHours();
}

/**
 * Starts this Manufacture.
 * @note Necessary checks are done in ManufactureStartState.
 * @param base		- pointer to a Base
 * @param playSave	- pointer to the SavedGame
 */
void ManufactureProject::startManufacture(
		Base* const base,
		SavedGame* const playSave) const
{
	const int cost (_mfRule->getManufactureCost());
	playSave->setFunds(playSave->getFunds() - cost);
	base->addCashSpent(cost);

	const Ruleset* const rules (playSave->getRules());

	for (std::map<std::string, int>::const_iterator
			i = _mfRule->getPartsRequired().begin();
			i != _mfRule->getPartsRequired().end();
			++i)
	{
		if (rules->getItemRule(i->first) != nullptr)
			base->getStorageItems()->removeItem(i->first, i->second);
		else if (rules->getCraft(i->first) != nullptr) // TODO: First send a warning that a/the Craft will be unloaded and deleted.
		{
			for (std::vector<Craft*>::const_iterator
					j = base->getCrafts()->begin();
					j != base->getCrafts()->end();
					++j)
			{
				if ((*j)->getRules()->getType() == i->first)
				{
					(*j)->unloadCraft(rules, false);

					delete *j;
					base->getCrafts()->erase(j);
					break;
				}
			}
		}
	}
}

/**
 * Advances this Manufacture by a step.
 * @param base		- pointer to a Base
 * @param playSave	- pointer to the SavedGame
 */
ManufactureProgress ManufactureProject::stepManufacture(
		Base* const base,
		SavedGame* const playSave)
{
	const Ruleset* const rules (playSave->getRules());

	const int qtyDone_prestep (getQuantityManufactured());
	_hoursSpent += _engineers;

	const int qtyDone_current (getQuantityManufactured());
	if (qtyDone_prestep < qtyDone_current)
	{
		int producedQty;
		if (_infinite == false) // don't overproduce units
			producedQty = std::min(qtyDone_current,
								  _qtyTotal) - qtyDone_prestep;
		else
			producedQty = qtyDone_current - qtyDone_prestep;

		do
		{
			for (std::map<std::string, int>::const_iterator
					i = _mfRule->getPartsProduced().begin();
					i != _mfRule->getPartsProduced().end();
					++i)
			{
				if (_sell == true) // sales takes precedence over refurbish.
				{
					int profit;
					if (_mfRule->isCraftProduced() == true)
						profit = rules->getCraft(i->first)->getSellCost();
					else
						profit = rules->getItemRule(i->first)->getSellCost() * i->second;

					playSave->setFunds(playSave->getFunds() + profit);
					base->addCashIncome(profit);
				}
				else
				{
					if (_mfRule->isCraftProduced() == true)
					{
						Craft* const craft (new Craft(
													rules->getCraft(i->first),
													base,
													playSave));
						base->getCrafts()->push_back(craft);
						craft->checkup();
						break; // <- Craft Manufacture produces 1 craft period.
					}
					else
					{
						base->getStorageItems()->addItem(i->first, i->second);
						base->refurbishCraft(i->first);
					}
				}
			}

			if (--producedQty != 0) // check to ensure there's enough money/materials to produce multiple-iterations (if applicable) *in this iteration*
			{
				if (hasEnoughMoney(playSave) == false)
					return PROG_NOT_ENOUGH_MONEY;

				if (hasEnoughMaterials(base, rules) == false)
					return PROG_NOT_ENOUGH_MATERIALS;

				startManufacture(base, playSave); // remove resources for the next of multiple-iterations *in this iteration*
			}
		}
		while (producedQty != 0);
	}

	if (_infinite == false && qtyDone_current >= _qtyTotal)
		return PROG_COMPLETE;

	if (qtyDone_prestep < qtyDone_current)
	{
		if (hasEnoughMoney(playSave) == false)
			return PROG_NOT_ENOUGH_MONEY;

		if (hasEnoughMaterials(base, rules) == false)
			return PROG_NOT_ENOUGH_MATERIALS;

		startManufacture(base, playSave); // start the next iteration
	}

	return PROG_NOT_COMPLETE;
}

/**
 * Calculates the duration till this Manufacture is completed.
 * param days	- reference in which to store days remaining
 * param hours	- reference in which to store hours remaining
 * @return, true if work is progressing
 */
bool ManufactureProject::tillFinish(
		int& days,
		int& hours) const
{
	if (_engineers != 0)
	{
		int qty;
		if (_infinite == true) //|| _sell == true
			qty = getQuantityManufactured() + 1;
		else
			qty = _qtyTotal;

		hours = qty * _mfRule->getManufactureHours() - _hoursSpent;
		hours = (hours + _engineers - 1) / _engineers;

		days = hours / 24;
		hours %= 24;

		return true;
	}
	return false;
}

/**
 * Sets the hours spent on this Manufacture so far.
 * @param spent - hours spent
 *
void ManufactureProject::setTimeSpent(int spent)
{
	_hoursSpent = spent;
} */
/**
 * Gets the hours spent on this Manufacture so far.
 * @return, hours spent
 *
int ManufactureProject::getTimeSpent() const
{
	return _hoursSpent;
} */

}
