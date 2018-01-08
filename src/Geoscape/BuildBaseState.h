/*
 * Copyright 2010-2018 OpenXcom Developers.
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

#ifndef OPENXCOM_BUILDBASESTATE_H
#define OPENXCOM_BUILDBASESTATE_H

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
 * Screen that allows the player to place a Base on the Globe.
 */
class BuildBaseState
	:
		public State
{

private:
	bool _isFirstBase;
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

	/// Handler for redrawing hover (delayed).
	void hoverRedraw();


	public:
		/// Creates a BuildBase state.
		BuildBaseState(
				Base* const base,
				Globe* const globe,
				bool isFirstBase = false);
		/// Cleans up the BuildBase state.
		~BuildBaseState();

		/// Resets Globe.
		void init() override;
		/// Runs the timer.
		void think() override;

		/// Handles actions.
		void handle(Action* action) override;
		/// Handler for clicking the Globe.
		void globeClick(Action* action);
		/// Handler for mouse hovering the Globe.
		void globeHover(Action* action);

		/// Handler for clicking the cancel-button.
		void btnCancelClick(Action* action);

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

		/// Lets the State know the window has been resized.
		void resize(
				int& dX,
				int& dY) override;
};

}

#endif
