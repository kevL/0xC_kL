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
	MBV_INFO		// 3
};


/**
 * Mini view of a base.
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
		/// Creates a new mini base view at the specified position and size.
		MiniBaseView(
				int width,
				int height,
				int x = 0,
				int y = 0,
				MiniBaseViewType mode = MBV_STANDARD);
		/// Cleans up the mini base view.
		~MiniBaseView();

		/// Sets the base list to display.
		void setBases(std::vector<Base*>* bases);

		/// Sets the texture for the mini base view.
		void setTexture(SurfaceSet* texture);

		/// Gets the base the mouse is over.
		size_t getHoveredBase() const;

		/// Sets the selected base for the mini base view.
		void setSelectedBase(size_t baseId);

		/// Draws the mini base view.
		void draw() override;

		/// Special handling for mouse hovers.
		void mouseOver(Action* action, State* state) override;
		/// Special handling for mouse hovering out.
		void mouseOut(Action* action, State* state) override;

		/// Handles timer.
		void think() override;
		/// Blinks the craft status indicators.
		void blink();
};

}

#endif
