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

#ifndef OPENXCOM_UNITSPRITE_H
#define OPENXCOM_UNITSPRITE_H

#include "../Engine/Surface.h"


namespace OpenXcom
{

class BattleItem;
class BattleUnit;
class SurfaceSet;


/**
 * A class that renders an individual BattleUnit given its render rules
 * combining the right frames from its SurfaceSet.
 */
class UnitSprite final
	:
		public Surface
{

private:
	static const int OFFSET = 16;	// to account for the fact that unit-sprites
									// are created double-width.
	int
		_aniCycle,
		_colorSize,
		_drawRoutine,
		_quad;

	const BattleItem
		* _itRT,
		* _itLT;
	BattleUnit* _unit;
	SurfaceSet
		* _itSetRT,
		* _itSetLT,
		* _unitSet;

	const std::pair<Uint8, Uint8>* _color;

	/// Drawing routine for soldiers, sectoids, and non-stock civilians (all
	/// routine 0), or mutons (subroutine 10).
	void drawRoutine0();
	/// Drawing routine for floaters and waspites.
	void drawRoutine1();
	/// Drawing routine for xCom tanks.
	void drawRoutine2();
	/// Drawing routine for cyberdiscs.
	void drawRoutine3();
	/// Drawing routine for stock civilians, ethereals, zombies, dogs,
	/// cybermites, and scout-drones.
	void drawRoutine4();
	/// Drawing routine for sectopods and reapers.
	void drawRoutine5();
	/// Drawing routine for snakemans.
	void drawRoutine6();
	/// Drawing routine for chryssalid.
	void drawRoutine7();
	/// Drawing routine for silacoids.
	void drawRoutine8();
	/// Drawing routine for celatids.
	void drawRoutine9();

	/// Sorts two-handed sprites out.
	void sortHandObjects();
	/// Draws surface with changed colors.
	void drawRecolored(Surface* const src);


	public:
		/// Creates a UnitSprite of the specified size.
		UnitSprite(
				int width,
				int height);
		/// Cleans up the UnitSprite.
		~UnitSprite();

		/// Sets the SurfaceSets for rendering.
		void setSurfaces(
				SurfaceSet* const unitSet,
				SurfaceSet* const itSetRT,
				SurfaceSet* const itSetLT);
		/// Sets the BattleUnit to be rendered.
		void setBattleUnit(
				BattleUnit* const unit,
				size_t quadrant);
		/// Sets the right-hand BattleItem to be rendered.
		void setBattleItRH(const BattleItem* const item = nullptr);
		/// Sets the left-hand BattleItem to be rendered.
		void setBattleItLH(const BattleItem* const item = nullptr);

		/// Sets the unit's animation state.
		void setSpriteCycle(int cycle);

		/// Draws the BattleUnit.
		void draw() override;
};

}

#endif
