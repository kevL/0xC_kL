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

#ifndef OPENXCOM_MANUFACTURESTATE_H
#define OPENXCOM_MANUFACTURESTATE_H

#include "../Engine/State.h"


namespace OpenXcom
{

class Base;
class BasescapeState;
class MiniBaseView;
class Text;
class TextButton;
class TextList;
class Window;


/**
 * Manufacture screen that lets the player manage the production operations of a
 * Base.
 */
class ManufactureState final
	:
		public State
{

private:
	Base* _base;
	BasescapeState* _state;
	MiniBaseView* _mini;
	Text
		* _txtAvailable,
		* _txtAllocated,
		* _txtBaseLabel,
		* _txtCost,
		* _txtEngineers,
		* _txtFunds,
		* _txtHoverBase,
		* _txtItem,
		* _txtProduced,
		* _txtSpace,
		* _txtDuration,
		* _txtTitle;
	TextButton
		* _btnProjects,
		* _btnOk;
	TextList
		* _lstManufacture,
		* _lstResources;
	Window* _window;

	std::vector<Base*>* _baseList;

	///
	void lstManufactureClick(Action* action);


	public:
		/// Creates the Manufacture state.
		ManufactureState(
				Base* const base,
				BasescapeState* const state = nullptr);
		/// Cleans up the Manufacture state.
		~ManufactureState();

		/// Updates the Manufacture list.
		void init() override;

		/// Handler for clicking the OK button.
		void btnOkClick(Action* action);
		/// Handler for the New Manufacture button.
		void btnManufactureClick(Action* action);

		/// Handler for clicking the MiniBase view.
		void miniClick(Action* action);
		/// Handler for hovering the MiniBase view.
		void miniMouseOver(Action* action);
		/// Handler for hovering out of the MiniBase view.
		void miniMouseOut(Action* action);
};

}

#endif
