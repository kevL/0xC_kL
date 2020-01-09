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

#include "RuleTexture.h"

#include "../Engine/Logger.h"
#include "../Engine/RNG.h"

#include "../Savegame/Target.h"


namespace OpenXcom
{

/**
 * Initializes a globe Texture.
 * @param id - Texture identifier
 */
RuleTexture::RuleTexture(int id)
	:
		_id(id)
{}

/**
 * dTor.
 */
RuleTexture::~RuleTexture()
{}

/**
 * Loads a RuleTexture type from a YAML file.
 * @param node - reference a YAML node
 */
void RuleTexture::load(const YAML::Node& node)
{
	_id				= node["id"]			.as<int>(_id);
	_deployTypes	= node["deployTypes"]	.as<std::map<std::string, int>>(_deployTypes);
	_details		= node["details"]		.as<std::vector<TextureDetail>>(_details);
}

/**
 * Gets the list of TextureDetail's associated with this RuleTexture.
 * @return, pointer to a vector of TextureDetail's
 */
const std::vector<TextureDetail>* RuleTexture::getTextureDetail() const
{
	return &_details;
}

/**
 * Returns a map of weighted deployments associated with this RuleTexture.
 * @return, reference to a map of deployment types & weights
 */
const std::map<std::string, int>& RuleTexture::getTextureDeployments() const
{
	return _deployTypes;
}

/**
 * Calculates a random deployment-type for a mission-target based on this
 * RuleTexture's available deployment-types.
 * @return, deployment type
 */
std::string RuleTexture::getTextureDeployment() const
{
	//Log(LOG_INFO) << "RuleTexture::getTextureDeployment()";
	if (_deployTypes.empty() == false)
	{
		std::map<std::string, int>::const_iterator i (_deployTypes.begin());
		if (_deployTypes.size() == 1u)
		{
			//Log(LOG_INFO) << ". ret " << i->first;
			return i->first;
		}

		int totalWeight (0);
		for (
				;
				i != _deployTypes.end();
				++i)
		{
			totalWeight += i->second;
		}
		//Log(LOG_INFO) << ". total wt= " << totalWeight;

		if (totalWeight != 0)
		{
			int pick (RNG::generate(0, totalWeight - 1));
			for (
					i = _deployTypes.begin();
					i != _deployTypes.end();
					++i)
			{
				//Log(LOG_INFO) << ". deploy= " << i->first << " wt= " << i->second;
				//Log(LOG_INFO) << ". pick= " << pick;
				if (pick < i->second)
				{
					//Log(LOG_INFO) << ". . ret " << i->first;
					return i->first;
				}
				else
					pick -= i->second;
			}
		}
	}
	//Log(LOG_INFO) << ". ret EMPTY";
	return "";
}

/**
 * Calculates a random terrain for a Target based on this RuleTexture's
 * available TextureDetails.
 * @param target - pointer to a target (default nullptr to exclude geographical bounds)
 * @return, terrain-type
 */
std::string RuleTexture::getTextureTerrain(const Target* const target) const
{
	//Log(LOG_INFO) << "RuleTexture::getTextureTerrain()";
	double
		lon,lat;
	std::map<std::string, int> terrains;

	int totalWeight (0);
	for (std::vector<TextureDetail>::const_iterator
			i = _details.begin();
			i != _details.end();
			++i)
	{
		//Log(LOG_INFO) << ". terrainType " << i->type << " wt= " << i->weight;
		if (i->weight != 0)
		{
			bool insideArea (false);
			if (target != nullptr)
			{
				lon = target->getLongitude();
				lat = target->getLatitude();

				if (   lon >= i->lonMin
					&& lon <  i->lonMax
					&& lat >= i->latMin
					&& lat <  i->latMax)
				{
					insideArea = true;
				}
			}

			if (insideArea == true || target == nullptr)
			{
				//Log(LOG_INFO) << ". . eligible";
				terrains[i->type] = i->weight;
				totalWeight += i->weight;
			}
			//else Log(LOG_INFO) << ". . target outside assigned geographical area";
		}
	}
	//Log(LOG_INFO) << ". total wt= " << totalWeight;

	if (totalWeight != 0)
	{
		int pick (RNG::generate(0, totalWeight - 1));
		for (std::map<std::string, int>::const_iterator
				i = terrains.begin();
				i != terrains.end();
				++i)
		{
			//Log(LOG_INFO) << ". terrain= " << i->first << " wt= " << i->second;
			//Log(LOG_INFO) << ". pick= " << pick;
			if (pick < i->second)
			{
				//Log(LOG_INFO) << ". . ret " << i->first;
				return i->first;
			}
			else
				pick -= i->second;
		}
	}
	//Log(LOG_INFO) << ". ret EMPTY";
	return "";
}

}
