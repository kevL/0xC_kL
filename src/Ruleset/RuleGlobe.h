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

#ifndef OPENXCOM_RULEGLOBE_H
#define OPENXCOM_RULEGLOBE_H

//#include <list>
//#include <string>

#include <yaml-cpp/yaml.h>


namespace OpenXcom
{

enum TextureTypes
{
	TT_PORT = -2,		// -2
	TT_CITY = -1,		// -1
	TT_FOREST,			//  0
	TT_PLAINS,			//  1
	TT_CULTIVATED,		//  2
	TT_FORESTMOUNT,		//  3
	TT_TUNDRA,			//  4
	TT_MOUNT,			//  5
	TT_JUNGLE,			//  6
	TT_DESERT,			//  7
	TT_DESERTMOUNT,		//  8
	TT_POLAR,			//  9
	TT_URBAN,			// 10
	TT_POLARMOUNT,		// 11
	TT_POLARICE			// 12
};

class Polygon;
class Polyline;
class RuleTexture;


/**
 * Represents the contents of the geoscape Globe such as Polygons, Polylines, etc.
 * @sa Globe
 */
class RuleGlobe
{

private:
	std::list<Polygon*> _polygons;
	std::list<Polyline*> _polylines;

	std::map<int, RuleTexture*> _textures;


	public:
		/// Creates a Globe ruleset.
		RuleGlobe();
		/// Cleans up the Globe ruleset.
		~RuleGlobe();

		/// Loads the Globe ruleset from YAML.
		void load(const YAML::Node& node);

		/// Gets the list of Polygons.
		std::list<Polygon*>* getPolygons();
		/// Gets the list of Polylines.
		std::list<Polyline*>* getPolylines();

		/// Loads a set of Polygons from a DAT-file.
		void loadDat(const std::string& file);

		/// Gets a RuleTexture from a specified texture-ID.
		RuleTexture* getTextureRule(int tex) const;
		/// Gets the eligible terrains for an RuleAlienDeployment type.
		std::vector<std::string> getGlobeTerrains(const std::string& deployType = "") const;
};

}

#endif
