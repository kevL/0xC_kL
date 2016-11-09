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

#include "Manufacture.h"

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
 * Tracks a Base manufacturing project.
 * @param mfRule - pointer to RuleManufacture
 */
Manufacture::Manufacture(const RuleManufacture* const mfRule)
	:
		_mfRule(mfRule),
		_units(1),
		_engineers(0),
		_timeSpent(0),
		_sell(false),
		_infinite(false)
{}

/**
 * dTor.
 */
Manufacture::~Manufacture()
{}

/**
 * Loads from YAML.
 * @param node - reference a YAML node
 */
void Manufacture::load(const YAML::Node& node)
{
	_engineers	= node["engineers"]	.as<int>(_engineers);
	_timeSpent	= node["timeSpent"]	.as<int>(_timeSpent);
	_sell		= node["sell"]		.as<bool>(_sell);
	_infinite	= node["infinite"]	.as<bool>(_infinite);
	_units		= node["units"]		.as<int>(_units);
}

/**
 * Saves to YAML.
 * @return, a YAML node
 */
YAML::Node Manufacture::save() const
{
	YAML::Node node;

	node["item"] = _mfRule->getType();

	if (_engineers != 0)	node["engineers"]	= _engineers;
	if (_timeSpent != 0)	node["timeSpent"]	= _timeSpent;
	if (_sell != false)		node["sell"]		= _sell;
	if (_infinite != false)	node["infinite"]	= _infinite;
	else					node["units"]		= _units;

	return node;
}

/**
 * Gets the rules for this Manufacture.
 * @return, pointer to RuleManufacture
 */
const RuleManufacture* Manufacture::getRules() const
{
	return _mfRule;
}

/**
 * Gets the total quantity to produce.
 * @return, total quantity
 */
int Manufacture::getProductionTotal() const
{
	return _units;
}

/**
 * Sets the total quantity to produce.
 * @param quantity - total quantity
 */
void Manufacture::setProductionTotal(int quantity)
{
	_units = quantity;
}

/**
 * Gets if this Manufacture is to produce an infinite quantity.
 * @return, true if infinite
 */
bool Manufacture::getInfinite() const
{
	return _infinite;
}

/**
 * Sets if this Manufacture is to produce an infinite quantity.
 * @param infinite - true if infinite
 */
void Manufacture::setInfinite(bool infinite)
{
	_infinite = infinite;
}

/**
 * Gets the quantity of assigned engineers to this Manufacture.
 * @return, quantity of engineers
 */
int Manufacture::getAssignedEngineers() const
{
	return _engineers;
}

/**
 * Sets the quantity of assigned engineers to this Manufacture.
 * @param engineers - quantity of engineers
 */
void Manufacture::setAssignedEngineers(int engineers)
{
	_engineers = engineers;
}

/**
 * Gets if the produced items are to be sold immediately.
 * @return, true if sell
 */
bool Manufacture::getAutoSales() const
{
	return _sell;
}

/**
 * Sets if the produced items are to be sold immediately.
 * @param sell - true if sell
 */
void Manufacture::setAutoSales(bool sell)
{
	_sell = sell;
}

/**
 * Checks if there is enough funds to continue production.
 * @return, true if funds available
 */
bool Manufacture::enoughMoney(const SavedGame* const gameSave) const // private.
{
	return (gameSave->getFunds() >= _mfRule->getManufactureCost());
}

/**
 * Checks if there is enough resource material to continue production.
 * @return, true if materials are available
 */
bool Manufacture::enoughMaterials( // private.
		Base* const base,
		const Ruleset* const rules) const
{
	for (std::map<std::string, int>::const_iterator
			i = _mfRule->getRequiredItems().begin();
			i != _mfRule->getRequiredItems().end();
			++i)
	{
		if ((rules->getItemRule(i->first) != nullptr
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
 * Gets the quantity of produced items so far.
 * @return, quantity produced
 */
int Manufacture::getProducedQuantity() const
{
	return _timeSpent / _mfRule->getManufactureTime();
}

/**
 * Advances this Manufacture by a step.
 * @param base		- pointer to a Base
 * @param gameSave	- pointer to the SavedGame
 */
ProductionProgress Manufacture::step(
		Base* const base,
		SavedGame* const gameSave)
{
	const Ruleset* const rules (gameSave->getRules());

	const int qtyDone_pre (getProducedQuantity());
	_timeSpent += _engineers;

	const int qtyDone_total (getProducedQuantity());
	if (qtyDone_pre < qtyDone_total)
	{
		int producedQty;
		if (_infinite == false) // don't overproduce units
			producedQty = std::min(qtyDone_total,
								  _units) - qtyDone_pre;
		else
			producedQty = qtyDone_total - qtyDone_pre;

		do
		{
			for (std::map<std::string, int>::const_iterator
					i = _mfRule->getProducedItems().begin();
					i != _mfRule->getProducedItems().end();
					++i)
			{
				if (_mfRule->isCraft() == true)
				{
					Craft* const craft (new Craft(
												rules->getCraft(i->first),
												base,
												gameSave,
												gameSave->getCanonicalId(i->first)));
					craft->setCraftStatus(CS_REFUELLING);
					base->getCrafts()->push_back(craft);
					break; // <- Craft Manufacture produces 1 craft period.
				}
				// NOTE: Craft cannot be set for auto-sell.

				if (_sell == true) // sales takes precedence over refurbish.
				{
					const int profit (rules->getItemRule(i->first)->getSellCost() * i->second);
					gameSave->setFunds(gameSave->getFunds() + profit);
					base->addCashIncome(profit);
				}
				else
				{
					base->getStorageItems()->addItem(i->first, i->second);
					base->refurbishCraft(i->first);
				}
			}

			if (--producedQty != 0) // check to ensure there's enough money/materials to produce multiple-items (if applicable) *in this step*
			{
				if (enoughMoney(gameSave) == false)
					return PROGRESS_NOT_ENOUGH_MONEY;

				if (enoughMaterials(base, rules) == false)
					return PROGRESS_NOT_ENOUGH_MATERIALS;

				startProduction(base, gameSave); // remove resources for the next of multiple-items *in this step*
			}
		}
		while (producedQty != 0);
	}

	if (_infinite == false && qtyDone_total >= _units)
		return PROGRESS_COMPLETE;

	if (qtyDone_pre < qtyDone_total)
	{
		if (enoughMoney(gameSave) == false)
			return PROGRESS_NOT_ENOUGH_MONEY;

		if (enoughMaterials(base, rules) == false)
			return PROGRESS_NOT_ENOUGH_MATERIALS;

		startProduction(base, gameSave); // start the next iteration
	}

	return PROGRESS_NOT_COMPLETE;
}

/**
 * Starts this Manufacture.
 * @param base		- pointer to a Base
 * @param gameSave	- pointer to the SavedGame
 */
void Manufacture::startProduction(
		Base* const base,
		SavedGame* const gameSave) const
{
	const int cost (_mfRule->getManufactureCost());
	gameSave->setFunds(gameSave->getFunds() - cost);
	base->addCashSpent(cost);

	const Ruleset* const rules (gameSave->getRules());

	for (std::map<std::string, int>::const_iterator
			i = _mfRule->getRequiredItems().begin();
			i != _mfRule->getRequiredItems().end();
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
 * Gets the time till this Manufacture is completed.
 * param days	- reference to store days remaining
 * param hours	- reference to store hours remaining
 * @return, true if work is progressing
 */
bool Manufacture::tillFinish(
		int& days,
		int& hours) const
{
	if (_engineers != 0)
	{
		int qty;
		if (_infinite == true) //|| _sell == true
			qty = getProducedQuantity() + 1;
		else
			qty = getProductionTotal();

		hours = qty * _mfRule->getManufactureTime() - _timeSpent;
		hours = (hours + _engineers - 1) / _engineers;

		days = hours / 24;
		hours %= 24;

		return true;
	}
	return false;
}

/**
 * Gets the time spent on this Manufacture so far.
 * @return, time spent
 *
int Manufacture::getTimeSpent() const
{
	return _timeSpent;
} */
/**
 * Sets the time spent on this Manufacture so far.
 * @param spent - time spent
 *
void Manufacture::setTimeSpent(int spent)
{
	_timeSpent = spent;
} */

}
