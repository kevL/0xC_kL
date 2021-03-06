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

#ifndef OPENXCOM_CEREMONYSTATE_H
#define OPENXCOM_CEREMONYSTATE_H

#include "../Engine/State.h"

#include <map>


namespace OpenXcom
{

class Soldier;
class Text;
class TextButton;
class TextList;
class Window;


/**
 * Screen that displays new soldier Awards.
 */
class CeremonyState
	:
		public State
{

private:
	static const Uint8
		CYAN	= 133u,
		OLIVE	= 138u,
		SLATE	= 160u,
		GREEN	= 239u;

	Text
		* _txtMedalInfo,
		* _txtTitle;
	TextButton* _btnOk;
	TextList* _lstSoldiers;
	Window* _window;

	std::map<size_t, std::string> _titleRows; // for mouseOver info.


	public:
		/// Creates a Ceremony state.
		explicit CeremonyState(std::vector<Soldier*> soldiers);
		/// Cleans up the Ceremony state.
		~CeremonyState();

		/// Handler for clicking the Ok button.
		void btnOkClick(Action* action);

		/// Handler for moving the mouse over an Award title.
		void lstInfoMouseOver(Action* action);
		/// Handler for moving the mouse outside the Awards list.
		void lstInfoMouseOut(Action* action);
};

}

#endif
