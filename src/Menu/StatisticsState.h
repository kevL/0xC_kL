/*
 * Copyright 2010-2017 OpenXcom Developers.
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

#ifndef OPENXCOM_STATISTICSSTATE_H
#define OPENXCOM_STATISTICSSTATE_H

#include "../Engine/State.h"

#include "../Savegame/SavedGame.h"


namespace OpenXcom
{

class Text;
class TextButton;
class TextList;
class Window;


/**
 * Statistics window that shows the Player his/her current game-statistics.
 */
class StatisticsState
	:
		public State
{

private:
	EndType _endType;

	Text* _txtTitle;
	TextButton* _btnOk;
	TextList* _lstStats;
	Window* _window;

	/// Totals a vector of integers or floating-points.
	template <typename T>
	T total(const std::vector<T>& vect) const;


	public:
		/// Creates a StatisticsState.
		StatisticsState();
		/// Cleans up the StatisticsState.
		~StatisticsState();

		/// Displays the statistics.
		void listStats();

		/// Handler for clicking the Ok button.
		void btnOkClick(Action* action);
};

}

#endif
