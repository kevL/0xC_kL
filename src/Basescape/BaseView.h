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

#ifndef OPENXCOM_BASEVIEW_H
#define OPENXCOM_BASEVIEW_H

#include "../Engine/InteractiveSurface.h"

#include "../Savegame/Base.h"


namespace OpenXcom
{

class Base;
class BaseFacility;
class Font;
class Language;
class RuleBaseFacility;
class Surface;
class SurfaceSet;
class Timer;


/**
 * Interactive view of a player's Base.
 * @note Takes a Base and displays all its facilities and stuff.
 */
class BaseView
	:
		public InteractiveSurface
{

private:
	static const int GRID_SIZE = 32;

	bool _blink;
	int
		_gridX,
		_gridY;
	size_t _selSize;
	Uint8
		_cellColor,
		_selectorColor;

	Base* _base;
	BaseFacility
		* _facilities[Base::BASE_SIZE]
					 [Base::BASE_SIZE],
		* _selFacility;
	Font
		* _big,
		* _small;
	const Language* _lang;
	Surface
		* _srfDog,
		* _selector;
	SurfaceSet* _texture;
	Timer* _timer;

	/// Updates a neighboring BaseFacility's build-time.
	void updateNeighborFacilityBuildTime(
			const BaseFacility* const facility,
			BaseFacility* const neighbor);


	public:
		/// Creates a BaseView at the specified position and size.
		BaseView(
				int width,
				int height,
				int x = 0,
				int y = 0);
		/// Cleans up the BaseView.
		~BaseView();

		/// Initializes the BaseView's various resources.
		void initText(
				Font* const big,
				Font* const small,
				const Language* const lang) override;

		/// Sets the base to display.
		void setBase(Base* const base);

		/// Sets the sprites for this BaseView.
		void setTexture(SurfaceSet* const texture);
		/// Sets the dog for this BaseView.
		void setDog(Surface* const dog);

		/// Gets the currently selected facility.
		BaseFacility* getSelectedFacility() const;
		/// Prevents any mouseover bugs on dismantling base facilities before setBase has had time to update the base.
		void resetSelectedFacility();

		/// Gets the x-position of the currently selected square.
		int getGridX() const;
		/// Gets the y-position of the currently selected square.
		int getGridY() const;

		/// Sets whether the BaseView is selectable.
		void setSelectable(size_t facSize);

		/// Checks if a facility can be placed.
		bool isPlaceable(const RuleBaseFacility* const facRule) const;
		/// Checks if the placed facility is placed in queue or not.
		bool isQueuedBuilding(const RuleBaseFacility* const facRule) const;
		/// ReCalculates the remaining build-time of all queued buildings.
		void reCalcQueuedBuildings();

		/// Handles the timers.
		void think() override;
		/// Blinks the selector.
		void blink();
		/// Draws the BaseView.
		void draw() override;
		/// Blits the BaseView onto another Surface.
		void blit(const Surface* const srf) override;

		/// Special handling for mouse hovers.
		void mouseOver(Action* action, State* state) override;
		/// Special handling for mouse hovering out.
		void mouseOut(Action* action, State* state) override;

		/// Sets the primary color.
		void setColor(Uint8 color) override;
		/// Sets the secondary color.
		void setSecondaryColor(Uint8 color) override;
};

}

#endif
