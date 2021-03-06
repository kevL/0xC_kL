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

#ifndef OPENXCOM_MANUFACTURESTARTSTATE_H
#define OPENXCOM_MANUFACTURESTARTSTATE_H

#include "../Engine/State.h"


namespace OpenXcom
{

class Base;
class RuleManufacture;
class Text;
class TextButton;
class TextList;
class Window;


/**
 * Screen which displays needed elements to start Manufacture.
 */
class ManufactureStartState
	:
		public State
{

private:
	bool _init;

	Base* _base;
	const RuleManufacture* const _mfRule;
	Text
		* _txtCost,
		* _txtItemRequired,
		* _txtManHour,
		* _txtRequiredItems,
		* _txtTitle,
		* _txtWorkSpace,
		* _txtUnitsAvailable,
		* _txtUnitsRequired;
	TextButton
		* _btnCancel,
		* _btnCostTable,
		* _btnStart;
	TextList* _lstRequiredItems;
	Window* _window;


	public:
		/// Creates a ManufactureStart state.
		ManufactureStartState(
				Base* const base,
				const RuleManufacture* const mfRule);
		/// dTor.
		~ManufactureStartState();

		/// Initializes the State.
		void init() override;

		/// Handler for the Costs button.
		void btnCostsClick(Action* action);
		/// Handler for the Cancel button.
		void btnCancelClick(Action* action);
		/// Handler for the Start button.
		void btnStartClick(Action* action);
};

}

#endif
