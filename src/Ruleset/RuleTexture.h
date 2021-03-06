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

#ifndef OPENXCOM_RULETEXTURE_H
#define OPENXCOM_RULETEXTURE_H

#include "../fmath.h"

//#include <string>
//#include <vector>

#include <yaml-cpp/yaml.h>


namespace OpenXcom
{

struct TextureDetail
{
	std::string type;
	int weight;
	double
		lonMin,
		lonMax,
		latMin,
		latMax;

	TextureDetail()
		:
			weight(10),
			lonMin(  0.),
			lonMax(360.),
			latMin(-90.),
			latMax( 90.)
	{}
};

class Target;


/**
 * Represents the relations between a Geoscape texture and the corresponding
 * Battlescape tactical attributes.
 */
class RuleTexture
{

private:
	int _id;
	std::map<std::string, int> _deployTypes;
	std::vector<TextureDetail> _details;

	public:
		/// Creates a new texture with mission data.
		explicit RuleTexture(int id);
		/// Cleans up the texture.
		~RuleTexture();

		/// Loads the texture from YAML.
		void load(const YAML::Node& node);

		/// Gets the list of terrain criteria.
		const std::vector<TextureDetail>* getTextureDetail() const;

		/// Gets the alien deployments for this Texture.
		const std::map<std::string, int>& getTextureDeployments() const;

		/// Gets a random deployment.
		std::string getTextureDeployment() const;

		/// Gets a randomly textured terrain-type for a given target.
		std::string getTextureTerrain(const Target* const target = nullptr) const;
};

}


namespace YAML
{

template<>
struct convert<OpenXcom::TextureDetail>
{
	///
	static Node encode(const OpenXcom::TextureDetail& rhs)
	{
		Node node;

		node["type"]	= rhs.type;
		node["weight"]	= rhs.weight;

		std::vector<double> area;
		area.push_back(rhs.lonMin);
		area.push_back(rhs.lonMax);
		area.push_back(rhs.latMin);
		area.push_back(rhs.latMax);

		node["area"] = area;

		return node;
	}

	///
	static bool decode(
			const Node& node,
			OpenXcom::TextureDetail& rhs)
	{
		if (node.IsMap() == false)
			return false;

		rhs.type	= node["type"]	.as<std::string>(rhs.type);
		rhs.weight	= node["weight"].as<int>(rhs.weight);

		if (node["area"])
		{
			std::vector<double> area (node["area"].as<std::vector<double>>());
			rhs.lonMin = area[0] * M_PI / 180.;
			rhs.lonMax = area[1] * M_PI / 180.;
			rhs.latMin = area[2] * M_PI / 180.;
			rhs.latMax = area[3] * M_PI / 180.;
		}

		return true;
	}
};

}

#endif
