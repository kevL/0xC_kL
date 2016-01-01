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
 * along with OpenXcom. If not, see <http:///www.gnu.org/licenses/>.
 */

#ifndef OPENXCOM_BUILDNEWBASESTATE_H
#define OPENXCOM_BUILDNEWBASESTATE_H

#include "../Engine/State.h"


namespace OpenXcom
{

class Base;
class Globe;
class InteractiveSurface;
class Text;
class TextButton;
class Timer;
class Window;


/**
 * Screen that allows the player to place a new base on the globe.
 */
class BuildNewBaseState
	:
		public State
{

private:
	bool
		_firstBase,
		_showRadar;
	int
		_mX,
		_mY;
	double
		_latPre,
		_lonPre;

	Base* _base;
	Globe* _globe;
	Text* _txtTitle;
	TextButton* _btnCancel;
	Timer* _hoverTimer;
	Window* _window;
//	InteractiveSurface* _btnRotateLeft, * _btnRotateRight, * _btnRotateUp, * _btnRotateDown, * _btnZoomIn, * _btnZoomOut;


	public:
		/// Creates the Build New Base state.
		BuildNewBaseState(
				Base* const base,
				Globe* const globe,
				bool firstBase = false);
		/// Cleans up the Build New Base state.
		~BuildNewBaseState();

		/// Resets globe.
		void init() override;
		/// Runs the timer.
		void think() override;

		/// Handles actions.
		void handle(Action* action) override;
		/// Handler for clicking the globe.
		void globeClick(Action* action);
		/// Handler for mouse hovering the globe.
		void globeHover(Action* action);
		/// Handler for redrawing hover (delayed)
		void hoverRedraw();

		/// Handler for pressing the Rotate Left arrow.
//		void btnRotateLeftPress(Action* action);
		/// Handler for releasing the Rotate Left arrow.
//		void btnRotateLeftRelease(Action* action);
		/// Handler for pressing the Rotate Right arrow.
//		void btnRotateRightPress(Action* action);
		/// Handler for releasing the Rotate Right arrow.
//		void btnRotateRightRelease(Action* action);
		/// Handler for pressing the Rotate Up arrow.
//		void btnRotateUpPress(Action* action);
		/// Handler for releasing the Rotate Up arrow.
//		void btnRotateUpRelease(Action* action);
		/// Handler for pressing the Rotate Down arrow.
//		void btnRotateDownPress(Action* action);
		/// Handler for releasing the Rotate Down arrow.
//		void btnRotateDownRelease(Action* action);
		/// Handler for left-clicking the Zoom In icon.
//		void btnZoomInLeftClick(Action* action);
		/// Handler for right-clicking the Zoom In icon.
//		void btnZoomInRightClick(Action* action);
		/// Handler for left-clicking the Zoom Out icon.
//		void btnZoomOutLeftClick(Action* action);
		/// Handler for right-clicking the Zoom Out icon.
//		void btnZoomOutRightClick(Action* action);

		/// Handler for clicking the Cancel button.
		void btnCancelClick(Action* action);

		/// Let the state know the window has been resized.
		void resize(
				int& dX,
				int& dY) override;
};

}

#endif
