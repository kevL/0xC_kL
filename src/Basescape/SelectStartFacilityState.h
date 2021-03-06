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

#ifndef OPENXCOM_SELECTSTARTFACILITYSTATE_H
#define OPENXCOM_SELECTSTARTFACILITYSTATE_H

#include "BuildFacilitiesState.h"


namespace OpenXcom
{

class Globe;


/**
 * Window shown with all the start-facilities available to build.
 */
class SelectStartFacilityState final
	:
		public BuildFacilitiesState
{

private:
	Globe* _globe;


	public:
		/// Creates a SelectStartFacility state.
		SelectStartFacilityState(
				Base* const base,
				State* const state,
				Globe* const globe);
		/// Cleans up the SelectStartFacility state.
		~SelectStartFacilityState();

		/// Populates the build-option list.
		void populateBuildList() override;

		/// Handler for clicking the Reset button.
//		void btnOkClick(Action* action) override;
		void btnResetClick(Action* action);
		/// Handler for clicking the Facilities list.
		void lstFacilitiesClick(Action* action) override;

		/// Handler for when the Facility is actually built.
		void facilityBuilt();
};

}

#endif
