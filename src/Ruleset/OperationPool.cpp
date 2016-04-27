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

#include "OperationPool.h"

//#include <fstream>
//#include <iostream>
//#include <sstream>

#include "../Engine/CrossPlatform.h"
//#include "../Engine/Exception.h"
#include "../Engine/Language.h"
#include "../Engine/RNG.h"


namespace OpenXcom
{

/**
 * Initializes an OperationPool with blank lists of titles.
 */
OperationPool::OperationPool()
{}

/**
 * dTor.
 */
OperationPool::~OperationPool()
{}

/**
 * Loads the pool from a YAML file.
 * @param file - reference a YAML file
 */
void OperationPool::load(const std::string& file)
{
	const std::string st = CrossPlatform::getDataFile("SoldierName/" + file + ".opr");
	const YAML::Node doc = YAML::LoadFile(st);

	for (YAML::const_iterator
			i = doc["operaFirst"].begin();
			i != doc["operaFirst"].end();
			++i)
		_operaFirst.push_back(Language::utf8ToWstr(i->as<std::string>()));

	for (YAML::const_iterator
			i = doc["operaLast"].begin();
			i != doc["operaLast"].end();
			++i)
		_operaLast.push_back(Language::utf8ToWstr(i->as<std::string>()));
}

/**
 * Generates an operation-title (adj + noun) from the lists of words.
 * @return, the operation-title
 */
std::wstring OperationPool::genOperation() const
{
	std::wostringstream title;

	if (_operaFirst.empty() == false)
		title << _operaFirst[RNG::pick(_operaFirst.size())];
	else
		title << L"oper";

	if (_operaLast.empty() == false)
		title << L" " << _operaLast[RNG::pick(_operaLast.size())];
	else
		title << L".ation";

	return title.str();
}

}
