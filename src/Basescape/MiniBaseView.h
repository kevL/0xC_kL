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

#ifndef OPENXCOM_MINIBASEVIEW_H
#define OPENXCOM_MINIBASEVIEW_H

//#include <vector>

#include "../Engine/InteractiveSurface.h"


namespace OpenXcom
{

class Base;
class SurfaceSet;
class Timer;


enum MiniBaseViewType
{
	MBV_STANDARD,	// 0
	MBV_RESEARCH,	// 1
	MBV_PRODUCTION,	// 2
	MBV_INFO,		// 3
	MBV_CONTAINMENT	// 4
};


/**
 * MiniView that shows all of the player's Bases.
 * @note Takes all the bases and displays their layouts and allows the player to
 * switch between them.
 */
class MiniBaseView final
	:
		public InteractiveSurface
{

private:
	static const int MINI_SIZE = 14;
	static const Uint8
		WHITE		=   1u,
		RED_L		=  33u,
		RED_D		=  37u,
		GREEN		=  50u,
		LAVENDER_L	=  69u,
		LAVENDER_D	=  72u,
		ORANGE_L	=  96u,
		ORANGE_D	=  98u,
		BLUE		= 130u,
		YELLOW_L	= 145u,
		YELLOW_D	= 153u,
		BROWN		= 161u;

	bool _blink;
	size_t
		_baseId,
		_hoverBase;

	SurfaceSet* _texture;
	Timer* _timer;

	std::vector<Base*>* _baseList;

	MiniBaseViewType _mode;


	public:
		/// Creates a MiniBaseView at the specified position and size.
		MiniBaseView(
				int width,
				int height,
				int x = 0,
				int y = 0,
				MiniBaseViewType mode = MBV_STANDARD);
		/// Cleans up the MiniBaseView.
		~MiniBaseView();

		/// Sets the base-list to display.
		void setBases(std::vector<Base*>* bases);

		/// Sets the texture-set for the MiniBaseView.
		void setTexture(SurfaceSet* texture);

		/// Gets the Base the mouse is over.
		size_t getHoveredBase() const;

		/// Sets the selected Base for the MiniBaseView.
		void setSelectedBase(size_t baseId);

		/// Draws the MiniBaseView.
		void draw() override;

		/// Special handling for mouse-overs.
		void mouseOver(Action* action, State* state) override;
		/// Special handling for mouse-outs.
		void mouseOut(Action* action, State* state) override;

		/// Handles the Timer.
		void think() override;
		/// Blinks the craft-status indicators.
		void blink();
};

}

#endif
