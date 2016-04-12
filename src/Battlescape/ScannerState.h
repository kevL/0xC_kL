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

#ifndef OPENXCOM_SCANNERSTATE_H
#define OPENXCOM_SCANNERSTATE_H

#include "../Engine/State.h"


namespace OpenXcom
{

class BattleUnit;
class ScannerView;
class Surface;
class Timer;


/**
 * The Scanner User Interface.
 */
class ScannerState
	:
		public State
{

private:
	ScannerView* _scanView;

	InteractiveSurface* _bg;
	Surface* _scan;
	Timer* _timer;

	/// Updates the Scanner interface.
//	void update();
	/// Handles the radar-blob animations.
	void animate();

	/// Handler for exiting the state.
	void exitClick(Action* action = nullptr);


	public:
		/// Creates a ScannerState.
		explicit ScannerState(const BattleUnit* const selUnit);
		/// dTor.
		~ScannerState();

		/// Handler for right-clicking anything.
		void handle(Action* action) override;

		/// Handles the Timer.
		void think() override;
};

}

#endif
