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

#ifndef OPENXCOM_MULTIPLETARGETSSTATE_H
#define OPENXCOM_MULTIPLETARGETSSTATE_H

//#include <vector>

#include "../Engine/State.h"


namespace OpenXcom
{

class Craft;
class GeoscapeState;
class Target;
class TextButton;
class Window;


/**
 * Displays a list of possible targets.
 */
class MultipleTargetsState
	:
		public State
{

private:
	static const int
		BUTTON_HEIGHT	= 16,
		MARGIN			= 10,
		SPACING			= 4;

	Craft* _craft;
	GeoscapeState* _state;

	TextButton* _btnCancel;
	Window* _window;

	std::vector<Target*> _targets;
	std::vector<TextButton*> _btnTargets;


	public:
		/// Creates the Multiple Targets state.
		MultipleTargetsState(
				std::vector<Target*> targets,
				Craft* const craft,
				GeoscapeState* const state);
		/// Cleans up the Multiple Targets state.
		~MultipleTargetsState();

		/// Updates the window.
		void init() override;

		/// Popup for a target.
		void popupTarget(Target* const target);

		/// Handler for clicking the Cancel button.
		void btnCancelClick(Action* action);
		/// Handler for clicking the Targets list.
		void btnTargetClick(Action* action);
};

}

#endif
