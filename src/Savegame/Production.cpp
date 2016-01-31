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

#include "Production.h"

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
 * @param manfRule	- pointer to RuleManufacture
 * @param amount	- quantity to produce
 */
Production::Production(
		const RuleManufacture* const manfRule,
		int amount)
	:
		_manfRule(manfRule),
		_amount(amount),
		_infinite(false),
		_timeSpent(0),
		_engineers(0),
		_sell(false)
{}

/**
 * dTor.
 */
Production::~Production()
{}

/**
 * Loads from YAML.
 * @param node - reference a YAML node
 */
void Production::load(const YAML::Node& node)
{
	_engineers	= node["assigned"]	.as<int>(_engineers);
	_timeSpent	= node["spent"]		.as<int>(_timeSpent);
	_amount		= node["amount"]	.as<int>(_amount);
	_infinite	= node["infinite"]	.as<bool>(_infinite);
	_sell		= node["sell"]		.as<bool>(_sell);
}

/**
 * Saves to YAML.
 * @return, a YAML node
 */
YAML::Node Production::save() const
{
	YAML::Node node;

	node["item"]	= _manfRule->getType();
	node["spent"]	= _timeSpent;
	node["amount"]	= _amount;

	if (_engineers != 0)	node["assigned"]	= _engineers;
	if (_infinite == true)	node["infinite"]	= _infinite;
	if (_sell == true)		node["sell"]		= _sell;

	return node;
}

/**
 * Gets the rules for this Production.
 * @return, pointer to RuleManufacture
 */
const RuleManufacture* Production::getRules() const
{
	return _manfRule;
}

/**
 * Gets the total quantity to produce.
 * @return, total quantity
 */
int Production::getAmountTotal() const
{
	return _amount;
}

/**
 * Sets the total quantity to produce.
 * @param amount - total quantity
 */
void Production::setAmountTotal(int amount)
{
	_amount = amount;
}

/**
 * Gets the quantity of produced items so far.
 * @return, quantity produced
 */
int Production::getAmountProduced() const
{
	return _timeSpent / _manfRule->getManufactureTime();
}

/**
 * Gets if this Production is to produce an infinite quantity.
 * @return, true if infinite
 */
bool Production::getInfiniteAmount() const
{
	return _infinite;
}

/**
 * Sets if this Production is to produce an infinite quantity.
 * @param infinite - true if infinite
 */
void Production::setInfiniteAmount(bool infinite)
{
	_infinite = infinite;
}

/**
 * Gets the time spent on this Production so far.
 * @return, time spent
 */
int Production::getTimeSpent() const
{
	return _timeSpent;
}

/**
 * Sets the time spent on this Production so far.
 * @param spent - time spent
 */
void Production::setTimeSpent(int spent)
{
	_timeSpent = spent;
}

/**
 * Gets the quantity of assigned engineers to this Production.
 * @return, quantity of engineers
 */
int Production::getAssignedEngineers() const
{
	return _engineers;
}

/**
 * Sets the quantity of assigned engineers to this Production.
 * @param engineers - quantity of engineers
 */
void Production::setAssignedEngineers(int engineers)
{
	_engineers = engineers;
}

/**
 * Gets if the produced items are to be sold immediately.
 * @return, true if sell
 */
bool Production::getSellItems() const
{
	return _sell;
}

/**
 * Sets if the produced items are to be sold immediately.
 * @param sell - true if sell
 */
void Production::setSellItems(bool sell)
{
	_sell = sell;
}

/**
 * Checks if there is enough funds to continue production.
 * @return, true if funds available
 */
bool Production::enoughMoney(const SavedGame* const gameSave) const // private.
{
	return (gameSave->getFunds() >= _manfRule->getManufactureCost());
}

/**
 * Checks if there is enough resource material to continue production.
 * @return, true if materials available
 */
bool Production::enoughMaterials( // private.
		Base* const base,
		const Ruleset* const rules) const
{
	for (std::map<std::string, int>::const_iterator
			i = _manfRule->getRequiredItems().begin();
			i != _manfRule->getRequiredItems().end();
			++i)
	{
		if ((rules->getItem(i->first) != nullptr
				&& base->getStorageItems()->getItemQuantity(i->first) >= i->second)
			|| (rules->getCraft(i->first) != nullptr
				&& base->getCraftCount(i->first) >= i->second))
		{
			return true;
		}
	}
	return false;
}

/**
 * Advances this Production by a step.
 * @param base		- pointer to a Base
 * @param gameSave	- pointer to the SavedGame
 * @param rules		- pointer to the Ruleset
 */
ProductionProgress Production::step(
		Base* const base,
		SavedGame* const gameSave,
		const Ruleset* const rules)
{
	_timeSpent += _engineers;

	const int qtyDone (getAmountProduced());
	if (qtyDone < getAmountProduced())
	{

		int produced;
		if (_infinite == false)
			produced = std::min( // required to not overproduce
							getAmountProduced(),
							_amount) - qtyDone;
		else
			produced = getAmountProduced() - qtyDone;

		int qty (0);
		do
		{
			for (std::map<std::string, int>::const_iterator
					i = _manfRule->getProducedItems().begin();
					i != _manfRule->getProducedItems().end();
					++i)
			{
				if (_manfRule->getCategory() == "STR_CRAFT")
				{
					Craft* const craft (new Craft(
												rules->getCraft(i->first),
												base,
												gameSave->getCanonicalId(i->first)));
					craft->setCraftStatus(CS_REFUELLING);
					base->getCrafts()->push_back(craft);
					break;
				}
				else
				{
					// Check if it's fuel OR ammo for a Craft
					if (rules->getItem(i->first)->getBattleType() == BT_NONE)
					{
						for (std::vector<Craft*>::const_iterator
								j = base->getCrafts()->begin();
								j != base->getCrafts()->end();
								++j)
						{
							if ((*j)->getWarned() == true)
							{
								switch ((*j)->getCraftStatus())
								{
									case CS_REFUELLING:
										if ((*j)->getRules()->getRefuelItem() == i->first)
											(*j)->setWarned(false);
										break;
									case CS_REARMING:
										for (std::vector<CraftWeapon*>::const_iterator
												k = (*j)->getWeapons()->begin();
												k != (*j)->getWeapons()->end();
												++k)
										{
											if (*k != nullptr
												&& (*k)->getRules()->getClipItem() == i->first)
											{
												(*k)->setCantLoad(false);
												(*j)->setWarned(false);
											}
										}
								}
							}
						}
					}

					// kL_note: MISSING - check if it's ammo to load a tank (onboard a Craft)!
					// from, ItemsArrivingState:
/*					RuleItem* item = _game->getRuleset()->getItem((*j)->getItems());

					if (rules->getItem(i->first)->getBattleType() == BT_AMMO)
					{
						for (std::vector<Craft*>::iterator
								c = base->getCrafts()->begin();
								c != base->getCrafts()->end();
								++c)
						{
							for (std::vector<Vehicle*>::iterator // check if it's ammo to reload a vehicle
									v = (*c)->getVehicles()->begin();
									v != (*c)->getVehicles()->end();
									++v)
							{
								std::vector<std::string>::iterator ammo = std::find(
																				(*v)->getRules()->getCompatibleAmmo()->begin(),
																				(*v)->getRules()->getCompatibleAmmo()->end(),
																				item->getType());
								if (ammo != (*v)->getRules()->getCompatibleAmmo()->end()
									&& (*v)->getAmmo() < item->getClipSize())
								{
									int used = std::min(
													(*j)->getQuantity(),
													item->getClipSize() - (*v)->getAmmo());
									(*v)->setAmmo((*v)->getAmmo() + used);

									// Note that the items have already been delivered --
									// so they are removed from the base, not the transfer.
									base->getItems()->removeItem(item->getType(), used);
								}
							}
						}
					} */

					if (_sell == true) // <- this may be Fucked.
					{
						gameSave->setFunds(gameSave->getFunds() + (rules->getItem(i->first)->getSellCost() * i->second));
						base->setCashIncome(rules->getItem(i->first)->getSellCost() * i->second);
					}
					else
						base->getStorageItems()->addItem(i->first, i->second);
				}
			}

			++qty;
			if (qty < produced)
			{
				if (enoughMoney(gameSave) == false)
					return PROGRESS_NOT_ENOUGH_MONEY;

				if (enoughMaterials(base, rules) == false)
					return PROGRESS_NOT_ENOUGH_MATERIALS;

				startProduction(base, gameSave, rules);
			}
		}
		while (qty < produced);
	}

	if (getAmountProduced() >= _amount
		&& _infinite == false)
	{
		return PROGRESS_COMPLETE;
	}

	if (qtyDone < getAmountProduced())
	{
		if (enoughMoney(gameSave) == false)
			return PROGRESS_NOT_ENOUGH_MONEY;

		if (enoughMaterials(base, rules) == false)
			return PROGRESS_NOT_ENOUGH_MATERIALS;

		startProduction(base, gameSave, rules);
	}

	return PROGRESS_NOT_COMPLETE;
}

/**
 * Starts this Production.
 * @param base		- pointer to a Base
 * @param gameSave	- pointer to the SavedGame
 */
void Production::startProduction(
		Base* const base,
		SavedGame* const gameSave,
		const Ruleset* const rules) const
{
	const int cost (_manfRule->getManufactureCost());
	gameSave->setFunds(gameSave->getFunds() - cost);
	base->setCashSpent(cost);

	for (std::map<std::string,int>::const_iterator
			i = _manfRule->getRequiredItems().begin();
			i != _manfRule->getRequiredItems().end();
			++i)
	{
		if (rules->getItem(i->first) != nullptr)
			base->getStorageItems()->removeItem(i->first, i->second);
		else if (rules->getCraft(i->first) != nullptr)
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
 * Gets the time till this Production is completed.
 * param days	- reference to store days remaining
 * param hours	- reference to store hours remaining
 * @return, true if work is progressing
 */
bool Production::tillFinish(
		int& days,
		int& hours) const
{
	if (_engineers != 0)
	{
		if (_sell == true || _infinite == true)
		{
			hours = (getAmountProduced() + 1) * getRules()->getManufactureTime()
				  - _timeSpent;
		}
		else
			hours = getAmountTotal() * _manfRule->getManufactureTime()
				  - _timeSpent;

		hours = (hours + _engineers - 1) / _engineers;

		days = (hours / 24);
		hours %= 24;

		return true;
	}
	return false;
}

}
