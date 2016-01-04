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

#ifndef OPENXCOM_UNITSPRITE_H
#define OPENXCOM_UNITSPRITE_H

#include "../Engine/Surface.h"


namespace OpenXcom
{

class BattleItem;
class BattleUnit;
class SurfaceSet;


/**
 * A class that renders a specific unit given its render rules combining the
 * right frames from the SurfaceSet.
 */
class UnitSprite final
	:
		public Surface
{

private:
	int
		_aniFrame,
		_colorSize,
		_drawRoutine,
		_quad;

	BattleItem
		* _itRT,
		* _itLT;
	BattleUnit* _unit;
	SurfaceSet
		* _itSetRT,
		* _itSetLT,
		* _unitSet;

	const std::pair<Uint8, Uint8>* _color;

	/// Drawing routine for xCom soldiers in overalls, sectoids (routine 0),
	/// or mutons (routine 10).
	void drawRoutine0();
	/// Drawing routine for floaters.
	void drawRoutine1();
	/// Drawing routine for XCom tanks.
	void drawRoutine2();
	/// Drawing routine for cyberdiscs.
	void drawRoutine3();
	/// Drawing routine for civilians, ethereals, zombies.
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

	/// sort two handed sprites out.
	void sortRifles();
	/// Draw surface with changed colors.
	void drawRecolored(Surface* src);


	public:
		/// Creates a new UnitSprite at the specified position and size.
		UnitSprite(
				int width,
				int height,
				int x = 0,
				int y = 0);
		/// Cleans up the UnitSprite.
		~UnitSprite();

		/// Sets the SurfaceSets for rendering.
		void setSurfaces(
				SurfaceSet* const unitSet,
				SurfaceSet* const itSetRT,
				SurfaceSet* const itSetLT);
		/// Sets the battleunit to be rendered.
		void setBattleUnit(
				BattleUnit* const unit,
				int part = 0);
		/// Sets the right-hand BattleItem to be rendered.
		void setBattleItRH(BattleItem* const item = nullptr);
		/// Sets the left-hand BattleItem to be rendered.
		void setBattleItLH(BattleItem* const item = nullptr);
		/// Sets the animation frame.
		void setAnimationFrame(int frame);

		/// Draws the unit.
		void draw() override;
};

}

#endif
