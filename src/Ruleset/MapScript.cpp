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

#include "MapScript.h"

#include "MapBlock.h"
#include "RuleTerrain.h"

#include "../Engine/Exception.h"
#include "../Engine/RNG.h"


namespace OpenXcom
{

/**
 * Constructs a MapScript.
 */
MapScript::MapScript()
	:
		_type(MSC_UNDEFINED),
		_sizeX(1),
		_sizeY(1),
		_sizeZ(0),
		_executionChance(100),
		_executions(1),
		_cumulativeFrequency(0),
		_label(0),
		_direction(MD_NONE),
		_tunnelData(nullptr)
{}

/**
 * dTor.
 */
MapScript::~MapScript()
{
	for (std::vector<SDL_Rect*>::const_iterator
			i = _rects.begin();
			i != _rects.end();
			++i)
		delete *i;

	delete _tunnelData;
}

/**
 * Loads a MapScript directive from YAML.
 * @param node - reference a YAML node
 */
void MapScript::load(const YAML::Node& node)
{
	if (const YAML::Node& subnode = node["type"])
	{
		const std::string type (subnode.as<std::string>());

		if (type == "addBlock")
			_type = MSC_ADDBLOCK;
		else if (type == "addLine")
			_type = MSC_ADDLINE;
		else if (type == "addCraft")
		{
			_type = MSC_ADDCRAFT;
			_groups.push_back(1); // this is a default and can be overridden
		}
		else if (type == "addUFO")
		{
			_type = MSC_ADDUFO;
			_groups.push_back(1); // this is a default and can be overridden
		}
		else if (type == "digTunnel")
			_type = MSC_DIGTUNNEL;
		else if (type == "fillArea")
			_type = MSC_FILLAREA;
		else if (type == "checkBlock")
			_type = MSC_CHECKBLOCK;
		else if (type == "removeBlock")
			_type = MSC_REMOVE;
		else if (type == "resize")
		{
			_type = MSC_RESIZE;
			_sizeX =
			_sizeY = 0; // defaults: don't resize anything unless specified.
		}
		else
		{
			throw Exception("Unknown type: " + type);
		}
	}
	else
	{
		throw Exception("Missing directive type.");
	}

	if (const YAML::Node& subnode = node["rects"])
	{
		for (YAML::const_iterator
				i = subnode.begin();
				i != subnode.end();
				++i)
		{
			SDL_Rect* const rect (new SDL_Rect());
			rect->x = static_cast<Sint16>((*i)[0u].as<int>()); // NOTE: not sure if YAML can do a cast to S/Uint16's.
			rect->y = static_cast<Sint16>((*i)[1u].as<int>());
			rect->w = static_cast<Uint16>((*i)[2u].as<int>());
			rect->h = static_cast<Uint16>((*i)[3u].as<int>());

			_rects.push_back(rect);
		}
	}

	if (const YAML::Node& subnode = node["tunnelData"])
	{
		_tunnelData = new TunnelData;
		_tunnelData->level = subnode["level"].as<int>(0);

		if (const YAML::Node& data = subnode["MCDReplacements"])
		{
			for (YAML::Node::const_iterator
					i = data.begin();
					i != data.end();
					++i)
			{
				MCDReplacement replacement;
				replacement.entry = (*i)["entry"].as<int>(-1);
				replacement.dataSet = (*i)["set"].as<int>(-1);

				const std::string type ((*i)["type"].as<std::string>());
				_tunnelData->replacements[type] = replacement;
			}
		}
	}

	if (const YAML::Node& subnode = node["conditions"])
	{
		if (subnode.Type() == YAML::NodeType::Sequence)
			_conditions = subnode.as<std::vector<int>>(_conditions);
		else
			_conditions.push_back(subnode.as<int>(0));
	}

	if (const YAML::Node& subnode = node["size"])
	{
		if (subnode.Type() == YAML::NodeType::Sequence)
		{
			int* sizes[3u]
			{
				&_sizeX,
				&_sizeY,
				&_sizeZ
			};

			size_t j (0u);
			for (YAML::const_iterator
					i = subnode.begin();
					i != subnode.end() && j != 3u;
					++i, ++j)
				*sizes[j] = (*i).as<int>(1);
		}
		else
			_sizeX =
			_sizeY = subnode.as<int>(_sizeX);
	}

	if (const YAML::Node& subnode = node["groups"])
	{
		_groups.clear();
		if (subnode.Type() == YAML::NodeType::Sequence)
		{
			for (YAML::const_iterator
					i = subnode.begin();
					i != subnode.end();
					++i)
				_groups.push_back((*i).as<int>(0));
		}
		else
			_groups.push_back(subnode.as<int>(0));
	}

	size_t selectionSize (_groups.size());
	if (const YAML::Node& subnode = node["blocks"])
	{
		_groups.clear();
		if (subnode.Type() == YAML::NodeType::Sequence)
		{
			for (YAML::const_iterator
					i = subnode.begin();
					i != subnode.end();
					++i)
				_blocks.push_back((*i).as<int>(0));
		}
		else
			_blocks.push_back(subnode.as<int>(0));

		selectionSize = _blocks.size();
	}

	_frequencies.resize(selectionSize, 1);
	_maxUses.resize(selectionSize, -1);

	if (const YAML::Node& subnode = node["freqs"])
	{
		if (subnode.Type() == YAML::NodeType::Sequence)
		{
			size_t j (0u);
			for (YAML::const_iterator
					i = subnode.begin();
					i != subnode.end() && j != selectionSize;
					++i, ++j)
				_frequencies.at(j) = (*i).as<int>(1);
		}
		else
			_frequencies.at(0u) = subnode.as<int>(1);
	}

	if (const YAML::Node& subnode = node["maxUses"])
	{
		if (subnode.Type() == YAML::NodeType::Sequence)
		{
			size_t j (0u);
			for (YAML::const_iterator
					i = subnode.begin();
					i != subnode.end() && j != selectionSize;
					++i, ++j)
				_maxUses.at(j) = (*i).as<int>(-1);
		}
		else
			_maxUses.at(0u) = subnode.as<int>(-1);
	}

	if (const YAML::Node& subnode = node["direction"])
	{
		std::string dir (subnode.as<std::string>());
		dir = dir.substr(0u,1u);
		if (dir == "v")
			_direction = MD_VERTICAL;
		else if (dir == "h")
			_direction = MD_HORIZONTAL;
		else if (dir == "b")
			_direction = MD_BOTH;
	}

	if (_direction == MD_NONE)
	{
		switch (_type)
		{
			case MSC_DIGTUNNEL:
				throw Exception("MapScript: The direction is undefined for digTunnel operation - specify [v]ertical, [h]orizontal, or [b]oth");
				break;

			case MSC_ADDLINE:
				throw Exception("MapScript: The direction is undefined for addLine operation - specify [v]ertical, [h]orizontal, or [b]oth");
		}
	}

	_executionChance	= node["executionChance"]	.as<int>(_executionChance);
	_executions			= node["executions"]		.as<int>(_executions);
	_ufoType			= node["ufoType"]			.as<std::string>(_ufoType);
	_label				= std::abs(node["label"]	.as<int>(_label)); // take no chances, use Head & Shoulders.
}

/**
 * Initializes all the various scratch values and such for the directive.
 */
void MapScript::init()
{
	_cumulativeFrequency = 0;
	_blocksTemp.clear();
	_groupsTemp.clear();
	_frequenciesTemp.clear();
	_maxUsesTemp.clear();

	for (std::vector<int>::const_iterator
			i = _frequencies.begin();
			i != _frequencies.end();
			++i)
		_cumulativeFrequency += *i;

	_blocksTemp = _blocks;
	_groupsTemp = _groups;
	_frequenciesTemp = _frequencies;
	_maxUsesTemp = _maxUses;
}

/**
 * Gets a random group number from the array accounting for frequencies and
 * max-uses.
 * @note If no groups or blocks are defined this will return the default group;
 * if all the max-uses are used up it will return undefined.
 * @return, group number
 */
int MapScript::getGroup() // private.
{
	if (_groups.size() == 0u)
		return 0;// MBT_DEFAULT; // NOTE: Returning a MapBlockType is ... misleading.

	if (_cumulativeFrequency > 0)
	{
		int pick (RNG::generate(0,
								_cumulativeFrequency - 1));

		for (size_t
				i = 0u;
				i != _groupsTemp.size();
				++i)
		{
			if (pick < _frequenciesTemp.at(i))
			{
				const int ret (_groupsTemp.at(i));
				if (_maxUsesTemp.at(i) > 0)
				{
					if (--_maxUsesTemp.at(i) == 0)
					{
						_groupsTemp.erase(_groupsTemp.begin() + static_cast<std::ptrdiff_t>(i));
						_cumulativeFrequency -= _frequenciesTemp.at(i);
						_frequenciesTemp.erase(_frequenciesTemp.begin() + static_cast<std::ptrdiff_t>(i));
						_maxUsesTemp.erase(_maxUsesTemp.begin() + static_cast<std::ptrdiff_t>(i));
					}
				}
				return ret;
			}
			pick -= _frequenciesTemp.at(i);
		}
	}
	return -1;// MBT_UNDEFINED; // NOTE: Returning a MapBlockType is ... misleading.
}

/**
 * Gets a random block number from the array accounting for frequencies and
 * max-uses.
 * @note If no blocks are defined it will use a group instead.
 * @return, block number
 */
int MapScript::getBlock() // private.
{
	if (_cumulativeFrequency > 0)
	{
		int pick (RNG::generate(0,
								_cumulativeFrequency - 1));

		for (size_t
				i = 0u;
				i != _blocksTemp.size();
				++i)
		{
			if (pick < _frequenciesTemp.at(i))
			{
				const int ret (_blocksTemp.at(i));
				if (_maxUsesTemp.at(i) > 0
					&& --_maxUsesTemp.at(i) == 0)
				{
					_blocksTemp.erase(_blocksTemp.begin() + static_cast<std::ptrdiff_t>(i));
					_cumulativeFrequency -= _frequenciesTemp.at(i);
					_frequenciesTemp.erase(_frequenciesTemp.begin() + static_cast<std::ptrdiff_t>(i));
					_maxUsesTemp.erase(_maxUsesTemp.begin() + static_cast<std::ptrdiff_t>(i));
				}
				return ret;
			}
			pick -= _frequenciesTemp.at(i);
		}
	}
	return -1;// MBT_UNDEFINED; // NOTE: Returning a MapBlockType is ... misleading.
}

/**
 * Gets a random MapBlock from a specified terrain using either the groups or
 * the blocks defined.
 * @param terrainRule - pointer to the RuleTerrain from which to pick a block
 * @return, pointer to a randomly chosen MapBlock given the options available in this MapScript
 */
MapBlock* MapScript::getNextBlock(const RuleTerrain* const terrainRule)
{
	if (_blocks.empty() == true)
		return terrainRule->getTerrainBlock(
										_sizeX * 10,
										_sizeY * 10,
										getGroup());

	const int blockId (getBlock());
	if (blockId != -1// MBT_UNDEFINED) // NOTE: Comparing 'blockId' with a MapBlockType is ... misleading.
		&& blockId < static_cast<int>(terrainRule->getMapBlocks()->size()))
	{
		return terrainRule->getMapBlocks()->at(static_cast<size_t>(blockId));
	}
	return nullptr;
}

/**
 * Gets the ufo-type for the ADD-UFO directive.
 * @return, ufo-type
 */
const std::string& MapScript::getUfoType() const
{
	return _ufoType;
}

}
