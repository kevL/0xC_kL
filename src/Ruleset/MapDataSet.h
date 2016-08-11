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

#ifndef OPENXCOM_MAPDATASET_H
#define OPENXCOM_MAPDATASET_H

//#include <string>
//#include <vector>

#include <SDL_stdinc.h>

#include <yaml-cpp/yaml.h>


namespace OpenXcom
{

class Game;
class MapData;
class SurfaceSet;


/**
 * Represents a Terrain Map Datafile that corresponds to an XCom MCD & PCK file.
 * Popularly known as a tileset.
 * @note The list of map datafiles is stored in RuleSet but referenced in RuleTerrain.
 * @sa http://www.ufopaedia.org/index.php?title=MCD
 */
class MapDataSet
{

private:
	bool _loaded;

	std::string _type;

	static MapData
		* _floorBlank,
		* _floorScorch;

	const Game* _game;
	SurfaceSet* _surfaceSet;

	std::vector<MapData*> _records;


	public:
		/// Constructs a MapDataSet.
		MapDataSet(
				const std::string& type,
				const Game* const game);
		/// Destructs the MapDataSet.
		~MapDataSet();

		/// Loads the MapDataSet from YAML.
		void load(const YAML::Node& node);

		/// Gets the dataset-type used for MAP generation.
		std::string getType() const;

		/// Gets the dataset-size.
		size_t getSize() const;

		/// Gets the parts/entries in the MapDataSet.
		std::vector<MapData*>* getRecords();

		/// Gets the Surfaces in the MapDataSet.
		SurfaceSet* getSurfaceset() const;

		/// Loads the MCD-file.
		void loadData();
		///	Unloads the MapDataSet to free-up RAM.
		void unloadData();

		/// Loads LoFT-voxel data from a DAT file.
		static void loadLoft(
				const std::string& file,
				std::vector<Uint16>* const voxelData);

		/// Gets the universal blank-floor part.
//		static MapData* getBlankFloor(); // not used.
		/// Gets the universal scorched-earth part.
		static MapData* getScorchedEarth();
};

}

#endif
