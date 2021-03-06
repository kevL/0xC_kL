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

#ifndef OPENXCOM_MAPDATA_H
#define OPENXCOM_MAPDATA_H

#include "RuleItem.h"


namespace OpenXcom
{

enum MoveType
{
	MT_WALK,	// 0
	MT_SLIDE,	// 1
	MT_FLY,		// 2
};

enum VoxelType
{
	VOXEL_EMPTY = -1,	// -1
	VOXEL_FLOOR,		//  0
	VOXEL_WESTWALL,		//  1
	VOXEL_NORTHWALL,	//  2
	VOXEL_OBJECT,		//  3
	VOXEL_UNIT,			//  4
	VOXEL_OUTOFBOUNDS,	//  5
	TRJ_STANDARD,		//  6 - special case for TileEngine::calcFovTiles() only.
	TRJ_DECREASE		//  7 - special case for TileEngine::calcFovTiles() only.
};

enum MapDataType
{
	O_NULPART = -1,	// -1
	O_FLOOR,		//  0
	O_WESTWALL,		//  1
	O_NORTHWALL,	//  2
	O_CONTENT,		//  3
};

/*
enum BigwallType
{
	BIGWALL_NONE,	// 0
	BIGWALL_BLOCK,	// 1
	BIGWALL_NESW,	// 2
	BIGWALL_NWSE,	// 3
	BIGWALL_WEST,	// 4
	BIGWALL_NORTH,	// 5
	BIGWALL_EAST,	// 6
	BIGWALL_SOUTH,	// 7
	BIGWALL_E_S		// 8
//	BIGWALL_W_N		// 9 NOT USED in stock UFO.
}; */
enum BigwallType
{
	BIGWALL_NONE  = 0x00,	//   0 - 0000 0000
	BIGWALL_BLOCK = 0x01,	//   1 - 0000 0001
	BIGWALL_NESW  = 0x02,	//   2 - 0000 0010
	BIGWALL_NWSE  = 0x04,	//   4 - 0000 0100
	BIGWALL_WEST  = 0x08,	//   8 - 0000 1000
	BIGWALL_NORTH = 0x10,	//  16 - 0001 0000
	BIGWALL_EAST  = 0x20,	//  32 - 0010 0000
	BIGWALL_SOUTH = 0x40,	//  64 - 0100 0000
	BIGWALL_E_S   = 0x80	// 128 - 1000 0000
};

//enum TerrainHeight
//{
//	TH_FLOOR,
//	TH_THIRD,
//	TH_HALF,
//	TH_TWOTHIRD,
//	TH_CEIL
//};

class MapDataSet;


/**
 * MapData is the smallest piece of a battlefield, holding info about a floor,
 * wall, or object.
 * @note THIS IS A TILEPART. A better ident for this class would be 'TileData'
 * or even 'PartData'. Or just 'TilePart'. -> TilepartData
 * @sa MapDataSet.
 */
class MapData
{

private:
	const unsigned int BT_LIGHT = 0u; // block types ->
	const unsigned int BT_LOS   = 1u;
	const unsigned int BT_HE    = 2u;
	const unsigned int BT_SMOKE = 3u;
	const unsigned int BT_FIRE  = 4u;
	const unsigned int BT_GAS   = 5u;

	bool
		_baseObject,
		_disallowFire,
		_disallowSmoke,
		_door,
		_gravLift,
		_hingeDoor,
		_noFloor,
		_slideDoor,
		_stopLos;
	int
		_altId,
		_armor,
		_blocks[6u],
		_dieId,
		_explosive,
		_flammable,
		_fuel,
		_level,
		_light,
		_miniId,
		_psychedelic,
		_sound,
		_sprites[8u],
		_tuWalk,
		_tuSlide,
		_tuFly,
		_yOffset;

	size_t _loftIds[12u];

	MapDataSet* _dataSet;

	BigwallType     _bigWall;
	DamageType      _explosiveType;
	MapDataType     _partType;
	TilepartSpecial _specialType;


	public:
//		static const int
//			TH_FLOOR    =  0,
//			TH_THIRD    =  8,
//			TH_HALF     = 12,
//			TH_TWOTHIRD = 16,
//			TH_CEIL     = 24;

		static const int INDESTRUCTIBLE = 255;

		/// cTor.
		explicit MapData(MapDataSet* const dataSet);
		/// dTor.
		~MapData();


		/// Gets the MapDataSet the part belongs to.
		MapDataSet* getDataset() const;

		/// Gets the sprite-index for a specified frame.
		int getSprite(int aniCycle) const;
		/// Sets the sprite-index for a specified frame.
		void setSprite(
				size_t aniCycle,
				int id);

		/// Gets if this tilepart is either a normal door or a ufo-door.
		bool isDoor() const;
		/// Gets whether the part is a normal door.
		bool isHingeDoor() const;
		/// Gets whether the part is an animated ufo-door.
		bool isSlideDoor() const;
		/// Gets whether the part stops LoS.
		bool stopsLos() const;
		/// Gets whether the part is considered a solid floor.
		bool isNoFloor() const;
		/// Gets whether the part is a BigWall.
		BigwallType getBigwall() const;
		/// Gets whether the part is a grav-lift.
		bool isGravLift() const;
		/// Gets whether the part blocks smoke.
		bool blocksSmoke() const;
		/// Gets whether the part blocks fire.
		bool blocksFire() const;

		/// Sets a whack of flags.
		void setFlags(
				bool slideDoor,
				bool stopLOS,
				bool noFloor,
				int  bigWall,
				bool gravLift,
				bool hingeDoor,
				bool disallowFire,
				bool disallowSmoke,
				bool baseObject);

		/// Gets the amount of blockage of a certain type.
		int getBlock(DamageType dType) const;
		/// Sets the amount of blockage for a bunch of types.
		void setBlocks(
				int light,
				int vision,
				int he,
				int smoke,
				int fire,
				int gas);
		/// Sets default blockage values if none are assigned in an MCD.
		void setDefaultBlocks(int armor);
		/// Sets whether the part stops LoS.
		void setStopLos(bool stopLos = true);
		/// Sets the amount of HE blockage.
		void setHeBlock(int he);

		/// Gets the offset on the y-axis when drawing the part.
		int getOffsetY() const;
		/// Sets the offset on the y-axis for drawing the part.
		void setOffsetY(int offset);

		/// Gets the type of the part.
		MapDataType getPartType() const;
		/// Sets the type of the part.
		void setPartType(MapDataType type);

		/// Gets a TilepartSpecial for the part.
		TilepartSpecial getSpecialType() const;
		/// Sets a TilepartSpecial for the part.
		void setSpecialType(TilepartSpecial type);

		/// Gets the TU-cost to move over the part.
		int getTuCostPart(MoveType type) const;
		/// Sets the TU-cost to move over the part.
		void setTuCosts(
				int walk,
				int slide,
				int fly);

		/// Gets the y-offset for units/items on the part.
		int getTerrainLevel() const;
		/// Sets the y-offset for units/items on the part.
		void setTerrainLevel(int offset);

		/// Gets the index of the footstep-sound.
		int getFootstepSound() const;
		/// Sets the index of the footstep-sound.
		void setFootstepSound(int sound);

		/// Gets the alternate object-ID.
		int getAltPart() const;
		/// Sets the alternate object-ID.
		void setAltPart(int id);
		/// Gets the dead-object-ID.
		int getDiePart() const;
		/// Sets the dead-object-ID.
		void setDiePart(int id);

		/// Gets the amount of light the part emits.
		int getLightSource() const;
		/// Sets the amount of light the part emits.
		void setLightSource(int light);

		/// Gets the amount of armor.
		int getArmorPoints() const;
		/// Sets the amount of armor.
		void setArmorPoints(int armor);

		/// Gets the amount of flammable.
		int getFlammable() const;
		/// Sets the amount of flammable.
		void setFlammable(int flammable);
		/// Gets the amount of fuel.
		int getFuel() const;
		/// Sets the amount of fuel.
		void setFuel(int fuel);

		/// Gets the LoFT-index of a specified layer.
		size_t getLoftId(size_t layer) const;
		/// Sets the LoFT-index of a specified layer.
		void setLoftId(
				size_t loft,
				size_t layer);

		/// Gets the amount of explosive.
		int getExplosive() const;
		/// Sets the amount of explosive.
		void setExplosive(int value);
		/// Gets the type of explosive.
		DamageType getExplosiveType() const;
		/// Sets the type of explosive.
		void setExplosiveType(int type);

		/// Sets the MiniMap index.
		void setMiniMapIndex(unsigned short id);
		/// Gets the MiniMap index.
		int getMiniMapIndex() const;

		/// Sets the BigWall value.
		void setBigWall(int bigWall);

		/// Sets the TU-walk value.
		void setTuWalk(int tu);
		/// Sets the TU-slide value.
		void setTuSlide(int tu);
		/// Sets the TU-fly value.
		void setTuFly(int tu);

		/// Sets the part as not-a-floor.
		void setNoFloor(bool noFloor);

		/// Checks if the part is an aLien-objective tilepart.
		bool isBaseObject() const;

		/// Sets if the tilepart is psychedelic.
		void setPsychedelic(int psycho);
		/// Gets if the tilepart is psychedelic.
		int getPsychedelic() const;

		/**
		 * Converts a VoxelType into a string for readable debug-logs.
		 */
		static std::string debugVoxelType(VoxelType type)
		{
			switch (type)
			{
				case VOXEL_EMPTY:       return "-1 VOXEL_EMPTY";
				case VOXEL_FLOOR:       return  "0 VOXEL_FLOOR";
				case VOXEL_WESTWALL:    return  "1 VOXEL_WESTWALL";
				case VOXEL_NORTHWALL:   return  "2 VOXEL_NORTHWALL";
				case VOXEL_OBJECT:      return  "3 VOXEL_OBJECT";
				case VOXEL_UNIT:        return  "4 VOXEL_UNIT";
				case VOXEL_OUTOFBOUNDS: return  "5 VOXEL_OUTOFBOUNDS";
			}
			return "ERROR: no VoxelType";
		}
};

}

#endif
