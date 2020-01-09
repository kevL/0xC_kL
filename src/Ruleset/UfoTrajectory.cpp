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

#include "UfoTrajectory.h"

#include "../Savegame/MovingTarget.h"


namespace YAML
{

template<>
struct convert<OpenXcom::MissionPoint>
{
	///
	static Node encode(const OpenXcom::MissionPoint& rhs)
	{
		Node node;

		node.push_back(rhs.zone);
		node.push_back(rhs.altitude);
		node.push_back(rhs.speed);

		return node;
	}

	///
	static bool decode(
			const Node& node,
			OpenXcom::MissionPoint& rhs)
	{
		if (node.IsSequence() == false || node.size() != 3u)
			return false;

		rhs.zone		= node[0u].as<size_t>();
		rhs.altitude	= node[1u].as<size_t>();
		rhs.speed		= node[2u].as<size_t>();

		return true;
	}
};

}


namespace OpenXcom
{

const std::string UfoTrajectory::XCOM_BASE_ASSAULT = "XCOM_BASE_ASSAULT"; // static.


/**
 * Creates rules for a UfoTrajectory.
 * @param type - reference to the ID string (eg. "P0")
 */
UfoTrajectory::UfoTrajectory(const std::string& type)
	:
		_type(type),
		_ground(5)
{}

/**
 * Overwrites the UfoTrajectory's data with data stored in @a node.
 * @note Only the fields contained in the node will be overwritten.
 * @param node - reference a YAML node
 */
void UfoTrajectory::load(const YAML::Node& node)
{
	_type	= node["type"]	.as<std::string>(_type);
	_ground	= node["ground"].as<int>(_ground);
	_points	= node["points"].as<std::vector<MissionPoint>>(_points);
}

/**
 * Gets the UfoTrajectory's altitude at a specific point.
 * @param pointId - the point-ID
 * @return, altitude as a string
 */
const std::string UfoTrajectory::getAltitude(size_t pointId) const // does not like return &ref
{
	return MovingTarget::stAltitude[_points[pointId].altitude];
}

}
