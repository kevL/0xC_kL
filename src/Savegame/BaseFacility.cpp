/*
 * Copyright 2010-2020 OpenXcom Developers.
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

#include "BaseFacility.h"

#include "Base.h"
#include "ManufactureProject.h"

#include "../Ruleset/RuleBaseFacility.h"
#include "../Ruleset/RuleManufacture.h"


namespace OpenXcom
{

/**
 * Initializes the BaseFacility of the specified type.
 * @param facRule	- pointer to RuleBaseFacility
 * @param base		- pointer to the Base of this facility
 */
BaseFacility::BaseFacility(
		const RuleBaseFacility* const facRule,
		Base* const base)
	:
		_facRule(facRule),
		_base(base),
		_x(-1),
		_y(-1),
		_buildTime(0),
		_craft(nullptr)
{}

/**
 * dTor.
 */
BaseFacility::~BaseFacility()
{}

/**
 * Loads this BaseFacility from a YAML file.
 * @param node - reference a YAML node
 */
void BaseFacility::load(const YAML::Node& node)
{
	_x = node["x"].as<int>(_x);
	_y = node["y"].as<int>(_y);

	_buildTime = node["buildTime"].as<int>(_buildTime);
}

/**
 * Saves this BaseFacility to a YAML file.
 * @return, YAML node
 */
YAML::Node BaseFacility::save() const
{
	YAML::Node node;

	node["type"]	= _facRule->getType();
	node["x"]		= _x;
	node["y"]		= _y;

	if (_buildTime != 0)
		node["buildTime"] = _buildTime;

	return node;
}

/**
 * Gets the ruleset for this BaseFacility's type.
 * @return, pointer to RuleBaseFacility
 */
const RuleBaseFacility* BaseFacility::getRules() const
{
	return _facRule;
}

/**
 * Gets this BaseFacility's x-position on the base-grid.
 * @return, x-position in grid-squares
 */
int BaseFacility::getX() const
{
	return _x;
}

/**
 * Sets this BaseFacility's x-position on the base-grid.
 * @param x - x-position in grid-squares
 */
void BaseFacility::setX(int x)
{
	_x = x;
}

/**
 * Gets this BaseFacility's y-position on the base-grid.
 * @return, y-position in grid-squares
 */
int BaseFacility::getY() const
{
	return _y;
}

/**
 * Sets this BaseFacility's y-position on the base-grid.
 * @param y - y-position in grid-squares
 */
void BaseFacility::setY(int y)
{
	_y = y;
}

/**
 * Gets this BaseFacility's remaining time until it's finished building.
 * @return, time left in days (0 = complete)
 */
int BaseFacility::getBuildTime() const
{
	return _buildTime;
}

/**
 * Sets this BaseFacility's remaining time until it's finished building.
 * @param buildTime - time left in days (0 = complete)
 */
void BaseFacility::setBuildTime(int buildTime)
{
	_buildTime = buildTime;
}

/**
 * Builds this BaseFacility day by day.
 * @return, true if finished
 */
bool BaseFacility::buildFacility()
{
	if (--_buildTime == 0)
	{
		if (_facRule->getType() == "STR_MIND_SHIELD") // reset Base exposed bool.
			_base->setBaseExposed(false);

		return true;
	}
	return false;
}

/**
 * Gets if this Facility has finished building.
 * @return, true if finished
 */
bool BaseFacility::buildFinished() const
{
	return (_buildTime == 0);
}

/**
 * Checks if this BaseFacility is currently being used by its Base.
 * @return, true if in use
 */
bool BaseFacility::inUse() const
{
	if (_buildTime == 0)
	{
		for (std::vector<ManufactureProject*>::const_iterator
				i = _base->getManufacture().begin();
				i != _base->getManufacture().end();
				++i)
		{
			if ((*i)->getRules()->getRequiredFacilities().count(_facRule->getType()) != 0)
//				&& (*i)->getRules()->getRequiredFacilities().at(_facRule->getType()) != 0 // safety. In case of a RuleManufacture that spec's 0 qty of a facility. hint: Don't do that.
			{
				const int facsRequired ((*i)->getRules()->getRequiredFacilities().at(_facRule->getType())); // Did i mention that I deplore c++ yet. Good.
				int facsFound (0);

				for (std::vector<BaseFacility*>::const_iterator
						j = _base->getFacilities()->begin();
						j != _base->getFacilities()->end();
						++j)
				{
					if ((*j)->getRules() == _facRule)
						++facsFound;
				}

				if (facsFound <= facsRequired)	// NOTE: In practice 'facsFound' should never be less-than (only equal or greater-than) 'facsRequired'.
					return true;				// TODO: Because a project should be cancelled immediately if facilities get destroyed in a BaseDefense tactical.
			}
		}

		return (_facRule->getPersonnel() != 0
				&& _base->getTotalQuarters() - _facRule->getPersonnel() < _base->getUsedQuarters())
			|| (_facRule->getStorage() != 0
				&& _base->getTotalStores() - _facRule->getStorage() < _base->getUsedStores())
			|| (_facRule->getLaboratories() != 0
				&& _base->getTotalLaboratories() - _facRule->getLaboratories() < _base->getUsedLaboratories())
			|| (_facRule->getWorkshops() != 0
				&& _base->getTotalWorkshops() - _facRule->getWorkshops() < _base->getUsedWorkshops())
			|| (_facRule->getCrafts() != 0
				&& _base->getTotalHangars() - _facRule->getCrafts() < _base->getUsedHangars())
			|| (_facRule->getPsiLaboratories() != 0
				&& _base->getTotalPsiLabs() - _facRule->getPsiLaboratories() < _base->getUsedPsiLabs())
			|| (_facRule->getAliens() != 0
				&& _base->getTotalContainment() - _facRule->getAliens() < _base->getUsedContainment());
	}

	return false;
}

/**
 * Gets the Craft used for drawing this BaseFacility if any.
 * @return, pointer to the Craft
 */
const Craft* BaseFacility::getCraft() const
{
	return _craft;
}

/**
 * Sets a Craft used for drawing this BaseFacility.
 * @param craft - pointer to the craft (default nullptr)
 */
void BaseFacility::setCraft(const Craft* const craft)
{
	_craft = craft;
}

}
