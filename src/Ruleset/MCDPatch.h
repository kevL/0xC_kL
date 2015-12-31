/*
 * Copyright 2010-2015 OpenXcom Developers.
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

#ifndef OPENXCOM_MCDPATCH_H
#define OPENXCOM_MCDPATCH_H

//#include <string>
#include <yaml-cpp/yaml.h>

#include "MapData.h"


namespace OpenXcom
{

class MapDataSet;

/**
 * An MCD data Patch.
 */
class MCDPatch
{

private:
	std::vector<std::pair<size_t, int>>
		_armors,
		_bigWalls,
		_deathTiles,
		_explosives,
		_flammabilities,
		_footstepSounds,
		_fuels,
		_heBlocks,
		_psychedelic,
		_scanG,
		_terrainHeight,
		_tuWalks,
		_tuFlys,
		_tuSlides;
	std::vector<std::pair<size_t, bool>>
		_noFloors,
		_stopLOSes;
	std::vector<std::pair<size_t, std::vector<size_t>>> _lofts;

	std::vector<std::pair<size_t, MapDataType>> _objectTypes;
	std::vector<std::pair<size_t, SpecialTileType>> _specialTypes;


	public:
		/// Creates an MCD Patch.
		MCDPatch();
		/// Cleans up the MCD Patch.
		~MCDPatch();

		/// Loads the MCD Patch from YAML.
		void load(const YAML::Node& node);

		/// Applies an MCD patch to a MapDataSet/Tileset.
		void modifyData(MapDataSet* const dataSet) const;
};

}

#endif
