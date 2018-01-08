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

#ifndef OPENXCOM_INTERCEPTSTATE_H
#define OPENXCOM_INTERCEPTSTATE_H

//#include <vector>

#include "../Engine/State.h"


namespace OpenXcom
{

class Base;
class Craft;
class GeoscapeState;
class Target;
class Text;
class TextButton;
class TextList;
class Window;


/**
 * Interception window that lets player launch Crafts on the Geoscape.
 */
class InterceptState
	:
		public State
{

private:
	static const Uint8
		OLIVE	=  48u,
		GREEN	= 112u,
		BROWN	= 144u,
		SLATE	= 160u,
		PURPLE	= 176u;

	Uint8 _cellColor;

	Base* _base;
	GeoscapeState* _geoState;
	Text
		* _txtBase,
		* _txtCraft,
		* _txtStatus,
		* _txtWeapons;
	TextButton
		* _btnCancel,
		* _btnBase;
	TextList* _lstCrafts;
	Window* _window;

	std::vector<std::wstring> _bases;

	std::vector<Craft*> _crafts;

	/// A more descriptive status of a Craft.
	std::wstring getAltStatus(Craft* const craft);


	public:
		/// Creates an Intercept state.
		InterceptState(
				GeoscapeState* const geoState,
				Base* const base = nullptr);
		/// Cleans up the Intercept state.
		~InterceptState();

		/// Handler for clicking the Cancel button.
		void btnCancelClick(Action* action);

		/// Handler for clicking the Go To Base button.
		void btnGotoBaseClick(Action* action);
		/// Handler for clicking the Crafts list.
		void lstCraftsClickLeft(Action* action);
		/// Handler for right clicking the Crafts list.
		void lstCraftsClickRight(Action* action);

		/// Handler for moving the mouse over a list item.
		void lstCraftsMouseOver(Action* action);
		/// Handler for moving the mouse outside the list borders.
		void lstCraftsMouseOut(Action* action);
};

}

#endif
