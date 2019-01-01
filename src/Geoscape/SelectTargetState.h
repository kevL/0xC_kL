/*
 * Copyright 2010-2019 OpenXcom Developers.
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

#ifndef OPENXCOM_SELECTTARGETSTATE_H
#define OPENXCOM_SELECTTARGETSTATE_H

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
class SelectTargetState
	:
		public State
{

private:
	static const int
		BUTTON_HEIGHT	= 16,
		MARGIN			= 10,
		SPACING			= 4;

	Craft* _craft;
	GeoscapeState* _geoState;

	TextButton* _btnCancel;
	Window* _window;

	std::vector<Target*> _targets;
	std::vector<TextButton*> _btnTargets;


	public:
		/// Creates a SelectTarget state.
		SelectTargetState(
				std::vector<Target*> targets,
				Craft* const craft,
				GeoscapeState* const geoState = nullptr);
		/// Cleans up the SelectTarget state.
		~SelectTargetState();

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
