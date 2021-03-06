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

#ifndef OPENXCOM_BUILDFACILITIESSTATE_H
#define OPENXCOM_BUILDFACILITIESSTATE_H

//#include <vector>

#include "../Engine/State.h"


namespace OpenXcom
{

class Base;
class RuleBaseFacility;
class Text;
class TextButton;
class TextList;
class Window;


/**
 * Window shown with all the facilities available to build.
 */
class BuildFacilitiesState
	:
		public State
{

protected:
	static const Uint8 YELLOW = 213u;

	Base* _base;
	State* _state;
	std::vector<RuleBaseFacility*> _facilities;

	Text* _txtTitle;
	TextButton* _btnOk;
	TextList* _lstFacilities;
	Window* _window;


	public:
		/// Creates a BuildFacilities state.
		BuildFacilitiesState(
				Base* const base,
				State* const state);
		/// Cleans up the BuildFacilities state.
		~BuildFacilitiesState();

		/// Populates the build-option list.
		virtual void populateBuildList();

		/// Updates the base-stats.
		void init() override;

		/// Handler for clicking the Ok button.
//		virtual void btnOkClick(Action* action);
		void btnOkClick(Action* action);
		/// Handler for clicking the Facilities list.
		virtual void lstFacilitiesClick(Action* action);
};

}

#endif
