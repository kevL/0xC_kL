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

#ifndef OPENXCOM_CANNOTREEQUIPSTATE_H
#define OPENXCOM_CANNOTREEQUIPSTATE_H

#include <vector>

#include "DebriefingState.h"

#include "../Engine/State.h"


namespace OpenXcom
{

class Text;
class TextButton;
class TextList;
class Window;


/**
 * Screen shown when there's not enough equipment to re-equip a craft after a
 * mission.
 */
class CannotReequipState
	:
		public State
{

private:
	Text
		* _txtTitle,
		* _txtItem,
		* _txtQty,
		* _txtCraft;
	TextButton* _btnOk;
	TextList* _lstItems;
	Window* _window;


	public:
		/// Creates a CannotReequip state.
		explicit CannotReequipState(std::vector<UnreplacedStat> its);
		/// Cleans up the Cannot Reequip state.
		~CannotReequipState();

		/// Handler for clicking the Ok button.
		void btnOkClick(Action* action);
};

}

#endif
