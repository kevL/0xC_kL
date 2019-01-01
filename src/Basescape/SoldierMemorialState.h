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

#ifndef OPENXCOM_SOLDIERMEMORIALSTATE_H
#define OPENXCOM_SOLDIERMEMORIALSTATE_H

#include "../Engine/State.h"


namespace OpenXcom
{

class Text;
class TextButton;
class TextList;
class Window;


/**
 * Screen that shows all the Soldiers that have died.
 */
class SoldierMemorialState
	:
		public State
{

private:
	Text
		* _txtDate,
		* _txtLost,
		* _txtName,
		* _txtRank,
		* _txtRecruited,
		* _txtTitle;
	TextButton
		* _btnOk,
		* _btnStatistics;
	TextList* _lstSoldiers;
	Window* _window;


	public:
		/// Creates a SoldierMemorial state.
		SoldierMemorialState();
		/// Cleans up the SoldierMemorial state.
		~SoldierMemorialState();

		/// Handler for clicking the Ok button.
		void btnOkClick(Action* action);
		/// Handler for clicking the Statistics button.
		void btnStatsClick(Action* action);
		/// Handler for clicking the SoldiersDead list.
		void lstSoldiersPress(Action* action);
};

}

#endif
