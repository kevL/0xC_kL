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

#ifndef OPENXCOM_ALIENCONTAINMENTSTATE_H
#define OPENXCOM_ALIENCONTAINMENTSTATE_H

//#include <string>
//#include <vector>

#include "../Engine/State.h"

#include "../Menu/OptionsBaseState.h"


namespace OpenXcom
{

class Base;
class BasescapeState;
class MiniBaseView;

class Text;
class TextButton;
class TextList;
class Timer;
class Window;


/**
 * AlienContainment screen that lets the player manage aLiens at a particular
 * Base.
 */
class AlienContainmentState final
	:
		public State
{

private:
	static const Uint8 YELLOW = 213u;

	int
		_fishFood,
		_totalSpace,
		_usedSpace;
	size_t _sel;

	OptionsOrigin _origin;

	Base* _base;
	BasescapeState* _baseState;
	MiniBaseView* _mini;
	Text
		* _txtBaseLabel,
		* _txtDeadAliens,
		* _txtHoverBase,
		* _txtResearch,
		* _txtInResearch,
		* _txtItem,
		* _txtLiveAliens,
		* _txtSpace,
		* _txtTitle;
	TextButton
		* _btnCancel,
		* _btnOk;
	TextList* _lstAliens;
	Timer
		* _timerLeft,
		* _timerRight;
	Window* _window;

	std::vector<int> _qtysCorpsify;
	std::vector<std::string> _aliens;

	std::vector<Base*>* _baseList;

	/// Gets quantity of the selected alien at the Base.
	int getBaseQuantity();
	/// Updates the list-row for the selected alien.
	void updateListrow();


	public:
		/// Creates an AlienContainment state.
		AlienContainmentState(
				Base* const base,
				OptionsOrigin origin,
				BasescapeState* const state = nullptr);
		/// Cleans up the AlienContainment state.
		~AlienContainmentState();

		/// Updates the list of Live aLiens.
		void init() override;

		/// Handler for clicking the Ok button.
		void btnOkClick(Action* action);
		/// Handler for clicking the Cancel button.
		void btnCancelClick(Action* action);

		/// Handler for clicking a decrease-arrow in the list.
		void lstLeftArrowPress(Action* action);
		/// Handler for releasing a decrease-arrow in the list.
		void lstLeftArrowRelease(Action* action);
		/// Handler for pressing an increase-arrow in the list.
		void lstRightArrowPress(Action* action);
		/// Handler for releasing an increase-arrow in the list.
		void lstRightArrowRelease(Action* action);

		/// Runs the Timers.
		void think() override;

		/// Decreases the quantity of an aLien by one.
		void onLeft();
		/// Decreases the quantity of an aLien by a given value.
		void leftByValue(int delta);
		/// Increases the quantity of an aLien by one.
		void onRight();
		/// Increases the quantity of an aLien by a given value.
		void rightByValue(int delta);

		/// Handler for clicking the MiniBase view.
		void miniClick(Action* action);
		/// Handler for hovering the MiniBase view.
		void miniMouseOver(Action* action);
		/// Handler for hovering out of the MiniBase view.
		void miniMouseOut(Action* action);
};

}

#endif
