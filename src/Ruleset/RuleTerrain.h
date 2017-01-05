/*
 * Copyright 2010-2017 OpenXcom Developers.
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

#ifndef OPENXCOM_RULETERRAIN_H
#define OPENXCOM_RULETERRAIN_H

//#include <string>
//#include <vector>

#include <yaml-cpp/yaml.h>


namespace OpenXcom
{

class MapBlock;
class MapData;
class MapDataSet;
class Ruleset;


/**
 * Represents a specific type of Battlescape Terrain.
 * - the names of the objectsets needed in this specific terrain.
 * - the mapblocks that can be used to build this terrain.
 * @sa http://www.ufopaedia.org/index.php?title=TERRAIN
 */
class RuleTerrain
{

private:
	std::string
		_basicArmor,
		_script,
		_type;

	std::vector<std::string>
		_civTypes,
		_musics;

	std::vector<MapBlock*> _blocks;
	std::vector<MapDataSet*> _dataSets;


	public:
		/// Constructs a RuleTerrain object.
		explicit RuleTerrain(const std::string& type);
		/// Destructs the RuleTerrain object.
		~RuleTerrain();

		/// Loads the terrain-type from YAML.
		void load(
				const YAML::Node& node,
				Ruleset* const rules);

		/// Gets the terrain's type.
		const std::string& getType() const;

		/// Gets the terrain-type's list of MapBlocks.
		const std::vector<MapBlock*>* getMapBlocks() const;
		/// Gets the terrain-type's list of MapDataSets (MCDs).
		const std::vector<MapDataSet*>* getMapDataSets() const;

		/// Gets a MapBlock in the terrain-type.
		MapBlock* getTerrainBlock(
				int sizeX,
				int sizeY,
				int group,
				bool force = true) const;
		/// Gets a MapBlock of a specified type in the terrain-type.
		MapBlock* getTerrainBlock(const std::string& type) const;

		/// Gets a MapData object (MCD tile-part) in the terrain-type.
		MapData* getTerrainPart(
				size_t* partId,
				int* partSetId) const;

		/// Gets the civilian-types that can appear in the terrain-type.
		const std::vector<std::string>& getCivilianTypes() const;

		/// Gets the generation script for the terrain-type.
		const std::string& getScriptType() const;

		/// Gets the list of music that can play in the terrain-type.
		const std::vector<std::string>& getTerrainMusics() const;

		/// Gets the basic-armor-type of the terrain-type.
		const std::string& getBasicArmorType() const;
};

}

#endif
