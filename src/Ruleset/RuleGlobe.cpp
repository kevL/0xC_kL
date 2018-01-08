/*
 * Copyright 2010-2018 OpenXcom Developers.
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

#include "RuleGlobe.h"

#include <cmath>
#include <fstream>

//#include <SDL/SDL_endian.h>

#include "Polygon.h"
#include "Polyline.h"
#include "RuleTexture.h"

#include "../Engine/CrossPlatform.h"
#include "../Engine/Exception.h"

#include "../Geoscape/Globe.h"


namespace OpenXcom
{

/**
 * Creates the RuleGlobe.
 */
RuleGlobe::RuleGlobe()
{}

/**
 * dTor.
 */
RuleGlobe::~RuleGlobe()
{
	for (std::list<Polygon*>::const_iterator
			i = _polygons.begin();
			i != _polygons.end();
			++i)
		delete *i;

	for (std::list<Polyline*>::const_iterator
			i = _polylines.begin();
			i != _polylines.end();
			++i)
		delete *i;

	for (std::map<int, RuleTexture*>::const_iterator
			i = _textures.begin();
			i != _textures.end();
			++i)
		delete i->second;
}

/**
 * Loads this RuleGlobe from a YAML file.
 * @param node - reference a YAML node
 */
void RuleGlobe::load(const YAML::Node& node)
{
	if (node["data"])
	{
		for (std::list<Polygon*>::const_iterator
				i = _polygons.begin();
				i != _polygons.end();
				++i)
		{
			delete *i;
		}
		_polygons.clear();

		loadDat(CrossPlatform::getDataFile(node["data"].as<std::string>()));
	}

	if (node["polygons"])
	{
		for (std::list<Polygon*>::const_iterator
				i = _polygons.begin();
				i != _polygons.end();
				++i)
		{
			delete *i;
		}
		_polygons.clear();

		Polygon* polygon;
		for (YAML::const_iterator
				i = node["polygons"].begin();
				i != node["polygons"].end();
				++i)
		{
			polygon = new Polygon(3u);
			polygon->load(*i);
			_polygons.push_back(polygon);
		}
	}

	if (node["polylines"])
	{
		for (std::list<Polyline*>::const_iterator
				i = _polylines.begin();
				i != _polylines.end();
				++i)
		{
			delete *i;
		}
		_polylines.clear();

		Polyline* polyline;
		for (YAML::const_iterator
				i = node["polylines"].begin();
				i != node["polylines"].end();
				++i)
		{
			polyline = new Polyline(3u);
			polyline->load(*i);
			_polylines.push_back(polyline);
		}
	}

	for (YAML::const_iterator
			i = node["textures"].begin();
			i != node["textures"].end();
			++i)
	{
		if ((*i)["id"])
		{
			const int id ((*i)["id"].as<int>());
			RuleTexture* texture;

			const std::map<int, RuleTexture*>::const_iterator j (_textures.find(id));
			if (j != _textures.end())
				texture = j->second;
			else
			{
				texture = new RuleTexture(id);
				_textures[id] = texture;
			}
			texture->load(*i);
		}
		else if ((*i)["delete"])
		{
			const int id ((*i)["delete"].as<int>());
			const std::map<int, RuleTexture*>::const_iterator j (_textures.find(id));
			if (j != _textures.end())
				_textures.erase(j);
		}
	}
//	if (node["textures"])
//	{
//		for (std::map<int, RuleTexture*>::const_iterator
//				i = _textures.begin();
//				i != _textures.end();
//				++i)
//		{
//			delete i->second;
//		}
//		_textures.clear();
//		for (YAML::const_iterator
//				i = node["textures"].begin();
//				i != node["textures"].end();
//				++i)
//		{
//			int id = (*i)["id"].as<int>();
//			RuleTexture* texture = new RuleTexture(id);
//			texture->load(*i);
//			_textures[id] = texture;
//		}
//	}

	Globe::C_LBLBASE	= static_cast<Uint8>(node["baseColor"]		.as<int>(Globe::C_LBLBASE));
	Globe::C_LBLCITY	= static_cast<Uint8>(node["cityColor"]		.as<int>(Globe::C_LBLCITY));
	Globe::C_LBLCOUNTRY	= static_cast<Uint8>(node["countryColor"]	.as<int>(Globe::C_LBLCOUNTRY));
	Globe::C_LINE		= static_cast<Uint8>(node["lineColor"]		.as<int>(Globe::C_LINE));		// country lines
//	Globe::C_RADAR1		= static_cast<Uint8>(node["radar1Color"]	.as<int>(Globe::C_RADAR1));		// base radars
	Globe::C_RADAR2		= static_cast<Uint8>(node["radar2Color"]	.as<int>(Globe::C_RADAR2));		// craft radars
	Globe::C_FLIGHT		= static_cast<Uint8>(node["flightColor"]	.as<int>(Globe::C_FLIGHT));		// flight paths
	Globe::C_OCEAN		= static_cast<Uint8>(node["oceanPalette"]	.as<int>(Globe::C_OCEAN));
}

/**
 * Gets the list of polygons in the globe.
 * @return, pointer to a list of pointers to Polygons
 */
std::list<Polygon*>* RuleGlobe::getPolygons()
{
	return &_polygons;
}

/**
 * Gets the list of polylines in the globe.
 * @return, pointer to a list of pointers to Polylines
 */
std::list<Polyline*>* RuleGlobe::getPolylines()
{
	return &_polylines;
}

/**
 * Loads a series of polar-coordinates in X-Com format, converts them and stores
 * them in a set of Polygons.
 * @param file - filename of a DAT-file
 * @sa http://www.ufopaedia.org/index.php?title=WORLD.DAT
 */
void RuleGlobe::loadDat(const std::string& file)
{
	std::ifstream ifstr (file.c_str(), std::ios::in | std::ios::binary);
	if (ifstr.fail() == true)
	{
		throw Exception(file + " not found");
	}

	short value[10u];
	while (ifstr.read(
					reinterpret_cast<char*>(&value),
					sizeof(value)))
	{
		Polygon* poly;
		size_t points;

		for (size_t
				i = 0u;
				i != 10u;
				++i)
		{
			value[i] = SDL_SwapLE16(value[i]);
		}

		if (value[6u] != -1)
			points = 4u;
		else
			points = 3u;

		poly = new Polygon(points);

		size_t j (0u);
		for (size_t
				i = 0u;
				i != points;
				++i)
		{
			double // correct X-Com degrees and convert to radians ( 7.25 arc-min = 1 xcomDegree ~or so~ /shrug )
				lonRad = value[j++] * 0.125 * M_PI / 180.,
				latRad = value[j++] * 0.125 * M_PI / 180.;

			poly->setLongitude(i, lonRad);
			poly->setLatitude (i, latRad);
		}

		poly->setPolyTexture(static_cast<size_t>(value[8u]));
		_polygons.push_back(poly);
	}

	if (ifstr.eof() == false)
	{
		throw Exception("Invalid globe map");
	}

	ifstr.close();
}

/**
 * Gets a RuleTexture from a specified texture-ID.
 * @param id - texture-ID
 * @return, rule for a Texture
 */
RuleTexture* RuleGlobe::getTextureRule(int id) const
{
	std::map<int, RuleTexture*>::const_iterator i (_textures.find(id));
	if (i != _textures.end())
		return i->second;

	return nullptr;
}

/**
 * Gets a list of all Terrains associated with a specified RuleAlienDeployment type.
 * @note If a blank-string is passed in then terrains that are not associated
 * with any RuleAlienDeployment type are returned.
 * @param deployType - reference to the deployment type (eg. "STR_TERROR_MISSION") (default "")
 * @return, vector of terrain-types as strings
 */
std::vector<std::string> RuleGlobe::getGlobeTerrains(const std::string& deployType) const
{
	std::vector<std::string> terrains;
	for (std::map<int, RuleTexture*>::const_iterator
			i = _textures.begin();
			i != _textures.end();
			++i)
	{
		if ((deployType.empty() == true && i->second->getTextureDeployments().empty() == true)
			|| i->second->getTextureDeployments().find(deployType) != i->second->getTextureDeployments().end())
		{
			for (std::vector<TextureDetail>::const_iterator
					j = i->second->getTextureDetail()->begin();
					j != i->second->getTextureDetail()->end();
					++j)
			{
				terrains.push_back(j->type);
			}
		}
	}
	return terrains;
}

}
