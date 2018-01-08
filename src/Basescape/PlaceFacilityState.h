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
 * along with OpenXcom. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef OPENXCOM_PLACEFACILITYSTATE_H
#define OPENXCOM_PLACEFACILITYSTATE_H

#include "../Engine/State.h"


namespace OpenXcom
{

class Base;
class BaseView;
class RuleBaseFacility;
class Text;
class TextButton;
class Window;


/**
 * Window shown when the player wants to build a Facility.
 */
class PlaceFacilityState
	:
		public State
{

protected:
	int64_t _cost;

	Base* _base;
	BaseView* _baseLayout;
	const RuleBaseFacility* _facRule;
	Text
		* _txtFacility,
		* _txtCost,
		* _txtCostAmount,
		* _txtTime,
		* _txtTimeAmount,
		* _txtMaintenance,
		* _txtMaintAmount;
	TextButton* _btnCancel;
	Window* _window;


	public:
		/// Creates a PlaceFacility state.
		PlaceFacilityState(
				Base* const base,
				const RuleBaseFacility* const facRule);
		/// Cleans up the PlaceFacility state.
		~PlaceFacilityState();

		/// Handler for clicking the Cancel button.
		void btnCancelClick(Action* action);
		/// Handler for clicking the BaseView grid.
		virtual void baseLayoutClick(Action* action);
};

}

#endif
