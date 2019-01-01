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

#include "SoldierDeath.h"


namespace OpenXcom
{

/**
 * Creates a SoldierDeath.
 */
SoldierDeath::SoldierDeath()
	:
		_deathTime(0,0,0,0,0,0)
{}

/**
 * dTor.
 */
SoldierDeath::~SoldierDeath()
{}

/**
 * Loads the SoldierDeath from a YAML file.
 * @param node - reference a YAML node
 */
void SoldierDeath::load(const YAML::Node& node)
{
	_deathTime.load(node["time"]);
}

/**
 * Saves the SoldierDeath to a YAML file.
 * @return, YAML node
 */
YAML::Node SoldierDeath::save() const
{
	YAML::Node node;

	node["time"] = _deathTime.save(true);

	return node;
}

/**
 * Gets the time of death of a Soldier.
 * @return, pointer to GameTime
 */
const GameTime* SoldierDeath::getTime() const
{
	return &_deathTime;
}

/**
 * Sets the time of death of a Soldier.
 * @param gt - reference to the time of death
 */
void SoldierDeath::setTime(const GameTime& gt)
{
	_deathTime = gt;
}

}
